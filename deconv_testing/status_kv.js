
class HtmlContainer{
	constructor(tag, ...list_of_attrs){
		//console.log(tag, attrs)
		this.html = Html.createElement(tag, ...list_of_attrs) // the HTML of this object
		this.containing_nodes = new Map() // nodes that contain the HTML of this object
		
		assert_all_defined(this.html, this.containing_nodes)
	}
	
	moveTo(parent_element){
		this.removeFromAll()
		this.addTo(parent_element)
	}
	
	addUniqueTo(parent_element){
		this.removeFrom(parent_element)
		this.addTo(parent_element)
	}
	
	addTo(parent_element){
		parent_element.appendChild(this.html)
	}
	
	removeFromAll(){
		for(const [parent_element, containing_node] of this.containing_nodes.entries()){
			parent_element.removeChild(containing_node)
		}
		this.containing_nodes.clear()
	}
	
	removeFrom(parent_element){
		let containing_node = this.containing_nodes.get(parent_element)
		if(containing_node !== undefined){
			parent_element.removeChild(containing_node)
			this.containing_nodes.delete(parent_element)
		}
	}
}

class StatusKV extends HtmlContainer {
	static default_opts = {
		tag : "div", 
		key_container_tag:"div", 
		value_container_tag:"div", 
		class: "status-kv", 
		key_class : "status-kv-key", 
		value_class:"status-kv-value",
		highlight_on_hover_target : null,
	}
	
	constructor(
			key, 
			value="", 
			attrs={}, 
			opts={}
		){
		opts = {...StatusKV.default_opts, ...opts}
		//console.log(key, value, opts)
		super(opts.tag, {class : opts.class}, attrs)
		
		this.html_key_element = Html.createElement(opts.key_container_tag, {class : opts.key_class})
		this.html_value_element = Html.createElement(opts.value_container_tag, {class : opts.value_class})
		
		this.setKey(key)
		this.setValue(value)
		this.setHighlightOnHover(opts.highlight_on_hover_target)
		
		assert_all_defined(this.key, this.value, this.html_key_element, this.html_value_element)
		
		this.html.appendChild(this.html_key_element)
		this.html.appendChild(this.html_value_element)
	}
	
	setKey(key){
		this.key = key
		this.html_key_element.textContent = key
	}
	
	setValue(value){
		this.value = value
		this.html_value_element.textContent = value
	}
	
	setHighlightOnHover(target){
		if (target === null){
			return
		}
		this.target = target
		
		this.target_original_style = target.getAttribute("style")
		
		this.html.onmouseenter = (e)=>{
			//console.log("MOUSE ENTER")
			
			this.target_rect = this.target.getBoundingClientRect()
			this.target_top = this.target_rect.top + window.scrollY
			this.target_left = this.target_rect.left + window.scrollX
			
			this.target_highlighter = Html.createElement("div",{"style":`position:absolute;left:${this.target_left}px;top:${this.target_top}px;width:${this.target_rect.width}px;height:${this.target_rect.height}px;border:5px solid red;z-index: 99;`})
			//console.log(this.target_highlighter.style)
			this.target_highlighter_node = document.body.appendChild(this.target_highlighter)
		}
		this.html.onmouseleave = (e)=>{
			//console.log("MOUSE LEAVE")
			
			this.target_highlighter_node.remove()
		}
	}
	
	
}

class StatusKVManager extends HtmlContainer {
	static default_opts = {
		tag:"div",
		class:"status-container", 
		default_kv_class : StatusKV
	}

	constructor(
			kv_args_list = [], 
			opts = {}
		){
		opts = {...StatusKVManager.default_opts, ...opts}
		super(opts.tag, {class:opts.class})
		
		this.status_kv_map = new Map()
		this.default_kv_class = opts.default_kv_class
		
		assert_all_defined(this.status_kv_map, this.default_kv_class)
		
		for (const kv_args of kv_args_list){
			this.add(...kv_args)
		}
	}
	
	add(key, value, attrs, opts, type){
		//console.log(key,value,opts,type)
		let default_kv_class = (type===undefined) ? this.default_kv_class : type
		let status_kv = new default_kv_class(key, value, attrs, opts)
		
		status_kv.addTo(this.html)
		this.status_kv_map.set(key, status_kv)
		return status_kv
	}
	
	set(key, value, attrs={}){
		let status_kv = this.status_kv_map.get(key)
		if(status_kv === undefined){
			Html.setAttrs(this.add(key, value).html, attrs)
		} else {
			status_kv.setValue(value)
			Html.setAttrs(status_kv.html, attrs)
		}
		
	}
	
}


