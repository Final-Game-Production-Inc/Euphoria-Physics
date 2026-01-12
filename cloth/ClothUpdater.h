#ifndef CLOTH_UPDATER_H
#define CLOTH_UPDATER_H

class CCloth;
#include "ClothParticles.h"
#include "ClothConstraints.h"
#include "ClothContacts.h"
#include "ClothConstraintGraph.h"
#include "ClothMgr.h"
#include "ClothSprings.h"

#include "phBound/boundCuller.h"

#if NORTH_CLOTHS

class CClothUpdater
{
public:

	friend class CClothMgr;
	friend class phInstBehaviourCloth;

private:

	CClothUpdater();
	~CClothUpdater();

	void Init();
	void Shutdown();

	void UpdateBallisticPhase(CCloth& cloth, const float dt);
	void UpdateBallisticPhaseV(CCloth& cloth, const float dt);
	void UpdateSprings(CCloth& cloth, const float dt);
	void UpdateSpringsV(CCloth& cloth, const float dt);
	void HandleConstraintsAndContacts(CCloth& cloth, const float dt);
	void HandleConstraintsAndContactsV(CCloth& cloth, const float dt);
	bool TestIsMoving(const CCloth& cloth, const float dt) const;

	void SetSolverMaxNumIterations(const int iMaxIter);
	void SetSolverTolerance(const float fTolerance);

	//Solver owns an array of contacts to be reused in the update stage of each cloth.
	//Need to be able to get access to the array of contacts.
	CClothContacts& GetContacts() {return m_contacts;}

	//Return arrays that are used as workspace for shape tests.
	phPolygon::Index* GetCulledPolysArrayPtr() {return culledPolys;}
	u16 GetCulledPolysArraySize() const {return DEFAULT_MAX_CULLED_POLYS;}

private:

	//Need these to update ballistic phase of particles.
	//Array of particle forces.
	Vector3 m_avForceTimesInvMasses[MAX_NUM_CLOTH_PARTICLES];
	float m_afInvMasses[MAX_NUM_CLOTH_PARTICLES];
	float m_afDampingRateTimesInvMasses[MAX_NUM_CLOTH_PARTICLES];

	//Need to apply contacts to particle positions.
	//Array of contacts reused for each cloth.
	CClothContacts m_contacts;
	//Need these to do intersections with shape tests.
	phPolygon::Index culledPolys[DEFAULT_MAX_CULLED_POLYS];

	int m_iMaxNumSolverIterations;
	float m_fSolverTolerance;

	struct TSepConstraintData
	{
		float m_afStrengthA[MAX_NUM_CLOTH_SEPARATION_CONSTRAINTS+MAX_NUM_CLOTH_SPRINGS];
		float m_afStrengthB[MAX_NUM_CLOTH_SEPARATION_CONSTRAINTS+MAX_NUM_CLOTH_SPRINGS];
	};
	struct TVolConstraintData
	{
	};

	TSepConstraintData m_sepConstraintData;
	TVolConstraintData m_volConstraintData;
	int m_aiActiveSepConstraintIndices[MAX_NUM_CLOTH_SEPARATION_CONSTRAINTS+MAX_NUM_CLOTH_SPRINGS];
	int m_aiActiveVolConstraintIndices[MAX_NUM_CLOTH_VOLUME_CONSTRAINTS];
	CClothConstraintGraph m_graph;
};

#endif //NORTH_CLOTHS

#endif //CLOTH_UPDATER_H
