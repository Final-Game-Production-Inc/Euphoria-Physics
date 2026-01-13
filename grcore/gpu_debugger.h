/* SCE CONFIDENTIAL
 * $PSLibId$
 * Copyright (C) 2014 Sony Computer Entertainment Inc.
 * All Rights Reserved.
 */

#ifndef _SCE_GPU_DEBUGGER_H
#define _SCE_GPU_DEBUGGER_H

// ------------------------------------------------------------------------------------------------/

#ifdef GPU_DEBUGGER_LIBRARY_IMPL
#define GPU_DEBUGGER_PRX_INTERFACE  __declspec(dllexport)
#else
#define GPU_DEBUGGER_PRX_INTERFACE  __declspec(dllimport)
#endif

// ------------------------------------------------------------------------------------------------/

#include <stdint.h>

// ------------------------------------------------------------------------------------------------/

typedef int32_t SceGpuDebuggerErrorCode;

#if !defined(SCE_OK)
#define SCE_OK (0)
#endif

#define SCE_GPU_DEBUGGER_ERROR_SHADER_ALREADY_REGISTERED -2128871423	/* 0x811C0001 */
#define SCE_GPU_DEBUGGER_ERROR_SHADER_DOESNT_EXIST -2128871422	/* 0x811C0002 */
#define SCE_GPU_DEBUGGER_ERROR_SHADER_DOESNT_MATCH_EXPECTED_TYPE -2128871421	/* 0x811C0003 */
#define SCE_GPU_DEBUGGER_ERROR_SHADER_REGISTRATION_FULL			-2128871420	/* 0x811C0004 */
#define SCE_GPU_DEBUGGER_ERROR_INVALID_SHADER	-2128871419	/* 0x811C0005 */
#define SCE_GPU_DEBUGGER_ERROR_INVALID_THREAD_GROUP_SIZE -2128871418	/* 0x811C0006 */
#define SCE_GPU_DEBUGGER_ERROR_UNKNOWN			-2128871169	/* 0x811C00FF */

// ------------------------------------------------------------------------------------------------/

typedef enum SceGpuDebuggerExceptionMask
{
	SCE_GPU_DEBUGGER_EXCEPTION_MASK_NONE							= 0x00000000,

	SCE_GPU_DEBUGGER_EXCEPTION_MASK_INVALID							= 0x00000001,
	SCE_GPU_DEBUGGER_EXCEPTION_MASK_INPUTDENORM						= 0x00000002,
	SCE_GPU_DEBUGGER_EXCEPTION_MASK_DIV0							= 0x00000004,
	SCE_GPU_DEBUGGER_EXCEPTION_MASK_OVERFLOW						= 0x00000008,
	SCE_GPU_DEBUGGER_EXCEPTION_MASK_UNDERFLOW						= 0x00000010,
	SCE_GPU_DEBUGGER_EXCEPTION_MASK_INEXACT							= 0x00000020,
	SCE_GPU_DEBUGGER_EXCEPTION_MASK_INTDIV0							= 0x00000040,
	SCE_GPU_DEBUGGER_EXCEPTION_MASK_MEMVIOL							= 0x00000100,
} SceGpuDebuggerExceptionMask;

// ------------------------------------------------------------------------------------------------/

#ifdef __cplusplus

// ------------------------------------------------------------------------------------------------/

namespace sce
{
	namespace Gnm
	{
		class VsStageRegisters;
		class PsStageRegisters;
		class CsStageRegisters;
		class GsStageRegisters;
		class EsStageRegisters;
		class HsStageRegisters;
		class LsStageRegisters;
	};
};

// ------------------------------------------------------------------------------------------------/
// Register the shader so you can find it in the host-tool
GPU_DEBUGGER_PRX_INTERFACE SceGpuDebuggerErrorCode sceGpuDebuggerRegisterShaderCode( const sce::Gnm::VsStageRegisters* pVsStageRegisters, uint32_t shaderSize, const char* pShaderName );
GPU_DEBUGGER_PRX_INTERFACE SceGpuDebuggerErrorCode sceGpuDebuggerRegisterShaderCode( const sce::Gnm::PsStageRegisters* pPsStageRegisters, uint32_t shaderSize, const char* pShaderName );
GPU_DEBUGGER_PRX_INTERFACE SceGpuDebuggerErrorCode sceGpuDebuggerRegisterShaderCode( const sce::Gnm::CsStageRegisters* pCsStageRegisters, uint32_t shaderSize, const char* pShaderName );
GPU_DEBUGGER_PRX_INTERFACE SceGpuDebuggerErrorCode sceGpuDebuggerRegisterShaderCode( const sce::Gnm::GsStageRegisters* pGsStageRegisters, uint32_t shaderSize, const char* pShaderName );
GPU_DEBUGGER_PRX_INTERFACE SceGpuDebuggerErrorCode sceGpuDebuggerRegisterShaderCode( const sce::Gnm::EsStageRegisters* pEsStageRegisters, uint32_t shaderSize, const char* pShaderName );
GPU_DEBUGGER_PRX_INTERFACE SceGpuDebuggerErrorCode sceGpuDebuggerRegisterShaderCode( const sce::Gnm::HsStageRegisters* pHsStageRegisters, uint32_t shaderSize, const char* pShaderName );
GPU_DEBUGGER_PRX_INTERFACE SceGpuDebuggerErrorCode sceGpuDebuggerRegisterShaderCode( const sce::Gnm::LsStageRegisters* pLsStageRegisters, uint32_t shaderSize, const char* pShaderName );

GPU_DEBUGGER_PRX_INTERFACE SceGpuDebuggerErrorCode sceGpuDebuggerRegisterFetchShaderCode( const void* pFetchShaderInstructions, uint32_t shaderSize, const char* pShaderName );

// Unregister the shader
GPU_DEBUGGER_PRX_INTERFACE SceGpuDebuggerErrorCode sceGpuDebuggerUnregisterShaderCode( const sce::Gnm::VsStageRegisters* pVsStageRegisters );
GPU_DEBUGGER_PRX_INTERFACE SceGpuDebuggerErrorCode sceGpuDebuggerUnregisterShaderCode( const sce::Gnm::PsStageRegisters* pPsStageRegisters );
GPU_DEBUGGER_PRX_INTERFACE SceGpuDebuggerErrorCode sceGpuDebuggerUnregisterShaderCode( const sce::Gnm::CsStageRegisters* pCsStageRegisters );
GPU_DEBUGGER_PRX_INTERFACE SceGpuDebuggerErrorCode sceGpuDebuggerUnregisterShaderCode( const sce::Gnm::GsStageRegisters* pGsStageRegisters );
GPU_DEBUGGER_PRX_INTERFACE SceGpuDebuggerErrorCode sceGpuDebuggerUnregisterShaderCode( const sce::Gnm::EsStageRegisters* pEsStageRegisters );
GPU_DEBUGGER_PRX_INTERFACE SceGpuDebuggerErrorCode sceGpuDebuggerUnregisterShaderCode( const sce::Gnm::HsStageRegisters* pHsStageRegisters );
GPU_DEBUGGER_PRX_INTERFACE SceGpuDebuggerErrorCode sceGpuDebuggerUnregisterShaderCode( const sce::Gnm::LsStageRegisters* pLsStageRegisters );

GPU_DEBUGGER_PRX_INTERFACE SceGpuDebuggerErrorCode sceGpuDebuggerUnregisterFetchShaderCode( const void* pFetchShaderInstructions );

// ------------------------------------------------------------------------------------------------/
// Optionally enable exceptions per shader.  Pass in the ORing together of the enum values above.
GPU_DEBUGGER_PRX_INTERFACE SceGpuDebuggerErrorCode sceGpuDebuggerSetEnabledExceptionsMask( sce::Gnm::VsStageRegisters* pVsStageRegisters, uint32_t enabledExceptionsMask );
GPU_DEBUGGER_PRX_INTERFACE SceGpuDebuggerErrorCode sceGpuDebuggerSetEnabledExceptionsMask( sce::Gnm::PsStageRegisters* pPsStageRegisters, uint32_t enabledExceptionsMask );
GPU_DEBUGGER_PRX_INTERFACE SceGpuDebuggerErrorCode sceGpuDebuggerSetEnabledExceptionsMask( sce::Gnm::CsStageRegisters* pCsStageRegisters, uint32_t enabledExceptionsMask );
GPU_DEBUGGER_PRX_INTERFACE SceGpuDebuggerErrorCode sceGpuDebuggerSetEnabledExceptionsMask( sce::Gnm::GsStageRegisters* pGsStageRegisters, uint32_t enabledExceptionsMask );
GPU_DEBUGGER_PRX_INTERFACE SceGpuDebuggerErrorCode sceGpuDebuggerSetEnabledExceptionsMask( sce::Gnm::EsStageRegisters* pEsStageRegisters, uint32_t enabledExceptionsMask );
GPU_DEBUGGER_PRX_INTERFACE SceGpuDebuggerErrorCode sceGpuDebuggerSetEnabledExceptionsMask( sce::Gnm::HsStageRegisters* pHsStageRegisters, uint32_t enabledExceptionsMask );
GPU_DEBUGGER_PRX_INTERFACE SceGpuDebuggerErrorCode sceGpuDebuggerSetEnabledExceptionsMask( sce::Gnm::LsStageRegisters* pLsStageRegisters, uint32_t enabledExceptionsMask );

GPU_DEBUGGER_PRX_INTERFACE SceGpuDebuggerErrorCode sceGpuDebuggerGetEnabledExceptionsMask( const sce::Gnm::VsStageRegisters* pVsStageRegisters, uint32_t* pEnabledExceptionsMask );
GPU_DEBUGGER_PRX_INTERFACE SceGpuDebuggerErrorCode sceGpuDebuggerGetEnabledExceptionsMask( const sce::Gnm::PsStageRegisters* pPsStageRegisters, uint32_t* pEnabledExceptionsMask );
GPU_DEBUGGER_PRX_INTERFACE SceGpuDebuggerErrorCode sceGpuDebuggerGetEnabledExceptionsMask( const sce::Gnm::CsStageRegisters* pCsStageRegisters, uint32_t* pEnabledExceptionsMask );
GPU_DEBUGGER_PRX_INTERFACE SceGpuDebuggerErrorCode sceGpuDebuggerGetEnabledExceptionsMask( const sce::Gnm::GsStageRegisters* pGsStageRegisters, uint32_t* pEnabledExceptionsMask );
GPU_DEBUGGER_PRX_INTERFACE SceGpuDebuggerErrorCode sceGpuDebuggerGetEnabledExceptionsMask( const sce::Gnm::EsStageRegisters* pEsStageRegisters, uint32_t* pEnabledExceptionsMask );
GPU_DEBUGGER_PRX_INTERFACE SceGpuDebuggerErrorCode sceGpuDebuggerGetEnabledExceptionsMask( const sce::Gnm::HsStageRegisters* pHsStageRegisters, uint32_t* pEnabledExceptionsMask );
GPU_DEBUGGER_PRX_INTERFACE SceGpuDebuggerErrorCode sceGpuDebuggerGetEnabledExceptionsMask( const sce::Gnm::LsStageRegisters* pLsStageRegisters, uint32_t* pEnabledExceptionsMask );

// ------------------------------------------------------------------------------------------------/

#endif

// ------------------------------------------------------------------------------------------------/

#endif /* _SCE_GPU_DEBUGGER_H */
