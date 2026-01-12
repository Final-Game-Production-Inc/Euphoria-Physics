//
// vector/Matrix33.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#if __SPU
#include "vector/vector3_consts_spu.cpp"
#endif // __SPU

#include "matrix33.h"
#include "quaternion.h"

#include "data/struct.h"
#include "qa/qa.h"

using namespace rage;


#if !__SPU
//const Matrix33 rage::M33_IDENTITY(1.0f,0.0f,0.0f, 0.f,1.0f,0.0f, 0.f,0.0f,1.0f);

//const Matrix33 rage::M33_ZERO(0.0f,0.0f,0.0f, 0.0f,0.0f,0.0f, 0.0f,0.0f,0.0f);
#endif

#if __DECLARESTRUCT
void Matrix33::DeclareStruct(datTypeStruct &s)
{
	STRUCT_BEGIN(Matrix33);
	STRUCT_FIELD(a);
	STRUCT_FIELD(b);
	STRUCT_FIELD(c);
	STRUCT_END();
}
#endif

#if !__FINAL
void Matrix33::Print(const char *s) const {
	if(s) Printf("%s {",s);
	else Printf("matrix {");
	a.Print(" a");
	b.Print(" b");
	c.Print(" c");
	Printf("}\n");
}
#else
void Matrix33::Print(const char *) const	{};
#endif // !__FINAL

void Matrix33::FromQuaternion(const Quaternion & q)
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
}
/*
void Matrix33::ToQuaternion(Quaternion &q) const 
{
	q.FromMatrix33(*this);
}

void Matrix33::Interpolate(const Matrix33 &source, const Matrix33 &goal,float t) 
{
	Quaternion s,g;
	s.FromMatrix33(source);
	g.FromMatrix33(goal);
	s.PrepareSlerp (g); // shortest path
	s.Slerp(t,s,g);
	FromQuaternion(s);
}
*/
void Matrix33::MirrorOnPlane( const Vector4& P )
{
	a = Vector3(	-2.0f * P.x * P.x + 1.0f,	-2.0f * P.x * P.y,			-2.0f * P.x * P.z);
	b = Vector3(	-2.0f * P.y * P.x  ,		-2.0f * P.y * P.y + 1.0f,	-2.0f * P.y * P.z);
	c = Vector3(	-2.0f * P.z * P.x ,			-2.0f * P.z * P.y,			-2.0f * P.z * P.z + 1.0f  );
}


#if __QA

#define M33QA_ITEM_BEGIN(Name)					\
class qa##Name : public qaItem					\
{ public:										\
	void Init() {};								\
	void Update(qaResult& result)

#define M33QA_ITEM_END()			};

inline bool FloatEqual(float fVal1, float fVal2, float eps = SMALL_FLOAT)
{
	float fTest = fVal1 - fVal2;
	return (fTest < eps && fTest > -eps);
}

inline bool VectorEqual(const Vector3& vec, float fVal1, float fVal2, float fVal3, float eps = SMALL_FLOAT)
{
	return (FloatEqual(vec.x, fVal1, eps) && FloatEqual(vec.y, fVal2, eps) && FloatEqual(vec.z, fVal3, eps));
}

inline bool MatrixEqual(const Matrix33& m, float fVal1, float fVal2, float fVal3, float fVal4, float fVal5, float fVal6, float fVal7, float fVal8, float fVal9, float eps = SMALL_FLOAT)
{
	return (VectorEqual(m.a, fVal1, fVal2, fVal3, eps) && VectorEqual(m.b, fVal4, fVal5, fVal6, eps) && VectorEqual(m.c, fVal7, fVal8, fVal9, eps));
}

M33QA_ITEM_BEGIN(M333T_Zero)
{
	Matrix33 test(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
	test.Zero();
	bool bPass = MatrixEqual(test, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
M33QA_ITEM_END();

M33QA_ITEM_BEGIN(M333T_Dot)
{
	Matrix33 test(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
	Matrix33 test2(13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f);
	test.Dot(test2);

	bool bPass = MatrixEqual(test, 102.0f, 108.0f, 114.0f, 246.0f, 261.0f, 276.0f, 390.0f, 414.0f, 438.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
M33QA_ITEM_END();

M33QA_ITEM_BEGIN(M333T_DotFromLeft)
{
	Matrix33 test(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
	Matrix33 test2(13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f);
	test.DotFromLeft(test2);

	bool bPass = MatrixEqual(test, 174.0f, 216.0f, 258.0f, 210.0f, 261.0f, 312.0f, 246.0f, 306.0f, 366.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
M33QA_ITEM_END();

M33QA_ITEM_BEGIN(M333T_DotM)
{
	Matrix33 test;
	Matrix33 test1(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
	Matrix33 test2(13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f);
	test.Dot(test1, test2);

	bool bPass = MatrixEqual(test, 102.0f, 108.0f, 114.0f, 246.0f, 261.0f, 276.0f, 390.0f, 414.0f, 438.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
M33QA_ITEM_END();

M33QA_ITEM_BEGIN(M333T_DotTranspose)
{
	Matrix33 test(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
	Matrix33 test2(13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f);
	test.DotTranspose(test2);

	bool bPass = MatrixEqual(test, 86.0f, 104.0f, 122.0f, 212.0f, 257.0f, 302.0f, 338.0f, 410.0f, 482.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
M33QA_ITEM_END();

M33QA_ITEM_BEGIN(M333T_DotTransposeM)
{
	Matrix33 test;
	Matrix33 test1(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
	Matrix33 test2(13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f);
	test.DotTranspose(test1, test2);

	bool bPass = MatrixEqual(test, 86.0f, 104.0f, 122.0f, 212.0f, 257.0f, 302.0f, 338.0f, 410.0f, 482.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
M33QA_ITEM_END();

M33QA_ITEM_BEGIN(M333T_IsEqual)
{
	Matrix33 test1(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
	Matrix33 test2(13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f);
	bool bPass = (!(test1.IsEqual(test2)) && test1.IsEqual(test1));
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
M33QA_ITEM_END();

M33QA_ITEM_BEGIN(M333T_IsNotEqual)
{
	Matrix33 test1(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
	Matrix33 test2(13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f);
	bool bPass = (test1.IsNotEqual(test2) && !(test1.IsNotEqual(test1)));
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
M33QA_ITEM_END();

M33QA_ITEM_BEGIN(M333T_OuterProduct)
{
	Matrix33 test(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
	Vector3 test1(1.0f, 2.0f, 3.0f);
	Vector3 test2(4.0f, 5.0f, 6.0f);
	test.OuterProduct(test1, test2);

	bool bPass = MatrixEqual(test, 4.0f, 5.0f, 6.0f, 8.0f, 10.0f, 12.0f, 12.0f, 15.0f, 18.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
M33QA_ITEM_END();

M33QA_ITEM_BEGIN(M333T_TransformV)
{
	Matrix33 test(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
	Vector3 test1(1.0f, 2.0f, 3.0f);
	Vector3 test2(4.0f, 5.0f, 6.0f);
	test.Transform(test1, test2);

	bool bPass = VectorEqual(test2, 30.0f, 36.0f, 42.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
M33QA_ITEM_END();

M33QA_ITEM_BEGIN(M333T_Transform)
{
	Matrix33 test(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
	Vector3 test1(1.0f, 2.0f, 3.0f);
	test.Transform(test1);

	bool bPass = VectorEqual(test1, 30.0f, 36.0f, 42.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
M33QA_ITEM_END();
M33QA_ITEM_BEGIN(M333T_UnTransformV)
{
	Matrix33 test(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
	Vector3 test1(1.0f, 2.0f, 3.0f);
	Vector3 test2(4.0f, 5.0f, 6.0f);
	test.UnTransform(test1, test2);

	bool bPass = VectorEqual(test2, 14.0f, 32.0f, 50.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
M33QA_ITEM_END();

M33QA_ITEM_BEGIN(M333T_UnTransform)
{
	Matrix33 test(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
	Vector3 test1(1.0f, 2.0f, 3.0f);
	test.UnTransform(test1);

	bool bPass = VectorEqual(test1, 14.0f, 32.0f, 50.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
M33QA_ITEM_END();

M33QA_ITEM_BEGIN(M333T_Scale)
{
	Matrix33 test(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
	test.Scale(0.5f);

	bool bPass = MatrixEqual(test, 0.5f, 1.0f, 1.5f, 2.0f, 2.5f, 3.0f, 3.5f, 4.0f, 4.5f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
M33QA_ITEM_END();

M33QA_ITEM_BEGIN(M333T_ScaleF)
{
	Matrix33 test(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
	test.Scale(0.5f, 0.5f, 0.5f);

	bool bPass = MatrixEqual(test, 0.5f, 1.0f, 1.5f, 2.0f, 2.5f, 3.0f, 3.5f, 4.0f, 4.5f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
M33QA_ITEM_END();

M33QA_ITEM_BEGIN(M333T_ScaleV)
{
	Matrix33 test(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
	Vector3 scalar(0.5f, 0.5f, 0.5f);
	test.Scale(scalar);

	bool bPass = MatrixEqual(test, 0.5f, 1.0f, 1.5f, 2.0f, 2.5f, 3.0f, 3.5f, 4.0f, 4.5f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
M33QA_ITEM_END();

M33QA_ITEM_BEGIN(M333T_Transpose)
{
	Matrix33 test(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
	test.Transpose();

	bool bPass = MatrixEqual(test, 1.0f, 4.0f, 7.0f, 2.0f, 5.0f, 8.0f, 3.0f, 6.0f, 9.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
M33QA_ITEM_END();

M33QA_ITEM_BEGIN(M333T_TransposeM)
{
	Matrix33 test(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
	Matrix33 test1(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
	test.Transpose(test1);

	bool bPass = MatrixEqual(test, 1.0f, 4.0f, 7.0f, 2.0f, 5.0f, 8.0f, 3.0f, 6.0f, 9.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
M33QA_ITEM_END();


QA_ITEM_FAMILY(qaM333T_Zero, (), ());
QA_ITEM_FAMILY(qaM333T_Dot, (), ());
QA_ITEM_FAMILY(qaM333T_DotFromLeft, (), ());
QA_ITEM_FAMILY(qaM333T_DotM, (), ());
QA_ITEM_FAMILY(qaM333T_DotTranspose, (), ());
QA_ITEM_FAMILY(qaM333T_DotTransposeM, (), ());
QA_ITEM_FAMILY(qaM333T_IsEqual, (), ());
QA_ITEM_FAMILY(qaM333T_IsNotEqual, (), ());
QA_ITEM_FAMILY(qaM333T_OuterProduct, (), ());
QA_ITEM_FAMILY(qaM333T_TransformV, (), ());
QA_ITEM_FAMILY(qaM333T_Transform, (), ());
QA_ITEM_FAMILY(qaM333T_UnTransformV, (), ());
QA_ITEM_FAMILY(qaM333T_UnTransform, (), ());
QA_ITEM_FAMILY(qaM333T_Scale, (), ());
QA_ITEM_FAMILY(qaM333T_ScaleF, (), ());
QA_ITEM_FAMILY(qaM333T_ScaleV, (), ());
QA_ITEM_FAMILY(qaM333T_Transpose, (), ());
QA_ITEM_FAMILY(qaM333T_TransposeM, (), ());

QA_ITEM_FAST(qaM333T_Zero, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAST(qaM333T_Dot, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAST(qaM333T_DotFromLeft, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAST(qaM333T_DotM, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAST(qaM333T_DotTranspose, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAST(qaM333T_DotTransposeM, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAST(qaM333T_IsEqual, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAST(qaM333T_IsNotEqual, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAST(qaM333T_OuterProduct, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAST(qaM333T_TransformV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAST(qaM333T_Transform, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAST(qaM333T_UnTransformV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAST(qaM333T_UnTransform, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAST(qaM333T_Scale, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAST(qaM333T_ScaleF, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAST(qaM333T_ScaleV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAST(qaM333T_Transpose, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAST(qaM333T_TransposeM, (), qaResult::FAIL_OR_TOTAL_TIME);

#endif
