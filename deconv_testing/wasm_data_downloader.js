


class WasmDataDownloader{
	constructor(
			filename, // <String> name of the file that will be downloaded, e.g. "image.jpg"
			file_id, // <Obj> argument passed to `wasm_data_provider` that enables retrieval of the data we want
			wasm_data_provider, // <Callable(Obj)->TypedArray> A function that provides the data for the download.
			mime_type="application/octet-stream" // <String>
		){
		
		assert_all_defined(filename, file_id, wasm_data_provider, mime_type)

		this.filename = filename
		this.file_id = file_id
		this.wasm_data_provider = wasm_data_provider
		this.mime_type = mime_type
	}
	
	handleEvent(){
		// This actually causes the download to start
		// Shamelessly copied from "https://gist.github.com/TrevorSundberg/74dc4f576b94d19711a4547dcf04af40"
				
		if (!deconv_complete){
			throw new Error("Deconvolution has not completed, cannot download results")
		}
		
		
		const a = document.createElement("a")
		a.style = 'display:none'
		document.body.appendChild(a)
		
		const wasm_data = this.wasm_data_provider(this.file_id)
		const blob = new Blob([wasm_data], {type:"octet/stream"})
		const url = window.URL.createObjectURL(blob)
		
		a.href=url
		a.download = this.filename
		a.click() // make the element perform its download
		
		window.URL.revokeObjectURL(url)
		document.body.removeChild(a)
	}
}