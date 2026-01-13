#if 0 // MIGRATE_FIXME - similar code exists now inside device, but unsure whether it is as fully fledged as the original
#include "gcmdebug.h"
#if !__FINAL && __PPU
#include "wrapper_gcm.h"
#include "grcorespu.h"
#include "file/stream.h"
#include "system/ipc.h"

namespace rage {

gcmDebug g_gcmDebug;
extern spuGcmState s_spuGcmState;

void dump_fifo(uint32_t *begin,uint32_t *stop);
void AnalyseFifo(u32* begin, u32* end);

DECLARE_THREAD_FUNC(StartHangThread)
{
    g_gcmDebug.HangThread();
}

void gcmDebug::Init()
{
    m_needFifoDump = false;
    m_labels = gcm::RsxSemaphoreRegistrar::Allocate(16);
    for(int i=0; i<16; ++i) 
    {
        s_spuGcmState.DebugLabels[i] = cellGcmGetLabelAddress(m_labels + i);
        *s_spuGcmState.DebugLabels[i] = 0;
    }
    m_hangFromSpu[15] = 0;
    s_spuGcmState.pDebugHang = m_hangFromSpu;
    s_spuGcmState.LastFifoExecutedLabel = gcm::RsxSemaphoreRegistrar::Allocate();
    s_spuGcmState.pLastFifoExecuted = cellGcmGetLabelAddress(s_spuGcmState.LastFifoExecutedLabel);
    cellGcmSetGraphicsHandler(&gcmDebug::ExceptionHandler);
    sysIpcCreateThread(StartHangThread,0,sysIpcMinThreadStackSize,PRIO_NORMAL,"gcmDebug spu hang detect");
}

int g_debugTextureLabelCount = 0;

void gcmDebug::Push(const char* name)
{
    Assert(m_stackLevel < 16);
    cellGcmSetWriteTextureLabel(GCM_CONTEXT, m_labels + m_stackLevel, (u32)name);
    SPU_SIMPLE_COMMAND(grcDevice__SetLastFifoLabel, s_spuGcmState.LastFifoExecutedLabel);
    ++m_stackLevel;

	// WAR for consecutive texture label HW bug:
	// invalidate L2 every N jobs (for N=8)
	++g_debugTextureLabelCount;
	if( !( g_debugTextureLabelCount & 0x7 ) )
		cellGcmSetInvalidateTextureCache(GCM_CONTEXT, CELL_GCM_INVALIDATE_TEXTURE);
}

void gcmDebug::Pop()
{
    Assert(m_stackLevel > 0);
    --m_stackLevel;
    cellGcmSetWriteTextureLabel(GCM_CONTEXT, m_labels + m_stackLevel, 0);

	// WAR for consecutive texture label HW bug:
	// invalidate L2 every N jobs (for N=8)
	++g_debugTextureLabelCount;
	if( !( g_debugTextureLabelCount & 0x7 ) )
		cellGcmSetInvalidateTextureCache(GCM_CONTEXT, CELL_GCM_INVALIDATE_TEXTURE);
}

void gcmDebug::HangThread()
{
    while (1)
    {
        sysIpcSleep(16);
        if (m_hangFromSpu[15])
        {
            grcErrorf("SPU detected hang, GCM debug stack:");
            grcErrorf("last set label executed = %x", *s_spuGcmState.pLastFifoExecuted);
            for(int i=0; i<15 && m_hangFromSpu[i]; ++i)
                grcDisplayf("%2i : %s", i, m_hangFromSpu[i]);
            m_hangFromSpu[15] = 0;
        }
        if (m_needFifoDump)
        {
            // wait for a few seconds for the GCM exception handler to spew its load
            sysIpcSleep(3000);
            // trigger PPU exception
            __debugbreak();
        }
    }
}

void gcmDebug::Report()
{
    grcDisplayf("GCM DEBUG STACK:");
    for(int i=0; i<16; ++i)
    {
        u32* pLabel = cellGcmGetLabelAddress(m_labels + i);
        if (!*pLabel)
            break;
        grcDisplayf("%2i : %s", i, (const char*)*pLabel);
    }
}

void gcmDebug::ExceptionHandler(u32)
{
    grcErrorf("*** RSX CRASH ***");

    grcDisplayf("get = %x put = %x ref = %x ringOffset = %x ringBegin = %p", 
        s_spuGcmState.Control->get, s_spuGcmState.Control->put, 
        s_spuGcmState.Control->ref, s_spuGcmState.RingOffset,
        s_spuGcmState.RingBegin);
    grcDisplayf("last set label executed = %x", *s_spuGcmState.pLastFifoExecuted);
    g_gcmDebug.Report();
    g_gcmDebug.m_needFifoDump = true;
}

void gcmDebug::HandlePpuException()
{
    if (!m_needFifoDump)
        return;

    const char* filename = "/app_home/fifodump.bin";
    fiStream* str = fiStream::Create(filename);
    if (str)
    {
        grcDisplayf("Dumping FIFO to %s", filename);
        str->Write(s_spuGcmState.RealContext.begin, 
            (char*)s_spuGcmState.RealContext.end - (char*)s_spuGcmState.RealContext.begin);
        str->Close();
    }
}

const char *regname(int reg) {
    switch (reg) {
        case CELL_GCM_NV406E_SET_REFERENCE	: return "SET_REFERENCE";
        case CELL_GCM_NV406E_SET_CONTEXT_DMA_SEMAPHORE	: return "SET_CONTEXT_DMA_SEMAPHORE";
        case CELL_GCM_NV406E_SEMAPHORE_OFFSET	: return "SEMAPHORE_OFFSET";
        case CELL_GCM_NV406E_SEMAPHORE_ACQUIRE	: return "SEMAPHORE_ACQUIRE";
        case CELL_GCM_NV406E_SEMAPHORE_RELEASE	: return "SEMAPHORE_RELEASE";
        case CELL_GCM_NV4097_SET_OBJECT		: return "NOP (SET_OBJECT)";
        case CELL_GCM_NV4097_NO_OPERATION	: return "NO_OPERATION";
        case CELL_GCM_NV4097_WAIT_FOR_IDLE	: return "WAIT_FOR_IDLE";
        case CELL_GCM_NV4097_PM_TRIGGER	: return "PM_TRIGGER";
        case CELL_GCM_NV4097_SET_CONTEXT_DMA_NOTIFIES	: return "SET_CONTEXT_DMA_NOTIFIES";
        case CELL_GCM_NV4097_SET_CONTEXT_DMA_A	: return "SET_CONTEXT_DMA_A";
        case CELL_GCM_NV4097_SET_CONTEXT_DMA_B	: return "SET_CONTEXT_DMA_B";
        case CELL_GCM_NV4097_SET_CONTEXT_DMA_COLOR_B	: return "SET_CONTEXT_DMA_COLOR_B";
        case CELL_GCM_NV4097_SET_CONTEXT_DMA_STATE	: return "SET_CONTEXT_DMA_STATE";
        case CELL_GCM_NV4097_SET_CONTEXT_DMA_COLOR_A	: return "SET_CONTEXT_DMA_COLOR_A";
        case CELL_GCM_NV4097_SET_CONTEXT_DMA_ZETA	: return "SET_CONTEXT_DMA_ZETA";
        case CELL_GCM_NV4097_SET_CONTEXT_DMA_VERTEX_A	: return "SET_CONTEXT_DMA_VERTEX_A";
        case CELL_GCM_NV4097_SET_CONTEXT_DMA_VERTEX_B	: return "SET_CONTEXT_DMA_VERTEX_B";
        case CELL_GCM_NV4097_SET_CONTEXT_DMA_SEMAPHORE	: return "SET_CONTEXT_DMA_SEMAPHORE";
        case CELL_GCM_NV4097_SET_CONTEXT_DMA_REPORT	: return "SET_CONTEXT_DMA_REPORT";
        case CELL_GCM_NV4097_SET_CONTEXT_DMA_CLIP_ID	: return "SET_CONTEXT_DMA_CLIP_ID";
        case CELL_GCM_NV4097_SET_CONTEXT_DMA_CULL_DATA	: return "SET_CONTEXT_DMA_CULL_DATA";
        case CELL_GCM_NV4097_SET_CONTEXT_DMA_COLOR_C	: return "SET_CONTEXT_DMA_COLOR_C";
        case CELL_GCM_NV4097_SET_CONTEXT_DMA_COLOR_D	: return "SET_CONTEXT_DMA_COLOR_D";
        case CELL_GCM_NV4097_SET_SURFACE_CLIP_HORIZONTAL	: return "SET_SURFACE_CLIP_HORIZONTAL";
        case CELL_GCM_NV4097_SET_SURFACE_CLIP_VERTICAL	: return "SET_SURFACE_CLIP_VERTICAL";
        case CELL_GCM_NV4097_SET_SURFACE_FORMAT	: return "SET_SURFACE_FORMAT";
        case CELL_GCM_NV4097_SET_SURFACE_PITCH_A	: return "SET_SURFACE_PITCH_A";
        case CELL_GCM_NV4097_SET_SURFACE_COLOR_AOFFSET	: return "SET_SURFACE_COLOR_AOFFSET";
        case CELL_GCM_NV4097_SET_SURFACE_ZETA_OFFSET	: return "SET_SURFACE_ZETA_OFFSET";
        case CELL_GCM_NV4097_SET_SURFACE_COLOR_BOFFSET	: return "SET_SURFACE_COLOR_BOFFSET";
        case CELL_GCM_NV4097_SET_SURFACE_PITCH_B	: return "SET_SURFACE_PITCH_B";
        case CELL_GCM_NV4097_SET_SURFACE_COLOR_TARGET	: return "SET_SURFACE_COLOR_TARGET";
        case CELL_GCM_NV4097_SET_SURFACE_PITCH_Z	: return "SET_SURFACE_PITCH_Z";
        case CELL_GCM_NV4097_INVALIDATE_ZCULL	: return "INVALIDATE_ZCULL";
        case CELL_GCM_NV4097_SET_CYLINDRICAL_WRAP	: return "SET_CYLINDRICAL_WRAP";
        case CELL_GCM_NV4097_SET_CYLINDRICAL_WRAP1	: return "SET_CYLINDRICAL_WRAP1";
        case CELL_GCM_NV4097_SET_SURFACE_PITCH_C	: return "SET_SURFACE_PITCH_C";
        case CELL_GCM_NV4097_SET_SURFACE_PITCH_D	: return "SET_SURFACE_PITCH_D";
        case CELL_GCM_NV4097_SET_SURFACE_COLOR_COFFSET	: return "SET_SURFACE_COLOR_COFFSET";
        case CELL_GCM_NV4097_SET_SURFACE_COLOR_DOFFSET	: return "SET_SURFACE_COLOR_DOFFSET";
        case CELL_GCM_NV4097_SET_WINDOW_OFFSET	: return "SET_WINDOW_OFFSET";
        case CELL_GCM_NV4097_SET_WINDOW_CLIP_TYPE	: return "SET_WINDOW_CLIP_TYPE";
        case CELL_GCM_NV4097_SET_WINDOW_CLIP_HORIZONTAL	: return "SET_WINDOW_CLIP_HORIZONTAL";
        case CELL_GCM_NV4097_SET_WINDOW_CLIP_VERTICAL	: return "SET_WINDOW_CLIP_VERTICAL";
        case CELL_GCM_NV4097_SET_DITHER_ENABLE	: return "SET_DITHER_ENABLE";
        case CELL_GCM_NV4097_SET_ALPHA_TEST_ENABLE	: return "SET_ALPHA_TEST_ENABLE";
        case CELL_GCM_NV4097_SET_ALPHA_FUNC	: return "SET_ALPHA_FUNC";
        case CELL_GCM_NV4097_SET_ALPHA_REF	: return "SET_ALPHA_REF";
        case CELL_GCM_NV4097_SET_BLEND_ENABLE	: return "SET_BLEND_ENABLE";
        case CELL_GCM_NV4097_SET_BLEND_FUNC_SFACTOR	: return "SET_BLEND_FUNC_SFACTOR";
        case CELL_GCM_NV4097_SET_BLEND_FUNC_DFACTOR	: return "SET_BLEND_FUNC_DFACTOR";
        case CELL_GCM_NV4097_SET_BLEND_COLOR	: return "SET_BLEND_COLOR";
        case CELL_GCM_NV4097_SET_BLEND_EQUATION	: return "SET_BLEND_EQUATION";
        case CELL_GCM_NV4097_SET_COLOR_MASK	: return "SET_COLOR_MASK";
        case CELL_GCM_NV4097_SET_STENCIL_TEST_ENABLE	: return "SET_STENCIL_TEST_ENABLE";
        case CELL_GCM_NV4097_SET_STENCIL_MASK	: return "SET_STENCIL_MASK";
        case CELL_GCM_NV4097_SET_STENCIL_FUNC	: return "SET_STENCIL_FUNC";
        case CELL_GCM_NV4097_SET_STENCIL_FUNC_REF	: return "SET_STENCIL_FUNC_REF";
        case CELL_GCM_NV4097_SET_STENCIL_FUNC_MASK	: return "SET_STENCIL_FUNC_MASK";
        case CELL_GCM_NV4097_SET_STENCIL_OP_FAIL	: return "SET_STENCIL_OP_FAIL";
        case CELL_GCM_NV4097_SET_STENCIL_OP_ZFAIL	: return "SET_STENCIL_OP_ZFAIL";
        case CELL_GCM_NV4097_SET_STENCIL_OP_ZPASS	: return "SET_STENCIL_OP_ZPASS";
        case CELL_GCM_NV4097_SET_TWO_SIDED_STENCIL_TEST_ENABLE	: return "SET_TWO_SIDED_STENCIL_TEST_ENABLE";
        case CELL_GCM_NV4097_SET_BACK_STENCIL_MASK	: return "SET_BACK_STENCIL_MASK";
        case CELL_GCM_NV4097_SET_BACK_STENCIL_FUNC	: return "SET_BACK_STENCIL_FUNC";
        case CELL_GCM_NV4097_SET_BACK_STENCIL_FUNC_REF	: return "SET_BACK_STENCIL_FUNC_REF";
        case CELL_GCM_NV4097_SET_BACK_STENCIL_FUNC_MASK	: return "SET_BACK_STENCIL_FUNC_MASK";
        case CELL_GCM_NV4097_SET_BACK_STENCIL_OP_FAIL	: return "SET_BACK_STENCIL_OP_FAIL";
        case CELL_GCM_NV4097_SET_BACK_STENCIL_OP_ZFAIL	: return "SET_BACK_STENCIL_OP_ZFAIL";
        case CELL_GCM_NV4097_SET_BACK_STENCIL_OP_ZPASS	: return "SET_BACK_STENCIL_OP_ZPASS";
        case CELL_GCM_NV4097_SET_SHADE_MODE	: return "SET_SHADE_MODE";
        case CELL_GCM_NV4097_SET_BLEND_ENABLE_MRT	: return "SET_BLEND_ENABLE_MRT";
        case CELL_GCM_NV4097_SET_COLOR_MASK_MRT	: return "SET_COLOR_MASK_MRT";
        case CELL_GCM_NV4097_SET_LOGIC_OP_ENABLE	: return "SET_LOGIC_OP_ENABLE";
        case CELL_GCM_NV4097_SET_LOGIC_OP	: return "SET_LOGIC_OP";
        case CELL_GCM_NV4097_SET_BLEND_COLOR2	: return "SET_BLEND_COLOR2";
        case CELL_GCM_NV4097_SET_DEPTH_BOUNDS_TEST_ENABLE	: return "SET_DEPTH_BOUNDS_TEST_ENABLE";
        case CELL_GCM_NV4097_SET_DEPTH_BOUNDS_MIN	: return "SET_DEPTH_BOUNDS_MIN";
        case CELL_GCM_NV4097_SET_DEPTH_BOUNDS_MAX	: return "SET_DEPTH_BOUNDS_MAX";
        case CELL_GCM_NV4097_SET_CLIP_MIN	: return "SET_CLIP_MIN";
        case CELL_GCM_NV4097_SET_CLIP_MAX	: return "SET_CLIP_MAX";
        case CELL_GCM_NV4097_SET_CONTROL0	: return "SET_CONTROL0";
        case CELL_GCM_NV4097_SET_LINE_WIDTH	: return "SET_LINE_WIDTH";
        case CELL_GCM_NV4097_SET_LINE_SMOOTH_ENABLE	: return "SET_LINE_SMOOTH_ENABLE";
        case CELL_GCM_NV4097_SET_ANISO_SPREAD       	: return "SET_ANISO_SPREAD       ";
        case CELL_GCM_NV4097_SET_SCISSOR_HORIZONTAL	: return "SET_SCISSOR_HORIZONTAL";
        case CELL_GCM_NV4097_SET_SCISSOR_VERTICAL	: return "SET_SCISSOR_VERTICAL";
        case CELL_GCM_NV4097_SET_FOG_MODE	: return "SET_FOG_MODE";
        case CELL_GCM_NV4097_SET_FOG_PARAMS	: return "SET_FOG_PARAMS";
        case CELL_GCM_NV4097_SET_SHADER_PROGRAM	: return "SET_SHADER_PROGRAM";
        case CELL_GCM_NV4097_SET_VERTEX_TEXTURE_OFFSET	: return "SET_VERTEX_TEXTURE_OFFSET";
        case CELL_GCM_NV4097_SET_VERTEX_TEXTURE_FORMAT	: return "SET_VERTEX_TEXTURE_FORMAT";
        case CELL_GCM_NV4097_SET_VERTEX_TEXTURE_ADDRESS	: return "SET_VERTEX_TEXTURE_ADDRESS";
        case CELL_GCM_NV4097_SET_VERTEX_TEXTURE_CONTROL0	: return "SET_VERTEX_TEXTURE_CONTROL0";
        case CELL_GCM_NV4097_SET_VERTEX_TEXTURE_CONTROL3	: return "SET_VERTEX_TEXTURE_CONTROL3";
        case CELL_GCM_NV4097_SET_VERTEX_TEXTURE_FILTER	: return "SET_VERTEX_TEXTURE_FILTER";
        case CELL_GCM_NV4097_SET_VERTEX_TEXTURE_IMAGE_RECT	: return "SET_VERTEX_TEXTURE_IMAGE_RECT";
        case CELL_GCM_NV4097_SET_VERTEX_TEXTURE_BORDER_COLOR	: return "SET_VERTEX_TEXTURE_BORDER_COLOR";
        case CELL_GCM_NV4097_SET_VIEWPORT_HORIZONTAL	: return "SET_VIEWPORT_HORIZONTAL";
        case CELL_GCM_NV4097_SET_VIEWPORT_VERTICAL	: return "SET_VIEWPORT_VERTICAL";
        case CELL_GCM_NV4097_SET_POINT_CENTER_MODE	: return "SET_POINT_CENTER_MODE";
        case CELL_GCM_NV4097_SET_VIEWPORT_OFFSET	: return "SET_VIEWPORT_OFFSET";
        case CELL_GCM_NV4097_SET_VIEWPORT_SCALE	: return "SET_VIEWPORT_SCALE";
        case CELL_GCM_NV4097_SET_POLY_OFFSET_POINT_ENABLE	: return "SET_POLY_OFFSET_POINT_ENABLE";
        case CELL_GCM_NV4097_SET_POLY_OFFSET_LINE_ENABLE	: return "SET_POLY_OFFSET_LINE_ENABLE";
        case CELL_GCM_NV4097_SET_POLY_OFFSET_FILL_ENABLE	: return "SET_POLY_OFFSET_FILL_ENABLE";
        case CELL_GCM_NV4097_SET_DEPTH_FUNC	: return "SET_DEPTH_FUNC";
        case CELL_GCM_NV4097_SET_DEPTH_MASK	: return "SET_DEPTH_MASK";
        case CELL_GCM_NV4097_SET_DEPTH_TEST_ENABLE	: return "SET_DEPTH_TEST_ENABLE";
        case CELL_GCM_NV4097_SET_POLYGON_OFFSET_SCALE_FACTOR	: return "SET_POLYGON_OFFSET_SCALE_FACTOR";
        case CELL_GCM_NV4097_SET_POLYGON_OFFSET_BIAS	: return "SET_POLYGON_OFFSET_BIAS";
        case CELL_GCM_NV4097_SET_VERTEX_DATA_SCALED4S_M	: return "SET_VERTEX_DATA_SCALED4S_M";
        case CELL_GCM_NV4097_SET_TEXTURE_CONTROL2	: return "SET_TEXTURE_CONTROL2";
        case CELL_GCM_NV4097_SET_TEX_COORD_CONTROL	: return "SET_TEX_COORD_CONTROL";
        case CELL_GCM_NV4097_SET_TRANSFORM_PROGRAM	: return "SET_TRANSFORM_PROGRAM";
        case CELL_GCM_NV4097_SET_SPECULAR_ENABLE	: return "SET_SPECULAR_ENABLE";
        case CELL_GCM_NV4097_SET_TWO_SIDE_LIGHT_EN	: return "SET_TWO_SIDE_LIGHT_EN";
        case CELL_GCM_NV4097_CLEAR_ZCULL_SURFACE	: return "CLEAR_ZCULL_SURFACE";
        case CELL_GCM_NV4097_SET_PERFORMANCE_PARAMS	: return "SET_PERFORMANCE_PARAMS";
        case CELL_GCM_NV4097_SET_FLAT_SHADE_OP	: return "SET_FLAT_SHADE_OP";
        case CELL_GCM_NV4097_SET_EDGE_FLAG	: return "SET_EDGE_FLAG";
        case CELL_GCM_NV4097_SET_USER_CLIP_PLANE_CONTROL	: return "SET_USER_CLIP_PLANE_CONTROL";
        case CELL_GCM_NV4097_SET_POLYGON_STIPPLE	: return "SET_POLYGON_STIPPLE";
        case CELL_GCM_NV4097_SET_POLYGON_STIPPLE_PATTERN	: return "SET_POLYGON_STIPPLE_PATTERN";
        case CELL_GCM_NV4097_SET_VERTEX_DATA3F_M	: return "SET_VERTEX_DATA3F_M";
        case CELL_GCM_NV4097_SET_VERTEX_DATA_ARRAY_OFFSET	: return "SET_VERTEX_DATA_ARRAY_OFFSET";
        case CELL_GCM_NV4097_INVALIDATE_VERTEX_CACHE_FILE	: return "INVALIDATE_VERTEX_CACHE_FILE";
        case CELL_GCM_NV4097_INVALIDATE_VERTEX_FILE	: return "INVALIDATE_VERTEX_FILE";
        case CELL_GCM_NV4097_PIPE_NOP	: return "PIPE_NOP";
        case CELL_GCM_NV4097_SET_VERTEX_DATA_BASE_OFFSET	: return "SET_VERTEX_DATA_BASE_OFFSET";
        case CELL_GCM_NV4097_SET_VERTEX_DATA_BASE_INDEX	: return "SET_VERTEX_DATA_BASE_INDEX";
        case CELL_GCM_NV4097_SET_VERTEX_DATA_ARRAY_FORMAT	: return "SET_VERTEX_DATA_ARRAY_FORMAT";
        case CELL_GCM_NV4097_CLEAR_REPORT_VALUE	: return "CLEAR_REPORT_VALUE";
        case CELL_GCM_NV4097_SET_ZPASS_PIXEL_COUNT_ENABLE	: return "SET_ZPASS_PIXEL_COUNT_ENABLE";
        case CELL_GCM_NV4097_GET_REPORT	: return "GET_REPORT";
        case CELL_GCM_NV4097_SET_ZCULL_STATS_ENABLE	: return "SET_ZCULL_STATS_ENABLE";
        case CELL_GCM_NV4097_SET_BEGIN_END	: return "SET_BEGIN_END";
        case CELL_GCM_NV4097_ARRAY_ELEMENT16	: return "ARRAY_ELEMENT16";
        case CELL_GCM_NV4097_ARRAY_ELEMENT32	: return "ARRAY_ELEMENT32";
        case CELL_GCM_NV4097_DRAW_ARRAYS	: return "DRAW_ARRAYS";
        case CELL_GCM_NV4097_INLINE_ARRAY	: return "INLINE_ARRAY";
        case CELL_GCM_NV4097_SET_INDEX_ARRAY_ADDRESS	: return "SET_INDEX_ARRAY_ADDRESS";
        case CELL_GCM_NV4097_SET_INDEX_ARRAY_DMA	: return "SET_INDEX_ARRAY_DMA";
        case CELL_GCM_NV4097_DRAW_INDEX_ARRAY	: return "DRAW_INDEX_ARRAY";
        case CELL_GCM_NV4097_SET_FRONT_POLYGON_MODE	: return "SET_FRONT_POLYGON_MODE";
        case CELL_GCM_NV4097_SET_BACK_POLYGON_MODE	: return "SET_BACK_POLYGON_MODE";
        case CELL_GCM_NV4097_SET_CULL_FACE	: return "SET_CULL_FACE";
        case CELL_GCM_NV4097_SET_FRONT_FACE	: return "SET_FRONT_FACE";
        case CELL_GCM_NV4097_SET_POLY_SMOOTH_ENABLE	: return "SET_POLY_SMOOTH_ENABLE";
        case CELL_GCM_NV4097_SET_CULL_FACE_ENABLE	: return "SET_CULL_FACE_ENABLE";
        case CELL_GCM_NV4097_SET_TEXTURE_CONTROL3	: return "SET_TEXTURE_CONTROL3";
        case CELL_GCM_NV4097_SET_VERTEX_DATA2F_M	: return "SET_VERTEX_DATA2F_M";
        case CELL_GCM_NV4097_SET_VERTEX_DATA2S_M	: return "SET_VERTEX_DATA2S_M";
        case CELL_GCM_NV4097_SET_VERTEX_DATA4UB_M	: return "SET_VERTEX_DATA4UB_M";
        case CELL_GCM_NV4097_SET_VERTEX_DATA4S_M	: return "SET_VERTEX_DATA4S_M";
        case CELL_GCM_NV4097_SET_TEXTURE_OFFSET	: return "SET_TEXTURE_OFFSET";
        case CELL_GCM_NV4097_SET_TEXTURE_FORMAT	: return "SET_TEXTURE_FORMAT";
        case CELL_GCM_NV4097_SET_TEXTURE_ADDRESS	: return "SET_TEXTURE_ADDRESS";
        case CELL_GCM_NV4097_SET_TEXTURE_CONTROL0	: return "SET_TEXTURE_CONTROL0";
        case CELL_GCM_NV4097_SET_TEXTURE_CONTROL1	: return "SET_TEXTURE_CONTROL1";
        case CELL_GCM_NV4097_SET_TEXTURE_FILTER	: return "SET_TEXTURE_FILTER";
        case CELL_GCM_NV4097_SET_TEXTURE_IMAGE_RECT	: return "SET_TEXTURE_IMAGE_RECT";
        case CELL_GCM_NV4097_SET_TEXTURE_BORDER_COLOR	: return "SET_TEXTURE_BORDER_COLOR";
        case CELL_GCM_NV4097_SET_VERTEX_DATA4F_M	: return "SET_VERTEX_DATA4F_M";
        case CELL_GCM_NV4097_SET_COLOR_KEY_COLOR	: return "SET_COLOR_KEY_COLOR";
        case CELL_GCM_NV4097_SET_SHADER_CONTROL	: return "SET_SHADER_CONTROL";
        case CELL_GCM_NV4097_SET_INDEXED_CONSTANT_READ_LIMITS	: return "SET_INDEXED_CONSTANT_READ_LIMITS";
        case CELL_GCM_NV4097_SET_SEMAPHORE_OFFSET	: return "SET_SEMAPHORE_OFFSET";
        case CELL_GCM_NV4097_BACK_END_WRITE_SEMAPHORE_RELEASE	: return "BACK_END_WRITE_SEMAPHORE_RELEASE";
        case CELL_GCM_NV4097_TEXTURE_READ_SEMAPHORE_RELEASE	: return "TEXTURE_READ_SEMAPHORE_RELEASE";
        case CELL_GCM_NV4097_SET_ZMIN_MAX_CONTROL	: return "SET_ZMIN_MAX_CONTROL";
        case CELL_GCM_NV4097_SET_ANTI_ALIASING_CONTROL	: return "SET_ANTI_ALIASING_CONTROL";
        case CELL_GCM_NV4097_SET_SURFACE_COMPRESSION	: return "SET_SURFACE_COMPRESSION";
        case CELL_GCM_NV4097_SET_ZCULL_EN	: return "SET_ZCULL_EN";
        case CELL_GCM_NV4097_SET_SHADER_WINDOW	: return "SET_SHADER_WINDOW";
        case CELL_GCM_NV4097_SET_ZSTENCIL_CLEAR_VALUE	: return "SET_ZSTENCIL_CLEAR_VALUE";
        case CELL_GCM_NV4097_SET_COLOR_CLEAR_VALUE	: return "SET_COLOR_CLEAR_VALUE";
        case CELL_GCM_NV4097_CLEAR_SURFACE	: return "CLEAR_SURFACE";
        case CELL_GCM_NV4097_SET_CLEAR_RECT_HORIZONTAL	: return "SET_CLEAR_RECT_HORIZONTAL";
        case CELL_GCM_NV4097_SET_CLEAR_RECT_VERTICAL	: return "SET_CLEAR_RECT_VERTICAL";
        case CELL_GCM_NV4097_SET_CLIP_ID_TEST_ENABLE	: return "SET_CLIP_ID_TEST_ENABLE";
        case CELL_GCM_NV4097_SET_RESTART_INDEX_ENABLE	: return "SET_RESTART_INDEX_ENABLE";
        case CELL_GCM_NV4097_SET_RESTART_INDEX	: return "SET_RESTART_INDEX";
        case CELL_GCM_NV4097_SET_LINE_STIPPLE	: return "SET_LINE_STIPPLE";
        case CELL_GCM_NV4097_SET_LINE_STIPPLE_PATTERN	: return "SET_LINE_STIPPLE_PATTERN";
        case CELL_GCM_NV4097_SET_VERTEX_DATA1F_M	: return "SET_VERTEX_DATA1F_M";
        case CELL_GCM_NV4097_SET_TRANSFORM_EXECUTION_MODE	: return "SET_TRANSFORM_EXECUTION_MODE";
        case CELL_GCM_NV4097_SET_RENDER_ENABLE	: return "SET_RENDER_ENABLE";
        case CELL_GCM_NV4097_SET_TRANSFORM_PROGRAM_LOAD	: return "SET_TRANSFORM_PROGRAM_LOAD";
        case CELL_GCM_NV4097_SET_TRANSFORM_PROGRAM_START	: return "SET_TRANSFORM_PROGRAM_START";
        case CELL_GCM_NV4097_SET_ZCULL_CONTROL0	: return "SET_ZCULL_CONTROL0";
        case CELL_GCM_NV4097_SET_ZCULL_CONTROL1	: return "SET_ZCULL_CONTROL1";
        case CELL_GCM_NV4097_SET_SCULL_CONTROL	: return "SET_SCULL_CONTROL";
        case CELL_GCM_NV4097_SET_POINT_SIZE	: return "SET_POINT_SIZE";
        case CELL_GCM_NV4097_SET_POINT_PARAMS_ENABLE	: return "SET_POINT_PARAMS_ENABLE";
        case CELL_GCM_NV4097_SET_POINT_SPRITE_CONTROL	: return "SET_POINT_SPRITE_CONTROL";
        case CELL_GCM_NV4097_SET_TRANSFORM_TIMEOUT	: return "SET_TRANSFORM_TIMEOUT";
        case CELL_GCM_NV4097_SET_TRANSFORM_CONSTANT_LOAD	: return "SET_TRANSFORM_CONSTANT_LOAD";
        case CELL_GCM_NV4097_SET_TRANSFORM_CONSTANT	: return "SET_TRANSFORM_CONSTANT";
        case CELL_GCM_NV4097_SET_FREQUENCY_DIVIDER_OPERATION	: return "SET_FREQUENCY_DIVIDER_OPERATION";
        case CELL_GCM_NV4097_SET_ATTRIB_COLOR	: return "SET_ATTRIB_COLOR";
        case CELL_GCM_NV4097_SET_ATTRIB_TEX_COORD	: return "SET_ATTRIB_TEX_COORD";
        case CELL_GCM_NV4097_SET_ATTRIB_TEX_COORD_EX	: return "SET_ATTRIB_TEX_COORD_EX";
        case CELL_GCM_NV4097_SET_ATTRIB_UCLIP0	: return "SET_ATTRIB_UCLIP0";
        case CELL_GCM_NV4097_SET_ATTRIB_UCLIP1	: return "SET_ATTRIB_UCLIP1";
        case CELL_GCM_NV4097_INVALIDATE_L2	: return "INVALIDATE_L2";
        case CELL_GCM_NV4097_SET_REDUCE_DST_COLOR	: return "SET_REDUCE_DST_COLOR";
        case CELL_GCM_NV4097_SET_SHADER_PACKER	: return "SET_SHADER_PACKER";
        case CELL_GCM_NV4097_SET_VERTEX_ATTRIB_INPUT_MASK	: return "SET_VERTEX_ATTRIB_INPUT_MASK";
        case CELL_GCM_NV4097_SET_VERTEX_ATTRIB_OUTPUT_MASK	: return "SET_VERTEX_ATTRIB_OUTPUT_MASK";
        case CELL_GCM_NV4097_SET_TRANSFORM_BRANCH_BITS	: return "SET_TRANSFORM_BRANCH_BITS";
        case CELL_GCM_DRIVER_QUEUE: return "DRIVER_QUEUE";
        default: return "unknown";
    }
}

void dump_fifo(uint32_t *begin,uint32_t *stop) 
{
    while (begin < stop) {
        if (*begin)
            if (*begin & CELL_GCM_METHOD_FLAG_JUMP)
                grcDisplayf("JUMP to %x",*begin & ~CELL_GCM_METHOD_FLAG_JUMP);
            else if (*begin & CELL_GCM_METHOD_FLAG_CALL)
                grcDisplayf("CALL to %x",*begin & ~CELL_GCM_METHOD_FLAG_CALL);
            else if (*begin & CELL_GCM_METHOD_FLAG_RETURN)
                grcDisplayf("RETURN (%x)",*begin);
            else {
                grcDisplayf("WRITE %x bytes to %x (%s)",(*begin>>16)&(2047<<2),*begin&0xFFFF,regname(*begin&0xFFFF));
                int count = (*begin >> 18) & 2047;
                for (int i=0; i<count; i++,begin++) {
                    if ((i & 7) == 0)
                        Printf("    ");
                    Printf("%08x ",begin[1]);
                    if ((i & 7) == 7)
                        Printf("\n");
                }
                if (count & 7)
                    Printf("\n");
            }
            ++begin;
    }
}

void AnalyseFifo(u32* begin, u32* end) 
{
    for(; begin < end; ++begin)
    {
        // todo: add something to check whether each command & jump/call addresses are actually valid!
        if (*begin & (CELL_GCM_METHOD_FLAG_JUMP|CELL_GCM_METHOD_FLAG_CALL))
        {
            u32 jumpAddr = *begin & ~(CELL_GCM_METHOD_FLAG_JUMP|CELL_GCM_METHOD_FLAG_CALL);
            const char* type = *begin & CELL_GCM_METHOD_FLAG_JUMP ? "JUMP" : "CALL";
            grcDisplayf("%p : %s to %x", begin, type, jumpAddr);
        }
        else 
        {
            int count = (*begin >> 18) & 2047;
            begin += count;
        }
    }
}

} // namespace rage

#endif // !__FINAL
#endif // 0