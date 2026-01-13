//
// grcore/viewport.cpp
//
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved.
//

#include "viewport.h"
#include "viewport_inline.h"
#include "device.h"
#include "effect.h"
#include "effect.inl"
#include "effect_config.h"
#include "atl/array_struct.h"
#include "data/safestruct.h"
#include "profile/element.h"
#include "vectormath/vectortypes.h"
#include "vectormath/mat34v.h"
#include "vectormath/mat44v.h"
#include "vectormath/legacyconvert.h"
#include "channel.h"
#include "shaderlib/rage_constants.h"
#include "system/memops.h"
#include "system/nelem.h"
#include "system/stack.h"
#include "system/threadtype.h"


#include <math.h>

#if __OPENGL
#include "opengl.h"
#endif

#if __XENON
#include "system/xtl.h"
#include <xboxmath.h>
#elif __WIN32PC
#include "system/xtl.h"
#include "system/d3d9.h"
#elif __PPU
#include "effect_config.h"
#if GRCORE_ON_SPU
#include "grcorespu.h"
#include "wrapper_gcm.h"
#endif
#endif

#if __BANK
#include "grcore/im.h"
#include "bank/bank.h"
#endif

namespace rage {

#if RAGE_INSTANCED_TECH
grcViewport* grcViewport::sm_InstViewport[NUMBER_OF_RENDER_THREADS][MAX_NUM_CBUFFER_INSTANCING];
DECLARE_MTR_THREAD u32 grcViewport::sm_viewportInstancedRenderBit = 0xff;
DECLARE_MTR_THREAD bool grcViewport::sm_bInstancing = false;
DECLARE_MTR_THREAD u32 grcViewport::sm_uNumInstVP = 0;
DECLARE_MTR_THREAD u32 grcViewport::sm_uVPInstCount = 0;

DECLARE_MTR_THREAD void (*grcViewport::m_SetCurrentWorldMtxFn)(Mat34V_In) = &(grcViewport::SetCurrentWorldMtxFn_NonInstanced);
DECLARE_MTR_THREAD void (*grcViewport::m_SetCurrentWorldIdentityFn)() = &(grcViewport::SetCurrentWorldIdentityFn_NonInstanced);
DECLARE_MTR_THREAD void (*grcViewport::m_SetCurrentNearFarClipFn)(float,float) = &(grcViewport::SetCurrentNearFarClipFn_NonInstanced);
DECLARE_MTR_THREAD void (*grcViewport::m_SetCurrentWindowMinMaxZFn)(float,float) = &(grcViewport::SetCurrentWindowMinMaxZFn_NonInstanced);
#endif

#if HACK_GTA4
DECLARE_MTR_THREAD grcViewport* grcViewport::sm_Current = NULL;
#else
DECLARE_MTR_THREAD grcViewport* grcViewport::sm_Current;
#endif // HACK_GTA4

const grcViewport* grcViewport::sm_ScreenViewport[MULTIPLE_RENDER_THREADS ? (MULTIPLE_RENDER_THREADS+2) : 1];

#define gvINIT(x) if (!s_gv##x) s_gv##x = grcEffect::LookupGlobalVar(#x,false);

static grcEffectGlobalVar
	s_gvWorld,
	// s_gvWorldInverseTranspose,
	s_gvWorldView,
	s_gvWorldViewProjection,
	s_gvViewInverse;

#if !MULTIPLE_RENDER_THREADS
#define OwnerCheck()
#endif

grcViewport::grcViewport() {

#if SUPPORT_INVERTED_PROJECTION
	SetInvertZInProjectionMatrix(false, false);
#endif

#if SUPPORT_DEPTH_BIAS_IN_PROJECTION
	m_NDCDepthBias = 0.0f;
#endif

	Mat34V ident3x4(V_IDENTITY);
	Mat44V ident4x4(V_IDENTITY);
	
	m_World44		= ident4x4;
	m_Camera44		= ident4x4;
	m_View			= ident4x4;
	m_Screen		= ident4x4;

	// Can't call Perspective directly because it
	// depends on the window being correct.
	m_FovY = 60;
	m_TanVFOV = m_TanHFOV = 1.f;
	m_DesiredAspect = 0;
	m_ComputedAspect = 1;
	m_ZClipNear = 1;
	m_ZClipFar = 1000;
	m_ScaleX = m_ScaleY = 1;
	m_ShearX = m_ShearY = 0;
	m_IsPerspective = true;
	m_Owner = 0;
#if !GRCDEVICE_IS_STATIC
	if (g_pGRCDEVICE)
#endif
		ResetWindow();
}

grcViewport::~grcViewport() {
	if (sm_Current == this)
		SetCurrent(NULL);
}

grcViewport* grcViewport::SetCurrent(const grcViewport *viewport, bool regenDevice) {
	grcViewport *old = sm_Current;

	// A bit cheesy, but need to make sure these stay correct even if things start to get used later on.
	gvINIT(World);
	gvINIT(WorldView);
	gvINIT(WorldViewProjection);
	gvINIT(ViewInverse);
	
	AssertMsg(GRCDEVICE.CheckThreadOwnership(),"Thread that doesn't own the GPU trying to set current viewport.");

	sm_Current = const_cast<grcViewport*>(viewport);

#if SUPPORT_INVERTED_PROJECTION
	if (sm_Current)
		g_grcDepthInversionInvertProjection = sm_Current->GetInvertZInProjectionMatrix();
	else
		g_grcDepthInversionInvertProjection = false; // use the default 
#endif

#if RSG_DURANGO || RSG_ORBIS
	g_MatrixBase->SetBackingStore(&sm_Current->m_World44);
#endif

	if (sm_Current && regenDevice) {
		sm_Current->RegenerateScreen();
		sm_Current->RegenerateDevice();
		GRCDEVICE.SetWindow(sm_Current->m_Window);
	}

	return old;
}

#if __PPU
extern void SetMatrixForSpu(const Matrix44&);
#endif

#if RAGE_INSTANCED_TECH
void grcViewport::RegenerateLRTB(const Mat44V &worldmat)
{
	// need to regen LRTB for incoming culling
	Multiply(m_LocalLRTB,m_FrustumLRTB,worldmat);

#if NV_SUPPORT
	if(GRCDEVICE.CanUseStereo() && GRCDEVICE.IsStereoEnabled())
	{
		Multiply(m_CullLocalLRTB, m_CullFrustumLRTB, worldmat);
	}
	else
	{
		m_CullLocalLRTB = m_LocalLRTB;
	}
#endif
}
#endif


void grcViewport::RegenerateDevice() {
#if RSG_DURANGO || RSG_ORBIS
	g_MatrixBase->SetDataIndirect(&m_World44, 256);
#elif RSG_PC && __D3D11
	sysMemCpy(g_MatrixBase->BeginUpdate(256),&m_World44, 256);
	g_MatrixBase->EndUpdate();
#elif RSG_PS3
	// SPU needs to know the current view and projection matrices so it can recompute the composites.
	SPU_COMMAND(grcViewport__RegenerateDevice,0);
	// These four must be in sequential order, they're sent down as one block
	cmd->world = m_World44;
	cmd->worldView = m_ModelView;
	cmd->composite = m_Composite;
	cmd->viewInverse = m_Camera44;
	// End sequential block
	cmd->view = m_View;
	cmd->proj = m_Projection;
	cmd->frustum = m_FrustumLRTB;
#else	// DX9 or generic Xenon path
	GRCDEVICE.GetCurrent()->SetVertexShaderConstantF(MATRIX_BASE, (float*)&m_World44, MATRIX_SIZE);
	GRCDEVICE.GetCurrent()->SetPixelShaderConstantF(MATRIX_BASE, (float*)&m_World44, MATRIX_SIZE);
#endif
}

void grcViewport::RegenerateScreen() {
	// TODO: Add an assertion here to make sure this doesn't get called outside of the render thread.
	if (m_LastWidth != GRCDEVICE.GetWidth() || m_LastHeight != GRCDEVICE.GetHeight()) {
		OwnerCheck();
		int x = m_Window.GetX();
		int y = m_Window.GetY();
		int width = m_Window.GetWidth();
		int height = m_Window.GetHeight();
		m_Screen.SetCol0(Vec4V((float)(width>>1),0.0f,0.0f,0.0f));
		m_Screen.SetCol1(Vec4V(0.0f,(float)(-height>>1),0.0f,0.0f));
		m_Screen.SetCol3(Vec4V((float)(x + (width>>1)),(float)(y + (height>>1)),m_Window.GetMinZ(),1.0f));
		m_LastWidth = GRCDEVICE.GetWidth();
		m_LastHeight = GRCDEVICE.GetHeight();
	}
}
void grcViewport::RegenerateMatrices() {
	OwnerCheck();

	Mat44V worldTemp = m_World44;
	worldTemp.SetCol3(worldTemp.GetCol3() - Vec4V(1.0f, 1.0f, 1.0f, 0.0f)*m_Camera44.GetCol3());
	Mat44V viewTemp = m_View;
	viewTemp.SetCol3(Vec4V(V_ZERO_WONE));
	
	//Multiply(m_ModelView,m_View,m_World44);
	Multiply(m_ModelView,viewTemp,worldTemp);
	Multiply(m_Composite,m_Projection,m_ModelView);
	Multiply(m_FullComposite,m_Screen,m_Composite);
	Multiply(m_LocalLRTB,m_FrustumLRTB,m_World44);
	
#if NV_SUPPORT
	if(GRCDEVICE.CanUseStereo() && GRCDEVICE.IsStereoEnabled())
	{
		Mat44V tmpProj = m_Projection;
		ModifyProjectionForStereo(tmpProj);

		Multiply(m_CullComposite, tmpProj, m_ModelView);
		Multiply(m_CullLocalLRTB, m_CullFrustumLRTB, m_World44);
	}
	else
	{
		m_CullComposite = m_Composite;
		m_CullLocalLRTB = m_LocalLRTB;
	}
	
#endif

	if (sm_Current == this)
		RegenerateDevice();
}

void grcViewport::ResetWindow() {
	OwnerCheck();
	m_LastWidth = m_LastHeight = 0;
	RegenerateScreen();
	SetWindow(0,0,m_LastWidth,m_LastHeight);
}

void grcViewport::SetNormWindow(float x,float y,float w,float h,float zmin,float zmax) {
	OwnerCheck();
	const float roundingBias = 0.5f;
	SetWindow(
		int(roundingBias + x*(float)m_LastWidth),
		int(roundingBias + y*(float)m_LastHeight),
		int(roundingBias + w*(float)m_LastWidth),
		int(roundingBias + h*(float)m_LastHeight),
		zmin,
		zmax
	);
}

void grcViewport::SetNormWindow_GRCDEVICE(float x,float y,float w,float h,float zmin,float zmax) {
	grcWindow window;

	window.m_NormX      = x;
	window.m_NormY      = y;
	window.m_NormWidth  = w;
	window.m_NormHeight = h;
	window.m_MinZ       = zmin;
	window.m_MaxZ       = zmax;

	GRCDEVICE.SetWindow(window);
}

void grcViewport::SetNormWindowPortal_UpdateThreadOnly(float x, float y, float w, float h, float windowX, float windowY, float windowW, float windowH) {
	AssertMsg(sysThreadType::IsUpdateThread(), "grcViewport::SetNormWindowPortal called from thread other than update thread");
	AssertMsg(IsPerspective(), "grcViewport::SetNormWindowPortal called on non-perspective viewport");

	m_Window.m_NormX      = windowX;
	m_Window.m_NormY      = windowY;
	m_Window.m_NormWidth  = windowW;
	m_Window.m_NormHeight = windowH;
	m_Window.m_MinZ       = 0.0f;
	m_Window.m_MaxZ       = 1.0f;

	m_UnclippedWindow.m_NormX      = 0.0f;
	m_UnclippedWindow.m_NormY      = 0.0f;
	m_UnclippedWindow.m_NormWidth  = 1.0f;
	m_UnclippedWindow.m_NormHeight = 1.0f;
	m_UnclippedWindow.m_MinZ       = 0.0f;
	m_UnclippedWindow.m_MaxZ       = 1.0f;

	m_Screen.SetCol0f(0.5f, 0.0f, 0.0f, 0.0f);
	m_Screen.SetCol1f(0.0f,-0.5f, 0.0f, 0.0f);
	m_Screen.SetCol2f(0.0f, 0.0f, 1.0f, 0.0f);
	m_Screen.SetCol3f(0.5f, 0.5f, 0.0f, 1.0f);

	const float tanHFOV = w*GetTanHFOV();
	const float tanVFOV = h*GetTanVFOV();
	const float aspect  = tanHFOV/tanVFOV;

	m_TanHFOV        = tanHFOV;
	m_TanVFOV        = tanVFOV;
	m_FovY           = 2.0f*RtoD*atanf(tanVFOV);
	m_DesiredAspect  = aspect;
	m_ComputedAspect = aspect;
	m_ScaleX         = 1.0f;
	m_ScaleY         = 1.0f;
	m_ShearX         = -(x*2.0f - 1.0f)/w;
	m_ShearY         = +(y*2.0f - 1.0f)/h;

	RegeneratePerspective();
}

void grcViewport::SetWindow(int x,int y,int width,int height,float minz,float maxz) {
    const int origX = x, origY = y, origW = width, origH = height;

	//AssertMsg(GRCDEVICE.CheckThreadOwnership(),"Thread that doesn't own the GPU trying to set window");

	// handle partially offscreen windows semi-gracefully
	if (x < 0) {
		width += x;
		x = 0;
	}
	if (x + width > m_LastWidth)
		width = m_LastWidth - x;
	if (y < 0) {
		height += y;
		y = 0;
	}
	if (y + height > m_LastHeight)
		height = m_LastHeight - y;

	if( width < 0 )
	{
		width = 0;
	}

	if( height < 0 )
	{
		height = 0;
	}

    m_Window.m_NormX = float(x) / m_LastWidth; 
	m_Window.m_NormY = float(y) / m_LastHeight; 
	m_Window.m_NormWidth = float(width) / m_LastWidth; 
	m_Window.m_NormHeight = float(height) / m_LastHeight;
	m_Window.m_MinZ = minz;
	m_Window.m_MaxZ = maxz; 

    m_UnclippedWindow.m_NormX = float(origX) / m_LastWidth; 
	m_UnclippedWindow.m_NormY = float(origY) / m_LastHeight; 
	m_UnclippedWindow.m_NormWidth = float(origW) / m_LastWidth; 
	m_UnclippedWindow.m_NormHeight = float(origH) / m_LastHeight;
	m_UnclippedWindow.m_MinZ = minz;
	m_UnclippedWindow.m_MaxZ = maxz; 

	RegenerateAspect();

	// protect against divide by 0
	//Assert(height);
	// m_AspectRatio = 1.0f /*GRCDEVICE.GetPixelAspect()*/ * ((float)width / (float)height);

	m_Screen.SetCol0f((float)(width>>1),0.0f,0.0f,0.0f);
	m_Screen.SetCol1f(0.0f,(float)(-height>>1),0.0f,0.0f);

	// Z is from 0..1 on D3D
	m_Screen.SetCol2f(0.0f,0.0f,maxz - minz,0.0f);
	m_Screen.SetCol3f((float)(x + (width>>1)),(float)(y + (height>>1)),minz,1.0f);

    //If the viewport was clipped to the device, set the scale to compensate
    //for the reduced viewport size.  This maintains proper image proportions.
    //Because this scale is applied uniformly about the center of the viewport,
    //the scaled image will spill over the edges of the clipped viewport.
    //We add a shear to "shift" the image so it lines back up with the viewport.

    if( 0 != width )
    {
        const float ooW = 1.0f / width;

        m_ShearX = ooW * (origW - width);
        m_ScaleX = ooW * origW;
    }
    else
    {
        m_ShearX = 0;
        m_ScaleX = 1;
    }

    if( 0 != height )
    {
        const float ooH = 1.0f / height;

        m_ShearY = ooH * (origH - height);
        m_ScaleY = ooH * origH;
    }
    else
    {
        m_ShearY = 0;
        m_ScaleY = 1;
    }

	if( origX > 0 )
	{
		m_ShearX = -m_ShearX;
	}

	if( origY < 0 )
	{
		m_ShearY = -m_ShearY;
	}

	Multiply(m_FullComposite,m_Screen,m_Composite);

	if (sm_Current == this)
		GRCDEVICE.SetWindow(m_Window);
}

void grcViewport::SetWindowMinMaxZ(float zmin,float zmax) {
	OwnerCheck();
	m_Window.m_MinZ = zmin;
	m_Window.m_MaxZ = zmax; 

	m_UnclippedWindow.m_MinZ = zmin;
	m_UnclippedWindow.m_MaxZ = zmax; 

	// Z is from 0..1 on D3D
	m_Screen.SetM22(zmax - zmin);
	m_Screen.SetM23(zmin);

	Multiply(m_FullComposite,m_Screen,m_Composite);

	if (sm_Current == this)
		GRCDEVICE.SetWindow(m_Window);
}

#if SUPPORT_INVERTED_PROJECTION
void grcViewport::SetInvertZInProjectionMatrix(bool set, bool regen /*= true*/) {
	m_InvertZInProjectionMatrix = set;

	if (regen)
	{
		RegeneratePerspective();
	}
}
#endif // SUPPORT_INVERTED_PROJECTION

// fovy is in DEGREES
void grcViewport::Perspective(float fovyDegrees,float aspect,float znear,float zfar) {
	OwnerCheck();

	m_FovY = fovyDegrees;
	m_DesiredAspect = aspect;
	m_ZClipNear = znear;
	m_ZClipFar = zfar;

	RegenerateAspect();

	// turn in to radians, and get half angle
	float tanFovy = tanf(m_FovY * 0.5f * DtoR);

	m_TanVFOV = tanFovy;
	m_TanHFOV = tanFovy * aspect;
	
	m_IsPerspective = true;
	RegeneratePerspective();
}


#if __PS3
float g_PixelAspect = 1.0f;
#endif

void grcViewport::RegenerateAspect() {
	if (m_DesiredAspect != 0.0f)
		m_ComputedAspect = m_DesiredAspect;
	else if (m_UnclippedWindow.GetNormHeight() != 0.0f)
		m_ComputedAspect = ( (m_UnclippedWindow.GetNormWidth()*(float)m_LastWidth) / (m_UnclippedWindow.GetNormHeight()*(float)m_LastHeight) );
	else
		m_ComputedAspect = 1.0f;
}

void grcViewport::RegeneratePerspective() {
	Assert(m_ZClipNear!=0.f && m_TanVFOV!=0.f);

	float aspect = m_ComputedAspect PS3_ONLY(* g_PixelAspect);
	float znear = m_ZClipNear;
	float zfar = m_ZClipFar;

	float h = 1.0f / m_TanVFOV;
    float w = h / aspect;

	// Rage uses a right-handed projection matrix.
	// This is identical to the matrix produced by D3DXMatrixPerspectiveFovRH:
    
	float zNorm = zfar / (znear - zfar);
	float zNearNorm = znear*zNorm;

#if SUPPORT_DEPTH_BIAS_IN_PROJECTION
	zNorm += m_NDCDepthBias;
#endif

#if SUPPORT_INVERTED_PROJECTION
	if ( GetInvertZInProjectionMatrix() )
	{
		zNearNorm = -zNearNorm; 
		zNorm = -zNorm - 1.0f;
	}
#endif // SUPPORT_INVERTED_PROJECTION

	m_Projection.SetCol0f(w*m_ScaleX,	0,			0,			0);
	m_Projection.SetCol1f(0,			h*m_ScaleY,	0,			0);
	m_Projection.SetCol2f(m_ShearX,		m_ShearY,	zNorm,		-1.0f);
	m_Projection.SetCol3f(0,			0,			zNearNorm,	0);

	RegenerateFrustumPlanes();
#if NV_SUPPORT
	RegenerateStereoFrustumPlanes();
#endif
	RegenerateMatrices();
}

void grcViewport::Ortho(float left,float right,float bottom,float top,float znear,float zfar,bool setFOVToZero) {
	OwnerCheck();

	AssertMsg(right != left && top != bottom && znear != zfar,"Invalid ortho params");

	float oorl = 1.0f / (right - left);
	float ootb = 1.0f / (top - bottom);
	float oolr = 1.0f / (left - right);
	float oobt = 1.0f / (bottom - top);

	m_ZClipNear = znear;
	m_ZClipFar = zfar;
	m_OrthoLRTB = Vec4V(left, right, top, bottom);

	float cz, dz;

	// D3DXMatrixOrthoOffCenterRH
	cz = 1 / (znear-zfar);
	dz = znear / (znear - zfar);

	//
	// taken from the D3D documentation 
	//
	//					2/(r-l)      			0            			0           0
	//					0            			2/(t-b)      			0           0
	//					0            			0            			1/(zn-zf)   0
	//					(l+r)/(l-r)  			(t+b)/(b-t)  			zn/(zn-zf)  l
	//
	// shearing sets it off in the x direction and / or in the y direction
	// scaling makes it bigger in the x or y direction
	//
	m_Projection.SetCol0f(2 * oorl * m_ScaleX,	0.0f,					0.0f,		0.0f);
	m_Projection.SetCol1f(0.0f,					2 * ootb * m_ScaleY,	0.0f,		0.0f);
	m_Projection.SetCol2f(m_ShearX,				m_ShearY,				cz,			0.0f);
	m_Projection.SetCol3f((left + right) * oolr,(top + bottom) * oobt,	dz,			1.0f);

	m_IsPerspective = false;

	if (setFOVToZero)
	{
		m_FovY = 0.0f;
		m_TanHFOV = 0.0f;
		m_TanVFOV = 0.0f;
	}

	RegenerateFrustumPlanes();
	NV_SUPPORT_ONLY(RegenerateStereoFrustumPlanes();)
	RegenerateMatrices();
}

void grcViewport::SetProjection(Mat44V_In projMtx) {
	m_IsPerspective = true;
	m_Projection = projMtx;

	RegenerateFrustumPlanes();
	NV_SUPPORT_ONLY(RegenerateStereoFrustumPlanes();)
	RegenerateMatrices();
}

void grcViewport::ResetPerspective() {
	m_IsPerspective = true;

	RegeneratePerspective();
}

void grcViewport::RegenerateNearFarClip() {
	float znear = m_ZClipNear;
	float zfar  = m_ZClipFar;
	float znorm = 1.0f / (znear - zfar);
	if (Likely(m_IsPerspective))
		znorm *= zfar;
	float znearnorm = znear * znorm;

#if SUPPORT_DEPTH_BIAS_IN_PROJECTION
	znorm += m_NDCDepthBias;
#endif

#if SUPPORT_INVERTED_PROJECTION
	if ( GetInvertZInProjectionMatrix() )
	{
		znearnorm = -znearnorm; 
		znorm = -znorm - 1.0f;
	}
#endif // SUPPORT_INVERTED_PROJECTION

	m_Projection.SetM22f(znorm);			// c.z
	m_Projection.SetM23f(znearnorm);	// d.z

	Multiply(m_Composite,m_Projection,m_ModelView);
	Multiply(m_FullComposite,m_Screen,m_Composite);

#if NV_SUPPORT
	if(GRCDEVICE.CanUseStereo() && GRCDEVICE.IsStereoEnabled())
	{
		Mat44V tmpProj = m_Projection;
		ModifyProjectionForStereo(tmpProj);
		Multiply(m_CullComposite, tmpProj, m_ModelView);
	}
	else
	{
		m_CullComposite = m_Composite;
	}
#endif

	if (sm_Current == this)
		RegenerateDevice();

	RegenerateFrustumPlanesNearFar();
}

void grcViewport::SetNearClip(float zNear) {
	m_ZClipNear = zNear;

	RegenerateNearFarClip();
}

void grcViewport::SetFarClip(float zFar) {
	m_ZClipFar = zFar;

	RegenerateNearFarClip();
}

void grcViewport::SetNearFarClip(float zNear, float zFar) {
	m_ZClipNear = zNear;
	m_ZClipFar = zFar;

	RegenerateNearFarClip();
}

namespace GraphicsStats {
	EXT_PF_COUNTER(grcViewport_SetWorldMtx_Calls);
}
using namespace GraphicsStats;

// utility function
static inline void ConvertMat34ToMat44(Mat44V_InOut dest,Mat34V_In src) {
	dest.SetCols(
		Vec4V(src.GetCol0(),ScalarV(V_ZERO)),
		Vec4V(src.GetCol1(),ScalarV(V_ZERO)),
		Vec4V(src.GetCol2(),ScalarV(V_ZERO)),
		Vec4V(src.GetCol3(),ScalarV(V_ONE)));
}

void grcViewport::SetWorldMtx(Mat34V_In worldMtx) {
	OwnerCheck();

	PF_INCREMENT(grcViewport_SetWorldMtx_Calls);

	ConvertMat34ToMat44(m_World44,worldMtx);

	RegenerateMatrices();
}

void grcViewport::SetWorldMtx(Mat44V_In worldMtx) {
	PF_INCREMENT(grcViewport_SetWorldMtx_Calls);

	m_World44 = worldMtx;

	RegenerateMatrices();
}

void grcViewport::SetWorldIdentity() {
	OwnerCheck();

	PF_INCREMENT(grcViewport_SetWorldMtx_Calls);

	m_World44 = Mat44V(V_IDENTITY);
	m_ModelView = m_View;
	Multiply(m_Composite,m_Projection,m_ModelView);
	Multiply(m_FullComposite,m_Screen,m_Composite);
	m_LocalLRTB = m_FrustumLRTB;

#if NV_SUPPORT	 
	if(GRCDEVICE.CanUseStereo() && GRCDEVICE.IsStereoEnabled())
	{
		Mat44V tmpProj = m_Projection;
		ModifyProjectionForStereo(tmpProj);
		Multiply(m_CullComposite,tmpProj,m_ModelView);
		m_CullLocalLRTB = m_CullFrustumLRTB;
	}
	else
	{
		m_CullComposite = m_Composite;
		m_CullLocalLRTB = m_CullFrustumLRTB = m_LocalLRTB;
	}
#endif

	if (sm_Current == this)
		RegenerateDevice();
}

#if RAGE_INSTANCED_TECH
void grcViewport::SetInstancing(bool bEnable)
{
	sm_bInstancing = bEnable;
	sm_viewportInstancedRenderBit = 0xff;

	if (bEnable)
	{
		m_SetCurrentWorldMtxFn = &(grcViewport::SetCurrentWorldMtxFn_Instanced);
		m_SetCurrentWorldIdentityFn = &(grcViewport::SetCurrentWorldIdentityFn_Instanced);
		m_SetCurrentNearFarClipFn = &(grcViewport::SetCurrentNearFarClipFn_Instanced);
		m_SetCurrentWindowMinMaxZFn = &(grcViewport::SetCurrentWindowMinMaxZFn_Instanced);
	}
	else
	{
		m_SetCurrentWorldMtxFn = &(grcViewport::SetCurrentWorldMtxFn_NonInstanced);
		m_SetCurrentWorldIdentityFn = &(grcViewport::SetCurrentWorldIdentityFn_NonInstanced);
		m_SetCurrentNearFarClipFn = &(grcViewport::SetCurrentNearFarClipFn_NonInstanced);
		m_SetCurrentWindowMinMaxZFn = &(grcViewport::SetCurrentWindowMinMaxZFn_NonInstanced);
	}

	if (sm_bInstancing)
	{
		grmModel::SetModelCullback( grmModel::ModelVisibleWithAABB_Instanced );
	}
	else
	{
		grmModel::SetModelCullback( grmModel::ModelVisibleWithAABB );
	}

#if RSG_ORBIS
	// enabling viewport scissor since we are doing viewport instancing
	GRCDEVICE.SetViewportScissor(bEnable);
#endif
}

void grcViewport::RegenerateInstVPMatrices(const Mat44V& worldmat, int numInst)
{
	struct InstanceData
	{
		Mat44V mInstWorld;
		u32 uInstParam[8];
	};

	InstanceData InstData;
	sm_uVPInstCount = 0;

	for (u32 i = 0; i < sm_uNumInstVP; i++)
	{
		u32 r = sm_viewportInstancedRenderBit & (1 << i);
		InstData.uInstParam[i] = i;

		if (r != 0)
		{
			sm_InstViewport[g_RenderThreadIndex][i]->RegenerateLRTB(worldmat);
			InstData.uInstParam[sm_uVPInstCount] = i;
			sm_uVPInstCount++;
		}
	}

	InstData.mInstWorld = worldmat;

	InstData.uInstParam[6] = numInst;

	// setting up 4x4 float matrices and then float start index value
	unsigned size = (4 * 4) * sizeof(float) + 8 * sizeof(u32);
	Assert(size == g_InstUpdateBase->GetSize());
#if RSG_DURANGO || RSG_ORBIS
#if MULTIPLE_RENDER_THREADS
	if (!g_IsSubRenderThread) {
		for (int i=0; i<NUMBER_OF_RENDER_THREADS; i++)
			memcpy((char*)g_InstUpdateBase->GetDataPtrForThread(i),(float*)&InstData.mInstWorld,size);
		g_InstUpdateBase->SetDirtyAllThreads();
	}
	else
#endif
	{
		memcpy((char*)g_InstUpdateBase->GetDataPtr(),(float*)&InstData.mInstWorld,size);
		g_InstUpdateBase->SetDirtySingleThread();
	}
#else
	sysMemCpy(g_InstUpdateBase->BeginUpdate(size),(float*)&InstData.mInstWorld, size);
	g_InstUpdateBase->EndUpdate();
#endif
}

grcCullStatus grcViewport::GetInstAABBCullStatus(const Vector3 &aabb_min, const Vector3 &aabb_max )
{
	//Assert(sm_bInstancing == true);

	grcCullStatus rStat = cullOutside;

	for (u32 i = 0; i < sm_uNumInstVP; i++)
	{
		u32 r = 1;

		r = sm_viewportInstancedRenderBit & (1 << i);

		if (r != 0)
		{
			grcCullStatus stat = grcViewport::GetAABBCullStatusInline(aabb_min,aabb_max,sm_InstViewport[g_RenderThreadIndex][i]->GetLocalLRTB());
			if (stat != cullOutside)
				rStat = stat;
		}
	}

	return rStat;
}
void grcViewport::SetCurrentInstVP(const grcViewport *viewport, Vec4V_In window, int index, bool bDefault )
{
	sm_InstViewport[g_RenderThreadIndex][index] = const_cast<grcViewport*>(viewport);

	const Vec4V temp = FloatToIntRaw<0>(window);
	const int x = temp.GetXi();
	const int y = temp.GetYi();
	const int w = temp.GetZi();
	const int h = temp.GetWi();

	sm_InstViewport[g_RenderThreadIndex][index]->ResetWindow();
	sm_InstViewport[g_RenderThreadIndex][index]->SetWindow(x, y, w, h);

	if (bDefault)
		sm_Current = sm_InstViewport[g_RenderThreadIndex][index];
}

// SetCurrentInstVP must be called previously
void grcViewport::SetInstVPWindow(u32 uNumInstVP)
{
	sm_uNumInstVP = uNumInstVP;

	Assert(uNumInstVP > 0 && sm_InstViewport[g_RenderThreadIndex][0] != NULL);

	const grcWindow *InstVPs[MAX_NUM_CBUFFER_INSTANCING];

	for (u32 i = 0; i < sm_uNumInstVP; i++)
	{
		InstVPs[i] = &(sm_InstViewport[g_RenderThreadIndex][i]->GetWindow());
	}
	
	GRCDEVICE.SetMultiWindow(InstVPs, sm_uNumInstVP);

	SetInstMatrices();
}

void grcViewport::SetInstMatrices()
{
	struct InstanceData
	{
		Mat44V mInstWorldViewProj[MAX_NUM_CBUFFER_INSTANCING];
		Mat44V mInstViewInverse[MAX_NUM_CBUFFER_INSTANCING];
	};

	InstanceData InstData;
	for (u32 i = 0; i < sm_uNumInstVP; i++)
	{
		sm_InstViewport[g_RenderThreadIndex][i]->SetWorldMtx(Mat44V(V_IDENTITY));

		InstData.mInstWorldViewProj[i] = sm_InstViewport[g_RenderThreadIndex][i]->GetCompositeMtx();
		InstData.mInstViewInverse[i] = sm_InstViewport[g_RenderThreadIndex][i]->GetCameraMtx44();
	}

	// setting up 4x4 float matrices and then float start index value
	int size = (MAX_NUM_CBUFFER_INSTANCING * 4 * 4 * 2) * sizeof(float);
#if RSG_DURANGO || RSG_ORBIS
#if MULTIPLE_RENDER_THREADS
	if (!g_IsSubRenderThread) {
		for (int i=0; i<NUMBER_OF_RENDER_THREADS; i++)
			memcpy((char*)g_InstBase->GetDataPtrForThread(i),(float*)&InstData.mInstWorldViewProj[0],size);
		g_InstBase->SetDirtyAllThreads();
	}
	else
#endif
	{
		memcpy((char*)g_InstBase->GetDataPtr(),(float*)&InstData.mInstWorldViewProj[0],size);
		g_InstBase->SetDirtySingleThread();
	}
#else
	sysMemCpy(g_InstBase->BeginUpdate(size),(float*)&InstData.mInstWorldViewProj[0], size);
	g_InstBase->EndUpdate();
#endif
}

void grcViewport::SetCurrentWorldMtxFn_NonInstanced(Mat34V_In worldMtx) {
	sm_Current->SetWorldMtx(worldMtx);
}

void grcViewport::SetCurrentWorldMtxFn_Instanced(Mat34V_In worldMtx) {
	Mat44V world44;
	ConvertMat34ToMat44(world44, worldMtx);

	RegenerateInstVPMatrices(world44);
}

void grcViewport::SetCurrentWorldIdentityFn_NonInstanced()
{
	GetCurrent()->SetWorldIdentity(); 
}

void grcViewport::SetCurrentWorldIdentityFn_Instanced()
{
	RegenerateInstVPMatrices(Mat44V(V_IDENTITY));
}

void grcViewport::SetCurrentNearFarClipFn_Instanced(float fNear, float fFar)
{
	for (u32 i = 0; i < sm_uNumInstVP; i++)
	{
		sm_InstViewport[g_RenderThreadIndex][i]->SetNearFarClip(fNear,fFar);
	}
}

void grcViewport::SetCurrentNearFarClipFn_NonInstanced(float fNear, float fFar)
{
	GetCurrent()->SetNearFarClip(fNear,fFar);
}

void grcViewport::SetCurrentWindowMinMaxZFn_Instanced(float minz, float maxz)
{
	for (u32 i = 0; i < sm_uNumInstVP; i++)
	{
		sm_InstViewport[g_RenderThreadIndex][i]->SetWindowMinMaxZ(minz, maxz);
	}
}

void grcViewport::SetCurrentWindowMinMaxZFn_NonInstanced(float minz, float maxz)
{
	GetCurrent()->SetWindowMinMaxZ(minz, maxz);
}
#endif // RAGE_INSTANCED_TECH

void grcViewport::SetCurrentWorldIdentity()
{ 
#if RAGE_INSTANCED_TECH
	m_SetCurrentWorldIdentityFn();
#else
	GetCurrent()->SetWorldIdentity(); 
#endif
}

void grcViewport::SetCurrentWorldMtx(Mat34V_In worldMtx) {
#if RSG_PC && __D3D11
	Mat44V matTemp;
	ConvertMat34ToMat44(matTemp,worldMtx);

	if (IsEqualAll(matTemp, sm_Current->m_World44))
		return;
#endif

#if RAGE_INSTANCED_TECH
	m_SetCurrentWorldMtxFn(worldMtx);
#else
	sm_Current->SetWorldMtx(worldMtx);
#endif
}

void grcViewport::SetCurrentNearFarClip(float fNear, float fFar)
{
#if GS_INSTANCED_CUBEMAP
	m_SetCurrentNearFarClipFn(fNear,fFar);
#else
	GetCurrent()->SetNearFarClip(fNear,fFar);
#endif
}

void grcViewport::SetCurrentWindowMinMaxZ(float minz, float maxz)
{
#if GS_INSTANCED_CUBEMAP
	m_SetCurrentWindowMinMaxZFn(minz,maxz);
#else
	GetCurrent()->SetWindowMinMaxZ(minz,maxz);
#endif
}

void grcViewport::SetCameraMtx(Mat34V_In camMtx) {
	ConvertMat34ToMat44(m_Camera44,camMtx);

	Mat34V temp;
	InvertTransformOrtho(temp,camMtx);
	ConvertMat34ToMat44(m_View,temp);

	RegenerateFrustumPlanes();
	NV_SUPPORT_ONLY(RegenerateStereoFrustumPlanes();)
	RegenerateMatrices();
}

void grcViewport::SetCameraIdentity() {
	m_Camera44 = Mat44V(V_IDENTITY);
	m_View = Mat44V(V_IDENTITY);

	RegenerateFrustumPlanes();
	NV_SUPPORT_ONLY(RegenerateStereoFrustumPlanes();)
	RegenerateMatrices();
}

// utility function
static inline Vec4V_Out NormalizePlane(Vec4V_In plane) {
	return plane * InvMag(plane.GetXYZ());
}

#if NV_SUPPORT
void grcViewport::ModifyProjectionForStereo(Mat44V_InOut proj)
{
	if( (!m_IsPerspective || !GRCDEVICE.CanUseStereo() || !GRCDEVICE.IsStereoEnabled())) return;

	float fEyeSep = GRCDEVICE.GetCachedEyeSeparation();
	float fSep = GRCDEVICE.GetDefaultSeparationPercentage();

	float separation = fEyeSep * fSep * 0.01f;
		
	proj.SetM00f(proj.GetM00f() * (1.0f - separation));
}

void grcViewport::RegenerateStereoFrustumPlanes()
{
	if (!(GRCDEVICE.CanUseStereo() && GRCDEVICE.IsStereoEnabled()))
	{
		m_CullFrustumLRTB = m_FrustumLRTB;
		m_CullLocalLRTB = m_LocalLRTB;
		return;
	}
		

	Mat44V viewProj;
	Mat44V tmpProj = m_Projection;	
	ModifyProjectionForStereo(tmpProj);

#if SUPPORT_INVERTED_PROJECTION
	if ( GetInvertZInProjectionMatrix() )
	{
		Mat44V tmpProjection = tmpProj;	
		tmpProjection.SetM22f( -(tmpProjection.GetM22f() + 1.0f) );
		tmpProjection.SetM23f( -tmpProjection.GetM23f() );

		Multiply(viewProj,tmpProjection,m_View);
	}
	else
#endif  // SUPPORT_INVERTED_PROJECTION
	{
		Multiply(viewProj,tmpProj,m_View);
	}


	const ScalarV p1(V_ONE);
	const ScalarV n1(V_NEGONE);
	const Vec4V p1n1n1p1 = Vec4V(p1, n1, n1, p1);

	const Vec4V col0 = viewProj.GetCol0();
	const Vec4V col1 = viewProj.GetCol1();
	const Vec4V col2 = viewProj.GetCol2();
	const Vec4V col3 = viewProj.GetCol3();

	Vec4V tmp0 = AddScaled(Vec4V(SplatW(col0)), MergeXY(col0, col0), p1n1n1p1);
	Vec4V tmp1 = AddScaled(Vec4V(SplatW(col1)), MergeXY(col1, col1), p1n1n1p1);
	Vec4V tmp2 = AddScaled(Vec4V(SplatW(col2)), MergeXY(col2, col2), p1n1n1p1);
	Vec4V tmp3 = AddScaled(Vec4V(SplatW(col3)), MergeXY(col3, col3), p1n1n1p1);

	Vec4V q = InvSqrt(AddScaled(AddScaled(Scale(tmp0, tmp0), tmp1, tmp1), tmp2, tmp2));

	m_CullFrustumLRTB.SetCols(tmp0*q, tmp1*q, tmp2*q, tmp3*q);

	m_CullFrustumClipPlanes[CLIP_PLANE_LEFT] = Vec4V(m_CullFrustumLRTB.GetM00f(), m_CullFrustumLRTB.GetM01f(), m_CullFrustumLRTB.GetM02f(), m_CullFrustumLRTB.GetM03f());
	m_CullFrustumClipPlanes[CLIP_PLANE_RIGHT] = Vec4V(m_CullFrustumLRTB.GetM10f(), m_CullFrustumLRTB.GetM11f(), m_CullFrustumLRTB.GetM12f(), m_CullFrustumLRTB.GetM13f());
}
#endif

void grcViewport::RegenerateFrustumPlanes() {
	// Based on http://www2.ravensoft.com/users/ggribb/plane%20extraction.pdf 
	// (Gil Gribb and Klaus Hartmann)
	Mat44V viewProj;

#if SUPPORT_INVERTED_PROJECTION
	if ( GetInvertZInProjectionMatrix() )
	{
		Mat44V tmpProjection = m_Projection;	
		tmpProjection.SetM22f( -(tmpProjection.GetM22f() + 1.0f) DEPTH_BIAS_IN_PROJECTION_ONLY( - m_NDCDepthBias ) );
		tmpProjection.SetM23f( -tmpProjection.GetM23f() );

		Multiply(viewProj,tmpProjection,m_View);
	}
	else
#endif  // SUPPORT_INVERTED_PROJECTION
	{
#if SUPPORT_DEPTH_BIAS_IN_PROJECTION
		Mat44V tmpProjection = m_Projection;	
		tmpProjection.SetM22f( tmpProjection.GetM22f() - m_NDCDepthBias );
		Multiply(viewProj,tmpProjection,m_View);
#else
		Multiply(viewProj,m_Projection,m_View);
#endif // SUPPORT_DEPTH_BIAS_IN_PROJECTION
	}

	const ScalarV p1(V_ONE);
	const ScalarV n1(V_NEGONE);
	const Vec4V p1n1n1p1 = Vec4V(p1, n1, n1, p1);

	const Vec4V col0 = viewProj.GetCol0();
	const Vec4V col1 = viewProj.GetCol1();
	const Vec4V col2 = viewProj.GetCol2();
	const Vec4V col3 = viewProj.GetCol3();

	Vec4V tmp0 = AddScaled(Vec4V(SplatW(col0)), MergeXY(col0, col0), p1n1n1p1);
	Vec4V tmp1 = AddScaled(Vec4V(SplatW(col1)), MergeXY(col1, col1), p1n1n1p1);
	Vec4V tmp2 = AddScaled(Vec4V(SplatW(col2)), MergeXY(col2, col2), p1n1n1p1);
	Vec4V tmp3 = AddScaled(Vec4V(SplatW(col3)), MergeXY(col3, col3), p1n1n1p1);

	Vec4V q = InvSqrt(AddScaled(AddScaled(Scale(tmp0, tmp0), tmp1, tmp1), tmp2, tmp2));

	m_FrustumLRTB.SetCols(tmp0*q, tmp1*q, tmp2*q, tmp3*q);

	Transpose(*(Mat44V*)&m_FrustumClipPlanes[CLIP_PLANE_LEFT], m_FrustumLRTB);

#if 0&&__DEV
	// test old code
	{
		Mat44V comboMatrix;
		Transpose(comboMatrix,viewProj);

		// Normalize all of the plane equations.
		// This is necessary so that our radius check is accurate.

		Vec4V clipPlanes[4];
		clipPlanes[CLIP_PLANE_LEFT] = NormalizePlane(comboMatrix.d() + comboMatrix.a());
		clipPlanes[CLIP_PLANE_RIGHT] = NormalizePlane(comboMatrix.d() - comboMatrix.a());
		clipPlanes[CLIP_PLANE_TOP] = NormalizePlane(comboMatrix.d() - comboMatrix.b());
		clipPlanes[CLIP_PLANE_BOTTOM] = NormalizePlane(comboMatrix.d() + comboMatrix.b());

		Mat44V planeSet;
		planeSet.SetCols(
			clipPlanes[CLIP_PLANE_LEFT],
			clipPlanes[CLIP_PLANE_RIGHT],
			clipPlanes[CLIP_PLANE_TOP],
			clipPlanes[CLIP_PLANE_BOTTOM]);

		Mat44V test;
		Transpose(test,planeSet);

		Vec4V err1 = Max(
			Abs(clipPlanes[0] - m_FrustumClipPlanes[0]),
			Abs(clipPlanes[1] - m_FrustumClipPlanes[1]),
			Abs(clipPlanes[2] - m_FrustumClipPlanes[2]),
			Abs(clipPlanes[3] - m_FrustumClipPlanes[3])
		);
		Vec4V err2 = Max(
			Abs(test.GetCol0() - m_FrustumLRTB.GetCol0()),
			Abs(test.GetCol1() - m_FrustumLRTB.GetCol1()),
			Abs(test.GetCol2() - m_FrustumLRTB.GetCol2()),
			Abs(test.GetCol3() - m_FrustumLRTB.GetCol3())
		);

		if (MaxElement(err1).Getf() > 0.01f ||
			MaxElement(err2).Getf() > 0.01f)
		{
			Assertf(0, "bad math!");
			Displayf("m_FrustumClipPlanes[0] = %f,%f,%f,%f", VEC4V_ARGS(m_FrustumClipPlanes[0]));
			Displayf("clipPlanes[0]          = %f,%f,%f,%f", VEC4V_ARGS(clipPlanes[0]));
			Displayf("m_FrustumClipPlanes[1] = %f,%f,%f,%f", VEC4V_ARGS(m_FrustumClipPlanes[1]));
			Displayf("clipPlanes[1]          = %f,%f,%f,%f", VEC4V_ARGS(clipPlanes[1]));
			Displayf("m_FrustumClipPlanes[2] = %f,%f,%f,%f", VEC4V_ARGS(m_FrustumClipPlanes[2]));
			Displayf("clipPlanes[2]          = %f,%f,%f,%f", VEC4V_ARGS(clipPlanes[2]));
			Displayf("m_FrustumClipPlanes[3] = %f,%f,%f,%f", VEC4V_ARGS(m_FrustumClipPlanes[3]));
			Displayf("clipPlanes[3]          = %f,%f,%f,%f", VEC4V_ARGS(clipPlanes[3]));
		}
	}
#endif // __DEV

	Mat44V comboMatrix;
	Transpose(comboMatrix,viewProj);

#if __OPENGL
	m_FrustumClipPlanes[CLIP_PLANE_NEAR] = NormalizePlane(comboMatrix.d() + comboMatrix.c());
#else
	m_FrustumClipPlanes[CLIP_PLANE_NEAR] = NormalizePlane(comboMatrix.c());
#endif
	m_FrustumClipPlanes[CLIP_PLANE_FAR] = NormalizePlane(comboMatrix.d() - comboMatrix.c());

	// We can add two more cull planes here for free. (replace the replicated planes at the end)
	// CLIP_PLANE_NEAR must be in .x channel so we can get outZ
	// NOTE: replicate a valid plane in the w slots, so the inside clip status is correct
	Mat44V planeSet;
	planeSet.SetCols(
		m_FrustumClipPlanes[CLIP_PLANE_NEAR],
		m_FrustumClipPlanes[CLIP_PLANE_FAR],
		m_FrustumClipPlanes[CLIP_PLANE_NEAR],
		m_FrustumClipPlanes[CLIP_PLANE_FAR]);
	Transpose(m_FrustumNFNF,planeSet);
}

void grcViewport::RegenerateFrustumPlanesNearFar() {
	Mat44V viewProj;

#if SUPPORT_INVERTED_PROJECTION
	if ( GetInvertZInProjectionMatrix() )
	{
		Mat44V tmpProjection = m_Projection;	
		tmpProjection.SetM22f( -(tmpProjection.GetM22f() + 1.0f) DEPTH_BIAS_IN_PROJECTION_ONLY( - m_NDCDepthBias ) );
		tmpProjection.SetM23f( -tmpProjection.GetM23f() );

		Multiply(viewProj,tmpProjection,m_View);
	}
	else
#endif  // SUPPORT_INVERTED_PROJECTION
	{
#if SUPPORT_DEPTH_BIAS_IN_PROJECTION
		Mat44V tmpProjection = m_Projection;	
		tmpProjection.SetM22f( tmpProjection.GetM22f() - m_NDCDepthBias );
		Multiply(viewProj,tmpProjection,m_View);
#else
		Multiply(viewProj,m_Projection,m_View);
#endif // SUPPORT_DEPTH_BIAS_IN_PROJECTION
	}

	Mat44V comboMatrix;
	Transpose(comboMatrix,viewProj);

#if __OPENGL
	m_FrustumClipPlanes[CLIP_PLANE_NEAR] = NormalizePlane(comboMatrix.d() + comboMatrix.c());
#else
	m_FrustumClipPlanes[CLIP_PLANE_NEAR] = NormalizePlane(comboMatrix.c());
#endif
	m_FrustumClipPlanes[CLIP_PLANE_FAR] = NormalizePlane(comboMatrix.d() - comboMatrix.c());

	// We can add two more cull planes here for free. (replace the replicated planes at the end)
	// CLIP_PLANE_NEAR must be in .x channel so we can get outZ
	// NOTE: replicate a valid plane in the w slots, so the inside clip status is correct
	Mat44V planeSet;
	planeSet.SetCols(
		m_FrustumClipPlanes[CLIP_PLANE_NEAR],
		m_FrustumClipPlanes[CLIP_PLANE_FAR],
		m_FrustumClipPlanes[CLIP_PLANE_NEAR],
		m_FrustumClipPlanes[CLIP_PLANE_FAR]);
	Transpose(m_FrustumNFNF,planeSet);
}

int grcViewport::IsSphereVisible(Vec4V_In sphere) const {
	return IsSphereVisibleInline(sphere);
}

int grcViewport::IsSphereVisible(Vec4V_In sphere,float &outZ) const {
	return IsSphereVisibleInline(sphere,outZ);
}

grcCullStatus grcViewport::GetSphereCullStatus(Vec4V_In sphere) const {
	return GetSphereCullStatusInline(sphere);
}

grcCullStatus grcViewport::GetSphereCullStatus(Vec4V_In sphere,float &zOut) const {
	return GetSphereCullStatusInline(sphere,zOut);
}

grcCullStatus grcViewport::IsSphereVisible(float x,float y,float z,float radius,float *zRet NV_SUPPORT_ONLY(,bool useStereoFrustum)) const {
	return zRet? GetSphereCullStatusInline(Vec4V(x,y,z,radius),*zRet NV_SUPPORT_ONLY(,useStereoFrustum)) : GetSphereCullStatusInline(Vec4V(x,y,z,radius)NV_SUPPORT_ONLY(,useStereoFrustum));
}

int grcViewport::IsAABBVisible(Vec::V4Param128 vmin,Vec::V4Param128 vmax,Mat44V_In cullPlanes) {
	return IsAABBVisibleInline(vmin,vmax,cullPlanes);
}

grcCullStatus grcViewport::GetAABBCullStatus(Vec::V4Param128 vmin,Vec::V4Param128 vmax,Mat44V_In cullPlanes) {
	return GetAABBCullStatusInline(vmin,vmax,cullPlanes);
}

grcCullStatus grcViewport::GetAABBCullStatus(Vec::V4Param128 vmin,Vec::V4Param128 vmax,Mat44V_In cullPlanes1,Mat44V_In cullPlanes2) {
	return GetAABBCullStatusInline(vmin,vmax,cullPlanes1,cullPlanes2);
}

grcCullStatus grcViewport::IsModelSphereVisible(Vec4V_In sphere,float *zRet) const {
	Vec3V newCenter = rage::Transform(m_World44.GetMat34(), sphere.GetXYZ());
	Vec4V newSphere = Vec4V(newCenter,sphere.GetW());
	float dummy;
	return GetSphereCullStatusInline(newSphere,zRet?*zRet:dummy);
}

bool grcViewport::IsPointSetVisible(const Vec3V *const PointSet_WS, const int kNumPoints) const {
	VecBoolV reject0(V_F_F_F_F), reject1(V_F_F_F_F);

	for (int i=0; i<kNumPoints; i++)
	{
		Vec4V xxxx = Vec4V(SplatX(PointSet_WS[i]));	// replicate .GetXf() through a vector
		Vec4V yyyy = Vec4V(SplatY(PointSet_WS[i]));	// ...and y, etc
		Vec4V zzzz = Vec4V(SplatZ(PointSet_WS[i]));

		Vec4V sum0, sum1;
		sum0 = AddScaled( m_FrustumLRTB.d(), zzzz, m_FrustumLRTB.c() );
		sum0 = AddScaled( sum0, yyyy, m_FrustumLRTB.b() );
		sum0 = AddScaled( sum0, xxxx, m_FrustumLRTB.a() );
		sum1 = AddScaled( m_FrustumNFNF.d(), zzzz, m_FrustumNFNF.c() );
		sum1 = AddScaled( sum1, yyyy, m_FrustumNFNF.b() );
		sum1 = AddScaled( sum1, xxxx, m_FrustumNFNF.a() );

		// Planes that pass visibility will have 0xFFFFFFFF in their column.
		VecBoolV cmp0 = IsGreaterThanOrEqual(sum0, Vec4V(V_ZERO));
		VecBoolV cmp1 = IsGreaterThanOrEqual(sum1, Vec4V(V_ZERO));

		// For each plane, remember if we passed the plane test at least once.  
		// (Near/far are tested twice due to replication)
		reject0 = Or(reject0, cmp0);
		reject1 = Or(reject1, cmp1);
	}

	// Test passes only if all four columns in each test vector are nonzero.
	return IsEqualIntAll( And(reject0,reject1), VecBoolV( V_TRUE ) ) != 0;
}

bool grcViewport::IsPointVisible(Vec3V_In point NV_SUPPORT_ONLY(, bool useStereoFrustum)) const {

	Vec4V xxxx = Vec4V(SplatX(point));	// replicate .GetXf() through a vector
	Vec4V yyyy = Vec4V(SplatY(point));	// ...and y, etc
	Vec4V zzzz = Vec4V(SplatZ(point));

	Vec4V sum0;
#if NV_SUPPORT
	sum0 = AddScaled( useStereoFrustum ? m_CullFrustumLRTB.d() : m_FrustumLRTB.d(), zzzz, useStereoFrustum ? m_CullFrustumLRTB.c() : m_FrustumLRTB.c() );	
	sum0 = AddScaled( sum0, yyyy, useStereoFrustum ? m_CullFrustumLRTB.b() : m_FrustumLRTB.b() );
	sum0 = AddScaled( sum0, xxxx, useStereoFrustum ? m_CullFrustumLRTB.a() : m_FrustumLRTB.a() );
#else
	sum0 = AddScaled( m_FrustumLRTB.d(), zzzz, m_FrustumLRTB.c() );
	sum0 = AddScaled( sum0, yyyy, m_FrustumLRTB.b() );
	sum0 = AddScaled( sum0, xxxx, m_FrustumLRTB.a() );
#endif
	// Planes that pass visibility will have 0xFFFFFFFF in their column.
	VecBoolV cmp0 = IsGreaterThanOrEqual(sum0, Vec4V(V_ZERO));

	// Test passes only if all four columns in each test vector are nonzero.
	return IsEqualIntAll( cmp0, VecBoolV( V_TRUE ) ) != 0;
}

void grcViewport::Screen() {
	Ortho(
		m_Window.m_NormX * m_LastWidth,
		(m_Window.m_NormX + m_Window.m_NormWidth) * m_LastWidth,
		(m_Window.m_NormY + m_Window.m_NormHeight) * m_LastHeight, 
		m_Window.m_NormY * m_LastHeight,
		0.0f,+1.0f);
}

const grcWindow& grcViewport::GetWindow() const {
	return m_Window;
}

const grcWindow& grcViewport::GetUnclippedWindow() const {
    return m_UnclippedWindow;
}

// utility function
static inline Vec3V_Out ProjectiveTransform(Mat44V_In mtx,Vec4V_In in) {
	Vec4V result = Multiply(mtx,in);
	return result.GetXYZ() / result.GetW();
}

void grcViewport::Transform(Vec3V_In worldPosition, float& windowX, float& windowY) const {
	Vec3V windowNorm = ProjectiveTransform(GetFullCompositeMtx(),Vec4V(worldPosition,ScalarV(V_ONE)));
	windowX = windowNorm.GetXf();
	windowY = windowNorm.GetYf();
}

Vec3V_Out grcViewport::Transform(Vec3V_In worldPosition) const {
	return ProjectiveTransform(GetFullCompositeMtx(),Vec4V(worldPosition,ScalarV(V_ONE)));
}

void grcViewport::ReverseTransform(float x,float y,Vec3V_InOut outNear,Vec3V_InOut outFar) const {
	Mat44V reverse;
	InvertFull(reverse,GetFullCompositeMtx());
	outNear = ProjectiveTransform(reverse,Vec4V(x,y,0.0f,1.0f));
	outFar = ProjectiveTransform(reverse,Vec4V(x,y,1.0f,1.0f));
}

void grcViewport::ReverseTransformNoWorld(float x,float y,Vec3V_InOut outNear,Vec3V_InOut outFar) const {
	Mat44V composite, fullComposite;
	Multiply(composite,m_Projection,m_View);
	Multiply(fullComposite,m_Screen,m_Composite);

	Mat44V reverse;
	InvertFull(reverse,fullComposite);
	outNear = ProjectiveTransform(reverse,Vec4V(x,y,0.0f,1.0f));
	outFar = ProjectiveTransform(reverse,Vec4V(x,y,1.0f,1.0f));
}

#if __BANK

static inline Vec3V_Out CalcIntersectionOf3Planes(Vec4V_In vP1, Vec4V_In vP2, Vec4V_In vP3)
{
	Vec3V p1 = vP1.GetXYZ();
	Vec3V p2 = vP2.GetXYZ();
	Vec3V p3 = vP3.GetXYZ();

	Vec3V cross1, cross2, cross3;
	cross1 = Cross(p2, p3);
	cross2 = Cross(p3, p1);
	cross3 = Cross(p1, p2);
	Vec3V crossTemp = cross1;

	cross1 *= vP1.GetW();
	cross2 *= vP2.GetW();
	cross3 *= vP3.GetW();

	Vec3V sum = cross1 + cross2 + cross3;
	Vec3V isect = sum / Dot(p1,crossTemp);
	return isect;
}

void grcViewport::DebugGetExtents(Vec3V extentsr[8]) const
{
	Vector3 extents[8];
	Vector3 camPos(VEC3V_TO_VECTOR3(GetCameraMtx().d()));
	Vector3 camDir(VEC3V_TO_VECTOR3(-GetCameraMtx().c())); 
	Vector3 camA(VEC3V_TO_VECTOR3(GetCameraMtx().a()));
	Vector3 camB(VEC3V_TO_VECTOR3(GetCameraMtx().b()));

	camDir.Normalize();
	camA.Normalize();
	camB.Normalize();

	Vector3 nearCenter(camPos);
	nearCenter.AddScaled(camDir, m_ZClipNear);
	Vector3 farCenter(camPos);
	farCenter.AddScaled(camDir, m_ZClipFar);

	if( m_IsPerspective )
	{
		// rectangle that is seen by the light camera
		const Vector2 camViewWidth	( GetTanHFOV() * m_ZClipNear, GetTanHFOV() * m_ZClipFar );
		const Vector2 camViewHeight	( GetTanVFOV() * m_ZClipNear, GetTanVFOV() * m_ZClipFar ); 

		// left/right near point from viewer camera frustum
		Vector3 leftNear(camA), rightNear(-camA);
		leftNear	.Scale( (1.f-m_ShearX) * camViewWidth.x );
		rightNear	.Scale( (1.f+m_ShearX) * camViewWidth.x );

		// up/down near point from viewer camera frustum
		Vector3 upNear(-camB), downNear(camB);
		upNear	.Scale( (1.f+m_ShearY) * camViewHeight.x );
		downNear.Scale( (1.f-m_ShearY) * camViewHeight.x );

		// left/right far point from viewer camera frustum
		Vector3 leftFar(camA), rightFar(-camA);
		leftFar	.Scale( (1.f-m_ShearX) * camViewWidth.y );
		rightFar.Scale( (1.f+m_ShearX) * camViewWidth.y );

		// up/down far point from viewer camera frustum
		Vector3 upFar(-camB), downFar(camB);
		upFar	.Scale( (1.f+m_ShearY) * camViewHeight.y );
		downFar	.Scale( (1.f-m_ShearY) * camViewHeight.y );

		// Generate visibility box
		for(int j = 0; j < 4; j++)
		{
			// fill up with the near and far center points
			extents[j].Set( nearCenter);
			extents[j + 4].Set(farCenter);
		}

		// Create near plane
		// creates based on the near center point the four points that form 
		// the near plane of a viewing frustum
		// go from near center point to the left and then up
		extents[0].Add( leftNear);  extents[0].Add( upNear);
		// go from near center point to the right and then up 
		extents[1].Add(rightNear);  extents[1].Add( upNear);
		// go from the near center point to the left and then down
		extents[2].Add( leftNear);  extents[2].Add(downNear);
		// go from the near center point to the right and then down
		extents[3].Add(rightNear);  extents[3].Add(downNear);

		// Create far plane
		// creates based on the far center point the four points that form
		// the far plane of a viewing frustum
		// go from the far center point to the left and then up
		extents[4].Add( leftFar);  extents[4].Add( upFar);
		// go from the far center point to the right and then up
		extents[5].Add(rightFar);  extents[5].Add( upFar);
		// go from the far center point to the left and then down
		extents[6].Add( leftFar);  extents[6].Add(downFar);
		// go from the far center point to the right and then down
		extents[7].Add(rightFar);  extents[7].Add(downFar);
	}
	else
	{
		Vector4 vNearPlane, vFarPlane;
		vNearPlane.SetVector3(camDir);
		vNearPlane.w = nearCenter.Dot(camDir);
		vFarPlane.SetVector3(-camDir);
		vFarPlane.w = farCenter.Dot(-camDir);

		Vector3 vNearTopLeft = nearCenter;
		Vector3 vNearTopRight = nearCenter;
		Vector3 vNearBottomLeft = nearCenter;
		Vector3 vNearBottomRight = nearCenter;
		Vector3 vFarTopLeft = farCenter;
		Vector3 vFarTopRight = farCenter;
		Vector3 vFarBottomLeft = farCenter;
		Vector3 vFarBottomRight = farCenter;

		vNearTopLeft.AddScaled(camA, m_OrthoLRTB.GetYf());
		vNearTopLeft.AddScaled(camB, m_OrthoLRTB.GetZf());

		vNearTopRight.AddScaled(camA, m_OrthoLRTB.GetXf());
		vNearTopRight.AddScaled(camB, m_OrthoLRTB.GetZf());

		vNearBottomLeft.AddScaled(camA, m_OrthoLRTB.GetYf());
		vNearBottomLeft.AddScaled(camB, m_OrthoLRTB.GetWf());

		vNearBottomRight.AddScaled(camA, m_OrthoLRTB.GetXf());
		vNearBottomRight.AddScaled(camB, m_OrthoLRTB.GetWf());

		vFarTopLeft.AddScaled(camA, m_OrthoLRTB.GetYf());
		vFarTopLeft.AddScaled(camB, m_OrthoLRTB.GetZf());

		vFarTopRight.AddScaled(camA, m_OrthoLRTB.GetXf());;
		vFarTopRight.AddScaled(camB, m_OrthoLRTB.GetZf());

		vFarBottomLeft.AddScaled(camA, m_OrthoLRTB.GetYf());
		vFarBottomLeft.AddScaled(camB, m_OrthoLRTB.GetWf());

		vFarBottomRight.AddScaled(camA, m_OrthoLRTB.GetXf());;
		vFarBottomRight.AddScaled(camB, m_OrthoLRTB.GetWf());

		extents[0] = vNearTopLeft;
		extents[1] = vNearTopRight;
		extents[2] = vNearBottomLeft;
		extents[3] = vNearBottomRight;
		extents[4] = vFarTopLeft;
		extents[5] = vFarTopRight;
		extents[6] = vFarBottomLeft;
		extents[7] = vFarBottomRight;
	}
	for (int i=0;i<8;i++)
		extentsr[i]=Vec3V(extents[i].x,extents[i].y, extents[i].z);
		   
	// This is the incorrect new version.
	Vec3V extents2[8];
	extents2[0] = CalcIntersectionOf3Planes(
		GetFrustumClipPlane(grcViewport::CLIP_PLANE_NEAR),
		GetFrustumClipPlane(grcViewport::CLIP_PLANE_LEFT),
		GetFrustumClipPlane(grcViewport::CLIP_PLANE_TOP) );
	extents2[1] = CalcIntersectionOf3Planes(
		GetFrustumClipPlane(grcViewport::CLIP_PLANE_NEAR),
		GetFrustumClipPlane(grcViewport::CLIP_PLANE_RIGHT),
		GetFrustumClipPlane(grcViewport::CLIP_PLANE_TOP) );
	extents2[2] = CalcIntersectionOf3Planes(
		GetFrustumClipPlane(grcViewport::CLIP_PLANE_NEAR),
		GetFrustumClipPlane(grcViewport::CLIP_PLANE_LEFT),
		GetFrustumClipPlane(grcViewport::CLIP_PLANE_BOTTOM) );
	extents2[3] = CalcIntersectionOf3Planes(
		GetFrustumClipPlane(grcViewport::CLIP_PLANE_NEAR),
		GetFrustumClipPlane(grcViewport::CLIP_PLANE_RIGHT),
		GetFrustumClipPlane(grcViewport::CLIP_PLANE_BOTTOM) );
	extents2[4] = CalcIntersectionOf3Planes(
		GetFrustumClipPlane(grcViewport::CLIP_PLANE_FAR),
		GetFrustumClipPlane(grcViewport::CLIP_PLANE_LEFT),
		GetFrustumClipPlane(grcViewport::CLIP_PLANE_TOP) );
	extents2[5] = CalcIntersectionOf3Planes(
		GetFrustumClipPlane(grcViewport::CLIP_PLANE_FAR),
		GetFrustumClipPlane(grcViewport::CLIP_PLANE_RIGHT),
		GetFrustumClipPlane(grcViewport::CLIP_PLANE_TOP) );
	extents2[6] = CalcIntersectionOf3Planes(
		GetFrustumClipPlane(grcViewport::CLIP_PLANE_FAR),
		GetFrustumClipPlane(grcViewport::CLIP_PLANE_LEFT),
		GetFrustumClipPlane(grcViewport::CLIP_PLANE_BOTTOM) );
	extents2[7] = CalcIntersectionOf3Planes(
		GetFrustumClipPlane(grcViewport::CLIP_PLANE_FAR),
		GetFrustumClipPlane(grcViewport::CLIP_PLANE_RIGHT),
		GetFrustumClipPlane(grcViewport::CLIP_PLANE_BOTTOM) );
}

void grcViewport::DebugDraw() const
{
	grcWorldIdentity();

	grcBindTexture(0);
	grcColor4f(1.0f, 0.0f, 0.0f, 1.0f);

	Vec3V extents[8];
	DebugGetExtents(extents);

	grcBegin(drawLines, 28);

	grcVertex3f(extents[0]);
	grcVertex3f(extents[1]);

	grcVertex3f(extents[1]);
	grcVertex3f(extents[3]);

	grcVertex3f(extents[3]);
	grcVertex3f(extents[2]);
	
	grcVertex3f(extents[2]);
	grcVertex3f(extents[0]);

	grcVertex3f(extents[4]);
	grcVertex3f(extents[5]);

	grcVertex3f(extents[5]);
	grcVertex3f(extents[7]);

	grcVertex3f(extents[7]);
	grcVertex3f(extents[6]);
	
	grcVertex3f(extents[6]);
	grcVertex3f(extents[4]);

	grcVertex3f(extents[0]);
	grcVertex3f(extents[4]);
	
	grcVertex3f(extents[1]);
	grcVertex3f(extents[5]);

	grcVertex3f(extents[2]);
	grcVertex3f(extents[6]);

	grcVertex3f(extents[3]);
	grcVertex3f(extents[7]);

	// diagonals on near plane
	grcVertex3f(extents[0]);
	grcVertex3f(extents[3]);
	
	grcVertex3f(extents[1]);
	grcVertex3f(extents[2]);

	grcEnd();
}

void grcViewport::AddWidgets(bkBank &B) {
	B.AddSlider("Proj Scale X",&m_ScaleX,-1000,+1000,0.01f);
	B.AddSlider("Proj Scale Y",&m_ScaleY,-1000,+1000,0.01f);
	B.AddSlider("Proj Shear X",&m_ShearX,-1000,+1000,0.01f);
	B.AddSlider("Proj Shear Y",&m_ShearY,-1000,+1000,0.01f);
}

#endif // __BANK


void grcViewport::InitClass() {
	Assert(!sm_ScreenViewport[0]);
	for (unsigned i=0; i<NELEM(sm_ScreenViewport); ++i) {
		grcViewport *vp = rage_new grcViewport;
		vp->Screen();
		sm_ScreenViewport[i] = vp;
	}

#if !HACK_GTA4
	if (grcEffect::LookupGlobalVar("WorldInverse",false))
		grcErrorf("*** WorldInverse is no longer supported, please remove it [use WorldInverseTranspose instead with params swapped]");
	if (grcEffect::LookupGlobalVar("WorldViewInverse",false))
		grcErrorf("*** WorldViewInverse is no longer supported, please remove it [use ViewInverse and WorldInverseTranspose]");
	// Matrix44 m_World44, m_WorldInverseTranspose44, m_ModelView, m_Composite, m_Camera44;
#endif

#if RAGE_INSTANCED_TECH
	sysMemSet(sm_InstViewport, 0, sizeof(sm_InstViewport));
#endif
}


void grcViewport::ShutdownClass() {
	if (sm_ScreenViewport[0])
		for (unsigned i=0; i<NELEM(sm_ScreenViewport); ++i)
			delete sm_ScreenViewport[i];
	sysMemSet(sm_ScreenViewport, 0, sizeof(sm_ScreenViewport));
}


void grcViewport::ApplyObliqueProjection( Vec4V_In worldSpacePlane ) {
	Mat44V transpose, inverseTranspose;

	Transpose(transpose,GetCompositeMtx());
	InvertFull(inverseTranspose,transpose);
	Vec4V projClipPlane = Multiply(inverseTranspose, worldSpacePlane);


	if (projClipPlane.GetWf() == 0)  // or less than a really small value
	{
		// plane is perpendicular to the near plane
		return;
	}

	if (projClipPlane.GetWf() > 0)
	{
		// Old code:
		// flip plane to point away from eye
		// transform clip plane into projection space
		// inverseTranspose.Transform(-worldSpacePlane, projClipPlane);
		// or projClipPlane = Multiply(inverseTranspose, -worldSpacePlane)
		// so therefore we can just negate projClipPlane directly.
		projClipPlane = -projClipPlane;
	}

	Mat44V clipProj(V_IDENTITY);

	// put projection space clip plane in Z column
	clipProj.SetM20(projClipPlane.GetXf());
	clipProj.SetM21(projClipPlane.GetYf());
	clipProj.SetM22(projClipPlane.GetZf());
	clipProj.SetM23(projClipPlane.GetWf());
	// multiply into projection matrix
	Mat44V projClipMatrix;
	Multiply(projClipMatrix, clipProj, GetProjection());
	SetProjection( projClipMatrix );
}

#if MULTIPLE_RENDER_THREADS
ORBIS_ONLY(__attribute__((noinline))) void grcViewport::OwnerCheck()  {
	if (!m_Owner)
		m_Owner = sysIpcGetCurrentThreadId();
	else if (m_Owner != sysIpcGetCurrentThreadId() && g_IsSubRenderThread) {
		sysStack::PrintStackTrace();
		Errorf("MTR violation, subthread is trying to modify a viewport it didn't create");
		m_Owner = sysIpcGetCurrentThreadId();
	}
}
#else
#undef OwnerCheck
#endif

const grcViewport& grcViewport::operator=(const grcViewport&that)
{
	sysMemCpy(this, &that, sizeof(*this));
	m_Owner = 0;
	return (*this);
}

}	// namespace rage

