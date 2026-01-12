//
// vector/matrix44.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "matrix44.h"
#include "vector/matrix34.h"
#include "data/struct.h"

using namespace rage;

Matrix44::Matrix44(class datResource&)
{
}

#if __DECLARESTRUCT
void Matrix44::DeclareStruct(datTypeStruct &s)
{
	STRUCT_BEGIN(Matrix44);
	STRUCT_FIELD(a);
	STRUCT_FIELD(b);
	STRUCT_FIELD(c);
	STRUCT_FIELD(d);
	STRUCT_END();
}
#endif

const Matrix44 rage::M44_IDENTITY
	(1.0f,0.0f,0.0f,0.0f
	,0.0f,1.0f,0.0f,0.0f
	,0.0f,0.0f,1.0f,0.0f
	,0.0f,0.0f,0.0f,1.0f);


/*
Purpose: Set the current vector to the transformation of another vector by a matrix.
Parameters:
 v - The vector being transformed.
 mtx - The matrix doing the transforming.
Return: the current vector, after being transformed.
*/
Vector4& Vector4::Dot(const Vector4 &v,const Matrix44 &mtx)
{
	mtx.Transform( v, *this );

	return *this;
}


/*
Purpose: Set the current vector to the transformation of another vector by the 3x3 part of a matrix.
Parameters:
 v - The vector being transformed.
mtx - The matrix whose 3x3 part is doing the transforming.
Return: the current vector, after being transformed.
*/
Vector4& Vector4::Dot3x3(const Vector4 &v,const Matrix44 &mtx)
{
	mtx.Transform3x3( (const Vector3 &)(v), (Vector3 &)(*this) );

	return *this;
}


////////////////////////////////////////////////////////////////////////////////

void Matrix44::FastInverse(const Matrix44 &m)
{
	register float rx(m.d.x),ry(m.d.y),rz(m.d.z);
	register float r,rd;

	r=m.a.x; rd =r*rx; a.x = r; 
	r=m.a.y; rd+=r*ry; b.x = r; 
	r=m.a.z; rd+=r*rz; c.x = r;
					d.x = -rd;

	r=m.b.x; rd =r*rx; a.y = r; 
	r=m.b.y; rd+=r*ry; b.y = r; 
	r=m.b.z; rd+=r*rz; c.y = r; 
					d.y = -rd;

	r=m.c.x; rd =r*rx; a.z = r; 
	r=m.c.y; rd+=r*ry; b.z = r; 
	r=m.c.z; rd+=r*rz; c.z = r;
					d.z = -rd;

	a.w = b.w = c.w = 0;
	d.w = 1;
}

void Matrix44::FastInverse(const Matrix34 &m)
{
	register float rx(m.d.x),ry(m.d.y),rz(m.d.z);
	register float r,rd;

	r=m.a.x; rd =r*rx; a.x = r; 
	r=m.a.y; rd+=r*ry; b.x = r; 
	r=m.a.z; rd+=r*rz; c.x = r;
					d.x = -rd;

	r=m.b.x; rd =r*rx; a.y = r; 
	r=m.b.y; rd+=r*ry; b.y = r; 
	r=m.b.z; rd+=r*rz; c.y = r; 
					d.y = -rd;

	r=m.c.x; rd =r*rx; a.z = r; 
	r=m.c.y; rd+=r*ry; b.z = r; 
	r=m.c.z; rd+=r*rz; c.z = r;
					d.z = -rd;

	a.w = b.w = c.w = 0;
	d.w = 1;
}

void Matrix44::FastInverseScaled(const Matrix44 &m)
{
	register float rx(m.d.x),ry(m.d.y),rz(m.d.z);
	register float r,rd;

	register float invScale = invsqrtf(m.a.x*m.a.x+ m.a.y*m.a.y+m.a.z*m.a.z);
	register float invScale2 = invScale*invScale;

	r=m.a.x*invScale2; rd =r*rx; a.x = r; 
	r=m.a.y*invScale2; rd+=r*ry; b.x = r; 
	r=m.a.z*invScale2; rd+=r*rz; c.x = r;
							     d.x = -rd;

	r=m.b.x*invScale2; rd =r*rx; a.y = r; 
	r=m.b.y*invScale2; rd+=r*ry; b.y = r; 
	r=m.b.z*invScale2; rd+=r*rz; c.y = r; 
								 d.y = -rd;

	r=m.c.x*invScale2; rd =r*rx; a.z = r; 
	r=m.c.y*invScale2; rd+=r*ry; b.z = r; 
	r=m.c.z*invScale2; rd+=r*rz; c.z = r;
								 d.z = -rd;

	a.w = b.w = c.w = 0;
	d.w = 1;
}



inline float Det33(float ax,float ay,float az,float bx,float by,float bz,float cx,float cy,float cz) 
{
	return(ax*by*cz+ay*bz*cx+az*bx*cy-ax*bz*cy-ay*bx*cz-az*by*cx);
}


float Matrix44::Determinant() const
{
	float det=0.0f;
	det+=a.x*Det33(b.y,b.z,b.w,  c.y,c.z,c.w,  d.y,d.z,d.w);
	det-=a.y*Det33(b.x,b.z,b.w,  c.x,c.z,c.w,  d.x,d.z,d.w);
	det+=a.z*Det33(b.x,b.y,b.w,  c.x,c.y,c.w,  d.x,d.y,d.w);
	det-=a.w*Det33(b.x,b.y,b.z,  c.x,c.y,c.z,  d.x,d.y,d.z);
	return(det);
}


void Matrix44::InvertTo(Matrix44 &mtx) const
{
	mthAssertf (this != &mtx , "Matrix44::InvertTo(mtx) can't handle this == &mtx");  //lint !e506 constant value boolean
	float Det=Determinant();
#if __DEV
	if(Det==0.0f) {
		mthWarningf("Matrix44::InvertTo()- Matrix not invertible");
		return;
	}
#endif
	Det=1.0f/Det;
	mtx.a.x= Det33(b.y,b.z,b.w,  c.y,c.z,c.w,  d.y,d.z,d.w)*Det;
	mtx.b.x=-Det33(b.x,b.z,b.w,  c.x,c.z,c.w,  d.x,d.z,d.w)*Det;
	mtx.c.x= Det33(b.x,b.y,b.w,  c.x,c.y,c.w,  d.x,d.y,d.w)*Det;
	mtx.d.x=-Det33(b.x,b.y,b.z,  c.x,c.y,c.z,  d.x,d.y,d.z)*Det;

	mtx.a.y=-Det33(a.y,a.z,a.w,  c.y,c.z,c.w,  d.y,d.z,d.w)*Det;
	mtx.b.y= Det33(a.x,a.z,a.w,  c.x,c.z,c.w,  d.x,d.z,d.w)*Det;
	mtx.c.y=-Det33(a.x,a.y,a.w,  c.x,c.y,c.w,  d.x,d.y,d.w)*Det;
	mtx.d.y= Det33(a.x,a.y,a.z,  c.x,c.y,c.z,  d.x,d.y,d.z)*Det;

	mtx.a.z= Det33(a.y,a.z,a.w,  b.y,b.z,b.w,  d.y,d.z,d.w)*Det;
	mtx.b.z=-Det33(a.x,a.z,a.w,  b.x,b.z,b.w,  d.x,d.z,d.w)*Det;
	mtx.c.z= Det33(a.x,a.y,a.w,  b.x,b.y,b.w,  d.x,d.y,d.w)*Det;
	mtx.d.z=-Det33(a.x,a.y,a.z,  b.x,b.y,b.z,  d.x,d.y,d.z)*Det;

	mtx.a.w=-Det33(a.y,a.z,a.w,  b.y,b.z,b.w,  c.y,c.z,c.w)*Det;
	mtx.b.w= Det33(a.x,a.z,a.w,  b.x,b.z,b.w,  c.x,c.z,c.w)*Det;
	mtx.c.w=-Det33(a.x,a.y,a.w,  b.x,b.y,b.w,  c.x,c.y,c.w)*Det;
	mtx.d.w= Det33(a.x,a.y,a.z,  b.x,b.y,b.z,  c.x,c.y,c.z)*Det;
}


void Matrix44::Inverse()
{
	float Det=Determinant();
#if __DEV
	if(Det==0.0f)
	{
		mthWarningf("Matrix44::Inverse()- Matrix not invertible");
		return;
	}
#endif
	Det=1.0f/Det;

	Matrix44 temp;

	temp.a.x= Det33(b.y,b.z,b.w,  c.y,c.z,c.w,  d.y,d.z,d.w)*Det;
	temp.b.x=-Det33(b.x,b.z,b.w,  c.x,c.z,c.w,  d.x,d.z,d.w)*Det;
	temp.c.x= Det33(b.x,b.y,b.w,  c.x,c.y,c.w,  d.x,d.y,d.w)*Det;
	temp.d.x=-Det33(b.x,b.y,b.z,  c.x,c.y,c.z,  d.x,d.y,d.z)*Det;

	temp.a.y=-Det33(a.y,a.z,a.w,  c.y,c.z,c.w,  d.y,d.z,d.w)*Det;
	temp.b.y= Det33(a.x,a.z,a.w,  c.x,c.z,c.w,  d.x,d.z,d.w)*Det;
	temp.c.y=-Det33(a.x,a.y,a.w,  c.x,c.y,c.w,  d.x,d.y,d.w)*Det;
	temp.d.y= Det33(a.x,a.y,a.z,  c.x,c.y,c.z,  d.x,d.y,d.z)*Det;

	temp.a.z= Det33(a.y,a.z,a.w,  b.y,b.z,b.w,  d.y,d.z,d.w)*Det;
	temp.b.z=-Det33(a.x,a.z,a.w,  b.x,b.z,b.w,  d.x,d.z,d.w)*Det;
	temp.c.z= Det33(a.x,a.y,a.w,  b.x,b.y,b.w,  d.x,d.y,d.w)*Det;
	temp.d.z=-Det33(a.x,a.y,a.z,  b.x,b.y,b.z,  d.x,d.y,d.z)*Det;

	temp.a.w=-Det33(a.y,a.z,a.w,  b.y,b.z,b.w,  c.y,c.z,c.w)*Det;
	temp.b.w= Det33(a.x,a.z,a.w,  b.x,b.z,b.w,  c.x,c.z,c.w)*Det;
	temp.c.w=-Det33(a.x,a.y,a.w,  b.x,b.y,b.w,  c.x,c.y,c.w)*Det;
	temp.d.w= Det33(a.x,a.y,a.z,  b.x,b.y,b.z,  c.x,c.y,c.z)*Det;

	Set (temp);
}


void Matrix44::Inverse(const Matrix44 &mtx)
{
	mthAssertf (this != &mtx , "Matrix44::Inverse(mtx) can't handle this == &mtx");	//lint !e506 constant value boolean
	float Det=mtx.Determinant();
#if __DEV
	if(Det==0.0f)
	{
		mthWarningf("Matrix44::Inverse()- Matrix not invertible");
		return;
	}
#endif
	Det=1.0f/Det;
	a.x= Det33(mtx.b.y,mtx.b.z,mtx.b.w,  mtx.c.y,mtx.c.z,mtx.c.w,  mtx.d.y,mtx.d.z,mtx.d.w)*Det;
	b.x=-Det33(mtx.b.x,mtx.b.z,mtx.b.w,  mtx.c.x,mtx.c.z,mtx.c.w,  mtx.d.x,mtx.d.z,mtx.d.w)*Det;
	c.x= Det33(mtx.b.x,mtx.b.y,mtx.b.w,  mtx.c.x,mtx.c.y,mtx.c.w,  mtx.d.x,mtx.d.y,mtx.d.w)*Det;
	d.x=-Det33(mtx.b.x,mtx.b.y,mtx.b.z,  mtx.c.x,mtx.c.y,mtx.c.z,  mtx.d.x,mtx.d.y,mtx.d.z)*Det;

	a.y=-Det33(mtx.a.y,mtx.a.z,mtx.a.w,  mtx.c.y,mtx.c.z,mtx.c.w,  mtx.d.y,mtx.d.z,mtx.d.w)*Det;
	b.y= Det33(mtx.a.x,mtx.a.z,mtx.a.w,  mtx.c.x,mtx.c.z,mtx.c.w,  mtx.d.x,mtx.d.z,mtx.d.w)*Det;
	c.y=-Det33(mtx.a.x,mtx.a.y,mtx.a.w,  mtx.c.x,mtx.c.y,mtx.c.w,  mtx.d.x,mtx.d.y,mtx.d.w)*Det;
	d.y= Det33(mtx.a.x,mtx.a.y,mtx.a.z,  mtx.c.x,mtx.c.y,mtx.c.z,  mtx.d.x,mtx.d.y,mtx.d.z)*Det;

	a.z= Det33(mtx.a.y,mtx.a.z,mtx.a.w,  mtx.b.y,mtx.b.z,mtx.b.w,  mtx.d.y,mtx.d.z,mtx.d.w)*Det;
	b.z=-Det33(mtx.a.x,mtx.a.z,mtx.a.w,  mtx.b.x,mtx.b.z,mtx.b.w,  mtx.d.x,mtx.d.z,mtx.d.w)*Det;
	c.z= Det33(mtx.a.x,mtx.a.y,mtx.a.w,  mtx.b.x,mtx.b.y,mtx.b.w,  mtx.d.x,mtx.d.y,mtx.d.w)*Det;
	d.z=-Det33(mtx.a.x,mtx.a.y,mtx.a.z,  mtx.b.x,mtx.b.y,mtx.b.z,  mtx.d.x,mtx.d.y,mtx.d.z)*Det;

	a.w=-Det33(mtx.a.y,mtx.a.z,mtx.a.w,  mtx.b.y,mtx.b.z,mtx.b.w,  mtx.c.y,mtx.c.z,mtx.c.w)*Det;
	b.w= Det33(mtx.a.x,mtx.a.z,mtx.a.w,  mtx.b.x,mtx.b.z,mtx.b.w,  mtx.c.x,mtx.c.z,mtx.c.w)*Det;
	c.w=-Det33(mtx.a.x,mtx.a.y,mtx.a.w,  mtx.b.x,mtx.b.y,mtx.b.w,  mtx.c.x,mtx.c.y,mtx.c.w)*Det;
	d.w= Det33(mtx.a.x,mtx.a.y,mtx.a.z,  mtx.b.x,mtx.b.y,mtx.b.z,  mtx.c.x,mtx.c.y,mtx.c.z)*Det;
}


float Det_22 (float a, float b, float c, float d)
{
	return a*d-b*c;
}


Vector4 Matrix44::SolveSVD (const Vector4& in) const
{
	// Find maximum absolute value element in this matrix and the smallest usable absolute value for
	// any element to be considered non-zero.
	float nearlyZero = 0.0f;   
	int rowM = 0;		// row with the largest absolute value element
	int colM = 0;		// column with the largest absolute value element

	if ( nearlyZero<a.x )  nearlyZero=a.x,  rowM=1, colM=1;
	else if ( nearlyZero<-a.x ) nearlyZero=-a.x, rowM=1, colM=1;
	if ( nearlyZero<a.y )  nearlyZero=a.y,  rowM=1, colM=2;
	else if ( nearlyZero<-a.y ) nearlyZero=-a.y, rowM=1, colM=2;
	if ( nearlyZero<a.z )  nearlyZero=a.z,  rowM=1, colM=3;
	else if ( nearlyZero<-a.z ) nearlyZero=-a.z, rowM=1, colM=3;
	if ( nearlyZero<a.w )  nearlyZero=a.w,  rowM=1, colM=4;
	else if ( nearlyZero<-a.w ) nearlyZero=-a.w, rowM=1, colM=4;

	if ( nearlyZero<b.x )  nearlyZero=b.x,  rowM=2, colM=1;
	else if ( nearlyZero<-b.x ) nearlyZero=-b.x, rowM=2, colM=1;
	if ( nearlyZero<b.y )  nearlyZero=b.y,  rowM=2, colM=2;
	else if ( nearlyZero<-b.y ) nearlyZero=-b.y, rowM=2, colM=2;
	if ( nearlyZero<b.z )  nearlyZero=b.z,  rowM=2, colM=3;
	else if ( nearlyZero<-b.z ) nearlyZero=-b.z, rowM=2, colM=3;
	if ( nearlyZero<b.w )  nearlyZero=b.w,  rowM=2, colM=4;
	else if ( nearlyZero<-b.w ) nearlyZero=-b.w, rowM=2, colM=4;

	if ( nearlyZero<c.x )  nearlyZero=c.x,  rowM=3, colM=1;
	else if ( nearlyZero<-c.x ) nearlyZero=-c.x, rowM=3, colM=1;
	if ( nearlyZero<c.y )  nearlyZero=c.y,  rowM=3, colM=2;
	else if ( nearlyZero<-c.y ) nearlyZero=-c.y, rowM=3, colM=2;
	if ( nearlyZero<c.z )  nearlyZero=c.z,  rowM=3, colM=3;
	else if ( nearlyZero<-c.z ) nearlyZero=-c.z, rowM=3, colM=3;
	if ( nearlyZero<c.w )  nearlyZero=c.w,  rowM=3, colM=4;
	else if ( nearlyZero<-c.w ) nearlyZero=-c.w, rowM=3, colM=4;

	if ( nearlyZero<d.x )  nearlyZero=d.x,  rowM=4, colM=1;
	else if ( nearlyZero<-d.x ) nearlyZero=-d.x, rowM=4, colM=1;
	if ( nearlyZero<d.y )  nearlyZero=d.y,  rowM=4, colM=2;
	else if ( nearlyZero<-d.y ) nearlyZero=-d.y, rowM=4, colM=2;
	if ( nearlyZero<d.z )  nearlyZero=d.z,  rowM=4, colM=3;
	else if ( nearlyZero<-d.z ) nearlyZero=-d.z, rowM=4, colM=3;
	if ( nearlyZero<d.w )  nearlyZero=d.w,  rowM=4, colM=4;
	else if ( nearlyZero<-d.w ) nearlyZero=-d.w, rowM=4, colM=4;

	if ( nearlyZero==0.0f )
	{
		// This matrix is all zeros so there is no answer unless b is also zero, in which case any answer
		// will work.
		return VECTOR4_ORIGIN;
	}

	// Make nearlyZero the the smallest number that can be quarted without risking precision errors.
	// This number to the fourth power is 2e-10f; so is the corresponding number in Matrix34::SolveSVD cubed.
	nearlyZero *= 4.0e-3f;

	// Compute all subdeterminants.
	Matrix44 subdet;
	subdet.a.x = Det33 ( b.y,b.z,b.w, c.y,c.z,c.w, d.y,d.z,d.w );
	subdet.a.y = -Det33 ( b.x,b.z,b.w, c.x,c.z,c.w, d.x,d.z,d.w );
	subdet.a.z = Det33 ( b.x,b.y,b.w, c.x,c.y,c.w, d.x,d.y,d.w );
	subdet.a.w = -Det33 ( b.x,b.y,b.z, c.x,c.y,c.z, d.x,d.y,d.z );

	subdet.b.x = -Det33 ( a.y,a.z,a.w, c.y,c.z,c.w, d.y,d.z,d.w );
	subdet.b.y = Det33 ( a.x,a.z,a.w, c.x,c.z,c.w, d.x,d.z,d.w );
	subdet.b.z = -Det33 ( a.x,a.y,a.w, c.x,c.y,c.w, d.x,d.y,d.w );
	subdet.b.w = Det33 ( a.x,a.y,a.z, c.x,c.y,c.z, d.x,d.y,d.z );

	subdet.c.x = Det33 ( a.y,a.z,a.w, b.y,b.z,b.w, d.y,d.z,d.w );
	subdet.c.y = -Det33 ( a.x,a.z,a.w, b.x,b.z,b.w, d.x,d.z,d.w );
	subdet.c.z = Det33 ( a.x,a.y,a.w, b.x,b.y,b.w, d.x,d.y,d.w );
	subdet.c.w = -Det33 ( a.x,a.y,a.z, b.x,b.y,b.z, d.x,d.y,d.z );

	subdet.d.x = -Det33 ( a.y,a.z,a.w, b.y,b.z,b.w, c.y,c.z,c.w );
	subdet.d.y = Det33 ( a.x,a.z,a.w, b.x,b.z,b.w, c.x,c.z,c.w );
	subdet.d.z = -Det33 ( a.x,a.y,a.w, b.x,b.y,b.w, c.x,c.y,c.w );
	subdet.d.w = Det33 ( a.x,a.y,a.z, b.x,b.y,b.z, c.x,c.y,c.z );


	// Find the maximum absolute value subdeterminant in each row and the column of each one.
	float maxSubdetA = 0.0f;
	int columnMaxSubdetA = 1;
	if (maxSubdetA<subdet.a.x) { maxSubdetA = subdet.a.x; }
	else if (maxSubdetA<-subdet.a.x) { maxSubdetA = -subdet.a.x; }
	if (maxSubdetA<subdet.a.y) { maxSubdetA = subdet.a.y; columnMaxSubdetA=2; }
	else if (maxSubdetA<-subdet.a.y) { maxSubdetA = -subdet.a.y; columnMaxSubdetA=2; }
	if (maxSubdetA<subdet.a.z) { maxSubdetA = subdet.a.z; columnMaxSubdetA=3; }
	else if (maxSubdetA<-subdet.a.z) { maxSubdetA = -subdet.a.z; columnMaxSubdetA=3; }
	if (maxSubdetA<subdet.a.w) { maxSubdetA = subdet.a.w; columnMaxSubdetA=4; }
	else if (maxSubdetA<-subdet.a.w) { maxSubdetA = -subdet.a.w; columnMaxSubdetA=4; }

	float maxSubdetB = 0.0f;
	int columnMaxSubdetB = 1;
	if (maxSubdetB<subdet.b.x) { maxSubdetB = subdet.b.x; }
	else if (maxSubdetB<-subdet.b.x) { maxSubdetB = -subdet.b.x; }
	if (maxSubdetB<subdet.b.y) { maxSubdetB = subdet.b.y; columnMaxSubdetB=2; }
	else if (maxSubdetB<-subdet.b.y) { maxSubdetB = -subdet.b.y; columnMaxSubdetB=2; }
	if (maxSubdetB<subdet.b.z) { maxSubdetB = subdet.b.z; columnMaxSubdetB=3; }
	else if (maxSubdetB<-subdet.b.z) { maxSubdetB = -subdet.b.z; columnMaxSubdetB=3; }
	if (maxSubdetB<subdet.b.w) { maxSubdetB = subdet.b.w; columnMaxSubdetB=4; }
	else if (maxSubdetB<-subdet.b.w) { maxSubdetB = -subdet.b.w; columnMaxSubdetB=4; }

	float maxSubdetC = 0.0f;
	int columnMaxSubdetC = 1;
	if (maxSubdetC<subdet.c.x) { maxSubdetC = subdet.c.x; }
	else if (maxSubdetC<-subdet.c.x) { maxSubdetC = -subdet.c.x; }
	if (maxSubdetC<subdet.c.y) { maxSubdetC = subdet.c.y; columnMaxSubdetC=2; }
	else if (maxSubdetC<-subdet.c.y) { maxSubdetC = -subdet.c.y; columnMaxSubdetC=2; }
	if (maxSubdetC<subdet.c.z) { maxSubdetC = subdet.c.z; columnMaxSubdetC=3; }
	else if (maxSubdetC<-subdet.c.z) { maxSubdetC = -subdet.c.z; columnMaxSubdetC=3; }
	if (maxSubdetC<subdet.c.w) { maxSubdetC = subdet.c.w; columnMaxSubdetC=4; }
	else if (maxSubdetC<-subdet.c.w) { maxSubdetC = -subdet.c.w; columnMaxSubdetC=4; }

	float maxSubdetD = 0.0f;
	int columnMaxSubdetD = 1;
	if (maxSubdetD<subdet.d.x) { maxSubdetD = subdet.d.x; }
	else if (maxSubdetD<-subdet.d.x) { maxSubdetD = -subdet.d.x; }
	if (maxSubdetD<subdet.d.y) { maxSubdetD = subdet.d.y; columnMaxSubdetD=2; }
	else if (maxSubdetD<-subdet.d.y) { maxSubdetD = -subdet.d.y; columnMaxSubdetD=2; }
	if (maxSubdetD<subdet.d.z) { maxSubdetD = subdet.d.z; columnMaxSubdetD=3; }
	else if (maxSubdetD<-subdet.d.z) { maxSubdetD = -subdet.d.z; columnMaxSubdetD=3; }
	if (maxSubdetD<subdet.d.w) { maxSubdetD = subdet.d.w; columnMaxSubdetD=4; }
	else if (maxSubdetD<-subdet.d.w) { maxSubdetD = -subdet.d.w; columnMaxSubdetD=4; }

	// Find the largest subdeterminant and its row.
	float maxSubdet = maxSubdetA;
	int maxSubdetRow = 1;
	if ( maxSubdet<maxSubdetB ) { maxSubdet=maxSubdetB; maxSubdetRow=2; }
	if ( maxSubdet<maxSubdetC ) { maxSubdet=maxSubdetC; maxSubdetRow=3; }
	if ( maxSubdet<maxSubdetD ) { maxSubdet=maxSubdetD; maxSubdetRow=4; }

	// Compute the determinant.
	float det = a.x*subdet.a.x + a.y*subdet.a.y + a.z*subdet.a.z + a.w*subdet.a.w;

	// Find tolerance levels for squares and cubes of elements to be near zero.
	float nearlyZero2 = square(nearlyZero);
	float nearlyZero3 = nearlyZero * nearlyZero2;
	float nearlyZero4 = nearlyZero * nearlyZero3;

	float absDet = fabsf(det);
	if ( absDet>nearlyZero4 && absDet>maxSubdet*nearlyZero &&
		(	(rowM==1 && maxSubdetB>nearlyZero3 && maxSubdetC>nearlyZero3 && maxSubdetD>nearlyZero3 ) ||
			(rowM==2 && maxSubdetA>nearlyZero3 && maxSubdetC>nearlyZero3 && maxSubdetD>nearlyZero3 ) ||
			(rowM==3 && maxSubdetA>nearlyZero3 && maxSubdetB>nearlyZero3 && maxSubdetD>nearlyZero3 ) ||
			(rowM==4 && maxSubdetA>nearlyZero3 && maxSubdetB>nearlyZero3 && maxSubdetC>nearlyZero3 ) ) )
	{
		// This matrix has full rank (4 linearly independent vectors), so return the usual solution.
		float invDet = 1.0f/det;
		Vector4 temp;
		temp.x = (subdet.a.x*in.x+subdet.a.y*in.y+subdet.a.z*in.z+subdet.a.w*in.w)*invDet;
		temp.y = (subdet.b.x*in.x+subdet.b.y*in.y+subdet.b.z*in.z+subdet.b.w*in.w)*invDet;
		temp.z = (subdet.c.x*in.x+subdet.c.y*in.y+subdet.c.z*in.z+subdet.c.w*in.w)*invDet;
		temp.w = (subdet.d.x*in.x+subdet.d.y*in.y+subdet.d.z*in.z+subdet.d.w*in.w)*invDet;
		return temp;
	}

	if (maxSubdet>nearlyZero3)
	{
		// This matrix has only three linearly independent vectors within the precision limits.
		// Find the column of the maximum subdeterminant and the three vectors that make the maximum subdeterminant.
		Vector4 maxSubDetRow1,maxSubDetRow2,maxSubDetRow3,ortho;
		int maxSubdetCol;
		if (maxSubdetRow==1)
		{
			maxSubdetCol = columnMaxSubdetA;
			maxSubDetRow1.Set(b);
			maxSubDetRow2.Set(c);
			maxSubDetRow3.Set(d);
			ortho.Set(subdet.a);
		}
		else
		{
			maxSubDetRow1.Set(a);
			if (maxSubdetRow==2)
			{
				maxSubdetCol = columnMaxSubdetB;
				maxSubDetRow2.Set(c);
				maxSubDetRow3.Set(d);
				ortho.Set(subdet.b);
			}
			else
			{
				maxSubDetRow2.Set(b);
				if (maxSubdetRow==3)
				{
					maxSubdetCol = columnMaxSubdetC;
					maxSubDetRow3.Set(d);
					ortho.Set(subdet.c);
				}
				else
				{
					maxSubdetCol = columnMaxSubdetD;
					maxSubDetRow3.Set(c);
					ortho.Set(subdet.d);
				}
			}
		}

		// Project the input vector onto the span of the three rows.
		Vector4 inProjection(in);
		inProjection.SubtractScaled(ortho,in.Dot(ortho)/ortho.Mag2());

		// Find the three columns that make the maximum subdeterminant.
		Vector4 maxSubDetCol1,maxSubDetCol2,maxSubDetCol3;
		float dAx,dAy,dAz,dBx,dBy,dBz,dCx,dCy,dCz;	// 3x3 matrix entries
		float ux,uy,uz;								// 3-vector

		if (maxSubdetCol==1)
		{
			maxSubDetCol1.Set(a.y,b.y,c.y,d.y);
			maxSubDetCol2.Set(a.z,b.z,c.z,d.z);
			maxSubDetCol3.Set(a.w,b.w,c.w,d.w);
			ux = inProjection.y;
			uy = inProjection.z;
			uz = inProjection.w;
			dAx = maxSubDetRow1.y;
			dAy = maxSubDetRow1.z;
			dAz = maxSubDetRow1.w;
			dBx = maxSubDetRow2.y;
			dBy = maxSubDetRow2.z;
			dBz = maxSubDetRow2.w;
			dCx = maxSubDetRow3.y;
			dCy = maxSubDetRow3.z;
			dCz = maxSubDetRow3.w;
		}
		else
		{
			maxSubDetCol1.Set(a.x,b.x,c.x,d.x);
			ux = inProjection.x;
			dAx = maxSubDetRow1.x;
			dBx = maxSubDetRow2.x;
			dCx = maxSubDetRow3.x;
			if (maxSubdetCol==2)
			{
				maxSubDetCol2.Set(a.z,b.z,c.z,d.z);
				maxSubDetCol3.Set(a.w,b.w,c.w,d.w);
				uy = inProjection.z;
				uz = inProjection.w;
				dAy = maxSubDetRow1.z;
				dAz = maxSubDetRow1.w;
				dBy = maxSubDetRow2.z;
				dBz = maxSubDetRow2.w;
				dCy = maxSubDetRow3.z;
				dCz = maxSubDetRow3.w;
			}
			else
			{
				maxSubDetCol2.Set(a.y,b.y,c.y,d.y);
				uy = inProjection.y;
				dAy = maxSubDetRow1.y;
				dBy = maxSubDetRow2.y;
				dCy = maxSubDetRow3.y;
				if (maxSubdetCol==3)
				{
					maxSubDetCol3.Set(a.w,b.w,c.w,d.w);
					uz = inProjection.w;
					dAz = maxSubDetRow1.w;
					dBz = maxSubDetRow2.w;
					dCz = maxSubDetRow3.w;
				}
				else
				{
					maxSubDetCol3.Set(a.z,b.z,c.z,d.z);
					uz = inProjection.z;
					dAz = maxSubDetRow1.z;
					dBz = maxSubDetRow2.z;
					dCz = maxSubDetRow3.z;
				}
			}
		}

		// Solve for the unknown with a linear combination of the three columns.
		// This determinant is the same as one element of the Matrix44 subdet - figure out which one instead of recalculating?
		float invDet33 = 1.0f/Det33(dAx,dAy,dAz,dBx,dBy,dBz,dCx,dCy,dCz);
		float alpha = (ux*Det_22(dBy,dBz,dCy,dCz) - uy*Det_22(dBx,dBz,dCx,dCz) + uz*Det_22(dBx,dBy,dCx,dCy))*invDet33;
		float beta = (ux*Det_22(dAy,dAz,dCy,dCz) - uy*Det_22(dAx,dAz,dCx,dCz) + uz*Det_22(dAx,dAy,dCx,dCy))*invDet33;
		float gamma = (ux*Det_22(dAy,dAz,dBy,dBz) - uy*Det_22(dAx,dAz,dBx,dBz) + uz*Det_22(dAx,dAy,dBx,dBy))*invDet33;

		Vector4 temp;
		if (maxSubdetRow==1)
		{
			temp.Set(0.0f,alpha,beta,gamma);
		}
		else if (maxSubdetRow==2)
		{
			temp.Set(alpha,0.0f,beta,gamma);
		}
		else if (maxSubdetRow==3)
		{
			temp.Set(alpha,beta,0.0f,gamma);
		}
		else
		{
			temp.Set(alpha,beta,gamma,0.0f);
		}
		// Make 2 perpendicular to 1.
		float maxSubDetCol1Mag2Inv = 1.0f/maxSubDetCol1.Mag2();
		maxSubDetCol2.SubtractScaled(maxSubDetCol1,maxSubDetCol1.Dot(maxSubDetCol2)*maxSubDetCol1Mag2Inv);
		// Make 3 perpendicular to 1.
		maxSubDetCol3.SubtractScaled(maxSubDetCol1,maxSubDetCol1.Dot(maxSubDetCol3)*maxSubDetCol1Mag2Inv);
		// Make 3 also perpendicular to 2.
		float maxSubDetCol2Mag2Inv = 1.0f/maxSubDetCol2.Mag2();
		maxSubDetCol3.SubtractScaled(maxSubDetCol2,maxSubDetCol2.Dot(maxSubDetCol3)*maxSubDetCol2Mag2Inv);
		// Make ortho perpendicular to 3.
		ortho.Set(1.0f);
		ortho.SubtractScaled(maxSubDetCol3,maxSubDetCol3.Dot(ortho)/maxSubDetCol3.Mag2());
		// Make ortho also perpendicular to 2.
		ortho.SubtractScaled(maxSubDetCol2,maxSubDetCol2.Dot(ortho)*maxSubDetCol2Mag2Inv);
		// Make ortho also perpendicular to 1.
		ortho.SubtractScaled(maxSubDetCol1,maxSubDetCol1.Dot(ortho)*maxSubDetCol1Mag2Inv);
		// Make the solution perpendicular to ortho.
		temp.SubtractScaled(ortho,temp.Dot(ortho)/ortho.Mag2());
		return temp;
	}

	// Figure out how to check for two linearly independent vectors and add that here!!!!!!!!!!!
	// Until then, one will do.

	// This matrix has only one linearly independent vector within the precision limits.
	Vector4 maxSubDetRow0;
	if (rowM==1)
	{
		maxSubDetRow0.Set(a);
	}
	else if (rowM==2)
	{
		maxSubDetRow0.Set(b);
	}
	else if (rowM==3)
	{
		maxSubDetRow0.Set(c);
	}
	else
	{
		maxSubDetRow0.Set(d);
	}

	// Project the input vector onto the row with the maximum subdeterminant.
	Vector4 inProjection(maxSubDetRow0);
	inProjection.Scale(in.Dot(maxSubDetRow0)/maxSubDetRow0.Mag2());
	Vector4 maxSubDetCol0;
	float u;
	if (colM==1)
	{
		maxSubDetCol0.Set(a.x,b.x,c.x,d.x);
		u = inProjection.x;
	}
	else if (colM==2)
	{
		maxSubDetCol0.Set(a.y,b.y,c.y,d.y);
		u = inProjection.y;
	}
	else if (colM==3)
	{
		maxSubDetCol0.Set(a.z,b.z,c.z,d.z);
		u = inProjection.z;
	}
	else
	{
		maxSubDetCol0.Set(a.w,b.w,c.w,d.w);
		u = inProjection.w;
	}

	Vector4 temp(maxSubDetCol0);
	temp.Scale(u/maxSubDetCol0.Mag2());
	return temp;
}


Matrix44& Matrix44::MakeRotX(float theta)
{
	float ca,sa;
	//float ca = rage::Cosf(theta);
	//float sa = rage::Sinf(theta);
	rage::cos_and_sin( ca,sa,theta );

	a.Set(1.0f,0.0f,0.0f,0.0f);
	b.Set(0.0f,ca,sa,0.0f);
	c.Set(0.0f,-sa,ca,0.0f);
	d.Set(0.0f,0.0f,0.0f,1.0f);
	return *this;
}


Matrix44& Matrix44::MakeRotY(float theta)
{
	float ca,sa;
	//float ca = rage::Cosf(theta);
	//float sa = rage::Sinf(theta);
	rage::cos_and_sin( ca,sa,theta );
	a.Set(ca,0.0f,-sa,0.0f);
	b.Set(0.0f,1.0f,0.0f,0.0f);
	c.Set(sa,0.0f,ca,0.0f);
	d.Set(0.0f,0.0f,0.0f,1.0f);
	return *this;
}


Matrix44& Matrix44::MakeRotZ(float theta)
{
	float ca,sa;
	//float ca = rage::Cosf(theta);
	//float sa = rage::Sinf(theta);
	rage::cos_and_sin( ca,sa,theta );
	a.Set(ca,sa,0.0f,0.0f);
	b.Set(-sa,ca,0.0f,0.0f);
	c.Set(0.0f,0.0f,1.0f,0.0f);
	d.Set(0.0f,0.0f,0.0f,1.0f);
	return *this;
}

Matrix44& Matrix44::MakeReflect(const Vector4& plane)
{
	a.Set(	-2.0f * plane.x * plane.x + 1.0f,
			-2.0f * plane.y * plane.x,
			-2.0f * plane.z * plane.x,
			0.0f);
	b.Set(	-2.0f * plane.x * plane.y,
			-2.0f * plane.y * plane.y + 1.0f,
			-2.0f * plane.z * plane.y,
			0.0f);
	c.Set(	-2.0f * plane.x * plane.z,
			-2.0f * plane.y * plane.z,
			-2.0f * plane.z * plane.z + 1.0f,
			0.0f);
	d.Set(	-2.0f * plane.x * plane.w,
			-2.0f * plane.y * plane.w,
			-2.0f * plane.z * plane.w,
			1.0f);
	return *this;
}



void Matrix44::Print(const char *s) const
{
	if(s) Printf("%s {",s);
	else Printf("matrix {");
	a.Print(" a");
	b.Print(" b");
	c.Print(" c");
	d.Print(" d");
	Printf("}\n");
}


void Matrix44::Transform4(const Vector3 *in, Vector4 *out, int count) const
{
	int i;
	mthAssertf((count & 3) == 0, "Count %d isn't a multiple of 4", count); //lint !e506 constant value boolean
	for (i = count-1; i >= 0; i--)
	{
		out[i].x = in[i].x*a.x + in[i].y*b.x + in[i].z*c.x + d.x;
		out[i].y = in[i].x*a.y + in[i].y*b.y + in[i].z*c.y + d.y;
		out[i].z = in[i].x*a.z + in[i].y*b.z + in[i].z*c.z + d.z;
		out[i].w = 0;
	}
}
  
  
