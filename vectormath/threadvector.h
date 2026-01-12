#ifndef VECTORMATH_THREADVECTOR_H
#define VECTORMATH_THREADVECTOR_H

#include "vector/vector3.h"
#include "vector/vector4.h"
#include "vectormath/mat34v.h"

namespace rage {

/* These classes are TLS-safe versions of Vector3, Vector4, etc -- they do not have constructors */

struct ALIGNAS(16) ThreadVector3 
{ 
	float x,y,z,w;
} ;


struct ALIGNAS(16) ThreadVector4 
{ 
	float x,y,z,w; 
	float& operator[](unsigned i) { return (&x)[i]; } 
	bool operator==(const Vector4&that) const { return x==that.x&&y==that.y&&z==that.z&&w==that.w; }
	bool operator!=(const Vector4&that) const { return !operator==(that); }
	void operator=(const Vector4&that) { x=that.x; y=that.y; z=that.z; w=that.w; }
	void Setf(float a,float b,float c,float d) { x=a; y=b; z=c; w=d; }
	void SetW(float d) { w=d; }
	Vector4 asVector4() const { return Vector4(x,y,z,w); }
} ;

struct ALIGNAS(16) ThreadMat34V
{
	ThreadVector3 a, b, c, d;
} ;

}

#endif