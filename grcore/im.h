//
// grcore/im.h
//
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved.
//

#ifndef GRCORE_IM_H
#define GRCORE_IM_H

/*
	Immediate mode API for grcore
*/

#include "atl/array.h"
#include "system/ipc.h"
#include "system/pipepacket.h"
#include "system/tls.h"
#include "vector/color32.h"
#include "vector/matrix34.h"
#include "vector/vector2.h"
#include "vector/vector3.h"
#include "vector/vector4.h"
#include "vectormath/mat34v.h"
#include "vectormath/vec3v.h"

#include "drawmode.h"
#include "stateblock.h"

namespace rage {

class datResource;
class grcTexture;
class grcViewport;
class grcFont;

//
// PURPOSE:
//	Wrapper for immediate mode class.  Most of the functions are actually at global
//	scope to avoid excessive typing; only setup and teardown and other configuration
//	is kept in the class itself.
//
class grcImmediateMode {
public:
	static void InitClass();
	static void ShutdownClass();
};

#define __IM_BATCHER	(__DEV || (__BANK && HACK_GTA4))			// mushroom MUSHROOM

#if __IM_BATCHER

class grcBatcher;

extern __THREAD grcBatcher* grcBatcher_sm_Current;

/*
	Class which manages batched rendering of immediate mode primitives.
	Yes, this means it's a retained immediate mode, sigh.  This is useful for debug
	visualization, since we are not allowed to render outside of a graphics frame.

	Create a grcBatcher object, initialize it with a buffer size, and then make
	it current and do any immediate mode rendering you want with grcBegin, grcVertex*,
	and grcEnd (grcWorldMtx and grcWorldIdentity are also supported).  All rendering
	will be recorded inside the batcher object; if the buffer fills up new primitives
	are dropped.  Simply call Render() on the batcher when you want to display it.

	The entire subsystem is compiled in only when __IM_BATCHER is nonzero.
*/
class grcBatcher {
public:
	// PURPOSE: Constructor
	grcBatcher();

	// PURPOSE: Destructor
	~grcBatcher();

	// RETURNS: Currently active batcher; may be NULL if normal rendering is enabled (the default)
	static inline grcBatcher *GetCurrent();

	// PURPOSE: Makes the specified batcher current
	// PARAMS:	b - Batcher to make current; use NULL to disable all batching.
	// RETURNS:	Previously active batcher or NULL if none was active
	static grcBatcher* SetCurrent(grcBatcher* b);

	// PURPOSE:	Run the accumulated rendering
	// PARAMS:	flushAfterRender - If true, flush the render buffer when done.
	//				Otherwise, if false, you can re-use the buffer later.
	void Render(bool flushAfterRender);

	// PURPOSE:	Flush the render buffer without drawing anything
	void Flush();

	// PURPOSE: Enable copying of commands from this batcher to another batcher while rendering.
	// PARAMS:  batcher - batcher to copy to, set to NULL to disable copying
	// NOTES:   This can be changed on the fly while rendering with AddRenderCallback callbacks.
	void CopyToBatcherWhileRendering(grcBatcher *batcher);

	// PURPOSE: These are the possible types of behavior when Reserve() is called to reserve 
	// more space than is available in the buffer.
	enum BufferFullMode {
		BUF_FULL_ASSERT,				// Asserts if the buffer is being overfilled
		BUF_FULL_ONSCREEN_WARNING,		// Puts a warning message on screen if the buffer is being overfilled
		BUF_FULL_IGNORE,				// Ignores any further draw calls
		BUF_FULL_DRAW,					// Draws what's currently in the buffer, emptys the buffer.
		BUF_FULL_DRAW_OVER_PIPE,		// Draws what's in the buffer, only if there is a named pipe to send the draw commands to. Ignores additional draw commands otherwise.
		BUF_FULL_WARN_DRAW_OVER_PIPE,	// When the buffer is full, either display an onscreen warning or (if there's a named pipe to send commands to) sends the commands.
		BUF_FULL_ALLOC,					// Allocate a new buffer via callback

		BUF_FULL_INTERNAL_OSW_IGNORE,	// For internal use only, ignores additional reservations, changes mode to ONSCREEN_WARNING after flush
		BUF_FULL_INTERNAL_WDOP_IGNORE,	// For internal use only, ignores additional reservations, changes mode to WARN_DRAW_OVER_PIPE after flush
	};

	// PURPOSE: Callback type for BUF_FULL_ALLOC.
	// PARAMS:  alloc - pointer to store allocated pointer to
	//          sizeBytes - pointer to store size in bytes of allocation to
	// RETURNS: true iff allocation successful
	typedef bool BufferAllocCallback(void **alloc, size_t *sizeBytes);

	// PURPOSE: Callback type for BUF_FULL_ALLOC.
	// PARAMS:  alloc - allocation to be freed
	typedef void BufferFreeCallback(void *alloc);

	// PURPOSE:	Initialize the accumulation buffer.
	// PARAMS:	bufferSize - Size of buffer to allocate, in bytes.
	//			fullMode - What to do if the buffer fills up.
	//			buffer - Pre-allocated buffer, if so desired. Must have space for bufferSizeBytes.
	// NOTES:	It takes about 28 bytes per vertex and 48 bytes per matrix change.
	//			There is also 4 bytes per command overhead (for each Begin or SetMatrix call)
	void Init(size_t bufferSizeBytes, BufferFullMode fullMode = BUF_FULL_ASSERT, void *buffer = NULL);

	// PURPOSE:	Initialize the accumulation buffer with BUF_FULL_ALLOC mode.
	// PARAMS:  allocCallback - callback for allocating buffer memory
	//          freeCallback - callback for freeing buffer memory
	void Init(BufferAllocCallback *allocCallback, BufferFreeCallback *freeCallback);

	// PURPOSE: Mark batcher as closed.
	// NOTES:   Generally this doesn't need to be called explicitly.
	void Close();

	// PURPOSE: Check if batcher has been marked as closed.
	// RETURNS: true iff closed
	bool IsClosed() const;

	// PURPOSE: Set flag to cause attempts to render the batcher to block.
	// PARAMS:  renderMustWait - true if rendering should block.
	// NOTES:   Init and Flush will set this to false.
	void SetRenderMustWait(bool renderMustWait);

	// PURPOSE: Check if batcher is currently flagged to block rendering.
	// RETURNS: true last call to SetRenderMustWait after Init/Flush specified true.
	bool GetRenderMustWait() const;

	// PURPOSE:	Begin a batched immediate mode primitive.
	// PARAMS:	dm - Draw mode
	//			count - Vertex count
	// NOTES:	You generally don't need to call this yourself; grcBegin will
	//			do this automatically if it detects a current batcher.
	void Begin(grcDrawMode dm,int count);

	// PURPOSE:	Send a batched immediate mode vertex
	// NOTES:	You generally don't need to call this yourself; grcVertex will
	//			do this automatically if it detects a current batcher.
	void Vertex(float x,float y,float z,float nx,float ny,float nz,Color32 c,float s,float t);

	// PURPOSE:	Send a batched immediate mode matrix change
	// RETURNS: Previous world matrix
	// NOTES:	You generally don't need to call this yourself; grcWorldMtx will
	//			do this automatically if it detects a current batcher.
	Mat34V SetWorld(const Mat34V &mtx);

	// PURPOSE:	Gets the batched immediate mode matrix
	inline const Mat34V &GetWorld() const {
		return m_WorldMtx;
	}

	// PURPOSE:	Send a batched immediate mode matrix change to identity matrix
	// NOTES:	You generally don't need to call this yourself; grcWorldMtx will
	//			do this automatically if it detects a current batcher.
	void SetWorldIdentity();

	// PURPOSE:	Send a batched immediate mode lighting state change
	void SetLighting(bool enable);
	void SaveLightingState();
	void RestoreLightingState();

	// PURPOSE: Send a batched immediate mode state change
	void SetRasterizerState(grcRasterizerStateHandle rasterizerState);
	void SetBlendState(grcBlendStateHandle blendState);
	void SetDepthStencilState(grcDepthStencilStateHandle depthStencilState);
	//-------
	void SaveRasterizerState();
	void SaveBlendState();
	void SaveDepthStencilState();
	//-------
	void RestoreRasterizerState();
	void RestoreBlendState();
	void RestoreDepthStencilState();

	// PURPOSE:	End a batched immediate mode primitve
	// NOTES:	You generally don't need to call this yourself; grcEnd will
	//			do this automatically if it detects a current batcher.
	void End();

	// RETURNS: Pointer to payloadSize bytes if available, else NULL
	// NOTES:	Lower eight bits of cmd are the command field; upper 24
	//			bits are available for any other data that will fit.
	void *Reserve(u32 cmd,size_t payloadSize);

	// RETURNS: The current cursor position of the batcher.
	inline const u32 *GetCursor() const
	{
		return m_Curr;
	}

	// PURPOSE: Adds the data to a pre-reserved section of the display list
	// NOTES:	This adds to the most recently reserved section of the list.
	//			Also, data must be 32bit aligned.
	template <typename _T> void PushData(const _T& data, int offset = 0, int size = sizeof(_T)) {
#if HACK_GTA4 || __DEV
		if ((m_FullMode != BUF_FULL_ASSERT && !m_CurrReserved)) {
			return;
		}
#endif // HACK_GTA4 || __DEV
#if __DEV
		if (m_FreezeBatcher)
			return;
#endif // __DEV
		FastAssert(m_CurrReserved);
		FastAssert(m_CurrReserved + ((offset + size)/sizeof(u32)) <= m_End);
		m_CurrReserved += (offset/sizeof(u32));
		memcpy(m_CurrReserved, (void*) &data, size);	// source of this 'memcpy' call is a pointer to dynamic class 'const rage::grcDrawProxy'; vtable pointer will be copied [-Werror,-Wdynamic-class-memaccess]
		m_CurrReserved += (size/sizeof(u32));
		if (m_CurrReserved == m_Curr)
			m_CurrReserved = (u32*)-1;  // reached end of reservation, crash if more data is pushed
	}

#if __DEV
	void SetFreezeBatcher(bool freeze)
	{
		m_FreezeBatcher = freeze;
	}
#endif

	// PURPOSE: Callback type for AddRenderCallback.
	// PARAMS:  data - pointer to data embedded in command
	//          size - size in bytes of data embedded in command
	//          batcher - batcher being rendered
	typedef void RenderCallback(const void *data, size_t size, grcBatcher *batcher);

	// Flags for AddRenderCallback
	enum
	{
		ARC_NEVER_COPY          = 1
	};

	// PURPOSE: Add a command to the batcher to call a callback when rendered.
	// PARAMS:  callback - callback function
	//          size - size in bytes of data to embed in command
	//          flags - ARC_*
	// RETURNS: pointer to data embedded in command
	void *AddRenderCallback(RenderCallback *callback, size_t size, u32 flags=0);

	// PURPOSE: Templated helper version of AddRenderCallback.
	// PARAMS:  T - type of data to be embedded in command
	//          callback - callback function
	//          flags - ARC_*
	// RETURNS: pointer to data embedded in command
	template<class T>
	T *AddRenderCallback(RenderCallback *callback, u32 flags=0)
	{
		// If larger alignment than 4-bytes is required, then caller should use
		// the basic AddRenderCallback overload, allocate additional padding,
		// then align the pointer in the callback function.
		CompileTimeAssert(__alignof(T) <= 4);
		return (T*)AddRenderCallback(callback, sizeof(T), flags);
	}

private:
	Mat34V m_WorldMtx;

	u32 *m_BeginAll;        // First command, not necessarily in the current segment
	u32 *m_BeginSeg;        // First command in current segment (points to NEXT command)
	u32 *m_Curr;            // One past last writen command
	u32 *m_CurrReserved;    // Pointer into currently being built command, NULL if no reserve, -1 reserve finished (to cause crash if not enough was reserved)
	u32 *m_End;             // End of allocated memory in current segment

	BufferFullMode m_FullMode;
	BufferAllocCallback *m_AllocCallback;
	BufferFreeCallback  *m_FreeCallback;

	struct BatchVtx;
	BatchVtx *m_Vtx;

	grcBatcher *m_CopyToBatcherWhileRendering;

	bool m_IsUserBuffer;		// If true, m_Buffer was provided by the user
	bool m_Closed;
	bool m_RenderMustWait;

#if __DEV
	bool m_FreezeBatcher;
#endif

	void FreeBufferMemory();
	void LinkNewBuffer(void *buffer, size_t size);
	void RenderInternal();
};

grcBatcher* grcBatcher::GetCurrent() {
	return grcBatcher_sm_Current;
}

#endif

// PURPOSE: Current packed color as per most recent grcColor, grcColor3f, or grcColor4f call.
extern __THREAD u32 grcCurrentColor;

// PURPOSE: Current texture coordinates as per most recent grcTexCoord2f call.
extern __THREAD float grcCurrentS, grcCurrentT;

// PURPOSE: Current normal
extern __THREAD float grcCurrentNX, grcCurrentNY, grcCurrentNZ;

// PURPOSE: Previous render states 
extern __THREAD grcRasterizerStateHandle   grcRS_Previous;
extern __THREAD grcBlendStateHandle        grcBS_Previous;
extern __THREAD grcDepthStencilStateHandle grcDSS_Previous;
extern __THREAD bool                       grcLightState_PreviousState;

#if __WIN32PC
// PURPOSE: Maximum number of vertices in a single cmcBegin call.
const int grcBeginMax = 16384;

// PURPOSE: Maximum number of vertices in a single cmcBegin call that is evenly divisible by three (useful for independent triangles)
const int grcBeginMax3 = 16383;
#else
// PURPOSE: Maximum number of vertices in a single cmcBegin call.
const int grcBeginMax = __PS3? 340 : 1024;

// PURPOSE: Maximum number of vertices in a single cmcBegin call that is evenly divisible by three (useful for independent triangles)
const int grcBeginMax3 = __PS3? 339 : 1023;
#endif

// PURPOSE: used with grcDrawArrowOrtho
const float fGRCDefaultArrowheadLength				= 0.8f;
const float fGRCDefaultArrowheadBaseHalfLength		= 0.2f;

// PURPOSE: Begins rendering a single primitive.
// PARAMS:	dm - Type of primitive to render
//			vertexCount - Number of vertices (not primitives) to expect.
// NOTES:	The immediate mode API has been designed to keep overhead as low as possible,
//			but it's still worth it to batch independent lines or triangles into larger
//			calls when possible.  Making any state changes after a grcBegin call is an
//			error and will result in nonportable code.
extern void grcBegin(grcDrawMode dm,int vertexCount);

#if __WIN32PC
// PURPOSE: Begins rendering an indexed primitive.
// PARAMS:	dm - Type of primitive to render
//			vertexCount - Number of vertices (not primitives) to expect.
//			indexCount - Number of indices to expect.
// NOTES:	The immediate mode API has been designed to keep overhead as low as possible,
//			but it's still worth it to batch independent lines or triangles into larger
//			calls when possible.  Making any state changes after a grcBegin call is an
//			error and will result in nonportable code.
extern void grcBeginIndexed(grcDrawMode dm, int vertexCount, int indexCount);
#endif

// PURPOSE:	Fully specifies a single vertex for rendering.
// PARAMS:	x, y, z - Position
//			nx, ny, nz - Normal
//			c - Packed color
//			s, t - Texture coordinate
// NOTES:	All other grcVertex functions are implemented in terms of this one.
extern void grcVertex(float x,float y,float z,float nx,float ny,float nz,Color32 c,float s,float t);

// PURPOSE:	Specify the current texture coordinates
// PARAMS:	s, t - New current texture coordinates
// NOTES:	These texture coordinates are used implicitly by subsequent grcVertex[23][fi][v] function calls.
static inline void grcTexCoord2f(float s,float t) { 
	grcCurrentS = s; 
	grcCurrentT = t; 
}

static inline void grcTexCoord2f(Vec2V_In t) { 
	grcCurrentS = t.GetXf(); 
	grcCurrentT = t.GetYf(); 
}

// PURPOSE:	Specify the current packed color
// PARAMS:	c - New current color
// NOTES:	This color is used implicitly by subsequent grcVertex[23][fi][v] function calls.
static inline void grcColor(Color32 c) { 
	grcCurrentColor = c.GetColor();
}

// PURPOSE:	Specify the current color (implicit alpha of 1.0)
// PARAMS:	r, g, b - New current normalized color
// NOTES:	This color is used implicitly by subsequent grcVertex[23][fi][v] function calls.
static inline void grcColor3f(float r,float g,float b) { 
	grcColor(Color32(r,g,b)); 
}

// PURPOSE:	Specify the current color  (implicit alpha of 1.0)
// PARAMS:	c - New current normalized color
// NOTES:	This color is used implicitly by subsequent grcVertex[23][fi][v] function calls.
static inline void grcColor3f(const Vector3 &c) { 
	grcColor3f(c.x,c.y,c.z); 
}

static inline void grcColor3f(Vec3V_In c) { 
	grcColor(Color32(c));
}

// PURPOSE:	Specify the current color (with explicit alpha)
// PARAMS:	r, g, b, a - New current normalized color
// NOTES:	This color is used implicitly by subsequent grcVertex[23][fi][v] function calls.
static inline void grcColor4f(float r,float g,float b,float a) { 
	grcColor(Color32(r,g,b,a)); 
}

// PURPOSE:	Specify the current color (with explicit alpha)
// PARAMS:	c - New current normalized color
// NOTES:	This color is used implicitly by subsequent grcVertex3* function calls.
static inline void grcColor4f(const Vector4 &c) { 
	grcColor4f(c.x,c.y,c.z,c.w); 
}

static inline void grcColor4f(Vec4V_In c) { 
	grcColor(Color32(c));
}

// PURPOSE: Get the current color.
static inline Color32 grcGetCurrentColor() {
	return Color32(grcCurrentColor);
}

// PURPOSE:	Send down a new vertex using current color and texture coordinates.
// PARAMS:	x, y, z - New position
static inline void grcVertex3f(float x,float y,float z) { 
	grcVertex(x,y,z,grcCurrentNX,grcCurrentNY,grcCurrentNZ,Color32(grcCurrentColor),grcCurrentS,grcCurrentT); 
}

// PURPOSE:	Send down a new vertex using current color and texture coordinates.
// PARAMS:	v - New position
static inline void grcVertex3f(const Vector3 &v) {
	grcVertex3f(v.x,v.y,v.z);
}

// PURPOSE:	Send down a new vertex using current color and texture coordinates.
// PARAMS:	v - New position
static inline void grcVertex3f(Vec3V_In v) {
	grcVertex3f(v.GetXf(),v.GetYf(),v.GetZf());
}

// PURPOSE:	Send down a new vertex using current color and texture coordinates.
// PARAMS:	v - New position (three-element array)
static inline void grcVertex3fv(const float *v) { 
	grcVertex3f(v[0],v[1],v[2]); 
}

// PURPOSE:	Send down a new vertex using current color and texture coordinates (implicit z of zero)
// PARAMS:	x, y - New position
static inline void grcVertex2f(float x,float y) { 
	grcVertex3f(x,y,0); 
}

// PURPOSE:	Send down a new vertex using current color and texture coordinates (implicit z of zero)
// PARAMS:	v - New position
static inline void grcVertex2f(const Vector2 &v) { 
	grcVertex3f(v.x,v.y,0); 
}

static inline void grcVertex2f(Vec2V_In v) { 
	grcVertex3f(v.GetXf(),v.GetYf(),0);
}

// PURPOSE:	Send down a new vertex using current color and texture coordinates (implicit z of zero)
// PARAMS:	x, y - New position (as an integer)
static inline void grcVertex2i(int x,int y) { 
	grcVertex3f((float)x,(float)y,0); 
}

// PURPOSE: Set the current normal
// PARAMS:	x, y, z - New normal
static inline void grcNormal3f(float x,float y,float z) {
	grcCurrentNX = x;
	grcCurrentNY = y;
	grcCurrentNZ = z;
}

static inline void grcNormal3f(const Vector3 &v) {
	grcNormal3f(v.x,v.y,v.z);
}

static inline void grcNormal3f(Vec3V_In v) {
	grcNormal3f(v.GetXf(),v.GetYf(),v.GetZf());
}

// PURPOSE:	Terminate current primitive
// NOTES:	You must have specified exactly as many vertices as you promised when calling grcBegin
//			or else either grcVertex will assert out if you gave too many, or this function will
//			assert out if you didn't supply enough.
extern void grcEnd();
#if __D3D11
extern void grcEnd(u32 numVertices);
#endif

#if __WIN32PC
// PURPOSE:	Send down a new index to the index buffer.
// PARAMS:	index - New index number;
// NOTES:	
extern void grcIndex(u16 index);

// PURPOSE:	Terminate current primitive
// PARAMS:	vertexCount - Number of vertices to render.
// NOTES:	This is implemented a bit differently from the above grcEnd() so that it doesn't matter
//			how many vertices you specified in grcBegin, it will render as many as you specified
//			in the vertexCount variable. Makes it easier if you don't know how many vertices you
//			need to render at the time of the grcBegin call.
extern void grcEndIndexed(int indexCount, int vertexCount);
#endif

// PURPOSE: Convenience function to get current world matrix.
// NOTES:	This is implemented in terms of grcViewport and is provided so that many
//			types of debugging code doesn't otherwise need to depend directly on grcore/viewport.h.
//			This is also the version you should use if you want the matrix changes to be
//			properly recorded by the batcher.
extern Mat34V grcWorldMtx();

// PURPOSE:	Convenience function to specify current world matrix.
// PARAMS:	mtx - New world matrix
// RETURNS: previous world matrix
// NOTES:	This is implemented in terms of grcViewport and is provided so that many
//			types of debugging code doesn't otherwise need to depend directly on grcore/viewport.h.
//			This is also the version you should use if you want the matrix changes to be
//			properly recorded by the batcher.
extern Mat34V grcWorldMtx(Mat34V_In mtx);
extern Matrix34 grcWorldMtx(const Matrix34& mtx);

// PURPOSE:	Convenience function to set current world matrix to identity
// NOTES:	This is implemented in terms of rmcState::SetIdentity and is provided so that many
//			types of debugging code doesn't otherwise need to depend directly on grcore/state.h.
//			This is also the version you should use if you want the matrix changes to be
//			properly recorded by the batcher.
extern void grcWorldIdentity();

// PURPOSE:	Convenience function to change lighting mode
// PARAMS:	enable - Lighting enable
// RETURNS:	Previous lighting enable
// NOTES:	This is implemented in terms of rmcState::SetLighting Mode and is provided so that many
//			types of debugging code doesn't otherwise need to depend directly on grcore/state.h.
// WARNING:	if you want to save the lighting state use grcDebugDraw::Push/PopLightingState() instead of this !
extern bool grcLighting(bool enable);

// PURPOSE:	Convenience function which binds a new stage zero texture
// PARAMS:	tex - New texture to bind, or NULL to disable texturing
extern void grcBindTexture(const grcTexture *tex);

// PURPOSE:	Returns active texture, or NULL if none.
// NOTES:	We return NULL even if we called grcBindTexture(grcTexture::None).
extern const grcTexture* grcGetTexture();

// PURPOSE:	Shorthand for grcTextureFactory::GetCurrent().Create(name);
extern grcTexture* grcCreateTexture(const char *name);

// PURPOSE:	Shorthand for if (tex) tex->Release();
extern void grcReleaseTexture(grcTexture *tex);

// PURPOSE: Shorthand for grcTextureFactory::GetInstance().PlaceTexture(rsc,&texture,1)
extern void grcFixupTexture(datResource&,grcTexture*&);

// PURPOSE: Convenience function to draw a single line segment
extern void grcDrawLine(const Vector3 &v0,const Vector3 &v1,const Color32 color); //MDFTODO: make all color params in here const?
extern void grcDrawLine(Vec3V_In v0,Vec3V_In v1,const Color32 color);

// PURPOSE: Convenience function to draw a single line segment with 2 different colors
extern void grcDrawLine(const Vector3 &v0,const Vector3 &v1,Color32 color, Color32 colorB);

// PURPOSE: Convenience function to draw an arrow with the arrowhead orthogonal to the local camera view (camera at vCameraPosition)
extern void grcDrawArrowOrtho( const Vector3& vStartPos, const Vector3& vEndPos, const Color32& Color, const Vector3& vCameraPosition, float fArrowheadLength=fGRCDefaultArrowheadLength, float fArrowheadBaseHalfLength=fGRCDefaultArrowheadBaseHalfLength );

// PURPOSE:	Convenience function to draw a 2D wireframe quad, connecting p1->p2->p3->p4
extern void grcDraw2dQuad(const Vector3& p1, const Vector3& p2, const Vector3& p3, const Vector3& p4, Color32 color);

// PURPOSE:	Convenience function to draw a wireframe box
extern void grcDrawBox(const Vector3 &size,const Matrix34 &mtx, Color32 color);


// PURPOSE:	Convenience function to draw a bounding box
extern void grcDrawBox(const Vector3 &min,const Vector3& max, Color32 color);

// PURPOSE: Convenience function to draw a solid box
extern void grcDrawSolidBox(const Vector3 &size,const Matrix34 &mtx,Color32 color);

// PURPOSE: Convenience function to draw a solid box
extern void grcDrawSolidBox(const Vector3& min,const Vector3& max, Color32 color);

// PURPOSE: Convenience function to draw a frustum
extern void grcDrawFrustum(const Matrix34& mtx, float sx, float sy, float sz, Color32 color);

// PURPOSE: Convenience function to draw an ellipsoid
void grcDrawEllipsoid(const Vector3 &size,const Matrix34 &mtx,int steps=8,bool longitudinalCircles=false,bool solid=false);
void grcDrawEllipsoid(Vec3V_In size,Mat34V_In mtx,int steps=8,bool longitudinalCircles=false,bool solid=false);

// PURPOSE: Convenience function to draw an ellipse
void grcDrawEllipse(const float sizeX, const float sizeY,const Matrix34 &mtx,int steps=8);

// PURPOSE: Convenience function to draw a sphere
void grcDrawSphere(float rad,const Vector3 &center,int steps=8,bool longitudinalCircles=false,bool solid=false);
void grcDrawSphere(float rad,Vec3V_In center,int steps=8,bool longitudinalCircles=false,bool solid=false);

// PURPOSE: Convenience function to draw a sphere
void grcDrawSphere(float rad,const Matrix34 &mtx,int steps=8,bool longitudinalCircles=false,bool solid=false);
void grcDrawSphere(float rad,Mat34V_In mtx,int steps=8,bool longitudinalCircles=false,bool solid=false);

// PURPOSE: Convenience function to draw a sphere
void grcDrawSphere(float rad,int steps=8);

void grcDrawPartialSphere(Vec3V_In center, float radius, Vec3V_In direction, float angle, int steps, bool longitudinalCircles, bool solid);
void grcDrawPartialSphere(Mat34V_In mtx, float radius, float angle, int steps, bool longitudinalCircles, bool solid);

// PURPOSE: Convenience function to draw a spiral
void grcDrawSpiral(const Vector3& start, const Vector3& end, float startRadius, float endRadius, float revolutionsPerMeter, float initialPhase = 0.0f, float arrowLength = 0.0f, int steps=12);

// PURPOSE: Convenience function to draw a capsule (two hemispheres connected by a cylinder).
void grcDrawCapsule(float length,float radius,const Matrix34 &mtx,int steps,bool solid=false, float halfHeight=0.0f);

// PURPOSE: Convenience function to draw a capsule (two hemispheres connected by a cylinder).
void grcDrawCapsule(float length,float radius,const Vector3 &center,int steps,bool solid=false);

// PURPOSE: Convenience function to draw a tapered capsule (sphere parts connected by a tapered cylinder).
void grcDrawTaperedCapsule(float length,float radiusA,float radiusB,const Matrix34 &mtx,int steps,bool solid=false);

// PURPOSE: Convenience function to draw a circle of arbitrary orientation.
void grcDrawCircle(float r, const Vector3 &center, const Vector3 &axisX, const Vector3 &axisY, int steps=11, bool dashed=false, bool solid=false);

// PURPOSE: Convenience function to draw an arc or wedge of a circle of arbitrary orientation.
void grcDrawArc(float r, const Vector3 &center, const Vector3 &axisX, const Vector3 &axisY, float beginAngle, float endAngle, int steps=11, bool solid=false);

// PURPOSE: Convenience function to draw a circle oriented in the x-z plane.
void grcDrawFlatCircle(float r, const Vector3 &center, int steps=11, bool dashed=false);

// PURPOSE: Convenience function to draw a cylinder of arbitrary orientation.
void grcDrawCylinder(float length,float radius,const Matrix34 &mtx,int steps, const bool cbDrawTop);

// PURPOSE: Convenience function to draw a cylinder oriented along the y-axis.
void grcDrawCylinder(float length,float radius,const Vector3 &center,int steps, const bool cbDrawTop);

void grcDraw2dText(float x, float y, const char* str,bool drawBgQuad=false, float scaleX = 1.0f, float scaleY = 1.0f, const grcFont* font = NULL);

void grcDrawAxis(float size,const Matrix34 &mtx, bool drawArrows=false);
void grcDrawAxis(float size,Mat34V_In mtx, bool drawArrows=false);

void grcDrawSolidSphericalCone(const Vector3 &pos,
							const Vector3 &dir, const float halfAngleRadians, const float radius,
							const Color32 solidCol, const Color32 lineCol,
							const int coneSteps, const int capSteps);


void grcDrawLabelf(const Vector3 &,int xOffset,int yOffset,const char *fmt,...);
void grcDrawLabelf(const Vector3 &,const char *fmt,...);
void grcDrawLabel(const Vector3 &,int xOffset,int yOffset,const char *fmt,bool drawBgQuad=false, float xScale = 1.0f, float yScale = 1.0f);
void grcDrawLabel(const Vector3 &,const char *fmt,bool drawBgQuad=false);
void grcDrawLabel(Vec3V_In ,const char *fmt,bool drawBgQuad=false);

void grcDrawPolygon(const Vector3 *points, int count, const Vector3 *pNormal, bool solid, Color32 color);
void grcDrawTexturedPolygon(const Vector3 *points, int count, const Vector3 *pNormal, bool solid, Color32 color, grcTexture *pTex, const Vector2 *uvs);
void grcDrawPolygon(const Vector3 *points, int count, const Vector3 *pNormal, bool solid, Color32 *pColor);
void grcDrawTexturedPolygon(const Vector3 *points, int count, const Vector3 *pNormal, bool solid, Color32 *pColor, grcTexture *pTex, const Vector2 *uvs);
void grcDrawPolygon(const atArray<Vector3> &points, const Vector3 *pNormal, bool solid, Color32 color);

class grcDrawProxy
{
public:
	explicit grcDrawProxy() {}
	virtual ~grcDrawProxy() {}

	virtual void Draw() const = 0;
	virtual int  Size() const = 0;
};

void grcDraw(const grcDrawProxy& proxy);

void grcVertexArray(grcDrawMode dm, const atArray<Vec3V>& verts);

enum grcProjectStatus { 
	grcProjectBehind,		// Object has negative W, behind the player in negative clip space
	grcProjectOffscreen, 
	grcProjectVisible
};
grcProjectStatus grcProject(Vec3V_In V,float &x,float &y);	// uses current viewport and view matrix
grcProjectStatus grcProject(Vec3V_In V,float &x,float &y,Mat44V_In viewMatrix,const grcViewport &viewport);

}	// namespace rage

#endif
