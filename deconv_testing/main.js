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

run_deconv_button.addEventListener("click", async (e)=>{
		if ((sci_image_holder.name === null) || (psf_image_holder.name === null)) {
			console.log("ERROR: Must have uploaded a science or psf image")
			return
		}

		let deconv_type = "clean_modified"
		let deconv_name = "test_deconvolver"

		console.log("Creating deconvolver")
		let n_max_iter = parseInt(n_max_iter_field.value, 10)
		Module.create_deconvolver(deconv_type, deconv_name, n_max_iter)

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





