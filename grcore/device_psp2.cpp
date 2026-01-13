//
// grcore/device_psp2.cpp
//
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.
//
#if __PSP2

#pragma diag_suppress 828

#include "device.h"

#include <string.h>
#include <stdlib.h>
#include <Assert.h>
#include <sceconst.h>
#include <kernel.h>
#include <display.h>
#include <ctrl.h>
#include <gxm.h>
#include <math.h>
#include <stdio.h>


// Enable the following define to allow Razor HUD.
//#define ENABLE_RAZOR_HUD

// Enable the following define to create a Razor capture file.
//#define ENABLE_RAZOR_CAPTURE

#if defined(ENABLE_RAZOR_CAPTURE) || defined(ENABLE_RAZOR_HUD)
#include <libsysmodule.h>
#endif
#ifdef ENABLE_RAZOR_CAPTURE
#include <razor_capture.h>
#endif

#pragma comment(lib,"SceDisplay_stub")
#pragma comment(lib,"SceGxm_stub")

namespace rage {

// ...or 480x272 (PSP) or 640x368 or 720x408
grcDisplayWindow grcDevice::sm_CurrentWindow(960,544);
grcDisplayWindow grcDevice::sm_GlobalWindow(960,544);

grcEffect *g_DefaultEffect;
grcEffectVar g_DefaultSampler;
grcCommandBuffer *g_grcCommandBuffer;		// this is never going to work on this platform.

/* SCE CONFIDENTIAL
 * PSP2 Programmer Tool Runtime Library Release 00.945.040
 * Copyright (C) 2010 Sony Computer Entertainment Inc.
 * All Rights Reserved.
 */

/*	Basic rendering sample.

	This sample shows how to initialize libgxm and render two triangles (one
	to clear the screen, one animated).

	This sample is split into the following sections:

		1. Load optional Razor modules.
		2. Initialize libgxm
		3. Create a libgxm context
		4. Create a render target
		5. Allocate display buffers, set up the display queue
		6. Allocate a depth buffer
		7. Create a shader patcher and register programs
		8. Create the programs and data for the clear
		9. Create the programs and data for the spinning triangle
		10. Start the main loop
			11. Update step
			12. Rendering step
			13. Flip operation
		14. Wait for rendering to complete and shut down
		15. Unload optional Razor modules.

	Please refer to the individual comment blocks for details of each section.
*/

/*	Define the width and height to render at the native resolution on ES2
	hardware.
*/
#define DISPLAY_WIDTH				960
#define DISPLAY_HEIGHT				544
#define DISPLAY_STRIDE_IN_PIXELS	1024

/*	Define the libgxm color format to render to.  This should be kept in sync
	with the display format to use with the SceDisplay library.
*/
#define DISPLAY_COLOR_FORMAT		SCE_GXM_COLOR_FORMAT_A8B8G8R8
#define DISPLAY_PIXEL_FORMAT		SCE_DISPLAY_PIXELFORMAT_A8B8G8R8

/* 	Define the number of back buffers to use with this sample.  Most applications
	should use a value of 2 (double buffering) or 3 (triple buffering).
*/
#define DISPLAY_BUFFER_COUNT		3

/*	Define the maximum number of queued swaps that the display queue will allow.
	This limits the number of frames that the CPU can get ahead of the GPU,
	and is independent of the actual number of back buffers.  The display
	queue will block during sceGxmDisplayQueueAddEntry if this number of swaps
	have already been queued.
*/
#define DISPLAY_MAX_PENDING_SWAPS	2

/*	Define the MSAA mode.  This can be changed to 4X on ES1 hardware to use 4X
	multi-sample anti-aliasing, and can be changed to 4X or 2X on ES2 hardware.
*/
#define MSAA_MODE					SCE_GXM_MULTISAMPLE_NONE

// Helper macro to align a value
#define ALIGN(x, a)					(((x) + ((a) - 1)) & ~((a) - 1))

/*	The build process for the sample embeds the shader programs directly into the
	executable using the symbols below.  This is purely for convenience, it is
	equivalent to simply load the binary file into memory and cast the contents
	to type SceGxmProgram.
*/
// extern const SceGxmProgram _binary_clear_v_gxp_start;
// extern const SceGxmProgram _binary_clear_f_gxp_start;;
// extern const SceGxmProgram _binary_basic_v_gxp_start;
// extern const SceGxmProgram _binary_basic_f_gxp_start;

// Data structure for clear geometry
typedef struct ClearVertex {
	float x;
	float y;
} ClearVertex;

// Data structure for basic geometry
typedef struct BasicVertex {
	float x;
	float y;
	float z;
	uint32_t color;
} BasicVertex;

/*	Data structure to pass through the display queue.  This structure is
	serialized during sceGxmDisplayQueueAddEntry, and is used to pass
	arbitrary data to the display callback function, called from an internal
	thread once the back buffer is ready to be displayed.

	In this example, we only need to pass the base address of the buffer.
*/
typedef struct DisplayData
{
	void *address;
} DisplayData;

// Callback function for displaying a buffer
static void displayCallback(const void *callbackData);

// Callback function to allocate memory for the shader patcher
static void *patcherHostAlloc(void *userData, uint32_t size);

// Callback function to allocate memory for the shader patcher
static void patcherHostFree(void *userData, void *mem);

// Helper function to allocate memory and map it for the GPU
static void *graphicsAlloc(SceKernelMemBlockType type, uint32_t size, uint32_t alignment, uint32_t attribs, SceUID *uid);

// Helper function to free memory mapped to the GPU
static void graphicsFree(SceUID uid);

// Helper function to allocate memory and map it as vertex USSE code for the GPU
static void *vertexUsseAlloc(uint32_t size, SceUID *uid, uint32_t *usseOffset);

// Helper function to free memory mapped as vertex USSE code for the GPU
// static void vertexUsseFree(SceUID uid);

// Helper function to allocate memory and map it as fragment USSE code for the GPU
static void *fragmentUsseAlloc(uint32_t size, SceUID *uid, uint32_t *usseOffset);

// Helper function to free memory mapped as fragment USSE code for the GPU
static void fragmentUsseFree(SceUID uid);

// Mark variable as used
#define	UNUSED(a)					(void)(a)

// User main thread parameters
// const char		sceUserMainThreadName[]		= "libgxm_basic_main_thr";
int				sceUserMainThreadPriority	= SCE_KERNEL_DEFAULT_PRIORITY_USER;
unsigned int	sceUserMainThreadStackSize	= SCE_KERNEL_STACK_SIZE_DEFAULT_USER_MAIN;

// Define a 1MiB heap for this sample
unsigned int	sceLibcHeapSize	= 1*1024*1024;

#if 0
// Entry point
int main(void)
{
	int err;


	/* ---------------------------------------------------------------------
		8. Create the programs and data for the clear

		On SGX hardware, vertex programs must perform the unpack operations
		on vertex data, so we must define our vertex formats in order to
		create the vertex program.  Similarly, fragment programs must be
		specialized based on how they output their pixels and MSAA mode.

		We define the clear geometry vertex format here and create the vertex
		and fragment program.

		The clear vertex and index data is static, we allocate and write the
		data here.
	   --------------------------------------------------------------------- */

	// get attributes by name to create vertex format bindings
	const SceGxmProgram *clearProgram = sceGxmShaderPatcherGetProgramFromId(clearVertexProgramId);
	Assert(clearProgram);
	const SceGxmProgramParameter *paramClearPositionAttribute = sceGxmProgramFindParameterByName(clearProgram, "aPosition");
	Assert(paramClearPositionAttribute && (sceGxmProgramParameterGetCategory(paramClearPositionAttribute) == SCE_GXM_PARAMETER_CATEGORY_ATTRIBUTE));

	// create clear vertex format
	SceGxmVertexAttribute clearVertexAttributes[1];
	SceGxmVertexStream clearVertexStreams[1];
	clearVertexAttributes[0].streamIndex = 0;
	clearVertexAttributes[0].offset = 0;
	clearVertexAttributes[0].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
	clearVertexAttributes[0].componentCount = 2;
	clearVertexAttributes[0].regIndex = sceGxmProgramParameterGetResourceIndex(paramClearPositionAttribute);
	clearVertexStreams[0].stride = sizeof(ClearVertex);
	clearVertexStreams[0].indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;

	// create sclear programs
	SceGxmVertexProgram *clearVertexProgram = NULL;
	SceGxmFragmentProgram *clearFragmentProgram = NULL;
	err = sceGxmShaderPatcherCreateVertexProgram(
		shaderPatcher,
		clearVertexProgramId,
		clearVertexAttributes,
		1,
		clearVertexStreams,
		1,
		&clearVertexProgram);
	Assert(err == SCE_OK);
	err = sceGxmShaderPatcherCreateFragmentProgram(
		shaderPatcher,
		clearFragmentProgramId,
		SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
		MSAA_MODE,
		NULL,
		sceGxmShaderPatcherGetProgramFromId(clearVertexProgramId),
		&clearFragmentProgram);
	Assert(err == SCE_OK);

	// create the clear triangle vertex/index data
	SceUID clearVerticesUid;
	SceUID clearIndicesUid;
	ClearVertex *const clearVertices = (ClearVertex *)graphicsAlloc(
		SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE,
		3*sizeof(ClearVertex),
		4,
		SCE_GXM_MEMORY_ATTRIB_READ,
		&clearVerticesUid);
	uint16_t *const clearIndices = (uint16_t *)graphicsAlloc(
		SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE,
		3*sizeof(uint16_t),
		2,
		SCE_GXM_MEMORY_ATTRIB_READ,
		&clearIndicesUid);

	clearVertices[0].x = -1.0f;
	clearVertices[0].y = -1.0f;
	clearVertices[1].x =  3.0f;
	clearVertices[1].y = -1.0f;
	clearVertices[2].x = -1.0f;
	clearVertices[2].y =  3.0f;

	clearIndices[0] = 0;
	clearIndices[1] = 1;
	clearIndices[2] = 2;

	/* ---------------------------------------------------------------------
		9. Create the programs and data for the spinning triangle

		We define the spinning triangle vertex format here and create the
		vertex and fragment program.

		The vertex and index data is static, we allocate and write the data
		here too.
	   --------------------------------------------------------------------- */

	// get attributes by name to create vertex format bindings
	// first retrieve the underlying program to extract binding information
	const SceGxmProgram *basicProgram = sceGxmShaderPatcherGetProgramFromId(basicVertexProgramId);
	Assert(basicProgram);
	const SceGxmProgramParameter *paramBasicPositionAttribute = sceGxmProgramFindParameterByName(basicProgram, "aPosition");
	Assert(paramBasicPositionAttribute && (sceGxmProgramParameterGetCategory(paramBasicPositionAttribute) == SCE_GXM_PARAMETER_CATEGORY_ATTRIBUTE));
	const SceGxmProgramParameter *paramBasicColorAttribute = sceGxmProgramFindParameterByName(basicProgram, "aColor");
	Assert(paramBasicColorAttribute && (sceGxmProgramParameterGetCategory(paramBasicColorAttribute) == SCE_GXM_PARAMETER_CATEGORY_ATTRIBUTE));

	// create shaded triangle vertex format
	SceGxmVertexAttribute basicVertexAttributes[2];
	SceGxmVertexStream basicVertexStreams[1];
	basicVertexAttributes[0].streamIndex = 0;
	basicVertexAttributes[0].offset = 0;
	basicVertexAttributes[0].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
	basicVertexAttributes[0].componentCount = 3;
	basicVertexAttributes[0].regIndex = sceGxmProgramParameterGetResourceIndex(paramBasicPositionAttribute);
	basicVertexAttributes[1].streamIndex = 0;
	basicVertexAttributes[1].offset = 12;
	basicVertexAttributes[1].format = SCE_GXM_ATTRIBUTE_FORMAT_U8N;
	basicVertexAttributes[1].componentCount = 4;
	basicVertexAttributes[1].regIndex = sceGxmProgramParameterGetResourceIndex(paramBasicColorAttribute);
	basicVertexStreams[0].stride = sizeof(BasicVertex);
	basicVertexStreams[0].indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;

	// create shaded triangle shaders
	SceGxmVertexProgram *basicVertexProgram = NULL;
	SceGxmFragmentProgram *basicFragmentProgram = NULL;
	err = sceGxmShaderPatcherCreateVertexProgram(
		shaderPatcher,
		basicVertexProgramId,
		basicVertexAttributes,
		2,
		basicVertexStreams,
		1,
		&basicVertexProgram);
	Assert(err == SCE_OK);
	err = sceGxmShaderPatcherCreateFragmentProgram(
		shaderPatcher,
		basicFragmentProgramId,
		SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
		MSAA_MODE,
		NULL,
		sceGxmShaderPatcherGetProgramFromId(basicVertexProgramId),
		&basicFragmentProgram);
	Assert(err == SCE_OK);

	// find vertex uniforms by name and cache parameter information
	const SceGxmProgramParameter *wvpParam = sceGxmProgramFindParameterByName(basicProgram, "wvp");
	Assert(wvpParam && (sceGxmProgramParameterGetCategory(wvpParam) == SCE_GXM_PARAMETER_CATEGORY_UNIFORM));

	// create shaded triangle vertex/index data
	SceUID basicVerticesUid;
	SceUID basicIndiceUid;
	BasicVertex *const basicVertices = (BasicVertex *)graphicsAlloc(
		SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE,
		3*sizeof(BasicVertex),
		4,
		SCE_GXM_MEMORY_ATTRIB_READ,
		&basicVerticesUid);
	uint16_t *const basicIndices = (uint16_t *)graphicsAlloc(
		SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE,
		3*sizeof(uint16_t),
		2,
		SCE_GXM_MEMORY_ATTRIB_READ,
		&basicIndiceUid);

	basicVertices[0].x = -0.5f;
	basicVertices[0].y = -0.5f;
	basicVertices[0].z = 0.0f;
	basicVertices[0].color = 0xff0000ff;
	basicVertices[1].x = 0.5f;
	basicVertices[1].y = -0.5f;
	basicVertices[1].z = 0.0f;
	basicVertices[1].color = 0xff00ff00;
	basicVertices[2].x = -0.5f;
	basicVertices[2].y = 0.5f;
	basicVertices[2].z = 0.0f;
	basicVertices[2].color = 0xffff0000;

	basicIndices[0] = 0;
	basicIndices[1] = 1;
	basicIndices[2] = 2;

	/* ---------------------------------------------------------------------
		10. Start the main loop

		Now that everything has been initialized, we can start the main
		rendering loop of the sample.
	   --------------------------------------------------------------------- */

	// initialize controller data
	SceCtrlData ctrlData;
	memset(&ctrlData, 0, sizeof(ctrlData));

	// message for SDK sample auto test
	printf("## api_libgxm/basic: INIT SUCCEEDED ##\n");

	// loop until exit
	uint32_t backBufferIndex = 0;
	uint32_t frontBufferIndex = 0;
	float rotationAngle = 0.0f;
	bool quit = false;
	while (!quit) {
		/* -----------------------------------------------------------------
			11. Update step

			Firstly, we check the control data for quit.

			Next, we perform the update step for this sample.  We advance the
			triangle angle by a fixed amount and update its matrix data.
		   ----------------------------------------------------------------- */

		// check control data
		sceCtrlReadBufferPositive(0, &ctrlData, 1);
		if (ctrlData.buttons & SCE_CTRL_SELECT) {
			quit = true;
		}

		// update triangle angle
		rotationAngle += SCE_MATH_TWOPI/60.0f;
		if (rotationAngle > SCE_MATH_TWOPI)
			rotationAngle -= SCE_MATH_TWOPI;

		// set up a 4x4 matrix for a rotation
		float aspectRatio = (float)DISPLAY_WIDTH/(float)DISPLAY_HEIGHT;

		float s = sin(rotationAngle);
		float c = cos(rotationAngle);

		float wvpData[16];
		wvpData[ 0] = c/aspectRatio;
		wvpData[ 1] = s;
		wvpData[ 2] = 0.0f;
		wvpData[ 3] = 0.0f;

		wvpData[ 4] = -s/aspectRatio;
		wvpData[ 5] = c;
		wvpData[ 6] = 0.0f;
		wvpData[ 7] = 0.0f;

		wvpData[ 8] = 0.0f;
		wvpData[ 9] = 0.0f;
		wvpData[10] = 1.0f;
		wvpData[11] = 0.0f;

		wvpData[12] = 0.0f;
		wvpData[13] = 0.0f;
		wvpData[14] = 0.0f;
		wvpData[15] = 1.0f;

		/* -----------------------------------------------------------------
			12. Rendering step

			This sample renders a single scene containing the two triangles,
			the clear triangle followed by the spinning triangle.  Before
			any drawing can take place, a scene must be started.  We render
			to the back buffer, so it is also important to use a sync object
			to ensure that these rendering operations are synchronized with
			display operations.

			The clear triangle shaders do not declare any uniform variables,
			so this may be rendered immediately after setting the vertex and
			fragment program.

			The spinning triangle vertex program declares a matrix parameter,
			so this forms part of the vertex default uniform buffer and must
			be written before the triangle can be drawn.

			Once both triangles have been drawn the scene can be ended, which
			submits it for rendering on the GPU.
		   ----------------------------------------------------------------- */

		// start rendering to the main render target
		sceGxmBeginScene(
			context, 
			0,
			renderTarget,
			NULL,
			NULL,
			displayBufferSync[backBufferIndex],
			&displaySurface[backBufferIndex],
			&depthSurface);

		// set clear shaders
		sceGxmSetVertexProgram(context, clearVertexProgram);
		sceGxmSetFragmentProgram(context, clearFragmentProgram);

		// draw the clear triangle
		sceGxmSetVertexStream(context, 0, clearVertices);
		sceGxmDraw(context, SCE_GXM_PRIMITIVE_TRIANGLES, SCE_GXM_INDEX_FORMAT_U16, clearIndices, 3);

		// render the rotating triangle
		sceGxmSetVertexProgram(context, basicVertexProgram);
		sceGxmSetFragmentProgram(context, basicFragmentProgram);

		// set the vertex program constants
		void *vertexDefaultBuffer;
		sceGxmReserveVertexDefaultUniformBuffer(context, &vertexDefaultBuffer);
		sceGxmSetUniformDataF(vertexDefaultBuffer, wvpParam, 0, 16, wvpData);

		// draw the spinning triangle
		sceGxmSetVertexStream(context, 0, basicVertices);
		sceGxmDraw(context, SCE_GXM_PRIMITIVE_TRIANGLES, SCE_GXM_INDEX_FORMAT_U16, basicIndices, 3);

		// end the scene on the main render target, submitting rendering work to the GPU
		sceGxmEndScene(context, NULL, NULL);

		// PA heartbeat to notify end of frame
		sceGxmPadHeartbeat(&displaySurface[backBufferIndex], displayBufferSync[backBufferIndex]);

		/* -----------------------------------------------------------------
			13. Flip operation

			Now we have finished submitting rendering work for this frame it
			is time to submit a flip operation.  As part of specifying this
			flip operation we must provide the sync objects for both the old
			buffer and the new buffer.  This is to allow synchronization both
			ways: to not flip until rendering is complete, but also to ensure
			that future rendering to these buffers does not start until the
			flip operation is complete.

			The callback function will be called from an internal thread once
			queued GPU operations involving the sync objects is complete.
			Assuming we have not reached our maximum number of queued frames,
			this function returns immediately.

			Once we have queued our flip, we manually cycle through our back
			buffers before starting the next frame.
		   ----------------------------------------------------------------- */

		// queue the display swap for this frame
		DisplayData displayData;
		displayData.address = displayBufferData[backBufferIndex];
		sceGxmDisplayQueueAddEntry(
			displayBufferSync[frontBufferIndex],	// front buffer is OLD buffer
			displayBufferSync[backBufferIndex],		// back buffer is NEW buffer
			&displayData);

		// update buffer indices
		frontBufferIndex = backBufferIndex;
		backBufferIndex = (backBufferIndex + 1) % DISPLAY_BUFFER_COUNT;
	}


	// message for SDK sample auto test
	printf("## api_libgxm/basic: FINISHED ##\n");
	return SCE_OK;
}
#endif

void displayCallback(const void *callbackData)
{
	SceDisplayFrameBuf framebuf;
	int err = SCE_OK;
	UNUSED(err);

	// Cast the parameters back
	const DisplayData *displayData = (const DisplayData *)callbackData;

	// Swap to the new buffer on the next VSYNC
	memset(&framebuf, 0x00, sizeof(SceDisplayFrameBuf));
	framebuf.size        = sizeof(SceDisplayFrameBuf);
	framebuf.base        = displayData->address;
	framebuf.pitch       = DISPLAY_STRIDE_IN_PIXELS;
	framebuf.pixelformat = DISPLAY_PIXEL_FORMAT;
	framebuf.width       = DISPLAY_WIDTH;
	framebuf.height      = DISPLAY_HEIGHT;
	err = sceDisplaySetFrameBuf(&framebuf, SCE_DISPLAY_UPDATETIMING_NEXTVSYNC);
	Assert(err == SCE_OK);

	// Block this callback until the swap has occurred and the old buffer
	// is no longer displayed
	err = sceDisplayWaitVblankStart();
	Assert(err == SCE_OK);
}

static void *patcherHostAlloc(void *userData, uint32_t size)
{
	UNUSED(userData);
	return malloc(size);
}

static void patcherHostFree(void *userData, void *mem)
{
	UNUSED(userData);
	free(mem);
}

static void *graphicsAlloc(SceKernelMemBlockType type, uint32_t size, uint32_t alignment, uint32_t attribs, SceUID *uid)
{
	int err = SCE_OK;
	UNUSED(err);

	/*	Since we are using sceKernelAllocMemBlock directly, we cannot directly
		use the alignment parameter.  Instead, we must allocate the size to the
		minimum for this memblock type, and just Assert that this will cover
		our desired alignment.

		Developers using their own heaps should be able to use the alignment
		parameter directly for more minimal padding.
	*/
	if (type == SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RWDATA) {
		// CDRAM memblocks must be 256KiB aligned
		Assert(alignment <= 256*1024);
		size = ALIGN(size, 256*1024);
	} else {
		// LPDDR memblocks must be 4KiB aligned
		Assert(alignment <= 4*1024);
		size = ALIGN(size, 4*1024);
	}
	UNUSED(alignment);

	// allocate some memory
	*uid = sceKernelAllocMemBlock("basic", type, size, NULL);
	Assert(*uid >= SCE_OK);

	// grab the base address
	void *mem = NULL;
	err = sceKernelGetMemBlockBase(*uid, &mem);
	Assert(err == SCE_OK);

	// map for the GPU
	err = sceGxmMapMemory(mem, size, attribs);
	Assert(err == SCE_OK);

	// done
	return mem;
}

static void graphicsFree(SceUID uid)
{
	int err = SCE_OK;
	UNUSED(err);

	// grab the base address
	void *mem = NULL;
	err = sceKernelGetMemBlockBase(uid, &mem);
	Assert(err == SCE_OK);

	// unmap memory
	err = sceGxmUnmapMemory(mem);
	Assert(err == SCE_OK);

	// free the memory block
	err = sceKernelFreeMemBlock(uid);
	Assert(err == SCE_OK);
}

static void *vertexUsseAlloc(uint32_t size, SceUID *uid, uint32_t *usseOffset)
{
	int err = SCE_OK;
	UNUSED(err);

	// align to memblock alignment for LPDDR
	size = ALIGN(size, 4096);
	
	// allocate some memory
	*uid = sceKernelAllocMemBlock("basic", SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE, size, NULL);
	Assert(*uid >= SCE_OK);

	// grab the base address
	void *mem = NULL;
	err = sceKernelGetMemBlockBase(*uid, &mem);
	Assert(err == SCE_OK);

	// map as vertex USSE code for the GPU
	err = sceGxmMapVertexUsseMemory(mem, size, usseOffset);
	Assert(err == SCE_OK);

	// done
	return mem;
}

#if 0
static void vertexUsseFree(SceUID uid)
{
	int err = SCE_OK;
	UNUSED(err);

	// grab the base address
	void *mem = NULL;
	err = sceKernelGetMemBlockBase(uid, &mem);
	Assert(err == SCE_OK);

	// unmap memory
	err = sceGxmUnmapVertexUsseMemory(mem);
	Assert(err == SCE_OK);

	// free the memory block
	err = sceKernelFreeMemBlock(uid);
	Assert(err == SCE_OK);
}
#endif

static void *fragmentUsseAlloc(uint32_t size, SceUID *uid, uint32_t *usseOffset)
{
	int err = SCE_OK;
	UNUSED(err);

	// align to memblock alignment for LPDDR
	size = ALIGN(size, 4096);
	
	// allocate some memory
	*uid = sceKernelAllocMemBlock("basic", SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE, size, NULL);
	Assert(*uid >= SCE_OK);

	// grab the base address
	void *mem = NULL;
	err = sceKernelGetMemBlockBase(*uid, &mem);
	Assert(err == SCE_OK);

	// map as fragment USSE code for the GPU
	err = sceGxmMapFragmentUsseMemory(mem, size, usseOffset);
	Assert(err == SCE_OK);

	// done
	return mem;
}

static void fragmentUsseFree(SceUID uid)
{
	int err = SCE_OK;
	UNUSED(err);

	// grab the base address
	void *mem = NULL;
	err = sceKernelGetMemBlockBase(uid, &mem);
	Assert(err == SCE_OK);

	// unmap memory
	err = sceGxmUnmapFragmentUsseMemory(mem);
	Assert(err == SCE_OK);

	// free the memory block
	err = sceKernelFreeMemBlock(uid);
	Assert(err == SCE_OK);
}

grcFenceHandle grcDevice::InsertFence()
{
	return 0;	// not supported on PSP2 -- only scene flushes and mid scene flushes support any sort of notification
}


void grcDevice::BlockOnFence(grcFenceHandle)
{
}


void *displayBufferData[DISPLAY_BUFFER_COUNT];
SceUID displayBufferUid[DISPLAY_BUFFER_COUNT];
SceUID vdmRingBufferUid;
SceUID vertexRingBufferUid;
SceUID fragmentRingBufferUid;
SceUID fragmentUsseRingBufferUid;
SceGxmContextParams contextParams;
SceGxmContext *context = NULL;
SceGxmRenderTargetParams renderTargetParams;
SceGxmRenderTarget *renderTarget;
SceGxmColorSurface displaySurface[DISPLAY_BUFFER_COUNT];
SceGxmSyncObject *displayBufferSync[DISPLAY_BUFFER_COUNT];
SceGxmShaderPatcher *shaderPatcher = NULL;
SceUID depthBufferUid;

const int immediateVerticesSize = 65536;		// current impl requires this to be power of two
const int MAX_IMMEDIATE_VERTICES = 1024;
void *immediateVertices;
u16 *immediateIndices;
SceUID immediateVerticesUID, immediateIndicesUID;

void grcDevice::InitClass(bool,bool)
{
	int err = SCE_OK;
	UNUSED(err);

	/* ---------------------------------------------------------------------
		1. Load optional Razor modules.

		These modules must be loaded before libgxm is initialized.
	   --------------------------------------------------------------------- */

#ifdef ENABLE_RAZOR_HUD
	// Initialize the Razor HUD system.
	// This should be done before the call to sceGxmInitialize().
	err = sceSysmoduleLoadModule( SCE_SYSMODULE_RAZOR_HUD );
	Assert(err == SCE_OK);
#endif

#ifdef ENABLE_RAZOR_CAPTURE
	// Initialize the Razor capture system.
	// This should be done before the call to sceGxmInitialize().
	err = sceSysmoduleLoadModule( SCE_SYSMODULE_RAZOR_CAPTURE );
	Assert(err == SCE_OK);

	// Trigger a capture after 100 frames.
	sceRazorCaptureSetTrigger( 100, "host0:basic.sgx" );
#endif

	/* ---------------------------------------------------------------------
		2. Initialize libgxm

		We specify the default parameter buffer size of 16MiB.

	   --------------------------------------------------------------------- */

	// set up parameters
	SceGxmInitializeParams initializeParams;
	memset(&initializeParams, 0, sizeof(SceGxmInitializeParams));
	initializeParams.flags							= 0;
	initializeParams.displayQueueMaxPendingCount	= DISPLAY_MAX_PENDING_SWAPS;
	initializeParams.displayQueueCallback			= displayCallback;
	initializeParams.displayQueueCallbackDataSize	= sizeof(DisplayData);
	initializeParams.parameterBufferSize			= SCE_GXM_DEFAULT_PARAMETER_BUFFER_SIZE;

	// initialize
	err = sceGxmInitialize(&initializeParams);
	Assert(err == SCE_OK);

	/* ---------------------------------------------------------------------
		3. Create a libgxm context
		
		Once initialized, we need to create a rendering context to allow to us
		to render scenes on the GPU.  We use the default initialization
		parameters here to set the sizes of the various context ring buffers.
	   --------------------------------------------------------------------- */

	// allocate ring buffer memory using default sizes
	void *vdmRingBuffer = graphicsAlloc(
		SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE,
		SCE_GXM_DEFAULT_VDM_RING_BUFFER_SIZE,
		4,
		SCE_GXM_MEMORY_ATTRIB_READ,
		&vdmRingBufferUid);
	void *vertexRingBuffer = graphicsAlloc(
		SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE,
		SCE_GXM_DEFAULT_VERTEX_RING_BUFFER_SIZE,
		4,
		SCE_GXM_MEMORY_ATTRIB_READ,
		&vertexRingBufferUid);
	void *fragmentRingBuffer = graphicsAlloc(
		SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE,
		SCE_GXM_DEFAULT_FRAGMENT_RING_BUFFER_SIZE,
		4,
		SCE_GXM_MEMORY_ATTRIB_READ,
		&fragmentRingBufferUid);
	uint32_t fragmentUsseRingBufferOffset;
	void *fragmentUsseRingBuffer = fragmentUsseAlloc(
		SCE_GXM_DEFAULT_FRAGMENT_USSE_RING_BUFFER_SIZE,
		&fragmentUsseRingBufferUid,
		&fragmentUsseRingBufferOffset);

	memset(&contextParams, 0, sizeof(SceGxmContextParams));
	contextParams.hostMem						= malloc(SCE_GXM_MINIMUM_CONTEXT_HOST_MEM_SIZE);
	contextParams.hostMemSize					= SCE_GXM_MINIMUM_CONTEXT_HOST_MEM_SIZE;
	contextParams.vdmRingBufferMem				= vdmRingBuffer;
	contextParams.vdmRingBufferMemSize			= SCE_GXM_DEFAULT_VDM_RING_BUFFER_SIZE;
	contextParams.vertexRingBufferMem			= vertexRingBuffer;
	contextParams.vertexRingBufferMemSize		= SCE_GXM_DEFAULT_VERTEX_RING_BUFFER_SIZE;
	contextParams.fragmentRingBufferMem			= fragmentRingBuffer;
	contextParams.fragmentRingBufferMemSize		= SCE_GXM_DEFAULT_FRAGMENT_RING_BUFFER_SIZE;
	contextParams.fragmentUsseRingBufferMem		= fragmentUsseRingBuffer;
	contextParams.fragmentUsseRingBufferMemSize	= SCE_GXM_DEFAULT_FRAGMENT_USSE_RING_BUFFER_SIZE;
	contextParams.fragmentUsseRingBufferOffset	= fragmentUsseRingBufferOffset;
	
	err = sceGxmCreateContext(&contextParams, &context);
	Assert(err == SCE_OK);

	/* ---------------------------------------------------------------------
		4. Create a render target

		Finally we create a render target to describe the geometry of the back
		buffers we will render to.  This object is used purely to schedule
		rendering jobs for the given dimensions, the color surface and
		depth/stencil surface must be allocated separately.
	   --------------------------------------------------------------------- */

	// set up parameters
	memset(&renderTargetParams, 0, sizeof(SceGxmRenderTargetParams));
	renderTargetParams.flags				= 0;
	renderTargetParams.width				= DISPLAY_WIDTH;
	renderTargetParams.height				= DISPLAY_HEIGHT;
	renderTargetParams.scenesPerFrame		= 1;
	renderTargetParams.multisampleMode		= MSAA_MODE;
	renderTargetParams.multisampleLocations	= 0;
	renderTargetParams.hostMem				= NULL;
	renderTargetParams.hostMemSize			= 0;
	renderTargetParams.driverMemBlock		= -1;
	
	{
		// compute sizes
		uint32_t hostMemSize, driverMemSize;
		sceGxmGetRenderTargetMemSizes(&renderTargetParams, &hostMemSize, &driverMemSize);

		// allocate host memory
		renderTargetParams.hostMem = malloc(hostMemSize);
		renderTargetParams.hostMemSize = hostMemSize;

		// allocate driver memory
		renderTargetParams.driverMemBlock = sceKernelAllocMemBlock(
			"SampleRT", 
			SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE, 
			driverMemSize, 
			NULL);
	}

	// create the render target
	err = sceGxmCreateRenderTarget(&renderTargetParams, &renderTarget);
	Assert(err == SCE_OK);

	/* ---------------------------------------------------------------------
		5. Allocate display buffers and sync objects

		We will allocate our back buffers in CDRAM, and create a color
		surface for each of them.

		To allow display operations done by the CPU to be synchronized with
		rendering done by the GPU, we also create a SceGxmSyncObject for each
		display buffer.  This sync object will be used with each scene that
		renders to that buffer and when queueing display flips that involve
		that buffer (either flipping from or to).
	   --------------------------------------------------------------------- */

	// allocate memory and sync objects for display buffers
	for (uint32_t i = 0; i < DISPLAY_BUFFER_COUNT; ++i) {
		// allocate memory with large (1MiB) alignment to ensure physical contiguity
		displayBufferData[i] = graphicsAlloc(
			SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RWDATA,
			ALIGN(4*DISPLAY_STRIDE_IN_PIXELS*DISPLAY_HEIGHT, 1*1024*1024),
			SCE_GXM_COLOR_SURFACE_ALIGNMENT,
			SCE_GXM_MEMORY_ATTRIB_READ | SCE_GXM_MEMORY_ATTRIB_WRITE,
			&displayBufferUid[i]);

		// memset the buffer to a noticeable debug color
		for (uint32_t y = 0; y < DISPLAY_HEIGHT; ++y) {
			uint32_t *row = (uint32_t *)displayBufferData[i] + y*DISPLAY_STRIDE_IN_PIXELS;
			for (uint32_t x = 0; x < DISPLAY_WIDTH; ++x) {
				row[x] = 0xffff00ff;
			}
		}

		// initialize a color surface for this display buffer
		err = sceGxmColorSurfaceInit(
			&displaySurface[i],
			DISPLAY_COLOR_FORMAT,
			SCE_GXM_COLOR_SURFACE_LINEAR,
			(MSAA_MODE == SCE_GXM_MULTISAMPLE_NONE) ? SCE_GXM_COLOR_SURFACE_SCALE_NONE : SCE_GXM_COLOR_SURFACE_SCALE_MSAA_DOWNSCALE,
			SCE_GXM_OUTPUT_REGISTER_SIZE_32BIT,
			DISPLAY_WIDTH,
			DISPLAY_HEIGHT,
			DISPLAY_STRIDE_IN_PIXELS,
			displayBufferData[i]);
		Assert(err == SCE_OK);

		// create a sync object that we will associate with this buffer
		err = sceGxmSyncObjectCreate(&displayBufferSync[i]);
		Assert(err == SCE_OK);
	}

	/* ---------------------------------------------------------------------
		6. Allocate a depth buffer

		Note that since this sample renders in a strictly back-to-front order,
		a depth buffer is not strictly required.  However, since it is usually
		necessary to create one to handle partial renders, we will create one
		now.  Note that we do not enable force load or store, so this depth
		buffer will not actually be read or written by the GPU when this
		sample is executed, so will have zero performance impact.
	   --------------------------------------------------------------------- */

	// compute the memory footprint of the depth buffer
	const uint32_t alignedWidth = ALIGN(DISPLAY_WIDTH, SCE_GXM_TILE_SIZEX);
	const uint32_t alignedHeight = ALIGN(DISPLAY_HEIGHT, SCE_GXM_TILE_SIZEY);
	uint32_t sampleCount = alignedWidth*alignedHeight;
	uint32_t depthStrideInSamples = alignedWidth;
	if (MSAA_MODE == SCE_GXM_MULTISAMPLE_4X) {
		// samples increase in X and Y
		sampleCount *= 4;
		depthStrideInSamples *= 2;
	} else if (MSAA_MODE == SCE_GXM_MULTISAMPLE_2X) {
		// samples increase in Y only
		sampleCount *= 2;
	}

	// allocate it
	void *depthBufferData = graphicsAlloc(
		SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE,
		4*sampleCount,
		SCE_GXM_DEPTHSTENCIL_SURFACE_ALIGNMENT,
		SCE_GXM_MEMORY_ATTRIB_READ | SCE_GXM_MEMORY_ATTRIB_WRITE,
		&depthBufferUid);

	// create the SceGxmDepthStencilSurface structure
	SceGxmDepthStencilSurface depthSurface;
	err = sceGxmDepthStencilSurfaceInit(
		&depthSurface,
		SCE_GXM_DEPTH_STENCIL_FORMAT_S8D24,
		SCE_GXM_DEPTH_STENCIL_SURFACE_TILED,
		depthStrideInSamples,
		depthBufferData,
		NULL);
	Assert(err == SCE_OK);

	/* ---------------------------------------------------------------------
		7. Create a shader patcher and register programs

		A shader patcher object is required to produce vertex and fragment
		programs from the shader compiler output.  First we create a shader
		patcher instance, using callback functions to allow it to allocate
		and free host memory for internal state.

		We will use the shader patcher's internal heap to handle buffer
		memory and USSE memory for the final programs.  To do this, we
		leave the callback functions as NULL, but provide static memory
		blocks.

		In order to create vertex and fragment programs for a particular
		shader, the compiler output must first be registered to obtain an ID
		for that shader.  Within a single ID, vertex and fragment programs
		are reference counted and could be shared if created with identical
		parameters.  To maximise this sharing, programs should only be
		registered with the shader patcher once if possible, so we will do
		this now.
	   --------------------------------------------------------------------- */

	// set buffer sizes for this sample
	const uint32_t patcherBufferSize		= 64*1024;
	const uint32_t patcherVertexUsseSize 	= 64*1024;
	const uint32_t patcherFragmentUsseSize 	= 64*1024;
	
	// allocate memory for buffers and USSE code
	SceUID patcherBufferUid;
	void *patcherBuffer = graphicsAlloc(
		SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE,
		patcherBufferSize,
		4, 
		SCE_GXM_MEMORY_ATTRIB_READ | SCE_GXM_MEMORY_ATTRIB_WRITE,
		&patcherBufferUid);
	SceUID patcherVertexUsseUid;
	uint32_t patcherVertexUsseOffset;
	void *patcherVertexUsse = vertexUsseAlloc(
		patcherVertexUsseSize,
		&patcherVertexUsseUid,
		&patcherVertexUsseOffset);
	SceUID patcherFragmentUsseUid;
	uint32_t patcherFragmentUsseOffset;
	void *patcherFragmentUsse = fragmentUsseAlloc(
		patcherFragmentUsseSize,
		&patcherFragmentUsseUid,
		&patcherFragmentUsseOffset);

	// create a shader patcher
	SceGxmShaderPatcherParams patcherParams;
	memset(&patcherParams, 0, sizeof(SceGxmShaderPatcherParams));
	patcherParams.userData					= NULL;
	patcherParams.hostAllocCallback			= &patcherHostAlloc;
	patcherParams.hostFreeCallback			= &patcherHostFree;
	patcherParams.bufferAllocCallback		= NULL;
	patcherParams.bufferFreeCallback		= NULL;
	patcherParams.bufferMem					= patcherBuffer;
	patcherParams.bufferMemSize				= patcherBufferSize;
	patcherParams.vertexUsseAllocCallback	= NULL;
	patcherParams.vertexUsseFreeCallback	= NULL;
	patcherParams.vertexUsseMem				= patcherVertexUsse;
	patcherParams.vertexUsseMemSize			= patcherVertexUsseSize;
	patcherParams.vertexUsseOffset			= patcherVertexUsseOffset;
	patcherParams.fragmentUsseAllocCallback	= NULL;
	patcherParams.fragmentUsseFreeCallback	= NULL;
	patcherParams.fragmentUsseMem			= patcherFragmentUsse;
	patcherParams.fragmentUsseMemSize		= patcherFragmentUsseSize;
	patcherParams.fragmentUsseOffset		= patcherFragmentUsseOffset;

	err = sceGxmShaderPatcherCreate(&patcherParams, &shaderPatcher);
	Assert(err == SCE_OK);

	// register programs with the patcher
	/* SceGxmShaderPatcherId clearVertexProgramId;
	SceGxmShaderPatcherId clearFragmentProgramId;
	SceGxmShaderPatcherId basicVertexProgramId;
	SceGxmShaderPatcherId basicFragmentProgramId;
	err = sceGxmShaderPatcherRegisterProgram(shaderPatcher, &_binary_clear_v_gxp_start, &clearVertexProgramId);
	Assert(err == SCE_OK);
	err = sceGxmShaderPatcherRegisterProgram(shaderPatcher, &_binary_clear_f_gxp_start, &clearFragmentProgramId);
	Assert(err == SCE_OK);
	err = sceGxmShaderPatcherRegisterProgram(shaderPatcher, &_binary_basic_v_gxp_start, &basicVertexProgramId);
	Assert(err == SCE_OK);
	err = sceGxmShaderPatcherRegisterProgram(shaderPatcher, &_binary_basic_f_gxp_start, &basicFragmentProgramId);
	Assert(err == SCE_OK); */

	immediateVertices = graphicsAlloc(
		SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE,
		immediateVerticesSize,
		4,
		SCE_GXM_MEMORY_ATTRIB_READ,
		&immediateVerticesUID);
	uint16_t *const immediateIndices = (uint16_t *)graphicsAlloc(
		SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE,
		MAX_IMMEDIATE_VERTICES*sizeof(uint16_t),
		2,
		SCE_GXM_MEMORY_ATTRIB_READ,
		&immediateIndicesUID);
	for (int i=0; i<MAX_IMMEDIATE_VERTICES; i++)
		immediateIndices[i] = i;
}

void grcDevice::ShutdownClass()
{
	/* ---------------------------------------------------------------------
		14. Wait for rendering to complete and shut down

		Since there could be many queued operations not yet completed it is
		important to wait until the GPU has finished before destroying
		resources.  We do this by calling sceGxmFinish for our rendering
		context.

		Once the GPU is finished, we release all our programs, deallocate
		all our memory, destroy all object and finally terminate libgxm.
	   --------------------------------------------------------------------- */

	// wait until rendering is done
	sceGxmFinish(context);

	// clean up allocations
#if 0
	sceGxmShaderPatcherReleaseFragmentProgram(shaderPatcher, basicFragmentProgram);
	sceGxmShaderPatcherReleaseVertexProgram(shaderPatcher, basicVertexProgram);
	sceGxmShaderPatcherReleaseFragmentProgram(shaderPatcher, clearFragmentProgram);
	sceGxmShaderPatcherReleaseVertexProgram(shaderPatcher, clearVertexProgram);
	graphicsFree(basicIndiceUid);
	graphicsFree(basicVerticesUid);
	graphicsFree(clearIndicesUid);
	graphicsFree(clearVerticesUid);
#endif

	// wait until display queue is finished before deallocating display buffers
	int err = sceGxmDisplayQueueFinish();
	Assert(err == SCE_OK);

	// clean up display queue
	graphicsFree(depthBufferUid);
	for (uint32_t i = 0; i < DISPLAY_BUFFER_COUNT; ++i) {
		// clear the buffer then deallocate
		memset(displayBufferData[i], 0, DISPLAY_HEIGHT*DISPLAY_STRIDE_IN_PIXELS*4);
		graphicsFree(displayBufferUid[i]);

		// destroy the sync object
		sceGxmSyncObjectDestroy(displayBufferSync[i]);
	}

	// unregister programs and destroy shader patcher
#if 0
	sceGxmShaderPatcherUnregisterProgram(shaderPatcher, basicFragmentProgramId);
	sceGxmShaderPatcherUnregisterProgram(shaderPatcher, basicVertexProgramId);
	sceGxmShaderPatcherUnregisterProgram(shaderPatcher, clearFragmentProgramId);
	sceGxmShaderPatcherUnregisterProgram(shaderPatcher, clearVertexProgramId);
#endif
	sceGxmShaderPatcherDestroy(shaderPatcher);
	// fragmentUsseFree(patcherFragmentUsseUid);
	// vertexUsseFree(patcherVertexUsseUid);
	// graphicsFree(patcherBufferUid);

	// destroy the render target
	sceGxmDestroyRenderTarget(renderTarget);
	sceKernelFreeMemBlock(renderTargetParams.driverMemBlock);
	free(renderTargetParams.hostMem);

	// destroy the context
	sceGxmDestroyContext(context);
	fragmentUsseFree(fragmentUsseRingBufferUid);
	graphicsFree(fragmentRingBufferUid);
	graphicsFree(vertexRingBufferUid);
	graphicsFree(vdmRingBufferUid);
	free(contextParams.hostMem);

	// terminate libgxm
	sceGxmTerminate();

	/* ---------------------------------------------------------------------
		15. Unload optional Razor modules.

		These must be unloaded after terminating libgxm.
	   --------------------------------------------------------------------- */
#ifdef ENABLE_RAZOR_CAPTURE
	// Terminate Razor capture.
	// This should be done after the call to sceGxmTerminate().
	sceSysmoduleUnloadModule( SCE_SYSMODULE_RAZOR_CAPTURE );
#endif

#ifdef ENABLE_RAZOR_HUD
	// Terminate Razor HUD.
	// This should be done after the call to sceGxmTerminate().
	sceSysmoduleUnloadModule( SCE_SYSMODULE_RAZOR_HUD );
#endif
}

bool grcDevice::CheckThreadOwnership()
{
	return true;		// HACK
}

bool grcDevice::CaptureScreenShotToFile(const char*)
{
	return false;
}

bool grcDevice::CaptureScreenShotToJpegFile(const char*)
{
	return false;
}

static SceGxmPrimitiveType translateDraw[] = 
{
	SCE_GXM_PRIMITIVE_POINTS,
	SCE_GXM_PRIMITIVE_LINES,
	SCE_GXM_PRIMITIVE_LINES,		// WRONG
	SCE_GXM_PRIMITIVE_TRIANGLES,
	SCE_GXM_PRIMITIVE_TRIANGLE_STRIP,
	SCE_GXM_PRIMITIVE_TRIANGLE_FAN,
	SCE_GXM_PRIMITIVE_LINES,		// WRONG
	SCE_GXM_PRIMITIVE_LINES			// WRONG
};

static SceGxmPrimitiveType s_BeginFormat;
static u32 s_BeginOffset, s_BeginCount, s_BeginOffsetMax = immediateVerticesSize/2, s_BeginOffsetNext = immediateVerticesSize/2;
static u32 s_BeginActive;
static SceGxmNotification immediateNotify[2];

static void SwapImmediateBuffers()
{
	// Flush the current scene
	// TODO: Nothing is actually setting up these notifications properly yet!
	sceGxmMidSceneFlush(context, SCE_GXM_MIDSCENE_PRESERVE_DEFAULT_UNIFORM_BUFFERS, NULL, &immediateNotify[s_BeginActive]);
	// Wait for the previous scene to finish
	s_BeginActive ^= 1;
	sceGxmNotificationWait(&immediateNotify[s_BeginActive]);
	s_BeginOffset = s_BeginOffsetNext;
	s_BeginOffsetNext ^= (immediateVerticesSize/2);
	s_BeginOffsetMax ^= (immediateVerticesSize | (immediateVerticesSize/2));
}

void* grcDevice::BeginVertices(grcDrawMode dm,u32 vertexCount,u32 vertexSize)
{
	Assert(!s_BeginFormat);
	Assert(vertexCount <= MAX_IMMEDIATE_VERTICES);
	s_BeginFormat = translateDraw[dm];
	s_BeginCount = vertexCount;
	u32 vertexTotal = vertexCount * vertexSize;
	if (s_BeginOffset + vertexTotal > s_BeginOffsetMax)
		SwapImmediateBuffers();
	void *result = (char*)immediateVertices + s_BeginOffset;
	sceGxmSetVertexStream(context, 0, result);
	s_BeginOffset += vertexCount * vertexSize;
	return result;
}

void grcDevice::EndVertices(const void *)
{
	Assert(s_BeginFormat);
	sceGxmDraw(context, s_BeginFormat, SCE_GXM_INDEX_FORMAT_U16, immediateIndices, s_BeginCount);
	ASSERT_ONLY(s_BeginFormat = (SceGxmPrimitiveType)0);
}

bool grcDevice::BeginFrame()
{
	return true;
}

void grcDevice::EndFrame(const grcResolveFlags*)
{
	SwapImmediateBuffers();
}

void grcDevice::GetSafeZone(int &x0,int &y0,int &x1,int &y1)
{
	x0 = y0 = 0;
	x1 = sm_GlobalWindow.uWidth - 1;
	y1 = sm_GlobalWindow.uHeight - 1;
}

void grcDevice::SetDefaultEffect(bool,bool)
{
}

void grcDevice::SetWindow(const grcWindow &)
{
}

void grcDevice::SetWindowTitle(const char*)
{
}

void grcDevice::SetVertexShaderConstant(int start,const float *data,int reg)
{
}

void grcDevice::Clear(bool /*enableClearColor*/,Color32 /*clearColor*/,bool /*enableClearDepth*/,float /*clearDepth*/,bool /*enableClearStenci*/,u32 /*clearStencil*/)
{

}

grcVertexDeclaration* grcDevice::CreateVertexDeclaration(const grcVertexElement *pVertexElements, int elementCount, int strideOverride)
{
	grcVertexDeclaration *decl = rage_new grcVertexDeclaration();	// POD, init to zero
	decl->RefCount = 1;

	if (strideOverride)
		decl->Stream0Size = strideOverride;

	return decl;
}

void grcDevice::DestroyVertexDeclaration(grcVertexDeclaration *pDecl)
{
	if (pDecl)
		pDecl->Release();
}

grcDevice::Result grcDevice::SetVertexDeclaration(const grcVertexDeclaration *pDecl)
{
	return 0;
}

}	// namespace rage

#endif	// __PSP2