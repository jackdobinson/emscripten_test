

class Svg{
	static createElement(tag, attributes={}){
		let element = document.createElementNS('http://www.w3.org/2000/svg',tag)
		for (const key of Object.keys(attributes)){
			element.setAttribute(key, attributes[key])
		}
		return element
	}
	
	static setAttrs(element, ...attr_objs){
		for (const attrs of attr_objs){
			for (const key of Object.keys(attrs)){
				element.setAttribute(key, attrs[key])
			}
		}
		return element
	}
	
	static formatNumber(a, sig_figs=3, round_to=1E-12){
		if (round_to != 0){
			let diff = (a % round_to)
			a += (diff > round_to/2) ? diff : -diff
		}
		return a.toPrecision(sig_figs)
	}
	
	static rect(r, s, attrs={}){
		// return an svg rect
		// if height or width is -ve need to alter x and y coords
		attrs.x = r[0] + (s[0] < 0 ? s[0] : 0)
		attrs.y = r[1] + (s[1] < 0 ? s[1] : 0)
		attrs.width = (s[0] < 0 ? - s[0] : s[0])
		attrs.height = (s[1] < 0 ? - s[1] : s[1])
		
		O.insertIfNotPresent(
			attrs,
			{
				"stroke":"black",
				"stroke-width":"0.5",
				"fill":"none",
				"stroke-opacity":0.3,
			}
		)

		return Svg.createElement("rect", attrs)
	}
	
	static path(path, attrs={}){
		//console.log("toSvgLinearPath", path)
		let svg_d_string = `M ${path[0]}, ${path[1]}`
		
		if(path.length > 2){
			svg_d_string += " L"
		}
		else {
			return svg_d_string
		}
		
		let i=0
		for(i=2; i<path.length; i+=2){
			svg_d_string += ` ${path[i]}, ${path[i+1]}`
		}
		attrs.d = svg_d_string
		
		O.insertIfNotPresent(
			attrs,
			{
				"stroke":"black",
				"stroke-width":"0.5",
				"fill":"none",
				"stroke-opacity":0.3,
			}
		)
		
		return Svg.createElement("path", attrs)
	}

	static circle(pos, r, attrs={}){
		attrs.r = r
		attrs.cx = pos[0]
		attrs.cy = pos[1]
		
		O.insertIfNotPresent(
			attrs,
			{
				"stroke":"black",
				"stroke-width":"0.5",
				"fill":"none",
				"stroke-opacity":0.3,
			}
		)
		
		return Svg.createElement("circle", attrs)
	}
	
	static square(pos, r, attrs={}){
		attrs.x = pos[0] - r/2
		attrs.y = pos[1] - r/2
		attrs.width = r
		attrs.height = r
	}
	
	static triangle(pos, r, attrs={}){
		triangle_path = V.from(
			pos[0]+ -1*r/2, pos[1]+ -0.5*r/2, 
			pos[0]+ 0*r/2 , pos[1]+ Math.sin(Math.PI/3)*r/2, 
			pos[0]+ 1*r/2 , pos[1]+ -0.5*r/2
		)
		return Svg.path(triangle_path, attrs)
	}
	
	static text(pos, content, attrs={}){
		attrs.x = pos[0]
		attrs.y = pos[1]
		attrs.textContent = content
		
		O.insertIfNotPresent(
			attrs,
			{
				"text-anchor":"middle",
				"font-family":"sans-serif",
				"font-size" : 8,
				"stroke":"none",
				"stroke-width":"0.5",
				"fill":"black",
				"stroke-opacity":0.3,
			}
		)
		
		return Svg.createElement('text', attrs)
	}
	
	constructor({
			scale = V.from(4,6), //
			scale_units = "cm", //
			viewbox_rect = new R(0,0,scale[0],scale[1])
		}){
		this.parent_element = null
		this.scale = scale
		this.scale_units = scale_units
		this.viewbox_rect = viewbox_rect
		
		this.svg = createSvgElement('svg', {
			width:`${this.scale[0]}${this.scale_units}`, // width of svg element
			height:`${this.scale[1]}${this.scale_units}`, // height of svg element
			viewBox : viewbox_rect.asString(), // viewbox of svg element
		})
		
		
		this.groups = new Map()
	}
	
	renderToTarget(){
		this.parent_element.replaceChildren(this.svg)
	}
	
	setTarget(parent_element){
		this.parent_element = parent_element
	}
	
	addGroup(group_name, attrs={}, parent=null){
		if (group_name === null){
			return
		}
		parent = parent===null ? this.svg : parent
		let group = Svg.createElement('g', attrs)
		(parent===null ? this.svg : parent).appendChild(group)
		this.groups.set(group_name, group)
	}
	
	getGroup(group_name=null){
		return group_name===null ? this.svg : this.groups.get(group_name)
	}
	
	hasGroup(group_name){
		return this.groups.has(group_name)
	}
	
	add(element_type, positions, scales, invariants, attrs, group_name = null){
		console.log(element_type, positions, scales, invariants, attrs, group_name)
		this.getGroup(group_name).appendChild(Svg[element_type](...positions, ...scales, ...invariants, attrs))
	}
	
}