
//Game headers
#include "ClothRageInterface.h"
#include "Cloth.h"
#include "ClothMgr.h"
#include "ClothUpdater.h"
#include "ClothCapsuleBounds.h"
#include "physics/gtaArchetype.h"
#include "Peds/ped.h"
#include "physics/gtaInst.h"
#include "physics/physics.h"

//Rage headers
#include "physics/archetype.h"
#include "phbound/bound.h"
#include "phbound/boundcapsule.h"
#include "phbound/boundcomposite.h"
#include "physics/shapetest.h"
#include "physics/simulator.h"

#if NORTH_CLOTHS

CLOTH_OPTIMISATIONS()

FW_INSTANTIATE_BASECLASS_POOL /*_DEALWITHNOMEMORY*/(phInstCloth, CClothMgr::MAX_NUM_CLOTHS, "phInstCloth", sizeof(phInstCloth));
FW_INSTANTIATE_BASECLASS_POOL /*_DEALWITHNOMEMORY*/(phInstBehaviourCloth, CClothMgr::MAX_NUM_CLOTHS, "phInstBehaviourCloth", sizeof(phInstBehaviourCloth));

void phInstBehaviourCloth::SetStartTransform(const Matrix34& startTransform)
{
	//Transform the cloth particles and pendants etc to a start position.
	m_pCloth->Transform(startTransform);

	//Update the composite bound twice to make sure the current matrices equal the last matrices.
	UpdateCompositeBound();
	UpdateCompositeBound();

	//Update the matrices that will be used to pose the skeleton.
	const CClothArchetype* pClothArchetype = GetCloth()->GetArchetype();
	GetCloth()->UpdateSkeletonMatrices(*pClothArchetype);

	//Set the cloth back to sleep.
	GetCloth()->SetToSleep();
	GetCloth()->SetIsJustWokenUp(false);
	}

phBoundComposite* phInstBehaviourCloth::CreateBound() const
{
	//Make a const cloth ptr so we don't change the cloth here.
	//Use the const cloth ptr for the remainder of this function for safety.
	const CCloth* pCloth=const_cast<const CCloth*>(m_pCloth);

	//Get the number of capsules on the cloth.
	const int iNumCapsuleBounds=pCloth->GetCapsuleBounds().GetNumCapsuleBounds();

	//Construct and initialise the composite bound with the number of components equal to the number of cloth edges.
	phBoundComposite* pCompositeBound = rage_new phBoundComposite;
	pCompositeBound->Init(iNumCapsuleBounds, true);
	pCompositeBound->SetNumBounds(iNumCapsuleBounds);

	//Create a capsule bound for each edge.
	for(int i=0;i<iNumCapsuleBounds;i++)
	{
		Vector3 vA,vB;
		float fLength,fRadius;
		pCloth->GetCapsuleBounds().ComputeCapsule(i,pCloth->GetParticles(),vA,vB,fLength,fRadius);
		//The rotation of the bound can be constructed easily from a quaternion 
		//(zero degrees rotated around the vector vB->vA).
		Vector3 vDir=vB-vA;
		vDir.Normalize();
		Matrix34 partMatrix(M34_IDENTITY);
		partMatrix.RotateTo(partMatrix.b,vDir);
		Vector3 vBoundCentre=(vA+vB)*0.5f;
		partMatrix.Translate(vBoundCentre);

		//We're going to use a capsule bound for each edge.
		phBoundCapsule* pCapsuleBound = rage_new phBoundCapsule;
		pCapsuleBound->SetCapsuleSize(fRadius,fLength);
		//Add the capsule bound to the composite bound.
		pCompositeBound->SetBound(i,pCapsuleBound);
		pCompositeBound->SetCurrentMatrix(i,RCC_MAT34V(partMatrix));
		pCompositeBound->SetLastMatrix(i,RCC_MAT34V(partMatrix));
	}

	//Calculate the extents of the composite bound.
	bool onlyAdjustForInternalMotion = false;
	pCompositeBound->CalculateCompositeExtents(onlyAdjustForInternalMotion);

	//That's us finished.
	return pCompositeBound;
}

void phInstBehaviourCloth::PreUpdate (float UNUSED_PARAM(TimeStep))
{
	//Reset the number of objects that are colliding with the cloth.
	//We're going to record a list of them during the phSimulator::Update.
	m_iNumCollisionInstances=0;
}

void phInstBehaviourCloth::Update(float dt)
{
	//Do nothing if the cloth is asleep.
	if(m_pCloth->IsAsleep())
	{
		return;
	}

	dt=1.0f/60.0f;

	//x=x+v*dt+a*dt*dt except for particles attached to pendants that take the pendant position instead.
	//Emtpy the list of contacts.
	//Collision detect against world and force particles inside the world to touch the world surface after 
	//accounting for contact friction modelled with a bit of extra damping.
	//Handle constraints with the latest particle positions.
	//Free the list of contacts just for safety.

	//If the cloth has just woken up we might set it to sleep if it isn't involved in any contacts.
	//Store the cloth particle positions just in case we're going to set it back to sleep.
	Vector3 avParticlePositions[MAX_NUM_CLOTH_PARTICLES];
	if(m_pCloth->GetIsJustWokenUp())
	{
		const CClothParticles& particles=m_pCloth->GetParticles();
		int i;
		const int iNumParticles=particles.GetNumParticles();
		for(i=0;i<iNumParticles;i++)
		{
			avParticlePositions[i]=particles.GetPosition(i);
		}
	}

	//Update ballistic phase and apply springs.
#if !__BANK
	if(true)
#else
	if(CClothMgr::ms_bVectoriseClothMaths)
#endif
	{
		m_pClothUpdater->UpdateBallisticPhaseV(*m_pCloth,dt);
		m_pClothUpdater->UpdateSpringsV(*m_pCloth,dt);
	}
	else
	{
		m_pClothUpdater->UpdateBallisticPhase(*m_pCloth,dt);
		m_pClothUpdater->UpdateSprings(*m_pCloth,dt);
	}

	//Compute all the contacts.
	m_pClothUpdater->GetContacts().FreeContacts();
	GenerateContacts(m_pClothUpdater->GetContacts());
	//bool bFoundContacts=(m_pClothUpdater->GetContacts().GetNumContacts()>0);

	//If the cloth has just woken up and has experienced no contacts then set it back to sleep
	//and set the cloth back to the positions before the ballistic update.
	//If the cloth has just woken and did experience a contact then make sure to reset the justwokenup flag.
	if(m_pCloth->GetIsJustWokenUp() && 0==m_pClothUpdater->GetContacts().GetNumContacts())
	{
		m_pCloth->SetToSleep();
		m_pCloth->SetIsJustWokenUp(false);
		Assert(!m_pCloth->GetIsJustWokenUp());
		CClothParticles& particles=m_pCloth->GetParticles();
		int i;
		const int iNumParticles=particles.GetNumParticles();
		for(i=0;i<iNumParticles;i++)
		{
			particles.SetPosition(i,avParticlePositions[i]);
			particles.SetPositionPrev(i,avParticlePositions[i]);
		}
	}
	else if(m_pCloth->GetIsJustWokenUp())
	{
		m_pCloth->SetIsJustWokenUp(false);
	}

	//If the cloth is awake then handle constraints and contacts.
	if(!m_pCloth->IsAsleep())
	{
#if !__BANK
		if(true)
#else
		if(CClothMgr::ms_bVectoriseClothMaths)
#endif
		{
			m_pClothUpdater->HandleConstraintsAndContactsV(*m_pCloth,dt);
		}
		else
		{
			m_pClothUpdater->HandleConstraintsAndContacts(*m_pCloth,dt);
		}
	}
	
	//Free the contacts (don't really need to do this but do it all the same for safety).
	m_pClothUpdater->GetContacts().FreeContacts();

	/*
	//Now iterate through a few times.
	if(!m_pCloth->IsAsleep())
	{
		int iCount=0;
		while(iCount<5 && bFoundContacts)	
		{	
			iCount++;

			//Compute all the contacts.
			m_pClothUpdater->GetContacts().FreeContacts();
			GenerateContacts(m_pClothUpdater->GetContacts());
			bFoundContacts=(m_pClothUpdater->GetContacts().GetNumContacts()>0);
	#if !__BANK
			if(true)
	#else
			if(CClothMgr::ms_bVectoriseClothMaths)
	#endif
			{
				m_pClothUpdater->HandleConstraintsAndContactsV(*m_pCloth,dt);
			}
			else
			{
				m_pClothUpdater->HandleConstraintsAndContacts(*m_pCloth,dt);
			}

			//Free the contacts (don't really need to do this but do it all the same for safety).
			m_pClothUpdater->GetContacts().FreeContacts();
		}
	}
	*/


	

	//Update the composite bound of the archetype for collision detection in 
	//the next timestep.
	UpdateCompositeBound();

	// The bound radius has probably changed
	PHLEVEL->UpdateObjectLocationAndRadius(GetInstance()->GetLevelIndex(),(const Matrix34*) NULL);

	//Update the matrices that will be used to pose the skeleton
	const CClothArchetype* pClothArchetype= m_pCloth->GetArchetype();
	m_pCloth->UpdateSkeletonMatrices(*pClothArchetype);

	//We need to work out if the cloth has moved.
	//If it has moved then reset the not_moving count back to zero.
	//If it hasn't moved then increment the count.
	bool bIsMoving=m_pClothUpdater->TestIsMoving(*m_pCloth,dt);
	if(bIsMoving)
	{
		m_pCloth->ResetNotMovingTime();
	}
	else
	{
		m_pCloth->IncrementNotMovingTime(dt);
	}
	//Test the number of updates the cloth hasn't moved.
	//If its quite a few updates since the cloth moved then set it to sleep.
	if(m_pCloth->GetNotMovingTime()>1.0f)
	{
		if(PHLEVEL->IsActive(GetInstance()->GetLevelIndex()))
		{
			PHSIM->DeactivateObject(GetInstance()->GetLevelIndex());
		}		
		m_pCloth->SetToSleep();
	}

	/*
	//Inspect constraint lengths.
	int i;
	float fMaxDiff=0;
	for(i=0;i<m_pCloth->GetConstraints().GetNumConstraints();i++)
	{
		const int iA=m_pCloth->GetConstraints().GetParticleIndexA(i);
		const int iB=m_pCloth->GetConstraints().GetParticleIndexB(i);
		const float fRestLength=m_pCloth->GetConstraints().GetRestLength(i);
		const Vector3& vA=m_pCloth->GetParticles().GetPosition(iA);
		const Vector3& vB=m_pCloth->GetParticles().GetPosition(iB);
		const Vector3 vDiff=vA-vB;
		const float fLength=vDiff.Mag();
		float fDiff=fabsf(fLength-fRestLength);
		fMaxDiff=(fMaxDiff>fDiff) ? fMaxDiff : fDiff;
	}
	fMaxDiff=fMaxDiff;
	*/
}

void phInstBehaviourCloth::IntersectSpheres(const phInst* pOtherInst, const CCloth& cloth, CClothContacts& contacts)
{
	//We'll need these arrays for shapetests.
	phPolygon::Index* paiCulledPolyArray=m_pClothUpdater->GetCulledPolysArrayPtr();
	const u16 iCulledPolyArraySize=m_pClothUpdater->GetCulledPolysArraySize();

	//We'll definitely need the cloth particles.
	const CClothParticles& particles=cloth.GetParticles();

	//Is the other inst the player?
	//If other inst is player then get the ragdoll inst.
	//Cos we'll want to intersect with the ragdoll.
	const phInst* pTestInst=pOtherInst;
	Assert(pOtherInst->GetUserData());
	const CEntity* pEntity=static_cast<const CEntity*>(pOtherInst->GetUserData());
	if(pEntity->GetIsTypePed())
	{
		const CPed* pPed=static_cast<const CPed*>(pEntity);
		pTestInst=pPed->GetRagdollInst();
	}

	//const float fTestRadius=particles.GetRadius(0);
	//const float fCollisionRadius=fTestRadius; 

	//Iterate over all the cloth particles and test them against the ith collision instance.
	for(int j=0;j<particles.GetNumParticles();j++)
	{
		const float fInverseMass=particles.GetInverseMass(j);
		if(0!=fInverseMass)
		{
			const Vector3& vParticlePos=particles.GetPosition(j);
			const float fRadius=particles.GetRadius(j);

			// Make local arrays to so that the bound culler in the shape test doesn't allocate them.
			phShapeTest<phShapeSphere> sphereTester(paiCulledPolyArray,iCulledPolyArraySize);
			phIntersection intersection;
			//Initialise the capsule.  Use BAD_INDEX so that we always get only the deepest intersection.
			sphereTester.InitSphere(vParticlePos, fRadius, &intersection, BAD_INDEX);

			//Test the capsule against the other instance.
			const bool bResult=sphereTester.TestOneObject(*pTestInst);
			//If there's been a contact then store it.
			if(bResult)
			{
				Vector3 vPos=vParticlePos+RCC_VECTOR3(intersection.GetNormal())*intersection.GetDepth();
				contacts.AddContact(j,vPos,RCC_VECTOR3(intersection.GetNormal()));
#if __BANK
#if DEBUG_DRAW
					if(CClothMgr::sm_bRenderDebugContacts)
					{
						Color32 purple(1.0f,1.0f,0.0f);
					grcDebugDraw::Line(vParticlePos,vParticlePos+RCC_VECTOR3(intersection.GetNormal()),purple);
					}
#endif
#endif
				}
			}
		}
	}

void phInstBehaviourCloth::IntersectCapsules(const phInst* pOtherInst, const CCloth& cloth, CClothContacts& contacts)
{
	//We'll need these arrays for shapetests.
	phPolygon::Index* paiCulledPolyArray=m_pClothUpdater->GetCulledPolysArrayPtr();
	const u16 iCulledPolyArraySize=m_pClothUpdater->GetCulledPolysArraySize();

	//Need constraints and particles to construct capsules.
	const CClothSeparationConstraints& constraints=cloth.GetSeparationConstraints();
	const CClothParticles& particles=cloth.GetParticles();

	//Is the other inst the player?
	//If other inst is player then get the ragdoll inst.
	//Cos we'll want to intersect with the ragdoll.
	const phInst* pTestInst=pOtherInst;
	Assert(pOtherInst->GetUserData());
	const CEntity* pEntity=static_cast<const CEntity*>(pOtherInst->GetUserData());
	if(pEntity->GetIsTypePed())
	{
		const CPed* pPed=static_cast<const CPed*>(pEntity);
		pTestInst=pPed->GetRagdollInst();
	}

	const float fRadius=0.25f*particles.GetRadius(0);

	//Iterate over all the cloth particles and test them against the ith collision instance.
	for(int j=0;j<constraints.GetNumConstraints();j++)
	{
		//Get the two particle positions that will form the capsule.
		const int iA=constraints.GetParticleIndexA(j);
		const int iB=constraints.GetParticleIndexB(j);
		const Vector3& vA=particles.GetPosition(iA);
		const Vector3& vB=particles.GetPosition(iB);
		const float fInverseMassA=particles.GetInverseMass(iA);
		const float fInverseMassB=particles.GetInverseMass(iB);
		if((fInverseMassA+fInverseMassB) != 0)
		{
			//Start the shape test.
			phShapeTest<phShapeCapsule> capsuleTester(paiCulledPolyArray,iCulledPolyArraySize);
			//Make a segment from A->B.
			phSegment segment;
			segment.Set(vA,vB);

			phIntersection intersection[4];
			//Initialise the capsule.  Use BAD_INDEX so that we always get only the deepest intersection.
			capsuleTester.InitCapsule(segment, fRadius, &intersection[0], 4);

			//Test the capsule against the other instance.
			const int iNumIntersections=capsuleTester.TestOneObject(*pTestInst);
			//If there's been a contact then store it.
			if(iNumIntersections!=0)
			{
				int iSmallestDepth=-1;
				float fSmallestDepth=FLT_MAX;
				int i;
				for(i=0;i<iNumIntersections;i++)
				{
					if(intersection[i].GetDepth()<fSmallestDepth)
					{
						iSmallestDepth=i;
						fSmallestDepth=intersection[i].GetDepth();
					}
				}
				Assert(iSmallestDepth!=-1);

				//Get the shallowest contact.
				if(fInverseMassB>0)
				{
					contacts.AddContact(iB,RCC_VECTOR3(intersection[iSmallestDepth].GetPosition()),RCC_VECTOR3(intersection[iSmallestDepth].GetNormal()));

#if __BANK
#if DEBUG_DRAW
					if(CClothMgr::sm_bRenderDebugContacts)
					{
						Color32 purple(1.0f,1.0f,0.0f);
						grcDebugDraw::Line(vB,vB+RCC_VECTOR3(intersection[iSmallestDepth].GetNormal()),purple);
					}
#endif
#endif
				}
			}
		}
	}
}


#define SPHERE_INTERSECTION 1
#define CAPSULE_INTERSECTION 0

void phInstBehaviourCloth::GenerateContacts(CClothContacts& contacts)
{
	const CCloth* pCloth=const_cast<CCloth*>(m_pCloth);

	//Identify the map and non-map collision entries in the list of contacting instances.
	phInst* apMapCollisionInstances[MAX_NUM_COLLISION_INSTANCES];
	int iNumMapInstances=0;
	phInst* apNonMapCollisionInstances[MAX_NUM_COLLISION_INSTANCES];
	int iNumNonMapInstances=0;
	for(int i=0;i<m_iNumCollisionInstances;i++)
	{
		phInst* pOtherInst=m_apCollisionInstances[i];
		Assertf(pOtherInst, "Null inst ptr");
		Assertf(pOtherInst->GetArchetype(), "Null archetype ptr");
		Assertf(pOtherInst->GetArchetype()->GetBound(), "Null bound ptr");

		if(pOtherInst->GetArchetype()->GetTypeFlag(ArchetypeFlags::GTA_ALL_MAP_TYPES))
		{
			apMapCollisionInstances[iNumMapInstances]=pOtherInst;
			iNumMapInstances++;
		}
		else
		{
			apNonMapCollisionInstances[iNumNonMapInstances]=pOtherInst;
			iNumNonMapInstances++;
		}
	}

	//Iterate over all non-map collision instances that might have hit the cloth.
	for(int i=0;i<iNumNonMapInstances;i++)
	{
		phInst* pOtherInst=apNonMapCollisionInstances[i];
		Assertf(pOtherInst, "Null inst ptr");
		Assertf(pOtherInst->GetArchetype(), "Null archetype ptr");
		Assertf(pOtherInst->GetArchetype()->GetBound(), "Null bound ptr");
#if SPHERE_INTERSECTION
		IntersectSpheres(pOtherInst,*pCloth,contacts);
#elif CAPSULE_INTERSECTION
		IntersectCapsules(pOtherInst,*pCloth,contacts);
#endif
	}

	//If cloth has just been woken up then only bother testing for intersection against the map if there are contacts
	//with moveable instances.
	bool bDoMapCollision=true;
	if(pCloth->GetIsJustWokenUp() && 0==contacts.GetNumContacts())
	{
		bDoMapCollision=false;
	}
	if(bDoMapCollision)
	{
		for(int i=0;i<iNumMapInstances;i++)
		{
			phInst* pOtherInst=apMapCollisionInstances[i];
#if SPHERE_INTERSECTION
			IntersectSpheres(pOtherInst,*pCloth,contacts);
#elif CAPSULE_INTERSECTION
			IntersectCapsules(pOtherInst,*pCloth,contacts);
#endif
		}
	}
}

void phInstBehaviourCloth::UpdateCompositeBound()
{
	//Make a const cloth ptr so we don't change the cloth here.
	//Use the const cloth ptr for the remainder of this function for safety.
	const CCloth* pCloth=const_cast<const CCloth*>(m_pCloth);

	//Get the instance and the archetype and the bound.
	//Get the cloth instance.
	phInst* pInst=GetInstance();
	Assertf(dynamic_cast<phInstCloth*>(pInst), "Instance is not a cloth");
	phInstCloth* pInstCloth=static_cast<phInstCloth*>(pInst);
	//Get the archetype.
	phArchetype* pArchetype=pInstCloth->GetArchetype();
	Assertf(pArchetype, "Null archetype ptr");
	//Get the composite bound.
	phBound* pBound=pArchetype->GetBound();
	Assertf(pBound, "Null bound ptr");
	Assertf(dynamic_cast<phBoundComposite*>(pBound),"Bound is not a composite bound");
	phBoundComposite* pBoundComposite=static_cast<phBoundComposite*>(pBound);

	//Set the last matrices to be the current  matrices before we update the current matrices.
	pBoundComposite->UpdateLastMatricesFromCurrent();

	//Update the capsules of the composite.
	const Matrix34 instMatrix=MAT34V_TO_MATRIX34(pInst->GetMatrix());
	Matrix34 instMatrixInverse;
	instMatrixInverse.Inverse(instMatrix);
	for(int i=0;i<pCloth->GetCapsuleBounds().GetNumCapsuleBounds();i++)
	{
		Vector3 vA,vB;
		pCloth->GetCapsuleBounds().ComputeCapsuleEnds(i,pCloth->GetParticles(),vA,vB);
	
		//The rotation of the bound can be constructed easily from a quaternion 
		//(zero degrees rotated around the vector vB->vA).
		Matrix34 partMatrixWorldSpace(M34_IDENTITY);
		Vector3 vDir=vB-vA;
		vDir.Normalize();
		partMatrixWorldSpace.RotateTo(partMatrixWorldSpace.b,vDir);
		Vector3 vBoundCentre=(vA+vB)*0.5f;
		partMatrixWorldSpace.Translate(vBoundCentre);

		//Matrix in world space but we need it in local space for the bound.
		Matrix34 partMatrixLocalSpace;
		partMatrixLocalSpace.Dot(partMatrixWorldSpace,instMatrixInverse);

		//Set the matrix of the ith bound.
		pBoundComposite->SetCurrentMatrix(i,RCC_MAT34V(partMatrixLocalSpace));
	}

	//Now update the extents of the composite after updating the matrices of each part.
	bool onlyAdjustForInternalMotion = false;
	pBoundComposite->CalculateCompositeExtents(onlyAdjustForInternalMotion);
}

bool phInstBehaviourCloth::CollideObjects(Vec::V3Param128 timeStep, phInst* myInst, phCollider* UNUSED_PARAM(myCollider), phInst* otherInst, phCollider* UNUSED_PARAM(otherCollider), phInstBehavior* ASSERT_ONLY(otherInstBehaviour))
{
	Assertf(myInst==GetInstance(), "Wrong instance in callback");
	//Assertf(0==myCollider || myCollider->GetSleep()==0, "myCollider should have null sleep");
	Assertf(otherInst, "Other inst is null ptr");

	//Test the flags to see if the two instances should be tested for collision.
	const phArchetype* pMyArchetype=myInst->GetArchetype();
	const u32 myTypeFlags=pMyArchetype->GetTypeFlags();
	const u32 myIncludeFlags=pMyArchetype->GetIncludeFlags();
	const phArchetype* pOtherArchetype=otherInst->GetArchetype();
	const u32 otherTypeFlags=pOtherArchetype->GetTypeFlags();
	const u32 otherIncludeFlags=pOtherArchetype->GetIncludeFlags();
	if(otherIncludeFlags & myTypeFlags || myIncludeFlags & otherTypeFlags)
	{
		Assertf(!otherInstBehaviour, "Two plants colliding is forbidden by type and include flags");
		Assertf(m_iNumCollisionInstances<MAX_NUM_COLLISION_INSTANCES, "Out of bounds");

		if(m_iNumCollisionInstances<MAX_NUM_COLLISION_INSTANCES)
		{
			m_apCollisionInstances[m_iNumCollisionInstances]=otherInst;
			m_iNumCollisionInstances++;
		}

		//Wake the instance if there has been a collision.
		//Once the instance is awake we'll end up getting to phInstBehaviourCloth::Update()
		//For a bit of optimisation we need only do this on the first collision detected each update.
		//Obviously, only activate objects that are inactive.
		if(1==m_iNumCollisionInstances)
		{
			Assertf(GetInstance(), "Inst ptr is  null");

			if(PHLEVEL->IsInactive(GetInstance()->GetLevelIndex()))
			{
				PHSIM->ActivateObject(GetInstance()->GetLevelIndex());
			}
			if(m_pCloth->IsAsleep())
			{
				m_pCloth->SetToAwake();
				m_pCloth->SetIsJustWokenUp(true);
			}
		}
	}

	// Return false to indicate that the simulator should not calculate this collision.
	return false;
}

#endif // NORTH_CLOTHS


