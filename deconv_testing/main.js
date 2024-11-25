"use strict"

//import ImageHolder from "./image_holder.js"

let deconv_type = "clean_modified"
let deconv_name = "test_deconvolver"
let deconv_complete = false

let scratch_canvas = document.getElementById("canvas")
let scratch_canvas_ctx = scratch_canvas.getContext("2d")
scratch_canvas_ctx.imageSmoothingEnabled = false // NOTE: this needs to be set after each canvas resize

let sci_file_picker = document.getElementById("sci_file")
let psf_file_picker = document.getElementById("psf_file")

let sci_canvas = document.getElementById("sci_canvas")
let psf_canvas = document.getElementById("psf_canvas")

let adjusted_psf_canvas = document.getElementById("adjusted-psf-canvas")

let clean_map_canvas = document.getElementById("clean_map_canvas")
let residual_canvas = document.getElementById("residual_canvas")

let download_clean_map_button = document.getElementById("download-clean-map-button")
let download_residual_button = document.getElementById("download-residual-button")

let run_deconv_button = document.getElementById("run_deconv")
let n_max_iter_field = document.getElementById("n_max_iter")

let sci_image_holder = new ImageHolder(sci_canvas, sci_file_picker, "change")
let psf_image_holder = new ImageHolder(psf_canvas, psf_file_picker, "change")
let deconv_clean_map = null
let deconv_residual = null

let clean_modified_params = new CleanModifiedParameters(document.getElementById("parameter-container"))

let svg_figure = new SvgFig(
	"progress plots", 
	document.getElementById("progress-plots"),
	V.from(20,20),
	new R(0,0,2,2)
)
let plot_name_map = new Map();

/*
const std::string& deconv_type, 
		const std::string& deconv_name, 
		const std::string& file_id,
		const std::string& original_file_name
*/
download_clean_map_button.addEventListener(
	"click",
	new WasmDataDownloader( 
		()=>{
			let fn = ""
			let ext = ".tiff"
			if(sci_image_holder.file.name.endsWith(".tiff")){
				fn = sci_image_holder.file.name.slice(0,-5)+"_"
			}
			else if (sci_image_holder.file.name.endsWith(".tif")){
				fn = sci_image_holder.file.name.slice(0,-4)+"_"
				ext = ".tif"
			}
			
			return fn+"deconvolved_clean_map" + ext
		},
		"deconv.clean_map", 
		(file_id)=>Module.get_tiff(deconv_type, deconv_name, file_id, sci_image_holder.name)
	)
)

download_residual_button.addEventListener(
	"click",
	new WasmDataDownloader( 
		()=>{
			let fn = ""
			let ext=".tiff"
			if(sci_image_holder.file.name.endsWith(".tiff")){
				fn = sci_image_holder.file.name.slice(0,-5)+"_"
			}
			else if (sci_image_holder.file.name.endsWith(".tif")){
				fn = sci_image_holder.file.name.slice(0,-4)+"_"
				ext = ".tif"
			}
			
			return fn+"deconvolved_residual"+ext
		},
		"deconv.residual", 
		(file_id)=>Module.get_tiff(deconv_type, deconv_name, file_id, sci_image_holder.name)
	)
)



plot_name_map.set(
	"fabs_record", 
	new LogLinePlot({
		label: "fabs_record", 
		ax_labels: ['iteration', 'absolute brightest pixel fraction'],
		ax_extent: E.from(0,1,0,1),//E.from(0,5,0,1),
		svg_fig: svg_figure
	})
)

plot_name_map.set(
	"rms_record", 
	new LogLinePlot({
		label: "rms_record", 
		ax_labels: ['iteration', 'residual rms'],
		ax_extent: E.from(0,1,0,1),//E.from(0,5,0,1),
		svg_fig: svg_figure,
		p_rect_in_f: new R(1,0,1,1),
	})
)

plot_name_map.set(
	"threshold_record", 
	new LogLinePlot({
		label: "threshold_record", 
		ax_labels: ['iteration', 'threshold value'],
		ax_extent: E.from(0,1,0,1),//E.from(0,5,0,1),
		svg_fig: svg_figure,
		p_rect_in_f: new R(0,1,1,1),
	})
)


run_deconv_button.addEventListener("click", async (e)=>{
		if ((sci_image_holder.name === null) || (psf_image_holder.name === null)) {
			alert("ERROR: Missing input data.\n\nDeconvolution requires upload of a science image and a psf image")
			return
		}

		let invalid_params = clean_modified_params.validate()
		if(invalid_params.length != 0){
			alert(`ERROR: Could not run deconvolution.\n\nThe following parameters are invalid and need to be corrected:\n\t${invalid_params.join("\n\t")}`)
			return;
		}
		
		deconv_complete = false
		console.log("Creating deconvolver")
		//let n_max_iter = parseInt(n_max_iter_field.value, 10)
		
		
		
		await Module.create_deconvolver(deconv_type, deconv_name)

		// Check for invalid params again when we set the values
		invalid_params = clean_modified_params.set_params(deconv_type, deconv_name)
		if(invalid_params.length != 0){
			alert(`ERROR: Could not run deconvolution.\n\nThe following parameters are invalid and need to be corrected:\n\t${invalid_params.join("\n\t")}`)
			return;
		}
		
		// Automatically size the plot for the expected number of iterations
		plot_name_map.get("fabs_record").current_data_area.setExtent(
			E.from(
				0,
				clean_modified_params.valueOf("n_iter"),
				Math.log10(clean_modified_params.valueOf("fabs_frac_threshold")),
				0
			)
		)
		plot_name_map.get("rms_record").current_data_area.setExtent(
			E.from(
				0,
				clean_modified_params.valueOf("n_iter"),
				Math.log10(clean_modified_params.valueOf("rms_frac_threshold")),
				0
			)
		)
		
		console.log("run_deconv_button.addEventListener::click", Math.log10(clean_modified_params.valueOf("fabs_frac_threshold")))

		console.log("Preparing deconvolver for ${sci_image_holder.name} ${psf_image_holder.name}")
		await Module.prepare_deconvolver(deconv_type, deconv_name, sci_image_holder.name, psf_image_holder.name, "")
		
		
		console.log("Running prepared deconvolver")
		await Module.run_deconvolver(deconv_type, deconv_name)
		
		deconv_complete = true
		
		let width = sci_image_holder.im_w
		let height = sci_image_holder.im_h
		width += (1-width%2)
		height += (1-height%2)

		console.log("Get results from deconvolver")
		// assume results are the same size as the science image
		deconv_clean_map = getImageDataFromResult(
			Module.get_deconvolver_clean_map, 
			[deconv_type, deconv_name], 
			width, 
			height
		)
		deconv_residual = getImageDataFromResult(
			Module.get_deconvolver_residual,
			[deconv_type, deconv_name],
			width,
			height
		)

		console.log("deconv_clean_map", deconv_clean_map)
		console.log("deconv_residual", deconv_residual)

		console.log("Display results on canvas elements")

		clean_map_canvas.width = width
		clean_map_canvas.height = height
		clean_map_canvas.getContext("2d").putImageData(deconv_clean_map,0,0)

	
		residual_canvas.width = width
		residual_canvas.height = height
		residual_canvas.getContext("2d").putImageData(deconv_residual,0,0)	

		console.log("Results should be displayed")
	}
)





