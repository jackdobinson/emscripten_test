"use strict"

class V{

	static zero = Float64Array.from([0,0])
	static one =  Float64Array.from([1,1])
	
	static unit(i,n=2){
		let v = new Float64Array(n)
		for (let j=0;j<n;j++){
			if(j==i){
				v[j] = 1
			} else {
				v[j] = 0
			}
		}
		return v
	}

	static of_size(n){
		return new Float64Array(n)
	}

	static from(...args){
		return Float64Array.from(args)
	}
	
	static from_matrix(m){
		return Float64Array.from(m.elements)
	}

	static typed_from(type, ...args){
		// type : Float64Array | Float32Array | Int32Array | Int16Array | Uint32Array | Uint16Array | Uint8Array | ...
		return type.from(args)
	}
	
	static const_of_size(n,c){
		let v = new Float64Array(n)
		for(let i=0; i<n;i++){
			v[i] = c
		}
		return v
	}

	static arange(start, stop, step=1, type=Float64Array){
		const n = Math.ceil((stop-start)/step)
		const r = new type(n)		
		for(let i=0; i<n; i++){
			r[i] = start + i*step
		}
		return r
	}

	static new_like(v1){
		//console.trace()
		return new v1.constructor(v1.length)
	}
	
	static copy(v1){
		const v2 = V.new_like(v1)
		V.assign(v2, v1)
		return v2
	}


	static check_length(v1,v2){
		//console.trace()
		if (v1.length != v2.length){
			Error('Vectors muse be of equal length to assign')
		}
	}


	static assign(v1, v2){
		V.check_length(v1, v2)
		for(let i=0; i<v1.length; i++){
			v1[i] = v2[i]
		}
		return v1
	}

	static set(v1, ...args){
		V.check_length(v1, args)
		for(let i=0; i<v1.length; i++){
			v1[i] = args[i]
		}
		return v1
	}

	static mutate(v1, mutator, predicate=(i,v)=>{return true}){
		// Mutate the elements of v1 (i.e. change in place), using the `mutator` static, only when `predicate` is true.
		// mutator : callable = (index, original_value) -> new_value
		// 		Calculates the `new_value` of v1[index] from the `index` and `original_value` of v1[index]
		// predicate : callable = (index, original_value) -> bool
		// 		When this is true, v1[index] will be mutated using the `mutator` static
		
		
		for(let i=0; i < v1.length; i++){
			if (predicate(i,v1[i])){
				v1[i] = mutator(i,v1[i])
			}
		}
	}

	static component_min(v1,v2){
		// returns the minimum of each component between v1 and v2
		V.check_length(v1, v2)
		const r = V.new_like(v1)
		
		for(let i=0; i<v1.length; i++){
			r[i] = Math.min(v1[i], v2[i])
		}
		return r
	}
	
	static component_abs(v1){
		// returns the absolute value of each component in v1
		const r = V.new_like(v1)
		for(let i=0; i<v1.length; i++){
			r[i] = Math.abs(v1[i])
		}
		return r
	}

	static intersection_rect(v1, v2, r1, s1){
		// v1 - start of a line
		// v2 - end of a line
		// r1 - bottom left of rectangle
		// s1 - size of rectangle
		return R.intersect(V.component_min(v1,v2), V.component_abs(V.sub(v1,v2)), r1, s1)
	}

	static add_many(...args){
		let r = V.new_like(args[0])
		for(const v of args){
			r = V.add_inplace(r, v)
		}
		return r
	}
	
	static add_inplace(v1, v2){
		V.check_length(v1, v2)
		
		for(let i=0; i<v1.length; i++){
			v1[i] += v2[i]
		}
		return v1
	}

	static add(v1, v2){
		V.check_length(v1, v2)
		
		const r = V.new_like(v1)
		for(let i=0; i<v1.length; i++){
			r[i] = v1[i] + v2[i]
		}
		return r
	}

	static sub(v1, v2){
		V.check_length(v1, v2)
		
		const r = V.new_like(v1)
		for(let i=0; i<v1.length; i++){
			r[i] = v1[i] - v2[i]
		}
		return r
	}
	
	static prod(v1, v2){
		V.check_length(v1, v2)
		
		const r = V.new_like(v1)
		for(let i=0; i<v1.length; i++){
			r[i] = v1[i] * v2[i]
		}
		return r
	}
	
	static div(v1, v2){
		V.check_length(v1, v2)
		
		const r = V.new_like(v1)
		for(let i=0; i<v1.length; i++){
			r[i] = v1[i] / v2[i]
		}
		return r
	}

	static dot(v1, v2){
		V.check_length(v1, v2)
		
		let r = 0
		for(let i=0; i<v1.length; i++){
			r += v1[i] * v2[i]
		}
		return r
	}
	
	static mag(v1){
		return Math.sqrt(V.dot(v1,v1))
	}
	
	static norm(v1){
		const r = V.new_like(v1)
		const a = Math.sqrt(V.dot(v1,v1))
		for(let i=0; i<v1.length; i++){
			r[i] = v1[i]/a
		}
		return r
	}
	
	static abs(v1){
		const r = V.new_like(v1)
		for(let i=0; i<v1.length; i++){
			r[i] = Math.abs(v1[i])
		}
		return r
	}
	
	static scalar_prod(v1, s){
		const r = V.new_like(v1)
		for(let i=0; i<v1.length; i++){
			r[i] = v1[i] * s
		}
		return r
	}
	
	static scalar_add(v1, s){
		const r = V.new_like(v1)
		for(let i=0; i<v1.length; i++){
			r[i] = v1[i] + s
		}
		return r
	}
	
	static scalar_sub(v1, s){
		const r = V.new_like(v1)
		for(let i=0; i<v1.length; i++){
			r[i] = v1[i] - s
		}
		return r
	}
	
	static scalar_div(v1, s){
		const r = V.new_like(v1)
		for(let i=0; i<v1.length; i++){
			r[i] = v1[i] / s
		}
		return r
	}

	static accumulate_prod(v1){
		let r = 1
		for(const x of v1){
			r *= x
		}
		return r
	}
	
	static accumulate_sum(v1){
		let r = 0
		for(const x of v1){
			r += x
		}
		return r
	}

}


