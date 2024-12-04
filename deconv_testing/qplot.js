
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

"display coordinates" are in terms of whatever we are drawing on, e.g. a screen or a canvas. This is
the coordinate system that we ultimately convert to.

"figure coordinates" always run from 0 to 1, with (0,0) at the bottom left and (1,1) at the top right.

"plot_area coordinates" always run from 0 to 1, with (0,0) at the bottom left and (1,1) at the top right.

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
* figure2display_transform

An axis is a visualisation of a "representation coordinate" system. It is placed in plot coordinates,
usually not covering the dataArea. One axis represents the change of one component of a "representation coordinate" system
with the other components held constant. This is usually visuallised as a solid line (the constant part in a 2d coord system),
with tick-marks and tick-labels along its length (marking the varying part in a 2d coord system).

An axis can be placed anywhere, but is best placed at the edges of the dataArea its "representation coordinate" system corresponds to.

A "representation coordinate" system can have multiple visualisations if needed, for example gridding over the dataArea is
another visualisation of a "representation coordinate" system.

Annotations on a plot are things like drawing a box around a region, vertical lines to indicate something important, text to
highlight specific datapoints or label datasets etc. Depending on the annotation they will usually be defined in either the
"dataset", "representation", "dataArea", or sometimes "plotArea" coordinate systems.

NOTE: with text annotations, it is often important to define their position in "dataset" coords, but to draw the characters in
"plotArea", "figure" or even "display" coords. This is because generally we do not want to apply transforms to the shapes of the
character glyphs, we only want to apply transforms to where the text is placed.

*/

class Dataset{
	constructor(
		name
	){
		this.name = name
		this.storage = [] // simplest for now
		this.read_heads = new Map() // index of next data to read for axes that read data from dataset
	}
	
	set(data){
		this.storage = data
	}
	
	push(data){
		this.storage.push(data)
	}
	
	resetIterator(axes_name){
		//console.log("Dataset::resetIterator", axes_name)
		this.read_heads.set(axes_name,0)
	}
	
	*getNewData(axes_name){
		let current_index = this.read_heads.get(axes_name)
		if (current_index === undefined){
			current_index = 0
		}
		
		for(; current_index < this.storage.length; current_index++){
			yield this.storage[current_index]
		}
		this.read_heads.set(axes_name, current_index)
	}
	
	*getData(axes_name){
		let current_index = 0
		for(; current_index < this.storage.length; current_index++){
			yield this.storage[current_index]
		}
		this.read_heads.set(axes_name, current_index)
	}
}

class PlotTypeArtist{
	// This object is responsible for creating the SVG representation of a dataset
	// this should be subclassed for different types of plot
	// The default implementation is a line graph
	
	static colour_progression = [
		"blue",
		"red",
		"green",
		"purple",
		"brown",
		"orange",
	]
	
	static n_instances = 0
	
	constructor({
		name = null,
		
	} = {}){
		PlotTypeArtist.n_instances++
	
		this.name = (name===null) ? `plot-type-artist-${PlotTypeArtist.n_instances}` : name
		this.needs_data_init = true
		this.default_primary_colour = PlotTypeArtist.colour_progression[(PlotTypeArtist.n_instances % PlotTypeArtist.colour_progression.length)]
		
		this.svg = new SvgContainer(
			Svg.group(`group-${this.name}`, null, {})
		)
		this.data_svg = new SvgContainer(
			this.svg.add('group',`group-data-${this.name}`, null, {})
		)
	}
	
	clear(){
		this.data_svg.clear()
		this.needs_data_init = true
	}
	
	addSvgTo(parent){
		parent.appendChild(this.svg.root)
	}
	
	drawData(data){
		console.log("PlotTypeArtist::drawData is abstract method")
	}
	
	drawDataIterable(data_iterable){
		for(const item of data_iterable){
			this.drawData(item)
		}
	}
	
}


class LinePlotArtist extends PlotTypeArtist {

	static marker_style_defaults = {
		"stroke" : "none",//"context-stroke", // "none"
		"fill" : null,//"context-fill", //colour,
		"stroke-width" : 0.2,
		"stroke-opacity" : 1,
	}
	
	static line_style_defaults = {
		"marker-start" : null,
		"marker-mid" : null,
		"marker-end" : null,
		"stroke" : null,
		"stroke-width" : 0.3,
		"fill" : "none",
	}

	constructor({
		name = null,
		marker_type = "circle",
		marker_style = {},
		line_style = {},
	}={}){
		super({name:name})
	
		this.marker_type = marker_type
		this.default_marker_name = `marker-${this.name}`
		this.create_default_marker = false
		
		this.setMarkerStyle(marker_style)
		this.setLineStyle(line_style)
	
	
		this.defs = new SvgContainer(this.svg.add("defs"))
		
		
		if(this.create_default_marker){
			this.createMarker()
		}
		
		this.createLine()
	}
	
	createLine(){
		this.needs_data_init = false
		this.line = this.data_svg.add(
			"polyline",
			"",
			this.line_style
		)
	}
	
	setMarkerStyle(marker_style={}){
		this.marker_style = O.insertIfNotPresent(marker_style, LinePlotArtist.marker_style_defaults)
		if (this.marker_style["fill"] === null){
			this.marker_style["fill"] = this.default_primary_colour
		}
	}
	
	setLineStyle(line_style={}){
		this.line_style = O.insertIfNotPresent(line_style, LinePlotArtist.line_style_defaults)
		if (this.line_style["stroke"] === null){
			this.line_style["stroke"] = this.default_primary_colour
		}
		if (this.line_style["marker-start"] === null){
			this.line_style["marker-start"] = `url(#${this.default_marker_name})`
			this.create_default_marker = true
		}
		if (this.line_style["marker-mid"] === null){
			this.line_style["marker-mid"] = `url(#${this.default_marker_name})`
			this.create_default_marker = true
		}
		if (this.line_style["marker-end"] === null){
			this.line_style["marker-end"] = `url(#${this.default_marker_name})`
			this.create_default_marker = true
		}
	}
	
	createMarker(){
		this.default_marker = new SvgContainer(
			this.defs.add(
				"marker", 
				{
					id : this.default_marker_name,
					viewBox : "0 0 3 3",
					markerWidth : 5,
					markerHeight: 5,
					markerUnits : "strokeWidth",
					orient : 0, //"auto",//"auto-start-reverse",
					refX : 1.5,
					refY: 1.5,
				}
			)
		)
		
		switch (this.marker_type) {
			case "circle":
				this.default_marker.add(
					"circle", 
					V.scalar_prod(V.ones(2), 1.5),
					1,
					this.marker_style
				)
				break
			default:
				throw Error(`Unknown marker type "${this.marker_type}" requested`)
		}
	}
	
	drawData(data){
		// May have to swap this for a DOMPoint at some time in the future
		if (this.needs_data_init){
			this.createLine()
		}
		
		//console.log(this.line)
		
		let p = this.line.ownerSVGElement.createSVGPoint()
		//console.log(data[0], data[1])
		//console.log(typeof(data[0]), typeof(data[1]))
		p.x = data[0]
		p.y = data[1]
		this.line.points.appendItem(p)
	}
}


class ReferenceFrame{
	
	static fromRect(parent_frame, this_rect_in_parent){
		return new ReferenceFrame(
			parent_frame,
			R.getTransformFromUnitCoordsTo(this_rect_in_parent)
		)
	}
	
	static fromRectReverse(parent_frame, this_rect_in_parent){
		return new ReferenceFrame(
			parent_frame,
			R.getTransformToUnitCoordsFrom(this_rect_in_parent)
		)
	}
	
	static fromExtent(parent_frame, this_extent_in_parent){
		console.log("this_extent_in_parent", this_extent_in_parent)
		return ReferenceFrame.fromRect(parent_frame, R.fromExtent(this_extent_in_parent))
	}

	static fromExtentReverse(parent_frame, this_extent_in_parent){
		console.log("this_extent_in_parent", this_extent_in_parent)
		return ReferenceFrame.fromRectReverse(parent_frame, R.fromExtent(this_extent_in_parent))
	}
	
	constructor(
		parent_frame = null,
		toParent_transform = T.identity,
		child_frames = []
	){
		assert(parent_frame !== undefined, "parent frame cannot be undefined, but may be null")
		this.parent_frame = parent_frame
		console.log("this.parent_frame", this.parent_frame)
		
		
		
		// transform to previous reference frame
		this.toParent_transform = toParent_transform
		this.child_frames = child_frames
		
		this.toRoot_transform = null
		this.updateRootTransform()
	}
	
	updateRootTransform(){
		this.toRoot_transform = T.prod(
			this.parent_frame === null ? T.identity : this.parent_frame.toRoot_transform, 
			this.toParent_transform
		)
	}
	
	addChild(fromChild_transform){
		this.child_frames.push(new ReferenceFrame(this, fromChild_transform))
	}
	
	addChildFromRect(child_rect_in_this){
		this.child_frames.push(ReferenceFrame.fromRect(this, child_rect_in_this))
	}
	
	addChildFromExtent(child_extent_in_this){
		this.child_frames.push(ReferenceFrame.fromExtent(this, child_extent_in_this))
	}
	
}


class Axis{
	static n_instances = 0
	
	constructor({
		index,
		ndim,
		fromData_transform,
		toRoot_transform,
		containing_rect,
		pos_in_rect = 1,
		name = null,
		
	}={}){
		Axis.n_instances++
		this.name = (name===null) ? `axis_${Axis.n_instances}` : name
		this.index = index
		this.ndim = ndim
		this.fromData_transform = fromData_transform
		this.toRoot_transform = toRoot_transform
		this.containing_rect = containing_rect
		this.pos_in_rect = pos_in_rect
		O.assert_has_attributes(this, "name", "index", "ndim", "fromData_transform", "toRoot_transform", "containing_rect", "pos_in_rect")
		
		this.label_anchor_pos = V.scalar_prod(V.ones(this.ndim),0.5)
		this.label_rotation = this.index*270 // rotation angle of axis lable
		this.display_direction_factors = V.ones(this.ndim)
		this.display_direction_factors[1] = 0
		
		
		
		this.path = null
		this.label_pos = null
		this.tick_set = []
		this.tick_label_pos_set = []
		this.tick_label_set = []
		this.tick_label_anchor_pos = null
		
		
		this.do_redraw_set = new Set() // to be filled with functions that redraw things when a redraw is required
		
		this.svg = new SvgContainer(Svg.group(`group-${this.name}`, null, {}))
		
		this.line_svg_holder = new SvgContainer(this.svg.add("group",`group-${this.name}-axis-line`, null, {}))
		this.label_svg_holder = new SvgContainer(this.svg.add("group",`group-${this.name}-axis-label`, null, {}))
		this.tick_set_svg_holder = new SvgContainer(this.svg.add("group",`group-${this.name}-axis-ticks`, null, {}))
		this.tick_label_set_svg_holder = new SvgContainer(this.svg.add("group",`group-${this.name}-axis-tick-labels`, null, {}))
		
	}
	
	addSvgTo(parent){
		parent.appendChild(this.svg.root)
	}
	
	updateContainingRect(rect){
		this.containing_rect = rect
		this.calcLine()
		
		this.redraw()
	}
	
	updateRootTransform(transform){
		this.toRoot_transform = transform
		
		this.draw() // everything needs to be redrawn if toRoot transform changes
	}
	
	updateFromDataTransform(transform){
		this.fromData_transform = transform
		
		this.calcTickLabels() // only the labels are updated if the fromData transform changes
		this.redraw()
	}
	
	redraw(){
		for(const redraw_callable of this.do_redraw_set){
			redraw_callable()
		}
		this.do_redraw_set.clear()
	}
	
	calc(){
		this.calcLine() // calcLine triggers the others to be calculated too
	}
	
	draw(){
		// Draw everything manually here
		this.drawLine()
		this.drawLabel()
		this.drawTicks()
		this.drawTickLabels()
		
		// as we have just drawn everything, remove everything from redraw_set
		this.do_redraw_set.clear()
	}
	
	calcLine(){
		//console.log("this.containing_rect", this.containing_rect)
		let a = V.const_of_size(this.ndim, this.pos_in_rect)
		a[this.index] = 0
		//console.log("a", a)
		let b = V.unit(this.index, this.ndim)
		//console.log("b", b)
		let start = V.add(V.prod(a,this.containing_rect.s), this.containing_rect.r)
		//console.log("start", start)
		this.path = V.from(
			...start,
			...V.add(start, b)
		)
		//console.log("this.path", this.path)
		this.do_redraw_set.add(this.drawLine.bind(this))
		
		// need to recalculate these as they depend on the line
		this.calcLabel()
		this.calcTicks()
	}
	
	drawLine(attrs={"stroke":"black"}){
		this.line_svg_holder.clear()
		
		this.line_svg_holder.add(
			"line",
			T.apply_block(this.toRoot_transform, this.path),
			attrs
		)
	}
	
	calcLabel(label_offset_dir = -1, label_offset = 0.12){
		
		let delta = V.scalar_prod(V.sub(V.ones(this.ndim), V.unit(this.index, this.ndim)), label_offset_dir*label_offset)
		this.label_pos = V.add(
			V.scalar_div(
				V.add(this.path.subarray(0,this.ndim), this.path.subarray(this.ndim, 2*this.ndim)),
				this.ndim
			),
			delta
		)
		this.do_redraw_set.add(this.drawLabel.bind(this))
	}
	
	drawLabel(){
		let pos = T.apply(this.toRoot_transform, this.label_pos)
		this.label_svg_holder.add(
			"text", 
			pos, 
			this.name, 
			{
				transform : `rotate(${this.label_rotation} ${pos})`,
			}, 
			this.label_anchor_pos
		)
	}
	
	calcTicks(n_ticks=6, tick_length=0.02, tick_direction=-1){
		console.log("this.path", this.path)
		let tick_displacement = V.scalar_prod(V.sub(V.ones(this.ndim),V.unit(this.index, this.ndim)), tick_direction*tick_length)
		console.log("tick_displacement",tick_displacement)
		
		let starts = P.interpolatePointsAlong(this.path, n_ticks, this.ndim)
		console.log("starts", starts)
		let ends = V.block_apply(V.add, starts, tick_displacement, this.ndim)
		console.log("ends", ends)
		
		this.tick_set = []
		for(let i=0;i<n_ticks;i++){
			this.tick_set.push(
				V.from(...starts.subarray(i*this.ndim, (i+1)*this.ndim), ...ends.subarray(i*this.ndim, (i+1)*this.ndim))
			)
		}
		this.do_redraw_set.add(this.drawTicks.bind(this))
		
		// Need to recalculate these as they depend on the ticks
		this.calcTickLabelPositions()
		this.calcTickLabels()
	}
	
	drawTicks(
			attrs={
				stroke:"black",
				"stroke-width":0.2
			}
		){
		this.tick_set_svg_holder.clear() // remove all previous ticks
		
		for(const [i, tick] of this.tick_set.entries()){
			this.tick_set_svg_holder.add(
				"line",
				T.apply_block(this.toRoot_transform, tick),
				attrs
			)
		}
	}
	
	calcTickLabelPositions(offset=0.1){
		this.tick_label_pos_set = []
		this.tick_label_anchor_pos = V.scalar_prod(
			this.display_direction_factors,
			this.pos_in_rect
		)
		this.tick_label_anchor_pos[this.index] = 0.5
		
		console.log("this.tick_label_anchor_pos", this.tick_label_anchor_pos)
		
		for(const [i, tick] of this.tick_set.entries()){
			this.tick_label_pos_set.push(
				V.add(
					tick.subarray(0,this.ndim),
					V.scalar_prod(
						V.sub(tick.subarray(this.ndim,2*this.ndim), tick.subarray(0,this.ndim)),
						1+offset
					)
				)
			)
		}
		this.do_redraw_set.add(this.drawTickLabels.bind(this))
	}
	
	calcTickLabels(){
		this.tick_label_set = []
		
		for(const [i, tick] of this.tick_set.entries()){
			this.tick_label_set.push(
				Svg.formatNumber(
					T.apply(this.fromData_transform, tick)[this.index]
				)
			)
		}
		this.do_redraw_set.add(this.drawTickLabels.bind(this))
	}
	
	drawTickLabels(){
		this.tick_label_set_svg_holder.clear() // remove previous tick labels
		
		for(let i=0; i<this.tick_label_set.length; i++){
			this.tick_label_set_svg_holder.add(
				"text",
				T.apply(this.toRoot_transform, this.tick_label_pos_set[i]),
				this.tick_label_set[i],
				{},
				this.tick_label_anchor_pos
			)
		}
	}
	
}

class AxesSet{
	// Knows about the data coordinate system
	// can transform from data coordinates to "dataArea" (unit) coordinates
	// Need to attach to a data area to be able to visualise axes
	
	static n_instances = 0
	
	constructor({
		name = null,
		//parent_frame = null,
		data_area = null, // place that data is draw into
		extent_in_data_coords = E.from(0,1,0,1),
		autoresize = [true, true], // autoresizing of each axis
	} = {}){
		console.log("name", name)
		
		AxesSet.n_instances++
		this.ndim = extent_in_data_coords.length/2
		this.name = (name===null) ? `axes_set_${AxesSet.n_instances}` : name
		
		this.autoresize = autoresize
		
		this.registered_datasets = []
		this.setExtent(extent_in_data_coords)
		
		//console.log("this.toAxis_transform", toAxis_transform)
		
		this.data_area = null
		
		
		this.svg = new SvgContainer(Svg.group(`group-${this.name}`, null, {}))

		this.axis_list = []
		
		
		this.attachDataArea(data_area)
		
		console.log("this.data_area", this.data_area)
		console.trace()
		
	}
	
	setExtent(extent_in_data_coords){
		console.log("extent_in_data_coords2",extent_in_data_coords)
		assert(this.ndim == extent_in_data_coords.length/2, "New extext must have same number of dimensions as old extent")
		this.extent_in_data_coords = extent_in_data_coords
		this.fromData_transform = R.getTransformFromUnitCoordsTo(R.fromExtent(this.extent_in_data_coords))
		
		if (this.svg !== undefined){
			this.createSpatialAxes()
			this.draw()
		}
		
		if (this.data_area !== null){
			for(const dataset_name of this.registered_datasets){
				this.data_area.setDatasetTransform(dataset_name, this.fromData_transform)
			}
		}
	}
	
	
	updateWhenExtentChanged(
			dimensions_updated, // a list of dimensions that have changed from previous value
		){ // -> bool : true if we needed to perform a redraw, false otherwise
		assert(this.ndim == dimensions_updated.length, "Cannot change number of dimensions of AxesSet after creation")
		let perform_redraw = (V.accumulate_sum(dimensions_updated) > 0)

		//console.log("AxesSet::updateWhenExtentChanged", perform_redraw)

		if (perform_redraw){
			this.fromData_transform = R.getTransformFromUnitCoordsTo(R.fromExtent(this.extent_in_data_coords))
			assert(!T.is_rank_deficient(this.fromData_transform), "this.fromData_transform must be of full rank")
			assert(!V.any_nan(this.fromData_transform), "this.fromData_transform not have any NaN entries")
			//console.log("New transform: ", this.fromData_transform)
			
			for( const[i, dim_updated_flag] of dimensions_updated.entries()){
				if(dim_updated_flag != 0){
					this.axis_list[i].updateFromDataTransform(this.fromData_transform)
				}
				else {
					// No need to bother with redraws if updated transform will not affect this axis
					this.axis_list[i].fromData_transform = this.fromData_transform
				}
			}
			
			if (this.data_area !== null){
				for(const dataset_name of this.registered_datasets){
					this.data_area.setDatasetTransform(dataset_name, this.fromData_transform)
					this.data_area.clearData(dataset_name)
				}
			}
		}
		
		return perform_redraw
	}
	
	registerDataset(dataset_name){
		// Add a new dataset that this axes is responsible for representing
		let idx = this.registered_datasets.indexOf(dataset_name)
		if(idx < 0){
			this.registered_datasets.push(dataset_name)
		}
		this.data_area.registerDataset(dataset_name, this.fromData_transform)
	}
	
	deregisterDataset(dataset_name){
		// Remove a dataset that is axes is responsible for representing
		let idx = this.registered_datasets.indexOf(dataset_name)
		if(idx < 0){
			this.registered_datasets.splice(idx,1)
		}
		this.data_area.deregisterDataset(dataset_name)
	}
	
	drawDataset(dataset){
		// Update component axes if required, and draw dataset on the associated "data_area" of this axis
		//let resize = false
		let resize = V.of_size(this.ndim, Uint8Array) // uninitialised : {1,5,3,6,...} 
		let redraw_flag = false
		for(const data of dataset.getNewData(this.name)){
			resize = V.scalar_prod_inplace(resize, 0) // {0,0,0,...}
			for(const [i,ar_flag] of this.autoresize.entries()){
				if(ar_flag){
					if(!(this.extent_in_data_coords[i*2] <= data[i])){
						resize[i] = 1
						this.extent_in_data_coords[i*2] = data[i]
					}
					if(!(data[i] <= this.extent_in_data_coords[i*2 + 1])){
						resize[i] = 1
						this.extent_in_data_coords[i*2+1] = data[i]
					}
					
					// if our extent has no size, alter it slightly so it is bigger
					// this avoids problems with transforms later
					if (resize[i]==1 &&(this.extent_in_data_coords[i*2] == this.extent_in_data_coords[i*2+1])){
						//console.log("Trying to prevent rank deficient transform")
						this.extent_in_data_coords[i*2] -= 50*Number.EPSILON
						this.extent_in_data_coords[i*2+1] += 50*Number.EPSILON
					}
				}
			}
			
			redraw_flag = redraw_flag || this.updateWhenExtentChanged(resize)
		
			if(!redraw_flag){
				this.data_area.drawData(dataset.name, data)
			}
			
		}
		return redraw_flag
	}
	
	attachDataArea(data_area){
		if(data_area === null){
			console.log("Warning: Attempting to attach null data_area to axes_set")
			return
		}
		this.data_area = data_area
		this.createSpatialAxes()
	}
	
	createSpatialAxes(names = [], containing_rects = []){
		this.axis_list = []
		for(let i=0; i< this.data_area.dimensions.spatial; i++){
			this.createAxis(
				(names.length <= i) ? null : names[i],
				(containing_rects.length <= i) ? null : containing_rects[i],
			)
		}
	}
	
	getDefaultContainingRect(ax_idx){
		let v_el = V.unit(ax_idx, this.data_area.dimensions.spatial)
		let v_not_el = V.sub(V.ones(this.data_area.dimensions.spatial), v_el)
		let a = 0.1
		return new R(
			...V.scalar_prod(
				v_not_el,
				-a
			),
			...V.add(
				v_el,
				V.scalar_prod(v_not_el, a)
			)
		)
	}
	
	createAxis(name=null, containing_rect = null){
		if(containing_rect===null){
			containing_rect = this.getDefaultContainingRect(this.axis_list.length)
		}
		console.log("containing_rect", containing_rect)
		this.axis_list.push(new Axis({
			index : this.axis_list.length,
			ndim : this.data_area.dimensions.spatial,
			fromData_transform : this.fromData_transform, // transform to go from data to "dataArea" (unit) coords
			toRoot_transform : this.data_area.frame.toRoot_transform, // transform to go from "dataArea" (unit) coords to "root" (screen) coords
			containing_rect : containing_rect,
			name : (name === null) ? `axis-${this.axis_list.length}` : name
		}))
	}
	
	checkAxIdx(ax_idx){
		assert_not_null(ax_idx, "Index of axis to update must be a positive non-zero number less than the number of dimensions of the AxesSet")
		assert(Number.isInteger(ax_idx) && (ax_idx < this.ndim), "Index of axis to update must be a positive non-zero number less than the number of dimensions of the AxesSet")
	}
	
	updateAxisContainingRect(ax_idx, containing_rect = null){
		this.checkAxIdx(ax_idx)
		if (containing_rect !== null){
			this.axis_list[ax_idx].updateContainingRect(containing_rect)
		}
	}
	
	addSvgTo(parent){
		parent.appendChild(this.svg.root)
	}
	
	
	draw(){
		this.svg.clear()
		for(const axis of this.axis_list){
			axis.addSvgTo(this.svg.root)
			axis.calc()
			axis.draw()
		}
	}
}


class DataArea{
	static n_instances = 0
		
	constructor({
		name = null,
		parent_frame = null,
		rect_in_parent = new R(0,0,1,1),
		dimensions = {
			spatial: 2,
		},
	} = {}){
		console.log("name", name)
		
		DataArea.n_instances++
		this.name = (name===null) ? `data_area_${PlotArea.n_instances}` : name
		this.rect_in_parent = rect_in_parent
		this.frame = ReferenceFrame.fromRect(parent_frame, this.rect_in_parent)
		this.dimensions = dimensions
		
		console.log("this.frame.toRoot_transform", this.frame.toRoot_transform)
		
		this.svg = new SvgContainer(Svg.group(`group-${this.name}`, null, {}))
		this.defs = new SvgContainer(this.svg.add("defs"))
		
		
		this.next_dataset_index = 0 // dataset index counter
		this.dataset_drawn_data = new Map() // map of dataset with drawn values (original values, not transformed)
		this.dataset_indices = new Map() // map of dataset name to dataset index
		this.dataset_transforms = new Map() // transform from data coords to DataArea coords
		
		this.dataset_plot_type_artists = new Map() // artist for each dataset
		this.dataset_svg_holders = new Map() // SVG holders for drawings of datasets

		
	}
	
	setDatasetPlotTypeArtist(dataset_name, plot_type_artist){
		assert_not_null(dataset_name)
		assert_not_null(plot_type_artist)
		this.dataset_plot_type_artists.set(dataset_name, plot_type_artist)
		plot_type_artist.addSvgTo(this.getDatasetHolder(dataset_name).root)
	}
	
	getDatasetPlotTypeArtist(dataset_name, default_plot_type_artist = null){
		let da = this.dataset_plot_type_artists.get(dataset_name)
		if(da === undefined){
			this.setDatasetPlotTypeArtist(
				dataset_name, 
				(default_plot_type_artist === null) ? new LinePlotArtist() : default_plot_type_artist
			)
			da = this.dataset_plot_type_artists.get(dataset_name)
			assert_not_null(da)
		}
		return da
	}
	
	getDatasetDrawnData(dataset_name){
		let ddd = this.dataset_drawn_data.get(dataset_name)
		if (ddd === undefined) {
			ddd = []
			this.dataset_drawn_data.set(dataset_name,ddd)
		}
		return ddd
	}
	
	setDatasetTransform(dataset_name, fromData_transform){
		//console.log("fromData_transform", fromData_transform)
		this.dataset_transforms.set(
			dataset_name, 
			T.prod(
				this.frame.toRoot_transform,
				T.invert(fromData_transform)
			)
		)
		this.redrawData(dataset_name)
	}
	
	getDatasetTransform(dataset_name){
		return this.dataset_transforms.get(dataset_name)
	}
	
	newDatasetHolder(dataset_name){
		return new SvgContainer(
			this.svg.add(
				"group", 
				`dataset-${dataset_name}`,
				//`matrix(${this.getDatasetTransform(dataset_name)})`,
				null, 
				{}
			)
		)
	}
	
	getDatasetHolder(dataset_name){
		let dataset_svg_holder = this.dataset_svg_holders.get(dataset_name)
		if (dataset_svg_holder === undefined){
			dataset_svg_holder = this.newDatasetHolder(dataset_name)
			this.dataset_svg_holders.set(dataset_name,dataset_svg_holder)
		}
		return dataset_svg_holder
	}
	
	getDatasetIndex(dataset_name){
		let di = this.dataset_indices.get(dataset_name)
		if (di === undefined){
			di = this.next_dataset_index
			this.next_dataset_index++
		}
		return di
	}
	
	registerDataset(dataset_name, fromData_transform){
		this.setDatasetTransform(dataset_name, fromData_transform)
		this.getDatasetIndex(dataset_name)
		this.getDatasetPlotTypeArtist(dataset_name)
		this.getDatasetHolder(dataset_name)
	}
	
	deregisterDataset(dataset_name){
		this.dataset_transforms.delete(dataset_name)
		this.dataset_indices.delete(dataset_name)
		this.dataset_plot_type_artists.delete(dataset_name)
		this.dataset_svg_holders.delete(dataset_name)
	}
	
	
	clearData(dataset_name){
		let da = this.getDatasetPlotTypeArtist(dataset_name)
		if (da !== undefined){
			da.clear()
		}
	}
	
	redrawData(dataset_name){
		this.clearData(dataset_name)
		let ddd = this.getDatasetDrawnData(dataset_name)
		for(const data of ddd){
			this.drawData(dataset_name, data, null, false)
		}
	}
	
	drawData(dataset_name, data, default_plot_type_artist = null, record_data = true){
		//console.log("DataArea::drawData", dataset_name, data, record_data)
		
		if(record_data) {
			this.getDatasetDrawnData(dataset_name).push(data)
		}
		this.getDatasetPlotTypeArtist(dataset_name, default_plot_type_artist).drawData(T.apply(this.getDatasetTransform(dataset_name), data))
		
	}
	
	addSvgTo(parent){
		parent.appendChild(this.svg.root)
	}
	
	draw(){
		this.svg.add(
			"rect",
			T.apply_block(this.frame.toRoot_transform, E.from(0,0,1,1)),
			{stroke:"blue"}
		)
	}
	
	
	
}


class PlotArea{
	static n_instances = 0
	
	constructor({
		name = null,
		parent_frame = null,
		rect_in_parent = new R(0,0,1,1)
	} = {}){
		console.log("name", name)
		
		PlotArea.n_instances++
		this.name = (name===null) ? `plot_area_${PlotArea.n_instances}` : name
		this.rect_in_parent = rect_in_parent
		this.frame = ReferenceFrame.fromRect(parent_frame, this.rect_in_parent)
		
		console.log("this.frame.toRoot_transform", this.frame.toRoot_transform)
		
		this.current_axes = null
		this.current_dataset = null
		this.data_areas = new Map()
		this.axes = new Map()
		this.datasets = new Map()
		this.axes_for_dataset = new Map() // associate a dataset with one or more axes
		
		
		this.svg = new SvgContainer(Svg.group(`group-${this.name}`, null, {}))
		
		
	}
	
	setCurrentAxes(axes_name){
		// Sets axes `axes_name` as the "current axes" i.e. the default unless a different axes is specified
		assert_not_null(axes_name)
		this.current_axes = axes_name
	}
	
	setCurrentDataset(dataset_name){
		// Sets dataset `dataset_name` as the "current_dataset" i.e. the default if a different dataset is not specified
		assert_not_null(dataset_name)
		this.current_dataset = dataset_name
	}
	
	setAsDataset(data, dataset_name = null){
		// Sets dataset `dataset_name` to only contain `data`
		if(dataset_name === null){
			dataset_name = this.current_dataset
		}
		this.datasets.get(dataset_name).set(data)
		// send a message to axes for dataset telling it to draw the new point
		for(const axes_name of this.axes_for_dataset.get(dataset_name)){
			this.axes.get(axes_name).drawDataset(this.datasets.get(dataset_name))
		}
	}
	
	add_data_point(...data){
		this.appendToDataset(data)
	}
	
	appendToDataset(data, dataset_name = null){
		let axes = null
		let dataset = null
		let needs_redraw = false
		
		if(dataset_name === null){
			dataset_name = this.current_dataset
		}
		this.datasets.get(dataset_name).push(data)
		
		
		
		// send a message to axes for dataset telling it to draw the new point
		for(const axes_name of this.axes_for_dataset.get(dataset_name)){
			// if axes.drawDataset(...) returns true, we need to redra
			axes = this.axes.get(axes_name)
			dataset = this.datasets.get(dataset_name)
			needs_redraw = axes.drawDataset(dataset)
			
			//console.log("PlotArea::appendToDataset ", dataset_name, needs_redraw)
			
			if(needs_redraw){
				for(const redraw_dataset_name of axes.registered_datasets){
					//console.log("PlotArea::appendToDataset ", redraw_dataset_name)
					dataset = this.datasets.get(redraw_dataset_name)
					dataset.resetIterator(axes.name)
					axes.drawDataset(dataset)
				}
			}
			
		}
	}
	
	addSvgTo(parent){
		parent.appendChild(this.svg.root)
	}
	
	draw(){
		this.svg.add(
			"rect",
			T.apply_block(this.frame.toRoot_transform, E.from(0,0,1,1)),
			{stroke:"green"}
		)
		this.svg.add(
			"text",
			T.apply(this.frame.toRoot_transform, V.from(0.5,1)),
			this.name,
			{},
			V.from(0.5,0)
		)
	}
	
	addDatasetToAxes(axes_name, dataset){
		this.datasets.set(dataset.name, dataset)
		let afd = this.axes_for_dataset.get(dataset.name)
		if(afd === undefined){
			afd = []
		}
		afd.push(axes_name)
		this.axes_for_dataset.set(dataset.name, afd)
		
		let axes = this.axes.get(axes_name)
		axes.registerDataset(dataset.name)
		this.current_dataset = dataset.name
		this.current_axes = axes
	}
	
	newDatasetForAxes(axes_name, dataset_name = null){
		let ds = new Dataset(
			(dataset_name === null) ? `dataset-${this.datasets.size}` : dataset_name
		)
		this.addDatasetToAxes(axes_name, ds)
		return ds.name
	}
	
	appendDataArea(name, data_area){
		this.data_areas.set(name, data_area)
		data_area.addSvgTo(this.svg.root)
		this.frame.addChild(data_area.frame)
		data_area.draw()
	}
	
	appendDataAreaFromRect(name, d_rect_in_p){
		this.appendDataArea(
			name, 
			new DataArea({
				name : name,
				parent_frame : this.frame,
				rect_in_parent : d_rect_in_p
			})
		)
	}
	
	appendAxes(name, axes){
		this.axes.set(name, axes)
		axes.addSvgTo(this.svg.root)
		this.frame.addChild(axes.frame)
		axes.draw()
		this.current_axes = axes
	}
	
	appendAxesFromExtent(name, extent, data_area = null){
		console.log(name, extent, data_area)
		if(data_area === null){
			if (this.data_areas.size > 0){
				data_area = this.data_areas.values().next().value
			}
		}
		this.appendAxes(
			name,
			new AxesSet({
				name : name,
				parent_frame : this.frame,
				extent_in_data_coords : extent,
				data_area : data_area
			})
		)
	}
}

class Figure{
	constructor({
		container,
		shape = V.from(6,4),
		units = "cm",
		scale = null,
	} = {}){
		
		if (scale === null){
			// Assign scale so the largest value in "shape" (after multiplication by scale) is 100
			scale = 100/V.max(shape)
		}
		
		this.container = container
		// Unit coords go from zero to one in each dimension
		this.aspect_ratio = shape[0]/shape[1] // dx/dy
		this.f_rect_in_s = new R(0,scale*shape[1],scale*shape[0],-scale*shape[1])
		
		this.frame = ReferenceFrame.fromRect(null, this.f_rect_in_s)
		console.log("this.frame.toRoot_transform", this.frame.toRoot_transform)
		
		this.plot_areas = new Map()
		
		this.svg = new SvgContainer(Svg.svg(shape, units, scale, {"style":`font-size:${Math.abs(V.min(V.div(shape,V.from(200, 100))))}${units};`}))
		
		
		
		this.container.appendChild(this.svg.root)
		
		this.draw()
	}
	
	draw(){
		this.svg.add(
			"rect",
			T.apply_block(this.frame.toRoot_transform, E.from(0,0,1,1)),
			{stroke:"red"}
		)
	}
	
	appendPlotArea(name, plot_area){
		this.plot_areas.set(name, plot_area)
		plot_area.addSvgTo(this.svg.root)
		this.frame.addChild(plot_area.frame)
		plot_area.draw()
	}
	
	appendPlotAreaFromRect(name, p_rect_in_f){
		this.appendPlotArea(
			name, 
			new PlotArea({
				name : name,
				parent_frame : this.frame,
				rect_in_parent : p_rect_in_f
			})
		)
	}
	
	
}

