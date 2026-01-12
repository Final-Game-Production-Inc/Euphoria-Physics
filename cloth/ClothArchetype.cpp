#include "ClothArchetype.h"
#include "ClothMgr.h"
#include "Core/Game.h"
#include "Peds/PlayerInfo.h"
#include "Peds/Ped.h"
#include "Scene/Entity.h"
#include "Scene/DataFileMgr.h"

#if NORTH_CLOTHS

CLOTH_OPTIMISATIONS()

FW_INSTANTIATE_CLASS_POOL /*_DEALWITHNOMEMORY*/(CClothArchetype, CClothMgr::MAX_NUM_CLOTH_ARCHETYPES, "CClothArchetype");
AUTOID_IMPL(CClothArchetype);

float CClothArchetype::sm_fDefaultMass=1.0f;
float CClothArchetype::sm_fDefaultTipMassMultiplier=0.05f;
float CClothArchetype::sm_fDefaultDampingRate=1.5f;
float CClothArchetype::sm_fDefaultCapsuleRadius=0.05f;

// number of plant object types
#define NUM_PLANT_MODEL_INFOS	(50)
static CStaticStore<CPlantTuning> gPlantTuningStore(NUM_PLANT_MODEL_INFOS);

void CPlantTuning::SetModelName(const strStreamingObjectName pName) 
{
	m_modelName = pName;
}

void CPlantTuning::Init()
{
	gPlantTuningStore.Init();
}

void CPlantTuning::Shutdown()
{
	gPlantTuningStore.Shutdown();
}

void CPlantTuning::ClearPlantTuneData()
{
	gPlantTuningStore.SetItemsUsed(0);
}

CPlantTuning& CPlantTuning::AddPlantTuneData(const strStreamingObjectName pName)
{
	CPlantTuning* pTuneData = gPlantTuningStore.GetNextItem();
	pTuneData->SetModelName(pName);
	return *pTuneData;
}

const CPlantTuning& CPlantTuning::GetPlantTuneData(const u32 key)
{
	ASSERT_ONLY(int index=-1;)
	int i;
	for(i=0;i<gPlantTuningStore.GetItemsUsed();i++)
	{
		const CPlantTuning& r=gPlantTuningStore[i];
		const u32 iKey=r.GetModelName().GetHash();
		if(iKey==key)
		{
			ASSERT_ONLY(index=i;)
			break;
		}
	}
	Assertf(index!=-1, "Couldn't find plant model for hashkey %d", key);
	return gPlantTuningStore[i];
}

void CPlantTuning::Init(unsigned initMode)
{
    if(initMode == INIT_AFTER_MAP_LOADED)
    {
	    CPlantTuning::ClearPlantTuneData();
	    const CDataFileMgr::DataFile* pData = DATAFILEMGR.GetLastFile(CDataFileMgr::BENDABLEPLANTS_FILE);
	    while(DATAFILEMGR.IsValid(pData))
	    {
		    FileHandle fid = CFileMgr::OpenFile(pData->m_filename);
		    Assertf( CFileMgr::IsValidFileHandle( fid ), "%s:Cannot find handling file", pData->m_filename);

		    //Read a line at a time putting values into the handling data structure
		    char* pLine;
		    while((pLine = CFileMgr::ReadLine(fid)) != NULL)
		    {
			    //Check for comments.
			    if(pLine[0] == '#')
				    continue;

			    char name[STORE_NAME_LENGTH];
			    float fTotalMass=1.0f;
			    float fDensityMultiplier=0.05f;
			    float fDampingRate=1.5f;
			    float fCapsuleRadius=0.15f;
			    const int count= sscanf(pLine, "%s %f %f %f %f", &name[0], &fTotalMass, &fDensityMultiplier, &fDampingRate, &fCapsuleRadius);
			    if(5!=count)
			    {
				    Assertf(false,"%s is using in valid data format in plants.ide",name);
			    }

			    CPlantTuning& rTuning = CPlantTuning::AddPlantTuneData(&name[0]);
			    rTuning.SetTotalMass(fTotalMass);
			    rTuning.SetDensityMultiplier(fDensityMultiplier);
			    rTuning.SetDampingRate(fDampingRate);
			    rTuning.SetCapsuleRadius(fCapsuleRadius);
		    }

            CFileMgr::CloseFile(fid);

		    // Get next file
		    pData = DATAFILEMGR.GetPrevFile(pData);
	    }
    }
}


void CClothArchetype::RefreshFromDefaults()
{
	//Reset the damping rate.
	int i;
	for(i=0;i<GetNumParticles();i++)
	{
		SetParticleDampingRate(i,sm_fDefaultDampingRate);
	}

	//Reset the particle masses.
	//First work out the maximum/minimum height so we can scale particle masses according to their height.
	//We're trying to simulate a plant getting thinner towards the tip.
	float fMinHeight=FLT_MAX;
	float fMaxHeight=-FLT_MAX;
	for(i=0;i<GetNumParticles();i++)
	{
		const Vector3& vPos=GetParticlePosition(i);
		const float fHeight=vPos.z;
		if(fHeight>fMaxHeight)
		{
			fMaxHeight=fHeight;
		}
		if(fHeight<fMinHeight)
		{
			fMinHeight=fHeight;
		}
	}
	//Rescale the masses of all the particles according to their height.
	const float fBaseMass=1.0f;
	float fTotalMass=1.0f;
	for(i=0;i<GetNumParticles();i++)
	{
		const float fCurrInvMass=GetParticleInvMass(i);
		if(0!=fCurrInvMass)
		{
			const Vector3& vPos=GetParticlePosition(i);
			const float fHeight=vPos.z;
			const float fNewMass=fBaseMass*(1.0f + (sm_fDefaultTipMassMultiplier-1)*(fHeight-fMinHeight)/(fMaxHeight-fMinHeight));
			Assert(fNewMass>0);
			const float fNewInvMass=1.0f/fNewMass;
			Assert(fNewInvMass>0);
			SetParticleInvMass(i,fNewInvMass);
			fTotalMass+=fNewMass;	
		}
	}
	//Now scale all the masses so they sum up to the target total mass.
	const float fMassMultplier=sm_fDefaultMass/fTotalMass;
	for(i=0;i<GetNumParticles();i++)
	{
		const float fCurrInvMass=GetParticleInvMass(i);
		if(0!=fCurrInvMass)
		{
			const float fCurrInvMass=GetParticleInvMass(i);
			const float fNewInvMass=fCurrInvMass/fMassMultplier;
			SetParticleInvMass(i,fNewInvMass);
		}
	}

	//Set all the radii to the target values.
	for(i=0;i<GetNumCapsuleBounds();i++)
	{
		SetCapsuleBoundRadius(i,sm_fDefaultCapsuleRadius);
	}
	for(i=0;i<GetNumParticles();i++)
	{
		SetParticleRadius(i,sm_fDefaultCapsuleRadius);
	}
	//Move the particles so they obey the new radius.
	Assertf(0==GetNumParticles()%MAX_NUM_PARTICLES_PER_CAPSULE, "Irregular number of particles");
	const int N=GetNumParticles()/MAX_NUM_PARTICLES_PER_CAPSULE;
	for(i=0;i<N;i++)
	{
		//Work out the particle ids of the ith layer of the cloth.
		int aiParticleIds[MAX_NUM_PARTICLES_PER_CAPSULE];
		int j;
		for(j=0;j<MAX_NUM_PARTICLES_PER_CAPSULE;j++)
		{
			const int iParticleId=MAX_NUM_PARTICLES_PER_CAPSULE*i+j;
			aiParticleIds[j]=iParticleId;
		}
		//Work out the centre of the ith layer of the cloth.
		Vector3 vCentre(0,0,0);
		for(j=0;j<MAX_NUM_PARTICLES_PER_CAPSULE;j++)
		{
			const int iParticleId=aiParticleIds[j];
			vCentre+=GetParticlePosition(iParticleId);
		}
		vCentre*=(1.0f/(1.0f*MAX_NUM_PARTICLES_PER_CAPSULE));
		//Move all the particles so they are the new radius from the centre.
		for(j=0;j<MAX_NUM_PARTICLES_PER_CAPSULE;j++)
		{
			const int iParticleId=aiParticleIds[j];
			const Vector3& vCurrPos=GetParticlePosition(iParticleId);
			Vector3 vDiff=vCurrPos-vCentre;
			vDiff.Normalize();
			vDiff*=sm_fDefaultCapsuleRadius;
			const Vector3 vNewPos=vCentre+vDiff;
			SetParticlePosition(iParticleId,vNewPos);
		}
	}

	//Now that we've moved all the particles we need to update the constraint and spring rest lengths.
	for(i=0;i<GetNumSprings();i++)
	{
		const int iParticleA=GetPtrToSpringParticleIdsA()[i];
		const int iParticleB=GetPtrToSpringParticleIdsB()[i];
		const Vector3& vA=GetParticlePosition(iParticleA);
		const Vector3& vB=GetParticlePosition(iParticleB);
		const Vector3 vDiff=vA-vB;
		const float fLength=vDiff.Mag();
		SetSpringRestLength(i,fLength);
	}
	for(i=0;i<GetNumSeparationConstraints();i++)
	{
		const int iParticleA=GetPtrToSeparationConstraintParticleIdsA()[i];
		const int iParticleB=GetPtrToSeparationConstraintParticleIdsB()[i];
		const Vector3& vA=GetParticlePosition(iParticleA);
		const Vector3& vB=GetParticlePosition(iParticleB);
		const Vector3 vDiff=vA-vB;
		const float fLength=vDiff.Mag();
		SetSeparationConstraintRestLength(i,fLength);
	}
}

CompileTimeAssert(3==MAX_NUM_PARTICLES_PER_CAPSULE || 4==MAX_NUM_PARTICLES_PER_CAPSULE);

#if 3==MAX_NUM_PARTICLES_PER_CAPSULE
Vector3 avOffsets[MAX_NUM_PARTICLES_PER_CAPSULE]=
{
	Vector3(0,CClothArchetype::sm_fDefaultCapsuleRadius,0),
	Vector3(0.707f*CClothArchetype::sm_fDefaultCapsuleRadius,-0.707f*CClothArchetype::sm_fDefaultCapsuleRadius,0),
	Vector3(-0.707f*CClothArchetype::sm_fDefaultCapsuleRadius,-0.707f*CClothArchetype::sm_fDefaultCapsuleRadius,0)
};
#elif 4==MAX_NUM_PARTICLES_PER_CAPSULE
Vector3 avOffsets[MAX_NUM_PARTICLES_PER_CAPSULE]=
{
	Vector3(-CClothArchetype::sm_fDefaultCapsuleRadius,-CClothArchetype::sm_fDefaultCapsuleRadius,0),
	Vector3(CClothArchetype::sm_fDefaultCapsuleRadius,-CClothArchetype::sm_fDefaultCapsuleRadius,0),
	Vector3(CClothArchetype::sm_fDefaultCapsuleRadius,CClothArchetype::sm_fDefaultCapsuleRadius,0),
	Vector3(-CClothArchetype::sm_fDefaultCapsuleRadius,CClothArchetype::sm_fDefaultCapsuleRadius,0)
};
#endif

CClothArchetype* CClothArchetypeCreator::CreatePlant(const crSkeletonData& skeletonData, const CPlantTuning& plantTuningData)
{
	CClothArchetype* pArchetype=rage_checked_pool_new (CClothArchetype);
	if(NULL==pArchetype)
	{
		return NULL;
	}

	//Get the root bone of the skeleton.
	const crBoneData* pRootBoneData0=skeletonData.GetBoneData(0);

	//Default mass (just set all the particles to the default mass then compute the particle masses
	//after all the particles have been created).
	const float fDefaultMass=1.0f;

	//Need these to walk the skeleton.
	const crBoneData* pGrandParent=0;
	const crBoneData* pParent=pRootBoneData0;

	//Work out all the node positions and the number of nodes.
	int iNumNodes=1; //count the root before we count the children.
	Vector3 avNodePositions[MAX_NUM_CLOTH_PARTICLES];
	avNodePositions[0]=RCC_VECTOR3(pParent->GetDefaultTranslation());
	WalkSkeleton(pParent,MAX_NUM_CLOTH_PARTICLES,avNodePositions,iNumNodes);

	//Work out which nodes are duplicates of others.
	int i;
	int aiDuplicates[MAX_NUM_CLOTH_PARTICLES];
	for(i=0;i<iNumNodes;i++)
	{
		aiDuplicates[i]=-1;
	}
	for(i=0;i<iNumNodes;i++)
	{
		if(-1==aiDuplicates[i])
		{
			const Vector3& vPos=avNodePositions[i];
			int j;
			for(j=i+1;j<iNumNodes;j++)
			{
				if(-1==aiDuplicates[j])
				{
					const Vector3& vOtherPos=avNodePositions[j];
					const Vector3 vDiff=vOtherPos-vPos;
					if(0.0f==vDiff.x && 0.0f==vDiff.y && 0.0f==vDiff.z)
					{
						aiDuplicates[j]=i;
					}
				}
			}
		}
	}
	//Work out how many non-duplicated nodes exist and record a map between skeleton node and non-duplicated node id.
	int aiSkeletonNodeMap[MAX_NUM_CLOTH_PARTICLES];
	int iNumDuplicateNodes=0;
	for(i=0;i<iNumNodes;i++)
	{
		if(-1==aiDuplicates[i])
		{
			aiSkeletonNodeMap[i]=i-iNumDuplicateNodes;
		}
		else
		{
			Assert(aiDuplicates[i]<i);
			aiSkeletonNodeMap[i]=aiSkeletonNodeMap[aiDuplicates[i]];
			iNumDuplicateNodes++;
		}
	}

	//Set the number of skeleton bones that will be posed by cloths cloned from the archetype.
	pArchetype->SetNumSkeletonBones(iNumNodes);

	//Set the number of particles on the archetype.
	Assertf(4==MAX_NUM_PARTICLES_PER_CAPSULE || 3==MAX_NUM_PARTICLES_PER_CAPSULE, "Invalid MAX_NUM_PARTICLES_PER_CAPSULE");
	const int iNumNonDuplicateNodes=iNumNodes-iNumDuplicateNodes;
	pArchetype->SetNumParticles(MAX_NUM_PARTICLES_PER_CAPSULE*iNumNonDuplicateNodes);

	//Set up the root particles (make them infinite mass).
	Assertf(pRootBoneData0->GetIndex()==0, "Root particle should have index zero");
	for(i=0;i<MAX_NUM_PARTICLES_PER_CAPSULE;i++)
	{
		const Vector3& vRootPos=RCC_VECTOR3(pRootBoneData0->GetDefaultTranslation());
		Vector3 vParticlePos=vRootPos+avOffsets[i];
		const int iParticleId=MAX_NUM_PARTICLES_PER_CAPSULE*pRootBoneData0->GetIndex()+i;
		pArchetype->SetParticle(iParticleId,vParticlePos,CClothArchetype::sm_fDefaultDampingRate,-1,CClothArchetype::sm_fDefaultCapsuleRadius);
	}
	
	//Walk the skeleton and add all the particles, constraints, and springs.
	WalkSkeleton(aiSkeletonNodeMap,aiDuplicates,pGrandParent,pParent,CClothArchetype::sm_fDefaultDampingRate,fDefaultMass,CClothArchetype::sm_fDefaultCapsuleRadius,*pArchetype);

#define ADD_BRANCH_SPRINGS (0)
#if ADD_BRANCH_SPRINGS

	//Work out which particles are at branches.
	//Now look for branches and add springs to add stability.
	bool abIsBranchingParticle[MAX_NUM_CLOTH_PARTICLES];
	for(i=0;i<iNumNodes;i++)
	{
		abIsBranchingParticle[i]=false;
	}
	for(i=0;i<iNumNodes;i++)
	{
		if(-1!=aiDuplicates[i])
		{
			abIsBranchingParticle[aiSkeletonNodeMap[aiDuplicates[i]]]=true;
		}
	}		
	for(i=0;i<iNumNodes;i++)
	{
		if(abIsBranchingParticle[i])
		{
			const int iParticleId=MAX_NUM_PARTICLES_PER_CAPSULE*i+0;

			int iNumChildParticles=0;
			int aiChildParticles[MAX_NUM_CLOTH_CONSTRAINTS];
			int j;
			for(j=0;j<pArchetype->GetNumConstraints();j++)
			{
				const int iParticleIdA=pArchetype->GetPtrToConstraintParticleIdsA()[j];
				if(iParticleId==iParticleIdA)
				{
					const int iOtherParticle=pArchetype->GetPtrToConstraintParticleIdsB()[j];
					if((iOtherParticle/MAX_NUM_PARTICLES_PER_CAPSULE) != (iParticleId/MAX_NUM_PARTICLES_PER_CAPSULE))
					{
						aiChildParticles[iNumChildParticles]=pArchetype->GetPtrToConstraintParticleIdsB()[j]/MAX_NUM_PARTICLES_PER_CAPSULE;
						iNumChildParticles++;
					}
				}
			}
			Assert(iNumChildParticles>0);

			if(iNumChildParticles>1)
			{
				//Find the pair that are furthest apart.
				float fMaxDist2=0;
				int iParticleA=-1;
				int iParticleB=-1;
				int j;
				for(j=0;j<iNumChildParticles;j++)
				{
					const Vector3& vj=pArchetype->GetParticlePosition(MAX_NUM_PARTICLES_PER_CAPSULE*aiChildParticles[j]+0);
					int k;
					for(k=j+1;k<iNumChildParticles;k++)
					{
						const Vector3& vk=pArchetype->GetParticlePosition(MAX_NUM_PARTICLES_PER_CAPSULE*aiChildParticles[k]+0);
						const Vector3 vDiff=vj-vk;
						const float fDist2=vDiff.Mag2();
						if(fDist2>fMaxDist2)
						{
							fMaxDist2=fDist2;
							iParticleA=j;
							iParticleB=k;
						}
					}
				}
				Assert(iParticleA>=0);
				Assert(iParticleB>=0);
				Assert(iParticleA!=iParticleB);

				//Start with one of the particles of the most separated pair and iterate to each subsequent
				//closest particle until we reach the other particle of the most separated pair.
				int iNumSpringsAdded=0;
				while(iNumSpringsAdded<(iNumChildParticles-1))
				{
					const Vector3& vPos=pArchetype->GetParticlePosition(MAX_NUM_PARTICLES_PER_CAPSULE*aiChildParticles[iParticleA]+0);
					const int iParticleId=aiChildParticles[iParticleA];
					aiChildParticles[iParticleA]=-1;

					//Find the closest particle.
					float fMinDist2=FLT_MAX;
					int iClosestParticle=-1;
					int j;
					for(j=0;j<iNumChildParticles;j++)
					{
						if(-1!=aiChildParticles[j])
						{
							const Vector3& vOtherPos=pArchetype->GetParticlePosition(MAX_NUM_PARTICLES_PER_CAPSULE*aiChildParticles[j]+0);
							const Vector3 vDiff=vPos-vOtherPos;
							const float fDist2=vDiff.Mag2();
							if(fDist2<fMinDist2)
							{
								fMinDist2=fDist2;
								iClosestParticle=j;
							}
						}
					}
					Assert(iClosestParticle!=-1);
					Assert(iClosestParticle!=iParticleA);

					//Add springs between the current particle and the closest particle.
					for(j=0;j<MAX_NUM_PARTICLES_PER_CAPSULE;j++)
					{
						const Vector3& vPosA=pArchetype->GetParticlePosition(MAX_NUM_PARTICLES_PER_CAPSULE*iParticleId+j);
						const Vector3& vPosB=pArchetype->GetParticlePosition(MAX_NUM_PARTICLES_PER_CAPSULE*aiChildParticles[iClosestParticle]+j);
						Vector3 vDiff=vPosA-vPosB;
						const float fDist=vDiff.Mag();
						pArchetype->AddSpring(MAX_NUM_PARTICLES_PER_CAPSULE*iParticleId+j,MAX_NUM_PARTICLES_PER_CAPSULE*aiChildParticles[iClosestParticle]+j,fDist,0);
					}

					iParticleA=iClosestParticle;
					iNumSpringsAdded++;
				}
			}
		}
	}
#endif

#if 1
	const crBoneData* pChild=0;
	for (pChild = pRootBoneData0->GetChild(); pChild; pChild = pChild->GetNext())
	{
		//Get the index of the child bone.
		const int iChildIndex=aiSkeletonNodeMap[pChild->GetIndex()];

		for(i=0;i<MAX_NUM_PARTICLES_PER_CAPSULE;i++)
		{
			//Set the particles of this layer to infinite mass.
			const int iParticleId=MAX_NUM_PARTICLES_PER_CAPSULE*iChildIndex+i;
			const Vector3& vPos=pArchetype->GetParticlePosition(iParticleId);
			pArchetype->SetParticle(iParticleId,vPos,CClothArchetype::sm_fDefaultDampingRate,-1,CClothArchetype::sm_fDefaultCapsuleRadius);
		}
	}
#else
	//Set a second infinite mass layer of particles.
	const crBoneData* pRootBoneData1=skeletonData.GetBoneData(1);
	Assertf(1==pRootBoneData1->GetIndex(), "First bone should have index 1");
	for(i=0;i<MAX_NUM_PARTICLES_PER_CAPSULE;i++)
	{
		const int iParticleId=MAX_NUM_PARTICLES_PER_CAPSULE*pRootBoneData1->GetIndex()+i;
		const Vector3& vPos=pArchetype->GetParticlePosition(iParticleId);
		pArchetype->SetParticle(iParticleId,vPos,CClothArchetype::sm_fDefaultDampingRate,-1,CClothArchetype::sm_fDefaultCapsuleRadius);
	}
#endif

	//Set masses and damping rates and capsule radii to match the tuning data.
	//Just set the widget values to be the tuning data values and reuse the function that 
	//resets the archetype from the widget values.  Don't forget to reset the widget values
	//at the end.
	{
	const float fDefaultMass=CClothArchetype::sm_fDefaultMass;
	const float fDefaultTipMassMultiplier=CClothArchetype::sm_fDefaultTipMassMultiplier;
	const float fDefaultDampingRate=CClothArchetype::sm_fDefaultDampingRate;
	const float fDefaultCapusleRadius=CClothArchetype::sm_fDefaultCapsuleRadius;
	CClothArchetype::sm_fDefaultMass=plantTuningData.GetTotalMass();
	CClothArchetype::sm_fDefaultTipMassMultiplier=plantTuningData.GetDensityMultiplier();
	CClothArchetype::sm_fDefaultDampingRate=plantTuningData.GetDampingRate();
	CClothArchetype::sm_fDefaultCapsuleRadius=plantTuningData.GetCapsuleRadius();
	pArchetype->RefreshFromDefaults();
	CClothArchetype::sm_fDefaultMass=fDefaultMass;
	CClothArchetype::sm_fDefaultTipMassMultiplier=fDefaultTipMassMultiplier;
	CClothArchetype::sm_fDefaultDampingRate=fDefaultDampingRate;
	CClothArchetype::sm_fDefaultCapsuleRadius=fDefaultCapusleRadius;
	}

	//That's us finished.
	return pArchetype;
}

void CClothArchetypeCreator::WalkSkeleton
(const crBoneData* pParent, 
 const int iNodePositionArraySize, Vector3* avNodePositions, int& iNumNodes)
{
	//Add the parent-child relationship as a constraint and set the position of the child.
	const crBoneData* pChild=0;
	for (pChild = pParent->GetChild(); pChild; pChild = pChild->GetNext())
	{
		Assertf(pChild, "Null child ptr");

		//Get the index of the parent bone.
		const int iParentIndex=pParent->GetIndex();
		Assert(iParentIndex<iNodePositionArraySize);
		//Get the index of the child bone.
		const int iChildIndex=pChild->GetIndex();
		Assert(iChildIndex<iNodePositionArraySize);

		//Increment the number of nodes.
		iNumNodes++;

		//Get the translation of the child and set up 4 child 
		//particles as offsets from the parent particles.
		const Vector3& vTranslation=RCC_VECTOR3(pChild->GetDefaultTranslation());
		const Vector3& vParentPos=avNodePositions[iParentIndex];
		const Vector3 vChildPos=vParentPos+vTranslation;
		avNodePositions[iChildIndex]=vChildPos;
		//Carry on walking the skeleton.
		WalkSkeleton(pChild,iNodePositionArraySize,avNodePositions,iNumNodes);
	}
}

void CClothArchetypeCreator::AddParticleLayer
(const int iGrandParentNodeIndexNotDuplicated, const int iParentNodeIndexNotDuplicated, const int iChildNodeIndexNotDuplicated, 
 const int iParentNodeIndex,
 const Vector3& vTranslation, 
 const float fDefaultDampingRate, const float fDefaultMass, const float fDefaultRadius,
 CClothArchetype& archetype) 
{
	int i;

#define ANGLED_PARTICLE_LAYERS 1

#if ANGLED_PARTICLE_LAYERS

	//Work out the centre position of the parent particle layer.
	Vector3 vParentCentrePos(0,0,0);
	for(i=0;i<MAX_NUM_PARTICLES_PER_CAPSULE;i++)
	{
		//Get the position of the parent particle.
		//(we need this to work out the position of the child as an offset from the parent).
		const int iParentParticleIndex=MAX_NUM_PARTICLES_PER_CAPSULE*iParentNodeIndexNotDuplicated+i;
		vParentCentrePos+=archetype.GetParticlePosition(iParentParticleIndex);
	}
	vParentCentrePos*=1.0f/(1.0f*MAX_NUM_PARTICLES_PER_CAPSULE);

	//Work out the centre position of the child particle layer.
	Vector3 vChildCentrePos=vParentCentrePos+vTranslation;

	//Work out the matrix that transforms from (0,0,1) to the normalised translation vector.
	Vector3 vFrom(0,0,1);
	Vector3 vTo(vTranslation);
	vTo.Normalize();
	Matrix34 mat;
	mat.Identity3x3();
	mat.RotateTo(vFrom,vTo);
	mat.d.Zero();

	//Work out the particle positions as offsets from the centre of the child layer of particles.
	for(i=0;i<MAX_NUM_PARTICLES_PER_CAPSULE;i++)
	{
		const Vector3 vOffset=mat*avOffsets[i];
		const Vector3 vChildParticlePos=vChildCentrePos+vOffset;
		const int iParticleIndex=MAX_NUM_PARTICLES_PER_CAPSULE*iChildNodeIndexNotDuplicated+i;
		archetype.SetParticle(iParticleIndex,vChildParticlePos,fDefaultDampingRate,fDefaultMass,fDefaultRadius);
	}

#else

	for(i=0;i<MAX_NUM_PARTICLES_PER_CAPSULE;i++)
	{
		//Get the position of the parent particle.
		//(we need this to work out the position of the child as an offset from the parent).
		const int iParentParticleIndex=MAX_NUM_PARTICLES_PER_CAPSULE*iParentNodeIndexNotDuplicated+i;
		const Vector3& vParentPos=archetype.GetParticlePosition(iParentParticleIndex);
		//Compute the position of the child particle.
		const Vector3 vChildPos=vParentPos+vTranslation;
		//Set the position of the child particle.
		const int iParticleIndex=MAX_NUM_PARTICLES_PER_CAPSULE*iChildNodeIndexNotDuplicated+i;
		archetype.SetParticle(iParticleIndex,vChildPos,fDefaultDampingRate,fDefaultMass,fDefaultRadius);
	}

#endif

	//Add horizontal constraints between the child particles.
	for(i=0;i<MAX_NUM_PARTICLES_PER_CAPSULE;i++)
	{
		const int i0=MAX_NUM_PARTICLES_PER_CAPSULE*iChildNodeIndexNotDuplicated+(i+0)%MAX_NUM_PARTICLES_PER_CAPSULE;
		const int i1=MAX_NUM_PARTICLES_PER_CAPSULE*iChildNodeIndexNotDuplicated+(i+1)%MAX_NUM_PARTICLES_PER_CAPSULE;
		const Vector3& v0=archetype.GetParticlePosition(i0);
		const Vector3& v1=archetype.GetParticlePosition(i1);
		const Vector3 vDiff=v0-v1;
		const float fRestLength=vDiff.Mag();
		archetype.AddSeparationConstraint(i0,i1,fRestLength);
	}
#if 4==MAX_NUM_PARTICLES_PER_CAPSULE
	//Add diagonal constraints between the child particles.
	{
		const int i0=MAX_NUM_PARTICLES_PER_CAPSULE*iChildNodeIndexNotDuplicated+(0+0)%MAX_NUM_PARTICLES_PER_CAPSULE;
		const int i1=MAX_NUM_PARTICLES_PER_CAPSULE*iChildNodeIndexNotDuplicated+(0+2)%MAX_NUM_PARTICLES_PER_CAPSULE;
		const Vector3& v0=archetype.GetParticlePosition(i0);
		const Vector3& v1=archetype.GetParticlePosition(i1);
		const Vector3 vDiff=v0-v1;
		const float fRestLength=vDiff.Mag();
		archetype.AddSeparationConstraint(i0,i1,fRestLength);
	}
	{
		const int i0=MAX_NUM_PARTICLES_PER_CAPSULE*iChildNodeIndexNotDuplicated+(1+0)%MAX_NUM_PARTICLES_PER_CAPSULE;
		const int i1=MAX_NUM_PARTICLES_PER_CAPSULE*iChildNodeIndexNotDuplicated+(1+2)%MAX_NUM_PARTICLES_PER_CAPSULE;
		const Vector3& v0=archetype.GetParticlePosition(i0);
		const Vector3& v1=archetype.GetParticlePosition(i1);
		const Vector3 vDiff=v0-v1;
		const float fRestLength=vDiff.Mag();
		archetype.AddSeparationConstraint(i0,i1,fRestLength);
	}
#endif
	//Add vertical constraints between the child and parent particles.
	for(i=0;i<MAX_NUM_PARTICLES_PER_CAPSULE;i++)
	{
		const int i0=MAX_NUM_PARTICLES_PER_CAPSULE*iParentNodeIndexNotDuplicated+i;
		const int i1=MAX_NUM_PARTICLES_PER_CAPSULE*iChildNodeIndexNotDuplicated+i;
		const Vector3& v0=archetype.GetParticlePosition(i0);
		const Vector3& v1=archetype.GetParticlePosition(i1);
		const Vector3 vDiff=v0-v1;
		const float fRestLength=vDiff.Mag();
		archetype.AddSeparationConstraint(i0,i1,fRestLength);
	}
	//Add capsule bound between parent child.
	{
		//Work out the averaged positions of the parent and child particles.
		Vector3 vParentCentre(0,0,0);
		Vector3 vChildCentre(0,0,0);
		for(i=0;i<MAX_NUM_PARTICLES_PER_CAPSULE;i++)
		{
			vParentCentre+=archetype.GetParticlePosition(MAX_NUM_PARTICLES_PER_CAPSULE*iParentNodeIndexNotDuplicated+i);
			vChildCentre+=archetype.GetParticlePosition(MAX_NUM_PARTICLES_PER_CAPSULE*iChildNodeIndexNotDuplicated+i);
		}
		vParentCentre*=1.0f/(1.0f*MAX_NUM_PARTICLES_PER_CAPSULE);
		vChildCentre*=1.0f/(1.0f*MAX_NUM_PARTICLES_PER_CAPSULE);
		//Compute the length of the capsule.
		const Vector3 vDiff=vParentCentre-vChildCentre;
		const float fRestLength=vDiff.Mag();

#if 3==MAX_NUM_PARTICLES_PER_CAPSULE
		//Add the capsule bound.
		archetype.AddCapsuleBound
			(iParentNodeIndex,
			MAX_NUM_PARTICLES_PER_CAPSULE*iParentNodeIndexNotDuplicated+0,MAX_NUM_PARTICLES_PER_CAPSULE*iParentNodeIndexNotDuplicated+1,MAX_NUM_PARTICLES_PER_CAPSULE*iParentNodeIndexNotDuplicated+2,
			MAX_NUM_PARTICLES_PER_CAPSULE*iChildNodeIndexNotDuplicated+0, MAX_NUM_PARTICLES_PER_CAPSULE*iChildNodeIndexNotDuplicated+1, MAX_NUM_PARTICLES_PER_CAPSULE*iChildNodeIndexNotDuplicated+2, 
			fRestLength,fDefaultRadius);
#elif 4==MAX_NUM_PARTICLES_PER_CAPSULE
		//Add the capsule bound.
		archetype.AddCapsuleBound
			(iParentNodeIndex,
			MAX_NUM_PARTICLES_PER_CAPSULE*iParentNodeIndexNotDuplicated+0,MAX_NUM_PARTICLES_PER_CAPSULE*iParentNodeIndexNotDuplicated+1,MAX_NUM_PARTICLES_PER_CAPSULE*iParentNodeIndexNotDuplicated+2,MAX_NUM_PARTICLES_PER_CAPSULE*iParentNodeIndexNotDuplicated+3,
			MAX_NUM_PARTICLES_PER_CAPSULE*iChildNodeIndexNotDuplicated+0, MAX_NUM_PARTICLES_PER_CAPSULE*iChildNodeIndexNotDuplicated+1, MAX_NUM_PARTICLES_PER_CAPSULE*iChildNodeIndexNotDuplicated+2, MAX_NUM_PARTICLES_PER_CAPSULE*iChildNodeIndexNotDuplicated+3,
			fRestLength,fDefaultRadius);
#endif
	}

	//Add bender springs between grandparent particles and child particles.
	if(iGrandParentNodeIndexNotDuplicated!=-1)
	{
		//Add vertical bender springs.
		for(i=0;i<MAX_NUM_PARTICLES_PER_CAPSULE;i++)
		{
			const int iChildParticleIndex=MAX_NUM_PARTICLES_PER_CAPSULE*iChildNodeIndexNotDuplicated+i;
			const int iGrandParentParticleIndex=MAX_NUM_PARTICLES_PER_CAPSULE*iGrandParentNodeIndexNotDuplicated+i;
			const Vector3& v0=archetype.GetParticlePosition(iChildParticleIndex);
			const Vector3& v1=archetype.GetParticlePosition(iGrandParentParticleIndex);
			const Vector3 vDiff=v0-v1;
			const float fRestLength=vDiff.Mag();
			archetype.AddSpring(iGrandParentParticleIndex,iChildParticleIndex,fRestLength,0);
		}
		//Add diagonal bender springs.
		for(i=0;i<MAX_NUM_PARTICLES_PER_CAPSULE;i++)
		{
			const int iChildParticleIndex=MAX_NUM_PARTICLES_PER_CAPSULE*iChildNodeIndexNotDuplicated+(i+0)%MAX_NUM_PARTICLES_PER_CAPSULE;
			const int iGrandParentParticleIndex=MAX_NUM_PARTICLES_PER_CAPSULE*iGrandParentNodeIndexNotDuplicated+(i+2)%MAX_NUM_PARTICLES_PER_CAPSULE;
			const Vector3& v0=archetype.GetParticlePosition(iChildParticleIndex);
			const Vector3& v1=archetype.GetParticlePosition(iGrandParentParticleIndex);
			const Vector3 vDiff=v0-v1;
			const float fRestLength=vDiff.Mag();
			archetype.AddSpring(iGrandParentParticleIndex,iChildParticleIndex,fRestLength,0);
		}
	}

	//Add volume conservation constraints
	//Split cube into two pyramids then each pyramid into three tetrahedra.
	{
		int cuboidVerts[8]=
		{
				MAX_NUM_PARTICLES_PER_CAPSULE*iParentNodeIndexNotDuplicated+0,
				MAX_NUM_PARTICLES_PER_CAPSULE*iParentNodeIndexNotDuplicated+1,
				MAX_NUM_PARTICLES_PER_CAPSULE*iParentNodeIndexNotDuplicated+2,
				MAX_NUM_PARTICLES_PER_CAPSULE*iParentNodeIndexNotDuplicated+3,
				MAX_NUM_PARTICLES_PER_CAPSULE*iChildNodeIndexNotDuplicated+0,
				MAX_NUM_PARTICLES_PER_CAPSULE*iChildNodeIndexNotDuplicated+1,
				MAX_NUM_PARTICLES_PER_CAPSULE*iChildNodeIndexNotDuplicated+2,
				MAX_NUM_PARTICLES_PER_CAPSULE*iChildNodeIndexNotDuplicated+3
		};
		int pyramidVerts[12]=
		{
			cuboidVerts[0],cuboidVerts[2],cuboidVerts[3],cuboidVerts[4],cuboidVerts[6],cuboidVerts[7],
			cuboidVerts[2],cuboidVerts[0],cuboidVerts[1],cuboidVerts[6],cuboidVerts[4],cuboidVerts[5]
		};
		float fTotalVol=0;
		for(int k=0;k<2;k++)
		{
			{
				const int i1=pyramidVerts[k*6+0];
				const int i2=pyramidVerts[k*6+1];
				const int i3=pyramidVerts[k*6+2];
				const int i4=pyramidVerts[k*6+5];
				const Vector3& v1=archetype.GetParticlePosition(i1);
				const Vector3& v2=archetype.GetParticlePosition(i2);
				const Vector3& v3=archetype.GetParticlePosition(i3);
				const Vector3& v4=archetype.GetParticlePosition(i4);
				const float fVol=CClothVolumeConstraints::ComputeVolume(v1,v2,v3,v4);
				fTotalVol+=fVol;
				archetype.AddVolumeConstraint(i1,i2,i3,i4,fVol);
			}
			{
				const int i1=pyramidVerts[k*6+5];
				const int i2=pyramidVerts[k*6+3];
				const int i3=pyramidVerts[k*6+1];
				const int i4=pyramidVerts[k*6+4];
				const Vector3& v1=archetype.GetParticlePosition(i1);
				const Vector3& v2=archetype.GetParticlePosition(i2);
				const Vector3& v3=archetype.GetParticlePosition(i3);
				const Vector3& v4=archetype.GetParticlePosition(i4);
				const float fVol=CClothVolumeConstraints::ComputeVolume(v1,v2,v3,v4);
				fTotalVol+=fVol;
				archetype.AddVolumeConstraint(i1,i2,i3,i4,fVol);
			}
			{
				const int i1=pyramidVerts[k*6+3];
				const int i2=pyramidVerts[k*6+5];
				const int i3=pyramidVerts[k*6+1];
				const int i4=pyramidVerts[k*6+0];
				const Vector3& v1=archetype.GetParticlePosition(i1);
				const Vector3& v2=archetype.GetParticlePosition(i2);
				const Vector3& v3=archetype.GetParticlePosition(i3);
				const Vector3& v4=archetype.GetParticlePosition(i4);
				const float fVol=CClothVolumeConstraints::ComputeVolume(v1,v2,v3,v4);
				fTotalVol+=fVol;
				archetype.AddVolumeConstraint(i1,i2,i3,i4,fVol);
			}
		}
	}
}

void CClothArchetypeCreator::WalkSkeleton
(const int* paiSkeletonNodeMap, const int* paiDuplicates,
 const crBoneData* pGrandParent, const crBoneData* pParent, 
 const float fDefaultDampingRate, const float fDefaultMass, const float fDefaultRadius, 
 CClothArchetype& archetype)
{
	//Get the index of the grandparent bone and particle.
	int iGrandParentNodeIndex=-1;
	int iGrandParentNodeIndexNotDuplicated=-1;
	if(pGrandParent)
	{
		iGrandParentNodeIndex=pGrandParent->GetIndex();
		iGrandParentNodeIndexNotDuplicated=paiSkeletonNodeMap[iGrandParentNodeIndex];
	}

	//Get the index of the parent bone and particle.
	Assertf(pParent, "Null parent ptr");
	const int iParentNodeIndex=pParent->GetIndex();
	const int iParentNodeIndexNotDuplicated=paiSkeletonNodeMap[iParentNodeIndex];

	//Add the parent-child relationship as a constraint and set the position of the child.
	const crBoneData* pChild=pParent->GetChild();
	if(0==pChild)
	{
		/*
		//Get the index of the child bone and particle.
		Assert(0==archetype.GetNumParticles()%MAX_NUM_PARTICLES_PER_CAPSULE);
		const int iChildNodeIndex=archetype.GetNumParticles()/MAX_NUM_PARTICLES_PER_CAPSULE;
		const int iChildNodeIndexNotDuplicated=iChildNodeIndex;

		//Get the translation of the parent that we will apply to the child.
		const Vector3& vTranslation=pParent->GetDefaultTranslation();

		//Add another layer of particles (we need to increment the particle count for tip particles because they
		//don't correspond to a bone but we need them to compute the matrix of the tip bones).
		//   (i)	add the child particles.
		//  (ii)	add constraints between the child particles.
		// (iii)	add constraints between the parent and child particles.
		//  (iv)	add a capsule bound between parent and child particles for collision detection.
		//   (v)	add bender springs between grandparent and child particles.
		archetype.SetNumParticles(archetype.GetNumParticles()+MAX_NUM_PARTICLES_PER_CAPSULE);
		AddParticleLayer
			(iGrandParentNodeIndexNotDuplicated,iParentNodeIndexNotDuplicated,iChildNodeIndexNotDuplicated,
			iParentNodeIndex,
			vTranslation,
			fDefaultDampingRate,fDefaultMass,fDefaultRadius, 
			archetype);
		*/

		//Add capsule bound between parent child.
		{
			//Work out the averaged positions of the parent and child particles.
			Vector3 vParentCentre(0,0,0);
			Vector3 vGrandparentCentre(0,0,0);
			int i;
			for(i=0;i<MAX_NUM_PARTICLES_PER_CAPSULE;i++)
			{
				vParentCentre+=archetype.GetParticlePosition(MAX_NUM_PARTICLES_PER_CAPSULE*iParentNodeIndexNotDuplicated+i);
				vGrandparentCentre+=archetype.GetParticlePosition(MAX_NUM_PARTICLES_PER_CAPSULE*iGrandParentNodeIndexNotDuplicated+i);
			}
			vParentCentre*=1.0f/(1.0f*MAX_NUM_PARTICLES_PER_CAPSULE);
			vGrandparentCentre*=1.0f/(1.0f*MAX_NUM_PARTICLES_PER_CAPSULE);
			//Compute the length of the capsule.
			const Vector3 vDiff=vParentCentre-vGrandparentCentre;
			const float fRestLength=vDiff.Mag();

			//Add the capsule bound.
#if 3==MAX_NUM_PARTICLES_PER_CAPSULE
			archetype.AddCapsuleBound
				(iParentNodeIndex,
				MAX_NUM_PARTICLES_PER_CAPSULE*iParentNodeIndexNotDuplicated+0,MAX_NUM_PARTICLES_PER_CAPSULE*iParentNodeIndexNotDuplicated+1,MAX_NUM_PARTICLES_PER_CAPSULE*iParentNodeIndexNotDuplicated+2,
				MAX_NUM_PARTICLES_PER_CAPSULE*iGrandParentNodeIndexNotDuplicated+0, MAX_NUM_PARTICLES_PER_CAPSULE*iGrandParentNodeIndexNotDuplicated+1, MAX_NUM_PARTICLES_PER_CAPSULE*iGrandParentNodeIndexNotDuplicated+2,
				fRestLength,fDefaultRadius);
#elif 4==MAX_NUM_PARTICLES_PER_CAPSULE
			archetype.AddCapsuleBound
				(iParentNodeIndex,
				MAX_NUM_PARTICLES_PER_CAPSULE*iParentNodeIndexNotDuplicated+0,MAX_NUM_PARTICLES_PER_CAPSULE*iParentNodeIndexNotDuplicated+1,MAX_NUM_PARTICLES_PER_CAPSULE*iParentNodeIndexNotDuplicated+2,MAX_NUM_PARTICLES_PER_CAPSULE*iParentNodeIndexNotDuplicated+3,
				MAX_NUM_PARTICLES_PER_CAPSULE*iGrandParentNodeIndexNotDuplicated+0, MAX_NUM_PARTICLES_PER_CAPSULE*iGrandParentNodeIndexNotDuplicated+1, MAX_NUM_PARTICLES_PER_CAPSULE*iGrandParentNodeIndexNotDuplicated+2, MAX_NUM_PARTICLES_PER_CAPSULE*iGrandParentNodeIndexNotDuplicated+3,
				fRestLength,fDefaultRadius);
#endif
		}
	}
	else
	{	
		for (pChild = pParent->GetChild(); pChild; pChild = pChild->GetNext())
		{
			//Get the index of the child bone and particle.
			const int iChildNodeIndex=pChild->GetIndex();
			const int iChildNodeIndexNotDuplicated=paiSkeletonNodeMap[iChildNodeIndex];

			//Work out if the child node is a duplicate.
			bool bIsChildDuplicate=false;
			if(paiDuplicates[iChildNodeIndex]!=-1)
			{
				bIsChildDuplicate=true;
			}

			//If the child isn't a duplicate particle then 
			//   (i)	add the child particles.
			//  (ii)	add constraints between the child particles.
			// (iii)	add constraints between the parent and child particles.
			//  (iv)	add a capsule bound between parent and child particles for collision detection.
			//   (v)	add bender springs between grandparent and child particles.
			if(!bIsChildDuplicate)
			{
				//Get the translation of the parent that we will apply to the child.
				const Vector3& vTranslation=RCC_VECTOR3(pChild->GetDefaultTranslation());

				//Add the next layer of particles.
				AddParticleLayer
					(iGrandParentNodeIndexNotDuplicated,iParentNodeIndexNotDuplicated,iChildNodeIndexNotDuplicated,
					 iParentNodeIndex,
					 vTranslation,
					 fDefaultDampingRate,fDefaultMass,fDefaultRadius, 
					 archetype);
			}

			//Carry on walking the skeleton.
			WalkSkeleton(paiSkeletonNodeMap,paiDuplicates,pParent,pChild,fDefaultDampingRate,fDefaultMass,fDefaultRadius,archetype);
		}
	}
}

/*
CClothArchetype* CClothArchetypeCreator::Create(const int iType)
{
	CClothArchetype* p=0;
	switch(iType)
	{
	case TYPE_PLAYER_HAIR:
		p=CreatePlayerHair(1.0f,10);
		break;
	case TYPE_PLAYER_ANTENNA:
		p=CreatePlayerAntenna(1.0f,10);
		break;
	case TYPE_PLANT:
		{
			const int iNumParticles=5;
			const float fTotalMass=3.125f;
			const float fBottomMassMultplier=8.0f;//bottomMass=bottomMassMultiplier*topMass
			const float fTopMass=2*fTotalMass/((fBottomMassMultplier+1)*iNumParticles);
			const float fBottomMass=fBottomMassMultplier*fTopMass;
			p=CreatePlantNearPlayer(1.0f,fBottomMass,fTopMass,iNumParticles);
		}
		break;
	case TYPE_PLANT_3D:
		{
			const int iNumParticles=5;
			const float fTotalMass=1.0f;
			const float fBottomMass=1.0f;
			const float fTopMass=0.125f;
			p=CreatePlantNearPlayer3D(1.0f,0.125f,fTotalMass,fBottomMass,fTopMass,iNumParticles);

		}
		break;
	default:
		Assert(false);
		break;
	}
	return p;
}

CClothArchetype* CClothArchetypeCreator::CreatePlayerAntenna(const float UNUSED_PARAM(fLength), const int UNUSED_PARAM(iNumParticles))
{
	//Instantiate the cloth archetype.
	CClothArchetype* pClothArchetype=rage_new CClothArchetype;

	//Set the rest length of the joints.
	const float fRestLength=1.0f;

	//Default mass and damping rate.
	const float fDefaultMass=1.0f;
	const float fDefaultDampingRate=1.0f;
	//Default radius of capsules and particle spheres.
	const float fDefaultRadius=0.2f;

	//Offset of zeroth particle from ped's position.
	Vector3 vOffset(0,-0.4f,0.75f);

	//Set up the zeroth particle.
	Vector3 vPos;
	vPos.Zero();
	pClothArchetype->AddParticle(vPos+vOffset,fDefaultDampingRate,fDefaultMass,fDefaultRadius);

	//Set up the remaining antenna particles.
	vPos=Vector3(0,0,fRestLength);
	pClothArchetype->AddParticle(vPos+vOffset,fDefaultDampingRate,fDefaultMass,fDefaultRadius);
	pClothArchetype->AddConstraint(0,1,fRestLength);
	pClothArchetype->AddCapsuleBound(0,1,fRestLength,0.2f);

	vPos=Vector3(-fRestLength/Sqrtf(2.0f),0,fRestLength + fRestLength/Sqrtf(2.0f));
	pClothArchetype->AddParticle(vPos+vOffset,fDefaultDampingRate,fDefaultMass,fDefaultRadius);
	pClothArchetype->AddConstraint(1,2,fRestLength);
	pClothArchetype->AddCapsuleBound(1,2,fRestLength,0.2f);

	vPos=Vector3(fRestLength/Sqrtf(2.0f),0,fRestLength + fRestLength/Sqrtf(2.0f));
	pClothArchetype->AddParticle(vPos+vOffset,fDefaultDampingRate,fDefaultMass,fDefaultRadius);
	pClothArchetype->AddConstraint(1,3,fRestLength);
	pClothArchetype->AddCapsuleBound(1,3,fRestLength,0.2f);

	//Add the two springs.
	const Vector3& v0=pClothArchetype->GetParticlePosition(0);
	const Vector3& v1=pClothArchetype->GetParticlePosition(1);
	const Vector3& v2=pClothArchetype->GetParticlePosition(2);
	const Vector3& v3=pClothArchetype->GetParticlePosition(3);

	const Vector3 v20=v2-v0;
	const float f20=v20.Mag();
	pClothArchetype->AddSpring(0,2,f20,0.05f);
	const Vector3 v30=v3-v0;
	const float f30=v30.Mag();
	pClothArchetype->AddSpring(0,3,f30,0.05f);

	//Set the zeroth particle to be a pendant attached to the back of the player's head.
	CEntity* pEntity=FindPlayerPed();
	{
		CClothPendantMethodEntityOffset* pMethod1=rage_new CClothPendantMethodEntityOffset(pEntity,v0);
		pClothArchetype->AddPendant(pMethod1,0);
	}
	{
		CClothPendantMethodEntityOffset* pMethod2=rage_new CClothPendantMethodEntityOffset(pEntity,v1);
		pClothArchetype->AddPendant(pMethod2,1);
	}

	//That's us finished.
	return pClothArchetype;
}

CClothArchetype* CClothArchetypeCreator::CreatePlayerHair(const float fLength, const int iNumParticles)
{
	//Instantiate the cloth archetype.
	CClothArchetype* pClothArchetype=rage_new CClothArchetype;

	//Set up offset of zeroth particle to player's transform.
	Vector3 vOffset(0,-0.4f,0.75f);

	//Set up the remaining particles and constraints.
	const float fRestLength=fLength/(1.0f*(iNumParticles-1));
	int i;
	Vector3 vPos(vOffset);
	for(i=0;i<iNumParticles;i++)
	{
		vPos.x = fRestLength*i;
		pClothArchetype->AddParticle(vPos,5.0f,1.0f,0.2f);
	}
	for(i=0;i<(iNumParticles-1);i++)
	{
		const int i0=i;
		const int i1=i+1;
		pClothArchetype->AddConstraint(i0,i1,fRestLength);
		pClothArchetype->AddCapsuleBound(i0,i1,fRestLength,0.2f);
	}
	//Add bender springs to add a bit of stiffness.
	for(i=0;i<(iNumParticles-2);i++)
	{
		const int i0=i;
		const int i2=i+2;
		const Vector3& v0=pClothArchetype->GetParticlePosition(i0);
		const Vector3& v2=pClothArchetype->GetParticlePosition(i2);
		Vector3 vDiff=v0-v2;
		const float fLength=vDiff.Mag();
		pClothArchetype->AddSpring(i0,i2,fLength,0);
	}

	//Set the zeroth particle to be a pendant attached to the back of the player's head.
	{
		const int iParticleId=0;
		CEntity* pEntity=FindPlayerPed();
		CClothPendantMethodEntityOffset* pMethod=rage_new CClothPendantMethodEntityOffset(pEntity,pClothArchetype->GetParticlePosition(iParticleId));
		pClothArchetype->AddPendant(pMethod,iParticleId);
	}
	{
		const int iParticleId=1;
		CEntity* pEntity=FindPlayerPed();
		CClothPendantMethodEntityOffset* pMethod=rage_new CClothPendantMethodEntityOffset(pEntity,pClothArchetype->GetParticlePosition(iParticleId));
		pClothArchetype->AddPendant(pMethod,iParticleId);
	}

	//pClothArchetype->SetParticleInfiniteMass(0);

	//That's us finished.
	return pClothArchetype;
}

CClothArchetype* CClothArchetypeCreator::CreatePlantNearPlayer(const float fLength,const float fMassAtBottom, const float fMassAtTop, const int iNumParticles)
{
	//Instantiate the cloth archetype.
	CClothArchetype* pClothArchetype=rage_new CClothArchetype;

	//Set up offset of zeroth particle to player's transform.
	Vector3 vOffset(-1.0f,1.0f,1.0f);

	//Set the damping rate.
	const float fDampingRate=1.0f;

	//Set up the remaining particles and constraints.
	const float fRestLength=fLength/(1.0f*(iNumParticles-1));
	int i;
	for(i=0;i<iNumParticles;i++)
	{
		const Vector3 vPos=vOffset+Vector3(0,0,fRestLength*i);
		if(0==i || 1==i)
		{
			pClothArchetype->AddParticle(vPos,fDampingRate,-1,0.2f);
		}
		else
		{
			const float fMass=fMassAtBottom + (fMassAtTop-fMassAtBottom)*i/(1.0f*(iNumParticles-1));
			pClothArchetype->AddParticle(vPos,fDampingRate,fMass,0.2f);
		}
	}
	//Add constraints between adjacent particles.
	for(i=0;i<(iNumParticles-1);i++)
	{
		const int i0=i;
		const int i1=i+1;
		pClothArchetype->AddConstraint(i0,i1,fRestLength);
		pClothArchetype->AddCapsuleBound(i0,i1,fRestLength,0.2f);
	}
	//Add bender springs to add a bit of stiffness.
	for(i=0;i<(iNumParticles-2);i++)
	{
		const int i0=i;
		const int i2=i+2;
		const Vector3& v0=pClothArchetype->GetParticlePosition(i0);
		const Vector3& v2=pClothArchetype->GetParticlePosition(i2);
		Vector3 vDiff=v0-v2;
		const float fLength=vDiff.Mag();
		pClothArchetype->AddSpring(i0,i2,fLength,0);
	}

	//That's us finished.
	return pClothArchetype;
}

CClothArchetype* CClothArchetypeCreator::CreatePlantNearPlayer3D
(const float UNUSED_PARAM(fLength), const float UNUSED_PARAM(fWidth), const float UNUSED_PARAM(fTargetMass), const float UNUSED_PARAM(fMassAtBottom), const float UNUSED_PARAM(fMassAtTop), const int UNUSED_PARAM(iNumParticles))
{
	return 0;
}
*/
/*
CClothArchetype* CClothArchetypeCreator::CreatePlantNearPlayer3D
(const float fLength, const float fWidth, const float fTargetMass, const float fMassAtBottom, const float fMassAtTop, const int iNumParticles)
{
	//Instantiate the cloth archetype.
	CClothArchetype* pClothArchetype=rage_new CClothArchetype;

	//Set up offset of zeroth particle to player's transform.
	Vector3 vOffset(-1.0f,1.0f,-0.875f);

	//Set the damping rate.
	const float fDampingRate=1.0f;

	//Set up the remaining particles and constraints.
	const float fRestLength=fLength/(1.0f*(iNumParticles-1));
	const float fHalfWidth=0.5f*fWidth;
	Vector3 avOffsets[4]=
	{
		Vector3(-fHalfWidth,-fHalfWidth,0),
		Vector3(fHalfWidth,-fHalfWidth,0),
		Vector3(fHalfWidth,fHalfWidth,0),
		Vector3(-fHalfWidth,fHalfWidth,0)
	};

	int i;
	float fMassSum=0;
	for(i=0;i<iNumParticles;i++)
	{
		//Add 4 more particles for each vertical level.
		int j;
		for(j=0;j<4;j++)
		{
			const Vector3 vPos=vOffset+avOffsets[j]+Vector3(0,0,fRestLength*i);

			if(0==i || 1==i)
			{
				pClothArchetype->AddParticle(vPos,fDampingRate,-1,fHalfWidth);
			}
			else
			{
				const float fMass=fMassAtBottom + (fMassAtTop-fMassAtBottom)*i/(1.0f*(iNumParticles-1));
				pClothArchetype->AddParticle(vPos,fDampingRate,fMass,fHalfWidth);
				fMassSum+=fMass;
			}
		}
	}
	const float fMassMultiplier=fMassSum/fTargetMass;
	for(i=0;i<iNumParticles;i++)
	{
		const float fInvMass=pClothArchetype->GetParticleInvMass(i);
		pClothArchetype->SetParticleInvMass(i,fInvMass*fMassMultiplier);
	}

	//Add constraints between adjacent particles.
	for(i=0;i<iNumParticles;i++)
	{
		//Add horizontal constraints.
		int j;
		for(j=0;j<4;j++)
		{
			const int j0=(j+0)%4;
			const int j1=(j+1)%4;
			const int iParticle0=i*4+j0;
			const int iParticle1=i*4+j1;
			const Vector3& v0=pClothArchetype->GetParticlePosition(iParticle0);
			const Vector3& v1=pClothArchetype->GetParticlePosition(iParticle1);
			const Vector3 vDiff=v0-v1;
			const float fRestLength=vDiff.Mag();
			pClothArchetype->AddConstraint(iParticle0,iParticle1,fRestLength);
		}
		//
		//{
		//	const int iParticle0=i*4+0;
		//	const int iParticle1=i*4+2;
		//	const Vector3& v0=pClothArchetype->GetParticlePosition(iParticle0);
		//	const Vector3& v1=pClothArchetype->GetParticlePosition(iParticle1);
		//	const Vector3 vDiff=v0-v1;
		//	const float fRestLength=vDiff.Mag();
		//	pClothArchetype->AddConstraint(iParticle0,iParticle1,fRestLength);
		//}
		//{
		//	const int iParticle0=i*4+1;
		//	const int iParticle1=i*4+3;
		//	const Vector3& v0=pClothArchetype->GetParticlePosition(iParticle0);
		//	const Vector3& v1=pClothArchetype->GetParticlePosition(iParticle1);
		//	const Vector3 vDiff=v0-v1;
		//	const float fRestLength=vDiff.Mag();
		//	pClothArchetype->AddConstraint(iParticle0,iParticle1,fRestLength);
		//}
		//
	}
	for(i=0;i<(iNumParticles-1);i++)
	{
		//Add vertical constraints.
		int j;
		for(j=0;j<4;j++)
		{
			const int iParticle0=(i+0)*4+j;
			const int iParticle1=(i+1)*4+j;
			const Vector3& v0=pClothArchetype->GetParticlePosition(iParticle0);
			const Vector3& v1=pClothArchetype->GetParticlePosition(iParticle1);
			const Vector3 vDiff=v0-v1;
			const float fRestLength=vDiff.Mag();
			pClothArchetype->AddConstraint(iParticle0,iParticle1,fRestLength);
		}
		const int iParticleA0=(i+0)*4+(0);
		const int iParticleA1=(i+0)*4+(1);
		const int iParticleA2=(i+0)*4+(2);
		const int iParticleA3=(i+0)*4+(3);
		const int iParticleB0=(i+1)*4+(0);
		const int iParticleB1=(i+1)*4+(1);
		const int iParticleB2=(i+1)*4+(2);
		const int iParticleB3=(i+1)*4+(3);
		pClothArchetype->AddCapsuleBound(
			-1,
			iParticleA0,iParticleA1,iParticleA2,iParticleA3,
			iParticleB0,iParticleB1,iParticleB2,iParticleB3,
			fRestLength,fHalfWidth);
	}
	//
	//for(i=0;i<(iNumParticles-1);i++)
	//{
	//	//Add diagonal constraints
	//	int j;
	//	for(j=0;j<4;j++)
	//	{
	//		const int iParticle0=(i+0)*4+(j+0)%4;
	//		const int iParticle1=(i+1)*4+(j+2)%4;
	//		const Vector3& v0=pClothArchetype->GetParticlePosition(iParticle0);
	//		const Vector3& v1=pClothArchetype->GetParticlePosition(iParticle1);
	//		const Vector3 vDiff=v0-v1;
	//		const float fRestLength=vDiff.Mag();
	//		pClothArchetype->AddConstraint(iParticle0,iParticle1,fRestLength,false);
	//	}
	//}
	//
	//Add bender springs to add a bit of stiffness.
	for(i=0;i<(iNumParticles-2);i++)
	{
		//Add vertical springs.
		int j;
		for(j=0;j<4;j++)
		{
			const int iParticle0=(i+0)*4+j;
			const int iParticle2=(i+2)*4+j;
			const Vector3& v0=pClothArchetype->GetParticlePosition(iParticle0);
			const Vector3& v2=pClothArchetype->GetParticlePosition(iParticle2);
			Vector3 vDiff=v0-v2;
			const float fLength=vDiff.Mag();
			pClothArchetype->AddSpring(iParticle0,iParticle2,fLength,0);
		}
		//Add diagonal springs.
		for(j=0;j<4;j++)
		{
			const int iParticle0=(i+0)*4+(j+0)%4;
			const int iParticle2=(i+2)*4+(j+2)%4;
			const Vector3& v0=pClothArchetype->GetParticlePosition(iParticle0);
			const Vector3& v2=pClothArchetype->GetParticlePosition(iParticle2);
			Vector3 vDiff=v0-v2;
			const float fLength=vDiff.Mag();
			pClothArchetype->AddSpring(iParticle0,iParticle2,fLength,0);
		}
		
	}
	//
	////Add horizontal bender springs.
	//for(i=2;i<iNumParticles;i++)
	//{
	//	{
	//		const int iParticle0=i*4 + 0;
	//		const int iParticle2=i*4 + 2;
	//		const Vector3& v0=pClothArchetype->GetParticlePosition(iParticle0);
	//		const Vector3& v2=pClothArchetype->GetParticlePosition(iParticle2);
	//		Vector3 vDiff=v0-v2;
	//		const float fLength=vDiff.Mag();
	//		pClothArchetype->AddSpring(iParticle0,iParticle2,fLength,0);
	//	}
	//	{
	//		const int iParticle0=i*4 + 1;
	//		const int iParticle2=i*4 + 3;
	//		const Vector3& v0=pClothArchetype->GetParticlePosition(iParticle0);
	//		const Vector3& v2=pClothArchetype->GetParticlePosition(iParticle2);
	//		Vector3 vDiff=v0-v2;
	//		const float fLength=vDiff.Mag();
	//		pClothArchetype->AddSpring(iParticle0,iParticle2,fLength,0);
	//	}
	//}
	//

	//That's us finished.
	return pClothArchetype;
}
*/

#endif //NORTH_CLOTHS
