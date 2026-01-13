/////////////////////////////////////////////////////////////////////////////////
// FILE		: debugdraw.h
// PURPOSE	: To provide in game visual helpers for debugging.
// AUTHOR	: G. Speirs, Adam Croston, Alex Hadjadj
// STARTED	: 10/08/99
/////////////////////////////////////////////////////////////////////////////////
#ifndef GRCORE_DEBUGDRAW_H
#define GRCORE_DEBUGDRAW_H

// rage headers
#include "grcore/im.h"			// For __IM_BATCHER
#include "grcore/effect_mrt_config.h"
#include "vector/color32.h"
#include "vector/vector3.h"
#include "vectormath/legacyconvert.h"

// Game header
#define DEBUG_DRAW (__IM_BATCHER)

#if !DEBUG_DRAW
#define DEBUG_DRAW_ONLY(...)
#define DEBUG_DRAW_SCOPED_THREAD_ENABLE()    (void)0
#else
#define DEBUG_DRAW_ONLY(...)	__VA_ARGS__
namespace rage {
	class bkBank;
	class grcRenderTarget;
	class grcViewport;

enum DD_ePixelCoordSpace // TODO -- share this with ePixelCoordSpace in sprite2d.h used by Blit() .. which should itself be moved out of sprite2d
{
	DD_ePCS_Normalised, // [-1..1]
	DD_ePCS_NormalisedZeroToOne, // [0..1]
	DD_ePCS_Pixels,
};

namespace grcDebugDraw
{
	enum
	{
		NUM_DEBUG_COLORS = 64,
	};

	enum LingerBuffer
	{
		LINGER_2D,
		LINGER_3D,

		MAX_LINGER_BUFFERS
	};

	struct MemoryAllocationInterface
	{
		virtual ~MemoryAllocationInterface() {}
		virtual void Init() {};
		virtual void Shutdown() {};
		virtual void *Alloc(size_t size) = 0;
		virtual void Free(void *mem) = 0;
	};

	struct TextRenderer
	{
		virtual ~TextRenderer() {};
		virtual void Begin() {};
		virtual void RenderText(Color32 color, int xPos, int yPos, const char *string, bool proportional, bool drawBGQuad, bool rawCoords) = 0;
		virtual void End() {};
		virtual int GetTextHeight(bool proportional) const = 0;
	};

	//--------------------------------------------------------------------------
	// System
	void Init(grcDepthStencilStateHandle depthTestOnOverride);
	void Shutdown();
	void Update();

	// PURPOSE: Set the state for frozen mode. If this mode is enabled, all debug draw with
	// a negative "framesToLive" time will remain on the screen until the mode is disabled.
	void SetFrozenMode(bool frozenMode);

	void AddWidgets(bkBank &bank);
	void Render2D(u32 numBatchers);
	void Render3D(u32 numBatchers);

	void SetMemoryAllocationInterface(MemoryAllocationInterface *memoryAllocationInterface);

	void DiscardRenderBuffers();

	void CloseBatcher(LingerBuffer buffer);

	u32 GetNumBatchers(LingerBuffer buffer);

	typedef u32 BatcherHandle;
	bool IsExpectingSubRenderThreadDebugDraw(LingerBuffer buffer);
	BatcherHandle AllocateSubRenderBatcher(LingerBuffer buffer);
	void SetSubRenderBatcher(LingerBuffer buffer, BatcherHandle batcherHandle);
	// DLC_Add can't infer template type for function when there is multiple overloads, hence the silly 2 in the function name
	void ClearSubRenderBatcher2(LingerBuffer buffer, BatcherHandle batcherHandle);
	void ClearSubRenderBatcher(LingerBuffer buffer);
	void ClearRenderState();

	extern bool g_allowScriptDebugDraw;
	extern bool g_doDebugZTest;
	extern const Color32 g_debugCols[NUM_DEBUG_COLORS];
	extern Vector2 g_aspect2D;

	extern float g_textDist3DNear;
	extern float g_textDist3DFar;

	extern TextRenderer *g_textRenderer;

	// to prevent nested render states pushes
	extern int g_PushRasterizerStateCalls;
	extern int g_PushBlendStateCalls;
	extern int g_PushDepthStencilStateCalls;
	extern int g_PushLightingStateCalls;

	//--------------------------------------------------------------------------
	// Controls and Queries
	bool GetDisableCulling();
	void SetDisableCulling(bool bDisableCulling);
	inline const Vector2& Get2DAspect() {return g_aspect2D;}
	bool GetDoDebugZTest();
	void SetDoDebugZTest(bool value);
	bool GetGlobalDoDebugZTest();
	void SetGlobalDoDebugZTest(bool value);
	void SetDebugDrawAdditively(bool value);
	s32 GetScreenSpaceTextHeight();
	void SetScreenSpaceTextHeight(s32 iTextHeight);
	float GetDrawLabelScaleX();
	float GetDrawLabelScaleY();
	void SetDrawLabelScaleX(float xScale);
	void SetDrawLabelScaleY(float yScale);
	bool GetDisplayDebugText();
	void SetDisplayDebugText(bool display);
	bool GetIsInitialized();
	bool GetDisplayOverBudgetTimebarText();
	inline const Color32 *GetDebugColor(u32 idx) { return(&g_debugCols[(idx%NUM_DEBUG_COLORS)]);}
	void SetDefaultDebugRenderStates();
	bool StartRecording(LingerBuffer buffer, int framesToLive);
	void EndRecording(LingerBuffer buffer);
	const u32 *GetCurrentBatcherCursor();
	//---------------
	void PushRasterizerState(grcRasterizerStateHandle RasterizerStateHandle);
	void PushBlendState(grcBlendStateHandle BlendStateHandle);
	void PushDepthStencilState(grcDepthStencilStateHandle depthStencilHandle);
	bool PushLightingState(bool lightingState, int framesToLive=1);
	void PopRasterizerState();
	void PopBlendState();
	void PopDepthStencilState();
	void PopLightingState();

	//--------------------------------------------------------------------------
	// 2d debug drawing
	// All functions are expecting (0,0) to (1,1) space coordinates unless specified
	// Use Get2DAspect() to get aspect-ratio correct unit vector
	void Line				(Vec2V_In v1, Vec2V_In v2, const Color32 col, int framesToLive = 1);
	void Line				(Vec2V_In v1, Vec2V_In v2, const Color32 color1, const Color32 color2, int framesToLive = 1);
	void Quad				(Vec2V_In v1, Vec2V_In v2, Vec2V_In v3, Vec2V_In v4, const Color32 col, bool solid = true, int framesToLive = 1);
	void Poly				(Vec2V_In v1, Vec2V_In v2, Vec2V_In v3, const Color32 col, bool solid = true, int framesToLive = 1);
	void Poly				(Vec2V_In v1, Vec2V_In v2, Vec2V_In v3, const Color32 color1, const Color32 color2, const Color32 color3, bool solid = true, int framesToLive = 1);
	void Cross				(Vec2V_In pos, const float &size, const Color32 col, int framesToLive = 1);
	void Circle				(Vec2V_In center, float radius, Color32 col, bool solid = true, int numSides = 12, bool correctAspect = true, int framesToLive = 1);
	void Arc				(Vec2V_In center, float r, const Color32 col, float fBeginAngle, float fEndAngle, bool solid = false, int numSides = 12, bool correctAspect = true, int framesToLive = 1);
	void RectAxisAligned	(Vec2V_In min, Vec2V_In max, Color32 col, bool solid = true, int framesToLive = 1);
	void RectOriented		(Vec2V_In localMin, Vec2V_In localMax, const float &rotRadians, Vec2V_In translation, Color32 col, bool solid = true, int framesToLive = 1);
	void RectOriented		(Vec2V_In localMin, Vec2V_In localMax, Vec2V_In xBasis, Vec2V_In yBasis, Vec2V_In translation, Color32 col, bool solid = true, int framesToLive = 1);
	void Axis				(const float &rotRadians, Vec2V_In translation, const float &scale, int framesToLive = 1);
	void Axis				(Vec2V_In xBasis, Vec2V_In yBasis, Vec2V_In translation, const float &scale, int framesToLive = 1);

	inline void Line		(const Vector2& v1, const Vector2& v2, const Color32 col, int framesToLive = 1)
	{
		Line(Vec2V(v1.x, v1.y), Vec2V(v2.x, v2.y), col, framesToLive);
	}

	inline void Line		(const Vector2& v1, const Vector2& v2, const Color32 color1, const Color32 color2, int framesToLive = 1)
	{
		Line(Vec2V(v1.x, v1.y), Vec2V(v2.x, v2.y), color1, color2, framesToLive);
	}

	inline void Quad		(const Vector2& v1, const Vector2& v2, const Vector2& v3, const Vector2& v4, const Color32 color, bool solid = true, int framesToLive = 1)
	{
		Quad(Vec2V(v1.x, v1.y), Vec2V(v2.x, v2.y), Vec2V(v3.x, v3.y), Vec2V(v4.x, v4.y), color, solid, framesToLive);
	}

	inline void Poly		(const Vector2& v1, const Vector2& v2, const Vector2& v3, const Color32 col, bool solid = true, int framesToLive = 1)
	{
		Poly(Vec2V(v1.x, v1.y), Vec2V(v2.x, v2.y), Vec2V(v3.x, v3.y), col, solid, framesToLive);
	}

	inline void Poly		(const Vector2& v1, const Vector2& v2, const Vector2& v3, const Color32 color1, const Color32 color2, const Color32 color3, bool solid = true, int framesToLive = 1)
	{
		Poly(Vec2V(v1.x, v1.y), Vec2V(v2.x, v2.y), Vec2V(v3.x, v3.y), color1, color2, color3, solid, framesToLive);
	}

	inline void Cross		(const Vector2& pos, const float &size, const Color32 col, int framesToLive = 1)
	{
		Cross(Vec2V(pos.x, pos.y), size, col, framesToLive);
	}

	inline void Circle		(const Vector2& center, float radius, Color32 col, bool solid = true, int numSides = 12, bool correctAspect = true, int framesToLive = 1)
	{
		Circle(Vec2V(center.x, center.y), radius, col, solid, numSides, correctAspect, framesToLive);
	}

	inline void Arc			(const Vector2& center, float r, const Color32 col, float fBeginAngle, float fEndAngle, bool solid = false, int numSides = 12, bool correctAspect = true, int framesToLive = 1)
	{
		Arc(Vec2V(center.x, center.y), r, col, fBeginAngle, fEndAngle, solid, numSides, correctAspect, framesToLive);
	}

	inline void RectAxisAligned	(const Vector2& min, const Vector2& max, Color32 col, bool solid = true, int framesToLive = 1)
	{
		RectAxisAligned(Vec2V(min.x, min.y), Vec2V(max.x, max.y), col, solid, framesToLive);
	}

	inline void RectOriented(const Vector2& localMin, const Vector2& localMax, const float &rotRadians, const Vector2& translation, Color32 col, bool solid = true, int framesToLive = 1)
	{
		RectOriented(Vec2V(localMin.x, localMin.y), Vec2V(localMax.x, localMax.y), rotRadians, Vec2V(translation.x, translation.y), col, solid, framesToLive);
	}

	inline void RectOriented(const Vector2& localMin, const Vector2& localMax, const Vector2& xBasis, const Vector2& yBasis, const Vector2& translation, Color32 col, bool solid = true, int framesToLive = 1)
	{
		RectOriented(Vec2V(localMin.x, localMin.y), Vec2V(localMax.x, localMax.y), Vec2V(xBasis.x, xBasis.y), Vec2V(yBasis.x, yBasis.y), Vec2V(translation.x, translation.y), col, solid, framesToLive);
	}

	inline void Axis		(const float &rotRadians, const Vector2& translation, const float &scale, int framesToLive = 1)
	{
		Axis(rotRadians, Vec2V(translation.x, translation.y), scale, framesToLive);
	}

	inline void Axis		(const Vector2& xBasis, const Vector2& yBasis, const Vector2& translation, const float &scale, int framesToLive = 1)
	{
		Axis(Vec2V(xBasis.x, xBasis.y), Vec2V(yBasis.x, yBasis.y), Vec2V(translation.x, translation.y), scale, framesToLive);
	}


	// Useful methods for drawing values on a scale
	// Scale are scaled from -1 to 1.
	void Meter				(Vec2V_In axisPositionStart, Vec2V_In axisDir, const float &axisLength, const float &endPointWidth, const Color32 axisColor, const char* axisName, int framesToLive = 1);
	void MeterValue			(Vec2V_In axisPositionStart, Vec2V_In axisDir, const float &axisLength, const float &val, const float &valWidth, const Color32 valColor, bool bFull = false, int framesToLive = 1);

	inline void Meter		(const Vector2& axisPositionStart, const Vector2& axisDir, const float &axisLength, const float &endPointWidth, const Color32 axisColor, const char* axisName, int framesToLive = 1)
	{
		Meter(Vec2V(axisPositionStart.x, axisPositionStart.y), Vec2V(axisDir.x, axisDir.y), axisLength, endPointWidth, axisColor, axisName, framesToLive);
	}

	inline void MeterValue	(const Vector2& axisPositionStart, const Vector2& axisDir, float axisLength, const float &val, const float &valWidth, const Color32 valColor, bool bFull = false, int framesToLive = 1)
	{
		MeterValue(Vec2V(axisPositionStart.x, axisPositionStart.y), Vec2V(axisDir.x, axisDir.y), axisLength, val, valWidth, valColor, bFull, framesToLive);
	}

	// font stack (affects Text drawing functions)
	void           TextFontPush(const grcFont* font);
	void           TextFontPop ();
	const grcFont* TextFontGet ();

	// 2d text rendering
	void Text				(float x, float y,                                 const Color32 col, const char* pText, bool drawBGQuad = true, float scaleX = 1.0f, float scaleY = 1.0f, int framesToLive = 1);
	void Text				(float x, float y, DD_ePixelCoordSpace coordSpace, const Color32 col, const char* pText, bool drawBGQuad = true, float scaleX = 1.0f, float scaleY = 1.0f, int framesToLive = 1);

	// Flags for PrintToScreenCoors
	enum
	{
		FIXED_WIDTH             = 0x00000001,
		NO_BG_QUAD              = 0x00000002,
		RAW_COORDS              = 0x00000004,

		PTSC_PRIVATE_FLAGS      = 0xffff0000,
	};

	void PrintToScreenCoors	(const char *string,                      s32 x, s32 y, Color32 col=Color32(0.90f,0.90f,0.90f), u32 flags=0, int framesToLive = 1);
	void PrintToScreenCoors	(const char *string, const grcFont* font, s32 x, s32 y, Color32 col=Color32(0.90f,0.90f,0.90f), u32 flags=0, int framesToLive = 1);

	void AddDebugOutputEx	(bool proportional,                const char* fmt, ...);
	void AddDebugOutputEx	(bool proportional, Color32 color, const char* fmt, ...);
	void AddDebugOutputExV	(bool proportional,                const char* fmt, va_list args);
	void AddDebugOutputExV	(bool proportional, Color32 color, const char* fmt, va_list args);
	void AddDebugOutputExRaw(bool proportional, Color32 color, const char* fmt);
	void AddDebugOutput		(               const char* fmt, ...);
	void AddDebugOutput		(Color32 color, const char* fmt, ...);
	void AddDebugOutputSeparator(s32 pixelHeight);
	void BeginDebugOutputColumnized(s32 maxLines = -1, s32 colCharOffset = 40);
	int GetDebugOutputColumCount();
	void EndDebugOutputColumnized();
	void NewDebugOutputColumn(s32 charOffset);

	void SetTextRenderer	(TextRenderer &renderer);

	inline void Text(float x, float y, const char* pText, bool drawBGQuad = true, float scaleX = 1.0f, float scaleY = 1.0f, int framesToLive = 1)
	{
		Text(x, y, Color32(0.90f,0.90f,0.90f), pText, drawBGQuad, scaleX, scaleY, framesToLive);
	}

	inline void Text(float x, float y, DD_ePixelCoordSpace coordSpace, const char* pText, bool drawBGQuad = true, float scaleX = 1.0f, float scaleY = 1.0f, int framesToLive = 1)
	{
		Text(x, y, coordSpace, Color32(0.90f,0.90f,0.90f), pText, drawBGQuad, scaleX, scaleY, framesToLive);
	}

	inline void Text(Vec2V_In pos, const Color32 col, const char* pText, bool drawBGQuad = true, float scaleX = 1.0f, float scaleY = 1.0f, int framesToLive = 1)
	{
		Text(pos.GetXf(), pos.GetYf(), col, pText, drawBGQuad, scaleX, scaleY, framesToLive);
	}

	inline void Text(Vec2V_In pos, DD_ePixelCoordSpace coordSpace, const Color32 col, const char* pText, bool drawBGQuad = true, float scaleX = 1.0f, float scaleY = 1.0f, int framesToLive = 1)
	{
		Text(pos.GetXf(), pos.GetYf(), coordSpace, col, pText, drawBGQuad, scaleX, scaleY, framesToLive);
	}

	inline void Text(const Vector2& pos, const Color32 col, const char* pText, bool drawBGQuad = true, float scaleX = 1.0f, float scaleY = 1.0f, int framesToLive = 1)
	{
		Text(pos.x, pos.y, col, pText, drawBGQuad, scaleX, scaleY, framesToLive);
	}

	inline void Text(const Vector2& pos, DD_ePixelCoordSpace coordSpace, const Color32 col, const char* pText, bool drawBGQuad = true, float scaleX = 1.0f, float scaleY = 1.0f, int framesToLive = 1)
	{
		Text(pos.x, pos.y, coordSpace, col, pText, drawBGQuad, scaleX, scaleY, framesToLive);
	}

	enum GizmoInputType
	{
		NO_INPUT = 0,
		X_AXIS,
		Y_AXIS,
		Z_AXIS,
		XY_PLANE,
		XZ_PLANE,
		ZY_PLANE,
		ROT_X_PLANE, 
		ROT_Y_PLANE,
		ROT_Z_PLANE,
		FREE_AXIS, 
	};

	//--------------------------------------------------------------------------
	// 3d debug drawing
	// All functions are expecting world space coordinates
	void Line				(Vec3V_In v1, Vec3V_In v2, const Color32 col, int framesToLive = 1);
	void Line				(Vec3V_In v1, Vec3V_In v2, const Color32 color1, const Color32 color2, int framesToLive = 1);
	void Quad				(Vec3V_In v1, Vec3V_In v2, Vec3V_In v3, Vec3V_In v4, const Color32 col, bool drawDoubleSided = false, bool solid = true, int framesToLive = 1);
	void Poly				(Vec3V_In v1, Vec3V_In v2, Vec3V_In v3, const Color32 col,bool drawDoubleSided = false, bool solid = true, int framesToLive = 1);
	void Poly				(Vec3V_In v1, Vec3V_In v2, Vec3V_In v3, const Color32 color1, const Color32 color2, const Color32 color3, bool drawDoubleSided = false, bool solid = true, int framesToLive = 1);
	void Cross				(Vec3V_In pos, float size, const Color32 col, int framesToLive = 1, bool bZAxis = false);
	void Circle				(Vec3V_In center, float r, const Color32 col, Vec3V_In vXAxis, Vec3V_In vYAxis, bool dashed = false, bool solid = false, int framesToLive = 1, int numsides = 11);
	void Arc				(Vec3V_In center, float r, const Color32 col, Vec3V_In vXAxis, Vec3V_In vYAxis, float fBeginAngle, float fEndAngle, bool solid = false, int framesToLive = 1);
	void Sphere				(Vec3V_In center, float r, const Color32 col, bool solid = true, int framesToLive = 1, int steps=8, bool longitudinalCircles = true);
	void Ellipsoidal		(Mat34V_In mat, Vec3V_In size,const Color32 col, bool solid = true, int framesToLive = 1, int steps=8, bool longitudinalCircles = true);
	void PartialSphere		(Vec3V_In center, float r, Vec3V_In direction, float angle, const Color32 col, bool solid = true, int framesToLive = 1, int steps=8, bool longitudinalCircles = true);
	void BoxAxisAligned		(Vec3V_In min, Vec3V_In max, const Color32 col, bool solid = true, int framesToLive = 1);
	void BoxOriented		(Vec3V_In min, Vec3V_In max, Mat34V_In mat, const Color32 col, bool solid = true, int framesToLive = 1);
	void CubeOriented		(Vec3V_In center, float size, Mat34V_In mat, const Color32 col, bool solid = true, int framesToLive = 1);
	void Cylinder			(Vec3V_In start, Vec3V_In end, float r, const Color32 col, bool cap = true, bool solid = true, int numSides = 12, int framesToLive = 1);
	void Cylinder			(Vec3V_In start, Vec3V_In end, float r, const Color32 color1, const Color32 color2, bool cap = true, bool solid = true, int numSides = 12, int framesToLive = 1);
	void Cylinder			(Vec3V_In start, Vec3V_In end, Vec3V_In basisX, Vec3V_In basisY, Vec3V_In basisZ, float r, const Color32 col, bool cap = true, bool solid = true, int numSides = 12, int framesToLive = 1);
	void Cone				(Vec3V_In start, Vec3V_In end, float r, const Color32 col, bool cap = true, bool solid = true, int numSides = 12, int framesToLive = 1);
	void Cone				(Vec3V_In start, Vec3V_In end, Vec3V_In basisX, Vec3V_In basisY, Vec3V_In basisZ, float r, const Color32 col, bool cap = true, bool solid = true, int numSides = 12, int framesToLive = 1);
	void Capsule			(Vec3V_In start, Vec3V_In end, float r, const Color32 col, bool solid = true, int framesToLive = 1);
	void Axis				(Mat34V_In mtx, float scale, bool drawArrows = false, int framesToLive = 1);
	void Arrow				(Vec3V_In v1, Vec3V_In v2, float fArrowHeadSize, const Color32 col, int framesToLive = 1);
	void Proxy              (const grcDrawProxy& proxy, int framesToLive = 1);
	void SetCameraPosition	(Vec3V_In camPos);
	void SetCullingViewport	(const grcViewport *viewport);
	void GizmoPosition(Mat34V_In mtx, float scale, int selection = NO_INPUT);
	void GizmoRotation(Mat34V_In mtx, float scale, int selection = NO_INPUT);
	void GizmoScale(Mat34V_In mtx, float scale, int selection = NO_INPUT);
	void GizmoPosition_Internal(Mat34V_In mtx, float scale, bool faded, int selection);
	void GizmoRotation_Internal(Mat34V_In mtx, float scale, bool faded, int selection);
	void GizmoScale_Internal(Mat34V_In mtx, float scale, bool faded, int selection);

	inline void Line		(const Vector3& v1, const Vector3& v2, const Color32 col, int framesToLive = 1)
	{
		Line(VECTOR3_TO_VEC3V(v1), VECTOR3_TO_VEC3V(v2), col, framesToLive);
	}
//
	inline void Line		(const Vector3& v1, const Vector3& v2, const Color32 color1, const Color32 color2, int framesToLive = 1)
	{
		Line(VECTOR3_TO_VEC3V(v1), VECTOR3_TO_VEC3V(v2), color1, color2, framesToLive);
	}
//
	inline void Poly		(const Vector3& v1, const Vector3& v2, const Vector3& v3, const Color32 color1, const Color32 color2, const Color32 color3, bool drawDoubleSided = false, bool solid = true, int framesToLive = 1)
	{
		Poly(VECTOR3_TO_VEC3V(v1), VECTOR3_TO_VEC3V(v2), VECTOR3_TO_VEC3V(v3), color1, color2, color3, drawDoubleSided, solid, framesToLive);
	}
//
	inline void Circle		(const Vector3& center, float r, const Color32 col, const Vector3& vXAxis, const Vector3& vYAxis, bool dashed = false, bool solid = false, int framesToLive = 1)
	{
		Circle(VECTOR3_TO_VEC3V(center), r, col, VECTOR3_TO_VEC3V(vXAxis), VECTOR3_TO_VEC3V(vYAxis), dashed, solid, framesToLive);
	}
//
	inline void Sphere		(const Vector3& center, float r, const Color32 col, bool solid = true, int framesToLive = 1, int steps=8, bool longitudinalCircles = true)
	{
		Sphere(VECTOR3_TO_VEC3V(center), r, col, solid, framesToLive, steps, longitudinalCircles);
	}
//
	inline void Axis		(const Matrix34& mtx, float scale, bool drawArrows = false, int framesToLive = 1)
	{
		Axis(MATRIX34_TO_MAT34V(mtx), scale, drawArrows, framesToLive);
	}

	// 3d text rendering
	// (text is 2d in screen space but positioned in 3D)
	bool Text				(Vec3V_In pos, const Color32 col,                                                               const char *pText, bool drawBGQuad = true, int framesToLive = 1);
	bool Text				(Vec3V_In pos, const Color32 col, const s32 iScreenSpaceXOffset, const s32 iScreenSpaceYOffset, const char *pText, bool drawBGQuad = true, int framesToLive = 1);

	inline bool Text		(const Vector3& pos, const Color32 col, const char *pText, bool drawBGQuad = true, int framesToLive = 1)
	{
		return Text(VECTOR3_TO_VEC3V(pos), col, pText, drawBGQuad, framesToLive);
	}

	inline bool Text		(const Vector3& pos, const Color32 col, const s32 iScreenSpaceXOffset, const s32 iScreenSpaceYOffset, const char *pText, bool drawBGQuad = true, int framesToLive = 1)
	{
		return Text(VECTOR3_TO_VEC3V(pos), col, iScreenSpaceXOffset, iScreenSpaceYOffset, pText, drawBGQuad, framesToLive);
	}

} // namespace grcDebugDraw

} // namespace rage

#endif // DEBUG_DRAW

#endif // GRCORE_DEBUGDRAW_H
