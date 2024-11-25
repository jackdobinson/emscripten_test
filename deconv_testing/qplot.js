
// TODO:
// * Have a think about breaking up the dependence of DataArea and Axis upon eachother
// * Want to have some way to draw arbitrary lines on plots e.g. "axvline" from matplotlib
// * Ideally be able to hot-swap between log and non-log axes (plus others if possible)
// * Would be nice to be able to have more than one set of axes on a DataArea
// * Would be nice to plot multiple datasets in their own colour
// * Would be nice to be able to set styles etc. of lines, text, markers etc.


/*
Anatomy of a plot:

Figures hold plot areas
plot areas hold axes and data areas
data areas hold data points

##############################################
# figure                                     #
#                                            #
#    ---------------------------             #
#    | plot_area               |             #
#    |                         |             #
#    |    |axes                |             #
#    |    V                    |             #
#    |   ##   ...........      |             #
#    |   ##   . data    .      |             #
#    |   ##   . Area    .      |             #
#    |   ##   .         .      |             #
#    |   ##   ...........      |             #
#    |   ##                    |             #
#    |   ################      |             #
#    |   ################      |             #
#    |                         |             #
#    ---------------------------             #
#                                            #
#                                            #
##############################################

dataArea represents the area of a plot that we want to
draw a dataset inside. Can be any geometrical shape but usually a rectangle

A dataset is a collection of data. I.e. the (x,y) coords of point data, 
the (category, count) values of categorical data. All data in a dataset
MUST share the same interpretation. I.e. cannot mix (x,y) data with 
(category, count) data within a dataset.

When a dataset is drawn to a dataArea, it's values are transformed from
its "dataset coordinates" to "representation coordinates" to "dataArea coordinates" 

"dataArea coordinates" always run from 0 to 1, with (0,0) at the bottom left and (1,1) at the top right.

"representation coordinates" are an intermediate coordinate system that defines how the axes are drawn.
This allows us to e.g. plot the logarithm of a dataset instead of the dataset itself just by changing 
the "representation coordinate transform". Data that is transformed to the same "representation coordinates"
can SHARE and axis.

The "dataset coordinates" are the raw values of the data in the dataset. In simple cases the transformation
from "dataset coordinates" to "representation coordinates" is the identity transform. However, if we
e.g. have two datasets (A and B) where A is a collection of (time, instrument_A_reading), and B is a collection
of (time, instrument_B_reading), and we know that instrument_A_reading is in units X, but instrument_B_reading is
in units log(X). We can plot them on the same axis by letting the dataset2representation_transform for each dataset be
datasetA2representation_transform = identity, datasetB2representation_transform=(b[0],exp(b[1])).

Therefore, each dataset requires its own dataset2representation_transform.

These coord systems are defined in terms of transforms:
* dataset2representation_transform
* representation2dataArea_transfrom
* dataArea2plotArea_transform
* plotArea2figure_transform

An axis is a visualisation of a "representation coordinate" system. It is placed in plot coordinates,
usually not covering the dataArea. One axis represents the change of one component of a "representation coordinate" system
with the other components held constant. This is usually visuallised as a solid line (the constant part in a 2d coord system),
with tick-marks and tick-labels along its length (marking the varying part in a 2d coord system).

An axis can be placed anywhere, but is best placed at the edges of the dataArea its "representation coordinate" system corresponds to.

A "representation coordinate" system can have multiple visualisations if needed, for example gridding over the dataArea is
another visualisation of a "representation coordinate" system.



*/


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

function formatNumber(a, sig_figs=3, round_to=1E-12){
	if (round_to != 0){
		let diff = (a % round_to)
		a += (diff > round_to/2) ? diff : -diff
	}
	return a.toPrecision(sig_figs)
}

class DataArea{
	
	constructor(
			label,
			p2f_transform, // transform from plot coords to figure coords
			d_rect_in_p = new R(0.2,0.8,0.6,-0.6), // rectangle that defines the data area in plot coords
			d_extent = E.from(-1.5, 2, -1.5, 2), // extent that defines the data area coords
			ax_labels=['x-axis', 'y-axis'], // labels for axes
			ax_auto_resize=[true,true], // should axes resize automatically when data is out of range?
			n_max_data_points=1024, // Maximum number of data points that can be displayed at once
			n_dim_data_points = 2, // number of dimensions the data has
			data_storage_as_ringbuffer = true, // should the data storage work as a ringbuffer?
		){
		this.label = label
		this.p2f_transform = p2f_transform
		this.d_rect_in_p = d_rect_in_p
		this.d_extent = d_extent
		this.ax_labels = ax_labels
		this.ax_auto_resize = ax_auto_resize
		this.n_max_data_points = n_max_data_points
		this.n_dim_data_points = n_dim_data_points
		this.data_storage_as_ringbuffer = data_storage_as_ringbuffer
		
		O.assert_has_attributes(this, 
			"label", 
			"p2f_transform", 
			"d_rect_in_p", 
			"d_extent", 
			"ax_labels"
		)
		
		this.n_displayed_data_points = 0
		this.n_data_points = 0
		// NOTE: data_point_storage stores the ORIGINAL VALUE of the data point, NOT THE TRANSFORMED VALUE
		this.data_point_storage = V.of_size(this.n_dim_data_points*this.n_max_data_points, Float32Array) // don't need much precision
		
		// Unit to plot and unit to data transforms
		this.u2p_transform = R.getTransformFromUnitCoordsTo(this.d_rect_in_p)
		this.u2d_transform = E.getTransformFromUnitCoordsTo(this.d_extent)
		console.log("DataArea: u2d_transform", this.u2d_transform)
		
		// Data to plot transforms
		this.d2p_transform = T.prod(this.u2p_transform, T.invert(this.u2d_transform)) // d2u then u2p
		console.log("DataArea: this.d2p_transform", this.d2p_transform)
		this.p2d_transform = T.invert(this.d2p_transform)
		
		// Data to figure transform
		this.d2f_transform = T.prod(this.p2f_transform, this.d2p_transform)
		this.d_rect_in_f = this.d_rect_in_p.applyTransform(this.p2f_transform)
		
		console.log("DataArea", this.p2f_transform)
		console.log("DataArea", this.d_rect_in_p, this.d_rect_in_f)
		console.log("DataArea d2f_transform", this.d2f_transform)
		
		this.svg_group = createSvgElement("g", {class:"data-area "+this.label})
		this.svg_axes_group = createSvgElement("g", {class:"axes"})
		this.svg_data_group = createSvgElement("g", {class:"data"})
		
		this.svg_group.appendChild(this.svg_axes_group)
		this.svg_group.appendChild(this.svg_data_group)
		
		
		this.svg_scaling_factor =  2E-3*(V.accumulate_sum(V.component_abs(this.d_rect_in_p.s)))
		console.log("svg_scaling_factor", this.svg_scaling_factor)
		
		/*
		// debugging box
		let bbox_width = 2E-2
		let bbox_color = "blue"
		this.svg_box = this.d_rect_in_f.asSvg({
			"class":"data-area-bbox",
			"stroke":bbox_color,
			"stroke-width":bbox_width,
			"fill":"none",
			"stroke-opacity":0.3,
		})
		this.svg_group.appendChild(this.svg_box)
		*/
		
		
		this.axes = this.createAxes(this.ax_labels)
		
		
		this.prev_data_point = null
		
		// data display params
		this.p2f_transform = null
		this.d2f_transform = null
		
		this.marker_size = 2*this.svg_scaling_factor
		this.marker_colour = "blue"
		this.marker_factory = (x,y)=>{
			return createSvgElement("circle", {cx:x,cy:y,r:this.marker_size,fill:this.marker_colour})
		}
		
		this.line_width = this.svg_scaling_factor
		this.line_colour = "blue"
		this.line_factory = (x1,y1,x2,y2)=>{
			return createSvgElement("line", {
				x1:x1,
				y1:y1,
				x2:x2,
				y2:y2,
				stroke:this.line_colour,
				"stroke-width":this.line_width,
			})
		}
	}
	
	svg(){
		return this.svg_group
	}
	
	redrawPoints(){
		console.log("DataArea::redrawPoints()")
		//remove points
		this.svg_data_group.replaceChildren()
		let ringbuffer_full = this.data_storage_as_ringbuffer && (this.n_displayed_data_points == this.n_max_data_points)
		
		//
		
		console.log(this.n_data_points, this.n_displayed_data_points, ringbuffer_full, this.n_max_data_points, this.n_dim_data_points)
		
		let start_idx = 0
		let count = this.n_data_points
		if(ringbuffer_full){
			start_idx = this.n_data_points
			count = this.n_max_data_points
		} else {
		
		}
		
		this.n_data_points = 0
		this.n_displayed_data_points = 0
		this.prev_data_point = null
		
		for(let i=0;i<count;i++){
			let data_point = P.at(this.data_point_storage, (start_idx+i)%this.n_max_data_points, this.n_dim_data_points)
			this.displayDataPoint(this.storeDataPoint(...data_point))
			this.n_displayed_data_points++
		}
		
	}
	
	redrawAxes(){
		console.log("DataArea::redrawAxes()")
		this.svg_axes_group.replaceChildren()
		this.axes = this.createAxes(this.ax_labels)
		this.createAxesSvg()
	}
	
	recalcDataToFigureTransform(){
		this.d2f_transform = T.prod(this.p2f_transform, this.d2p_transform)
		this.d_rect_in_f = this.d_rect_in_p.applyTransform(this.p2f_transform)
		this.redrawAxes()
		this.redrawPoints()
	}
	
	recalcDataToPlotTransforms(){
		this.d2p_transform = T.prod(this.u2p_transform, T.invert(this.u2d_transform)) // d2u then u2p
		this.p2d_transform = T.invert(this.d2p_transform)
	}
	
	setPlotToFigTransform(p2f_transform){
		this.p2f_transform = p2f_transform
		this.recalcDataToFigureTransform()
	}
	
	setRect(d_rect_in_p){
		this.d_rect_in_p = d_rect_in_p
		this.u2p_transform = R.getTransformFromUnitCoordsTo(this.d_rect_in_p)
		this.recalcDataToPlotTransforms()
		this.recalcDataToFigureTransform()
	}
	
	setExtent(d_extent){
		console.log("DataArea::setExtent d_extent", d_extent)
		this.d_extent = d_extent
		this.u2d_transform = E.getTransformFromUnitCoordsTo(this.d_extent)
		this.recalcDataToPlotTransforms()
		this.recalcDataToFigureTransform()
	}
	
	
	add_data_point(...args){
		let do_resize = false
		let new_extent = V.copy(this.d_extent)
		for(const [i, auto_resize_flag] of this.ax_auto_resize.entries()){
			if (! auto_resize_flag){
				continue
			}
			if (new_extent[2*i] > args[i]){
				new_extent[2*i] = args[i]
				do_resize = true
			}
			if (new_extent[2*i+1] < args[i]){
				new_extent[2*i+1] = args[i]
				do_resize = true
			}
		}
		
		if (do_resize){
			this.setExtent(new_extent)
		}
		
	
		//console.log("DataArea::add_data_point()")
		if(this.n_displayed_data_points == this.n_max_data_points){
			if(this.data_storage_as_ringbuffer){
				if (this.n_displayed_data_points == this.n_max_data_points){
					this.svg_data_group.removeChild(this.svg_data_group.firstElementChild)
					this.n_displayed_data_points--
				}
			} else {
				throw new Error("DataArea is displaying too many datapoints at once")
			}
		}
		
		this.displayDataPoint(this.storeDataPoint(...args))
		this.n_displayed_data_points++
	}
	
	storeDataPoint(...args){
		let data_point = P.set(this.data_point_storage, this.n_data_points, args, this.n_dim_data_points)
		console.log("DataArea::storeDataPoint() args, data_point", args, data_point)
		this.n_data_points++
		
		if (this.n_data_points >= this.n_max_data_points){
			if (this.data_storage_as_ringbuffer){
				this.n_data_points -= this.n_max_data_points
			} else {
				throw new Error("DataArea has run out of data storage and we are not using the data storage as a ringbuffer")
			}
		}
		
		return data_point
	}
	
	displayDataPoint(data_point){
		console.log("DataArea::displayDataPoint()")
		
		console.log(this.d2f_transform)
		let transformed_point = T.apply(this.d2f_transform, data_point)
		console.log(data_point, transformed_point)
		
		if (this.marker_factory !== null && this.marker_factory !== undefined){
			this.svg_data_group.appendChild(this.marker_factory(...transformed_point))
		}
		if (this.line_factory !== null && this.line_factory !== undefined && this.prev_data_point !== null){
			this.svg_data_group.appendChild(this.line_factory(...T.apply(this.d2f_transform, this.prev_data_point), ...transformed_point))
		}
		
		this.prev_data_point = data_point
	}
	
	getPlotCoordOfPoint(...args){
		return [...T.apply(this.d2p_transform, args)]
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
			x_fmtr=(a)=>formatNumber(a), 
			y_fmtr=(a)=>formatNumber(a),
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
	
	createAxes(){
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
					this.ax_labels[i],
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
			this.svg_axes_group.append(axis.svg())
		}
		return axes
	}
	
	createAxesSvg(){
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
		this.setRect(p_rect_in_f)
		
		console.log("PlotArea", this.p2f_transform)
		
		this.svg_group = createSvgElement("g", {id:`plot-area ${this.label}`})
		
		/*
		// debugging box
		let bbox_width = 2E-2
		let bbox_color = "green"
		this.svg_box = this.p_rect_in_f.asSvg({
			"class":"plot-area-bbox",
			"stroke":bbox_color,
			"stroke-width":bbox_width,
			"fill":"none",
			"stroke-opacity":0.3,
		})
		this.svg_group.appendChild(this.svg_box)
		*/
		
		this.data_area_map = new Map()
		this.current_data_area = null
	}
	
	setRect(p_rect_in_f){
		this.p_rect_in_f = p_rect_in_f
		this.p2f_transform = R.getTransformFromUnitCoordsTo(p_rect_in_f) // plot area coords always go from (0,0) to (1,1)
	}
	
	addDataArea(data_area){
		data_area.setPlotToFigTransform(this.p2f_transform)
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
	
	addToFigure(svg_fig){
		svg_fig.addPlotArea(this)
	}
}

class SvgFig{
	constructor(
			label, // label of the figure
			parent_element, // element to attach the svg figure as a child of
			s_scale=V.from(10,10), // scale of svg figure in screen coords
			f_rect_in_f=new R(0,0,1,1), // rectangle that defines how the figure coords relate to the screen coords
			scale_units='cm', // units of svg figure scale in screen coords
			
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
		
		/*
		// debugging box
		let bbox_width = 2E-2
		let bbox_color = "red"
		this.svg_box = this.f_rect_in_f.asSvg({
			"class":"figure-bbox",
			"stroke":bbox_color,
			"stroke-width":bbox_width,
			"fill":"none",
			"stroke-opacity":0.3,
		})
		this.svg.appendChild(this.svg_box)
		*/
		
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

class LinePlot extends PlotArea{
	constructor({
			label,
			ax_labels = ['x-axis', 'y-axis'],
			ax_extent = E.from(0,1,0,1),
			p_rect_in_f = new R(0.,0.,1,1),
			svg_fig = null,
			
		}){
		super(
			label, 
			p_rect_in_f
		)
		this.line_data_area = new DataArea(
			'line_data_area', 
			this.p2f_transform,
			new R(0.2,0.8,0.6,-0.6),
			ax_extent,
			ax_labels
		)
		
		this.addDataArea(this.line_data_area)
		
		if (svg_fig !== null){
			this.addToFigure(svg_fig)
		}
		
	}
}

class LogLinePlot extends LinePlot{
	add_data_point(...args){
		this.current_data_area.add_data_point(args[0], Math.log10(args[1]))
	}
}
