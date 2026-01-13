// 
// grcore/im_common.cpp 
// 
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved. 
// 

#include "im.h"

#include "allocscope.h"
#include "font.h"
#include "effect.h"
#include "light.h"
#include "texture.h"
#include "quads.h"
#include "viewport.h"
#include "file/asset.h"
#include "math/angmath.h"
#include "string/string.h"
#include "system/alloca.h"
#include "system/namedpipe.h"
#include "vectormath/legacyconvert.h"

#include <stdarg.h>

namespace rage {

bool grcFlashEnable;

__THREAD u32 grcCurrentColor;
__THREAD float grcCurrentS, grcCurrentT;
__THREAD float grcCurrentNX, grcCurrentNY, grcCurrentNZ;

__THREAD grcRasterizerStateHandle   grcRS_Previous;
__THREAD grcBlendStateHandle        grcBS_Previous;
__THREAD grcDepthStencilStateHandle grcDSS_Previous;
__THREAD bool                       grcLightState_PreviousState=false;

#if __IM_BATCHER

// There can be at most 255 of these commands
enum {
	JUMP,									// LinkNewBuffer
	CLOSE,									// Close
	NEXT,									// pointer to next buffer stored at the start of the current buffer, used to quickly jump through list
	RENDER_CALLBACK,                        // AddRenderCallback
	DRAW,									// draws with a draw mode and a group of vertices (each with xyz, rgb, st components)
	WORLD,									// sets the world matrix
	WORLDIDENTITY,							// sets the world matrix to identity matrix
	LIGHTING,								// sets the lighting mode
	SAVE_LIGHTING,							// saves the lighting mode
	RESTORE_LIGHTING,						// restores the lighting mode
	DRAW_LINE,								// grcDrawLine
	DRAW_LINE_2C,							// grcDrawLine 2 colors
	DRAW_QUAD2D,							// grcDraw2dQuad
	DRAW_STRING2D,							// grcDraw2dString
	DRAW_LABEL,								// grcDrawLabel
	DRAW_BOX,								// grcDrawBox
	DRAW_SOLIDBOX,							// grcDrawSolidBox
	DRAW_FRUSTUM,							// grcDrawFrustum
	DRAW_ELLIPSOID,							// grcDrawEllipsoid
	DRAW_SPHERE,							// grcDrawSphere
	DRAW_SPIRAL,							// grcDrawSpiral
	DRAW_CIRCLE,							// grcDrawCircle
	DRAW_ARC,								// grcDrawArc
	DRAW_CAPSULE,							// grcDrawCapsule
	DRAW_TAPERED_CAPSULE,					// grcDrawTaperedCapsule
	DRAW_CYLINDER,							// grcDrawCylinder
	DRAW_AXIS,								// grcDrawAxis
	DRAW_POLYGON,							// grcDrawPolygon
	DRAW_POLYGON_TEX,						// grcDrawTexturedPolygonTex
	DRAW_POLYGON_COLOR,						// grcDrawPolygon with per vtx color
	DRAW_POLYGON_TEX_COLOR,					// grcDrawTexturedPolygon with per vtx color
	DRAW_PARTIAL_SPHERE,					// grcDrawPartialSphere
	DRAW_PROXY,
	DRAW_FULL_BUFFER_WARNING,
	DRAW_FULL_BUFFER_NO_PIPE_WARNING,
	RASTERIZERSTATE,
	BLENDSTATE,
	DEPTHSTENCILSTATE,
	SAVE_RASTERIZERSTATE,
	SAVE_BLENDSTATE,
	SAVE_DEPTHSTENCILSTATE,
	RESTORE_RASTERIZERSTATE,
	RESTORE_BLENDSTATE,
	RESTORE_DEPTHSTENCILSTATE,
};

// Reserved size so that buffer can always fit a JUMP or CLOSE command at the end.
enum {
	BUFFER_RESERVED_END_BYTES	= (sizeof(u32)+sizeof(void*)),
	BUFFER_MIN_SIZE_BYTES		= (sizeof(u32)+sizeof(void*)) + BUFFER_RESERVED_END_BYTES,
};

__THREAD grcBatcher* grcBatcher_sm_Current;

grcBatcher::grcBatcher()
	: m_WorldMtx(Mat34V(V_IDENTITY))
	, m_BeginAll(NULL)
	, m_BeginSeg(NULL)
	, m_Curr(NULL)
	, m_End(NULL)
	, m_FullMode(BUF_FULL_ASSERT)
	, m_AllocCallback(NULL)
	, m_FreeCallback(NULL)
	, m_Vtx(NULL)
	, m_CopyToBatcherWhileRendering(NULL)
	, m_IsUserBuffer(false)
	, m_Closed(false)
#if __DEV
	, m_FreezeBatcher(false)
#endif
{
}

void grcBatcher::FreeBufferMemory() {
	u32 *buf = m_BeginAll;
	if (buf) {
		if (m_FullMode == BUF_FULL_ALLOC) {
			BufferFreeCallback *const freeCallback = m_FreeCallback;
			do {
				Assert(*buf == NEXT);
				u32 *next;
				sysMemCpy(&next, buf+1, sizeof(next));
				freeCallback(buf);
				buf = next;
			} while (buf);
		}
		else if (!m_IsUserBuffer)
			delete [] buf;
	}
}

grcBatcher::~grcBatcher() {
	FreeBufferMemory();
}

static inline u32 *StoreNext(u32 *cmd, const u32 *next) {
	*cmd = NEXT;
	sysMemCpy(cmd+1, &next, sizeof(void*));
	return cmd+1+sizeof(void*)/4;
}

void grcBatcher::Init(size_t bufferSizeBytes, BufferFullMode fullMode, void *buffer) {
	Assert(fullMode != BUF_FULL_ALLOC); // use Init(BufferAllocCallback*,BufferFreeCallback*) overload instead
	FreeBufferMemory();
	CompileTimeAssert((BUFFER_MIN_SIZE_BYTES & 3) == 0);
	Assert(!bufferSizeBytes || bufferSizeBytes>BUFFER_MIN_SIZE_BYTES);
	if (bufferSizeBytes)
	{
		size_t bufferSize32 = bufferSizeBytes/4;
		m_IsUserBuffer = (buffer != NULL);
		m_BeginAll = m_BeginSeg = m_Curr = (buffer) ? (u32*)buffer : rage_new u32[bufferSize32];
		m_End = m_BeginSeg + bufferSize32 - BUFFER_RESERVED_END_BYTES/4;
	}
	else
	{
		m_IsUserBuffer = false;
		m_BeginAll = m_BeginSeg = m_Curr = m_End = NULL;
	}
	m_FullMode = fullMode;
	m_Vtx = NULL;
	m_Closed = false;
	m_CopyToBatcherWhileRendering = NULL;
	m_RenderMustWait = false;
	m_CurrReserved = NULL;
}

void grcBatcher::Init(BufferAllocCallback *allocCallback, BufferFreeCallback *freeCallback) {
	FreeBufferMemory();
	m_FullMode = BUF_FULL_ALLOC;
	m_AllocCallback = allocCallback;
	m_FreeCallback = freeCallback;
	m_BeginAll = m_BeginSeg = m_Curr = m_End = NULL;
	m_Vtx = NULL;
	m_IsUserBuffer = false;
	m_Closed = false;
	m_CopyToBatcherWhileRendering = NULL;
	m_RenderMustWait = false;
	m_CurrReserved = NULL;
}

void grcBatcher::LinkNewBuffer(void *buffer, size_t sizeBytes) {
	Assert(buffer);
	Assert(((uptr)buffer & 3) == 0);
	Assert(m_FullMode == BUF_FULL_ALLOC);
	Assert(sizeBytes >= BUFFER_MIN_SIZE_BYTES);
	if (m_Curr) {
		Assert(m_BeginSeg && *m_BeginSeg==NEXT);
		StoreNext(m_BeginSeg, (u32*)buffer);
		Assert(m_Curr+1+sizeof(void*)/4 <= m_End+BUFFER_RESERVED_END_BYTES/4);
		m_Curr[0] = JUMP;
		sysMemCpy(m_Curr+1, &buffer, sizeof(void*));
		m_BeginSeg = (u32*)buffer;
	}
	else {
		Assert(!m_BeginAll && !m_BeginSeg && !m_End);
		m_BeginAll = m_BeginSeg = (u32*)buffer;
	}
	m_End = m_BeginSeg + sizeBytes/4 - BUFFER_RESERVED_END_BYTES/4;
	m_Curr = StoreNext(m_BeginSeg, NULL);
}

void grcBatcher::Close() {
	Assert(!m_Closed);
	if (m_Curr) {
		Assert(m_Curr+1 <= m_End+BUFFER_RESERVED_END_BYTES/4);
		*m_Curr++ = CLOSE;
	}
	m_Closed = true;
}

bool grcBatcher::IsClosed() const {
	return m_Closed;
}

void grcBatcher::SetRenderMustWait(bool renderMustWait) {
	m_RenderMustWait = renderMustWait;
}

bool grcBatcher::GetRenderMustWait() const {
	return m_RenderMustWait;
}

grcBatcher* grcBatcher::SetCurrent(grcBatcher *b) {
	grcBatcher *old = grcBatcher_sm_Current;
	grcBatcher_sm_Current = b;
	return old;
}

struct grcBatcher::BatchVtx {
	float x, y, z;
	float nx, ny, nz;
	Color32 c;
	float s, t;
};

void grcBatcher::Begin(grcDrawMode dm,int count) {
	Assert(count <= grcBeginMax);
	m_Vtx = (BatchVtx*) Reserve(DRAW | (dm << 8) | (count << 11),count * sizeof(BatchVtx));
}

void grcBatcher::Vertex(float x,float y,float z,float nx,float ny,float nz,Color32 c,float s,float t) {
	if (m_Vtx) {
		BatchVtx *v = m_Vtx++;
		v->x = x;
		v->y = y;
		v->z = z;
		v->nx = nx;
		v->ny = ny;
		v->nz = nz;
		v->c = c;
		v->s = s;
		v->t = t;
	}
}

void grcBatcher::End() {
	m_Vtx = NULL;
}

void grcBatcher::SetWorldIdentity() {
	Reserve(WORLDIDENTITY, 0);
}

Mat34V grcBatcher::SetWorld(Mat34V_In mtx) {
	Mat34V prev = m_WorldMtx;
	m_WorldMtx = mtx;
	void *m = Reserve(WORLD,sizeof(mtx));
	if (m)
		memcpy(m,&mtx,sizeof(mtx));
	return prev;
}

void grcBatcher::SetLighting(bool enable) {
	int *l = (int*) Reserve(LIGHTING,sizeof(int));
	if (l)
		*l = enable;
}

void grcBatcher::SetRasterizerState(grcRasterizerStateHandle h) {
	u32 *l = (u32 *) Reserve(RASTERIZERSTATE,sizeof(int)*1);
	if (l)
		l[0] = h;
}

void grcBatcher::SetBlendState(grcBlendStateHandle h) {
	u32 *l = (u32 *) Reserve(BLENDSTATE,sizeof(int)*1);
	if (l)
		l[0] = h;
}

void grcBatcher::SetDepthStencilState(grcDepthStencilStateHandle h) {
	u32 *l = (u32 *) Reserve(DEPTHSTENCILSTATE,sizeof(int)*1);
	if (l)
		l[0] = h;
}
//-------
void grcBatcher::SaveRasterizerState()
{
	Reserve(SAVE_RASTERIZERSTATE, 0);
}
void grcBatcher::SaveBlendState()
{
	Reserve(SAVE_BLENDSTATE, 0);
}
void grcBatcher::SaveDepthStencilState()
{
	Reserve(SAVE_DEPTHSTENCILSTATE, 0);
}
void grcBatcher::SaveLightingState()
{
	Reserve(SAVE_LIGHTING, 0);
}
//-------
void grcBatcher::RestoreRasterizerState()
{
	Reserve(RESTORE_RASTERIZERSTATE, 0);
}
void grcBatcher::RestoreBlendState()
{
	Reserve(RESTORE_BLENDSTATE, 0);
}
void grcBatcher::RestoreDepthStencilState()
{ 
	Reserve(RESTORE_DEPTHSTENCILSTATE, 0);
}
void grcBatcher::RestoreLightingState()
{
	Reserve(RESTORE_LIGHTING, 0);
}

template <typename _T>
void GetFromBuffer(const u32*& cursor, _T& data) {
	memcpy(&data, cursor, sizeof(_T));
	cursor += sizeof(_T)/sizeof(u32);
}

const grcDrawProxy& GetDrawProxyFromBuffer(const u32*& cursor, int offset) {
	cursor += offset;
	const grcDrawProxy& proxy = *(const grcDrawProxy*)cursor;
	cursor += proxy.Size()/sizeof(u32);
	return proxy;
}

void GetAndSetColor(const u32*& cursor) {
	Color32 color;
	GetFromBuffer(cursor, color);
	grcColor(color);
}

void grcBatcher::RenderInternal() {
	const u32 *cursor = m_BeginAll;
	Assert(((uptr)cursor & 3) == 0);

	// Not an error for m_BeginAll to be NULL, can validly happen when no commands are batched.
	if (!cursor)
		return;

	char *tempBuf = NULL;
	int tempBufSize = 0;
	const u32 *prevCursor = NULL;
	for (;;) {
		// Note that we need to refetch m_CopyToBatcherWhileRendering every
		// command loop, since callback commands can modify this.
		grcBatcher *const copyToBatcherWhileRendering = m_CopyToBatcherWhileRendering;
		if (copyToBatcherWhileRendering && prevCursor) {
			const u32 prevCmd = *prevCursor&255;
			const size_t payloadSizeBytes = (cursor-prevCursor-1)*4;
			void *const dst = copyToBatcherWhileRendering->Reserve(prevCmd, payloadSizeBytes);
			Assert(dst);
			sysMemCpy(dst, prevCursor+1, payloadSizeBytes);
		}
		prevCursor = cursor;

		// An alloc scope per-primitive is a little excessive, but a single
		// batcher can be gigantic in some debug situations, and will not
		// necessarily fit in memory within a single scope.
		GRC_ALLOC_SCOPE_AUTO_PUSH_POP()

		u32 cmd = *cursor++;
		switch (cmd & 255) {
			case JUMP: {
				u32 *jmp;
				GetFromBuffer(cursor, jmp);
				cursor = jmp;
				prevCursor = NULL;  // don't want to copy JUMP commands when m_CopyToBatcherWhileRendering is set
				break;
			}
			case CLOSE: {
				return;
			}
			case NEXT: {
				cursor = (u32*)((uptr)cursor+sizeof(void*));
				prevCursor = NULL;  // don't want to copy NEXT commands when m_CopyToBatcherWhileRendering is set
				break;
			}
			case RENDER_CALLBACK: {
				const u32 flags = cmd>>8;
				RenderCallback *callback;
				size_t size;
				GetFromBuffer(cursor, callback);
				GetFromBuffer(cursor, size);
				callback(cursor, size, this);
				cursor = (u32*)(((uptr)cursor+size+3)&~3);
				if ((flags & ARC_NEVER_COPY) != 0) {
					prevCursor = NULL;
				}
				break;
			}
			case DRAW: {
				BatchVtx *v = (BatchVtx*) cursor;
				int dm = (cmd >> 8) & 7;
				int count = (cmd >> 11);
				grcBegin((grcDrawMode)dm,count);
				do {
					grcVertex(v->x,v->y,v->z,v->nx,v->ny,v->nz,v->c,v->s,v->t);
					cursor += sizeof(BatchVtx)/sizeof(u32);
					++v;
				} while (--count);
				grcEnd();
				break;
			}
			case WORLD: {
				Matrix34 m;
				memcpy(&m,cursor,sizeof(m));
				grcWorldMtx(m);
				cursor += sizeof(Matrix34)/sizeof(u32);
				break;
			}
			case WORLDIDENTITY: {
				Matrix34 m(Matrix34::IdentityType);
				grcWorldMtx(m);
				break;
			}
			case LIGHTING: {
				int l;
				memcpy(&l,cursor,sizeof(l));
				grcLightState::SetEnabled(l != 0);
				++cursor;
				break;
			}
			case SAVE_LIGHTING: {
				grcLightState_PreviousState = grcLightState::IsEnabled();
				break;
			}
			case RESTORE_LIGHTING: {
				grcLightState::SetEnabled(grcLightState_PreviousState);
				break;
			}
			case RASTERIZERSTATE: {
				u32 l[1];
				memcpy(&l,cursor,sizeof(l));
				grcStateBlock::SetRasterizerState(grcRasterizerStateHandle(l[0]));
				cursor += 1;
				break;
			}
			case BLENDSTATE: {
				u32 l[1];
				memcpy(&l,cursor,sizeof(l));
				grcStateBlock::SetBlendState(grcBlendStateHandle(l[0]));
				cursor += 1;
				break;
			}
			case DEPTHSTENCILSTATE: {
				u32 l[1];
				memcpy(&l,cursor,sizeof(l));
				grcStateBlock::SetDepthStencilState(grcDepthStencilStateHandle(l[0]));
				cursor += 1;
				break;
			}
			case SAVE_RASTERIZERSTATE: {
				grcRS_Previous = grcStateBlock::RS_Active;
				break;
			}
			case SAVE_BLENDSTATE: {
				grcBS_Previous  = grcStateBlock::BS_Active;
				break;
			}
			case SAVE_DEPTHSTENCILSTATE: {
				grcDSS_Previous = grcStateBlock::DSS_Active;
				break;
			}
			case RESTORE_RASTERIZERSTATE: {
				grcStateBlock::SetRasterizerState(grcRS_Previous);
				break;
			}
			case RESTORE_BLENDSTATE: {
				grcStateBlock::SetBlendState(grcBS_Previous);
				break;
			}
			case RESTORE_DEPTHSTENCILSTATE: {
				grcStateBlock::SetDepthStencilState(grcDSS_Previous);
				break;
			}
			case DRAW_LINE: {
				Vector3 a,b;
				Color32 color;
				GetFromBuffer(cursor, a);
				GetFromBuffer(cursor, b);
				GetFromBuffer(cursor, color);
				grcDrawLine(a,b,color);
				break;
			}
			case DRAW_LINE_2C: {
				Vector3 a,b;
				Color32 colora,colorb;
				GetFromBuffer(cursor, a);
				GetFromBuffer(cursor, b);
				GetFromBuffer(cursor, colora);
				GetFromBuffer(cursor, colorb);
				grcDrawLine(a,b,colora,colorb);
				break;
			}
			case DRAW_QUAD2D: {
				Vector4 p1p2, p3p4;
				Color32 color;
				GetFromBuffer(cursor, p1p2);
				GetFromBuffer(cursor, p3p4);
				GetFromBuffer(cursor, color);

				// TODO: This can be done entirely in the vector pipeline.
				Vector3 p1(p1p2.x, p1p2.y, 0.0f);
				Vector3 p2(p1p2.z, p1p2.w, 0.0f);
				Vector3 p3(p3p4.x, p3p4.y, 0.0f);
				Vector3 p4(p3p4.z, p3p4.w, 0.0f);

				grcDraw2dQuad(p1, p2, p3, p4, color);
				break;
			  }
			case DRAW_STRING2D: {
				GetAndSetColor(cursor);

				float x,y;
				GetFromBuffer(cursor, x);
				GetFromBuffer(cursor, y);
				int drawBgQuad;
				GetFromBuffer(cursor, drawBgQuad);
				float scaleX,scaleY;
				GetFromBuffer(cursor, scaleX);
				GetFromBuffer(cursor, scaleY);
				int paddedLen;
				GetFromBuffer(cursor, paddedLen);
				const grcFont* fontPtr;
				GetFromBuffer(cursor, fontPtr);
				if (tempBufSize < paddedLen)
				{
					tempBuf = Alloca(char, paddedLen);
					tempBufSize = paddedLen;
				}
				for(int i = 0; i < paddedLen; i += 4) {
					GetFromBuffer(cursor, *(u32*)(tempBuf+i));
				}
				grcDraw2dText(x, y, tempBuf,drawBgQuad != 0,scaleX,scaleY,fontPtr);
				break;
			}
			case DRAW_LABEL: {
				GetAndSetColor(cursor);

				Vector3 V;
				int xOffset,yOffset;
				float xScale, yScale;
				GetFromBuffer(cursor, V);
				GetFromBuffer(cursor, xOffset);
				GetFromBuffer(cursor, yOffset);
				GetFromBuffer(cursor, xScale);
				GetFromBuffer(cursor, yScale);
				int drawBgQuad;
				GetFromBuffer(cursor, drawBgQuad);
				int paddedLen;
				GetFromBuffer(cursor, paddedLen);
				if (tempBufSize < paddedLen)
				{
					tempBuf = Alloca(char, paddedLen);
					tempBufSize = paddedLen;
				}
				for(int i = 0; i < paddedLen; i += 4) {
					GetFromBuffer(cursor, *(u32*)(tempBuf+i));
				}
				grcDrawLabel(V, xOffset, yOffset, tempBuf,drawBgQuad != 0, xScale, yScale);
				break;
			}
			case DRAW_FULL_BUFFER_WARNING: {
#if __DEV
#if HACK_GTA4
				grcColor(Color32(255,0,0,255));
				grcDraw2dText(200, 670, "grcBatcher: buffer overflow");
#else // HACK_GTA4
				grcColor(Color32(255,255,255,100));
				grcDraw2dText(200, 100, "grcBatcher: buffer overflow");
#endif // HACK_GTA4
#endif
				break;
			}
			case DRAW_FULL_BUFFER_NO_PIPE_WARNING: {
#if __DEV
				grcColor(Color32(255,255,255,100));
				grcDraw2dText(200, 100, "grcBatcher: buffer overflow, use RAG for full vizualization");
#endif
				break;
			}
			case DRAW_BOX: {
				Vector3 size;
				Matrix34 mtx;
				Color32 color;
				GetFromBuffer(cursor, size);
				GetFromBuffer(cursor, mtx);
				GetFromBuffer(cursor, color);
				grcDrawBox(size, mtx, color);
				break;
			}
			case DRAW_SOLIDBOX: {
				Vector3 size;
				Matrix34 mtx;
				Color32 color;
				GetFromBuffer(cursor, size);
				GetFromBuffer(cursor, mtx);
				GetFromBuffer(cursor, color);
				grcDrawSolidBox(size, mtx, color);
				break;
			}
			case DRAW_FRUSTUM: {
				float sx, sy, sz;
				Matrix34 mtx;
				Color32 color;
				GetFromBuffer(cursor, mtx);
				GetFromBuffer(cursor, sx);
				GetFromBuffer(cursor, sy);
				GetFromBuffer(cursor, sz);
				GetFromBuffer(cursor, color);
				grcDrawFrustum(mtx, sx, sy, sz, color);
				break;
			}
			case DRAW_ELLIPSOID: {
				GetAndSetColor(cursor);

				Vector3 size;
				Matrix34 mtx;
				int steps;
				int longitudinalCircles;
				int solid;
				GetFromBuffer(cursor, size);
				GetFromBuffer(cursor, mtx);
				GetFromBuffer(cursor, steps);
				GetFromBuffer(cursor, longitudinalCircles);
				GetFromBuffer(cursor, solid);
				grcDrawEllipsoid(size, mtx, steps, longitudinalCircles != 0, solid != 0);
				break;
			}
			case DRAW_SPHERE: {
				GetAndSetColor(cursor);

				float rad;
				int steps;
				GetFromBuffer(cursor, rad);
				GetFromBuffer(cursor, steps);
				grcDrawSphere(rad, steps);
				break;
			}
			case DRAW_PARTIAL_SPHERE: {
				GetAndSetColor(cursor);

				float radius;
				Matrix34 mtx;
				float angle;
				int steps;
				int longitudinalCircles;
				int solid;
				GetFromBuffer(cursor, radius);
				GetFromBuffer(cursor, mtx);
				GetFromBuffer(cursor, angle);
				GetFromBuffer(cursor, steps);
				GetFromBuffer(cursor, longitudinalCircles);
				GetFromBuffer(cursor, solid);
				grcDrawPartialSphere(RCC_MAT34V(mtx), radius, angle, steps, longitudinalCircles != 0, solid != 0);
				break;
			}
			case DRAW_SPIRAL: {
				GetAndSetColor(cursor);

				Vector3 start;
				Vector3 end;
				float startRadius;
				float endRadius;
				float rotationsPerMeter;
				float initialPhase;
				float arrowLength;
				int steps;
				GetFromBuffer(cursor, start);
				GetFromBuffer(cursor, end);
				GetFromBuffer(cursor, startRadius);
				GetFromBuffer(cursor, endRadius);
				GetFromBuffer(cursor, rotationsPerMeter);
				GetFromBuffer(cursor, initialPhase);
				GetFromBuffer(cursor, arrowLength);
				GetFromBuffer(cursor, steps);
				grcDrawSpiral(start, end, startRadius, endRadius, rotationsPerMeter, initialPhase, arrowLength, steps);
				break;
			}
			case DRAW_CIRCLE:  {
				GetAndSetColor(cursor);

				float r;
				Vector3 center;
				Vector3 axisX, axisY;
				int steps, dashed, solid;
				GetFromBuffer(cursor, r);
				GetFromBuffer(cursor, center);
				GetFromBuffer(cursor, axisX);
				GetFromBuffer(cursor, axisY);
				GetFromBuffer(cursor, steps);
				GetFromBuffer(cursor, dashed);
				GetFromBuffer(cursor, solid);
				grcDrawCircle(r, center, axisX, axisY, steps, dashed != 0, solid != 0);
				break;
			}
			case DRAW_ARC:  {
				GetAndSetColor(cursor);

				float r;
				Vector3 center;
				Vector3 axisX, axisY;
				float beginAngle, endAngle;
				int steps, solid;
				GetFromBuffer(cursor, r);
				GetFromBuffer(cursor, center);
				GetFromBuffer(cursor, axisX);
				GetFromBuffer(cursor, axisY);
				GetFromBuffer(cursor, beginAngle);
				GetFromBuffer(cursor, endAngle);
				GetFromBuffer(cursor, steps);
				GetFromBuffer(cursor, solid);
				grcDrawArc(r, center, axisX, axisY, beginAngle, endAngle, steps, solid != 0);
				break;
			}
			case DRAW_CAPSULE: {
				GetAndSetColor(cursor);

				float length;
				float radius;
				float halfHeight;
				Matrix34 mtx;
				int steps;
				int solid;
				GetFromBuffer(cursor, length);
				GetFromBuffer(cursor, radius);
				GetFromBuffer(cursor, mtx);
				GetFromBuffer(cursor, steps);
				GetFromBuffer(cursor, solid);
				GetFromBuffer(cursor, halfHeight);
				grcDrawCapsule(length, radius, mtx, steps, solid != 0, halfHeight);
				break;
			}
			case DRAW_TAPERED_CAPSULE: {
				GetAndSetColor(cursor);

				float length;
				float radiusA;
				float radiusB;
				Matrix34 mtx;
				int steps;
				int solid;
				GetFromBuffer(cursor, length);
				GetFromBuffer(cursor, radiusA);
				GetFromBuffer(cursor, radiusB);
				GetFromBuffer(cursor, mtx);
				GetFromBuffer(cursor, steps);
				GetFromBuffer(cursor, solid);
				grcDrawTaperedCapsule(length, radiusA, radiusB, mtx, steps, solid != 0);
				break;
			}
			case DRAW_CYLINDER: {
				GetAndSetColor(cursor);

				float length;
				float radius;
				Matrix34 mtx;
				int steps;
				int cbDrawTop;
				GetFromBuffer(cursor, length);
				GetFromBuffer(cursor, radius);
				GetFromBuffer(cursor, mtx);
				GetFromBuffer(cursor, steps);
				GetFromBuffer(cursor, cbDrawTop);
				grcDrawCylinder(length, radius, mtx, steps, cbDrawTop != 0);
				break;
			}
			case DRAW_AXIS: {
				float size;
				Matrix34 mtx;
				int drawArrows;
				GetFromBuffer(cursor, size);
				GetFromBuffer(cursor, mtx);
				GetFromBuffer(cursor, drawArrows);
				grcDrawAxis(size, mtx, drawArrows != 0);
				break;
			}
			case DRAW_POLYGON: {
				int count;
				Vector3 *points;
				Color32 color;

				GetFromBuffer(cursor, color);
				GetFromBuffer(cursor, count);
				if (tempBufSize < (int)(count * sizeof(Vector3)))
				{
					tempBuf = Alloca(char, count * sizeof(Vector3) );
					tempBufSize = count * sizeof(Vector3);
				}

				points = (Vector3*)tempBuf;
				for(int i=0;i<count;i++)
				{
					GetFromBuffer(cursor, points[i]);
				}

				grcDrawPolygon(points, count, NULL, true, color);
				break;
			}
			case DRAW_POLYGON_TEX: {
				int count;
				Vector3 *points;
				Color32 color;
				grcTexture *tex;
				Vector2 *uvs;

				GetFromBuffer(cursor, color);
				GetFromBuffer(cursor, tex);
				GetFromBuffer(cursor, count);

				const int requiredBufSize = count * (sizeof(Vector3)+sizeof(Vector2));
				if (tempBufSize < requiredBufSize)
				{
					tempBuf = Alloca(char, requiredBufSize);
					tempBufSize = requiredBufSize;
				}

				points	= (Vector3*)tempBuf;
				uvs		= (Vector2*)(((u8*)points) + count * sizeof(Vector3));

				for(int i=0;i<count;i++)
				{
					GetFromBuffer(cursor, points[i]);
					GetFromBuffer(cursor, uvs[i]);
				}

				grcDrawTexturedPolygon(points, count, NULL, true, color, tex, uvs);
				break;
			}
			case DRAW_POLYGON_COLOR: {
				int count;
				Vector3 *points;
				Color32 *color;

				GetFromBuffer(cursor, count);
				if (tempBufSize < (int)(count * (sizeof(Vector3)+sizeof(Color32))))
				{
					tempBuf = Alloca(char, count * (sizeof(Vector3)+sizeof(Color32)) );
					tempBufSize = count * (sizeof(Vector3)+sizeof(Color32));
				}

				points = (Vector3*)tempBuf;
				color = (Color32*)((unsigned char*)points + count * sizeof(Vector3));
				for(int i=0;i<count;i++)
				{
					GetFromBuffer(cursor, points[i]);
					GetFromBuffer(cursor, color[i]);
				}

				grcDrawPolygon(points, count, NULL, true, color);
				break;
			}
			case DRAW_POLYGON_TEX_COLOR: {
				int count;
				Vector3 *points;
				Color32* colors;
				grcTexture *tex;
				Vector2 *uvs;

				GetFromBuffer(cursor, tex);
				GetFromBuffer(cursor, count);

				const int requiredBufSize = count * (sizeof(Vector3)+sizeof(Color32)+sizeof(Vector2));
				if (tempBufSize < requiredBufSize)
				{
					tempBuf = Alloca(char, requiredBufSize);
					tempBufSize = requiredBufSize;
				}

				points	= (Vector3*)tempBuf;
				colors  = (Color32*)(((u8*)points) + count * sizeof(Vector3));
				uvs		= (Vector2*)(((u8*)colors) + count * sizeof(Color32));

				for(int i=0;i<count;i++)
				{
					GetFromBuffer(cursor, points[i]);
					GetFromBuffer(cursor, colors[i]);
					GetFromBuffer(cursor, uvs[i]);
				}

				grcDrawTexturedPolygon(points, count, NULL, true, colors, tex, uvs);
				break;
			}
			case DRAW_PROXY:
				grcDraw(GetDrawProxyFromBuffer(cursor, (cmd >> 8) & 3));
				break;
			default:
				Quitf("Unknown command (%d & 255 = %d), cursor=%p", cmd, cmd & 255, cursor-1);
		}
	}
}

void grcBatcher::Render(bool flushAfterRender) {
	// Close if not already
	if (!m_Closed)
		Close();

	grcBatcher *old = SetCurrent(NULL);	// Don't recurse!
	RenderInternal();
	SetCurrent(old);
	if (flushAfterRender)
		Flush();
}

void grcBatcher::Flush() {
	if (m_FullMode == BUF_FULL_ALLOC) {
		FreeBufferMemory();
		m_BeginAll = m_BeginSeg = m_Curr = m_End = NULL;
	}
	else {
		m_Curr = m_BeginSeg;
		Assert(m_BeginSeg == m_BeginAll);

#if __DEV
		if (m_FullMode == BUF_FULL_INTERNAL_OSW_IGNORE) {
			m_FullMode = BUF_FULL_ONSCREEN_WARNING;
		}
		else if (m_FullMode == BUF_FULL_INTERNAL_WDOP_IGNORE) {
			m_FullMode = BUF_FULL_WARN_DRAW_OVER_PIPE;
		}
#endif
	}
	m_Closed = false;
	m_CurrReserved = NULL;
}

void grcBatcher::CopyToBatcherWhileRendering(grcBatcher *batcher) {
	m_CopyToBatcherWhileRendering = batcher;
}

void* grcBatcher::Reserve(u32 cmd,size_t payloadSize) {

	size_t wordsAvail = m_End-m_Curr;
#if __DEV
	if (m_FreezeBatcher)
	{
		return NULL;
	}
#endif
	size_t origPayloadSize = payloadSize;
	if (m_FullMode == BUF_FULL_ONSCREEN_WARNING || m_FullMode == BUF_FULL_WARN_DRAW_OVER_PIPE) {
		Assertf(wordsAvail > 4, "Buffer must be at least 16 bytes to use BUF_FULL_ONSCREEN_WARNING. Buffer size remaining is %d", (u32)wordsAvail);
		wordsAvail -= 4; // reserve 4 words for the error message
	}
	if (m_FullMode == BUF_FULL_INTERNAL_OSW_IGNORE || m_FullMode == BUF_FULL_INTERNAL_WDOP_IGNORE) {
		m_CurrReserved = NULL;
		return NULL;
	}
	Assert((payloadSize&3)==0);
	payloadSize >>= 2;
	if (wordsAvail > payloadSize) {
		m_Curr[0] = cmd;
		m_CurrReserved = m_Curr + 1;
		m_Curr += payloadSize + 1;
		return m_CurrReserved;
	}
	else
	{
		switch(m_FullMode) {
			case BUF_FULL_ALLOC:
				{
					void *alloc;
					size_t sizeBytes;
					if (m_AllocCallback(&alloc, &sizeBytes)) {
						LinkNewBuffer(alloc, sizeBytes);
						m_Curr[0] = cmd;
						m_CurrReserved = m_Curr + 1;
						m_Curr += payloadSize + 1;
						return m_CurrReserved;
					}
				}
				break;
			case BUF_FULL_ASSERT:
				Quitf("grcBatcher: Buffer overflow. Tried to reserve %d bytes and %u were available. Batcher : %u bytes used over %d total. ", (u32)payloadSize*4, (u32)(wordsAvail*4), (u32)((m_Curr-m_BeginSeg)*4), (u32)((m_End-m_BeginSeg)*4));
				return NULL;
			case BUF_FULL_DRAW:
				{
					if (grcViewport::GetCurrent())
					{
						Render(true); // Draw what's currently in the buffer, and empty it.
					}
					else {
						Flush();
					}
					m_FullMode = BUF_FULL_ASSERT;
					void* buf = Reserve(cmd, origPayloadSize); // try the reserve again, assert if it fails this time.
					m_FullMode = BUF_FULL_DRAW;
					return buf;
				}
				/*NOTREACHED*/
				break;
			case BUF_FULL_DRAW_OVER_PIPE:
				{
#if 0 // DISABLED FOR NOW
					if (grcGetImNamedPipe() != NULL) {
						if (grcViewport::GetCurrent())
						{
							Render(true); // Draw what's currently in the buffer, and empty it.
						}
						else {
							Flush();
						}
						m_FullMode = BUF_FULL_ASSERT;
						void* buf = Reserve(cmd, origPayloadSize); // try the reserve again, assert if it fails this time.
						m_FullMode = BUF_FULL_DRAW;
						return buf;
					}
#endif
				}
				break;
			case BUF_FULL_WARN_DRAW_OVER_PIPE:
				{
#if 0 // DISABLED FOR NOW
					if (grcGetImNamedPipe() != NULL) {
						if (grcViewport::GetCurrent()) {
							Render(true);
						}
						else {
							Flush();
						}
						m_FullMode = BUF_FULL_ASSERT;
						void* buf = Reserve(cmd, origPayloadSize); // try the reserve again, assert if it fails this time.
						m_FullMode = BUF_FULL_DRAW;
						return buf;
					}
					else 
#endif
					{
						*m_Curr++ = DRAW_FULL_BUFFER_NO_PIPE_WARNING;
						m_FullMode = BUF_FULL_INTERNAL_WDOP_IGNORE;
						return NULL;
					}
				}
			case BUF_FULL_ONSCREEN_WARNING:
				*m_Curr++ = DRAW_FULL_BUFFER_WARNING;
				m_FullMode = BUF_FULL_INTERNAL_OSW_IGNORE;
				break;
			case BUF_FULL_IGNORE:
			case BUF_FULL_INTERNAL_OSW_IGNORE:
			case BUF_FULL_INTERNAL_WDOP_IGNORE:
			default:
				;
		}

		m_CurrReserved = NULL;
		return NULL;
	}
}

void *grcBatcher::AddRenderCallback(RenderCallback *callback, size_t size, u32 flags) {
	const size_t totalSize = sizeof(void*) + sizeof(size_t) + ((size+3)&~3);
	char *data = (char*)Reserve(RENDER_CALLBACK, totalSize);
	if (data) {
		Assert(((u32*)data)[-1] == RENDER_CALLBACK);
		((u32*)data)[-1] |= flags<<8;
		*(RenderCallback**)data = callback;
		data += sizeof(void*);
		*(size_t*)data = size;
		data += sizeof(size_t);
	}
	return data;
}


#endif

void grcWorldIdentity() {
#if __IM_BATCHER
	if (grcBatcher::GetCurrent())
		grcBatcher::GetCurrent()->SetWorldIdentity();
	else
#endif
		grcViewport::SetCurrentWorldIdentity();
}

Mat34V grcWorldMtx() {
#if __IM_BATCHER
	if (grcBatcher::GetCurrent())
		return grcBatcher::GetCurrent()->GetWorld();
#endif
	return grcViewport::GetCurrentWorldMtx();
}

Mat34V grcWorldMtx(Mat34V_In mtx) {
#if __IM_BATCHER
	if (grcBatcher::GetCurrent())
		return grcBatcher::GetCurrent()->SetWorld(mtx);
#endif
	Mat34V prev = grcViewport::GetCurrentWorldMtx();
	grcViewport::SetCurrentWorldMtx(mtx);
	return prev;
}

Matrix34 grcWorldMtx(const Matrix34 &mtx) {
	return MAT34V_TO_MATRIX34(grcWorldMtx(RCC_MAT34V(mtx)));
}

bool grcLighting(bool enable) {
	bool previous = grcLightState::IsEnabled();
#if __IM_BATCHER
	if (grcBatcher::GetCurrent())
		grcBatcher::GetCurrent()->SetLighting(enable);
	else
#endif
		grcLightState::SetEnabled(enable);
	return previous;
}

static __THREAD const grcTexture *s_CurrentTex;
extern grcEffect *g_DefaultEffect;
extern grcEffectVar g_DefaultSampler;

void grcBindTexture(const grcTexture *tex) {
	if (!tex)
		tex = grcTexture::None;
#if __IM_BATCHER
	if (grcBatcher::GetCurrent())
	{
		// For now, grcBatcher doesn't support textures, but it does let you call
		// grcBindTexture(grcTexture::None), for convenience. /FF
		Assert(tex == grcTexture::None);
		return;
	}
#endif
	g_DefaultEffect->SetVar(g_DefaultSampler, s_CurrentTex = tex);
}

const grcTexture* grcGetTexture() {
	if (s_CurrentTex == grcTexture::None)
		return NULL;
	else
		return s_CurrentTex;
}


grcTexture* grcCreateTexture(const char *filename) {
	return grcTextureFactory::GetInstance().Create(filename);
}


grcTexture* grcCreateTexture(const char *folder,const char *filename) {
	ASSET.PushFolder(folder);
	grcTexture *texture = grcTextureFactory::GetInstance().Create(filename);
	ASSET.PopFolder();
	return texture;
}


void grcReleaseTexture(grcTexture *tex) {
	if (tex)
		tex->Release();
}


void grcFixupTexture(datResource &rsc,grcTexture *&texture) {
	if (texture) {
		rsc.PointerFixup(texture);
		grcTextureFactory::GetInstance().PlaceTexture(rsc,*texture);
	}
}


#if __IM_BATCHER
void grcbReserve(u32 cmd, size_t size) {
	grcBatcher::GetCurrent()->Reserve(cmd, size);
}

template <typename _T> void grcbPush(const _T& data)
{
	grcBatcher::GetCurrent()->PushData(data);
}

void grcbPushColor()
{
	grcBatcher::GetCurrent()->PushData(grcCurrentColor);
}
#endif

void grcDrawLine(const Vector3 &ptA,const Vector3 &ptB,const Color32 color) {
#if __IM_BATCHER
	if (grcBatcher::GetCurrent())
	{
		grcbReserve(DRAW_LINE, sizeof(Vector3) + sizeof(Vector3) + sizeof(Color32));
		grcbPush(ptA);
		grcbPush(ptB);
		grcbPush(color);
		return;
	}
#endif
	grcBegin(drawLines,2);
	grcColor(color);
	grcVertex3f(ptA);
	grcVertex3f(ptB);
	grcEnd();
}

void grcDrawLine(Vec3V_In v0,Vec3V_In v1,const Color32 color)
{
	grcDrawLine(RCC_VECTOR3(v0),RCC_VECTOR3(v1),color);
}

void grcDrawArrowOrtho( const Vector3& vStartPos, const Vector3& vEndPos, const Color32& Color, const Vector3& vCameraPosition, float fArrowheadLength, float fArrowheadBaseHalfLength )
{
	// get camera position
	if( vStartPos != vEndPos )
	{
		// draw arrowhead orthogonal to the camera
		Vector3 vLineNormal = (vEndPos - vStartPos);
		if( !vLineNormal.NormalizeSafeRet() )
			return;
			
		Vector3 vAdjustedEnd = vEndPos;
		vAdjustedEnd -= fArrowheadLength*vLineNormal;

		Vector3 vBaseLocation = vAdjustedEnd;
		Vector3 vHeadLocation = vBaseLocation + vLineNormal*fArrowheadLength;

		// determine normal for base of arrowhead which is orthogonal to the view direction
		Vector3 vBaseVect = vLineNormal;
		vBaseVect.Cross( vCameraPosition - vBaseLocation );
		if( !vBaseVect.NormalizeSafeRet() )
			return;
	
		Vector3 vBaseEnd1 = vBaseLocation + vBaseVect*fArrowheadBaseHalfLength;
		Vector3 vBaseEnd2 = vBaseLocation - vBaseVect*fArrowheadBaseHalfLength;

		grcDrawLine( vStartPos, vBaseLocation, Color );	// arrow body
		grcDrawLine( vBaseEnd1, vBaseEnd2, Color );		// arrowhead base
		grcDrawLine( vBaseEnd1, vHeadLocation, Color );	// arrowhead base end #1 to point
		grcDrawLine( vBaseEnd2, vHeadLocation, Color );	// arrowhead base end #2 to point
	}
}

void grcDrawLine(const Vector3 &ptA,const Vector3 &ptB,Color32 colorA,Color32 colorB) {
#if __IM_BATCHER
	if (grcBatcher::GetCurrent())
	{
		grcbReserve(DRAW_LINE_2C, sizeof(Vector3) + sizeof(Vector3) + sizeof(Color32) + sizeof(Color32));
		grcbPush(ptA);
		grcbPush(ptB);
		grcbPush(colorA);
		grcbPush(colorB);
		return;
	}
#endif
	grcBegin(drawLines,2);
	grcColor(colorA);
	grcVertex3f(ptA);
	grcColor(colorB);
	grcVertex3f(ptB);
	grcEnd();
}

void grcDraw2dQuad(const Vector3& p1, const Vector3& p2, const Vector3& p3, const Vector3& p4, Color32 color)
{
#if __IM_BATCHER
	if (grcBatcher::GetCurrent())
	{
		grcbReserve(DRAW_QUAD2D, sizeof(Vector4) + sizeof(Vector4) + sizeof(Color32));

		// TODO: This could be done entirely in the vector pipeline
		Vector4 p1p2(p1.x, p1.y, p2.x, p2.y);
		Vector4 p3p4(p3.x, p3.y, p4.x, p4.y);

		grcbPush(p1p2);
		grcbPush(p3p4);
		grcbPush(color);
		return;
	}
#endif

	grcBegin(drawLineStrip,5);
	grcColor(color);
	grcVertex3f(p1);
	grcVertex3f(p2);
	grcVertex3f(p3);
	grcVertex3f(p4);
	grcVertex3f(p1);
	grcEnd();
}

void grcDrawBox(const Vector3 &size,const Matrix34 &mtx, Color32 color) {
#if __IM_BATCHER
	if (grcBatcher::GetCurrent())
	{
		grcbReserve(DRAW_BOX, sizeof(Vector3) + sizeof(Matrix34) + sizeof(Color32));
		grcbPush(size);
		grcbPush(mtx);
		grcbPush(color);
		return;
	}
#endif
	Vector3 pos;
	pos.Scale(size,0.5f);

	Vector3 a(-pos.x,-pos.y,-pos.z);
	Vector3 b(+pos.x,-pos.y,-pos.z);
	Vector3 c(+pos.x,+pos.y,-pos.z);
	Vector3 d(-pos.x,+pos.y,-pos.z);
	Vector3 e(-pos.x,-pos.y,+pos.z);
	Vector3 f(+pos.x,-pos.y,+pos.z);
	Vector3 g(+pos.x,+pos.y,+pos.z);
	Vector3 h(-pos.x,+pos.y,+pos.z);

	grcWorldMtx(mtx);
	grcBegin(drawLines,24);
	grcColor(color);

	grcVertex3f(a);	grcVertex3f(b);
	grcVertex3f(b);	grcVertex3f(c);
	grcVertex3f(c);	grcVertex3f(d);
	grcVertex3f(d);	grcVertex3f(a);

	grcVertex3f(a);	grcVertex3f(e);
	grcVertex3f(b);	grcVertex3f(f);
	grcVertex3f(c);	grcVertex3f(g);
	grcVertex3f(d);	grcVertex3f(h);

	grcVertex3f(e);	grcVertex3f(f);
	grcVertex3f(f);	grcVertex3f(g);
	grcVertex3f(g);	grcVertex3f(h);
	grcVertex3f(h);	grcVertex3f(e);

	grcEnd();
}

void grcDrawBox(const Vector3& min,const Vector3& max, Color32 color)
{
	Vector3 size = max - min;
	Matrix34 mtx;
	mtx.Identity();
	mtx.d = ( max + min ) * 0.5f;
	grcDrawBox( size, mtx , color );
}



void grcDrawSolidBox(const Vector3 &size,const Matrix34 &mtx,Color32 color) {
#if __IM_BATCHER
	if (grcBatcher::GetCurrent())
	{
		grcbReserve(DRAW_SOLIDBOX, sizeof(Vector3) + sizeof(Matrix34) + sizeof(Color32));
		grcbPush(size);
		grcbPush(mtx);
		grcbPush(color);
		return;
	}
#endif
	Vector3 pos;
	pos.Scale(size,0.5f);

	Vector3 a(-pos.x,-pos.y,-pos.z);
	Vector3 b(+pos.x,-pos.y,-pos.z);
	Vector3 c(+pos.x,+pos.y,-pos.z);
	Vector3 d(-pos.x,+pos.y,-pos.z);
	Vector3 e(-pos.x,-pos.y,+pos.z);
	Vector3 f(+pos.x,-pos.y,+pos.z);
	Vector3 g(+pos.x,+pos.y,+pos.z);
	Vector3 h(-pos.x,+pos.y,+pos.z);

	grcWorldMtx(mtx);

	grcColor(color);

	grcBegin(drawTris,12 * 3);
	grcNormal3f( 0.0f,  0.0f, -1.0f);
	grcVertex3f(d);
	grcVertex3f(c);
	grcVertex3f(b);

	grcVertex3f(d);
	grcVertex3f(b);
	grcVertex3f(a);

	grcNormal3f( 0.0f,  0.0f,  1.0f);
	grcVertex3f(e);
	grcVertex3f(f);
	grcVertex3f(g);

	grcVertex3f(e);
	grcVertex3f(g);
	grcVertex3f(h);

	grcNormal3f( 1.0f,  0.0f, 0.0f);
	grcVertex3f(b);
	grcVertex3f(c);
	grcVertex3f(g);

	grcVertex3f(b);
	grcVertex3f(g);
	grcVertex3f(f);

	grcNormal3f( 0.0f,  1.0f, 0.0f);
	grcVertex3f(c);
	grcVertex3f(d);
	grcVertex3f(h);

	grcVertex3f(c);
	grcVertex3f(h);
	grcVertex3f(g);

	grcNormal3f(-1.0f,  0.0f, 0.0f);
	grcVertex3f(a);
	grcVertex3f(e);
	grcVertex3f(h);

	grcVertex3f(a);
	grcVertex3f(h);
	grcVertex3f(d);

	grcNormal3f( 0.0f, -1.0f, 0.0f);
	grcVertex3f(a);
	grcVertex3f(b);
	grcVertex3f(f);

	grcVertex3f(a);
	grcVertex3f(f);
	grcVertex3f(e);
	grcEnd();
}

void grcDrawSolidBox(const Vector3& min,const Vector3& max, Color32 color)
{
	Vector3 size = max - min;
	Matrix34 mtx;
	mtx.Identity();
	mtx.d = ( max + min ) * 0.5f;
	grcDrawSolidBox( size, mtx , color );
}
void grcDrawFrustum(const Matrix34& m, float sx, float sy, float sz, Color32 color)
{
#if __IM_BATCHER
	if (grcBatcher::GetCurrent())
	{
		grcbReserve(DRAW_FRUSTUM, sizeof(Matrix34) + 3*sizeof(float) + sizeof(Color32));
		grcbPush(m);
		grcbPush(sx);
		grcbPush(sy);
		grcbPush(sz);
		grcbPush(color);
		return;
	}
#endif
	const float a=0.5f*sx, b=0.5f*sy, c=sz;
	Vector3 base[4];
	Vector3 apex(0.0f, 0.0f, 0.0f);

	base[0].Set(a, b, -c);
	base[1].Set(a, -b, -c);
	base[2].Set(-a, b, -c);
	base[3].Set(-a, -b, -c);

	// draw base
	grcWorldMtx(m);
	grcColor(color);
	grcBegin(drawLineStrip,5);
	grcVertex3f(base[0]);
	grcVertex3f(base[1]);
	grcVertex3f(base[3]);
	grcVertex3f(base[2]);
	grcVertex3f(base[0]);
	grcEnd();

	// draw to apex
	grcBegin(drawLines, 8);
	for(int i=0; i<4; i++)
	{
		grcVertex3f(apex);
		grcVertex3f(base[i]);
	}
	grcEnd();
}


void grcDrawSphere(float rad,const Vector3 &center,int steps,bool longitudinalCircles,bool solid) {
	Matrix34 mtx(M34_IDENTITY);
	mtx.d.Set(center);
	grcDrawSphere(rad,mtx,steps,longitudinalCircles,solid);
}

void grcDrawSphere(float rad,Vec3V_In center,int steps,bool longitudinalCircles,bool solid) {
	grcDrawSphere(rad,RCC_VECTOR3(center),steps,longitudinalCircles,solid);
}

void grcDrawSphere(float rad,const Matrix34 &mtx,int steps,bool longitudinalCircles,bool solid) {
	grcDrawEllipsoid(Vector3(rad,rad,rad),mtx,steps,longitudinalCircles,solid);
}

void grcDrawSphere(float rad,Mat34V_In mtx,int steps,bool longitudinalCircles,bool solid) {
	grcDrawEllipsoid(Vector3(rad,rad,rad),RCC_MATRIX34(mtx),steps,longitudinalCircles,solid);
}

void grcDrawEllipsoid(Vec3V_In size,Mat34V_In mtx,int steps,bool longitudinalCircles,bool solid) {
	grcDrawEllipsoid(RCC_VECTOR3(size),RCC_MATRIX34(mtx),steps,longitudinalCircles,solid);
}

void grcDrawEllipsoid(const Vector3 &size,const Matrix34 &mtx,int steps,bool longitudinalCircles,bool solid) {
#if __IM_BATCHER
	if (grcBatcher::GetCurrent())
	{
		grcbReserve(DRAW_ELLIPSOID, sizeof(Color32) + sizeof(Vector3) + sizeof(Matrix34) + sizeof(int) + 2*sizeof(int));
		grcbPushColor();
		grcbPush(size);
		grcbPush(mtx);
		grcbPush(steps);
		grcbPush((int)longitudinalCircles);
		grcbPush((int)solid);
		return;
	}
#endif
	grcWorldMtx(mtx);
	if (solid)
	{
		for(int i=(steps>>1);i>0;--i) {
			float y=size.y*cosf(PI*float(i)/float(steps>>1));
			float nexty=size.y*cosf(PI*float(i-1)/float(steps>>1));
			float s=sinf(PI*float(i)/float(steps>>1));
			float nexts=sinf(PI*float(i-1)/float(steps>>1));
			grcBegin(drawTriStrip,(steps+1) * 2);
			for(int j=0;j<=steps;j++) {
				float x=size.x*s*cosf(2.0f*PI*float(j)/float(steps));
				float nextx=size.x*nexts*cosf(2.0f*PI*float(j+1)/float(steps));
				float z=size.z*s*sinf(2.0f*PI*float(j)/float(steps));
				float nextz=size.z*nexts*sinf(2.0f*PI*float(j+1)/float(steps));
				Vector3 normal(x, y, z);
				normal.Normalize();
				grcNormal3f(normal);
				grcVertex3f(x,y,z);
                Vector3 nextNormal(nextx, nexty, nextz);
                nextNormal.Normalize();
                grcNormal3f(nextNormal);
                grcVertex3f(nextx,nexty,nextz);
			}
			grcEnd();
		}
	}
	else
	{
		int i, j;
		float x,y,z,s;
		if(steps<4) steps=4;
		grcWorldMtx(mtx);
		for(i=1;i<(steps>>1);i++) {
			y=size.y*cosf(PI*float(i)/float(steps>>1));
			s=sinf(PI*float(i)/float(steps>>1));
			grcBegin(drawLineStrip,steps+1);
			for(j=0;j<=steps;j++) {
				x=size.x*s*cosf(2.0f*PI*float(j)/float(steps));
				z=size.z*s*sinf(2.0f*PI*float(j)/float(steps));
				grcVertex3f(x,y,z);
			}
			grcEnd();
		}
		if(longitudinalCircles) {
			for(i=0;i<(steps>>1);i++) {
				x=size.x*cosf(PI*float(i)/float(steps>>1));
				z=size.z*sinf(PI*float(i)/float(steps>>1));
				grcBegin(drawLineStrip,steps+1);
				for(j=0;j<=steps;j++) {
					s=sinf(2.0f*PI*float(j)/float(steps));
					y=size.y*cosf(2.0f*PI*float(j)/float(steps));
					grcVertex3f(x*s,y,z*s);
				}
				grcEnd();
			}
		}
	}
}

void grcDrawPartialSphere(Vec3V_In center, float radius, Vec3V_In direction, float angle, int steps, bool longitudinalCircles, bool solid) {
	// Create a full matrix out of this direction.
	Mat34V mtx;

	// For this, we need an arbitrary up vector.
	Vec3V upVector(V_Z_AXIS_WZERO);

	if (fabs(Dot(upVector, direction).Getf()) > 0.95f)
	{
		// The direction is too close to the up vector we chose.
		// Let's pick a different one.
		upVector = Vec3V(V_Y_AXIS_WZERO);
	}

	LookAt(mtx, center, center + direction, upVector);

	grcDrawPartialSphere(mtx, radius, angle, steps, longitudinalCircles, solid);
}

void grcDrawPartialSphere(Mat34V_In mtx, float radius, float angle, int steps, bool longitudinalCircles, bool solid) {
#if __IM_BATCHER
	if (grcBatcher::GetCurrent())
	{
		grcbReserve(DRAW_PARTIAL_SPHERE, sizeof(Color32) + sizeof(Mat34V) + 2*sizeof(float) + 3*sizeof(int));
		grcbPushColor();
		grcbPush(radius);
		grcbPush(mtx);
		grcbPush(angle);
		grcbPush(steps);
		grcbPush((int)longitudinalCircles);
		grcbPush((int)solid);
		return;
	}
#endif

	// Just do a regular sphere if it's just that.
	if (angle >= PI)
	{
		grcDrawSphere(radius, mtx, steps, longitudinalCircles, solid);
		return;
	}

	grcWorldMtx(mtx);
	if (solid)
	{
		// The outer shell.
		for(int i=(steps>>1);i>0;--i) {
			float z=radius*cosf(angle*float(i)/float(steps>>1));
			float nextz=radius*cosf(angle*float(i-1)/float(steps>>1));
			float s=sinf(angle*float(i)/float(steps>>1));
			float nexts=sinf(angle*float(i-1)/float(steps>>1));
			grcBegin(drawTriStrip,(steps+1) * 2);
			for(int j=0;j<=steps;j++) {
				float x=radius*s*cosf(2.0f*PI*float(j)/float(steps));
				float nextx=radius*nexts*cosf(2.0f*PI*float(j+1)/float(steps));
				float y=radius*s*sinf(2.0f*PI*float(j)/float(steps));
				float nexty=radius*nexts*sinf(2.0f*PI*float(j+1)/float(steps));
				Vector3 normal(x, y, z);
				normal.Normalize();
				grcNormal3f(normal);
				grcVertex3f(x,y,z);
				Vector3 nextNormal(nextx, nexty, nextz);
				nextNormal.Normalize();
				grcNormal3f(nextNormal);
				grcVertex3f(nextx,nexty,nextz);
			}
			grcEnd();
		}

		// The inner part.
		for(int j=0;j<=steps;j++) {
			float z=radius*cosf(angle);
			float nextz=radius*cosf(angle);
			float s=sinf(angle);
			float nexts=sinf(angle);
			grcBegin(drawTris,3);
			float x=radius*s*cosf(2.0f*PI*float(j)/float(steps));
			float nextx=radius*nexts*cosf(2.0f*PI*float(j+1)/float(steps));
			float y=radius*s*sinf(2.0f*PI*float(j)/float(steps));
			float nexty=radius*nexts*sinf(2.0f*PI*float(j+1)/float(steps));
			grcNormal3f(0.0f, -1.0f, 0.0f);
			grcVertex3f(0.0f, 0.0f, 0.0f);
			Vector3 normal(x, y, z);
			normal.Normalize();
			grcNormal3f(normal);
			grcVertex3f(x,y,z);
			Vector3 nextNormal(nextx, nexty, nextz);
			nextNormal.Normalize();
			grcNormal3f(nextNormal);
			grcVertex3f(nextx,nexty,nextz);
			grcEnd();
		}
	}
	else
	{
		int i, j;
		float x,y,z,s;
		if(steps<4) steps=4;
		grcWorldMtx(mtx);
		for(i=1;i<=(steps>>1);i++) {
			z=radius*cosf(angle*float(i)/float(steps>>1));
			s=sinf(angle*float(i)/float(steps>>1));
			grcBegin(drawLineStrip,steps+1);
			for(j=0;j<=steps;j++) {
				x=radius*s*cosf(2.0f*PI*float(j)/float(steps));
				y=radius*s*sinf(2.0f*PI*float(j)/float(steps));
				grcVertex3f(x,y,z);
			}
			grcEnd();
		}
		if(longitudinalCircles) {
			for(i=0;i<(steps>>1);i++) {
				x=radius*cosf(2.0f*PI*float(i)/float(steps>>1));
				y=radius*sinf(2.0f*PI*float(i)/float(steps>>1));
				grcBegin(drawLineStrip,steps+1);
				for(j=0;j<=steps;j++) {
					z=radius*cosf(angle*float(j)/float(steps));
					s=sinf(angle*float(j)/float(steps));
					grcVertex3f(x*s,y*s,z);
				}
				grcEnd();
			}

			z=radius*cosf(angle);
			float s1=sinf(angle);
			for(i=1;i<=(steps>>1);i++) {
				s=float(i)/float(steps>>1);
				grcBegin(drawLineStrip,steps+1);
				for(j=0;j<=steps;j++) {
					x=radius*s*s1*cosf(2.0f*PI*float(j)/float(steps));
					y=radius*s*s1*sinf(2.0f*PI*float(j)/float(steps));
					grcVertex3f(x,y,z*s);
				}
				grcEnd();
			}

			z = cosf(angle) * radius;
			for(i=0;i<(steps>>1);i++) {
				x=radius*cosf(2.0f*PI*float(i)/float(steps>>1));
				y=radius*sinf(2.0f*PI*float(i)/float(steps>>1));
				grcBegin(drawLineStrip,2);
				grcVertex3f(0.0f, 0.0f, 0.0f);
				s=sinf(angle);
				grcVertex3f(x*s,y*s,z);
				grcEnd();
			}
		}
	}
}

void grcDrawEllipse(const float sizeX, const float sizeZ, const Matrix34 &mtx,int steps)
{
	float x,y,z;
	grcWorldMtx(mtx);
	y = 0.0f;
	grcBegin(drawLineStrip,steps+1);
	for(int j=0;j<=steps;j++) {
		x=sizeX*cosf(2.0f*PI*float(j)/float(steps));
		z=sizeZ*sinf(2.0f*PI*float(j)/float(steps));
		grcVertex3f(x,y,z);
	}
	grcEnd();
}


void grcDrawSphere(float rad,int steps) {
#if __IM_BATCHER
	if (grcBatcher::GetCurrent())
	{
		grcbReserve(DRAW_SPHERE, sizeof(Color32) + sizeof(float) + sizeof(int));
		grcbPushColor();
		grcbPush(rad);
		grcbPush(steps);
		return;
	}
#endif
	int j;
	float x, y, z, s;

	s=rad;

	x=0;
	grcBegin(drawLineStrip,steps+1);
	for(j=0;j<=steps;j++) {
		y=s*cosf(2.0f*PI*float(j)/float(steps));
		z=s*sinf(2.0f*PI*float(j)/float(steps));
		grcVertex3f(x,y,z);
	}
	grcEnd();

	y=0;
	grcBegin(drawLineStrip,steps+1);
	for(j=0;j<=steps;j++) {
		x=s*cosf(2.0f*PI*float(j)/float(steps));
		z=s*sinf(2.0f*PI*float(j)/float(steps));
		grcVertex3f(x,y,z);
	}
	grcEnd();

	z=0;
	grcBegin(drawLineStrip,steps+1);
	for(j=0;j<=steps;j++) {
		x=s*cosf(2.0f*PI*float(j)/float(steps));
		y=s*sinf(2.0f*PI*float(j)/float(steps));
		grcVertex3f(x,y,z);
	}
	grcEnd();
}


void grcDrawSpiral(const Vector3& start, const Vector3& end, float startRadius, float endRadius, float revolutionsPerMeter, float initialPhase, float arrowLength, int steps)
{
#if __IM_BATCHER
	if (grcBatcher::GetCurrent())
	{
		grcbReserve(DRAW_SPIRAL, sizeof(Color32) + 2 * sizeof(Vector3) + 5 * sizeof(float) + 1 * sizeof(int));
		grcbPushColor();
		grcbPush(start);
		grcbPush(end);
		grcbPush(startRadius);
		grcbPush(endRadius);
		grcbPush(revolutionsPerMeter);
		grcbPush(initialPhase);
		grcbPush(arrowLength);
		grcbPush(steps);

		return;
	}
#endif
	// A vector down the axis of the spiral
	Vector3 delta(end);
	delta.Subtract(start);

	// One of two basis vectors that form a matrix when combined with delta
	Vector3 basis2;
	basis2.Cross(delta, YAXIS);

	float length = delta.Mag();
	float epsilon = length * length * 0.01f;

	if (basis2.Mag2() < epsilon)
	{
		basis2.Cross(delta, ZAXIS);
		Assert(basis2.Mag2() >= epsilon);
	}

	basis2.Normalize();

	// The other basis vector
	Vector3 basis3;
	basis3.Cross(delta, basis2);
	basis3.Normalize();

	// A non-orthonormal matrix to speed computation in the loop
	Matrix34 mtx;
	mtx.Set(basis2, delta, basis3, start);

	float revolutions = length * revolutionsPerMeter;
	int totalSteps = (int)(revolutions * steps);
	Vector3 previousVertex(start);
	Vector3 vertex(start);

	totalSteps = Min(totalSteps, grcBeginMax - 1);

	if (totalSteps > 0)
	{
		grcBegin(drawLineStrip,totalSteps+1);
		for(int j=0;j<=totalSteps;j++) {
			previousVertex = vertex;

			float totalPhase = float(j) / float(totalSteps);
			float revolutionPhase = float(j)/float(steps) + initialPhase;
			float radius = startRadius + totalPhase * (endRadius - startRadius);

			vertex.x=radius*cosf(-2.0f*PI*revolutionPhase);
			vertex.y=totalPhase;
			vertex.z=radius*sinf(-2.0f*PI*revolutionPhase);
			mtx.Transform(vertex);

			grcVertex3fv(&vertex[0]);
		}
		grcEnd();

		arrowLength = fabs(arrowLength);
		if (arrowLength > 0.0f)
		{
			Vector3 direction(vertex);
			direction.Subtract(previousVertex);
			float dirLen2 = direction.Mag2();

			if (dirLen2 > SMALL_FLOAT * SMALL_FLOAT)
			{
				direction.Scale(1.0f / sqrt(dirLen2));

				Vector3 side;
				side.Cross(direction, YAXIS);

				float sideLen2 = side.Mag2();
				if (sideLen2 < SMALL_FLOAT * SMALL_FLOAT)
				{
					side.Cross(direction,XAXIS);
				}

				side.Normalize();

				Vector3 up;
				up.Cross(direction, side);
				up.Normalize();

				Vector3 vertex2;

				grcBegin(drawLines,8);

				vertex2.Scale(direction, -arrowLength);
				vertex2.AddScaled(up, arrowLength);
				vertex2.Add(vertex);
				grcVertex3fv(&vertex[0]);
				grcVertex3fv(&vertex2[0]);

				vertex2.Scale(direction, -arrowLength);
				vertex2.AddScaled(up, -arrowLength);
				vertex2.Add(vertex);
				grcVertex3fv(&vertex[0]);
				grcVertex3fv(&vertex2[0]);

				vertex2.Scale(direction, -arrowLength);
				vertex2.AddScaled(side, arrowLength);
				vertex2.Add(vertex);
				grcVertex3fv(&vertex[0]);
				grcVertex3fv(&vertex2[0]);

				vertex2.Scale(direction, -arrowLength);
				vertex2.AddScaled(side, -arrowLength);
				vertex2.Add(vertex);
				grcVertex3fv(&vertex[0]);
				grcVertex3fv(&vertex2[0]);

				grcEnd();
			}
		}
	}
}

void grcDrawFlatCircle(float r, const Vector3 &center, int steps, bool dashed)
{
	grcDrawCircle(r,center,XAXIS,ZAXIS,steps,dashed);
}


void grcDrawCircle(float r, const Vector3 &center, const Vector3 &axisX, const Vector3 &axisY, int steps, bool dashed, bool solid)
{
#if __IM_BATCHER
	if (grcBatcher::GetCurrent())
	{
		grcbReserve(DRAW_CIRCLE, sizeof(Color32) + sizeof(float) + 3*sizeof(Vector3) + 3*sizeof(int));
		grcbPushColor();
		grcbPush(r);
		grcbPush(center);
		grcbPush(axisX);
		grcbPush(axisY);
		grcbPush(steps);
		grcbPush((int)dashed);
		grcbPush((int)solid);
		return;
	}
#endif
	int j;
	float x, y;

	int circleVerts = steps;
	int grcVerts;

	if(dashed)
	{
		steps -= (steps % 2) ? 1 : 0;
		grcBegin(drawLines,steps);
		grcVerts = steps;
	}
	else if (solid)
	{
#if __WIN32PC
		grcBegin(drawTris,steps * 3);
		grcVerts = steps+1;
#else
		grcBegin(drawTriFan,steps+2);
		grcVerts = steps+1;
		grcVertex3f(center);
#endif
	}
	else
	{
		grcBegin(drawLineStrip,steps+1);
		grcVerts = steps+1;
	}

#if __WIN32PC
	if (solid)
	{
		for(j=0;j < (grcVerts - 1);j++)
		{
			grcVertex3f(center);

			x=r*cosf(2.0f*PI*float(j)/float(circleVerts));
			y=r*sinf(2.0f*PI*float(j)/float(circleVerts));
			Vector3 pos;
			pos.SetScaled(axisX,x);
			pos.AddScaled(axisY,y);
			pos.Add(center);
			grcVertex3f(pos);

			x=r*cosf(2.0f*PI*float(j + 1)/float(circleVerts));
			y=r*sinf(2.0f*PI*float(j + 1)/float(circleVerts));
			pos.SetScaled(axisX,x);
			pos.AddScaled(axisY,y);
			pos.Add(center);
			grcVertex3f(pos);
		}
	}
	else
#endif
	{
		for(j=0;j<grcVerts;j++)
		{
			x=r*cosf(2.0f*PI*float(j)/float(circleVerts));
			y=r*sinf(2.0f*PI*float(j)/float(circleVerts));
			Vector3 pos;
			pos.SetScaled(axisX,x);
			pos.AddScaled(axisY,y);
			pos.Add(center);
			grcVertex3f(pos);
		}
	}
	grcEnd();
}


void grcDrawArc(float r, const Vector3 &center, const Vector3 &axisX, const Vector3 &axisY, float beginAngle, float endAngle, int steps, bool solid)
{
	beginAngle = CanonicalizeAngle(beginAngle);
	endAngle = CanonicalizeAngle(endAngle);

#if __IM_BATCHER
	if (grcBatcher::GetCurrent())
	{
		grcbReserve(DRAW_ARC, sizeof(Color32) + 3*sizeof(float) + 3*sizeof(Vector3) + 2*sizeof(int));
		grcbPushColor();
		grcbPush(r);
		grcbPush(center);
		grcbPush(axisX);
		grcbPush(axisY);
		grcbPush(beginAngle);
		grcbPush(endAngle);
		grcbPush(steps);
		grcbPush((int)solid);
		return;
	}
#endif

	int j;
	float x, y;

	int beginVert = int(steps * beginAngle / (2.0f * PI));
	int endVert = int(steps * endAngle / (2.0f * PI)) + 1;
	int circleVerts = endVert - beginVert;

	if (circleVerts <= 0)
	{
		circleVerts += steps;
	}

	if (circleVerts < 2)
	{
		// Always have at least two verts
		circleVerts = 2;
	}


	if (solid)
	{
#if __WIN32PC
		grcBegin(drawTris,(circleVerts - 1) * 3);
#else
		grcBegin(drawTriFan,circleVerts+1);
		grcVertex3f(center);
#endif
	}
	else
	{
		grcBegin(drawLineStrip,circleVerts+1);
	}

	// Draw the first tri/fan
#if __WIN32PC
	if (solid)
	{
		grcVertex3f(center);

		x=r*cosf(beginAngle);
		y=r*sinf(beginAngle);
		Vector3 pos;
		pos.SetScaled(axisX,x);
		pos.AddScaled(axisY,y);
		pos.Add(center);
		grcVertex3f(pos);

		float currentAngle = 2.0f * PI * float(beginVert + 1) / float(steps);
		x=r*cosf(currentAngle);
		y=r*sinf(currentAngle);
		pos.SetScaled(axisX,x);
		pos.AddScaled(axisY,y);
		pos.Add(center);
		grcVertex3f(pos);
	}
	else
#endif
	{
		x=r*cosf(beginAngle);
		y=r*sinf(beginAngle);
		Vector3 pos;
		pos.SetScaled(axisX,x);
		pos.AddScaled(axisY,y);
		pos.Add(center);
		grcVertex3f(pos);
	}

	// Draw the arc tris/fans
#if __WIN32PC
	if (solid)
	{
		for(j=1;j < circleVerts - 2;j++)
		{
			grcVertex3f(center);

			float currentAngle = 2.0f * PI * float((j + beginVert) % steps) / float(steps);
			x=r*cosf(currentAngle);
			y=r*sinf(currentAngle);
			Vector3 pos;
			pos.SetScaled(axisX,x);
			pos.AddScaled(axisY,y);
			pos.Add(center);
			grcVertex3f(pos);

			float nextAngle = 2.0f * PI * float((j + 1 + beginVert) % steps) / float(steps);
			x=r*cosf(nextAngle);
			y=r*sinf(nextAngle);
			pos.SetScaled(axisX,x);
			pos.AddScaled(axisY,y);
			pos.Add(center);
			grcVertex3f(pos);
		}
	}
	else
#endif
	{
		for(j=0;j<circleVerts - 1;j++)
		{
			float currentAngle = 2.0f * PI * float((j + beginVert) % steps) / float(steps);
			x=r*cosf(currentAngle);
			y=r*sinf(currentAngle);
			Vector3 pos;
			pos.SetScaled(axisX,x);
			pos.AddScaled(axisY,y);
			pos.Add(center);
			grcVertex3f(pos);
		}
	}

	// Draw the final tri/fan
#if __WIN32PC
	if (solid)
	{
		grcVertex3f(center);

		float currentAngle = 2.0f * PI * float((circleVerts - 2 + beginVert) % steps) / float(steps);
		x=r*cosf(currentAngle);
		y=r*sinf(currentAngle);
		Vector3 pos;
		pos.SetScaled(axisX,x);
		pos.AddScaled(axisY,y);
		pos.Add(center);
		grcVertex3f(pos);

		x=r*cosf(endAngle);
		y=r*sinf(endAngle);
		pos.SetScaled(axisX,x);
		pos.AddScaled(axisY,y);
		pos.Add(center);
		grcVertex3f(pos);
	}
	else
#endif
	{
		x=r*cosf(endAngle);
		y=r*sinf(endAngle);
		Vector3 pos;
		pos.SetScaled(axisX,x);
		pos.AddScaled(axisY,y);
		pos.Add(center);
		grcVertex3f(pos);
	}

	grcEnd();
}


void grcDrawCapsule(float length,float radius,const Vector3 &center,int steps,bool solid) {
	Matrix34 mtx(M34_IDENTITY);
	mtx.d.Set(center);
	grcDrawCapsule(length,radius,mtx,steps,solid);
}


void grcDrawCapsule(float length,float radius,const Matrix34 &mtx,int steps,bool solid, float halfHeight) {
#if __IM_BATCHER
	if (grcBatcher::GetCurrent())
	{
		grcbReserve(DRAW_CAPSULE, sizeof(Color32) + 3*sizeof(float) + sizeof(Matrix34) + 2*sizeof(int));
		grcbPushColor();
		grcbPush(length);
		grcbPush(radius);
		grcbPush(mtx);
		grcbPush(steps);
		grcbPush((int)solid);
		grcbPush(halfHeight);
		return;
	}
#endif
	
	grcWorldMtx(mtx);

	if (solid)
	{
		float halflength = length * 0.5f;
// TODO: fix drawing of solid capsule with non 0.0 halfWidth 

		for(int i=0;i<(steps>>1);i++) 
		{
			float y=radius*cosf(PI*float(i)/float(steps>>1));
			float nexty=radius*cosf(PI*float(i+1)/float(steps>>1));
			float deltay;
			if (i < (steps>>2))
			{
				deltay = halflength;
			}
			else
			{
				deltay = -halflength;
			}
			float s=sinf(PI*float(i)/float(steps>>1));
			float nexts=sinf(PI*float(i+1)/float(steps>>1));
			grcBegin(drawTriStrip,(steps+1) * 2);
			for(int j=0;j<=steps;j++) 
			{
				const float x = radius*s*sinf(2.0f*PI*float(j)/float(steps));
				const float nextx = radius*nexts*sinf(2.0f*PI*float(j+1)/float(steps));
				const float z = radius*s*cosf(2.0f*PI*float(j)/float(steps));
				const float nextz = radius*nexts*cosf(2.0f*PI*float(j+1)/float(steps));
				Vector3 normal(x, y, z);
				normal.Normalize();
				grcNormal3f(normal);
				grcVertex3f(x,y+deltay,z);
				Vector3 nextNormal(nextx, nexty, nextz);
				nextNormal.Normalize();
				grcNormal3f(nextNormal);
				grcVertex3f(nextx,nexty+deltay,nextz);
			}
			grcEnd();
		}

		grcBegin(drawTriStrip,2*(steps+1));
		for(int j=0;j<=steps;j++) 
		{
			const float y = 0.0f;
			Vector3 normal(cosf(2.0f*PI*float(j)/float(steps)), 0.0f, sinf(2.0f*PI*float(j)/float(steps)));
			const float x = radius*normal.x;
			const float z = radius*normal.z;

			grcNormal3f(normal);
			grcVertex3f(x,y-halflength,z);
			grcVertex3f(x,y+halflength,z);
		}
		grcEnd();
	}
	else
	{
		const float halflength = length * 0.5f;
		const float invSteps = 1.0f / float(steps);
		const float halfInvSteps = invSteps * 0.5f;
		float f;
		int j;

		// vertical bottom half circle
		grcBegin(drawLineStrip,steps+1);
		for(j=0, f=0.0f ;j<=steps;j++, f +=1.0f) 
		{
			const float x = 0.0f;
			const float y = radius*cosf(-PI/2+PI* f * invSteps);
			const float angle = sinf(-PI/2+PI* f * invSteps);
			const float z = radius*angle;
			const float mult = Selectf( angle, 1.0f, -1.0f );
			grcVertex3f(x,y+halflength,z+mult*halfHeight);
		}
		grcEnd();

		// vertical bottom full circle cap 90 degrees to previous step
		grcBegin(drawLineStrip,steps*2+1);
		for(j=0, f=0.0f ;j<=steps*2;j++, f +=1.0f) 
		{
			const float y = 0.0f;
			const float x = radius*cosf(2.0f*PI* f * halfInvSteps );
			const float angle = sinf(2.0f*PI* f * halfInvSteps );
			const float z= radius*angle;
			const float mult = Selectf( angle, 1.0f, -1.0f );
			grcVertex3f(x,y+halflength,z+mult*halfHeight);
		}
		grcEnd();

		// vertical bottom half circle 90 degrees to previous half circle
		grcBegin(drawLineStrip,steps+1);
		for(j=0, f=0.0f ;j<=steps;j++, f +=1.0f) 
		{
			const float z = 0.0f;
			const float x = radius*cosf(PI* f * invSteps );
			const float y = radius*sinf(PI* f * invSteps );
			grcVertex3f(x,y+halflength,z);
		}
		grcEnd();

		// vertical top half circle
		grcBegin(drawLineStrip,steps+1);
		for(j=0, f=0.0f ;j<=steps;j++, f +=1.0f) 
		{
			const float x = 0.0f;
			const float y = radius*cosf(PI/2+PI* f * invSteps );
			const float angle = sinf(PI/2+PI* f * invSteps );
			const float z = radius*angle;
			const float mult = Selectf( angle, 1.0f, -1.0f );
			grcVertex3f(x,y-halflength,z+mult*halfHeight);
		}
		grcEnd();

		// vertical top full circle cap 90 degrees to previous step
		grcBegin(drawLineStrip,steps*2+1);
		for(j=0, f=0.0f;j<=steps*2;j++, f +=1.0f) 
		{
			const float y = 0.0f;
			const float x  =radius*cosf(2.0f*PI*f * halfInvSteps );
			const float angle = sinf(2.0f*PI*f * halfInvSteps );
			const float z = radius*angle;
			const float mult = Selectf( angle, 1.0f, -1.0f );
			grcVertex3f(x,y-halflength,z+mult*halfHeight);
		}
		grcEnd();

		// vertical top half circle 90 degrees to previous half circle
		grcBegin(drawLineStrip,steps+1);
		for(j=0;j<=steps;j++) 
		{
			const float z = 0.0f;
			const float x = radius*cosf(PI+PI*f * invSteps );
			const float y = radius*sinf(PI+PI*f * invSteps );
			grcVertex3f(x,y-halflength,z);
		}
		grcEnd();

		// lines connecting top and bottom full circles
		grcBegin(drawLines,2*steps);
		for(j=0, f=0.0f;j<steps;j++, f +=1.0f) 
		{
			const float y = 0.0f;
			const float x = radius*cosf(2.0f*PI* f * invSteps);
			const float angle = sinf(2.0f*PI* f * invSteps);
			const float z = radius*angle;			
			const float mult = Selectf( angle, 1.0f, -1.0f );

			grcVertex3f(x,y-halflength,z+mult*halfHeight);
			grcVertex3f(x,y+halflength,z+mult*halfHeight);			
		}
		grcEnd();
	}
}


void grcDrawTaperedCapsule(float length,float radiusA,float radiusB,const Matrix34 &mtx,int steps,bool solid) {
#if __IM_BATCHER
	if (grcBatcher::GetCurrent())
	{
		grcbReserve(DRAW_TAPERED_CAPSULE, sizeof(Color32) + 3*sizeof(float) + sizeof(Matrix34) + 2*sizeof(int));
		grcbPushColor();
		grcbPush(length);
		grcbPush(radiusA);
		grcbPush(radiusB);
		grcbPush(mtx);
		grcbPush(steps);
		grcbPush((int)solid);
		return;
	}
#endif
	float x, y, z;
	int j;

	grcWorldMtx(mtx);

	float halfLength = length * 0.5f;

	float slopeSine = (radiusA-radiusB)/length;
	float slopeAngle = asinf(slopeSine);
	float twiceSlopeAngle = 2.0f*slopeAngle;
	float angleAroundA = PI+twiceSlopeAngle;
	float angleAroundB = PI-twiceSlopeAngle;
	float verticalA = halfLength-radiusA*slopeSine;
	float verticalB = -halfLength-radiusB*slopeSine;

	if (solid)
	{
		for(int i=0;i<(steps>>1);i++) {
			float deltay,radius,angleAround;
			if (i < (steps>>2))
			{
				deltay = halfLength;
				radius = radiusA;
				angleAround = angleAroundA;
			}
			else
			{
				deltay = -halfLength;
				radius = radiusB;
				angleAround = angleAroundB;
			}

			float stepAngle = angleAround*(-0.5f+float(i)/float(steps>>1));
			float y = radius*cosf(stepAngle)*deltay/halfLength;
			float s = sinf(stepAngle);
			float nextStepAngle = angleAround*(-0.5f+float(i+1)/float(steps>>1));
			float nexty = radius*cosf(nextStepAngle)*deltay/halfLength;
			float nexts = sinf(nextStepAngle);
			grcBegin(drawTriStrip,(steps+1) * 2);
			for(int j=0;j<=steps;j++) {
				float x=radius*s*cosf(2.0f*PI*float(j)/float(steps));
				float nextx=radius*nexts*cosf(2.0f*PI*float(j+1)/float(steps));
				float z=radius*s*sinf(2.0f*PI*float(j)/float(steps));
				float nextz=radius*nexts*sinf(2.0f*PI*float(j+1)/float(steps));
				Vector3 normal(x, y, z);
				normal.Normalize();
				grcNormal3f(normal);
				grcVertex3f(x,y+deltay,z);
				Vector3 nextNormal(nextx, nexty, nextz);
				nextNormal.Normalize();
				grcNormal3f(nextNormal);
				grcVertex3f(nextx,nexty+deltay,nextz);
			}
			grcEnd();
		}

		y=0;
		float slopeCosine = SqrtfSafe(1.0f-square(slopeSine));
		grcBegin(drawTriStrip,2*(steps+1));
		for(j=0;j<=steps;j++) {
			Vector3 normal(slopeCosine*cosf(2.0f*PI*float(j)/float(steps)), -slopeSine, slopeCosine*sinf(2.0f*PI*float(j)/float(steps)));

			grcNormal3f(normal);
			grcVertex3f(radiusB*normal.x,y+verticalB,radiusB*normal.z);
			grcVertex3f(radiusA*normal.x,y+verticalA,radiusA*normal.z);
		}
		grcEnd();
	}
	else
	{
		// Draw a curved line in y,z for the top hemisphere.
		x=0;
		grcBegin(drawLineStrip,steps+1);
		for(j=0;j<=steps;j++) {
			float stepAngle = angleAroundA*(-0.5f+float(j)/float(steps));
			y=radiusA*cosf(stepAngle);
			z=radiusA*sinf(stepAngle);
			grcVertex3f(x,y+halfLength,z);
		}
		grcEnd();

		// Draw a circle in x,z for the top hemisphere.
		y=0;
		grcBegin(drawLineStrip,steps*2+1);
		for(j=0;j<=steps*2;j++) {
			float stepAngle = 2.0f*PI*float(j)/float(steps*2);
			x=radiusA*cosf(stepAngle);
			z=radiusA*sinf(stepAngle);
			grcVertex3f(x,y+verticalA,z);
		}
		grcEnd();

		// Draw a curved line in x,y for the top hemisphere.
		z=0;
		grcBegin(drawLineStrip,steps+1);
		for(j=0;j<=steps;j++) {
			float stepAngle = angleAroundA*(-0.5f+float(j)/float(steps));
			x=radiusA*sinf(stepAngle);
			y=radiusA*cosf(stepAngle);
			grcVertex3f(x,y+halfLength,z);
		}
		grcEnd();
		
		// Draw a curved line in y,z for the bottom hemisphere.
		x=0;
		grcBegin(drawLineStrip,steps+1);
		for(j=0;j<=steps;j++) {
			float stepAngle = angleAroundB*(-0.5f+float(j)/float(steps));
			y=-radiusB*cosf(stepAngle);
			z=radiusB*sinf(stepAngle);
			grcVertex3f(x,y-halfLength,z);
		}
		grcEnd();

		// Draw a circle in x,z for the bottom hemisphere.
		y=0;
		grcBegin(drawLineStrip,steps*2+1);
		for(j=0;j<=steps*2;j++) {
			float stepAngle = 2.0f*PI*float(j)/float(steps*2);
			x=radiusB*cosf(stepAngle);
			z=radiusB*sinf(stepAngle);
			grcVertex3f(x,y+verticalB,z);
		}
		grcEnd();

		// Draw a curved line in x,y for the bottom hemisphere.
		z=0;
		grcBegin(drawLineStrip,steps+1);
		for(j=0;j<=steps;j++) {
			float stepAngle = angleAroundB*(-0.5f+float(j)/float(steps));
			x=radiusB*sinf(stepAngle);
			y=-radiusB*cosf(stepAngle);
			grcVertex3f(x,y-halfLength,z);
		}
		grcEnd();

		// Draw the shaft as a set of angled lines in a horizontal circle.
		y=0;
		for(j=0;j<steps;j++) {

			grcBegin(drawLines,2);

			float cosineStep = cosf(2.0f*PI*float(j)/float(steps));
			float sineStep = sinf(2.0f*PI*float(j)/float(steps));
			x=radiusB*cosineStep;
			z=radiusB*sineStep;
			grcVertex3f(x,y+verticalB,z);

			x=radiusA*cosineStep;
			z=radiusA*sineStep;
			grcVertex3f(x,y+verticalA,z);
			grcEnd();
		}
	}
}


void grcDrawCylinder(float length,float radius,const Vector3 &center,int steps, const bool cbDrawTop) {
	Matrix34 mtx(M34_IDENTITY);
	mtx.d.Set(center);
	grcDrawCylinder(length,radius,mtx,steps,cbDrawTop);
}


void grcDrawCylinder(float length,float radius,const Matrix34 &mtx,int steps, const bool cbDrawTop) {
#if __IM_BATCHER
	if (grcBatcher::GetCurrent())
	{
		grcbReserve(DRAW_CYLINDER, sizeof(Color32) + 2*sizeof(float) + sizeof(Matrix34) + 2*sizeof(int));
		grcbPushColor();
		grcbPush(length);
		grcbPush(radius);
		grcbPush(mtx);
		grcbPush(steps);
		grcbPush((int)cbDrawTop);
		return;
	}
#endif
	float x, y, z, halflength;
	float prevX=0.0f, prevZ=0.0f;
	int j;
	grcWorldMtx(mtx);

	halflength = length * 0.5f;

	y=0;
	for(j=0;j<=steps;j++) {
		x=radius*cosf(2.0f*PI*float(j)/float(steps));
		z=radius*sinf(2.0f*PI*float(j)/float(steps));
		if(cbDrawTop)
		{
			grcBegin(drawLines,6);
			{
				grcVertex3f(x,y-halflength,z);
				grcVertex3f(x,y+halflength,z);
				grcVertex3f(x,y+halflength,z);
				grcVertex3f(0,y+halflength,0);
				grcVertex3f(x,y-halflength,z);
				grcVertex3f(0,y-halflength,0);
			}
			grcEnd();
		}
		else
		{
			grcBegin(drawLines,4);
			{
				grcVertex3f(x,y-halflength,z);
				grcVertex3f(x,y+halflength,z);
				grcVertex3f(x,y+halflength,z);
				grcVertex3f(x,y-halflength,z);
			}
			grcEnd();
		}

		if (j>0 ) {
			grcBegin(drawLines,4);
			grcVertex3f(x,y+halflength,z);
			grcVertex3f(prevX,y+halflength,prevZ);
			grcVertex3f(x,y-halflength,z);
			grcVertex3f(prevX,y-halflength,prevZ);
			grcEnd();
		}

		prevX = x;
		prevZ = z;
	}
}

void grcDraw2dText(float x, float y, const char* str, bool drawBgQuad, float scaleX, float scaleY, const grcFont* font) {
#if __IM_BATCHER
	if (grcBatcher::GetCurrent())
	{
		// get the size of the string (including terminating NUL, rounded up to the nearest 4 byte interval)
		int paddedLen = (StringLength(str) + 4) & ~3;

		grcbReserve(DRAW_STRING2D, sizeof(Color32) + 2*sizeof(float) + sizeof(int) + 2*sizeof(float) + sizeof(int) + sizeof(void*) + paddedLen);
		grcbPushColor();
		grcbPush(x);
		grcbPush(y);
		grcbPush(int(drawBgQuad));
		grcbPush(scaleX);
		grcbPush(scaleY);
		grcbPush(paddedLen);
		grcbPush(font);

		// push the contents of the string, a word at a time. The last word will contain the zero byte
		// possibly followed by some garbage data.
		for(int i = 0; i < paddedLen; i += 4) {
			grcbPush(*(u32*)(str + i));
		}
		return;
	}
#endif
	Assert(str);
	PUSH_DEFAULT_SCREEN();

	if (font == NULL)
	{
		font = &grcFont::GetCurrent();
	}

	const float xx = x;
	const float yy = y;
	
	Color32 currCol(grcCurrentColor);

	if( drawBgQuad ) 
	{
		u32 strWidth = 0, strHeight =0;
		
		if (strrchr(str, '\n'))
		{
			char buffer[255];
			strcpy(buffer, str);
			char *pch = strtok (buffer, "\n");
			while (pch != NULL)
			{
				strWidth = Max(strWidth, (u32)strlen(pch));
				++strHeight;
				pch = strtok (NULL, "\n");
			}
		}
		else
		{
			strHeight = 1;
			strWidth = StringLength(str);
		}

		const u8 r = currCol.GetRed();
		const u8 g = currCol.GetGreen();
		const u8 b = currCol.GetBlue();
		const u8 luminosity = (u8)(0.2126f * r + 0.7152f * g + 0.0722 * b);
		const Color32 bgQuadColor((luminosity < 128) ? 255 : 0, (luminosity < 128) ? 255 : 0, (luminosity < 128) ? 255 : 0, currCol.GetAlpha()/2 );
									
		const float ww = float(font->GetWidth()) * strWidth * scaleX;
		const float hh = float(font->GetHeight()) * strHeight * scaleY;

		const float offset_y = __PS3 ? 1.0f : 0.0f; // offset aligns background quad correctly on PS3 (not tested on 360 yet)

		grcBindTexture(NULL);
		grcDrawSingleQuadf(	xx - 1.5f,
						yy - 1.5f + offset_y,
						xx + ww - 0.5f,
						yy + hh - 0.5f + offset_y,
						0.0f,
						0.0f,0.0f,1.0f,1.0f,
						bgQuadColor);
	}

	font->DrawScaled(xx,yy,0,currCol,scaleX,scaleY,str);
	POP_DEFAULT_SCREEN();
}

void grcDrawAxis(float size,const Matrix34 &mtx, bool drawArrows) {
#if __IM_BATCHER
	if (grcBatcher::GetCurrent())
	{
		grcbReserve(DRAW_AXIS, sizeof(float) + sizeof(Matrix34) + sizeof(int));
		grcbPush(size);
		grcbPush(mtx);
		grcbPush((int)drawArrows);
		return;
	}
#endif

	grcWorldMtx(mtx);
	grcBegin(drawLines,6);
	grcColor3f(1,0,0);
	grcVertex3f(0,0,0);
	grcVertex3f(size,0,0);
	grcColor3f(0,1,0);
	grcVertex3f(0,0,0);
	grcVertex3f(0,size,0);
	grcColor3f(0,0,1);
	grcVertex3f(0,0,0);
	grcVertex3f(0,0,size);
	grcEnd();

	if(drawArrows) {
		const float a0 = 0.8f * size;
		const float b0 = 0.07f * size;
		const float c0 = 1.0f * size;
		const float a1 = a0 + 0.01f * size;
		const float b1 = b0 - 0.01f * size;
	 	const float c1 = c0 - 0.01f * size;

		grcBegin(drawTris,9);
		grcColor4f(1.0f,0.0f,0.0f,0.5f);
		grcVertex3f(c1,0.0f,0.0f);
		grcVertex3f(a1,b1,b1);
		grcVertex3f(a1,-b1,-b1);
		grcColor4f(0.0f,1.0f,0.0f,0.5f);
		grcVertex3f(0.0f,c1,0.0f);
		grcVertex3f(b1,a1,b1);
		grcVertex3f(-b1,a1,-b1);
		grcColor4f(0.0f,0.0f,1.0f,0.5f);
		grcVertex3f(0.0f,0.0f,c1);
		grcVertex3f(b1,b1,a1);
		grcVertex3f(-b1,-b1,a1);
		grcEnd();

		grcBegin(drawLineStrip,4);
		grcColor3f(1,0,0);
		grcVertex3f(c0,0.0f,0.0f);
		grcVertex3f(a0,b0,b0);
		grcVertex3f(a0,-b0,-b0);
		grcVertex3f(c0,0.0f,0.0f);
		grcEnd();

		grcBegin(drawLineStrip,4);
		grcColor3f(0,1,0);
		grcVertex3f(0.0f,c0,0.0f);
		grcVertex3f(b0,a0,b0);
		grcVertex3f(-b0,a0,-b0);
		grcVertex3f(0.0f,c0,0.0f);
		grcEnd();

		grcBegin(drawLineStrip,4);
		grcColor3f(0,0,1);
		grcVertex3f(0.0f,0.0f,c0);
		grcVertex3f(b0,b0,a0);
		grcVertex3f(-b0,-b0,a0);
		grcVertex3f(0.0f,0.0f,c0);
		grcEnd();
	}
}

void grcDrawAxis(float size,Mat34V_In mtx, bool drawArrows)
{
	grcDrawAxis(size,RCC_MATRIX34(mtx),drawArrows);
}

//
// Spherical Cone
// http://mathworld.wolfram.com/SphericalCone.html
//
//
void grcDrawSolidSphericalCone(const Vector3 &pos,
							   const Vector3 &dir, 
							   const float halfAngleRadians, 
							   const float radius,
							   const Color32 solidCol, 
							   const Color32 lineCol,
							   const int coneSteps, 
							   const int capSteps)
{
	Matrix34 mtrx(M34_IDENTITY);
	if ( fabs( dir.y ) < 0.99f)		// check so mess up cross product.
	{
		mtrx.LookAt(dir, Vector3(0, 0, 0), YAXIS);
	}
	else
	{
		mtrx.LookAt(dir, Vector3(0, 0, 0), XAXIS);
	}

	mtrx.d.Set(pos);

	grcWorldMtx(mtrx);

	int i, j;
	Vector3 p1, p2;
	p2.Zero();
	Vector3 orth, yAxis(0, 1.0f, 0);
	orth.Cross(yAxis, dir);
	float f = halfAngleRadians;
	float x1, z1;
	float x = 0, z = 0;
	float fs, fc;
	cos_and_sin(fc, fs, f);
	for(i = 0; i <= coneSteps; i++)
	{
		p1 = p2;
		x1 = x;
		z1 = z;
		cos_and_sin(x, z, i/(float)coneSteps*2*PI - PI);

		p2.Set(radius*x*fs, radius*z*fs, radius*fc);

		if(i == 0)
			continue;

		grcColor(solidCol);
		grcBegin(drawTriFan, 3);
		grcVertex3f(Vector3(0, 0, 0));
		grcVertex3f(p1);
		grcVertex3f(p2);
		grcEnd();

		if(lineCol != solidCol)
		{
			grcColor(lineCol);
			grcBegin(drawLines, 2);
			grcVertex3f(Vector3(0, 0, 0));
			grcVertex3f(p1);
			grcEnd();
		}

		Vector3 p1a, p1b, p2a, p2b;
		p1b.Zero(); p2b.Zero();
		for(j = 0; j <= capSteps; j++)
		{
			p1a.Set(p1b);
			p2a.Set(p2b);

			float q = f*j/(float)capSteps;
			float qc, qs;
			cos_and_sin(qc, qs, q);
			p1b.Set(radius*x*qs, radius*z*qs, radius*qc);
			p2b.Set(radius*x1*qs, radius*z1*qs, radius*qc);

			if(j == 0)
				continue;

			grcColor(solidCol);
			grcBegin(drawTriFan, 6); // this was drawTriFan
			grcVertex3f(p2b);
			grcVertex3f(p2a);
			grcVertex3f(p1a);

			grcVertex3f(p1a);
			grcVertex3f(p1b);
			grcVertex3f(p2b);
			grcEnd();

			if(lineCol != solidCol)
			{
				grcColor(lineCol);
				grcBegin(drawLines, 2);
				grcVertex3f(p2a);
				grcVertex3f(p2b);
				grcEnd();

				grcBegin(drawLines, 2);
				grcVertex3f(p1b);
				grcVertex3f(p2b);
				grcEnd();
			}
		}
	}
}


grcProjectStatus grcProject(Vec3V_In V,float &x,float &y) {
	AssertMsg(grcViewport::GetCurrent(), "Must supply your own camera and viewport when calling vglProject outside of frame");
	const grcViewport &vp = *grcViewport::GetCurrent();
	return grcProject(V,x,y,vp.GetViewMtx(),vp);
}



grcProjectStatus grcProject(Vec3V_In V,float &x,float &y,Mat44V_In viewMtx,const grcViewport &viewport) {
	Vec4V dest = Multiply(viewport.GetProjection(),Multiply(viewMtx,Vec4V(V,ScalarV(V_ONE))));
	if (dest.GetWf() > 0) {
		const grcWindow &window = viewport.GetWindow();
		float hw = window.GetNormWidth() * 0.5f;
		float hh = window.GetNormHeight() * 0.5f;
		float nx = (window.GetNormX() + (hw + hw * (dest.GetXf() / dest.GetWf())));
		float ny = (window.GetNormY() + (hh - hh * (dest.GetYf() / dest.GetWf())));
		x = nx * viewport.GetWidth();
		y = ny * viewport.GetHeight();
		return (nx >= window.GetNormX() && nx < window.GetNormX() + window.GetNormWidth() && 
			ny >= window.GetNormY() && ny < window.GetNormY() + window.GetNormHeight())? grcProjectVisible : grcProjectOffscreen;
	}
	else
		return grcProjectBehind;
}



void grcDrawLabel(const Vector3 &V,int xOffset,int yOffset,const char *string, bool drawBgQuad, float xScale, float yScale) {
#if __IM_BATCHER
	if (grcBatcher::GetCurrent())
	{
		// get the size of the string (rounded up to the nearest 4 byte interval)
		int len = StringLength(string);
		len++; // add the '\0' byte.
		int paddedLen = len;
		while (paddedLen % 4 != 0) {
			paddedLen++;
		}

		grcbReserve(DRAW_LABEL, sizeof(Color32) + sizeof(Vector3) + 4*sizeof(float) + sizeof(int) + sizeof(int) + paddedLen);
		grcbPushColor();
		grcbPush(V);
		grcbPush(xOffset);
		grcbPush(yOffset);
		grcbPush(xScale);
		grcbPush(yScale);
		grcbPush(int(drawBgQuad));
		grcbPush(paddedLen);

		// push the contents of the string, a word at a time. The last word will contain the zero byte
		// possibly followed by some garbage data.
		for(int i = 0; i < paddedLen; i += 4) {
			char temp[4] = {string[i], i + 1 < len ? string[i + 1] : 0, i + 2 < len ? string[i + 2] : 0, i + 3 < len ? string[i + 3] : 0};
			grcbPush(*(u32*)temp);
		}
		return;
	}
#endif
	float x, y;
	if (grcProject(RCC_VEC3V(V),x,y) == grcProjectVisible) {
		PUSH_DEFAULT_SCREEN();
		grcFont& font = grcFont::GetCurrent();

		const float xx = xOffset + x;
		const float yy = yOffset + y;
		
		Color32 currCol(grcCurrentColor);

		if( drawBgQuad ) 
		{
			u32 strWidth = 0, strHeight = 0;
			
			if (strrchr(string, '\n') != strchr(string, '\n'))
			{
				char buffer[255];
				strcpy(buffer, string);
				char *pch = strtok (buffer, "\n");
				while (pch != NULL)
				{
					strWidth = Max(strWidth, (u32)strlen(pch));
					++strHeight;
					pch = strtok (NULL, "\n");
				}
			}
			else
			{
				strHeight = 1;
				strWidth = StringLength(string);
			}

			const Color32 bgQuadColor(	255 - currCol.GetRed(),
										255 - currCol.GetGreen(),
										255 - currCol.GetBlue(),
										currCol.GetAlpha()/2 );
										
			const float ww = float(font.GetWidth()) * strWidth * xScale;
			const float hh = float(font.GetHeight()) * strHeight * yScale;

			grcBindTexture(NULL);
			grcDrawSingleQuadf(	xx - 1.5f,
							yy - 1.5f,
							xx + ww - 0.5f,
							yy + hh - 0.5f,
							USE_INVERTED_PROJECTION_ONLY ? -1.0 : 0.0, // PLEASE MAKE IT STOP
							0.0f,0.0f,1.0f,1.0f,
							bgQuadColor);
		}
		
		font.DrawScaled(xx,yy,USE_INVERTED_PROJECTION_ONLY ? -1.0 : 0.0,currCol,xScale,yScale,string); // PLLLLLLEEEEAAAASE
		POP_DEFAULT_SCREEN();
	}
}



void grcDrawLabel(const Vector3 &V,const char *string,bool drawBgQuad) {
	grcDrawLabel(V,0,0,string,drawBgQuad);
}

void grcDrawLabel(Vec3V_In V,const char *fmt,bool drawBgQuad)
{
	grcDrawLabel(RCC_VECTOR3(V),0,0,fmt,drawBgQuad);
}

void grcDrawLabelf(const Vector3 &V,const char *fmt,...) {
	va_list args;
	va_start(args,fmt);
	char buf[256];
	vformatf(buf,sizeof(buf),fmt,args);
	grcDrawLabel(V,0,0,buf);
	va_end(args);
}



void grcDrawLabelf(const Vector3 &V,int xo,int yo,const char *fmt,...) {
	va_list args;
	va_start(args,fmt);
	char buf[256];
	vformatf(buf,sizeof(buf),fmt,args);
	grcDrawLabel(V,xo,yo,buf);
	va_end(args);
}


void grcDrawPolygon(const Vector3 *points, int count, const Vector3 *pNormal, bool solid, Color32 color)
{
	Assert(points);
	Assert(count > 0);

#if __IM_BATCHER
	if( solid && grcBatcher::GetCurrent() )
	{
		// get the size of the string (rounded up to the nearest 4 byte interval)
		grcbReserve(DRAW_POLYGON, sizeof(Color32) + sizeof(int) + sizeof(Vector3) * count);
		
		grcbPush(color);
		grcbPush(count);
		for(int i=0;i<count;i++)
		{
			grcbPush(points[i]);
		}
		return;
	}
#endif

	// Might as well calculate this.
	Vector3 center(0.0f, 0.0f, 0.0f);

	if (solid)
	{
		grcColor(color);
#if __WIN32PC
		int i;
		for (i = 0; i < count; i++)
		{
			center += points[i];
		}

		grcBegin(drawTris, (count - 2) * 3);
		for (int i = 2; i < count; i++)
		{
			grcVertex3f(points[0]);
			grcVertex3f(points[i-1]);
			grcVertex3f(points[i]);
		}
#else
		grcBegin(drawTriFan, count);

		for (int i = 0; i < count; i++)
		{
			center += points[i];
			grcVertex3f(points[i]);
		}
#endif
		grcEnd();
	}
	else
	{
		for (int i = 0; i < count; i++)
		{
			center += points[i];

			int j = i + 1;
			if (j == count)
				j = 0;

			Vector3 p1(points[i]);
			Vector3 p2(points[j]);

			// Draw an edge.
			grcDrawLine(p1, p2, color);
		}
	}

	if (pNormal)
	{
		center /= (float)count;
		const Vec4V scale = Vec4V(Vec3V(V_HALF), ScalarV(V_ONE)); // {0.5,0.5,0.5,1.0}
		grcDrawLine(center, center + *pNormal, Color32(color.GetRGBA()*scale));
	}
}

void grcDrawTexturedPolygon(const Vector3 *points, int count, const Vector3 *pNormal, bool solid, Color32 color, grcTexture *pTex, const Vector2 *uvs)
{
	Assert(points);
	Assert(count > 0);
	Assert(pTex);

#if __IM_BATCHER
	if( solid && grcBatcher::GetCurrent() )
	{
		// get the size of the string (rounded up to the nearest 4 byte interval)
		grcbReserve(DRAW_POLYGON_TEX, sizeof(Color32) + sizeof(grcTexture*) + sizeof(int)  + (sizeof(Vector3)+sizeof(Vector2)) * count);
		
		grcbPush(color);
		grcbPush(pTex);
		grcbPush(count);
		for(int i=0;i<count;i++)
		{
			grcbPush(points[i]);
			grcbPush(uvs[i]);
		}
		return;
	}
#endif

	// Might as well calculate this.
	Vector3 center(0.0f, 0.0f, 0.0f);


	if (solid)
	{
		grcBindTexture(pTex);

		grcColor(color);
#if __WIN32PC
		int i;
		for (i = 0; i < count; i++)
		{
			center += points[i];
		}

		grcBegin(drawTris, (count - 2) * 3);
		for (int i = 2; i < count; i++)
		{
			grcTexCoord2f(uvs[0].x, uvs[0].y);
			grcVertex3f(points[0]);

			grcTexCoord2f(uvs[i-1].x, uvs[i-1].y);
			grcVertex3f(points[i-1]);

			grcTexCoord2f(uvs[i].x, uvs[i].y);
			grcVertex3f(points[i]);
		}
#else
		grcBegin(drawTriFan, count);

		for (int i = 0; i < count; i++)
		{
			center += points[i];

			grcTexCoord2f(uvs[i].x, uvs[i].y);
			grcVertex3f(points[i]);
		}
#endif
		grcEnd();
		grcBindTexture(NULL);
	}
	else
	{
		for (int i = 0; i < count; i++)
		{
			center += points[i];

			int j = i + 1;
			if (j == count)
				j = 0;

			Vector3 p1(points[i]);
			Vector3 p2(points[j]);

			// Draw an edge.
			grcDrawLine(p1, p2, color);
		}
	}

	if (pNormal)
	{
		center /= (float)count;
		const Vec4V scale = Vec4V(Vec3V(V_HALF), ScalarV(V_ONE)); // {0.5,0.5,0.5,1.0}
		grcDrawLine(center, center + *pNormal, Color32(color.GetRGBA()*scale));
	}
}

void grcDrawPolygon(const Vector3 *points, int count, const Vector3 *pNormal, bool solid, Color32 *color)
{
	Assert(points);
	Assert(count > 0);

#if __IM_BATCHER
	if( solid && grcBatcher::GetCurrent() )
	{
		// get the size of the string (rounded up to the nearest 4 byte interval)
		grcbReserve(DRAW_POLYGON_COLOR, sizeof(int) + sizeof(Vector3) * count + sizeof(Color32) * count);
		
		grcbPush(count);
		for(int i=0;i<count;i++)
		{
			grcbPush(points[i]);
			grcbPush(color[i]);
		}
		return;
	}
#endif

	// Might as well calculate this.
	Vector3 center(0.0f, 0.0f, 0.0f);

	if (solid)
	{
#if __WIN32PC
		int i;
		for (i = 0; i < count; i++)
		{
			center += points[i];
		}

		grcBegin(drawTris, (count - 2) * 3);
		for (int i = 2; i < count; i++)
		{
			grcColor(color[0]);
			grcVertex3f(points[0]);
			grcColor(color[i-1]);
			grcVertex3f(points[i-1]);
			grcColor(color[i]);
			grcVertex3f(points[i]);
		}
#else
		grcBegin(drawTriFan, count);

		for (int i = 0; i < count; i++)
		{
			center += points[i];
			grcColor(color[i]);
			grcVertex3f(points[i]);
		}
#endif
		grcEnd();
	}
	else
	{
		for (int i = 0; i < count; i++)
		{
			center += points[i];

			int j = i + 1;
			if (j == count)
				j = 0;

			Vector3 p1(points[i]);
			Vector3 p2(points[j]);

			// Draw an edge.
			grcDrawLine(p1, p2, color[i],color[j]);
		}
	}

	if (pNormal)
	{
		center /= (float)count;
		const Vec4V scale = Vec4V(0.5f/3.0f, 0.5f/3.0f, 0.5f/3.0f, 1.0f/3.0f);
		grcDrawLine(center, center + *pNormal, Color32((color[0].GetRGBA() + color[1].GetRGBA() + color[2].GetRGBA())*scale));
	}
}

void grcDrawTexturedPolygon(const Vector3 *points, int count, const Vector3 *pNormal, bool solid, Color32 *colors, grcTexture *pTex, const Vector2 *uvs)
{
	Assert(points);
	Assert(count > 0);
	Assert(pTex);

#if __IM_BATCHER
	if( solid && grcBatcher::GetCurrent() )
	{
		// get the size of the string (rounded up to the nearest 4 byte interval)
		grcbReserve(DRAW_POLYGON_TEX_COLOR, sizeof(grcTexture*) + sizeof(int)  + (sizeof(Vector3)+sizeof(Color32)+sizeof(Vector2)) * count);
		
		grcbPush(pTex);
		grcbPush(count);
		for(int i=0;i<count;i++)
		{
			grcbPush(points[i]);
			grcbPush(colors[i]);
			grcbPush(uvs[i]);
		}
		return;
	}
#endif

	// Might as well calculate this.
	Vector3 center(0.0f, 0.0f, 0.0f);


	if (solid)
	{
		grcBindTexture(pTex);

#if __WIN32PC
		int i;
		for (i = 0; i < count; i++)
		{
			center += points[i];
		}

		grcBegin(drawTris, (count - 2) * 3);
		for (int i = 2; i < count; i++)
		{
			grcColor(colors[0]);
			grcTexCoord2f(uvs[0].x, uvs[0].y);
			grcVertex3f(points[0]);

			grcColor(colors[i-1]);
			grcTexCoord2f(uvs[i-1].x, uvs[i-1].y);
			grcVertex3f(points[i-1]);

			grcColor(colors[i]);
			grcTexCoord2f(uvs[i].x, uvs[i].y);
			grcVertex3f(points[i]);
		}
#else
		grcBegin(drawTriFan, count);

		for (int i = 0; i < count; i++)
		{
			center += points[i];

			grcColor(colors[i]);
			grcTexCoord2f(uvs[i].x, uvs[i].y);
			grcVertex3f(points[i]);
		}
#endif
		grcEnd();
		grcBindTexture(NULL);
	}
	else
	{
		for (int i = 0; i < count; i++)
		{
			center += points[i];

			int j = i + 1;
			if (j == count)
				j = 0;

			Vector3 p1(points[i]);
			Vector3 p2(points[j]);

			// Draw an edge.
			grcDrawLine(p1, p2, colors[i],colors[j]);
		}
	}

	if (pNormal)
	{
		center /= (float)count;
		const Vec4V scale = Vec4V(Vec3V(V_HALF), ScalarV(V_ONE)); // {0.5,0.5,0.5,1.0}
		grcDrawLine(center, center + *pNormal, Color32((colors[0].GetRGBA() + colors[1].GetRGBA() + colors[2].GetRGBA())*scale));
	}
}

void grcDrawPolygon(const atArray<Vector3> &points, const Vector3 *pNormal, bool solid, Color32 color)
{
	int count = points.GetCount();
	Assert(count > 0);

	// Might as well calculate this.
	Vector3 center(0.0f, 0.0f, 0.0f);

	if (solid)
	{
		grcColor(color);

#if __WIN32PC
		int i;
		for (i = 0; i < count; i++)
		{
			center += points[i];
		}

		grcBegin(drawTris, (count - 2) * 3);
		for (int i = 2; i < count; i++)
		{
			grcVertex3f(points[0]);
			grcVertex3f(points[i-1]);
			grcVertex3f(points[i]);
		}
#else
		grcBegin(drawTriFan, count);

		for (int i = 0; i < count; i++)
		{
			center += points[i];
			grcVertex3f(points[i]);
		}
#endif
		grcEnd();
	}
	else
	{
		for (int i = 0; i < count; i++)
		{
			center += points[i];

			int j = i + 1;
			if (j == count)
				j = 0;

			Vector3 p1(points[i]);
			Vector3 p2(points[j]);

			// Draw an edge.
			grcDrawLine(p1, p2, color);
		}
	}

	if (pNormal)
	{
		center /= (float)count;
		const Vec4V scale = Vec4V(Vec3V(V_HALF), ScalarV(V_ONE)); // {0.5,0.5,0.5,1.0}
		grcDrawLine(center, center + *pNormal, Color32(color.GetRGBA()*scale));
	}
}

void grcDraw(const grcDrawProxy& proxy)
{
#if __IM_BATCHER || __ASSERT
	const bool bNeedsAlign16 = true;
#endif

	Assertf((((size_t)&proxy) & 15) == 0 || !bNeedsAlign16, "grcDrawProxy not aligned!");

#if __IM_BATCHER
	grcBatcher* batcher = grcBatcher::GetCurrent();

	if (batcher)
	{
		const int size = proxy.Size();
		int cmd = DRAW_PROXY;
		int offset = 0;

		if (bNeedsAlign16)
		{
			size_t addr = (size_t)batcher->GetCursor() + sizeof(u32); // address where proxy would be stored (after command)

			offset = (16 - (addr & 15)) & 15; // offset in bytes so that proxy is aligned
			cmd |= (offset << 6); // encode 2-bit offset along with command (note that offset should have lowest two bits clear)
		}

		grcbReserve(cmd, offset + size);
		batcher->PushData(proxy, offset, size);
		return;
	}
#endif
	proxy.Draw();
}

void grcVertexArray(grcDrawMode dm, const atArray<Vec3V>& verts)
{
	int vertsPerPrim = 1;

	switch (dm)
	{
	case drawPoints    : vertsPerPrim = 1; break;
	case drawLines     : vertsPerPrim = 2; break;
	case drawLineStrip : vertsPerPrim = 1; break; // join by repeating last vertex
	case drawTris      : vertsPerPrim = 3; break;
	case drawTriStrip  : vertsPerPrim = 1; break; // join by repeating last two vertices (possibly an extra vertex to preserve winding)
	case drawTriFan    : vertsPerPrim = 1; break; // join by repeating first vertex followed by last vertex
	case drawQuads     : vertsPerPrim = 4; break;
	case drawTrisAdj   : vertsPerPrim = 6; break;
	default: return;
	}

	const int vertsPerDraw = (grcBeginMax/vertsPerPrim)*vertsPerPrim; // max verts per draw call, round down to multiple of vertsPerPrim
	int start = 0;
	int count = verts.size();

	Assert(count%vertsPerPrim == 0);

	int glue[3]; // extra vert indices to glue multiple draw call together
	int glueCount = 0;

	while (start < count)
	{
		const int end = Min<int>(start + vertsPerDraw - glueCount, count);

		grcBegin(dm, end - start + glueCount);

		for (int i = 0; i < glueCount; i++)
		{
			grcVertex3f(verts[glue[i]]);
		}

		glueCount = 0;

		for (int i = start; i < end; i++)
		{
			grcVertex3f(verts[i]); 
		}

		start = end;

		if (start < count)
		{
			if (dm == drawLineStrip)
			{
				glue[glueCount++] = end - 1;
			}
			else if (dm == drawTriStrip)
			{
				if ((end & 1) == 0) // not sure, maybe should be the opposite?
				{
					glue[glueCount++] = end - 2;
				}

				glue[glueCount++] = end - 2;
				glue[glueCount++] = end - 1;
			}
			else if (dm == drawTriFan)
			{
				glue[glueCount++] = 0;
				glue[glueCount++] = end - 1;
			}
		}

		grcEnd();
	}
}

} // namespace rage
