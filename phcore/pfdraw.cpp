// 
// phcore/pfdraw.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#include "grprofile/drawmanager.h"
#include "grprofile/drawgroup.h"
#include "vector/colors.h"
#include "phbound/boundpolyhedron.h" // required for density visualisation widget initial values
#include "phbullet/DiscreteCollisionDetectorInterface.h"

namespace rage {

PFD_DECLARE_GROUP(Physics);
PFD_DECLARE_ITEM_ON(Models,Color_white,Physics);
PFD_DECLARE_ITEM(Centroid,Color_yellow,Physics);
PFD_DECLARE_ITEM(CenterOfGravity,Color_white,Physics);
PFD_DECLARE_ITEM(AngularInertia,Color_white,Physics);
PFD_DECLARE_ITEM_ON(InvertAngularInertia,Color_white,Physics);
PFD_DECLARE_ITEM_SLIDER(AngularInertiaScale,Color_white,Physics,1.0f,100.0f,0.01f);
PFD_DECLARE_ITEM(BroadphasePairs,Color_SkyBlue,Physics);
PFD_DECLARE_ITEM(CullSpheres,Color_green,Physics);
PFD_DECLARE_ITEM(CullBoxes,Color_green,Physics);
PFD_DECLARE_ITEM(SweptCullBoxes,Color_green,Physics);
PFD_DECLARE_ITEM(CullSolid,Color_white,Physics);
PFD_DECLARE_ITEM(Islands,Color_white,Physics);
PFD_DECLARE_ITEM(SleepIslands,Color_green,Physics);
PFD_DECLARE_ITEM_SLIDER_INT(CullOpacity,Color_white,Physics,200,255,1);
PFD_DECLARE_ITEM(Constraints,Color_purple,Physics);
PFD_DECLARE_ITEM_SLIDER(ConstraintLimitScale,Color_white,Physics,1.0f,10.0f,0.1f);
PFD_DECLARE_ITEM(LevelIndices,Color_white,Physics);
PFD_DECLARE_ITEM(InactiveCollidesVsInactive,Color_white,Physics);
PFD_DECLARE_ITEM(InactiveCollidesVsFixed,Color_white,Physics);
PFD_DECLARE_ITEM(InstMatrices,Color_white,Physics);
PFD_DECLARE_ITEM(BvhNodeCollisions,Color_white,Physics);
#if TRACK_COLLISION_TIME
PFD_DECLARE_ITEM(CollisionTime,Color_white,Physics);
#endif
PFD_DECLARE_ITEM_ON(PhysicsDemoWorld,Color_white,Physics);
PFD_DECLARE_ITEM(AutoCCD,Color_yellow,Physics);

PFD_DECLARE_SUBGROUP_ON(Bounds,Physics);
PFD_DECLARE_ITEM(Solid,Color_red,Bounds);
PFD_DECLARE_ITEM(EdgeAngles,Color_red,Bounds);
PFD_DECLARE_ITEM_ON(Wireframe,Color_red,Bounds);
PFD_DECLARE_ITEM_ON(Active,Color_red,Bounds);
PFD_DECLARE_ITEM_ON(Inactive,Color_green,Bounds);
PFD_DECLARE_ITEM_ON(Fixed,Color_grey,Bounds);
PFD_DECLARE_ITEM(Quads,Color_grey,Bounds);
PFD_DECLARE_ITEM(FlattenQuads,Color_blue,Bounds);
PFD_DECLARE_ITEM_ON(SphereCull,Color_grey,Bounds);
PFD_DECLARE_ITEM(PolyNeighbors,Color_green,Bounds);
PFD_DECLARE_ITEM_SLIDER(PolyNeighborsLength,Color_green,Bounds,0.1f,1.0f,0.01f);
PFD_DECLARE_ITEM_SLIDER(PolyNeighborsDensity,Color_green,Bounds,2.0f,100.0f,0.1f);
PFD_DECLARE_ITEM(SupportPoints,Color_red,Bounds);
PFD_DECLARE_ITEM_SLIDER(SupportPointsSamples,Color_red,Bounds,10.0f,64.0f,1.0f);
PFD_DECLARE_ITEM_ON(SupportMargin,Color_red,Bounds);
PFD_DECLARE_ITEM(AnimateFromLast,Color_red,Bounds);
PFD_DECLARE_ITEM_SLIDER(AnimateTime,Color_red,Bounds,3.0f,100.0f,1.0f);
#if EARLY_FORCE_SOLVE
PFD_DECLARE_ITEM(CollisionSweep,Color_white,Bounds);
PFD_DECLARE_ITEM_SLIDER_INT(CollisionSweepIteration,Color_white,Bounds,0,100,1);
PFD_DECLARE_ITEM_SLIDER(CollisionSweepTime,Color_red,Bounds,3.0f,100.0f,1.0f);
#endif // EARLY_FORCE_SOLVE
PFD_DECLARE_ITEM(PreviousFrame,Color_grey,Bounds);
PFD_DECLARE_ITEM(PreviousSafeFrame,Color_green,Bounds);
PFD_DECLARE_ITEM(PrePush,Color_white,Bounds);
PFD_DECLARE_ITEM(CulledPolygons,Color_white,Bounds);
PFD_DECLARE_ITEM(BoundNames,Color_white,Bounds);
PFD_DECLARE_ITEM(BoundTypes,Color_white,Bounds);
PFD_DECLARE_ITEM_ON(SolidBoundLighting,Color_white,Bounds);
PFD_DECLARE_ITEM(SolidBoundRandom,Color_white,Bounds);
PFD_DECLARE_ITEM_SLIDER(BoundDrawDistance,Color_white,Bounds,25.0f,1000.0f,0.01f);
PFD_DECLARE_ITEM_SLIDER(BoundDistanceOpacity,Color_white,Bounds,1.0f,1.0f,0.01f);
PFD_DECLARE_ITEM_SLIDER(BoundSolidOpacity,Color_white,Bounds,1.0f,1.0f,0.01f);
PFD_DECLARE_SUBGROUP(BoundQA,Bounds);
PFD_DECLARE_ITEM_ON(ThinPolys,Color_white,BoundQA);
PFD_DECLARE_ITEM_ON(BadNormalPolys,Color_white,BoundQA);
PFD_DECLARE_ITEM_ON(BadNeighborPolys,Color_white,BoundQA);
PFD_DECLARE_ITEM_ON(BigPrimitives,Color_white,BoundQA);
PFD_DECLARE_ITEM_SLIDER(AttentionSphere,Color_red,BoundQA,3.0f,10.0f,1.0f);
PFD_DECLARE_ITEM_SLIDER(ThinPolyTolerance,Color_red,BoundQA,3.0f,10.0f,1.0f);
PFD_DECLARE_ITEM_SLIDER(BigPrimTolerance,Color_red,BoundQA,50.0f,100.0f,1.0f);
PFD_DECLARE_SUBGROUP(PolygonDensity,BoundQA);
PFD_DECLARE_ITEM_SLIDER_INT_FULL(NumRecursions,Color_red,PolygonDensity,DEFAULT_PFD_NumRecursions,0,8,1);
PFD_DECLARE_ITEM_SLIDER_INT(RecusiveWeightingStyle,Color_red,PolygonDensity,DEFAULT_PFD_RecusiveWeightingStyle,2,1);
PFD_DECLARE_ITEM_SLIDER(ColorMidPoint,Color_red,PolygonDensity,DEFAULT_PFD_ColorMidPoint,1.0f,0.01f);
PFD_DECLARE_ITEM_SLIDER_FULL(ColorExpBias,Color_red,PolygonDensity,DEFAULT_PFD_ColorExpBias,-4.0f,4.0f,0.01f);
PFD_DECLARE_ITEM_SLIDER(SmallPolygonArea,Color_red,PolygonDensity,DEFAULT_PFD_SmallPolygonArea,1.0f,0.01f);
PFD_DECLARE_ITEM_SLIDER(MediumPolygonArea,Color_red,PolygonDensity,DEFAULT_PFD_MediumPolygonArea,10.0f,0.1f);
PFD_DECLARE_ITEM_SLIDER(LargePolygonArea,Color_red,PolygonDensity,DEFAULT_PFD_LargePolygonArea,100.0f,1.0f);
PFD_DECLARE_ITEM_TOGGLE(PolygonAngleDensity,Color_red,PolygonDensity,DEFAULT_PFD_PolygonAngleDensity);
PFD_DECLARE_ITEM_TOGGLE(MaxNeighborAngleEnabled,Color_red,PolygonDensity,DEFAULT_PFD_MaxNeighborAngleEnabled);
PFD_DECLARE_ITEM_SLIDER_FULL(MaxNeighborAngle,Color_red,PolygonDensity,DEFAULT_PFD_MaxNeighborAngle,0.0f,180.0f,0.1f);

PFD_DECLARE_SUBGROUP(PrimitiveDensity,BoundQA);
PFD_DECLARE_ITEM_TOGGLE(IncludePolygons,Color_red,PrimitiveDensity,DEFAULT_PFD_IncludePolygons);
PFD_DECLARE_ITEM_SLIDER(DensityCullRadius,Color_red,PrimitiveDensity,DEFAULT_PFD_DensityCullRadius,30.0f,0.1f);
PFD_DECLARE_ITEM_SLIDER(MaxPrimitivesPerMeterSquared,Color_red,PrimitiveDensity,DEFAULT_PFD_MaxPrimitivesPerMeterSquared,10.0f,0.1f);
PFD_DECLARE_ITEM_SLIDER(MaxNonPolygonsPerMeterSquared,Color_red,PrimitiveDensity,DEFAULT_PFD_MaxNonPolygonsPerMeterSquared,10.0f,0.1f);

PFD_DECLARE_SUBGROUP_ON(TypeFlagFilter, Bounds);
#if __BANK
PFD_DECLARE_ITEM_BUTTON(EnableAllTypeFlags, TypeFlagFilter, pfDrawGroup::EnableChildren, &PFDGROUP_TypeFlagFilter, 0);
PFD_DECLARE_ITEM_BUTTON(DisableAllTypeFlags, TypeFlagFilter, pfDrawGroup::DisableChildren, &PFDGROUP_TypeFlagFilter, 0);
#endif	// __BANK
PFD_DECLARE_ITEM_ON(TypeFlag0, Color_white, TypeFlagFilter);
PFD_DECLARE_ITEM_ON(TypeFlag1, Color_white, TypeFlagFilter);
PFD_DECLARE_ITEM_ON(TypeFlag2, Color_white, TypeFlagFilter);
PFD_DECLARE_ITEM_ON(TypeFlag3, Color_white, TypeFlagFilter);
PFD_DECLARE_ITEM_ON(TypeFlag4, Color_white, TypeFlagFilter);
PFD_DECLARE_ITEM_ON(TypeFlag5, Color_white, TypeFlagFilter);
PFD_DECLARE_ITEM_ON(TypeFlag6, Color_white, TypeFlagFilter);
PFD_DECLARE_ITEM_ON(TypeFlag7, Color_white, TypeFlagFilter);
PFD_DECLARE_ITEM_ON(TypeFlag8, Color_white, TypeFlagFilter);
PFD_DECLARE_ITEM_ON(TypeFlag9, Color_white, TypeFlagFilter);
PFD_DECLARE_ITEM_ON(TypeFlag10, Color_white, TypeFlagFilter);
PFD_DECLARE_ITEM_ON(TypeFlag11, Color_white, TypeFlagFilter);
PFD_DECLARE_ITEM_ON(TypeFlag12, Color_white, TypeFlagFilter);
PFD_DECLARE_ITEM_ON(TypeFlag13, Color_white, TypeFlagFilter);
PFD_DECLARE_ITEM_ON(TypeFlag14, Color_white, TypeFlagFilter);
PFD_DECLARE_ITEM_ON(TypeFlag15, Color_white, TypeFlagFilter);
PFD_DECLARE_ITEM_ON(TypeFlag16, Color_white, TypeFlagFilter);
PFD_DECLARE_ITEM_ON(TypeFlag17, Color_white, TypeFlagFilter);
PFD_DECLARE_ITEM_ON(TypeFlag18, Color_white, TypeFlagFilter);
PFD_DECLARE_ITEM_ON(TypeFlag19, Color_white, TypeFlagFilter);
PFD_DECLARE_ITEM_ON(TypeFlag20, Color_white, TypeFlagFilter);
PFD_DECLARE_ITEM_ON(TypeFlag21, Color_white, TypeFlagFilter);
PFD_DECLARE_ITEM_ON(TypeFlag22, Color_white, TypeFlagFilter);
PFD_DECLARE_ITEM_ON(TypeFlag23, Color_white, TypeFlagFilter);
PFD_DECLARE_ITEM_ON(TypeFlag24, Color_white, TypeFlagFilter);
PFD_DECLARE_ITEM_ON(TypeFlag25, Color_white, TypeFlagFilter);
PFD_DECLARE_ITEM_ON(TypeFlag26, Color_white, TypeFlagFilter);
PFD_DECLARE_ITEM_ON(TypeFlag27, Color_white, TypeFlagFilter);
PFD_DECLARE_ITEM_ON(TypeFlag28, Color_white, TypeFlagFilter);
PFD_DECLARE_ITEM_ON(TypeFlag29, Color_white, TypeFlagFilter);
PFD_DECLARE_ITEM_ON(TypeFlag30, Color_white, TypeFlagFilter);
PFD_DECLARE_ITEM_ON(TypeFlag31, Color_white, TypeFlagFilter);

PFD_DECLARE_SUBGROUP_ON(IncludeFlagFilter, Bounds);
#if __BANK
PFD_DECLARE_ITEM_BUTTON(EnableAllIncludeFlags, IncludeFlagFilter, pfDrawGroup::EnableChildren, &PFDGROUP_IncludeFlagFilter, 0);
PFD_DECLARE_ITEM_BUTTON(DisableAllIncludeFlags, IncludeFlagFilter, pfDrawGroup::DisableChildren, &PFDGROUP_IncludeFlagFilter, 0);
#endif	// __BANK
PFD_DECLARE_ITEM_ON(IncludeFlag0, Color_white, IncludeFlagFilter);
PFD_DECLARE_ITEM_ON(IncludeFlag1, Color_white, IncludeFlagFilter);
PFD_DECLARE_ITEM_ON(IncludeFlag2, Color_white, IncludeFlagFilter);
PFD_DECLARE_ITEM_ON(IncludeFlag3, Color_white, IncludeFlagFilter);
PFD_DECLARE_ITEM_ON(IncludeFlag4, Color_white, IncludeFlagFilter);
PFD_DECLARE_ITEM_ON(IncludeFlag5, Color_white, IncludeFlagFilter);
PFD_DECLARE_ITEM_ON(IncludeFlag6, Color_white, IncludeFlagFilter);
PFD_DECLARE_ITEM_ON(IncludeFlag7, Color_white, IncludeFlagFilter);
PFD_DECLARE_ITEM_ON(IncludeFlag8, Color_white, IncludeFlagFilter);
PFD_DECLARE_ITEM_ON(IncludeFlag9, Color_white, IncludeFlagFilter);
PFD_DECLARE_ITEM_ON(IncludeFlag10, Color_white, IncludeFlagFilter);
PFD_DECLARE_ITEM_ON(IncludeFlag11, Color_white, IncludeFlagFilter);
PFD_DECLARE_ITEM_ON(IncludeFlag12, Color_white, IncludeFlagFilter);
PFD_DECLARE_ITEM_ON(IncludeFlag13, Color_white, IncludeFlagFilter);
PFD_DECLARE_ITEM_ON(IncludeFlag14, Color_white, IncludeFlagFilter);
PFD_DECLARE_ITEM_ON(IncludeFlag15, Color_white, IncludeFlagFilter);
PFD_DECLARE_ITEM_ON(IncludeFlag16, Color_white, IncludeFlagFilter);
PFD_DECLARE_ITEM_ON(IncludeFlag17, Color_white, IncludeFlagFilter);
PFD_DECLARE_ITEM_ON(IncludeFlag18, Color_white, IncludeFlagFilter);
PFD_DECLARE_ITEM_ON(IncludeFlag19, Color_white, IncludeFlagFilter);
PFD_DECLARE_ITEM_ON(IncludeFlag20, Color_white, IncludeFlagFilter);
PFD_DECLARE_ITEM_ON(IncludeFlag21, Color_white, IncludeFlagFilter);
PFD_DECLARE_ITEM_ON(IncludeFlag22, Color_white, IncludeFlagFilter);
PFD_DECLARE_ITEM_ON(IncludeFlag23, Color_white, IncludeFlagFilter);
PFD_DECLARE_ITEM_ON(IncludeFlag24, Color_white, IncludeFlagFilter);
PFD_DECLARE_ITEM_ON(IncludeFlag25, Color_white, IncludeFlagFilter);
PFD_DECLARE_ITEM_ON(IncludeFlag26, Color_white, IncludeFlagFilter);
PFD_DECLARE_ITEM_ON(IncludeFlag27, Color_white, IncludeFlagFilter);
PFD_DECLARE_ITEM_ON(IncludeFlag28, Color_white, IncludeFlagFilter);
PFD_DECLARE_ITEM_ON(IncludeFlag29, Color_white, IncludeFlagFilter);
PFD_DECLARE_ITEM_ON(IncludeFlag30, Color_white, IncludeFlagFilter);
PFD_DECLARE_ITEM_ON(IncludeFlag31, Color_white, IncludeFlagFilter);

PFD_DECLARE_SUBGROUP_ON(BoundMaterials,Bounds);
PFD_DECLARE_ITEM(DrawBoundMaterials,Color_white,BoundMaterials);
PFD_DECLARE_ITEM(DrawBoundMaterialNames,Color_white,BoundMaterials);
PFD_DECLARE_ITEM_ON(MaterialUseColorPalette,Color_white,BoundMaterials);
PFD_DECLARE_ITEM(MaterialUseFlagColors,Color_white,BoundMaterials);
PFD_DECLARE_ITEM_SLIDER(MaterialColorPalette,Color_white,BoundMaterials,0,65535,1);
PFD_DECLARE_ITEM_SLIDER_INT(HighlightFlags,Color_white,BoundMaterials,0,(1<<24)-1,1);

PFD_DECLARE_SUBGROUP_ON(BoundIndices,Bounds);
PFD_DECLARE_ITEM(BvhVertexIndices,Color_red,BoundIndices);
PFD_DECLARE_ITEM(NonBvhVertexIndices,Color_red,BoundIndices);
PFD_DECLARE_ITEM(BvhPolygonIndices,Color_blue,BoundIndices);
PFD_DECLARE_ITEM(NonBvhPolygonIndices,Color_blue,BoundIndices);
PFD_DECLARE_ITEM(ComponentIndices,Color_black,BoundIndices);
PFD_DECLARE_ITEM(DrawSingleComponent,Color_white,BoundIndices);
PFD_DECLARE_ITEM_SLIDER_INT(SelectComponentToDraw,Color_white,BoundIndices,0,65535,1);

PFD_DECLARE_SUBGROUP_ON(BoundMargins,Bounds);
PFD_DECLARE_ITEM_ON(MarginOriginalPolygons,Color_red,BoundMargins);
PFD_DECLARE_ITEM_ON(MarginOriginalPolygonsSolid,Color_red,BoundMargins);
PFD_DECLARE_ITEM(MarginShrunkPolygons,Color_red,BoundMargins);
PFD_DECLARE_ITEM(MarginExpandedShrunkPolygons,Color_red,BoundMargins);
PFD_DECLARE_ITEM(MarginExpandedShrunkRoundEdges,Color_red,BoundMargins);
PFD_DECLARE_ITEM(MarginExpandedShrunkRoundCorners,Color_red,BoundMargins);

PFD_DECLARE_SUBGROUP(BVHs,Bounds);
PFD_DECLARE_ITEM(BVHHierarchy,Color_chartreuse,BVHs);
PFD_DECLARE_ITEM(BVHHierarchyNodeIndices,Color_yellow,BVHs);
PFD_DECLARE_ITEM_SLIDER(NodeDepth,Color_chartreuse,BVHs,0.0f,20.0f,1.0f);
PFD_DECLARE_ITEM(BVHSubtreeColoring,Color_chartreuse,BVHs);
PFD_DECLARE_ITEM(BVHNodeColoring,Color_chartreuse,BVHs);
PFD_DECLARE_ITEM(BVHDepthColoring,Color_chartreuse,BVHs);

PFD_DECLARE_SUBGROUP_ON(Octrees,Bounds);
PFD_DECLARE_ITEM(OctreeGridCellBoundaries,Color_chartreuse,Octrees);
PFD_DECLARE_ITEM(OctreePolyCount,Color32(0.2f,1.0f,0.0f,0.5f),Octrees);
PFD_DECLARE_ITEM(OctreeCells,Color_chartreuse,Octrees);

PFD_DECLARE_SUBGROUP_ON(Quadtrees,Bounds);
PFD_DECLARE_ITEM(QuadtreeCells,Color_chartreuse,Quadtrees);

PFD_DECLARE_SUBGROUP(Normals,Physics);
PFD_DECLARE_ITEM_SLIDER(NormalLength,Color_red,Normals,1.0f,10.0f,0.01f);
PFD_DECLARE_ITEM_ON(Face,Color_red,Normals);
PFD_DECLARE_ITEM(Edge,Color_green,Normals);

PFD_DECLARE_SUBGROUP(Impetuses,Physics);
PFD_DECLARE_ITEM_ON(ImpactNormals,Color_yellow,Impetuses);
PFD_DECLARE_ITEM_ON(Forces,Color_white,Impetuses);
PFD_DECLARE_ITEM_ON(Impulses,Color_green,Impetuses);
PFD_DECLARE_ITEM_ON(Pushes,Color_purple,Impetuses);
PFD_DECLARE_ITEM_ON(Torques,Color_blue,Impetuses);
PFD_DECLARE_ITEM_ON(AngularImpulses,Color_orange,Impetuses);
PFD_DECLARE_ITEM_ON(Turns,Color_beige,Impetuses);

PFD_DECLARE_SUBGROUP(Manifolds,Physics);
PFD_DECLARE_ITEM_ON(ManifoldsA,Color_SkyBlue1,Manifolds);
PFD_DECLARE_ITEM(ManifoldsB,Color_PaleGreen,Manifolds);
PFD_DECLARE_ITEM(ManifoldsARefreshed,Color_SkyBlue1,Manifolds);
PFD_DECLARE_ITEM(ManifoldsBRefreshed,Color_PaleGreen,Manifolds);
PFD_DECLARE_ITEM_ON(ManifoldInactiveContacts,Color_SkyBlue1,Manifolds);
PFD_DECLARE_ITEM(ManifoldHUD,Color_white,Manifolds);
PFD_DECLARE_ITEM(ManifoldNewest,Color_pink1,Manifolds);
PFD_DECLARE_ITEM(ManifoldNormals,Color_yellow,Manifolds);
PFD_DECLARE_ITEM(ManifoldDepths,Color_magenta4,Manifolds);
PFD_DECLARE_ITEM(ManifoldImpulses,Color_green,Manifolds);
PFD_DECLARE_ITEM(ManifoldPushes,Color_purple,Manifolds);
PFD_DECLARE_ITEM_SLIDER(ManifoldDrawLength,Color_red,Manifolds,0.1f,10.0f,0.01f);

PFD_DECLARE_SUBGROUP_ON(ShapeTests,Physics);
PFD_DECLARE_ITEM(PhysicsLevelCullShape,Color_purple,ShapeTests);
PFD_DECLARE_ITEM(ShapeTestBoundCull,Color_magenta3,ShapeTests);
PFD_DECLARE_ITEM(ProbeSegments,Color_white,ShapeTests);
PFD_DECLARE_ITEM(ProbeIsects,Color_red,ShapeTests);
PFD_DECLARE_ITEM(ProbeNormals,Color_green,ShapeTests);
PFD_DECLARE_ITEM(EdgeSegments,Color_white,ShapeTests);
PFD_DECLARE_ITEM(EdgeIsects,Color_red,ShapeTests);
PFD_DECLARE_ITEM(EdgeNormals,Color_green,ShapeTests);
PFD_DECLARE_ITEM(SphereSegments,Color_white,ShapeTests);
PFD_DECLARE_ITEM(SphereIsects,Color_red,ShapeTests);
PFD_DECLARE_ITEM(SphereNormals,Color_green,ShapeTests);
PFD_DECLARE_ITEM(CapsuleSegments,Color_white,ShapeTests);
PFD_DECLARE_ITEM(CapsuleIsects,Color_red,ShapeTests);
PFD_DECLARE_ITEM(CapsuleNormals,Color_green,ShapeTests);
PFD_DECLARE_ITEM(SweptSphereSegments,Color_white,ShapeTests);
PFD_DECLARE_ITEM(SweptSphereIsects,Color_red,ShapeTests);
PFD_DECLARE_ITEM(SweptSphereNormals,Color_green,ShapeTests);
PFD_DECLARE_ITEM(TaperedSweptSphereSegments,Color_white,ShapeTests);
PFD_DECLARE_ITEM(TaperedSweptSphereIsects,Color_red,ShapeTests);
PFD_DECLARE_ITEM(TaperedSweptSphereNormals,Color_green,ShapeTests);
PFD_DECLARE_ITEM(ScalingSweptQuadSegments,Color_white,ShapeTests);
PFD_DECLARE_ITEM(ScalingSweptQuadIsects,Color_red,ShapeTests);
PFD_DECLARE_ITEM(ScalingSweptQuadNormals,Color_green,ShapeTests);
PFD_DECLARE_ITEM(ObjectSegments,Color_white,ShapeTests);
PFD_DECLARE_ITEM(ObjectIsects,Color_red,ShapeTests);
PFD_DECLARE_ITEM(ObjectNormals,Color_green,ShapeTests);
PFD_DECLARE_ITEM(BoxSegments,Color_white,ShapeTests);
PFD_DECLARE_ITEM(BoxIsects,Color_red,ShapeTests);
PFD_DECLARE_ITEM(BoxNormals,Color_green,ShapeTests);
PFD_DECLARE_ITEM(ObjectManifolds,Color_green,ShapeTests);

PFD_DECLARE_SUBGROUP(Sleep,Physics);
PFD_DECLARE_ITEM_ON(PutToSleep,Color_green,Sleep);
PFD_DECLARE_ITEM_ON(Awakened,Color_red,Sleep);
PFD_DECLARE_ITEM(SleepHUD,Color_white,Sleep);
PH_SLEEP_DEBUG_ONLY(PFD_DECLARE_ITEM(SleepDebug,Color_white,Sleep);)

PFD_DECLARE_SUBGROUP_ON(Colliders,Physics);
PFD_DECLARE_ITEM(ColliderMassText,Color_LightBlue,Colliders);
PFD_DECLARE_ITEM(ColliderAngInertiaText,Color_LightBlue,Colliders);
PFD_DECLARE_ITEM(ColliderExtraAllowedPenetrationText,Color_LightBlue,Colliders);

PFD_DECLARE_SUBGROUP_ON(ArticulatedBodies,Physics);
PFD_DECLARE_SUBGROUP(ArticulatedLinks,ArticulatedBodies);
PFD_DECLARE_ITEM(ArticulatedLinkMassText,Color_white,ArticulatedLinks);
PFD_DECLARE_ITEM(ArticulatedLinkAngInertiaText,Color_white,ArticulatedLinks);
PFD_DECLARE_SUBGROUP(ArticulatedJoints,ArticulatedBodies);
PFD_DECLARE_ITEM_ON(Articulated1DofJoints,Color_white,ArticulatedJoints);
PFD_DECLARE_ITEM_ON(Articulated3DofJoints,Color_white,ArticulatedJoints);
PFD_DECLARE_ITEM(ArticulatedJointAngles,Color_white,ArticulatedBodies);
PFD_DECLARE_ITEM_SLIDER(ArticulatedJointLength,Color_white,ArticulatedBodies,0.2f,10.0f,0.01f);
PFD_DECLARE_ITEM(ArticulatedJointLimitAngImpulse,Color_yellow,ArticulatedBodies);
PFD_DECLARE_ITEM(ArticulatedMuscleSpeedTorque,Color_MediumAquamarine,ArticulatedBodies);
PFD_DECLARE_ITEM(ArticulatedMuscleAngleTorque,Color_green,ArticulatedBodies);

} // namespace rage
