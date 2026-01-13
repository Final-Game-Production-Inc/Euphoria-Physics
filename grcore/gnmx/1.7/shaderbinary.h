/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.700.081
* Copyright (C) 2012 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#ifndef _SCE_GNMX_SHADERBINARY_H
#define _SCE_GNMX_SHADERBINARY_H

#include <gnm/common.h>
#include <gnm/error.h>
#include <gnm/shader.h>
#include "grcore/gnmx/common.h"

namespace sce
{
	namespace Gnmx
	{
		/** @brief Defines the types of shader that can appear in a shader file. */
		typedef enum ShaderType
		{
			kInvalidShader,			///< Invalid or unrecognized shader.
			kVertexShader,			///< VS stage shader
			kPixelShader,			///< PS stage shader.
			kGeometryShader,		///< GS stage shader.
			kComputeShader,			///< CS stage shader.
			kExportShader,			///< ES stage shader.
			kLocalShader,			///< LS stage shader.
			kHullShader,			///< HS stage shader.
			kComputeVertexShader,	///< VS stage shader with embedded CS stage frontend shader.
		} ShaderType;

		const uint32_t kShaderFileHeaderId = 0x72646853; ///< The magic string "Shdr" that appears in every shader file header.

		const uint16_t kShaderMajorVersion = 7; ///< The internal major version of the shader binary format. A non-matching major version means incompatible binaries.
		const uint16_t kShaderMinorVersion = 1; ///< The internal minor version of the shader binary format.

		/** @brief Represents a shader binary file header. */
		class ShaderFileHeader
		{
		public:
			uint32_t        m_fileHeader;			///< File identifier. Should be equal to Gnmx::kShaderFileHeaderId
			uint16_t        m_majorVersion;			///< Major version of the shader binary.
			uint16_t        m_minorVersion;			///< Minor version of the shader binary.
			uint8_t         m_type;					///< Type of shader. Comes from Gnmx::ShaderType.
			uint8_t			m_shaderHeaderSizeInDW;	///< <c>\<Type\>Shader.computeSize()/4</c>. For example, see CsShader::computeSize().
			uint8_t			m_shaderAuxData;		///< A flag that indicates whether shader auxiliary data is present after end of the shader data ( <c>sizeof(ShaderFileHeader) +</c>
																///< <c>m_shaderHeaderSizeInDW * 4 + Gnmx::ShaderCommonData::m_shaderSize +</c>
																///< <c>Gnmx::ShaderCommonData::m_embeddedConstantBufferSizeInDQW * 16)</c>. Set to 1 to indicate it is
			uint8_t         m_reserved0;			///< Must be 0.
			uint32_t        m_reserved1;			///< Must be 0.
		};

		/** @brief Represents the common data shared by all shader variants */
		class ShaderCommonData
		{
		public:
			// Memory Layout:
			// - Shader setup data (starting with ShaderCommonData)
			// - n InputUsage (4 bytes each)
			// - immediateConstants
			uint32_t        m_shaderSize         :23;		   ///< The size of the shader binary code block in bytes.
			uint32_t        m_shaderIsUsingSrt   :1;		   ///< A bitflag that indicates if the shader is using a Shader Resource Table.
			uint32_t        m_numInputUsageSlots :8;           ///< The number of Gnm::InputUsageSlot entries following the main shader structure.
			uint16_t        m_embeddedConstantBufferSizeInDQW; ///< The size of the embedded constant buffer in 16-byte dqwords.
			uint16_t        m_scratchSizeInDWPerThread;        ///< The scratch size required by each thread in 4-byte dwords.
		};



		/**
		 * @brief Defines the constants used for bit packing a <c>VGT_GS_MODE</c> value into a <c>uint8_t</c> VsShader::m_gsModeOrNumInputSemanticsCs value.
		 */
		typedef enum VsShaderGsMode {
			kVsShaderGsModeG =				0x01,
			// if (m_gsMode & kVsShaderGsModeG)
			kVsShaderGsModeGOnChip =		0x02,
			kVsShaderGsModeGEsPassthru =	0x04,
			kVsShaderGsModeGEsElementInfo =	0x08,
			kVsShaderGsModeGCutMode1024 =	0x00,
			kVsShaderGsModeGCutMode512 =	0x10,
			kVsShaderGsModeGCutMode256 =	0x20,
			kVsShaderGsModeGCutMode128 =	0x30,
			kVsShaderGsModeGSuppressCuts =	0x40,
			kVsShaderGsModeGCutModeMask =	0x30,
			kVsShaderGsModeGCutModeShift =	4,
			// if (!(m_gsMode & kVsShaderGsModeG))
			kVsShaderGsModeOff =			0x00,
			kVsShaderGsModeA =				0x02,
			kVsShaderGsModeB =				0x04,
			kVsShaderGsModeC =				0x08,
			kVsShaderGsModeSpriteEn =		0x0A,
			kVsShaderGsModeMask =			0x0E,
			kVsShaderGsModeCPack =			0x10,
		} VsShaderGsMode;


		/**
		 * @brief Converts a <c>uint8_t</c> VsShader::m_gsModeOrNumInputSemanticsCs value into a Gnm::GsMode register value.
		 *
		 *	@param vsShaderGsMode	The VsShader::m_gsModeOrNumInputSemanticsCs value to convert. 
		 *
		 *	@return	The Gnm::GsMode register value.	
		 *
		 * @sa Gnm::DrawCommandBuffer::setGsMode()
		 */
		static inline Gnm::GsMode vsShaderGsModeToGnmGsMode(uint8_t vsShaderGsMode)
		{
			uint32_t vgtGsMode;
			if (vsShaderGsMode & kVsShaderGsModeG) {
				vgtGsMode = (vsShaderGsMode & kVsShaderGsModeGOnChip) ? Gnm::kGsModeEnableOnChip : Gnm::kGsModeEnable;
				vgtGsMode = (vgtGsMode &~0x00000003 /*CUT_MODE*/) | ((vsShaderGsMode & kVsShaderGsModeGCutModeMask)>>kVsShaderGsModeGCutModeShift);
				vgtGsMode |= (vsShaderGsMode & kVsShaderGsModeGEsPassthru)<<11;
				vgtGsMode |= (vsShaderGsMode & kVsShaderGsModeGEsElementInfo)<<13;
				vgtGsMode |= (vsShaderGsMode & kVsShaderGsModeGSuppressCuts)<<12;
			} else {
				vgtGsMode = (vsShaderGsMode & kVsShaderGsModeMask)>>1;
				vgtGsMode |= (vsShaderGsMode & kVsShaderGsModeCPack)<<7;
			}
			return (Gnm::GsMode)vgtGsMode;
		}

		/**
		 * @brief Represents a shader that runs in the VS shader stage.
		 *
		 * Depending on the active shader stages, an instance of this class can contain a vertex shader, a domain shader or an export shader.
		 */
		class SCE_GNMX_EXPORT VsShader
		{
		public:
			Gnmx::ShaderCommonData m_common;			///< The common data for all shader stages.

			Gnm::VsStageRegisters m_vsStageRegisters;	///< The data to be loaded into the VS shader stage registers. Please see Gnm::DrawCommandBuffer::setVsShader() for more information.
			// not used if domain shader => vertex shader

			uint8_t m_numInputSemantics;				///< The number of entries in the input semantic table.
			uint8_t m_numExportSemantics;				///< The number of entries in the export semantic table.
			uint8_t m_gsModeOrNumInputSemanticsCs;		///< Stores a union of Gnmx::VsShaderGsMode values for a VsShader or GsShader::getCopyShader(), which are translated into a Gnm::GsMode constant. For CsVsShader::getVertexShader(), the number of input semantic table entries to use for the CsVsShader::getComputeShader() fetch shader is stored.
			uint8_t m_fetchControl;						///< The user registers that receive vertex and instance offsets for use in the fetch shader.

			/** @brief Applies the shader modifier to the shader registers.
				
				@note When applying the shader modifier to the shader using this function,
					pass 0 as the shader modifier when calling the GfxContext::setVsShader() function.
				
				@param[in] shaderModifier	The shader modifier value generated by generateVsFetchShaderBuildState().

				@see generateVsFetchShader()
			 */
			void applyFetchShaderModifier(uint32_t shaderModifier)
			{
				m_vsStageRegisters.applyFetchShaderModifier(shaderModifier);
			}

			/** @brief Patches the GPU address of the shader code.

				@param[in] gpuAddress		This address to patch. This must be aligned to a 256-byte boundary.
			 */
			void patchShaderGpuAddress(void *gpuAddress)
			{
				SCE_GNM_VALIDATE((uintptr_t(gpuAddress) & (Gnm::kAlignmentOfShaderInBytes - 1)) == 0, "Shader's gpu address (0x%p) needs to be 256 bytes aligned", gpuAddress);
				SCE_GNM_VALIDATE(m_vsStageRegisters.m_spiShaderPgmHiVs == 0xFFFFFFFF, "VsShader gpu address has already been patched.");
				m_vsStageRegisters.m_spiShaderPgmLoVs = static_cast<uint32_t>(uintptr_t(gpuAddress) >>  8);
				m_vsStageRegisters.m_spiShaderPgmHiVs = static_cast<uint32_t>(uintptr_t(gpuAddress) >> 40);
			}

			/** @brief Retrieves the GPU address of the shader code.
				
				@return The address of the shader code.
				*/
			void* getBaseAddress() const
			{
				SCE_GNM_VALIDATE(m_vsStageRegisters.m_spiShaderPgmHiVs != 0xFFFFFFFF, "VsShader gpu address has not yet been patched.");
				return (void*)((((uint64_t)m_vsStageRegisters.m_spiShaderPgmHiVs)<<40) | (((uint64_t)m_vsStageRegisters.m_spiShaderPgmLoVs)<<8)); 
			}

			/** @brief Gets a pointer to this shader's input usage slot table that immediately follows this shader's structure in memory.
				
				@return A pointer to this shader's input usage slot table.
				*/
			const Gnm::InputUsageSlot       *getInputUsageSlotTable() const { return (const Gnm::InputUsageSlot *)(this+1); }

			/** @brief Gets a pointer to this shader's input semantic table that immediately follows the input usage table in memory.
				
				@return A pointer to this shader's input semantic table.
				*/
			const Gnm::VertexInputSemantic  *getInputSemanticTable()  const { return (const Gnm::VertexInputSemantic *)(getInputUsageSlotTable() + m_common.m_numInputUsageSlots); }

			/** @brief Gets a pointer to this shader's export semantic table that immediately follows the input semantic table in memory.
				
				@return A pointer to this shader's export semantic table.
				*/
			const Gnm::VertexExportSemantic *getExportSemanticTable() const { return (const Gnm::VertexExportSemantic *)(getInputSemanticTable() + m_numInputSemantics); }

			/** @brief Computes the total size (in bytes) of the shader binary including this structure, the input usage table, and the input and export semantic tables.
				
				@return The total size in bytes of this shader binary and its associated tables.
				*/
			uint32_t computeSize() const
			{
				const uint32_t size = sizeof(VsShader) +
									  sizeof(Gnm::InputUsageSlot) * m_common.m_numInputUsageSlots +
									  sizeof(Gnm::VertexInputSemantic) * m_numInputSemantics +
									  sizeof(Gnm::VertexExportSemantic) * m_numExportSemantics;

				return (size + 3) & ~3;
			}
			/** @brief Gets the user register that contains the vertex offset.

				@return The index of the register containing the vertex offset. A value of 0 indicates no register contains the vertex offset. 
				*/
			uint8_t getVertexOffsetUserRegister() const
			{
				return m_fetchControl & 0xf;
			}
			/** @brief Gets the user register that contains the instance offset.

				@return The index of the register containing the instance offset. A value of 0 indicates no register contains the instance offset. 
				*/
			uint8_t getInstanceOffsetUserRegister() const
			{
				return (m_fetchControl>>4) & 0xf;
			}
		};


		/**
		 * @brief Represents a shader that runs in the PS shader stage.
		 *
		 * An instance of this class always contains a pixel shader.
		 */
		class SCE_GNMX_EXPORT PsShader
		{
		public:
			Gnmx::ShaderCommonData m_common;				///< The common data for all shader stages.

			Gnm::PsStageRegisters  m_psStageRegisters;		///< The data to be loaded into the PS shader stage registers. Please see Gnm::DrawCommandBuffer::setPsShader() for more details.

			uint8_t              m_numInputSemantics;		///< The number of entries in the input semantic table.
			uint8_t              m_reserved[3];				///< Unused

			/** @brief Patches the GPU address of the shader code.
			
				@param[in] gpuAddress		The address to patch. This must be aligned to a 256-byte boundary.
			 */
			void patchShaderGpuAddress(void *gpuAddress)
			{
				SCE_GNM_VALIDATE((uintptr_t(gpuAddress) & (Gnm::kAlignmentOfShaderInBytes - 1)) == 0, "Shader's gpu address (0x%p) needs to be 256 bytes aligned", gpuAddress);
				SCE_GNM_VALIDATE(m_psStageRegisters.m_spiShaderPgmHiPs == 0xFFFFFFFF, "PsShader gpu address has already been patched.");
				m_psStageRegisters.m_spiShaderPgmLoPs = static_cast<uint32_t>(uintptr_t(gpuAddress) >>  8);
				m_psStageRegisters.m_spiShaderPgmHiPs = static_cast<uint32_t>(uintptr_t(gpuAddress) >> 40);
			}

			/** @brief Retrieves the GPU address of the shader code.
				
				@return The address of the shader code.
				*/
			void* getBaseAddress() const
			{
				SCE_GNM_VALIDATE(m_psStageRegisters.m_spiShaderPgmHiPs != 0xFFFFFFFF, "PsShader gpu address has not yet been patched.");
				return (void*)((((uint64_t)m_psStageRegisters.m_spiShaderPgmHiPs)<<40) | (((uint64_t)m_psStageRegisters.m_spiShaderPgmLoPs)<<8)); 
			}

			/** @brief Gets a pointer to this shader's input usage slot table that immediately follows this shader's structure in memory.
				
				@return A pointer to this shader's input usage slot table.
				*/
			const Gnm::InputUsageSlot     *getInputUsageSlotTable()     const { return (const Gnm::InputUsageSlot *)(this+1); }

			/** @brief Gets a pointer to this shader's input semantic table that immediately follows the input usage table in memory.
				
				@return A pointer to this shader's input semantic table.
				*/
			const Gnm::PixelInputSemantic *getPixelInputSemanticTable() const { return (const Gnm::PixelInputSemantic *)(getInputUsageSlotTable() + m_common.m_numInputUsageSlots); }

			/** @brief Computes the total size (in bytes) of the shader binary including this structure, the input usage table and the input semantic table.
				
				@return The total size in bytes of this shader binary and its associated tables.
				*/
			uint32_t computeSize() const
			{
				const uint32_t size = sizeof(PsShader) +
									  sizeof(Gnm::InputUsageSlot) * m_common.m_numInputUsageSlots +
									  sizeof(Gnm::PixelInputSemantic) * m_numInputSemantics;

				return (size + 3) & ~3;
			}
		};


		/**
		 * @brief Represents a shader that runs in the CS shader stage.
		 *
		 * An instance of this class always contains a compute shader.
		 */
		class SCE_GNMX_EXPORT CsShader
		{
		public:
			Gnmx::ShaderCommonData m_common;			///< The common data for all shader stages.

			Gnm::CsStageRegisters m_csStageRegisters;	///< The data to be loaded into the CS shader stage registers. Please see Gnm::DrawCommandBuffer::setCsShader() for more details.

			uint8_t	m_orderedAppendMode;				///< The mode to use when assigning ordered append indices to the dispatched CS wavefronts. Please see Gnm::DispatchOrderedAppendMode.
			uint8_t m_dispatchDrawIndexDeallocNumBits;	///< 0 if not DispatchDraw or [1:15] for DispatchDraw. Match index value (<c>0xFFFF & (0xFFFF << <i>m_dispatchDrawIndexDeallocNumBits</i>)</c>) 
			uint8_t m_reserved[2];						///< Reserved.	

			/** @brief Patches the GPU address of the shader code.

				@param[in] gpuAddress	The address to patch. This must be aligned to a 256-byte boundary.
			 */
			void patchShaderGpuAddress(void *gpuAddress)
			{
				SCE_GNM_VALIDATE((uintptr_t(gpuAddress) & (Gnm::kAlignmentOfShaderInBytes - 1)) == 0, "Shader's gpu address (0x%p) needs to be 256 bytes aligned", gpuAddress);
				SCE_GNM_VALIDATE(m_csStageRegisters.m_computePgmHi == 0xFFFFFFFF, "CsShader gpu address has already been patched.");
				m_csStageRegisters.m_computePgmLo = static_cast<uint32_t>(uintptr_t(gpuAddress) >>  8);
				m_csStageRegisters.m_computePgmHi = static_cast<uint32_t>(uintptr_t(gpuAddress) >> 40);
			}

			/** @brief Retrieves the GPU address of the shader code.

				@return The address of the shader code.
				*/
			void* getBaseAddress() const
			{
				SCE_GNM_VALIDATE(m_csStageRegisters.m_computePgmHi != 0xFFFFFFFF, "CsShader gpu address has not yet been patched.");
				return (void*)((((uint64_t)m_csStageRegisters.m_computePgmHi)<<40) | (((uint64_t)m_csStageRegisters.m_computePgmLo)<<8));
			}

			/** @brief Gets a pointer to this shader's input usage slot table that immediately follows this shader's structure in memory.
				
				@return A pointer to this shader's input usage slot table.
				*/
			const Gnm::InputUsageSlot *getInputUsageSlotTable() const { return (const Gnm::InputUsageSlot *)(this+1); }

			/** @brief Computes the total size (in bytes) of the shader binary including this structure and the input usage table.
				
				@return The total size in bytes of this shader binary and its input usage table.
				*/
			uint32_t computeSize() const
			{
				const uint32_t size =  sizeof(CsShader) +
									   sizeof(Gnm::InputUsageSlot) * this->m_common.m_numInputUsageSlots;
				return size;
			}
		};


		/**
		 * @brief Represents a shader that runs in the LS shader stage.
		 *
		 * If this stage is active, it always contains a vertex shader.
		 */
		class SCE_GNMX_EXPORT LsShader
		{
		public:
			Gnmx::ShaderCommonData m_common;				///< The common data for all shader stages.

			Gnm::LsStageRegisters  m_lsStageRegisters;		///< The data to be loaded into the LS shader stage registers. Please see Gnm::DrawCommandBuffer::setLsShader() for more details.

			uint8_t m_numInputSemantics;					///< The number of entries in the input semantic table.
			uint8_t m_fetchControl;							///< The user registers that receive the vertex and instance offset for use in the fetch shader.
			uint8_t m_reserved[2];							///< Currently unused.

			uint32_t m_lsStride;							///< The stride (in bytes) of LS shader's output in LDS.

			/** @brief Applies the shader modifier to the shader registers.

				@note When applying the shader modifier to the shader using this function,
					pass 0 as the shader modifier to the Gfxcontext::setLsShader() function.

				@param[in] shaderModifier		The shader modifier value generated by generateLsFetchShaderBuildState().
				
				@see generateLsFetchShader()
			 */
			void applyFetchShaderModifier(uint32_t shaderModifier)
			{
				m_lsStageRegisters.applyFetchShaderModifier(shaderModifier);
			}

			/** @brief Patches the GPU address of the shader code.
				
				@param[in] gpuAddress			The address to patch. This must be aligned to a 256-byte boundary.
			 */
			void patchShaderGpuAddress(void *gpuAddress)
			{
				SCE_GNM_VALIDATE((uintptr_t(gpuAddress) & (Gnm::kAlignmentOfShaderInBytes - 1)) == 0, "Shader's gpu address (0x%p) needs to be 256 bytes aligned", gpuAddress);
				SCE_GNM_VALIDATE(m_lsStageRegisters.m_spiShaderPgmHiLs == 0xFFFFFFFF, "LsShader gpu address has already been patched.");
				m_lsStageRegisters.m_spiShaderPgmLoLs = static_cast<uint32_t>(uintptr_t(gpuAddress) >>  8);
				m_lsStageRegisters.m_spiShaderPgmHiLs = static_cast<uint32_t>(uintptr_t(gpuAddress) >> 40);
			}

			/** @brief Retrieves the GPU address of the shader code.
				
				@return The address of the shader code.
				*/
			void* getBaseAddress() const
			{
				SCE_GNM_VALIDATE(m_lsStageRegisters.m_spiShaderPgmHiLs != 0xFFFFFFFF, "CsShader gpu address has not yet been patched.");
				return (void*)((((uint64_t)m_lsStageRegisters.m_spiShaderPgmHiLs)<<40) | (((uint64_t)m_lsStageRegisters.m_spiShaderPgmLoLs)<<8));
			}

			/** @brief Gets a pointer to this shader's input usage slot table that immediately follows this shader's structure in memory.
				
				@return A pointer to this shader's input usage slot table.
				*/
			const Gnm::InputUsageSlot       *getInputUsageSlotTable() const { return (const Gnm::InputUsageSlot *)(this+1); }

			/** @brief Gets a pointer to this shader's input semantic table that immediately follows the input usage table in memory.
				
				@return A pointer to this shader's input semantic table.
				*/
			const Gnm::VertexInputSemantic  *getInputSemanticTable()  const { return (const Gnm::VertexInputSemantic *)(getInputUsageSlotTable() + m_common.m_numInputUsageSlots); }

			/** @brief Computes the total size (in bytes) of the shader binary including this structure, the input usage table and the input semantic table.
				
				@return The total size in bytes of this shader binary and all associated tables.
				*/
			uint32_t computeSize() const
			{
				const uint32_t size = sizeof(LsShader) +
									  sizeof(Gnm::InputUsageSlot) * m_common.m_numInputUsageSlots +
									  sizeof(Gnm::VertexInputSemantic) * m_numInputSemantics;

				return (size + 3) & ~3;
			}

			/** @brief Gets the user register that contains the vertex offset.

				@return The index of the register containing the vertex offset. A value of 0 indicates no register contains the vertex offset. 
				*/
			uint8_t getVertexOffsetUserRegister() const
			{
				return m_fetchControl & 0xf;
			}

			/** @brief Gets the user register that contains the instance offset.

				@return The index of the register containing the instance offset. A value of 0 indicates no register contains the instance offset. 
				*/
			uint8_t getInstanceOffsetUserRegister() const
			{
				return (m_fetchControl>>4) & 0xf;
			}
		};



		/**
		 * @brief Represents a shader that runs in the HS shader stage.
		 *
		 * If this stage is active, it always contains a hull shader.
		 */
		class SCE_GNMX_EXPORT HsShader
		{
		public:
			Gnmx::ShaderCommonData m_common;				///< The common data for all shader stages.

			Gnm::HsStageRegisters   m_hsStageRegisters;		///< The data to be loaded into the HS shader stage registers. Please see Gnm::DrawCommandBuffer::setHsShader() for more details.
			Gnm::HullStateConstants m_hullStateConstants;

			/** @brief Patches the GPU address of the shader code.
			
				@param[in] gpuAddress		The address to patch. Must be aligned to a 256-byte boundary.
			 */
			void patchShaderGpuAddress(void *gpuAddress)
			{
				SCE_GNM_VALIDATE((uintptr_t(gpuAddress) & (Gnm::kAlignmentOfShaderInBytes - 1)) == 0, "Shader's gpu address (0x%p) needs to be 256 bytes aligned", gpuAddress);
				SCE_GNM_VALIDATE(m_hsStageRegisters.m_spiShaderPgmHiHs == 0xFFFFFFFF, "HsShader gpu address has already been patched.");
				m_hsStageRegisters.m_spiShaderPgmLoHs = static_cast<uint32_t>(uintptr_t(gpuAddress) >>  8);
				m_hsStageRegisters.m_spiShaderPgmHiHs = static_cast<uint32_t>(uintptr_t(gpuAddress) >> 40);
			}

			/** @brief Retrieves the GPU address of the shader code.

				@return The address of the shader code.
				*/
			void* getBaseAddress() const
			{
				SCE_GNM_VALIDATE(m_hsStageRegisters.m_spiShaderPgmHiHs != 0xFFFFFFFF, "CsShader gpu address has not yet been patched.");
				return (void*)((((uint64_t)m_hsStageRegisters.m_spiShaderPgmHiHs)<<40) | (((uint64_t)m_hsStageRegisters.m_spiShaderPgmLoHs)<<8));
			}

			/** @brief Gets a pointer to this shader's input usage slot table that immediately follows this shader's structure in memory.
				
				@return A pointer to this shader's input usage slot table.
				*/
			const Gnm::InputUsageSlot *getInputUsageSlotTable() const { return (const Gnm::InputUsageSlot *)(this+1); }

			/** @brief Computes the total size (in bytes) of the shader binary including this structure and the input usage table.
				
				@return The total size in bytes of this shader binary and its input usage table.
				*/
			uint32_t computeSize() const
			{
				const uint32_t size = sizeof(HsShader) +
									  sizeof(Gnm::InputUsageSlot) * m_common.m_numInputUsageSlots;

				return (size + 3) & ~3;
			}

			/** @brief Gets the number of VGPRS needed by the hull shader.
				
				@return The number of VGPRS needed by the hull shader.
				*/
			uint32_t getNumVgprs() const
			{
				return m_hsStageRegisters.getNumVgprs();
			}
		};



		/**
		 * @brief Represents a shader that runs in the ES shader stage.
		 *
		 * Depending on the active shader stages, an instance of this class contains a vertex shader or a domain shader.
		 */
		class SCE_GNMX_EXPORT EsShader
		{
		public:
			Gnmx::ShaderCommonData m_common;			///< The common data for all shader stages.

			Gnm::EsStageRegisters m_esStageRegisters;	///< The data to be loaded into the ES shader stage registers. Please see Gnm::DrawCommandBuffer::setEsShader() for more details.

			uint8_t		m_numInputSemantics;			///< The number of entries in the input semantic table.
			uint8_t		m_fetchControl;					///< The user registers that receive vertex and instance offsets for use in the fetch shader.
			uint16_t	m_isOnChip	: 1;				///< Is this is an ES shader for use with on-chip-GS?
			uint16_t	m_reserved	:15;				///< Currently unused.
			uint32_t    m_memExportVertexSizeInDWord;	///< The maximum size (in dwords) of a vertex export [0:128].

			/** @brief Applies the shader modifier to the shader registers.
				
				@note When applying the shader modifier to the shader using this function,
				pass 0 as the shader modifier to the GfxContext::setEsShader() function.
				
				@param[in] shaderModifier The shader modifier value generated by generateEsFetchShaderBuildState().

				@see generateEsFetchShader()
			 */
			void applyFetchShaderModifier(uint32_t shaderModifier)
			{
				m_esStageRegisters.applyFetchShaderModifier(shaderModifier);
			}

			/** @brief Updates the LDS allocation size in the shader registers.

				On-chip-GS ES shaders must have their LDS allocation size set based on the GsShader::getSizePerPrimitiveInBytes() and
				the desired number of GS input primitives per GS sub-group, typically chosen to maximize thread utilization within the
				limits placed by the total available LDS.
				
				@param[in] ldsSizeIn512Byte The LDS allocation size in 512-byte granularity allocation units.
			 */
			void updateLdsSize(uint32_t ldsSizeIn512Byte)
			{
				SCE_GNM_VALIDATE(m_isOnChip != 0, "Only on-chip-GS ES shaders should be patched with an LDS allocation size");
				m_esStageRegisters.updateLdsSize(ldsSizeIn512Byte);
			}

			/** @brief Patches the GPU address of the shader code.

				@param[in] gpuAddress Address to patch. Must be aligned to a 256-byte boundary.
			 */
			void patchShaderGpuAddress(void *gpuAddress)
			{
				SCE_GNM_VALIDATE((uintptr_t(gpuAddress) & (Gnm::kAlignmentOfShaderInBytes - 1)) == 0, "Shader's gpu address (0x%p) needs to be 256 bytes aligned", gpuAddress);
				SCE_GNM_VALIDATE(m_esStageRegisters.m_spiShaderPgmHiEs == 0xFFFFFFFF, "EsShader gpu address has already been patched.");
				m_esStageRegisters.m_spiShaderPgmLoEs = static_cast<uint32_t>(uintptr_t(gpuAddress) >>  8);
				m_esStageRegisters.m_spiShaderPgmHiEs = static_cast<uint32_t>(uintptr_t(gpuAddress) >> 40);
			}

			/** @brief Retrieves the GPU address of the shader code.

				@return The address of the shader code.
				*/
			void* getBaseAddress() const
			{
				SCE_GNM_VALIDATE(m_esStageRegisters.m_spiShaderPgmHiEs != 0xFFFFFFFF, "CsShader gpu address has not yet been patched.");
				return (void*)((((uint64_t)m_esStageRegisters.m_spiShaderPgmHiEs)<<40) | (((uint64_t)m_esStageRegisters.m_spiShaderPgmLoEs)<<8));
			}

			/** @brief Gets a pointer to this shader's input usage slot table that immediately follows this shader's structure in memory.
				
				@return A pointer to this shader's input usage slot table.
				*/
			const Gnm::InputUsageSlot       *getInputUsageSlotTable() const { return (const Gnm::InputUsageSlot *)(this+1); }

			/** @brief Gets a pointer to this shader's input semantic table that immediately follows the input usage table in memory.
				
				@return A pointer to this shader's input semantic table.
				*/
			const Gnm::VertexInputSemantic  *getInputSemanticTable()  const { return (const Gnm::VertexInputSemantic *)(getInputUsageSlotTable() + m_common.m_numInputUsageSlots); }

			/** @brief Computes the total size (in bytes) of the shader binary, including this structure, the input usage table and the input semantic table.
				
				@return The total size in bytes of this shader binary and all associated tables.
				*/
			uint32_t computeSize() const
			{
				const uint32_t size = sizeof(EsShader) +
									  sizeof(Gnm::InputUsageSlot) * m_common.m_numInputUsageSlots +
									  sizeof(Gnm::VertexInputSemantic) * m_numInputSemantics;

				return (size + 3) & ~3;
			}
			/** @brief Gets the user register that contains the vertex offset.

				@return The index of the register containing the vertex offset. A value of 0 indicates no register contains the vertex offset. 
				*/
			uint8_t getVertexOffsetUserRegister() const
			{
				return m_fetchControl & 0xf;
			}
			/** @brief Gets the user register that contains the instance offset.

				@return The index of the register containing the instance offset. A value of 0 indicates no register contains the instance offset. 
				*/
			uint8_t getInstanceOffsetUserRegister() const
			{
				return (m_fetchControl>>4) & 0xf;
			}

			/** @brief Returns true if this is an on-chip-GS ES shader, meaning that the ES-GS data is written to LDS, not memory rings.
				@return true if this is an on-chip-GS ES shader, meaning that the ES-GS data is written to LDS, not memory rings, or false if not.
				*/
			bool isOnChip() const
			{
				return (m_isOnChip != 0);
			}
		};



		/**
		 * @brief Represents a shader that runs in the GS shader stage.
		 *
		 * If this stage is active, it always contains a geometry shader.
		 */
		class SCE_GNMX_EXPORT GsShader
		{
		public:
			Gnmx::ShaderCommonData m_common;					///< The common data for all shader stages.

			Gnm::GsStageRegisters m_gsStageRegisters;			///< The data to be loaded into the GS shader stage registers. Please see Gnm::DrawCommandBuffer::setGsShader() for more details.

			uint32_t        m_memExportVertexSizeInDWord[4];	///< The vertex size for each of the four GS output streams in dwords.

			uint32_t	m_maxOutputVertexCount	:11;		///< The maximum number of output vertices each GS thread will write to GS-VS. [0:1024]
			uint32_t	m_inputVertexCountMinus1: 5;		///< The number of input vertices each GS thread will read from ES-GS. [1,2,3,4,6,1:32]-1 for [Point, Line, Triangle, LineAdj, TriangleAdj, Patch1:32] input topology
			uint32_t	m_inputVertexSizeInDWord: 8;		///< The expected size in dwords of input vertices from ES-GS. [0:128], must match EsShader::m_memExportVertexSizeInDWord
			uint32_t	m_isPatchTopology		: 1;		///< This geometry shader expects patch input topology with (m_inputVertexCountMinus1+1) vertices per patch.
			uint32_t	m_reserved				: 7;

			/** @brief Patches the GPU address of the shader code.
			
			@param[in] gsGpuAddress		The GS stage address to patch. This must be aligned to a 256-byte boundary.
			@param[in] vsCopyGpuAddress The VS stage address to patch. This must be aligned to a 256-byte boundary.
			 */
			void patchShaderGpuAddresses(void *gsGpuAddress, void *vsCopyGpuAddress)
			{
				SCE_GNM_VALIDATE((uintptr_t(gsGpuAddress    ) & (Gnm::kAlignmentOfShaderInBytes - 1)) == 0, "GS Shader's gpu address (0x%p) needs to be 256 bytes aligned", gsGpuAddress);
				SCE_GNM_VALIDATE((uintptr_t(vsCopyGpuAddress) & (Gnm::kAlignmentOfShaderInBytes - 1)) == 0, "VS Shader's gpu address (0x%p) needs to be 256 bytes aligned", vsCopyGpuAddress);
				SCE_GNM_VALIDATE(m_gsStageRegisters.m_spiShaderPgmHiGs == 0xFFFFFFFF, "GsShader gpu address has already been patched.");
				SCE_GNM_VALIDATE(getCopyShader()->m_vsStageRegisters.m_spiShaderPgmHiVs == 0xFFFFFFFF, "GsShader VS copy shader gpu address has already been patched.");

				m_gsStageRegisters.m_spiShaderPgmLoGs = static_cast<uint32_t>(uintptr_t(gsGpuAddress) >>  8);
				m_gsStageRegisters.m_spiShaderPgmHiGs = static_cast<uint32_t>(uintptr_t(gsGpuAddress) >> 40);
				const_cast<Gnmx::VsShader*>(getCopyShader())->patchShaderGpuAddress(vsCopyGpuAddress);
			}

			/** @brief Retrieves the GPU address of the geometry shader code.

				@note Use getCopyShader()->getBaseAddress() to retrieve the GPU address of the embedded VS copy shader code.
			 
				@return The address of the GS shader code.
			  */
			void* getBaseAddress() const
			{
				SCE_GNM_VALIDATE(m_gsStageRegisters.m_spiShaderPgmHiGs != 0xFFFFFFFF, "CsShader gpu address has not yet been patched.");
				return (void*)((((uint64_t)m_gsStageRegisters.m_spiShaderPgmHiGs)<<40) | (((uint64_t)m_gsStageRegisters.m_spiShaderPgmLoGs)<<8));
			}

			/** @brief Gets a pointer to this shader's input usage slot table, which immediately follows this shader's structure in memory.
				
				@return A pointer to this shader's input usage slot table.
				*/
			const Gnm::InputUsageSlot *getInputUsageSlotTable() const { return (const Gnm::InputUsageSlot *)(this+1); }

			/** @brief Retrieves a pointer to the copy shader which should be bound to the VS stage when this GsShader is bound.
				
				@return A pointer to the copy shader that should be bound to the VS stage when this GsShader is bound.
				*/
			const Gnmx::VsShader      *getCopyShader()          const { return (const Gnmx::VsShader *)(getInputUsageSlotTable() + m_common.m_numInputUsageSlots); }

			/** @brief Computes the total size (in bytes) of the shader binary including this structure, the input usage table and the copy shader data.
				
				@return The total size in bytes of this shader binary, its input usage table and the copy shader data.
				*/
			uint32_t computeSize() const
			{
				const uint32_t size = sizeof(GsShader) +
									  sizeof(Gnm::InputUsageSlot) * m_common.m_numInputUsageSlots +
									  getCopyShader()->computeSize();
				return (size + 3) & ~3;
			}

			/** @brief Returns true if this is an on-chip GS shader, meaning that the ES-GS and GS-VS data is transfered via LDS, not memory rings.
				@return true if this is an on-chip GS shader, meaning that the ES-GS and GS-VS data is transfered via LDS, not memory rings, false if not.
				*/
			bool isOnChip() const
			{
				return (getCopyShader()->m_gsModeOrNumInputSemanticsCs & sce::Gnmx::kVsShaderGsModeGOnChip) != 0;
			}

			/** @brief Returns the GsMode max output vertex count setting for this shader.
				@return GsMaxOutputVertexCount value corresponding to the max output vertex count for this shader.
				*/
			sce::Gnm::GsMaxOutputVertexCount getGsMaxOutputVertexCount() const
			{
				if (isOnChip())
					return (sce::Gnm::GsMaxOutputVertexCount)((getCopyShader()->m_gsModeOrNumInputSemanticsCs & kVsShaderGsModeGCutModeMask)>>kVsShaderGsModeGCutModeShift);
				if (m_maxOutputVertexCount <= 128)
					return sce::Gnm::kGsMaxOutputVertexCount128;
				if (m_maxOutputVertexCount <= 256)
					return sce::Gnm::kGsMaxOutputVertexCount256;
				if (m_maxOutputVertexCount <= 512)
					return sce::Gnm::kGsMaxOutputVertexCount512;
				return sce::Gnm::kGsMaxOutputVertexCount1024;
			}

			/** @brief Returns the size in bytes required per GS input primitive (i.e. per GS thread) for ES-GS input data.

				@return The size in bytes required per GS input primitive (i.e. per GS thread) for ES-GS input data.
				*/
			uint32_t getEsGsSizePerPrimitiveInBytes() const
			{
				return (m_inputVertexCountMinus1+1) * m_inputVertexSizeInDWord * 4;
			}

			/** @brief Returns the size in bytes required per GS input primitive (i.e. per GS thread) for GS-VS output data.

				@return The size in bytes required per GS input primitive (i.e. per GS thread) for GS-VS output data.
				*/
			uint32_t getGsVsSizePerPrimitiveInBytes() const
			{
				return m_maxOutputVertexCount * (m_memExportVertexSizeInDWord[0] + m_memExportVertexSizeInDWord[1] + m_memExportVertexSizeInDWord[2] + m_memExportVertexSizeInDWord[3]) * 4;
			}

			/** @brief Returns the total size in bytes required per GS input primitive (i.e. per GS thread) for ES-GS input data plus GS-VS output data.

				@return The total size in bytes required per GS input primitive (i.e. per GS thread) for ES-GS input data plus GS-VS output data.
				*/
			uint32_t getSizePerPrimitiveInBytes() const
			{
				return getEsGsSizePerPrimitiveInBytes() + getGsVsSizePerPrimitiveInBytes();
			}

			/** @brief For an on-chip GS shader, returns the maximum number of GS input primitives that can be run in ldsSizePerSubgroupInBytes of LDS.

				DrawCommandBuffer::setGsOnChipControl(gsPrimsPerSubgroup * (GsShader::m_inputVertexCountMinus1+1), gsPrimsPerSubgroup) must be called to set 
				the size of the on-chip-GS sub-group in ES threads and GS threads.  getOnChipGsPrimsPerSubgroup(64*1024-extraLdsSize) will return the
				maximum possible value of gsPrimsPerSubgroup.  Better performance (thread utilization and compute utilization) may result from 
				limiting gsPrimsPerSubgroup to be no larger than 64 (one full GS wavefront per sub-group).

				The GS and VS shaders also typically need to be notified with the offset of the GS-VS output region in LDS, by setting their
				input usage kShaderInputUsageImmLdsEsGsSize input SGPR to byte offset (gsPrimsPerSubgroup * getEsGsSizePerPrimitiveInBytes()).

				@param[in] ldsSizePerSubgroupInBytes The maximum size in bytes of LDS to allocate per GS sub-group.
				@return The number of GS input primitives (i.e. GS threads) that could be run in ldsSizePerSubgroupInBytes bytes of LDS.
				*/
			uint32_t getOnChipGsPrimsPerSubgroup(uint32_t ldsSizePerSubgroupInBytes) const
			{
				return ldsSizePerSubgroupInBytes / getSizePerPrimitiveInBytes();
			}

			/** @brief For an on-chip GS shader, returns the LDS allocation size required to run gsPrimsPerSubgroup GS threads.

				The ES shader run with this GS shader must be set with this LDS allocation size.

				@param[in] gsPrimsPerSubgroup The number of GS input primitives (i.e. GS threads) to run per sub-group.
				@param[in] extraLdsSize The size in bytes of additional LDS after the ES-GS and GS-VS rings required by the shaders, if any.
				@return The size of the LDS allocation required for this sub-group, which is always a multiple of the 512 byte LDS granularity.
				*/
			uint32_t getOnChipGsLdsAllocationSize(uint32_t gsPrimsPerSubgroup, uint32_t extraLdsSizeInBytes = 0) const
			{
				return (gsPrimsPerSubgroup * getSizePerPrimitiveInBytes() + extraLdsSizeInBytes + 0x1FF) &~0x1FF;
			}
		};

		/**
		 * @brief Represents a shader that runs in the VS shader stage and contains an embedded associated shader to run in the CS shader stage.
		 *
		 * This is used by dispatch draw but may potentially have other uses in the future.
		 */
		class SCE_GNMX_EXPORT CsVsShader
		{
		public:
			VsShader m_vsShader;	///< The bound vertex shader.

			/** @brief Applies the shader modifiers to the shader registers.
				
				@note When applying the shader modifier to the shader using this function,
						please pass 0 as the shader modifier to the setCsVsShader() functions
				
				@param[in] shaderModifierVs The shader modifier value generated by generateVsFetchShaderBuildState().
				@param[in] shaderModifierCs The shader modifier value generated by generateCsFetchShaderBuildState().

				@see generateVsFetchShader(), generateCsFetchShader()
			 */
			void applyFetchShaderModifiers(uint32_t shaderModifierVs, uint32_t shaderModifierCs)
			{
				const_cast<Gnmx::VsShader*>(getVertexShader())->m_vsStageRegisters.applyFetchShaderModifier(shaderModifierVs);
				const_cast<Gnmx::CsShader*>(getComputeShader())->m_csStageRegisters.applyFetchShaderModifier(shaderModifierCs);
			}

			/** @brief Patches the GPU address of the shader code.
			
			@param[in] vsGpuAddress The VS stage address to patch. This must be aligned to a 256-byte boundary.
			@param[in] csGpuAddress The CS stage address to patch. This must be aligned to a 256-byte boundary.
			 */
			void patchShaderGpuAddresses(void *vsGpuAddress, void *csGpuAddress)
			{
				const_cast<Gnmx::VsShader*>(getVertexShader())->patchShaderGpuAddress(vsGpuAddress);
				const_cast<Gnmx::CsShader*>(getComputeShader())->patchShaderGpuAddress(csGpuAddress);
			}

			/** @brief Retrieves the GPU address of the vertex shader code.

				@note Use getComputeShader()->getBaseAddress() to retrieve the GPU address of the compute shader code.
				
				@return The address of the VS shader code.
			  */
			void* getBaseAddress() const { return getVertexShader()->getBaseAddress(); }

			/** @brief Retrieves a pointer to the vertex shader, which should be bound to the VS stage when this CsVsShader is bound. 
			    
				@return Pointer to vertex shader.
			*/
			const Gnmx::VsShader      *getVertexShader()          const { return &m_vsShader; }
			
			/** @brief Retrieves a pointer to the compute shader, which should be bound to the CS stage when this CsVsShader is bound. 
				
				@return Pointer to compute shader.
			*/
			const Gnmx::CsShader      *getComputeShader()          const { return (const Gnmx::CsShader*)((uintptr_t)&m_vsShader + m_vsShader.computeSize()); }

			/** @brief Computes the total size (in bytes) of the shader binary header including this structure, the embedded VsShader and the embedded CsShader.
				
				@return The total size in bytes of this shader binary and the associated embedded shaders.
				*/
			uint32_t computeSize() const
			{
				return getVertexShader()->computeSize() + getComputeShader()->computeSize();
			}
		};
	}
}

#endif
