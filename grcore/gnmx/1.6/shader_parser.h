/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.600.051
* Copyright (C) 2012 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#ifndef _SCE_GNMX_SHADER_PARSER_H
#define _SCE_GNMX_SHADER_PARSER_H

#if !defined(DOXYGEN_IGNORE)
#if defined(__ORBIS__) || defined(SCE_SHADER_BINARY_INTERNAL_STATIC_EXPORT)
#	define SCE_SHADER_BINARY_EXPORT   
#else
#	if defined( SCE_SHADER_BINARY_DLL_EXPORT )
#		define SCE_SHADER_BINARY_EXPORT __declspec( dllexport )
#	else
#		define SCE_SHADER_BINARY_EXPORT __declspec( dllimport )
#	endif
#endif
#endif

#include "grcore/gnmx/shaderbinary.h"

namespace sce
{
	namespace Gnmx
	{
		/** @brief Encapsulates the pointers to a shader's code and its Gnmx shader object.

			@see parseShader(), parseGsShader()
		*/
		class ShaderInfo
		{
		public:

			union
			{
				const void *m_shaderStruct;		///< A pointer to the shader struct -- typeless.

				Gnmx::VsShader *m_vsShader;		///< A pointer to the shader struct -- used for VS shaders.
				Gnmx::PsShader *m_psShader;		///< A pointer to the shader struct -- used for PS shaders.
				Gnmx::CsShader *m_csShader;		///< A pointer to the shader struct -- used for CS shaders.
				Gnmx::LsShader *m_lsShader;		///< A pointer to the shader struct -- used for LS shaders.
				Gnmx::HsShader *m_hsShader;		///< A pointer to the shader struct -- used for HS shaders.
				Gnmx::EsShader *m_esShader;		///< A pointer to the shader struct -- used for ES shaders.
				Gnmx::GsShader *m_gsShader;		///< A pointer to the shader struct -- used for GS shaders.
				Gnmx::CsVsShader *m_csvsShader; ///< A pointer to the shader struct -- used for CS-VS shaders.
			};

			const uint32_t *m_gpuShaderCode;		///< A pointer to the GPU Shader Code which will need to be copied into GPU visible memory.
			uint32_t m_gpuShaderCodeSize;			///< The size of the GPU Shader Code in bytes.
		};


		/** @brief
			Extracts, from an .sb file, a pointer to the shader object, 
			and, for the GPU Shader Code that must be copied to GPU visible memory, a pointer to
			the code and its size in bytes.
			
			@param shaderInfo		Receives the shader information.
			@param data				A pointer to the contents of an .sb file in memory.
			@param shaderType		The type of shader to be parsed. This must not be a Geometry Shader.
		*/
		SCE_SHADER_BINARY_EXPORT void parseShader(ShaderInfo* shaderInfo, const void* data, sce::Gnmx::ShaderType shaderType);

		/** @brief
			Extracts, from an .sb file, shader information regarding the GS shader as well
			as shader information for its associated VS copy shader.  
			
			@param vsShaderInfo		Receives the information for the GS shader.
			@param gsShaderInfo		Receives the information for the VS shader.
			@param data				A pointer to the contents of an .sb file in memory.
		*/
		SCE_SHADER_BINARY_EXPORT void parseGsShader(ShaderInfo* gsShaderInfo, ShaderInfo* vsShaderInfo, const void* data);

		/** @brief

			Extracts, from an .sb file, shader information regarding the CS-VS shader as well
			as shader information for its associated CS compute shader.  

			@param csvsShaderInfo	Receives the information for the CS-VS shader.
			@param csShaderInfo		Receives the information for the CS shader.
			@param data				A pointer to the contents of an .sb file in memory.
		*/
		SCE_SHADER_BINARY_EXPORT void parseCsVsShader(ShaderInfo* csvsShaderInfo, ShaderInfo* csShaderInfo, const void* data);
	}
}

#endif // _SCE_GNMX_SHADER_PARSER_H
