#if __SPU
// This is #included ... why the heck is it a .cpp file?
#ifndef VECTOR3_CONSTS_SPU_H
#define VECTOR3_CONSTS_SPU_H

#include <float.h>
#include "math/constants.h"

namespace rage
{   

	static const vector float VEC3_XAXISc=(vector float ){ 1.0f, 0.0f, 0.0f, 0.0f };
	static const vector float VEC3_YAXISc=(vector float ){ 0.0f, 1.0f, 0.0f, 0.0f };
	static const vector float VEC3_ZAXISc=(vector float ){ 0.0f, 0.0f, 1.0f, 0.0f };
	static const vector float VEC3_ZEROc=(vector float){ 0.0f, 0.0f, 0.0f, 0.0f};

	#define XAXIS (__vector4(VEC3_XAXISc))
	#define YAXIS (__vector4(VEC3_YAXISc))
	#define ZAXIS (__vector4(VEC3_ZAXISc))

    static const vector float VEC3_IDENTITYc=(vector float){ 1.0f, 1.0f, 1.0f, 1.0f};
	static const vector float VEC3_IDENTITY=(vector float){ 1.0f, 1.0f, 1.0f, 1.0f};
	static const vector float VEC3_HALFc=(vector float){ 0.5f, 0.5f, 0.5f, 0.5f};
	static const vector float VEC3_THIRDc=(vector float){ 1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f };
	static const vector float VEC3_QUARTERc=(vector float){ 0.25f, 0.25f, 0.25f, 0.25f };
	static const vector float VEC3_SQRTTWOc=(vector float)(1.41421356237309504880f, 1.41421356237309504880f, 1.41421356237309504880f,0);
	static const vector float VEC3_SQRTTHREEc=(vector float)(1.73205080756887729352f, 1.73205080756887729352f, 1.73205080756887729352f,0);

	//!me should make sure nan and inf actually are these values on PS3 SPU
	static const vector unsigned char VEC3_INFc=(vector unsigned char){
		0x7f, 0x80, 0x00, 0x00,
		0x7f, 0x80, 0x00, 0x00,
		0x7f, 0x80, 0x00, 0x00,
		0x7f, 0x80, 0x00, 0x00};

	static const vector unsigned char VEC3_NANc=(vector unsigned char){
		0x7f, 0xc0, 0x00, 0x00,
		0x7f, 0xc0, 0x00, 0x00,
		0x7f, 0xc0, 0x00, 0x00,
		0x7f, 0xc0, 0x00, 0x00};

	static const vector float VEC3_ONEWc=(vector float){ 0.0f, 0.0f, 0.0f, 1.0f};
	static const vector float VEC3_MAXc=(vector float){ 0x1.fffffep+128f, 0x1.fffffep+128f, 0x1.fffffep+128f, 0x1.fffffep+128f};
	static const vector float VEC3_SMALL_FLOATc=(vector float){ SMALL_FLOAT, SMALL_FLOAT, SMALL_FLOAT, SMALL_FLOAT};
	static const vector float VEC3_VERY_SMALL_FLOATc=(vector float){ VERY_SMALL_FLOAT, VERY_SMALL_FLOAT, VERY_SMALL_FLOAT, VERY_SMALL_FLOAT};
	static const vector float VEC3_LARGE_FLOATc=(vector float){ LARGE_FLOAT, LARGE_FLOAT, LARGE_FLOAT, LARGE_FLOAT};

	static const vector unsigned char __VECTOR4_ANDXc=(vector unsigned char){
		0x00, 0x00, 0x00, 0x00,
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF};

    #define VEC3_ANDX (__vector4(::rage::__VECTOR4_ANDXc))

	static const vector unsigned char __VECTOR4_ANDYc=(vector unsigned char){
		0xFF, 0xFF, 0xFF, 0xFF,
		0x00, 0x00, 0x00, 0x00,
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF};

    #define VEC3_ANDY (__vector4(::rage::__VECTOR4_ANDYc))

	static const vector unsigned char __VECTOR4_ANDZc=(vector unsigned char){
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,
		0x00, 0x00, 0x00, 0x00,
		0xFF, 0xFF, 0xFF, 0xFF};

    #define VEC3_ANDZ (__vector4(::rage::__VECTOR4_ANDZc))

	static const vector unsigned char __VECTOR4_ALLBITSc=(vector unsigned char){
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,
		0x00, 0x00, 0x00, 0x00};

    #define VEC3_ANDW (__vector4(::rage::__VECTOR4_ALLBITSc))

	static const vector unsigned char __VECTOR4_MASKXc=(vector unsigned char){
		0xFF, 0xFF, 0xFF, 0xFF,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00};

    #define VEC3_MASKX (__vector4(::rage::__VECTOR4_MASKXc))

	static const vector unsigned char __VECTOR4_MASKYc=(vector unsigned char){
		0x00, 0x00, 0x00, 0x00,
		0xFF, 0xFF, 0xFF, 0xFF,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00};

    #define VEC3_MASKY (__vector4(::rage::__VECTOR4_MASKYc))

	static const vector unsigned char __VECTOR4_MASKZc=(vector unsigned char){
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0xFF, 0xFF, 0xFF, 0xFF,
		0x00, 0x00, 0x00, 0x00};

    #define VEC3_MASKZ (__vector4(::rage::__VECTOR4_MASKZc))

	static const vector unsigned char __VECTOR4_MASKWc=(vector unsigned char){
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0xFF, 0xFF, 0xFF, 0xFF};

    #define VEC3_MASKW (__vector4(::rage::__VECTOR4_MASKWc))

	static const vector unsigned char __VECTOR4_MASKXYZWc=(vector unsigned char){
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF};

    #define VEC3_MASKXYZW (__vector4(::rage::__VECTOR4_MASKXYZWc))

	#define VEC3_ZERO (__vector4(::rage::VEC3_ZEROc))
	#define ORIGIN VEC3_ZERO

	#define __VECTOR4_ALLBITS (__vector4(__VECTOR4_ALLBITS)

	#define __VECTOR4_ANDX (__vector4(::rage::__VECTOR4_ANDXc))
	#define __VECTOR4_ANDY (__vector4(::rage::__VECTOR4_ANDYc))
	#define __VECTOR4_ANDZ (__vector4(::rage::__VECTOR4_ANDZc))
	#define __VECTOR4_ANDW (__vector4(::rage::__VECTOR4_ANDWc))

	#define __VECTOR4_MASKX (__vector4(::rage::__VECTOR4_MASKXc))
	#define __VECTOR4_MASKY (__vector4(::rage::__VECTOR4_MASKYc))
	#define __VECTOR4_MASKZ (__vector4(::rage::__VECTOR4_MASKZc))
	#define __VECTOR4_MASKW (__vector4(::rage::__VECTOR4_MASKWc))

	#define __VECTOR4_ONLYW (__vector4(::rage::VEC3_ONEWc))
	#define __VECTOR4_ZERO VEC3_ZERO
	#define __VECTOR4_ONE (__vector4(::rage::VEC3_IDENTITYc))
	#define __VECTOR4_HALF (__vector4(::rage::VEC3_HALFc))
	#define __VECTOR3_THIRD (__vector3(::rage::VEC3_THIRDc))
	#define __VECTOR3_QUARTER (__vector3(::rage::VEC3_QUARTERc))
	#define __VECTOR3_SQRTTWO (__vector3(::rage::VEC3_SQRTTWOc))
	#define __VECTOR3_SQRTTHREE (__vector3(::rage::VEC3_SQRTTHREEc))
	#define __VECTOR4_INF (__vector4(::rage::VEC3_INFc))
	#define __VECTOR4_MAX (__vector4(::rage::VEC3_MAXc))
	#define __VECTOR4_NAN (__vector4(::rage::VEC3_NANc))
	#define __VECTOR4_SMALL_FLOAT (__vector4(::rage::VEC3_SMALL_FLOATc))
	#define __VECTOR4_VERY_SMALL_FLOAT (__vector4(::rage::VEC3_VERY_SMALL_FLOATc))
	#define __VECTOR4_LARGE_FLOAT (__vector4(::rage::VEC3_LARGE_FLOATc))
	#define VEC3_HALF (__vector4(::rage::VEC3_HALFc))
	#define VEC3_THIRD (__vector4(::rage::VEC3_THIRDc))
	#define VEC3_QUARTER (__vector4(::rage::VEC3_QUARTERc))
	#define VEC3_SQRTTWO (__vector4(::rage::VEC3_SQRTTWOc))
	#define VEC3_SQRTTHREE (__vector4(::rage::VEC3_SQRTTHREEc))
	#define VEC3_ONLYW (__vector4(::rage::VEC3_ONLYWc))
	#define VEC3_ONEW __VECTOR4_ONLYW
	#define VEC3_INF __VECTOR4_INF
	#define VEC3_NAN __VECTOR4_NAN
	#define VEC3_MAX __VECTOR4_MAX
	#define VEC3_SMALL_FLOAT __VECTOR4_SMALL_FLOAT
	#define VEC3_VERY_SMALL_FLOAT __VECTOR4_VERY_SMALL_FLOAT
	#define VEC3_LARGE_FLOAT __VECTOR4_LARGE_FLOAT
}

#endif // ndef VECTOR3_CONSTS_SPU_H

#endif
