//
// phbound/bound.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHBOUND_BOUND_H
#define PHBOUND_BOUND_H


#if !__SPU
#include "atl/functor.h"
#include "paging/base.h"
#include "data/resource.h"
#endif

#include "parser/macros.h"
#include "phbullet/CollisionMargin.h"
#include "grprofile/drawcore.h"

#if !__SPU
#include "phcore/config.h"
#include "phcore/constants.h"
#include "phcore/resourceversions.h"
#else
#ifdef __NMDRAW
#undef __NMDRAW
#endif
#define __NMDRAW 0
#endif	// !__SPU
#include "phcore/materialmgr.h"

#include "system/floattoint.h"
#include "vectormath/classes.h"
#include "vectormath/legacyconvert.h"
#include "vectormath/vectortypes.h"
#include "phbullet/GjkSimplexCache.h"

#if __SPU && !__OPTIMIZED
#define FORCE_INLINE_SIMPLE_SUPPORT inline
#else
#define FORCE_INLINE_SIMPLE_SUPPORT __forceinline
#endif

#define DISABLE_DRAW_GRCVIEWPORT_GETCURRENT 0

// This is also defined in archetype.h, hence the check for whether it's already defined or not.  This should really be in a single,
//   centralized location.
#ifndef PH_NON_SPU_VIRTUAL
#if __SPU
#define PH_NON_SPU_VIRTUAL 
#define PH_NON_SPU_VIRTUAL_ONLY(X) X
#else
#define PH_NON_SPU_VIRTUAL virtual
#define PH_NON_SPU_VIRTUAL_ONLY(X)
#endif
#endif // ndef PH_NON_SPU_VIRTUAL

namespace rage {

typedef int phRemovedBound;

class fiAsciiTokenizer;
class phSegment;
class phSurface;

#define DEFAULT_SUPPORT_INDEX Vec::V4VConstant(V_ZERO)

//=============================================================================
// phBoundBase
// PURPOSE
//   This class is used to ensure that the virtual pointers of all classes derived
//   from phBound are in the same known place -- specifically, at the beginning
//   of the class.  This was certainly used for the first version of the
//   resource system.  It is not clear that this is necessary anymore.
//
class phBoundBase
#if !__SPU
	: public pgBase
#endif
{
public:
#if __SPU
	// In order to ensure that all members end up at the same offset between SPU and non-SPU builds we have to add this to account for the fact
	//   that there is are virtual functions, and hence a v-table, when not on the SPU.
	void *m_MissingVTablePtr;
	void *m_PageMap;
#endif
	// PURPOSE: Virtual destructor to force virtual pointer into class.
	PH_NON_SPU_VIRTUAL ~phBoundBase ();
#if !__SPU
	// PURPOSE: Magic number to indicate virtual pointer.
	static const int sVirtualPointerRsc;

	// PURPOSE: Magic number to indicate default material.
	static const int sMaterialDefaultRsc;
#endif
};


//=============================================================================
// phBound
// PURPOSE
//   The base class for all bounds used by the Rage physics engine. Bounds are physical boundaries used for
//	 collision detection and other proximity tests.
// NOTES
//  1. Bounds are normally used by physics instances to define their physical boundaries. Bound information, such as
//     vertex locations, is in the instance's coordinate system, so that the same bound can be shared among
//     multiple instances.
//  2. The phBound class does not specify a shape or how the shape is composed.  Derived classes include spheres,
//     capsules, bounds composed of polygons, and bounds composed of other bounds.
//  3.  This class defines:
//      * Two bounding spheres for the bound, one centered on the local coordinate origin, and one centered
//        on the centroid of the bound, which is the center of the smallest enclosing sphere.
//      * The center of gravity of the bound, offset from the physics instance position.
//      * The centroid of the bound (the center of the smallest enclosing sphere), offset from the physics
//        instance position.
// <FLAG Component>
// 
class phBound : public phBoundBase
{
public:
	//============================================================================
	// PURPOSE: types of bounds
	// NOTES:	The bound type integers are used when loading from resources, to create the correct bound classes. When an existing bound type has its index number changed,
	//			existing resources will not work. When adding a new type, put it any place in the list but give it the last used integer so that resources aren't broken.
    //          If this assert fires, you must be breaking resources anyway. In that case, please renumber the bound type enum to the logical order.
#if !__SPU
    CompileTimeAssert(phResourceBaseVersionConstant == 30);
#endif

	enum BoundType
	{
		#undef BOUND_TYPE_INC
		#define BOUND_TYPE_INC(className,enumLabel,enumValue,stringName,isUsed) enumLabel = enumValue,
		#include "boundtypes.inc"
		#undef BOUND_TYPE_INC
		
		NUM_BOUND_TYPES = 15,				// pheffects/morphgeometry uses this as its bound type to make virtuals work.
		INVALID_TYPE = 255
	};

	static int sm_MemoryBucket;

	// PURPOSE: temporary definition of outdated octree grid bound type name
#if USE_GRIDS
	enum { OCTREEGRID = GRID };
#endif

#if __64BIT
	enum { MAX_BOUND_SIZE = 336 };
#else
	#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE_DATA && HACK_GTA4_64BIT_MATERIAL_ID_COLORS
		// Combining these two options causes geom bounds to be bigger by a quad word
		#if OCTANT_MAP_SUPPORT_ACCEL
			enum { MAX_BOUND_SIZE = 288 };
		#else // OCTANT_MAP_SUPPORT_ACCEL
			enum { MAX_BOUND_SIZE = 272 };
		#endif// OCTANT_MAP_SUPPORT_ACCEL
	#else
		#if OCTANT_MAP_SUPPORT_ACCEL
			enum { MAX_BOUND_SIZE = 272 };
		#else // OCTANT_MAP_SUPPORT_ACCEL
			enum { MAX_BOUND_SIZE = 256 };
		#endif// OCTANT_MAP_SUPPORT_ACCEL
	#endif
#endif	// __64BIT

	//============================================================================

	// PURPOSE: Default constructor.
	phBound();

	// PURPOSE: Destructor.
	PH_NON_SPU_VIRTUAL ~phBound();

	// PURPOSE: Returns the type, from enum BoundType, of this bound.
	// RETURN:	the type of this bound
	int GetType () const;
#if !__SPU
	// PURPOSE: Returns the number of outstanding references to this bound.
	// RETURN:	the number of physics instances (or sometimes other classes) using this bound
	int GetRefCount () const;

	// PURPOSE: Increment the number of outstanding references.
	void AddRef () const;

	// PURPOSE: Provide a custom bound loader in case we want to load something other than basic bound types.
	static void SetCustomConstructor (Functor1Ret<phBound*,const char*> func) { sm_CustomConstructor = func; }

	// PURPOSE: Provide a custom resource constructor for user defined bound types.
	static void SetCustomResourceConstructor(Functor2Ret<bool, phBound*, datResource&> func) { sm_CustomResourceConstructor = func; }

	// PURPOSE: Notify the bound that it is no longer being referred to by something that held a reference.
	// PARAMS:
	//   deleteAtZero - whether or not to delete the bound when the reference count reaches 0
	// RETURNS: The number of outstanding references to this bound.
	// NOTES: Has no effect if reference counting is not enabled in phConfig.
	int Release(bool deleteAtZero=true);

	// PURPOSE: Return the type of bound as a string.
	// TODO: Remove due to RTTI?
	const char * GetTypeString () const;
#endif

	// The intent of this function is to handle initialization that needs to happen once after the bound has been fully configured (which might be getting handled
	//   by an external system).  It should also be safe to allocate memory from within this function (and have it go to an appropriate heap such as the resource
	//   heap, if relevant).
	PH_NON_SPU_VIRTUAL bool PostLoadCompute() { return false; }

	//============================================================================
	// Centroid offset and center-of-gravity.

	// PURPOSE: Get the offset of the centroid from the local coordinate system origin.
	// RETURN: the offset of the centroid from the local coordinate system origin
	// NOTES
	//	1.	The centroid is the center of the smallest sphere that can enclose the bound.
	//	2.	The centroid offset is returned in the local coordinate system.
	// SEE ALSO: SetCentroidOffset, ShiftCentroidOffset
	Vec3V_Out GetCentroidOffset () const;

#if __SPU
	Vec3V* GetCentroidOffsetPtr ();
	float* GetRadiusAroundCentroidPtr();
#endif

	void SetIndexInBound(int indexInBound);
	int  GetIndexFromBound() const { return m_PartIndex; }

	// PURPOSE: Set the offset of the centroid from the local coordinate system origin.
	// PARAMS:
	//	offset	- the offset of the centroid from the local coordinate system origin
	// NOTES
	//	1.	The centroid is the center of the smallest sphere that can enclose the bound.
	//	2.	The centroid offset is in the local coordinate system.
	PH_NON_SPU_VIRTUAL void SetCentroidOffset (Vec3V_In offset)
	{
		m_CentroidOffsetXYZMaterialId0W.SetXYZ(offset);
	}

	// PURPOSE: Translate the centroid offset by offsetDelta.
	// PARAMS
	//   offsetDelta - The vector by which to shift the offset.
	// SEE ALSO: GetCentroidOffset, IsCentroidOffset, SetCentroidOffset
	PH_NON_SPU_VIRTUAL void ShiftCentroidOffset (Vec3V_In offsetDelta);

	// PURPOSE: Check/control whether or not CCD should be forced on for this bound.
	// NOTES: Be aware that this flag will currently not have any effect if set on a composite bound.  That is, setting it on a composite bound will *not*
	//   cause each of the components of that composite to be forced to use CCD.  Set this flag on each of the components if that is what you want.
	bool GetForceCCD() const;
	u32 GetForceCCDFlag() const;
	void SetForceCCD();
	void ClearForceCCD();

	bool GetUseCurrentInstsanceMatrixOnly() const;
	u32 GetUseCurrentInstsanceMatrixOnlyFlag() const;
	void SetUseCurrentInstsanceMatrixOnly();
	void ClearUseCurrentInstsanceMatrixOnly();

#if USE_NEW_TRIANGLE_BACK_FACE_CULL_OPTIONAL
	u32 GetUseNewBackFaceCull() const;
	void EnableUseNewBackFaceCull();
	void DisableUseNewBackFaceCull();
#endif // USE_NEW_TRIANGLE_BACK_FACE_CULL_OPTIONAL

#if USE_PROJECTION_EDGE_FILTERING
	u32 GetUseProjectionEdgeFiltering() const;
	void EnableProjectionEdgeFiltering();
	void DisableProjectionEdgeFiltering();
#endif // USE_PROJECTION_EDGE_FILTERING

	// PURPOSE: Get the center-of-gravity for this bound relative to its local coordinate system origin.
	// RETURN: the center of gravity location in the local coordinate system
	// SEE ALSO: SetCGOffset
	Vec3V_Out GetCGOffset () const;

	Vec3V* GetCGOffsetPtr ();

	// PURPOSE: specify the location of the center of gravity in the local coordinate system
	// PARAMS
	//   cg - the new center of gravity location in the local coordinate system
	// SEE ALSO: GetCGOffset
	void SetCGOffset (Vec3V_In cg);

	// PURPOSE: Calculate the centroid of the bound, transformed with the given coordinate system matrix.
	// PARAMS
	//   matrix - The matrix that positions this bound in the world.
	//   center - Output parameter in which the center is stored.
	// NOTES:
	//	- The parameter matrix is normally GetMatrix() from the instance using this bound, to get the center in world coordinates.
	// SEE ALSO: IsCentroidOffset, GetCentroidOffset, SetCentroidOffset
	Vec3V_Out GetWorldCentroid (Mat34V_In instancePose) const;

	// PURPOSE: Calculate the center of gravity of this bound, transformed by the given coordinate matrix.
	// PARAMS:
	//   pose - the matrix that positions this bound in the world
	// RETURN: the center of mass of this bound, transformed by the given coordinate matrix
	// NOTES:
	//	- The parameter matrix is normally GetMatrix() from the instance using this bound, to get the center of gravity in world coordinates.
	//	- Center of mass and center of gravity are the same thing.
	Vec3V_Out GetCenterOfMass (Mat34V_In pose) const;

	//============================================================================
	// Bounding volume information.

	// PURPOSE: Get the maximum extents of a bounding box that contains this bound in the local coordinate system.
	// RETURN: the maximum extents of a bounding box that contains this bound in the local coordinate system
	Vec3V_ConstRef GetBoundingBoxMin () const;

	// PURPOSE: Get the minimum extents of a bounding box that contains this bound in the local coordinate system.
	// RETURN: the minimum extents of a bounding box that contains this bound in the local coordinate system
	Vec3V_ConstRef GetBoundingBoxMax () const;

	void SetBoundingBox (Vec3V_In boxMin, Vec3V_In boxMax);

	// PURPOSE: Fills in a vector with the dimensions of the bounding box.
	Vec3V_Out GetBoundingBoxSize () const;

	ScalarV_Out GetBoundingBoxVolume () const;

	Vec3V_Out ComputeBoundingBoxCenter () const;

	// PURPOSE: Calculate the half-widths and center (in local coordinates) of this bound's bounding box.
	// PARAMS:
	//	halfWidth - reference to the box half-widths, filled in by this method
	//	center - reference to the box center in local coordinates, filled in by this method
	void GetBoundingBoxHalfWidthAndCenter (Vec3V_InOut outHalfWidth, Vec3V_InOut outCenter) const;

	// PURPOSE: Returns the upper bound on the distance from m_Offset to any point in this bound.
	float GetRadiusAroundCentroid() const;

	// PURPOSE: Returns the upper bound on the distance from m_Offset to any point in this bound.
	ScalarV_Out GetRadiusAroundCentroidV() const;

	// PURPOSE: Set the upper bound on the distance from m_Offset to any point in this bound.
	// PARAMS:
	//	radius - the new radius of the bound's smallest enclosing sphere
	// NOTES:	Normally this is only computed internally. This method is for use by cloth or other flexible bounds that change size.
	void SetRadiusAroundCentroid (ScalarV_In radius);

	// PURPOSE: Get the volume enclosed by this bound.
	// RETURN:	the volume enclosed by this bound
	// NOTES:	The GetCompute version is outdated (it used to compute the volume the first time this is called, now it does in CalculateExtents).
	float GetVolume () const;
	float GetComputeVolume () const;

	// PURPOSE: Get the angular inertia vector for a rigid body of this shape and the given mass.
	// PARAMS
	//   mass - the mass to use when calculating the angular inertia of this bound
	// NOTES:
	//   - Assumes that the density of the bound is uniform.
	//   - Returns the vector (1,1,1) by default to allow some bounds to not override this.
	Vec3V_Out GetComputeAngularInertia (float mass) const;

	//============================================================================
	// Materials

	// PURPOSE: Get the material assigned to a particular part of a bound.
	// PARAMS:
	//   partIndex - The index of the "part", sometimes a polygon index, as returned by a collision, probe, etc.
	//   component - The index of the "component", e.g. the octree within a grid or the bound in a composite,
	//               returned by a collision, probe, etc.
	inline phMaterialMgr::Id GetMaterialIdFromPartIndex (int partIndex) const;

	// PURPOSE: Get a material ID from this bound, via an index into its local material list.
	// PARAMS:  materialIndex - Which material to get, must be between 0 and GetNumMaterial() - 1.
	// RETURN:	The material ID that is requested.
	phMaterialMgr::Id GetMaterialId (phMaterialIndex materialIndex) const;

#if !__SPU
	// PURPOSE: Find out how many materials this bound is made from.
	// RETURN:	The number of unique materials that are used on this bound.
	// NOTES:
	//   - Many bounds only have one material, which is why the base class returns 1.
	//   - If this function returns N, then material indices between 0 and N - 1 can be passed to GetMaterial.
	virtual int GetNumMaterials () const								{ return 1; }

	// PURPOSE: Get a material from this bound, via an index into its local material list.
	// PARAMS:  materialIndex - Which material to get, must be between 0 and GetNumMaterial() - 1.
	// RETURN:	A reference to the material that is requested.
	virtual const phMaterial & GetMaterial (phMaterialIndex UNUSED_PARAM(materialIndex)) const { return MATERIALMGR.GetDefaultMaterial(); }

	// PURPOSE: Set one material for the entire bound.
	// PARAMS: The material manager ID of the material you want to use.
	// NOTES:
	//   - This function is kind of specialized, usually you should use the material on the bound
	//     when the bound is loaded from disk.
	virtual void SetMaterial (phMaterialMgr::Id UNUSED_PARAM(materialId), phMaterialIndex UNUSED_PARAM(materialIndex)=-1) {}

	// PURPOSE: Get the material assigned to a particular part of a bound.
	// PARAMS:
	//   partIndex - The index of the "part", sometimes a polygon index, as returned by a collision, probe, etc.
	//   component - The index of the "component", e.g. the octree within a grid or the bound in a composite,
	//               returned by a collision, probe, etc.
	virtual phMaterialMgr::Id GetMaterialIdFromPartIndexAndComponent (int partIndex, int component) const;

	// PURPOSE: Find out if this bound is made of polygons (if it is a polyhedron).
	// PARAMS:
	//	component	- optional component number, for use in composite bounds
	// RETURN:	true in derived bound classes that are polyhedrons, false in derived bound classes that are not polyhedrons
	virtual bool IsPolygonal (int UNUSED_PARAM(component)) const { return false; }

#endif

	// PURPOSE: Non virtual way to find out if this bound is polygonal without checking component bounds
	bool IsPolygonal() const;

#if !__SPU
	//============================================================================
	// FINDIMPACTS routines
	
	// PURPOSE: Tell whether this bound type can be used on an active object.
	// RETURN:	true if this bound can be used on a physically active object, false if it can not
	// NOTES:
	//	Only some bound types are permitted as physically active objects. Bound types with cullable elements (octrees, quadtrees and octree grids) can not become active.
	virtual bool CanBecomeActive () const;

	//============================================================================
	// utility functions

	// copying and cloning
	virtual void Copy (const phBound* original);
	virtual phBound* Clone () const;

	//============================================================================
	// warning control
	static bool GetBoundFlag (u32 mask)									{ return (sm_BoundFlags & mask)!=0; }
	static void SetBoundFlag (u32 mask, bool value)						{ sm_BoundFlags = (value?(sm_BoundFlags|mask):(sm_BoundFlags&~mask)); }
	static void SetBoundFlags (u32 flags)								{ sm_BoundFlags = flags; }
	static void SetErrorsAsWarnings (bool b=true)						{ SetBoundFlag(ERRORS_NOT_WARNINGS,!b); }
	static void DisableMessages (bool b=true)							{ SetBoundFlag(ENABLE_MESSAGES,!b); }
	static bool MessagesEnabled ()										{ return GetBoundFlag(ENABLE_MESSAGES); }

	// PURPOSE: Prevent messages from repeating every frame, and instead make them display only once per second (default is on).
	// PARAMS:
	//	enableControl - whether to turn on or turn off control of message spew
	static void ControlMessageSpew (bool enableControl=true)			{ SetBoundFlag(ENABLE_SPEW_CONTROL,enableControl); }

	// PURPOSE: Find out whether message spew control is enabled or disabled.
	// RETURN:	true if message spew control is enabled, false if it is disabled
	static bool MessageSpewControlEnabled ()							{ return GetBoundFlag(ENABLE_SPEW_CONTROL); }

	// PURPOSE: Determine if repeating messages should be allowed.
	// RETURN:	true if messages are enabled, false if messages should be disabled to stop excessive repitition
	static bool SpewControlAllowMessage ();

	// Flags for warning messages and loading actions.
	// Some are stated as negatives because the default state is true for all of them.
	enum eOptions
	{
		WARN_ZERO_VERTICES	= (0x01 << 0),			// warn when a bound is loaded with zero verts
		WARN_BAD_POLYGONS	= (0x01 << 1),			// warn when a polygon has non-unit normal vector (usually, a polygon with zero area)
		WARN_BAD_EDGES		= (0x01 << 2),			// warn on edge problems (too many polygons, or zero normal)
		DELETE_BAD_POLYGONS	= (0x01 << 3),			// discard the bad polygons during loading
		ALLOW_BAD_POLYGONS	= (0x01 << 4),			// allow bad polygons to be loaded (assert if not ignored)
		ASSERT_MATH_ERRORS	= (0x01 << 5),			// stop initializing on math errors (edges with no polygons so no normal, etc)
		WARN_MISSING_BOUNDS	= (0x01 << 6),			// warn when a load attempt is made on a nonexistent bound
		DONT_WARN_MISS_PHYS	= (0x01 << 7),			// don't warn when a load attempt is made on a nonexistent phys file
		ERRORS_NOT_WARNINGS	= (0x01 << 8),			// show error messages when errors occur, otherwise show warnings
		ENABLE_MESSAGES		= (0x01 << 9),			// show messages (not implemented for all message displays)
		ENABLE_SPEW_CONTROL	= (0x01 << 10),			// only allow one message per second of certain kinds
	};
#endif

	// Bullet support

	// Convex vs. Convex

	// PURPOSE: Find the vertex on the bound whose direction from the local origin is closest to the given position, extended outward by the distance margin.
	// PARAMS:
	//	localDirection - the direction into the object from which to find the first vertex hit
	// RETURN:	the position of the bound's vertex that is closest in direction from the origin to the given position, extended outward by the distance margin
	// NOTES:	For non-polygonal bounds, this returns the extended position on the bound surface that is in the same direction from the origin as the given point.
	Vec3V_Out LocalGetSupportingVertex (Vec::V3Param128 localDirection) const;

	// PURPOSE: Find the vertex on the bound whose direction from the local origin is closest to the given position.
	// PARAMS:
	//	localDirection - the direction into the object from which to find the first vertex hit
	//	vertexIndex - optional pointer to fill in the index of the supporting vertex
	// RETURN:	the position of the bound's vertex that is closest in direction from the origin to the given position
	// NOTES:	For non-polygonal bounds, this returns the position on the bound surface that is in the same direction from the origin as the given point.

	struct SupportPoint
	{
		Vec3V m_vertex;
		Vec::Vector_4V m_index;
	};

	void LocalGetSupportingVertexWithoutMarginNotInlined(Vec::V3Param128 localDirection, SupportPoint & sp) const;
	
	__forceinline Vec3V_Out LocalGetSupportingVertexWithoutMarginNotInlined(Vec::V3Param128 localDirection) const
	{
		SupportPoint sp;
		LocalGetSupportingVertexWithoutMarginNotInlined(localDirection,sp);
		return sp.m_vertex;
	}

#if __DEV && !__SPU
	// PURPOSE: This is the same as LocalGetSupportingVertex except it is not __forceinlined.
	Vec3V_Out LocalGetSupportingVertexDebug(Vec::V3Param128 localDirection) const;

	// PURPOSE: This will assert if any support of the bound is outside of the bounding box
	bool DoesBoundingBoxContainsSupports() const;
#endif // __DEV && !__SPU 

	// PURPOSE: Find the bound part index from the normal vector and a list of vertices.
	// PARAMS:
	//	localNormal - the outgoing normal on the surface of the bound
	//	vertexIndices - a list of 1, 2 or 3 vertex index numbers
	//	numVertices - the number of vertex indices
	// RETURN:	The index number of the polygon containing the given vertices with a normal closest to the given normal (for polyhedron bounds),
	//			or the index number of the bound part for primitive bounds, such as a capsule hemisphere or shaft for capsule bounds.
	// NOTES:	For polyhedron bounds, this returns the polygon index.
    int FindElementIndex (Vec::V3Param128 localNormal, Vec::V3Param128 localContact, int* vertexIndices, int numVertices) const;

	// Choosing concave vs. convex
	bool IsConvex() const;
	bool IsConcave() const;

    static bool IsTypeConcave(int type);
    static bool IsTypeConvex(int type);
    static bool IsTypeComposite(int type);

#if !__SPU
	// Extra functions that have to be defined
	virtual int	GetShapeType() const;
	virtual const char* GetName() const;
#endif

	// PURPOSE: Get the collision margin for this bound.
	// RETURN: the distance by which collision detection will be expanded beyond the bound's surface
	ScalarV_Out GetMarginV () const;
    float GetMargin() const;

	// PURPOSE: Set the collision margin for this bound.
	// PARAMS:
	//	collisionMargin - the distance by which collision detection will be expanded beyond the bound's surface
	void SetMargin (ScalarV_In collisionMargin);
	void SetMargin (float collisionMargin);

	void SetBoundingBoxMin(Vec3V_In boundingBoxMin);
	void SetBoundingBoxMax(Vec3V_In boundingBoxMax);

	//============================================================================
	// Debug drawing
#if __PFDRAW
	enum
	{
		FACE_NORMALS,
		EDGE_NORMALS
	};

	enum
	{
		ALL_POLYS					= 0,
		RENDER_THIN_POLYS			= 1 << 0,
		RENDER_BAD_NORMAL_POLYS		= 1 << 1,
		RENDER_BAD_NEIGHBOR_POLYS	= 1 << 2,
		RENDER_BIG_PRIMITIVES		= 1 << 3
	};

	virtual void Draw(Mat34V_In mtx, bool colorMaterials = false, bool solid = false, int whichPolys = ALL_POLYS, phMaterialFlags highlightFlags = 0,
						unsigned int typeFilter = 0xffffffff, unsigned int includeFilter = 0xffffffff, unsigned int boundTypeFlags = 0, unsigned int boundIncludeFlags = 0) const;
	virtual void DrawNormals(Mat34V_In mtx, int normalType = FACE_NORMALS, int whichPolys = ALL_POLYS, float length = 1.0f,
								unsigned int typeFilter = 0xffffffff, unsigned int includeFilter = 0xffffffff) const;
	virtual void DrawLast(Mat34V_In mtx, bool colorMaterials = false, bool solid = false, int whichPolys = ALL_POLYS, phMaterialFlags highlightFlags = 0) const;

	void DrawCentroid(Mat34V_In mtx) const;
	void DrawCenterOfGravity(Mat34V_In mtx) const;
	void DrawAngularInertia(Mat34V_In mtx, float scale = 1.0f, bool invert = false, Color32 boxColor = Color32()) const;
    virtual void DrawSupport(Mat34V_In mtx, unsigned int typeFilter = 0xffffffff, unsigned int includeFilter = 0xffffffff) const;
#endif // __PFDRAW

#if __NMDRAW
  virtual void NMRender(Mat34V_In mtx) const;
#endif // __NMDRAW

#if __DEV && !__SPU
  // Function for generically formatting a bunch of bound data into a single string
  // - Writes into the given string buffer from the end of any existing string
  // - Assumes BoundStringBuffer comes in as a valid string with NULL terminator somewhere
  // -- If you want just this new stuff, simply set BoundStringBuffer[0] = 0
  static void FormatBoundInfo(char* BoundStringBuffer, int bufSize, const phBound* Bound, int Component, int Element, Mat34V_In ParentMatrix);
#endif

	//============================================================================
	// resources
	// TODO: Evaluate state of resources in Rage.
	phBound (datResource & rsc);										// resource constructor
	static void Place(void *that,datResource &rsc);
#if !__SPU
	enum { RORC_VERSION = 4 + phResourceBaseVersion };
	static void VirtualConstructFromPtr (class datResource & rsc, phBound *Bound);		// hides virtual construction as much as possible - bd...
	static void ResourcePageIn (class datResource & rsc);				// bound page-in function (setup and work)
	static void ResourcePageInDoWork (class datResource & rsc);	// bound page-in function (work)
#if __DECLARESTRUCT
	virtual void DeclareStruct(datTypeStruct &s);
#endif // __DECLARESTRUCT

	static const bool ms_RequiresDestructorOnStack = true;

	//============================================================================

	//============================================================================
	// static load / save
	// TODO: These functions will likely change soon.
	enum FileVersion {VERSION_101=101, VERSION_110=110};
	enum FileMode {ASCII, Binary};
	static phBound * CreateOfType (int boundType);
	static phBound * Load (const char *filename);						// generic bound loader (from resources first)
#if !__FINAL && !IS_CONSOLE
	static bool Save (const char * filename, phBound * boundToSave, FileVersion version=VERSION_110, FileMode mode=ASCII);
#endif																		// generic bound saver (writes header then bound info)
	static phBound * Load (fiAsciiTokenizer & token);					// generic bound loader, tokenizer, ASCII
#if !__FINAL && !IS_CONSOLE
	static bool Save (fiAsciiTokenizer & token, phBound * boundToSave, FileVersion version=VERSION_110);
	// generic bound saver, tokenizer, ASCII
#endif

    static void SetOctreeAsBVH(bool)  { }

protected:
	bool LoadData (fiAsciiTokenizer & token, FileVersion version);		// distribute to version specific load functions
	virtual bool Load_v110 (fiAsciiTokenizer & token);					// load, ascii, v1.10
#if !__FINAL && !IS_CONSOLE
	bool SaveData (fiAsciiTokenizer & token, FileVersion version);		// distribute to version specific save functions
	virtual bool Save_v110 (fiAsciiTokenizer & token);					// save, ascii, v1.10
#endif
#endif // !__SPU
	// end of likely to change functions.
	//============================================================================

protected:
	//============================================================================
	// Protected utility functions

#if !__SPU
	static phMaterialMgr::Id GetMaterialIdFromFile(fiAsciiTokenizer& token);
	static void WriteMaterialIdToFile(fiAsciiTokenizer& token, phMaterialMgr::Id materialId);

    void SetRefCount(int refCount);

	// PURPOSE: Calculate the bounding box and bounding sphere.
	virtual void CalculateExtents ();
#endif

	// PURPOSE: Set the bounding sphere to be smallest sphere enclosing bounding box.
	// NOTES:
	//   - This is a convenience function that probably shouldn't be used by most derived
	//     phBound objects since a tighter sphere bound is possible in most cases.
	//   - For an example of calculating the optimal bounding sphere for an arbitrary set 
	//     of points, see phBoundPolyhedron::CalculateBoundingSphere().
	void CalculateSphereFromBoundingBox ();

	phMaterialMgr::Id  GetPrimitiveMaterialId() const; 
	void SetPrimitiveMaterialId(phMaterialMgr::Id materialId); 

	void SetFlag(u8 mask, bool value);

	//============================================================================
	// Class data

	// PURPOSE: Static flags that control what events should be considered warnings or errors.
	static u32 sm_BoundFlags;
#if !__SPU	
	// PURPOSE: Static functor in case projects want to create their own bound types to be loaded.
	static Functor1Ret<phBound*,const char*>	sm_CustomConstructor;

	static Functor2Ret<bool, phBound*, datResource& >	sm_CustomResourceConstructor;
#endif
	//============================================================================
	// Data

	// PURPOSE: Entry from the enum BoundType indicating the type of this bound.
	u8 m_Type;

	enum BoundInfo
	{
#if USE_OCTANT_MAP_INDEX_U16
		OCTANT_MAP_INDEX_IS_U16 = (1<<0),
#endif // USE_OCTANT_MAP_INDEX_U16
#if USE_OCTANT_MAP_PERMANENT_ALLOCATION
		GEOMETRY_BOUND_HAS_OCTANT_MAP = (1<<1),
#endif // USE_OCTANT_MAP_PERMANENT_ALLOCATION
		FORCE_CCD_THIS_BOUND = (1<<2),
#if USE_NEW_TRIANGLE_BACK_FACE_CULL_OPTIONAL
		USE_NEW_BACK_FACE_CULL = (1 << 3),
#endif // USE_NEW_TRIANGLE_BACK_FACE_CULL_OPTIONAL
#if USE_PROJECTION_EDGE_FILTERING
		USE_PROJECTION_EDGE_FILTERING_FLAG = (1 << 4),
#endif // USE_PROJECTION_EDGE_FILTERING
		USE_CURRENT_INSTANCE_MATRIX_ONLY = (1 << 5),
	};

	// PURPOSE: Bit flags to store info about this object.
	u8 m_Flags;

	// PURPOSE: stores the part index of this bound
	u16 m_PartIndex;

	// PURPOSE: Upper bound on the distance from m_Offset to any point in this bound.
	// [SPHERE-OPTIMISE] {m_CentroidOffsetXYZMaterialId0W,m_RadiusAroundCentroid} should be a Vec4V
	float m_RadiusAroundCentroid;

private:
	// PURPOSE: BoundingBoxMax - XYZ - The maximum extents of a bounding box that contains this bound -- in local CS.
	//          Margin - W - The collision margin, a buffer to keep bounds from penetrating even when they are
	Vec4V m_BoundingBoxMaxXYZMarginW;

	// PURPOSE:  BoundingBoxMin - XYZ - The minimum extents of a bounding box that contains this bound -- in local CS.
	//           RefCount - W - The number of references to this bound
	Vec4V m_BoundingBoxMinXYZRefCountW;

	// <COMBINE phBound::GetCentroidOffset>
	Vec4V m_CentroidOffsetXYZMaterialId0W;

	// PURPOSE: Vector from the origin (local CS) to the center of gravity of this bound.
	Vec4V m_CGOffsetXYZMaterialId1W;
protected:
	// PURPOSE: The angular inertia that this bound would have with a mass of 1kg (elements x, y, and z)
	//			and the bound's volume (element w).
	Vec4V m_VolumeDistribution;

#if __RESOURCECOMPILER
public:
	static char s_currentBoundFilename[RAGE_MAX_PATH];
	static char s_currentFragChildBoneName[64];
#endif // __RESOURCECOMPILER

#if __TOOL
public:
	u32 m_CompositeCollisionBoundFlag;
#endif // __TOOL

	PAR_SIMPLE_PARSABLE;
};


//=============================================================================
// Implementations

__forceinline phBound::phBound ()
: m_Type(INVALID_TYPE)
, m_Flags(0)
, m_PartIndex(0)
, m_RadiusAroundCentroid(0.0f)
, m_BoundingBoxMinXYZRefCountW(V_ZERO)
, m_CentroidOffsetXYZMaterialId0W(V_ZERO)
, m_CGOffsetXYZMaterialId1W(V_ZERO)
, m_VolumeDistribution(V_ONE)
{
#if !__SPU
	m_BoundingBoxMaxXYZMarginW.SetIntrin128(Vec::V4VConstant<0x00000000, 0x00000000, 0x00000000, FLOAT_TO_INT(CONVEX_DISTANCE_MARGIN)>());
	SetRefCount(1);
#else
	m_BoundingBoxMaxXYZMarginW = Vec4V(0.0f,0.0f,0.0f,(float)CONVEX_DISTANCE_MARGIN);
#endif	// !__SPU

#if __TOOL
	// Initialise the storage for the BoundFlags token used to set type / include flags on component parts of composite bounds.
	// Needs to be at this level due to the frag tool chain.
	m_CompositeCollisionBoundFlag = 0;
#endif // __TOOL
}


__forceinline phBound::~phBound ()
{
#if !__SPU
	Assert(GetRefCount()==0 /*&& "phBound:~phBound - deleting a bound with outstanding references") */ || !phConfig::IsRefCountingEnabled());
#endif
}


inline int phBound::GetType() const
{
	return m_Type;
}
#if !__SPU
inline int phBound::GetRefCount() const
{
	return (phConfig::IsRefCountingEnabled()) ? m_BoundingBoxMinXYZRefCountW.GetWi() : 1;
}

inline void phBound::SetRefCount(int refCount)
{
    m_BoundingBoxMinXYZRefCountW.SetWi(refCount);
}

inline void phBound::AddRef() const
{
	if (phConfig::IsRefCountingEnabled())
	{
		const_cast<phBound*>(this)->SetRefCount(GetRefCount() + 1);
	}
}
#endif

inline void phBound::SetBoundingBoxMin(Vec3V_In boundingBoxMin)
{
	m_BoundingBoxMinXYZRefCountW.SetXYZ(boundingBoxMin);
}

inline void phBound::SetBoundingBoxMax(Vec3V_In boundingBoxMax)
{
	m_BoundingBoxMaxXYZMarginW.SetXYZ(boundingBoxMax);
}

inline phMaterialMgr::Id phBound::GetPrimitiveMaterialId() const 
{ 
#if PH_MATERIAL_ID_64BIT
	return (((u64)(u32)m_CGOffsetXYZMaterialId1W.GetWi()) << 32) | (u64)(u32)m_CentroidOffsetXYZMaterialId0W.GetWi(); 
#else
	return m_CentroidOffsetXYZMaterialId0W.GetWi(); 
#endif
}

inline void phBound::SetPrimitiveMaterialId(phMaterialMgr::Id materialId)       
{ 
#if PH_MATERIAL_ID_64BIT
	m_CGOffsetXYZMaterialId1W.SetWi((u32)((materialId & 0xffffffff00000000LL) >> 32));
#endif
	m_CentroidOffsetXYZMaterialId0W.SetWi((u32)(materialId & 0xffffffffL));
}

inline Vec3V_Out phBound::GetCentroidOffset() const
{
	return m_CentroidOffsetXYZMaterialId0W.GetXYZ();
}

#if __SPU
inline Vec3V* phBound::GetCentroidOffsetPtr()
{
	return (Vec3V*)&m_CentroidOffsetXYZMaterialId0W;
}

inline float* phBound::GetRadiusAroundCentroidPtr()
{
	return &m_RadiusAroundCentroid;
}
#endif


inline void phBound::SetIndexInBound(int indexInBound)
{
	Assert(indexInBound >= 0 && indexInBound <= 0xFFFF);
	m_PartIndex = (u16)indexInBound;
}

inline void phBound::ShiftCentroidOffset (Vec3V_In UNUSED_PARAM(offsetDelta))
{
}

__forceinline bool phBound::GetForceCCD() const
{
	return (m_Flags & FORCE_CCD_THIS_BOUND) != 0;
}

__forceinline u32 phBound::GetForceCCDFlag() const
{
	return m_Flags & FORCE_CCD_THIS_BOUND;
}

__forceinline void phBound::SetForceCCD()
{
	m_Flags |= FORCE_CCD_THIS_BOUND;
}

__forceinline void phBound::ClearForceCCD()
{
	m_Flags &= ~FORCE_CCD_THIS_BOUND;
}

__forceinline bool phBound::GetUseCurrentInstsanceMatrixOnly() const
{
	return (m_Flags & USE_CURRENT_INSTANCE_MATRIX_ONLY) != 0;
}

__forceinline u32 phBound::GetUseCurrentInstsanceMatrixOnlyFlag() const
{
	return m_Flags & USE_CURRENT_INSTANCE_MATRIX_ONLY;
}

__forceinline void phBound::SetUseCurrentInstsanceMatrixOnly()
{
	m_Flags |= USE_CURRENT_INSTANCE_MATRIX_ONLY;
}

__forceinline void phBound::ClearUseCurrentInstsanceMatrixOnly()
{
	m_Flags &= ~USE_CURRENT_INSTANCE_MATRIX_ONLY;
}

#if USE_NEW_TRIANGLE_BACK_FACE_CULL_OPTIONAL
__forceinline u32 phBound::GetUseNewBackFaceCull() const
{
	return m_Flags & USE_NEW_BACK_FACE_CULL;
}

__forceinline void phBound::EnableUseNewBackFaceCull()
{
	m_Flags |= USE_NEW_BACK_FACE_CULL;
}

__forceinline void phBound::DisableUseNewBackFaceCull()
{
	m_Flags &= ~USE_NEW_BACK_FACE_CULL;
}
#endif // USE_NEW_TRIANGLE_BACK_FACE_CULL_OPTIONAL

#if USE_PROJECTION_EDGE_FILTERING
__forceinline u32 phBound::GetUseProjectionEdgeFiltering() const
{
	return m_Flags & USE_PROJECTION_EDGE_FILTERING_FLAG;
}

__forceinline void phBound::EnableProjectionEdgeFiltering()
{
	m_Flags |= USE_PROJECTION_EDGE_FILTERING_FLAG;
}

__forceinline void phBound::DisableProjectionEdgeFiltering()
{
	m_Flags &= ~USE_PROJECTION_EDGE_FILTERING_FLAG;
}
#endif // USE_PROJECTION_EDGE_FILTERING

inline const Vec3V_Out phBound::GetCGOffset() const
{
	return m_CGOffsetXYZMaterialId1W.GetXYZ();
}

inline Vec3V* phBound::GetCGOffsetPtr ()
{
	return (Vec3V*)&m_CGOffsetXYZMaterialId1W;
}

inline Vec3V_ConstRef phBound::GetBoundingBoxMin() const
{
	return (Vec3V_ConstRef)(m_BoundingBoxMinXYZRefCountW);
}


inline Vec3V_ConstRef phBound::GetBoundingBoxMax() const
{
	return (Vec3V_ConstRef)(m_BoundingBoxMaxXYZMarginW);
}


inline void phBound::SetBoundingBox (Vec3V_In boxMin, Vec3V_In boxMax)
{
	SetBoundingBoxMin(boxMin);
	SetBoundingBoxMax(boxMax);
}

inline Vec3V_Out phBound::GetBoundingBoxSize() const
{
	return GetBoundingBoxMax() - GetBoundingBoxMin();
}

inline ScalarV_Out phBound::GetBoundingBoxVolume () const
{
	Vec3V boundingBoxSize = GetBoundingBoxSize();
	return boundingBoxSize.GetX() * boundingBoxSize.GetY() * boundingBoxSize.GetZ();	
}

inline Vec3V_Out phBound::ComputeBoundingBoxCenter () const
{
	return Average(GetBoundingBoxMax(), GetBoundingBoxMin());
}


inline float phBound::GetRadiusAroundCentroid() const
{
	return m_RadiusAroundCentroid;
}

__forceinline ScalarV_Out phBound::GetRadiusAroundCentroidV() const
{
	return ScalarVFromF32(m_RadiusAroundCentroid);
}

inline void phBound::SetRadiusAroundCentroid (ScalarV_In radius)
{
	m_RadiusAroundCentroid = radius.Getf();
}

inline Vec3V_Out phBound::GetWorldCentroid (Mat34V_In instancePose) const
{
	return Transform(instancePose, GetCentroidOffset());
}

__forceinline void phBound::GetBoundingBoxHalfWidthAndCenter (Vec3V_InOut halfWidth, Vec3V_InOut center) const
{

	Vec3V v_BoundingBoxMin = GetBoundingBoxMin();
	Vec3V v_BoundingBoxMax = GetBoundingBoxMax();

    halfWidth = Subtract(v_BoundingBoxMax, v_BoundingBoxMin);
	halfWidth = Scale(halfWidth, Vec3V(V_HALF));
    center = Average(v_BoundingBoxMax, v_BoundingBoxMin);
}

inline Vec3V_Out phBound::GetCenterOfMass (Mat34V_In matrix) const
{
	return Transform(matrix, GetCGOffset());
}

__forceinline bool phBound::IsConvex() const
{
	return IsTypeConvex(GetType());
}


__forceinline bool phBound::IsConcave() const
{
	return !IsConvex();
}

__forceinline bool phBound::IsPolygonal() const
{
	return (m_Type == phBound::GEOMETRY) || (m_Type == phBound::BVH) USE_GEOMETRY_CURVED_ONLY(|| (m_Type == phBound::GEOMETRY_CURVED));
}
	
__forceinline float phBound::GetMargin() const
{
	return GetMarginV().Getf();
}


__forceinline ScalarV_Out phBound::GetMarginV () const
{
	return m_BoundingBoxMaxXYZMarginW.GetW();
}

__forceinline void phBound::SetMargin (ScalarV_In collisionMargin)
{
	m_BoundingBoxMaxXYZMarginW.SetW(collisionMargin);
}


__forceinline void phBound::SetMargin (float collisionMargin)
{
	SetMargin(ScalarVFromF32(collisionMargin));
}

// This will create a bound on the stack that will not have its destructor called. 
// Only simple bounds like boxes, capsules, and discs can use this feature because
//   they don't own any memory that could be leaked. The advantage of this is that
//   we avoid the pgBase destructor cost which can cause stalls. 
#define CREATE_SIMPLE_BOUND_ON_STACK(BoundType, BoundName, ...)					\
	CompileTimeAssert(!BoundType::ms_RequiresDestructorOnStack);				\
	ALIGNAS(16) u8 BoundName##Buffer[sizeof(BoundType)] ;						\
	BoundType& BoundName = *new(&BoundName##Buffer) BoundType( __VA_ARGS__ );	\

} // namespace rage

#endif // ndef PHBOUND_BOUND_H
