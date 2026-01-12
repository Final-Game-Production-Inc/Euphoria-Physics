#define NORTH_CLOTHS 0

#ifndef CLOTH_MGR_H
#define CLOTH_MGR_H

#include "ClothUpdater.h"

#if NORTH_CLOTHS

class CCloth;
class CClothArchetype;

class CClothMgr
{
public:

	enum
	{
		MAX_NUM_CLOTHS=64,
		MAX_NUM_CLOTH_ARCHETYPES=8
	};

	CClothMgr()
	{
	}
	~CClothMgr()
	{
	}

	static void Init();
	static void Shutdown();

	static void RenderDebug();

	//static void Update();

	static void SetSolverMaxNumIterations(const int iMaxIter)
	{
		sm_clothUpdater.SetSolverMaxNumIterations(iMaxIter);
	}
	static void SetSolverTolerance(const float fTolerance)
	{
		sm_clothUpdater.SetSolverTolerance(fTolerance);
	}

	static CCloth* AddCloth(const CClothArchetype* pClothArchetype,  const Matrix34& startTransform);	
	static void RemoveCloth(CCloth* pCloth);
	static int GetNumCloths() {return sm_iNumCloths;}

	static CClothUpdater& GetUpdater()
	{
		return sm_clothUpdater;
	}


#if __BANK
	static void InitWidgets();
	static bool sm_bRenderDebugParticles;
	static bool sm_bRenderDebugConstraints;
	static bool sm_bRenderDebugSprings;
	static bool sm_bRenderDebugContacts;
	static bool ms_bVectoriseClothMaths;
	static void RefreshArchetypeAndAssociatedCloths();
#endif

private:

	static CClothUpdater sm_clothUpdater;
	static int sm_iNumCloths;
	static CCloth* sm_apCloths[MAX_NUM_CLOTHS];

	static void SetUpRageInterface(CCloth* pCloth, class phInstBehaviourCloth* pInstBehaviourCloth, class phInstCloth* pInstCloth, const Matrix34& transform);
	static void ClearRageInterface(CCloth* pCloth);
};

#endif //NORTH_CLOTHS

#define RAGE_CLOTHS 0

#if RAGE_CLOTHS
class CClothRageMgr
{
public:

	enum
	{
		MAX_NUM_RAGE_CLOTHS=64
	};

	CClothRageMgr()
	{
	}
	~CClothRageMgr()
	{
	}

	static void Init()
	{
		sm_iNumRageCloths=0;
		for(int i=0;i<MAX_NUM_RAGE_CLOTHS;i++)
		{
			sm_apRageCloths[i]=0;
		}
	}
	static void Shutdown();

	static void RenderDebug();

	static int GetNumCloths() {return sm_iNumRageCloths;}

	static void AddClothStrand(const Vector3& position, const Vector3& rotation, float length, float radius, int numEdges);

private:

	static int sm_iNumRageCloths;
	static phVerletCloth* sm_apRageCloths[MAX_NUM_RAGE_CLOTHS];

	static void  AddBender(const Vector3& endPosition, const Vector3& rotation, const char* boundName=NULL, float length=2.0f, float startRadius=0.02f, float endRadius=0.01f, int numEdges=12,
		float density=1.0f, float stretchStrength=200.0f, float stretchDamping=20.0f, float airFriction=0.1f, float bendStrength=60.0f, float bendDamping=6.0f, bool held=true);

};
#endif //RAGE_CLOTHS


#endif //CLOTH_MGR_H
