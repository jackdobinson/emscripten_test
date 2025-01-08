#include "deconv.hpp"



// TODO:
// * [DONE] Write code to center PSF on brightest pixel/center of brightness
// * Add hooks/callbacks so I can update things after each iteration/at specific points in processing
// * Add user feedback for bad values in input parameters
// * Extend grey-out for non-usable parameters to name of parameter as well as value setter
// * Put onto proper repository site as a page.
// * Communicate how brightnesses of produed images relate to original file

using namespace emscripten;


// TODO:
// * Move into it's own file
// * Can I make this more generic, i.e. can I write to a named canvas?
// * Can I expose a set of "data sources" from CleanModifiedAlgorithm, create
//   a load of these functions as "data representers", then associate "data sources"
//   with "data representers" on the JS side? I.e. C++ give JS a handle to some raw
//   data. JS (or here) has functions that display raw data. I associate raw data
//   handles with display functions on JS side. C++ side updates raw data, JS side
//   periodically calls display functions.

EM_JS(void, update_deconv_stats, (int this_iteration, int last_plot_update_iteration), {
	// Pass -1 to 'this_iteration' and/or 'last_plot_update_iteration' to not update value
	if(this_iteration >=0) deconv_status_mgr.set("Deconvolution Iteration", this_iteration);
	if(last_plot_update_iteration >=0) deconv_status_mgr.set("Last Plot Update Iteration", last_plot_update_iteration);
})


EM_JS(void, send_to_named_js_canvas, (const char* name, void* ptr, int size, int width, int height), {
	let canvas_id = UTF8ToString(name);
	//console.log("EM_JS: Sending image to canvas ", canvas_id);
	//console.log(ptr, size, width, height);
	let named_canvas = document.getElementById(canvas_id);
	//console.log("EM_JS: named canvas", named_canvas);
	let im_data = new ImageData(new Uint8ClampedArray(Module.HEAPU8.buffer, ptr, size), width, height);
	//console.log("EM_JS: got image data");
	named_canvas.width = width;
	named_canvas.height = height;
	//console.log("EM_JS: getting canvas context");
	let named_canvas_ctx = named_canvas.getContext("2d");
	named_canvas_ctx.imageSmoothingEnabled = false;
	named_canvas_ctx.putImageData(im_data, 0, 0);
	//console.log("EM_JS: Image data written");
});


EM_JS(void, js_set_dataset_cpp_array_cache, (const char* dataset_name, const void* ptr, int capacity), {
	dataset_name = UTF8ToString(dataset_name);
	
	dataset_cpp_array_cache.set(
		dataset_name, 
		Module.HEAPF64.subarray(ptr/8, ptr/8 + capacity)
	);
});

EM_JS(void, js_plot_point_from_cpp_array_cache, (const char* plot_name, const char* dataset_name, int idx_start, int idx_end, bool clear=false), {
	plot_name = UTF8ToString(plot_name);
	dataset_name = UTF8ToString(dataset_name);
	//console.log("EM_JS: js_plot_point_from_cpp_array_cache");
	if(plot_name_map === undefined){
		console.log("EM_JS: plot_name_map is undefined, cannot update any plots");
		return;
	}
	if(!plot_name_map.has(plot_name)){
		console.log("EM_JS: plot name not found", plot_name);
		//console.log(plot_name_map);
		return;
	}
	
	let plot = plot_name_map.get(plot_name);
	if(clear){
		//console.log("clearing plot", plot_name);
		plot.clear();
	}
	
	
	let array_cache = dataset_cpp_array_cache.get(dataset_name).subarray(idx_start, idx_end);
	//console.log(idx_start, idx_end);
	//console.log(array_cache);
	
	//plot.addDataToDataset(dataset_name,array_cache);
	plot.appendToDataset(array_cache, dataset_name)
});

EM_JS(void, js_plot_set_data_from_cpp_array_cache, (const char* plot_name, const char* dataset_name, int size, int n_dimensions, bool clear=false), {
	plot_name = UTF8ToString(plot_name);
	dataset_name = UTF8ToString(dataset_name);
	//console.log("EM_JS: js_plot_set_data_from_cpp_array_cache");
	if(plot_name_map === undefined){
		console.log("EM_JS: plot_name_map is undefined, cannot update any plots");
		return;
	}
	if(!plot_name_map.has(plot_name)){
		console.log("EM_JS: plot name not found", plot_name);
		//console.log(plot_name_map);
		return;
	}
	
	let plot = plot_name_map.get(plot_name);
	if(clear){
		//console.log("clearing plot", plot_name);
		plot.clear();
	}
	
	let array_cache = dataset_cpp_array_cache.get(dataset_name).subarray(0,size);
	
	plot.setAsDataset(
		V.toBlockArray(array_cache, n_dimensions),
		dataset_name
	);
});


#include <initializer_list>

template<class T>
struct PointStorage{
	constexpr static size_t initial_capacity = 1024;

	using value_t = T;
	using callback_t = void(*)(const PointStorage<T>&, const int);
	
	size_t capacity;
	size_t m_size;
	value_t* ptr;
	bool do_fill = false;
	value_t fill_value;
	
	int userdata_id;
	callback_t after_reallocate_callback = nullptr;
	
	PointStorage(
			size_t _capacity = initial_capacity,
			int _userdata_id = -1
		) 
	: capacity(_capacity)
	, userdata_id(_userdata_id)
	{
		ptr = new value_t[capacity];
		m_size = 0;
	}
	
	PointStorage(
			std::initializer_list<value_t> init_list
		) 
	: userdata_id(-1)
	{
		capacity = initial_capacity;
		while(capacity < init_list.size()){
			capacity *= 2;
		}
		ptr = new value_t[capacity];
		m_size=0;
		for(auto& item : init_list){
			ptr[m_size] = item;
			++m_size;
		}
	}
	
	template<class ...Ts>
	void assign_vectors_zipped(Ts ...args){
		size_t n_dims = sizeof...(Ts);
		size_t new_size = (args.size() + ...);
		size_t new_capacity = capacity;
		size_t dim_size = new_size/n_dims;
		
		
		while(new_capacity < new_size){
			new_capacity *= 2;
		}
		
		reallocate(new_capacity);
		
		size_t j;
		for(size_t i=0;i<dim_size;++i){
			j = 0;
			([&]{ // apply function to each element of parameter pack
				ptr[n_dims*i + j] = args[i];
				++j;
			}(), ...);
		}
		m_size = new_size;
	}
	void set_fill(const value_t& _fill_value, bool _do_fill=true){
		fill_value = _fill_value;
		do_fill = _do_fill;
	}
	void fill(bool include_set_values = true){
		if(!do_fill){return;}
		
		size_t i = (include_set_values) ? 0 : size();
		for(; i<capacity; ++i){
			ptr[i] = fill_value;
		}
	}
	void reallocate_to_fit(size_t required_capacity){
		size_t new_capacity = capacity;
		while(new_capacity < required_capacity){
			new_capacity *= 2;
		}
		reallocate(new_capacity);
	}
	void reallocate(size_t new_capacity){
		value_t *temp = new value_t[new_capacity];
		
		std::memcpy(temp, ptr, size());
		
		delete[] ptr;
		
		ptr = temp;
		capacity = new_capacity;
		
		fill();
		
		if(after_reallocate_callback != nullptr){
			after_reallocate_callback(*this, userdata_id);
		}
	}
	void push_back(const value_t& v){
		if(size() == capacity){
			reallocate(2*capacity);
		}
		ptr[size()] = v;
		++m_size;
	}
	value_t* data() const {
		return ptr;
	}
	size_t size() const {
		return m_size;
	}
	value_t& operator[](size_t idx){
		 return ptr[idx]; 
	}
	const value_t& operator[](size_t idx) const {
		return ptr[idx]; 
	}
	value_t* begin(){
		return ptr;
	}
	value_t* end(){
		return ptr+size();
	}
};

std::map<std::string, PointStorage<double>> dataset_points;
std::map<std::string, int> userdata_ids;
std::map<int, std::string> dataset_names;

template<class T>
void update_cpp_array_cache_on_reallocate(const PointStorage<T>& points, const int userdata_id){
	const char* dataset_name = dataset_names[userdata_id].c_str();
	js_set_dataset_cpp_array_cache(dataset_name, points.data(), points.capacity); //(const char* dataset_name, void* ptr, int capacity)
}

PointStorage<double>& get_dataset_point_storage(const std::string& dataset_name){
	PointStorage<double>* points ;
	if(!dataset_points.contains(dataset_name)){
		dataset_points[dataset_name] = PointStorage<double>();
		points = &dataset_points[dataset_name];
		
		userdata_ids.try_emplace(dataset_name, dataset_names.size());
		dataset_names.try_emplace(userdata_ids[dataset_name], dataset_name);
		points->userdata_id = userdata_ids[dataset_name];
		
		points->after_reallocate_callback = update_cpp_array_cache_on_reallocate;
		update_cpp_array_cache_on_reallocate(*points, userdata_ids[dataset_name]);
	}
	return dataset_points[dataset_name];
}

void send_data_to_plot(
		const std::string plot_name, 
		const std::string dataset_name, 
		double x, 
		double y
	){
	
	PointStorage<double>& points = get_dataset_point_storage(dataset_name);
	
	points.push_back(x);
	points.push_back(y);
	
	js_plot_point_from_cpp_array_cache(plot_name.c_str(), dataset_name.c_str(), points.size()-2, points.size());
}


void send_data_to_plot(
		const std::string plot_name, 
		const std::string dataset_name, 
		const std::vector<double>& values,
		size_t start_idx,
		size_t end_idx
	){
	//GET_LOGGER;
	PointStorage<double>& points = get_dataset_point_storage(dataset_name);
	
	for(size_t i=start_idx; i<end_idx; ++i){
		//LOGV_DEBUG(i, values[i]);
		
		points.push_back(i);
		points.push_back(values[i]);
		js_plot_point_from_cpp_array_cache(plot_name.c_str(), dataset_name.c_str(), points.size()-2, points.size());
	}
	
	
}

void send_data_to_plot(
		const std::string plot_name, 
		const std::string dataset_name, 
		const std::vector<double>& x, 
		const std::vector<uint32_t>& y
	){
	
	PointStorage<double>& points = get_dataset_point_storage(dataset_name);
	
	
	points.assign_vectors_zipped(x,y);
	
	js_plot_set_data_from_cpp_array_cache(plot_name.c_str(), dataset_name.c_str(), points.size(), 2, true);
}






// TODO: 
// * rewrite this to be easier to integrate with the JS side of things
// * On the JS side, need to be able to get handles for any plots. So they need to be exposed here.
// * The C++ side needs to be able to accept values from the JS interface to set algorithm parameters.
// * Break up ".run()" method into smaller chunks. Ideally want to have a ".doIter()" method that does
//   a single iteration.
// * Should pre-allocate as much as possible. Ideally will be able to give JS side a chunk of memory
//   where in-progress plot data will be written and have it update the plots in a loop.
// * May need to run the C++ side in a web-worker.
CleanModifiedAlgorithm::CleanModifiedAlgorithm(
		size_t _n_iter,
		size_t _n_positive_iter,
		double _loop_gain,
		double _threshold,
		double _clean_beam_gaussian_sigma,
		bool _add_residual,
		double _noise_std,
		double _rms_frac_threshold,
		double _fabs_frac_threshold
	): 
		n_iter(_n_iter), 
		n_positive_iter(_n_positive_iter), 
		loop_gain(_loop_gain),
		threshold(_threshold), 
		clean_beam_gaussian_sigma(_clean_beam_gaussian_sigma), 
		add_residual(_add_residual),
		noise_std(_noise_std), 
		rms_frac_threshold(_rms_frac_threshold), 
		fabs_frac_threshold(_fabs_frac_threshold),
		plot_update_interval(0),
		data_size(0),
		data_shape(),
		residual_data(0), 
		components_data(0), 
		px_choice_map(0),
		data_shape_adjustment(0),
		padded_psf_data(0), 
		selected_pixels(0), 
		current_convolved(0),
		fabs_record(_n_iter), 
		rms_record(_n_iter),
		threshold_record(_n_iter),
		histogram_n_bins(100),
		histogram_edges(histogram_n_bins),
		histogram_counts(histogram_n_bins),
		temp_data(0),
		fft(), 
		ifft(), 
		psf_fft(), 
		selected_px_fft(), 
		tag("")
{
}




void CleanModifiedAlgorithm::_get_residual_from_obs(const std::vector<double>& obs_data, const std::vector<size_t>& obs_shape){
	GET_LOGGER;
	std::vector<bool> obs_nan_mask(obs_data.size());
	residual_data = obs_data;
	LOGV_DEBUG(obs_data.size(), obs_nan_mask.size(), residual_data.size());

	obs_nan_mask = du::mask_where(residual_data, std::function<bool(double)>(du::isnan<double>));
	du::set_at_mask(residual_data, obs_nan_mask, 0.0);
}

void CleanModifiedAlgorithm::_get_padded_psf(
		const std::vector<double>& psf_data, 
		const std::vector<size_t>& psf_shape,
		const std::string& centering_mode = "center_of_brightness"
	){
	GET_LOGGER;

	// zero the array that will hold our result
	du::multiply_inplace(padded_psf_data, 0);

	// define the first pixel that psf_data will be written from 
	std::vector<size_t> psf_fpixel(psf_shape.size(),0); // from (0,0)

	// define the first pixel that psf data will be written to
	std::vector<size_t> psf_fpixel_obs(data_shape);
	LOGV_DEBUG(du::multiply(psf_fpixel_obs, 0.5));
	LOGV_DEBUG(du::multiply(psf_shape, 0.5));
	du::subtract_inplace(du::multiply_inplace(psf_fpixel_obs, 1./2), du::multiply(psf_shape, 1./2));
	LOGV_DEBUG(data_shape);
	LOGV_DEBUG(psf_fpixel_obs);
	LOGV_DEBUG(psf_shape);
	LOGV_DEBUG(psf_fpixel);


	LOG_DEBUG("Copying PSF data to padded array");
	du::copy_to_rect(psf_data, padded_psf_data, psf_shape, data_shape, psf_fpixel, psf_fpixel_obs);
	du::write_as_image(_sprintf("./plots/%psf_padded_before.pgm", tag), padded_psf_data, data_shape);

	LOG_DEBUG("Removing NANs from padded_psf_data");
	// remove NANs from padded_psf_data
	std::vector<bool> psf_nan_mask = du::mask_where(padded_psf_data, std::function<bool(double)>(du::isnan<double>));
	du::set_at_mask(padded_psf_data, psf_nan_mask, 0.0);
	du::multiply_inplace(padded_psf_data, 1.0/du::sum(padded_psf_data));

	std::vector<int> center_offset_nd_idx(data_shape.size(), 0);
	
	if(centering_mode == ""){
		LOG_INFO("No centering of PSF will be performed.");
	}
	if(centering_mode == "center_of_brightness"){
		LOG_INFO("Centering PSF by center of brightness");
		center_offset_nd_idx = du::subtract(
			du::as_type<int>(du::ratio(data_shape, 2)),
			du::as_type<int>(du::idx_moment_1(padded_psf_data, data_shape))
		);
	}
	else if(centering_mode == "brightest_pixel"){
		LOG_INFO("Centering PSF by brightest pixel");
		size_t center_offset_1d_idx = du::idx_max(padded_psf_data);
		center_offset_nd_idx = du::subtract(
			du::as_type<int>(du::ratio(data_shape, 2)),
			du::as_type<int>(du::index_1d_to_nd(data_shape, center_offset_1d_idx))
		);
	}
	else {
		LOG_ERROR("");
	}
	LOGV_DEBUG(center_offset_nd_idx);

	// If there is no shift from the center, don't bother
	if (!du::is_identical(center_offset_nd_idx, du::zeros<int>(center_offset_nd_idx.size()))){
		du::shift_inplace(
			padded_psf_data, 
			data_shape, 
			center_offset_nd_idx
		);
	}

	LOG_DEBUG("Adjusted padded_psf_data for convolution centering");
	// Re-center the padded_psf_data so that the convolution in "run()" 
	// is performed in the correct way.
	std::vector<double> delta_f(psf_fft.size(), 0);

	LOGV_DEBUG(padded_psf_data.size());
	LOGV_DEBUG(psf_fft.size(), delta_f.size(), du::idx_max(padded_psf_data));

	// Because of how fftw works, need to align on 0th pixel
	// we DO NOT want the PSF to be centered in it's frame
	du::shift_inplace(padded_psf_data, padded_psf_data.size()/2);
	
}

void CleanModifiedAlgorithm::__str__(){
	GET_LOGGER;
	LOGV_DEBUG(n_iter);
	LOGV_DEBUG(n_positive_iter);
	LOGV_DEBUG(loop_gain);
	LOGV_DEBUG(threshold);
	LOGV_DEBUG(clean_beam_gaussian_sigma);
	LOGV_DEBUG(add_residual);
	LOGV_DEBUG(noise_std);
	LOGV_DEBUG(rms_frac_threshold);
	LOGV_DEBUG(fabs_frac_threshold);
	LOGV_DEBUG(residual_data);
	LOGV_DEBUG(components_data);
	LOGV_DEBUG(px_choice_map);
}

void CleanModifiedAlgorithm::_calc_pixel_threshold(){
	//px_threshold = threshold * du::max(residual_data);
	if (threshold > 0){
		// Static threshold as a fraction of brightest pixel of the residual
		px_threshold = threshold * du::absmax(residual_data);
	}
	else {
		//puts("NOTE: Using Otsu's method for thresholding");
		// Otsu's method
		// Minimises the intra-class variance (variance of pixels inside a class)
		//
		// probability of in bin i of histogram, p_i = f_i/N, f_i is number of pixels in bin i, N is total number of pixels
		// for class 'c' with boundaries [t0,t1), mean of class is sum(p_i*x(i)) from i(t0) to i(t1), 
		// where i(x) gets the histogram bin of value x, and x(i) is the midpoint value of histogram bin i.
		// u_c = Sum(x(i)*p_i)_{i_0}^{i_1}/(i_1-i_0)
		// variance is averange square distance from mean
		// sigma_c^2 = sum( p_i*(x(i)-u_c)^2 )
		//           = sum( p_i*(x(i)^2 -2*x(i)*u_c + u_c^2) )
		//           = sum( p_i*x(i)^2 -2*p_i*x(i)*u_c + p_i*u_c^2)
				
		// sort based on indices
		std::sort(indices.data(), indices.data()+indices.size(), [this](const short a, const short b)->bool{return (residual_data[a] < residual_data[b]);});
		
		
		
		//std::vector<double> class_sum_prob(indices.data());
		//std::vector<double> class_mean(indices.data());
		double total_mean = 0;
		double p_i = 1.0/indices.size();
		for(const double x : residual_data){
			total_mean += p_i*x;
		}
		
		double cumulative_prob = 0;
		double cumulative_value = 0;
		double cumulative_mean = 0;
		double class_1_adj_prob = 0;
		double class_1_sigma = 0, class_2_sigma=0;
		double intra_class_variance = 0;
		double min_intra_class_variance = 1E300; // very large number
		short min_threshold_index = 0;
		for(short i=0; i<indices.size(); ++i){
			cumulative_value += residual_data[i];
			cumulative_prob += p_i;
			class_1_adj_prob = (p_i/cumulative_prob);
			cumulative_mean += class_1_adj_prob*cumulative_value;
			class_1_sigma = 0;
			class_2_sigma = 0;
			for(short j=0; i< indices.size(); ++j){
				class_1_sigma += class_1_adj_prob*(residual_data[i] - cumulative_mean)*(residual_data[i] - cumulative_mean);
				class_2_sigma += p_i/(1-cumulative_prob)*(residual_data[i] - (total_mean - cumulative_mean))*(residual_data[i] - (total_mean - cumulative_mean));
				
			}
			intra_class_variance = cumulative_prob*class_1_sigma + (1-cumulative_prob)*class_2_sigma;
			if (intra_class_variance < min_intra_class_variance){
				min_threshold_index = i;
			}
		}
		
		px_threshold = residual_data[min_threshold_index];
	}
}

void CleanModifiedAlgorithm::_select_update_pixels(){
	px_choice_map = du::mask_where(
		residual_data, 
		std::function<bool(double)>(
			[this](double v)->bool{
				return(abs(v) > px_threshold);
			}
		)
	);
	du::set_to(selected_pixels, 0.0);
	du::set_at_mask(selected_pixels, px_choice_map, residual_data);
}

std::pair<std::vector<double>, std::vector<size_t>> CleanModifiedAlgorithm::_ensure_odd(
		const std::vector<double>& obs_data, 
		const std::vector<size_t>& obs_shape
	){
	GET_LOGGER;
	
	LOG_DEBUG("Ensuring odd shape of input data");
	LOGV_DEBUG(obs_shape);
	
	data_shape_adjustment.resize(obs_shape.size());
	for(size_t i=0; i<obs_shape.size(); ++i){
		data_shape_adjustment[i] = 1-obs_shape[i]%2;
	}
	LOGV_DEBUG(data_shape_adjustment);
	
	std::vector<size_t> new_obs_shape = du::add(obs_shape, data_shape_adjustment);

	std::vector<double> new_obs_data(du::product(new_obs_shape), 0);
	
	std::vector<size_t> zero(obs_shape.size(),0);

	du::copy_to_rect(obs_data, new_obs_data, obs_shape, new_obs_shape, zero, zero);

	

	return std::make_pair(new_obs_data, new_obs_shape);
}


// Helper function
void calculate_histogram(std::vector<double>& temp_data, std::vector<double>& histogram_edges, std::vector<uint32_t>& histogram_counts){
	//histogram_edges.assign(0, histogram_edges.size());
	//histogram_counts.assign(0, histogram_counts.size());
	std::sort(temp_data.begin(), temp_data.end());
	double temp_av_delta = (temp_data.back() - temp_data.front())/temp_data.size();
	du::set_linspace(histogram_edges, temp_data.front() - temp_av_delta/2, temp_data.back() + temp_av_delta/2);
	du::set_to(histogram_counts, 0);
	// calculate histogram
	size_t bin_idx = 0;
	for(double a : temp_data){
		while(histogram_edges[bin_idx] < a){
			++bin_idx;
		}
		++histogram_counts[bin_idx];
	}
}


// Helper Function
template <class T>
void send_to_canvas(const T& data, const std::vector<size_t>& data_shape, const std::string& canvas_name){
	const std::span<std::byte>& temp2 = image_as_blob(canvas_name+"_data_inprogress", data);
	send_to_named_js_canvas(
		(canvas_name+"-canvas").c_str(), 
		temp2.data(), 
		temp2.size(), 
		data_shape[0], 
		data_shape[1]
	);
}

bool CleanModifiedAlgorithm::doIter(
		size_t i
	){
	bool iter_continue = true;
	
	GET_LOGGER;
	
	LOGV_DEBUG(i);
	
	update_deconv_stats(i, -1);
	
	
	_calc_pixel_threshold();
	
	_select_update_pixels();
	
	du::multiply_inplace(selected_pixels, loop_gain);

	selected_px_fft = fft(selected_pixels);

	current_convolved = du::real_part(ifft(du::multiply(selected_px_fft, psf_fft)));

	du::subtract_inplace(residual_data, current_convolved);
	du::add_inplace(components_data, selected_pixels);

	
	
	fabs_record[i] = du::max(du::apply(residual_data, abs ));
	rms_record[i] = sqrt(du::sum(du::apply(residual_data, du::square ))/residual_data.size());
	threshold_record[i] = px_threshold;
	
	// Check stoping criteria
	if( i == (n_iter-1)){
		iter_continue = false;
		LOG_INFO("Deconvolution finished maximum number of iterations (%).", i+1);
	}
	if( fabs_record[i] < fabs_record[0]*fabs_frac_threshold){
		iter_continue = false;
		LOG_INFO("Deconvolution finished at % iterations. Absolute value of brightest pixel % is lower than threshold value %.", i+1,fabs_record[i], fabs_record[0]*fabs_frac_threshold);
	}
	if( rms_record[i] < rms_record[0]*rms_frac_threshold){
		iter_continue = false;
		LOG_INFO("Deconvolution finished at % iterations. Root mean square of residual % is lower than threshold value %.", i+1,rms_record[i], rms_record[0]*rms_frac_threshold);
	}
	
	if ((plot_update_interval > 0) && (!(i%plot_update_interval) || !iter_continue)){
		// NOTE: Plotting preparation etc. goes inside this if statement
		size_t idx_start = i+1 - plot_update_interval;
		size_t idx_end = i+1;
		
		update_deconv_stats(-1, i);
		
		emscripten_sleep(1); // pass control back to javascript to allow event loop to run
		send_data_to_plot("stopping_criteria", "fabs_record", fabs_record, idx_start, idx_end);
		send_data_to_plot("stopping_criteria", "rms_record", rms_record, idx_start, idx_end);
		send_data_to_plot("stopping_criteria", "threshold_record", threshold_record, idx_start, idx_end);
		
		
		emscripten_sleep(1); // pass control back to javascript to allow event loop to run
		temp_data = residual_data;
		calculate_histogram(temp_data, histogram_edges, histogram_counts);
	
		send_data_to_plot("residual_histogram", "residual_histogram_data", histogram_edges, histogram_counts);
		send_data_to_plot("residual_histogram", "threshold_line_data", threshold_record[i], 0);
		
		emscripten_sleep(1); // pass control back to javascript to allow event loop to run
		temp_data = components_data;
		calculate_histogram(temp_data, histogram_edges, histogram_counts);
		send_data_to_plot("component_histogram", "component_data", histogram_edges, histogram_counts);
		
		emscripten_sleep(1); // pass control back to javascript to allow event loop to run
		temp_data = selected_pixels;
		calculate_histogram(temp_data, histogram_edges, histogram_counts);
		send_data_to_plot("selected_pixels_histogram", "selected_pixels_data", histogram_edges, histogram_counts);
		
		emscripten_sleep(1); // pass control back to javascript to allow event loop to run
		temp_data = current_convolved;
		calculate_histogram(temp_data, histogram_edges, histogram_counts);
		send_data_to_plot("current_convolved_histogram", "current_convolved_data", histogram_edges, histogram_counts);
		
		emscripten_sleep(1); // pass control back to javascript to allow event loop to run
		send_to_canvas(residual_data, data_shape, "inprogress-residual");
		send_to_canvas(selected_pixels, data_shape, "inprogress-selected-pixels");
		send_to_canvas(current_convolved, data_shape, "inprogress-current-convolved");
		send_to_canvas(components_data, data_shape, "inprogress-components");
	}
	
	
	
	
	
	emscripten_sleep(1); // pass control back to javascript to allow event loop to run
	return iter_continue;
}

void CleanModifiedAlgorithm::prepare_observations(
		const std::vector<double>& input_obs_data, 
		const std::vector<size_t>& input_obs_shape, 
		const std::vector<double>& input_psf_data, 
		const std::vector<size_t>& input_psf_shape,
		const std::string& run_tag
	){
	GET_LOGGER;
	LOG_DEBUG("declare variables");
	emscripten_sleep(1); // pass control back to javascript to allow event loop to run
	
	tag=run_tag;
	data_shape_adjustment.resize(input_obs_shape.size());
	
	auto [adjusted_obs_data, adjusted_obs_shape ] = _ensure_odd(input_obs_data, input_obs_shape);
	emscripten_sleep(1); // pass control back to javascript to allow event loop to run
	
	data_shape = adjusted_obs_shape;
	data_size = du::product(data_shape);

	LOG_DEBUG("resize dynamic arrays");
	// resize arrays to hold desired data
	padded_psf_data.resize(data_size);
	psf_fft.resize(data_size);
	selected_pixels.resize(data_size);
	px_choice_map.resize(data_size);
	selected_px_fft.resize(data_size);
	current_convolved.resize(data_size);
	components_data.resize(data_size);
	temp_data.resize(data_size);
	
	indices.resize(data_size);
	for(short i=0; i<indices.size(); ++i){
		indices[i] = i;
	}

	du::multiply_inplace(components_data, 0);

	emscripten_sleep(1); // pass control back to javascript to allow event loop to run

	LOG_DEBUG("set FFT attributes");
	// set attributes for fourier transformers
	//fft.set_attrs(data_shape, false, true);
	fft.set_attrs(data_shape, false, false);
	LOG_DEBUG("forward fft attributes set");
	//ifft.set_attrs(data_shape, true, true);
	ifft.set_attrs(data_shape, true, false);
	LOG_DEBUG("backward fft attributes set");

	LOG_DEBUG("Getting residual from obs_data");
	_get_residual_from_obs(adjusted_obs_data, data_shape);
	emscripten_sleep(1); // pass control back to javascript to allow event loop to run
	
	LOG_DEBUG("Padding PSF data");
	_get_padded_psf(input_psf_data, input_psf_shape);
	emscripten_sleep(1); // pass control back to javascript to allow event loop to run
		
	LOG_DEBUG("precompute PSF FFT");
	// get the FFT of the PSF, will need it later
	psf_fft = fft(padded_psf_data);

	du::write_as_image(_sprintf("./plots/%psf_padded.pgm", tag), padded_psf_data, data_shape);

	du::write_as_image(_sprintf("./plots/%psf_fft_real.pgm", tag), du::real_part(psf_fft), data_shape);
	du::write_as_image(_sprintf("./plots/%psf_fft_imag.pgm", tag), du::imag_part(psf_fft), data_shape);

	// print the backward fft of the psf_fft to see if it agrees with the original data.
	auto psf_fft_ifft = ifft(psf_fft);
	du::write_as_image(_sprintf("./plots/%psf_fft_ifft_real.pgm", tag), du::real_part(psf_fft_ifft), data_shape);
	du::write_as_image(_sprintf("./plots/%psf_fft_ifft_imag.pgm", tag), du::imag_part(psf_fft_ifft), data_shape);
	
	emscripten_sleep(1); // pass control back to javascript to allow event loop to run
}



void CleanModifiedAlgorithm::run(){
	GET_LOGGER;
	LOG_DEBUG("Starting deconvolution n_iter %", n_iter);

	bool iter_continue = true;

	timer::start();
	for(size_t i=0; i<n_iter && iter_continue; ++i){
		iter_continue = doIter(i);
	}

	LOGV_DEBUG(data_shape);

	du::write_as_image(_sprintf("./plots/%components.pgm", tag), components_data, data_shape);
	du::write_as_image(_sprintf("./plots/%residual.pgm", tag), residual_data, data_shape);
	du::write_as_image(_sprintf("./plots/%residual_log.pgm", tag), du::log(residual_data), data_shape);
	
	emscripten_sleep(1); // pass control back to javascript to allow event loop to run
	if (clean_beam_gaussian_sigma > 0){
		LOG_DEBUG("Convolving result with gaussian clean beam with sigma=%", clean_beam_gaussian_sigma);
		Eigen::MatrixXd Kernel(data_shape[0], data_shape[1]);
		std::vector<FourierTransformer::complex> Kernel_fft(Kernel.size());
		std::vector<FourierTransformer::complex> components_data_fft(Kernel.size());

		Eigen::Matrix<double, 2,2> Sigma {	{1.0/(clean_beam_gaussian_sigma*clean_beam_gaussian_sigma), 0},
											{0, 1.0/(clean_beam_gaussian_sigma*clean_beam_gaussian_sigma)}
											};
		
		// As fftw produces non-centered FFTs, pos should be difference from center,
		// however, will that account for everything correctly?
		// I don't think so, I should really adjust the Kernel after it's created
		// so that it is re-centered on (0,0) instead of (data_shape[0]/2, data_shape[1]/2).
		//Eigen::Vector2d pos{0,0};
		Eigen::Vector2d pos{data_shape[0]/2.0,data_shape[1]/2.0}; 
		
		Eigen::Vector2d idx {0,0};

		for(int i=0;i < Kernel.rows(); idx[0]+=1, ++i){
			emscripten_sleep(1); // pass control back to javascript to allow event loop to run
			for(int j=0; j<Kernel.cols(); idx[1]+=1, ++j){
				Kernel(i,j) = exp(-static_cast<double>((idx-pos).transpose() * Sigma * (idx-pos)));
				//LOG_DEBUG("Kernel(%,%) = %",i,j,Kernel(i,j));
				//LOGV_DEBUG(idx);
				//LOGV_DEBUG(Sigma);
				//LOGV_DEBUG(-static_cast<double>((idx-pos).transpose() * Sigma * (idx-pos)));
			}
			idx[1] = 0;
		}
		
		Kernel.array() /= sqrt(M_PI*M_PI/Sigma.determinant());
		LOGV_DEBUG(Kernel.sum());
		//LOGV_DEBUG(pos);
	
		du::write_as_image(_sprintf("./plots/%Kernel.pgm", tag), std::vector<double>(Kernel.data(), Kernel.data()+Kernel.size()), data_shape);

		// TODO:
		// * Re-center Kernel on (0,0) so that the fft-convolution doesn't go weird.
		std::vector<double> temp(Kernel.size(), 0);
		temp[temp.size() - (pos[0]*Kernel.colStride() + pos[1]*Kernel.rowStride())] = 1;


		Kernel_fft = fft(std::vector<double>(Kernel.data(), Kernel.data()+Kernel.size()));
		du::write_as_image(_sprintf("./plots/%Kernel_fft_real.pgm", tag), du::real_part(Kernel_fft), data_shape);
		du::write_as_image(_sprintf("./plots/%Kernel_fft_imag.pgm", tag), du::imag_part(Kernel_fft), data_shape);
			
		du::write_as_image(_sprintf("./plots/%Kernel_fft_ifft_real.pgm", tag), du::real_part(ifft(Kernel_fft)), data_shape);
		du::write_as_image(_sprintf("./plots/%Kernel_fft_ifft_imag.pgm", tag), du::imag_part(ifft(Kernel_fft)), data_shape);
		
		components_data_fft = fft(components_data);
		clean_map = du::real_part(ifft(du::multiply(components_data_fft, du::multiply(Kernel_fft,fft(temp)))));
		LOGV_DEBUG(du::sum(components_data));
		LOGV_DEBUG(du::max(components_data));
		LOGV_DEBUG(du::sum(clean_map));
		LOGV_DEBUG(du::max(clean_map));
	}
	else {
		LOG_DEBUG("Convolution with clean-beam not performed");
		clean_map = components_data;
	}

	emscripten_sleep(1); // pass control back to javascript to allow event loop to run
	if(add_residual){
		LOG_DEBUG("Adding residual to clean map");
		du::add_inplace(clean_map, residual_data);
	} else {
		LOG_DEBUG("Residual NOT added to clean map");
	}


	du::write_as_image(_sprintf("./plots/%clean_map.pgm",tag), clean_map, data_shape);

	timer::stop();
	LOGV_DEBUG(timer::n_seconds());
}





