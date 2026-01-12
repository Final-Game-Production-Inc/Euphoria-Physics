//  
// pheffects/instbehaviorliquid.cpp 
// 
// Copyright (C) 1999-2007 Rockstar Games.  All Rights Reserved. 
// 
#include "instbehaviorliquid.h"

#include "phbound/boundcomposite.h"
#include "physics/colliderdispatch.h"
#include "physics/sleep.h"

// This define, and the code it disables, should be removed once the changes are confirmed to work correctly.
#define OBSOLETE_PRE_VECTOR_LIBRARY 0


#define USE_UNSAFE_FUNCTIONS		1	// Turn this on to enable the unsafe surface grid functions.
										// They speed up our update by 340% by omitting all bounds
										// checking (which /should/ be within the range at all times.)
#define PERFORM_LIQUID_UPDATE		0	// Set this to 0 to disable any updating of phInstBehaviorLiquid
										// objects.


#if USE_UNSAFE_FUNCTIONS
	#define LIQUID_FUNCTION(x)		x##Unsafe
#else // USE_UNSAFE_FUNCTIONS
	#define LIQUID_FUNCTION(x)		x
#endif  // USE_UNSAFE_FUNCTIONS



namespace rage {

#if USE_SURFACES

	phInstBehaviorLiquidCollidingObjectTuning::phInstBehaviorLiquidCollidingObjectTuning()
			: m_DragFactorScale(1.0f)
			, m_EnableBuoyancy(true)
			, m_EnablePushResistance(true)
			, m_EnableSlidingFriction(true)
			, m_EnableSurfaceReaction(true)
			, m_EnableFlow(true)
	{
	}

	phInstBehaviorLiquidTuning::phInstBehaviorLiquidTuning()
	{
		m_fDampening = 0.99f;
		m_fPropogationSpeed = 18.0f;
		m_fInstBuoyancy = 1.0f;
		m_fPushDrag = -0.154f;
		m_fHorizPushDragScale = 0.2f;
		m_fSlideDrag = -0.031f;
		m_fHorizSlideDragScale = 1.5f; 
		m_fSurfaceNoiseRange = 0.0f;
		m_iSurfaceNoiseCount = 0;
		m_bEnableSurfaceReaction = false;
		m_fMaxWaveHeight = 0.7f;
		m_fSplashScale = 0.036f;
		m_fMaxSplash = 0.10f;
		m_fMaxLocalVelocity = 20.0f;
		m_fConstantPushDrag = 0.0f;
		m_fConstantSlideDrag = 0.0f;
		m_WaterDirection.Set(0.0f, 0.0f, 0.0f);
		m_fWaterVelocityScale = 1.0f;
		m_fDeadFloatForwardScale = 8.0f;
		m_fDeadFloatToShoreScale = 2.0f;
		m_fPushDragRisingFactor = 0.977f;
		m_fPushDragSinkingFactor = 1.0f;
	}

	phInstBehaviorLiquidTuning phInstBehaviorLiquid::sm_DefaultLiquidTuning;
	phInstBehaviorLiquidCollidingObjectTuning phInstBehaviorLiquid::sm_DefaultCollidingObjectTuning;

	void phInstBehaviorLiquid::Reset()
	{
	}


	bool phInstBehaviorLiquid::IsActive() const
	{
		return true;
	}

	void phInstBehaviorLiquid::SetTuningData (const phInstBehaviorLiquidTuning& liquidTuning)
	{
		m_Tune.m_fDampening = liquidTuning.m_fDampening;
		m_Tune.m_fPropogationSpeed = liquidTuning.m_fPropogationSpeed;
		m_Tune.m_fInstBuoyancy = liquidTuning.m_fInstBuoyancy;
		m_Tune.m_fPushDrag = liquidTuning.m_fPushDrag;
		m_Tune.m_fHorizPushDragScale = liquidTuning.m_fHorizPushDragScale;
		m_Tune.m_fSlideDrag = liquidTuning.m_fSlideDrag;
		m_Tune.m_fHorizSlideDragScale = liquidTuning.m_fHorizSlideDragScale; 
		m_Tune.m_fSurfaceNoiseRange = liquidTuning.m_fSurfaceNoiseRange;
		m_Tune.m_iSurfaceNoiseCount = liquidTuning.m_iSurfaceNoiseCount;
		m_Tune.m_bEnableSurfaceReaction = liquidTuning.m_bEnableSurfaceReaction;
		m_Tune.m_fMaxWaveHeight = liquidTuning.m_fMaxWaveHeight;
		m_Tune.m_fSplashScale = liquidTuning.m_fSplashScale;
		m_Tune.m_fMaxSplash = liquidTuning.m_fMaxSplash;
		m_Tune.m_fMaxLocalVelocity = liquidTuning.m_fMaxLocalVelocity;
		m_Tune.m_fConstantPushDrag = liquidTuning.m_fConstantPushDrag;
		m_Tune.m_fConstantSlideDrag = liquidTuning.m_fConstantSlideDrag;
		m_Tune.m_WaterDirection = liquidTuning.m_WaterDirection;
		m_Tune.m_fWaterVelocityScale = liquidTuning.m_fWaterVelocityScale;
		m_Tune.m_fPushDragRisingFactor = liquidTuning.m_fPushDragRisingFactor;
		m_Tune.m_fPushDragSinkingFactor = liquidTuning.m_fPushDragSinkingFactor;
	}

	void phInstBehaviorLiquid::Update(float UNUSED_PARAM(TimeStep))
	{
#if PERFORM_LIQUID_UPDATE
		if(m_Instance->GetArchetype()->GetBound()->GetType() == phBound::SURFACE)
		{
			// It's a surface bound so will check if it has a grid and, if so, we will simulate it.
			phBoundSurface &ThisBound = *(static_cast<phBoundSurface *>(m_Instance->GetArchetype()->GetBound()));
			phSurfaceGrid *SurfaceGrid = ThisBound.GetGridData();

			if(SurfaceGrid && SurfaceGrid->GetOffsetGrid())
			{
				// Constants that are the same for all simulated bodies of water.
				const float kc = m_Tune.m_fPropogationSpeed;							// Wave propagation speed in this liquid.
				const float maxWaveHeight = m_Tune.m_fMaxWaveHeight;
				const float kh = 2.0f;											// Spacing between adjacent grid points.
				const float kA = square(kc * (1.0f / 60.0f) * (1.0f / kh));
				const float kB = 2.0f - 4.0f * kA;

#if 1
				int IndexX, IndexZ;
				for(IndexX = 0; IndexX < phSurfaceGrid::kSurfaceGridLength; ++IndexX)
				{
					for(IndexZ = 0; IndexZ < phSurfaceGrid::kSurfaceGridLength; ++IndexZ)
					{
						int index = SurfaceGrid->LIQUID_FUNCTION(GetIndex)(IndexX, IndexZ);
						if(SurfaceGrid->LIQUID_FUNCTION(IsOffsetValidFast)(index))
						{
							float Total = 0.0f;
							Total = SurfaceGrid->LIQUID_FUNCTION(FindLocalElevation)(IndexX - 1, IndexZ - 1)
								+ SurfaceGrid->LIQUID_FUNCTION(FindLocalElevation)(IndexX + 1, IndexZ - 1)
								+ SurfaceGrid->LIQUID_FUNCTION(FindLocalElevation)(IndexX - 1, IndexZ + 1)
								+ SurfaceGrid->LIQUID_FUNCTION(FindLocalElevation)(IndexX + 1, IndexZ + 1);

							float elevation = kA * Total + kB * SurfaceGrid->LIQUID_FUNCTION(GetLocalElevation)(index) - SurfaceGrid->LIQUID_FUNCTION(GetNextLocalElevation)(index);
							//
							///////////////////////////////////////////////////////////////////////////////////////////////

							// Apply a little dampening.
							elevation *= m_Tune.m_fDampening;

							// Clamp the surface to make sure that it doesn't get out of control.
							elevation = Min(maxWaveHeight, elevation);
							elevation = Max(-maxWaveHeight, elevation);
							SurfaceGrid->LIQUID_FUNCTION(SetNextLocalElevation)(index, elevation);
						}
					}
				}
#if ENABLE_SURFACE_NOISE
				if(m_Tune.m_fSurfaceNoiseRange > 0.0f)
				{
					// This creates a little bit of noise on the surface of the liquid.
					for(int Temp = 0; Temp < m_Tune.m_iSurfaceNoiseCount; ++Temp)
					{
						IndexX = g_ReplayRand.GetRanged(0, 31);
						IndexZ = g_ReplayRand.GetRanged(0, 31);
						int index = SurfaceGrid->GetIndex(IndexX, IndexZ);
						if(SurfaceGrid->IsOffsetValidFast(index))
						{
							float elevation = SurfaceGrid->GetNextLocalElevation(index);
							elevation -= m_Tune.m_fSurfaceNoiseRange;
							SurfaceGrid->SetNextLocalElevation(index, elevation);
						}
					}
				}
#endif // ENABLE_SURFACE_NOISE
#else
				int IndexX, IndexZ;
				for(IndexX = 0; IndexX < 32; ++IndexX)
				{
					for(IndexZ = 0; IndexZ < 32; ++IndexZ)
					{
						///////////////////////////////////////////////////////////////////////////////////////////////
						// This is the regular water simulation from GPG1.
						float Total = 0.0f;
						Total += IndexX != 0 ? SurfaceGrid->m_CurrentGrid[(IndexZ) * 32 + (IndexX - 1)] : SurfaceGrid->m_NeighborGrids[0] == NULL ? 0.0f : SurfaceGrid->m_NeighborGrids[0]->m_CurrentGrid[(IndexZ) * 32 + 31];
						Total += IndexX != 31 ? SurfaceGrid->m_CurrentGrid[(IndexZ) * 32 + (IndexX + 1)] : SurfaceGrid->m_NeighborGrids[1] == NULL ? 0.0f : SurfaceGrid->m_NeighborGrids[1]->m_CurrentGrid[(IndexZ) * 32 + 0];
						Total += IndexZ != 0 ? SurfaceGrid->m_CurrentGrid[(IndexZ - 1) * 32 + (IndexX)] : SurfaceGrid->m_NeighborGrids[2] == NULL ? 0.0f : SurfaceGrid->m_NeighborGrids[2]->m_CurrentGrid[(31) * 32 + IndexX];//CurWaterGrid.m_CurrentGrid[(IndexZ + 1) * 32 + (IndexX)];
						Total += IndexZ != 31 ? SurfaceGrid->m_CurrentGrid[(IndexZ + 1) * 32 + (IndexX)] : SurfaceGrid->m_NeighborGrids[3] == NULL ? 0.0f : SurfaceGrid->m_NeighborGrids[3]->m_CurrentGrid[(0) * 32 + IndexX];//CurWaterGrid.m_CurrentGrid[(IndexZ - 1) * 32 + (IndexX)];
						SurfaceGrid->m_NextGrid[IndexZ * 32 + IndexX] = kA * Total + kB * SurfaceGrid->m_CurrentGrid[IndexZ * 32 + IndexX] - SurfaceGrid->m_NextGrid[IndexZ * 32 + IndexX];
						//
						///////////////////////////////////////////////////////////////////////////////////////////////

						// Apply a little dampening.
						SurfaceGrid->m_NextGrid[IndexZ * 32 + IndexX] *= m_Tune.m_fDampening;

						// Clamp the surface to make sure that it doesn't get out of control.
						SurfaceGrid->m_NextGrid[IndexZ * 32 + IndexX] = Min(maxWaveHeight, SurfaceGrid->m_NextGrid[IndexZ * 32 + IndexX]);
						SurfaceGrid->m_NextGrid[IndexZ * 32 + IndexX] = Max(-maxWaveHeight, SurfaceGrid->m_NextGrid[IndexZ * 32 + IndexX]);
					}
				}
#if ENABLE_SURFACE_NOISE
				if(m_Tune.m_fSurfaceNoiseRange > 0.0f)
				{
					// This creates a little bit of noise on the surface of the liquid.
					for(int Temp = 0; Temp < m_Tune.m_iSurfaceNoiseCount; ++Temp)
					{
						IndexX = g_ReplayRand.GetRanged(0, 31);
						IndexZ = g_ReplayRand.GetRanged(0, 31);
						SurfaceGrid->m_NextGrid[IndexZ * 32 + IndexX] -= m_Tune.m_fSurfaceNoiseRange;
					}
				}
#endif // ENABLE_SURFACE_NOISE
#endif

				SurfaceGrid->SwapGrids();
			}
		}
#endif // PERFORM_LIQUID_UPDATE
	}

	bool phInstBehaviorLiquid::FindLiquidImpacts(phLiquidImpactSet &ImpactSet)
	{
		// Right now, surfaces are the only kind of bound that supports liquid impacts.
		FastAssert(ImpactSet.m_BoundA->GetType() == phBound::SURFACE);
		phBoundSurface &ThisBound = *(static_cast<phBoundSurface *>(ImpactSet.m_BoundA));

		bool ImpactFound = false;
		phBound &OtherBound = *ImpactSet.m_BoundB;
		switch(OtherBound.GetType())
		{
			case phBound::SPHERE:
			{
				ImpactFound = ThisBound.FindLiquidImpactsToSphere(ImpactSet);
				break;
			}

			case phBound::CAPSULE:
			{
				ImpactFound = ThisBound.FindLiquidImpactsToCapsule(ImpactSet);
				break;
			}

			USE_GEOMETRY_CURVED_ONLY(case phBound::GEOMETRY_CURVED:)
			case phBound::DISC:
			case phBound::BOX:
			{
				ImpactFound = ThisBound.FindLiquidImpactsToBox(ImpactSet);
				break;
			}

			case phBound::GEOMETRY:
			{
				ImpactFound = ThisBound.FindLiquidImpactsToPoly(ImpactSet);
				break;
			}

			case phBound::COMPOSITE:
			{
				phBoundComposite &CompositeBound = *(static_cast<phBoundComposite *>(&OtherBound));
				Matrix34 RootMatrix(*ImpactSet.m_CurrentB);
				Matrix34 ChildMatrix;
				ImpactSet.m_CurrentB = &ChildMatrix;
				for(int ComponentIndex = 0; ComponentIndex < CompositeBound.GetNumBounds(); ++ComponentIndex)
				{
					phBound* bound = CompositeBound.GetBound(ComponentIndex);
					if (bound)
					{
						//u32 typeFlags = CompositeBound.GetTypeFlags(ComponentIndex);
						u32* typeAndIncludeFlags = CompositeBound.GetTypeAndIncludeFlags(); // At least some movers are composites, but return null typeAndInclude arrays
						if(typeAndIncludeFlags && !CanImpactWater(bound, typeAndIncludeFlags[ComponentIndex*2]))
						{
							continue;
						}

						ChildMatrix.Set(RCC_MATRIX34(CompositeBound.GetCurrentMatrix(ComponentIndex)));
						ChildMatrix.Dot(RootMatrix);
						ImpactSet.m_BoundB = bound;
						int StartLiquidImpactIndex = ImpactSet.m_NumImpacts;
						if(FindLiquidImpacts(ImpactSet))
						{
							ImpactFound = true;
							for(int LiquidImpactIndex = StartLiquidImpactIndex; LiquidImpactIndex < ImpactSet.m_NumImpacts; ++LiquidImpactIndex)
							{
								ImpactSet.m_LiquidImpactList[LiquidImpactIndex].m_Component = ComponentIndex;
							}
						}

					}
				}
				break;
			}

			default:
			{
				Assertf(false, "Unknown bound type %d for phInstBehaviorLiquid impact.", (int)OtherBound.GetType());
				break;
			}
		}

		return ImpactFound;
	}


#define MIN_BUOYANCY_SCALE 0.0001f
	void phInstBehaviorLiquid::ComputeAndApplyImpactResponse (const phLiquidImpactData& kImpactData, phInst* thisInst, phInst* otherInst, phCollider* otherCollider, Vec::V3Param128 timestep) const
	{
		LIQUID_ASSERT_LEGIT(kImpactData.m_SubmergedVolume);
		LIQUID_ASSERT_LEGIT(kImpactData.m_SubmergedVolume);
		LIQUID_ASSERT_LEGIT(kImpactData.m_ForcePos);

		const phInstBehaviorLiquidCollidingObjectTuning &collidingObjectTuning = GetCollidingObjectTuning(otherInst, otherCollider);
		float dragFactorScale = collidingObjectTuning.m_DragFactorScale;

		// Going to want the buoyancy factor later
		//   Also, modifying drag based on buoyancy because it generally correlates with porousness
		float instBuoyancy = m_Tune.m_fInstBuoyancy;
		if(otherInst->GetArchetype()->GetClassType() >= phArchetype::ARCHETYPE_PHYS)
		{
			phArchetypePhys* pPhys = static_cast<phArchetypePhys*>(otherInst->GetArchetype());
			instBuoyancy *= pPhys->GetBuoyancyFactor();

			// Force "untuned" buoyancies to something small
			//instBuoyancy = instBuoyancy == 1.0f ? 0.1f : instBuoyancy;

			dragFactorScale = Min(dragFactorScale, instBuoyancy * 10.0f);
		}

		// Bail if nothing is going to happen here -- Otherwise we keep objects awake with zero strength force applications
		if(!((collidingObjectTuning.m_EnableBuoyancy && instBuoyancy > MIN_BUOYANCY_SCALE) || collidingObjectTuning.m_EnablePushResistance || collidingObjectTuning.m_EnableSlidingFriction || collidingObjectTuning.m_EnableFlow))
		{
			return;
		}

		// The call to ShouldApplyBuoyancyToCollidingObject() here gives subclasses a chance to disable
		// the buoyancy force for certain instances. /FF
		Vector3 vecImpulse(Vector3::ZeroType);
		if(collidingObjectTuning.m_EnableBuoyancy)
		{
			// Find the force (on the non-liquid object) from buoyancy.  We will calculate all forces as if we are applying them to the non-liquid object.
			float gravityFactor = otherInst->GetArchetype()->GetGravityFactor();

			// TODO: This density could come from the liquid bound itself.
			vecImpulse.SetScaled(kImpactData.m_Normal, GRAVITY * instBuoyancy * gravityFactor * WATER_DENSITY * kImpactData.m_SubmergedVolume);
			LIQUID_ASSERT_LEGIT(vecImpulse);
		}
		else
		{
			vecImpulse.Zero();
		}

		Vector3 vecLocation(kImpactData.m_ForcePos);

		Vector3 vecLocalVel;
		if (otherCollider)
		{
#if OBSOLETE_PRE_VECTOR_LIBRARY
			otherCollider->GetLocalVelocity(vecLocation,vecLocalVel,kImpactData.m_Component);
#else
			vecLocalVel= VEC3V_TO_VECTOR3(otherCollider->GetLocalVelocity(VECTOR3_TO_INTRIN(vecLocation),kImpactData.m_Component));
#endif
		}
		else
		{
			vecLocalVel = VEC3V_TO_VECTOR3(otherInst->GetExternallyControlledLocalVelocity(vecLocation,kImpactData.m_Component));
		}
		LIQUID_ASSERT_LEGIT(vecLocalVel);

		//vecLocalVel -= m_Tune.m_WaterDirection; // Not sure what this water direction is, or its purpose here.  <--- it was for the debug widgets.
#define DEBUG_APPLY_WATER_VELOCITY 1
		phBoundSurface &ThisBound = *(static_cast<phBoundSurface *>(m_Instance->GetArchetype()->GetBound()));
		phSurfaceGrid *SurfaceGrid = ThisBound.GetGridData();
		int GridIndexX = -1;
		int GridIndexZ = -1;
		if(collidingObjectTuning.m_EnableFlow && SurfaceGrid != NULL)
		{
			const phSurfaceGrid::GridData<Vector2>* pVelocityGrid = SurfaceGrid->GetVelocityGrid();
			if(pVelocityGrid)
			{
#if OBSOLETE_PRE_VECTOR_LIBRARY
				const Vector3& thisPos = thisInst->GetPosition();
#else
				const Vector3 thisPos = RCC_VECTOR3(thisInst->GetPosition());
#endif

				// Find the grid vertex closest to the impact point.
				GridIndexX = static_cast<int>(0.5f * (vecLocation.x - thisPos.x + 32.0f));
				GridIndexZ = static_cast<int>(0.5f * (vecLocation.z - thisPos.z + 32.0f));
				if(GridIndexX >= 0 && GridIndexX < 32 && GridIndexZ >= 0 && GridIndexZ < 32)
				{
					// extract the velocity from the grid 
					Vector2 waterVelocity = SurfaceGrid->GetVelocity(GridIndexX, GridIndexZ);
					waterVelocity.Scale(m_Tune.m_fWaterVelocityScale);

					//FastAssert(FPIsFinite(waterVelocity.x) && FPIsFinite(waterVelocity.y));
					bool validWaterSpeed = FPIsFinite(waterVelocity.x) && FPIsFinite(waterVelocity.y) && (fabsf(waterVelocity.x) < 100.0f) && (fabsf(waterVelocity.y) < 100.0f);
					if(Verifyf(validWaterSpeed, "Bad water velocity"))
					{
						vecLocalVel.Subtract(waterVelocity.x, 0.0f, waterVelocity.y);
					}
				}
			}
		}
		FastAssert(vecLocalVel.FiniteElements());

		if(vecLocalVel.Mag2() > square(m_Tune.m_fMaxLocalVelocity))
		{
			vecLocalVel.Normalize();
			vecLocalVel.Scale(m_Tune.m_fMaxLocalVelocity);
			FastAssert(vecLocalVel.FiniteElements());
		}


		// determine if the other inst center is within this bound's box. Drag can only be applied once, so it
		// is applied by the surface that contains the centerpoint of the other object.

#if OBSOLETE_PRE_VECTOR_LIBRARY
		// non-vectorized relative pos calculation.
		Vector3 relativePos(thisInst->GetMatrix().d - otherInst->GetMatrix().d);
#else
		// vectorized relative pos calculation.
		const Mat34V thisMatrix = thisInst->GetMatrix();
		const Mat34V otherMatrix = otherInst->GetMatrix();
		Vector3 relativePos(reinterpret_cast<const Matrix34*>(&thisMatrix)->d - reinterpret_cast<const Matrix34*>(&otherMatrix)->d);
#endif

		Vector3 thisSize = VEC3V_TO_VECTOR3(thisInst->GetArchetype()->GetBound()->GetBoundingBoxSize());
		bool applyDrag = fabsf(relativePos.x * 2.0f) < thisSize.x && fabsf(relativePos.z * 2.0f) < thisSize.z;
		
		Vector3 dragImpulse;
		// TODO: This density could come from the bound itself.
		float dragFactor = WATER_DENSITY * kImpactData.m_SubmergedArea;
		dragFactor *= dragFactorScale;

		//	const float kfHorizSlideDragScale = 8.0f * 0.006f;
		// Find the impulse from forward motion through the water.


		float normDotVel;
		Vector3 impactNormal;
		if(kImpactData.m_UseBoundDirectionForDrag)
		{
			// get (relative.. but the phBoundSurface is stationary) velocity from the bound and normalize it.
			impactNormal.Normalize(RCC_VECTOR3(otherCollider->GetVelocity()));
		}
		else
		{
			impactNormal = kImpactData.m_Normal; //impactNormal.Zero(); impactNormal.SetY(1.0f);
		}
		LIQUID_ASSERT_LEGIT(impactNormal);
		normDotVel = impactNormal.Dot(vecLocalVel);

		if(applyDrag)
		{
			if(collidingObjectTuning.m_EnablePushResistance)
			{
				const float kfPushDrag = m_Tune.m_fPushDrag;// -1.0f;
				const float kfHorizPushDragScale = m_Tune.m_fHorizPushDragScale; //.006f;

				if (normDotVel > 0.0f)
				{
					//float pushDragMag = kfPushDrag * dragFactor * square(normDotVel);
					//			float pushDragMag = kfPushDrag * dragFactor * fabs(normDotVel) * normDotVel;
					const float kfConstantPushDrag = m_Tune.m_fConstantPushDrag;
					float pushDragMag = kfConstantPushDrag + kfPushDrag * normDotVel * (normDotVel > 0.0f ? m_Tune.m_fPushDragRisingFactor : m_Tune.m_fPushDragSinkingFactor);
					pushDragMag *= dragFactor;

					//		float pushDragMag = kfPushDrag * dragFactor * normDotVel;

					float horizontalDrag = pushDragMag * kfHorizPushDragScale;
					dragImpulse.Set(impactNormal.x * horizontalDrag, impactNormal.y * pushDragMag, impactNormal.z * horizontalDrag);

					// Modulate the drag impulse based on damping values, if available.
					phArchetype* pArch = otherInst->GetArchetype();
					static bool disablePush = true;
					if(!disablePush	&& (pArch->GetClassType() == phArchetype::ARCHETYPE_DAMP))
					{
						phArchetypeDamp* pDamp = static_cast<phArchetypeDamp*>(pArch);
						Vector3 worldDamping(pDamp->GetDampingConstant(phArchetypeDamp::LINEAR_V));
#if OBSOLETE_PRE_VECTOR_LIBRARY
						otherInst->GetMatrix().Transform3x3(worldDamping);
#else
						reinterpret_cast<const Matrix34*>(&otherMatrix)->Transform3x3(worldDamping);
#endif
						dragImpulse.Multiply(worldDamping);

						// Scale the result by 50 because this is the reciprocal of the default damping value set by fragtype - this really should be 
						//   rolled into m_Tune.m_fPushDrag and it should this section should use default dampening values if no damped archetype
						//   is found.
						dragImpulse.Scale(50.0f);
					}

					LIQUID_ASSERT_LEGIT(dragImpulse);
					vecImpulse.Add(dragImpulse);
				}
			}

			if(collidingObjectTuning.m_EnableSlidingFriction)
			{
				const float kfSlideDrag = m_Tune.m_fSlideDrag;
				const float kfHorizSlideDragScale = m_Tune.m_fHorizSlideDragScale;

				// Find the impulse from sliding friction in water.
				vecLocalVel.SubtractScaled(impactNormal, normDotVel);
				LIQUID_ASSERT_LEGIT(vecLocalVel);

				float perpVelMag2 = vecLocalVel.Mag2();
				if(perpVelMag2>SMALL_FLOAT)
				{
					// Add the impulse along the local velocity direction because the impulse applies on object A in the impact
					// list (the water), and the negated impulse applies on the moving object.
					const float kfConstantSlideDrag = m_Tune.m_fConstantSlideDrag;
					float slideDragMag = kfConstantSlideDrag + kfSlideDrag * sqrtf(perpVelMag2);
					slideDragMag *= dragFactor;
					float horizontalDrag = slideDragMag * kfHorizSlideDragScale;
					dragImpulse.Set(vecLocalVel.x * horizontalDrag, vecLocalVel.y * slideDragMag, vecLocalVel.z * horizontalDrag);
					LIQUID_ASSERT_LEGIT(dragImpulse);
					vecImpulse.Add(dragImpulse);
				}
			}
		}

		if (otherCollider)
		{
			otherCollider->ApplyForce(vecImpulse,vecLocation,timestep,kImpactData.m_Component);
		}


#if DEBUG_APPLY_WATER_VELOCITY
#if ENABLE_SURFACE_REACTION
		if (m_Tune.m_bEnableSurfaceReaction && collidingObjectTuning.m_EnableSurfaceReaction)
		{
			// Make the water surface react to the collision.
			FastAssert(m_Instance->GetArchetype()->GetBound()->GetType() == phBound::SURFACE);
			if(SurfaceGrid != NULL && normDotVel > 1.5f && vecImpulse.y > 0.0f)
			{
				if(GridIndexX < 32 && GridIndexZ < 32 && GridIndexX >= 0 && GridIndexZ >= 0)
				{
					float NewLiquidDelta = Min(vecImpulse.Mag() * 2.0f / WATER_DENSITY * m_Tune.m_fSplashScale, m_Tune.m_fMaxSplash);
					SurfaceGrid->ModifyLocalElevation(GridIndexX, GridIndexZ, -NewLiquidDelta);
				}
			}
		}
#endif

#else
#if ENABLE_SURFACE_REACTION
		if (m_Tune.m_bEnableSurfaceReaction && collidingObjectTuning.m_EnableSurfaceReaction)
		{
			// Make the water surface react to the collision.
			FastAssert(m_Instance->GetArchetype()->GetBound()->GetType() == phBound::SURFACE);
			phBoundSurface &ThisBound = *(static_cast<phBoundSurface *>(m_Instance->GetArchetype()->GetBound()));
			phSurfaceGrid *SurfaceGrid = ThisBound.GetGridData();
			if(SurfaceGrid != NULL && normDotVel > 1.5f && vecImpulse.y > 0.0f)
			{
#if OBSOLETE_PRE_VECTOR_LIBRARY
				vecLocation.Subtract(thisInst->GetPosition());
#else
				vecLocation.Subtract(RCC_VECTOR3(thisInst->GetPosition()));
#endif

				// Find the grid vertex closest to the impact point.
				int GridIndexX = static_cast<int>(0.5f * vecLocation.x + 16.0f + 0.5f);
				//FastAssert(GridIndexX >= 0);// && GridIndexX < 32);
				int GridIndexZ = static_cast<int>(0.5f * vecLocation.z + 16.0f + 0.5f);
				//FastAssert(GridIndexZ >= 0);// && GridIndexZ < 32);

				if(GridIndexX < 32 && GridIndexZ < 32 && GridIndexX >= 0 && GridIndexZ >= 0)
				{
#if 0
					if(m_pWaterGrid->m_CurrentGrid[GridIndexZ * 32 + GridIndexX] > 0.75f * vecLocation.y)
					{
						m_pWaterGrid->m_CurrentGrid[GridIndexZ * 32 + GridIndexX] = 0.75f * vecLocation.y;
					}
#elif 1
					//				float NewLiquidDelta = Min(vecImpulse.Mag() * 2.0f / WATER_DENSITY * (1.0f / 60.0f) * 0.08f, 0.07f);
					float NewLiquidDelta = Min(vecImpulse.Mag() * 2.0f / WATER_DENSITY * m_Tune.m_fSplashScale, m_Tune.m_fMaxSplash);
					SurfaceGrid->ModifyLocalElevation(GridIndexX, GridIndexZ, -NewLiquidDelta);
					//SurfaceGrid->m_CurrentGrid[GridIndexZ * 32 + GridIndexX] -= NewLiquidDelta;
#elif 0
					float LiquidDelta = Min(vecImpulse.y * 2.0f / WATER_DENSITY * (1.0f / 60.0f) * 0.08f, 0.08f);
					SurfaceGrid->m_CurrentGrid[GridIndexZ * 32 + GridIndexX] += LiquidDelta;
#endif
				}
			}
		}
#endif
#endif
	}

	bool phInstBehaviorLiquid::CollideObjects(Vec::V3Param128 timeStep, phInst* myInst, phCollider* UNUSED_PARAM(myCollider), phInst* otherInst, phCollider* otherCollider, phInstBehavior* UNUSED_PARAM(otherInstBehavior))
	{
		if(!otherCollider)
		{ 
			// No collider!?? Return true to let the sim take care of this. It should never have reached here without a collider. -ecosky.
			// We might get here due to an inactive instance that's been declared as a 'moving' instance.  One example of when this would occur is
			//   with a rope instance.
			// Returning false here would probably be better as the simulator really isn't going to know what to do with a surface bound.
			return true;
		}

		// Find impacts between the given other object and this liquid.
		phLiquidImpactSet impactSet;
		impactSet.m_BoundA = myInst->GetArchetype()->GetBound();				// The bound that is made into a liquid.
		impactSet.m_BoundB = otherInst->GetArchetype()->GetBound();				// The non-liquid bound.
#if OBSOLETE_PRE_VECTOR_LIBRARY
		impactSet.m_CurrentA = &myInst->GetMatrix();
		impactSet.m_CurrentB = &otherInst->GetMatrix();
#else
		impactSet.m_CurrentA = &RCC_MATRIX34(myInst->GetMatrix());
		impactSet.m_CurrentB = &RCC_MATRIX34(otherInst->GetMatrix());
#endif
		if (FindLiquidImpacts(impactSet))
		{
			ModifyImpactsNotifyParent(impactSet,myInst,otherInst,otherCollider);

			// Going to find the "best" impact to toss to fx processing (Currently for splashes)
			float largestSubmergedVolume = -1.0f;
			int bestImpactComponent = -1;

			bool allCompletelySubmerged = true;
			int numImpacts = impactSet.m_NumImpacts;
			for (int impactIndex=0; impactIndex<numImpacts; impactIndex++)
			{
				ComputeAndApplyImpactResponse(impactSet.m_LiquidImpactList[impactIndex],myInst,otherInst,otherCollider,timeStep);

				// Don't accept if totally submerged
				if(!impactSet.m_LiquidImpactList[impactIndex].m_CompletelySubmerged)
				{
					if(impactSet.m_LiquidImpactList[impactIndex].m_SubmergedVolume > largestSubmergedVolume)
					{
						largestSubmergedVolume = impactSet.m_LiquidImpactList[impactIndex].m_SubmergedVolume;
						bestImpactComponent = impactIndex;
					}
				}
				allCompletelySubmerged &= impactSet.m_LiquidImpactList[impactIndex].m_CompletelySubmerged;
			}

			// HACK to mess with the sleep thresholds of objects in water
			//  - On the surface we want relatively low thresholds so buoyant objects don't go to sleep in front of us
			//  - But underneath it is very hard to see what is happening so we really jsut want stuff to go to sleep and stop wasting our time
			float waterVelTol2;
			float waterAngVelTol2;
			float waterInternalMotionTolerance2;
			// If not completely submerged but we have any interaction with the liquid we must be somewhere on the border, (possibly)floating
			if(!allCompletelySubmerged)
			{
				static float onWaterVelTol2 = 0.007f;
				static float onWaterAngVelTol2 = 0.004f;
				static float onWaterInternalMotionTolerance2 = 0.34f;
				waterVelTol2 = onWaterVelTol2;
				waterAngVelTol2 = onWaterAngVelTol2;
				waterInternalMotionTolerance2 = onWaterInternalMotionTolerance2;
			}
			else
			{
				// We're fully submerged - So bump our sleep thresholds up a bunch so we can come to rest
				static float inWaterVelTol2 = 0.1f;
				static float inWaterAngVelTol2 = 0.08f;
				static float inWaterInternalMotionTolerance2 = 0.4f;
				waterVelTol2 = inWaterVelTol2;
				waterAngVelTol2 = inWaterAngVelTol2;
				waterInternalMotionTolerance2 = inWaterInternalMotionTolerance2;
			}
			// Actually set (potentially) new sleep thresholds
			phSleep* pOtherSleep = otherCollider ? otherCollider->GetSleep() : NULL;
			if(pOtherSleep)
			{
				pOtherSleep->SetVelTolerance2(waterVelTol2);
				pOtherSleep->SetAngVelTolerance2(waterAngVelTol2);
				pOtherSleep->SetInternalMotionTolerance2Sum(waterInternalMotionTolerance2);
			}

			// Maybe give fx a chance to do something with the impact
			if(bestImpactComponent != -1)
			{
				NotifyCollidingObjectImpacts(impactSet.m_LiquidImpactList[bestImpactComponent],myInst,otherInst,otherCollider);
			}

			// Let subclasses know that the instance is at least partially submerged. /FF
			NotifyCollidingObject(otherInst, otherCollider);
		}


		// Return false to indicate that the liquid handled the collision, so the simulator should not.
		return false;
	}


	void phInstBehaviorLiquid::NotifyCollidingObject(phInst* /*otherInst*/, phCollider* /*otherCollider*/) const
	{
	}

	void phInstBehaviorLiquid::NotifyCollidingObjectImpacts(const phLiquidImpactData& /*impactData*/, phInst* /*thisInst*/, phInst* /*otherInst*/, phCollider* /*otherCollider*/) const
	{
	}

	bool phInstBehaviorLiquid::CanImpactWater(phBound* /*otherBound*/, u32 /*typeFlags*/) const
	{
		return true;
	}

	const phInstBehaviorLiquidCollidingObjectTuning &phInstBehaviorLiquid::GetCollidingObjectTuning(const phInst* /*otherInst*/, const phCollider* /*otherCollider*/) const
	{
		return sm_DefaultCollidingObjectTuning;
	}

	bool phInstBehaviorLiquid::ActivateWhenHit() const
	{
		return false;
	}

#endif // USE_SURFACES

} // namespace rage
