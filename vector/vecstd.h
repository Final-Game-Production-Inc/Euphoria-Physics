//
// vector/vecstd.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef VECTOR_VECSTD_H
#define VECTOR_VECSTD_H

#include "math/amath.h"

//////////////// SCALAR VECTOR COMPONENTS (FOR BROADCAST OPERATIONS) ///////////////////

namespace rage {

// Vec_x
// PURPOSE
//   This class is for vector unit operations using only the x component.
//
// <FLAG Component>
class Vec_x { 
	friend class Vec_xyz; 
	friend class Vec_xyzw; 
public:
	// PURPOSE: Make a 4-vector with the given float in its first element.
	// PARAMS:
	//	f -	the value to put in 4-vector's first element
	// RETURN: a 4-vector with the given float in its first element (the other three elements will be ignored)
	static Vec_x Create(float f) { Vec_x v; v.x=f; return v; }

	// PURPOSE: Get the only used element in this 4-vector.
	// RETURN: the only used element in this 4-vector
	operator float() const { return x; }
private:
	float x,y,z,w; 
};


// Vec_y
// PURPOSE
//   This class is for vector unit operations using only the y component.
//
// <FLAG Component>
class Vec_y { 
	friend class Vec_xyz; 
	friend class Vec_xyzw; 
public:
	// PURPOSE: Get the only used element in this 4-vector.
	// RETURN: the only used element in this 4-vector
	operator float() const { return y; }
private:
	float x,y,z,w; 
};


// Vec_z
// PURPOSE
//   This class is for vector unit operations using only the z component.
//
// <FLAG Component>
class Vec_z { 
	friend class Vec_xyz; 
	friend class Vec_xyzw; 
public:
	// PURPOSE: Get the only used element in this 4-vector.
	// RETURN: the only used element in this 4-vector
	operator float() const { return z; }
private:
	float x,y,z,w; 
};


// Vec_w
// PURPOSE
//   This class is for vector unit operations using only the w component.
//
// <FLAG Component>
class Vec_w { 
	friend class Vec_xyz; 
	friend class Vec_xyzw; 
public:
	// PURPOSE: Get the only used element in this 4-vector.
	// RETURN: the only used element in this 4-vector
	operator float() const { return w; }
private:
	float x,y,z,w; 
};


// Vec_xyz
// PURPOSE
//   This class is for vector unit operations using only the first three components.
//
// <FLAG Component>
class Vec_xyz { 
	friend class Mtx_xyz;
	friend class Mtx_xyzw;
public:
	Vec_xyz() { }		//lint !e1401 constructor does not init vars
	Vec_xyz(class datResource&) { } //lint !e1401 constructor does not init vars
	Vec_xyz(float _x,float _y,float _z) : x(_x), y(_y), z(_z), w(1.0f) { }

	//DOM-IGNORE-BEGIN

	static Vec_xyz Create(float a,float b,float c) {
		return Vec_xyz(a,b,c);
	}

	void Set(const Vec_xyz &v) { x=v.x; y=v.y; z=v.z; }

	void Set(float a,float b,float c) { x=a; y=b; z=c; }

	void Zero() { x=y=z=0; }

#define OP(sym,symeq,opcode) \
	inline Vec_xyz operator sym(const Vec_xyz v) const { \
		return Vec_xyz(x sym v.x,y sym v.y,z sym v.z); } \
	inline Vec_xyz operator symeq(const Vec_xyz v) { \
		x symeq v.x; y symeq v.y; z symeq v.z; return *this; }
#define OP2(sym,symeq,opcode,bc) \
	inline Vec_xyz operator sym(const Vec_##bc v) const { \
		return Vec_xyz(x sym v.bc,y sym v.bc,z sym v.bc); } \
	inline Vec_xyz operator symeq(const Vec_##bc v) { \
		x symeq v.bc; y symeq v.bc; z symeq v.bc; return *this; }

#define BCOP(sym,symeq,opcode) \
	OP(sym,symeq,opcode) \
	OP2(sym,symeq,opcode,x) \
	OP2(sym,symeq,opcode,y) \
	OP2(sym,symeq,opcode,z) \
	OP2(sym,symeq,opcode,w)

	BCOP(+,+=,vadd)		//lint !e666 !e1746 expression with side effects passed (not really, just symbols)
	BCOP(-,-=,vsub)		//lint !e666 !e1746 expression with side effects passed (not really, just symbols)
	BCOP(*,*=,vmul)		//lint !e666 !e1746 expression with side effects passed (not really, just symbols)

#undef OP2
#undef OP

	inline Vec_xyz operator-() const { return Vec_xyz(-x,-y,-z); }

	inline Vec_xyz operator~() const {
		float scale = invsqrtf(x*x + y*y + z*z);
		return Vec_xyz(x * scale,y * scale,z * scale);
	}

	inline Vec_xyz operator %(const Vec_xyz &a) const {
		return Vec_xyz(y*a.z-z*a.y,z*a.x-x*a.z,x*a.y-y*a.x);
	}

	void Max(const Vec_xyz &v) {
		x = (x>v.x)? x : v.x;
		y = (y>v.y)? y : v.y;
		z = (z>v.z)? z : v.z;
	}

	void Min(const Vec_xyz &v) {
		x = (x<v.x)? x : v.x;
		y = (y<v.y)? y : v.y;
		z = (z<v.z)? z : v.z;
	}

	inline Vec_x GetX() const { return *(Vec_x*)this; }		//lint !e740 unusual pointer cast
	inline Vec_y GetY() const { return *(Vec_y*)this; }		//lint !e740 unusual pointer cast
	inline Vec_z GetZ() const { return *(Vec_z*)this; }		//lint !e740 unusual pointer cast
	inline Vec_w GetW() const { return *(Vec_w*)this; }		//lint !e740 unusual pointer cast

	void Lerp(float t,const Vec_xyz &a,const Vec_xyz &b) {
		x = (1-t)*a.x + t*b.x;
		y = (1-t)*a.y + t*b.y;
		z = (1-t)*a.z + t*b.z;
	}

	float operator^(const Vec_xyz &v) const {
		return x*v.x + y*v.y + z*v.z;
	}
	float Dot(const Vec_xyz &v) const { return *this ^ v; }


	inline void SetX(float f) { x=f; }
	inline void SetY(float f) { y=f; }
	inline void SetZ(float f) { z=f; }

	void Add(const Vec_xyz &v) { *this += v; }
	void AddScaled(const Vec_xyz &a,const Vec_xyz &b,float s) {
		x = a.x + b.x * s;
		y = a.y + b.y * s;
		z = a.z + b.z * s;
	}
	void AddScaled(const Vec_xyz &b,float s) {
		x += b.x * s;
		y += b.y * s;
		z += b.z * s;
	}
	void Multiply(const Vec_xyz &b) {
		x *= b.x;
		y *= b.y;
		z *= b.z;
	}
	void Subtract(const Vec_xyz &a,const Vec_xyz &b) {
		x = a.x - b.x;
		y = a.y - b.y;
		z = a.z - b.z;
	}

	//DOM-IGNORE-END

private:
	float x, y, z, w; 
};


// Mtx_xyz
// PURPOSE
//   This class is a 3x4 matrix using the first three elements of each of four vector unit 4-vectors.
//
// <FLAG Component>
class Mtx_xyz {
public:
	inline Vec_xyz Transform(const Vec_xyz &v) const {
		return Vec_xyz(v.x*a.x + v.y*b.x + v.z*c.x + d.x,
			v.x*a.y + v.y*b.y + v.z*c.y + d.y,
			v.x*a.z + v.y*b.z + v.z*c.z + d.z);
	}
	inline Vec_xyz Transform3x3(const Vec_xyz &v) const {
		return Vec_xyz(v.x*a.x + v.y*b.x + v.z*c.x,
			v.x*a.y + v.y*b.y + v.z*c.y,
			v.x*a.z + v.y*b.z + v.z*c.z);
	}
	void Dot(const Mtx_xyz &m,const Mtx_xyz &n) {
		a = n.Transform3x3(m.a);
		b = n.Transform3x3(m.b);
		c = n.Transform3x3(m.c);
		d = n.Transform(m.d);
	}
	void Identity() {
		a.Set(1.0f,0.0f,0.0f); 
		b.Set(0.0f,1.0f,0.0f); 
		c.Set(0.0f,0.0f,1.0f); 
		d.Zero();
	}
	void Identity3x3() {
		a.Set(1.0f,0.0f,0.0f); 
		b.Set(0.0f,1.0f,0.0f); 
		c.Set(0.0f,0.0f,1.0f); 
	}
	void FromEulersXZY(const Vec_xyz&);
	Vec_xyz a, b, c, d;
};


// Vec_xyzw
// PURPOSE
//   This class is for vector unit operations using all four components.
//
// <FLAG Component>
class Vec_xyzw {
	friend class Mtx_xyzw;
	Vec_xyzw(float a,float b,float c,float d) :x(a),y(b),z(c),w(d) { }  //lint --e{1704} constructor has private access (use create)
public:
	Vec_xyzw() { }		//lint !e1401 constructor does not init vars

	//DOM-IGNORE-BEGIN

	static Vec_xyzw Create(float a,float b,float c,float d) {
		return Vec_xyzw(a,b,c,d);
	}
	void Set(float a,float b,float c,float d) { *this = Create(a,b,c,d); }	//lint !e1762 method could be made const (it can't because of assignemnt)
	void Zero() { x=y=z=w=0; }

	float DistanceToPlane(const Vec_xyz &v) const { return x*v.GetX()+y*v.GetY()+z*v.GetZ()-w; }

#define OP(sym,symeq,opcode) \
	inline Vec_xyzw operator sym(const Vec_xyzw v) const { \
		return Vec_xyzw(x sym v.x,y sym v.y,z sym v.z,w sym v.w); } \
	inline Vec_xyzw operator symeq(const Vec_xyzw v) { \
		x symeq v.x; y symeq v.y; z symeq v.z; w symeq v.w; return *this; }
#define OP2(sym,symeq,opcode,bc) \
	inline Vec_xyzw operator sym(const Vec_##bc v) const { \
		return Vec_xyzw(x sym v.bc,y sym v.bc,z sym v.bc,w sym v.bc); } \
	inline Vec_xyzw operator symeq(const Vec_##bc v) { \
		x symeq v.bc; y symeq v.bc; z symeq v.bc; w symeq v.bc; return *this; }

	BCOP(+,+=,vadd)		//lint !e666 !e1746 expression with side effects passed (not really they are just symbols), make all operator functions const
	BCOP(-,-=,vsub)		//lint !e666 !e1746 expression with side effects passed (not really they are just symbols), make all operator functions const
	BCOP(*,*=,vmul)		//lint !e666 !e1746 expression with side effects passed (not really they are just symbols), make all operator functions const

#undef OP2
#undef OP

	inline Vec_x GetX() const { return *(Vec_x*)this; }		//lint !e740 unusual pointer cast
	inline Vec_y GetY() const { return *(Vec_y*)this; }		//lint !e740 unusual pointer cast
	inline Vec_z GetZ() const { return *(Vec_z*)this; }		//lint !e740 unusual pointer cast
	inline Vec_w GetW() const { return *(Vec_w*)this; }		//lint !e740 unusual pointer cast

	u32 PackRgba() {
		return ((u32)( (u8)(z*255.0f) | ((u8)(y*255.0f)<<8) | ((u8)(x*255.0f)<<16) | ((u8)(w*255.0f)<<24)));
	}

	void AddScaled(const Vec_xyzw &b,float s) {
		x += b.x * s;
		y += b.y * s;
		z += b.z * s;
		w += b.w * s;
	}
	void AddScaled(const Vec_xyzw &a,const Vec_xyzw &b,float s) {
		x = a.x + b.x * s;
		y = a.y + b.y * s;
		z = a.z + b.z * s;
		w = a.w + b.w * s;
	}

	//DOM-IGNORE-END

private:
	float x,y,z,w;
};


// Quat_xyzw
// PURPOSE
//   This class is a vector unit quaternion using all four elements.
//
// <FLAG Component>
class Quat_xyzw {
	Quat_xyzw(float a,float b,float c,float d) :x(a),y(b),z(c),w(d) { }
public:		//lint !e1704 constructor has private access (use create)
	Quat_xyzw() { }		//lint !e1401 constructor does not init vars
	static Quat_xyzw Create(float a,float b,float c,float d) {
		return Quat_xyzw(a,b,c,d);
	}
	void Set(float a,float b,float c,float d) { *this = Create(a,b,c,d); }  //lint !e1762  method could be made const (no it cant)
	inline Vec_x GetX() const { return *(Vec_x*)this; }		//lint !e740 unusual pointer cast
	inline Vec_y GetY() const { return *(Vec_y*)this; }		//lint !e740 unusual pointer cast
	inline Vec_z GetZ() const { return *(Vec_z*)this; }		//lint !e740 unusual pointer cast
	inline Vec_w GetW() const { return *(Vec_w*)this; }		//lint !e740 unusual pointer cast

	void Multiply(const Quat_xyzw &q1,const Quat_xyzw &q2) {
		float tw=q1.w*q2.w-q1.x*q2.x-q1.y*q2.y-q1.z*q2.z;
		float tx=q1.w*q2.x+q2.w*q1.x+q1.y*q2.z-q1.z*q2.y;
		float ty=q1.w*q2.y+q2.w*q1.y+q1.z*q2.x-q1.x*q2.z;
		float tz=q1.w*q2.z+q2.w*q1.z+q1.x*q2.y-q1.y*q2.x;
		w=tw; x=tx; y=ty; z=tz;
	}
	void Multiply(const Quat_xyzw &b) {
		this->Multiply(*this,b);
	}
	void Normalize() {
		float scale = invsqrtf(x*x + y*y + z*z + w*w);
		x *= scale; y *= scale; z *= scale; w *= scale;
	}
	void FromRotation(const Vec_xyz &axis,float cosHalfAngle,float sinHalfAngle) {
		Set(axis.GetX()*sinHalfAngle,axis.GetY()*sinHalfAngle,axis.GetZ()*sinHalfAngle,cosHalfAngle);
	}

private:
	float x,y,z,w;
};


// Mtx_xyzw
// PURPOSE
//   This class is a 4x4 matrix using four vector unit 4-vectors.
//
// <FLAG Component>
class Mtx_xyzw {
public:
	inline Vec_xyzw Transform(const Vec_xyzw &v) const {
		return Vec_xyzw::Create(v.x*a.x + v.y*b.x + v.z*c.x + v.w*d.x,
			v.x*a.y + v.y*b.y + v.z*c.y + v.w*d.y,
			v.x*a.z + v.y*b.z + v.z*c.z + v.w*d.z,
			v.x*a.w + v.y*b.w + v.z*c.w + v.w*d.w);
	}
	void Dot(const Mtx_xyzw &m,const Mtx_xyzw &n) {
		a = n.Transform(m.a);
		b = n.Transform(m.b);
		c = n.Transform(m.c);
		d = n.Transform(m.d);
	}
	void Identity() {
		a.Set(1.0f,0.0f,0.0f,0.0f); 
		b.Set(0.0f,1.0f,0.0f,0.0f); 
		c.Set(0.0f,0.0f,1.0f,0.0f); 
		d.Set(0.0f,0.0f,0.0f,1.0f);
	}
	void Identity3x3() {
		a.Set(1.0f,0.0f,0.0f,0.0f); 
		b.Set(0.0f,1.0f,0.0f,0.0f); 
		c.Set(0.0f,0.0f,1.0f,0.0f); 
	}
	Vec_xyzw a, b, c, d;
};

}	// namespace rage

#endif // VECTOR_VECSTD_H
