
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

// QUESTION: How to hold style information for a graph?

/*
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

*/

/*
RepresentationDimensionDefaultStyles = {
	"colour" : [
		"red",
		"green",
		"blue",
		"purple",
		"brown",
		"yellow",
	],
	"marker" : [
		"circle",
		"square",
		"triangle",
		"cross",
		"dot",
		"plus",
	],
	"line" : [
		"solid",
		"dashed",
		"dotted",
	],
}

class Transformer{
	// Want this to be able to apply matrix and general functional transforms
	// therefore need to chain the transforms together
	//
	// NOTE: If there are no transforms in the chain, should just return input unchanged (identity transform)
	//
	// IDEA: This should possibly be a tree, with each node on the tree being the transform for a specific
	// coord system towards the screen coord system
	
	static combine(t_a2b2c, t_c2d2e){
		// transformer 1 is applied, then transformer 2 in forward direction, reversed for backwards direction
		return new Transformer({
			fwd_transform_chain : ((t_a2b2c===null)? [] : t_a2b2c.fwd_transform_chain).concat((t_c2d2e===null)? [] :t_c2d2e.fwd_transform_chain),
			bck_transform_chain : ((t_c2d2e===null)? [] : t_c2d2e.bck_transform_chain).concat((t_a2b2c===null)? [] : t_a2b2c.bck_transform_chain)
		})
	}
	
	static single(fwd_t, bck_t){
		return new Transformer({
			fwd_transform_chain :[fwd_t],
			bck_transform_chain :[bck_t]
		})
	}
	
	static fromT(t){
		return Transformer.single(
			(...args)=>T.apply(t,args),
			(...args)=>T.iapply(t,args)
		)
	}
	
	constructor({
			fwd_transform_chain = [],
			bck_transform_chain = [],
		} = {}){
		assert_all_defined(fwd_transform_chain, fwd_transform_chain)
		this.fwd_transform_chain = fwd_transform_chain
		this.bck_transform_chain = bck_transform_chain
		
		console.log(this.fwd_transform_chain)
		console.log(this.bck_transform_chain)
		
	}
	
	forward(...args){
		let i=0
		console.log(`transform input: ${args}`)
		for(const t of this.fwd_transform_chain){
			args = t(...args)
			console.log(`transform ${i} result: ${args}`)
			i++
		}
		return args
	}
	
	forward_scale(...args){
		console.log(`forward_scale input ${args}`)
		let a = V.zeros(args.length)
		let b = V.from(...args)
		let [at, bt] = this.block_forward(a, b)
		console.log(`forward_scale result ${V.sub(bt, at)}`)
		return V.sub(bt,at)
	}
	
	backward(...args){
		for(const t of this.bck_transform_chain){
			args = t(...args)
		}
		return args
	}
	
	backward_scale(...args){
		let a = V.zeros(args.length)
		let b = V.from(...args)
		let [at, bt] = this.block_backward(a, b)
		return V.sub(bt,at)
	}
	
	block_forward(...args){
		console.log(`block transform input ${args}`)
		let result = []
		for(const item of args){
			result.push(this.forward(...item))
		}
		console.log(`block transform result ${result}`)
		return result
	}
	
	block_backward(...args){
		let result=[]
		for(const item of args){
			result.push(this.backward(...item))
		}
		return result
	}
	
	block_forward_scale(...args){
		console.log(`block transform scale input ${args}`)
		let result = []
		for(const item of args){
			result.push(this.forward_scale(...item))
		}
		console.log(`block transform scale result ${result}`)
		return result
	}
	
	block_backward_scale(...args){
		let result=[]
		for(const item of args){
			result.push(this.backward_scale(...item))
		}
		return result
	}
}


class DataSet{
	static from(iterable, ...args){
		let dataset = new DataSet(...args)
		dataset.fill_from(iterable)
		return dataset
	}

	constructor({
			field_info = {}, // {"field_name_1":{...info...}, "field_name_2":{...info...}, ...}
			ds2r_transform = new Transformer(),
			n_max_entries = 1024,
			storage_type = Float32Array,
		}={}){
		this.n_fields = field_info.length
		this.field_info = field_info
		
		// from coords the data is in, to arbitrary "representation" coords. In the simplistic case this is the identity transform
		this.ds2r_transform = ds2r_transform
		
		this.n_max_entries = n_max_entries
		this.is_typed_array = false
		this.size = 0 // number of valid entries. NOTE: Each entry consists of 'this.n_fields' fields. So the total number of bits of data is this.n_fields*this.size
		
		if (storage_type.prototype instanceof Object.getPrototypeOf(Uint8Array)){
			this.is_typed_array = true
			this.storage = new storage_type(this.n_fields*this.n_max_entries)
		} 
		else if (storage_type == Array){
			this.storage = []
		}else {
			this.storage = new storage_type(this.n_max_entries)
		}
		
	}
	
	check_size(index=0){
		if (this.size <= index){
			throw new Error(`Cannot set index ${index} at or beyond size ${this.size}`)
		}
		if (this.n_max_entries < this.size){
			throw new Error(`Size ${this.size} is larger than max entries ${this.n_max_entries}`)
		}
	}
	
	*entries(){
		if(this.is_typed_array){
			for(let i=0; i<this.size; ++i){
				yield [i, this.storage.subarray(index*this.n_fields, (index+1)*this.n_fields)]
			}
		} else {
			yield* this.storage.entries()
		}
	}
	
	field(n){
		assert(n < this.n_fields, "Must have n or more fields to request n^th one.")
		
		return Object.entries(this.field_info)[n]
	}
	
	indexOfField(field_name){
		idx = Object.keys(this.field_info).indexOf(field_name)
		assert(idx != -1, "Field name must be present to get its index")
		return idx
	}
	
	*getDataByField(field_name){
		field_index = this.indexOfField(field_name)
		yield* this.getDataByIndex(field_index)
	}
	
	*getDataByIndex(idx){
		if (this.is_typed_array){
			for(let i=0;i<this.size; ++i){
				yield this.storage[i*this.n_fields + idx]
			}
		} else {
			for(const datum of this.storage){
				yield datum[idx]
			}
		}
	}
	
	*fields(){
		yield* Object.entries(this.field_info)
	}
	
	at(index){
		this.check_size(index)
		if (this.is_typed_array){
			return this.storage.subarray(index*this.n_fields, (index+1)*this.n_fields)
		}
		return this.storage[index]
	}
	
	
	
	_set_typed_array(index, value){
		if ((this.n_fields == 1) && (value.length == undefined)){
			this.storage[index] = value
			return
		}
		
		let x = this.storage.subarray(index*this.n_fields, (index+1)*this.n_fields)
		for(const [i,item] of value.entries()){
			x[i] = item
		}
	}
	
	set(index, value){
		this.check_size(index)
		if (this.is_typed_array){
			this._set_typed_array(index, value)
		}
		else {
			this.storage[index] = value
		}
		return this
	}
	
	push(value){
		this.size++
		this.check_size()
		if(this.is_typed_array){
			this._set_typed_array(this.size-1,value)
		} else {
			this.storage.push(value)
		}
		return this
	}
	
	fill_from(iterable){
		for(const item of iterable){
			this.push(item)
		}
	}
}
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
		
		
		
		
		this.svg = new SvgContainer(Svg.group(`group-${this.name}`, null, {}))
		
		this.line_svg_holder = new SvgContainer(this.svg.add("group",`group-${this.name}-axis-line`, null, {}))
		this.label_svg_holder = new SvgContainer(this.svg.add("group",`group-${this.name}-axis-label`, null, {}))
		this.tick_set_svg_holder = new SvgContainer(this.svg.add("group",`group-${this.name}-axis-ticks`, null, {}))
		this.tick_label_set_svg_holder = new SvgContainer(this.svg.add("group",`group-${this.name}-axis-tick-labels`, null, {}))
		
	}
	
	addSvgTo(parent){
		parent.appendChild(this.svg.root)
	}
	
	updateRootTransform(transform){
		this.toRoot_transform = transform
		this.calc()
		this.draw()
	}
	
	calc(rect){
		this.calcLine(rect)
		this.calcLabel()
		this.calcTicks()
		this.calcTickLabelPositions()
		this.calcTickLabels()
	}
	
	draw(){
		this.drawLine()
		this.drawLabel()
		this.drawTicks()
		this.drawTickLabels()
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
	}
	
	drawTickLabels(){
		for(let i=0; i<this.tick_label_set.length; i++){
			this.tick_set_svg_holder.add(
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
	
	registerDataset(dataset_name){
		let idx = this.registered_datasets.indexOf(dataset_name)
		if(idx < 0){
			this.registered_datasets.push(dataset_name)
		}
		this.data_area.registerDataset(dataset_name, this.fromData_transform)
	}
	
	deregisterDataset(dataset_name){
		let idx = this.registered_datasets.indexOf(dataset_name)
		if(idx < 0){
			this.registered_datasets.splice(idx,1)
		}
		this.data_area.deregisterDataset(dataset_name)
	}
	
	drawDataset(dataset){
		let resize = false
		for(const data of dataset.getNewData(this.name)){
			resize = false
			for(const [i,ar_flag] of this.autoresize.entries()){
				if(ar_flag && !(this.extent_in_data_coords[i*2] <= data[i])){
					resize = true
					this.extent_in_data_coords[i*2] = data[i]
				}
				if(ar_flag && !(data[i] <= this.extent_in_data_coords[i*2 + 1])){
					resize = true
					this.extent_in_data_coords[i*2+1] = data[i]
				}
			}
			
			if (resize){
				this.setExtent(this.extent_in_data_coords)
			}
		
			this.data_area.drawData(dataset.name, data)
		}
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
	
	createAxis(name=null, containing_rect = null){
		if(containing_rect===null){
			let v_el = V.unit(this.axis_list.length, this.data_area.dimensions.spatial)
			let v_not_el = V.sub(V.ones(this.data_area.dimensions.spatial), v_el)
			let a = 0.1
			containing_rect = new R(
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
	
	addSvgTo(parent){
		parent.appendChild(this.svg.root)
	}
	
	
	draw(
			rect= new R(0,0,1,1), // rectangle around which to draw axes (in "dataArea" coords)
		){
		this.svg.clear()
		for(const axis of this.axis_list){
			axis.addSvgTo(this.svg.root)
			axis.calc(rect)
			axis.draw(
				this.data_area.frame.toRoot_transform, // transform to go from "dataArea" (unit) coords to "root" (screen) coords
			)
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
		this.dataset_styles = new Map() // styles associated with each datset that is drawn to this DataArea
		this.dataset_markers = new Map() // markers for each dataset 
		this.dataset_svg_holders = new Map() // SVG holders for drawings of datasets
		this.dataset_svg = new Map() // references all SVG for a dataset
		
		this.line_width = 0.3
		this.marker_size = 5
		this.colour_progression = [
			"blue",
			"red",
			"green",
			"purple",
			"brown",
			"orange",
		]
		
	}
	
	getDatasetDrawnData(dataset_name){
		let ddd = this.dataset_drawn_data.get(dataset_name)
		if (ddd === undefined) {
			ddd = []
			this.dataset_drawn_data.set(dataset_name,ddd)
		}
		return ddd
	}
	
	newDatasetMarker(dataset_name){
		// override to customise marker generation
		let dm = new SvgContainer(
				this.defs.add(
				"marker", 
				{
					id : `marker-${dataset_name}`,
					viewBox : "0 0 3 3",
					markerWidth : this.marker_size,
					markerHeight: this.marker_size,
					markerUnits : "strokeWidth",
					orient : 0, //"auto",//"auto-start-reverse",
					refX : 1.5,
					refY: 1.5,
					//preserveAspectRatio : "xMidyMid meet"
				}
			)
		)
			
		dm.add(
			"circle", 
			V.scalar_prod(V.ones(this.dimensions.spatial), 1.5),
			1,
			this.getDatasetStyle(dataset_name).marker_style
		)
		
		return dm
	}
	
	getDatasetMarker(dataset_name){
		let dm = this.dataset_markers.get(dataset_name)
		if(dm === undefined){
			dm = this.newDatasetMarker(dataset_name)
			this.dataset_markers.set(dataset_name, dm)
		}
		return dm
	}
	
	setDatasetTransform(dataset_name, fromData_transform){
		console.log("fromData_transform", fromData_transform)
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
	
	newDatasetStyle(dataset_name){
		// This should be altered to have different style progression when adding new datasets
		let colour = this.colour_progression[this.getDatasetIndex(dataset_name) % this.colour_progression.length]
		let style_info = {
			polyline_style : {
				"marker-start" : `url(#marker-${dataset_name})`,
				"marker-mid" : `url(#marker-${dataset_name})`,
				"marker-end" : `url(#marker-${dataset_name})`,
				"stroke" : colour,
				"stroke-width" : this.line_width,
				"fill" : "none",
			},
			marker_style : {
				"stroke" : "none",//"context-stroke", // "none"
				//"stroke" : colour,
				"fill" : colour,//"context-fill", //colour,
				//"fill" : "none",
				"stroke-width" : 0.2,
				"stroke-opacity" : 1,
			},
		}
		return style_info
	}
	
	getDatasetStyle(dataset_name){
		let ds = this.dataset_styles.get(dataset_name)
		if (ds === undefined){
			ds = this.newDatasetStyle(dataset_name)
			this.dataset_styles.set(dataset_name, ds)
		}
		return ds
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
		this.getDatasetStyle(dataset_name)
		this.getDatasetHolder(dataset_name)
		this.getDatasetMarker(dataset_name)
	}
	
	deregisterDataset(dataset_name){
		this.dataset_transforms.delete(dataset_name)
		this.dataset_indices.delete(dataset_name)
		this.dataset_styles.delete(dataset_name)
		this.dataset_svg_holders.delete(dataset_name)
		this.dataset_markers.delete(dataset_name)
	}
	
	
	clearData(dataset_name){
		let dsh = this.getDatasetHolder(dataset_name)
		if (dsh !== undefined){
			dsh.clear()
		}
	}
	
	redrawData(dataset_name){
		this.clearData(dataset_name)
		let ddd = this.getDatasetDrawnData(dataset_name)
		for(const data of ddd){
			this.drawData(dataset_name, data, false)
		}
	}
	
	drawData(dataset_name, data, record_data = true){
		// Draws a single data point
		// This should be altered to draw different types of graph
		// default is a line plot	
			
		let dataset_style = this.getDatasetStyle(dataset_name)
		let dataset_svg_holder = this.getDatasetHolder(dataset_name)
		
		if(record_data) {
			this.getDatasetDrawnData(dataset_name).push(data)
		}
		data = T.apply(this.getDatasetTransform(dataset_name),data)

		if (dataset_svg_holder.root.children.length == 0){
			//console.log("New polyline")
			dataset_svg_holder.add(
				"polyline",
				data.toString(),
				dataset_style.polyline_style
			)
		} 
		else {
			
			let el = dataset_svg_holder.root.firstChild
			
			// May have to swap this for a DOMPoint at some time in the future
			let p = el.ownerSVGElement.createSVGPoint()
			p.x = data[0]
			p.y = data[1]
			el.points.appendItem(p)
			
			//el.setAttribute("points",  el.points.toString() + (" "+d.toString()))
		}
		
		
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
	
	setAsDataset(data, dataset_name = null){
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
		if(dataset_name === null){
			dataset_name = this.current_dataset
		}
		this.datasets.get(dataset_name).push(data)
		// send a message to axes for dataset telling it to draw the new point
		for(const axes_name of this.axes_for_dataset.get(dataset_name)){
			this.axes.get(axes_name).drawDataset(this.datasets.get(dataset_name))
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



/*

class Representation{
	// This class is responsible for transforming dataset values to the data area coords
	static fromExtent(extent){
		return new Representation({
			r2da_transform : Transformer.fromT(T.invert(E.getTransformFromUnitCoordsTo(extent))),
		})
	}

	constructor({
		r2da_transform = new Transformer(),
		dimension_names = [
			"x",
			"y",
			"size",
			"colour",
			"marker",
			"line",
		],
	}={}){
		// Transform from arbitrary "representation coords" to dataArea coords which run from (0,1) in each dimension
		this.r2da_transform = r2da_transform
		this.dimension_names = dimension_names
		
		
		this.datasets = new Map()
		//this.dataset_field_to_dimension_associations = new Map()
		this.dimension_to_dataset_field_associations = new Map()
		this.dataset_field_pos_indices = new Map()
		
		
		
	}
	
	associateDatasetFieldWithDimension(dataset_name, field_name, dimension_name){
		let current_associations = this.dataset_field_to_dimension_associations.get(dataset_name)
		let new_associations = {field_name : field_name, field_idx : this.datasets.get(dataset_name).indexOfField(field_name)}
		if(current_associations === undefined){
			current_associations = {dimension_name : new_associations}
		} else {
			current_associations[dimension_name] = new_associations
		}
		this.dataset_field_to_dimension_associations.set(dataset_name, current_associations)
	}
	
	getFieldIndexOfDimension(dataset_name, dimension_name){
		let d2f_assoc = this.dimension_to_dataset_field_associations.get(dataset_name)
		if(d2f_assoc === undefined){
			return this.dimension_names.indexOf(dimension_name)
		}
		
		let f_assoc = d2f_assoc[dimension_name]
		assert(f_assoc === undefined, "If we have an association between dimensions and field names, must be able to find passed dimension name")
		
		return f_assoc.field_idx
	}

	
	getDatasetIdx(name){
		return this.datasets.keys().indexOf(name)
	}
	
	addDataset(name, dataset){
		this.datasets.set(name, dataset)
		let first_two_fields = [dataset.field(0).name, dataset.field(1).name]
		for(const [i, field_name] of first_two_fields.entries()){
			this.associateDatasetFieldWithDimension(name, field_name, dimension_names[i])
		}
	}
	
	draw(renderer, transfrom, element_type, positions, scales, invariants, attrs, group_name = null){
		renderer.add(element_type, transfrom.block_forward(...positions), transfrom.block_forward_scale(...scales), invariants, attrs, group_name)
	}
	
	setDatasetPosFieldIndices(dataset_name){
		this.dataset_field_pos_indices.set(dataset_name, [this.getFieldIndexOfDimension(dataset_name, "x"), this.getFieldIndexOfDimension(dataset_name,"y")])
	}
	
	getPosFromData(dataset_name, data){
		let pos_idxs = this.dataset_field_pos_indices.get(dataset_name)
		return V.select_by_idxs(data, pos_idxs)
	}
	
	render(renderer, transform){
		let r2s_transform = Transformer.combine(this.r2da_transform, transform)
		
		// datasets
		for (const [name, dataset] of this.datasets.entries()){
			if(!renderer.hasGroup(name)){
				renderer.addGroup(name)
			}
			for(const data of dataset.entries()){
				renderer.draw
			}
		}
	}
}




class DataArea{
	constructor({
		da_rect_in_p = new R(0.1,0.1,0.8,0.8)
	}={}){
		this.da_rect_in_p = da_rect_in_p
		this.da2p_transform = null
		this.updateTransform()
		
		this.representations = new Map()
		
	}
	
	addRepresentation(name, representation){
		this.representations.set(name, representation)
	}
	
	draw(renderer, transfrom, element_type, positions, scales, invariants, attrs, group_name = null){
		renderer.add(element_type, transfrom.block_forward(...positions), transfrom.block_forward_scale(...scales), invariants, attrs, group_name)
	}
	
	render(renderer, transform){
		let da2s_transform = Transformer.combine(this.da2p_transform, transform)
	
		// Debug box
		this.draw(renderer, transform, "rect", [this.da_rect_in_p.r], [this.da_rect_in_p.s], [], {
			"stroke":"yellow",
			"stroke-width":"0.1",
		})
		
		// representations
		for(const [name, representation] of this.representations.entries()){
			representation.render(
				renderer, 
				da2s_transform
			)
		}
		
	}
	updateTransform(){
		this.da2p_transform = Transformer.fromT(R.getTransformFromUnitCoordsTo(this.da_rect_in_p))
	}

}

class PlotArea{
	constructor({
		p_rect_in_f = new R(0,0,1,1),
	}={}){
		this.p_rect_in_f = p_rect_in_f
		this.p2f_transform = null
		this.updateTransform()
		
		this.data_areas = new Map()
		
	}
	
	addDataArea(name, plot_area){
		this.data_areas.set(name, plot_area)
	}
	
	draw(renderer, transfrom, element_type, positions, scales, invariants, attrs, group_name = null){
		renderer.add(element_type, transfrom.block_forward(...positions), transfrom.block_forward_scale(...scales), invariants, attrs, group_name)
	}
	
	render(renderer, transform){
		let p2s_transform = Transformer.combine(this.p2f_transform, transform)
	
		// Debug box
		this.draw(renderer, transform, "rect", [this.p_rect_in_f.r], [this.p_rect_in_f.s], [], {
			"stroke":"blue",
			"stroke-width":"0.1",
		})
		
		// data_area
		for(const [name, data_area] of this.data_areas.entries()){
			data_area.render(
				renderer, 
				p2s_transform
			)
		}
	}
	
	updateTransform(){
		this.p2f_transform = Transformer.fromT(R.getTransformFromUnitCoordsTo(this.p_rect_in_f))
	}

}

class Figure{

	constructor({
		f_rect_in_d = new R(0.1,0.1,0.8,-0.8), // rectangle that defines the figure position in display coords
	}={}){
		this.f_rect_in_d = f_rect_in_d
		
		this.f2d_transform = null // set in a method
		this.updateTransform()
		
		this.toScreen_transform = new Transformer()
		this.plot_areas = new Map()
		
	}
	
	addPlotArea(name, plot_area){
		this.plot_areas.set(name, plot_area)
	}
	
	draw(renderer, element_type, positions, scales, invariants, attrs, group_name = null){
		renderer.add(element_type, this.toScreen_transform.block_forward(...positions), this.toScreen_transform.block_forward_scale(...scales), invariants, attrs, group_name)
	}
	
	render(renderer){
		console.log(this.f_rect_in_d)
		// Debug box
		this.draw(renderer, "rect", [this.f_rect_in_d.r], [this.f_rect_in_d.s], [], {
			"stroke":"green",
			"stroke-width":"0.1",
		})
		
		
		// plot_area
		for(const [name, plot_area] of this.plot_areas.entries()){
			plot_area.render(
				renderer
			)
		}
	}

	updateTransform(){
		this.f2d_transform = Transformer.fromT(R.getTransformFromUnitCoordsTo(this.f_rect_in_d))
	}

	updateScreenTransform(transform){
		this.toScreen_transform = Transformer.combine(this.f2d_transform, transform)
	}

}

// Maybe split into Display class and SvgRenderer class?
class SvgDisplay{
	constructor({
		renderTarget = null,
		s_scale = V.from(6,4), // display scale
		s_scale_units = "cm", //units of display scale
	}={}){
		
		this.s_scale = s_scale
		this.s_scale_units = s_scale_units
		
		// Unit coords go from zero to one in each dimension
		this.d_rect_in_s = new R(0,this.s_scale[1],this.s_scale[0],-this.s_scale[1])
		//this.d_rect_in_s = new R(0,0,1,1)
		
		this.frame = new ReferenceFrame(
			null,
			R.getTransformFromUnitCoordsTo(this.d_rect_in_s),
		)
		
		this.renderer = new Svg({
			scale : s_scale,
			scale_units : s_scale_units,
		})
		
		this.d2s_transform=null
		this.updateTransform()
		
		this.setRendererTarget(renderTarget)
		
		this.figures = new Map()
		
	}
	
	updateTransform(){
		this.d2s_transform = Transformer.fromT(R.getTransformFromUnitCoordsTo(this.d_rect_in_s))
		//this.d2s_transform = new Transformer()
	}
	
	addFigure(name, figure){
		figure.updateScreenTransform(this.d2s_transform)
		this.figures.set(name, figure)
	}
	
	addFigureFull(name, extent=E.from(0,0,1,1)){
		this.addFigure(
			name,
			new Figure({f_rect_in_d : R.fromExtent(extent)})
		)
	}
	
	addFigureWithAspectRatio(name, aspect_ratio = 0.5, extent=E.from(0,0,1,1)){
		if (aspect_ratio >= 1){
			extent[2]/=aspect_ratio
		} else {
			extent[3]*=aspect_ratio
		}
		
		let scaled_shape = V.scalar_div(V.from(...this.s_scale,...this.s_scale), V.min(this.s_scale))
		console.log("scaled_shape", scaled_shape)
		
		let scaled_extent = V.div(extent, scaled_shape)
		console.log("scaled_extent", scaled_extent)
		let f_rect_in_d_close_to_unit_coords = R.fromExtent(scaled_extent)
		console.log("f_rect_in_d_close_to_unit_coords", f_rect_in_d_close_to_unit_coords)
	
		this.addFigure(
			name,
			new Figure({f_rect_in_d : f_rect_in_d_close_to_unit_coords})
		)
	}
	
	addFigureOnGrid(name, gridspec=V.from(1,1), gridpos=V.from(0,0), unit_rect_in_grid_cell = new R(0.1,0.1,0.8,0.8)){
		// gridspec : V(nx, ny)
		// gridpos : V(ix, iy)
		// NOTE: Grid indices go from top left instead of bottom left
		
		let grid_flow_start = V.from(0,(gridspec[1]-1)/gridspec[1]) // (0, 3/4)
		let grid_flow_dir = V.from(1,-1)
		
		let grid_shape = V.div(V.ones(2), gridspec)
		console.log("grid_shape", grid_shape)
		
		let grid_start =V.prod(grid_flow_dir, V.sub(V.prod(gridpos, grid_shape), grid_flow_start))
		console.log("grid_start", grid_start)
		
		let full_grid_extent = E.from(...grid_start, ...(V.add(grid_start, grid_shape)))
		console.log("full_grid_extent", full_grid_extent)
		
		let full_grid_rect = R.fromExtent(full_grid_extent)
		
		let grid_rect = R.fromUnitRectWithinRect(full_grid_rect, unit_rect_in_grid_cell)
		
		this.addFigure(
			name,
			new Figure({f_rect_in_d : grid_rect})
		)
		
	}
	
	draw(renderer, transfrom, element_type, positions, scales, invariants, attrs, group_name = null){
		renderer.add(element_type, transfrom.block_forward(...positions), transfrom.block_forward_scale(...scales), invariants, attrs, group_name)
	}
	
	render(){
		// Debug box
		this.renderer.add( "rect", [this.d_rect_in_s.r], [this.d_rect_in_s.s], [], {
			"stroke":"red",
			"stroke-width":"0.1",
		})
		
		// figures
		for(const [name, figure] of this.figures.entries()){
			let topright_of_figure = V.add(figure.f_rect_in_d.r, figure.f_rect_in_d.s)
			figure.draw(this.renderer, "text", [V.from(topright_of_figure[0]/2, topright_of_figure[1])], [], [name], {})
			
			figure.render(this.renderer, this.d2s_transform)
		}
	}
	
	setRendererTarget(renderTarget){
		assert_all_defined(renderTarget)
		this.renderer.attachTo(renderTarget)
	}
	
}
*/

/*
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
		
		
		// debugging box
		//let bbox_width = 2E-2
		//let bbox_color = "blue"
		//this.svg_box = this.d_rect_in_f.asSvg({
		//	"class":"data-area-bbox",
		//	"stroke":bbox_color,
		//	"stroke-width":bbox_width,
		//	"fill":"none",
		//	"stroke-opacity":0.3,
		//})
		//this.svg_group.appendChild(this.svg_box)
		
		
		
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
		
		
		// debugging box
		//let bbox_width = 2E-2
		//let bbox_color = "green"
		//this.svg_box = this.p_rect_in_f.asSvg({
		//	"class":"plot-area-bbox",
		//	"stroke":bbox_color,
		//	"stroke-width":bbox_width,
		//	"fill":"none",
		//	"stroke-opacity":0.3,
		//})
		//this.svg_group.appendChild(this.svg_box)
		
		
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
		
		
		// debugging box
		//let bbox_width = 2E-2
		//let bbox_color = "red"
		//this.svg_box = this.f_rect_in_f.asSvg({
		//	"class":"figure-bbox",
		//	"stroke":bbox_color,
		//	"stroke-width":bbox_width,
		//	"fill":"none",
		//	"stroke-opacity":0.3,
		//})
		//this.svg.appendChild(this.svg_box)
		
		
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


*/
