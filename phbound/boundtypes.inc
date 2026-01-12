//BOUND_TYPE_INC(className,enumName,enumValue,stringName,isUsed)
BOUND_TYPE_INC(phBoundSphere,SPHERE,0,"sphere",1)
BOUND_TYPE_INC(phBoundCapsule,CAPSULE,1,"capsule",1)
#if USE_TAPERED_CAPSULE
	BOUND_TYPE_INC(phBoundTaperedCapsule,TAPERED_CAPSULE,2,"tapered capsule",1)
#else
	BOUND_TYPE_INC(phRemovedBound,REMOVED_TAPERED_CAPSULE,2,"removed tapered capsule",0)
#endif
BOUND_TYPE_INC(phBoundBox,BOX,3,"box",1)
BOUND_TYPE_INC(phBoundGeometry,GEOMETRY,4,"geometry",1)
#if USE_GEOMETRY_CURVED
	BOUND_TYPE_INC(phBoundCurvedGeometry,GEOMETRY_CURVED,5,"curved geometry",1)
#else
	BOUND_TYPE_INC(phRemovedBound,REMOVED_GEOMETRY_CURVED,5,"removed curved geometry",0)
#endif
#if USE_GRIDS
	BOUND_TYPE_INC(phBoundGrid,GRID,6,"grid",1)
#else
	BOUND_TYPE_INC(phRemovedBound,REMOVED_GRID,6,"removed grid",0)
#endif
#if USE_RIBBONS
	BOUND_TYPE_INC(phBoundRibbon,RIBBON,7,"ribbon",1)
#else
	BOUND_TYPE_INC(phRemovedBound,REMOVED_RIBBON,7,"removed ribbon",0)
#endif
BOUND_TYPE_INC(phBoundBVH,BVH,8,"bvh",1)
#if USE_SURFACES
	BOUND_TYPE_INC(phBoundSurface,SURFACE,9,"surface",1)
#else
	BOUND_TYPE_INC(phRemovedBound,REMOVED_SURFACE,9,"removed surface",0)
#endif
BOUND_TYPE_INC(phBoundComposite,COMPOSITE,10,"composite",1)
BOUND_TYPE_INC(TriangleShape,TRIANGLE,11,"triangle",1)
BOUND_TYPE_INC(phBoundDisc,DISC,12,"disc",1)
BOUND_TYPE_INC(phBoundCylinder,CYLINDER,13,"cylinder",1)
BOUND_TYPE_INC(phRemovedBound,REMOVED_PROBE,14,"removed probe",0)
BOUND_TYPE_INC(phBoundPlane,PLANE,15,"plane",1)
