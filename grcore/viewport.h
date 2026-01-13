//
// grcore/viewport.h
//
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved.
//

#ifndef GRCORE_VIEWPORT_H
#define GRCORE_VIEWPORT_H

#include "vectormath/vec4v.h"
#include "vectormath/mat34v.h"
#include "vectormath/mat44v.h"
#include "data/resource.h"
#include "grcore/device.h"
#include "grcore/effect.h"
#include "grcore/cullstatus.h"
#include "grmodel/model.h"

namespace rage {

class Matrix34;
class bkBank;

// Support adding an NDC-space depth bias in the projection matrix
#define SUPPORT_DEPTH_BIAS_IN_PROJECTION (0) // No longer used now that decals don't get bias'd
#if SUPPORT_DEPTH_BIAS_IN_PROJECTION
#define DEPTH_BIAS_IN_PROJECTION_ONLY(x)	x
#else
#define DEPTH_BIAS_IN_PROJECTION_ONLY(x)
#endif

/*
  PURPOSE
   grcViewport replaces the old gfxViewport class. It owns a
   projection matrix and knows how to perform visibility checks.
   Various useful composite matrices are kept up-to-date at all
   times. 
   
  NOTES
   There are several transforms maintained by the
   viewport class:
   
   
   <TABLE>
   Transform    Read            Write          \Description
                 Accessor        Accessor       
   -----------  --------------  -------------  -----------------------------------------
   World        GetWorld        SetWorld       First transform applied to all rendered
                                                \objects
   Camera       GetCamera       SetCamera      Second transform applied to all rendered
                                                \objects. In practice, we apply the
                                                inverse of the camera matrix after the
                                                world matrix to arrive at the modelview
                                                matrix.
   Projection   GetProjection   Perspective,   Transform which maps world space into
                                 Ortho,         normalized device space.
                                 Screen         
                                SetProjection
   Window       GetWindow       SetWindow,     Final transform which maps normalize
                                 ResetWindow    device space to a final screen position
                                                and depth range.
   </TABLE>
   
   
   There are two composite matrices available to clients,
   GetModelViewMtx, which returns World dot View, and
   GetCompositeMtx, which returns World dot View dot Projection  
 <FLAG Component>

 Strictly speaking, all sphere culling functions in grcViewport are computing visibility 
 of a bounding box aligned to the view frustum that inscribes the supplied sphere, and the 
 difference between the two means we return a false positive very near one of the twelve
 edges of the view frustum.

*/
class grcViewport {
public:
	enum eDoNotInitialize {
		DoNotInitialize,
	};

	// PURPOSE:	Constructor
	// NOTES:	GRCDEVICE.InitClass() must be called before any grcViewports can be created!
	grcViewport();

	// PURPOSE:	Destructor
	~grcViewport();

	const grcViewport& operator=(const grcViewport&);

	// PURPOSE: Dummy constructor that will not initialize anything - use this if you
	// plan to populate the viewport yourself.
	inline grcViewport(eDoNotInitialize) {};

	// RETURNS:	Pointer to default orthographic viewport (mapped to non-normalized screen pixels)
	inline static const grcViewport *GetDefaultScreen();

	// PURPOSE:	Perform world space sphere visibility check against view frustum
	// PARAMS:	x: Sphere center x coordinate
	// 			y: Sphere center y coordinate
	// 			z: Sphere center z coordinate
	//			radius: Sphere radius
	//			outZ: optional computed depth value (in eyespace)
	//			useStereoFrustum: override left & right clip planes with stereo clip planes
	// RETURNS:	Cull status (see enumerant; zero means offscreen)
	// NOTE:    The sphere center is assumed to be transformed by any relevant world matrix prior to
	//          sending it to IsSphereVisible();  This version wraps GetSphereCullStatus
	//			so it is faster to call that directly where possible.
	grcCullStatus IsSphereVisible(float x,float y,float z,float radius,float *outZ = NULL NV_SUPPORT_ONLY(,bool useStereoFrustum=false)) const;

	// PURPOSE:	Perform AABB visibility check against four cull planes
	// PARAMS:	vmin - AABB min corner (only xyz are accessed)
	//			vmax - AABB max corner (only xyz are accessed)
	//			cullPlanes - should be either this->GetFrustumLRTB() to work in world space, 
	//				or this->GetLocalLRTB() to work in model local space.
	// RETURNS: grcCullStatus enumerant
	// NOTES:	Only checks the four cull planes you specify, which means when checking against only LRTB,
	//			objects directly behind you that were large enough to cross all four planes could fail.
	//			Viewport class updates a local version of LRTB on every world matrix change
	//			If you only care whether something is visible or notm IsAABBVisible is faster.
	static grcCullStatus GetAABBCullStatus(Vec::V4Param128 vmin,Vec::V4Param128 vmax,Mat44V_In cullPlanes);

	// PURPOSE:	Perform AABB visibility check against four cull planes
	// PARAMS:	vmin - AABB min corner (only xyz are accessed)
	//			vmax - AABB max corner (only xyz are accessed)
	//			cullPlanes - should be either this->GetFrustumLRTB() to work in world space, 
	//				or this->GetLocalLRTB() to work in model local space.
	// RETURNS: Nonzero if object is visible, else zero
	// NOTES:	Only checks the four cull planes you specify, which means when checking against only LRTB,
	//			objects directly behind you that were large enough to cross all four planes could fail.
	// NOTES:	Viewport class updates a local version of LRTB on every world matrix change
	static int IsAABBVisible(Vec::V4Param128 vmin,Vec::V4Param128 vmax,Mat44V_In cullPlanes);

	// PURPOSE:	Perform AABB visibility check against eight cull planes
	// PARAMS:	vmin - AABB min corner (only xyz are accessed)
	//			vmax - AABB max corner (only xyz are accessed)
	//			cullPlanes1 - typically this->GetFrustumLRTB(), and works in world space
	//			cullPlanes2 - typically this->GetFrustumNFNF(), and works in world space
	static grcCullStatus GetAABBCullStatus(Vec::V4Param128 vmin,Vec::V4Param128 vmax,Mat44V_In cullPlanes1,Mat44V_In cullPlanes2);

	__forceinline static int IsAABBVisibleInline(Vec::V4Param128 vmin,Vec::V4Param128 vmax,Mat44V_In cullPlanes) ;
	__forceinline static grcCullStatus GetAABBCullStatusInline(Vec::V4Param128 vmin,Vec::V4Param128 vmax,Mat44V_In cullPlanes) ;
	__forceinline static grcCullStatus GetAABBCullStatusInline(Vec::V4Param128 vmin,Vec::V4Param128 vmax,Mat44V_In cullPlanes1,Mat44V_In cullPlanes2);

	// Test harness code, please ignore.
	__forceinline grcCullStatus GetAABBCullStatusInline_Test(Vec3V_In vmin,Vec3V_In vmax) const;
	__forceinline grcCullStatus GetAABBCullStatusInline_Test2(Vec3V_In vmin,Vec3V_In vmax) const;

#if RAGE_INSTANCED_TECH
	static grcCullStatus GetInstAABBCullStatus(const Vector3 &aabb_min, const Vector3 &aabb_max );
#endif

	// PURPOSE:	Perform world space sphere visibility check against view frustum
	// PARAMS:	sphere: Sphere center and radius (in w)
	// RETURNS:	True if sphere is not completely outside the view frustum planes.
	int IsSphereVisible(Vec4V_In sphere) const;

	// PURPOSE:	Perform world space sphere visibility check against view frustum
	// PARAMS:	sphere: Sphere center and radius (in w)
	//			outZ: distance from near clip plane
	// RETURNS:	True if sphere is not completely outside the view frustum planes.
	int IsSphereVisible(Vec4V_In sphere,float &outZ) const;

	// PURPOSE:	Perform world space sphere visibility check against view frustum
	// PARAMS:	sphere: Sphere center and radius (in w)
	// RETURNS:	True if sphere is not completely outside the view frustum planes.
	__forceinline int IsSphereVisibleInline(Vec4V_In sphere) const;

	// PURPOSE:	Perform world space sphere visibility check against view frustum
	// PARAMS:	sphere: Sphere center and radius (in w)
	//			outZ: distance from near clip plane
	// RETURNS:	True if sphere is not completely outside the view frustum planes.
	// NOTE:	Identical to overloaded IsSphereVisible above (which actually calls 
	//			this) but allows calling code to inline the test to maximize speed.
	__forceinline int IsSphereVisibleInline(Vec4V_In sphere,float &outZ) const;

	// PURPOSE:	Perform world space sphere visibility check against view frustum
	// PARAMS:	sphere: Sphere center and radius (in w)
	//			outZ: distance from near clip plane
	// RETURNS:	Cull status (see enumerant; zero means offscreen)
	grcCullStatus GetSphereCullStatus(Vec4V_In sphere) const;

	// PURPOSE:	Perform world space sphere visibility check against view frustum
	// PARAMS:	sphere: Sphere center and radius (in w)
	//			outZ: distance from near clip plane
	//			useStereoFrustum: override left & right clip planes by stereo clip planes
	// RETURNS:	Cull status (see enumerant; zero means offscreen)
	// NOTE:	Identical to overloaded GetSphereCullStatus above (which actually calls 
	//			this) but allows calling code to inline the test to maximize speed.
	__forceinline grcCullStatus GetSphereCullStatusInline(Vec4V_In sphere NV_SUPPORT_ONLY(,bool useStereoFrustum=false)) const;

	grcCullStatus GetSphereCullStatus(Vec4V_In sphere,float &outZ) const;
	// PURPOSE:	Perform world space sphere visibility check against view frustum
	// PARAMS:	sphere: Sphere center and radius (in w)
	//			outZ: distance from near clip plane
	//			useStereoFrustum: override left & right clip planes by stereo clip planes
	// RETURNS:	Cull status (see enumerant; zero means offscreen)
	// NOTE:	Identical to overloaded GetSphereCullStatus above (which actually calls 
	//			this) but allows calling code to inline the test to maximize speed.
	__forceinline grcCullStatus GetSphereCullStatusInline(Vec4V_In sphere,float &outZ NV_SUPPORT_ONLY(,bool useStereoFrustum=false)) const;

	// PURPOSE:	Perform model space sphere visibility check against view frustum
	// PARAMS:	sphere.xyz: Sphere center
	//			sphere.w:   Sphere radius
	//			outZ: optional computed depth value (in eyespace)
	// RETURNS:	Cull status (see enumerant; zero means offscreen)
	// NOTE:    Since the the sphere center is in model space, it will be transformed by the world
	//			matrix prior to checking its visibility against the view frustum (which we
	//			do by calling IsSphereVisible, which itself wraps GetCullSphereStatus, so the
	//			overhead is starting to add up).
	grcCullStatus IsModelSphereVisible(Vec4V_In sphere,float *outZ = NULL) const;

	// PURPOSE: Determine whether the convex hull set of points is at least partially visible.
	//          This is a conservative test, so it may return false positives (say that something is visible when it's not)
	//          but it will never return a false negative (say that something is not visible when it is).
	// PARAMS:  PointSet_WS: A pointer to an array of points, in world space, that you want tested.
	//          kNumPoints: The number of points pointed to be PointSet_WS.
	// RETURNS: True if the point set is visible, false if it is not.
	// NOTES:	You probably want the sphere or AABB vis functions instead.
	bool IsPointSetVisible(const Vec3V *const PointSet_WS, const int kNumPoints) const;

	// PURPOSE: Determine whether a point is visible.
	// PARAMS:  point: Point, in world space, that you want tested.
	// RETURNS: True if the point is visible, false if it is not.
	// NOTES:	This test does not check if the point is within the near/far bounds
	bool IsPointVisible(Vec3V_In point NV_SUPPORT_ONLY(, bool useStereoFrustum=false)) const;

	// PURPOSE:	Sets world matrix associated with this viewport
	// PARAMS:	worldMtx - New world matrix to use
	//			prevWorldMtx - Pointer to optional previous matrix for motion blur
	// NOTES:	rmcState::SetWorld and rmcState::SetIdentity simply call this
	//			function on the currently active viewport.  This version is faster
	//			if you already have the inverse transpose available, such as when it's
	//			the identity matrix.
	void SetWorldMtx(Mat34V_In worldMtx);
	void SetWorldMtx(Mat44V_In worldMtx);
	void SetWorldIdentity();

#if RAGE_INSTANCED_TECH
	static void SetViewportInstancedRenderBit(u32 uBit)	{ sm_viewportInstancedRenderBit = uBit; }
	static u32 GetViewportInstancedRenderBit()			{ return sm_viewportInstancedRenderBit; }
	static void RegenerateInstVPMatrices(const Mat44V& worldmat, int numInst = 1);

	static void SetCurrentWorldMtxFn_NonInstanced(Mat34V_In worldMtx);
	static void SetCurrentWorldMtxFn_Instanced(Mat34V_In worldMtx);
	static void SetCurrentWorldIdentityFn_NonInstanced();
	static void SetCurrentWorldIdentityFn_Instanced();
	static void SetCurrentNearFarClipFn_NonInstanced(float fNear, float fFar);
	static void SetCurrentNearFarClipFn_Instanced(float fNear, float fFar);
	static void SetCurrentWindowMinMaxZFn_NonInstanced(float minz, float maxz);
	static void SetCurrentWindowMinMaxZFn_Instanced(float minz, float maxz);
#endif
	static void SetCurrentWorldMtx(Mat34V_In worldMtx);
	static void SetCurrentWorldIdentity();
	static void SetCurrentNearFarClip(float fNear, float fFar);
	static void SetCurrentWindowMinMaxZ(float minz, float maxz);

	// PURPOSE:	Sets camera matrix associated with this viewport
	// PARAMS:	camMtx - New camera matrix to use
	// NOTES:	rmcState::SetCamera simply calls this function on the currently 
	//			active viewport.  Internally the viewport computes the inverse
	//			of the camera matrix to arrive at the view matrix that is used
	//			by the composite matrix calculations.
	void SetCameraMtx(Mat34V_In camMtx);
	void SetCameraIdentity();

	// RETURNS: Current world matrix
	inline Mat34V_ConstRef GetWorldMtx() const;

	static Mat34V_ConstRef GetCurrentWorldMtx() { return GetCurrent()->GetWorldMtx(); }

	// RETURNS: Current world matrix
	inline Mat44V_ConstRef GetWorldMtx44() const;

	// RETURNS: Current camera matrix
	inline Mat34V_ConstRef GetCameraMtx() const;

	inline Vec3V_Out GetCameraPosition() const { return m_Camera44.GetCol3().GetXYZ(); }

	static Mat34V_ConstRef GetCurrentCameraMtx() { return GetCurrent()->GetCameraMtx(); }

	static Vec3V_Out GetCurrentCameraPosition() { return GetCurrent()->GetCameraPosition(); }

	// RETURNS: Current camera matrix
	inline Mat44V_ConstRef GetCameraMtx44() const;

	// RETURNS: Current screen matrix
	inline Mat44V_ConstRef GetScreenMtx() const;

	// RETURNS: Current full composite matrix (world * view * projection * screen)
	inline Mat44V_ConstRef GetFullCompositeMtx() const;

	// RETURNS: Current composite matrix (world * view * projection)
	inline Mat44V_ConstRef GetCompositeMtx() const;

	// RETURNS: Current composite matrix (world * view * projection) for stereo culling (on PC) or regular composite (consoles)
	inline Mat44V_ConstRef GetCullCompositeMtx() const;

#if NV_SUPPORT
	// RETURNS: Current full composite matrix (world * view * projection * screen) for stereo culling (on PC)
	// NOTES: Calculates full composite on demand (fullcomposite for stereo is not stored anywhere)
	inline Mat44V_Out GetCullFullCompositeMtx() const;
#endif

	// RETURNS: Current modelview matrix (world * view )
	inline Mat44V_ConstRef GetModelViewMtx() const;

	// RETURNS: Current view matrix (inverse of camera matrix)
	inline Mat44V_ConstRef GetViewMtx() const;

	// PURPOSE:	Modify near clip on current projection matrix
	// PARAMS:	zNear - New z near clip value (in positive eyespace units)
	void SetNearClip(float zNear);

	// PURPOSE:	Modify far clip on current projection matrix
	// PARAMS:	zFar - New z far clip value (in positive eyespace units)
	void SetFarClip(float zFar);

	// PURPOSE:	Modify near and far clip on current projection matrix
	// PARAMS:	zNear - New z near clip value (in positive eyespace units)
	//			zFar - New z far clip value (in positive eyespace units)
	void SetNearFarClip(float zNear, float zFar);

	// RETURNS:	Current near clip plane value
	inline float GetNearClip() const;

	// RETURNS: Current far clip plane value
	inline float GetFarClip() const;

	// RETURNS:	Tangent of the horizontal field of view
	inline float GetTanHFOV() const;

	// RETURNS:	Tangent of the vertical field of view
	inline float GetTanVFOV() const;

	// RETURNS: Current vertical field of view (in degrees) 
	inline float GetFOVY() const;

	// RETURNS: Rectangular bound of FOV tangents 
	inline fwRect GetTanFOVRect() const;

	// PURPOSE:	Set projection matrix to a perspective transform
	// PARAMS:	fovy - Vertical field of view, in degrees
	//			aspect - aspect ratio; leave at zero to auto-compute from pixel aspect
	//				ratio of device and actual window size.
	//			znear - Near clip value (in positive eyespace units)
	//			zfar - Far clip value (in positive eyespace units)
	void Perspective(float fovyDegrees,float aspect,float znear,float zfar);

	// PURPOSE:	Set projection matrix to an orthographic transform
	// PARAMS:	left, right, bottom, top - Eyespace coordinates of each edge, as per OpenGL
	//			znear - Near clip value (in positive eyespace units)
	//			zfar - Far clip value (in positive eyespace units)
	void Ortho(float left,float right,float bottom,float top,float znear,float zfar,bool setFOVToZero=false);

	// PURPOSE:	Set projection matrix to a screenspace transform
	// NOTES:	Simply calls Ortho(x,x+width,y+height,y,-1,+1) based on current window transform
	void Screen();

	// RETURNS: Width of this viewport's screen window
	inline int GetWidth() const;

	// RETURNS: Height of this viewport's screen window
	inline int GetHeight() const;

	// PURPOSE:	Sets current window transform to full device resolution
	// NOTES:	Simply calls SetWindow(0,0,GRCDEVICE.GetWidth(),GRCDEVICE.GetHeight());
	void ResetWindow();

	// RETURNS:	Current window transform
    // NOTES:   The current window is the window used for rendering and may be
    //          different than that passed to SetWindow() if it was clipped to
    //          the edges of the display.
    //          Call GetUnclippedWindow() to retrieve the window parameters
    //          passed to SetWindow().
	const grcWindow& GetWindow() const;

    // RETURNS: Unclipped window transform.
    // NOTES:   Call GetUnclippedWindow() when the aim is to retrieve the window
    //          parameters passed to GetWindow().
    //          The window actually used for rendering (returned by GetWindow())
    //          may have been clipped to the edges of the display.
    const grcWindow& GetUnclippedWindow() const;

	// RETURNS:	Current projection matrix
	Mat44V_ConstRef GetProjection() const { return m_Projection; }

	// PURPOSE:	Sets current projection matrix directly for certain special effects.
	//			You should generally call GetProjection first and then multiply in
	//			your additional transform, because the projection matrix is partially
	//			platform-dependent.
	// PARAMS:	projMtx - New projection matrix to use.
	// NOTES:	The sphere culling line equations will NOT be properly updated if you
	//			use this function, so don't expect sphere culling to work.  Call
	//			ResetPerspective to regenerate the most recently used perspective matrix 
	//			and make it active.
	void SetProjection(Mat44V_In projMtx);

	// PURPOSE:	Restore the previous perspective matrix
	// NOTES:	Intended for use after calling SetProjection directly.
	void ResetPerspective();

	// RETURNS: Current aspect ratio
	inline float GetAspectRatio() const;

	// RETURNS: Perspective shear vector
	inline Vector2 GetPerspectiveShear() const;

	inline Vector2 GetWidthHeightScale() const;

	// PURPOSE:	Set the shear X and Y used to build a perspective matrix
	inline void SetPerspectiveShear(float shearX, float shearY);

	// PURPOSE:	Sets current window transform
	// PARAMS:	x - x coordinate of left edge
	//			y - y coordinate of top edge
	//			width - Width of window (in screen pixels)
	//			height - Height of window (in screen pixels)
	//			zmin - Map nearclip depth value to this value (nearly always want zero here)
	//			zmax - Map farclip depth value to this value (nearly always want one here)
	void SetWindow(int x,int y,int width,int height,float zmin = 0.0f,float zmax = 1.0f);
	void SetWindowMinMaxZ(float zmin,float zmax);

	// PURPOSE:	Sets current window transform using normalized coordinates
	// PARAMS:	x - x coordinate of left edge
	//			y - y coordinate of top edge
	//			width - Width of window
	//			height - Height of window
	//			zmin - Map nearclip depth value to this value (nearly always want zero here)
	//			zmax - Map farclip depth value to this value (nearly always want one here)
	void SetNormWindow(float x,float y,float width,float height,float zmin = 0.0f,float zmax = 1.0f);

	static void SetNormWindow_GRCDEVICE(float x,float y,float w,float h,float zmin,float zmax);
	void SetNormWindowPortal_UpdateThreadOnly(float x, float y, float w, float h, float windowX = 0.0f, float windowY = 0.0f, float windowW = 1.0f, float windowH = 1.0f);

	// PURPOSE:	Transforms world coordinates into window space
	// PARAMS:	worldPosition - 3D world position to transform to window space
	//			windowX - X window coordinate
	//			windowY - Y window coordinate
	void Transform (Vec3V_In worldPosition, float& windowX, float& windowY) const;

	// PURPOSE:	Transforms world coordinates into window space
	// PARAMS:	worldPosition - 3D world position to transform to window space
	// RETURNS:	window - window coordinates
	Vec3V_Out Transform (Vec3V_In worldPosition) const;

	// PURPOSE:	Reverse-transforms window coordinates into local space
	// PARAMS:	x - X window coordinate
	//			y - Y window coordinate
	//			outNear - Point in local space on near clip plane
	//			outFar - Point in local space on far clip plane
	// NOTES:	Be careful rendering a 2D primitive directly on either clip plane;
	//			it might get completely clipped out.  You may want to lerp by a small
	//			value from near to far (or vice versa) first.  Window coordinates
	//			are relative to the viewport's window, so a raw mouse cursor position
	//			may need to account for the viewport window's position on the screen.
	//			This function includes the current world matrix in the computation,
	//			so if the world matrix is not identity the line segment will end up
	//			in object local space, which may not have been what you expected.
	void ReverseTransform(float x,float y,Vec3V_InOut outNear,Vec3V_InOut outFar) const;

	// PURPOSE:	Reverse-transforms window coordinates into local space, ignoring the world matrix
	// PARAMS:	x - X window coordinate
	//			y - Y window coordinate
	//			outNear - Point in local space on near clip plane
	//			outFar - Point in local space on far clip plane
	// NOTES:	Like ReverseTransform, but assumes the world matrix is identity
	void ReverseTransformNoWorld(float x,float y,Vec3V_InOut outNear,Vec3V_InOut outFar) const;

	// RETURNS: True if viewport is perspective type
	inline bool IsPerspective() const;

	// RETURNS: The LRTB vector that represents the left/right/top/bottom dimensions
	// of this viewport. Used in orthogonal viewports only.
	inline Vec4V_Out GetLRTB() const
	{
		Assert(!IsPerspective());
		return m_OrthoLRTB;
	}

#if SUPPORT_INVERTED_PROJECTION
	
	// PURPOSE:	Toggle whether the projection matrix inverts z (1-z). For platforms that use an 
	//			inverted depth buffer, this can further improve depth buffer precision.
	void SetInvertZInProjectionMatrix(bool set, bool regen=true);

	// PURPOSE:	Query if the projection matrix inverts z (1-z).
	inline bool GetInvertZInProjectionMatrix() const {	return m_InvertZInProjectionMatrix; }

#endif // SUPPORT_INVERTED_PROJECTION

#if SUPPORT_DEPTH_BIAS_IN_PROJECTION

	// PURPOSE: Add an NDC-space depth bias to the projection matrix. This achieves a similiar 
	//			result to what most graphics APIs call a DepthBias but doesn't have the	overhead
	//			of changing render states.
	inline void SetNDCDepthBias(float bias, bool regen=true);

	// PURPOSE: Get the current depth bias
	inline float GetNDCDepthBias() const { return m_NDCDepthBias; }

#endif // SUPPORT_DEPTH_BIAS_IN_PROJECTION

	// PURPOSE:	Changes active viewport.
	// PARAMS:	viewport - New viewport.  Can be NULL if you don't need to render anything.
	// RETURNS:	Previously active viewport
	static grcViewport* SetCurrent(const grcViewport *viewport, bool regenDevice = true);
#if RAGE_INSTANCED_TECH
	static void SetInstancing(bool bEnable);
	static bool GetInstancing()	{ return sm_bInstancing; }
	static void SetInstVP(const grcViewport *viewport, int index)	{ sm_InstViewport[g_RenderThreadIndex][index] = const_cast<grcViewport*>(viewport); }
	static void SetInstMatrices();
	static void SetCurrentInstVP(const grcViewport *viewport, Vec4V_In window, int index , bool dDefault = false);
	static void SetInstVPWindow(u32 uNumMaxInst);
	static inline void SetNumInstVP(u32 uNum)		{	sm_uNumInstVP = uNum; }
	static inline u32 GetNumInstVP()				{ return sm_uNumInstVP; }
	static inline u32 GetVPInstCount()				{ return sm_uVPInstCount; }

	static inline grcViewport* GetCurrentInstVP(int index) {
		AssertMsg(GRCDEVICE.CheckThreadOwnership(),"Thread that doesn't own the GPU trying to get current viewport.");
		return sm_InstViewport[g_RenderThreadIndex][index];
	}
#endif

	// RETURNS:	Currently active viewport (may be NULL)
	static inline grcViewport* GetCurrent() { 
		AssertMsg(GRCDEVICE.CheckThreadOwnership(),"Thread that doesn't own the GPU trying to get current viewport.");
		return sm_Current; 
	}

	// PURPOSE:	Called internally by rmcDevice::InitClass to create the
	//			viewport returned by GetDefaultScreen
	static void InitClass();

	// PURPOSE:	Called internally by rmcDevice::Shutdownlass to destroy the
	//			viewport returned by GetDefaultScreen
	static void ShutdownClass();

#if __BANK
	void DebugGetExtents(Vec3V extents[8]) const;
	void DebugDraw() const;
	void AddWidgets(bkBank &);
#endif

	enum { CLIP_PLANE_LEFT, CLIP_PLANE_RIGHT, CLIP_PLANE_TOP, CLIP_PLANE_BOTTOM, CLIP_PLANE_NEAR, CLIP_PLANE_FAR };

	// PURPOSE:	Returns the specified clip plane (CLIP_PLANE_... above)
	// NOTES:	The plane equation is of the form distance=Ax+By+Cz+W, which is more amenable to vectorization.
	//			Unfortunately this is inconsistent with the Vector4 versions which are very old and predate
	//			vectorized implementations; they use distance=Ax+By+Cz-W.
	Vec4V_Out GetFrustumClipPlane(int whichPlane) const;
	const Vec4V * GetFrustumClipPlaneArray() const;

#if NV_SUPPORT
	// PURPOSE:	Returns the modified clip plane that should be used for stereo culling (CLIP_PLANE_LEFT and CLIP_PLANE_RIGHT only)
	// NOTES:	Same as for GetFrustumClipPlane(int whichPlane).
	Vec4V_Out GetCullFrustumClipPlane(int whichPlane) const;
#endif

	// PURPOSE: Transforms the projection matrix so the near clip plane is aligned with the plane passed in
	//
	// NOTES:	This allows for the same effect as a single clip plane to be implemented without the requirement
	//			for a clip plane. See http://www.terathon.com/code/oblique.php or GP Gems 5 ( Section 2.6 ) for more info.
	//
	void ApplyObliqueProjection( Vec4V_In worldSpacePlane );

#if NV_SUPPORT
	Mat44V_ConstRef GetCullFrustumLRTB() const { return m_CullFrustumLRTB; }
#else
	Mat44V_ConstRef GetCullFrustumLRTB() const { return m_FrustumLRTB; }
#endif
	Mat44V_ConstRef GetFrustumLRTB() const { return m_FrustumLRTB; }

	Mat44V_ConstRef GetFrustumNFNF() const { return m_FrustumNFNF; }

	Mat44V_ConstRef GetLocalLRTB() const { return m_LocalLRTB; }

#if NV_SUPPORT
	Mat44V_ConstRef GetCullLocalLRTB() const { return m_CullLocalLRTB; }
#else
	Mat44V_ConstRef GetCullLocalLRTB() const { return m_LocalLRTB; }
#endif

	Vec4V_Out GetPlane0x() const { return m_FrustumLRTB.a(); }
	Vec4V_Out GetPlane0y() const { return m_FrustumLRTB.b(); }
	Vec4V_Out GetPlane0z() const { return m_FrustumLRTB.c(); }
	Vec4V_Out GetPlane0w() const { return m_FrustumLRTB.d(); }
	Vec4V_Out GetPlane1x() const { return m_FrustumNFNF.a(); }
	Vec4V_Out GetPlane1y() const { return m_FrustumNFNF.b(); }
	Vec4V_Out GetPlane1z() const { return m_FrustumNFNF.c(); }
	Vec4V_Out GetPlane1w() const { return m_FrustumNFNF.d(); }

	// Call this after a viewport is copied
	void ClearOwner() { m_Owner = 0; }
private:
	static DECLARE_MTR_THREAD grcViewport *sm_Current;
	static const grcViewport *sm_OrthoViewport;
	// There is a sm_ScreenViewport per subrender thread render thread, one for
	// the primary render thread (even if
	// MULTIPLE_RENDER_THREADS_ALLOW_DRAW_LISTS_ON_RENDER_THREAD is not set),
	// and one for everyone else.
	static const grcViewport *sm_ScreenViewport[MULTIPLE_RENDER_THREADS ? (MULTIPLE_RENDER_THREADS+2) : 1];
#if RAGE_INSTANCED_TECH
	static grcViewport *sm_InstViewport[NUMBER_OF_RENDER_THREADS][MAX_NUM_CBUFFER_INSTANCING];

	static DECLARE_MTR_THREAD u32 sm_viewportInstancedRenderBit;
	static DECLARE_MTR_THREAD bool sm_bInstancing;
	static DECLARE_MTR_THREAD u32 sm_uNumInstVP;
	static DECLARE_MTR_THREAD u32 sm_uVPInstCount;
#endif

	void OwnerCheck();
	void RegeneratePerspective();
	void RegenerateMatrices();
	void RegenerateDevice();
	void RegenerateScreen();
	void RegenerateFrustumPlanes();
	NV_SUPPORT_ONLY(void RegenerateStereoFrustumPlanes();)
	void RegenerateFrustumPlanesNearFar();
	void RegenerateNearFarClip();
	void RegenerateAspect();
#if RAGE_INSTANCED_TECH
	void RegenerateLRTB(const Mat44V &worldmat);
#endif

	// THESE FOUR MATRICES MUST BE CONTIGUOUS: (they are downloaded as once block for efficiency)
	Mat44V m_World44,			// gWorld in rage_common.fxh
			m_ModelView,		// gWorldView in rage_common.fxh
			m_Composite,		// gWorldViewProj in rage_common.fxh
			m_Camera44;			// gViewInverse in rage_common.fxh

	Mat44V m_View, m_Projection;
	Mat44V m_FullComposite, m_Screen;

	// The four primary clip planes, in local model space (for AABB checks)
	Mat44V m_LocalLRTB;

#if NV_SUPPORT
	Mat44V m_CullComposite;
	Mat44V m_CullFrustumLRTB, m_CullLocalLRTB;	

	atRangeArray<Vec4V,2> m_CullFrustumClipPlanes;

	void ModifyProjectionForStereo(Mat44V_InOut proj);
#endif

	Vec4V m_OrthoLRTB; // TODO -- only used by fragShaft, can we get rid of this or store it in another vector?
	atRangeArray<Vec4V,6> m_FrustumClipPlanes;

	// Transposed clip plane equations (for better vectorization)
	Mat44V m_FrustumLRTB, m_FrustumNFNF;

	grcWindow m_Window; // 6 floats
	//The viewport requested by the caller of SetWindow().
	//The actual viewport may have been clipped to the display.
	grcWindow m_UnclippedWindow; // 6 floats

	mutable int m_LastWidth, m_LastHeight;
	float m_ZClipNear, m_ZClipFar;
	float m_ScaleX, m_ScaleY, m_ShearX, m_ShearY;
	float m_FovY, m_TanVFOV, m_TanHFOV;
	float m_ComputedAspect, m_DesiredAspect;

	bool m_IsPerspective;

#if RAGE_INSTANCED_TECH
	static DECLARE_MTR_THREAD void (*m_SetCurrentWorldMtxFn)(Mat34V_In);
	static DECLARE_MTR_THREAD void (*m_SetCurrentWorldIdentityFn)();
	static DECLARE_MTR_THREAD void (*m_SetCurrentNearFarClipFn)(float,float);
	static DECLARE_MTR_THREAD void (*m_SetCurrentWindowMinMaxZFn)(float,float);
#endif

	sysIpcCurrentThreadId m_Owner;
#if SUPPORT_INVERTED_PROJECTION
	bool m_InvertZInProjectionMatrix;
#endif // SUPPORT_INVERTED_PROJECTION

#if SUPPORT_DEPTH_BIAS_IN_PROJECTION
	float m_NDCDepthBias;
#endif // SUPPORT_DEPTH_BIAS_IN_PROJECTION

};

inline const grcViewport *grcViewport::GetDefaultScreen() {
#if MULTIPLE_RENDER_THREADS
	const unsigned idx = !!sm_Current + (MULTIPLE_RENDER_THREADS_ALLOW_DRAW_LISTS_ON_RENDER_THREAD?0:g_IsSubRenderThread) + g_RenderThreadIndex;
#else
	const unsigned idx = 0;
#endif
	const grcViewport *const vp = sm_ScreenViewport[idx];
	FastAssert(vp);
	return vp;
}

inline Mat34V_ConstRef grcViewport::GetCameraMtx() const {
	return (Mat34V_ConstRef)m_Camera44;
}

inline Mat44V_ConstRef grcViewport::GetCameraMtx44() const {
	return m_Camera44;
}

inline Mat44V_ConstRef grcViewport::GetScreenMtx() const {
	return m_Screen;
}

inline Mat44V_ConstRef grcViewport::GetFullCompositeMtx() const {
	return m_FullComposite;
}

inline Mat34V_ConstRef grcViewport::GetWorldMtx() const {
	return (Mat34V_ConstRef)m_World44;
}

inline Mat44V_ConstRef grcViewport::GetWorldMtx44() const {
	return m_World44;
}

inline Mat44V_ConstRef grcViewport::GetCompositeMtx() const {
	return m_Composite;
}

#if NV_SUPPORT
inline Mat44V_ConstRef grcViewport::GetCullCompositeMtx() const {
	return m_CullComposite;
}

inline Mat44V_Out grcViewport::GetCullFullCompositeMtx() const
{
	Mat44V fullCompositeMtx;
	if(GRCDEVICE.CanUseStereo() && GRCDEVICE.IsStereoEnabled())
	{
		Multiply(fullCompositeMtx, m_Screen, m_CullComposite);
		return fullCompositeMtx;
	}

	return m_FullComposite;
}

#else
inline Mat44V_ConstRef grcViewport::GetCullCompositeMtx() const {
	return m_Composite;
}
#endif

inline Mat44V_ConstRef grcViewport::GetModelViewMtx() const {
	return m_ModelView;
}

inline Mat44V_ConstRef grcViewport::GetViewMtx() const {
	return m_View;
}

inline int grcViewport::GetHeight() const {
	const float roundingBias = 0.5f;
	return int(roundingBias + m_Window.m_NormHeight*(float)m_LastHeight);
}

inline int grcViewport::GetWidth() const {
	const float roundingBias = 0.5f;
	return int(roundingBias + m_Window.m_NormWidth*(float)m_LastWidth);
}

inline float grcViewport::GetNearClip() const {
	return m_ZClipNear;
}

inline float grcViewport::GetFarClip() const {
	return m_ZClipFar;
}

inline float grcViewport::GetTanHFOV() const {
	return m_TanHFOV;
}

inline float grcViewport::GetTanVFOV() const {
	return m_TanVFOV;
}

inline float grcViewport::GetFOVY() const {
	return m_FovY;
}

inline fwRect grcViewport::GetTanFOVRect() const {
	return fwRect(
		m_TanHFOV*(-1.f+m_ShearX), m_TanVFOV*(-1.f+m_ShearY),
		m_TanHFOV*(+1.f+m_ShearX), m_TanVFOV*(+1.f+m_ShearY) );
}

inline float grcViewport::GetAspectRatio() const {
    return m_ComputedAspect;
}

inline bool grcViewport::IsPerspective() const {
	return m_IsPerspective != 0;
}

inline Vec4V_Out grcViewport::GetFrustumClipPlane(int whichPlane) const {
	return m_FrustumClipPlanes[whichPlane]; 
}

inline const Vec4V *grcViewport::GetFrustumClipPlaneArray() const {
	return m_FrustumClipPlanes.GetElements();
}

#if NV_SUPPORT
inline Vec4V_Out grcViewport::GetCullFrustumClipPlane(int whichPlane) const {
	return (GRCDEVICE.CanUseStereo() && GRCDEVICE.IsStereoEnabled()) ? m_CullFrustumClipPlanes[whichPlane] : m_FrustumClipPlanes[whichPlane]; 
}
#endif

inline Vector2 grcViewport::GetPerspectiveShear() const	{
	return Vector2( m_ShearX, m_ShearY );
}

inline Vector2 grcViewport::GetWidthHeightScale() const	{
	return Vector2( m_ScaleX, m_ScaleY );
}

inline void grcViewport::SetPerspectiveShear(float shearX, float shearY) {
	if (m_IsPerspective) //double check
	{
		m_ShearX = shearX;
		m_ShearY = shearY;

		RegeneratePerspective();
	}
}

#if SUPPORT_DEPTH_BIAS_IN_PROJECTION
inline void grcViewport::SetNDCDepthBias(float bias, bool regen) {
	if ( m_NDCDepthBias == bias ) // Written this way in case m_NDCDepthBias is NaN
		return;

	m_NDCDepthBias = bias;
	if (regen)
	{
		RegeneratePerspective();
	}
}
#endif // SUPPORT_DEPTH_BIAS_IN_PROJECTION

#define PUSH_DEFAULT_SCREEN()		grcViewport *curVp = grcViewport::GetCurrent(); \
									if (curVp != grcViewport::GetDefaultScreen()) \
										grcViewport::SetCurrent(grcViewport::GetDefaultScreen())

#define POP_DEFAULT_SCREEN()		if (curVp != grcViewport::GetDefaultScreen()) \
										grcViewport::SetCurrent(curVp)

/*
  PURPOSE
   Record the current grcViewport pointer and value (presumably because you are about to start
   changing things). The previous viewport pointer and value are restored in the destructor.
   */
#define GRC_VIEWPORT_AUTO_PUSH_POP()	grcViewportAutoScope MacroJoin(grcViewportAutoScope_,__LINE__);

class grcViewportAutoScope
{
public:
	grcViewportAutoScope() { 
		m_viewportPtr = grcViewport::GetCurrent(); 
		if (m_viewportPtr) {
			m_viewportVal = *m_viewportPtr; 
		}
	}

	~grcViewportAutoScope()	{ 
		if (m_viewportPtr) {
			*m_viewportPtr = m_viewportVal;
		}
		grcViewport::SetCurrent(m_viewportPtr); 
	}

private:
	grcViewport m_viewportVal;
	grcViewport* m_viewportPtr;
};

}	// namespace rage


#endif
