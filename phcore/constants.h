//
// phcore/constants.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHCORE_CONSTANTS_H
#define PHCORE_CONSTANTS_H

namespace rage {

//////////////////////////////////////////////////////////
// physics draw modes
#define ENABLE_DRAW_PHYS			0

#if ENABLE_DRAW_PHYS
#define DRAW_PHYS_ONLY(x)			x
#else
#define DRAW_PHYS_ONLY(x)
#endif
//////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////
// a cast that uses run-time type information (RTTI)
// to catch improper casting (returns NULL if not derived
// from "type"
//
//#define PH_CAST(type)	dynamic_cast<type>
#define PH_CAST(type)	(type)
//////////////////////////////////////////////////////////

// Each bound has a gravity scaling factor
#define	GRAVITY							-9.8f

#define WATER_DENSITY	1000.0f

#define BAD_INDEX		-1

// The magnitudes of normal errors should be within in this range.
#define POLYGONMAXNORMALERROR	0.03f

#define POLYGON_INDEX u16
#define POLYGON_INDEX_IS_U32	0
#define POLY_MAX_VERTICES	3

#define EARLY_FORCE_SOLVE 1

#if EARLY_FORCE_SOLVE
#define EARLY_FORCE_SOLVE_ONLY(X) X
#define NOT_EARLY_FORCE_SOLVE_ONLY(X)
#define EARLY_FORCE_SOLVE_PARAM(X) , X
#else
#define EARLY_FORCE_SOLVE_ONLY(X)
#define NOT_EARLY_FORCE_SOLVE_ONLY(X) X
#define EARLY_FORCE_SOLVE_PARAM(X)
#endif

#define TRACK_PUSH_COLLIDERS 1
#if TRACK_PUSH_COLLIDERS
#define TRACK_PUSH_COLLIDERS_ONLY(X) X
#else
#define TRACK_PUSH_COLLIDERS_ONLY(X)
#endif

#define USE_RELOCATE_MATRICES 1		// This may help reduce roundoff error for objects that are far from the origin

#define	USE_PRECOMPUTE_SEPARATEBIAS		1
#define	USE_CAPSULE_EXTRA_EXTENTS		1

#define ENSURE_ART_VELOCITY_PROP_IN_UPDATE 1

#define MIDPHASE_USES_QUATS	0

#define PROPHYLACTIC_SWAPS 0

#define USE_GEOMETRY_CURVED 0
#if USE_GEOMETRY_CURVED
	#define USE_GEOMETRY_CURVED_ONLY(x) x
#else
	#define USE_GEOMETRY_CURVED_ONLY(x) 
#endif

#define USE_GRIDS 0
#if USE_GRIDS
	#define USE_GRIDS_ONLY(x) x
#else
	#define USE_GRIDS_ONLY(x) 
#endif

#define USE_RIBBONS 0
#if USE_RIBBONS
	#define USE_RIBBONS_ONLY(x) x
#else
	#define USE_RIBBONS_ONLY(x) 
#endif
	
#define USE_TAPERED_CAPSULE 0
#if USE_TAPERED_CAPSULE
	#define USE_TAPERED_CAPSULE_ONLY(x) x
#else
	#define USE_TAPERED_CAPSULE_ONLY(x) 
#endif
	
#define USE_SURFACES 0
#if USE_SURFACES
	#define USE_SURFACES_ONLY(x) x
#else
	#define USE_SURFACES_ONLY(x) 
#endif
	
#if __SPU
	#define PRIM_CACHE_RENDER 0
#else
	#define PRIM_CACHE_RENDER 0
#endif

// Some defines that switch the existence of some older code we probably don't need in general (Likely we can simply delete the files at some point)
#define ENABLE_UNUSED_PHYSICS_CODE 0
#define ENABLE_UNUSED_CURVE_CODE (ENABLE_UNUSED_PHYSICS_CODE || __TOOL || __RESOURCECOMPILER)
#define ENABLE_EPA_PENETRATION_SOLVER_CODE 1

#define CCD_RESTRICT __restrict

#define CHECK_FOR_DUPLICATE_MANIFOLDS 0//__ASSERT

#define PHPOOL_EXTRA_VERIFICATION 0//__ASSERT		// When enabled, this currently needs REDUCE_SPU_CODE_SIZE enabled.

#define REDUCE_SPU_CODE_SIZE (PHPOOL_EXTRA_VERIFICATION || __ASSERT)

#define PHLEVELNEW_DISABLE_SECONDARY_BROADPHASE	(!ENABLE_UNUSED_PHYSICS_CODE)

#define USE_PHYSICS_PROFILE_CAPTURE (0 && !__SPU)

#define USE_DETERMINISTIC_ORDERING 0			// Use this to improve the determinism of the simulation when profiling.

#define USE_NEW_SELF_COLLISION 1

#define USE_NEW_MID_PHASE 1

#define ALLOW_MID_PHASE_SWAP 0//((!__PS3 || !__ASSERT) && __BANK)

// TODO: MOVE THESE CONTACT CONSTANTS INTO A SEPARATE HEADER.
#define USE_NEGATIVE_DEPTH 1
#define USE_NEGATIVE_DEPTH_TUNABLE (0 && USE_NEGATIVE_DEPTH)

#define USE_CONTACT_PENETRATION_OFFSET 0

#define USE_ACTIVE_COLLISION_DISTANCE 1

#if __DEV
	#define PHYSICS_FORCE_INLINE
#else // __DEV
	#define PHYSICS_FORCE_INLINE __forceinline
#endif // __DEV

#define USE_NEW_TRIANGLE_BACK_FACE_CULL 0
#define COLLISION_MAY_USE_TRIANGLE_PD_SOLVER (!USE_NEW_TRIANGLE_BACK_FACE_CULL)
#define USE_NEW_TRIANGLE_BACK_FACE_CULL_OPTIONAL 1

#define DONT_USE_COMPOSITE_BOUND_LOCAL_BOX_MINMAXS 0

#define USE_NEW_SIMPLEX_SOLVER 1

#define REGENERATE_OCTANT_MAP_AFTER_DEFORMATION 1
#define USE_OCTANT_MAP_INDEX_U16 0
#define USE_OCTANT_MAP_SINGLE_ALLOCATION 1
#define USE_OCTANT_MAP_PERMANENT_ALLOCATION (0 && USE_OCTANT_MAP_SINGLE_ALLOCATION)

#define USE_STATIC_BVH_REJECTION 1
#define USE_STATIC_BVH_REJECTION_SWAP (0 && USE_STATIC_BVH_REJECTION)
#define USE_STATIC_BVH_REJECTION_DEBUG 0
#define USE_PRUNE_NEW_PAIRS 1

#define DELETED_ENITITY_ZPOS -100.0f

// Used to get the frame rate-correct bias to be equal to the old bias for 30fps
#define MAGIC_SEPARATION_BIAS_SYNC_NUMBER 3600.0f

#define USER_JBN 0

#define USE_PROJECTION_EDGE_FILTERING 1

// Turn this on to classify verts into which faces of a cube they are needed for in the support function
// This invalidates resources
#define OCTANT_MAP_SUPPORT_ACCEL (1 && !__TOOL)
#define VERIFY_SUPPORT_FUNCTIONS (__DEV && OCTANT_MAP_SUPPORT_ACCEL)

#define USE_CONSTRAINT_UPDATE_CHECK (!__FINAL && !__TOOL && !__RESOURCECOMPILER)

//We have the option of deforming the surface of polygons on a phBoundGeometry using extra data that 
//represents the movement of all the vertices from the principal surface to the second surface.
#define HACK_GTA4_BOUND_GEOM_SECOND_SURFACE ( (0 && HACK_GTA4) && ENABLE_UNUSED_PHYSICS_CODE )
#define HACK_GTA4_BOUND_GEOM_SECOND_SURFACE_DATA ( ((1 && HACK_GTA4) || HACK_GTA4_BOUND_GEOM_SECOND_SURFACE) /*&& ENABLE_UNUSED_PHYSICS_CODE*/ ) // We should really get rid of this - but changes resources so not simple to flip on/off
#define DEFAULT_SECOND_SURFACE_INTERP_VALUE (0.0001f)

//Changes to way that frag damage and breaking impulses are applied.
#define HACK_GTA4_FRAG_BREAKANDDAMAGE (1 && HACK_GTA4)

//Store results of every ApplyImpulseAndPush-type function invoked by the new force solver.
#define	HACK_GTA4_ADD_FORCER_SOLVER_DEBUG_RECORD (0 && HACK_GTA4)

//Encode material id in 64 bit integer so that we can pack it with lots of lovely flags.
#define PH_MATERIAL_ID_64BIT			(1 && HACK_GTA4)	// EUGENE_APPROVED - but want to remove define
#define HACK_GTA4_64BIT_MATERIAL_ID_COLORS	(1 && PH_MATERIAL_ID_64BIT)	// material color palette stored in phBoundGeometry

#define TRACK_COLLISION_TYPE_PAIRS	(__ASSERT)

// Currently enabled by default on PS3 BankRelease and Beta, and on 360 BankRelease. Not enabled on 360 Beta due to code size limit problems.
#define PH_SLEEP_DEBUG (1 && __BANK && !__SPU && (!__XENON || !__DEV))

#if PH_SLEEP_DEBUG
#define PH_SLEEP_DEBUG_ONLY(X) X
#else // PH_SLEEP_DEBUG
#define PH_SLEEP_DEBUG_ONLY(X)
#endif // PH_SLEEP_DEBUG


#if __SPU
	#define SPU_VALIDATE2(ptr, x)	Assertf((u32)(ptr) < 256*1024 && (u32)(x) < 256*1024, "this = %p,"#x" = %x", (ptr), (u32)x)
	#define SPU_VALIDATE(x)			Assertf((u32)(x) < 256*1024, " "#x" = %x", (u32)x)
#else
	#define SPU_VALIDATE2(ptr, x)
	#define SPU_VALIDATE(x)
#endif


} // namespace rage

#endif //PHCORE_CONSTANTS_H



