


function getImageDataFromResult(get_result, arg_list, image_width, image_height){
	let typed_array = get_result(...arg_list)
	let data = new Uint8ClampedArray(typed_array.buffer, typed_array.byteOffset, typed_array.byteLength)
	return new ImageData( data, image_width, image_height)
}


class ImageHolder {
	constructor(target_canvas, html_element, event_name, after_event_fn=()=>{}){
		this.target_canvas = target_canvas
		this.target_canvas_ctx = this.target_canvas.getContext("2d")
		this.after_event_fn = after_event_fn
		this.file=null
		this.name=null
		this.im_w=null
		this.im_h=null
		this.im_display_data=null
		this.status_message = ""
		this.html_element = html_element

		if ((this.html_element !== undefined) && (event_name !== undefined)){
			this.html_element.addEventListener(event_name, this)
		}
	}

	discardPreviousImage(){
		if (this.name !== null){
			Module.remove_image(this.name)
		}
		this.file = null
		this.name = null
		this.im_w = null
		this.im_h = null
		this.im_display_data = null
		this.status_message = ""
	}

	async loadImageToModule(){
		if (this.file === null){
			this.status_message = "ERROR: Attempting to load NULL file."
			return
		}

		if (this.file.name.endsWith(".tiff") || this.file.name.endsWith(".tif")) {
			let uint8_file_data = null
			try{
				if (this.file.bytes !== undefined){ // For some reason File.bytes() is only available for Firefox.
					uint8_file_data = await this.file.bytes()
				} else {
					uint8_file_data = new Uint8Array((await this.file.arrayBuffer()).transferToFixedLength())
				}
			} catch (e) {
				this.status_message = `ERROR: File "${this.file.name}", could not read bytes from file. Recieved error message: ${e.message}`
				return
			}
			try{
				this.name = Module.TIFF_from_js_array(this.file.name, uint8_file_data)
			} catch (e) {
				this.status_message = `ERROR: File "${this.file.name}", could not read bytes as TIFF file. Recieved error message: ${e.message}`
				return
			}
		}
		else {
			this.status_message = `ERROR: File "${this.file.name}", unknown file extension.`
		}
	}

	getImageDimensions(){
		if(this.name === null){
			this.status_message = "ERROR: Must have an image name to get image dimensions"
			return
		}
		this.im_w = Module.TIFF_get_width(this.name)
		this.im_h = Module.TIFF_get_height(this.name)
	}

	async displayImage(){
		if ((this.name===null) || (this.im_w===null) || (this.im_h===null)){
			this.status_message = "ERROR: Must have image name, image width, and image height to display image"
			return
		}
		try{
			this.im_display_data = await getImageDataFromResult(Module.image_as_JSImageData, [this.name], this.im_w, this.im_h)
		}
		catch (e){
			this.status_message = `ERROR: File "${this.file.name}", could not retrieve image data using internal name "${this.name}". Recieved error message: ${e.message}`
			return
		}
		this.target_canvas.width = this.im_w
		this.target_canvas.height = this.im_h
		this.target_canvas.style = `aspect-ratio: ${this.im_w} / ${this.im_h};`
		this.target_canvas_ctx.imageSmoothingEnabled = false
		this.target_canvas_ctx.putImageData(this.im_display_data, 0, 0)

	}

	alert_status(){
		if (this.status_message.length > 0){
			console.error(this.status_message)
			alert(this.status_message)
			for (const label of this.html_element.labels){
				let inner_html = label
				while (inner_html.firstElementChild !== null){
					inner_html = inner_html.firstElementChild
				}
				inner_html.innerText = "ERROR LOADING FILE..."
			}
			return true
		}
		return false
	}

	async handleEvent(e) {
		this.discardPreviousImage()
		if (this.alert_status()){
			return
		}
		
		if (e.target.files.length != 1) {
			this.status_message = "ERROR: Selected multiple files, only want a single file"
			console.log(this.status_message)
		}
		if (this.alert_status()){
			return
		}
		
		// update label show "LOADING..."
		for (const label of e.target.labels){
			let inner_html = label
			while (inner_html.firstElementChild !== null){
				inner_html = inner_html.firstElementChild
			}
			console.log(inner_html)
			inner_html.innerText= "LOADING..."
		}

		this.file = e.target.files[0]
		let filename = this.file.name

		await this.loadImageToModule()
		if (this.alert_status()){
			return
		}

		this.getImageDimensions()
		if (this.alert_status()){
			return
		}
		
		this.displayImage()
		if (this.alert_status()){
			return
		}
		
		// update label to be the new file name
		for (const label of e.target.labels){
			let inner_html = label
			while (inner_html.firstElementChild !== null){
				inner_html = inner_html.firstElementChild
			}
			console.log(inner_html)
			inner_html.innerText=filename
		}
		
		this.after_event_fn()
	}
}


