/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.600.051
* Copyright (C) 2011 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/
#if !defined(_SCE_GNMX_COMMON_H)
#define _SCE_GNMX_COMMON_H


//******************************************************************************
// Note regarding the Constant Update Engine:
//------------------------------------------------------------------------------
// TODO: Preface to explain the issue with existing CUE.
// TODO: Mention that long term there will only be two CUE: the normal CUE (likely CUE_V2) and the LCUE.
// Options:
// - New CUE         -- Enabled using #define CUE_V2 [Currently experimental]
// - LightWeight CUE -- Enabled using #define ...
//******************************************************************************


//
// Uncomment the following #define to use the new ConstantUpdateEngine instead of the existing one:
//

//#define CUE_V2

//
// Note regarding the new Constant Update Engine:
// - This new Constant Update Engine is currently experimental.
//   Its main goal is to fix the CPU performance issue encountered with the original CUE while providing similar services.
// - Once fully tested and fully operational, if successful, it will replace the existing CUE.
// - The new CUE must be linked to a pair (dcb/ccb) right after being initialized
//   Whereas the old CUE only considered these duplex when predraw/postdraw/predispatch/postdispatch function were called.
// - It is highly recommended to have the CCB/DCB command buffers in Onion memory. Otherwise, performance will be greatly reduced.
// - If you are using ring buffers to allocate the command buffers for the DCBs and CCBs, please use EOP to guarantee the safety 
//   of the memory reclamation.  That is, you'll need to make sure that a previously used command buffer is GPU-freed before
//   reallocating it (this includes in-flight draws).
//

/** @brief If defined, lwconstantupdateengine_cuetolcue.h will override the 
 *  default Gnmx::gfxContext with a Lightweight Constant Update Engine replacement (LCUE). All subsequent 
 *  shader resource bindings will be handled by the LCUE. Please see the Gnm Library Overview and 
 *  Reference for more details.
 */
//#define SCE_GNMX_ENABLE_GFX_LCUE

#if !defined(DOXYGEN_IGNORE)
#if defined(__ORBIS__) || defined(SCE_GNMX_LIBRARY_STATIC_EXPORT)
#	define SCE_GNMX_EXPORT   
#else
#	if defined( SCE_GNMX_DLL_EXPORT )
#		define SCE_GNMX_EXPORT __declspec( dllexport )
#	else
#		define SCE_GNMX_EXPORT __declspec( dllimport )
#	endif
#endif
#endif
#endif // _SCE_GNMX_COMMON_H
