"use strict"

// Requires:
// * vector_ops.js
// * transform_ops.js

class R{
	constructor(x,y,w,h){
		this.r = V.from(x,y)
		this.s = V.from(w,h)
	}
	
	applyTransform(t){
		this.r = T.apply(t, this.r)
		this.s = T.scale(t, this.s)
		return this
	}
	
	static getTransformFrom(a){
		// get a transform from rect a coords to unit rect coords R(0,0,1,1)
		return T.from(a.s[0], a.s[1], -a.r[0],-a.r[1])
	}

	static getTransformFromTo(a, b){
		// Get a transform from rect a coords to rect b coords
		// both a and b have to be in the same coord system
		a2u_transform = R.getTransformFrom(a)
		b2u_transform = R.getTransformFrom(b)
		return T.prod(b2u_transform.invert(), a2u_transform)
	}

	static intersect(r1, s1, r2, s2){
		// r1, r2 are positions of rect corners
		// s1, s2 are width and height of rects.
		//
		// Tests if rect1 and rect2 intersect, returns intersection rect if they do, or null if they dont
		// rect1 : [r1x, r1y, s1w, s1h]
		// rect2 : [r2x, r2y, s2w, s2h]
		
		
		// If both corners of rect1 are on one side of rect 2, then they do not intersect
		if ( 
			((r1[0]+s1[0]) < r2[0])
			|| (r1[0] > (r2[0]+s2[0]))
			|| ((r1[1]+s1[1]) < r2[1])
			|| (r1[1] > (r2[1]+s2[1]))
		){
			return null
		}
		
		const r3 = new Float64Array(2)
		const s3 = new Float64Array(2)
		
		r3[0] = Math.max(r1[0],r2[0])
		r3[1] = Math.max(r1[1], r2[1])
		s3[0] = Math.min(r1[0]+s1[0], r2[0]+s2[0]) - r3[0]
		s3[1] = Math.min(r1[1]+s1[1], r2[1]+s2[1]) - r3[1]
		
		return [r3, s3]
	}
}