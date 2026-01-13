//
// grcore/setup.cpp
//
// Copyright (C) 1999-2015 Rockstar Games.  All Rights Reserved.
//
#include "setup.h"

#include "device.h"
#include "font.h"
#include "light.h"
#include "texture.h"
#include "viewport.h"
#include "bankio.h"
#include "quads.h"

#if !__FINAL
#include "image.h"
#include "minifixedwidthfont.h"
#include "Verdana18_fnt.h"
#endif

#include "qa/BugstarInterface.h"

#include "bank/bank.h"
#include "bank/bkmgr.h"
#include "bank/console.h"
#include "bank/imageviewer.h"
#include "bank/packet.h"
#include "data/marker.h"
#include "diag/tracker.h"
#include "diag/xmllog.h"
#include "file/asset.h"	
#include "file/token.h"
#include "grcore/im.h"
#include "grcore/channel.h"
#include "grcore/device.h"
#include "grcore/effect.h"
#include "grcore/effect_values.h"
#include "grcore/vertexdecl.h"
#include "grcore/wrapper_gcm.h"
#include "input/eventq.h"
#include "input/input.h"
#include "input/mouse.h"
#include "input/mapper.h"
#include "grprofile/profiler.h"
#include "grprofile/timebars.h"
#include "system/bootmgr.h"
#include "system/exec.h"
#include "system/memory.h"
#include "grprofile/pix.h"
#include "system/param.h"
#include "system/timemgr.h"
#include "system/wndproc.h"

#if __WIN32
#include "system/xtl.h"
#include "grcore/vertexbuffer.h"
#include "grcore/indexbuffer.h"
#include "grcore/resourcecache.h"
#include "grcore/wrapper_d3d.h"
#endif

#if __PPU
#include "wrapper_gcm.h"
#include "grcorespu.h"
#pragma comment(lib,"sysutil_stub")
namespace rage {
extern u32 g_CommandBufferUsage;
}
#endif

#if __PPU
#include <sysutil/sysutil_common.h>
#endif

#if ENABLE_RAZOR_CPU
#include <perf.h>
#endif

#if __BANK
#include "system/ipc.h"
#include "file/tcpip.h"
#endif

using namespace rage;

#define USE_GPU_PERF_API (__STATS && RSG_PC && RSG_CPU_X64 && !__TOOL && !__RESOURCECOMPILER)

#if USE_GPU_PERF_API
PARAM(usePerfAPI, "Use the realtime perf API");

///<	AMD GPUPerfAPI
#include "../../3rdParty/AMD/GPUPerfAPI-2.14.1054.0/Include/GPUPerfAPI.h"
#include "../../3rdParty/AMD/GPUPerfAPI-2.14.1054.0/Include/GPUPerfAPIFunctionTypes.h"

GPA_InitializePtrType			__GPA_Initialize;
GPA_GetNumCountersPtrType		__GPA_GetNumCounters;
GPA_EnableAllCountersPtrType	__GPA_EnableAllCounters;
GPA_EnableCounterPtrType		__GPA_EnableCounter;
GPA_EnableCounterStrPtrType		__GPA_EnableCounterStr;
GPA_OpenContextPtrType			__GPA_OpenContext;
GPA_GetPassCountPtrType			__GPA_GetPassCount;
GPA_BeginPassPtrType			__GPA_BeginPass;
GPA_EndPassPtrType				__GPA_EndPass;
GPA_BeginSamplePtrType			__GPA_BeginSample;
GPA_EndSamplePtrType			__GPA_EndSample;
GPA_BeginSessionPtrType			__GPA_BeginSession;
GPA_EndSessionPtrType			__GPA_EndSession;
GPA_IsSessionReadyPtrType		__GPA_IsSessionReady;
GPA_CloseContextPtrType			__GPA_CloseContext;
GPA_DestroyPtrType				__GPA_Destroy;
GPA_GetSampleCountPtrType		__GPA_GetSampleCount;
GPA_GetEnabledIndexPtrType		__GPA_GetEnabledIndex;
GPA_GetSampleUInt32PtrType		__GPA_GetSampleUInt32;
GPA_GetSampleFloat64PtrType		__GPA_GetSampleFloat64;
GPA_GetCounterDataTypePtrType	__GPA_GetCounterDataType;
GPA_GetCounterNamePtrType		__GPA_GetCounterName;
GPA_GetCounterIndexPtrType		__GPA_GetCounterIndex;
GPA_GetEnabledCountPtrType		__GPA_GetEnabledCount;
GPA_RegisterLoggingCallbackPtrType	__GPA_RegisterLoggingCallback;

///<	NVIDIA GPUPerfAPI
#define	NVPM_INITGUID
#include "../../3rdParty/NVidia/PerfKit/inc/NvPmApi.Manager.h"

static NvPmApiManager	s_NVPMManager;
extern NvPmApiManager*	GetNvPmApiManager()	{ return &s_NVPMManager; }

const NvPmApi*			GetNvPmApi() { return s_NVPMManager.Api(); }

static NVPMContext		g_hNVPMContext(0);
static u32				g_SessionID;
static u32				g_RequiredPass;
static u32				g_CurrentSessionID = 1;
static u32				g_CurrentPassID;
static u32				g_LastBatchCount = 0;
static bool				g_NVIDIA_PerfKit_Enabled = false;
static bool				g_GPA_Enabled = false;
static bool				g_FrameFreezed = false;
static size_t			g_CountersCount;

///< NVIDIA PAGE
PF_PAGE(NVIDIAGPUPerf, "GPU Perf counters NVIDIA");
PF_GROUP(NVIDIAPerfCounters);
PF_LINK(NVIDIAGPUPerf, NVIDIAPerfCounters);
PF_VALUE_FLOAT(NVIDIAGPUIdle, NVIDIAPerfCounters);
PF_VALUE_FLOAT(NVIDIAGPUPrimitiveSetupBottleneck, NVIDIAPerfCounters);
PF_VALUE_FLOAT(NVIDIAGPUROPBottleneck, NVIDIAPerfCounters);
PF_VALUE_FLOAT(NVIDIAGPUTEXBottleneck, NVIDIAPerfCounters);
PF_VALUE_FLOAT(NVIDIAGPUSHDBottleneck, NVIDIAPerfCounters);
PF_VALUE_FLOAT(NVIDIAGPUFBBottleneck, NVIDIAPerfCounters);
PF_VALUE_FLOAT(NVIDIAGPUZCullBottleneck, NVIDIAPerfCounters);
PF_VALUE_FLOAT(NVIDIAGPURasterBottleneck, NVIDIAPerfCounters);
PF_VALUE_FLOAT(NVIDIAGPUIABottleneck, NVIDIAPerfCounters);
PF_VALUE_INT(NVIDIAGPUPrimitiveCount, NVIDIAPerfCounters);
PF_VALUE_INT(NVIDIAGPUBatchCount, NVIDIAPerfCounters);
PF_VALUE_INT(NVIDIAGPUFrameTime, NVIDIAPerfCounters);
PF_VALUE_INT(NVIDIAGPUDriverTimer, NVIDIAPerfCounters);
PF_VALUE_INT(NVIDIAGPUDriverTimerWaiting, NVIDIAPerfCounters);
PF_VALUE_INT(NVIDIAGPUDriverTimerWaitingForGPU, NVIDIAPerfCounters);
PF_VALUE_INT(NVIDIAGPUDriverTimerWaitingForKernel, NVIDIAPerfCounters);
PF_VALUE_INT(NVIDIAGPUDriverTimerWaitingForLock, NVIDIAPerfCounters);
PF_VALUE_INT(NVIDIAGPUDriverTimerWaitingForRender, NVIDIAPerfCounters);
PF_VALUE_INT(NVIDIAGPUDriverTimerWaitingForPresent, NVIDIAPerfCounters);
PF_VALUE_INT(NVIDIAGPUMemoryAllocated, NVIDIAPerfCounters);

///< AMD PAGE
PF_PAGE(AMDGPUPerf, "GPU Perf counters AMD");
PF_GROUP(AMDPerfCounters);
PF_LINK(AMDGPUPerf, AMDPerfCounters);
PF_VALUE_FLOAT(AMDGPUTime,				AMDPerfCounters);
PF_VALUE_FLOAT(AMDTexCacheStalled,		AMDPerfCounters);
PF_VALUE_FLOAT(AMDShaderBusy,			AMDPerfCounters);
PF_VALUE_FLOAT(AMDVSBusy,				AMDPerfCounters);
PF_VALUE_FLOAT(AMDPSBusy,				AMDPerfCounters);
PF_VALUE_FLOAT(AMDCSBusy,				AMDPerfCounters);
PF_VALUE_FLOAT(AMDPrimitiveAssembly,	AMDPerfCounters);
PF_VALUE_FLOAT(AMDPSExportStalls,		AMDPerfCounters);
PF_VALUE_FLOAT(AMDFetchUnitStalled,		AMDPerfCounters);
PF_VALUE_FLOAT(AMDMemUnitBusy,			AMDPerfCounters);
PF_VALUE_FLOAT(AMDMemUnitStalled,		AMDPerfCounters);
PF_VALUE_FLOAT(AMDWriteUnitStalled,		AMDPerfCounters);
PF_VALUE_FLOAT(AMDCSALUStalledByLDS,	AMDPerfCounters);
PF_VALUE_FLOAT(AMDCSLDSBankConflict,	AMDPerfCounters);
PF_VALUE_FLOAT(AMDPAStalledOnRasterizer,	AMDPerfCounters);

struct GPUPerfCounter
{
	const char* counterName;
	void*		counterValue;
	u8			counterType;		//	0 - UINT 1 - FLOAT 2 - PERCENT
	bool		realtime;
};

const GPUPerfCounter	c_NVIDIAPerfCounters[] = {
	//	Counter Name					Counter Value									TYPE	REALTIME
 	{	"D3D batch count",				&PF_VALUE_VAR(NVIDIAGPUBatchCount),						0,		true	},
 	{	"gpu_idle",						&PF_VALUE_VAR(NVIDIAGPUIdle),							2,		true	},
 	{	"Primitive Setup Bottleneck",	&PF_VALUE_VAR(NVIDIAGPUPrimitiveSetupBottleneck),		1,		false	},
 	{	"SHD Bottleneck",				&PF_VALUE_VAR(NVIDIAGPUSHDBottleneck),					1,		false	},
 	{	"ROP Bottleneck",				&PF_VALUE_VAR(NVIDIAGPUROPBottleneck),					1,		false	},
 	{	"TEX Bottleneck",				&PF_VALUE_VAR(NVIDIAGPUTEXBottleneck),					1,		false	},
 	{	"FB Bottleneck",				&PF_VALUE_VAR(NVIDIAGPUFBBottleneck),					1,		false	},
 	{	"ZCull Bottleneck",				&PF_VALUE_VAR(NVIDIAGPUZCullBottleneck),				1,		false	},
 	{	"Rasterization Bottleneck",		&PF_VALUE_VAR(NVIDIAGPURasterBottleneck),				1,		false	},
 	{	"IA Bottleneck",				&PF_VALUE_VAR(NVIDIAGPUIABottleneck),					1,		false	},
	{	"D3D primitive count",			&PF_VALUE_VAR(NVIDIAGPUPrimitiveCount),					0,		true	},
	{	"D3D frame time",				&PF_VALUE_VAR(NVIDIAGPUFrameTime),						0,		false	},
	{	"D3D driver time",				&PF_VALUE_VAR(NVIDIAGPUDriverTimer),					0,		false	},
	{	"D3D driver time waiting",		&PF_VALUE_VAR(NVIDIAGPUDriverTimerWaiting),				0,		false	},
 	{	"D3D driver waits for GPU",		&PF_VALUE_VAR(NVIDIAGPUDriverTimerWaitingForGPU),		0,		false	},
 	{	"D3D driver waits for kernel",	&PF_VALUE_VAR(NVIDIAGPUDriverTimerWaitingForKernel),	0,		false	},
 	{	"D3D driver waits for lock",	&PF_VALUE_VAR(NVIDIAGPUDriverTimerWaitingForLock),		0,		false	},
 	{	"D3D driver waits for render",	&PF_VALUE_VAR(NVIDIAGPUDriverTimerWaitingForRender),	0,		false	},
 	{	"D3D driver waits for present",	&PF_VALUE_VAR(NVIDIAGPUDriverTimerWaitingForPresent),	0,		false	},
	{	"D3D memory allocated",			&PF_VALUE_VAR(NVIDIAGPUMemoryAllocated),				0,		false	},
};

const GPUPerfCounter	c_AMDPerfCounters[] = {
	//	Counter Name					Counter Value									TYPE	REALTIME
	{	"GPUTime",						&PF_VALUE_VAR(AMDGPUTime),						1,		false	},
	{	"TexCacheStalled",				&PF_VALUE_VAR(AMDTexCacheStalled),				1,		false	},
	{	"ShaderBusy",					&PF_VALUE_VAR(AMDShaderBusy),					1,		false	},
	{	"VSBusy",						&PF_VALUE_VAR(AMDVSBusy),						1,		false	},
	{	"PSBusy",						&PF_VALUE_VAR(AMDPSBusy),						1,		false	},
	{	"CSBusy",						&PF_VALUE_VAR(AMDCSBusy),						1,		false	},
	{	"PrimitiveAssemblyBusy",		&PF_VALUE_VAR(AMDPrimitiveAssembly),			1,		false	},
	{	"PSExportStalls",				&PF_VALUE_VAR(AMDPSExportStalls),				1,		false	},
	{	"FetchUnitStalled",				&PF_VALUE_VAR(AMDFetchUnitStalled),				1,		false	},
	{	"MemUnitBusy",					&PF_VALUE_VAR(AMDMemUnitBusy),					1,		false	},
	{	"MemUnitStalled",				&PF_VALUE_VAR(AMDMemUnitStalled),				1,		false	},
	{	"WriteUnitStalled",				&PF_VALUE_VAR(AMDWriteUnitStalled),				1,		false	},
	{	"CSALUStalledByLDS",			&PF_VALUE_VAR(AMDCSALUStalledByLDS),			1,		false	},
	{	"CSLDSBankConflict",			&PF_VALUE_VAR(AMDCSLDSBankConflict),			1,		false	},
	{	"PAStalledOnRasterizer",		&PF_VALUE_VAR(AMDPAStalledOnRasterizer),		1,		false	},
};

void	RegisterGPUPerfCounters(const bool& realtimeOnly)
{
	if(g_NVIDIA_PerfKit_Enabled || g_GPA_Enabled)
	{
		const GPUPerfCounter*	counters = g_NVIDIA_PerfKit_Enabled ? c_NVIDIAPerfCounters : c_AMDPerfCounters;
		
		for(size_t idx=0; idx <  g_CountersCount; idx++)
		{
			if(realtimeOnly && !counters[idx].realtime)
			{
				continue;
			}

			if(g_NVIDIA_PerfKit_Enabled)
			{
				GetNvPmApi()->AddCounterByName(g_hNVPMContext, counters[idx].counterName);
			}
			else
			{
				GPA_Status status;
				
				status = __GPA_EnableCounterStr(counters[idx].counterName);
				Assert(status == GPA_STATUS_OK);
			}
		}
	} 
}

template<typename T>
void	SetCounterValue(const GPUPerfCounter& counter, const T& value)
{

	switch (counter.counterType)
	{
	case 0:
		{
			::rage::pfValueT<int>*	pfValue = (::rage::pfValueT<int>*)counter.counterValue;
			pfValue->Set((int)value);
		}
		break;
	default:
		{
			::rage::pfValueT<float>*	pfValue = (::rage::pfValueT<float>*)counter.counterValue;
			pfValue->Set((float)value);
		}
		break;
	}
}

void	ResetGPUStats()
{
	if(g_NVIDIA_PerfKit_Enabled || g_GPA_Enabled)
	{
		const GPUPerfCounter*	counters = g_NVIDIA_PerfKit_Enabled ? c_NVIDIAPerfCounters : c_AMDPerfCounters;

		for(size_t idx=0; idx <  g_CountersCount; idx++)
		{
			SetCounterValue(counters[idx], 0);
		}
	}
}

void	FlushGPU()
{
	grcFenceHandle	fence = GRCDEVICE.AllocFence(grcDevice::ALLOC_FENCE_INIT_AS_PENDING);
	GRCDEVICE.GpuMarkFenceDone(fence, 0);
	GRCDEVICE.CpuWaitOnFenceAndFree(fence);
}

void	GPALoggin(GPA_Logging_Type messageType, const char* message)
{
	const char* errorString[] = {
		"GPA_LOGGING_NONE",
		"GPA_LOGGING_ERROR",
		"GPA_LOGGING_MESSAGE",
		"GPA_LOGGING_ERROR_AND_MESSAGE",
		"GPA_LOGGING_TRACE",
		"GPA_LOGGING_ERROR_AND_TRACE",
		"GPA_LOGGING_MESSAGE_AND_TRACE",
		"GPA_LOGGING_ERROR_MESSAGE_AND_TRACE"
	};
	Displayf("[AMDPerf] %s : %s", errorString[messageType], message);
}
#endif	//	#if USE_GPU_PERF_API

RAGE_DEFINE_CHANNEL(Graphics);

RAGE_DEFINE_CHANNEL(PCDevice);

using namespace rage;

PARAM(localbank,"[bank] Display local widgets instead of remote widgets.  Optional parameters specify number of lines to show, draw scale (in percent), and base x and y");
PARAM(remotebank,"[bank] Attempt to connect to remote bank server.");
PARAM(rag,"[bank] Connect to RAG (a main application).");
PARAM(ragEventQBufferSize, "[bank] The size of the event queue for receiving keyboard and mouse input from Rag.  The default is 256*ioEventQueue::COMMAND_SIZE (1280 bytes).  A larger size will allow mouse input to be handled better in low framerate situations." );
PARAM(ragviewer,"[bank] Connect to RAG (a rage viewer).");
PARAM(ragAddr,"[bank] Connect with Rag on a specific IP address." );
PARAM(appname,"[bank] Visible name of the application.");
PARAM(frametime,"[setup] [TOGGLE] Display update, draw, and intra-frame time.");
PARAM(frameticker,"[setup] [TOGGLE] Display update, draw, and intra-frame ticker.");
PARAM(frametickerScale,"[setup] Set the vertical scale for the frame ticker.");
#if !__FINAL
PARAM(loadtimer,"[setup] Display a loading timer");
namespace rage { sysTimer g_grcLoadTimer; }
#endif
PARAM(nographics,"[setup] Disable graphics window (for automation).");
PARAM(nullhardware,"[setup] Use D3D null hardware (only on Xenon).");	
PARAM(powerconsole,"[setup] allows the ability to use powershell console.");
PARAM(localconsole,"[setup] run with a local console.");
PARAM(nomousepointer,"[setup] Never draw the mouse pointer.");
PARAM(reddotsight,"[setup] Change the mouse pointer into a red dot.");
PARAM(autoscreenshot,"[setup] Automatically starts to take screenshots.");
PARAM(sanitycheckupdate,"[setup] Sanity check game heap at end of every update.");
PARAM(jpegsavequality,"[setup] Set JPEG save quality (default 75, can be 60-100)");
#if !__FINAL
PARAM(propsysfont,"[setup] Use a proportional system font.");
PARAM(debugtextfixedwidth,"[debug] Use fixed-width font for EVERYTHING");
PARAM(bigframetime,"[setup] Draw frametime at twice size");
#endif
#if RAGE_TRACKING
//PARAM(trackerrag,"[diag] Log all memory tracking activity to a rag plugin.  Add 'true' as a parameter to enable detailed deallocation tracking.");
#endif
PARAM(maxtexturesize, "[setup] (DEV only) Sets the maximum size for a texture");
#if !__FINAL && !__BANK
PARAM(console,"[setup] Enable console on release builds");
#endif

#if ENABLE_PIX_TAGGING
namespace rage
{
	BANK_ONLY(XPARAM(monoRuntime));
}; // namespace rage

extern unsigned int g_EnablePixAnnotation;
static bool s_enablePIX=(g_EnablePixAnnotation != 0);
#endif // ENABLE_PIX_TAGGING

#if __BANK
static volatile bool s_RagWantsUsToQuit;
static sysIpcSema s_ScreenshotDialogSema;
#endif
namespace rage { extern int g_JpegSaveQuality; }

#define MINIMUM_IMAGE_VIEWER_UPDATE_INTERVAL 0.5f	// half a second


#if !__FINAL
float g_SafeZoneSize = 0.85f;

#define TICKER_ONLY(x) x
static const u32 History = 128;
static const u32 BufferSize = History + 4;

float g_TickerScaleY = 1.0f;

class Ticker {
public:
	Ticker(const char *label,float r,float g,float b) { 
		memset(Data,0,sizeof(Data)); 
		Label = label;
		Color.Setf(r,g,b);
		Next = First;
		First = this;
	}
	void Add(float value) {	Data[Put] = value; }	// must match scaleY below
	static void Draw(float x,float y) {
		const float scaleX = 4.0f;
		const float scaleY = 3.0f * g_TickerScaleY;
		grcBegin(drawLines,20);
		grcColor4f(0.7f,0.7f,0.7f,1.0f);
		if (GRCDEVICE.GetRefreshRate() == 50) {
			grcVertex2f(x,y - 100.0f * scaleY); grcVertex2f(x + History * scaleX,y - 100.0f * scaleY);
			grcVertex2f(x,y - 80.0f * scaleY); grcVertex2f(x + History * scaleX,y - 80.0f * scaleY);
			grcVertex2f(x,y - 60.0f * scaleY); grcVertex2f(x + History * scaleX,y - 60.0f * scaleY);
			grcVertex2f(x,y - 40.0f * scaleY); grcVertex2f(x + History * scaleX,y - 40.0f * scaleY);
			grcVertex2f(x,y - 20.0f * scaleY); grcVertex2f(x + History * scaleX,y - 20.0f * scaleY);
		}
		else {
			grcVertex2f(x,y - 83.33f * scaleY); grcVertex2f(x + History * scaleX,y - 83.33f * scaleY);
			grcVertex2f(x,y - 66.67f * scaleY); grcVertex2f(x + History * scaleX,y - 66.67f * scaleY);
			grcVertex2f(x,y - 50.00f * scaleY); grcVertex2f(x + History * scaleX,y - 50.00f * scaleY);
			grcVertex2f(x,y - 33.34f * scaleY); grcVertex2f(x + History * scaleX,y - 33.34f * scaleY);
			grcVertex2f(x,y - 16.67f * scaleY); grcVertex2f(x + History * scaleX,y - 16.67f * scaleY);
		}
		grcColor4f(0.4f,0.4f,0.4f,1.0f);
		grcVertex2f(x,y - 1.0f * scaleY); grcVertex2f(x + History * scaleX,y - 1.0f * scaleY);
		grcVertex2f(x,y - 2.0f * scaleY); grcVertex2f(x + History * scaleX,y - 2.0f * scaleY);
		grcVertex2f(x,y - 3.0f * scaleY); grcVertex2f(x + History * scaleX,y - 3.0f * scaleY);
		grcVertex2f(x,y - 4.0f * scaleY); grcVertex2f(x + History * scaleX,y - 4.0f * scaleY);
		grcVertex2f(x,y - 5.0f * scaleY); grcVertex2f(x + History * scaleX,y - 5.0f * scaleY);
		grcEnd();
		float lx = x - 30.0f, ly = y - 10.0f;
		for (Ticker *t = First; t; t=t->Next) {
			grcColor(t->Color);
			grcDraw2dText(lx,ly,t->Label);
			ly -= 10.0f;
		}
		grcColor4f(0.7f,0.7f,0.7f,1.0f);
		if (GRCDEVICE.GetRefreshRate() == 50) {
			grcDraw2dText(lx - 8.0f,y - 80.00f * scaleY - 6.0f,"80.0");
			grcDraw2dText(lx - 8.0f,y - 40.00f * scaleY - 6.0f,"40.0");
		}
		else {
			grcDraw2dText(lx - 8.0f,y - 83.33f * scaleY - 6.0f,"83.3");
			grcDraw2dText(lx - 8.0f,y - 66.67f * scaleY - 6.0f,"66.6");
			grcDraw2dText(lx - 8.0f,y - 50.00f * scaleY - 6.0f,"50.0");
			grcDraw2dText(lx - 8.0f,y - 33.34f * scaleY - 6.0f,"33.3");
		}

		int ix = (int) x, iy = (int) y;
		for (Ticker *t = First; t; t=t->Next) {
			grcBegin(drawLineStrip,History);
			grcColor(t->Color);
			for (int i=0, j=Get; i<History; i++,j++) {
				if (j==BufferSize) j = 0;
				float val = t->Data[j] * scaleY;
				grcVertex2f((float)(ix + i * scaleX), (float)(iy - val));
			};
			grcEnd();
		}
	}
	static void StepPut() {
		// in some cases we step on update thread but not on render, clamp here so we don't go too far
		if (Put == (Get - 1))
			return;

		if (++Put == BufferSize)
			Put = 0;
	}
	static void StepGet() {
		// in some cases (e.g. loading) we do the render calls but not update,
		// need to make sure Get is always in sync with Put or the graphs will be rendered odd.
		if (Get == (Put + 3))
			return;

		if (++Get == BufferSize)
			Get = 0;
	}

	static void Reset() {
		Put = 0;
		Get = 3;
	}

	static int Put;
	static int Get;

private:
	Ticker *Next;
	Color32 Color;
	float Data[BufferSize];
	static Ticker *First;
	const char *Label;
};
int Ticker::Put = 0;
int Ticker::Get = 3;
Ticker* Ticker::First = 0;
Ticker 
	UpdateTime("Upd",0.9f,0.0f,0.0f),   UpdateTask("UpT",0.7f,0.6f,0.0f),
	DrawTime("Drw",0.0f,0.0f,0.8f),     DrawTask("DrT",0.0f,0.6f,0.8f),
	GpuTime("Gpu",0.0f,0.8f,0.0f),      TbTime("TB",1.0f,0.0f,1.0f),
	GpuIdleTime("GId",0.6f,0.8f,0.0f),  GpuBusyTime("GBs",0.0f,0.8f,0.6f),
	DeltaTime("Dt",1.00f,0.65f,0.00f),  FlipWaits("FW", 0.6f, 0.6f, 0.8f)/*, Wait("Wait", 1.00f,0.25f,0.25f)*/;

#else
#define TICKER_ONLY(x)
#endif


static void SmoothTimer(float &dest,float src) {
	const float beta = 1/64.0f;
	const float alpha = 1.0f - beta;
	if (fabsf(dest - src) > 1.0f)
		dest = src;
	else
		dest = alpha * dest + beta * src;
}

static u8 s_FontBitmap[127 * 8] = {
	// 1-15
	0,0,60,60,60,60,0,0,					// 1 - Checkbox
	0x10,0x38,0x7C,0xFE,0x38,0x38,0x38,0,	// 2 - up arrow
	0x38,0x38,0x38,0xFE,0x7C,0x38,0x10,0,	// 3 - down arrow
	0x10,0x18,0xFC,0xFF,0xFC,0x18,0x10,0,	// 4 - right arrow
	8, 16, 120, 204, 252, 192, 124, 0,	// 5 accented e
	48, 72, 120, 204, 252, 192, 124, 0,	// 6 carrot e
	118, 220, 0, 220, 102, 102, 102, 0,	// 7 tilde n
	8, 16, 56, 12, 124, 204, 118, 0,	// 8 accented a
	36, 0, 60, 102, 102, 102, 60, 0,	// 9 umlaut o
	0,0,0,0,0,0,0,0,			// 10 Line Feed
	36, 0, 56, 12, 124, 204, 118, 0,	// 11 umlaut a
	0,0,0,0,0,0,0,0,			// 12
	0,0,0,0,0,0,0,0,			// 13
	0,0,0,0,0,0,0,0,			// 14
	0,0,0,0,0,0,0,0,			// 15

	// all possible four-bit representations:
	0,0,0,0,0,0,0,0,
	3,3,3,3,3,3,3,3,
	12,12,12,12,12,12,12,12,
	15,15,15,15,15,15,15,15,
	48,48,48,48,48,48,48,48,
	51,51,51,51,51,51,51,51,
	60,60,60,60,60,60,60,60,
	63,63,63,63,63,63,63,63,
	192,192,192,192,192,192,192,192,
	195,195,195,195,195,195,195,195,
	204,204,204,204,204,204,204,204,
	207,207,207,207,207,207,207,207,
	240,240,240,240,240,240,240,240,
	243,243,243,243,243,243,243,243,
	252,252,252,252,252,252,252,252,
	255,255,255,255,255,255,255,255,

	// ASCII 32:
	0, 0, 0, 0, 0, 0, 0, 0,
	24, 24, 24, 24, 24, 0, 24, 0,
	204, 204, 204, 0, 0, 0, 0, 0,
	54, 108, 254, 108, 254, 108, 216, 0,
	24, 126, 192, 124, 6, 252, 48, 0,
	194, 198, 12, 24, 48, 102, 198, 0,
	56, 108, 56, 112, 222, 204, 118, 0,
	48, 48, 96, 0, 0, 0, 0, 0,
	12, 24, 48, 48, 48, 24, 12, 0,
	48, 24, 12, 12, 12, 24, 48, 0,
	0, 108, 56, 254, 56, 108, 0, 0,
	0, 24, 24, 126, 24, 24, 0, 0,
	0, 0, 0, 0, 0, 24, 24, 48,
	0, 0, 0, 126, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 24, 24, 0,
	2, 6, 12, 24, 48, 96, 192, 0,
	124, 206, 222, 246, 230, 198, 124, 0,
	24, 56, 24, 24, 24, 24, 126, 0,
	124, 198, 6, 28, 112, 198, 254, 0,
	124, 198, 6, 28, 6, 198, 124, 0,
	28, 60, 108, 204, 254, 12, 12, 0,
	254, 192, 252, 6, 6, 198, 124, 0,
	60, 96, 192, 252, 198, 198, 124, 0,
	254, 198, 12, 24, 48, 48, 48, 0,
	124, 198, 198, 124, 198, 198, 124, 0,
	124, 198, 198, 126, 6, 12, 120, 0,
	0, 24, 24, 0, 0, 24, 24, 0,
	0, 24, 24, 0, 0, 24, 24, 48,
	12, 24, 48, 96, 48, 24, 12, 0,
	0, 0, 126, 0, 0, 126, 0, 0,
	48, 24, 12, 6, 12, 24, 48, 0,
	60, 102, 6, 12, 24, 0, 24, 0,
	124, 198, 222, 222, 222, 192, 124, 0,
	56, 108, 198, 198, 254, 198, 198, 0,		// A
	252, 110, 102, 124, 102, 110, 252, 0,
	62, 98, 192, 192, 192, 98, 62, 0,
	248, 110, 102, 102, 102, 110, 248, 0,
	254, 98, 96, 120, 96, 98, 254, 0,
	254, 98, 96, 120, 96, 96, 240, 0,
	62, 98, 192, 192, 206, 102, 62, 0,
	198, 198, 198, 254, 198, 198, 198, 0,
	60, 24, 24, 24, 24, 24, 60, 0,
	30, 12, 12, 12, 12, 204, 120, 0,
	230, 102, 108, 120, 120, 108, 230, 0,
	240, 96, 96, 96, 96, 102, 254, 0,
	198, 238, 254, 214, 198, 198, 198, 0,
	198, 230, 246, 254, 222, 206, 198, 0,
	124, 198, 198, 198, 198, 198, 124, 0,
	252, 102, 102, 124, 96, 96, 224, 0,
	124, 198, 198, 214, 222, 124, 6, 0,
	252, 102, 102, 124, 120, 108, 230, 0,
	124, 198, 224, 56, 14, 198, 124, 0,
	126, 90, 24, 24, 24, 24, 60, 0,
	102, 102, 102, 102, 102, 102, 60, 0,
	102, 102, 102, 102, 102, 60, 24, 0,
	198, 198, 198, 214, 254, 254, 198, 0,
	198, 108, 56, 56, 108, 198, 198, 0,
	102, 102, 102, 60, 24, 24, 60, 0,
	254, 204, 24, 48, 96, 198, 254, 0,
	60, 48, 48, 48, 48, 48, 60, 0,
	128, 192, 96, 48, 24, 12, 6, 0,
	60, 12, 12, 12, 12, 12, 60, 0,
	24, 60, 102, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 255,
	24, 24, 12, 0, 0, 0, 0, 0,
	0, 0, 56, 12, 124, 204, 118, 0,				// a
	96, 96, 96, 124, 102, 102, 220, 0,
	0, 0, 124, 196, 192, 196, 124, 0,
	12, 12, 12, 124, 204, 204, 118, 0,
	0, 0, 120, 204, 252, 192, 124, 0,
	56, 108, 96, 248, 96, 96, 224, 0,
	0, 0, 118, 204, 204, 124, 12, 124,
	224, 96, 124, 102, 102, 102, 102, 0,
	48, 0, 112, 48, 48, 48, 56, 0,
	12, 0, 28, 12, 12, 204, 204, 120,
	224, 96, 102, 108, 120, 108, 102, 0,
	112, 48, 48, 48, 48, 48, 56, 0,
	0, 0, 204, 254, 214, 198, 198, 0,
	0, 0, 220, 102, 102, 102, 102, 0,
	0, 0, 60, 102, 102, 102, 60, 0,
	0, 0, 220, 102, 102, 124, 96, 224,
	0, 0, 118, 204, 204, 124, 12, 14,
	0, 0, 220, 118, 96, 96, 96, 0,
	0, 0, 120, 192, 120, 12, 120, 0,
	16, 48, 120, 48, 48, 52, 24, 0,
	0, 0, 204, 204, 204, 204, 118, 0,
	0, 0, 102, 102, 102, 60, 24, 0,
	0, 0, 198, 198, 214, 254, 108, 0,
	0, 0, 204, 120, 48, 120, 204, 0,
	0, 0, 204, 204, 204, 124, 12, 124,
	0, 0, 252, 24, 48, 96, 252, 0,
	14, 24, 24, 112, 24, 24, 14, 0,
	48, 48, 48, 0, 48, 48, 48, 0,
	112, 24, 24, 14, 24, 24, 112, 0,
	0, 118, 220, 0, 0, 0, 0, 0,
	// 16, 56, 108, 198, 198, 198, 254, 0,
	85, 170, 85, 170, 85, 170, 85, 170
};

static grcFont *s_Font;

#if !__FINAL
static grcFont *s_ProportionalFont;
static grcFont *s_MiniFixedWidthFont;
#endif // !__FINAL

bool s_Paused;

namespace rage { 
	grcSetup* grcSetupInstance; 
}

#if __PPU

namespace rage
{
	bool g_RequestSysutilExit = false;
}

void grcSetup::InitSysUtilCallback()
{
#if __PPU
	AssertVerify(CELL_OK == cellSysutilRegisterCallback( 0, grcSetup::_SysutilCallback, NULL ));
#endif
}

void grcSetup::_SysutilCallback( uint64_t status, uint64_t /*param*/, void* /*userdata*/ )
{
	if (status == CELL_SYSUTIL_REQUEST_EXITGAME) {
		grcDisplayf("*** Got sysutil quit request");
		HANG_DETECT_EXIT();	
		g_RequestSysutilExit = true;
	}
}

#if !__FINAL
void grcSetup::SetFlipWaits(u32 waits)
{
     FlipWaits.Add((1 + waits) * 16.667f);
}
#endif // !__FINAL
#endif

grcSetup::grcSetup() {
	m_AddBug = false;
	m_TakeScreenshot = false;
	m_ScreenshotFrameDelay = 0;
	m_ScreenshotGamma = 2.2f;
	m_ScreenshotNamingConvention = PROMPT_FOR_EACH_SCREENSHOT;
	m_DisableMousePointerDraw = PARAM_nomousepointer.Get();
	m_DisableMousePointerDrawDuringScreenshot = true;
	m_RedDotSight = PARAM_reddotsight.Get();
	m_ScreenshotsInARow = 1;
	m_ScreenshotsLeft = 0;
	m_ScreenshotIntervalTime = 0.0f;
	m_ScreenshotLastTakenTime = 0.0f;
	m_ScreenshotImageViewerLastUpdatedTime = 0.0f;
#if __BANK
    m_ImageViewer = NULL;
	m_ScreenshotName[0] = '\0';
	m_UseScreenShotFileDialogue =true;
	m_needToDisplayScreenshotInRagApplicationWindow = false;
	m_DisplayScreenshotInRagApplicationWindow = false;
	m_FrameToCapture = static_cast<u32>(-1);
	m_DebugRenderTargetName = NULL;
	if (!s_ScreenshotDialogSema)
		s_ScreenshotDialogSema = sysIpcCreateSema(true);
#else
	strcpy(m_ScreenshotName, "c:/Screenshot");
	m_UseScreenShotFileDialogue =false;
#endif
	m_ShowFrameTime = PARAM_frametime.Get() || PARAM_frameticker.Get();
	m_ShowFrameTicker = PARAM_frameticker.Get();
	m_BugX = m_BugY = m_BugZ = 0;
	strcpy(m_BugGrid,"Unknown");
	strcpy(m_BugOwner,"Unknown");
	sysGetEnv("USERNAME",m_BugOwner,sizeof(m_BugOwner));
#if __BANK
	m_LastSendTime=0.25f;
	m_DrawColorBars = COLORBARS_NONE;	
#endif
#if !__FINAL
	m_DrawSafeZone = false;
	m_InputLength = 0;
	m_Input[0] = 0;
	m_InputEnabled = false;
#endif
	PARAM_jpegsavequality.Get(g_JpegSaveQuality);

#if __PPU
	//AssertVerify(CELL_OK == cellSysutilRegisterCallback( 0, grcSetup::SysutilCallback, this ));
	m_LastFifoStallTime = m_LastFragmentStallTime = m_LastJobTime = m_LastDmaTime = 0;
	m_FifoStallTime = m_FragmentStallTime = m_JobTime = m_DmaTime = 0.0f;
#endif

#if MOUSE_RENDERING_SUPPORT
	m_UnmovedMouseDrawCount = 0;
	m_MaxUnmovedMouseDrawCount = 120;
	m_PreviousMouseX = -1;
	m_PreviousMouseY = -1;
#endif
}

grcSetup::~grcSetup() {
#if __PPU
	cellSysutilUnregisterCallback( 0 );
#endif
}

u8* grcSetup::GetInternalFontBits() { return s_FontBitmap; }

void grcSetup::CreateDefaultFactories() {
#if __XENON
	if (PARAM_nullhardware.Get()) {
		D3D__NullHardware = TRUE;
	}
#endif

#if __WIN32PC && !__RESOURCECOMPILER
	static grcVertexManager soVertexManager;
	static grcIndexManager  soIndexManager;
#endif

	if (PARAM_nographics.Get())
		grcTextureFactory::CreateStringTextureFactory(true);
#if !__PPU
	// this is a hack I know I call this function a bit earlier on the PS3to get back buffer and depth buffer render targets
	else
		grcTextureFactory::CreatePagedTextureFactory();
#endif
	grcTextureFactory::InitClass();

#if __FINAL
	s_Font = grcFont::CreateFontFromBitmap(s_FontBitmap, 8, 8, 1, 127);
	grcFont::SetCurrent(s_Font);
#else  // !__FINAL
	if (PARAM_debugtextfixedwidth.Get())
	{
		s_MiniFixedWidthFont = grcFont::CreateFontFromBitmap(MiniFixedWidthFont_data, 6, 8, 1, 127);
		s_ProportionalFont = s_MiniFixedWidthFont;
		s_Font = s_MiniFixedWidthFont;
	}
	else
	{
		// Intentional hard-coded path since we don't want to ship the game with it.
		// s_ProportionalFont = grcFont::CreateFontFromFiles("x:\\gta5\\src\\dev\\rage\\base\\src\\grcore","Verdana18");
		s_ProportionalFont = grcFont::CreateFontFromMetrics(Verdana18_Metrics);
		s_MiniFixedWidthFont = grcFont::CreateFontFromBitmap(MiniFixedWidthFont_data, 6, 8, 1, 127);
		s_Font = grcFont::CreateFontFromBitmap(s_FontBitmap, 8, 8, 1, 127);
		if (!s_ProportionalFont)
			s_ProportionalFont = s_Font;
	}
	grcFont::SetCurrent((PARAM_propsysfont.Get()) ? s_ProportionalFont : s_Font);
#endif // __FINAL
}

bool grcSetup::IsPaused()
{
	return s_Paused;
}

void grcSetup::SetPaused(bool pause)
{
	s_Paused = pause;
}

grcFont *grcSetup::GetFixedWidthFont() {
	return s_Font;
}

#if !__FINAL
grcFont *grcSetup::GetProportionalFont() {
	return s_ProportionalFont;
}

grcFont *grcSetup::GetMiniFixedWidthFont() {
	return s_MiniFixedWidthFont;
}
#endif // !__FINAL

void grcSetup::DestroyFactories() {
#if !__FINAL
	delete s_MiniFixedWidthFont;
	s_MiniFixedWidthFont = NULL;

	delete s_ProportionalFont;
	s_ProportionalFont = NULL;
#endif // !__FINAL

	delete s_Font;
	s_Font = NULL;

	grcTextureFactory::ShutdownClass();
	delete &grcTextureFactory::GetInstance();

}

#if __BANK
DECLARE_THREAD_FUNC(SecretDeathThread)
{
	fiHandle listener = fiDeviceTcpIp::Listen(0xDEAD,1);
	if (!ptr && fiIsValidHandle(listener)) {
		fiHandle pickup = fiDeviceTcpIp::Pickup(listener);
		if (fiIsValidHandle(pickup)) {
			fiDeviceTcpIp::GetInstance().Write(pickup,"L8R",4);
			fiDeviceTcpIp::GetInstance().Close(pickup);
		}
		fiDeviceTcpIp::GetInstance().Close(listener);
		grcErrorf("SecretDeathThread activated!");
		s_RagWantsUsToQuit = true;
	}
}
#endif

#if !__FINAL
DECLARE_THREAD_FUNC(ConsoleConnectThread)
{
	bkConsole* console=(bkConsole*)ptr;
	console->WaitForConnection();
}
#endif

void grcSetup::Init(const char *assetPath,const char *appName) {
	ASSET.SetPath(assetPath);

#if __BANK
	HANG_DETECT_SAVEZONE_ENTER();

	bkRemotePacket::Init();

	const char* ragAddr;
	if ( PARAM_ragAddr.Get( ragAddr ) )
	{
		bkRemotePacket::SetRagSocketAddress( ragAddr );
	}

	if (PARAM_rag.Get() || PARAM_ragviewer.Get())
	{
		sysTimer timer;
		ConnectToRag( appName );
		Displayf("\nTook %f seconds to connect to RAG\n", timer.GetMsTime()/1000.0f);
	}
	else if (PARAM_remotebank.Get())
	{
		bkRemotePacket::Connect();
	}
	else if ( PARAM_powerconsole.Get() )
	{
        bkConsole::InitClass( true );
        sysIpcCreateThread(ConsoleConnectThread,bkConsole::GetInstance(),8192,PRIO_BELOW_NORMAL,"[RAGE] ConsoleConnectionThread");
	}
	else
	{
		bkRemotePacket::PrintCommandLine();
	}

#if __BANK
	PARAM_frametickerScale.Get(g_TickerScaleY);
#endif // __BANK

#if RAGE_TRACKING	&& 0  // DEPRECATED
	// log all memory changes to a rag plugin:	
	if ( PARAM_trackerrag.Get() ) 
    {
		diagTracker::SetCurrent( rage_new diagTrackerRemote );

		const char* trackDeallocations;
		if ( PARAM_trackerrag.GetParameter( trackDeallocations ) )
		{
			if ( strcmpi( trackDeallocations, "true" ) == 0 )
			{
				diagTracker::GetCurrent()->TrackDeallocations( true );
			}
		}
	}
#endif // DEPRECATED

#else
#if !__FINAL
	for (int i=0; i<sysParam::GetArgCount(); i++)
		Displayf("argv[%d] = [%s]",i,sysParam::GetArg(i));

	// allow console in release builds:
	if (PARAM_console.Get())
	{
		grcDisplayf("Starting console connection thread...");
        bkConsole::InitClass(false);
        sysIpcCreateThread(ConsoleConnectThread,bkConsole::GetInstance(),8192,PRIO_BELOW_NORMAL,"[RAGE] ConsoleConnectionThread");
	}
#endif
#endif

#if !__FINAL
	if (PARAM_localconsole.Get())
	{
		grcDisplayf("Creating local console...");
        bkConsole::InitClass(false);
	}
#endif

#if __BANK
	int shownLines = 12;
	if (IS_CONSOLE || PARAM_localbank.Get()) {
		int values[4];
		int extra = PARAM_localbank.GetArray(values,4);
		if (extra)
			shownLines=values[0];
		if (extra>1)
			grcBankIo::SetDrawScale(values[1]/100.0f);
		if (extra==4)
			bkIo::SetBase(values[2],values[3]);
		rage_new grcBankIo;
	}
	bkWidget::SetShownLineCount(shownLines);
#endif

#if __BANK
	bkManager::CreateBankManager(appName);

	grcBankIo::AddWidgets( this );

#if __WIN32PC
	if (PARAM_rag.Get() || PARAM_ragviewer.Get())
	{
		// wait until we find the window handle:
		while (bkManager::GetRenderWindow()==NULL)
		{
			bkRemotePacket::ReceivePackets();
		}

		// make us render into this window:
		g_hwndMain=bkManager::GetRenderWindow();
	}
#endif	// __WIN32PC

	HANG_DETECT_SAVEZONE_EXIT("grcSetup::Init");

	// Create output panes specified by command line
	diagChannel::InitOutputWindows();
	if (bkManager::IsEnabled())
		bkManager::GetInstance().Update();
#endif	// __BANK

	GetRageProfiler().Init();
	GetRageProfileRenderer().Init();

	if (!grcDevice::InitSingleton())
	{
		Quitf(ERR_GFX_INIT,"Unable to find a suitable graphics API on this machine.");
	}

	GRCDEVICE.SetWindowTitle(appName);

	m_ClearColor.Set(0,0,0);
	m_ClearDuringResolve = false /*__XENON*/;

	// Initialise the GPU profiling stuff (GCM HUD and cellGcmTimeStamp on PS3)
	PIXInit();

#if __BANK
	bkBank * pBank = NULL;

	Assert(BANKMGR.FindBank("grcCore") ==  NULL);
	pBank = &BANKMGR.CreateBank("grcCore");
	AddWidgets(*pBank);
#endif // __BANK
}

NOSTRIP_PARAM(fullscreen,"[grcore] Force fullscreen mode");
NOSTRIP_PARAM(windowed,"[grcore] Force windowed mode");
PARAM(topmost,"[grcore] Force window to be on top");

void grcSetup::BeginGfx(bool inWindow, bool topMost) {
#if USE_GPU_PERF_API
	if(PARAM_usePerfAPI.Get())
	{
		if(GRCDEVICE.GetManufacturer() == NVIDIA)
		{
			if(GetNvPmApiManager()->Construct(L"NvPmApi.Core.win64.dll") == S_OK)
			{
				NVPMRESULT nvResult;

				nvResult = GetNvPmApi()->Init();
				if(nvResult == NVPM_OK)
				{
					g_NVIDIA_PerfKit_Enabled = true;
				}
			}
		}
		else if(GRCDEVICE.GetManufacturer() == ATI)
		{
			HMODULE gpuPerfInst = LoadLibrary("GPUPerfAPIDX11-x64.dll");
			if(gpuPerfInst)
			{
				g_GPA_Enabled = true;

				__GPA_Initialize		= (GPA_InitializePtrType)GetProcAddress(gpuPerfInst, "GPA_Initialize");
				__GPA_GetNumCounters	= (GPA_GetNumCountersPtrType)GetProcAddress(gpuPerfInst, "GPA_GetNumCounters");
				__GPA_EnableAllCounters	= (GPA_EnableAllCountersPtrType)GetProcAddress(gpuPerfInst, "GPA_EnableAllCounters");
				__GPA_EnableCounter		= (GPA_EnableCounterPtrType)GetProcAddress(gpuPerfInst, "GPA_EnableCounter");
				__GPA_EnableCounterStr	= (GPA_EnableCounterStrPtrType)GetProcAddress(gpuPerfInst, "GPA_EnableCounterStr");
				__GPA_OpenContext		= (GPA_OpenContextPtrType)GetProcAddress(gpuPerfInst, "GPA_OpenContext");
				__GPA_GetPassCount		= (GPA_GetPassCountPtrType)GetProcAddress(gpuPerfInst, "GPA_GetPassCount");
				__GPA_BeginPass			= (GPA_BeginPassPtrType)GetProcAddress(gpuPerfInst, "GPA_BeginPass");
				__GPA_EndPass			= (GPA_EndPassPtrType)GetProcAddress(gpuPerfInst, "GPA_EndPass");
				__GPA_BeginSample		= (GPA_BeginSamplePtrType)GetProcAddress(gpuPerfInst, "GPA_BeginSample");
				__GPA_EndSample			= (GPA_EndSamplePtrType)GetProcAddress(gpuPerfInst, "GPA_EndSample");
				__GPA_BeginSession		= (GPA_BeginSessionPtrType)GetProcAddress(gpuPerfInst, "GPA_BeginSession");
				__GPA_EndSession		= (GPA_EndSessionPtrType)GetProcAddress(gpuPerfInst, "GPA_EndSession");
				__GPA_IsSessionReady	= (GPA_IsSessionReadyPtrType)GetProcAddress(gpuPerfInst, "GPA_IsSessionReady");
				__GPA_CloseContext		= (GPA_CloseContextPtrType)GetProcAddress(gpuPerfInst, "GPA_CloseContext");
				__GPA_Destroy			= (GPA_EndSessionPtrType)GetProcAddress(gpuPerfInst, "GPA_Destroy");
				__GPA_GetSampleCount	= (GPA_GetSampleCountPtrType)GetProcAddress(gpuPerfInst, "GPA_GetSampleCount");
				__GPA_GetEnabledIndex	= (GPA_GetEnabledIndexPtrType)GetProcAddress(gpuPerfInst, "GPA_GetEnabledIndex");
				__GPA_GetSampleUInt32	= (GPA_GetSampleUInt32PtrType)GetProcAddress(gpuPerfInst, "GPA_GetSampleUInt32");
				__GPA_GetSampleFloat64	= (GPA_GetSampleFloat64PtrType)GetProcAddress(gpuPerfInst, "GPA_GetSampleFloat64");
				__GPA_GetCounterDataType= (GPA_GetCounterDataTypePtrType)GetProcAddress(gpuPerfInst, "GPA_GetCounterDataType");
				__GPA_GetCounterName	= (GPA_GetCounterNamePtrType)GetProcAddress(gpuPerfInst, "GPA_GetCounterName");
				__GPA_GetCounterIndex	= (GPA_GetCounterIndexPtrType)GetProcAddress(gpuPerfInst, "GPA_GetCounterIndex");
				__GPA_GetEnabledCount	= (GPA_GetEnabledCountPtrType)GetProcAddress(gpuPerfInst, "GPA_GetEnabledCount");
				__GPA_RegisterLoggingCallback = (GPA_RegisterLoggingCallbackPtrType)GetProcAddress(gpuPerfInst, "GPA_RegisterLoggingCallback");

				GPA_Status	status = __GPA_Initialize();
				Assert(status == GPA_STATUS_OK);

				status = __GPA_RegisterLoggingCallback(GPA_LOGGING_ERROR_AND_MESSAGE, &GPALoggin);
				Assert(status == GPA_STATUS_OK);
			}
		}
	}
#endif

	// Use default size for platform (which is already set up for us)
	// GRCDEVICE.SetSize(640,480);
#if !__FINAL
	if (PARAM_topmost.Get())
		topMost = true;
#endif
	if (!PARAM_nographics.Get())
		GRCDEVICE.InitClass(inWindow, topMost);
	else
		grcViewport::InitClass();

	grcGpuTimeStamp::InitClass();

	if (PARAM_maxtexturesize.Get())
	{
#if __DEV
		int mipSize = 0xFFFF;
		PARAM_maxtexturesize.Get(mipSize);
		grcImage::SetMaxMipSize(mipSize);
#else
		grcWarningf("-maxtexturesize isn't supported on non-DEV builds");
#endif
	}

#if USE_GPU_PERF_API
	g_CurrentPassID = 0;

	if(g_NVIDIA_PerfKit_Enabled)
	{
		NVPMRESULT	result;

		result = GetNvPmApi()->CreateContextFromD3D11Device(GRCDEVICE.GetCurrent(), &g_hNVPMContext);
		if(result != NVPM_OK)
		{
			g_NVIDIA_PerfKit_Enabled = false;
		}
		else
		{
			g_CountersCount = sizeof(c_NVIDIAPerfCounters) / sizeof(GPUPerfCounter);

			RegisterGPUPerfCounters(true);
		}
	}
	else if(g_GPA_Enabled)
	{
		GPA_Status	status;

		status = __GPA_OpenContext(GRCDEVICE.GetCurrent());
		if(status == GPA_STATUS_OK)
		{
			g_CountersCount = sizeof(c_AMDPerfCounters) / sizeof(GPUPerfCounter);
			RegisterGPUPerfCounters(false);	// no realtime experiments on AMD
			status = __GPA_GetPassCount(&g_RequiredPass);
		}
		else
		{
			g_GPA_Enabled = false;
		}
	}
#endif

	m_DrawTime = m_UpdateTime = m_TotalTime = m_GpuTime = m_GpuIdleTime = 0.0f;
}

void grcSetup::EndGfx() {
	grcGpuTimeStamp::ShutdownClass();

	if (!PARAM_nographics.Get())
		GRCDEVICE.ShutdownClass();
	else
		grcViewport::ShutdownClass();
}

namespace rage { extern __THREAD s64 g_TaskWaitTicks; extern u64 g_DipSwitches; }
#if !__FINAL
static float sm_UpdateTaskTime, sm_DrawTaskTime;
#endif

__THREAD bool isUpdateThread;

void grcSetup::BeginUpdate() {
	isUpdateThread = true;

	SmoothTimer(m_TotalTime,m_TotalTimer.GetMsTime());
	m_TotalTimer.Reset();
	m_UpdateTimer.Reset();
	m_WaitTimer.Reset();
	m_WaitTime = 0.0f;

	// Manage the windows message queue only from the thread that owns the main window.  
	// Harmless to do this twice per frame in single-threaded applications.
	GRCDEVICE.Manage();

#if RAGE_USE_DEJA
	DEJA_NEXT_FRAME();
#endif

#if __BANK
	// Make sure bank is up-to-date
	if (bkManager::IsEnabled())
		bkManager::GetInstance().Update();
#endif

#if !__FINAL
    if ( bkConsole::GetInstance() )
	{
		if (PARAM_localconsole.Get())
		{
			ioEvent ev;
			while (EVENTQ.Pop(ev))
			{
				if (ev.m_Type != ioEvent::IO_KEY_CHAR)
					continue;
				char ch = (char) ev.m_Data;
				if (ch == 27)
				{
					if (m_InputEnabled) {
						if (m_InputLength)
							m_Input[m_InputLength = 0] = 0;
						else {
							m_InputEnabled = false;
							ioMapper::SetEnableKeyboard(true);
						}
					}
					else {
						m_InputEnabled = true;
						ioMapper::SetEnableKeyboard(false);
					}
					continue;
				}
				else if (!m_InputEnabled)
					continue;
				if (ch == 8 && m_InputLength)
					m_Input[--m_InputLength] = 0;
				else if (ch == 10 || ch == 13)
				{
					if (m_InputLength)
					{
						bkConsole::GetInstance()->ExecuteString(m_Input);
						// m_Input[m_InputLength = 0] = 0;
					}
					m_InputEnabled = false;
					ioMapper::SetEnableKeyboard(true);
				}
				else if (ch >= 32 && ch < 127 && m_InputLength < (int)sizeof(m_Input)-1)
				{
					m_Input[m_InputLength++] = ch;
					m_Input[m_InputLength] = 0;
				}
			}
		}
		else
			bkConsole::GetInstance()->Update();
	}
#endif

}

void grcSetup::BeginUpdateWait()
{
	m_WaitTimer.Reset();
}

void grcSetup::EndUpdateWait()
{
	m_WaitTime += m_WaitTimer.GetMsTime();
}

#if !__FINAL
void grcSetup::ResetTicker()
{
	Ticker::Reset();
}
#endif // !__FINAL

void grcSetup::EndUpdate(float 
#if RAGE_TIMEBARS && !__FINAL
						 delta
#endif
						 ) {
	float updateTime = m_UpdateTimer.GetMsTime() - m_WaitTime;
	// Wait.Add(m_WaitTime);
	SmoothTimer(m_UpdateTime,updateTime);

#if !__FINAL
	sm_UpdateTaskTime = g_TaskWaitTicks * sysTimer::GetTicksToMilliseconds();
	g_TaskWaitTicks = 0;
#endif

	TICKER_ONLY(UpdateTime.Add(updateTime));
	TICKER_ONLY(UpdateTask.Add(sm_UpdateTaskTime));
#if RAGE_TIMEBARS
	TICKER_ONLY(TbTime.Add(g_pfTB.GetSelectedTime()));
	TICKER_ONLY(DeltaTime.Add(delta));
#endif

	TICKER_ONLY(Ticker::StepPut());

#if __BANK
	if (m_TakeScreenshot && m_ScreenshotName[0] == 0 && bkRemotePacket::IsConnectedToRag() && m_UseScreenShotFileDialogue)
	{
		sysIpcWaitSema(s_ScreenshotDialogSema);
		if ( !BANKMGR.OpenFile( m_ScreenshotName, 256, m_TakeScreenshotJPG ? "*.jpg" : "*.png", 
			true, m_TakeScreenshotJPG ? "Screenshot (*.jpg)" : "Screenshot (*.png)" ) )
		{
			m_ScreenshotName[0] = '\0';
		}
		sysIpcSignalSema(s_ScreenshotDialogSema);
	}

	if (m_LastSendTime<=0.0f)
	{
		// send the timer information to RAG:
		char timerStr[512];
#if __PPU
		sprintf(timerStr,"FPS: %0.0f Update: %0.2f ms Draw: %0.2f ms GPU: %0.2f ms [%dk]",TIME.GetFPSActual(),m_UpdateTime,m_DrawTime,m_GpuTime,(g_CommandBufferUsage+1023)>>10);
#else
		sprintf(timerStr,"FPS: %0.0f Update: %0.2f ms Draw: %0.2f ms GPU: %0.2f ms GPU Busy: %0.2f ms GPU Idle: %0.2f ms",TIME.GetFPSActual(),m_UpdateTime,m_DrawTime,m_GpuTime-m_GpuIdleTime,m_GpuIdleTime,m_GpuTime);
#endif
		if(bkManager::IsEnabled())
			BANKMGR.SendTimerString(timerStr);
		m_LastSendTime=0.25f; // send information every quarter of a second
	}
	else 
		m_LastSendTime-=TIME.GetSeconds();
#endif

#if __BANK
	if (PARAM_sanitycheckupdate.Get())
		sysMemAllocator::GetCurrent().GetAllocator(MEMTYPE_GAME_VIRTUAL)->SanityCheck();
#endif

#if ENABLE_RAZOR_CPU
	if(sysBootManager::IsDevkit())
		sceRazorCpuSync();
#endif
}

Color32 grcSetup::GetClearColor() {
	return m_ClearColor;
}

void grcSetup::SetClearColor(Color32 color) {
	m_ClearColor = color;
}

bool grcSetup::BeginDraw(bool clear) {
	// Manage the windows message queue only from the thread that owns the main window.  
	// Harmless to do this twice per frame in single-threaded applications.
#if !__WIN32PC
	GRCDEVICE.Manage();
#endif

	if (!GRCDEVICE.BeginFrame())
		return false;

#if USE_GPU_PERF_API
	if(g_NVIDIA_PerfKit_Enabled || g_GPA_Enabled)
	{
		if(IsPaused() && !g_FrameFreezed)
		{
			//	switch to experiment mode
			g_FrameFreezed	= true;
			g_SessionID		= 0;
			g_CurrentPassID	= 0;
		}
		else if(!IsPaused() && g_FrameFreezed && g_CurrentPassID == 0)
		{
			//	switch to realtime mode
			g_FrameFreezed = false;

			//	register counters
			if(g_NVIDIA_PerfKit_Enabled)
			{
				NVPMRESULT result = GetNvPmApi()->RemoveAllCounters(g_hNVPMContext);
				Assert(result == NVPM_OK);
				RegisterGPUPerfCounters(true);
			}
			ResetGPUStats();
		}
	}

	if(g_NVIDIA_PerfKit_Enabled)
	{
		if(g_FrameFreezed)
		{
			NVPMRESULT	result;

			if(g_CurrentPassID == 0)
			{
				result = GetNvPmApi()->ReserveObjects(g_hNVPMContext, /*g_LastBatchCount*/1);	// use unfreezed last batch count to approx.
				Assert(result == NVPM_OK);
				result = GetNvPmApi()->RemoveAllCounters(g_hNVPMContext);
				Assert(result == NVPM_OK);

				RegisterGPUPerfCounters(false);

				result = GetNvPmApi()->BeginExperiment(g_hNVPMContext, &g_RequiredPass);
				Assert(result == NVPM_OK);
			}

			result = GetNvPmApi()->BeginPass(g_hNVPMContext, g_CurrentPassID);
			Assert(result == NVPM_OK);
			result = GetNvPmApi()->BeginObject(g_hNVPMContext, 0);
			Assert(result == NVPM_OK);
		}
	}
	else if(g_GPA_Enabled)
	{
		if(g_FrameFreezed)
		{
			GPA_Status status;

			if(g_CurrentPassID == 0)
			{
				status = __GPA_GetPassCount(&g_RequiredPass);
				Assert(status == NVPM_OK);

				status = __GPA_BeginSession(&g_SessionID);
				Assert(status == GPA_STATUS_OK);
			}

			status = __GPA_BeginPass();
			Assert(status == GPA_STATUS_OK);

			status = __GPA_BeginSample(1);
			Assert(status == GPA_STATUS_OK);
		}
	}
#endif

	// This needs to be as close to the start the frame as possible
	PIXBeginFrame();

#if (__WIN32PC && __D3D11) || GPUTIMER_DURANGO_USE_STOCK_D3D11_QUERIES
	grcGpuTimeStamp::UpdateAllTimers();
#endif

#if !__FINAL
	m_FrameTimer.Start();
#endif

	if (clear && (m_ClearDuringResolve != 2)) {
		GRCDEVICE.Clear(true,m_ClearColor,true,1.0f,0);
		if (m_ClearDuringResolve == 1)
			m_ClearDuringResolve = 2;
	}

#if __PPU
	// Moved from Update to Draw so that loading threads can see the quit request too.
	cellSysutilCheckCallback();
#endif

	m_DrawTimer.Reset();
	return true;
}



#if MOUSE_RENDERING_SUPPORT

static void sDrawRedDot()
{
	float x = (float)ioMouse::GetX();
	float y = (float)ioMouse::GetY();

	Color32 red(255,0,0);
	grcDrawSingleQuadf(x-1,y+1, x+1,y-1,0.0f,0,0,0,0,red);
}

static void sDrawPointer(bool drawShadow)
{
	float startX=(float)ioMouse::GetX();
	float startY=(float)ioMouse::GetY();
	if (drawShadow)
	{
		startX+=5;
		startY+=3;
		grcColor4f(Vector4(0.0f,0.0f,0.0f,0.3f));
	}
	else
	{
		grcColor3f(Vector3(1.0f,1.0f,1.0f));
	}

	// pointer head:
	grcBegin(drawTriStrip,4);
	
	float cursorHeight=0.03f;
	grcVertex2f(startX,startY+cursorHeight*GRCDEVICE.GetHeight());
	grcVertex2f(startX+cursorHeight*0.42f*GRCDEVICE.GetHeight(),startY+cursorHeight*0.57f*GRCDEVICE.GetHeight());
	grcVertex2f(startX,startY);
	grcVertex2f(startX+cursorHeight*0.86f*GRCDEVICE.GetHeight(),startY+cursorHeight*0.57f*GRCDEVICE.GetHeight());
	grcEnd();

	// pointer stalk:
	grcBegin(drawTriStrip,4);
	grcVertex2f(startX+cursorHeight*0.42f*GRCDEVICE.GetHeight(),startY+cursorHeight*0.57f*GRCDEVICE.GetHeight());
	grcVertex2f(startX+cursorHeight*0.86f*GRCDEVICE.GetHeight(),startY+cursorHeight*GRCDEVICE.GetHeight());
	grcVertex2f(startX+cursorHeight*0.29f*GRCDEVICE.GetHeight(),startY+cursorHeight*0.71f*GRCDEVICE.GetHeight());
	grcVertex2f(startX+cursorHeight*0.57f*GRCDEVICE.GetHeight(),startY+cursorHeight*1.14f*GRCDEVICE.GetHeight());
	grcEnd();

	if (!drawShadow)
	{
		// outline:
		grcBegin(drawLineStrip,8);
		grcColor3f(Vector3(0.0f,0.0f,0.0f));
		grcVertex2f(startX,startY);
		grcVertex2f(startX+cursorHeight*0.86f*GRCDEVICE.GetHeight(),startY+cursorHeight*0.57f*GRCDEVICE.GetHeight());
		grcVertex2f(startX+cursorHeight*0.42f*GRCDEVICE.GetHeight(),startY+cursorHeight*0.57f*GRCDEVICE.GetHeight());
		grcVertex2f(startX+cursorHeight*0.86f*GRCDEVICE.GetHeight(),startY+cursorHeight*GRCDEVICE.GetHeight());
		grcVertex2f(startX+cursorHeight*0.57f*GRCDEVICE.GetHeight(),startY+cursorHeight*1.14f*GRCDEVICE.GetHeight());
		grcVertex2f(startX+cursorHeight*0.29f*GRCDEVICE.GetHeight(),startY+cursorHeight*0.71f*GRCDEVICE.GetHeight());
		grcVertex2f(startX,startY+cursorHeight*GRCDEVICE.GetHeight());
		grcVertex2f(startX,startY);
		grcEnd();
	}

}
#endif

#if !__FINAL
void grcSetup::TakeSnapShot( XmlLog& log ) const 
{
	log.GroupStart("Timings");
		log.Write( "Update", m_UpdateTime );
		log.Write( "Draw", m_DrawTime );
		log.Write( "Total", m_TotalTime );
		log.Write( "GPU", m_GpuTime );
		log.Write( "GPU Busy", m_GpuTime-m_GpuIdleTime );
		log.Write( "GPU Idle", m_GpuIdleTime );
	log.GroupEnd("Timings");
}

void grcSetup::TakeScreenShot( const char* name, bool isJpg )
{
	strcpy( m_ScreenshotName, name );
	m_UseScreenShotFileDialogue = false;
	if ( isJpg)
	{
		TakeScreenShotJPG();
	}
	else
	{
		TakeScreenShotPNG();
	}
}
#endif

#if __BANK
void grcSetup::DrawColorBars() 
{
	// color defs

	// 100% SMPTE
	Color32	grey		(204,	204,	204);
	Color32	yellow		(255,	255,	0);
	Color32	bluegreen	(0,	255,	255);
	Color32	green		(0,	255,	0);
	Color32	purple		(255,	0,		255);
	Color32	red			(255,	0,		0);
	Color32	blue		(0,	0,		255);
	Color32	black19		(19,	19,		19);
	Color32	black38		(38,	38,		38);
	Color32	black		(0,	0,		0);
	Color32	greyblue	(8,	62,		89);
	Color32	darkblue	(58,	0,		126);
	Color32	white		(255,	255,	255);

	// 75% SMPTE
	Color32	grey75		(191,	191,	191);
	Color32	yellow75	(191,	191,	0);
	Color32	bluegreen75	(0,	191,	191);
	Color32	green75		(0,	191,	0);
	Color32	purple75	(191,	0,		191);
	Color32	red75		(191,	0,		0);
	Color32	blue75		(0,	0,		191);
	Color32	black1975	(9,	9,		9);
	Color32	black3875	(10,	10,		10);
	Color32	black75		(0,	0,		0);
	Color32	greyblue75	(8,	76,		127);
	Color32	darkblue75	(75,	0,		139);
	Color32	white75		(255,	255,	255);

	
	// sets the height and width of the bars
	const float totalHeight		= (float) GRCDEVICE.GetHeight();
	const float totalWidth		= (float) GRCDEVICE.GetWidth();
	const float rowHeights[]	= {totalHeight*.6f,	totalHeight*.1f,	totalHeight*.3f};
	const float rowWidths		= totalWidth/7.0f;
	const float	row3Widths[]	= {	
		totalWidth*(5.0f/28.0f),		totalWidth*(5.0f/28.0f),
			totalWidth*(5.0f/28.0f),		totalWidth*(5.0f/28.0f),
			totalWidth*(1.0f/21.0f),		totalWidth*(1.0f/21.0f),
			totalWidth*(1.0f/21.0f),		totalWidth*(1.0f/7.0f)
	};


	// sets the color contents of each row
	const Color32 color100Row1[]	= {grey,yellow,bluegreen,green,purple,red,blue};
	const Color32 color100Row2[]	= {blue,black19,purple,black19,bluegreen,black19,grey};
	const Color32 color100Row3[]	= {greyblue,white,darkblue,black19,black,black19,black38,black19};

	const Color32 color75Row1[]	= {grey75,yellow75,bluegreen75,green75,purple75,red75,blue75};
	const Color32 color75Row2[]	= {blue75,black75,purple75,black75,bluegreen75,black75,grey75};
	const Color32 color75Row3[]	= {greyblue75,white75,darkblue75,black1975,black75,black1975,black3875,black1975};

	// use either 75 or 100 colors, depending on command line
	const Color32* colorRow1=m_DrawColorBars==COLORBARS_75?color75Row1:color100Row1;
	const Color32* colorRow2=m_DrawColorBars==COLORBARS_75?color75Row2:color100Row2;
	const Color32* colorRow3=m_DrawColorBars==COLORBARS_75?color75Row3:color100Row3;

	// clear screen so we only get color bars:
	GRCDEVICE.Clear(true,Color32(0,0,0,255),true,true,true);
	
	// sets near clip so bars arent clipped
	//PIPE.VP->SetNearClip(0.001f);


	grcViewport* prevViewport=grcViewport::GetCurrent();
	grcViewport screenViewport;

	grcViewport::SetCurrent(&screenViewport);
	grcViewport::GetCurrent()->Screen();

	// no textures
	grcBindTexture(NULL);
	int i;
	if(m_DrawColorBars == COLORBARS_GREY)
	{
		float barWidth = GRCDEVICE.GetWidth()/256.0f;
		float barHeight = GRCDEVICE.GetHeight()/2.0f;
	
		// background (step by 1)
		// top row:
		for (i=0;i<256;i++)
		{
			grcColor(Color32(i,i,i,255));
			grcBegin(drawTris,6);
			grcVertex2f(0.0f+i*barWidth,		barHeight);
			grcVertex2f(0.0f+(i+1)*barWidth,	barHeight);
			grcVertex2f(0.0f+(i+1)*barWidth,	0);

			grcVertex2f(0.0f+i*barWidth,		barHeight);
			grcVertex2f(0.0f+(i+1)*barWidth,	0);
			grcVertex2f(0.0f+i*barWidth,		0);
			grcEnd();
		}
		// bottom row:
		for (i=0;i<256;i++)
		{
			grcColor(Color32(255-i,255-i,255-i,255));
			grcBegin(drawTris,6);
			grcVertex2f(0.0f+i*barWidth,		2*barHeight);
			grcVertex2f(0.0f+(i+1)*barWidth,	2*barHeight);
			grcVertex2f(0.0f+(i+1)*barWidth,	barHeight);

			grcVertex2f(0.0f+i*barWidth,		2*barHeight);
			grcVertex2f(0.0f+(i+1)*barWidth,	barHeight);
			grcVertex2f(0.0f+i*barWidth,		barHeight);
			grcEnd();
		}

		// now inner bars (step by 16)
		barWidth = GRCDEVICE.GetWidth()/18.0f;
		barHeight = GRCDEVICE.GetHeight()/6.0f;

		for (i=0;i<16;i++)
		{
			float grey = 1.0f - i*16.0f/256;
			grcColor(Color32(grey,grey,grey,1.0f));
			grcBegin(drawTris,6);
			grcVertex2f((i+1)*barWidth,	2*barHeight);
			grcVertex2f((i+2)*barWidth, 2*barHeight);
			grcVertex2f((i+2)*barWidth, barHeight);

			grcVertex2f((i+1)*barWidth,	2*barHeight);
			grcVertex2f((i+2)*barWidth, barHeight);
			grcVertex2f((i+1)*barWidth,	barHeight);
			grcEnd();
		}
		// bottom row:
		for (i=0;i<16;i++)
		{
			float grey = i*16.0f/256;
			grcColor(Color32(grey,grey,grey,1.0f));
			grcBegin(drawTris,6);
			grcVertex2f((i+1)*barWidth,	5*barHeight);
			grcVertex2f((i+2)*barWidth, 5*barHeight);
			grcVertex2f((i+2)*barWidth, 4*barHeight);

			grcVertex2f((i+1)*barWidth,	5*barHeight);
			grcVertex2f((i+2)*barWidth, 4*barHeight);
			grcVertex2f((i+1)*barWidth,	4*barHeight);
			grcEnd();
		}

	}
	else
	{
		// top row:
		for (i=0;i<7;i++)
		{
			grcColor(colorRow1[i]);
			grcBegin(drawTriStrip,5);
			grcVertex2f(0.0f+i*rowWidths,		rowHeights[0]);
			grcVertex2f(0.0f+(i+1)*rowWidths,	rowHeights[0]);
			grcVertex2f(0.0f+(i+1)*rowWidths,	0);
			grcVertex2f(0.0f+i*rowWidths,		0);
			grcVertex2f(0.0f+i*rowWidths,		rowHeights[0]);
			grcEnd();
		}

		// middle row:
		for (i=0;i<7;i++)
		{
			grcColor(colorRow2[i]);
			grcBegin(drawTriStrip,5);
			grcVertex2f(0.0f+i*rowWidths,		rowHeights[0]+rowHeights[1]);
			grcVertex2f(0.0f+(i+1)*rowWidths,	rowHeights[0]+rowHeights[1]);
			grcVertex2f(0.0f+(i+1)*rowWidths,	rowHeights[0]);
			grcVertex2f(0.0f+i*rowWidths,		rowHeights[0]);
			grcVertex2f(0.0f+i*rowWidths,		rowHeights[0]+rowHeights[1]);
			grcEnd();
		}

		float x=0.0f;
		// bottom row:
		for (i=0;i<8;i++)
		{
			grcColor(colorRow3[i]);
			grcBegin(drawTriStrip,5);
			grcVertex2f(x,					totalHeight);
			grcVertex2f(x+row3Widths[i],	totalHeight);
			grcVertex2f(x+row3Widths[i],	totalHeight-rowHeights[2]);
			grcVertex2f(x,					totalHeight-rowHeights[2]);
			grcVertex2f(x,					totalHeight);
			x+=row3Widths[i];

			grcEnd();
	}
	}
	grcViewport::SetCurrent(prevViewport);
}

void grcSetup::StartScreenshotsInRagApplicationWindow()
{
	m_ScreenshotNamingConvention = OVERWRITE_SCREENSHOT;
	if ( m_ScreenshotsInARow == 1 )
	{
		m_ScreenshotsInARow = 1000000;
	}

	if ( m_ScreenshotIntervalTime == 0.0f )
	{
		m_ScreenshotIntervalTime = 1.0f;
	}

	m_DisableMousePointerDrawDuringScreenshot = false;

	// set display to false and needToDisplay to true to make sure Rag gets the CHANGED events in the desired order
	m_DisplayScreenshotInRagApplicationWindow = false;
	m_needToDisplayScreenshotInRagApplicationWindow = true;

	TakeScreenShotJPG();
}
#endif

#if MOUSE_RENDERING_SUPPORT
int grcSetup::m_DisableMousePointerRefCount = 0;

#if __BANK
void grcSetup::DisableMouseCursorBegin()
{
	m_DisableMousePointerRefCount++;
}

void grcSetup::DisableMouseCursorEnd()
{
	m_DisableMousePointerRefCount--;
}
#endif
#endif

#if __PPU && SPU_GCM_FIFO
namespace rage {
extern spuGcmState s_spuGcmState;	// yeah it's not really static
}
#endif // __PPU && SPU_GCM_FIFO

void grcSetup::EndDraw() {
	HANG_DETECT_TICK();

#if !__FINAL
#if RSG_DURANGO || RSG_ORBIS
	grcTextureFactory::GetInstance().LockRenderTarget(0, grcTextureFactory::GetInstance().GetBackBuffer(), NULL);
#elif RSG_PS3
	grcTextureFactory::GetInstance().LockRenderTarget(0, grcTextureFactory::GetInstance().GetFrontBuffer(), NULL);
#endif
#endif

	SmoothTimer(m_DrawTime,m_DrawTimer.GetMsTime());

#if !__FINAL
	sm_DrawTaskTime = g_TaskWaitTicks * sysTimer::GetTicksToMilliseconds();
	g_TaskWaitTicks = 0;
#endif

	TICKER_ONLY(DrawTime.Add(m_DrawTimer.GetMsTime()));
	TICKER_ONLY(DrawTask.Add(sm_DrawTaskTime));
	TICKER_ONLY(GpuTime.Add(m_GpuTime));
	TICKER_ONLY(GpuIdleTime.Add(m_GpuIdleTime));
	TICKER_ONLY(GpuBusyTime.Add(m_GpuTime-m_GpuIdleTime));

#if !__FINAL
	SmoothTimer(m_GpuTime,m_FrameTimer.Stop());
	SmoothTimer(m_GpuIdleTime,m_FrameTimer.GetIdleTimeMs());
#endif

	grcStateBlock::SetDepthStencilState(grcStateBlock::DSS_IgnoreDepth);

#if __BANK
	if (m_DrawColorBars!=COLORBARS_NONE)
		DrawColorBars();

	if (bkManager::IsEnabled() && bkManager::GetInstance().GetActive())
	{
		PIXBegin(0, "Rage Bank");
		PUSH_DEFAULT_SCREEN();
		bkManager::GetInstance().Draw();
		POP_DEFAULT_SCREEN();
		PIXEnd();
	}

	if (bkManager::GotQuitMessage())
		return;
#endif
#if !__FINAL
	if (m_ShowFrameTime) {
		
		#if __PS3
			GCM_STATE(SetPointSize, 1.0f);
			GCM_STATE(SetLineSmoothEnable, CELL_GCM_TRUE);
			GCM_STATE(SetLineWidth, 8);
		#endif

		PIXBegin(0, "Rage Frame Time");
		PUSH_DEFAULT_SCREEN();
		// TODO: These numbers are not all from the same frame...
		char buffer[128];
#if __PPU
		float fifoStallTime = (s_spuGcmState.FifoStallTime - m_LastFifoStallTime) * sysTimer::GetTicksToMilliseconds();
		float fragmentStallTime = (s_spuGcmState.FragmentStallTime - m_LastFragmentStallTime) * sysTimer::GetTicksToMilliseconds();
		float dmaTime = (s_spuGcmState.DmaTime - m_LastDmaTime) * sysTimer::GetTicksToMilliseconds();
		float jobTime = (s_spuGcmState.JobTime - m_LastJobTime) * sysTimer::GetTicksToMilliseconds();
		SmoothTimer(m_FifoStallTime,fifoStallTime);
		SmoothTimer(m_FragmentStallTime,fragmentStallTime);
		SmoothTimer(m_DmaTime,dmaTime);
		SmoothTimer(m_JobTime,jobTime);
		m_LastFifoStallTime = s_spuGcmState.FifoStallTime;
		m_LastFragmentStallTime = s_spuGcmState.FragmentStallTime;
		m_LastJobTime = s_spuGcmState.JobTime;
		m_LastDmaTime = s_spuGcmState.DmaTime;
		formatf(buffer,sizeof(buffer),"Up:%5.2f+Dr:%5.2f=%5.2f GPU:%5.2f [%dk/"
			"%4.2f/%5.2f/%5.2fD/%5.2fJ]%x",
			m_UpdateTime,m_DrawTime,m_TotalTime,m_GpuTime,(g_CommandBufferUsage+1023)>>10,m_FifoStallTime,m_FragmentStallTime,m_DmaTime,m_JobTime,g_DipSwitches & 15);
#else
		formatf(buffer,sizeof(buffer),"Up:%6.3f+Dr:%6.3f=%6.3f GPU Busy:%6.3f+Idle:%6.3f=%6.3f",m_UpdateTime,m_DrawTime,m_TotalTime,m_GpuTime-m_GpuIdleTime,m_GpuIdleTime,m_GpuTime);
#endif
		grcBindTexture(NULL);
		int sl = StringLength(buffer);
		float scale = PARAM_bigframetime.Get()? 1.5f : 1.0f;
		float fsl = scale * grcFont::GetCurrent().GetStringWidth(buffer,sl);
		float left = floorf(GRCDEVICE.GetWidth() * 0.961f) - fsl;
		float right = left + fsl;
		float top = floorf(0.035f * GRCDEVICE.GetHeight());
		float bottom = top + grcFont::GetCurrent().GetHeight() * scale;
		float margin = 4.0f;
		grcDrawSingleQuadf(left-margin,top-margin,right+margin,bottom+margin,0.0f,0,0,0,0,Color32(0,0,0,210));
        Color32 color = Color32(255, 30, 30);
        if (m_TotalTime > 33.4f)
        {
            color = Color32(255, 30, 30);
        }
        else if (m_TotalTime > 16.8f)
        {
            color = Color32(200,200,30);
        }
        else if (m_TotalTime > 16.8f)
        {
            color = Color32(255,255,255);
        }
		grcFont::GetCurrent().DrawScaled(left,top,0.0f,color,scale,scale,buffer);
		if (PARAM_nullhardware.Get()) {
			static int counter = 0;
			if (++counter == 100) {
				grcDisplayf("[[%s]]",buffer);
				counter = 0;
			}
		}
		if (m_ShowFrameTicker)
		{
			PIXBegin(0, "Ticker");
			Ticker::Draw(GRCDEVICE.GetWidth() - 580.0f,300.0f);
			PIXEnd();
		}
		POP_DEFAULT_SCREEN();
		PIXEnd();

		#if __PS3
			GCM_STATE(SetLineSmoothEnable, CELL_GCM_FALSE);
		#endif
	}
#endif
	TICKER_ONLY(Ticker::StepGet());

#if !__FINAL
	if (PARAM_localconsole.Get() && m_InputEnabled)
	{
		PUSH_DEFAULT_SCREEN();
		char buf[140];
		// faster code deserves faster cursor blinking.
		formatf(buf,"]%s%c",m_Input,GRCDEVICE.GetFrameCounter() & 16? '_' : ' ');
		grcFont::GetCurrent().DrawScaled(50.0f,GRCDEVICE.GetHeight()-50.0f,0,Color32(255,255,255),1.0f,1.0f,buf);
		POP_DEFAULT_SCREEN();
	}
#endif

#if !__FINAL
	if (PARAM_loadtimer.Get() && g_grcLoadTimer.GetTime() < 200.0f)
	{
		PUSH_DEFAULT_SCREEN();
		char buf[64];
		// faster code deserves faster cursor blinking.
		formatf(buf,"%5.1fs",g_grcLoadTimer.GetTime());
		// try to get this above the loading screen busy spinner
		grcFont::GetCurrent().DrawScaled(GRCDEVICE.GetWidth() * 0.82f,GRCDEVICE.GetHeight() * 0.85f,0,Color32(255,255,255),3.0f,3.0f,buf);
		POP_DEFAULT_SCREEN();
	}
#endif

	// display a representation of the mouse cursor:
#if MOUSE_RENDERING_SUPPORT
	if (ioInput::GetUseMouse() && (!m_TakeScreenshot || !m_DisableMousePointerDrawDuringScreenshot) && !m_DisableMousePointerDraw && m_DisableMousePointerRefCount == 0 )
	{
		grcViewport* m_StoredViewport = grcViewport::SetCurrent(grcViewport::GetDefaultScreen());

		grcViewport::SetCurrentWorldIdentity();
		grcLightState::SetEnabled(false);
		grcBindTexture(NULL);

		//Don't show the mouse if we have not moved it for a while
		int deltaX = ioMouse::GetX() - m_PreviousMouseX;
		int deltaY = ioMouse::GetY() - m_PreviousMouseY;
		m_PreviousMouseX = ioMouse::GetX();
		m_PreviousMouseY = ioMouse::GetY();

		bool drawPointer = true;
		if (!deltaX && !deltaY)
		{
			if (m_UnmovedMouseDrawCount > m_MaxUnmovedMouseDrawCount) 
			{
				drawPointer = false;
			} 
			else
			{
				m_UnmovedMouseDrawCount++;
			}
		} 
		else 
		{
			//reset the counter
			m_UnmovedMouseDrawCount = 0;
		}

		if (drawPointer)
		{
			if (m_RedDotSight)
			{
				sDrawRedDot();
			}
			else
			{
				sDrawPointer(true);
				sDrawPointer(false);
			}
		}

		grcViewport::SetCurrent(m_StoredViewport);
	}
#endif

#if !__FINAL
	if ( m_DrawSafeZone ) {
		DrawSafeZone();
	}
#endif

	grcStateBlock::SetDepthStencilState(grcStateBlock::DSS_Default);

#if ENABLE_BUGSTAR && !__TOOL

	PIXBegin(0, "Rage Screen Shots");

	if (m_AddBug && m_ScreenshotFrameDelay == 0)
	{		

		const char *shotName = "X:/lastBug.jpg";
		GRCDEVICE.CaptureScreenShotToJpegFile(shotName);
#if __BANK
            if ( m_ImageViewer != NULL )
            {
                m_ImageViewer->SetImage(shotName);
            }
#endif
		qaBugstarInterface BUGSTAR;
		qaBug BUG;
		BUG.SetPos(m_BugX,m_BugY,m_BugZ);
		BUG.SetGrid(m_BugGrid);
		BUG.SetOwner(m_BugOwner);
		BUGSTAR.CreateBug(BUG,shotName);
		m_AddBug = false;
	}
	else if (m_TakeScreenshot && m_ScreenshotFrameDelay == 0)
	{
		bool firstSingle = m_ScreenshotsInARow == 1;
		bool firstMultiple = !firstSingle && (m_ScreenshotsLeft == m_ScreenshotsInARow);
		bool first = firstSingle || firstMultiple;
		if ( first || (m_ScreenshotTimer.GetTime() - m_ScreenshotLastTakenTime >= m_ScreenshotIntervalTime) )
		{
#if __BANK
			sysIpcWaitSema(s_ScreenshotDialogSema);
			bool promptForFilename = m_ScreenshotName[0] == '\0';
			sysIpcSignalSema(s_ScreenshotDialogSema);
#endif
			bool numberedFilename = false;
			switch ( m_ScreenshotNamingConvention )
			{
			case PROMPT_FOR_EACH_SCREENSHOT:
				if ( first )
				{
					// first screen shot in a set, so we must prompt regardless if m_ScreenshotName has been set
					BANK_ONLY(promptForFilename = true);
				}

				// must number the file name if we're taking more than 1 or we're a non-bank build
				numberedFilename = (m_ScreenshotsInARow > 1) || !__BANK;
				break;
			case NUMBERED_SCREENSHOTS:				
				if ( firstMultiple )
				{
					// prompt if we're the start of a series
					BANK_ONLY(promptForFilename = true);
				}
				else
				{
					// don't allow us to reset the shot number below
					first = false;
				}

				// always number the file name
				numberedFilename = true;
				break;
			case OVERWRITE_SCREENSHOT:				
				if ( firstMultiple )
				{
					// prompt if we're the start of a series
					BANK_ONLY(promptForFilename = true);
				}

				// must number the file name if we're a non-bank build
				numberedFilename = !__BANK;
				break;
			}

			static int shotNum = 0;

#if __BANK
			if ( first && numberedFilename )
			{
				shotNum = 0;
			}

			if ( promptForFilename )
			{
				// If it's not safe to bring up a file prompt, use a default (but should have already been set in update)
				if (!isUpdateThread)
					safecpy(m_ScreenshotName,"c:/Screenshot");
				else if ( m_UseScreenShotFileDialogue )
				{
					if ( !BANKMGR.OpenFile( m_ScreenshotName, 256, m_TakeScreenshotJPG ? "*.jpg" : "*.png", 
						true, m_TakeScreenshotJPG ? "Screenshot (*.jpg)" : "Screenshot (*.png)" ) )
					{
						m_ScreenshotName[0] = '\0';
					}
				}

				if ( char* dot = strrchr( m_ScreenshotName, '.' ) )
				{
					*dot = '\0';
				}
			}
#endif

			char shotNameBuffer[256];
			shotNameBuffer[0] = '\0';

			if ( numberedFilename )
			{
				// Remove the extension before inserting the number
				const char *ext = m_TakeScreenshotJPG ? ".jpg" : ".png";
				char *existingExt = strstr(m_ScreenshotName, ext);
				if (existingExt)
					*existingExt = 0;
				sprintf( shotNameBuffer, "%s-%03d%s", m_ScreenshotName, shotNum++, ext);
			}
			else
			{
				sprintf( shotNameBuffer, "%s.%s", m_ScreenshotName, m_TakeScreenshotJPG ? "jpg" : "png" );
			}

			if ( shotNameBuffer[0] && !m_TakeScreenshotJPG ) 
			{
				if (!GRCDEVICE.CaptureScreenShotToFile(shotNameBuffer,m_ScreenshotGamma))
				{
					grcWarningf("Unable to take screenshot to file %s.",shotNameBuffer);
				}
			}
			else 
			{
				if ( shotNameBuffer[0] )
				{
					bool result=false;
					if ( m_TakeScreenshotJPG )
					{
						result = GRCDEVICE.CaptureScreenShotToJpegFile( shotNameBuffer );
					}
					else
					{
						result = GRCDEVICE.CaptureScreenShotToFile( shotNameBuffer, m_ScreenshotGamma );
					}

					if ( result )
					{
						grcDisplayf( "screenshot '%s' saved.", shotNameBuffer );
					}
					else
					{
						grcErrorf( "screenshot '%s' failed!", shotNameBuffer );
					}

#if __BANK
						// Try to regulate how fast the ImageViewer Widget gets updated.  Update on the first and the last one taken,
						// and anything in between if enough time has elapsed since the last one was updated.
						if ( (m_ScreenshotsLeft == 1) || (m_ScreenshotsLeft == m_ScreenshotsInARow)
							|| (m_ScreenshotTimer.GetTime() - m_ScreenshotImageViewerLastUpdatedTime >= MINIMUM_IMAGE_VIEWER_UPDATE_INTERVAL) )
						{
                            if ( m_ImageViewer != NULL )
                            {
                                m_ImageViewer->SetImage( shotNameBuffer );
                            }

						m_ScreenshotImageViewerLastUpdatedTime = m_ScreenshotTimer.GetTime();

						if ( m_needToDisplayScreenshotInRagApplicationWindow )
						{
							m_DisplayScreenshotInRagApplicationWindow = true;
							m_needToDisplayScreenshotInRagApplicationWindow = false;
						}
					}
#endif
				}
				else
				{
					grcDisplayf( "screenshot cancelled." );
				}
			}

			--m_ScreenshotsLeft;
			if ( m_ScreenshotsLeft <= 0 ) 
			{
				m_TakeScreenshot = false;
				m_ScreenshotLastTakenTime = 0.0f;
			}
			else
			{
				m_ScreenshotLastTakenTime = m_ScreenshotTimer.GetTime();
			}
		}
	}

	if (m_ScreenshotFrameDelay > 0)
		m_ScreenshotFrameDelay--;

	PIXEnd();
#endif

#if (RSG_DURANGO || RSG_ORBIS || RSG_PS3) && !__FINAL
	grcTextureFactory::GetInstance().UnlockRenderTarget(0);
#endif

	// This needs to be as close to the end of frame as possible - not after
	PIXEndFrame();

#if USE_GPU_PERF_API
	if(g_NVIDIA_PerfKit_Enabled)
	{
		NVPMRESULT	result;
		bool		updateCounters = false;

		if(g_FrameFreezed)
		{
			//
			FlushGPU();
			GetNvPmApi()->EndObject(g_hNVPMContext, 0);

			//	experiment
			result = GetNvPmApi()->EndPass(g_hNVPMContext, g_CurrentPassID);
			g_CurrentPassID++;

			if(g_CurrentPassID == g_RequiredPass)
			{
				GetNvPmApi()->EndExperiment(g_hNVPMContext);
				updateCounters = true;
				g_CurrentPassID = 0;
			}
		}
		else
		{
			//	real-time - use sample directlyEndExperiment
			updateCounters = true;
		}

		if(updateCounters)
		{
			u32			nCount;

			result = GetNvPmApi()->Sample(g_hNVPMContext, NULL, &nCount);
			if(result == NVPM_OK)
			{
				u64 value, cycle;
				for(size_t idx = 0; idx < g_CountersCount; idx++)
				{
					result = GetNvPmApi()->GetCounterValueByName(g_hNVPMContext, c_NVIDIAPerfCounters[idx].counterName, 0, &value, &cycle);
					if(result == NVPM_OK)
					{
						switch(c_NVIDIAPerfCounters[idx].counterType)
						{
						case 0:
							{
								::rage::pfValueT<int>*	pfValue = (::rage::pfValueT<int>*)c_NVIDIAPerfCounters[idx].counterValue;

								pfValue->Set((u32)value);

								if(idx == 0 && !g_FrameFreezed)
								{
									//	save drawcall count for experiment
									g_LastBatchCount = (u32)value;
								}
							}
							break;

						case 1:
						case 2:
							{
								::rage::pfValueT<float>*	pfValue = (::rage::pfValueT<float>*)c_NVIDIAPerfCounters[idx].counterValue;
								
								if(c_NVIDIAPerfCounters[idx].counterType == 1)
								{
									pfValue->Set((float)value);
								}
								else
								{
									pfValue->Set((float)value / cycle);
								}
							}
							break;
						}
					}
				}
			}

			if(g_FrameFreezed && updateCounters)
			{
				result = GetNvPmApi()->DeleteObjects(g_hNVPMContext);
				//Assert(result == NVPM_OK);
			}
		}
	}
	if(g_GPA_Enabled)
	{
		if(g_FrameFreezed)
		{
			FlushGPU();

			GPA_Status status;
			status = __GPA_EndSample();
			Assert(status == GPA_STATUS_OK);
			status = __GPA_EndPass();
			Assert(status == GPA_STATUS_OK);
			g_CurrentPassID++;

			if(g_CurrentPassID == g_RequiredPass)
			{
				GPA_Status status = __GPA_EndSession();
				Assert(status == GPA_STATUS_OK);

				GPA_Status	sessionStatus = GPA_STATUS_ERROR_SESSION_NOT_FOUND;
				bool		readyResult = false;

				if(g_CurrentSessionID != g_SessionID)
				{
					sessionStatus = __GPA_IsSessionReady(&readyResult, g_CurrentSessionID);
					while(sessionStatus == GPA_STATUS_ERROR_SESSION_NOT_FOUND)
					{
						g_CurrentSessionID++;
						sessionStatus = __GPA_IsSessionReady(&readyResult, g_CurrentSessionID);
					}
				}

				if(readyResult && sessionStatus == GPA_STATUS_OK)
				{
					gpa_uint32	sampleCount;
					gpa_uint32	count;

					__GPA_GetEnabledCount(&count);
					__GPA_GetSampleCount(g_CurrentSessionID, &sampleCount);

					for(size_t index = 0; index < g_CountersCount; index++)
					{
						gpa_uint32	counterIndex;

						if(__GPA_GetCounterIndex(c_AMDPerfCounters[index].counterName, &counterIndex) == GPA_STATUS_OK)
						{
							GPA_Type	valueType;

							__GPA_GetCounterDataType(counterIndex, &valueType);

							switch (valueType)
							{
							case GPA_TYPE_FLOAT64:
								gpa_float64	floatResult;
								if(__GPA_GetSampleFloat64(g_CurrentSessionID, 1, counterIndex, &floatResult) == GPA_STATUS_OK)
								{
									SetCounterValue(c_AMDPerfCounters[index], floatResult);
								}
								break;

							case GPA_TYPE_UINT32:
								gpa_uint32	uintResult;
								if(__GPA_GetSampleUInt32(g_CurrentSessionID, 1, counterIndex, &uintResult) == GPA_STATUS_OK)
								{
									SetCounterValue(c_AMDPerfCounters[index], uintResult);
								}
								break;

							default:
								Assert(false);
								break;
							}
						}
					}

					g_CurrentSessionID++;
				}
				g_CurrentPassID = 0;
			}
		}
	}
#endif

	grcResolveFlags rf;
	rf.Color = m_ClearColor;
	rf.Depth = 1.0f;
	GRCDEVICE.EndFrame(m_ClearDuringResolve==2?&rf:NULL);

	DURANGO_ONLY(GRCDEVICE.HandleSuspendResume());

#if ENABLE_PIX_TAGGING
	// Do this at the end of the frame to help re-enabling.
	const bool pixEnabled=(g_EnablePixAnnotation != 0);
	if (s_enablePIX && !pixEnabled)
		g_EnablePixAnnotation = (unsigned int)~0;
	else if (!s_enablePIX && pixEnabled)
		g_EnablePixAnnotation = 0;
#endif // ENABLE_PIX_TAGGING

#if __BANK && RAGE_TRACKING && 0	// DEPRECATED
    if ( PARAM_trackerrag.Get() )
    {
        if ( ::rage::diagTracker::GetCurrent() )
        {
            ::rage::diagTracker::GetCurrent()->Update();
        }
    }
#endif // DEPRECATED
}

bool grcSetup::WantExit() {
#if __PPU
	if (g_RequestSysutilExit)
		return true;
#endif
	// see if bank manager has received quit message:
#if __BANK
	if (bkManager::GotQuitMessage() || s_RagWantsUsToQuit)
	{
		return true;
	}
	else 
#endif
/*	if (GRCDEVICE.QueryCloseWindow() ) 
	{
		//GRCDEVICE.ClearCloseWindow();
		return true;
	}
	else*/
		return false;
}

void grcSetup::Shutdown() {
#if __BANK
    if ( bkConsole::GetInstance() )
    {
        bkConsole::ShutdownClass();
    }


	bkIo::DeleteIoManager();
	bkManager::DeleteBankManager();

	//if (!(PARAM_rag.Get() || PARAM_ragviewer.Get())) // if not running rag
	{
        EVENTQ.DisconnectPipe();
		bkRemotePacket::Close();  // clean up memory
	}

#endif
#if __D3D && __WIN32PC
    GRCDEVICE.UnregisterAllDeviceLostCallbacks();
#endif	// __WIN32PC

	// Shutdown the PIX profiling stuff (GCM HUD and cellGcmTimeStamp on PS3)
	PIXShutdown();
}

// RETURNS:	Last update task time (UpT), in milliseconds
#if !__FINAL
float grcSetup::GetUpdateTaskTime()
{
	return sm_UpdateTaskTime;
}
#endif // !__FINAL

// RETURNS:	Last draw task time (DrT), in milliseconds
#if !__FINAL
float grcSetup::GetDrawTaskTime()
{
	return sm_DrawTaskTime;
}

// the minimum Safe zone for all supported platforms
// NOTE: PS3 uses 85, Xbox 90. Artists want to see both for tuning purposes...
void GetMinSafeZone(int &x0, int &y0, int &x1, int &y1) {

	float width = (float) GRCDEVICE.GetWidth();
	float height = (float) GRCDEVICE.GetHeight();

	float safeW = width NOTFINAL_ONLY(* g_SafeZoneSize);
	float safeH = height NOTFINAL_ONLY(* g_SafeZoneSize);  
	float offsetW = (width - safeW) * 0.5f;
	float offsetH = (height - safeH) * 0.5f;
	// Round down to lowest area
	x0 = (int) ceil(offsetW);
	y0 = (int) ceil(offsetH);
	x1 = (int) floor(width - offsetW - 1.f);
	y1 = (int) floor(height - offsetH - 1.f);
}

void grcSetup::DrawSafeZone() const {
	grcViewport *old = grcViewport::GetCurrent();
	grcViewport::SetCurrent(grcViewport::GetDefaultScreen());
	grcViewport::SetCurrentWorldIdentity();
	grcBindTexture(NULL);
	grcStateBlock::SetDepthStencilState(grcStateBlock::DSS_IgnoreDepth);
	grcBegin(drawLineStrip, 5);
	int x0, x1, y0, y1;
	GRCDEVICE.GetSafeZone(x0, y0, x1, y1);
	grcColor(Color32(0.5f + 0.5f * sinf(TIME.GetElapsedTime() * 1.25f), 0.5f + 0.5f * cosf(TIME.GetElapsedTime() * 2.f), 0.5f + -0.5f * sinf(TIME.GetElapsedTime())));
	grcVertex2i(x0, y0);
	grcVertex2i(x1, y0);
	grcVertex2i(x1, y1);
	grcVertex2i(x0, y1);
	grcVertex2i(x0, y0);
	grcEnd();	

	grcBegin(drawLineStrip, 5);
	GetMinSafeZone(x0, y0, x1, y1);
	grcColor(Color32(0.5f + 0.5f * sinf(TIME.GetElapsedTime() * 1.25f), 0.5f + 0.5f * cosf(TIME.GetElapsedTime() * 2.f), 0.5f + -0.5f * sinf(TIME.GetElapsedTime())));
	grcVertex2i(x0, y0);
	grcVertex2i(x1, y0);
	grcVertex2i(x1, y1);
	grcVertex2i(x0, y1);
	grcVertex2i(x0, y0);
	grcEnd();

	grcViewport::SetCurrent(old);
}

void grcSetup::EnableSafeZoneDraw(bool b/*=true*/, float size/*=0.85f*/)
{
	m_DrawSafeZone = b;
	g_SafeZoneSize = size;
}

void grcSetup::SetBugInfo(float x,float y,float z,const char *grid,const char *owner) {
	m_BugX = x;
	m_BugY = y;
	m_BugZ = z;
	safecpy(m_BugGrid, grid, sizeof(m_BugGrid));
	safecpy(m_BugOwner, owner, sizeof(m_BugOwner));
}


void grcSetup::AddBug() {
	m_AddBug = true;
	m_ScreenshotFrameDelay = 3;
}

void grcSetup::TakeScreenShotPNG() {
	m_TakeScreenshot = true;
	m_TakeScreenshotJPG = false;
	m_ScreenshotsLeft = m_ScreenshotsInARow;

	m_ScreenshotTimer.Reset();
	m_ScreenshotLastTakenTime = 0.0f;
	m_ScreenshotImageViewerLastUpdatedTime = 0.0f;

	m_ScreenshotFrameDelay = 0;
}

void grcSetup::TakeScreenShotJPG() {
	m_TakeScreenshot = true;
	m_TakeScreenshotJPG = true;
	m_ScreenshotsLeft = m_ScreenshotsInARow;

	m_ScreenshotTimer.Reset();
	m_ScreenshotLastTakenTime = 0.0f;
	m_ScreenshotImageViewerLastUpdatedTime = 0.0f;

	m_ScreenshotFrameDelay = 0;
}

#if SUPPORT_RENDERTARGET_DUMP
void grcSetup::TakeRenderTargetShots(const char *name)
{
	m_FrameToCapture = GRCDEVICE.GetFrameCounter() + 1;
	m_DebugRenderTargetName = name;
}

void grcSetup::TakeRenderTargetShotsNow()
{
	m_FrameToCapture = GRCDEVICE.GetFrameCounter();
}

void grcSetup::UntakeRenderTargetShots()
{
	m_FrameToCapture = 0;
	m_DebugRenderTargetName = NULL;
}

bool grcSetup::ShouldCaptureRenderTarget(const char *name)
{
	return m_FrameToCapture == GRCDEVICE.GetFrameCounter() &&
		!(m_DebugRenderTargetName && name && strcmp(m_DebugRenderTargetName,name));
		
}
#endif //SUPPORT_RENDERTARGET_DUMP

void grcSetup::StopScreenShots() {
	m_TakeScreenshot = false;
	m_ScreenshotsLeft = 0;
	m_ScreenshotFrameDelay = 0;
}
#endif

#if __BANK

bool grcSetup::ConnectToRag( const char *appName )
{
	PARAM_appname.Get(appName);
	bkManager::SetAppName(appName);

	if ( bkRemotePacket::ConnectToRag(PARAM_ragviewer.Get()) )
	{
		// construct pipe name:
		char pipeName[128];
		Assert(strlen(appName)<(sizeof(pipeName)-1-sizeof(".pipe.event_queue")));
		formatf(pipeName,sizeof(pipeName),"%s.pipe.event_queue",appName);

        int commandBufferSize;
        if ( PARAM_ragEventQBufferSize.Get( commandBufferSize ) )
        {
            EVENTQ.ConnectPipe( pipeName, 
			    bkRemotePacket::GetRagSocketAddress(), bkRemotePacket::GetRagSocketPort(bkRemotePacket::SOCKET_OFFSET_EVENTS), commandBufferSize );
        }
        else
        {
            EVENTQ.ConnectPipe( pipeName, 
                bkRemotePacket::GetRagSocketAddress(), bkRemotePacket::GetRagSocketPort(bkRemotePacket::SOCKET_OFFSET_EVENTS) );
        }

		sysIpcCreateThread(SecretDeathThread,NULL,8192,PRIO_ABOVE_NORMAL,"[RAGE] SecretDeathThread");

		if (!PARAM_ragviewer.Get())
		{
            bkConsole::InitClass( PARAM_powerconsole.Get() );
            sysIpcCreateThread(ConsoleConnectThread,bkConsole::GetInstance(),8192,PRIO_BELOW_NORMAL,"[RAGE] ConsoleConnectionThread");
		}

		return true;
	}
	else {
		PARAM_rag.Set(NULL);
		PARAM_ragviewer.Set(NULL);

		return false;
	}
}

#if __WIN32
static bool toggle_EM_INVALID, toggle_EM_DENORMAL;
#define IMPL_EXC(tag) static void callback_##tag() { _clearfp(); grcDisplayf("FLOATING POINT EXCEPTION %s %smasked",#tag,toggle_##tag?"UN":""); if (toggle_##tag) _controlfp(~tag & _controlfp(0,0), _MCW_EM); else _controlfp(tag | _controlfp(0,0), _MCW_EM); }
IMPL_EXC(EM_INVALID)
IMPL_EXC(EM_DENORMAL)

#undef IMPL_EXC
#define IMPL_EXC(tag)	bk.AddToggle(#tag,&toggle_##tag,datCallback(callback_##tag))
#endif

void grcSetup::AddWidgets(bkBank &bk) 
{
	grcDevice::AddWidgets( bk );
//	grcRenderTarget::AddLoggingWidgets(bk);

	static const char* colorBars[] = {"none","75% SMPTE", "100% SMPTE", "GreyScale"};
	bk.AddCombo("Draw Color Bars", (int*)&m_DrawColorBars,4,colorBars);

	AddScreenShotViewerWidgets(bk);

	bk.AddSeparator();

	bk.AddToggle("Display Frame Time",&m_ShowFrameTime);

#if ENABLE_PIX_TAGGING
#if __D3D11_MONO_DRIVER
	u32 monoRuntime=(u32)-1;
	PARAM_monoRuntime.Get(monoRuntime);
	if (monoRuntime == 0)
	{	// PIX tagging isn't available when monoRuntime=0 anyway. Save a few function calls :)
		s_enablePIX = false;
	}
	else
#endif // __D3D11_MONO_DRIVER
	{
		bk.AddToggle("Enable PIX markers", &s_enablePIX);
	}
#endif // ENABLE_PIX_TAGGING

	TICKER_ONLY( bk.AddSlider("Timebar ticker Y-scale", &g_TickerScaleY, 0.1f, 10.0f, 0.1f); )

#if __WIN32
	bk.PushGroup("Floating-Point Exceptions",false);
	IMPL_EXC(EM_INVALID);
	IMPL_EXC(EM_DENORMAL);
	bk.PopGroup();
#endif
}


void grcSetup::AddScreenShotViewerWidgets(bkBank &bk) 
{
	bk.AddToggle("Draw Safe Zone", &m_DrawSafeZone);

	bk.AddSeparator();

	static const char* namingConventions[] = { "Prompt for name", "Numbered screenshots", "Overwrite last screenshot" };
	bk.AddCombo("File Naming Convention", (int*)&m_ScreenshotNamingConvention, 3, namingConventions, NullCB, "Determines how files are named and when the user is prompted to provide one.");
	bk.AddSlider("Screenshots in a row",&m_ScreenshotsInARow,1,1000000,1, NullCB, "Increase to take a series of screen shots." );
	bk.AddSlider("Interval (seconds)",&m_ScreenshotIntervalTime,0.0f,1000000.0f,0.1f, NullCB, "The time to wait in between screen shots when taking a series of them." );
	bk.AddToggle("Disable mouse pointer for Screenshot", &m_DisableMousePointerDrawDuringScreenshot);
	bk.AddButton("Save Screenshot (.png)", datCallback(MFA(grcSetup::TakeScreenShotPNG),this));
	bk.AddButton("Save Screenshot (.jpg)", datCallback(MFA(grcSetup::TakeScreenShotJPG),this));
	bk.AddSlider("JPEG Quality", &g_JpegSaveQuality, 60, 100, 1);
	bk.AddSlider("PNG Gamma", &m_ScreenshotGamma, 1.5f, 3.0f, 0.001f);
#if !MULTIPLE_RENDER_THREADS
	bk.AddButton("Save Render Targets Unlocked", datCallback(MFA1(grcSetup::TakeRenderTargetShots),this,NULL,false));
#endif // MULTIPLE_RENDER_THREADS

	// add image view of screenshot:
	m_ImageViewer = bk.AddImageViewer( "Screen Shot Viewer", "Displays the current saved screen shot" );

	bk.AddButton("Stop Screenshots", datCallback(MFA(grcSetup::StopScreenShots),this));
	bk.AddButton("Add Bug With Screenshot", datCallback(MFA(grcSetup::AddBug),this), "Takes a screen shot and submits a bug to BugStar." );

#if __XENON || __PPU
	if ( bkRemotePacket::IsConnectedToRag() )
	{
		bk.AddButton("Start Screenshots in Rag's Application Window", datCallback(MFA(grcSetup::StartScreenshotsInRagApplicationWindow),this));
		bk.AddToggle("Display last Screenshot in Rag's Application Window", &m_DisplayScreenshotInRagApplicationWindow);
	}
#endif

	bk.AddToggle("Simulate controller input via keyboard (ghetto pad)", ioInput::GetIsGhettoKeyboardEnabledPtr());

	// automatically switch on the screenshot viewer if the param is passed and use autoscreenshot.png as default
#if __BANK
	if (PARAM_autoscreenshot.Get())
	{
		strcpy(m_ScreenshotName, "c:/autoscreenshot");
		m_UseScreenShotFileDialogue = false;
		grcSetup::StartScreenshotsInRagApplicationWindow();
	}
#endif
}
#endif	// __BANK
