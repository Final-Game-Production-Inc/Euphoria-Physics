/* PowerPC AltiVec include file.
   Copyright (C) 2002, 2003, 2004 Free Software Foundation, Inc.
   Contributed by Aldy Hernandez (aldyh@redhat.com).

   This file is part of GCC.

   GCC is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published
   by the Free Software Foundation; either version 2, or (at your
   option) any later version.

   GCC is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with GCC; see the file COPYING.  If not, write to the
   Free Software Foundation, 59 Temple Place - Suite 330, Boston,
   MA 02111-1307, USA.  */

/* As a special exception, if you include this header file into source
   files compiled by GCC, this header file does not by itself cause
   the resulting executable to be covered by the GNU General Public
   License.  This exception does not however invalidate any other
   reasons why the executable file might be covered by the GNU General
   Public License.  */

/* Implemented to conform to the specification included in the AltiVec
   Technology Programming Interface Manual (ALTIVECPIM/D 6/1999 Rev 0).  */

///// NOTE - Only occluder.cpp in rmocclude and occlusion.cpp in game\renderer gta5 use this code.
///// Once they are converted to the new vector library, this code can be deleted.

#ifndef ALTIVEC_INTRINSICS_H
#define ALTIVEC_INTRINSICS_H 1

#if __PPU
#include <vec_types.h>
#include <altivec.h>
#elif __SPU
#include <vmx2spu.h>
#elif __XENON
#include <ppcintrinsics.h>
#else
#error Unsupported platform
#endif

#if !__XENON

// ps3 versions of xenon's extra instructions

#ifndef USE_SCE_DOT_PRODUCT
#define USE_SCE_DOT_PRODUCT 0
#endif

__forceinline vec_float4 
vec_mul (vec_float4 a1, vec_float4 a2)
{
	return vec_madd(a1, a2, (vec_float4)vec_splat_s32(0));
}

__forceinline vec_float4 
vec_dot3 (vec_float4 a1, vec_float4 a2)
{
#if USE_SCE_DOT_PRODUCT
	vec_float4 r = vec_mul(a1, a2);
	r = vec_madd(vec_sld(a1, a1, 4), vec_sld(a2, a2, 4), r);
	r = vec_madd(vec_sld(a1, a1, 8), vec_sld(a2, a2, 8), r);
	return vec_splat(r, 0);
#else
	vec_float4 prod = vec_mul(a1, a2);
	return vec_add(vec_add(vec_splat(prod,0),vec_splat(prod,1)),vec_splat(prod,2));
#endif
}

__forceinline vec_float4 
vec_dot4 (vec_float4 a1, vec_float4 a2)
{
	vec_float4 r = vec_mul(a1, a2);
	r = vec_madd(vec_sld(a1, a1, 4), vec_sld(a2, a2, 4), r);
	return vec_add(vec_sld(r, r, 8), r);
}

#else

// gcc's altivec.h converted for xenon by matts 16/10/07

typedef int OFFSET_T;

// type-safe vector template
template<class T>
struct vec_type
{
	union
	{
		T i[sizeof(__vector4)/sizeof(T)];
		__vector4 v;
	};
	typedef T value_type;
	// conversion operators (these are much less safe than GCC equivalents because all casts become implicit)
	__forceinline operator __vector4& () {return v;}
	__forceinline operator __vector4 () const {return v;}
	template<class O> __forceinline operator vec_type<O> () const 
	{
		vec_type<O> r; r.v = v; return r;
	}
};
typedef vec_type<unsigned char> vec_uchar16;
typedef vec_type<signed char> vec_char16;
typedef vec_type<unsigned short> vec_ushort8;
typedef vec_type<signed short> vec_short8;
typedef vec_type<unsigned int> vec_uint4;
typedef vec_type<signed int> vec_int4;
typedef vec_type<unsigned __int64> vec_ullong2;
typedef vec_type<signed __int64> vec_llong2;
typedef vec_type<double> vec_double2;
typedef vec_type<float> vec_float4;

// not even sure what the point of these pixel/bool types is ..
struct vpixel {rage::u8 p;}; 
struct vbool8 {rage::u8 b;};
struct vbool16 {rage::u16 b;};
struct vbool32 {rage::u32 b;};
typedef vec_type<vbool8> vec_bool16;
typedef vec_type<vbool16> vec_bool8;
typedef vec_type<vbool32> vec_bool4;
typedef vec_type<vpixel> vec_pixel;

/* vector memory hints */
#define __lvxl __lvx
#define __lvlxl __lvlx
#define __lvrxl __lvrx
#define __stvxl __stvx
#define __stvlxl __stvlx
#define __stvrxl __stvrx

/* These are easy... Same exact arguments.  */

#define vec_vaddcuw vec_addc
#define vec_vand vec_and
#define vec_vandc vec_andc
#define vec_vrfip vec_ceil
#define vec_vcmpbfp vec_cmpb
#define vec_vcmpgefp vec_cmpge
#define vec_vctsxs vec_cts
#define vec_vctuxs vec_ctu
#define vec_vexptefp vec_expte
#define vec_vrfim vec_floor
#define vec_lvx vec_ld
#define vec_lvxl vec_ldl
#define vec_vlogefp vec_loge
#define vec_vmaddfp vec_madd
#define vec_vmhaddshs vec_madds
#define vec_vmladduhm vec_mladd
#define vec_vmhraddshs vec_mradds
#define vec_vnmsubfp vec_nmsub
#define vec_vnor vec_nor
#define vec_vor vec_or
#define vec_vpkpx vec_packpx
#define vec_vperm vec_perm
#define vec_vrefp vec_re
#define vec_vrfin vec_round
#define vec_vrsqrtefp vec_rsqrte
#define vec_vsel vec_sel
#define vec_vsldoi vec_sld
#define vec_vsl vec_sll
#define vec_vslo vec_slo
#define vec_vspltisb vec_splat_s8
#define vec_vspltish vec_splat_s16
#define vec_vspltisw vec_splat_s32
#define vec_vsr vec_srl
#define vec_vsro vec_sro
#define vec_stvx vec_st
#define vec_stvxl vec_stl
#define vec_vsubcuw vec_subc
#define vec_vsum2sws vec_sum2s
#define vec_vsumsws vec_sums
#define vec_vrfiz vec_trunc
#define vec_vxor vec_xor

/* vec_step */

template<class T> int vec_step(T) {return sizeof(__vector4)/sizeof(T::value_type);}

/* vec_add */

__forceinline vec_char16
vec_add (vec_bool16 a1, vec_char16 a2)
{
	vec_char16 r; 
	r.v = __vaddubm (a1, a2);
	return r;
}

__forceinline vec_char16
vec_add (vec_char16 a1, vec_bool16 a2)
{
	vec_char16 r;
	r.v = __vaddubm (a1, a2);
	return r;
}

__forceinline vec_char16
vec_add (vec_char16 a1, vec_char16 a2)
{
	vec_char16 r;
	r.v = __vaddubm (a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_add (vec_bool16 a1, vec_uchar16 a2)
{
	vec_uchar16 r;
	r.v = __vaddubm (a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_add (vec_uchar16 a1, vec_bool16 a2)
{
	vec_uchar16 r;
	r.v = __vaddubm (a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_add (vec_uchar16 a1, vec_uchar16 a2)
{
	vec_uchar16 r;
	r.v = __vaddubm (a1, a2);
	return r;
}

__forceinline vec_short8
vec_add (vec_bool8 a1, vec_short8 a2)
{
	vec_short8 r;
	r.v = __vadduhm (a1, a2);
	return r;
}

__forceinline vec_short8
vec_add (vec_short8 a1, vec_bool8 a2)
{
	vec_short8 r;
	r.v = __vadduhm (a1, a2);
	return r;
}

__forceinline vec_short8
vec_add (vec_short8 a1, vec_short8 a2)
{
	vec_short8 r;
	r.v = __vadduhm (a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_add (vec_bool8 a1, vec_ushort8 a2)
{
	vec_ushort8 r;
	r.v = __vadduhm (a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_add (vec_ushort8 a1, vec_bool8 a2)
{
	vec_ushort8 r;
	r.v = __vadduhm (a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_add (vec_ushort8 a1, vec_ushort8 a2)
{
	vec_ushort8 r;
	r.v = __vadduhm(a1, a2);
	return r;
}

__forceinline vec_int4
vec_add (vec_bool4 a1, vec_int4 a2)
{
	vec_int4 r;
	r.v = __vadduwm(a1, a2);
	return r;
}

__forceinline vec_int4
vec_add (vec_int4 a1, vec_bool4 a2)
{
	vec_int4 r;
	r.v = __vadduwm(a1, a2);
	return r;
}

__forceinline vec_int4
vec_add (vec_int4 a1, vec_int4 a2)
{
	vec_int4 r;
	r.v = __vadduwm(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_add (vec_bool4 a1, vec_uint4 a2)
{
	vec_uint4 r;
	r.v = __vadduwm(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_add (vec_uint4 a1, vec_bool4 a2)
{
	vec_uint4 r;
	r.v = __vadduwm(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_add (vec_uint4 a1, vec_uint4 a2)
{
	vec_uint4 r;
	r.v = __vadduwm(a1, a2);
	return r;
}

__forceinline vec_float4
vec_add (vec_float4 a1, vec_float4 a2)
{
	vec_float4 r;
	r.v = __vaddfp(a1, a2);
	return r;
}

/* vec_vaddfp */

__forceinline vec_float4
vec_vaddfp (vec_float4 a1, vec_float4 a2)
{
	vec_float4 r;
	r.v = __vaddfp(a1, a2);
	return r;
}

/* vec_vadduwm */

__forceinline vec_int4
vec_vadduwm (vec_bool4 a1, vec_int4 a2)
{
	vec_int4 r;
	r.v = __vadduwm(a1, a2);
	return r;
}

__forceinline vec_int4
vec_vadduwm (vec_int4 a1, vec_bool4 a2)
{
	vec_int4 r;
	r.v = __vadduwm(a1, a2);
	return r;
}

__forceinline vec_int4
vec_vadduwm (vec_int4 a1, vec_int4 a2)
{
	vec_int4 r;
	r.v = __vadduwm(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_vadduwm (vec_bool4 a1, vec_uint4 a2)
{
	vec_uint4 r;
	r.v = __vadduwm(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_vadduwm (vec_uint4 a1, vec_bool4 a2)
{
	vec_uint4 r;
	r.v = __vadduwm(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_vadduwm (vec_uint4 a1, vec_uint4 a2)
{
	vec_uint4 r;
	r.v = __vadduwm(a1, a2);
	return r;
}

/* vec_vadduhm */

__forceinline vec_short8
vec_vadduhm (vec_bool8 a1, vec_short8 a2)
{
	vec_short8 r;
	r.v = __vadduhm(a1, a2);
	return r;
}

__forceinline vec_short8
vec_vadduhm (vec_short8 a1, vec_bool8 a2)
{
	vec_short8 r;
	r.v = __vadduhm(a1, a2);
	return r;
}

__forceinline vec_short8
vec_vadduhm (vec_short8 a1, vec_short8 a2)
{
	vec_short8 r;
	r.v = __vadduhm(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_vadduhm (vec_bool8 a1, vec_ushort8 a2)
{
	vec_ushort8 r;
	r.v = __vadduhm(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_vadduhm (vec_ushort8 a1, vec_bool8 a2)
{
	vec_ushort8 r;
	r.v = __vadduhm(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_vadduhm (vec_ushort8 a1, vec_ushort8 a2)
{
	vec_ushort8 r;
	r.v = __vadduhm(a1, a2);
	return r;
}

/* vec_vaddubm */

__forceinline vec_char16
vec_vaddubm (vec_bool16 a1, vec_char16 a2)
{
	vec_char16 r;
	r.v = __vaddubm(a1, a2);
	return r;
}

__forceinline vec_char16
vec_vaddubm (vec_char16 a1, vec_bool16 a2)
{
	vec_char16 r;
	r.v = __vaddubm(a1, a2);
	return r;
}

__forceinline vec_char16
vec_vaddubm (vec_char16 a1, vec_char16 a2)
{
	vec_char16 r;
	r.v = __vaddubm(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_vaddubm (vec_bool16 a1, vec_uchar16 a2)
{
	vec_uchar16 r;
	r.v = __vaddubm(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_vaddubm (vec_uchar16 a1, vec_bool16 a2)
{
	vec_uchar16 r;
	r.v = __vaddubm(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_vaddubm (vec_uchar16 a1, vec_uchar16 a2)
{
	vec_uchar16 r;
	r.v = __vaddubm(a1, a2);
	return r;
}

/* vec_addc */

__forceinline vec_uint4
vec_addc (vec_uint4 a1, vec_uint4 a2)
{
	vec_uint4 r;
	r.v = __vaddcuw(a1, a2);
	return r;
}

/* vec_adds */

__forceinline vec_uchar16
vec_adds (vec_bool16 a1, vec_uchar16 a2)
{
	vec_uchar16 r;
	r.v = __vaddubs(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_adds (vec_uchar16 a1, vec_bool16 a2)
{
	vec_uchar16 r;
	r.v = __vaddubs(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_adds (vec_uchar16 a1, vec_uchar16 a2)
{
	vec_uchar16 r;
	r.v = __vaddubs(a1, a2);
	return r;
}

__forceinline vec_char16
vec_adds (vec_bool16 a1, vec_char16 a2)
{
	vec_char16 r;
	r.v = __vaddsbs(a1, a2);
	return r;
}

__forceinline vec_char16
vec_adds (vec_char16 a1, vec_bool16 a2)
{
	vec_char16 r;
	r.v = __vaddsbs(a1, a2);
	return r;
}

__forceinline vec_char16
vec_adds (vec_char16 a1, vec_char16 a2)
{
	vec_char16 r;
	r.v = __vaddsbs(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_adds (vec_bool8 a1, vec_ushort8 a2)
{
	vec_ushort8 r;
	r.v = __vadduhs(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_adds (vec_ushort8 a1, vec_bool8 a2)
{
	vec_ushort8 r;
	r.v = __vadduhs(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_adds (vec_ushort8 a1, vec_ushort8 a2)
{
	vec_ushort8 r;
	r.v = __vadduhs(a1, a2);
	return r;
}

__forceinline vec_short8
vec_adds (vec_bool8 a1, vec_short8 a2)
{
	vec_short8 r;
	r.v = __vaddshs(a1, a2);
	return r;
}

__forceinline vec_short8
vec_adds (vec_short8 a1, vec_bool8 a2)
{
	vec_short8 r;
	r.v = __vaddshs(a1, a2);
	return r;
}

__forceinline vec_short8
vec_adds (vec_short8 a1, vec_short8 a2)
{
	vec_short8 r;
	r.v = __vaddshs(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_adds (vec_bool4 a1, vec_uint4 a2)
{
	vec_uint4 r;
	r.v = __vadduws(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_adds (vec_uint4 a1, vec_bool4 a2)
{
	vec_uint4 r;
	r.v = __vadduws(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_adds (vec_uint4 a1, vec_uint4 a2)
{
	vec_uint4 r;
	r.v = __vadduws(a1, a2);
	return r;
}

__forceinline vec_int4
vec_adds (vec_bool4 a1, vec_int4 a2)
{
	vec_int4 r;
	r.v = __vaddsws(a1, a2);
	return r;
}

__forceinline vec_int4
vec_adds (vec_int4 a1, vec_bool4 a2)
{
	vec_int4 r;
	r.v = __vaddsws(a1, a2);
	return r;
}

__forceinline vec_int4
vec_adds (vec_int4 a1, vec_int4 a2)
{
	vec_int4 r;
	r.v = __vaddsws(a1, a2);
	return r;
}

/* vec_vaddsws */

__forceinline vec_int4
vec_vaddsws (vec_bool4 a1, vec_int4 a2)
{
	vec_int4 r;
	r.v = __vaddsws(a1, a2);
	return r;
}

__forceinline vec_int4
vec_vaddsws (vec_int4 a1, vec_bool4 a2)
{
	vec_int4 r;
	r.v = __vaddsws(a1, a2);
	return r;
}

__forceinline vec_int4
vec_vaddsws (vec_int4 a1, vec_int4 a2)
{
	vec_int4 r;
	r.v = __vaddsws(a1, a2);
	return r;
}

/* vec_vadduws */

__forceinline vec_uint4
vec_vadduws (vec_bool4 a1, vec_uint4 a2)
{
	vec_uint4 r;
	r.v = __vadduws(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_vadduws (vec_uint4 a1, vec_bool4 a2)
{
	vec_uint4 r;
	r.v = __vadduws(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_vadduws (vec_uint4 a1, vec_uint4 a2)
{
	vec_uint4 r;
	r.v = __vadduws(a1, a2);
	return r;
}

/* vec_vaddshs */

__forceinline vec_short8
vec_vaddshs (vec_bool8 a1, vec_short8 a2)
{
	vec_short8 r;
	r.v = __vaddshs(a1, a2);
	return r;
}

__forceinline vec_short8
vec_vaddshs (vec_short8 a1, vec_bool8 a2)
{
	vec_short8 r;
	r.v = __vaddshs(a1, a2);
	return r;
}

__forceinline vec_short8
vec_vaddshs (vec_short8 a1, vec_short8 a2)
{
	vec_short8 r;
	r.v = __vaddshs(a1, a2);
	return r;
}

/* vec_vadduhs */

__forceinline vec_ushort8
vec_vadduhs (vec_bool8 a1, vec_ushort8 a2)
{
	vec_ushort8 r;
	r.v = __vadduhs(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_vadduhs (vec_ushort8 a1, vec_bool8 a2)
{
	vec_ushort8 r;
	r.v = __vadduhs(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_vadduhs (vec_ushort8 a1, vec_ushort8 a2)
{
	vec_ushort8 r;
	r.v = __vadduhs(a1, a2);
	return r;
}

/* vec_vaddsbs */

__forceinline vec_char16
vec_vaddsbs (vec_bool16 a1, vec_char16 a2)
{
	vec_char16 r;
	r.v = __vaddsbs(a1, a2);
	return r;
}

__forceinline vec_char16
vec_vaddsbs (vec_char16 a1, vec_bool16 a2)
{
	vec_char16 r;
	r.v = __vaddsbs(a1, a2);
	return r;
}

__forceinline vec_char16
vec_vaddsbs (vec_char16 a1, vec_char16 a2)
{
	vec_char16 r;
	r.v = __vaddsbs(a1, a2);
	return r;
}

/* vec_vaddubs */

__forceinline vec_uchar16
vec_vaddubs (vec_bool16 a1, vec_uchar16 a2)
{
	vec_uchar16 r;
	r.v = __vaddubs(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_vaddubs (vec_uchar16 a1, vec_bool16 a2)
{
	vec_uchar16 r;
	r.v = __vaddubs(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_vaddubs (vec_uchar16 a1, vec_uchar16 a2)
{
	vec_uchar16 r;
	r.v = __vaddubs(a1, a2);
	return r;
}

/* vec_and */

__forceinline vec_float4
vec_and (vec_float4 a1, vec_float4 a2)
{
	vec_float4 r;
	r.v = __vand(a1, a2);
	return r;
}

__forceinline vec_float4
vec_and (vec_float4 a1, vec_bool4 a2)
{
	vec_float4 r;
	r.v = __vand(a1, a2);
	return r;
}

__forceinline vec_float4
vec_and (vec_bool4 a1, vec_float4 a2)
{
	vec_float4 r;
	r.v = __vand(a1, a2);
	return r;
}

__forceinline vec_bool4
vec_and (vec_bool4 a1, vec_bool4 a2)
{
	vec_bool4 r;
	r.v = __vand(a1, a2);
	return r;
}

__forceinline vec_int4
vec_and (vec_bool4 a1, vec_int4 a2)
{
	vec_int4 r;
	r.v = __vand(a1, a2);
	return r;
}

__forceinline vec_int4
vec_and (vec_int4 a1, vec_bool4 a2)
{
	vec_int4 r;
	r.v = __vand(a1, a2);
	return r;
}

__forceinline vec_int4
vec_and (vec_int4 a1, vec_int4 a2)
{
	vec_int4 r;
	r.v = __vand(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_and (vec_bool4 a1, vec_uint4 a2)
{
	vec_uint4 r;
	r.v = __vand(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_and (vec_uint4 a1, vec_bool4 a2)
{
	vec_uint4 r;
	r.v = __vand(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_and (vec_uint4 a1, vec_uint4 a2)
{
	vec_uint4 r;
	r.v = __vand(a1, a2);
	return r;
}

__forceinline vec_bool8
vec_and (vec_bool8 a1, vec_bool8 a2)
{
	vec_bool8 r;
	r.v = __vand(a1, a2);
	return r;
}

__forceinline vec_short8
vec_and (vec_bool8 a1, vec_short8 a2)
{
	vec_short8 r;
	r.v = __vand(a1, a2);
	return r;
}

__forceinline vec_short8
vec_and (vec_short8 a1, vec_bool8 a2)
{
	vec_short8 r;
	r.v = __vand(a1, a2);
	return r;
}

__forceinline vec_short8
vec_and (vec_short8 a1, vec_short8 a2)
{
	vec_short8 r;
	r.v = __vand(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_and (vec_bool8 a1, vec_ushort8 a2)
{
	vec_ushort8 r;
	r.v = __vand(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_and (vec_ushort8 a1, vec_bool8 a2)
{
	vec_ushort8 r;
	r.v = __vand(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_and (vec_ushort8 a1, vec_ushort8 a2)
{
	vec_ushort8 r;
	r.v = __vand(a1, a2);
	return r;
}

__forceinline vec_char16
vec_and (vec_bool16 a1, vec_char16 a2)
{
	vec_char16 r;
	r.v = __vand(a1, a2);
	return r;
}

__forceinline vec_bool16
vec_and (vec_bool16 a1, vec_bool16 a2)
{
	vec_bool16 r;
	r.v = __vand(a1, a2);
	return r;
}

__forceinline vec_char16
vec_and (vec_char16 a1, vec_bool16 a2)
{
	vec_char16 r;
	r.v = __vand(a1, a2);
	return r;
}

__forceinline vec_char16
vec_and (vec_char16 a1, vec_char16 a2)
{
	vec_char16 r;
	r.v = __vand(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_and (vec_bool16 a1, vec_uchar16 a2)
{
	vec_uchar16 r;
	r.v = __vand(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_and (vec_uchar16 a1, vec_bool16 a2)
{
	vec_uchar16 r;
	r.v = __vand(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_and (vec_uchar16 a1, vec_uchar16 a2)
{
	vec_uchar16 r;
	r.v = __vand(a1, a2);
	return r;
}

/* vec_andc */

__forceinline vec_float4
vec_andc (vec_float4 a1, vec_float4 a2)
{
	vec_float4 r;
	r.v = __vandc(a1, a2);
	return r;
}

__forceinline vec_float4
vec_andc (vec_float4 a1, vec_bool4 a2)
{
	vec_float4 r;
	r.v = __vandc(a1, a2);
	return r;
}

__forceinline vec_float4
vec_andc (vec_bool4 a1, vec_float4 a2)
{
	vec_float4 r;
	r.v = __vandc(a1, a2);
	return r;
}

__forceinline vec_bool4
vec_andc (vec_bool4 a1, vec_bool4 a2)
{
	vec_bool4 r;
	r.v = __vandc(a1, a2);
	return r;
}

__forceinline vec_int4
vec_andc (vec_bool4 a1, vec_int4 a2)
{
	vec_int4 r;
	r.v = __vandc(a1, a2);
	return r;
}

__forceinline vec_int4
vec_andc (vec_int4 a1, vec_bool4 a2)
{
	vec_int4 r;
	r.v = __vandc(a1, a2);
	return r;
}

__forceinline vec_int4
vec_andc (vec_int4 a1, vec_int4 a2)
{
	vec_int4 r;
	r.v = __vandc(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_andc (vec_bool4 a1, vec_uint4 a2)
{
	vec_uint4 r;
	r.v = __vandc(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_andc (vec_uint4 a1, vec_bool4 a2)
{
	vec_uint4 r;
	r.v = __vandc(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_andc (vec_uint4 a1, vec_uint4 a2)
{
	vec_uint4 r;
	r.v = __vandc(a1, a2);
	return r;
}

__forceinline vec_bool8
vec_andc (vec_bool8 a1, vec_bool8 a2)
{
	vec_bool8 r;
	r.v = __vandc(a1, a2);
	return r;
}

__forceinline vec_short8
vec_andc (vec_bool8 a1, vec_short8 a2)
{
	vec_short8 r;
	r.v = __vandc(a1, a2);
	return r;
}

__forceinline vec_short8
vec_andc (vec_short8 a1, vec_bool8 a2)
{
	vec_short8 r;
	r.v = __vandc(a1, a2);
	return r;
}

__forceinline vec_short8
vec_andc (vec_short8 a1, vec_short8 a2)
{
	vec_short8 r;
	r.v = __vandc(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_andc (vec_bool8 a1, vec_ushort8 a2)
{
	vec_ushort8 r;
	r.v = __vandc(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_andc (vec_ushort8 a1, vec_bool8 a2)
{
	vec_ushort8 r;
	r.v = __vandc(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_andc (vec_ushort8 a1, vec_ushort8 a2)
{
	vec_ushort8 r;
	r.v = __vandc(a1, a2);
	return r;
}

__forceinline vec_char16
vec_andc (vec_bool16 a1, vec_char16 a2)
{
	vec_char16 r;
	r.v = __vandc(a1, a2);
	return r;
}

__forceinline vec_bool16
vec_andc (vec_bool16 a1, vec_bool16 a2)
{
	vec_bool16 r;
	r.v = __vandc(a1, a2);
	return r;
}

__forceinline vec_char16
vec_andc (vec_char16 a1, vec_bool16 a2)
{
	vec_char16 r;
	r.v = __vandc(a1, a2);
	return r;
}

__forceinline vec_char16
vec_andc (vec_char16 a1, vec_char16 a2)
{
	vec_char16 r;
	r.v = __vandc(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_andc (vec_bool16 a1, vec_uchar16 a2)
{
	vec_uchar16 r;
	r.v = __vandc(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_andc (vec_uchar16 a1, vec_bool16 a2)
{
	vec_uchar16 r;
	r.v = __vandc(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_andc (vec_uchar16 a1, vec_uchar16 a2)
{
	vec_uchar16 r;
	r.v = __vandc(a1, a2);
	return r;
}

/* vec_avg */

__forceinline vec_uchar16
vec_avg (vec_uchar16 a1, vec_uchar16 a2)
{
	vec_uchar16 r;
	r.v = __vavgub(a1, a2);
	return r;
}

__forceinline vec_char16
vec_avg (vec_char16 a1, vec_char16 a2)
{
	vec_char16 r;
	r.v = __vavgsb(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_avg (vec_ushort8 a1, vec_ushort8 a2)
{
	vec_ushort8 r;
	r.v = __vavguh(a1, a2);
	return r;
}

__forceinline vec_short8
vec_avg (vec_short8 a1, vec_short8 a2)
{
	vec_short8 r;
	r.v = __vavgsh(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_avg (vec_uint4 a1, vec_uint4 a2)
{
	vec_uint4 r;
	r.v = __vavguw(a1, a2);
	return r;
}

__forceinline vec_int4
vec_avg (vec_int4 a1, vec_int4 a2)
{
	vec_int4 r;
	r.v = __vavgsw(a1, a2);
	return r;
}

/* vec_vavgsw */

__forceinline vec_int4
vec_vavgsw (vec_int4 a1, vec_int4 a2)
{
	vec_int4 r;
	r.v = __vavgsw(a1, a2);
	return r;
}

/* vec_vavguw */

__forceinline vec_uint4
vec_vavguw (vec_uint4 a1, vec_uint4 a2)
{
	vec_uint4 r;
	r.v = __vavguw(a1, a2);
	return r;
}

/* vec_vavgsh */

__forceinline vec_short8
vec_vavgsh (vec_short8 a1, vec_short8 a2)
{
	vec_short8 r;
	r.v = __vavgsh(a1, a2);
	return r;
}

/* vec_vavguh */

__forceinline vec_ushort8
vec_vavguh (vec_ushort8 a1, vec_ushort8 a2)
{
	vec_ushort8 r;
	r.v = __vavguh(a1, a2);
	return r;
}

/* vec_vavgsb */

__forceinline vec_char16
vec_vavgsb (vec_char16 a1, vec_char16 a2)
{
	vec_char16 r;
	r.v = __vavgsb(a1, a2);
	return r;
}

/* vec_vavgub */

__forceinline vec_uchar16
vec_vavgub (vec_uchar16 a1, vec_uchar16 a2)
{
	vec_uchar16 r;
	r.v = __vavgub(a1, a2);
	return r;
}

/* vec_ceil */

__forceinline vec_float4
vec_ceil (vec_float4 a1)
{
	vec_float4 r;
	r.v = __vrfip(a1);
	return r;
}

/* vec_cmpb */

__forceinline vec_int4
vec_cmpb (vec_float4 a1, vec_float4 a2)
{
	vec_int4 r;
	r.v = __vcmpbfp(a1, a2);
	return r;
}

/* vec_cmpeq */

__forceinline vec_bool16
vec_cmpeq (vec_char16 a1, vec_char16 a2)
{
	vec_bool16 r;
	r.v = __vcmpequb(a1, a2);
	return r;
}

__forceinline vec_bool16
vec_cmpeq (vec_uchar16 a1, vec_uchar16 a2)
{
	vec_bool16 r;
	r.v = __vcmpequb(a1, a2);
	return r;
}

__forceinline vec_bool8
vec_cmpeq (vec_short8 a1, vec_short8 a2)
{
	vec_bool8 r;
	r.v = __vcmpequh(a1, a2);
	return r;
}

__forceinline vec_bool8
vec_cmpeq (vec_ushort8 a1, vec_ushort8 a2)
{
	vec_bool8 r;
	r.v = __vcmpequh(a1, a2);
	return r;
}

__forceinline vec_bool4
vec_cmpeq (vec_int4 a1, vec_int4 a2)
{
	vec_bool4 r;
	r.v = __vcmpequw(a1, a2);
	return r;
}

__forceinline vec_bool4
vec_cmpeq (vec_uint4 a1, vec_uint4 a2)
{
	vec_bool4 r;
	r.v = __vcmpequw(a1, a2);
	return r;
}

__forceinline vec_bool4
vec_cmpeq (vec_float4 a1, vec_float4 a2)
{
	vec_bool4 r;
	r.v = __vcmpeqfp(a1, a2);
	return r;
}

/* vec_vcmpeqfp */

__forceinline vec_bool4
vec_vcmpeqfp (vec_float4 a1, vec_float4 a2)
{
	vec_bool4 r;
	r.v = __vcmpeqfp(a1, a2);
	return r;
}

/* vec_vcmpequw */

__forceinline vec_bool4
vec_vcmpequw (vec_int4 a1, vec_int4 a2)
{
	vec_bool4 r;
	r.v = __vcmpequw(a1, a2);
	return r;
}

__forceinline vec_bool4
vec_vcmpequw (vec_uint4 a1, vec_uint4 a2)
{
	vec_bool4 r;
	r.v = __vcmpequw(a1, a2);
	return r;
}

/* vec_vcmpequh */

__forceinline vec_bool8
vec_vcmpequh (vec_short8 a1, vec_short8 a2)
{
	vec_bool8 r;
	r.v = __vcmpequh(a1, a2);
	return r;
}

__forceinline vec_bool8
vec_vcmpequh (vec_ushort8 a1, vec_ushort8 a2)
{
	vec_bool8 r;
	r.v = __vcmpequh(a1, a2);
	return r;
}

/* vec_vcmpequb */

__forceinline vec_bool16
vec_vcmpequb (vec_char16 a1, vec_char16 a2)
{
	vec_bool16 r;
	r.v = __vcmpequb(a1, a2);
	return r;
}

__forceinline vec_bool16
vec_vcmpequb (vec_uchar16 a1, vec_uchar16 a2)
{
	vec_bool16 r;
	r.v = __vcmpequb(a1, a2);
	return r;
}

/* vec_cmpge */

__forceinline vec_bool4
vec_cmpge (vec_float4 a1, vec_float4 a2)
{
	vec_bool4 r;
	r.v = __vcmpgefp(a1, a2);
	return r;
}

/* vec_cmpgt */

__forceinline vec_bool16
vec_cmpgt (vec_uchar16 a1, vec_uchar16 a2)
{
	vec_bool16 r;
	r.v = __vcmpgtub(a1, a2);
	return r;
}

__forceinline vec_bool16
vec_cmpgt (vec_char16 a1, vec_char16 a2)
{
	vec_bool16 r;
	r.v = __vcmpgtsb(a1, a2);
	return r;
}

__forceinline vec_bool8
vec_cmpgt (vec_ushort8 a1, vec_ushort8 a2)
{
	vec_bool8 r;
	r.v = __vcmpgtuh(a1, a2);
	return r;
}

__forceinline vec_bool8
vec_cmpgt (vec_short8 a1, vec_short8 a2)
{
	vec_bool8 r;
	r.v = __vcmpgtsh(a1, a2);
	return r;
}

__forceinline vec_bool4
vec_cmpgt (vec_uint4 a1, vec_uint4 a2)
{
	vec_bool4 r;
	r.v = __vcmpgtuw(a1, a2);
	return r;
}

__forceinline vec_bool4
vec_cmpgt (vec_int4 a1, vec_int4 a2)
{
	vec_bool4 r;
	r.v = __vcmpgtsw(a1, a2);
	return r;
}

__forceinline vec_bool4
vec_cmpgt (vec_float4 a1, vec_float4 a2)
{
	vec_bool4 r;
	r.v = __vcmpgtfp(a1, a2);
	return r;
}

/* vec_vcmpgtfp */

__forceinline vec_bool4
vec_vcmpgtfp (vec_float4 a1, vec_float4 a2)
{
	vec_bool4 r;
	r.v = __vcmpgtfp(a1, a2);
	return r;
}

/* vec_vcmpgtsw */

__forceinline vec_bool4
vec_vcmpgtsw (vec_int4 a1, vec_int4 a2)
{
	vec_bool4 r;
	r.v = __vcmpgtsw(a1, a2);
	return r;
}

/* vec_vcmpgtuw */

__forceinline vec_bool4
vec_vcmpgtuw (vec_uint4 a1, vec_uint4 a2)
{
	vec_bool4 r;
	r.v = __vcmpgtuw(a1, a2);
	return r;
}

/* vec_vcmpgtsh */

__forceinline vec_bool8
vec_vcmpgtsh (vec_short8 a1, vec_short8 a2)
{
	vec_bool8 r;
	r.v = __vcmpgtsh(a1, a2);
	return r;
}

/* vec_vcmpgtuh */

__forceinline vec_bool8
vec_vcmpgtuh (vec_ushort8 a1, vec_ushort8 a2)
{
	vec_bool8 r;
	r.v = __vcmpgtuh(a1, a2);
	return r;
}

/* vec_vcmpgtsb */

__forceinline vec_bool16
vec_vcmpgtsb (vec_char16 a1, vec_char16 a2)
{
	vec_bool16 r;
	r.v = __vcmpgtsb(a1, a2);
	return r;
}

/* vec_vcmpgtub */

__forceinline vec_bool16
vec_vcmpgtub (vec_uchar16 a1, vec_uchar16 a2)
{
	vec_bool16 r;
	r.v = __vcmpgtub(a1, a2);
	return r;
}

/* vec_cmple */

__forceinline vec_bool4
vec_cmple (vec_float4 a1, vec_float4 a2)
{
	vec_bool4 r;
	r.v = __vcmpgefp(a2, a1);
	return r;
}

/* vec_cmplt */

__forceinline vec_bool16
vec_cmplt (vec_uchar16 a1, vec_uchar16 a2)
{
	vec_bool16 r;
	r.v = __vcmpgtub(a2, a1);
	return r;
}

__forceinline vec_bool16
vec_cmplt (vec_char16 a1, vec_char16 a2)
{
	vec_bool16 r;
	r.v = __vcmpgtsb(a2, a1);
	return r;
}

__forceinline vec_bool8
vec_cmplt (vec_ushort8 a1, vec_ushort8 a2)
{
	vec_bool8 r;
	r.v = __vcmpgtuh(a2, a1);
	return r;
}

__forceinline vec_bool8
vec_cmplt (vec_short8 a1, vec_short8 a2)
{
	vec_bool8 r;
	r.v = __vcmpgtsh(a2, a1);
	return r;
}

__forceinline vec_bool4
vec_cmplt (vec_uint4 a1, vec_uint4 a2)
{
	vec_bool4 r;
	r.v = __vcmpgtuw(a2, a1);
	return r;
}

__forceinline vec_bool4
vec_cmplt (vec_int4 a1, vec_int4 a2)
{
	vec_bool4 r;
	r.v = __vcmpgtsw(a2, a1);
	return r;
}

__forceinline vec_bool4
vec_cmplt (vec_float4 a1, vec_float4 a2)
{
	vec_bool4 r;
	r.v = __vcmpgtfp(a2, a1);
	return r;
}

/* vec_ctf */

template<unsigned short a2> __forceinline vec_float4
vec_ctf (vec_uint4 a1)
{
	vec_float4 r;
	r.v = __vcfux(a1, a2);
	return r;
}

template<unsigned short a2> __forceinline vec_float4
vec_ctf (vec_int4 a1)
{
	vec_float4 r;
	r.v = __vcfsx(a1, a2);
	return r;
}

#define vec_ctf(a1, a2) vec_ctf<a2>(a1)

/* vec_vcfsx */

template<unsigned short a2> __forceinline vec_float4
vec_vcfsx (vec_int4 a1)
{
	vec_float4 r;
	r.v = __vcfsx(a1, a2);
	return r;
}

#define vec_vcfsx(a1, a2) vec_vcfsx<a2>(a1)

/* vec_vcfux */

template<unsigned short a2> __forceinline vec_float4
vec_vcfux (vec_uint4 a1)
{
	vec_float4 r;
	r.v = __vcfux(a1, a2);
	return r;
}

#define vec_vcfux(a1, a2) vec_vcfux<a2>(a1)

/* vec_cts */

template<int a2> __forceinline vec_int4
vec_cts (vec_float4 a1)
{
	vec_int4 r;
	r.v = __vctsxs(a1, a2);
	return r;
}

#define vec_cts(a1, a2) vec_cts<a2>(a1)

/* vec_ctu */

template<int a2> __forceinline vec_uint4
vec_ctu (vec_float4 a1)
{
	vec_uint4 r;
	r.v = __vctuxs(a1, a2);
	return r;
}

#define vec_ctu(a1, a2) vec_ctu<a2>(a1)

/* vec_expte */

__forceinline vec_float4
vec_expte (vec_float4 a1)
{
	vec_float4 r;
	r.v = __vexptefp(a1);
	return r;
}

/* vec_floor */

__forceinline vec_float4
vec_floor (vec_float4 a1)
{
	vec_float4 r;
	r.v = __vrfim(a1);
	return r;
}

/* vec_ld */

__forceinline vec_float4
vec_ld (OFFSET_T a1, const vec_float4 *a2)
{
	vec_float4 r;
	r.v = __lvx(a2, a1);
	return r;
}

__forceinline vec_float4
vec_ld (OFFSET_T a1, const float *a2)
{
	vec_float4 r;
	r.v = __lvx(a2, a1);
	return r;
}

__forceinline vec_bool4
vec_ld (OFFSET_T a1, const vec_bool4 *a2)
{
	vec_bool4 r;
	r.v = __lvx(a2, a1);
	return r;
}

__forceinline vec_int4
vec_ld (OFFSET_T a1, const vec_int4 *a2)
{
	vec_int4 r;
	r.v = __lvx(a2, a1);
	return r;
}

__forceinline vec_int4
vec_ld (OFFSET_T a1, const int *a2)
{
	vec_int4 r;
	r.v = __lvx(a2, a1);
	return r;
}

__forceinline vec_int4
vec_ld (OFFSET_T a1, const long *a2)
{
	vec_int4 r;
	r.v = __lvx(a2, a1);
	return r;
}

__forceinline vec_uint4
vec_ld (OFFSET_T a1, const vec_uint4 *a2)
{
	vec_uint4 r;
	r.v = __lvx(a2, a1);
	return r;
}

__forceinline vec_uint4
vec_ld (OFFSET_T a1, const unsigned int *a2)
{
	vec_uint4 r;
	r.v = __lvx(a2, a1);
	return r;
}

__forceinline vec_uint4
vec_ld (OFFSET_T a1, const unsigned long *a2)
{
	vec_uint4 r;
	r.v = __lvx(a2, a1);
	return r;
}

__forceinline vec_bool8
vec_ld (OFFSET_T a1, const vec_bool8 *a2)
{
	vec_bool8 r;
	r.v = __lvx(a2, a1);
	return r;
}

__forceinline vec_pixel
vec_ld (OFFSET_T a1, const vec_pixel *a2)
{
	vec_pixel r;
	r.v = __lvx(a2, a1);
	return r;
}

__forceinline vec_short8
vec_ld (OFFSET_T a1, const vec_short8 *a2)
{
	vec_short8 r;
	r.v = __lvx(a2, a1);
	return r;
}

__forceinline vec_short8
vec_ld (OFFSET_T a1, const short *a2)
{
	vec_short8 r;
	r.v = __lvx(a2, a1);
	return r;
}

__forceinline vec_ushort8
vec_ld (OFFSET_T a1, const vec_ushort8 *a2)
{
	vec_ushort8 r;
	r.v = __lvx(a2, a1);
	return r;
}

__forceinline vec_ushort8
vec_ld (OFFSET_T a1, const unsigned short *a2)
{
	vec_ushort8 r;
	r.v = __lvx(a2, a1);
	return r;
}

__forceinline vec_bool16
vec_ld (OFFSET_T a1, const vec_bool16 *a2)
{
	vec_bool16 r;
	r.v = __lvx(a2, a1);
	return r;
}

__forceinline vec_char16
vec_ld (OFFSET_T a1, const vec_char16 *a2)
{
	vec_char16 r;
	r.v = __lvx(a2, a1);
	return r;
}

__forceinline vec_char16
vec_ld (OFFSET_T a1, const signed char *a2)
{
	vec_char16 r;
	r.v = __lvx(a2, a1);
	return r;
}

__forceinline vec_uchar16
vec_ld (OFFSET_T a1, const vec_uchar16 *a2)
{
	vec_uchar16 r;
	r.v = __lvx(a2, a1);
	return r;
}

__forceinline vec_uchar16
vec_ld (OFFSET_T a1, const unsigned char *a2)
{
	vec_uchar16 r;
	r.v = __lvx(a2, a1);
	return r;
}

/* vec_lde */

__forceinline vec_char16
vec_lde (OFFSET_T a1, const signed char *a2)
{
	vec_char16 r;
	r.v = __lvebx(a2, a1);
	return r;
}

__forceinline vec_uchar16
vec_lde (OFFSET_T a1, const unsigned char *a2)
{
	vec_uchar16 r;
	r.v = __lvebx(a2, a1);
	return r;
}

__forceinline vec_short8
vec_lde (OFFSET_T a1, const short *a2)
{
	vec_short8 r;
	r.v = __lvehx(a2, a1);
	return r;
}

__forceinline vec_ushort8
vec_lde (OFFSET_T a1, const unsigned short *a2)
{
	vec_ushort8 r;
	r.v = __lvehx(a2, a1);
	return r;
}

__forceinline vec_float4
vec_lde (OFFSET_T a1, const float *a2)
{
	vec_float4 r;
	r.v = __lvewx(a2, a1);
	return r;
}

__forceinline vec_int4
vec_lde (OFFSET_T a1, const int *a2)
{
	vec_int4 r;
	r.v = __lvewx(a2, a1);
	return r;
}

__forceinline vec_uint4
vec_lde (OFFSET_T a1, const unsigned int *a2)
{
	vec_uint4 r;
	r.v = __lvewx(a2, a1);
	return r;
}

__forceinline vec_int4
vec_lde (OFFSET_T a1, const long *a2)
{
	vec_int4 r;
	r.v = __lvewx(a2, a1);
	return r;
}

__forceinline vec_uint4
vec_lde (OFFSET_T a1, const unsigned long *a2)
{
	vec_uint4 r;
	r.v = __lvewx(a2, a1);
	return r;
}

/* vec_lvewx */

__forceinline vec_float4
vec_lvewx (OFFSET_T a1, float *a2)
{
	vec_float4 r;
	r.v = __lvewx(a2, a1);
	return r;
}

__forceinline vec_int4
vec_lvewx (OFFSET_T a1, int *a2)
{
	vec_int4 r;
	r.v = __lvewx(a2, a1);
	return r;
}

__forceinline vec_uint4
vec_lvewx (OFFSET_T a1, unsigned int *a2)
{
	vec_uint4 r;
	r.v = __lvewx(a2, a1);
	return r;
}

__forceinline vec_int4
vec_lvewx (OFFSET_T a1, long *a2)
{
	vec_int4 r;
	r.v = __lvewx(a2, a1);
	return r;
}

__forceinline vec_uint4
vec_lvewx (OFFSET_T a1, unsigned long *a2)
{
	vec_uint4 r;
	r.v = __lvewx(a2, a1);
	return r;
}

/* vec_lvehx */

__forceinline vec_short8
vec_lvehx (OFFSET_T a1, short *a2)
{
	vec_short8 r;
	r.v = __lvehx(a2, a1);
	return r;
}

__forceinline vec_ushort8
vec_lvehx (OFFSET_T a1, unsigned short *a2)
{
	vec_ushort8 r;
	r.v = __lvehx(a2, a1);
	return r;
}

/* vec_lvebx */

__forceinline vec_char16
vec_lvebx (OFFSET_T a1, signed char *a2)
{
	vec_char16 r;
	r.v = __lvebx(a2, a1);
	return r;
}

__forceinline vec_uchar16
vec_lvebx (OFFSET_T a1, unsigned char *a2)
{
	vec_uchar16 r;
	r.v = __lvebx(a2, a1);
	return r;
}

/* vec_ldl */

__forceinline vec_float4
vec_ldl (OFFSET_T a1, const vec_float4 *a2)
{
	vec_float4 r;
	r.v = __lvxl(a2, a1);
	return r;
}

__forceinline vec_float4
vec_ldl (OFFSET_T a1, const float *a2)
{
	vec_float4 r;
	r.v = __lvxl(a2, a1);
	return r;
}

__forceinline vec_bool4
vec_ldl (OFFSET_T a1, const vec_bool4 *a2)
{
	vec_bool4 r;
	r.v = __lvxl(a2, a1);
	return r;
}

__forceinline vec_int4
vec_ldl (OFFSET_T a1, const vec_int4 *a2)
{
	vec_int4 r;
	r.v = __lvxl(a2, a1);
	return r;
}

__forceinline vec_int4
vec_ldl (OFFSET_T a1, const int *a2)
{
	vec_int4 r;
	r.v = __lvxl(a2, a1);
	return r;
}

__forceinline vec_int4
vec_ldl (OFFSET_T a1, const long *a2)
{
	vec_int4 r;
	r.v = __lvxl(a2, a1);
	return r;
}

__forceinline vec_uint4
vec_ldl (OFFSET_T a1, const vec_uint4 *a2)
{
	vec_uint4 r;
	r.v = __lvxl(a2, a1);
	return r;
}

__forceinline vec_uint4
vec_ldl (OFFSET_T a1, const unsigned int *a2)
{
	vec_uint4 r;
	r.v = __lvxl(a2, a1);
	return r;
}

__forceinline vec_uint4
vec_ldl (OFFSET_T a1, const unsigned long *a2)
{
	vec_uint4 r;
	r.v = __lvxl(a2, a1);
	return r;
}

__forceinline vec_bool8
vec_ldl (OFFSET_T a1, const vec_bool8 *a2)
{
	vec_bool8 r;
	r.v = __lvxl(a2, a1);
	return r;
}

__forceinline vec_pixel
vec_ldl (OFFSET_T a1, const vec_pixel *a2)
{
	vec_pixel r;
	r.v = __lvxl(a2, a1);
	return r;
}

__forceinline vec_short8
vec_ldl (OFFSET_T a1, const vec_short8 *a2)
{
	vec_short8 r;
	r.v = __lvxl(a2, a1);
	return r;
}

__forceinline vec_short8
vec_ldl (OFFSET_T a1, const short *a2)
{
	vec_short8 r;
	r.v = __lvxl(a2, a1);
	return r;
}

__forceinline vec_ushort8
vec_ldl (OFFSET_T a1, const vec_ushort8 *a2)
{
	vec_ushort8 r;
	r.v = __lvxl(a2, a1);
	return r;
}

__forceinline vec_ushort8
vec_ldl (OFFSET_T a1, const unsigned short *a2)
{
	vec_ushort8 r;
	r.v = __lvxl(a2, a1);
	return r;
}

__forceinline vec_bool16
vec_ldl (OFFSET_T a1, const vec_bool16 *a2)
{
	vec_bool16 r;
	r.v = __lvxl(a2, a1);
	return r;
}

__forceinline vec_char16
vec_ldl (OFFSET_T a1, const vec_char16 *a2)
{
	vec_char16 r;
	r.v = __lvxl(a2, a1);
	return r;
}

__forceinline vec_char16
vec_ldl (OFFSET_T a1, const signed char *a2)
{
	vec_char16 r;
	r.v = __lvxl(a2, a1);
	return r;
}

__forceinline vec_uchar16
vec_ldl (OFFSET_T a1, const vec_uchar16 *a2)
{
	vec_uchar16 r;
	r.v = __lvxl(a2, a1);
	return r;
}

__forceinline vec_uchar16
vec_ldl (OFFSET_T a1, const unsigned char *a2)
{
	vec_uchar16 r;
	r.v = __lvxl(a2, a1);
	return r;
}

/* vec_loge */

__forceinline vec_float4
vec_loge (vec_float4 a1)
{
	vec_float4 r;
	r.v = __vlogefp(a1);
	return r;
}

/* vec_lvsl */

__forceinline vec_uchar16
vec_lvsl (OFFSET_T a1, const volatile unsigned char *a2)
{
	vec_uchar16 r;
	r.v = __lvsl((const void*)a2, a1);
	return r;
}

__forceinline vec_uchar16
vec_lvsl (OFFSET_T a1, const volatile signed char *a2)
{
	vec_uchar16 r;
	r.v = __lvsl((const void*)a2, a1);
	return r;
}

__forceinline vec_uchar16
vec_lvsl (OFFSET_T a1, const volatile unsigned short *a2)
{
	vec_uchar16 r;
	r.v = __lvsl((const void*)a2, a1);
	return r;
}

__forceinline vec_uchar16
vec_lvsl (OFFSET_T a1, const volatile short *a2)
{
	vec_uchar16 r;
	r.v = __lvsl((const void*)a2, a1);
	return r;
}

__forceinline vec_uchar16
vec_lvsl (OFFSET_T a1, const volatile unsigned int *a2)
{
	vec_uchar16 r;
	r.v = __lvsl((const void*)a2, a1);
	return r;
}

__forceinline vec_uchar16
vec_lvsl (OFFSET_T a1, const volatile int *a2)
{
	vec_uchar16 r;
	r.v = __lvsl((const void*)a2, a1);
	return r;
}

__forceinline vec_uchar16
vec_lvsl (OFFSET_T a1, const volatile unsigned long *a2)
{
	vec_uchar16 r;
	r.v = __lvsl((const void*)a2, a1);
	return r;
}

__forceinline vec_uchar16
vec_lvsl (OFFSET_T a1, const volatile long *a2)
{
	vec_uchar16 r;
	r.v = __lvsl((const void*)a2, a1);
	return r;
}

__forceinline vec_uchar16
vec_lvsl (OFFSET_T a1, const volatile float *a2)
{
	vec_uchar16 r;
	r.v = __lvsl((const void*)a2, a1);
	return r;
}

/* vec_lvsr */

__forceinline vec_uchar16
vec_lvsr (OFFSET_T a1, const volatile unsigned char *a2)
{
	vec_uchar16 r;
	r.v = __lvsr((const void*)a2, a1);
	return r;
}

__forceinline vec_uchar16
vec_lvsr (OFFSET_T a1, const volatile signed char *a2)
{
	vec_uchar16 r;
	r.v = __lvsr((const void*)a2, a1);
	return r;
}

__forceinline vec_uchar16
vec_lvsr (OFFSET_T a1, const volatile unsigned short *a2)
{
	vec_uchar16 r;
	r.v = __lvsr((const void*)a2, a1);
	return r;
}

__forceinline vec_uchar16
vec_lvsr (OFFSET_T a1, const volatile short *a2)
{
	vec_uchar16 r;
	r.v = __lvsr((const void*)a2, a1);
	return r;
}

__forceinline vec_uchar16
vec_lvsr (OFFSET_T a1, const volatile unsigned int *a2)
{
	vec_uchar16 r;
	r.v = __lvsr((const void*)a2, a1);
	return r;
}

__forceinline vec_uchar16
vec_lvsr (OFFSET_T a1, const volatile int *a2)
{
	vec_uchar16 r;
	r.v = __lvsr((const void*)a2, a1);
	return r;
}

__forceinline vec_uchar16
vec_lvsr (OFFSET_T a1, const volatile unsigned long *a2)
{
	vec_uchar16 r;
	r.v = __lvsr((const void*)a2, a1);
	return r;
}

__forceinline vec_uchar16
vec_lvsr (OFFSET_T a1, const volatile long *a2)
{
	vec_uchar16 r;
	r.v = __lvsr((const void*)a2, a1);
	return r;
}

__forceinline vec_uchar16
vec_lvsr (OFFSET_T a1, const volatile float *a2)
{
	vec_uchar16 r;
	r.v = __lvsr((const void*)a2, a1);
	return r;
}

/* begin sce local, bugizlla #8763 */
/* vec_lvlx */

__forceinline vec_uchar16
vec_lvlx (OFFSET_T a1, const volatile unsigned char *a2)
{
	vec_uchar16 r;
	r.v = __lvlx((const void*)a2, a1);
	return r;
}

__forceinline vec_uchar16
vec_lvlx (OFFSET_T a1, const volatile signed char *a2)
{
	vec_uchar16 r;
	r.v = __lvlx((const void*)a2, a1);
	return r;
}

__forceinline vec_uchar16
vec_lvlx (OFFSET_T a1, const volatile unsigned short *a2)
{
	vec_uchar16 r;
	r.v = __lvlx((const void*)a2, a1);
	return r;
}

__forceinline vec_uchar16
vec_lvlx (OFFSET_T a1, const volatile short *a2)
{
	vec_uchar16 r;
	r.v = __lvlx((const void*)a2, a1);
	return r;
}

__forceinline vec_uchar16
vec_lvlx (OFFSET_T a1, const volatile unsigned int *a2)
{
	vec_uchar16 r;
	r.v = __lvlx((const void*)a2, a1);
	return r;
}

__forceinline vec_uchar16
vec_lvlx (OFFSET_T a1, const volatile int *a2)
{
	vec_uchar16 r;
	r.v = __lvlx((const void*)a2, a1);
	return r;
}

__forceinline vec_uchar16
vec_lvlx (OFFSET_T a1, const volatile unsigned long *a2)
{
	vec_uchar16 r;
	r.v = __lvlx((const void*)a2, a1);
	return r;
}

__forceinline vec_uchar16
vec_lvlx (OFFSET_T a1, const volatile long *a2)
{
	vec_uchar16 r;
	r.v = __lvlx((const void*)a2, a1);
	return r;
}

__forceinline vec_uchar16
vec_lvlx (OFFSET_T a1, const volatile float *a2)
{
	vec_uchar16 r;
	r.v = __lvlx((const void*)a2, a1);
	return r;
}


/* vec_lvlxl */

__forceinline vec_uchar16
vec_lvlxl (OFFSET_T a1, const volatile unsigned char *a2)
{
	vec_uchar16 r;
	r.v = __lvlxl((const void*)a2, a1);
	return r;
}

__forceinline vec_uchar16
vec_lvlxl (OFFSET_T a1, const volatile signed char *a2)
{
	vec_uchar16 r;
	r.v = __lvlxl((const void*)a2, a1);
	return r;
}

__forceinline vec_uchar16
vec_lvlxl (OFFSET_T a1, const volatile unsigned short *a2)
{
	vec_uchar16 r;
	r.v = __lvlxl((const void*)a2, a1);
	return r;
}

__forceinline vec_uchar16
vec_lvlxl (OFFSET_T a1, const volatile short *a2)
{
	vec_uchar16 r;
	r.v = __lvlxl((const void*)a2, a1);
	return r;
}

__forceinline vec_uchar16
vec_lvlxl (OFFSET_T a1, const volatile unsigned int *a2)
{
	vec_uchar16 r;
	r.v = __lvlxl((const void*)a2, a1);
	return r;
}

__forceinline vec_uchar16
vec_lvlxl (OFFSET_T a1, const volatile int *a2)
{
	vec_uchar16 r;
	r.v = __lvlxl((const void*)a2, a1);
	return r;
}

__forceinline vec_uchar16
vec_lvlxl (OFFSET_T a1, const volatile unsigned long *a2)
{
	vec_uchar16 r;
	r.v = __lvlxl((const void*)a2, a1);
	return r;
}

__forceinline vec_uchar16
vec_lvlxl (OFFSET_T a1, const volatile long *a2)
{
	vec_uchar16 r;
	r.v = __lvlxl((const void*)a2, a1);
	return r;
}

__forceinline vec_uchar16
vec_lvlxl (OFFSET_T a1, const volatile float *a2)
{
	vec_uchar16 r;
	r.v = __lvlxl((const void*)a2, a1);
	return r;
}

/* vec_lvrx */
__forceinline vec_uchar16
vec_lvrx (OFFSET_T a1, const volatile unsigned char *a2)
{
	vec_uchar16 r;
	r.v = __lvrx((const void*)a2, a1);
	return r;
}

__forceinline vec_uchar16
vec_lvrx (OFFSET_T a1, const volatile signed char *a2)
{
	vec_uchar16 r;
	r.v = __lvrx((const void*)a2, a1);
	return r;
}

__forceinline vec_uchar16
vec_lvrx (OFFSET_T a1, const volatile unsigned short *a2)
{
	vec_uchar16 r;
	r.v = __lvrx((const void*)a2, a1);
	return r;
}

__forceinline vec_uchar16
vec_lvrx (OFFSET_T a1, const volatile short *a2)
{
	vec_uchar16 r;
	r.v = __lvrx((const void*)a2, a1);
	return r;
}

__forceinline vec_uchar16
vec_lvrx (OFFSET_T a1, const volatile unsigned int *a2)
{
	vec_uchar16 r;
	r.v = __lvrx((const void*)a2, a1);
	return r;
}

__forceinline vec_uchar16
vec_lvrx (OFFSET_T a1, const volatile int *a2)
{
	vec_uchar16 r;
	r.v = __lvrx((const void*)a2, a1);
	return r;
}

__forceinline vec_uchar16
vec_lvrx (OFFSET_T a1, const volatile unsigned long *a2)
{
	vec_uchar16 r;
	r.v = __lvrx((const void*)a2, a1);
	return r;
}

__forceinline vec_uchar16
vec_lvrx (OFFSET_T a1, const volatile long *a2)
{
	vec_uchar16 r;
	r.v = __lvrx((const void*)a2, a1);
	return r;
}

__forceinline vec_uchar16
vec_lvrx (OFFSET_T a1, const volatile float *a2)
{
	vec_uchar16 r;
	r.v = __lvrx((const void*)a2, a1);
	return r;
}

/* vec_lvrxl */

__forceinline vec_uchar16
vec_lvrxl (OFFSET_T a1, const volatile unsigned char *a2)
{
	vec_uchar16 r;
	r.v = __lvrxl((const void*)a2, a1);
	return r;
}

__forceinline vec_uchar16
vec_lvrxl (OFFSET_T a1, const volatile signed char *a2)
{
	vec_uchar16 r;
	r.v = __lvrxl((const void*)a2, a1);
	return r;
}

__forceinline vec_uchar16
vec_lvrxl (OFFSET_T a1, const volatile unsigned short *a2)
{
	vec_uchar16 r;
	r.v = __lvrxl((const void*)a2, a1);
	return r;
}

__forceinline vec_uchar16
vec_lvrxl (OFFSET_T a1, const volatile short *a2)
{
	vec_uchar16 r;
	r.v = __lvrxl((const void*)a2, a1);
	return r;
}

__forceinline vec_uchar16
vec_lvrxl (OFFSET_T a1, const volatile unsigned int *a2)
{
	vec_uchar16 r;
	r.v = __lvrxl((const void*)a2, a1);
	return r;
}

__forceinline vec_uchar16
vec_lvrxl (OFFSET_T a1, const volatile int *a2)
{
	vec_uchar16 r;
	r.v = __lvrxl((const void*)a2, a1);
	return r;
}

__forceinline vec_uchar16
vec_lvrxl (OFFSET_T a1, const volatile unsigned long *a2)
{
	vec_uchar16 r;
	r.v = __lvrxl((const void*)a2, a1);
	return r;
}

__forceinline vec_uchar16
vec_lvrxl (OFFSET_T a1, const volatile long *a2)
{
	vec_uchar16 r;
	r.v = __lvrxl((const void*)a2, a1);
	return r;
}

__forceinline vec_uchar16
vec_lvrxl (OFFSET_T a1, const volatile float *a2)
{
	vec_uchar16 r;
	r.v = __lvrxl((const void*)a2, a1);
	return r;
}

/* end sce local */
/* vec_madd */

__forceinline vec_float4
vec_madd (vec_float4 a1, vec_float4 a2, vec_float4 a3)
{
	vec_float4 r;
	r.v = __vmaddfp(a1, a2, a3);
	return r;
}

__forceinline vec_float4
vec_mul (vec_float4 a1, vec_float4 a2)
{
	vec_float4 r;
	r.v = __vmulfp(a1, a2);
	return r;
}

/* vec_max */

__forceinline vec_uchar16
vec_max (vec_bool16 a1, vec_uchar16 a2)
{
	vec_uchar16 r;
	r.v = __vmaxub(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_max (vec_uchar16 a1, vec_bool16 a2)
{
	vec_uchar16 r;
	r.v = __vmaxub(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_max (vec_uchar16 a1, vec_uchar16 a2)
{
	vec_uchar16 r;
	r.v = __vmaxub(a1, a2);
	return r;
}

__forceinline vec_char16
vec_max (vec_bool16 a1, vec_char16 a2)
{
	vec_char16 r;
	r.v = __vmaxsb(a1, a2);
	return r;
}

__forceinline vec_char16
vec_max (vec_char16 a1, vec_bool16 a2)
{
	vec_char16 r;
	r.v = __vmaxsb(a1, a2);
	return r;
}

__forceinline vec_char16
vec_max (vec_char16 a1, vec_char16 a2)
{
	vec_char16 r;
	r.v = __vmaxsb(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_max (vec_bool8 a1, vec_ushort8 a2)
{
	vec_ushort8 r;
	r.v = __vmaxuh(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_max (vec_ushort8 a1, vec_bool8 a2)
{
	vec_ushort8 r;
	r.v = __vmaxuh(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_max (vec_ushort8 a1, vec_ushort8 a2)
{
	vec_ushort8 r;
	r.v = __vmaxuh(a1, a2);
	return r;
}

__forceinline vec_short8
vec_max (vec_bool8 a1, vec_short8 a2)
{
	vec_short8 r;
	r.v = __vmaxsh(a1, a2);
	return r;
}

__forceinline vec_short8
vec_max (vec_short8 a1, vec_bool8 a2)
{
	vec_short8 r;
	r.v = __vmaxsh(a1, a2);
	return r;
}

__forceinline vec_short8
vec_max (vec_short8 a1, vec_short8 a2)
{
	vec_short8 r;
	r.v = __vmaxsh(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_max (vec_bool4 a1, vec_uint4 a2)
{
	vec_uint4 r;
	r.v = __vmaxuw(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_max (vec_uint4 a1, vec_bool4 a2)
{
	vec_uint4 r;
	r.v = __vmaxuw(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_max (vec_uint4 a1, vec_uint4 a2)
{
	vec_uint4 r;
	r.v = __vmaxuw(a1, a2);
	return r;
}

__forceinline vec_int4
vec_max (vec_bool4 a1, vec_int4 a2)
{
	vec_int4 r;
	r.v = __vmaxsw(a1, a2);
	return r;
}

__forceinline vec_int4
vec_max (vec_int4 a1, vec_bool4 a2)
{
	vec_int4 r;
	r.v = __vmaxsw(a1, a2);
	return r;
}

__forceinline vec_int4
vec_max (vec_int4 a1, vec_int4 a2)
{
	vec_int4 r;
	r.v = __vmaxsw(a1, a2);
	return r;
}

__forceinline vec_float4
vec_max (vec_float4 a1, vec_float4 a2)
{
	vec_float4 r;
	r.v = __vmaxfp(a1, a2);
	return r;
}

/* vec_vmaxfp */

__forceinline vec_float4
vec_vmaxfp (vec_float4 a1, vec_float4 a2)
{
	vec_float4 r;
	r.v = __vmaxfp(a1, a2);
	return r;
}

/* vec_vmaxsw */

__forceinline vec_int4
vec_vmaxsw (vec_bool4 a1, vec_int4 a2)
{
	vec_int4 r;
	r.v = __vmaxsw(a1, a2);
	return r;
}

__forceinline vec_int4
vec_vmaxsw (vec_int4 a1, vec_bool4 a2)
{
	vec_int4 r;
	r.v = __vmaxsw(a1, a2);
	return r;
}

__forceinline vec_int4
vec_vmaxsw (vec_int4 a1, vec_int4 a2)
{
	vec_int4 r;
	r.v = __vmaxsw(a1, a2);
	return r;
}

/* vec_vmaxuw */

__forceinline vec_uint4
vec_vmaxuw (vec_bool4 a1, vec_uint4 a2)
{
	vec_uint4 r;
	r.v = __vmaxuw(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_vmaxuw (vec_uint4 a1, vec_bool4 a2)
{
	vec_uint4 r;
	r.v = __vmaxuw(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_vmaxuw (vec_uint4 a1, vec_uint4 a2)
{
	vec_uint4 r;
	r.v = __vmaxuw(a1, a2);
	return r;
}

/* vec_vmaxsh */

__forceinline vec_short8
vec_vmaxsh (vec_bool8 a1, vec_short8 a2)
{
	vec_short8 r;
	r.v = __vmaxsh(a1, a2);
	return r;
}

__forceinline vec_short8
vec_vmaxsh (vec_short8 a1, vec_bool8 a2)
{
	vec_short8 r;
	r.v = __vmaxsh(a1, a2);
	return r;
}

__forceinline vec_short8
vec_vmaxsh (vec_short8 a1, vec_short8 a2)
{
	vec_short8 r;
	r.v = __vmaxsh(a1, a2);
	return r;
}

/* vec_vmaxuh */

__forceinline vec_ushort8
vec_vmaxuh (vec_bool8 a1, vec_ushort8 a2)
{
	vec_ushort8 r;
	r.v = __vmaxuh(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_vmaxuh (vec_ushort8 a1, vec_bool8 a2)
{
	vec_ushort8 r;
	r.v = __vmaxuh(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_vmaxuh (vec_ushort8 a1, vec_ushort8 a2)
{
	vec_ushort8 r;
	r.v = __vmaxuh(a1, a2);
	return r;
}

/* vec_vmaxsb */

__forceinline vec_char16
vec_vmaxsb (vec_bool16 a1, vec_char16 a2)
{
	vec_char16 r;
	r.v = __vmaxsb(a1, a2);
	return r;
}

__forceinline vec_char16
vec_vmaxsb (vec_char16 a1, vec_bool16 a2)
{
	vec_char16 r;
	r.v = __vmaxsb(a1, a2);
	return r;
}

__forceinline vec_char16
vec_vmaxsb (vec_char16 a1, vec_char16 a2)
{
	vec_char16 r;
	r.v = __vmaxsb(a1, a2);
	return r;
}

/* vec_vmaxub */

__forceinline vec_uchar16
vec_vmaxub (vec_bool16 a1, vec_uchar16 a2)
{
	vec_uchar16 r;
	r.v = __vmaxub(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_vmaxub (vec_uchar16 a1, vec_bool16 a2)
{
	vec_uchar16 r;
	r.v = __vmaxub(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_vmaxub (vec_uchar16 a1, vec_uchar16 a2)
{
	vec_uchar16 r;
	r.v = __vmaxub(a1, a2);
	return r;
}

/* vec_mergeh */

__forceinline vec_bool16
vec_mergeh (vec_bool16 a1, vec_bool16 a2)
{
	vec_bool16 r;
	r.v = __vmrghb(a1, a2);
	return r;
}

__forceinline vec_char16
vec_mergeh (vec_char16 a1, vec_char16 a2)
{
	vec_char16 r;
	r.v = __vmrghb(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_mergeh (vec_uchar16 a1, vec_uchar16 a2)
{
	vec_uchar16 r;
	r.v = __vmrghb(a1, a2);
	return r;
}

__forceinline vec_bool8
vec_mergeh (vec_bool8 a1, vec_bool8 a2)
{
	vec_bool8 r;
	r.v = __vmrghh(a1, a2);
	return r;
}

__forceinline vec_pixel
vec_mergeh (vec_pixel a1, vec_pixel a2)
{
	vec_pixel r;
	r.v = __vmrghh(a1, a2);
	return r;
}

__forceinline vec_short8
vec_mergeh (vec_short8 a1, vec_short8 a2)
{
	vec_short8 r;
	r.v = __vmrghh(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_mergeh (vec_ushort8 a1, vec_ushort8 a2)
{
	vec_ushort8 r;
	r.v = __vmrghh(a1, a2);
	return r;
}

__forceinline vec_float4
vec_mergeh (vec_float4 a1, vec_float4 a2)
{
	vec_float4 r;
	r.v = __vmrghw(a1, a2);
	return r;
}

__forceinline vec_bool4
vec_mergeh (vec_bool4 a1, vec_bool4 a2)
{
	vec_bool4 r;
	r.v = __vmrghw(a1, a2);
	return r;
}

__forceinline vec_int4
vec_mergeh (vec_int4 a1, vec_int4 a2)
{
	vec_int4 r;
	r.v = __vmrghw(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_mergeh (vec_uint4 a1, vec_uint4 a2)
{
	vec_uint4 r;
	r.v = __vmrghw(a1, a2);
	return r;
}

/* vec_vmrghw */

__forceinline vec_float4
vec_vmrghw (vec_float4 a1, vec_float4 a2)
{
	vec_float4 r;
	r.v = __vmrghw(a1, a2);
	return r;
}

__forceinline vec_bool4
vec_vmrghw (vec_bool4 a1, vec_bool4 a2)
{
	vec_bool4 r;
	r.v = __vmrghw(a1, a2);
	return r;
}

__forceinline vec_int4
vec_vmrghw (vec_int4 a1, vec_int4 a2)
{
	vec_int4 r;
	r.v = __vmrghw(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_vmrghw (vec_uint4 a1, vec_uint4 a2)
{
	vec_uint4 r;
	r.v = __vmrghw(a1, a2);
	return r;
}

/* vec_vmrghh */

__forceinline vec_bool8
vec_vmrghh (vec_bool8 a1, vec_bool8 a2)
{
	vec_bool8 r;
	r.v = __vmrghh(a1, a2);
	return r;
}

__forceinline vec_short8
vec_vmrghh (vec_short8 a1, vec_short8 a2)
{
	vec_short8 r;
	r.v = __vmrghh(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_vmrghh (vec_ushort8 a1, vec_ushort8 a2)
{
	vec_ushort8 r;
	r.v = __vmrghh(a1, a2);
	return r;
}

__forceinline vec_pixel
vec_vmrghh (vec_pixel a1, vec_pixel a2)
{
	vec_pixel r;
	r.v = __vmrghh(a1, a2);
	return r;
}

/* vec_vmrghb */

__forceinline vec_bool16
vec_vmrghb (vec_bool16 a1, vec_bool16 a2)
{
	vec_bool16 r;
	r.v = __vmrghb(a1, a2);
	return r;
}

__forceinline vec_char16
vec_vmrghb (vec_char16 a1, vec_char16 a2)
{
	vec_char16 r;
	r.v = __vmrghb(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_vmrghb (vec_uchar16 a1, vec_uchar16 a2)
{
	vec_uchar16 r;
	r.v = __vmrghb(a1, a2);
	return r;
}

/* vec_mergel */

__forceinline vec_bool16
vec_mergel (vec_bool16 a1, vec_bool16 a2)
{
	vec_bool16 r;
	r.v = __vmrglb(a1, a2);
	return r;
}

__forceinline vec_char16
vec_mergel (vec_char16 a1, vec_char16 a2)
{
	vec_char16 r;
	r.v = __vmrglb(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_mergel (vec_uchar16 a1, vec_uchar16 a2)
{
	vec_uchar16 r;
	r.v = __vmrglb(a1, a2);
	return r;
}

__forceinline vec_bool8
vec_mergel (vec_bool8 a1, vec_bool8 a2)
{
	vec_bool8 r;
	r.v = __vmrglh(a1, a2);
	return r;
}

__forceinline vec_pixel
vec_mergel (vec_pixel a1, vec_pixel a2)
{
	vec_pixel r;
	r.v = __vmrglh(a1, a2);
	return r;
}

__forceinline vec_short8
vec_mergel (vec_short8 a1, vec_short8 a2)
{
	vec_short8 r;
	r.v = __vmrglh(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_mergel (vec_ushort8 a1, vec_ushort8 a2)
{
	vec_ushort8 r;
	r.v = __vmrglh(a1, a2);
	return r;
}

__forceinline vec_float4
vec_mergel (vec_float4 a1, vec_float4 a2)
{
	vec_float4 r;
	r.v = __vmrglw(a1, a2);
	return r;
}

__forceinline vec_bool4
vec_mergel (vec_bool4 a1, vec_bool4 a2)
{
	vec_bool4 r;
	r.v = __vmrglw(a1, a2);
	return r;
}

__forceinline vec_int4
vec_mergel (vec_int4 a1, vec_int4 a2)
{
	vec_int4 r;
	r.v = __vmrglw(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_mergel (vec_uint4 a1, vec_uint4 a2)
{
	vec_uint4 r;
	r.v = __vmrglw(a1, a2);
	return r;
}

/* vec_vmrglw */

__forceinline vec_float4
vec_vmrglw (vec_float4 a1, vec_float4 a2)
{
	vec_float4 r;
	r.v = __vmrglw(a1, a2);
	return r;
}

__forceinline vec_int4
vec_vmrglw (vec_int4 a1, vec_int4 a2)
{
	vec_int4 r;
	r.v = __vmrglw(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_vmrglw (vec_uint4 a1, vec_uint4 a2)
{
	vec_uint4 r;
	r.v = __vmrglw(a1, a2);
	return r;
}

__forceinline vec_bool4
vec_vmrglw (vec_bool4 a1, vec_bool4 a2)
{
	vec_bool4 r;
	r.v = __vmrglw(a1, a2);
	return r;
}

/* vec_vmrglh */

__forceinline vec_bool8
vec_vmrglh (vec_bool8 a1, vec_bool8 a2)
{
	vec_bool8 r;
	r.v = __vmrglh(a1, a2);
	return r;
}

__forceinline vec_short8
vec_vmrglh (vec_short8 a1, vec_short8 a2)
{
	vec_short8 r;
	r.v = __vmrglh(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_vmrglh (vec_ushort8 a1, vec_ushort8 a2)
{
	vec_ushort8 r;
	r.v = __vmrglh(a1, a2);
	return r;
}

__forceinline vec_pixel
vec_vmrglh (vec_pixel a1, vec_pixel a2)
{
	vec_pixel r;
	r.v = __vmrglh(a1, a2);
	return r;
}

/* vec_vmrglb */

__forceinline vec_bool16
vec_vmrglb (vec_bool16 a1, vec_bool16 a2)
{
	vec_bool16 r;
	r.v = __vmrglb(a1, a2);
	return r;
}

__forceinline vec_char16
vec_vmrglb (vec_char16 a1, vec_char16 a2)
{
	vec_char16 r;
	r.v = __vmrglb(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_vmrglb (vec_uchar16 a1, vec_uchar16 a2)
{
	vec_uchar16 r;
	r.v = __vmrglb(a1, a2);
	return r;
}

/* vec_min */

__forceinline vec_uchar16
vec_min (vec_bool16 a1, vec_uchar16 a2)
{
	vec_uchar16 r;
	r.v = __vminub(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_min (vec_uchar16 a1, vec_bool16 a2)
{
	vec_uchar16 r;
	r.v = __vminub(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_min (vec_uchar16 a1, vec_uchar16 a2)
{
	vec_uchar16 r;
	r.v = __vminub(a1, a2);
	return r;
}

__forceinline vec_char16
vec_min (vec_bool16 a1, vec_char16 a2)
{
	vec_char16 r;
	r.v = __vminsb(a1, a2);
	return r;
}

__forceinline vec_char16
vec_min (vec_char16 a1, vec_bool16 a2)
{
	vec_char16 r;
	r.v = __vminsb(a1, a2);
	return r;
}

__forceinline vec_char16
vec_min (vec_char16 a1, vec_char16 a2)
{
	vec_char16 r;
	r.v = __vminsb(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_min (vec_bool8 a1, vec_ushort8 a2)
{
	vec_ushort8 r;
	r.v = __vminuh(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_min (vec_ushort8 a1, vec_bool8 a2)
{
	vec_ushort8 r;
	r.v = __vminuh(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_min (vec_ushort8 a1, vec_ushort8 a2)
{
	vec_ushort8 r;
	r.v = __vminuh(a1, a2);
	return r;
}

__forceinline vec_short8
vec_min (vec_bool8 a1, vec_short8 a2)
{
	vec_short8 r;
	r.v = __vminsh(a1, a2);
	return r;
}

__forceinline vec_short8
vec_min (vec_short8 a1, vec_bool8 a2)
{
	vec_short8 r;
	r.v = __vminsh(a1, a2);
	return r;
}

__forceinline vec_short8
vec_min (vec_short8 a1, vec_short8 a2)
{
	vec_short8 r;
	r.v = __vminsh(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_min (vec_bool4 a1, vec_uint4 a2)
{
	vec_uint4 r;
	r.v = __vminuw(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_min (vec_uint4 a1, vec_bool4 a2)
{
	vec_uint4 r;
	r.v = __vminuw(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_min (vec_uint4 a1, vec_uint4 a2)
{
	vec_uint4 r;
	r.v = __vminuw(a1, a2);
	return r;
}

__forceinline vec_int4
vec_min (vec_bool4 a1, vec_int4 a2)
{
	vec_int4 r;
	r.v = __vminsw(a1, a2);
	return r;
}

__forceinline vec_int4
vec_min (vec_int4 a1, vec_bool4 a2)
{
	vec_int4 r;
	r.v = __vminsw(a1, a2);
	return r;
}

__forceinline vec_int4
vec_min (vec_int4 a1, vec_int4 a2)
{
	vec_int4 r;
	r.v = __vminsw(a1, a2);
	return r;
}

__forceinline vec_float4
vec_min (vec_float4 a1, vec_float4 a2)
{
	vec_float4 r;
	r.v = __vminfp(a1, a2);
	return r;
}

/* vec_vminfp */

__forceinline vec_float4
vec_vminfp (vec_float4 a1, vec_float4 a2)
{
	vec_float4 r;
	r.v = __vminfp(a1, a2);
	return r;
}

/* vec_vminsw */

__forceinline vec_int4
vec_vminsw (vec_bool4 a1, vec_int4 a2)
{
	vec_int4 r;
	r.v = __vminsw(a1, a2);
	return r;
}

__forceinline vec_int4
vec_vminsw (vec_int4 a1, vec_bool4 a2)
{
	vec_int4 r;
	r.v = __vminsw(a1, a2);
	return r;
}

__forceinline vec_int4
vec_vminsw (vec_int4 a1, vec_int4 a2)
{
	vec_int4 r;
	r.v = __vminsw(a1, a2);
	return r;
}

/* vec_vminuw */

__forceinline vec_uint4
vec_vminuw (vec_bool4 a1, vec_uint4 a2)
{
	vec_uint4 r;
	r.v = __vminuw(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_vminuw (vec_uint4 a1, vec_bool4 a2)
{
	vec_uint4 r;
	r.v = __vminuw(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_vminuw (vec_uint4 a1, vec_uint4 a2)
{
	vec_uint4 r;
	r.v = __vminuw(a1, a2);
	return r;
}

/* vec_vminsh */

__forceinline vec_short8
vec_vminsh (vec_bool8 a1, vec_short8 a2)
{
	vec_short8 r;
	r.v = __vminsh(a1, a2);
	return r;
}

__forceinline vec_short8
vec_vminsh (vec_short8 a1, vec_bool8 a2)
{
	vec_short8 r;
	r.v = __vminsh(a1, a2);
	return r;
}

__forceinline vec_short8
vec_vminsh (vec_short8 a1, vec_short8 a2)
{
	vec_short8 r;
	r.v = __vminsh(a1, a2);
	return r;
}

/* vec_vminuh */

__forceinline vec_ushort8
vec_vminuh (vec_bool8 a1, vec_ushort8 a2)
{
	vec_ushort8 r;
	r.v = __vminuh(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_vminuh (vec_ushort8 a1, vec_bool8 a2)
{
	vec_ushort8 r;
	r.v = __vminuh(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_vminuh (vec_ushort8 a1, vec_ushort8 a2)
{
	vec_ushort8 r;
	r.v = __vminuh(a1, a2);
	return r;
}

/* vec_vminsb */

__forceinline vec_char16
vec_vminsb (vec_bool16 a1, vec_char16 a2)
{
	vec_char16 r;
	r.v = __vminsb(a1, a2);
	return r;
}

__forceinline vec_char16
vec_vminsb (vec_char16 a1, vec_bool16 a2)
{
	vec_char16 r;
	r.v = __vminsb(a1, a2);
	return r;
}

__forceinline vec_char16
vec_vminsb (vec_char16 a1, vec_char16 a2)
{
	vec_char16 r;
	r.v = __vminsb(a1, a2);
	return r;
}

/* vec_vminub */

__forceinline vec_uchar16
vec_vminub (vec_bool16 a1, vec_uchar16 a2)
{
	vec_uchar16 r;
	r.v = __vminub(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_vminub (vec_uchar16 a1, vec_bool16 a2)
{
	vec_uchar16 r;
	r.v = __vminub(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_vminub (vec_uchar16 a1, vec_uchar16 a2)
{
	vec_uchar16 r;
	r.v = __vminub(a1, a2);
	return r;
}

/* vec_nmsub */

__forceinline vec_float4
vec_nmsub (vec_float4 a1, vec_float4 a2, vec_float4 a3)
{
	vec_float4 r;
	r.v = __vnmsubfp(a1, a2, a3);
	return r;
}

/* vec_nor */

__forceinline vec_float4
vec_nor (vec_float4 a1, vec_float4 a2)
{
	vec_float4 r;
	r.v = __vnor(a1, a2);
	return r;
}

__forceinline vec_int4
vec_nor (vec_int4 a1, vec_int4 a2)
{
	vec_int4 r;
	r.v = __vnor(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_nor (vec_uint4 a1, vec_uint4 a2)
{
	vec_uint4 r;
	r.v = __vnor(a1, a2);
	return r;
}

__forceinline vec_bool4
vec_nor (vec_bool4 a1, vec_bool4 a2)
{
	vec_bool4 r;
	r.v = __vnor(a1, a2);
	return r;
}

__forceinline vec_short8
vec_nor (vec_short8 a1, vec_short8 a2)
{
	vec_short8 r;
	r.v = __vnor(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_nor (vec_ushort8 a1, vec_ushort8 a2)
{
	vec_ushort8 r;
	r.v = __vnor(a1, a2);
	return r;
}

__forceinline vec_bool8
vec_nor (vec_bool8 a1, vec_bool8 a2)
{
	vec_bool8 r;
	r.v = __vnor(a1, a2);
	return r;
}

__forceinline vec_char16
vec_nor (vec_char16 a1, vec_char16 a2)
{
	vec_char16 r;
	r.v = __vnor(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_nor (vec_uchar16 a1, vec_uchar16 a2)
{
	vec_uchar16 r;
	r.v = __vnor(a1, a2);
	return r;
}

__forceinline vec_bool16
vec_nor (vec_bool16 a1, vec_bool16 a2)
{
	vec_bool16 r;
	r.v = __vnor(a1, a2);
	return r;
}

/* vec_or */

__forceinline vec_float4
vec_or (vec_float4 a1, vec_float4 a2)
{
	vec_float4 r;
	r.v = __vor(a1, a2);
	return r;
}

__forceinline vec_float4
vec_or (vec_float4 a1, vec_bool4 a2)
{
	vec_float4 r;
	r.v = __vor(a1, a2);
	return r;
}

__forceinline vec_float4
vec_or (vec_bool4 a1, vec_float4 a2)
{
	vec_float4 r;
	r.v = __vor(a1, a2);
	return r;
}

__forceinline vec_bool4
vec_or (vec_bool4 a1, vec_bool4 a2)
{
	vec_bool4 r;
	r.v = __vor(a1, a2);
	return r;
}

__forceinline vec_int4
vec_or (vec_bool4 a1, vec_int4 a2)
{
	vec_int4 r;
	r.v = __vor(a1, a2);
	return r;
}

__forceinline vec_int4
vec_or (vec_int4 a1, vec_bool4 a2)
{
	vec_int4 r;
	r.v = __vor(a1, a2);
	return r;
}

__forceinline vec_int4
vec_or (vec_int4 a1, vec_int4 a2)
{
	vec_int4 r;
	r.v = __vor(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_or (vec_bool4 a1, vec_uint4 a2)
{
	vec_uint4 r;
	r.v = __vor(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_or (vec_uint4 a1, vec_bool4 a2)
{
	vec_uint4 r;
	r.v = __vor(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_or (vec_uint4 a1, vec_uint4 a2)
{
	vec_uint4 r;
	r.v = __vor(a1, a2);
	return r;
}

__forceinline vec_bool8
vec_or (vec_bool8 a1, vec_bool8 a2)
{
	vec_bool8 r;
	r.v = __vor(a1, a2);
	return r;
}

__forceinline vec_short8
vec_or (vec_bool8 a1, vec_short8 a2)
{
	vec_short8 r;
	r.v = __vor(a1, a2);
	return r;
}

__forceinline vec_short8
vec_or (vec_short8 a1, vec_bool8 a2)
{
	vec_short8 r;
	r.v = __vor(a1, a2);
	return r;
}

__forceinline vec_short8
vec_or (vec_short8 a1, vec_short8 a2)
{
	vec_short8 r;
	r.v = __vor(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_or (vec_bool8 a1, vec_ushort8 a2)
{
	vec_ushort8 r;
	r.v = __vor(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_or (vec_ushort8 a1, vec_bool8 a2)
{
	vec_ushort8 r;
	r.v = __vor(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_or (vec_ushort8 a1, vec_ushort8 a2)
{
	vec_ushort8 r;
	r.v = __vor(a1, a2);
	return r;
}

__forceinline vec_char16
vec_or (vec_bool16 a1, vec_char16 a2)
{
	vec_char16 r;
	r.v = __vor(a1, a2);
	return r;
}

__forceinline vec_bool16
vec_or (vec_bool16 a1, vec_bool16 a2)
{
	vec_bool16 r;
	r.v = __vor(a1, a2);
	return r;
}

__forceinline vec_char16
vec_or (vec_char16 a1, vec_bool16 a2)
{
	vec_char16 r;
	r.v = __vor(a1, a2);
	return r;
}

__forceinline vec_char16
vec_or (vec_char16 a1, vec_char16 a2)
{
	vec_char16 r;
	r.v = __vor(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_or (vec_bool16 a1, vec_uchar16 a2)
{
	vec_uchar16 r;
	r.v = __vor(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_or (vec_uchar16 a1, vec_bool16 a2)
{
	vec_uchar16 r;
	r.v = __vor(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_or (vec_uchar16 a1, vec_uchar16 a2)
{
	vec_uchar16 r;
	r.v = __vor(a1, a2);
	return r;
}

/* vec_pack */

__forceinline vec_char16
vec_pack (vec_short8 a1, vec_short8 a2)
{
	vec_char16 r;
	r.v = __vpkuhum(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_pack (vec_ushort8 a1, vec_ushort8 a2)
{
	vec_uchar16 r;
	r.v = __vpkuhum(a1, a2);
	return r;
}

__forceinline vec_bool16
vec_pack (vec_bool8 a1, vec_bool8 a2)
{
	vec_bool16 r;
	r.v = __vpkuhum(a1, a2);
	return r;
}

__forceinline vec_short8
vec_pack (vec_int4 a1, vec_int4 a2)
{
	vec_short8 r;
	r.v = __vpkuwum(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_pack (vec_uint4 a1, vec_uint4 a2)
{
	vec_ushort8 r;
	r.v = __vpkuwum(a1, a2);
	return r;
}

__forceinline vec_bool8
vec_pack (vec_bool4 a1, vec_bool4 a2)
{
	vec_bool8 r;
	r.v = __vpkuwum(a1, a2);
	return r;
}

/* vec_vpkuwum */

__forceinline vec_bool8
vec_vpkuwum (vec_bool4 a1, vec_bool4 a2)
{
	vec_bool8 r;
	r.v = __vpkuwum(a1, a2);
	return r;
}

__forceinline vec_short8
vec_vpkuwum (vec_int4 a1, vec_int4 a2)
{
	vec_short8 r;
	r.v = __vpkuwum(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_vpkuwum (vec_uint4 a1, vec_uint4 a2)
{
	vec_ushort8 r;
	r.v = __vpkuwum(a1, a2);
	return r;
}

/* vec_vpkuhum */

__forceinline vec_bool16
vec_vpkuhum (vec_bool8 a1, vec_bool8 a2)
{
	vec_bool16 r;
	r.v = __vpkuhum(a1, a2);
	return r;
}

__forceinline vec_char16
vec_vpkuhum (vec_short8 a1, vec_short8 a2)
{
	vec_char16 r;
	r.v = __vpkuhum(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_vpkuhum (vec_ushort8 a1, vec_ushort8 a2)
{
	vec_uchar16 r;
	r.v = __vpkuhum(a1, a2);
	return r;
}

/* vec_packpx */

__forceinline vec_pixel
vec_packpx (vec_uint4 a1, vec_uint4 a2)
{
	vec_pixel r;
	r.v = __vpkpx(a1, a2);
	return r;
}

/* vec_packs */

__forceinline vec_uchar16
vec_packs (vec_ushort8 a1, vec_ushort8 a2)
{
	vec_uchar16 r;
	r.v = __vpkuhus(a1, a2);
	return r;
}

__forceinline vec_char16
vec_packs (vec_short8 a1, vec_short8 a2)
{
	vec_char16 r;
	r.v = __vpkshss(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_packs (vec_uint4 a1, vec_uint4 a2)
{
	vec_ushort8 r;
	r.v = __vpkuwus(a1, a2);
	return r;
}

__forceinline vec_short8
vec_packs (vec_int4 a1, vec_int4 a2)
{
	vec_short8 r;
	r.v = __vpkswss(a1, a2);
	return r;
}

/* vec_vpkswss */

__forceinline vec_short8
vec_vpkswss (vec_int4 a1, vec_int4 a2)
{
	vec_short8 r;
	r.v = __vpkswss(a1, a2);
	return r;
}

/* vec_vpkuwus */

__forceinline vec_ushort8
vec_vpkuwus (vec_uint4 a1, vec_uint4 a2)
{
	vec_ushort8 r;
	r.v = __vpkuwus(a1, a2);
	return r;
}

/* vec_vpkshss */

__forceinline vec_char16
vec_vpkshss (vec_short8 a1, vec_short8 a2)
{
	vec_char16 r;
	r.v = __vpkshss(a1, a2);
	return r;
}

/* vec_vpkuhus */

__forceinline vec_uchar16
vec_vpkuhus (vec_ushort8 a1, vec_ushort8 a2)
{
	vec_uchar16 r;
	r.v = __vpkuhus(a1, a2);
	return r;
}

/* vec_packsu */

__forceinline vec_uchar16
vec_packsu (vec_ushort8 a1, vec_ushort8 a2)
{
	vec_uchar16 r;
	r.v = __vpkuhus(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_packsu (vec_short8 a1, vec_short8 a2)
{
	vec_uchar16 r;
	r.v = __vpkshus(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_packsu (vec_uint4 a1, vec_uint4 a2)
{
	vec_ushort8 r;
	r.v = __vpkuwus(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_packsu (vec_int4 a1, vec_int4 a2)
{
	vec_ushort8 r;
	r.v = __vpkswus(a1, a2);
	return r;
}

/* vec_vpkswus */

__forceinline vec_ushort8
vec_vpkswus (vec_int4 a1, vec_int4 a2)
{
	vec_ushort8 r;
	r.v = __vpkswus(a1, a2);
	return r;
}

/* vec_vpkshus */

__forceinline vec_uchar16
vec_vpkshus (vec_short8 a1, vec_short8 a2)
{
	vec_uchar16 r;
	r.v = __vpkshus(a1, a2);
	return r;
}

/* vec_perm */

__forceinline vec_float4
vec_perm (vec_float4 a1, vec_float4 a2, vec_uchar16 a3)
{
	vec_float4 r;
	r.v = __vperm(a1, a2, a3);
	return r;
}

__forceinline vec_int4
vec_perm (vec_int4 a1, vec_int4 a2, vec_uchar16 a3)
{
	vec_int4 r;
	r.v = __vperm(a1, a2, a3);
	return r;
}

__forceinline vec_uint4
vec_perm (vec_uint4 a1, vec_uint4 a2, vec_uchar16 a3)
{
	vec_uint4 r;
	r.v = __vperm(a1, a2, a3);
	return r;
}

__forceinline vec_bool4
vec_perm (vec_bool4 a1, vec_bool4 a2, vec_uchar16 a3)
{
	vec_bool4 r;
	r.v = __vperm(a1, a2, a3);
	return r;
}

__forceinline vec_short8
vec_perm (vec_short8 a1, vec_short8 a2, vec_uchar16 a3)
{
	vec_short8 r;
	r.v = __vperm(a1, a2, a3);
	return r;
}

__forceinline vec_ushort8
vec_perm (vec_ushort8 a1, vec_ushort8 a2, vec_uchar16 a3)
{
	vec_ushort8 r;
	r.v = __vperm(a1, a2, a3);
	return r;
}

__forceinline vec_bool8
vec_perm (vec_bool8 a1, vec_bool8 a2, vec_uchar16 a3)
{
	vec_bool8 r;
	r.v = __vperm(a1, a2, a3);
	return r;
}

__forceinline vec_pixel
vec_perm (vec_pixel a1, vec_pixel a2, vec_uchar16 a3)
{
	vec_pixel r;
	r.v = __vperm(a1, a2, a3);
	return r;
}

__forceinline vec_char16
vec_perm (vec_char16 a1, vec_char16 a2, vec_uchar16 a3)
{
	vec_char16 r;
	r.v = __vperm(a1, a2, a3);
	return r;
}

__forceinline vec_uchar16
vec_perm (vec_uchar16 a1, vec_uchar16 a2, vec_uchar16 a3)
{
	vec_uchar16 r;
	r.v = __vperm(a1, a2, a3);
	return r;
}

__forceinline vec_bool16
vec_perm (vec_bool16 a1, vec_bool16 a2, vec_uchar16 a3)
{
	vec_bool16 r;
	r.v = __vperm(a1, a2, a3);
	return r;
}

/* vec_re */

__forceinline vec_float4
vec_re (vec_float4 a1)
{
	vec_float4 r;
	r.v = __vrefp(a1);
	return r;
}

/* vec_rl */

__forceinline vec_char16
vec_rl (vec_char16 a1, vec_uchar16 a2)
{
	vec_char16 r;
	r.v = __vrlb(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_rl (vec_uchar16 a1, vec_uchar16 a2)
{
	vec_uchar16 r;
	r.v = __vrlb(a1, a2);
	return r;
}

__forceinline vec_short8
vec_rl (vec_short8 a1, vec_ushort8 a2)
{
	vec_short8 r;
	r.v = __vrlh(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_rl (vec_ushort8 a1, vec_ushort8 a2)
{
	vec_ushort8 r;
	r.v = __vrlh(a1, a2);
	return r;
}

__forceinline vec_int4
vec_rl (vec_int4 a1, vec_uint4 a2)
{
	vec_int4 r;
	r.v = __vrlw(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_rl (vec_uint4 a1, vec_uint4 a2)
{
	vec_uint4 r;
	r.v = __vrlw(a1, a2);
	return r;
}

/* vec_vrlw */

__forceinline vec_int4
vec_vrlw (vec_int4 a1, vec_uint4 a2)
{
	vec_int4 r;
	r.v = __vrlw(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_vrlw (vec_uint4 a1, vec_uint4 a2)
{
	vec_uint4 r;
	r.v = __vrlw(a1, a2);
	return r;
}

/* vec_vrlh */

__forceinline vec_short8
vec_vrlh (vec_short8 a1, vec_ushort8 a2)
{
	vec_short8 r;
	r.v = __vrlh(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_vrlh (vec_ushort8 a1, vec_ushort8 a2)
{
	vec_ushort8 r;
	r.v = __vrlh(a1, a2);
	return r;
}

/* vec_vrlb */

__forceinline vec_char16
vec_vrlb (vec_char16 a1, vec_uchar16 a2)
{
	vec_char16 r;
	r.v = __vrlb(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_vrlb (vec_uchar16 a1, vec_uchar16 a2)
{
	vec_uchar16 r;
	r.v = __vrlb(a1, a2);
	return r;
}

/* vec_round */

__forceinline vec_float4
vec_round (vec_float4 a1)
{
	vec_float4 r;
	r.v = __vrfin(a1);
	return r;
}

/* vec_rsqrte */

__forceinline vec_float4
vec_rsqrte (vec_float4 a1)
{
	vec_float4 r;
	r.v = __vrsqrtefp(a1);
	return r;
}

/* vec_sel */

__forceinline vec_float4
vec_sel (vec_float4 a1, vec_float4 a2, vec_bool4 a3)
{
	vec_float4 r;
	r.v = __vsel(a1, a2, a3);
	return r;
}

__forceinline vec_float4
vec_sel (vec_float4 a1, vec_float4 a2, vec_uint4 a3)
{
	vec_float4 r;
	r.v = __vsel(a1, a2, a3);
	return r;
}

__forceinline vec_int4
vec_sel (vec_int4 a1, vec_int4 a2, vec_bool4 a3)
{
	vec_int4 r;
	r.v = __vsel(a1, a2, a3);
	return r;
}

__forceinline vec_int4
vec_sel (vec_int4 a1, vec_int4 a2, vec_uint4 a3)
{
	vec_int4 r;
	r.v = __vsel(a1, a2, a3);
	return r;
}

__forceinline vec_uint4
vec_sel (vec_uint4 a1, vec_uint4 a2, vec_bool4 a3)
{
	vec_uint4 r;
	r.v = __vsel(a1, a2, a3);
	return r;
}

__forceinline vec_uint4
vec_sel (vec_uint4 a1, vec_uint4 a2, vec_uint4 a3)
{
	vec_uint4 r;
	r.v = __vsel(a1, a2, a3);
	return r;
}

__forceinline vec_bool4
vec_sel (vec_bool4 a1, vec_bool4 a2, vec_bool4 a3)
{
	vec_bool4 r;
	r.v = __vsel(a1, a2, a3);
	return r;
}

__forceinline vec_bool4
vec_sel (vec_bool4 a1, vec_bool4 a2, vec_uint4 a3)
{
	vec_bool4 r;
	r.v = __vsel(a1, a2, a3);
	return r;
}

__forceinline vec_short8
vec_sel (vec_short8 a1, vec_short8 a2, vec_bool8 a3)
{
	vec_short8 r;
	r.v = __vsel(a1, a2, a3);
	return r;
}

__forceinline vec_short8
vec_sel (vec_short8 a1, vec_short8 a2, vec_ushort8 a3)
{
	vec_short8 r;
	r.v = __vsel(a1, a2, a3);
	return r;
}

__forceinline vec_ushort8
vec_sel (vec_ushort8 a1, vec_ushort8 a2, vec_bool8 a3)
{
	vec_ushort8 r;
	r.v = __vsel(a1, a2, a3);
	return r;
}

__forceinline vec_ushort8
vec_sel (vec_ushort8 a1, vec_ushort8 a2, vec_ushort8 a3)
{
	vec_ushort8 r;
	r.v = __vsel(a1, a2, a3);
	return r;
}

__forceinline vec_bool8
vec_sel (vec_bool8 a1, vec_bool8 a2, vec_bool8 a3)
{
	vec_bool8 r;
	r.v = __vsel(a1, a2, a3);
	return r;
}

__forceinline vec_bool8
vec_sel (vec_bool8 a1, vec_bool8 a2, vec_ushort8 a3)
{
	vec_bool8 r;
	r.v = __vsel(a1, a2, a3);
	return r;
}

__forceinline vec_char16
vec_sel (vec_char16 a1, vec_char16 a2, vec_bool16 a3)
{
	vec_char16 r;
	r.v = __vsel(a1, a2, a3);
	return r;
}

__forceinline vec_char16
vec_sel (vec_char16 a1, vec_char16 a2, vec_uchar16 a3)
{
	vec_char16 r;
	r.v = __vsel(a1, a2, a3);
	return r;
}

__forceinline vec_uchar16
vec_sel (vec_uchar16 a1, vec_uchar16 a2, vec_bool16 a3)
{
	vec_uchar16 r;
	r.v = __vsel(a1, a2, a3);
	return r;
}

__forceinline vec_uchar16
vec_sel (vec_uchar16 a1, vec_uchar16 a2, vec_uchar16 a3)
{
	vec_uchar16 r;
	r.v = __vsel(a1, a2, a3);
	return r;
}

__forceinline vec_bool16
vec_sel (vec_bool16 a1, vec_bool16 a2, vec_bool16 a3)
{
	vec_bool16 r;
	r.v = __vsel(a1, a2, a3);
	return r;
}

__forceinline vec_bool16
vec_sel (vec_bool16 a1, vec_bool16 a2, vec_uchar16 a3)
{
	vec_bool16 r;
	r.v = __vsel(a1, a2, a3);
	return r;
}

/* vec_sl */

__forceinline vec_char16
vec_sl (vec_char16 a1, vec_uchar16 a2)
{
	vec_char16 r;
	r.v = __vslb(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_sl (vec_uchar16 a1, vec_uchar16 a2)
{
	vec_uchar16 r;
	r.v = __vslb(a1, a2);
	return r;
}

__forceinline vec_short8
vec_sl (vec_short8 a1, vec_ushort8 a2)
{
	vec_short8 r;
	r.v = __vslh(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_sl (vec_ushort8 a1, vec_ushort8 a2)
{
	vec_ushort8 r;
	r.v = __vslh(a1, a2);
	return r;
}

__forceinline vec_int4
vec_sl (vec_int4 a1, vec_uint4 a2)
{
	vec_int4 r;
	r.v = __vslw(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_sl (vec_uint4 a1, vec_uint4 a2)
{
	vec_uint4 r;
	r.v = __vslw(a1, a2);
	return r;
}

/* vec_vslw */

__forceinline vec_int4
vec_vslw (vec_int4 a1, vec_uint4 a2)
{
	vec_int4 r;
	r.v = __vslw(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_vslw (vec_uint4 a1, vec_uint4 a2)
{
	vec_uint4 r;
	r.v = __vslw(a1, a2);
	return r;
}

/* vec_vslh */

__forceinline vec_short8
vec_vslh (vec_short8 a1, vec_ushort8 a2)
{
	vec_short8 r;
	r.v = __vslh(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_vslh (vec_ushort8 a1, vec_ushort8 a2)
{
	vec_ushort8 r;
	r.v = __vslh(a1, a2);
	return r;
}

/* vec_vslb */

__forceinline vec_char16
vec_vslb (vec_char16 a1, vec_uchar16 a2)
{
	vec_char16 r;
	r.v = __vslb(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_vslb (vec_uchar16 a1, vec_uchar16 a2)
{
	vec_uchar16 r;
	r.v = __vslb(a1, a2);
	return r;
}

/* vec_sld */

template<rage::u32 a3, class T> __forceinline T vec_sld(T a1, T a2)
{
	T r; r.v = __vsldoi(a1, a2, a3); return r;
}
#define vec_sld(a1,a2,a3) vec_sld<a3>((a1),(a2))

/* vec_sll */

__forceinline vec_int4
vec_sll (vec_int4 a1, vec_uint4 a2)
{
	vec_int4 r;
	r.v = __vsl(a1, a2);
	return r;
}

__forceinline vec_int4
vec_sll (vec_int4 a1, vec_ushort8 a2)
{
	vec_int4 r;
	r.v = __vsl(a1, a2);
	return r;
}

__forceinline vec_int4
vec_sll (vec_int4 a1, vec_uchar16 a2)
{
	vec_int4 r;
	r.v = __vsl(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_sll (vec_uint4 a1, vec_uint4 a2)
{
	vec_uint4 r;
	r.v = __vsl(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_sll (vec_uint4 a1, vec_ushort8 a2)
{
	vec_uint4 r;
	r.v = __vsl(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_sll (vec_uint4 a1, vec_uchar16 a2)
{
	vec_uint4 r;
	r.v = __vsl(a1, a2);
	return r;
}

__forceinline vec_bool4
vec_sll (vec_bool4 a1, vec_uint4 a2)
{
	vec_bool4 r;
	r.v = __vsl(a1, a2);
	return r;
}

__forceinline vec_bool4
vec_sll (vec_bool4 a1, vec_ushort8 a2)
{
	vec_bool4 r;
	r.v = __vsl(a1, a2);
	return r;
}

__forceinline vec_bool4
vec_sll (vec_bool4 a1, vec_uchar16 a2)
{
	vec_bool4 r;
	r.v = __vsl(a1, a2);
	return r;
}

__forceinline vec_short8
vec_sll (vec_short8 a1, vec_uint4 a2)
{
	vec_short8 r;
	r.v = __vsl(a1, a2);
	return r;
}

__forceinline vec_short8
vec_sll (vec_short8 a1, vec_ushort8 a2)
{
	vec_short8 r;
	r.v = __vsl(a1, a2);
	return r;
}

__forceinline vec_short8
vec_sll (vec_short8 a1, vec_uchar16 a2)
{
	vec_short8 r;
	r.v = __vsl(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_sll (vec_ushort8 a1, vec_uint4 a2)
{
	vec_ushort8 r;
	r.v = __vsl(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_sll (vec_ushort8 a1, vec_ushort8 a2)
{
	vec_ushort8 r;
	r.v = __vsl(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_sll (vec_ushort8 a1, vec_uchar16 a2)
{
	vec_ushort8 r;
	r.v = __vsl(a1, a2);
	return r;
}

__forceinline vec_bool8
vec_sll (vec_bool8 a1, vec_uint4 a2)
{
	vec_bool8 r;
	r.v = __vsl(a1, a2);
	return r;
}

__forceinline vec_bool8
vec_sll (vec_bool8 a1, vec_ushort8 a2)
{
	vec_bool8 r;
	r.v = __vsl(a1, a2);
	return r;
}

__forceinline vec_bool8
vec_sll (vec_bool8 a1, vec_uchar16 a2)
{
	vec_bool8 r;
	r.v = __vsl(a1, a2);
	return r;
}

__forceinline vec_pixel
vec_sll (vec_pixel a1, vec_uint4 a2)
{
	vec_pixel r;
	r.v = __vsl(a1, a2);
	return r;
}

__forceinline vec_pixel
vec_sll (vec_pixel a1, vec_ushort8 a2)
{
	vec_pixel r;
	r.v = __vsl(a1, a2);
	return r;
}

__forceinline vec_pixel
vec_sll (vec_pixel a1, vec_uchar16 a2)
{
	vec_pixel r;
	r.v = __vsl(a1, a2);
	return r;
}

__forceinline vec_char16
vec_sll (vec_char16 a1, vec_uint4 a2)
{
	vec_char16 r;
	r.v = __vsl(a1, a2);
	return r;
}

__forceinline vec_char16
vec_sll (vec_char16 a1, vec_ushort8 a2)
{
	vec_char16 r;
	r.v = __vsl(a1, a2);
	return r;
}

__forceinline vec_char16
vec_sll (vec_char16 a1, vec_uchar16 a2)
{
	vec_char16 r;
	r.v = __vsl(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_sll (vec_uchar16 a1, vec_uint4 a2)
{
	vec_uchar16 r;
	r.v = __vsl(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_sll (vec_uchar16 a1, vec_ushort8 a2)
{
	vec_uchar16 r;
	r.v = __vsl(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_sll (vec_uchar16 a1, vec_uchar16 a2)
{
	vec_uchar16 r;
	r.v = __vsl(a1, a2);
	return r;
}

__forceinline vec_bool16
vec_sll (vec_bool16 a1, vec_uint4 a2)
{
	vec_bool16 r;
	r.v = __vsl(a1, a2);
	return r;
}

__forceinline vec_bool16
vec_sll (vec_bool16 a1, vec_ushort8 a2)
{
	vec_bool16 r;
	r.v = __vsl(a1, a2);
	return r;
}

__forceinline vec_bool16
vec_sll (vec_bool16 a1, vec_uchar16 a2)
{
	vec_bool16 r;
	r.v = __vsl(a1, a2);
	return r;
}

/* vec_slo */

__forceinline vec_float4
vec_slo (vec_float4 a1, vec_char16 a2)
{
	vec_float4 r;
	r.v = __vslo(a1, a2);
	return r;
}

__forceinline vec_float4
vec_slo (vec_float4 a1, vec_uchar16 a2)
{
	vec_float4 r;
	r.v = __vslo(a1, a2);
	return r;
}

__forceinline vec_int4
vec_slo (vec_int4 a1, vec_char16 a2)
{
	vec_int4 r;
	r.v = __vslo(a1, a2);
	return r;
}

__forceinline vec_int4
vec_slo (vec_int4 a1, vec_uchar16 a2)
{
	vec_int4 r;
	r.v = __vslo(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_slo (vec_uint4 a1, vec_char16 a2)
{
	vec_uint4 r;
	r.v = __vslo(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_slo (vec_uint4 a1, vec_uchar16 a2)
{
	vec_uint4 r;
	r.v = __vslo(a1, a2);
	return r;
}

__forceinline vec_short8
vec_slo (vec_short8 a1, vec_char16 a2)
{
	vec_short8 r;
	r.v = __vslo(a1, a2);
	return r;
}

__forceinline vec_short8
vec_slo (vec_short8 a1, vec_uchar16 a2)
{
	vec_short8 r;
	r.v = __vslo(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_slo (vec_ushort8 a1, vec_char16 a2)
{
	vec_ushort8 r;
	r.v = __vslo(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_slo (vec_ushort8 a1, vec_uchar16 a2)
{
	vec_ushort8 r;
	r.v = __vslo(a1, a2);
	return r;
}

__forceinline vec_pixel
vec_slo (vec_pixel a1, vec_char16 a2)
{
	vec_pixel r;
	r.v = __vslo(a1, a2);
	return r;
}

__forceinline vec_pixel
vec_slo (vec_pixel a1, vec_uchar16 a2)
{
	vec_pixel r;
	r.v = __vslo(a1, a2);
	return r;
}

__forceinline vec_char16
vec_slo (vec_char16 a1, vec_char16 a2)
{
	vec_char16 r;
	r.v = __vslo(a1, a2);
	return r;
}

__forceinline vec_char16
vec_slo (vec_char16 a1, vec_uchar16 a2)
{
	vec_char16 r;
	r.v = __vslo(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_slo (vec_uchar16 a1, vec_char16 a2)
{
	vec_uchar16 r;
	r.v = __vslo(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_slo (vec_uchar16 a1, vec_uchar16 a2)
{
	vec_uchar16 r;
	r.v = __vslo(a1, a2);
	return r;
}

/* vec_splat */

template<int a2> __forceinline vec_char16
vec_splat (vec_char16 a1)
{
	vec_char16 r;
	r.v = __vspltb(a1, a2);
	return r;
}

template<int a2> __forceinline vec_uchar16
vec_splat (vec_uchar16 a1)
{
	vec_uchar16 r;
	r.v = __vspltb(a1, a2);
	return r;
}

template<int a2> __forceinline vec_bool16
vec_splat (vec_bool16 a1)
{
	vec_bool16 r;
	r.v = __vspltb(a1, a2);
	return r;
}

template<int a2> __forceinline vec_short8
vec_splat (vec_short8 a1)
{
	vec_short8 r;
	r.v = __vsplth(a1, a2);
	return r;
}

template<int a2> __forceinline vec_ushort8
vec_splat (vec_ushort8 a1)
{
	vec_ushort8 r;
	r.v = __vsplth(a1, a2);
	return r;
}

template<int a2> __forceinline vec_bool8
vec_splat (vec_bool8 a1)
{
	vec_bool8 r;
	r.v = __vsplth(a1, a2);
	return r;
}

template<int a2> __forceinline vec_pixel
vec_splat (vec_pixel a1)
{
	vec_pixel r;
	r.v = __vsplth(a1, a2);
	return r;
}

template<int a2> __forceinline vec_float4
vec_splat (vec_float4 a1)
{
	vec_float4 r;
	r.v = __vspltw(a1, a2);
	return r;
}

template<int a2> __forceinline vec_int4
vec_splat (vec_int4 a1)
{
	vec_int4 r;
	r.v = __vspltw(a1, a2);
	return r;
}

template<int a2> __forceinline vec_uint4
vec_splat (vec_uint4 a1)
{
	vec_uint4 r;
	r.v = __vspltw(a1, a2);
	return r;
}

template<int a2> __forceinline vec_bool4
vec_splat (vec_bool4 a1)
{
	vec_bool4 r;
	r.v = __vspltw(a1, a2);
	return r;
}

#define vec_splat(a1, a2) vec_splat<a2>(a1)

/* vec_vspltw */

template<int a2> __forceinline vec_float4
vec_vspltw (vec_float4 a1)
{
	vec_float4 r;
	r.v = __vspltw(a1, a2);
	return r;
}

template<int a2> __forceinline vec_int4
vec_vspltw (vec_int4 a1)
{
	vec_int4 r;
	r.v = __vspltw(a1, a2);
	return r;
}

template<int a2> __forceinline vec_uint4
vec_vspltw (vec_uint4 a1)
{
	vec_uint4 r;
	r.v = __vspltw(a1, a2);
	return r;
}

template<int a2> __forceinline vec_bool4
vec_vspltw (vec_bool4 a1)
{
	vec_bool4 r;
	r.v = __vspltw(a1, a2);
	return r;
}

#define vec_vspltw(a1, a2) vec_vspltw<a2>(a1)

/* vec_vsplth */

template<int a2> __forceinline vec_bool8
vec_vsplth (vec_bool8 a1)
{
	vec_bool8 r;
	r.v = __vsplth(a1, a2);
	return r;
}

template<int a2> __forceinline vec_short8
vec_vsplth (vec_short8 a1)
{
	vec_short8 r;
	r.v = __vsplth(a1, a2);
	return r;
}

template<int a2> __forceinline vec_ushort8
vec_vsplth (vec_ushort8 a1)
{
	vec_ushort8 r;
	r.v = __vsplth(a1, a2);
	return r;
}

template<int a2> __forceinline vec_pixel
vec_vsplth (vec_pixel a1)
{
	vec_pixel r;
	r.v = __vsplth(a1, a2);
	return r;
}

#define vec_vsplth(a1, a2) vec_vsplth<a2>(a1)

/* vec_vspltb */

template<int a2> __forceinline vec_char16
vec_vspltb (vec_char16 a1)
{
	vec_char16 r;
	r.v = __vspltb(a1, a2);
	return r;
}

template<int a2> __forceinline vec_uchar16
vec_vspltb (vec_uchar16 a1)
{
	vec_uchar16 r;
	r.v = __vspltb(a1, a2);
	return r;
}

template<int a2> __forceinline vec_bool16
vec_vspltb (vec_bool16 a1)
{
	vec_bool16 r;
	r.v = __vspltb(a1, a2);
	return r;
}

#define vec_spltb(a1, a2) vec_spltb<a2>(a1)

/* vec_splat */

template<rage::s32 a1, class T> __forceinline T vec_splat_8() {T r; r.v = __vspltisb(a1); return r;}
template<rage::s32 a1, class T> __forceinline T vec_splat_16() {T r; r.v = __vspltish(a1); return r;}
template<rage::s32 a1, class T> __forceinline T vec_splat_32() {T r; r.v = __vspltisw(a1); return r;}
#define vec_splat_s8(a1)  vec_splat_8 <a1, vec_char16>()
#define vec_splat_s16(a1) vec_splat_16<a1, vec_short8>()
#define vec_splat_s32(a1) vec_splat_32<a1, vec_int4>()
#define vec_splat_u8(a1)  vec_splat_8 <a1, vec_uchar16>()
#define vec_splat_u16(a1) vec_splat_16<a1, vec_ushort8>()
#define vec_splat_u32(a1) vec_splat_32<a1, vec_uint4>()

/* vec_sr */

__forceinline vec_char16
vec_sr (vec_char16 a1, vec_uchar16 a2)
{
	vec_char16 r;
	r.v = __vsrb(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_sr (vec_uchar16 a1, vec_uchar16 a2)
{
	vec_uchar16 r;
	r.v = __vsrb(a1, a2);
	return r;
}

__forceinline vec_short8
vec_sr (vec_short8 a1, vec_ushort8 a2)
{
	vec_short8 r;
	r.v = __vsrh(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_sr (vec_ushort8 a1, vec_ushort8 a2)
{
	vec_ushort8 r;
	r.v = __vsrh(a1, a2);
	return r;
}

__forceinline vec_int4
vec_sr (vec_int4 a1, vec_uint4 a2)
{
	vec_int4 r;
	r.v = __vsrw(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_sr (vec_uint4 a1, vec_uint4 a2)
{
	vec_uint4 r;
	r.v = __vsrw(a1, a2);
	return r;
}

/* vec_vsrw */

__forceinline vec_int4
vec_vsrw (vec_int4 a1, vec_uint4 a2)
{
	vec_int4 r;
	r.v = __vsrw(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_vsrw (vec_uint4 a1, vec_uint4 a2)
{
	vec_uint4 r;
	r.v = __vsrw(a1, a2);
	return r;
}

/* vec_vsrh */

__forceinline vec_short8
vec_vsrh (vec_short8 a1, vec_ushort8 a2)
{
	vec_short8 r;
	r.v = __vsrh(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_vsrh (vec_ushort8 a1, vec_ushort8 a2)
{
	vec_ushort8 r;
	r.v = __vsrh(a1, a2);
	return r;
}

/* vec_vsrb */

__forceinline vec_char16
vec_vsrb (vec_char16 a1, vec_uchar16 a2)
{
	vec_char16 r;
	r.v = __vsrb(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_vsrb (vec_uchar16 a1, vec_uchar16 a2)
{
	vec_uchar16 r;
	r.v = __vsrb(a1, a2);
	return r;
}

/* vec_sra */

__forceinline vec_char16
vec_sra (vec_char16 a1, vec_uchar16 a2)
{
	vec_char16 r;
	r.v = __vsrab(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_sra (vec_uchar16 a1, vec_uchar16 a2)
{
	vec_uchar16 r;
	r.v = __vsrab(a1, a2);
	return r;
}

__forceinline vec_short8
vec_sra (vec_short8 a1, vec_ushort8 a2)
{
	vec_short8 r;
	r.v = __vsrah(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_sra (vec_ushort8 a1, vec_ushort8 a2)
{
	vec_ushort8 r;
	r.v = __vsrah(a1, a2);
	return r;
}

__forceinline vec_int4
vec_sra (vec_int4 a1, vec_uint4 a2)
{
	vec_int4 r;
	r.v = __vsraw(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_sra (vec_uint4 a1, vec_uint4 a2)
{
	vec_uint4 r;
	r.v = __vsraw(a1, a2);
	return r;
}

/* vec_vsraw */

__forceinline vec_int4
vec_vsraw (vec_int4 a1, vec_uint4 a2)
{
	vec_int4 r;
	r.v = __vsraw(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_vsraw (vec_uint4 a1, vec_uint4 a2)
{
	vec_uint4 r;
	r.v = __vsraw(a1, a2);
	return r;
}

/* vec_vsrah */

__forceinline vec_short8
vec_vsrah (vec_short8 a1, vec_ushort8 a2)
{
	vec_short8 r;
	r.v = __vsrah(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_vsrah (vec_ushort8 a1, vec_ushort8 a2)
{
	vec_ushort8 r;
	r.v = __vsrah(a1, a2);
	return r;
}

/* vec_vsrab */

__forceinline vec_char16
vec_vsrab (vec_char16 a1, vec_uchar16 a2)
{
	vec_char16 r;
	r.v = __vsrab(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_vsrab (vec_uchar16 a1, vec_uchar16 a2)
{
	vec_uchar16 r;
	r.v = __vsrab(a1, a2);
	return r;
}

/* vec_srl */

__forceinline vec_int4
vec_srl (vec_int4 a1, vec_uint4 a2)
{
	vec_int4 r;
	r.v = __vsr(a1, a2);
	return r;
}

__forceinline vec_int4
vec_srl (vec_int4 a1, vec_ushort8 a2)
{
	vec_int4 r;
	r.v = __vsr(a1, a2);
	return r;
}

__forceinline vec_int4
vec_srl (vec_int4 a1, vec_uchar16 a2)
{
	vec_int4 r;
	r.v = __vsr(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_srl (vec_uint4 a1, vec_uint4 a2)
{
	vec_uint4 r;
	r.v = __vsr(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_srl (vec_uint4 a1, vec_ushort8 a2)
{
	vec_uint4 r;
	r.v = __vsr(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_srl (vec_uint4 a1, vec_uchar16 a2)
{
	vec_uint4 r;
	r.v = __vsr(a1, a2);
	return r;
}

__forceinline vec_bool4
vec_srl (vec_bool4 a1, vec_uint4 a2)
{
	vec_bool4 r;
	r.v = __vsr(a1, a2);
	return r;
}

__forceinline vec_bool4
vec_srl (vec_bool4 a1, vec_ushort8 a2)
{
	vec_bool4 r;
	r.v = __vsr(a1, a2);
	return r;
}

__forceinline vec_bool4
vec_srl (vec_bool4 a1, vec_uchar16 a2)
{
	vec_bool4 r;
	r.v = __vsr(a1, a2);
	return r;
}

__forceinline vec_short8
vec_srl (vec_short8 a1, vec_uint4 a2)
{
	vec_short8 r;
	r.v = __vsr(a1, a2);
	return r;
}

__forceinline vec_short8
vec_srl (vec_short8 a1, vec_ushort8 a2)
{
	vec_short8 r;
	r.v = __vsr(a1, a2);
	return r;
}

__forceinline vec_short8
vec_srl (vec_short8 a1, vec_uchar16 a2)
{
	vec_short8 r;
	r.v = __vsr(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_srl (vec_ushort8 a1, vec_uint4 a2)
{
	vec_ushort8 r;
	r.v = __vsr(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_srl (vec_ushort8 a1, vec_ushort8 a2)
{
	vec_ushort8 r;
	r.v = __vsr(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_srl (vec_ushort8 a1, vec_uchar16 a2)
{
	vec_ushort8 r;
	r.v = __vsr(a1, a2);
	return r;
}

__forceinline vec_bool8
vec_srl (vec_bool8 a1, vec_uint4 a2)
{
	vec_bool8 r;
	r.v = __vsr(a1, a2);
	return r;
}

__forceinline vec_bool8
vec_srl (vec_bool8 a1, vec_ushort8 a2)
{
	vec_bool8 r;
	r.v = __vsr(a1, a2);
	return r;
}

__forceinline vec_bool8
vec_srl (vec_bool8 a1, vec_uchar16 a2)
{
	vec_bool8 r;
	r.v = __vsr(a1, a2);
	return r;
}

__forceinline vec_pixel
vec_srl (vec_pixel a1, vec_uint4 a2)
{
	vec_pixel r;
	r.v = __vsr(a1, a2);
	return r;
}

__forceinline vec_pixel
vec_srl (vec_pixel a1, vec_ushort8 a2)
{
	vec_pixel r;
	r.v = __vsr(a1, a2);
	return r;
}

__forceinline vec_pixel
vec_srl (vec_pixel a1, vec_uchar16 a2)
{
	vec_pixel r;
	r.v = __vsr(a1, a2);
	return r;
}

__forceinline vec_char16
vec_srl (vec_char16 a1, vec_uint4 a2)
{
	vec_char16 r;
	r.v = __vsr(a1, a2);
	return r;
}

__forceinline vec_char16
vec_srl (vec_char16 a1, vec_ushort8 a2)
{
	vec_char16 r;
	r.v = __vsr(a1, a2);
	return r;
}

__forceinline vec_char16
vec_srl (vec_char16 a1, vec_uchar16 a2)
{
	vec_char16 r;
	r.v = __vsr(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_srl (vec_uchar16 a1, vec_uint4 a2)
{
	vec_uchar16 r;
	r.v = __vsr(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_srl (vec_uchar16 a1, vec_ushort8 a2)
{
	vec_uchar16 r;
	r.v = __vsr(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_srl (vec_uchar16 a1, vec_uchar16 a2)
{
	vec_uchar16 r;
	r.v = __vsr(a1, a2);
	return r;
}

__forceinline vec_bool16
vec_srl (vec_bool16 a1, vec_uint4 a2)
{
	vec_bool16 r;
	r.v = __vsr(a1, a2);
	return r;
}

__forceinline vec_bool16
vec_srl (vec_bool16 a1, vec_ushort8 a2)
{
	vec_bool16 r;
	r.v = __vsr(a1, a2);
	return r;
}

__forceinline vec_bool16
vec_srl (vec_bool16 a1, vec_uchar16 a2)
{
	vec_bool16 r;
	r.v = __vsr(a1, a2);
	return r;
}

/* vec_sro */

__forceinline vec_float4
vec_sro (vec_float4 a1, vec_char16 a2)
{
	vec_float4 r;
	r.v = __vsro(a1, a2);
	return r;
}

__forceinline vec_float4
vec_sro (vec_float4 a1, vec_uchar16 a2)
{
	vec_float4 r;
	r.v = __vsro(a1, a2);
	return r;
}

__forceinline vec_int4
vec_sro (vec_int4 a1, vec_char16 a2)
{
	vec_int4 r;
	r.v = __vsro(a1, a2);
	return r;
}

__forceinline vec_int4
vec_sro (vec_int4 a1, vec_uchar16 a2)
{
	vec_int4 r;
	r.v = __vsro(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_sro (vec_uint4 a1, vec_char16 a2)
{
	vec_uint4 r;
	r.v = __vsro(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_sro (vec_uint4 a1, vec_uchar16 a2)
{
	vec_uint4 r;
	r.v = __vsro(a1, a2);
	return r;
}

__forceinline vec_short8
vec_sro (vec_short8 a1, vec_char16 a2)
{
	vec_short8 r;
	r.v = __vsro(a1, a2);
	return r;
}

__forceinline vec_short8
vec_sro (vec_short8 a1, vec_uchar16 a2)
{
	vec_short8 r;
	r.v = __vsro(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_sro (vec_ushort8 a1, vec_char16 a2)
{
	vec_ushort8 r;
	r.v = __vsro(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_sro (vec_ushort8 a1, vec_uchar16 a2)
{
	vec_ushort8 r;
	r.v = __vsro(a1, a2);
	return r;
}

__forceinline vec_pixel
vec_sro (vec_pixel a1, vec_char16 a2)
{
	vec_pixel r;
	r.v = __vsro(a1, a2);
	return r;
}

__forceinline vec_pixel
vec_sro (vec_pixel a1, vec_uchar16 a2)
{
	vec_pixel r;
	r.v = __vsro(a1, a2);
	return r;
}

__forceinline vec_char16
vec_sro (vec_char16 a1, vec_char16 a2)
{
	vec_char16 r;
	r.v = __vsro(a1, a2);
	return r;
}

__forceinline vec_char16
vec_sro (vec_char16 a1, vec_uchar16 a2)
{
	vec_char16 r;
	r.v = __vsro(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_sro (vec_uchar16 a1, vec_char16 a2)
{
	vec_uchar16 r;
	r.v = __vsro(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_sro (vec_uchar16 a1, vec_uchar16 a2)
{
	vec_uchar16 r;
	r.v = __vsro(a1, a2);
	return r;
}

/* vec_st */

__forceinline void
vec_st (vec_float4 a1, OFFSET_T a2, vec_float4 *a3)
{
	__stvx(a1, (void *) a3, a2);
}

__forceinline void
vec_st (vec_float4 a1, OFFSET_T a2, float *a3)
{
	__stvx(a1, (void *) a3, a2);
}

__forceinline void
vec_st (vec_int4 a1, OFFSET_T a2, vec_int4 *a3)
{
	__stvx(a1, (void *) a3, a2);
}

__forceinline void
vec_st (vec_int4 a1, OFFSET_T a2, int *a3)
{
	__stvx(a1, (void *) a3, a2);
}

__forceinline void
vec_st (vec_uint4 a1, OFFSET_T a2, vec_uint4 *a3)
{
	__stvx(a1, (void *) a3, a2);
}

__forceinline void
vec_st (vec_uint4 a1, OFFSET_T a2, unsigned int *a3)
{
	__stvx(a1, (void *) a3, a2);
}

__forceinline void
vec_st (vec_bool4 a1, OFFSET_T a2, vec_bool4 *a3)
{
	__stvx(a1, (void *) a3, a2);
}

__forceinline void
vec_st (vec_bool4 a1, OFFSET_T a2, unsigned int *a3)
{
	__stvx(a1, (void *) a3, a2);
}

__forceinline void
vec_st (vec_bool4 a1, OFFSET_T a2, int *a3)
{
	__stvx(a1, (void *) a3, a2);
}

__forceinline void
vec_st (vec_short8 a1, OFFSET_T a2, vec_short8 *a3)
{
	__stvx(a1, (void *) a3, a2);
}

__forceinline void
vec_st (vec_short8 a1, OFFSET_T a2, short *a3)
{
	__stvx(a1, (void *) a3, a2);
}

__forceinline void
vec_st (vec_ushort8 a1, OFFSET_T a2, vec_ushort8 *a3)
{
	__stvx(a1, (void *) a3, a2);
}

__forceinline void
vec_st (vec_ushort8 a1, OFFSET_T a2, unsigned short *a3)
{
	__stvx(a1, (void *) a3, a2);
}

__forceinline void
vec_st (vec_bool8 a1, OFFSET_T a2, vec_bool8 *a3)
{
	__stvx(a1, (void *) a3, a2);
}

__forceinline void
vec_st (vec_bool8 a1, OFFSET_T a2, unsigned short *a3)
{
	__stvx(a1, (void *) a3, a2);
}

__forceinline void
vec_st (vec_pixel a1, OFFSET_T a2, vec_pixel *a3)
{
	__stvx(a1, (void *) a3, a2);
}

__forceinline void
vec_st (vec_pixel a1, OFFSET_T a2, unsigned short *a3)
{
	__stvx(a1, (void *) a3, a2);
}

__forceinline void
vec_st (vec_pixel a1, OFFSET_T a2, short *a3)
{
	__stvx(a1, (void *) a3, a2);
}

__forceinline void
vec_st (vec_bool8 a1, OFFSET_T a2, short *a3)
{
	__stvx(a1, (void *) a3, a2);
}

__forceinline void
vec_st (vec_char16 a1, OFFSET_T a2, vec_char16 *a3)
{
	__stvx(a1, (void *) a3, a2);
}

__forceinline void
vec_st (vec_char16 a1, OFFSET_T a2, signed char *a3)
{
	__stvx(a1, (void *) a3, a2);
}

__forceinline void
vec_st (vec_uchar16 a1, OFFSET_T a2, vec_uchar16 *a3)
{
	__stvx(a1, (void *) a3, a2);
}

__forceinline void
vec_st (vec_uchar16 a1, OFFSET_T a2, unsigned char *a3)
{
	__stvx(a1, (void *) a3, a2);
}

__forceinline void
vec_st (vec_bool16 a1, OFFSET_T a2, vec_bool16 *a3)
{
	__stvx(a1, (void *) a3, a2);
}

__forceinline void
vec_st (vec_bool16 a1, OFFSET_T a2, unsigned char *a3)
{
	__stvx(a1, (void *) a3, a2);
}

__forceinline void
vec_st (vec_bool16 a1, OFFSET_T a2, signed char *a3)
{
	__stvx(a1, (void *) a3, a2);
}

/* vec_ste */

__forceinline void
vec_ste (vec_char16 a1, OFFSET_T a2, signed char *a3)
{
	__stvebx(a1, (void *) a3, a2);
}

__forceinline void
vec_ste (vec_uchar16 a1, OFFSET_T a2, unsigned char *a3)
{
	__stvebx(a1, (void *) a3, a2);
}

__forceinline void
vec_ste (vec_bool16 a1, OFFSET_T a2, signed char *a3)
{
	__stvebx(a1, (void *) a3, a2);
}

__forceinline void
vec_ste (vec_bool16 a1, OFFSET_T a2, unsigned char *a3)
{
	__stvebx(a1, (void *) a3, a2);
}

__forceinline void
vec_ste (vec_short8 a1, OFFSET_T a2, short *a3)
{
	__stvehx(a1, (void *) a3, a2);
}

__forceinline void
vec_ste (vec_ushort8 a1, OFFSET_T a2, unsigned short *a3)
{
	__stvehx(a1, (void *) a3, a2);
}

__forceinline void
vec_ste (vec_bool8 a1, OFFSET_T a2, short *a3)
{
	__stvehx(a1, (void *) a3, a2);
}

__forceinline void
vec_ste (vec_bool8 a1, OFFSET_T a2, unsigned short *a3)
{
	__stvehx(a1, (void *) a3, a2);
}

__forceinline void
vec_ste (vec_pixel a1, OFFSET_T a2, short *a3)
{
	__stvehx(a1, (void *) a3, a2);
}

__forceinline void
vec_ste (vec_pixel a1, OFFSET_T a2, unsigned short *a3)
{
	__stvehx(a1, (void *) a3, a2);
}

__forceinline void
vec_ste (vec_float4 a1, OFFSET_T a2, float *a3)
{
	__stvewx(a1, (void *) a3, a2);
}

__forceinline void
vec_ste (vec_int4 a1, OFFSET_T a2, int *a3)
{
	__stvewx(a1, (void *) a3, a2);
}

__forceinline void
vec_ste (vec_uint4 a1, OFFSET_T a2, unsigned int *a3)
{
	__stvewx(a1, (void *) a3, a2);
}

__forceinline void
vec_ste (vec_bool4 a1, OFFSET_T a2, int *a3)
{
	__stvewx(a1, (void *) a3, a2);
}

__forceinline void
vec_ste (vec_bool4 a1, OFFSET_T a2, unsigned int *a3)
{
	__stvewx(a1, (void *) a3, a2);
}

/* vec_stvewx */

__forceinline void
vec_stvewx (vec_float4 a1, OFFSET_T a2, float *a3)
{
	__stvewx(a1, (void *) a3, a2);
}

__forceinline void
vec_stvewx (vec_int4 a1, OFFSET_T a2, int *a3)
{
	__stvewx(a1, (void *) a3, a2);
}

__forceinline void
vec_stvewx (vec_uint4 a1, OFFSET_T a2, unsigned int *a3)
{
	__stvewx(a1, (void *) a3, a2);
}

__forceinline void
vec_stvewx (vec_bool4 a1, OFFSET_T a2, int *a3)
{
	__stvewx(a1, (void *) a3, a2);
}

__forceinline void
vec_stvewx (vec_bool4 a1, OFFSET_T a2, unsigned int *a3)
{
	__stvewx(a1, (void *) a3, a2);
}

/* vec_stvehx */

__forceinline void
vec_stvehx (vec_short8 a1, OFFSET_T a2, short *a3)
{
	__stvehx(a1, (void *) a3, a2);
}

__forceinline void
vec_stvehx (vec_ushort8 a1, OFFSET_T a2, unsigned short *a3)
{
	__stvehx(a1, (void *) a3, a2);
}

__forceinline void
vec_stvehx (vec_bool8 a1, OFFSET_T a2, short *a3)
{
	__stvehx(a1, (void *) a3, a2);
}

__forceinline void
vec_stvehx (vec_bool8 a1, OFFSET_T a2, unsigned short *a3)
{
	__stvehx(a1, (void *) a3, a2);
}

__forceinline void
vec_stvehx (vec_pixel a1, OFFSET_T a2, short *a3)
{
	__stvehx(a1, (void *) a3, a2);
}

__forceinline void
vec_stvehx (vec_pixel a1, OFFSET_T a2, unsigned short *a3)
{
	__stvehx(a1, (void *) a3, a2);
}

/* vec_stvebx */

__forceinline void
vec_stvebx (vec_char16 a1, OFFSET_T a2, signed char *a3)
{
	__stvebx(a1, (void *) a3, a2);
}

__forceinline void
vec_stvebx (vec_uchar16 a1, OFFSET_T a2, unsigned char *a3)
{
	__stvebx(a1, (void *) a3, a2);
}

__forceinline void
vec_stvebx (vec_bool16 a1, OFFSET_T a2, signed char *a3)
{
	__stvebx(a1, (void *) a3, a2);
}

__forceinline void
vec_stvebx (vec_bool16 a1, OFFSET_T a2, unsigned char *a3)
{
	__stvebx(a1, (void *) a3, a2);
}

/* vec_stl */

__forceinline void
vec_stl (vec_float4 a1, OFFSET_T a2, vec_float4 *a3)
{
	__stvxl(a1, (void *) a3, a2);
}

__forceinline void
vec_stl (vec_float4 a1, OFFSET_T a2, float *a3)
{
	__stvxl(a1, (void *) a3, a2);
}

__forceinline void
vec_stl (vec_int4 a1, OFFSET_T a2, vec_int4 *a3)
{
	__stvxl(a1, (void *) a3, a2);
}

__forceinline void
vec_stl (vec_int4 a1, OFFSET_T a2, int *a3)
{
	__stvxl(a1, (void *) a3, a2);
}

__forceinline void
vec_stl (vec_uint4 a1, OFFSET_T a2, vec_uint4 *a3)
{
	__stvxl(a1, (void *) a3, a2);
}

__forceinline void
vec_stl (vec_uint4 a1, OFFSET_T a2, unsigned int *a3)
{
	__stvxl(a1, (void *) a3, a2);
}

__forceinline void
vec_stl (vec_bool4 a1, OFFSET_T a2, vec_bool4 *a3)
{
	__stvxl(a1, (void *) a3, a2);
}

__forceinline void
vec_stl (vec_bool4 a1, OFFSET_T a2, unsigned int *a3)
{
	__stvxl(a1, (void *) a3, a2);
}

__forceinline void
vec_stl (vec_bool4 a1, OFFSET_T a2, int *a3)
{
	__stvxl(a1, (void *) a3, a2);
}

__forceinline void
vec_stl (vec_short8 a1, OFFSET_T a2, vec_short8 *a3)
{
	__stvxl(a1, (void *) a3, a2);
}

__forceinline void
vec_stl (vec_short8 a1, OFFSET_T a2, short *a3)
{
	__stvxl(a1, (void *) a3, a2);
}

__forceinline void
vec_stl (vec_ushort8 a1, OFFSET_T a2, vec_ushort8 *a3)
{
	__stvxl(a1, (void *) a3, a2);
}

__forceinline void
vec_stl (vec_ushort8 a1, OFFSET_T a2, unsigned short *a3)
{
	__stvxl(a1, (void *) a3, a2);
}

__forceinline void
vec_stl (vec_bool8 a1, OFFSET_T a2, vec_bool8 *a3)
{
	__stvxl(a1, (void *) a3, a2);
}

__forceinline void
vec_stl (vec_bool8 a1, OFFSET_T a2, unsigned short *a3)
{
	__stvxl(a1, (void *) a3, a2);
}

__forceinline void
vec_stl (vec_bool8 a1, OFFSET_T a2, short *a3)
{
	__stvxl(a1, (void *) a3, a2);
}

__forceinline void
vec_stl (vec_pixel a1, OFFSET_T a2, vec_pixel *a3)
{
	__stvxl(a1, (void *) a3, a2);
}

__forceinline void
vec_stl (vec_pixel a1, OFFSET_T a2, unsigned short *a3)
{
	__stvxl(a1, (void *) a3, a2);
}

__forceinline void
vec_stl (vec_pixel a1, OFFSET_T a2, short *a3)
{
	__stvxl(a1, (void *) a3, a2);
}

__forceinline void
vec_stl (vec_char16 a1, OFFSET_T a2, vec_char16 *a3)
{
	__stvxl(a1, (void *) a3, a2);
}

__forceinline void
vec_stl (vec_char16 a1, OFFSET_T a2, signed char *a3)
{
	__stvxl(a1, (void *) a3, a2);
}

__forceinline void
vec_stl (vec_uchar16 a1, OFFSET_T a2, vec_uchar16 *a3)
{
	__stvxl(a1, (void *) a3, a2);
}

__forceinline void
vec_stl (vec_uchar16 a1, OFFSET_T a2, unsigned char *a3)
{
	__stvxl(a1, (void *) a3, a2);
}

__forceinline void
vec_stl (vec_bool16 a1, OFFSET_T a2, vec_bool16 *a3)
{
	__stvxl(a1, (void *) a3, a2);
}

__forceinline void
vec_stl (vec_bool16 a1, OFFSET_T a2, unsigned char *a3)
{
	__stvxl(a1, (void *) a3, a2);
}

__forceinline void
vec_stl (vec_bool16 a1, OFFSET_T a2, signed char *a3)
{
	__stvxl(a1, (void *) a3, a2);
}


/* begin sce local, bugzilla #8763 */
/* vec_stvlx */

__forceinline void
vec_stvlx (vec_char16 a1, OFFSET_T a2, signed char *a3)
{
	__stvlx(a1, (void *) a3, a2);
}

__forceinline void
vec_stvlx (vec_uchar16 a1, OFFSET_T a2, unsigned char *a3)
{
	__stvlx(a1, (void *) a3, a2);
}

__forceinline void
vec_stvlx (vec_bool16 a1, OFFSET_T a2, signed char *a3)
{
	__stvlx(a1, (void *) a3, a2);
}

__forceinline void
vec_stvlx (vec_bool16 a1, OFFSET_T a2, unsigned char *a3)
{
	__stvlx(a1, (void *) a3, a2);
}

/* vec_stvlxl */

__forceinline void
vec_stvlxl (vec_char16 a1, OFFSET_T a2, signed char *a3)
{
	__stvlxl(a1, (void *) a3, a2);
}

__forceinline void
vec_stvlxl (vec_uchar16 a1, OFFSET_T a2, unsigned char *a3)
{
	__stvlxl(a1, (void *) a3, a2);
}

__forceinline void
vec_stvlxl (vec_bool16 a1, OFFSET_T a2, signed char *a3)
{
	__stvlxl(a1, (void *) a3, a2);
}

__forceinline void
vec_stvlxl (vec_bool16 a1, OFFSET_T a2, unsigned char *a3)
{
	__stvlxl(a1, (void *) a3, a2);
}

/* vec_stvrx */

__forceinline void
vec_stvrx (vec_char16 a1, OFFSET_T a2, signed char *a3)
{
	__stvrx(a1, (void *) a3, a2);
}

__forceinline void
vec_stvrx (vec_uchar16 a1, OFFSET_T a2, unsigned char *a3)
{
	__stvrx(a1, (void *) a3, a2);
}

__forceinline void
vec_stvrx (vec_bool16 a1, OFFSET_T a2, signed char *a3)
{
	__stvrx(a1, (void *) a3, a2);
}

__forceinline void
vec_stvrx (vec_bool16 a1, OFFSET_T a2, unsigned char *a3)
{
	__stvrx(a1, (void *) a3, a2);
}

/* vec_stvrxl */

__forceinline void
vec_stvrxl (vec_char16 a1, OFFSET_T a2, signed char *a3)
{
	__stvrxl(a1, (void *) a3, a2);
}

__forceinline void
vec_stvrxl (vec_uchar16 a1, OFFSET_T a2, unsigned char *a3)
{
	__stvrxl(a1, (void *) a3, a2);
}

__forceinline void
vec_stvrxl (vec_bool16 a1, OFFSET_T a2, signed char *a3)
{
	__stvrxl(a1, (void *) a3, a2);
}

__forceinline void
vec_stvrxl (vec_bool16 a1, OFFSET_T a2, unsigned char *a3)
{
	__stvrxl(a1, (void *) a3, a2);
}
/* end sce local */
 
/* vec_sub */

__forceinline vec_char16
vec_sub (vec_bool16 a1, vec_char16 a2)
{
	vec_char16 r;
	r.v = __vsububm(a1, a2);
	return r;
}

__forceinline vec_char16
vec_sub (vec_char16 a1, vec_bool16 a2)
{
	vec_char16 r;
	r.v = __vsububm(a1, a2);
	return r;
}

__forceinline vec_char16
vec_sub (vec_char16 a1, vec_char16 a2)
{
	vec_char16 r;
	r.v = __vsububm(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_sub (vec_bool16 a1, vec_uchar16 a2)
{
	vec_uchar16 r;
	r.v = __vsububm(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_sub (vec_uchar16 a1, vec_bool16 a2)
{
	vec_uchar16 r;
	r.v = __vsububm(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_sub (vec_uchar16 a1, vec_uchar16 a2)
{
	vec_uchar16 r;
	r.v = __vsububm(a1, a2);
	return r;
}

__forceinline vec_short8
vec_sub (vec_bool8 a1, vec_short8 a2)
{
	vec_short8 r;
	r.v = __vsubuhm(a1, a2);
	return r;
}

__forceinline vec_short8
vec_sub (vec_short8 a1, vec_bool8 a2)
{
	vec_short8 r;
	r.v = __vsubuhm(a1, a2);
	return r;
}

__forceinline vec_short8
vec_sub (vec_short8 a1, vec_short8 a2)
{
	vec_short8 r;
	r.v = __vsubuhm(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_sub (vec_bool8 a1, vec_ushort8 a2)
{
	vec_ushort8 r;
	r.v = __vsubuhm(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_sub (vec_ushort8 a1, vec_bool8 a2)
{
	vec_ushort8 r;
	r.v = __vsubuhm(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_sub (vec_ushort8 a1, vec_ushort8 a2)
{
	vec_ushort8 r;
	r.v = __vsubuhm(a1, a2);
	return r;
}

__forceinline vec_int4
vec_sub (vec_bool4 a1, vec_int4 a2)
{
	vec_int4 r;
	r.v = __vsubuwm(a1, a2);
	return r;
}

__forceinline vec_int4
vec_sub (vec_int4 a1, vec_bool4 a2)
{
	vec_int4 r;
	r.v = __vsubuwm(a1, a2);
	return r;
}

__forceinline vec_int4
vec_sub (vec_int4 a1, vec_int4 a2)
{
	vec_int4 r;
	r.v = __vsubuwm(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_sub (vec_bool4 a1, vec_uint4 a2)
{
	vec_uint4 r;
	r.v = __vsubuwm(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_sub (vec_uint4 a1, vec_bool4 a2)
{
	vec_uint4 r;
	r.v = __vsubuwm(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_sub (vec_uint4 a1, vec_uint4 a2)
{
	vec_uint4 r;
	r.v = __vsubuwm(a1, a2);
	return r;
}

__forceinline vec_float4
vec_sub (vec_float4 a1, vec_float4 a2)
{
	vec_float4 r;
	r.v = __vsubfp(a1, a2);
	return r;
}

/* vec_vsubfp */

__forceinline vec_float4
vec_vsubfp (vec_float4 a1, vec_float4 a2)
{
	vec_float4 r;
	r.v = __vsubfp(a1, a2);
	return r;
}

/* vec_vsubuwm */

__forceinline vec_int4
vec_vsubuwm (vec_bool4 a1, vec_int4 a2)
{
	vec_int4 r;
	r.v = __vsubuwm(a1, a2);
	return r;
}

__forceinline vec_int4
vec_vsubuwm (vec_int4 a1, vec_bool4 a2)
{
	vec_int4 r;
	r.v = __vsubuwm(a1, a2);
	return r;
}

__forceinline vec_int4
vec_vsubuwm (vec_int4 a1, vec_int4 a2)
{
	vec_int4 r;
	r.v = __vsubuwm(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_vsubuwm (vec_bool4 a1, vec_uint4 a2)
{
	vec_uint4 r;
	r.v = __vsubuwm(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_vsubuwm (vec_uint4 a1, vec_bool4 a2)
{
	vec_uint4 r;
	r.v = __vsubuwm(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_vsubuwm (vec_uint4 a1, vec_uint4 a2)
{
	vec_uint4 r;
	r.v = __vsubuwm(a1, a2);
	return r;
}

/* vec_vsubuhm */

__forceinline vec_short8
vec_vsubuhm (vec_bool8 a1, vec_short8 a2)
{
	vec_short8 r;
	r.v = __vsubuhm(a1, a2);
	return r;
}

__forceinline vec_short8
vec_vsubuhm (vec_short8 a1, vec_bool8 a2)
{
	vec_short8 r;
	r.v = __vsubuhm(a1, a2);
	return r;
}

__forceinline vec_short8
vec_vsubuhm (vec_short8 a1, vec_short8 a2)
{
	vec_short8 r;
	r.v = __vsubuhm(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_vsubuhm (vec_bool8 a1, vec_ushort8 a2)
{
	vec_ushort8 r;
	r.v = __vsubuhm(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_vsubuhm (vec_ushort8 a1, vec_bool8 a2)
{
	vec_ushort8 r;
	r.v = __vsubuhm(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_vsubuhm (vec_ushort8 a1, vec_ushort8 a2)
{
	vec_ushort8 r;
	r.v = __vsubuhm(a1, a2);
	return r;
}

/* vec_vsububm */

__forceinline vec_char16
vec_vsububm (vec_bool16 a1, vec_char16 a2)
{
	vec_char16 r;
	r.v = __vsububm(a1, a2);
	return r;
}

__forceinline vec_char16
vec_vsububm (vec_char16 a1, vec_bool16 a2)
{
	vec_char16 r;
	r.v = __vsububm(a1, a2);
	return r;
}

__forceinline vec_char16
vec_vsububm (vec_char16 a1, vec_char16 a2)
{
	vec_char16 r;
	r.v = __vsububm(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_vsububm (vec_bool16 a1, vec_uchar16 a2)
{
	vec_uchar16 r;
	r.v = __vsububm(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_vsububm (vec_uchar16 a1, vec_bool16 a2)
{
	vec_uchar16 r;
	r.v = __vsububm(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_vsububm (vec_uchar16 a1, vec_uchar16 a2)
{
	vec_uchar16 r;
	r.v = __vsububm(a1, a2);
	return r;
}

/* vec_subc */

__forceinline vec_uint4
vec_subc (vec_uint4 a1, vec_uint4 a2)
{
	vec_uint4 r;
	r.v = __vsubcuw(a1, a2);
	return r;
}

/* vec_subs */

__forceinline vec_uchar16
vec_subs (vec_bool16 a1, vec_uchar16 a2)
{
	vec_uchar16 r;
	r.v = __vsububs(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_subs (vec_uchar16 a1, vec_bool16 a2)
{
	vec_uchar16 r;
	r.v = __vsububs(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_subs (vec_uchar16 a1, vec_uchar16 a2)
{
	vec_uchar16 r;
	r.v = __vsububs(a1, a2);
	return r;
}

__forceinline vec_char16
vec_subs (vec_bool16 a1, vec_char16 a2)
{
	vec_char16 r;
	r.v = __vsubsbs(a1, a2);
	return r;
}

__forceinline vec_char16
vec_subs (vec_char16 a1, vec_bool16 a2)
{
	vec_char16 r;
	r.v = __vsubsbs(a1, a2);
	return r;
}

__forceinline vec_char16
vec_subs (vec_char16 a1, vec_char16 a2)
{
	vec_char16 r;
	r.v = __vsubsbs(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_subs (vec_bool8 a1, vec_ushort8 a2)
{
	vec_ushort8 r;
	r.v = __vsubuhs(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_subs (vec_ushort8 a1, vec_bool8 a2)
{
	vec_ushort8 r;
	r.v = __vsubuhs(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_subs (vec_ushort8 a1, vec_ushort8 a2)
{
	vec_ushort8 r;
	r.v = __vsubuhs(a1, a2);
	return r;
}

__forceinline vec_short8
vec_subs (vec_bool8 a1, vec_short8 a2)
{
	vec_short8 r;
	r.v = __vsubshs(a1, a2);
	return r;
}

__forceinline vec_short8
vec_subs (vec_short8 a1, vec_bool8 a2)
{
	vec_short8 r;
	r.v = __vsubshs(a1, a2);
	return r;
}

__forceinline vec_short8
vec_subs (vec_short8 a1, vec_short8 a2)
{
	vec_short8 r;
	r.v = __vsubshs(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_subs (vec_bool4 a1, vec_uint4 a2)
{
	vec_uint4 r;
	r.v = __vsubuws(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_subs (vec_uint4 a1, vec_bool4 a2)
{
	vec_uint4 r;
	r.v = __vsubuws(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_subs (vec_uint4 a1, vec_uint4 a2)
{
	vec_uint4 r;
	r.v = __vsubuws(a1, a2);
	return r;
}

__forceinline vec_int4
vec_subs (vec_bool4 a1, vec_int4 a2)
{
	vec_int4 r;
	r.v = __vsubsws(a1, a2);
	return r;
}

__forceinline vec_int4
vec_subs (vec_int4 a1, vec_bool4 a2)
{
	vec_int4 r;
	r.v = __vsubsws(a1, a2);
	return r;
}

__forceinline vec_int4
vec_subs (vec_int4 a1, vec_int4 a2)
{
	vec_int4 r;
	r.v = __vsubsws(a1, a2);
	return r;
}

/* vec_vsubsws */

__forceinline vec_int4
vec_vsubsws (vec_bool4 a1, vec_int4 a2)
{
	vec_int4 r;
	r.v = __vsubsws(a1, a2);
	return r;
}

__forceinline vec_int4
vec_vsubsws (vec_int4 a1, vec_bool4 a2)
{
	vec_int4 r;
	r.v = __vsubsws(a1, a2);
	return r;
}

__forceinline vec_int4
vec_vsubsws (vec_int4 a1, vec_int4 a2)
{
	vec_int4 r;
	r.v = __vsubsws(a1, a2);
	return r;
}

/* vec_vsubuws */

__forceinline vec_uint4
vec_vsubuws (vec_bool4 a1, vec_uint4 a2)
{
	vec_uint4 r;
	r.v = __vsubuws(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_vsubuws (vec_uint4 a1, vec_bool4 a2)
{
	vec_uint4 r;
	r.v = __vsubuws(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_vsubuws (vec_uint4 a1, vec_uint4 a2)
{
	vec_uint4 r;
	r.v = __vsubuws(a1, a2);
	return r;
}

/* vec_vsubshs */

__forceinline vec_short8
vec_vsubshs (vec_bool8 a1, vec_short8 a2)
{
	vec_short8 r;
	r.v = __vsubshs(a1, a2);
	return r;
}

__forceinline vec_short8
vec_vsubshs (vec_short8 a1, vec_bool8 a2)
{
	vec_short8 r;
	r.v = __vsubshs(a1, a2);
	return r;
}

__forceinline vec_short8
vec_vsubshs (vec_short8 a1, vec_short8 a2)
{
	vec_short8 r;
	r.v = __vsubshs(a1, a2);
	return r;
}

/* vec_vsubuhs */

__forceinline vec_ushort8
vec_vsubuhs (vec_bool8 a1, vec_ushort8 a2)
{
	vec_ushort8 r;
	r.v = __vsubuhs(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_vsubuhs (vec_ushort8 a1, vec_bool8 a2)
{
	vec_ushort8 r;
	r.v = __vsubuhs(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_vsubuhs (vec_ushort8 a1, vec_ushort8 a2)
{
	vec_ushort8 r;
	r.v = __vsubuhs(a1, a2);
	return r;
}

/* vec_vsubsbs */

__forceinline vec_char16
vec_vsubsbs (vec_bool16 a1, vec_char16 a2)
{
	vec_char16 r;
	r.v = __vsubsbs(a1, a2);
	return r;
}

__forceinline vec_char16
vec_vsubsbs (vec_char16 a1, vec_bool16 a2)
{
	vec_char16 r;
	r.v = __vsubsbs(a1, a2);
	return r;
}

__forceinline vec_char16
vec_vsubsbs (vec_char16 a1, vec_char16 a2)
{
	vec_char16 r;
	r.v = __vsubsbs(a1, a2);
	return r;
}

/* vec_vsububs */

__forceinline vec_uchar16
vec_vsububs (vec_bool16 a1, vec_uchar16 a2)
{
	vec_uchar16 r;
	r.v = __vsububs(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_vsububs (vec_uchar16 a1, vec_bool16 a2)
{
	vec_uchar16 r;
	r.v = __vsububs(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_vsububs (vec_uchar16 a1, vec_uchar16 a2)
{
	vec_uchar16 r;
	r.v = __vsububs(a1, a2);
	return r;
}

/* vec_trunc */

__forceinline vec_float4
vec_trunc (vec_float4 a1)
{
	vec_float4 r;
	r.v = __vrfiz(a1);
	return r;
}

/* vec_unpackh */

__forceinline vec_short8
vec_unpackh (vec_char16 a1)
{
	vec_short8 r;
	r.v = __vupkhsb(a1);
	return r;
}

__forceinline vec_bool8
vec_unpackh (vec_bool16 a1)
{
	vec_bool8 r;
	r.v = __vupkhsb(a1);
	return r;
}

__forceinline vec_int4
vec_unpackh (vec_short8 a1)
{
	vec_int4 r;
	r.v = __vupkhsh(a1);
	return r;
}

__forceinline vec_bool4
vec_unpackh (vec_bool8 a1)
{
	vec_bool4 r;
	r.v = __vupkhsh(a1);
	return r;
}

__forceinline vec_uint4
vec_unpackh (vec_pixel a1)
{
	vec_uint4 r;
	r.v = __vupkhpx(a1);
	return r;
}

/* vec_vupkhsh */

__forceinline vec_bool4
vec_vupkhsh (vec_bool8 a1)
{
	vec_bool4 r;
	r.v = __vupkhsh(a1);
	return r;
}

__forceinline vec_int4
vec_vupkhsh (vec_short8 a1)
{
	vec_int4 r;
	r.v = __vupkhsh(a1);
	return r;
}

/* vec_vupkhpx */

__forceinline vec_uint4
vec_vupkhpx (vec_pixel a1)
{
	vec_uint4 r;
	r.v = __vupkhpx(a1);
	return r;
}

/* vec_vupkhsb */

__forceinline vec_bool8
vec_vupkhsb (vec_bool16 a1)
{
	vec_bool8 r;
	r.v = __vupkhsb(a1);
	return r;
}

__forceinline vec_short8
vec_vupkhsb (vec_char16 a1)
{
	vec_short8 r;
	r.v = __vupkhsb(a1);
	return r;
}

/* vec_unpackl */

__forceinline vec_short8
vec_unpackl (vec_char16 a1)
{
	vec_short8 r;
	r.v = __vupklsb(a1);
	return r;
}

__forceinline vec_bool8
vec_unpackl (vec_bool16 a1)
{
	vec_bool8 r;
	r.v = __vupklsb(a1);
	return r;
}

__forceinline vec_uint4
vec_unpackl (vec_pixel a1)
{
	vec_uint4 r;
	r.v = __vupklpx(a1);
	return r;
}

__forceinline vec_int4
vec_unpackl (vec_short8 a1)
{
	vec_int4 r;
	r.v = __vupklsh(a1);
	return r;
}

__forceinline vec_bool4
vec_unpackl (vec_bool8 a1)
{
	vec_bool4 r;
	r.v = __vupklsh(a1);
	return r;
}

/* vec_vupklpx */

__forceinline vec_uint4
vec_vupklpx (vec_pixel a1)
{
	vec_uint4 r;
	r.v = __vupklpx(a1);
	return r;
}

/* vec_upklsh */

__forceinline vec_bool4
vec_vupklsh (vec_bool8 a1)
{
	vec_bool4 r;
	r.v = __vupklsh(a1);
	return r;
}

__forceinline vec_int4
vec_vupklsh (vec_short8 a1)
{
	vec_int4 r;
	r.v = __vupklsh(a1);
	return r;
}

/* vec_vupklsb */

__forceinline vec_bool8
vec_vupklsb (vec_bool16 a1)
{
	vec_bool8 r;
	r.v = __vupklsb(a1);
	return r;
}

__forceinline vec_short8
vec_vupklsb (vec_char16 a1)
{
	vec_short8 r;
	r.v = __vupklsb(a1);
	return r;
}

/* vec_xor */

__forceinline vec_float4
vec_xor (vec_float4 a1, vec_float4 a2)
{
	vec_float4 r;
	r.v = __vxor(a1, a2);
	return r;
}

__forceinline vec_float4
vec_xor (vec_float4 a1, vec_bool4 a2)
{
	vec_float4 r;
	r.v = __vxor(a1, a2);
	return r;
}

__forceinline vec_float4
vec_xor (vec_bool4 a1, vec_float4 a2)
{
	vec_float4 r;
	r.v = __vxor(a1, a2);
	return r;
}

__forceinline vec_bool4
vec_xor (vec_bool4 a1, vec_bool4 a2)
{
	vec_bool4 r;
	r.v = __vxor(a1, a2);
	return r;
}

__forceinline vec_int4
vec_xor (vec_bool4 a1, vec_int4 a2)
{
	vec_int4 r;
	r.v = __vxor(a1, a2);
	return r;
}

__forceinline vec_int4
vec_xor (vec_int4 a1, vec_bool4 a2)
{
	vec_int4 r;
	r.v = __vxor(a1, a2);
	return r;
}

__forceinline vec_int4
vec_xor (vec_int4 a1, vec_int4 a2)
{
	vec_int4 r;
	r.v = __vxor(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_xor (vec_bool4 a1, vec_uint4 a2)
{
	vec_uint4 r;
	r.v = __vxor(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_xor (vec_uint4 a1, vec_bool4 a2)
{
	vec_uint4 r;
	r.v = __vxor(a1, a2);
	return r;
}

__forceinline vec_uint4
vec_xor (vec_uint4 a1, vec_uint4 a2)
{
	vec_uint4 r;
	r.v = __vxor(a1, a2);
	return r;
}

__forceinline vec_bool8
vec_xor (vec_bool8 a1, vec_bool8 a2)
{
	vec_bool8 r;
	r.v = __vxor(a1, a2);
	return r;
}

__forceinline vec_short8
vec_xor (vec_bool8 a1, vec_short8 a2)
{
	vec_short8 r;
	r.v = __vxor(a1, a2);
	return r;
}

__forceinline vec_short8
vec_xor (vec_short8 a1, vec_bool8 a2)
{
	vec_short8 r;
	r.v = __vxor(a1, a2);
	return r;
}

__forceinline vec_short8
vec_xor (vec_short8 a1, vec_short8 a2)
{
	vec_short8 r;
	r.v = __vxor(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_xor (vec_bool8 a1, vec_ushort8 a2)
{
	vec_ushort8 r;
	r.v = __vxor(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_xor (vec_ushort8 a1, vec_bool8 a2)
{
	vec_ushort8 r;
	r.v = __vxor(a1, a2);
	return r;
}

__forceinline vec_ushort8
vec_xor (vec_ushort8 a1, vec_ushort8 a2)
{
	vec_ushort8 r;
	r.v = __vxor(a1, a2);
	return r;
}

__forceinline vec_char16
vec_xor (vec_bool16 a1, vec_char16 a2)
{
	vec_char16 r;
	r.v = __vxor(a1, a2);
	return r;
}

__forceinline vec_bool16
vec_xor (vec_bool16 a1, vec_bool16 a2)
{
	vec_bool16 r;
	r.v = __vxor(a1, a2);
	return r;
}

__forceinline vec_char16
vec_xor (vec_char16 a1, vec_bool16 a2)
{
	vec_char16 r;
	r.v = __vxor(a1, a2);
	return r;
}

__forceinline vec_char16
vec_xor (vec_char16 a1, vec_char16 a2)
{
	vec_char16 r;
	r.v = __vxor(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_xor (vec_bool16 a1, vec_uchar16 a2)
{
	vec_uchar16 r;
	r.v = __vxor(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_xor (vec_uchar16 a1, vec_bool16 a2)
{
	vec_uchar16 r;
	r.v = __vxor(a1, a2);
	return r;
}

__forceinline vec_uchar16
vec_xor (vec_uchar16 a1, vec_uchar16 a2)
{
	vec_uchar16 r;
	r.v = __vxor(a1, a2);
	return r;
}

/* vec_all_eq */

__forceinline bool
vec_all_eq (vec_char16 a1, vec_bool16 a2)
{
	unsigned int compareResult;
	__vcmpequbR(a1, a2, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_eq (vec_char16 a1, vec_char16 a2)
{
	unsigned int compareResult;
	__vcmpequbR(a1, a2, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_eq (vec_uchar16 a1, vec_bool16 a2)
{
	unsigned int compareResult;
	__vcmpequbR(a1, a2, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_eq (vec_uchar16 a1, vec_uchar16 a2)
{
	unsigned int compareResult;
	__vcmpequbR(a1, a2, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_eq (vec_bool16 a1, vec_bool16 a2)
{
	unsigned int compareResult;
	__vcmpequbR(a1, a2, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_eq (vec_bool16 a1, vec_uchar16 a2)
{
	unsigned int compareResult;
	__vcmpequbR(a1, a2, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_eq (vec_bool16 a1, vec_char16 a2)
{
	unsigned int compareResult;
	__vcmpequbR(a1, a2, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_eq (vec_short8 a1, vec_bool8 a2)
{
	unsigned int compareResult;
	__vcmpequhR(a1, a2, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_eq (vec_short8 a1, vec_short8 a2)
{
	unsigned int compareResult;
	__vcmpequhR(a1, a2, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_eq (vec_ushort8 a1, vec_bool8 a2)
{
	unsigned int compareResult;
	__vcmpequhR(a1, a2, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_eq (vec_ushort8 a1, vec_ushort8 a2)
{
	unsigned int compareResult;
	__vcmpequhR(a1, a2, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_eq (vec_bool8 a1, vec_bool8 a2)
{
	unsigned int compareResult;
	__vcmpequhR(a1, a2, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_eq (vec_bool8 a1, vec_ushort8 a2)
{
	unsigned int compareResult;
	__vcmpequhR(a1, a2, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_eq (vec_bool8 a1, vec_short8 a2)
{
	unsigned int compareResult;
	__vcmpequhR(a1, a2, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_eq (vec_pixel a1, vec_pixel a2)
{
	unsigned int compareResult;
	__vcmpequhR(a1, a2, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_eq (vec_int4 a1, vec_bool4 a2)
{
	unsigned int compareResult;
	__vcmpequwR(a1, a2, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_eq (vec_int4 a1, vec_int4 a2)
{
	unsigned int compareResult;
	__vcmpequwR(a1, a2, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_eq (vec_uint4 a1, vec_bool4 a2)
{
	unsigned int compareResult;
	__vcmpequwR(a1, a2, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_eq (vec_uint4 a1, vec_uint4 a2)
{
	unsigned int compareResult;
	__vcmpequwR(a1, a2, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_eq (vec_bool4 a1, vec_bool4 a2)
{
	unsigned int compareResult;
	__vcmpequwR(a1, a2, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_eq (vec_bool4 a1, vec_uint4 a2)
{
	unsigned int compareResult;
	__vcmpequwR(a1, a2, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_eq (vec_bool4 a1, vec_int4 a2)
{
	unsigned int compareResult;
	__vcmpequwR(a1, a2, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_eq (vec_float4 a1, vec_float4 a2)
{
	unsigned int compareResult;
	__vcmpeqfpR(a1, a2, &compareResult);
	return (compareResult>>7)&1;
}

/* vec_all_ge */

__forceinline bool
vec_all_ge (vec_bool16 a1, vec_uchar16 a2)
{
	unsigned int compareResult;
	__vcmpgtubR(a2, a1, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_ge (vec_uchar16 a1, vec_bool16 a2)
{
	unsigned int compareResult;
	__vcmpgtubR(a2, a1, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_ge (vec_uchar16 a1, vec_uchar16 a2)
{
	unsigned int compareResult;
	__vcmpgtubR(a2, a1, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_ge (vec_bool16 a1, vec_char16 a2)
{
	unsigned int compareResult;
	__vcmpgtsbR(a2, a1, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_ge (vec_char16 a1, vec_bool16 a2)
{
	unsigned int compareResult;
	__vcmpgtsbR(a2, a1, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_ge (vec_char16 a1, vec_char16 a2)
{
	unsigned int compareResult;
	__vcmpgtsbR(a2, a1, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_ge (vec_bool8 a1, vec_ushort8 a2)
{
	unsigned int compareResult;
	__vcmpgtuhR(a2, a1, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_ge (vec_ushort8 a1, vec_bool8 a2)
{
	unsigned int compareResult;
	__vcmpgtuhR(a2, a1, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_ge (vec_ushort8 a1, vec_ushort8 a2)
{
	unsigned int compareResult;
	__vcmpgtuhR(a2, a1, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_ge (vec_short8 a1, vec_short8 a2)
{
	unsigned int compareResult;
	__vcmpgtshR(a2, a1, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_ge (vec_bool8 a1, vec_short8 a2)
{
	unsigned int compareResult;
	__vcmpgtshR(a2, a1, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_ge (vec_short8 a1, vec_bool8 a2)
{
	unsigned int compareResult;
	__vcmpgtshR(a2, a1, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_ge (vec_bool4 a1, vec_uint4 a2)
{
	unsigned int compareResult;
	__vcmpgtuwR(a2, a1, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_ge (vec_uint4 a1, vec_bool4 a2)
{
	unsigned int compareResult;
	__vcmpgtuwR(a2, a1, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_ge (vec_uint4 a1, vec_uint4 a2)
{
	unsigned int compareResult;
	__vcmpgtuwR(a2, a1, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_ge (vec_bool4 a1, vec_int4 a2)
{
	unsigned int compareResult;
	__vcmpgtswR(a2, a1, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_ge (vec_int4 a1, vec_bool4 a2)
{
	unsigned int compareResult;
	__vcmpgtswR(a2, a1, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_ge (vec_int4 a1, vec_int4 a2)
{
	unsigned int compareResult;
	__vcmpgtswR(a2, a1, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_ge (vec_float4 a1, vec_float4 a2)
{
	unsigned int compareResult;
	__vcmpgefpR(a1, a2, &compareResult);
	return (compareResult>>7)&1;
}

/* vec_all_gt */

__forceinline bool
vec_all_gt (vec_bool16 a1, vec_uchar16 a2)
{
	unsigned int compareResult;
	__vcmpgtubR(a1, a2, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_gt (vec_uchar16 a1, vec_bool16 a2)
{
	unsigned int compareResult;
	__vcmpgtubR(a1, a2, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_gt (vec_uchar16 a1, vec_uchar16 a2)
{
	unsigned int compareResult;
	__vcmpgtubR(a1, a2, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_gt (vec_bool16 a1, vec_char16 a2)
{
	unsigned int compareResult;
	__vcmpgtsbR(a1, a2, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_gt (vec_char16 a1, vec_bool16 a2)
{
	unsigned int compareResult;
	__vcmpgtsbR(a1, a2, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_gt (vec_char16 a1, vec_char16 a2)
{
	unsigned int compareResult;
	__vcmpgtsbR(a1, a2, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_gt (vec_bool8 a1, vec_ushort8 a2)
{
	unsigned int compareResult;
	__vcmpgtuhR(a1, a2, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_gt (vec_ushort8 a1, vec_bool8 a2)
{
	unsigned int compareResult;
	__vcmpgtuhR(a1, a2, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_gt (vec_ushort8 a1, vec_ushort8 a2)
{
	unsigned int compareResult;
	__vcmpgtuhR(a1, a2, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_gt (vec_bool8 a1, vec_short8 a2)
{
	unsigned int compareResult;
	__vcmpgtshR(a1, a2, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_gt (vec_short8 a1, vec_bool8 a2)
{
	unsigned int compareResult;
	__vcmpgtshR(a1, a2, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_gt (vec_short8 a1, vec_short8 a2)
{
	unsigned int compareResult;
	__vcmpgtshR(a1, a2, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_gt (vec_bool4 a1, vec_uint4 a2)
{
	unsigned int compareResult;
	__vcmpgtuwR(a1, a2, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_gt (vec_uint4 a1, vec_bool4 a2)
{
	unsigned int compareResult;
	__vcmpgtuwR(a1, a2, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_gt (vec_uint4 a1, vec_uint4 a2)
{
	unsigned int compareResult;
	__vcmpgtuwR(a1, a2, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_gt (vec_bool4 a1, vec_int4 a2)
{
	unsigned int compareResult;
	__vcmpgtswR(a1, a2, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_gt (vec_int4 a1, vec_bool4 a2)
{
	unsigned int compareResult;
	__vcmpgtswR(a1, a2, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_gt (vec_int4 a1, vec_int4 a2)
{
	unsigned int compareResult;
	__vcmpgtswR(a1, a2, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_gt (vec_float4 a1, vec_float4 a2)
{
	unsigned int compareResult;
	__vcmpgtfpR(a1, a2, &compareResult);
	return (compareResult>>7)&1;
}

/* vec_all_in */

__forceinline bool
vec_all_in (vec_float4 a1, vec_float4 a2)
{
	unsigned int compareResult;
	__vcmpbfpR(a1, a2, &compareResult);
	return (compareResult>>5)&1;
}

/* vec_all_le */

__forceinline bool
vec_all_le (vec_bool16 a1, vec_uchar16 a2)
{
	unsigned int compareResult;
	__vcmpgtubR(a1, a2, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_le (vec_uchar16 a1, vec_bool16 a2)
{
	unsigned int compareResult;
	__vcmpgtubR(a1, a2, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_le (vec_uchar16 a1, vec_uchar16 a2)
{
	unsigned int compareResult;
	__vcmpgtubR(a1, a2, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_le (vec_bool16 a1, vec_char16 a2)
{
	unsigned int compareResult;
	__vcmpgtsbR(a1, a2, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_le (vec_char16 a1, vec_bool16 a2)
{
	unsigned int compareResult;
	__vcmpgtsbR(a1, a2, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_le (vec_char16 a1, vec_char16 a2)
{
	unsigned int compareResult;
	__vcmpgtsbR(a1, a2, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_le (vec_bool8 a1, vec_ushort8 a2)
{
	unsigned int compareResult;
	__vcmpgtuhR(a1, a2, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_le (vec_ushort8 a1, vec_bool8 a2)
{
	unsigned int compareResult;
	__vcmpgtuhR(a1, a2, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_le (vec_ushort8 a1, vec_ushort8 a2)
{
	unsigned int compareResult;
	__vcmpgtuhR(a1, a2, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_le (vec_bool8 a1, vec_short8 a2)
{
	unsigned int compareResult;
	__vcmpgtshR(a1, a2, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_le (vec_short8 a1, vec_bool8 a2)
{
	unsigned int compareResult;
	__vcmpgtshR(a1, a2, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_le (vec_short8 a1, vec_short8 a2)
{
	unsigned int compareResult;
	__vcmpgtshR(a1, a2, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_le (vec_bool4 a1, vec_uint4 a2)
{
	unsigned int compareResult;
	__vcmpgtuwR(a1, a2, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_le (vec_uint4 a1, vec_bool4 a2)
{
	unsigned int compareResult;
	__vcmpgtuwR(a1, a2, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_le (vec_uint4 a1, vec_uint4 a2)
{
	unsigned int compareResult;
	__vcmpgtuwR(a1, a2, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_le (vec_bool4 a1, vec_int4 a2)
{
	unsigned int compareResult;
	__vcmpgtswR(a1, a2, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_le (vec_int4 a1, vec_bool4 a2)
{
	unsigned int compareResult;
	__vcmpgtswR(a1, a2, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_le (vec_int4 a1, vec_int4 a2)
{
	unsigned int compareResult;
	__vcmpgtswR(a1, a2, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_le (vec_float4 a1, vec_float4 a2)
{
	unsigned int compareResult;
	__vcmpgefpR(a2, a1, &compareResult);
	return (compareResult>>7)&1;
}

/* vec_all_lt */

__forceinline bool
vec_all_lt (vec_bool16 a1, vec_uchar16 a2)
{
	unsigned int compareResult;
	__vcmpgtubR(a2, a1, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_lt (vec_uchar16 a1, vec_bool16 a2)
{
	unsigned int compareResult;
	__vcmpgtubR(a2, a1, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_lt (vec_uchar16 a1, vec_uchar16 a2)
{
	unsigned int compareResult;
	__vcmpgtubR(a2, a1, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_lt (vec_bool16 a1, vec_char16 a2)
{
	unsigned int compareResult;
	__vcmpgtsbR(a2, a1, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_lt (vec_char16 a1, vec_bool16 a2)
{
	unsigned int compareResult;
	__vcmpgtsbR(a2, a1, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_lt (vec_char16 a1, vec_char16 a2)
{
	unsigned int compareResult;
	__vcmpgtsbR(a2, a1, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_lt (vec_bool8 a1, vec_ushort8 a2)
{
	unsigned int compareResult;
	__vcmpgtuhR(a2, a1, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_lt (vec_ushort8 a1, vec_bool8 a2)
{
	unsigned int compareResult;
	__vcmpgtuhR(a2, a1, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_lt (vec_ushort8 a1, vec_ushort8 a2)
{
	unsigned int compareResult;
	__vcmpgtuhR(a2, a1, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_lt (vec_bool8 a1, vec_short8 a2)
{
	unsigned int compareResult;
	__vcmpgtshR(a2, a1, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_lt (vec_short8 a1, vec_bool8 a2)
{
	unsigned int compareResult;
	__vcmpgtshR(a2, a1, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_lt (vec_short8 a1, vec_short8 a2)
{
	unsigned int compareResult;
	__vcmpgtshR(a2, a1, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_lt (vec_bool4 a1, vec_uint4 a2)
{
	unsigned int compareResult;
	__vcmpgtuwR(a2, a1, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_lt (vec_uint4 a1, vec_bool4 a2)
{
	unsigned int compareResult;
	__vcmpgtuwR(a2, a1, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_lt (vec_uint4 a1, vec_uint4 a2)
{
	unsigned int compareResult;
	__vcmpgtuwR(a2, a1, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_lt (vec_bool4 a1, vec_int4 a2)
{
	unsigned int compareResult;
	__vcmpgtswR(a2, a1, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_lt (vec_int4 a1, vec_bool4 a2)
{
	unsigned int compareResult;
	__vcmpgtswR(a2, a1, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_lt (vec_int4 a1, vec_int4 a2)
{
	unsigned int compareResult;
	__vcmpgtswR(a2, a1, &compareResult);
	return (compareResult>>7)&1;
}

__forceinline bool
vec_all_lt (vec_float4 a1, vec_float4 a2)
{
	unsigned int compareResult;
	__vcmpgtfpR(a2, a1, &compareResult);
	return (compareResult>>7)&1;
}

/* vec_all_nan */

__forceinline bool
vec_all_nan (vec_float4 a1)
{
	unsigned int compareResult;
	__vcmpeqfpR(a1, a1, &compareResult);
	return (compareResult>>5)&1;
}

/* vec_all_ne */

__forceinline bool
vec_all_ne (vec_char16 a1, vec_bool16 a2)
{
	unsigned int compareResult;
	__vcmpequbR(a1, a2, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_ne (vec_char16 a1, vec_char16 a2)
{
	unsigned int compareResult;
	__vcmpequbR(a1, a2, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_ne (vec_uchar16 a1, vec_bool16 a2)
{
	unsigned int compareResult;
	__vcmpequbR(a1, a2, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_ne (vec_uchar16 a1, vec_uchar16 a2)
{
	unsigned int compareResult;
	__vcmpequbR(a1, a2, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_ne (vec_bool16 a1, vec_bool16 a2)
{
	unsigned int compareResult;
	__vcmpequbR(a1, a2, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_ne (vec_bool16 a1, vec_uchar16 a2)
{
	unsigned int compareResult;
	__vcmpequbR(a1, a2, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_ne (vec_bool16 a1, vec_char16 a2)
{
	unsigned int compareResult;
	__vcmpequbR(a1, a2, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_ne (vec_short8 a1, vec_bool8 a2)
{
	unsigned int compareResult;
	__vcmpequhR(a1, a2, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_ne (vec_short8 a1, vec_short8 a2)
{
	unsigned int compareResult;
	__vcmpequhR(a1, a2, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_ne (vec_ushort8 a1, vec_bool8 a2)
{
	unsigned int compareResult;
	__vcmpequhR(a1, a2, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_ne (vec_ushort8 a1, vec_ushort8 a2)
{
	unsigned int compareResult;
	__vcmpequhR(a1, a2, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_ne (vec_bool8 a1, vec_bool8 a2)
{
	unsigned int compareResult;
	__vcmpequhR(a1, a2, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_ne (vec_bool8 a1, vec_ushort8 a2)
{
	unsigned int compareResult;
	__vcmpequhR(a1, a2, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_ne (vec_bool8 a1, vec_short8 a2)
{
	unsigned int compareResult;
	__vcmpequhR(a1, a2, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_ne (vec_pixel a1, vec_pixel a2)
{
	unsigned int compareResult;
	__vcmpequhR(a1, a2, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_ne (vec_int4 a1, vec_bool4 a2)
{
	unsigned int compareResult;
	__vcmpequwR(a1, a2, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_ne (vec_int4 a1, vec_int4 a2)
{
	unsigned int compareResult;
	__vcmpequwR(a1, a2, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_ne (vec_uint4 a1, vec_bool4 a2)
{
	unsigned int compareResult;
	__vcmpequwR(a1, a2, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_ne (vec_uint4 a1, vec_uint4 a2)
{
	unsigned int compareResult;
	__vcmpequwR(a1, a2, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_ne (vec_bool4 a1, vec_bool4 a2)
{
	unsigned int compareResult;
	__vcmpequwR(a1, a2, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_ne (vec_bool4 a1, vec_uint4 a2)
{
	unsigned int compareResult;
	__vcmpequwR(a1, a2, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_ne (vec_bool4 a1, vec_int4 a2)
{
	unsigned int compareResult;
	__vcmpequwR(a1, a2, &compareResult);
	return (compareResult>>5)&1;
}

__forceinline bool
vec_all_ne (vec_float4 a1, vec_float4 a2)
{
	unsigned int compareResult;
	__vcmpeqfpR(a1, a2, &compareResult);
	return (compareResult>>5)&1;
}

/* vec_all_nge */

__forceinline bool
vec_all_nge (vec_float4 a1, vec_float4 a2)
{
	unsigned int compareResult;
	__vcmpgefpR(a1, a2, &compareResult);
	return (compareResult>>5)&1;
}

/* vec_all_ngt */

__forceinline bool
vec_all_ngt (vec_float4 a1, vec_float4 a2)
{
	unsigned int compareResult;
	__vcmpgtfpR(a1, a2, &compareResult);
	return (compareResult>>5)&1;
}

/* vec_all_nle */

__forceinline bool
vec_all_nle (vec_float4 a1, vec_float4 a2)
{
	unsigned int compareResult;
	__vcmpgefpR(a2, a1, &compareResult);
	return (compareResult>>5)&1;
}

/* vec_all_nlt */

__forceinline bool
vec_all_nlt (vec_float4 a1, vec_float4 a2)
{
	unsigned int compareResult;
	__vcmpgtfpR(a2, a1, &compareResult);
	return (compareResult>>5)&1;
}

/* vec_all_numeric */

__forceinline bool
vec_all_numeric (vec_float4 a1)
{
	unsigned int compareResult;
	__vcmpeqfpR(a1, a1, &compareResult);
	return (compareResult>>7)&1;
}

/* vec_any_eq */

__forceinline bool
vec_any_eq (vec_char16 a1, vec_bool16 a2)
{
	unsigned int compareResult;
	__vcmpequbR(a1, a2, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_eq (vec_char16 a1, vec_char16 a2)
{
	unsigned int compareResult;
	__vcmpequbR(a1, a2, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_eq (vec_uchar16 a1, vec_bool16 a2)
{
	unsigned int compareResult;
	__vcmpequbR(a1, a2, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_eq (vec_uchar16 a1, vec_uchar16 a2)
{
	unsigned int compareResult;
	__vcmpequbR(a1, a2, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_eq (vec_bool16 a1, vec_bool16 a2)
{
	unsigned int compareResult;
	__vcmpequbR(a1, a2, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_eq (vec_bool16 a1, vec_uchar16 a2)
{
	unsigned int compareResult;
	__vcmpequbR(a1, a2, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_eq (vec_bool16 a1, vec_char16 a2)
{
	unsigned int compareResult;
	__vcmpequbR(a1, a2, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_eq (vec_short8 a1, vec_bool8 a2)
{
	unsigned int compareResult;
	__vcmpequhR(a1, a2, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_eq (vec_short8 a1, vec_short8 a2)
{
	unsigned int compareResult;
	__vcmpequhR(a1, a2, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_eq (vec_ushort8 a1, vec_bool8 a2)
{
	unsigned int compareResult;
	__vcmpequhR(a1, a2, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_eq (vec_ushort8 a1, vec_ushort8 a2)
{
	unsigned int compareResult;
	__vcmpequhR(a1, a2, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_eq (vec_bool8 a1, vec_bool8 a2)
{
	unsigned int compareResult;
	__vcmpequhR(a1, a2, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_eq (vec_bool8 a1, vec_ushort8 a2)
{
	unsigned int compareResult;
	__vcmpequhR(a1, a2, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_eq (vec_bool8 a1, vec_short8 a2)
{
	unsigned int compareResult;
	__vcmpequhR(a1, a2, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_eq (vec_pixel a1, vec_pixel a2)
{
	unsigned int compareResult;
	__vcmpequhR(a1, a2, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_eq (vec_int4 a1, vec_bool4 a2)
{
	unsigned int compareResult;
	__vcmpequwR(a1, a2, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_eq (vec_int4 a1, vec_int4 a2)
{
	unsigned int compareResult;
	__vcmpequwR(a1, a2, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_eq (vec_uint4 a1, vec_bool4 a2)
{
	unsigned int compareResult;
	__vcmpequwR(a1, a2, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_eq (vec_uint4 a1, vec_uint4 a2)
{
	unsigned int compareResult;
	__vcmpequwR(a1, a2, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_eq (vec_bool4 a1, vec_bool4 a2)
{
	unsigned int compareResult;
	__vcmpequwR(a1, a2, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_eq (vec_bool4 a1, vec_uint4 a2)
{
	unsigned int compareResult;
	__vcmpequwR(a1, a2, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_eq (vec_bool4 a1, vec_int4 a2)
{
	unsigned int compareResult;
	__vcmpequwR(a1, a2, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_eq (vec_float4 a1, vec_float4 a2)
{
	unsigned int compareResult;
	__vcmpeqfpR(a1, a2, &compareResult);
	return (~compareResult>>5)&1;
}

/* vec_any_ge */

__forceinline bool
vec_any_ge (vec_char16 a1, vec_bool16 a2)
{
	unsigned int compareResult;
	__vcmpgtubR(a2, a1, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_ge (vec_uchar16 a1, vec_bool16 a2)
{
	unsigned int compareResult;
	__vcmpgtubR(a2, a1, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_ge (vec_uchar16 a1, vec_uchar16 a2)
{
	unsigned int compareResult;
	__vcmpgtubR(a2, a1, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_ge (vec_char16 a1, vec_char16 a2)
{
	unsigned int compareResult;
	__vcmpgtsbR(a2, a1, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_ge (vec_bool16 a1, vec_uchar16 a2)
{
	unsigned int compareResult;
	__vcmpgtubR(a2, a1, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_ge (vec_bool16 a1, vec_char16 a2)
{
	unsigned int compareResult;
	__vcmpgtubR(a2, a1, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_ge (vec_ushort8 a1, vec_bool8 a2)
{
	unsigned int compareResult;
	__vcmpgtuhR(a2, a1, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_ge (vec_ushort8 a1, vec_ushort8 a2)
{
	unsigned int compareResult;
	__vcmpgtuhR(a2, a1, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_ge (vec_short8 a1, vec_short8 a2)
{
	unsigned int compareResult;
	__vcmpgtshR(a2, a1, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_ge (vec_short8 a1, vec_bool8 a2)
{
	unsigned int compareResult;
	__vcmpgtshR(a2, a1, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_ge (vec_bool8 a1, vec_ushort8 a2)
{
	unsigned int compareResult;
	__vcmpgtuhR(a2, a1, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_ge (vec_bool8 a1, vec_short8 a2)
{
	unsigned int compareResult;
	__vcmpgtuhR(a2, a1, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_ge (vec_int4 a1, vec_bool4 a2)
{
	unsigned int compareResult;
	__vcmpgtuwR(a2, a1, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_ge (vec_uint4 a1, vec_bool4 a2)
{
	unsigned int compareResult;
	__vcmpgtuwR(a2, a1, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_ge (vec_uint4 a1, vec_uint4 a2)
{
	unsigned int compareResult;
	__vcmpgtuwR(a2, a1, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_ge (vec_int4 a1, vec_int4 a2)
{
	unsigned int compareResult;
	__vcmpgtswR(a2, a1, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_ge (vec_bool4 a1, vec_uint4 a2)
{
	unsigned int compareResult;
	__vcmpgtuwR(a2, a1, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_ge (vec_bool4 a1, vec_int4 a2)
{
	unsigned int compareResult;
	__vcmpgtuwR(a2, a1, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_ge (vec_float4 a1, vec_float4 a2)
{
	unsigned int compareResult;
	__vcmpgefpR(a1, a2, &compareResult);
	return (~compareResult>>5)&1;
}

/* vec_any_gt */

__forceinline bool
vec_any_gt (vec_bool16 a1, vec_uchar16 a2)
{
	unsigned int compareResult;
	__vcmpgtubR(a1, a2, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_gt (vec_uchar16 a1, vec_bool16 a2)
{
	unsigned int compareResult;
	__vcmpgtubR(a1, a2, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_gt (vec_uchar16 a1, vec_uchar16 a2)
{
	unsigned int compareResult;
	__vcmpgtubR(a1, a2, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_gt (vec_bool16 a1, vec_char16 a2)
{
	unsigned int compareResult;
	__vcmpgtsbR(a1, a2, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_gt (vec_char16 a1, vec_bool16 a2)
{
	unsigned int compareResult;
	__vcmpgtsbR(a1, a2, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_gt (vec_char16 a1, vec_char16 a2)
{
	unsigned int compareResult;
	__vcmpgtsbR(a1, a2, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_gt (vec_bool8 a1, vec_ushort8 a2)
{
	unsigned int compareResult;
	__vcmpgtuhR(a1, a2, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_gt (vec_ushort8 a1, vec_bool8 a2)
{
	unsigned int compareResult;
	__vcmpgtuhR(a1, a2, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_gt (vec_ushort8 a1, vec_ushort8 a2)
{
	unsigned int compareResult;
	__vcmpgtuhR(a1, a2, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_gt (vec_bool8 a1, vec_short8 a2)
{
	unsigned int compareResult;
	__vcmpgtshR(a1, a2, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_gt (vec_short8 a1, vec_bool8 a2)
{
	unsigned int compareResult;
	__vcmpgtshR(a1, a2, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_gt (vec_short8 a1, vec_short8 a2)
{
	unsigned int compareResult;
	__vcmpgtshR(a1, a2, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_gt (vec_bool4 a1, vec_uint4 a2)
{
	unsigned int compareResult;
	__vcmpgtuwR(a1, a2, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_gt (vec_uint4 a1, vec_bool4 a2)
{
	unsigned int compareResult;
	__vcmpgtuwR(a1, a2, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_gt (vec_uint4 a1, vec_uint4 a2)
{
	unsigned int compareResult;
	__vcmpgtuwR(a1, a2, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_gt (vec_bool4 a1, vec_int4 a2)
{
	unsigned int compareResult;
	__vcmpgtswR(a1, a2, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_gt (vec_int4 a1, vec_bool4 a2)
{
	unsigned int compareResult;
	__vcmpgtswR(a1, a2, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_gt (vec_int4 a1, vec_int4 a2)
{
	unsigned int compareResult;
	__vcmpgtswR(a1, a2, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_gt (vec_float4 a1, vec_float4 a2)
{
	unsigned int compareResult;
	__vcmpgtfpR(a1, a2, &compareResult);
	return (~compareResult>>5)&1;
}

/* vec_any_le */

__forceinline bool
vec_any_le (vec_bool16 a1, vec_uchar16 a2)
{
	unsigned int compareResult;
	__vcmpgtubR(a1, a2, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_le (vec_uchar16 a1, vec_bool16 a2)
{
	unsigned int compareResult;
	__vcmpgtubR(a1, a2, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_le (vec_uchar16 a1, vec_uchar16 a2)
{
	unsigned int compareResult;
	__vcmpgtubR(a1, a2, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_le (vec_bool16 a1, vec_char16 a2)
{
	unsigned int compareResult;
	__vcmpgtsbR(a1, a2, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_le (vec_char16 a1, vec_bool16 a2)
{
	unsigned int compareResult;
	__vcmpgtsbR(a1, a2, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_le (vec_char16 a1, vec_char16 a2)
{
	unsigned int compareResult;
	__vcmpgtsbR(a1, a2, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_le (vec_bool8 a1, vec_ushort8 a2)
{
	unsigned int compareResult;
	__vcmpgtuhR(a1, a2, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_le (vec_ushort8 a1, vec_bool8 a2)
{
	unsigned int compareResult;
	__vcmpgtuhR(a1, a2, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_le (vec_ushort8 a1, vec_ushort8 a2)
{
	unsigned int compareResult;
	__vcmpgtuhR(a1, a2, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_le (vec_bool8 a1, vec_short8 a2)
{
	unsigned int compareResult;
	__vcmpgtshR(a1, a2, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_le (vec_short8 a1, vec_bool8 a2)
{
	unsigned int compareResult;
	__vcmpgtshR(a1, a2, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_le (vec_short8 a1, vec_short8 a2)
{
	unsigned int compareResult;
	__vcmpgtshR(a1, a2, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_le (vec_bool4 a1, vec_uint4 a2)
{
	unsigned int compareResult;
	__vcmpgtuwR(a1, a2, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_le (vec_uint4 a1, vec_bool4 a2)
{
	unsigned int compareResult;
	__vcmpgtuwR(a1, a2, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_le (vec_uint4 a1, vec_uint4 a2)
{
	unsigned int compareResult;
	__vcmpgtuwR(a1, a2, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_le (vec_bool4 a1, vec_int4 a2)
{
	unsigned int compareResult;
	__vcmpgtswR(a1, a2, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_le (vec_int4 a1, vec_bool4 a2)
{
	unsigned int compareResult;
	__vcmpgtswR(a1, a2, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_le (vec_int4 a1, vec_int4 a2)
{
	unsigned int compareResult;
	__vcmpgtswR(a1, a2, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_le (vec_float4 a1, vec_float4 a2)
{
	unsigned int compareResult;
	__vcmpgefpR(a2, a1, &compareResult);
	return (~compareResult>>5)&1;
}

/* vec_any_lt */

__forceinline bool
vec_any_lt (vec_bool16 a1, vec_uchar16 a2)
{
	unsigned int compareResult;
	__vcmpgtubR(a2, a1, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_lt (vec_uchar16 a1, vec_bool16 a2)
{
	unsigned int compareResult;
	__vcmpgtubR(a2, a1, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_lt (vec_uchar16 a1, vec_uchar16 a2)
{
	unsigned int compareResult;
	__vcmpgtubR(a2, a1, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_lt (vec_bool16 a1, vec_char16 a2)
{
	unsigned int compareResult;
	__vcmpgtsbR(a2, a1, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_lt (vec_char16 a1, vec_bool16 a2)
{
	unsigned int compareResult;
	__vcmpgtsbR(a2, a1, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_lt (vec_char16 a1, vec_char16 a2)
{
	unsigned int compareResult;
	__vcmpgtsbR(a2, a1, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_lt (vec_bool8 a1, vec_ushort8 a2)
{
	unsigned int compareResult;
	__vcmpgtuhR(a2, a1, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_lt (vec_ushort8 a1, vec_bool8 a2)
{
	unsigned int compareResult;
	__vcmpgtuhR(a2, a1, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_lt (vec_ushort8 a1, vec_ushort8 a2)
{
	unsigned int compareResult;
	__vcmpgtuhR(a2, a1, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_lt (vec_bool8 a1, vec_short8 a2)
{
	unsigned int compareResult;
	__vcmpgtshR(a2, a1, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_lt (vec_short8 a1, vec_bool8 a2)
{
	unsigned int compareResult;
	__vcmpgtshR(a2, a1, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_lt (vec_short8 a1, vec_short8 a2)
{
	unsigned int compareResult;
	__vcmpgtshR(a2, a1, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_lt (vec_bool4 a1, vec_uint4 a2)
{
	unsigned int compareResult;
	__vcmpgtuwR(a2, a1, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_lt (vec_uint4 a1, vec_bool4 a2)
{
	unsigned int compareResult;
	__vcmpgtuwR(a2, a1, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_lt (vec_uint4 a1, vec_uint4 a2)
{
	unsigned int compareResult;
	__vcmpgtuwR(a2, a1, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_lt (vec_bool4 a1, vec_int4 a2)
{
	unsigned int compareResult;
	__vcmpgtswR(a2, a1, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_lt (vec_int4 a1, vec_bool4 a2)
{
	unsigned int compareResult;
	__vcmpgtswR(a2, a1, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_lt (vec_int4 a1, vec_int4 a2)
{
	unsigned int compareResult;
	__vcmpgtswR(a2, a1, &compareResult);
	return (~compareResult>>5)&1;
}

__forceinline bool
vec_any_lt (vec_float4 a1, vec_float4 a2)
{
	unsigned int compareResult;
	__vcmpgtfpR(a2, a1, &compareResult);
	return (~compareResult>>5)&1;
}

/* vec_any_nan */

__forceinline bool
vec_any_nan (vec_float4 a1)
{
	unsigned int compareResult;
	__vcmpeqfpR(a1, a1, &compareResult);
	return (~compareResult>>7)&1;
}

/* vec_any_ne */

__forceinline bool
vec_any_ne (vec_char16 a1, vec_bool16 a2)
{
	unsigned int compareResult;
	__vcmpequbR(a1, a2, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_ne (vec_char16 a1, vec_char16 a2)
{
	unsigned int compareResult;
	__vcmpequbR(a1, a2, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_ne (vec_uchar16 a1, vec_bool16 a2)
{
	unsigned int compareResult;
	__vcmpequbR(a1, a2, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_ne (vec_uchar16 a1, vec_uchar16 a2)
{
	unsigned int compareResult;
	__vcmpequbR(a1, a2, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_ne (vec_bool16 a1, vec_bool16 a2)
{
	unsigned int compareResult;
	__vcmpequbR(a1, a2, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_ne (vec_bool16 a1, vec_uchar16 a2)
{
	unsigned int compareResult;
	__vcmpequbR(a1, a2, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_ne (vec_bool16 a1, vec_char16 a2)
{
	unsigned int compareResult;
	__vcmpequbR(a1, a2, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_ne (vec_short8 a1, vec_bool8 a2)
{
	unsigned int compareResult;
	__vcmpequhR(a1, a2, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_ne (vec_short8 a1, vec_short8 a2)
{
	unsigned int compareResult;
	__vcmpequhR(a1, a2, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_ne (vec_ushort8 a1, vec_bool8 a2)
{
	unsigned int compareResult;
	__vcmpequhR(a1, a2, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_ne (vec_ushort8 a1, vec_ushort8 a2)
{
	unsigned int compareResult;
	__vcmpequhR(a1, a2, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_ne (vec_bool8 a1, vec_bool8 a2)
{
	unsigned int compareResult;
	__vcmpequhR(a1, a2, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_ne (vec_bool8 a1, vec_ushort8 a2)
{
	unsigned int compareResult;
	__vcmpequhR(a1, a2, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_ne (vec_bool8 a1, vec_short8 a2)
{
	unsigned int compareResult;
	__vcmpequhR(a1, a2, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_ne (vec_pixel a1, vec_pixel a2)
{
	unsigned int compareResult;
	__vcmpequhR(a1, a2, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_ne (vec_int4 a1, vec_bool4 a2)
{
	unsigned int compareResult;
	__vcmpequwR(a1, a2, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_ne (vec_int4 a1, vec_int4 a2)
{
	unsigned int compareResult;
	__vcmpequwR(a1, a2, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_ne (vec_uint4 a1, vec_bool4 a2)
{
	unsigned int compareResult;
	__vcmpequwR(a1, a2, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_ne (vec_uint4 a1, vec_uint4 a2)
{
	unsigned int compareResult;
	__vcmpequwR(a1, a2, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_ne (vec_bool4 a1, vec_bool4 a2)
{
	unsigned int compareResult;
	__vcmpequwR(a1, a2, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_ne (vec_bool4 a1, vec_uint4 a2)
{
	unsigned int compareResult;
	__vcmpequwR(a1, a2, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_ne (vec_bool4 a1, vec_int4 a2)
{
	unsigned int compareResult;
	__vcmpequwR(a1, a2, &compareResult);
	return (~compareResult>>7)&1;
}

__forceinline bool
vec_any_ne (vec_float4 a1, vec_float4 a2)
{
	unsigned int compareResult;
	__vcmpeqfpR(a1, a2, &compareResult);
	return (~compareResult>>7)&1;
}

/* vec_any_nge */

__forceinline bool
vec_any_nge (vec_float4 a1, vec_float4 a2)
{
	unsigned int compareResult;
	__vcmpgefpR(a1, a2, &compareResult);
	return (~compareResult>>7)&1;
}

/* vec_any_ngt */

__forceinline bool
vec_any_ngt (vec_float4 a1, vec_float4 a2)
{
	unsigned int compareResult;
	__vcmpgtfpR(a1, a2, &compareResult);
	return (~compareResult>>7)&1;
}

/* vec_any_nle */

__forceinline bool
vec_any_nle (vec_float4 a1, vec_float4 a2)
{
	unsigned int compareResult;
	__vcmpgefpR(a2, a1, &compareResult);
	return (~compareResult>>7)&1;
}

/* vec_any_nlt */

__forceinline bool
vec_any_nlt (vec_float4 a1, vec_float4 a2)
{
	unsigned int compareResult;
	__vcmpgtfpR(a2, a1, &compareResult);
	return (~compareResult>>7)&1;
}

/* vec_any_numeric */

__forceinline bool
vec_any_numeric (vec_float4 a1)
{
	unsigned int compareResult;
	__vcmpeqfpR(a1, a1, &compareResult);
	return (~compareResult>>5)&1;
}

/* vec_any_out */

__forceinline bool
vec_any_out (vec_float4 a1, vec_float4 a2)
{
	unsigned int compareResult;
	__vcmpbfpR(a1, a2, &compareResult);
	return (~compareResult>>5)&1;
}

//////////////////////////////////////////////////////////////////////////
// xenon support for extra ops

__forceinline vec_float4 
vec_dot3 (vec_float4 a1, vec_float4 a2)
{
	vec_float4 r;
	r.v = __vdot3fp(a1, a2);
	return r;
}

__forceinline vec_float4 
vec_dot4 (vec_float4 a1, vec_float4 a2)
{
	vec_float4 r;
	r.v = __vdot4fp(a1, a2);
	return r;
}

//////////////////////////////////////////////////////////////////////////
// xenon support for missing ops

/* vec_abs */

__forceinline vec_char16
vec_abs (vec_char16 a1)
{
	return vec_max(a1, vec_sub(vec_splat_s8(0), a1));
}

__forceinline vec_short8
vec_abs (vec_short8 a1)
{
	return vec_max(a1, vec_sub(vec_splat_s16(0), a1));
}

__forceinline vec_int4
vec_abs (vec_int4 a1)
{
	return vec_max(a1, vec_sub(vec_splat_s32(0), a1));
}

// __forceinline vec_float4
// vec_abs (vec_float4 a1)
// {
// 	return vec_max(a1, vec_sub((vec_float4)vec_splat_s32(0), a1));
// }

/* vec_abss */

// __forceinline vec_char16
// vec_abss (vec_char16 a1)
// {
//   return __abss_v16qi (a1);
// }
// 
// __forceinline vec_short8
// vec_abss (vec_short8 a1)
// {
//   return __abss_v8hi (a1);
// }
// 
// __forceinline vec_int4
// vec_abss (vec_int4 a1)
// {
//   return __abss_v4si (a1);
// }

/* vec_dss */

// __forceinline void
// vec_dss (const int a1)
// {
//   __dss (a1);
// }

/* vec_dssall */

// __forceinline void
// vec_dssall (void)
// {
//   __dssall ();
// }

/* vec_dst */

// __forceinline void
// vec_dst (const vec_uchar16 *a1, OFFSET_T a2, const int a3)
// {
//   __dst ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dst (const vec_char16 *a1, OFFSET_T a2, const int a3)
// {
//   __dst ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dst (const vec_bool16 *a1, OFFSET_T a2, const int a3)
// {
//   __dst ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dst (const vec_ushort8 *a1, OFFSET_T a2, const int a3)
// {
//   __dst ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dst (const vec_short8 *a1, OFFSET_T a2, const int a3)
// {
//   __dst ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dst (const vec_bool8 *a1, OFFSET_T a2, const int a3)
// {
//   __dst ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dst (const vec_pixel *a1, OFFSET_T a2, const int a3)
// {
//   __dst ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dst (const vec_uint4 *a1, OFFSET_T a2, const int a3)
// {
//   __dst ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dst (const vec_int4 *a1, OFFSET_T a2, const int a3)
// {
//   __dst ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dst (const vec_bool4 *a1, OFFSET_T a2, const int a3)
// {
//   __dst ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dst (const vec_float4 *a1, OFFSET_T a2, const int a3)
// {
//   __dst ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dst (const unsigned char *a1, OFFSET_T a2, const int a3)
// {
//   __dst ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dst (const signed char *a1, OFFSET_T a2, const int a3)
// {
//   __dst ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dst (const unsigned short *a1, OFFSET_T a2, const int a3)
// {
//   __dst ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dst (const short *a1, OFFSET_T a2, const int a3)
// {
//   __dst ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dst (const unsigned int *a1, OFFSET_T a2, const int a3)
// {
//   __dst ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dst (const int *a1, OFFSET_T a2, const int a3)
// {
//   __dst ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dst (const unsigned long *a1, OFFSET_T a2, const int a3)
// {
//   __dst ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dst (const long *a1, OFFSET_T a2, const int a3)
// {
//   __dst ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dst (const float *a1, OFFSET_T a2, const int a3)
// {
//   __dst ((void *) a1, a2, a3);
// }

/* vec_dstst */

// __forceinline void
// vec_dstst (const vec_uchar16 *a1, OFFSET_T a2, const int a3)
// {
//   __dstst ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dstst (const vec_char16 *a1, OFFSET_T a2, const int a3)
// {
//   __dstst ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dstst (const vec_bool16 *a1, OFFSET_T a2, const int a3)
// {
//   __dstst ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dstst (const vec_ushort8 *a1, OFFSET_T a2, const int a3)
// {
//   __dstst ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dstst (const vec_short8 *a1, OFFSET_T a2, const int a3)
// {
//   __dstst ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dstst (const vec_bool8 *a1, OFFSET_T a2, const int a3)
// {
//   __dstst ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dstst (const vec_pixel *a1, OFFSET_T a2, const int a3)
// {
//   __dstst ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dstst (const vec_uint4 *a1, OFFSET_T a2, const int a3)
// {
//   __dstst ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dstst (const vec_int4 *a1, OFFSET_T a2, const int a3)
// {
//   __dstst ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dstst (const vec_bool4 *a1, OFFSET_T a2, const int a3)
// {
//   __dstst ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dstst (const vec_float4 *a1, OFFSET_T a2, const int a3)
// {
//   __dstst ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dstst (const unsigned char *a1, OFFSET_T a2, const int a3)
// {
//   __dstst ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dstst (const signed char *a1, OFFSET_T a2, const int a3)
// {
//   __dstst ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dstst (const unsigned short *a1, OFFSET_T a2, const int a3)
// {
//   __dstst ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dstst (const short *a1, OFFSET_T a2, const int a3)
// {
//   __dstst ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dstst (const unsigned int *a1, OFFSET_T a2, const int a3)
// {
//   __dstst ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dstst (const int *a1, OFFSET_T a2, const int a3)
// {
//   __dstst ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dstst (const unsigned long *a1, OFFSET_T a2, const int a3)
// {
//   __dstst ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dstst (const long *a1, OFFSET_T a2, const int a3)
// {
//   __dstst ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dstst (const float *a1, OFFSET_T a2, const int a3)
// {
//   __dstst ((void *) a1, a2, a3);
// }
// 
// /* vec_dststt */
// 
// __forceinline void
// vec_dststt (const vec_uchar16 *a1, OFFSET_T a2, const int a3)
// {
//   __dststt ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dststt (const vec_char16 *a1, OFFSET_T a2, const int a3)
// {
//   __dststt ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dststt (const vec_bool16 *a1, OFFSET_T a2, const int a3)
// {
//   __dststt ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dststt (const vec_ushort8 *a1, OFFSET_T a2, const int a3)
// {
//   __dststt ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dststt (const vec_short8 *a1, OFFSET_T a2, const int a3)
// {
//   __dststt ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dststt (const vec_bool8 *a1, OFFSET_T a2, const int a3)
// {
//   __dststt ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dststt (const vec_pixel *a1, OFFSET_T a2, const int a3)
// {
//   __dststt ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dststt (const vec_uint4 *a1, OFFSET_T a2, const int a3)
// {
//   __dststt ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dststt (const vec_int4 *a1, OFFSET_T a2, const int a3)
// {
//   __dststt ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dststt (const vec_bool4 *a1, OFFSET_T a2, const int a3)
// {
//   __dststt ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dststt (const vec_float4 *a1, OFFSET_T a2, const int a3)
// {
//   __dststt ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dststt (const unsigned char *a1, OFFSET_T a2, const int a3)
// {
//   __dststt ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dststt (const signed char *a1, OFFSET_T a2, const int a3)
// {
//   __dststt ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dststt (const unsigned short *a1, OFFSET_T a2, const int a3)
// {
//   __dststt ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dststt (const short *a1, OFFSET_T a2, const int a3)
// {
//   __dststt ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dststt (const unsigned int *a1, OFFSET_T a2, const int a3)
// {
//   __dststt ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dststt (const int *a1, OFFSET_T a2, const int a3)
// {
//   __dststt ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dststt (const unsigned long *a1, OFFSET_T a2, const int a3)
// {
//   __dststt ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dststt (const long *a1, OFFSET_T a2, const int a3)
// {
//   __dststt ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dststt (const float *a1, OFFSET_T a2, const int a3)
// {
//   __dststt ((void *) a1, a2, a3);
// }
// 
// /* vec_dstt */
// 
// __forceinline void
// vec_dstt (const vec_uchar16 *a1, OFFSET_T a2, const int a3)
// {
//   __dstt ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dstt (const vec_char16 *a1, OFFSET_T a2, const int a3)
// {
//   __dstt ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dstt (const vec_bool16 *a1, OFFSET_T a2, const int a3)
// {
//   __dstt ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dstt (const vec_ushort8 *a1, OFFSET_T a2, const int a3)
// {
//   __dstt ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dstt (const vec_short8 *a1, OFFSET_T a2, const int a3)
// {
//   __dstt ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dstt (const vec_bool8 *a1, OFFSET_T a2, const int a3)
// {
//   __dstt ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dstt (const vec_pixel *a1, OFFSET_T a2, const int a3)
// {
//   __dstt ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dstt (const vec_uint4 *a1, OFFSET_T a2, const int a3)
// {
//   __dstt ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dstt (const vec_int4 *a1, OFFSET_T a2, const int a3)
// {
//   __dstt ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dstt (const vec_bool4 *a1, OFFSET_T a2, const int a3)
// {
//   __dstt ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dstt (const vec_float4 *a1, OFFSET_T a2, const int a3)
// {
//   __dstt ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dstt (const unsigned char *a1, OFFSET_T a2, const int a3)
// {
//   __dstt ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dstt (const signed char *a1, OFFSET_T a2, const int a3)
// {
//   __dstt ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dstt (const unsigned short *a1, OFFSET_T a2, const int a3)
// {
//   __dstt ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dstt (const short *a1, OFFSET_T a2, const int a3)
// {
//   __dstt ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dstt (const unsigned int *a1, OFFSET_T a2, const int a3)
// {
//   __dstt ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dstt (const int *a1, OFFSET_T a2, const int a3)
// {
//   __dstt ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dstt (const unsigned long *a1, OFFSET_T a2, const int a3)
// {
//   __dstt ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dstt (const long *a1, OFFSET_T a2, const int a3)
// {
//   __dstt ((void *) a1, a2, a3);
// }
// 
// __forceinline void
// vec_dstt (const float *a1, OFFSET_T a2, const int a3)
// {
//   __dstt ((void *) a1, a2, a3);
// }

/* vec_madds */

// __forceinline vec_short8
// vec_madds (vec_short8 a1, vec_short8 a2, vec_short8 a3)
// {
// 	vec_short8 r;
// 	r.v = __vmhaddshs(a1, a2, a3);
// 	return r;
// }

/* vec_mladd */

// __forceinline vec_short8
// vec_mladd (vec_short8 a1, vec_short8 a2, vec_short8 a3)
// {
// 	vec_short8 r;
// 	r.v = __vmladduhm(a1, a2, a3);
// 	return r;
// }
// 
// __forceinline vec_short8
// vec_mladd (vec_short8 a1, vec_ushort8 a2, vec_ushort8 a3)
// {
// 	vec_short8 r;
// 	r.v = __vmladduhm(a1, a2, a3);
// 	return r;
// }
// 
// __forceinline vec_short8
// vec_mladd (vec_ushort8 a1, vec_short8 a2, vec_short8 a3)
// {
// 	vec_short8 r;
// 	r.v = __vmladduhm(a1, a2, a3);
// 	return r;
// }
// 
// __forceinline vec_ushort8
// vec_mladd (vec_ushort8 a1, vec_ushort8 a2, vec_ushort8 a3)
// {
// 	vec_ushort8 r;
// 	r.v = __vmladduhm(a1, a2, a3);
// 	return r;
// }
// 
// /* vec_mradds */
// 
// __forceinline vec_short8
// vec_mradds (vec_short8 a1, vec_short8 a2, vec_short8 a3)
// {
// 	vec_short8 r;
// 	r.v = __vmhraddshs(a1, a2, a3);
// 	return r;
// }
// 
// /* vec_msum */
// 
// __forceinline vec_uint4
// vec_msum (vec_uchar16 a1, vec_uchar16 a2, vec_uint4 a3)
// {
// 	vec_uint4 r;
// 	r.v = __vmsumubm(a1, a2, a3);
// 	return r;
// }
// 
// __forceinline vec_int4
// vec_msum (vec_char16 a1, vec_uchar16 a2, vec_int4 a3)
// {
// 	vec_int4 r;
// 	r.v = __vmsummbm(a1, a2, a3);
// 	return r;
// }
// 
// __forceinline vec_uint4
// vec_msum (vec_ushort8 a1, vec_ushort8 a2, vec_uint4 a3)
// {
// 	vec_uint4 r;
// 	r.v = __vmsumuhm(a1, a2, a3);
// 	return r;
// }
// 
// __forceinline vec_int4
// vec_msum (vec_short8 a1, vec_short8 a2, vec_int4 a3)
// {
// 	vec_int4 r;
// 	r.v = __vmsumshm(a1, a2, a3);
// 	return r;
// }
// 
// /* vec_vmsumshm */
// 
// __forceinline vec_int4
// vec_vmsumshm (vec_short8 a1, vec_short8 a2, vec_int4 a3)
// {
// 	vec_int4 r;
// 	r.v = __vmsumshm(a1, a2, a3);
// 	return r;
// }
// 
// /* vec_vmsumuhm */
// 
// __forceinline vec_uint4
// vec_vmsumuhm (vec_ushort8 a1, vec_ushort8 a2, vec_uint4 a3)
// {
// 	vec_uint4 r;
// 	r.v = __vmsumuhm(a1, a2, a3);
// 	return r;
// }
// 
// /* vec_vmsummbm */
// 
// __forceinline vec_int4
// vec_vmsummbm (vec_char16 a1, vec_uchar16 a2, vec_int4 a3)
// {
// 	vec_int4 r;
// 	r.v = __vmsummbm(a1, a2, a3);
// 	return r;
// }
// 
// /* vec_vmsumubm */
// 
// __forceinline vec_uint4
// vec_vmsumubm (vec_uchar16 a1, vec_uchar16 a2, vec_uint4 a3)
// {
// 	vec_uint4 r;
// 	r.v = __vmsumubm(a1, a2, a3);
// 	return r;
// }
// 
// /* vec_msums */
// 
// __forceinline vec_uint4
// vec_msums (vec_ushort8 a1, vec_ushort8 a2, vec_uint4 a3)
// {
// 	vec_uint4 r;
// 	r.v = __vmsumuhs(a1, a2, a3);
// 	return r;
// }
// 
// __forceinline vec_int4
// vec_msums (vec_short8 a1, vec_short8 a2, vec_int4 a3)
// {
// 	vec_int4 r;
// 	r.v = __vmsumshs(a1, a2, a3);
// 	return r;
// }
// 
// /* vec_vmsumshs */
// 
// __forceinline vec_int4
// vec_vmsumshs (vec_short8 a1, vec_short8 a2, vec_int4 a3)
// {
// 	vec_int4 r;
// 	r.v = __vmsumshs(a1, a2, a3);
// 	return r;
// }
// 
// /* vec_vmsumuhs */
// 
// __forceinline vec_uint4
// vec_vmsumuhs (vec_ushort8 a1, vec_ushort8 a2, vec_uint4 a3)
// {
// 	vec_uint4 r;
// 	r.v = __vmsumuhs(a1, a2, a3);
// 	return r;
// }
// 
// /* vec_mtvscr */
// 
// __forceinline void
// vec_mtvscr (vec_int4 a1)
// {
// 	__mtvscr(a1);
// }
// 
// __forceinline void
// vec_mtvscr (vec_uint4 a1)
// {
// 	__mtvscr(a1);
// }
// 
// __forceinline void
// vec_mtvscr (vec_bool4 a1)
// {
// 	__mtvscr(a1);
// }
// 
// __forceinline void
// vec_mtvscr (vec_short8 a1)
// {
// 	__mtvscr(a1);
// }
// 
// __forceinline void
// vec_mtvscr (vec_ushort8 a1)
// {
// 	__mtvscr(a1);
// }
// 
// __forceinline void
// vec_mtvscr (vec_bool8 a1)
// {
// 	__mtvscr(a1);
// }
// 
// __forceinline void
// vec_mtvscr (vec_pixel a1)
// {
// 	__mtvscr(a1);
// }
// 
// __forceinline void
// vec_mtvscr (vec_char16 a1)
// {
// 	__mtvscr(a1);
// }
// 
// __forceinline void
// vec_mtvscr (vec_uchar16 a1)
// {
// 	__mtvscr(a1);
// }
// 
// __forceinline void
// vec_mtvscr (vec_bool16 a1)
// {
// 	__mtvscr(a1);
// }
// 
// /* vec_mule */
// 
// __forceinline vec_ushort8
// vec_mule (vec_uchar16 a1, vec_uchar16 a2)
// {
// 	vec_ushort8 r;
// 	r.v = __vmuleub(a1, a2);
// 	return r;
// }
// 
// __forceinline vec_short8
// vec_mule (vec_char16 a1, vec_char16 a2)
// {
// 	vec_short8 r;
// 	r.v = __vmulesb(a1, a2);
// 	return r;
// }
// 
// __forceinline vec_uint4
// vec_mule (vec_ushort8 a1, vec_ushort8 a2)
// {
// 	vec_uint4 r;
// 	r.v = __vmuleuh(a1, a2);
// 	return r;
// }
// 
// __forceinline vec_int4
// vec_mule (vec_short8 a1, vec_short8 a2)
// {
// 	vec_int4 r;
// 	r.v = __vmulesh(a1, a2);
// 	return r;
// }
// 
// /* vec_vmulesh */
// 
// __forceinline vec_int4
// vec_vmulesh (vec_short8 a1, vec_short8 a2)
// {
// 	vec_int4 r;
// 	r.v = __vmulesh(a1, a2);
// 	return r;
// }
// 
// /* vec_vmuleuh */
// 
// __forceinline vec_uint4
// vec_vmuleuh (vec_ushort8 a1, vec_ushort8 a2)
// {
// 	vec_uint4 r;
// 	r.v = __vmuleuh(a1, a2);
// 	return r;
// }
// 
// /* vec_vmulesb */
// 
// __forceinline vec_short8
// vec_vmulesb (vec_char16 a1, vec_char16 a2)
// {
// 	vec_short8 r;
// 	r.v = __vmuleub(a1, a2);
// 	return r;
// }
// 
// /* vec_vmuleub */
// 
// __forceinline vec_ushort8
// vec_vmuleub (vec_uchar16 a1, vec_uchar16 a2)
// {
// 	vec_ushort8 r;
// 	r.v = __vmuleub(a1, a2);
// 	return r;
// }
// 
// /* vec_mulo */
// 
// __forceinline vec_ushort8
// vec_mulo (vec_uchar16 a1, vec_uchar16 a2)
// {
// 	vec_ushort8 r;
// 	r.v = __vmuloub(a1, a2);
// 	return r;
// }
// 
// __forceinline vec_short8
// vec_mulo (vec_char16 a1, vec_char16 a2)
// {
// 	vec_short8 r;
// 	r.v = __vmulosb(a1, a2);
// 	return r;
// }
// 
// __forceinline vec_uint4
// vec_mulo (vec_ushort8 a1, vec_ushort8 a2)
// {
// 	vec_uint4 r;
// 	r.v = __vmulouh(a1, a2);
// 	return r;
// }
// 
// __forceinline vec_int4
// vec_mulo (vec_short8 a1, vec_short8 a2)
// {
// 	vec_int4 r;
// 	r.v = __vmulosh(a1, a2);
// 	return r;
// }
// 
// /* vec_vmulosh */
// 
// __forceinline vec_int4
// vec_vmulosh (vec_short8 a1, vec_short8 a2)
// {
// 	vec_int4 r;
// 	r.v = __vmulosh(a1, a2);
// 	return r;
// }
// 
// /* vec_vmulouh */
// 
// __forceinline vec_uint4
// vec_vmulouh (vec_ushort8 a1, vec_ushort8 a2)
// {
// 	vec_uint4 r;
// 	r.v = __vmulouh(a1, a2);
// 	return r;
// }
// 
// /* vec_vmulosb */
// 
// __forceinline vec_short8
// vec_vmulosb (vec_char16 a1, vec_char16 a2)
// {
// 	vec_short8 r;
// 	r.v = __vmulosb(a1, a2);
// 	return r;
// }
// 
// /* vec_vmuloub */
// 
// __forceinline vec_ushort8
// vec_vmuloub (vec_uchar16 a1, vec_uchar16 a2)
// {
// 	vec_ushort8 r;
// 	r.v = __vmuloub(a1, a2);
// 	return r;
// }

/* vec_mfvscr */

// __forceinline vec_ushort8
// vec_mfvscr (void)
// {
// 	vec_ushort8 r;
// 	r.v = __mfvscr ();
// 	return r;
// }

/* vec_sum4s */

// __forceinline vec_uint4
// vec_sum4s (vec_uchar16 a1, vec_uint4 a2)
// {
// 	vec_uint4 r;
// 	r.v = __vsum4ubs(a1, a2);
// 	return r;
// }
// 
// __forceinline vec_int4
// vec_sum4s (vec_char16 a1, vec_int4 a2)
// {
// 	vec_int4 r;
// 	r.v = __vsum4sbs(a1, a2);
// 	return r;
// }
// 
// __forceinline vec_int4
// vec_sum4s (vec_short8 a1, vec_int4 a2)
// {
// 	vec_int4 r;
// 	r.v = __vsum4shs(a1, a2);
// 	return r;
// }
// 
// /* vec_vsum4shs */
// 
// __forceinline vec_int4
// vec_vsum4shs (vec_short8 a1, vec_int4 a2)
// {
// 	vec_int4 r;
// 	r.v = __vsum4shs(a1, a2);
// 	return r;
// }
// 
// /* vec_vsum4sbs */
// 
// __forceinline vec_int4
// vec_vsum4sbs (vec_char16 a1, vec_int4 a2)
// {
// 	vec_int4 r;
// 	r.v = __vsum4sbs(a1, a2);
// 	return r;
// }
// 
// /* vec_vsum4ubs */
// 
// __forceinline vec_uint4
// vec_vsum4ubs (vec_uchar16 a1, vec_uint4 a2)
// {
// 	vec_uint4 r;
// 	r.v = __vsum4ubs(a1, a2);
// 	return r;
// }
// 
// /* vec_sum2s */
// 
// __forceinline vec_int4
// vec_sum2s (vec_int4 a1, vec_int4 a2)
// {
// 	vec_int4 r;
// 	r.v = __vsum2sws(a1, a2);
// 	return r;
// }
// 
// /* vec_sums */
// 
// __forceinline vec_int4
// vec_sums (vec_int4 a1, vec_int4 a2)
// {
// 	vec_int4 r;
// 	r.v = __vsumsws(a1, a2);
// 	return r;
// }

#endif 

#endif /* ALTIVEC_INTRINSICS_H */
