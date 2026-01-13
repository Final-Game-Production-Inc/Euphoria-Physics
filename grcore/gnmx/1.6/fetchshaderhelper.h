/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.600.051
* Copyright (C) 2011 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#ifndef _SCE_GNMX_FETCHSHADERHELPER_H
#define _SCE_GNMX_FETCHSHADERHELPER_H

#include "grcore/gnmx/shaderbinary.h"

namespace sce
{
	namespace Gnmx
	{
		/** @brief Computes the size of the fetch shader in bytes for a VS-stage vertex shader.
		 * @param vsb The vertex shader to generate a fetch shader for.
		 *
		 * @return The size of the fetch shader in bytes.
		 *
		 * @see generateVsFetchShader()
		 */
		SCE_GNMX_EXPORT uint32_t computeVsFetchShaderSize(const VsShader *vsb);


		/** @brief Generates the fetch shader for a VS-stage vertex shader.
		 *
		 * This <b>Direct Mapping</b> variant assumes that all vertex buffer slots map directly to the corresponding vertex shader input slots.
		 *
		 * @param fs				Receives the generated fetch shader. The supplied buffer must be at least as large as the size returned by computeVsFetchShaderSize().
		 * @param shaderModifier	Receives a value which will need to be passed to either ConstantUpdateEngine::setVsShader() or VsShader::applyFetchShaderModifier().
		 * @param vsb				A pointer to the vertex shader binary data (this may get modified).
		 * @param instancingData	A pointer to a table describing which index to use to fetch the data for each shader entry. Specify NULL to always default to Vertex Index.
		 *
		 * @see computeVsFetchShaderSize()
		 */
		SCE_GNMX_EXPORT void generateVsFetchShader(void *fs, uint32_t* shaderModifier, const VsShader *vsb, const Gnm::FetchShaderInstancingMode *instancingData);


		/** @brief Generates the fetch shader for a VS-stage vertex shader.
		 * 
		 * This <b>Remapping Table</b> variant allows arbitrary remapping of vertex buffer slots to vertex shader input slots using the provided semantic remapping table.
		 *
		 * @param fs             			Receives the generated fetch shader. The supplied buffer must be at least as large as the size returned by computeVsFetchShaderSize().
		 * @param shaderModifier   			Receives a value which will need to be passed to either ConstantUpdateEngine::setVsShader() or VsShader::applyFetchShaderModifier().
		 * @param vsb                		A pointer to the vertex shader binary data (this may get modified).
		 * @param instancingData			A pointer to a table describing which index to use to fetch the data for each shader entry. Specify NULL to default to always Vertex Index.
		 * @param semanticRemapTable		A pointer to the semantic remapping table to match the vertex shader input with the Vertex Buffer definition. This table contains
		 									one element for each vertex buffer slot that may be bound. Each vertex buffer slot's table entry contains the index of the
		 									vertex shader input that the vertex buffer should map to (or -1 if the buffer is unused).
		 * @param numElementsInRemapTable	The size of the table passed to <c><i>semanticRemapTable</i></c>.
		 */
		SCE_GNMX_EXPORT void generateVsFetchShader(void *fs,
												   uint32_t *shaderModifier,
												   const VsShader *vsb,
												   const Gnm::FetchShaderInstancingMode *instancingData,
												   const void *semanticRemapTable, const uint32_t numElementsInRemapTable);

		/** @brief Computes the size of the fetch shader in bytes for an LS-stage vertex shader.
		 * @param lsb 	The vertex shader to generate a fetch shader for.
		 * @return 			The size of the fetch shader in bytes.
		 * @see generateLsFetchShader()
		 */
		SCE_GNMX_EXPORT uint32_t computeLsFetchShaderSize(const LsShader *lsb);

		/** @brief Generates the Fetch Shader for an LS-stage vertex shader.
		 *
		 * The <b>Direct Mapping</b> variant assumes that all vertex buffer slots map directly to the corresponding vertex shader input slots.
		 *	
 		 * @param fs             Buffer to generate the fetch shader into. Must be at least as large as the size returned by computeLsFetchShaderSize().
		 * @param shaderModifier Output value which will need to be passed to the ConstantUpdateEngine::setLsShader() function.
		 * @param lsb            Pointer to the vertex shader binary data (may get modified).
		 * @see computeLsFetchShaderSize()
		 */
		SCE_GNMX_EXPORT void generateLsFetchShader(void *fs, uint32_t *shaderModifier, const LsShader *lsb);

		/** @brief Generates the Fetch Shader for an LS-stage vertex shader.
		 *
		 * This <b>Direct Mapping</b> variant assumes that all vertex buffer slots map directly to the corresponding vertex shader input slots.
		 *
		 * @param fs				Receives the generated fetch shader. The supplied buffer must be at least as large as the size returned by computeLsFetchShaderSize().
		 * @param shaderModifier	Receives a value which will need to be passed to ConstantUpdateEngine::setLsShader().
		 * @param lsb				A pointer to the vertex shader binary data (this may get modified).
		 * @param instancingData	A pointer to a table describing which index to use to fetch the data for each shader entry. Specify NULL to always default to Vertex Index.
		 *
		 * @see computeLsFetchShaderSize()
		 */		 
		SCE_GNMX_EXPORT void generateLsFetchShader(void *fs, uint32_t *shaderModifier, const LsShader *lsb, const Gnm::FetchShaderInstancingMode *instancingData);

		/** @brief Generates the Fetch Shader for an LS-stage vertex shader.
		 *
		 * The <b>Remapping Table</b> variant allows arbitrary remapping of vertex buffer slots to vertex shader input slots using the provided semantic remapping table.
		 *
		 * @param fs             			Receives the generated fetch shader. The supplied buffer must be at least as large as the size returned by computeLsFetchShaderSize().
		 * @param shaderModifier   			Receives a value which will need to be passed to either ConstantUpdateEngine::setLsShader() or LsShader::applyFetchShaderModifier().
		 * @param vsb                		A pointer to the vertex shader binary data (this may get modified).
		 * @param semanticRemapTable		A pointer to the semantic remapping table to match the vertex shader input with the Vertex Buffer definition. This table contains
		 									one element for each vertex buffer slot that may be bound. Each vertex buffer slot's table entry contains the index of the
		 									vertex shader input that the vertex buffer should map to (or 0xFFFFFFFF if the buffer is unused).
		 * @param numElementsInRemapTable	The size of the table passed to <c><i>semanticRemapTable</i></c>.
		 */
		SCE_GNMX_EXPORT void generateLsFetchShader(void *fs,
												   uint32_t *shaderModifier,
												   const LsShader *lsb,
												   const void *semanticRemapTable, const uint32_t numElementsInRemapTable);
		/** @brief Generates the Fetch Shader for an LS-stage vertex shader.
		 *
		 * The <b>Remapping Table</b> variant allows arbitrary remapping of vertex buffer slots to vertex shader input slots using the provided semantic remapping table.
		 *
		 * @param fs             Buffer to generate the fetch shader into. Must be at least as large as the size returned by computeLsFetchShaderSize().
		 * @param shaderModifier Output value which will need to be passed to either the ConstantUpdateEngine::setLsShader() function or LsShader::applyFetchShaderModifier.
		 * @param lsb            Pointer to the vertex shader binary data (may get modified).
 		 * @param instancingData			A pointer to a table describing which index to use to fetch the data for each shader entry. Specify NULL to default to always Vertex Index.
		 * @param semanticRemapTable		Pointer to the semantic remapping table to match the vertex shader input with the Vertex Buffer definition. This table contains
		 												one element for each vertex buffer slot that may be bound. Each vertex buffer slot's table entry contains the index of the vertex
		 												shader input that the vertex buffer should map to (or -1, if the buffer is unused).
		 * @param numElementsInRemapTable Size of the <c><i>semanticRemapTable</i></c>.
		 */
		SCE_GNMX_EXPORT void generateLsFetchShader(void *fs,
												   uint32_t *shaderModifier,
												   const LsShader *lsb,
												   const Gnm::FetchShaderInstancingMode *instancingData,
												   const void *semanticRemapTable, const uint32_t numElementsInRemapTable);


		/** @brief Computes the size of the fetch shader in bytes for an ES-stage vertex shader.
		 * @param esb The vertex shader to generate a fetch shader for.
		 * @return The size of the fetch shader in bytes.
		 * @see generateEsFetchShader()
		 */
		SCE_GNMX_EXPORT uint32_t computeEsFetchShaderSize(const EsShader *esb);


		/** @brief Generates the fetch shader for an ES-stage vertex shader.
		 *
		 * The <b>Direct Mapping</b> variant assumes that all vertex buffer slots map directly to the corresponding vertex shader input slots.
		 * @param fs             Buffer to generate the fetch shader into. Must be at least as large as the size returned by computeEsFetchShaderSize().
		 * @param shaderModifier Output Value which will need to be passed to either the setEsShader function or EsShader::applyFetchShaderModifier.
		 * @param esb            Pointer to the export shader binary data (may get modified).
		 * @param instancingData Pointer to a table describing which index to use to fetch the data for each shader entry (NULL to default to always Vertex Index).
		 * @see computeEsFetchShaderSize()
		 */
		SCE_GNMX_EXPORT void generateEsFetchShader(void *fs, uint32_t *shaderModifier,const EsShader *esb, const Gnm::FetchShaderInstancingMode *instancingData);


		/**
		 * The <b>Remapping Table</b> variant allows arbitrary remapping of vertex buffer slots to vertex shader input slots using the provided semantic remapping table.
		 *
		 * @param fs             Buffer to generate the fetch shader into. Must be at least as large as the size returned by computeEsFetchShaderSize().
		 * @param shaderModifier Output Value which will need to be passed to either the setEsShader function or EsShader::applyFetchShaderModifier.
		 * @param esb            Pointer to the export shader binary data (may get modified).
		 * @param instancingData Pointer to a table describing which index to use to fetch the data for each shader entry (NULL to default to always Vertex Index).
		 * @param semanticRemapTable      Pointer to the semantic remapping table to match the vertex shader input with the Vertex Buffer definition. This table contains one element for each vertex buffer slot
		 *                                that may be bound. Each vertex buffer slot's table entry contains the index of the vertex shader input that the vertex buffer should map to (or -1, if the buffer is unused).
		 * @param numElementsInRemapTable Size of the <c><i>semanticRemapTable</i></c>.
		 */
		SCE_GNMX_EXPORT void generateEsFetchShader(void *fs,
												   uint32_t *shaderModifier,
												   const EsShader *esb,
												   const Gnm::FetchShaderInstancingMode *instancingData,
												   const void *semanticRemapTable, const uint32_t numElementsInRemapTable);

		/** 
		 * The <b>CsVsShader</b> variant computes the size of the fetch shader in bytes for the VS-stage of a CsVsShader.
		 *
		 * @param csvsb 	The combined compute and vertex shader to generate a VS-stage fetch shader for.
		 * @return 			The size of the fetch shader in bytes.
		 * @see generateVsFetchShader()
		 */
		SCE_GNMX_EXPORT uint32_t computeVsFetchShaderSize(const CsVsShader *csvsb);

		/** 
		 * The <b>DispatchDraw</b> variants generate the fetch shader for the VS stage part of a DispatchDraw and Compute Vertex shader pair.
		 *			 
 		 * @param fs             Buffer to generate the fetch shader into. Must be at least as large as the size returned by computeVsFetchShaderSize().
		 * @param shaderModifier Output Value which will need to be passed to either the setCsVsShader function or CsVsShader::applyFetchShaderModifiers.
		 * @param csvsb          The combined compute and vertex shader to generate a VS-stage fetch shader for.
		 * @param instancingData Pointer to a table describing which index to use to fetch the data for each shader entry (NULL to default to always Vertex Index).
		 * @see computeCsFetchShaderSize()
		 */
		SCE_GNMX_EXPORT void generateVsFetchShader(void *fs, uint32_t *shaderModifier, const CsVsShader *csvsb, const Gnm::FetchShaderInstancingMode *instancingData);

		/** 
		 * @param fs             Buffer to generate the fetch shader into for the VS stage part of a DispatchDraw and Compute Vertex shader pair. Must be at least as large as the size returned by computeVsFetchShaderSize().
		 * @param shaderModifier Output Value which will need to be passed to either the setCsVsShader function or CsVsShader::applyFetchShaderModifiers.
		 * @param csvsb          The combined compute and vertex shader to generate a VS-stage fetch shader for.
		 * @param instancingData Pointer to a table describing which index to use to fetch the data for each shader entry (NULL to default to always Vertex Index).
		 * @param semanticRemapTable		Pointer to the semantic remapping table to match the vertex shader input with the Vertex Buffer definition. This table contains 
		 *												one element for each vertex buffer slot that may be bound. Each vertex buffer slot's table entry contains the index of the vertex 
		 *												shader input that the vertex buffer should map to (or -1, if the buffer is unused).
		 * @param numElementsInRemapTable Size of the <c><i>semanticRemapTable</i></c>.
		 * @see computeCsFetchShaderSize()
		 */
		SCE_GNMX_EXPORT void generateVsFetchShader(void *fs,
												   uint32_t *shaderModifier,
												   const CsVsShader *csvsb,
												   const Gnm::FetchShaderInstancingMode *instancingData, 
												   const void *semanticRemapTable, const uint32_t numElementsInRemapTable);

		/** @brief Computes the size of the fetch shader in bytes for the CS-stage of a CsVsShader.
		 * @param csvsb 	The combined compute and vertex shader to generate a CS-stage fetch shader for.
		 * @return 			The size of the fetch shader in bytes.
		 * @see generateCsFetchShader()
		 */
		SCE_GNMX_EXPORT uint32_t computeCsFetchShaderSize(const CsVsShader *csvsb);

		/** @brief Generates the Fetch Shader for an CS-stage compute shader, most often for one embedded in a CsVsShader.
		 *
		 * The <b>Direct Mapping</b> variant assumes that all vertex buffer slots map directly to the corresponding vertex shader input slots.
		 *
 		 * @param fs             Buffer to generate the fetch shader into. Must be at least as large as the size returned by computeCsFetchShaderSize().
		 * @param shaderModifier Output Value which will need to be passed to either one of the <c>setCsVsShaders()</c> functions or CsVsShader::applyFetchShaderModifiers.
		 * @param csvsb          The combined compute and vertex shader to generate a CS-stage fetch shader for.
		 * @param instancingData Pointer to a table describing which index to use to fetch the data for each shader entry (NULL to default to always Vertex Index).
		 * @see computeCsFetchShaderSize()
		 */
		SCE_GNMX_EXPORT void generateCsFetchShader(void *fs, uint32_t *shaderModifier, const CsVsShader *csvsb, const Gnm::FetchShaderInstancingMode *instancingData);

		/** 
		 *
		 * The <b>Remapping Table</b> variant allows arbitrary remapping of vertex buffer slots to vertex shader input slots using the provided semantic remapping table.
		 * @param fs             Buffer to generate the fetch shader into. Must be at least as large as the size returned by computeCsFetchShaderSize().
		 * @param shaderModifier Output Value which will need to be passed to either one of the <c>setCsVsShaders()</c> functions or CsVsShader::applyFetchShaderModifiers.
		 * @param csvsb          The combined compute and vertex shader to generate a CS-stage fetch shader for.
		 * @param instancingData Pointer to a table describing which index to use to fetch the data for each shader entry (NULL to default to always Vertex Index).
		 * @param semanticRemapTable		Pointer to the semantic remapping table to match the vertex shader input with the Vertex Buffer definition. This table contains 
		 												one element for each vertex buffer slot that may be bound. Each vertex buffer slot's table entry contains the index of the vertex 
		 												shader input that the vertex buffer should map to (or -1, if the buffer is unused).
		 * @param numElementsInRemapTable Size of the <c><i>semanticRemapTable</i></c>.
		 */
		SCE_GNMX_EXPORT void generateCsFetchShader(void *fs,
												   uint32_t *shaderModifier,
												   const CsVsShader *csvsb,
												   const Gnm::FetchShaderInstancingMode *instancingData, 
												   const void *semanticRemapTable, const uint32_t numElementsInRemapTable);
	}
}
#endif
