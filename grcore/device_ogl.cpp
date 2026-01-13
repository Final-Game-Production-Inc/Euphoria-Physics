// 
// grcore/device_ogl.cpp 
// 
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved. 
// 

#include "config.h"

#if __OPENGL

#include "device.h"
#include "effect.h"
#include "im.h"
#include "textureogl.h"		// HACK
#include "system/typeinfo.h"	// HACK
#include "viewport.h"
#include "system/param.h"
#include "system/wndproc.h"
#include "indexbuffer.h"
#include "vertexbuffer.h"

#include "opengl.h"

#include "shader_rage_im_psn.h"

#if __PPU
#include <PSGL/psgl.h>
#include <PSGL/psglu.h>
#endif

#if GL_EXTENDED
glDeclareProc(glCompressedTexImage2D);
glDeclareProc(glBindBuffer);
glDeclareProc(glDeleteBuffers);
glDeclareProc(glGenBuffers);
glDeclareProc(glBufferData);

glDeclareProc(glIsRenderbufferEXT);
glDeclareProc(glBindRenderbufferEXT);
glDeclareProc(glDeleteRenderbuffersEXT);
glDeclareProc(glGenRenderbuffersEXT);
glDeclareProc(glRenderbufferStorageEXT);
glDeclareProc(glGetRenderbufferParameterivEXT);
glDeclareProc(glIsFramebufferEXT);
glDeclareProc(glBindFramebufferEXT);
glDeclareProc(glDeleteFramebuffersEXT);
glDeclareProc(glGenFramebuffersEXT);
glDeclareProc(glCheckFramebufferStatusEXT);
glDeclareProc(glFramebufferTexture1DEXT);
glDeclareProc(glFramebufferTexture2DEXT);
glDeclareProc(glFramebufferTexture3DEXT);
glDeclareProc(glFramebufferRenderbufferEXT);
glDeclareProc(glGetFramebufferAttachmentParameterivEXT);
#endif

namespace rage {

// With the new DEH hardware we seem to get best results from
// using a PS2 component cable.
int grcDevice::sm_Width = 1280, grcDevice::sm_Height = 720;

const char *grcDevice::sm_DefaultEffectName = "x:\\rage\\assets\\tune\\shaders\\lib\\rage_im";

#if __PPU
int grcDevice::sm_LastWidth = -1, grcDevice::sm_LastHeight = -1;
#endif

int grcDevice::sm_FrameLock = 1;
bool grcDevice::sm_HardwareShaders = true;
bool grcDevice::sm_HardwareTransform = true;
bool grcDevice::sm_LetterBox=true; 
#if __PPU
bool grcDevice::m_ForceFlipHack = false;
#endif

#if __WIN32PC
HDC g_glHdc;
HGLRC g_glRc;

static bool s_WeCreatedHwndMain;
static bool s_ResetSettings;
#elif __PPU
static PSGLdevice *s_Device;
static PSGLcontext *s_Context;

u32 grcDevice::sm_ClipPlaneEnable = 0;
#if !HACK_GTA4 // no shader clip planes
static Vec4V s_ClipPlanes[RAGE_MAX_CLIPPLANES];
#endif // !HACK_GTA4

#endif

extern const grcVertexBuffer* grcLockedVertexBuffer;
extern const grcIndexBuffer* grcLockedIndexBuffer;
static const grcVertexDeclaration* grcCurrentVertexDeclaration = 0;

PARAM(width,"[grcore] Set width of main render window (default is 640)");
PARAM(height,"[grcore] Set height of main render window (default is 480)");
PARAM(setHwndMain,"[grcore] override the window that DirectX will render to");
PARAM(debugshaders,"[grcore] Enable CG shader debugging");
PARAM(glhaltonerror,"[grcore] Halt on GL/Cg errors (PS3)");

grcEffect g_DefaultEffect;
grcEffectVar g_DefaultSampler;
static grcEffectTechnique s_DefaultLit, s_DefaultUnlit, s_DefaultLitSkinned, s_DefaultUnlitSkinned, s_DefaultBlit;
static grcEffectGlobalVar s_gvClipToScreen;

#if __PPU
static void ragePsglReportError(GLenum reportEnum, GLuint reportClassMask, const char* string)
{
	grcErrorf("PSGL: %s", string);
	if (PARAM_glhaltonerror.Get())
	{
		asm("trap");
	}
}
#endif

void grcDevice::InitClass(bool inWindow, bool topMost) {
	PARAM_width.Get(sm_Width);
	PARAM_height.Get(sm_Height);

	g_WindowWidth = sm_Width;
	g_WindowHeight = sm_Height;
	g_InvWindowWidth = 1.0f / float(sm_Width);
	g_InvWindowHeight = 1.0f / float(sm_Height);

#if __WIN32PC
	if (!inWindow) {
		DEVMODE dm;
		dm.dmSize = sizeof(dm);
		dm.dmBitsPerPel = 32;
		dm.dmPelsWidth = sm_Width;
		dm.dmPelsHeight = sm_Height;
		dm.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
		if (ChangeDisplaySettings(&dm,0) != DISP_CHANGE_SUCCESSFUL)
			grcErrorf("ChangeDisplayMode failed.");
		/* else {
			ScreenWidth = Width;
			ScreenHeight = Height;
		} */
		s_ResetSettings = true;
	}
	else
		s_ResetSettings = false;

	g_inWindow = inWindow;
	g_isTopMost = g_inWindow && topMost;

	/* Open up a window. */
	static PIXELFORMATDESCRIPTOR pfdinit = {
		sizeof(PIXELFORMATDESCRIPTOR),	// size of this pfd
		1,	// version number
		PFD_DRAW_TO_WINDOW |	// support window
		PFD_SUPPORT_OPENGL |	// support OpenGL
		PFD_GENERIC_FORMAT |
		PFD_DOUBLEBUFFER,	// double buffered
		PFD_TYPE_RGBA,	// RGBA type
		24,	// 24-bit color depth
		0, 0, 0, 0, 0, 0,	// color bits ignored
		0,	// no alpha buffer
		0,	// shift bit ignored
		0,	// no accumulation buffer
		0, 0, 0, 0, 	// accum bits ignored
		16,	// 16-bit z-buffer (everyone supports this?)
		0,	// no stencil buffer
		0,	// no auxiliary buffer
		PFD_MAIN_PLANE,	// main layer
		0,	// reserved
		0, 0, 0	// layer masks ignored
	};
	PIXELFORMATDESCRIPTOR pfd = pfdinit;
	int  iPixelFormat;

	PARAM_setHwndMain.Get((int&)g_hwndMain);
	if (!g_hwndMain) {
		g_hwndMain = CreateDeviceWindow(g_hwndParent);
		s_WeCreatedHwndMain = true;
	}

	HWND hwnd = g_hwndMain; // (HWND)PIPE.Parent;
	g_glHdc = GetDC(hwnd);

	/* int nFormat = DescribePixelFormat(g_glHdc, 0, 0, NULL);
	for (int i=0; i<nFormat; i++) {
		DescribePixelFormat(g_glHdc, i, sizeof(pfd), &pfd);
		Printf("%d: ",i);
#define F(x) if (pfd.dwFlags & x) Printf("%s ",#x);
		F(PFD_DRAW_TO_WINDOW);
		F(PFD_DRAW_TO_BITMAP);
		F(PFD_SUPPORT_GDI);
		F(PFD_SUPPORT_OPENGL);
		F(PFD_GENERIC_ACCELERATED);
		F(PFD_GENERIC_FORMAT);
#undef F
		grcDisplayf("");
	} */

    // get the device context's best-available-match pixel format
    iPixelFormat = ChoosePixelFormat(g_glHdc, &pfd);

	//if (Format != -1)
	//	iPixelFormat = Format;

	// make that the device context's current pixel format
	SetPixelFormat(g_glHdc, iPixelFormat, &pfd);

    g_glRc = wglCreateContext(g_glHdc);
	wglMakeCurrent(g_glHdc,g_glRc);

	/* const char *exts = (const char *)glGetString(GL_EXTENSIONS);
	OutputDebugString("Extensions: ");
	OutputDebugString(exts);
	OutputDebugString("\n"); */
#elif __PPU
	PSGLinitOptions options;
	options.enable = PSGL_INIT_MAX_SPUS | PSGL_INIT_INITIALIZE_SPUS;
	options.maxSPUs = 1;
	options.initializeSPUs = GL_FALSE;	
	psglInit(&options);
	
	psglSetReportFunction(ragePsglReportError);

#if CELL_SDK_VERSION >= 0x083002
	s_Device = psglCreateDeviceAuto(GL_ARGB_SCE,GL_DEPTH_COMPONENT24,GL_MULTISAMPLING_NONE_SCE);
#else
	PSGLbufferParameters params = {
		width: sm_Width,
		height: sm_Height,
		colorBits: 24,
		alphaBits: 8,
		depthBits: 24,
		stencilBits: 8,
		deviceType: PSGL_DEVICE_TYPE_AUTO,
		TVStandard: PSGL_TV_STANDARD_NONE,
		TVFormat: PSGL_TV_FORMAT_AUTO,
		bufferingMode: PSGL_BUFFERING_MODE_DOUBLE,
		antiAliasing: GL_FALSE
	};

	s_Device = psglCreateDevice(&params);
#endif
	s_Context = psglCreateContext();

	psglMakeCurrent(s_Context, s_Device);

	psglResetCurrentContext();

	glViewport(0,0,sm_Width,sm_Height);
	glScissor(0,0,sm_Width,sm_Height);

	// init the screen since it starts as garbage
	glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
	glClearStencil(0);
	glClearDepthf(1.0f);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
	glFinish();
	psglSwap();
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
	glFinish();
	psglSwap();

	glClearColor(0,0,0,1);
	glFlush();
#endif

	glDisable(GL_CULL_FACE);

#if GL_EXTENDED
	// Must check the function pointers are valid before using them.
	// In particular, many of these fail over a Remote Desktop connection.
	glGetProc(glCompressedTexImage2D);
	glGetProc(glBindBuffer);
	glGetProc(glGenBuffers);
	glGetProc(glDeleteBuffers);
	glGetProc(glBufferData);

	// GL_EXT_framebuffer_object
	glGetProc(glIsRenderbufferEXT);
	glGetProc(glBindRenderbufferEXT);
	glGetProc(glDeleteRenderbuffersEXT);
	glGetProc(glGenRenderbuffersEXT);
	glGetProc(glRenderbufferStorageEXT);
	glGetProc(glGetRenderbufferParameterivEXT);
	glGetProc(glIsFramebufferEXT);
	glGetProc(glBindFramebufferEXT);
	glGetProc(glDeleteFramebuffersEXT);
	glGetProc(glGenFramebuffersEXT);
	glGetProc(glCheckFramebufferStatusEXT);
	glGetProc(glFramebufferTexture1DEXT);
	glGetProc(glFramebufferTexture2DEXT);
	glGetProc(glFramebufferTexture3DEXT);
	glGetProc(glFramebufferRenderbufferEXT);
	glGetProc(glGetFramebufferAttachmentParameterivEXT);

#endif

	grcEffect::InitClass();

	g_DefaultEffect.Init(sm_DefaultEffectName);
	s_DefaultLit = g_DefaultEffect.LookupTechnique("draw");
	s_DefaultUnlit = g_DefaultEffect.LookupTechnique("unlit_draw");
	s_DefaultLitSkinned = g_DefaultEffect.LookupTechnique("drawskinned");
	s_DefaultUnlitSkinned = g_DefaultEffect.LookupTechnique("unlit_drawskinned");
	s_DefaultBlit = g_DefaultEffect.LookupTechnique("drawblit");
	g_DefaultSampler = g_DefaultEffect.LookupVar("DiffuseTex");

	grcViewport::InitClass();

	s_gvClipToScreen = grcEffect::LookupGlobalVar("ClipToScreen", false);
}

void grcDevice::SetDefaultEffect(bool isLit,bool isSkinned) {
	grcEffectTechnique tech = isLit? (isSkinned? s_DefaultLitSkinned : s_DefaultLit) : (isSkinned? s_DefaultUnlitSkinned : s_DefaultUnlit);
	g_DefaultEffect.Bind(tech);
}

grcEffect& grcDevice::GetDefaultEffect()
{
	return g_DefaultEffect;
}

void grcDevice::SetLetterBox(bool enable)
{
	sm_LetterBox = enable;
	// probably need to do the equivalent of UpdatePresentationInterval() in D3D
}

void grcDevice::ReleaseThreadOwnership()
{
	grcWarningf("ReleaseThreadOwnership not implemented");
}

void grcDevice::AcquireThreadOwnership()
{
	grcWarningf("AcquireThreadOwnership not implemented");
}


void grcDevice::Clear(bool enableClearColor,Color32 clearColor,bool enableClearDepthStencil,float clearDepth,u32 clearStencil) {
	int mask = 0;
	if (enableClearColor)
		mask |= GL_COLOR_BUFFER_BIT;
	if (enableClearDepthStencil)
		mask |= GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
	glClearColor(clearColor.GetRedf(),clearColor.GetGreenf(),clearColor.GetBluef(),clearColor.GetAlphaf());
	glClearDepth(clearDepth);
	glClearStencil(clearStencil);
	glScissor(0,0,sm_Width,sm_Height);
#if !__PPU
	GLboolean depthMask, colorMask[4];
	glGetBooleanv(GL_DEPTH_WRITEMASK,&depthMask);
	glGetBooleanv(GL_COLOR_WRITEMASK,&colorMask[0]);
	if (!depthMask && enableClearDepthStencil)
		glDepthMask(true);
	if ((!colorMask[0] || !colorMask[1] || !colorMask[2] || !colorMask[3]) && enableClearColor)
		glColorMask(true,true,true,true);
#endif
	glClear(mask);
#if !__PPU
	if ((!colorMask[0] || !colorMask[1] || !colorMask[2] || !colorMask[3]) && enableClearColor)
		glColorMask(colorMask[0],colorMask[1],colorMask[2],colorMask[3]);
	if (!depthMask && enableClearDepthStencil)
		glDepthMask(false);
#endif
}


void grcDevice::ClearCachedState() {
}


void grcDevice::BeginFrame() {
#if __WIN32
	wglMakeCurrent(g_glHdc,g_glRc);
#endif

	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
//	glEnableClientState(GL_INDEX_ARRAY);

	grcState::Default();
}

void grcDevice::EndFrame(const grcResolveFlags *) {
	++sm_FrameCounter;

	g_DefaultEffect.Bind(grcetNONE);

#if __WIN32
	SwapBuffers(g_glHdc);
	wglMakeCurrent(0,0);

# if __WIN32PC
	if (sm_NewWindow.uWidth) {
		grcViewport::ShutdownClass();
		SetSize(sm_NewWindow.uWidth,sm_NewWindow.uHeight);
		// Deleting the context seems to invalidate existing VBO's which will cause model rendering
		// to completely fail.  Be careful here though -- some nvidia drivers will hard-lock if this
		// code is not enabled.  The latest 71.84 drivers seem to work fine with this code enabled and
		// also avoid the model rendering crash.
		// wglDeleteContext(g_glRc);
		// ReleaseDC(g_hwndMain, g_glHdc);
		// g_glHdc = GetDC(g_hwndMain);
		// g_glRc = wglCreateContext(g_glHdc);
		sm_NewWindow.uWidth = sm_NewWindow.uHeight = 0;
		grcViewport::InitClass();
	}
# endif
#elif __PPU
	psglSwap();
#endif
}

void grcDevice::CreateCommandBuffer(size_t /*size*/,grcCommandBuffer **pbuffer) {
	*pbuffer = NULL;
}

void grcDevice::DeleteCommandBuffer(grcCommandBuffer * /*buffer*/) {
}

void grcDevice::BeginCommandBuffer(grcCommandBuffer * /*buffer*/,bool /*isSkinned*/) {
}


size_t grcDevice::EndCommandBuffer(grcCommandBuffer ** /*cloneBuffer*/) {
	return 0;
}


void grcDevice::RunCommandBuffer(grcCommandBuffer * /*buffer*/,u32 /*predicationSelect*/) {
}

void grcDevice::SetCommandBufferPredication(u32 /*tile*/,u32 /*run*/) {
}


void grcDevice::ShutdownClass() {

	g_DefaultEffect.Shutdown();

	grcViewport::ShutdownClass();
	grcEffect::ShutdownClass();

#if __WIN32
	wglMakeCurrent(0, 0);
	// wglDeleteContext(g_glRc);
	ReleaseDC(g_hwndMain,g_glHdc);

	if (s_ResetSettings) {
		s_ResetSettings = false;
		ChangeDisplaySettings(NULL,0);
	}

	if (g_hwndMain) {
		DestroyWindow(g_hwndMain);
		g_hwndMain = 0;
		UnregisterClass("grcWindow", ::GetModuleHandle(0));
		g_winClass = 0;
	}
#elif __PPU
	psglDestroyContext(s_Context);
	psglDestroyDevice(s_Device);
#endif
}

bool grcDevice::GetWideScreen()
{
	return false;
}

bool grcDevice::GetHiDef()
{
	return false;
}

void grcDevice::SetWindow(const grcWindow &window) {
	glViewport(window.GetX(),window.GetY(),window.GetWidth(),window.GetHeight());
	glScissor(window.GetX(),window.GetY(),window.GetWidth(),window.GetHeight());
	glEnable(GL_SCISSOR_TEST);
	glDepthRange(window.GetMinZ(),window.GetMaxZ());
}

/* void grcDevice::ClearRect(int x,int y,int width,int height,float depth,const Color32 &color) {
	// NOTE: This is temporary until I read the manual...

	// hopefully, they turned off textures...
	// BlitRect(x,y,x+width,y+height,0,0,1,1,1.0f,color);
} */

static int s_BlitCount;

void grcDevice::BeginBlit()
{
	// update the clip -> screen mapping
	if ((sm_Width != sm_LastWidth) || (sm_Height != sm_LastHeight))
	{
		sm_LastWidth = sm_Width;
		sm_LastHeight = sm_Height;

		float left = 0.0f;
		float right = sm_Width;
		float bottom = sm_Height;
		float top = 0.0f;
		float znear = __D3D ? 0.0f : -1.0f;
		float zfar = +1.0f;

		float oorl = 1.0f / (right - left);
		float ootb = 1.0f / (top - bottom);
		float oolr = 1.0f / (left - right);
		float oobt = 1.0f / (bottom - top);

		float cz, dz;

		if ( __D3D )
		{
			// D3DXMatrixOrthoOffCenterRH
			cz = 1 / (znear-zfar);
			dz = znear / (znear - zfar);
		}
		else 
		{
			// glOrtho
			cz = -2 / (zfar-znear);
			dz = -(zfar + znear) / (zfar-znear);
		}

		Matrix44 m_ClipToScreen;
		m_ClipToScreen.a.Set(2.0f * oorl, 0.0f, 0.0f, 0.0f);
		m_ClipToScreen.b.Set(0.0f, 2.0f * ootb, 0.0f, 0.0f);
		m_ClipToScreen.c.Set(0.0f, 0.0f, cz, 0.0f);
		m_ClipToScreen.d.Set((left + right) * oolr, (top + bottom) * oobt, dz, 1.0f);

		grcEffect::SetGlobalVar(s_gvClipToScreen, m_ClipToScreen);
	}

	if (++s_BlitCount == 1) {
		if (!grcEffect::IsInDraw()) {
			g_DefaultEffect.Bind(s_DefaultBlit);
		}
	}
}


void grcDevice::EndBlit() {
	if (--s_BlitCount == 0) {
	}
}

void grcDevice::BlitRectf(float x1,float y1,float x2,float y2,float zVal,float u1,float v1,float u2,float v2,const Color32 &color)
{
	

	BeginBlit();

	// BEGIN LAME HACK
#if 1
	if (m_ForceFlipHack || grcTextureFactoryOGL::IsRenderTargetLocked())
#else
	const grcTexture *tex = grcState::GetTexture(0);
	if (tex && SafeCast(const grcRenderTargetOGL,tex))
#endif
	{
		v1 = 1.0f - v1;
		v2 = 1.0f - v2;
	}
	// END LAME HACK

	// Lame hack to prevent im_ogl from hosing the shader we just set up in BeginBlit.
	grcBegin(drawQuads, 4);

	grcColor(color);
	grcTexCoord2f(u1,v1);
	grcVertex3f(x1,y1,zVal);

	grcTexCoord2f(u1,v2);
	grcVertex3f(x1,y2,zVal);

	grcTexCoord2f(u2,v2);
	grcVertex3f(x2,y2,zVal);

	grcTexCoord2f(u2,v1);
	grcVertex3f(x2,y1,zVal);

	grcEnd();

	EndBlit();
}

void grcDevice::BlitRect(int x1,int y1,int x2,int y2,float zVal,int u1,int v1,int u2,int v2,const Color32 &color) {
	const grcTexture *tex = grcGetTexture();
	float tu = 1, tv = 1;
	if (tex) {
		tu = float(tex->GetWidth());
		tv = float(tex->GetHeight());
	}
	BlitRectf(float(x1),float(y1),float(x2),float(y2),zVal,u1/tu,v1/tv,u2/tu,v2/tv,color);

}

void grcDevice::GetSafeZone(int &x0, int &y0, int &x1, int &y1) {
#if __PPU
	// TODO: Compute PSN's safe zone here:
	x0 = 0;
	y0 = 0;
	x1 = sm_Width-1;
	y1 = sm_Height-1;
#else	
	// Assume PC
	x0 = 0;
	y0 = 0;
	x1 = sm_Width-1;
	y1 = sm_Height-1;
#endif
}


#if __PPU
void grcDevice::SetWindowTitle(const char*) {
}
#endif

void grcDevice::SetScissor(int x,int y,int width,int height) {
	glScissor(x,y,width,height);
}


void grcDevice::ClearRect(int /*x*/,int /*y*/,int /*width*/,int /*height*/,float /*depth*/,const Color32 &/*color*/) {
}


grcImage* grcDevice::CaptureScreenShot() {
	return NULL;
}

bool grcDevice::CaptureScreenShotToJpegFile(const char *outFile)
{
	//TODO
	return false;
}

void grcDevice::SetFrameLock(int,bool) {
}

int grcDevice::GetFrameLock() {
	return 1;
}

void grcDevice::BlitText(int posx,int posy,float posz,const s16 *destxywh,const u8 *srcxywh,int count,Color32 color,bool /*bilinear*/) {
	

	BeginBlit();

	//grcDisplayf("blit text! %d",count);
	grcBegin(drawQuads, count*4);
	grcColor(color);
	const grcTexture *tex = grcGetTexture();
	float itu = 1.0f, itv = 1.0f;
	if (tex) {
		itu = 1.0f / float(tex->GetWidth());
		itv = 1.0f / float(tex->GetHeight());
	}

	while (count--) {
		float x1 = posx+(destxywh[0]>>4), y1 = posy+(destxywh[1]>>4), 
			x2 = posx+((destxywh[0]+destxywh[2])>>4), y2 = posy+((destxywh[1]+destxywh[3])>>4),
			u1 = srcxywh[0] * itu, v1 = srcxywh[1] * itv,
			u2 = (srcxywh[0]+srcxywh[2]) * itu, v2 = (srcxywh[1]+srcxywh[3]) * itv;
		grcTexCoord2f(u1,v1);
		grcVertex3f(x1,y1,posz);
		grcTexCoord2f(u1,v2);
		grcVertex3f(x1,y2,posz);
		grcTexCoord2f(u2,v2);
		grcVertex3f(x2,y2,posz);
		grcTexCoord2f(u2,v1);
		grcVertex3f(x2,y1,posz);
		destxywh += 4;
		srcxywh += 4;
	}
	grcEnd();

	EndBlit();
}

u32 grcDevice::SetClipPlaneEnable(u32 enable) {
	u32 old = sm_ClipPlaneEnable;
	sm_ClipPlaneEnable = enable & ((1<<RAGE_MAX_CLIPPLANES) - 1); // hardware supports up to RAGE_MAX_CLIPPLANES clip planes
	//sm_Current->SetRenderState(D3DRS_CLIPPLANEENABLE, sm_ClipPlaneEnable);
	return old;
}

void grcDevice::SetClipPlane(int index,Vec4V_In plane) {
#if !HACK_GTA4 // no shader clip planes

	Assert(index >= 0 && index < RAGE_MAX_CLIPPLANES);
	s_ClipPlanes[index] = plane;
	Vec4V outPlane;
	Mat44V transpose, inverseTranspose;
	// I'd love to claim I'm really good at linear algebra but I just looked at the Xenon light shaft
	// example and tried different permutations of inverse and transpose until it works.
	Transpose(transpose,grcViewport::GetCurrent()->GetCompositeMtx());
	InvertFull(inverseTranspose,transpose);
	outPlane = Multiply(inverseTranspose,plane);
	//sm_Current->SetClipPlane(index, (float*)&outPlane);

#else

	if (index >= RAGE_MAX_CLIPPLANES) return; // not supported on hardware
	Vec4V outPlane;
	Mat44V transpose, inverseTranspose;
	// I'd love to claim I'm really good at linear algebra but I just looked at the Xenon light shaft
	// example and tried different permutations of inverse and transpose until it works.
	Transpose(transpose,grcViewport::GetCurrent()->GetCompositeMtx());
	InvertFull(inverseTranspose,transpose);
	outPlane = Multiply(inverseTranspose,plane);
	//sm_Current->SetClipPlane(index, (float*)&outPlane);

#endif
}

#if !HACK_GTA4 // no shader clip planes
void grcDevice::GetClipPlane(int index,Vec4V_InOut plane) {
	Assert(index >= 0 && index < RAGE_MAX_CLIPPLANES);
	plane = s_ClipPlanes[index];
}
#endif // !HACK_GTA4

grcVertexDeclaration* grcDevice::CreateVertexDeclaration(const grcVertexElement *pVertexElements, int elementCount)
{
	grcVertexDeclaration* retVal = rage_new grcVertexDeclaration();

	int dataOffset = 0;
	for( int i = 0; i < elementCount; i++ )
	{
		switch( pVertexElements[i].type )
		{
			case grcVertexElement::grcvetPosition:
				{
					retVal->vertexSize = pVertexElements[i].size / 4;
					retVal->vertexType = GL_FLOAT;
					retVal->vertexPointer = dataOffset;
				}
				break;
			case grcVertexElement::grcvetNormal:
				{
					retVal->normalType = GL_FLOAT;
					retVal->normalPointer = dataOffset;
				}
				break;
			case grcVertexElement::grcvetBinormal:
				grcWarningf("grcDevice::CreateVertexDeclaration - Binormals not supported");
				break;
			case grcVertexElement::grcvetTangent:
				grcWarningf("grcDevice::CreateVertexDeclaration - Tangents not supported");
				break;
			case grcVertexElement::grcvetTexture:
				{
					// For now make sure we dont use more than 5 texture channels
					Assert(pVertexElements[i].channel < 5);

					retVal->texCoordSizes[pVertexElements[i].channel] = pVertexElements[i].size / 4;
					retVal->texCoordTypes[pVertexElements[i].channel] = GL_FLOAT;
					retVal->texCoordPointers[pVertexElements[i].channel] = dataOffset;
				}
				break;
			case grcVertexElement::grcvetBlendWeights:
				{
					retVal->blendWeightSize = pVertexElements[i].size / 4;
					retVal->blendWeightType = GL_FLOAT;
					retVal->blendWeightPointer = dataOffset;
				}
				break;
			case grcVertexElement::grcvetBindings:
				{
					retVal->blendIndicesSize = pVertexElements[i].size / 4;
					retVal->blendIndicesType = GL_FLOAT;
					retVal->blendIndicesPointer = dataOffset;
				}
				break;
			case grcVertexElement::grcvetColor:
				{
					retVal->colorType = GL_FLOAT;
					retVal->colorPointer = dataOffset;
				}
				break;
			default:
				Assert(0);
				break;
		}
		dataOffset += pVertexElements[i].size;
	}
	retVal->stride = dataOffset;

	retVal->refCount = 1;
	return retVal;
}

void BindVertexFormat(long pointer)
{
	if( grcCurrentVertexDeclaration )
	{
		// Vertex Data
		if( grcCurrentVertexDeclaration->vertexType )
		{
			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(grcCurrentVertexDeclaration->vertexSize, grcCurrentVertexDeclaration->vertexType, grcCurrentVertexDeclaration->stride, (GLvoid*)(pointer + grcCurrentVertexDeclaration->vertexPointer));
		}
		else
		{
			glDisableClientState(GL_VERTEX_ARRAY);
		}

		// Normal Data
		if( grcCurrentVertexDeclaration->normalType )
		{
			glEnableClientState(GL_NORMAL_ARRAY);
			glNormalPointer(grcCurrentVertexDeclaration->normalType, grcCurrentVertexDeclaration->stride, (GLvoid*)(pointer + grcCurrentVertexDeclaration->normalPointer));
		}
		else
		{
			glDisableClientState(GL_NORMAL_ARRAY);
		}

		// Color Data
		if( grcCurrentVertexDeclaration->colorType )
		{
			glEnableClientState(GL_COLOR_ARRAY);
			glColorPointer(4, grcCurrentVertexDeclaration->colorType, grcCurrentVertexDeclaration->stride, (GLvoid*)(pointer + grcCurrentVertexDeclaration->colorPointer));
		}
		else
		{
			glDisableClientState(GL_COLOR_ARRAY);
		}

		// Texture 0
		glClientActiveTexture(GL_TEXTURE0);
		if( grcCurrentVertexDeclaration->texCoordTypes[0] )
		{
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glTexCoordPointer(grcCurrentVertexDeclaration->texCoordSizes[0], grcCurrentVertexDeclaration->texCoordTypes[0], grcCurrentVertexDeclaration->stride, (GLvoid*)(pointer + grcCurrentVertexDeclaration->texCoordPointers[0]));
		}
		else
		{
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}

		// Texture 1
		glClientActiveTexture(GL_TEXTURE1);
		if( grcCurrentVertexDeclaration->texCoordTypes[1] )
		{
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glTexCoordPointer(grcCurrentVertexDeclaration->texCoordSizes[1], grcCurrentVertexDeclaration->texCoordTypes[1], grcCurrentVertexDeclaration->stride, (GLvoid*)(pointer + grcCurrentVertexDeclaration->texCoordPointers[1]));
		}
		else
		{
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}

		// Texture 2
		glClientActiveTexture(GL_TEXTURE2);
		if( grcCurrentVertexDeclaration->texCoordTypes[2] )
		{
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glTexCoordPointer(grcCurrentVertexDeclaration->texCoordSizes[2], grcCurrentVertexDeclaration->texCoordTypes[2], grcCurrentVertexDeclaration->stride, (GLvoid*)(pointer + grcCurrentVertexDeclaration->texCoordPointers[2]));
		}
		else
		{
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}

		// Texture 3
		glClientActiveTexture(GL_TEXTURE3);
		if( grcCurrentVertexDeclaration->texCoordTypes[3] )
		{
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glTexCoordPointer(grcCurrentVertexDeclaration->texCoordSizes[3], grcCurrentVertexDeclaration->texCoordTypes[3], grcCurrentVertexDeclaration->stride, (GLvoid*)(pointer + grcCurrentVertexDeclaration->texCoordPointers[3]));
		}
		else
		{
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}
		
		// Texture 4
		glClientActiveTexture(GL_TEXTURE4);
		if( grcCurrentVertexDeclaration->texCoordTypes[4] )
		{
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glTexCoordPointer(grcCurrentVertexDeclaration->texCoordSizes[4], grcCurrentVertexDeclaration->texCoordTypes[4], grcCurrentVertexDeclaration->stride, (GLvoid*)(pointer + grcCurrentVertexDeclaration->texCoordPointers[4]));
		}
		else
		{
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}

		// Blend Weights
		glClientActiveTexture(GL_TEXTURE5);
		if( grcCurrentVertexDeclaration->blendWeightType )
		{
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glTexCoordPointer(grcCurrentVertexDeclaration->blendWeightSize, grcCurrentVertexDeclaration->blendWeightType, grcCurrentVertexDeclaration->stride, (GLvoid*)(pointer + grcCurrentVertexDeclaration->blendWeightPointer));
		}
		else
		{
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}

		// Blend Indices
		glClientActiveTexture(GL_TEXTURE6);
		if( grcCurrentVertexDeclaration->blendIndicesType )
		{
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glTexCoordPointer(grcCurrentVertexDeclaration->blendIndicesSize, grcCurrentVertexDeclaration->blendIndicesType, grcCurrentVertexDeclaration->stride, (GLvoid*)(pointer + grcCurrentVertexDeclaration->blendIndicesPointer));
		}
		else
		{
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}
	}
}

grcDevice::Result grcDevice::SetVertexDeclaration(const grcVertexDeclaration* pDecl)
{
	grcCurrentVertexDeclaration = pDecl;
	BindVertexFormat(0);
	
	return 0;
}

void grcDevice::SetIndices(const grcIndexBuffer& pBuffer)
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pBuffer.GetOGLBuffer());
	grcLockedIndexBuffer = &pBuffer;
}

void grcDevice::SetStreamSource(u32 ,const grcVertexBuffer& pStreamData,u32 ,u32 )
{
	glBindBuffer(GL_ARRAY_BUFFER, pStreamData.GetOGLBuffer());
	grcLockedVertexBuffer = &pStreamData;
	BindVertexFormat(0);
}

void grcDevice::ClearStreamSource(u32 )
{
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	grcLockedVertexBuffer = 0;
}

static GLenum modeMap[] =
{
	GL_POINTS,
	GL_LINES,
	GL_LINE_STRIP,
	GL_TRIANGLES,
	GL_TRIANGLE_STRIP,
	GL_TRIANGLE_FAN,
	GL_QUADS,
};

void grcDevice::DrawIndexedPrimitive(grcDrawMode dm, int startIndex, int indexCount)
{
	int indexSize = grcLockedIndexBuffer->GetIndexSize();
	GLenum type = GL_UNSIGNED_SHORT;
	if( indexSize != sizeof(unsigned short) )
		type = GL_UNSIGNED_INT;
	u8* pointer = (u8*)((long)indexSize * (long)startIndex);
	glDrawRangeElements(modeMap[dm], 0, grcLockedVertexBuffer->GetVertexCount() - 1, indexCount, type, pointer);
}

void grcDevice::DrawPrimitive(grcDrawMode dm, int startVertex, int vertexCount)
{
	glDrawArrays(modeMap[dm], startVertex, vertexCount);
}

}	// namespace rage

#endif		// __OPENGL
