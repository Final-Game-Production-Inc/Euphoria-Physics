#ifndef VECTORMATH_LAYOUTCONVERT_H
#define VECTORMATH_LAYOUTCONVERT_H

#include "classes.h"
#include "classes_soa.h"

namespace rage
{
	//
	// Conversion routines for AoS <==> SoA.
	//
	
	// SoA: 1-tuple x 4 --> four ScalarV's, or --> one vec4v
	void ToAoS( ScalarV_InOut outVec0, ScalarV_InOut outVec1, ScalarV_InOut outVec2, ScalarV_InOut outVec3, SoA_ScalarV_In inVec );
	void ToAoS( Vec4V_InOut outVec, SoA_ScalarV_In inVec );
	// SoA: 2-tuple x 4 --> four vec2v's
	void ToAoS( Vec2V_InOut outVec0, Vec2V_InOut outVec1, Vec2V_InOut outVec2, Vec2V_InOut outVec3, SoA_Vec2V_In inVec );
	// SoA: 3-tuple x 4 --> four vec3v's
	void ToAoS( Vec3V_InOut outVec0, Vec3V_InOut outVec1, Vec3V_InOut outVec2, Vec3V_InOut outVec3, SoA_Vec3V_In inVec );
	// SoA: 4-tuple x 4 --> four vec4v's
	void ToAoS( Vec4V_InOut outVec0, Vec4V_InOut outVec1, Vec4V_InOut outVec2, Vec4V_InOut outVec3, SoA_Vec4V_In inVec );
	// SoA: 4-tuple x 4 --> four quatv's
	void ToAoS( QuatV_InOut outQuat0, QuatV_InOut outQuat1, QuatV_InOut outQuat2, QuatV_InOut outQuat3, SoA_QuatV_In inQuat );
	// SoA: 9-tuple x 4 --> four mat33v's
	void ToAoS( Mat33V_InOut outMat0, Mat33V_InOut outMat1, Mat33V_InOut outMat2, Mat33V_InOut outMat3, SoA_Mat33V_In inMat );
	// SoA: 12-tuple x 4 --> four mat34v's
	void ToAoS( Mat34V_InOut outMat0, Mat34V_InOut outMat1, Mat34V_InOut outMat2, Mat34V_InOut outMat3, SoA_Mat34V_In inMat );
	// SoA: 16-tuple x 4 --> four mat44v's
	void ToAoS( Mat44V_InOut outMat0, Mat44V_InOut outMat1, Mat44V_InOut outMat2, Mat44V_InOut outMat3, SoA_Mat44V_In inMat );

	// AoS: four ScalarV's, or one vec4v --> 1-tuple x 4
	void ToSoA( SoA_ScalarV_InOut outVec, ScalarV_In inVec0, ScalarV_In inVec1, ScalarV_In inVec2, ScalarV_In inVec3 );
	void ToSoA( SoA_ScalarV_InOut outVec, Vec4V_In inVec );
	// AoS: four vec2v's --> 2-tuple x 4
	void ToSoA( SoA_Vec2V_InOut outVec, Vec2V_In inVec0, Vec2V_In inVec1, Vec2V_In inVec2, Vec2V_In inVec3 );
	// AoS: four vec3v's --> 3-tuple x 4
	void ToSoA( SoA_Vec3V_InOut outVec, Vec3V_In inVec0, Vec3V_In inVec1, Vec3V_In inVec2, Vec3V_In inVec3 );
	// AoS: four vec4v's --> 4-tuple x 4
	void ToSoA( SoA_Vec4V_InOut outVec, Vec4V_In inVec0, Vec4V_In inVec1, Vec4V_In inVec2, Vec4V_In inVec3 );
	// AoS: four quatv's --> 4-tuple x 4
	void ToSoA( SoA_QuatV_InOut outQuat, QuatV_In inQuat0, QuatV_In inQuat1, QuatV_In inQuat2, QuatV_In inQuat3 );
	// AoS: four mat33v's --> 9-tuple x 4
	void ToSoA( SoA_Mat33V_InOut outMat, Mat33V_In inMat0, Mat33V_In inMat1, Mat33V_In inMat2, Mat33V_In inMat3 );
	// AoS: four mat34v's --> 12-tuple x 4
	void ToSoA( SoA_Mat34V_InOut outMat, Mat34V_In inMat0, Mat34V_In inMat1, Mat34V_In inMat2, Mat34V_In inMat3 );
	// AoS: four mat44v's --> 16-tuple x 4
	void ToSoA( SoA_Mat44V_InOut outMat, Mat44V_In inMat0, Mat44V_In inMat1, Mat44V_In inMat2, Mat44V_In inMat3 );

	// AoS: one mat34v --> 12-tuple x 4
	void ToSoA( SoA_Mat34V_InOut outMat, Mat34V_In inMat );
}

#include "layoutconvert.inl"

#endif // VECTORMATH_LAYOUTCONVERT_H
