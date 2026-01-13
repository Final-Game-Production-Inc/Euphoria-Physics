/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.600.071
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
																///< <c>Gnmx::ShaderCommonData::m_embeddedConstantBufferSizeInDQW * 16)</c>. Set to 1 indicate it is
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
		 * @brief Defines the constants used for bit packing a VGT_GS_MODE value into a <c>uint8_t</c> VsShader::m_gsModeOrNumInputSemanticsCs value.
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
		 * @sa DrawCommandBuffer::setGsMode()
		 */
		static inline Gnm::GsMode vsShaderGsModeToGnmGsMode(uint8_t vsShaderGsMode)
		{
			uint32_t vgt_gs_mode;
			if (vsShaderGsMode & kVsShaderGsModeG) {
				vgt_gs_mode = (vsShaderGsMode & kVsShaderGsModeGOnChip) ? Gnm::kGsModeEnableOnChip : Gnm::kGsModeEnable;
				vgt_gs_mode |= (vsShaderGsMode & vsShaderGsMode);
				vgt_gs_mode |= (vsShaderGsMode & kVsShaderGsModeGEsPassthru)<<11;
				vgt_gs_mode |= (vsShaderGsMode & kVsShaderGsModeGEsElementInfo)<<13;
				vgt_gs_mode |= (vsShaderGsMode & kVsShaderGsModeGSuppressCuts)<<12;
			} else {
				vgt_gs_mode = (vsShaderGsMode & kVsShaderGsModeMask)>>1;
				vgt_gs_mode |= (vsShaderGsMode & kVsShaderGsModeCPack)<<7;
			}
			return (Gnm::GsMode)vgt_gs_mode;
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
					pass 0 as the shader modifier when calling the Gfxcontext::setVsShader() function.

				@param shaderModifier	The shader modifier value generated by generateVsFetchShaderBuildState().

				@see generateVsFetchShader()
			 */
			void applyFetchShaderModifier(uint32_t shaderModifier)
			{
				m_vsStageRegisters.applyFetchShaderModifier(shaderModifier);
			}

			/** @brief Patches the GPU address of the shader code.

				@param gpuAddress		This address to patch. This must be aligned to a 256-byte boundary.
			 */
			void patchShaderGpuAddress(void *gpuAddress)
			{
				SCE_GNM_VALIDATE((uintptr_t(gpuAddress) & (Gnm::kAlignmentOfShaderInBytes - 1)) == 0, "Shader's gpu address (0x%010llX) needs to be 256 bytes aligned", gpuAddress);
				SCE_GNM_VALIDATE(m_vsStageRegisters.m_spiShaderPgmHiVs == 0xFFFFFFFF, "VsShader gpu address has already been patched.");
				m_vsStageRegisters.m_spiShaderPgmLoVs = static_cast<uint32_t>(uintptr_t(gpuAddress) >>  8);
				m_vsStageRegisters.m_spiShaderPgmHiVs = static_cast<uint32_t>(uintptr_t(gpuAddress) >> 40);
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
			const uint32_t computeSize() const
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
			const uint8_t getVertexOffsetUserRegister() const
			{
				return m_fetchControl & 0xf;
			}
			/** @brief Gets the user register that contains the instance offset.

				@return The index of the register containing the instance offset. A value of 0 indicates no register contains the instance offset.
				*/
			const uint8_t getInstanceOffsetUserRegister() const
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
			Gnmx::ShaderCommonData m_common;				///< The common data for all shader stages

			Gnm::PsStageRegisters  m_psStageRegisters;		///< The data to be loaded into the PS shader stage registers. Please see Gnm::DrawCommandBuffer::setPsShader() for more details.

			uint8_t              m_numInputSemantics;		///< The number of entries in the input semantic table.
			uint8_t              m_reserved[3];				///< Unused

			/** @brief Patches the GPU address of the shader code.

				@param gpuAddress		The address to patch. This must be aligned to a 256-byte boundary.
			 */
			void patchShaderGpuAddress(void *gpuAddress)
			{
				SCE_GNM_VALIDATE((uintptr_t(gpuAddress) & (Gnm::kAlignmentOfShaderInBytes - 1)) == 0, "Shader's gpu address (0x%010llX) needs to be 256 bytes aligned", gpuAddress);
				SCE_GNM_VALIDATE(m_psStageRegisters.m_spiShaderPgmHiPs == 0xFFFFFFFF, "PsShader gpu address has already been patched.");
				m_psStageRegisters.m_spiShaderPgmLoPs = static_cast<uint32_t>(uintptr_t(gpuAddress) >>  8);
				m_psStageRegisters.m_spiShaderPgmHiPs = static_cast<uint32_t>(uintptr_t(gpuAddress) >> 40);
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
			const uint32_t computeSize() const
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

			uint8_t	m_orderedAppendMode;				///< The mode to use when assigning ordered append indices to the dispatched CS wavefronts.
			uint8_t m_dispatchDrawIndexDeallocNumBits;	///< 0 if not DispatchDraw or [1:15] for DispatchDraw. Match index value (<c>0xFFFF & (0xFFFF<<<i>m_dispatchDrawIndexDeallocNumBits</i>)</c>)
			uint8_t m_reserved[2];						///< Reserved.

			/** @brief Patches the GPU address of the shader code.

				@param gpuAddress	The address to patch. This must be aligned to a 256-byte boundary.
			 */
			void patchShaderGpuAddress(void *gpuAddress)
			{
				SCE_GNM_VALIDATE((uintptr_t(gpuAddress) & (Gnm::kAlignmentOfShaderInBytes - 1)) == 0, "Shader's gpu address (0x%010llX) needs to be 256 bytes aligned", gpuAddress);
				SCE_GNM_VALIDATE(m_csStageRegisters.m_computePgmHi == 0xFFFFFFFF, "CsShader gpu address has already been patched.");
				m_csStageRegisters.m_computePgmLo = static_cast<uint32_t>(uintptr_t(gpuAddress) >>  8);
				m_csStageRegisters.m_computePgmHi = static_cast<uint32_t>(uintptr_t(gpuAddress) >> 40);
			}

			/** @brief Gets a pointer to this shader's input usage slot table that immediately follows this shader's structure in memory.

				@return A pointer to this shader's input usage slot table.
				*/
			const Gnm::InputUsageSlot *getInputUsageSlotTable() const { return (const Gnm::InputUsageSlot *)(this+1); }

			/** @brief Computes the total size (in bytes) of the shader binary including this structure and the input usage table.

				@return The total size in bytes of this shader binary and its input usage table.
				*/
			const uint32_t computeSize() const
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
			Gnmx::ShaderCommonData m_common;				///< The common data for all shader stages

			Gnm::LsStageRegisters  m_lsStageRegisters;		///< The data to be loaded into the LS shader stage registers. Please see Gnm::DrawCommandBuffer::setLsShader() for more details.

			uint8_t m_numInputSemantics;					///< The number of entries in the input semantic table.
			uint8_t m_fetchControl;							///< The user registers that receive the vertex and instance offset for use in the fetch shader.
			uint8_t m_reserved[2];							///< Currently unused.

			uint32_t m_lsStride;							///< The stride (in bytes) of LS shader's output in LDS.

			/** @brief Applies the shader modifier to the shader registers.

				@note When applying the shader modifier to the shader using this function,
					pass 0 as the shader modifier to the Gfxcontext::setLsShader() function.

				@param shaderModifier		The shader modifier value generated by generateLsFetchShaderBuildState().

				@see generateLsFetchShader()
			 */
			void applyFetchShaderModifier(uint32_t shaderModifier)
			{
				m_lsStageRegisters.applyFetchShaderModifier(shaderModifier);
			}

			/** @brief Patches the GPU address of the shader code.

				@param gpuAddress			The address to patch. This must be aligned to a 256-byte boundary.
			 */
			void patchShaderGpuAddress(void *gpuAddress)
			{
				SCE_GNM_VALIDATE((uintptr_t(gpuAddress) & (Gnm::kAlignmentOfShaderInBytes - 1)) == 0, "Shader's gpu address (0x%010llX) needs to be 256 bytes aligned", gpuAddress);
				SCE_GNM_VALIDATE(m_lsStageRegisters.m_spiShaderPgmHiLs == 0xFFFFFFFF, "LsShader gpu address has already been patched.");
				m_lsStageRegisters.m_spiShaderPgmLoLs = static_cast<uint32_t>(uintptr_t(gpuAddress) >>  8);
				m_lsStageRegisters.m_spiShaderPgmHiLs = static_cast<uint32_t>(uintptr_t(gpuAddress) >> 40);
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
			const uint32_t computeSize() const
			{
				const uint32_t size = sizeof(LsShader) +
									  sizeof(Gnm::InputUsageSlot) * m_common.m_numInputUsageSlots +
									  sizeof(Gnm::VertexInputSemantic) * m_numInputSemantics;

				return (size + 3) & ~3;
			}

			/** @brief Gets the user register that contains the vertex offset.

				@return The index of the register containing the vertex offset. A value of 0 indicates no register contains the vertex offset.
				*/
			const uint8_t getVertexOffsetUserRegister() const
			{
				return m_fetchControl & 0xf;
			}

			/** @brief Gets the user register that contains the instance offset.

				@return The index of the register containing the instance offset. A value of 0 indicates no register contains the instance offset.
				*/
			const uint8_t getInstanceOffsetUserRegister() const
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
			Gnmx::ShaderCommonData m_common;				///< The common data for all shader stages

			Gnm::HsStageRegisters   m_hsStageRegisters;		///< The data to be loaded into the HS shader stage registers. Please see Gnm::DrawCommandBuffer::setHsShader() for more details.
			Gnm::HullStateConstants m_hullStateConstants;

			/** @brief Patches the GPU address of the shader code.

				@param gpuAddress		The address to patch. Must be aligned to a 256-byte boundary.
			 */
			void patchShaderGpuAddress(void *gpuAddress)
			{
				SCE_GNM_VALIDATE((uintptr_t(gpuAddress) & (Gnm::kAlignmentOfShaderInBytes - 1)) == 0, "Shader's gpu address (0x%010llX) needs to be 256 bytes aligned", gpuAddress);
				SCE_GNM_VALIDATE(m_hsStageRegisters.m_spiShaderPgmHiHs == 0xFFFFFFFF, "HsShader gpu address has already been patched.");
				m_hsStageRegisters.m_spiShaderPgmLoHs = static_cast<uint32_t>(uintptr_t(gpuAddress) >>  8);
				m_hsStageRegisters.m_spiShaderPgmHiHs = static_cast<uint32_t>(uintptr_t(gpuAddress) >> 40);
			}

			/** @brief Gets a pointer to this shader's input usage slot table that immediately follows this shader's structure in memory.

				@return A pointer to this shader's input usage slot table.
				*/
			const Gnm::InputUsageSlot *getInputUsageSlotTable() const { return (const Gnm::InputUsageSlot *)(this+1); }

			/** @brief Computes the total size (in bytes) of the shader binary including this structure and the input usage table.

				@return The total size in bytes of this shader binary and its input usage table.
				*/
			const uint32_t computeSize() const
			{
				const uint32_t size = sizeof(HsShader) +
									  sizeof(Gnm::InputUsageSlot) * m_common.m_numInputUsageSlots;

				return (size + 3) & ~3;
			}

			/** @brief Gets the number of VGPRS needed by the hull shader.

				@return The number of VGPRS needed by the hull shader.
				*/
			const uint32_t getNumVgprs() const
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
			uint8_t		m_reserved[2];					///< Currently unused.
			uint32_t    m_memExportVertexSizeInDWord;	///< The maximum size (in dwords) of a vertex export.

			/** @brief Applies the shader modifier to the shader registers.

				@note When applying the shader modifier to the shader using this function,
				pass 0 as the shader modifier to the Gfxcontext::setEsShader() function.

				@param shaderModifier The shader modifier value generated by generateEsFetchShaderBuildState().

				@see generateEsFetchShader()
			 */
			void applyFetchShaderModifier(uint32_t shaderModifier)
			{
				m_esStageRegisters.applyFetchShaderModifier(shaderModifier);
			}

			/** @brief Patches the GPU address of the shader code.

				@param gpuAddress Address to patch. Must be aligned to a 256-byte boundary.
			 */
			void patchShaderGpuAddress(void *gpuAddress)
			{
				SCE_GNM_VALIDATE((uintptr_t(gpuAddress) & (Gnm::kAlignmentOfShaderInBytes - 1)) == 0, "Shader's gpu address (0x%010llX) needs to be 256 bytes aligned", gpuAddress);
				SCE_GNM_VALIDATE(m_esStageRegisters.m_spiShaderPgmHiEs == 0xFFFFFFFF, "EsShader gpu address has already been patched.");
				m_esStageRegisters.m_spiShaderPgmLoEs = static_cast<uint32_t>(uintptr_t(gpuAddress) >>  8);
				m_esStageRegisters.m_spiShaderPgmHiEs = static_cast<uint32_t>(uintptr_t(gpuAddress) >> 40);
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
			const uint32_t computeSize() const
			{
				const uint32_t size = sizeof(EsShader) +
									  sizeof(Gnm::InputUsageSlot) * m_common.m_numInputUsageSlots +
									  sizeof(Gnm::VertexInputSemantic) * m_numInputSemantics;

				return (size + 3) & ~3;
			}
			/** @brief Gets the user register that contains the vertex offset.

				@return The index of the register containing the vertex offset. A value of 0 indicates no register contains the vertex offset.
				*/
			const uint8_t getVertexOffsetUserRegister() const
			{
				return m_fetchControl & 0xf;
			}
			/** @brief Gets the user register that contains the instance offset.

				@return The index of the register containing the instance offset. A value of 0 indicates no register contains the instance offset.
				*/
			const uint8_t getInstanceOffsetUserRegister() const
			{
				return (m_fetchControl>>4) & 0xf;
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
			Gnmx::ShaderCommonData m_common;					///< The common data for all shader stages

			Gnm::GsStageRegisters m_gsStageRegisters;			///< The data to be loaded into the GS shader stage registers. Please see Gnm::DrawCommandBuffer::setGsShader() for more details.

			uint32_t        m_memExportVertexSizeInDWord[4];	///< The vertex size for each of the four GS output streams in dwords.
			uint32_t        m_maxOutputVertexCount;				///< The maximum number of output vertices per thread.

			/** @brief Patches the GPU address of the shader code.

			@param gsGpuAddress		The GS stage address to patch. This must be aligned to a 256-byte boundary.
			@param vsCopyGpuAddress The VS stage address to patch. This must be aligned to a 256-byte boundary.
			 */
			void patchShaderGpuAddresses(void *gsGpuAddress, void *vsCopyGpuAddress)
			{
				SCE_GNM_VALIDATE((uintptr_t(gsGpuAddress    ) & (Gnm::kAlignmentOfShaderInBytes - 1)) == 0, "GS Shader's gpu address (0x%010llX) needs to be 256 bytes aligned", gsGpuAddress);
				SCE_GNM_VALIDATE((uintptr_t(vsCopyGpuAddress) & (Gnm::kAlignmentOfShaderInBytes - 1)) == 0, "VS Shader's gpu address (0x%010llX) needs to be 256 bytes aligned", vsCopyGpuAddress);
				SCE_GNM_VALIDATE(m_gsStageRegisters.m_spiShaderPgmHiGs == 0xFFFFFFFF, "GsShader gpu address has already been patched.");
				SCE_GNM_VALIDATE(getCopyShader()->m_vsStageRegisters.m_spiShaderPgmHiVs == 0xFFFFFFFF, "GsShader VS copy shader gpu address has already been patched.");

				m_gsStageRegisters.m_spiShaderPgmLoGs = static_cast<uint32_t>(uintptr_t(gsGpuAddress) >>  8);
				m_gsStageRegisters.m_spiShaderPgmHiGs = static_cast<uint32_t>(uintptr_t(gsGpuAddress) >> 40);
				const_cast<Gnmx::VsShader*>(getCopyShader())->patchShaderGpuAddress(vsCopyGpuAddress);
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
			const uint32_t computeSize() const
			{
				const uint32_t size = sizeof(GsShader) +
									  sizeof(Gnm::InputUsageSlot) * m_common.m_numInputUsageSlots +
									  getCopyShader()->computeSize();
				return (size + 3) & ~3;
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
			VsShader m_vsShader;

			/** @brief Applies the shader modifiers to the shader registers.

				@note When applying the shader modifier to the shader using this function,
						please pass 0 as the shader modifier to the setCsVsShader functions

				@param shaderModifierVs The shader modifier value generated by generateVsFetchShaderBuildState().
				@param shaderModifierCs The shader modifier value generated by generateCsFetchShaderBuildState().

				@see generateVsFetchShader(), generateCsFetchShader()
			 */
			void applyFetchShaderModifiers(uint32_t shaderModifierVs, uint32_t shaderModifierCs)
			{
				const_cast<Gnmx::VsShader*>(getVertexShader())->m_vsStageRegisters.applyFetchShaderModifier(shaderModifierVs);
				const_cast<Gnmx::CsShader*>(getComputeShader())->m_csStageRegisters.applyFetchShaderModifier(shaderModifierCs);
			}

			/** @brief Patches the GPU address of the shader code.

				@param vsGpuAddress The VS stage address to patch. This must be aligned to a 256-byte boundary.
				@param csGpuAddress The CS stage address to patch. This must be aligned to a 256-byte boundary.
			 */
			void patchShaderGpuAddresses(void *vsGpuAddress, void *csGpuAddress)
			{
				const_cast<Gnmx::VsShader*>(getVertexShader())->patchShaderGpuAddress(vsGpuAddress);
				const_cast<Gnmx::CsShader*>(getComputeShader())->patchShaderGpuAddress(csGpuAddress);
			}

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
			const uint32_t computeSize() const
			{
				return getVertexShader()->computeSize() + getComputeShader()->computeSize();
			}
		};
	}
}

#endif
