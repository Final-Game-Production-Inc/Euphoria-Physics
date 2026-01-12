//
// vector/matrix34.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#if __SPU
#include "vector/vector3_consts_spu.cpp"
#endif // __SPU

#include "matrix34.h"
#include "quaternion.h"
#include "matrix33.h"

#include "data/struct.h"
#include "qa/qa.h"

using namespace rage;

#if !__SPU
const Matrix34 rage::M34_IDENTITY(1.0f,0.0f,0.0f, 0.f,1.0f,0.0f, 0.f,0.0f,1.0f, 0.f,0.f,0.0f);

const Matrix34 rage::M34_ZERO(0.0f,0.0f,0.0f, 0.0f,0.0f,0.0f, 0.0f,0.0f,0.0f, 0.0f,0.0f,0.0f);
#endif

#if __DECLARESTRUCT
void Matrix34::DeclareStruct(datTypeStruct &s)
{
	STRUCT_BEGIN(Matrix34);
	STRUCT_FIELD(a);
	STRUCT_FIELD(b);
	STRUCT_FIELD(c);
	STRUCT_FIELD(d);
	STRUCT_END();
}
#endif

#if !__FINAL
void Matrix34::Print(const char *s) const {
	if(s) Printf("%s {",s);
	else Printf("matrix {");
	a.Print(" a");
	b.Print(" b");
	c.Print(" c");
	d.Print(" d");
	Printf("}\n");
}
#else
void Matrix34::Print(const char *) const	{};
#endif // !__FINAL

void Matrix34::FromQuaternion(const Quaternion & q)
{
    // Conversion equations:
    //   where xy = Q.x*Q.y, x2 = Q.x*Q.x, etc.
    //
    // a.Set(1.0f-2.0f*(y2+z2), 2.0f*(xy+zw), 2.0f*(xz-yw));
    // b.Set(2.0f*(xy-zw), 1.0f-2.0f*(x2+z2), 2.0f*(yz+xw));
    // c.Set(2.0f*(xz+yw), 2.0f*(yz-xw), 1.0f-2.0f*(x2+y2));

    register float tx(q.x*M_SQRT2);
    register float ty(q.y*M_SQRT2);
    register float tz(q.z*M_SQRT2);
    register float tw(q.w*M_SQRT2);
	


	mthAssertf(q.Mag2() >= square(0.999f) && q.Mag2() <= square(1.001f),"Quaternion <%f, %f, %f, %f> is not a pure rotation.",q.x,q.y,q.z,q.w);
#if __PPU
	
	register float txy,txz,tyz;
	register float ty1,tx1;

	txy = tx*ty;
	txz = tx*tz;
	tyz = ty*tz;

#ifdef __SNC__
#define __fnmsubs __builtin_fnmsubs
#define __fmadds __builtin_fmadds
#endif

	tx1 = __fnmsubs(tx,tx,1.0f);	//1.0f - tx*tx;
	ty1 = __fnmsubs(ty,ty,1.0f);	//1.0f - ty*ty;

	a.x = __fnmsubs(tz,tz,ty1);		//ty1 - tz*tz;
	a.y = __fmadds(tz,tw,txy);		//txy + tz*tw;
	a.z = __fnmsubs(ty,tw,txz);		//txz - ty*tw;

	b.x = __fnmsubs(tz,tw,txy);		//txy - tz*tw;
	b.y = __fnmsubs(tz,tz,tx1);		//tx1 - tz*tz;
	b.z = __fmadds(tx,tw,tyz);		//tyz + tx*tw;

	c.x = __fmadds(ty,tw,txz);		//txz + ty*tw;
	c.y = __fnmsubs(tx,tw,tyz);		//tyz - tx*tw;
	c.z = __fnmsubs(ty,ty,tx1);		//tx1 - ty*ty;




#else
    a.y = tx*ty + tz*tw;
    b.x = tx*ty - tz*tw;

    a.z = tx*tz - ty*tw;
    c.x = tx*tz + ty*tw;

    b.z = ty*tz + tx*tw;
    c.y = ty*tz - tx*tw;

    ty *= ty;   // need squares along diagonal
    tz *= tz;
    tx *= tx;
    a.x = 1.0f - (ty + tz);
    b.y = 1.0f - (tz + tx);
    c.z = 1.0f - (ty + tx);
#endif
}

void Matrix34::ToQuaternion(Quaternion &q) const 
{
	q.FromMatrix34(*this);
}

void Matrix34::Interpolate(const Matrix34 &source, const Matrix34 &goal,float t) 
{
	Quaternion s,g;
	s.FromMatrix34(source);
	g.FromMatrix34(goal);
	s.PrepareSlerp (g); // shortest path
	s.Slerp(t,s,g);
	FromQuaternion(s);
	d.Lerp(t,source.d,goal.d);
}

void Matrix34::MirrorOnPlane( const Vector4& P )
{
	a = Vector3(	-2.0f * P.x * P.x + 1.0f,	-2.0f * P.x * P.y,			-2.0f * P.x * P.z);
	b = Vector3(	-2.0f * P.y * P.x  ,		-2.0f * P.y * P.y + 1.0f,	-2.0f * P.y * P.z);
	c = Vector3(	-2.0f * P.z * P.x ,			-2.0f * P.z * P.y,			-2.0f * P.z * P.z + 1.0f  );
	d = Vector3(	-2.0f * P.x * P.w,			-2.0f * P.y * P.w,			-2.0f * P.z * P.w );
}

Matrix34& Matrix34::operator=(const Matrix33& matrix)
{
	a = matrix.a;
	b = matrix.b;
	c = matrix.c;
	d.Zero();
	return *this;
}



#if __QA

#define M34QA_ITEM_BEGIN(Name)					\
class qa##Name : public qaItem					\
{ public:										\
	void Init() {};								\
	void Update(qaResult& result)

#define M34QA_ITEM_END()			};

inline bool FloatEqual(float fVal1, float fVal2, float eps = SMALL_FLOAT)
{
	float fTest = fVal1 - fVal2;
	return (fTest < eps && fTest > -eps);
}

inline bool VectorEqual(const Vector3& vec, float fVal1, float fVal2, float fVal3, float eps = SMALL_FLOAT)
{
	return (FloatEqual(vec.x, fVal1, eps) && FloatEqual(vec.y, fVal2, eps) && FloatEqual(vec.z, fVal3, eps));
}

inline bool MatrixEqual(const Matrix34& m, float fVal1, float fVal2, float fVal3, float fVal4, float fVal5, float fVal6, float fVal7, float fVal8, float fVal9, float fVal10, float fVal11, float fVal12, float eps = SMALL_FLOAT)
{
	return (VectorEqual(m.a, fVal1, fVal2, fVal3, eps) && VectorEqual(m.b, fVal4, fVal5, fVal6, eps) && VectorEqual(m.c, fVal7, fVal8, fVal9, eps) && VectorEqual(m.d, fVal10, fVal11, fVal12, eps));
}

M34QA_ITEM_BEGIN(M343T_Zero3x3)
{
	Matrix34 test(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f);
	test.Zero3x3();
	bool bPass = MatrixEqual(test, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 10.0f, 11.0f, 12.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
M34QA_ITEM_END();

M34QA_ITEM_BEGIN(M343T_Zero)
{
	Matrix34 test(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f);
	test.Zero();
	bool bPass = MatrixEqual(test, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
M34QA_ITEM_END();

M34QA_ITEM_BEGIN(M343T_Dot)
{
	Matrix34 test(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f);
	Matrix34 test2(13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f);
	test.Dot(test2);

	bool bPass = MatrixEqual(test, 102.0f, 108.0f, 114.0f, 246.0f, 261.0f, 276.0f, 390.0f, 414.0f, 438.0f, 556.0f, 590.0f, 624.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
M34QA_ITEM_END();

M34QA_ITEM_BEGIN(M343T_DotFromLeft)
{
	Matrix34 test(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f);
	Matrix34 test2(13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f);
	test.DotFromLeft(test2);

	bool bPass = MatrixEqual(test, 174.0f, 216.0f, 258.0f, 210.0f, 261.0f, 312.0f, 246.0f, 306.0f, 366.0f, 292.0f, 362.0f, 432.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
M34QA_ITEM_END();

M34QA_ITEM_BEGIN(M343T_DotM)
{
	Matrix34 test;
	Matrix34 test1(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f);
	Matrix34 test2(13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f);
	test.Dot(test1, test2);

	bool bPass = MatrixEqual(test, 102.0f, 108.0f, 114.0f, 246.0f, 261.0f, 276.0f, 390.0f, 414.0f, 438.0f, 556.0f, 590.0f, 624.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
M34QA_ITEM_END();

M34QA_ITEM_BEGIN(M343T_DotTranspose)
{
	Matrix34 test(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f);
	Matrix34 test2(13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f);
	test.DotTranspose(test2);

	bool bPass = MatrixEqual(test, 86.0f, 104.0f, 122.0f, 212.0f, 257.0f, 302.0f, 338.0f, 410.0f, 482.0f, -504.0f, -612.0f, -720.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
M34QA_ITEM_END();

M34QA_ITEM_BEGIN(M343T_DotTransposeM)
{
	Matrix34 test;
	Matrix34 test1(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f);
	Matrix34 test2(13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f);
	test.DotTranspose(test1, test2);

	bool bPass = MatrixEqual(test, 86.0f, 104.0f, 122.0f, 212.0f, 257.0f, 302.0f, 338.0f, 410.0f, 482.0f, -504.0f, -612.0f, -720.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
M34QA_ITEM_END();

M34QA_ITEM_BEGIN(M343T_Dot3x3)
{
	Matrix34 test(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f);
	Matrix34 test2(13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f);
	test.Dot3x3(test2);

	bool bPass = MatrixEqual(test, 102.0f, 108.0f, 114.0f, 246.0f, 261.0f, 276.0f, 390.0f, 414.0f, 438.0f, 10.0f, 11.0f, 12.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
M34QA_ITEM_END();

M34QA_ITEM_BEGIN(M343T_Dot3x3FromLeft)
{
	Matrix34 test(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f);
	Matrix34 test2(13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f);
	test.Dot3x3FromLeft(test2);

	bool bPass = MatrixEqual(test, 174.0f, 216.0f, 258.0f, 210.0f, 261.0f, 312.0f, 246.0f, 306.0f, 366.0f, 10.0f, 11.0f, 12.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
M34QA_ITEM_END();

M34QA_ITEM_BEGIN(M343T_Dot3x3M)
{
	Matrix34 test(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f);
	Matrix34 test1(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f);
	Matrix34 test2(13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f);
	test.Dot3x3(test1, test2);

	bool bPass = MatrixEqual(test, 102.0f, 108.0f, 114.0f, 246.0f, 261.0f, 276.0f, 390.0f, 414.0f, 438.0f, 10.0f, 11.0f, 12.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
M34QA_ITEM_END();

M34QA_ITEM_BEGIN(M343T_Dot3x3Transpose)
{
	Matrix34 test(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f);
	Matrix34 test2(13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f);
	test.Dot3x3Transpose(test2);

	bool bPass = MatrixEqual(test, 86.0f, 104.0f, 122.0f, 212.0f, 257.0f, 302.0f, 338.0f, 410.0f, 482.0f, 10.0f, 11.0f, 12.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
M34QA_ITEM_END();

M34QA_ITEM_BEGIN(M343T_Dot3x3TransposeM)
{
	Matrix34 test(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f);
	Matrix34 test1(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f);
	Matrix34 test2(13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f);
	test.Dot3x3Transpose(test1, test2);

	bool bPass = MatrixEqual(test, 86.0f, 104.0f, 122.0f, 212.0f, 257.0f, 302.0f, 338.0f, 410.0f, 482.0f, 10.0f, 11.0f, 12.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
M34QA_ITEM_END();

M34QA_ITEM_BEGIN(M343T_IsEqual)
{
	Matrix34 test1(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f);
	Matrix34 test2(13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f);
	bool bPass = (!(test1.IsEqual(test2)) && test1.IsEqual(test1));
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
M34QA_ITEM_END();

M34QA_ITEM_BEGIN(M343T_IsNotEqual)
{
	Matrix34 test1(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f);
	Matrix34 test2(13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f);
	bool bPass = (test1.IsNotEqual(test2) && !(test1.IsNotEqual(test1)));
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
M34QA_ITEM_END();

M34QA_ITEM_BEGIN(M343T_OuterProduct)
{
	Matrix34 test(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f);
	Vector3 test1(1.0f, 2.0f, 3.0f);
	Vector3 test2(4.0f, 5.0f, 6.0f);
	test.OuterProduct(test1, test2);

	bool bPass = MatrixEqual(test, 4.0f, 5.0f, 6.0f, 8.0f, 10.0f, 12.0f, 12.0f, 15.0f, 18.0f, 10.0f, 11.0f, 12.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
M34QA_ITEM_END();

M34QA_ITEM_BEGIN(M343T_TransformV)
{
	Matrix34 test(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f);
	Vector3 test1(1.0f, 2.0f, 3.0f);
	Vector3 test2(4.0f, 5.0f, 6.0f);
	test.Transform(test1, test2);

	bool bPass = VectorEqual(test2, 40.0f, 47.0f, 54.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
M34QA_ITEM_END();

M34QA_ITEM_BEGIN(M343T_Transform)
{
	Matrix34 test(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f);
	Vector3 test1(1.0f, 2.0f, 3.0f);
	test.Transform(test1);

	bool bPass = VectorEqual(test1, 40.0f, 47.0f, 54.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
M34QA_ITEM_END();

M34QA_ITEM_BEGIN(M343T_Transform3x3V)
{
	Matrix34 test(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f);
	Vector3 test1(1.0f, 2.0f, 3.0f);
	Vector3 test2(4.0f, 5.0f, 6.0f);
	test.Transform3x3(test1, test2);

	bool bPass = VectorEqual(test2, 30.0f, 36.0f, 42.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
M34QA_ITEM_END();

M34QA_ITEM_BEGIN(M343T_Transform3x3)
{
	Matrix34 test(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f);
	Vector3 test1(1.0f, 2.0f, 3.0f);
	test.Transform3x3(test1);

	bool bPass = VectorEqual(test1, 30.0f, 36.0f, 42.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
M34QA_ITEM_END();

M34QA_ITEM_BEGIN(M343T_UnTransformV)
{
	Matrix34 test(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f);
	Vector3 test1(1.0f, 2.0f, 3.0f);
	Vector3 test2(4.0f, 5.0f, 6.0f);
	test.UnTransform(test1, test2);

	bool bPass = VectorEqual(test2, -54.0f, -135.0f, -216.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
M34QA_ITEM_END();

M34QA_ITEM_BEGIN(M343T_UnTransform)
{
	Matrix34 test(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f);
	Vector3 test1(1.0f, 2.0f, 3.0f);
	test.UnTransform(test1);

	bool bPass = VectorEqual(test1, -54.0f, -135.0f, -216.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
M34QA_ITEM_END();

M34QA_ITEM_BEGIN(M343T_UnTransform3x3V)
{
	Matrix34 test(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f);
	Vector3 test1(1.0f, 2.0f, 3.0f);
	Vector3 test2(4.0f, 5.0f, 6.0f);
	test.UnTransform3x3(test1, test2);

	bool bPass = VectorEqual(test2, 14.0f, 32.0f, 50.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
M34QA_ITEM_END();

M34QA_ITEM_BEGIN(M343T_UnTransform3x3)
{
	Matrix34 test(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f);
	Vector3 test1(1.0f, 2.0f, 3.0f);
	test.UnTransform3x3(test1);

	bool bPass = VectorEqual(test1, 14.0f, 32.0f, 50.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
M34QA_ITEM_END();

M34QA_ITEM_BEGIN(M343T_Scale)
{
	Matrix34 test(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f);
	test.Scale(0.5f);

	bool bPass = MatrixEqual(test, 0.5f, 1.0f, 1.5f, 2.0f, 2.5f, 3.0f, 3.5f, 4.0f, 4.5f, 10.0f, 11.0f, 12.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
M34QA_ITEM_END();

M34QA_ITEM_BEGIN(M343T_ScaleF)
{
	Matrix34 test(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f);
	test.Scale(0.5f, 0.5f, 0.5f);

	bool bPass = MatrixEqual(test, 0.5f, 1.0f, 1.5f, 2.0f, 2.5f, 3.0f, 3.5f, 4.0f, 4.5f, 10.0f, 11.0f, 12.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
M34QA_ITEM_END();

M34QA_ITEM_BEGIN(M343T_ScaleV)
{
	Matrix34 test(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f);
	Vector3 scalar(0.5f, 0.5f, 0.5f);
	test.Scale(scalar);

	bool bPass = MatrixEqual(test, 0.5f, 1.0f, 1.5f, 2.0f, 2.5f, 3.0f, 3.5f, 4.0f, 4.5f, 10.0f, 11.0f, 12.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
M34QA_ITEM_END();

M34QA_ITEM_BEGIN(M343T_ScaleFull)
{
	Matrix34 test(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f);
	test.ScaleFull(0.5f);

	bool bPass = MatrixEqual(test, 0.5f, 1.0f, 1.5f, 2.0f, 2.5f, 3.0f, 3.5f, 4.0f, 4.5f, 5.0f, 5.5f, 6.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
M34QA_ITEM_END();

M34QA_ITEM_BEGIN(M343T_ScaleFullF)
{
	Matrix34 test(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f);
	test.ScaleFull(0.5f, 0.5f, 0.5f);

	bool bPass = MatrixEqual(test, 0.5f, 1.0f, 1.5f, 2.0f, 2.5f, 3.0f, 3.5f, 4.0f, 4.5f, 5.0f, 5.5f, 6.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
M34QA_ITEM_END();

M34QA_ITEM_BEGIN(M343T_ScaleFullV)
{
	Matrix34 test(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f);
	Vector3 scalar(0.5f, 0.5f, 0.5f);
	test.ScaleFull(scalar);

	bool bPass = MatrixEqual(test, 0.5f, 1.0f, 1.5f, 2.0f, 2.5f, 3.0f, 3.5f, 4.0f, 4.5f, 5.0f, 5.5f, 6.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
M34QA_ITEM_END();

M34QA_ITEM_BEGIN(M343T_Transpose)
{
	Matrix34 test(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f);
	test.Transpose();

	bool bPass = MatrixEqual(test, 1.0f, 4.0f, 7.0f, 2.0f, 5.0f, 8.0f, 3.0f, 6.0f, 9.0f, 10.0f, 11.0f, 12.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
M34QA_ITEM_END();

M34QA_ITEM_BEGIN(M343T_TransposeM)
{
	Matrix34 test(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f);
	Matrix34 test1(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f);
	test.Transpose(test1);

	bool bPass = MatrixEqual(test, 1.0f, 4.0f, 7.0f, 2.0f, 5.0f, 8.0f, 3.0f, 6.0f, 9.0f, 10.0f, 11.0f, 12.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
M34QA_ITEM_END();


QA_ITEM_FAMILY(qaM343T_Zero3x3, (), ());
QA_ITEM_FAMILY(qaM343T_Zero, (), ());
QA_ITEM_FAMILY(qaM343T_Dot, (), ());
QA_ITEM_FAMILY(qaM343T_DotFromLeft, (), ());
QA_ITEM_FAMILY(qaM343T_DotM, (), ());
QA_ITEM_FAMILY(qaM343T_DotTranspose, (), ());
QA_ITEM_FAMILY(qaM343T_DotTransposeM, (), ());
QA_ITEM_FAMILY(qaM343T_Dot3x3, (), ());
QA_ITEM_FAMILY(qaM343T_Dot3x3FromLeft, (), ());
QA_ITEM_FAMILY(qaM343T_Dot3x3M, (), ());
QA_ITEM_FAMILY(qaM343T_Dot3x3Transpose, (), ());
QA_ITEM_FAMILY(qaM343T_Dot3x3TransposeM, (), ());
QA_ITEM_FAMILY(qaM343T_IsEqual, (), ());
QA_ITEM_FAMILY(qaM343T_IsNotEqual, (), ());
QA_ITEM_FAMILY(qaM343T_OuterProduct, (), ());
QA_ITEM_FAMILY(qaM343T_TransformV, (), ());
QA_ITEM_FAMILY(qaM343T_Transform, (), ());
QA_ITEM_FAMILY(qaM343T_Transform3x3V, (), ());
QA_ITEM_FAMILY(qaM343T_Transform3x3, (), ());
QA_ITEM_FAMILY(qaM343T_UnTransformV, (), ());
QA_ITEM_FAMILY(qaM343T_UnTransform, (), ());
QA_ITEM_FAMILY(qaM343T_UnTransform3x3V, (), ());
QA_ITEM_FAMILY(qaM343T_UnTransform3x3, (), ());
QA_ITEM_FAMILY(qaM343T_Scale, (), ());
QA_ITEM_FAMILY(qaM343T_ScaleF, (), ());
QA_ITEM_FAMILY(qaM343T_ScaleV, (), ());
QA_ITEM_FAMILY(qaM343T_ScaleFull, (), ());
QA_ITEM_FAMILY(qaM343T_ScaleFullF, (), ());
QA_ITEM_FAMILY(qaM343T_ScaleFullV, (), ());
QA_ITEM_FAMILY(qaM343T_Transpose, (), ());
QA_ITEM_FAMILY(qaM343T_TransposeM, (), ());

QA_ITEM_FAST(qaM343T_Zero3x3, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAST(qaM343T_Zero, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAST(qaM343T_Dot, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAST(qaM343T_DotFromLeft, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAST(qaM343T_DotM, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAST(qaM343T_DotTranspose, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAST(qaM343T_DotTransposeM, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAST(qaM343T_Dot3x3, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAST(qaM343T_Dot3x3FromLeft, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAST(qaM343T_Dot3x3M, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAST(qaM343T_Dot3x3Transpose, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAST(qaM343T_Dot3x3TransposeM, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAST(qaM343T_IsEqual, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAST(qaM343T_IsNotEqual, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAST(qaM343T_OuterProduct, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAST(qaM343T_TransformV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAST(qaM343T_Transform, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAST(qaM343T_Transform3x3V, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAST(qaM343T_Transform3x3, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAST(qaM343T_UnTransformV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAST(qaM343T_UnTransform, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAST(qaM343T_UnTransform3x3V, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAST(qaM343T_UnTransform3x3, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAST(qaM343T_Scale, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAST(qaM343T_ScaleF, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAST(qaM343T_ScaleV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAST(qaM343T_ScaleFull, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAST(qaM343T_ScaleFullF, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAST(qaM343T_ScaleFullV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAST(qaM343T_Transpose, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAST(qaM343T_TransposeM, (), qaResult::FAIL_OR_TOTAL_TIME);

#endif
