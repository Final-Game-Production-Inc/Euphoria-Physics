// ======================
// math/altivec2.h
// (c) 2010 RockstarNorth
// ======================

#ifndef _MATH_ALTIVEC2_H_
#define _MATH_ALTIVEC2_H_

#include "math/intrinsics.h"

// TODO -- load/store intrinsics
// TODO -- compare recording intrinsics
/*
lvebx      vec_lde
lvehx      vec_lde
lvewx      vec_lde
lvxl       vec_lvxl

stvebx     vec_ste *
stvehx     vec_ste *
stvewx     vec_ste *
stvx       vec_st
stvxl      vec_stl

dss        vec_dss
dssall     vec_dssall
dst        vec_dst
dstst      vec_dstst
dststt     vec_dststt
dstt       vec_dstt

mfvscr     vec_mfvscr
mtvscr     vec_mtvscr
*/

#if RSG_XENON

#define __vec128 __vector4 // type-agnostic 128-bit vector

#define __altivec_lvx(         p,i) __lvx(         p,i)
#define __altivec_lvsl(        p,i) __lvsl(        p,i)
#define __altivec_lvsr(        p,i) __lvsr(        p,i)
#define __altivec_lvlx(        p,i) __lvlx(        p,i)
#define __altivec_lvrx(        p,i) __lvrx(        p,i)
#define __altivec_stvebx(    a,p,i) __stvebx(    a,p,i)
#define __altivec_stvehx(    a,p,i) __stvehx(    a,p,i)
#define __altivec_stvewx(    a,p,i) __stvewx(    a,p,i)
#define __altivec_vaddubm(   a,b  ) __vaddubm(   a,b  )
#define __altivec_vadduhm(   a,b  ) __vadduhm(   a,b  )
#define __altivec_vadduwm(   a,b  ) __vadduwm(   a,b  )
#define __altivec_vaddcuw(   a,b  ) __vaddcuw(   a,b  )
#define __altivec_vaddubs(   a,b  ) __vaddubs(   a,b  )
#define __altivec_vadduhs(   a,b  ) __vadduhs(   a,b  )
#define __altivec_vadduws(   a,b  ) __vadduws(   a,b  )
#define __altivec_vaddsbs(   a,b  ) __vaddsbs(   a,b  )
#define __altivec_vaddshs(   a,b  ) __vaddshs(   a,b  )
#define __altivec_vaddsws(   a,b  ) __vaddsws(   a,b  )
#define __altivec_vsububm(   a,b  ) __vsububm(   a,b  )
#define __altivec_vsubuhm(   a,b  ) __vsubuhm(   a,b  )
#define __altivec_vsubuwm(   a,b  ) __vsubuwm(   a,b  )
#define __altivec_vsubcuw(   a,b  ) __vsubcuw(   a,b  )
#define __altivec_vsububs(   a,b  ) __vsububs(   a,b  )
#define __altivec_vsubuhs(   a,b  ) __vsubuhs(   a,b  )
#define __altivec_vsubuws(   a,b  ) __vsubuws(   a,b  )
#define __altivec_vsubsbs(   a,b  ) __vsubsbs(   a,b  )
#define __altivec_vsubshs(   a,b  ) __vsubshs(   a,b  )
#define __altivec_vsubsws(   a,b  ) __vsubsws(   a,b  )
#define __altivec_vminub(    a,b  ) __vminub(    a,b  )
#define __altivec_vminuh(    a,b  ) __vminuh(    a,b  )
#define __altivec_vminuw(    a,b  ) __vminuw(    a,b  )
#define __altivec_vminsb(    a,b  ) __vminsb(    a,b  )
#define __altivec_vminsh(    a,b  ) __vminsh(    a,b  )
#define __altivec_vminsw(    a,b  ) __vminsw(    a,b  )
#define __altivec_vmaxub(    a,b  ) __vmaxub(    a,b  )
#define __altivec_vmaxuh(    a,b  ) __vmaxuh(    a,b  )
#define __altivec_vmaxuw(    a,b  ) __vmaxuw(    a,b  )
#define __altivec_vmaxsb(    a,b  ) __vmaxsb(    a,b  )
#define __altivec_vmaxsh(    a,b  ) __vmaxsh(    a,b  )
#define __altivec_vmaxsw(    a,b  ) __vmaxsw(    a,b  )
#define __altivec_vavgub(    a,b  ) __vavgub(    a,b  )
#define __altivec_vavguh(    a,b  ) __vavguh(    a,b  )
#define __altivec_vavguw(    a,b  ) __vavguw(    a,b  )
#define __altivec_vavgsb(    a,b  ) __vavgsb(    a,b  )
#define __altivec_vavgsh(    a,b  ) __vavgsh(    a,b  )
#define __altivec_vavgsw(    a,b  ) __vavgsw(    a,b  )
#define __altivec_vand(      a,b  ) __vand(      a,b  )
#define __altivec_vandc(     a,b  ) __vandc(     a,b  )
#define __altivec_vor(       a,b  ) __vor(       a,b  )
#define __altivec_vnor(      a,b  ) __vnor(      a,b  )
#define __altivec_vxor(      a,b  ) __vxor(      a,b  )
#define __altivec_vspltb(    a,i  ) __vspltb(    a,i  )
#define __altivec_vsplth(    a,i  ) __vsplth(    a,i  )
#define __altivec_vspltw(    a,i  ) __vspltw(    a,i  )
#define __altivec_vspltisb(    i  ) __vspltisb(    i  )
#define __altivec_vspltish(    i  ) __vspltish(    i  )
#define __altivec_vspltisw(    i  ) __vspltisw(    i  )
#define __altivec_vslb(      a,b  ) __vslb(      a,b  )
#define __altivec_vslh(      a,b  ) __vslh(      a,b  )
#define __altivec_vslw(      a,b  ) __vslw(      a,b  )
#define __altivec_vsrb(      a,b  ) __vsrb(      a,b  )
#define __altivec_vsrh(      a,b  ) __vsrh(      a,b  )
#define __altivec_vsrw(      a,b  ) __vsrw(      a,b  )
#define __altivec_vsrab(     a,b  ) __vsrab(     a,b  )
#define __altivec_vsrah(     a,b  ) __vsrah(     a,b  )
#define __altivec_vsraw(     a,b  ) __vsraw(     a,b  )
#define __altivec_vrlb(      a,b  ) __vrlb(      a,b  )
#define __altivec_vrlh(      a,b  ) __vrlh(      a,b  )
#define __altivec_vrlw(      a,b  ) __vrlw(      a,b  )
#define __altivec_vsldoi(    a,b,i) __vsldoi(    a,b,i)
#define __altivec_vslo(      a,b  ) __vslo(      a,b  )
#define __altivec_vsl(       a,b  ) __vsl(       a,b  )
#define __altivec_vsro(      a,b  ) __vsro(      a,b  )
#define __altivec_vsr(       a,b  ) __vsr(       a,b  )
#define __altivec_vmrglb(    a,b  ) __vmrglb(    a,b  )
#define __altivec_vmrglh(    a,b  ) __vmrglh(    a,b  )
#define __altivec_vmrglw(    a,b  ) __vmrglw(    a,b  )
#define __altivec_vmrghb(    a,b  ) __vmrghb(    a,b  )
#define __altivec_vmrghh(    a,b  ) __vmrghh(    a,b  )
#define __altivec_vmrghw(    a,b  ) __vmrghw(    a,b  )
#define __altivec_vpkuhum(   a,b  ) __vpkuhum(   a,b  )
#define __altivec_vpkuwum(   a,b  ) __vpkuwum(   a,b  )
#define __altivec_vpkuhus(   a,b  ) __vpkuhus(   a,b  )
#define __altivec_vpkuwus(   a,b  ) __vpkuwus(   a,b  )
#define __altivec_vpkshus(   a,b  ) __vpkshus(   a,b  )
#define __altivec_vpkswus(   a,b  ) __vpkswus(   a,b  )
#define __altivec_vpkshss(   a,b  ) __vpkshss(   a,b  )
#define __altivec_vpkswss(   a,b  ) __vpkswss(   a,b  )
#define __altivec_vpkpx(     a,b  ) __vpkpx(     a,b  )
#define __altivec_vupklsb(   a    ) __vupklsb(   a    )
#define __altivec_vupklsh(   a    ) __vupklsh(   a    )
#define __altivec_vupklpx(   a    ) __vupklpx(   a    )
#define __altivec_vupkhsb(   a    ) __vupkhsb(   a    )
#define __altivec_vupkhsh(   a    ) __vupkhsh(   a    )
#define __altivec_vupkhpx(   a    ) __vupkhpx(   a    )
#define __altivec_vperm(     a,b,c) __vperm(     a,b,c)
#define __altivec_vsel(      a,b,c) __vsel(      a,b,c)
#define __altivec_vcmpgtub(  a,b  ) __vcmpgtub(  a,b  )
#define __altivec_vcmpgtuh(  a,b  ) __vcmpgtuh(  a,b  )
#define __altivec_vcmpgtuw(  a,b  ) __vcmpgtuw(  a,b  )
#define __altivec_vcmpgtsb(  a,b  ) __vcmpgtsb(  a,b  )
#define __altivec_vcmpgtsh(  a,b  ) __vcmpgtsh(  a,b  )
#define __altivec_vcmpgtsw(  a,b  ) __vcmpgtsw(  a,b  )
#define __altivec_vcmpequb(  a,b  ) __vcmpequb(  a,b  )
#define __altivec_vcmpequh(  a,b  ) __vcmpequh(  a,b  )
#define __altivec_vcmpequw(  a,b  ) __vcmpequw(  a,b  )
#define __altivec_vcmpgtubR( a,b,p) __vcmpgtubR( a,b,p)
#define __altivec_vcmpgtuhR( a,b,p) __vcmpgtuhR( a,b,p)
#define __altivec_vcmpgtuwR( a,b,p) __vcmpgtuwR( a,b,p)
#define __altivec_vcmpgtsbR( a,b,p) __vcmpgtsbR( a,b,p)
#define __altivec_vcmpgtshR( a,b,p) __vcmpgtshR( a,b,p)
#define __altivec_vcmpgtswR( a,b,p) __vcmpgtswR( a,b,p)
#define __altivec_vcmpequbR( a,b,p) __vcmpequbR( a,b,p)
#define __altivec_vcmpequhR( a,b,p) __vcmpequhR( a,b,p)
#define __altivec_vcmpequwR( a,b,p) __vcmpequwR( a,b,p)
#define __altivec_vaddfp(    a,b  ) __vaddfp(    a,b  )
#define __altivec_vsubfp(    a,b  ) __vsubfp(    a,b  )
#define __altivec_vminfp(    a,b  ) __vminfp(    a,b  )
#define __altivec_vmaxfp(    a,b  ) __vmaxfp(    a,b  )
#define __altivec_vmaddfp(   a,b,c) __vmaddfp(   a,b,c)
#define __altivec_vnmsubfp(  a,b,c) __vnmsubfp(  a,b,c)
#define __altivec_vrefp(     a    ) __vrefp(     a    )
#define __altivec_vrsqrtefp( a    ) __vrsqrtefp( a    )
#define __altivec_vlogefp(   a    ) __vlogefp(   a    )
#define __altivec_vexptefp(  a    ) __vexptefp(  a    )
#define __altivec_vrfim(     a    ) __vrfim(     a    )
#define __altivec_vrfin(     a    ) __vrfin(     a    )
#define __altivec_vrfip(     a    ) __vrfip(     a    )
#define __altivec_vrfiz(     a    ) __vrfiz(     a    )
#define __altivec_vcfux(     a,i  ) __vcfux(     a,i  )
#define __altivec_vcfsx(     a,i  ) __vcfsx(     a,i  )
#define __altivec_vctuxs(    a,i  ) __vctuxs(    a,i  )
#define __altivec_vctsxs(    a,i  ) __vctsxs(    a,i  )
#define __altivec_vcmpgtfp(  a,b  ) __vcmpgtfp(  a,b  )
#define __altivec_vcmpgefp(  a,b  ) __vcmpgefp(  a,b  )
#define __altivec_vcmpeqfp(  a,b  ) __vcmpeqfp(  a,b  )
#define __altivec_vcmpbfp(   a,b  ) __vcmpbfp(   a,b  )
#define __altivec_vcmpgtfpR( a,b,p) __vcmpgtfpR( a,b,p)
#define __altivec_vcmpgefpR( a,b,p) __vcmpgefpR( a,b,p)
#define __altivec_vcmpeqfpR( a,b,p) __vcmpeqfpR( a,b,p)
#define __altivec_vcmpbfpR(  a,b,p) __vcmpbfpR(  a,b,p)

// XENON-only instructions
#define __altivec_XENON_vpermwi(a,x,y,z,w)            __vpermwi(   a,((x)<<6)|((y)<<4)|((z)<<2)|((w)<<0))
#define __altivec_XENON_vrlimi(dst,a,x,y,z,w,shift)   __vrlimi(dst,a,((x)<<3)|((y)<<2)|((z)<<1)|((w)<<0),shift)
#define __altivec_XENON_vmulfp(a,b)                   __vmulfp(a,b)
#define __altivec_XENON_vmsum3fp(a,b)                 __vmsum3fp(a,b)
#define __altivec_XENON_vmsum4fp(a,b)                 __vmsum4fp(a,b)
#define __altivec_XENON_vupkd3d(a,type)               __vupkd3d(a,type)
#define __altivec_XENON_vpkd3d(dst,b,type,mask,shift) __vpkd3d(dst,b,type,mask,shift)

#define __ppc_cntlzw(a) _CountLeadingZeros(a)
#define __ppc_cntlzd(a) _CountLeadingZeros64(a)

#elif RSG_PPU

#define SUPPORT_NATIVE_VECTOR__x64_vec4 0
#define SUPPORT_NATIVE_VECTOR__f16_vec8 0

#ifdef __SNC__

typedef vector          float     __vec128; // type-agnostic 128-bit vector

#if SUPPORT_NATIVE_VECTOR__x64_vec4
typedef vector          double    __f32_vec4;
typedef vector unsigned long long __u32_vec4;
typedef vector signed   long long __s32_vec4;
typedef vector bool     long long __b32_vec4;
#endif
typedef vector          float     __f32_vec4;
typedef vector unsigned int       __u32_vec4;
typedef vector signed   int       __s32_vec4;
typedef vector bool     int       __b32_vec4;
#if SUPPORT_NATIVE_VECTOR__f16_vec8
typedef vector          half      __f16_vec8;
#endif
typedef vector unsigned short     __u16_vec8;
typedef vector signed   short     __s16_vec8;
typedef vector bool     short     __b16_vec8;

typedef vector unsigned char      __u8_vec16;
typedef vector signed   char      __s8_vec16;
typedef vector bool     char      __b8_vec16;

typedef vector          pixel     __rgb565_v;

#else // not __SNC__

typedef __attribute__((altivec(vector__)))          float     __vec128; // type-agnostic 128-bit vector

#if SUPPORT_NATIVE_VECTOR__x64_vec4
typedef __attribute__((altivec(vector__)))          double    __f64_vec4;
typedef __attribute__((altivec(vector__))) unsigned long long __u64_vec4;
typedef __attribute__((altivec(vector__))) signed   long long __s64_vec4;
typedef __attribute__((altivec(vector__))) bool     long long __b64_vec4;
#endif
typedef __attribute__((altivec(vector__)))          float     __f32_vec4;
typedef __attribute__((altivec(vector__))) unsigned int       __u32_vec4;
typedef __attribute__((altivec(vector__))) signed   int       __s32_vec4;
typedef __attribute__((altivec(vector__))) bool     int       __b32_vec4;
#if SUPPORT_NATIVE_VECTOR__f16_vec8
typedef __attribute__((altivec(vector__)))          half      __f16_vec8;
#endif
typedef __attribute__((altivec(vector__))) unsigned short     __u16_vec8;
typedef __attribute__((altivec(vector__))) signed   short     __s16_vec8;
typedef __attribute__((altivec(vector__))) bool     short     __b16_vec8;

typedef __attribute__((altivec(vector__))) unsigned char      __u8_vec16;
typedef __attribute__((altivec(vector__))) signed   char      __s8_vec16;
typedef __attribute__((altivec(vector__))) bool     char      __b8_vec16;

typedef __attribute__((altivec(vector__)))          pixel     __rgb565_v;

#endif // not __SNC__

#define __altivec_lvx(         p,i) ((__vec128)vec_ld       (                  (i),(u8 *)(p))) // (note that p,i are swapped)
#define __altivec_lvsl(        p,i) ((__vec128)vec_lvsl     (                  (i),(u8 *)(p))) // (note that p,i are swapped)
#define __altivec_lvsr(        p,i) ((__vec128)vec_lvsr     (                  (i),(u8 *)(p))) // (note that p,i are swapped)
#define __altivec_lvlx(        p,i) ((__vec128)vec_lvlx     (                  (i),(u8 *)(p))) // (note that p,i are swapped)
#define __altivec_lvrx(        p,i) ((__vec128)vec_lvrx     (                  (i),(u8 *)(p))) // (note that p,i are swapped)
#define __altivec_stvebx(    a,p,i)            vec_ste      ((__u8_vec16)(a),  (i),(u8 *)(p))  // (note that p,i are swapped)
#define __altivec_stvehx(    a,p,i)            vec_ste      ((__u16_vec8)(a),  (i),(u16*)(p))  // (note that p,i are swapped)
#define __altivec_stvewx(    a,p,i)            vec_ste      ((__u32_vec4)(a),  (i),(u32*)(p))  // (note that p,i are swapped)
#define __altivec_vaddubm(   a,b  ) ((__vec128)vec_add      ((__u8_vec16)(a),(__u8_vec16)(b))) // integer add
#define __altivec_vadduhm(   a,b  ) ((__vec128)vec_add      ((__u16_vec8)(a),(__u16_vec8)(b)))
#define __altivec_vadduwm(   a,b  ) ((__vec128)vec_add      ((__u32_vec4)(a),(__u32_vec4)(b)))
#define __altivec_vaddcuw(   a,b  ) ((__vec128)vec_addc     ((__u32_vec4)(a),(__u32_vec4)(b))) // integer add (carrying)
#define __altivec_vaddubs(   a,b  ) ((__vec128)vec_adds     ((__u8_vec16)(a),(__u8_vec16)(b))) // integer add (saturating)
#define __altivec_vadduhs(   a,b  ) ((__vec128)vec_adds     ((__u16_vec8)(a),(__u16_vec8)(b)))
#define __altivec_vadduws(   a,b  ) ((__vec128)vec_adds     ((__u32_vec4)(a),(__u32_vec4)(b)))
#define __altivec_vaddsbs(   a,b  ) ((__vec128)vec_adds     ((__s8_vec16)(a),(__s8_vec16)(b)))
#define __altivec_vaddshs(   a,b  ) ((__vec128)vec_adds     ((__s16_vec8)(a),(__s16_vec8)(b)))
#define __altivec_vaddsws(   a,b  ) ((__vec128)vec_adds     ((__s32_vec4)(a),(__s32_vec4)(b)))
#define __altivec_vsububm(   a,b  ) ((__vec128)vec_sub      ((__u8_vec16)(a),(__u8_vec16)(b))) // integer subtract
#define __altivec_vsubuhm(   a,b  ) ((__vec128)vec_sub      ((__u16_vec8)(a),(__u16_vec8)(b)))
#define __altivec_vsubuwm(   a,b  ) ((__vec128)vec_sub      ((__u32_vec4)(a),(__u32_vec4)(b)))
#define __altivec_vsubcuw(   a,b  ) ((__vec128)vec_subc     ((__u32_vec4)(a),(__u32_vec4)(b))) // integer subtract (carrying)
#define __altivec_vsububs(   a,b  ) ((__vec128)vec_subs     ((__u8_vec16)(a),(__u8_vec16)(b))) // integer subtract (saturating)
#define __altivec_vsubuhs(   a,b  ) ((__vec128)vec_subs     ((__u16_vec8)(a),(__u16_vec8)(b)))
#define __altivec_vsubuws(   a,b  ) ((__vec128)vec_subs     ((__u32_vec4)(a),(__u32_vec4)(b)))
#define __altivec_vsubsbs(   a,b  ) ((__vec128)vec_subs     ((__s8_vec16)(a),(__s8_vec16)(b)))
#define __altivec_vsubshs(   a,b  ) ((__vec128)vec_subs     ((__s16_vec8)(a),(__s16_vec8)(b)))
#define __altivec_vsubsws(   a,b  ) ((__vec128)vec_subs     ((__s32_vec4)(a),(__s32_vec4)(b)))
#define __altivec_vminub(    a,b  ) ((__vec128)vec_min      ((__u8_vec16)(a),(__u8_vec16)(b))) // integer min
#define __altivec_vminuh(    a,b  ) ((__vec128)vec_min      ((__u16_vec8)(a),(__u16_vec8)(b)))
#define __altivec_vminuw(    a,b  ) ((__vec128)vec_min      ((__u32_vec4)(a),(__u32_vec4)(b)))
#define __altivec_vminsb(    a,b  ) ((__vec128)vec_min      ((__s8_vec16)(a),(__s8_vec16)(b)))
#define __altivec_vminsh(    a,b  ) ((__vec128)vec_min      ((__s16_vec8)(a),(__s16_vec8)(b)))
#define __altivec_vminsw(    a,b  ) ((__vec128)vec_min      ((__s32_vec4)(a),(__s32_vec4)(b)))
#define __altivec_vmaxub(    a,b  ) ((__vec128)vec_max      ((__u8_vec16)(a),(__u8_vec16)(b))) // integer max
#define __altivec_vmaxuh(    a,b  ) ((__vec128)vec_max      ((__u16_vec8)(a),(__u16_vec8)(b)))
#define __altivec_vmaxuw(    a,b  ) ((__vec128)vec_max      ((__u32_vec4)(a),(__u32_vec4)(b)))
#define __altivec_vmaxsb(    a,b  ) ((__vec128)vec_max      ((__s8_vec16)(a),(__s8_vec16)(b)))
#define __altivec_vmaxsh(    a,b  ) ((__vec128)vec_max      ((__s16_vec8)(a),(__s16_vec8)(b)))
#define __altivec_vmaxsw(    a,b  ) ((__vec128)vec_max      ((__s32_vec4)(a),(__s32_vec4)(b)))
#define __altivec_vavgub(    a,b  ) ((__vec128)vec_avg      ((__u8_vec16)(a),(__u8_vec16)(b))) // integer average
#define __altivec_vavguh(    a,b  ) ((__vec128)vec_avg      ((__u16_vec8)(a),(__u16_vec8)(b)))
#define __altivec_vavguw(    a,b  ) ((__vec128)vec_avg      ((__u32_vec4)(a),(__u32_vec4)(b)))
#define __altivec_vavgsb(    a,b  ) ((__vec128)vec_avg      ((__s8_vec16)(a),(__s8_vec16)(b)))
#define __altivec_vavgsh(    a,b  ) ((__vec128)vec_avg      ((__s16_vec8)(a),(__s16_vec8)(b)))
#define __altivec_vavgsw(    a,b  ) ((__vec128)vec_avg      ((__s32_vec4)(a),(__s32_vec4)(b)))
#define __altivec_vand(      a,b  ) ((__vec128)vec_and      ((__u32_vec4)(a),(__u32_vec4)(b))) // logical and
#define __altivec_vandc(     a,b  ) ((__vec128)vec_andc     ((__u32_vec4)(a),(__u32_vec4)(b))) // logical and-complement = a & ~b
#define __altivec_vor(       a,b  ) ((__vec128)vec_or       ((__u32_vec4)(a),(__u32_vec4)(b))) // logical or
#define __altivec_vnor(      a,b  ) ((__vec128)vec_nor      ((__u32_vec4)(a),(__u32_vec4)(b))) // logical nor = ~(a | b)
#define __altivec_vxor(      a,b  ) ((__vec128)vec_xor      ((__u32_vec4)(a),(__u32_vec4)(b))) // logical xor
#define __altivec_vspltb(    a,i  ) ((__vec128)vec_splat    ((__u8_vec16)(a),            (i))) // i = [0..15] // element splat
#define __altivec_vsplth(    a,i  ) ((__vec128)vec_splat    ((__u16_vec8)(a),            (i))) // i = [0..7]
#define __altivec_vspltw(    a,i  ) ((__vec128)vec_splat    ((__u32_vec4)(a),            (i))) // i = [0..3]
#define __altivec_vspltisb(    i  ) ((__vec128)vec_splat_s8 (                            (i))) // i = [-16..15] // immediate splat signed 5-bit value
#define __altivec_vspltish(    i  ) ((__vec128)vec_splat_s16(                            (i))) // i = [-16..15]
#define __altivec_vspltisw(    i  ) ((__vec128)vec_splat_s32(                            (i))) // i = [-16..15]
#define __altivec_vslb(      a,b  ) ((__vec128)vec_sl       ((__u8_vec16)(a),(__u8_vec16)(b))) // element shift left by bits
#define __altivec_vslh(      a,b  ) ((__vec128)vec_sl       ((__u16_vec8)(a),(__u16_vec8)(b)))
#define __altivec_vslw(      a,b  ) ((__vec128)vec_sl       ((__u32_vec4)(a),(__u32_vec4)(b)))
#define __altivec_vsrb(      a,b  ) ((__vec128)vec_sr       ((__u8_vec16)(a),(__u8_vec16)(b))) // element shift right by bits
#define __altivec_vsrh(      a,b  ) ((__vec128)vec_sr       ((__u16_vec8)(a),(__u16_vec8)(b)))
#define __altivec_vsrw(      a,b  ) ((__vec128)vec_sr       ((__u32_vec4)(a),(__u32_vec4)(b)))
#define __altivec_vsrab(     a,b  ) ((__vec128)vec_sra      ((__u8_vec16)(a),(__u8_vec16)(b))) // element shift right arithmetic by bits
#define __altivec_vsrah(     a,b  ) ((__vec128)vec_sra      ((__u16_vec8)(a),(__u16_vec8)(b)))
#define __altivec_vsraw(     a,b  ) ((__vec128)vec_sra      ((__u32_vec4)(a),(__u32_vec4)(b)))
#define __altivec_vrlb(      a,b  ) ((__vec128)vec_rl       ((__u8_vec16)(a),(__u8_vec16)(b))) // element rotate left by bits
#define __altivec_vrlh(      a,b  ) ((__vec128)vec_rl       ((__u16_vec8)(a),(__u16_vec8)(b)))
#define __altivec_vrlw(      a,b  ) ((__vec128)vec_rl       ((__u32_vec4)(a),(__u32_vec4)(b)))
#define __altivec_vsldoi(    a,b,i) ((__vec128)vec_sld      ((__u8_vec16)(a),(__u8_vec16)(b),(i))) // vector shift left double by bytes immediate
#define __altivec_vslo(      a,b  ) ((__vec128)vec_slo      ((__u8_vec16)(a),(__u8_vec16)(b)    )) // vector shift left by bytes  (shift is specified in bits [3..6] of b)
#define __altivec_vsl(       a,b  ) ((__vec128)vec_sll      ((__u8_vec16)(a),(__u8_vec16)(b)    )) // vector shift left by bits   (shift is specified in *EACH* lower bits [0..2] of all 16 byte elements of b)
#define __altivec_vsro(      a,b  ) ((__vec128)vec_sro      ((__u8_vec16)(a),(__u8_vec16)(b)    )) // vector shift right by bytes (shift is specified in bits [3..6] of b)
#define __altivec_vsr(       a,b  ) ((__vec128)vec_srl      ((__u8_vec16)(a),(__u8_vec16)(b)    )) // vector shift right by bits  (shift is specified in *EACH* lower bits [0..2] of all 16 byte elements of b)
#define __altivec_vmrglb(    a,b  ) ((__vec128)vec_mergel   ((__u8_vec16)(a),(__u8_vec16)(b))) // merge low
#define __altivec_vmrglh(    a,b  ) ((__vec128)vec_mergel   ((__u16_vec8)(a),(__u16_vec8)(b)))
#define __altivec_vmrglw(    a,b  ) ((__vec128)vec_mergel   ((__u32_vec4)(a),(__u32_vec4)(b)))
#define __altivec_vmrghb(    a,b  ) ((__vec128)vec_mergeh   ((__u8_vec16)(a),(__u8_vec16)(b))) // merge high
#define __altivec_vmrghh(    a,b  ) ((__vec128)vec_mergeh   ((__u16_vec8)(a),(__u16_vec8)(b)))
#define __altivec_vmrghw(    a,b  ) ((__vec128)vec_mergeh   ((__u32_vec4)(a),(__u32_vec4)(b)))
#define __altivec_vpkuhum(   a,b  ) ((__vec128)vec_pack     ((__u16_vec8)(a),(__u16_vec8)(b))) // pack unsigned to unsigned (modulate)
#define __altivec_vpkuwum(   a,b  ) ((__vec128)vec_pack     ((__u32_vec4)(a),(__u32_vec4)(b)))
#define __altivec_vpkuhus(   a,b  ) ((__vec128)vec_packs    ((__u16_vec8)(a),(__u16_vec8)(b))) // pack unsigned to unsigned (saturate)
#define __altivec_vpkuwus(   a,b  ) ((__vec128)vec_packs    ((__u32_vec4)(a),(__u32_vec4)(b)))
#define __altivec_vpkshus(   a,b  ) ((__vec128)vec_packsu   ((__s16_vec8)(a),(__s16_vec8)(b))) // pack signed to unsigned (saturate)
#define __altivec_vpkswus(   a,b  ) ((__vec128)vec_packsu   ((__s32_vec4)(a),(__s32_vec4)(b)))
#define __altivec_vpkshss(   a,b  ) ((__vec128)vec_packs    ((__s16_vec8)(a),(__s16_vec8)(b))) // pack signed to signed (saturate)
#define __altivec_vpkswss(   a,b  ) ((__vec128)vec_packs    ((__s32_vec4)(a),(__s32_vec4)(b)))
#define __altivec_vpkpx(     a,b  ) ((__vec128)vec_packpx   ((__u32_vec4)(a),(__u32_vec4)(b))) // pack pixel
#define __altivec_vupklsb(   a    ) ((__vec128)vec_unpackl  ((__s8_vec16)(a)                )) // unpack low
#define __altivec_vupklsh(   a    ) ((__vec128)vec_unpackl  ((__s16_vec8)(a)                ))
#define __altivec_vupklpx(   a    ) ((__vec128)vec_unpackl  ((__rgb565_v)(a)                ))
#define __altivec_vupkhsb(   a    ) ((__vec128)vec_unpackh  ((__s8_vec16)(a)                )) // unpack high
#define __altivec_vupkhsh(   a    ) ((__vec128)vec_unpackh  ((__s16_vec8)(a)                ))
#define __altivec_vupkhpx(   a    ) ((__vec128)vec_unpackh  ((__rgb565_v)(a)                ))
#define __altivec_vperm(     a,b,c) ((__vec128)vec_perm     ((__u8_vec16)(a),(__u8_vec16)(b),(__u8_vec16)(c))) // permute
#define __altivec_vsel(      a,b,c) ((__vec128)vec_sel      ((__u8_vec16)(a),(__u8_vec16)(b),(__u8_vec16)(c))) // select
#define __altivec_vcmpgtub(  a,b  ) ((__vec128)vec_cmpgt    ((__u8_vec16)(a),(__u8_vec16)(b))) // integer element compare
#define __altivec_vcmpgtuh(  a,b  ) ((__vec128)vec_cmpgt    ((__u16_vec8)(a),(__u16_vec8)(b)))
#define __altivec_vcmpgtuw(  a,b  ) ((__vec128)vec_cmpgt    ((__u32_vec4)(a),(__u32_vec4)(b)))
#define __altivec_vcmpgtsb(  a,b  ) ((__vec128)vec_cmpgt    ((__s8_vec16)(a),(__s8_vec16)(b)))
#define __altivec_vcmpgtsh(  a,b  ) ((__vec128)vec_cmpgt    ((__s16_vec8)(a),(__s16_vec8)(b)))
#define __altivec_vcmpgtsw(  a,b  ) ((__vec128)vec_cmpgt    ((__s32_vec4)(a),(__s32_vec4)(b)))
#define __altivec_vcmpequb(  a,b  ) ((__vec128)vec_cmpeq    ((__u8_vec16)(a),(__u8_vec16)(b)))
#define __altivec_vcmpequh(  a,b  ) ((__vec128)vec_cmpeq    ((__u16_vec8)(a),(__u16_vec8)(b)))
#define __altivec_vcmpequw(  a,b  ) ((__vec128)vec_cmpeq    ((__u32_vec4)(a),(__u32_vec4)(b)))
#define __altivec_vcmpgtubR( a,b,p) // integer element compare (recording)
#define __altivec_vcmpgtuhR( a,b,p)
#define __altivec_vcmpgtuwR( a,b,p)
#define __altivec_vcmpgtsbR( a,b,p)
#define __altivec_vcmpgtshR( a,b,p)
#define __altivec_vcmpgtswR( a,b,p)
#define __altivec_vcmpequbR( a,b,p)
#define __altivec_vcmpequhR( a,b,p)
#define __altivec_vcmpequwR( a,b,p)
#define __altivec_vaddfp(    a,b  ) ((__vec128)vec_add      ((__f32_vec4)(a),(__f32_vec4)(b))) // floating point add/sub/min/max
#define __altivec_vsubfp(    a,b  ) ((__vec128)vec_sub      ((__f32_vec4)(a),(__f32_vec4)(b)))
#define __altivec_vminfp(    a,b  ) ((__vec128)vec_min      ((__f32_vec4)(a),(__f32_vec4)(b)))
#define __altivec_vmaxfp(    a,b  ) ((__vec128)vec_max      ((__f32_vec4)(a),(__f32_vec4)(b)))
#define __altivec_vmaddfp(   a,b,c) ((__vec128)vec_madd     ((__f32_vec4)(a),(__f32_vec4)(b),(__f32_vec4)(c))) // floating point multiply add (a*b + c)
#define __altivec_vnmsubfp(  a,b,c) ((__vec128)vec_nmsub    ((__f32_vec4)(a),(__f32_vec4)(b),(__f32_vec4)(c))) // floating point negative multiply subtract (a*b - c)
#define __altivec_vrefp(     a    ) ((__vec128)vec_re       ((__f32_vec4)(a)                )) // floating point recip/recipsqrt/log2/exp2 estimate
#define __altivec_vrsqrtefp( a    ) ((__vec128)vec_rsqrte   ((__f32_vec4)(a)                ))
#define __altivec_vlogefp(   a    ) ((__vec128)vec_loge     ((__f32_vec4)(a)                ))
#define __altivec_vexptefp(  a    ) ((__vec128)vec_expte    ((__f32_vec4)(a)                ))
#define __altivec_vrfim(     a    ) ((__vec128)vec_floor    ((__f32_vec4)(a)                )) // floating point rounding
#define __altivec_vrfin(     a    ) ((__vec128)vec_round    ((__f32_vec4)(a)                ))
#define __altivec_vrfip(     a    ) ((__vec128)vec_ceil     ((__f32_vec4)(a)                ))
#define __altivec_vrfiz(     a    ) ((__vec128)vec_trunc    ((__f32_vec4)(a)                ))
#define __altivec_vcfux(     a,i  ) ((__vec128)vec_ctf      ((__u32_vec4)(a),            (i))) // i = [0..31] // convert int to floating point
#define __altivec_vcfsx(     a,i  ) ((__vec128)vec_ctf      ((__s32_vec4)(a),            (i))) // i = [0..31]
#define __altivec_vctuxs(    a,i  ) ((__vec128)vec_ctu      ((__f32_vec4)(a),            (i))) // i = [0..31] // convert floating point to int
#define __altivec_vctsxs(    a,i  ) ((__vec128)vec_cts      ((__f32_vec4)(a),            (i))) // i = [0..31]
#define __altivec_vcmpgtfp(  a,b  ) ((__vec128)vec_cmpgt    ((__f32_vec4)(a),(__f32_vec4)(b))) // floating point element compare
#define __altivec_vcmpgefp(  a,b  ) ((__vec128)vec_cmpge    ((__f32_vec4)(a),(__f32_vec4)(b)))
#define __altivec_vcmpeqfp(  a,b  ) ((__vec128)vec_cmpeq    ((__f32_vec4)(a),(__f32_vec4)(b)))
#define __altivec_vcmpbfp(   a,b  ) ((__vec128)vec_cmpb     ((__f32_vec4)(a),(__f32_vec4)(b)))
#define __altivec_vcmpgtfpR( a,b,p) // floating point element compare (recording)
#define __altivec_vcmpgefpR( a,b,p)
#define __altivec_vcmpeqfpR( a,b,p)
#define __altivec_vcmpbfpR(  a,b,p)

#define __altivec_const_u32_vec4(i) ((__vec128)(__u32_vec4(i)))
#define __altivec_const_u16_vec8(i) ((__vec128)(__u16_vec8(i)))
#define __altivec_const_u8_vec16(i) ((__vec128)(__u8_vec16(i)))

// PPU-only instructions
#define __altivec_PPU_vmuleub(   a,b  ) ((__vec128)vec_mule  ((__u8_vec16)(a),(__u8_vec16)(b))) // integer multiply even
#define __altivec_PPU_vmuleuh(   a,b  ) ((__vec128)vec_mule  ((__u16_vec8)(a),(__u16_vec8)(b)))
#define __altivec_PPU_vmulesb(   a,b  ) ((__vec128)vec_mule  ((__s8_vec16)(a),(__s8_vec16)(b)))
#define __altivec_PPU_vmulesh(   a,b  ) ((__vec128)vec_mule  ((__s16_vec8)(a),(__s16_vec8)(b)))
#define __altivec_PPU_vmuloub(   a,b  ) ((__vec128)vec_mulo  ((__u8_vec16)(a),(__u8_vec16)(b))) // integer multiply odd
#define __altivec_PPU_vmulouh(   a,b  ) ((__vec128)vec_mulo  ((__u16_vec8)(a),(__u16_vec8)(b)))
#define __altivec_PPU_vmulosb(   a,b  ) ((__vec128)vec_mulo  ((__s8_vec16)(a),(__s8_vec16)(b)))
#define __altivec_PPU_vmulosh(   a,b  ) ((__vec128)vec_mulo  ((__s16_vec8)(a),(__s16_vec8)(b)))
#define __altivec_PPU_vmladduhm( a,b,c) ((__vec128)vec_mladd ((__u16_vec8)(a),(__u16_vec8)(b),(__u16_vec8)(c))) // integer multiply add low       = (a*b + c) & 0xffff
#define __altivec_PPU_vmhaddshs( a,b,c) ((__vec128)vec_madds ((__s16_vec8)(a),(__s16_vec8)(b),(__s16_vec8)(c))) // integer multiply add saturated = sat((a*b        )/2^^15 + c)
#define __altivec_PPU_vmhraddshs(a,b,c) ((__vec128)vec_mradds((__s16_vec8)(a),(__s16_vec8)(b),(__s16_vec8)(c))) // integer multiply add rounded   = sat((a*b + 2^^14)/2^^15 + c)
#define __altivec_PPU_vmsumubm(  a,b,c) ((__vec128)vec_msum  ((__u8_vec16)(a),(__u8_vec16)(b),(__u32_vec4)(c))) // integer multiply sum
#define __altivec_PPU_vmsumuhm(  a,b,c) ((__vec128)vec_msum  ((__u16_vec4)(a),(__u16_vec8)(b),(__u32_vec4)(c)))
#define __altivec_PPU_vmsummbm(  a,b,c) ((__vec128)vec_msum  ((__s8_vec16)(a),(__u8_vec16)(b),(__s32_vec4)(c)))
#define __altivec_PPU_vmsumshm(  a,b,c) ((__vec128)vec_msum  ((__s16_vec4)(a),(__s16_vec8)(b),(__s32_vec4)(c)))
#define __altivec_PPU_vmsumuhs(  a,b,c) ((__vec128)vec_msums ((__u16_vec4)(a),(__u16_vec8)(b),(__u32_vec4)(c))) // integer multiply sum (saturating)
#define __altivec_PPU_vmsumshs(  a,b,c) ((__vec128)vec_msums ((__s16_vec4)(a),(__s16_vec8)(b),(__s32_vec4)(c)))
#define __altivec_PPU_vsumsws(   a,b  ) ((__vec128)vec_sums  ((__s32_vec4)(a),(__s32_vec4)(b))) // sat(ax+ay+az+aw + bx) // integer sum (saturating)
#define __altivec_PPU_vsum2sws(  a,b  ) ((__vec128)vec_sum2s ((__s32_vec4)(a),(__s32_vec4)(b)))
#define __altivec_PPU_vsum4ubs(  a,b  ) ((__vec128)vec_sum4s ((__u8_vec16)(a),(__u32_vec4)(b)))
#define __altivec_PPU_vsum4sbs(  a,b  ) ((__vec128)vec_sum4s ((__s8_vec16)(a),(__s32_vec4)(b)))
#define __altivec_PPU_vsum4shs(  a,b  ) ((__vec128)vec_sum4s ((__s16_vec8)(a),(__s32_vec4)(b)))

#define __ppc_cntlzw(a) __cntlzw(a)
#define __ppc_cntlzd(a) __cntlzd(a)

#endif // RSG_PPU

#if RSG_CPU_PPC

#define __altivec_vsplatf(f) __altivec_vspltw(__altivec_lvlx(&f, 0), 0)

#if RSG_XENON
#define __altivec_vmulfp0(a,b) __altivec_XENON_vmulfp(a,b)
#else
#define __altivec_vmulfp0(a,b) __altivec_vmaddfp(a,b,__altivec_vspltisw(0))
#endif

#define __altivec_vcmpltub(a,b) __altivec_vcmpgtub(b,a)
#define __altivec_vcmpltuh(a,b) __altivec_vcmpgtuh(b,a)
#define __altivec_vcmpltuw(a,b) __altivec_vcmpgtuw(b,a)
#define __altivec_vcmpltsb(a,b) __altivec_vcmpgtsb(b,a)
#define __altivec_vcmpltsh(a,b) __altivec_vcmpgtsh(b,a)
#define __altivec_vcmpltsw(a,b) __altivec_vcmpgtsw(b,a)
#define __altivec_vcmpltfp(a,b) __altivec_vcmpgtfp(b,a)
#define __altivec_vcmplefp(a,b) __altivec_vcmpgefp(b,a)
#define __altivec_vcmpeqsb(a,b) __altivec_vcmpequb(b,a) // eq is equivalent for signed/unsigned
#define __altivec_vcmpeqsh(a,b) __altivec_vcmpequh(b,a) // eq is equivalent for signed/unsigned
#define __altivec_vcmpeqsw(a,b) __altivec_vcmpequw(b,a) // eq is equivalent for signed/unsigned

#define __altivec_vcmpltubR(a,b,p) __altivec_vcmpgtubR(b,a,p)
#define __altivec_vcmpltuhR(a,b,p) __altivec_vcmpgtuhR(b,a,p)
#define __altivec_vcmpltuwR(a,b,p) __altivec_vcmpgtuwR(b,a,p)
#define __altivec_vcmpltsbR(a,b,p) __altivec_vcmpgtsbR(b,a,p)
#define __altivec_vcmpltshR(a,b,p) __altivec_vcmpgtshR(b,a,p)
#define __altivec_vcmpltswR(a,b,p) __altivec_vcmpgtswR(b,a,p)
#define __altivec_vcmpltfpR(a,b,p) __altivec_vcmpgtfpR(b,a,p)
#define __altivec_vcmplefpR(a,b,p) __altivec_vcmpgefpR(b,a,p)
#define __altivec_vcmpeqsbR(a,b,p) __altivec_vcmpequbR(b,a,p) // eq is equivalent for signed/unsigned 
#define __altivec_vcmpeqshR(a,b,p) __altivec_vcmpequhR(b,a,p) // eq is equivalent for signed/unsigned 
#define __altivec_vcmpeqswR(a,b,p) __altivec_vcmpequwR(b,a,p) // eq is equivalent for signed/unsigned 

#if RSG_XENON

#define _XENON_CRMASK_CR6       0x000000f0
#define _XENON_CRMASK_CR6TRUE   0x00000080
#define _XENON_CRMASK_CR6FALSE  0x00000020
#define _XENON_CRMASK_CR6BOUNDS 0x00000020 // _XENON_CRMASK_CR6FALSE

// TODO -- need to test .. is it more efficient to deal with the cr value directly rather than converting to bool?

// u8
__forceinline bool __altivec_vcmpltub_all(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpltubR(b, a, &cr); return (cr & _XENON_CRMASK_CR6TRUE ) != 0; }
__forceinline bool __altivec_vcmpgtub_all(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpgtubR(a, b, &cr); return (cr & _XENON_CRMASK_CR6TRUE ) != 0; }
__forceinline bool __altivec_vcmpequb_all(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpequbR(a, b, &cr); return (cr & _XENON_CRMASK_CR6TRUE ) != 0; }
__forceinline bool __altivec_vcmpneub_all(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpequbR(a, b, &cr); return (cr & _XENON_CRMASK_CR6FALSE) != 0; }

__forceinline bool __altivec_vcmpltub_any(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpltubR(b, a, &cr); return (cr & _XENON_CRMASK_CR6FALSE) == 0; }
__forceinline bool __altivec_vcmpgtub_any(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpgtubR(a, b, &cr); return (cr & _XENON_CRMASK_CR6FALSE) == 0; }
__forceinline bool __altivec_vcmpequb_any(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpequbR(a, b, &cr); return (cr & _XENON_CRMASK_CR6FALSE) == 0; }
__forceinline bool __altivec_vcmpneub_any(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpequbR(a, b, &cr); return (cr & _XENON_CRMASK_CR6TRUE ) == 0; }

// s8
__forceinline bool __altivec_vcmpltsb_all(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpltsbR(b, a, &cr); return (cr & _XENON_CRMASK_CR6TRUE ) != 0; }
__forceinline bool __altivec_vcmpgtsb_all(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpgtsbR(a, b, &cr); return (cr & _XENON_CRMASK_CR6TRUE ) != 0; }
__forceinline bool __altivec_vcmpeqsb_all(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpeqsbR(a, b, &cr); return (cr & _XENON_CRMASK_CR6TRUE ) != 0; }
__forceinline bool __altivec_vcmpnesb_all(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpeqsbR(a, b, &cr); return (cr & _XENON_CRMASK_CR6FALSE) != 0; }

__forceinline bool __altivec_vcmpltsb_any(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpltsbR(b, a, &cr); return (cr & _XENON_CRMASK_CR6FALSE) == 0; }
__forceinline bool __altivec_vcmpgtsb_any(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpgtsbR(a, b, &cr); return (cr & _XENON_CRMASK_CR6FALSE) == 0; }
__forceinline bool __altivec_vcmpeqsb_any(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpeqsbR(a, b, &cr); return (cr & _XENON_CRMASK_CR6FALSE) == 0; }
__forceinline bool __altivec_vcmpnesb_any(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpeqsbR(a, b, &cr); return (cr & _XENON_CRMASK_CR6TRUE ) == 0; }

// u16
__forceinline bool __altivec_vcmpltuh_all(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpltuhR(b, a, &cr); return (cr & _XENON_CRMASK_CR6TRUE ) != 0; }
__forceinline bool __altivec_vcmpgtuh_all(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpgtuhR(a, b, &cr); return (cr & _XENON_CRMASK_CR6TRUE ) != 0; }
__forceinline bool __altivec_vcmpequh_all(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpequhR(a, b, &cr); return (cr & _XENON_CRMASK_CR6TRUE ) != 0; }
__forceinline bool __altivec_vcmpneuh_all(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpequhR(a, b, &cr); return (cr & _XENON_CRMASK_CR6FALSE) != 0; }

__forceinline bool __altivec_vcmpltuh_any(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpltuhR(b, a, &cr); return (cr & _XENON_CRMASK_CR6FALSE) == 0; }
__forceinline bool __altivec_vcmpgtuh_any(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpgtuhR(a, b, &cr); return (cr & _XENON_CRMASK_CR6FALSE) == 0; }
__forceinline bool __altivec_vcmpequh_any(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpequhR(a, b, &cr); return (cr & _XENON_CRMASK_CR6FALSE) == 0; }
__forceinline bool __altivec_vcmpneuh_any(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpequhR(a, b, &cr); return (cr & _XENON_CRMASK_CR6TRUE ) == 0; }

// s16
__forceinline bool __altivec_vcmpltsh_all(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpltshR(b, a, &cr); return (cr & _XENON_CRMASK_CR6TRUE ) != 0; }
__forceinline bool __altivec_vcmpgtsh_all(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpgtshR(a, b, &cr); return (cr & _XENON_CRMASK_CR6TRUE ) != 0; }
__forceinline bool __altivec_vcmpeqsh_all(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpeqshR(a, b, &cr); return (cr & _XENON_CRMASK_CR6TRUE ) != 0; }
__forceinline bool __altivec_vcmpnesh_all(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpeqshR(a, b, &cr); return (cr & _XENON_CRMASK_CR6FALSE) != 0; }

__forceinline bool __altivec_vcmpltsh_any(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpltshR(b, a, &cr); return (cr & _XENON_CRMASK_CR6FALSE) == 0; }
__forceinline bool __altivec_vcmpgtsh_any(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpgtshR(a, b, &cr); return (cr & _XENON_CRMASK_CR6FALSE) == 0; }
__forceinline bool __altivec_vcmpeqsh_any(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpeqshR(a, b, &cr); return (cr & _XENON_CRMASK_CR6FALSE) == 0; }
__forceinline bool __altivec_vcmpnesh_any(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpeqshR(a, b, &cr); return (cr & _XENON_CRMASK_CR6TRUE ) == 0; }

// unsigned int
__forceinline bool __altivec_vcmpltuw_all(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpltuwR(b, a, &cr); return (cr & _XENON_CRMASK_CR6TRUE ) != 0; }
__forceinline bool __altivec_vcmpgtuw_all(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpgtuwR(a, b, &cr); return (cr & _XENON_CRMASK_CR6TRUE ) != 0; }
__forceinline bool __altivec_vcmpequw_all(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpequwR(a, b, &cr); return (cr & _XENON_CRMASK_CR6TRUE ) != 0; }
__forceinline bool __altivec_vcmpneuw_all(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpequwR(a, b, &cr); return (cr & _XENON_CRMASK_CR6FALSE) != 0; }

__forceinline bool __altivec_vcmpltuw_any(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpltuwR(b, a, &cr); return (cr & _XENON_CRMASK_CR6FALSE) == 0; }
__forceinline bool __altivec_vcmpgtuw_any(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpgtuwR(a, b, &cr); return (cr & _XENON_CRMASK_CR6FALSE) == 0; }
__forceinline bool __altivec_vcmpequw_any(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpequwR(a, b, &cr); return (cr & _XENON_CRMASK_CR6FALSE) == 0; }
__forceinline bool __altivec_vcmpneuw_any(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpequwR(a, b, &cr); return (cr & _XENON_CRMASK_CR6TRUE ) == 0; }

// s32
__forceinline bool __altivec_vcmpltsw_all(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpltswR(b, a, &cr); return (cr & _XENON_CRMASK_CR6TRUE ) != 0; }
__forceinline bool __altivec_vcmpgtsw_all(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpgtswR(a, b, &cr); return (cr & _XENON_CRMASK_CR6TRUE ) != 0; }
__forceinline bool __altivec_vcmpeqsw_all(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpeqswR(a, b, &cr); return (cr & _XENON_CRMASK_CR6TRUE ) != 0; }
__forceinline bool __altivec_vcmpnesw_all(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpeqswR(a, b, &cr); return (cr & _XENON_CRMASK_CR6FALSE) != 0; }

__forceinline bool __altivec_vcmpltsw_any(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpltswR(b, a, &cr); return (cr & _XENON_CRMASK_CR6FALSE) == 0; }
__forceinline bool __altivec_vcmpgtsw_any(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpgtswR(a, b, &cr); return (cr & _XENON_CRMASK_CR6FALSE) == 0; }
__forceinline bool __altivec_vcmpeqsw_any(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpeqswR(a, b, &cr); return (cr & _XENON_CRMASK_CR6FALSE) == 0; }
__forceinline bool __altivec_vcmpnesw_any(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpeqswR(a, b, &cr); return (cr & _XENON_CRMASK_CR6TRUE ) == 0; }

// f32
__forceinline bool __altivec_vcmpltfp_all(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpltfpR(b, a, &cr); return (cr & _XENON_CRMASK_CR6TRUE ) != 0; }
__forceinline bool __altivec_vcmplefp_all(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmplefpR(b, a, &cr); return (cr & _XENON_CRMASK_CR6TRUE ) != 0; }
__forceinline bool __altivec_vcmpgtfp_all(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpgtfpR(a, b, &cr); return (cr & _XENON_CRMASK_CR6TRUE ) != 0; }
__forceinline bool __altivec_vcmpgefp_all(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpgefpR(a, b, &cr); return (cr & _XENON_CRMASK_CR6TRUE ) != 0; }
__forceinline bool __altivec_vcmpeqfp_all(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpeqfpR(a, b, &cr); return (cr & _XENON_CRMASK_CR6TRUE ) != 0; }
__forceinline bool __altivec_vcmpnefp_all(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpeqfpR(a, b, &cr); return (cr & _XENON_CRMASK_CR6FALSE) != 0; }

__forceinline bool __altivec_vcmpltfp_any(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpltfpR(b, a, &cr); return (cr & _XENON_CRMASK_CR6FALSE) == 0; }
__forceinline bool __altivec_vcmplefp_any(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmplefpR(b, a, &cr); return (cr & _XENON_CRMASK_CR6FALSE) == 0; }
__forceinline bool __altivec_vcmpgtfp_any(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpgtfpR(a, b, &cr); return (cr & _XENON_CRMASK_CR6FALSE) == 0; }
__forceinline bool __altivec_vcmpgefp_any(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpgefpR(a, b, &cr); return (cr & _XENON_CRMASK_CR6FALSE) == 0; }
__forceinline bool __altivec_vcmpeqfp_any(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpeqfpR(a, b, &cr); return (cr & _XENON_CRMASK_CR6FALSE) == 0; }
__forceinline bool __altivec_vcmpnefp_any(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpeqfpR(a, b, &cr); return (cr & _XENON_CRMASK_CR6TRUE ) == 0; }

// bounds
__forceinline bool __altivec_vcmpbfp_all_in (__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpbfpR(a, b, &cr); return (cr & _XENON_CRMASK_CR6TRUE ) != 0; }
__forceinline bool __altivec_vcmpbfp_all_out(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpbfpR(a, b, &cr); return (cr & _XENON_CRMASK_CR6FALSE) != 0; }
__forceinline bool __altivec_vcmpbfp_any_in (__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpbfpR(a, b, &cr); return (cr & _XENON_CRMASK_CR6FALSE) == 0; }
__forceinline bool __altivec_vcmpbfp_any_out(__vec128 a, __vec128 b) { unsigned int cr; __altivec_vcmpbfpR(a, b, &cr); return (cr & _XENON_CRMASK_CR6TRUE ) == 0; }

#else // not RSG_XENON

// u8
__forceinline bool __altivec_vcmpltub_all(__vec128 a, __vec128 b) { return vec_all_lt((__u8_vec16)a, (__u8_vec16)b) != 0; }
__forceinline bool __altivec_vcmpleub_all(__vec128 a, __vec128 b) { return vec_all_le((__u8_vec16)a, (__u8_vec16)b) != 0; }
__forceinline bool __altivec_vcmpgtub_all(__vec128 a, __vec128 b) { return vec_all_gt((__u8_vec16)a, (__u8_vec16)b) != 0; }
__forceinline bool __altivec_vcmpgeub_all(__vec128 a, __vec128 b) { return vec_all_ge((__u8_vec16)a, (__u8_vec16)b) != 0; }
__forceinline bool __altivec_vcmpequb_all(__vec128 a, __vec128 b) { return vec_all_eq((__u8_vec16)a, (__u8_vec16)b) != 0; }
__forceinline bool __altivec_vcmpneub_all(__vec128 a, __vec128 b) { return vec_all_ne((__u8_vec16)a, (__u8_vec16)b) != 0; }

__forceinline bool __altivec_vcmpltub_any(__vec128 a, __vec128 b) { return vec_any_lt((__u8_vec16)a, (__u8_vec16)b) != 0; }
__forceinline bool __altivec_vcmpleub_any(__vec128 a, __vec128 b) { return vec_any_le((__u8_vec16)a, (__u8_vec16)b) != 0; }
__forceinline bool __altivec_vcmpgtub_any(__vec128 a, __vec128 b) { return vec_any_gt((__u8_vec16)a, (__u8_vec16)b) != 0; }
__forceinline bool __altivec_vcmpgeub_any(__vec128 a, __vec128 b) { return vec_any_ge((__u8_vec16)a, (__u8_vec16)b) != 0; }
__forceinline bool __altivec_vcmpequb_any(__vec128 a, __vec128 b) { return vec_any_eq((__u8_vec16)a, (__u8_vec16)b) != 0; }
__forceinline bool __altivec_vcmpneub_any(__vec128 a, __vec128 b) { return vec_any_ne((__u8_vec16)a, (__u8_vec16)b) != 0; }

// s8
__forceinline bool __altivec_vcmpltsb_all(__vec128 a, __vec128 b) { return vec_all_lt((__s8_vec16)a, (__s8_vec16)b) != 0; }
__forceinline bool __altivec_vcmplesb_all(__vec128 a, __vec128 b) { return vec_all_le((__s8_vec16)a, (__s8_vec16)b) != 0; }
__forceinline bool __altivec_vcmpgtsb_all(__vec128 a, __vec128 b) { return vec_all_gt((__s8_vec16)a, (__s8_vec16)b) != 0; }
__forceinline bool __altivec_vcmpgesb_all(__vec128 a, __vec128 b) { return vec_all_ge((__s8_vec16)a, (__s8_vec16)b) != 0; }
__forceinline bool __altivec_vcmpeqsb_all(__vec128 a, __vec128 b) { return vec_all_eq((__s8_vec16)a, (__s8_vec16)b) != 0; }
__forceinline bool __altivec_vcmpnesb_all(__vec128 a, __vec128 b) { return vec_all_ne((__s8_vec16)a, (__s8_vec16)b) != 0; }

__forceinline bool __altivec_vcmpltsb_any(__vec128 a, __vec128 b) { return vec_any_lt((__s8_vec16)a, (__s8_vec16)b) != 0; }
__forceinline bool __altivec_vcmplesb_any(__vec128 a, __vec128 b) { return vec_any_le((__s8_vec16)a, (__s8_vec16)b) != 0; }
__forceinline bool __altivec_vcmpgtsb_any(__vec128 a, __vec128 b) { return vec_any_gt((__s8_vec16)a, (__s8_vec16)b) != 0; }
__forceinline bool __altivec_vcmpgesb_any(__vec128 a, __vec128 b) { return vec_any_ge((__s8_vec16)a, (__s8_vec16)b) != 0; }
__forceinline bool __altivec_vcmpeqsb_any(__vec128 a, __vec128 b) { return vec_any_eq((__s8_vec16)a, (__s8_vec16)b) != 0; }
__forceinline bool __altivec_vcmpnesb_any(__vec128 a, __vec128 b) { return vec_any_ne((__s8_vec16)a, (__s8_vec16)b) != 0; }

// u16
__forceinline bool __altivec_vcmpltuh_all(__vec128 a, __vec128 b) { return vec_all_lt((__u16_vec8)a, (__u16_vec8)b) != 0; }
__forceinline bool __altivec_vcmpleuh_all(__vec128 a, __vec128 b) { return vec_all_le((__u16_vec8)a, (__u16_vec8)b) != 0; }
__forceinline bool __altivec_vcmpgtuh_all(__vec128 a, __vec128 b) { return vec_all_gt((__u16_vec8)a, (__u16_vec8)b) != 0; }
__forceinline bool __altivec_vcmpgeuh_all(__vec128 a, __vec128 b) { return vec_all_ge((__u16_vec8)a, (__u16_vec8)b) != 0; }
__forceinline bool __altivec_vcmpequh_all(__vec128 a, __vec128 b) { return vec_all_eq((__u16_vec8)a, (__u16_vec8)b) != 0; }
__forceinline bool __altivec_vcmpneuh_all(__vec128 a, __vec128 b) { return vec_all_ne((__u16_vec8)a, (__u16_vec8)b) != 0; }

__forceinline bool __altivec_vcmpltuh_any(__vec128 a, __vec128 b) { return vec_any_lt((__u16_vec8)a, (__u16_vec8)b) != 0; }
__forceinline bool __altivec_vcmpleuh_any(__vec128 a, __vec128 b) { return vec_any_le((__u16_vec8)a, (__u16_vec8)b) != 0; }
__forceinline bool __altivec_vcmpgtuh_any(__vec128 a, __vec128 b) { return vec_any_gt((__u16_vec8)a, (__u16_vec8)b) != 0; }
__forceinline bool __altivec_vcmpgeuh_any(__vec128 a, __vec128 b) { return vec_any_ge((__u16_vec8)a, (__u16_vec8)b) != 0; }
__forceinline bool __altivec_vcmpequh_any(__vec128 a, __vec128 b) { return vec_any_eq((__u16_vec8)a, (__u16_vec8)b) != 0; }
__forceinline bool __altivec_vcmpneuh_any(__vec128 a, __vec128 b) { return vec_any_ne((__u16_vec8)a, (__u16_vec8)b) != 0; }

// s16
__forceinline bool __altivec_vcmpltsh_all(__vec128 a, __vec128 b) { return vec_all_lt((__s16_vec8)a, (__s16_vec8)b) != 0; }
__forceinline bool __altivec_vcmplesh_all(__vec128 a, __vec128 b) { return vec_all_le((__s16_vec8)a, (__s16_vec8)b) != 0; }
__forceinline bool __altivec_vcmpgtsh_all(__vec128 a, __vec128 b) { return vec_all_gt((__s16_vec8)a, (__s16_vec8)b) != 0; }
__forceinline bool __altivec_vcmpgesh_all(__vec128 a, __vec128 b) { return vec_all_ge((__s16_vec8)a, (__s16_vec8)b) != 0; }
__forceinline bool __altivec_vcmpeqsh_all(__vec128 a, __vec128 b) { return vec_all_eq((__s16_vec8)a, (__s16_vec8)b) != 0; }
__forceinline bool __altivec_vcmpnesh_all(__vec128 a, __vec128 b) { return vec_all_ne((__s16_vec8)a, (__s16_vec8)b) != 0; }

__forceinline bool __altivec_vcmpltsh_any(__vec128 a, __vec128 b) { return vec_any_lt((__s16_vec8)a, (__s16_vec8)b) != 0; }
__forceinline bool __altivec_vcmplesh_any(__vec128 a, __vec128 b) { return vec_any_le((__s16_vec8)a, (__s16_vec8)b) != 0; }
__forceinline bool __altivec_vcmpgtsh_any(__vec128 a, __vec128 b) { return vec_any_gt((__s16_vec8)a, (__s16_vec8)b) != 0; }
__forceinline bool __altivec_vcmpgesh_any(__vec128 a, __vec128 b) { return vec_any_ge((__s16_vec8)a, (__s16_vec8)b) != 0; }
__forceinline bool __altivec_vcmpeqsh_any(__vec128 a, __vec128 b) { return vec_any_eq((__s16_vec8)a, (__s16_vec8)b) != 0; }
__forceinline bool __altivec_vcmpnesh_any(__vec128 a, __vec128 b) { return vec_any_ne((__s16_vec8)a, (__s16_vec8)b) != 0; }

// u32
__forceinline bool __altivec_vcmpltuw_all(__vec128 a, __vec128 b) { return vec_all_lt((__u32_vec4)a, (__u32_vec4)b) != 0; }
__forceinline bool __altivec_vcmpleuw_all(__vec128 a, __vec128 b) { return vec_all_le((__u32_vec4)a, (__u32_vec4)b) != 0; }
__forceinline bool __altivec_vcmpgtuw_all(__vec128 a, __vec128 b) { return vec_all_gt((__u32_vec4)a, (__u32_vec4)b) != 0; }
__forceinline bool __altivec_vcmpgeuw_all(__vec128 a, __vec128 b) { return vec_all_ge((__u32_vec4)a, (__u32_vec4)b) != 0; }
__forceinline bool __altivec_vcmpequw_all(__vec128 a, __vec128 b) { return vec_all_eq((__u32_vec4)a, (__u32_vec4)b) != 0; }
__forceinline bool __altivec_vcmpneuw_all(__vec128 a, __vec128 b) { return vec_all_ne((__u32_vec4)a, (__u32_vec4)b) != 0; }

__forceinline bool __altivec_vcmpltuw_any(__vec128 a, __vec128 b) { return vec_any_lt((__u32_vec4)a, (__u32_vec4)b) != 0; }
__forceinline bool __altivec_vcmpleuw_any(__vec128 a, __vec128 b) { return vec_any_le((__u32_vec4)a, (__u32_vec4)b) != 0; }
__forceinline bool __altivec_vcmpgtuw_any(__vec128 a, __vec128 b) { return vec_any_gt((__u32_vec4)a, (__u32_vec4)b) != 0; }
__forceinline bool __altivec_vcmpgeuw_any(__vec128 a, __vec128 b) { return vec_any_ge((__u32_vec4)a, (__u32_vec4)b) != 0; }
__forceinline bool __altivec_vcmpequw_any(__vec128 a, __vec128 b) { return vec_any_eq((__u32_vec4)a, (__u32_vec4)b) != 0; }
__forceinline bool __altivec_vcmpneuw_any(__vec128 a, __vec128 b) { return vec_any_ne((__u32_vec4)a, (__u32_vec4)b) != 0; }

// s32
__forceinline bool __altivec_vcmpltsw_all(__vec128 a, __vec128 b) { return vec_all_lt((__s32_vec4)a, (__s32_vec4)b) != 0; }
__forceinline bool __altivec_vcmplesw_all(__vec128 a, __vec128 b) { return vec_all_le((__s32_vec4)a, (__s32_vec4)b) != 0; }
__forceinline bool __altivec_vcmpgtsw_all(__vec128 a, __vec128 b) { return vec_all_gt((__s32_vec4)a, (__s32_vec4)b) != 0; }
__forceinline bool __altivec_vcmpgesw_all(__vec128 a, __vec128 b) { return vec_all_ge((__s32_vec4)a, (__s32_vec4)b) != 0; }
__forceinline bool __altivec_vcmpeqsw_all(__vec128 a, __vec128 b) { return vec_all_eq((__s32_vec4)a, (__s32_vec4)b) != 0; }
__forceinline bool __altivec_vcmpnesw_all(__vec128 a, __vec128 b) { return vec_all_ne((__s32_vec4)a, (__s32_vec4)b) != 0; }

__forceinline bool __altivec_vcmpltsw_any(__vec128 a, __vec128 b) { return vec_any_lt((__s32_vec4)a, (__s32_vec4)b) != 0; }
__forceinline bool __altivec_vcmplesw_any(__vec128 a, __vec128 b) { return vec_any_le((__s32_vec4)a, (__s32_vec4)b) != 0; }
__forceinline bool __altivec_vcmpgtsw_any(__vec128 a, __vec128 b) { return vec_any_gt((__s32_vec4)a, (__s32_vec4)b) != 0; }
__forceinline bool __altivec_vcmpgesw_any(__vec128 a, __vec128 b) { return vec_any_ge((__s32_vec4)a, (__s32_vec4)b) != 0; }
__forceinline bool __altivec_vcmpeqsw_any(__vec128 a, __vec128 b) { return vec_any_eq((__s32_vec4)a, (__s32_vec4)b) != 0; }
__forceinline bool __altivec_vcmpnesw_any(__vec128 a, __vec128 b) { return vec_any_ne((__s32_vec4)a, (__s32_vec4)b) != 0; }

// f32
__forceinline bool __altivec_vcmpltfp_all(__vec128 a, __vec128 b) { return vec_all_lt((__f32_vec4)a, (__f32_vec4)b) != 0; }
__forceinline bool __altivec_vcmplefp_all(__vec128 a, __vec128 b) { return vec_all_le((__f32_vec4)a, (__f32_vec4)b) != 0; }
__forceinline bool __altivec_vcmpgtfp_all(__vec128 a, __vec128 b) { return vec_all_gt((__f32_vec4)a, (__f32_vec4)b) != 0; }
__forceinline bool __altivec_vcmpgefp_all(__vec128 a, __vec128 b) { return vec_all_ge((__f32_vec4)a, (__f32_vec4)b) != 0; }
__forceinline bool __altivec_vcmpeqfp_all(__vec128 a, __vec128 b) { return vec_all_eq((__f32_vec4)a, (__f32_vec4)b) != 0; }
__forceinline bool __altivec_vcmpnefp_all(__vec128 a, __vec128 b) { return vec_all_ne((__f32_vec4)a, (__f32_vec4)b) != 0; }

__forceinline bool __altivec_vcmpltfp_any(__vec128 a, __vec128 b) { return vec_any_lt((__f32_vec4)a, (__f32_vec4)b) != 0; }
__forceinline bool __altivec_vcmplefp_any(__vec128 a, __vec128 b) { return vec_any_le((__f32_vec4)a, (__f32_vec4)b) != 0; }
__forceinline bool __altivec_vcmpgtfp_any(__vec128 a, __vec128 b) { return vec_any_gt((__f32_vec4)a, (__f32_vec4)b) != 0; }
__forceinline bool __altivec_vcmpgefp_any(__vec128 a, __vec128 b) { return vec_any_ge((__f32_vec4)a, (__f32_vec4)b) != 0; }
__forceinline bool __altivec_vcmpeqfp_any(__vec128 a, __vec128 b) { return vec_any_eq((__f32_vec4)a, (__f32_vec4)b) != 0; }
__forceinline bool __altivec_vcmpnefp_any(__vec128 a, __vec128 b) { return vec_any_ne((__f32_vec4)a, (__f32_vec4)b) != 0; }

// bounds
__forceinline bool __altivec_vcmpbfp_all_in (__vec128 a, __vec128 b) { return vec_all_in ((__f32_vec4)a, (__f32_vec4)b) != 0; }
//__forceinline bool __altivec_vcmpbfp_all_out(__vec128 a, __vec128 b) { return vec_all_out((__f32_vec4)a, (__f32_vec4)b) != 0; }
//__forceinline bool __altivec_vcmpbfp_any_in (__vec128 a, __vec128 b) { return vec_any_in ((__f32_vec4)a, (__f32_vec4)b) != 0; }
__forceinline bool __altivec_vcmpbfp_any_out(__vec128 a, __vec128 b) { return vec_any_out((__f32_vec4)a, (__f32_vec4)b) != 0; }

#endif // not RSG_XENON

#endif // RSG_CPU_PPC

#endif // _MATH_ALTIVEC2_H_
