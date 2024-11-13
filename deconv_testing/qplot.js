function createSvgElement(tag, attributes={}){
	let element = document.createElementNS('http://www.w3.org/2000/svg',tag)
	for (const key of Object.keys(attributes)){
		element.setAttribute(key, attributes[key])
	}
	return element
}

function setAttrsFor(element, ...attr_objs){
	for (const attrs of attr_objs){
		for (const key of Object.keys(attrs)){
			element.setAttribute(key, attrs[key])
		}
	}
	return element
}


class DataArea{
	
	static fromRect(
			d_rect_in_p = new R(0.1,0.9,0.8,-0.8), // rectangle that defines this area in plot coords
			d_rect_in_d = new R(0,0,1,1), // rectangle that defines this area in data coords
		){
		return new DataArea(
			T.invert(R.getTransformFromTo(d_rect_in_d, d_rect_in_p)),
			p_rect
		)
	}
	
	constructor(
			label,
			d2p_transform = T.invert(T.from(1,-1,0,1)), // data coords to plot coords transform
			d_rect_in_p = new R(0,1,1,-1), // rectangle that defines the data area in plot coords
			ax_labels=['x-axis', 'y-axis'], // labels for axes
			
		){
		this.label = label
		this.d2p_transform = d2p_transform // data coords to plot coords
		this.p2d_transform = T.invert(d2p_transform)
		this.d_rect_in_p = d_rect_in_p
		
		this.svg_group = createSvgElement("g", {class:"data-area "+this.label})
		
		this.svg_scaling_factor =  2E-3*(V.accumulate_sum(V.component_abs(this.d_rect_in_p.s)))
		console.log("svg_scaling_factor", this.svg_scaling_factor)
		
		
		
		this.axes = this.createAxes(ax_labels)
		
		
		
		// data display params
		this.p2f_transform = null
		this.d2f_transform = null
		
		this.marker_size = this.svg_scaling_factor
		this.marker_colour = "blue"
		this.marker_factory = (x,y)=>{
			this.svg_group.appendChild(createSvgElement("circle", {x:x,y:y,r:this.marker_size,fill:this.marker_colour}))
		}
	}
	
	svg(){
		return this.svg_group
	}
	
	
	add_data_point(x,y){
		console.log("NOTE: THIS IS NOT WORKING RIGHT NOW")
		console.log("add_data_point", x, y)
		console.log(...T.apply(this.d2f_transform, V.from(x,y)))
		this.marker_factory(...T.apply(this.d2f_transform, V.from(x,y)))
	}
	
	getPlotCoordOfPoint(...args){
		return [...T.apply(this.d2p_transform, args)]
	}
	
	updateTransformFromDataRect(d_rect_in_d){
		this.d2p_transform = R.getTransformFromTo(d_rect_in_d, this.d_rect_in_p)
		this.p2d_transform = T.invert(this.d2p_transform)
	}
	
	getPathOfConstantCoord(
			start_in_p,
			length_in_p,
			const_coord_idx,
			n_segments=1,
		){
		console.log("a", start_in_p, length_in_p, const_coord_idx, n_segments)
		let start_in_d = T.apply(this.p2d_transform, start_in_p)
		let length_in_d = T.scalar_scale_along_dim(this.p2d_transform, const_coord_idx, length_in_p)
		let end_in_d = V.add(start_in_d, V.scalar_prod(V.unit(const_coord_idx), length_in_d))
		console.log("b", start_in_d, length_in_d, end_in_d)
		
		let path_in_p = T.apply_block(
			this.d2p_transform,
			P.interpolateBetween(start_in_d, end_in_d, n_segments)
		)
		
		console.log("c", path_in_p)
		
		return path_in_p
	}
	
	getPointsOfConstantCoord(
			start_in_p,
			length_in_p,
			const_coord_idx,
			n_points=6,
		){
		// Includes endpoints
		let start_in_d = T.apply(this.p2d_transform, start_in_p)
		let length_in_d = T.scalar_scale_along_dim(this.p2d_transform, const_coord_idx, length_in_p)
		let step_in_d = V.scalar_prod(V.unit(const_coord_idx), length_in_d/(n_points-1))
		
		console.log(start_in_p, length_in_p, const_coord_idx, n_points)
		console.log(start_in_d, length_in_d, step_in_d)
		
		let points_in_p = []
		for(let i=0; i<n_points; i++){
			points_in_p.push(T.apply(this.d2p_transform, V.add(start_in_d, V.scalar_prod(step_in_d, i))))
		}
		return points_in_p
	}
	
	createAxesPaths(n_segments = 1){
		let x_axis_path = this.getPathOfConstantCoord(this.d_rect_in_p.r, this.d_rect_in_p.s[0], 0, n_segments)
		let y_axis_path = this.getPathOfConstantCoord(this.d_rect_in_p.r, this.d_rect_in_p.s[1], 1, n_segments)
		
		return [x_axis_path, y_axis_path]
	}
	
	createAxesTickPositions(x_n_ticks=5, y_n_ticks=5){
		let x_tick_positions_in_p = this.getPointsOfConstantCoord(this.d_rect_in_p.r, this.d_rect_in_p.s[0], 0, x_n_ticks)
		let y_tick_positions_in_p = this.getPointsOfConstantCoord(this.d_rect_in_p.r, this.d_rect_in_p.s[1], 1, y_n_ticks)
		
		return [x_tick_positions_in_p, y_tick_positions_in_p]
	}
	
	createAxesTicklabels(
			x_tick_positions_in_p, 
			y_tick_positions_in_p, 
			x_label_mod=1, 
			y_label_mod=1, 
			x_fmtr=(a)=>a.toPrecision(3), 
			y_fmtr=(a)=>a.toPrecision(3),
		){
		let x_ticklabels = []
		let y_ticklabels = []
		
		for(const [i,x_p] of x_tick_positions_in_p.entries()){
			if(i%x_label_mod == 0){
				x_ticklabels.push(x_fmtr(T.apply(this.p2d_transform, x_p)[0]))
			} else {
				x_ticklabels.push("")
			}
		}
		
		for(const [i,y_p] of y_tick_positions_in_p.entries()){
			if(i%y_label_mod == 0){
				y_ticklabels.push(y_fmtr(T.apply(this.p2d_transform, y_p)[1]))
			} else {
				y_ticklabels.push("")
			}
		}
		
		return [x_ticklabels, y_ticklabels]
	}
	
	createAxes(ax_labels=['x-axis', 'y-axis']){
		let ax_label_pos_in_p = [
			V.add(this.d_rect_in_p.r, V.prod(this.d_rect_in_p.s, V.from(0.5,-0.15))),
			V.add(this.d_rect_in_p.r, V.prod(this.d_rect_in_p.s, V.from(-0.2,0.5))),
		]
		let ax_paths_in_p = this.createAxesPaths()
		let ax_tick_positions_in_p = this.createAxesTickPositions()
		let ax_ticklabels = this.createAxesTicklabels(...ax_tick_positions_in_p)
		
		console.log(ax_paths_in_p)
		console.log(ax_tick_positions_in_p[0].length)
		console.log(2*this.svg_scaling_factor)
		
		let axes = []
		for(let i=0;i<2;i++){
			axes.push(
				new Axis(
					i,
					[1,-1],
					ax_labels[i],
					ax_label_pos_in_p[i],
					ax_paths_in_p[i],
					ax_tick_positions_in_p[i],
					V.const_of_size(ax_tick_positions_in_p[i].length, 0.02),
					ax_ticklabels[i],
					this.svg_scaling_factor,
				)
			)
		}
		for(const axis of axes){
			this.svg_group.append(axis.svg())
		}
		return axes
	}
	
	createAxesSvg(p2f_transform){
		this.p2f_transform = p2f_transform
		this.d2f_transform = T.prod(this.p2f_transform, this.d2p_transform)
		for(const axis of this.axes){
			axis.createSvgElements(this.p2f_transform)
		}
	}
	
	
}

class Axis{
	constructor(
			dim, // dimension number of the axis (0=x, 1=y,...)
			grow_direction=[1,-1], // direction along dimension that we should put labels etc.
			label="", // string: axis label
			label_pos_in_p=V.from(), // position of label in plot coords
			path_in_p=V.from(), // Float64Array, even indices are x values odd are y values: line that defines the axis (constant value of 1 dimension in data coords)
			tick_positions_in_p=[], // list of position vectors in plot coords: ticks along axis to help with measuring (constant value of other dimensions in data coords)
			tick_lengths_in_p = [], // list of lengths in plot coords: lengths of ticks to draw
			ticklabels=[], //labels for the ticks so we can measure,
			svg_scaling_factor = 1,
			
		){
		console.log(tick_lengths_in_p)
		
		this.dim = dim
		this.grow_direction = grow_direction
		this.label = label
		this.label_pos_in_p = label_pos_in_p
		this.path_in_p = path_in_p
		this.tick_positions_in_p = tick_positions_in_p
		this.tick_lengths_in_p = tick_lengths_in_p
		this.ticklabels = ticklabels
		this.svg_scaling_factor = svg_scaling_factor
		
		this.svg_group = createSvgElement("g",{class:`axis-${this.dim} ${this.label}`})
		this.svg_label = null
		this.svg_path = null
		this.svg_ticks = []
		this.svg_ticklabels = []
	}
	
	createSvgElements(
			p2f_transform, // transform from plot coords to figure coords
			label_font_family="sans-serif",
			label_font_size=10,
			ticklabel_font_family="sans-serif",
			ticklabel_font_size=8,
			label_stroke_width=2,
			ticklabel_stroke_width=1,
		){
		
		let label_pos_in_f = T.apply(p2f_transform, this.label_pos_in_p)
		console.log("label pos", this.label_pos_in_p, label_pos_in_f)
		
		let svg_label_container = createSvgElement(
		"g", {
			"class" : `axis-${this.dim}-label`
		})
		let svg_label_text = createSvgElement("text", {
			x:label_pos_in_f[0], 
			y:label_pos_in_f[1], 
			"text-anchor":"middle", 
			"font-family":label_font_family, 
			"font-size":this.svg_scaling_factor*label_font_size
		})
		svg_label_container.appendChild(svg_label_text)
		this.svg_label = svg_label_container
		if (this.dim==1){
			setAttrsFor(this.svg_label, {
				"transform-origin":`${label_pos_in_f[0]} ${label_pos_in_f[1]}`,
				transform:"rotate(270)"}
			)
		}
		svg_label_text.textContent = this.label
		
		
		let path_in_f = T.apply_block(p2f_transform, this.path_in_p)
		console.log("path pos", this.path_in_p, path_in_f)
		this.svg_path = createSvgElement(
			"path", 
			{
				d:P.toSvgLinearPath(path_in_f),
				stroke:"black",
				"stroke-width":this.svg_scaling_factor*label_stroke_width,
			}
		)
		
		let tick_pos_in_f = V.zero
		let tick_delta_in_f = V.zero
		let tick_end_in_f = V.zero
		let ticklabel_pos_in_f = V.zero
		for(const [i, tick_pos_in_p] of this.tick_positions_in_p.entries()){
			
			
			// calculate where the ticks should go in figure coords
			tick_pos_in_f = T.apply(p2f_transform, tick_pos_in_p)
			console.log("grow_direction", this.grow_direction)
			console.log(V.scalar_prod(V.unit(this.dim), this.grow_direction[this.dim]*this.tick_lengths_in_p[i]))
			tick_delta_in_f = T.scale(p2f_transform, V.scalar_prod(V.unit(1-this.dim), this.grow_direction[this.dim]*this.tick_lengths_in_p[i]))
			tick_end_in_f = V.add(tick_pos_in_f, tick_delta_in_f)
			
			
			console.log("tick pos", i, tick_pos_in_p, tick_pos_in_f, tick_delta_in_f, tick_end_in_f)
			
			// Create ticks
			this.svg_ticks.push(
				createSvgElement("line", {
					x1:tick_pos_in_f[0], 
					y1:tick_pos_in_f[1], 
					x2:tick_end_in_f[0], 
					y2:tick_end_in_f[1],
					"stroke":"black",
					"stroke-width":this.svg_scaling_factor*ticklabel_stroke_width,
				})
			)
			
			// If we have ticklabels, use them
			if ((this.ticklabels[i]!="") && (this.ticklabels[i] !== null)){
				let svg_ticklabel_container = createSvgElement(
					"g",
					{"class":"ticklabel"}
				)
				let scaled_ticklabel_font_size = this.svg_scaling_factor*ticklabel_font_size
				let text_element = createSvgElement("text", {
					"text-anchor":"middle",
					"font-family":ticklabel_font_family,
					"font-size":scaled_ticklabel_font_size,
				})
				text_element.textContent = this.ticklabels[i]
				
				svg_ticklabel_container.appendChild(text_element)
				this.svg_ticklabels.push(svg_ticklabel_container)
				
				let text_bbox = text_element.getBBox()
				console.log("text_bbox", text_bbox)
				console.log("ticklabel length", this.ticklabels[i].length)
				
				let text_scale = V.from(
					scaled_ticklabel_font_size*1,
					scaled_ticklabel_font_size*0.8*this.ticklabels[i].length*0.5,
				)
				let text_pos_adj = [V.from(0,0), V.from(0,0.3*text_scale[0])]
				
				ticklabel_pos_in_f = V.add(
					tick_pos_in_f,
					V.add(
						V.add(
							tick_delta_in_f, 
							V.scalar_prod(
								V.unit(1-this.dim), 
								this.grow_direction[this.dim]*1.1*text_scale[this.dim]
							)
						),
						text_pos_adj[this.dim]
					)
				)
				
				setAttrsFor(text_element, {
					x:ticklabel_pos_in_f[0], 
					y:ticklabel_pos_in_f[1]
				})
				
			}
		}
		this.svg_group.append(this.svg_label, this.svg_path, ...this.svg_ticks, ...this.svg_ticklabels)
		
	}
	
	styleSvgElements(
			label_style_obj,
			path_style_obj,
			tick_style_obj,
			ticklabel_style_obj
		){
		setAttrsFor(this.svg_label, label_style_obj)
		setAttrsFor(this.svg_path, path_style_obj)
		for(const item of this.svg_ticks){
			setAttrsFor(item, tick_style_obj)
		}
		for(const item of this.svg_ticklabels){
			setAttrsFor(item, ticklabel_style_obj)
		}
	}
	
	svg(){
		return this.svg_group
	}
}



class PlotArea{
	constructor(
		label, // label of plot
		p_rect_in_f = new R(0,0,1,1), // rectangle that defines this plot area in figure coords
	){
		this.label = label
		this.p_rect_in_f = p_rect_in_f
		this.p2f_transform = R.getTransformFrom(p_rect_in_f) // plot area coords always go from (0,0) to (1,1)
		
		this.svg_group = createSvgElement("g", {id:`plot-area ${this.label}`})
		
		this.data_area_map = new Map()
		this.current_data_area = null
	}
	
	addDataArea(data_area){
		data_area.createAxesSvg(this.p2f_transform)
		this.svg_group.appendChild(data_area.svg())
		this.data_area_map.set(data_area.label, data_area)
		this.setCurrentDataArea(data_area.label)
	}
	
	setCurrentDataArea(da_label){
		this.current_data_area = this.data_area_map.get(da_label)
	}
	
	svg(){
		return this.svg_group
	}
	
	add_data_point(x,y){
		this.current_data_area.add_data_point(x,y)
	}
}

class SvgFig{
	constructor(
			label, // label of the figure
			parent_element, // element to attach the svg figure as a child of
			s_scale=V.from(10,10), // scale of svg figure in screen coords
			scale_units='cm', // units of svg figure scale in screen coords
			f_rect_in_f=new R(0,0,1,1), // rectangle that defines how the figure coords relate to the screen coords
			f_rect_units = '', // units of figure coords
		){
		this.label = label
		this.parent_element = parent_element
		this.s_scale = s_scale
		this.scale_units = scale_units
		this.f_rect_in_f = f_rect_in_f
		this.f_rect_units = f_rect_units
		this.plot_area_map = new Map()
		this.current_plot_area = null
		
		this.svg = createSvgElement('svg', {
			width:`${s_scale[0]}${scale_units}`, // width of svg element
			height:`${s_scale[1]}${scale_units}`, // height of svg element
		})
		this.updateViewbox()
		
		this.parent_element.appendChild(this.svg)
	}
	
	updateViewbox(){
		this.svg.setAttribute("viewBox", `${this.f_rect_in_f.r[0]}${this.f_rect_units} ${this.f_rect_in_f.r[1]}${this.f_rect_units} ${this.f_rect_in_f.s[0]}${this.f_rect_units} ${this.f_rect_in_f.s[1]}${this.f_rect_units}`)
	}
	
	addPlotArea(plot_area){
		this.svg.appendChild(plot_area.svg())
		this.plot_area_map.set(plot_area.label, plot_area)
		this.setCurrentPlotArea(plot_area.label)
	}
	
	setCurrentPlotArea(plot_area_label){
		this.current_plot_area = this.plot_area_map.get(plot_area_label)
	}
	
	add_data_point(x,y){
		this.current_plot_area.add_data_point(x,y)
	}
	
}

class SvgLinePlot extends SvgFig{
	constructor(...args){
		super(...args)
		this.line_plot_area = new PlotArea('line_plot_area', new R(0.1,0.1,0.8,0.8))
		this.line_data_area = new DataArea('line_data_area')
		
		this.line_plot_area.addDataArea(this.line_data_area)
		this.addPlotArea(this.line_plot_area)
		
	}
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
		
		// Transforms
		this.d2w_transform = null // transforms data coords to window coords so we can draw them
		
		// Window attributes
		this.w_rect = {x:-1, y:-1, w:14, h:14}
		
		// Axis attributes
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
