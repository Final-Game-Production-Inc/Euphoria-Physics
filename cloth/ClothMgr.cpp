#include "ClothMgr.h"
#include "Cloth.h"
#include "ClothArchetype.h"
#include "ClothRageInterface.h"
#include "bank/bkmgr.h"
#include "Physics/gtaArchetype.h"
#include "Scene/Entity.h"
#include "Scene/DynamicEntity.h"

#include "grcore/debugdraw.h"
#include "phbound/bound.h"
#include "phbound/boundcapsule.h"
#include "phbound/boundcomposite.h"
#include "physics/simulator.h"
#include "physics/archetype.h"

#if NORTH_CLOTHS

CLOTH_OPTIMISATIONS()

#if __BANK
bool CClothMgr::sm_bRenderDebugParticles=false;
bool CClothMgr:: sm_bRenderDebugConstraints=false;
bool CClothMgr::sm_bRenderDebugSprings=false;
bool CClothMgr::sm_bRenderDebugContacts=false;
bool CClothMgr::ms_bVectoriseClothMaths=true;
#endif

void CClothMgr::Init()
{
	sm_iNumCloths=0;
	for(int i=0;i<MAX_NUM_CLOTHS;i++)
	{
		sm_apCloths[i]=0;
	}
	//Set the updater solver type and its termination conditions.
	sm_clothUpdater.SetSolverTolerance(1e-3f);
	sm_clothUpdater.SetSolverMaxNumIterations(2);
}

void CClothMgr::Shutdown()
{
	for(int i=0;i<sm_iNumCloths;i++)
	{
		if(sm_apCloths[i])
		{
			sm_apCloths[i]->Shutdown();
			delete sm_apCloths[i];
			sm_apCloths[i]=0;
		}
	}
	sm_iNumCloths=0;
}


#if __BANK
void CClothMgr::InitWidgets()
{
	//Create the top-level tuner bank.
	bkBank& bank = BANKMGR.CreateBank("Plant Tuner");

	bank.AddSlider("Plant mass (kg)",&CClothArchetype::sm_fDefaultMass,0.1f,5.0f,0.001f);
	bank.AddSlider("Density at plant tip multiplier",&CClothArchetype::sm_fDefaultTipMassMultiplier,0.001f,1.0f,0.001f);
	bank.AddSlider("Damping rate (s^-1)", &CClothArchetype::sm_fDefaultDampingRate,0.0f,10.0f,0.001f);
	bank.AddSlider("Capsule radius (m)", &CClothArchetype::sm_fDefaultCapsuleRadius,0.01f,0.5f,0.001f);
	bank.AddButton("Refresh plant", &RefreshArchetypeAndAssociatedCloths);
	bank.AddToggle("Debug render particles", &sm_bRenderDebugParticles);
	bank.AddToggle("Debug render constraints", &sm_bRenderDebugConstraints);
	bank.AddToggle("Debug render springs", &sm_bRenderDebugSprings);
	bank.AddToggle("Debug render contacts", &sm_bRenderDebugContacts);
	bank.AddToggle("Vectorise cloth maths", &ms_bVectoriseClothMaths);
}

void CClothMgr::RefreshArchetypeAndAssociatedCloths()
{
	//Assume we're editing the zeroth archetype.
	const int iCurrArchetypeId=0;

	CClothArchetype* pArchetype = CClothArchetype::GetPool()->GetSlot(iCurrArchetypeId);
	if(pArchetype)
	{
		//Update the archetype with the latest default values edited from the widgets.
		pArchetype->RefreshFromDefaults();

		//Now update all the plants that use the archetype.
		int i;
		for(i=0;i<sm_iNumCloths;i++)
		{
			CCloth* pCloth=sm_apCloths[i];
			if(pCloth->GetArchetype()==pArchetype)
			{
				//Set the cloth up again with the latest archetype data.
				//This feeds the edited damping rate, mass, capsule radius into the archetype.
				pCloth->SetArchetype(pArchetype);

				//Resize all the capsule bounds used for collision detection.
				phInstBehaviourCloth* pClothBehaviour=pCloth->GetRageBehaviour();
				phBound* pBound=pClothBehaviour->GetInstance()->GetArchetype()->GetBound();
				Assert(dynamic_cast<phBoundComposite*>(pBound));
				phBoundComposite* pBoundComposite=static_cast<phBoundComposite*>(pBound);
				const int N=pBoundComposite->GetNumBounds();
				int j;
				for(j=0;j<N;j++)
				{
					phBound* pBoundJ=pBoundComposite->GetBound(j);
					Assert(dynamic_cast<phBoundCapsule*>(pBoundJ));
					phBoundCapsule* pBoundJCapsule=static_cast<phBoundCapsule*>(pBoundJ);
					pBoundJCapsule->SetCapsuleRadius(CClothArchetype::sm_fDefaultCapsuleRadius);
				}

				//Need to re-transform cloth back to its old position.
				const Matrix34& startTransform=pCloth->GetTransform();
				pClothBehaviour->SetStartTransform(startTransform);
			}
		}
	}
}
#endif


CCloth* CClothMgr::AddCloth(const CClothArchetype* pClothArchetype, const Matrix34& startTransform)
{
	Assert(pClothArchetype);

	if(pClothArchetype)
	{
		Assertf(sm_iNumCloths<MAX_NUM_CLOTHS, "Too many cloths, can't add another");
		if(sm_iNumCloths<MAX_NUM_CLOTHS)
		{
			//Everything ok so add a new cloth to the cloth array.

			//We'll need to make a cloth, a cloth behaviour and a cloth inst.
			CCloth* pCloth=rage_checked_pool_new(CCloth);
			if(0==pCloth)
			{
				return NULL;
			}
			phInstBehaviourCloth* pInstBehaviourCloth=rage_checked_pool_new(phInstBehaviourCloth) (&sm_clothUpdater,pCloth);
			if(0==pInstBehaviourCloth)
			{
				delete pCloth;
				return NULL;
			}
			phInstCloth* pInstCloth=rage_checked_pool_new(phInstCloth) (pInstBehaviourCloth);
			if(0==pInstCloth)
			{
				delete pCloth;
				delete pInstBehaviourCloth;
				return NULL;
			}

			//Must have successfully instantiated everything we need so add the cloth
			//to the array of active cloths.
			sm_apCloths[sm_iNumCloths]=pCloth;
			sm_iNumCloths++;

			//Just make sure there are no leaks.
			Assert(CCloth::GetPool()->GetNoOfUsedSpaces()==sm_iNumCloths);
			Assert(phInstBehaviourCloth::GetPool()->GetNoOfUsedSpaces()==sm_iNumCloths);
			Assert(phInstCloth::GetPool()->GetNoOfUsedSpaces()==sm_iNumCloths);

			//Instantiate the cloth with its cloth archetype data.
			pCloth->SetArchetype(pClothArchetype);

			//Set up the rage interface and add the cloth to the rage simulator.
			SetUpRageInterface(pCloth,pInstBehaviourCloth,pInstCloth,startTransform);

			//Finished.
			return pCloth;
		}
	}
	

	return NULL;
}

void CClothMgr::RemoveCloth(CCloth* pCloth)
{

	int i;
	for(i=0;i<sm_iNumCloths;i++)
	{
		if(pCloth==sm_apCloths[i])
		{
			//Identified cloth that we want to delete.
			//Perform reverse of AddCloth()

			ClearRageInterface(pCloth);

			//Remove cloth from the array of all cloths.
			int j;
			for(j=i;j<sm_iNumCloths-1;j++)
			{
				sm_apCloths[j]=sm_apCloths[j+1];
			}
			sm_iNumCloths--;

			Assert(pCloth);
			Assert(pCloth->GetRageBehaviour());
			Assert(pCloth->GetRageBehaviour()->GetInstance());
			delete pCloth->GetRageBehaviour()->GetInstance();
			delete pCloth->GetRageBehaviour();
			delete pCloth;

			//Just make sure there are no leaks.
			Assert(CCloth::GetPool()->GetNoOfUsedSpaces()==sm_iNumCloths);
			Assert(phInstBehaviourCloth::GetPool()->GetNoOfUsedSpaces()==sm_iNumCloths);
			Assert(phInstCloth::GetPool()->GetNoOfUsedSpaces()==sm_iNumCloths);

			//Finished.
			break;
		}
	}
}

void CClothMgr::SetUpRageInterface
(CCloth* pCloth, phInstBehaviourCloth* pInstBehaviourCloth, phInstCloth* pInstCloth, const Matrix34& transform)
{
	Assert(pCloth);
	Assert(pInstBehaviourCloth);
	Assert(pInstCloth);

	//The cloth maintains a ptr to the behaviour.
	pCloth->SetRageBehaviour(pInstBehaviourCloth);

	//The behaviour maintains a ptr to the cloth inst.
	pInstBehaviourCloth->SetInstance(*pInstCloth);

	//Set up the cloth archetype and compute a bound for it.
	phArchetype* pClothArch = rage_new rage::phArchetype();
	u32 includeFlags = ArchetypeFlags::GTA_PLANT_INCLUDE_TYPES;
	pClothArch->SetTypeFlags(ArchetypeFlags::GTA_PLANT_TYPE);
	pClothArch->SetIncludeFlags(includeFlags);
	pClothArch->SetBound(pInstBehaviourCloth->CreateBound());

	//Pass the cloth archetype to the cloth inst.
	pInstCloth->SetArchetype(pClothArch);

	//Set the inst transform and stop it being affected by gravity because the 
	//behaviour controls the cloth's dynamics.
	pInstCloth->SetMatrix(MATRIX34_TO_MAT34V(transform));
	PHSIM->SetLastInstanceMatrix(pInstCloth, MATRIX34_TO_MAT34V(transform));
	pInstCloth->SetInstFlag(phInst::FLAG_NO_GRAVITY,true);

	//Initialise the coordinates of the cloth particles.
	Assert(pInstBehaviourCloth);
	pInstBehaviourCloth->SetStartTransform(transform);

	//Now add the cloth to the level and add the cloth behaviour to the simulator.
	bool bDelayAdd=phSimulator::GetDelaySAPAddRemoveEnabled();
	phSimulator::SetDelaySAPAddRemoveEnabled(false);
	PHSIM->AddActiveObject(pInstCloth);
	PHSIM->AddInstBehavior(*pInstBehaviourCloth);
	phSimulator::SetDelaySAPAddRemoveEnabled(bDelayAdd);
	Assertf(PHLEVEL->IsActive(pInstCloth->GetLevelIndex()), "Cloth should be active");
}

void CClothMgr::ClearRageInterface(CCloth* pCloth)
{
	//Get the behaviour and inst and archetype and composite bound attached to the cloth in order to delete them.
	phInstBehaviourCloth* pClothBehaviour=pCloth->GetRageBehaviour();
	if(pClothBehaviour)
	{
		phInstCloth* pClothInst=static_cast<phInstCloth*>(pClothBehaviour->GetInstance());
		Assert(pClothInst);
		phArchetype* pClothArchetype=pClothInst->GetArchetype();
		Assert(pClothArchetype);
		phBoundComposite* pClothBoundComposite=static_cast<phBoundComposite*>(pClothArchetype->GetBound());
		Assert(pClothBehaviour);

		//Delete behaviour and inst and archetype and all bounds in the composite bound.
		Assert(pClothInst->IsInLevel());
		int j;
		for(j=0;j<pClothBoundComposite->GetNumBounds();j++)
		{
			phBound* pBound=pClothBoundComposite->GetBound(j);
			pBound->Release();
		}
		pClothBoundComposite->Release();
		pClothArchetype->Release();
		pClothInst->GetArchetype()->Release();
		PHSIM->RemoveInstBehavior(*pClothBehaviour);
		PHSIM->DeleteObject(pClothInst->GetLevelIndex());
	}
}

CClothUpdater CClothMgr::sm_clothUpdater;
int CClothMgr::sm_iNumCloths;
CCloth* CClothMgr::sm_apCloths[MAX_NUM_CLOTHS];

void CClothMgr::RenderDebug()
{
#if DEBUG_DRAW
#if __BANK
	for(int i=0;i<sm_iNumCloths;i++)
	{
		const CCloth* pCloth=sm_apCloths[i];
		const bool bIsAsleep=pCloth->IsAsleep();
		Assertf(pCloth, "Null cloth ptr");

		if(sm_bRenderDebugParticles)
		{
			const float fRadius=0.01f;
			const CClothParticles& particles=pCloth->GetParticles();
			Vector3 vOffset(0,0,0);
			Color32 red(1.0f,0.0f,0.0f);
			Color32 green(0.0f,1.0f,0.0f);
			Color32 blue(0.0f,0.0f,1.0f);
			for(int i=0;i<particles.GetNumParticles();i++)
			{
				const Vector3& vPos=particles.GetPosition(i);
				if(bIsAsleep)
				{
					grcDebugDraw::Sphere(vPos+vOffset,fRadius,blue);
				}
				else
				{
					if(0==i)
					{
						grcDebugDraw::Sphere(vPos+vOffset,fRadius,green);
					}
					else
					{
						grcDebugDraw::Sphere(vPos+vOffset,fRadius,red);
					}
				}
			}
		}

		if(sm_bRenderDebugConstraints)
		{
			const CClothParticles& particles=pCloth->GetParticles();
			const CClothSeparationConstraints& constraints=pCloth->GetSeparationConstraints();
			Vector3 vOffset(0,0,0);
			Color32 blue(0.0f,0.0f,1.0f);
			for(int i=0;i<constraints.GetNumConstraints();i++)
			{
				const Vector3& vA=particles.GetPosition(constraints.GetParticleIndexA(i));
				const Vector3& vB=particles.GetPosition(constraints.GetParticleIndexB(i));
				grcDebugDraw::Line(vA+vOffset,vB+vOffset,blue);
			}
		}

		if(sm_bRenderDebugSprings)
		{
			const CClothParticles& particles=pCloth->GetParticles();
			const CClothSprings& springs=pCloth->GetSprings();
			Vector3 vOffset(0,0,0);
			Color32 purple(1.0f,0.0f,1.0f);
			for(int i=0;i<springs.GetNumSprings();i++)
			{
				const Vector3& vA=particles.GetPosition(springs.GetParticleIdA(i));
				const Vector3& vB=particles.GetPosition(springs.GetParticleIdB(i));
				grcDebugDraw::Line(vA+vOffset,vB+vOffset,purple);
			}
		}
	}
#endif// __BANK
#endif // DEBUG_DRAW
}

#if RAGE_CLOTHS

#include "cloth/environmentCloth.h"
#include "pheffects/cloth_verlet.h"
#include "phcore/phmath.h"

int CClothRageMgr::sm_iNumRageCloths=0;
phVerletCloth* CClothRageMgr::sm_apRageCloths[MAX_NUM_RAGE_CLOTHS];

void CClothRageMgr::Shutdown()
{
	for(int i=0;i<sm_iNumRageCloths;i++)
	{
		if(sm_apRageCloths[i])
		{
			//sm_apRageCloths[i]->Shutdown();
			ClearRageInterface(sm_apRageCloths[i]);
			delete sm_apRageCloths[i];
			sm_apRageCloths[i]=0;
		}
	}
	sm_iNumRageCloths=0;
}

void CClothRageMgr::AddClothStrand(const Vector3& position, const Vector3& rotation, float length, float radius, int numEdges)
{
	const float density = 1.0f;
	const float stretchStrength = 200.0f;
	const float stretchDamping = 20.0f;
	const float airFriction = 0.1f;
	const float bendStrength = 1.0f;
	const float bendDamping = 1.0f;
	AddBender(position,rotation,NULL,length,radius,radius,numEdges,density,stretchStrength,stretchDamping,airFriction,bendStrength,bendDamping);
}


void CClothRageMgr::AddBender(const Vector3& endPosition, const Vector3& rotation, const char* boundName, float length, float startRadius, float endRadius, int numEdges,
							  float density, float stretchStrength, float stretchDamping, float airFriction, float bendStrength, float bendDamping, bool held)
{
	// Create a rope and its matrix pose.
	environmentCloth& newBender = *rage_new environmentCloth;
	Matrix34 ropePose(CreateRotatedMatrix(endPosition,rotation));

	// Set the rope vertex positions.
	int numVertices = numEdges+1;
	Vector3* vertices = Alloca(Vector3,numVertices);
	Vector3 vertexPosition(ORIGIN);
	float edgeLength = length/(float)numEdges;
	for (int vertexIndex=0; vertexIndex<numVertices; vertexIndex++)
	{
		ropePose.Transform(vertexPosition,vertices[vertexIndex]);
		vertexPosition.x += edgeLength;
	}

	// Initialize one-dimensional cloth.
	newBender.AllocateEnvironmentCloth();
	//m_Bender[m_NumPieces] = static_cast<phEnvClothVerletBehavior*>(newBender.GetBehavior());
	phVerletCloth* cloth = newBender.GetCloth();
	atArray<float> bendStrengthList;
	bendStrengthList.Resize(numVertices);
	for (int i=0; i<numVertices; i++) bendStrengthList[i]=bendStrength;
	cloth->InitStrand(vertices,numVertices,(bendStrength>0.0f ? &bendStrengthList[0] : NULL));
	newBender.CreatePhysics();
	cloth->InitCollisionData();

	cloth->InitEdgeData();
	if (held)
	{
		cloth->PinVertex(0);
		cloth->PinVertex(1);
	}

//	cloth->SetEdgeWeightsFromPinning();

	stretchStrength += 0.0f;
	stretchDamping += 0.0f;
	airFriction += 0.0f;
	bendStrength += 0.0f;
	bendDamping += 0.0f;
	density += 0.0f;
	endRadius += 0.0f;

	phBoundComposite* compositeBound = NULL;
	if (boundName)
	{
		// Load the specified bound file.
		phBound* bound = phBound::Load(boundName);
		if (bound->GetType()==phBound::COMPOSITE)
		{
			compositeBound = static_cast<phBoundComposite*>(bound);
		}
	}

	if (!compositeBound)
	{
		// No bound file was given, or it failed to load, so create a composite bound for the rope.
		compositeBound = &newBender.CreateRopeBound(numEdges,length,startRadius,endRadius);
	}

	phArchetypePhys& archetype = *(rage_new phArchetypePhys);
	archetype.SetBound(compositeBound);

	//m_BendingObject[m_NumPieces] = rage_new phSwingingObject;

	//phSwingingObject& benderObject = *m_BendingObject[m_NumPieces];

	Matrix34 matrix(CreateRotatedMatrix(endPosition,rotation));

	matrix.d.AddScaled(matrix.a,0.5f*length);
	//benderObject.SetInst(const_cast<phInst*>(newBender.GetInstance()));
	//benderObject.InitPhys(&archetype,matrix);
	phInst* pInst=const_cast<phInst*>(newBender.GetInstance());
	pInst->SetArchetype(&archetype);
	pInst->SetMatrix( *(const Mat34V*)(&matrix) );	
	//pInst->SetLastMatrix(matrix);


	//const int bucket = 0;
	//const float blur = 1.0f;
	//benderObject.SetDrawable(rage_new rmcRopeBasedModelDrawable(m_model,m_Bender[m_NumPieces],bucket,blur));
	//demoWorld.AddInactiveObject(&benderObject);
	PHSIM->AddInactiveObject(pInst);
	//demoWorld.AddUpdateObject(&benderObject);
	//demoWorld.GetSimulator()->AddInstBehavior(*newBender.GetBehavior());
	PHSIM->AddInstBehavior(*newBender.GetBehavior());
	//m_IsControlled[m_NumPieces] = false;
	//m_DemoWorld[m_NumPieces] = &demoWorld;
	//m_Constraint[m_NumPieces] = NULL;
	//return (m_NumPieces++);

	Assert(sm_iNumRageCloths<MAX_NUM_RAGE_CLOTHS);
	sm_apRageCloths[sm_iNumRageCloths]=cloth;
	sm_iNumRageCloths++;
}

void CClothRageMgr::RenderDebug()
{
#if DEBUG_DRAW

	Color32 red(1.0f,0.0f,0.0f);
	Color32 green(0.0f,1.0f,0.0f);
	Color32 blue(0.0f,0.0f,1.0f);

	for(int i=0;i<sm_iNumRageCloths;i++)
	{
		const phVerletCloth* pCloth=sm_apRageCloths[i];
		const phClothData& clothData=pCloth->GetClothData();
		const int iNumEdges=clothData.GetNumEdges();
		const int iNumVerts=clothData.GetNumVertices();

		const atArray<phEdgeData>& edges = pCloth->GetEdgeList();
		int j;
		for(j=0;j<iNumEdges;j++)
		{
			const int iVertA=edges[j].m_vertIndices[0];
			const int iVertB=edges[j].m_vertIndices[1];
			const Vector3& vA=clothData.GetVertexPosition(iVertA);
			const Vector3& vB=clothData.GetVertexPosition(iVertB);
			grcDebugDraw::Line(vA,vB,blue);
		}
		for(j=0;j<iNumVerts;j++)
		{
			const Vector3& vPos=clothData.GetVertexPosition(j);
			grcDebugDraw::Sphere(vPos,0.05f,red);
		}
	}
#endif // DEBUG_DRAW
}

#endif

#endif //NORTH_CLOTHS


