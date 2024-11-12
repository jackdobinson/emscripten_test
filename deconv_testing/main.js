"use strict"

//import ImageHolder from "./image_holder.js"

let scratch_canvas = document.getElementById("canvas")
let scratch_canvas_ctx = scratch_canvas.getContext("2d")
scratch_canvas_ctx.imageSmoothingEnabled = false // NOTE: this needs to be set after each canvas resize

let sci_file_picker = document.getElementById("sci_file")
let psf_file_picker = document.getElementById("psf_file")

let sci_canvas = document.getElementById("sci_canvas")
let psf_canvas = document.getElementById("psf_canvas")
let clean_map_canvas = document.getElementById("clean_map_canvas")
let residual_canvas = document.getElementById("residual_canvas")

let run_deconv_button = document.getElementById("run_deconv")
let n_max_iter_field = document.getElementById("n_max_iter")

let sci_image_holder = new ImageHolder(sci_canvas, sci_file_picker, "change")
let psf_image_holder = new ImageHolder(psf_canvas, psf_file_picker, "change")
let deconv_clean_map = null
let deconv_residual = null

let clean_modified_params = new CleanModifiedParameters(document.getElementById("parameter-container"))

let plot_name_map = new Map();

function createSvgElement(tag, attributes={}){
	let element = document.createElementNS('http://www.w3.org/2000/svg',tag)
	for (const key of Object.keys(attributes)){
		element.setAttribute(key, attributes[key])
	}
	return element
}

function setAttrsFor(element, attrs={}){
	for (const key of Object.keys(attrs)){
		element.setAttribute(key, attrs[key])
	}
	return element
}

class LinePlot{
	constructor(name, parentElement, resizeable=true, extent={xmin:0, ymin: 0, xmax:20, ymax:20}){
		this.name = name
		
		// Sizes of things
		this.width_of = {
			x_ax_line:0.2, 
			y_ax_line:0.2, 
			x_ax_tick:0.1, 
			y_ax_tick:0.1,
			x_ax_ticklabel_text:0.1,
			y_ax_ticklabel_text:0.1
		}
		this.length_of = {
			x_ax_tick:0.5,
			y_ax_tick:0.5,
			x_ax_ticklabels:2,
			y_ax_ticklabels:2,
		}
		this.number_of = {
			x_ax_ticks:6,
			y_ax_ticks:6,
			x_ax_ticks_per_ticklabels:1,
			y_ax_ticks_per_ticklabels:1,
		}
		this.angle_of = {
			x_ax_ticklabels : 0,
			y_ax_ticklabels : 0,
		}
		this.font_size_of = {
			x_ax_ticklabels: 1.6, //pixels
			y_ax_ticklabels: 1.6, //pixels
		}
		
		
		this.resizeable = resizeable
		this.data = null
		
		this.d2w_transform = null // transforms data coords to window coords so we can draw them
		
		this.w_rect = {x:-1, y:-1, w:14, h:14}
		
		// plot extent
		this.ax_rect = {x:0, y:0, w:10, h:10}
		this.ax_extent = extent
		this.x_ax = null
		this.y_ax = null
		this.x_ax_ticks = []
		this.y_ax_ticks = []
		this.x_ax_ticklabels = []
		this.y_ax_ticklabels = []
		this.x_ax_label = ""
		this.y_ax_label = ""
		
		this.data_extent = {xmin:null, xmax:null, ymin:null, ymax:null}
		this.last_point = null
		
		this.n_data_points = 0
		this.data = []
		this.markers = []
		this.line_segments = []
		
		this.auto_resize = {x:true, y:true}
		
		
		// default marker creation function
		this.createMarker = (x,y)=>{return createSvgElement("circle", {cx:x, cy:y, r:0.5})}
		
		
		this.svg = createSvgElement('svg', {
			width:"200px", // width of svg element
			height:"200px", // height of svg element
		})
		this.update_viewbox()
		this.plot_area = createSvgElement("g")
		this.calculate_d2w_transform()
		this.apply_d2w_transform()
		
		
		this.create_axes()
		
		
		
		
		//this.svg.append(this.circle_1, this.circle_2)
		this.svg.appendChild(this.plot_area)
		parentElement.appendChild(this.svg)
		
		// testing
		//this.circle_1 = createSvgElement('circle', {cx:"50", cy:"50", r:"50"})
		//this.circle_2 = createSvgElement('circle', {cx:"180", cy:"80", r:"20"})
		
		//this.draw_point(0,0)
		//this.draw_point(5,10)
		
	}
	
	calculate_d2w_transform(){
		// scalex, skewx, skewy, scaley, transposex, transposey
		let scale_x = this.ax_rect.w/(this.ax_extent.xmax-this.ax_extent.xmin)
		let scale_y = -1*this.ax_rect.h/(this.ax_extent.ymax-this.ax_extent.ymin)
		let skew_x = 0
		let skew_y = 0
		let trans_x = this.ax_rect.x
		let trans_y = this.ax_rect.h - this.ax_rect.y
		this.d2w_transform = `matrix( ${scale_x} ${skew_x} ${skew_y} ${scale_y} ${trans_x} ${trans_y})`
	}
	
	apply_d2w_transform(){
		this.plot_area.setAttribute("transform", this.d2w_transform)
	}
	
	use_d2w_transform(x,y){
		let scale_x = this.ax_rect.w/(this.ax_extent.xmax-this.ax_extent.xmin)
		let scale_y = -1*this.ax_rect.h/(this.ax_extent.ymax-this.ax_extent.ymin)
		let skew_x = 0
		let skew_y = 0
		let trans_x = this.ax_rect.x
		let trans_y = this.ax_rect.h - this.ax_rect.y
		return [x*scale_x + trans_x, y*scale_y + trans_y]
	}
	
	update_viewbox(){
		this.svg.setAttribute("viewBox", `${this.w_rect.x} ${this.w_rect.y} ${this.w_rect.w} ${this.w_rect.h}`)
	}
	
	set_extent(xmin, xmax, ymin, ymax){
		this.ax_extent = {xmin:xmin, xmax:xmax, ymin:ymin, ymax:ymax}
	}
	
	create_axes_lines(){
		this.x_ax = createSvgElement("line", {x1:this.ax_extent.xmin, y1:this.ax_extent.ymin, x2:this.ax_extent.xmax, y2:this.ax_extent.ymin})
		this.y_ax = createSvgElement("line", {x1:this.ax_extent.xmin, y1:this.ax_extent.ymin, x2:this.ax_extent.xmin, y2:this.ax_extent.ymax})
		this.plot_area.append(this.x_ax, this.y_ax)
	}
	update_axes_lines_style(){
		setAttrsFor(this.x_ax, {stroke:"black", "stroke-width":this.width_of.x_ax_line})
		setAttrsFor(this.y_ax, {stroke:"black", "stroke-width":this.width_of.y_ax_line})
	}
	update_axes_lines(){
		setAttrsFor(this.x_ax, {x1:this.ax_extent.xmin, y1:this.ax_extent.ymin, x2:this.ax_extent.xmax, y2:this.ax_extent.ymin})
		setAttrsFor(this.y_ax, {x1:this.ax_extent.xmin, y1:this.ax_extent.ymin, x2:this.ax_extent.xmin, y2:this.ax_extent.ymax})
	}
	
	create_axes_ticks(){
		let i=0
		for(i=0;i<this.number_of.x_ax_ticks;++i){
			this.x_ax_ticks.push(createSvgElement("line"))
		}
		for(i=0;i<this.number_of.y_ax_ticks;++i){
			this.y_ax_ticks.push(createSvgElement("line"))
		}
		this.plot_area.append(...this.x_ax_ticks, ...this.y_ax_ticks)
	}
	update_axes_ticks(){
		// have to subtract 1 as we are starting at zero and going to end of the line
		let x_tick_step = (this.ax_extent.xmax-this.ax_extent.xmin)/(this.number_of.x_ax_ticks-1)
		let y_tick_step = (this.ax_extent.ymax-this.ax_extent.ymin)/(this.number_of.y_ax_ticks-1)
		for (const [i, tick] of this.x_ax_ticks.entries()){
			setAttrsFor(tick, {
				x1:this.ax_extent.xmin+i*x_tick_step,
				x2:this.ax_extent.xmin+i*x_tick_step,
				y1:this.ax_extent.ymin,
				y2:this.ax_extent.ymin-this.length_of.x_ax_tick,
			})
		}
		for(const [i,tick] of this.y_ax_ticks.entries()){
			setAttrsFor(tick, {
				x1:this.ax_extent.xmin-this.length_of.y_ax_tick,
				x2:this.ax_extent.xmin,
				y1:this.ax_extent.ymin+i*y_tick_step,
				y2:this.ax_extent.ymin+i*y_tick_step,
			})
		}
	}
	update_axes_ticks_style(){
		for(const tick of this.x_ax_ticks){
			setAttrsFor(tick, {stroke:"black", "stroke-width":this.width_of.x_ax_tick})
		}
		for(const tick of this.y_ax_ticks){
			setAttrsFor(tick, {stroke:"black", "stroke-width":this.width_of.y_ax_tick})
		}
	}
	
	create_axes_ticklabels(){
		let i=0
		for(i=0;i<this.number_of.x_ax_ticks;++i){
			if(i%this.number_of.x_ax_ticks_per_ticklabels == 0){
				this.x_ax_ticklabels.push(createSvgElement("text"))
			}
		}
		for(i=0;i<this.number_of.y_ax_ticks;++i){
			if(i%this.number_of.y_ax_ticks_per_ticklabels == 0){
				this.y_ax_ticklabels.push(createSvgElement("text"))
			}
		}
		this.svg.append(...this.x_ax_ticklabels, ...this.y_ax_ticklabels)
	}
	update_axes_ticklabels(){
		// have to subtract 1 as we are starting at zero and going to end of the line
		let x_tick_step = (this.ax_extent.xmax-this.ax_extent.xmin)/(this.number_of.x_ax_ticks-1)
		let y_tick_step = (this.ax_extent.ymax-this.ax_extent.ymin)/(this.number_of.y_ax_ticks-1)
		
		let x=0
		let y=0
		let tl = 0
		
		let j=0
		for (const [i, tick] of this.x_ax_ticks.entries()){
			if((i%this.number_of.x_ax_ticks_per_ticklabels) == 0){
				[x,y] = this.use_d2w_transform(
					this.ax_extent.xmin + i*x_tick_step - 0.5*this.length_of.x_ax_ticklabels, 
					this.ax_extent.ymin - 5*this.length_of.x_ax_tick
				)
				
				setAttrsFor(this.x_ax_ticklabels[j], {
					x:x,
					y:y,
					textLength: this.use_d2w_transform(this.length_of.x_ax_ticklabels,0)[0],
				})
				this.x_ax_ticklabels[j].textContent = (this.ax_extent.xmin + i*x_tick_step).toString()
				j++
			}
		}
		j=0
		for(const [i,tick] of this.y_ax_ticks.entries()){
			if((i%this.number_of.y_ax_ticks_per_ticklabels) == 0){
				[x,y] = this.use_d2w_transform(
					this.ax_extent.xmin - 2*this.length_of.y_ax_tick - this.length_of.y_ax_ticklabels, 
					this.ax_extent.ymin + i*y_tick_step
				)
				setAttrsFor(this.y_ax_ticklabels[j], {
					x:x,
					y:y,
					textLength: this.use_d2w_transform(this.length_of.y_ax_ticklabels,0)[0],
				})
				this.y_ax_ticklabels[j].textContent = (this.ax_extent.ymin + i*y_tick_step).toString()
				j++
			}
		}
	}
	update_axes_ticklabels_style(){
		for(const label of this.x_ax_ticklabels){
			setAttrsFor(label, {
				stroke:"black", 
				"stroke-width":this.width_of.x_ax_ticklabel_text,
				"style":`font: normal ${this.font_size_of.x_ax_ticklabels/(this.use_d2w_transform(1,0)[0])}px sans-serif`,
			})
		}
		for(const label of this.y_ax_ticklabels){
			setAttrsFor(label, {
				stroke:"black", 
				"stroke-width":this.width_of.y_ax_ticklabel_text,
				"style":`font: normal ${this.font_size_of.y_ax_ticklabels/(this.use_d2w_transform(1,0)[0])}px sans-serif`,
			})
		}
	}
	
	create_axes(){
		this.create_axes_lines()
		this.create_axes_ticks()
		this.create_axes_ticklabels()
		this.update_axes()
		this.update_axes_style()
	}
	
	update_axes(){
		this.update_axes_lines()
		this.update_axes_ticks()
		this.update_axes_ticklabels()
	}
	
	update_axes_style(){
		this.update_axes_lines_style()
		this.update_axes_ticks_style()
		this.update_axes_ticklabels_style()
	}
	
	
	
	is_point_within_axes(x,y){
		return ((this.ax_extent.xmin<x) && (this.ax_extent.xmax>x) && (this.ax_extent.ymin<y) && (this.ax_extent.ymax >y))
	}
	
	set_marker(function_to_create_marker_at_pos){
		this.createMarker = function_to_create_marker_at_pos
	}
	
	update_data_extent(x,y){
		let changed = false
		if (this.data_extent.xmin === null || x < this.data_extent.xmin){
			this.data_extent.xmin = x
			changed = true
		}
		if (this.data_extent.xmax === null || x > this.data_extent.xmax){
			this.data_extent.xmax = x
			changed = true
		}
		if (this.data_extent.ymin === null || y < this.data_extent.ymin){
			this.data_extent.ymin = y
			changed = true
		}
		if (this.data_extent.ymax === null || y > this.data_extent.ymax){
			this.data_extent.ymax = y
			changed = true
		}
		return changed
	}
	
	do_auto_resize(){
		changed = false
		if (this.auto_resize.x){
			if(this.data_extent.xmin < this.extent.xmin){
				this.extent.xmin = this.data_extent.xmin
				changed = true
			}
			if(this.data_extent.xmax > this.extent.xmax){
				this.extent.xmax = this.data_extent.xmax
				changed = true
			}
		}
		if (this.auto_resize.y){
			if(this.data_extent.ymin < this.extent.ymin){
				this.extent.ymin = this.data_extent.ymin
				changed = true
			}
			if(this.data_extent.ymax > this.extent.ymax){
				this.extent.ymax = this.data_extent.ymax
				changed = true
			}
		}
		return changed
	}
	
	draw_point(x, y){
		console.log("drawing point", x, y)
		let should_update_axes = false
		
		let mkr = this.createMarker(x,y)
		this.plot_area.appendChild(mkr)
		this.markers.push(mkr)
		this.last_point = [x,y]
		this.data.push(this.last_point)
		
		should_update_axes ||= this.update_data_extent(x,y)
		if ((this.n_data_points < 2) || !this.is_point_within_axes(x,y) ){
			should_update_axes ||= this.do_auto_resize()
		}
		
		if(should_update_axes){
			this.update_axes()
		}
		
	}
}



plot_name_map.set("fabs_record", new LinePlot("fabs_record", document.getElementById("progress-plots")))



run_deconv_button.addEventListener("click", async (e)=>{
		if ((sci_image_holder.name === null) || (psf_image_holder.name === null)) {
			console.log("ERROR: Must have uploaded a science or psf image")
			return
		}

		let deconv_type = "clean_modified"
		let deconv_name = "test_deconvolver"

		console.log("Creating deconvolver")
		//let n_max_iter = parseInt(n_max_iter_field.value, 10)
		
		
		
		Module.create_deconvolver(deconv_type, deconv_name)

		clean_modified_params.set_params(deconv_type, deconv_name)

		console.log("Running deconvolver for ${sci_image_holder.name} ${psf_image_holder.name}")
		await Module.run_deconvolver(deconv_type, deconv_name, sci_image_holder.name, psf_image_holder.name, "")
		
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

		// SOMETHING WRONG HERE
		// For some reason the data send to the canvases
		// is the same. Despite them being different in here,
		// and when I send it to a different canvas from inside C++
		// May be something to do with JS being wierd when it comes
		// to objects. Maybe I should try splitting up some of this
		// or making it into a class?

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





