#ifndef CLOTH_CONSTRAINT_GRAPH_H
#define CLOTH_CONSTRAINT_GRAPH_H

#include "Debug/Debug.h"

class ConstraintGroup;

class CClothConstraintGraphElement
{
public:

	CClothConstraintGraphElement()
		: m_iConstraintIndex(-1),
	      m_iParticleAId(-1),
		  m_iParticleBId(-1),
		  m_iParticleCId(-1),
		  m_iParticleDId(-1),
		  m_fInvMassA(-1),
		  m_fInvMassB(-1),
		  m_fInvMassC(-1),
		  m_fInvMassD(-1),
		  m_fInvMassMultiplierA(1.0f),
		  m_fInvMassMultiplierB(1.0f),
		  m_fInvMassMultiplierC(1.0f),
		  m_fInvMassMultiplierD(1.0f)

	{
	}
	~CClothConstraintGraphElement()
	{
		m_iConstraintIndex=-1;;
		m_iParticleAId=-1;
		m_iParticleBId=-1;
		m_iParticleCId=-1;
		m_iParticleDId=-1;
		m_fInvMassA=-1;
		m_fInvMassB=-1;
		m_fInvMassC=-1;
		m_fInvMassD=-1;
		m_fInvMassMultiplierA=1.0f;
		m_fInvMassMultiplierB=1.0f;
		m_fInvMassMultiplierC=1.0f;
		m_fInvMassMultiplierD=1.0f;
	}

	void Set(const int iConstraintIndex, 
			 const int iParticleA, const int iParticleB, 
			 const float fInvMassA, const float fInvMassB)
	{
		m_iConstraintIndex=(s16)iConstraintIndex;
		m_iParticleAId=(s16)iParticleA;
		m_iParticleBId=(s16)iParticleB;
		m_iParticleCId=-1;
		m_iParticleDId=-1;
		m_fInvMassA=fInvMassA;
		m_fInvMassB=fInvMassB;
		m_fInvMassC=-1;
		m_fInvMassD=-1;
		m_fInvMassMultiplierA=(0==m_fInvMassA ? 0.0f : 1.0f);
		m_fInvMassMultiplierB=(0==m_fInvMassB ? 0.0f : 1.0f);
		m_fInvMassMultiplierC=1.0f;
		m_fInvMassMultiplierD=1.0f;
		Assert(1.0f==m_fInvMassMultiplierA || 1.0f==m_fInvMassMultiplierB);
	}

	void Set(const int iConstraintIndex, 
			 const int iParticleA, const int iParticleB, const int iParticleC, const int iParticleD, 
			 const float fInvMassA, const float fInvMassB, const float fInvMassC, const float fInvMassD)
	{
		m_iConstraintIndex=(s16)iConstraintIndex;
		m_iParticleAId=(s16)iParticleA;
		m_iParticleBId=(s16)iParticleB;
		m_iParticleCId=(s16)iParticleC;
		m_iParticleDId=(s16)iParticleD;
		m_fInvMassA=fInvMassA;
		m_fInvMassB=fInvMassB;
		m_fInvMassC=fInvMassC;
		m_fInvMassD=fInvMassD;
		m_fInvMassMultiplierA=(0==m_fInvMassA ? 0.0f : 1.0f);
		m_fInvMassMultiplierB=(0==m_fInvMassB ? 0.0f : 1.0f);
		m_fInvMassMultiplierC=(0==m_fInvMassC ? 0.0f : 1.0f);
		m_fInvMassMultiplierD=(0==m_fInvMassD ? 0.0f : 1.0f);
		Assert(1.0f==m_fInvMassMultiplierA || 1.0f==m_fInvMassMultiplierB || 1.0f==m_fInvMassMultiplierC || 1.0f==m_fInvMassMultiplierD); 
	}

	int GetConstraintIndex() const 
	{
		return m_iConstraintIndex;
	}
	int GetParticleA() const
	{
		return m_iParticleAId;
	}
	int GetParticleB() const
	{
		return m_iParticleBId;
	}
	int GetParticleC() const
	{
		return m_iParticleCId;
	}
	int GetParticleD() const
	{
		return m_iParticleDId;
	}
	float GetParticleAInverseMassMultiplier() const
	{
		return m_fInvMassMultiplierA;
	}
	float GetParticleBInverseMassMultiplier() const
	{
		return m_fInvMassMultiplierB;
	}
	float GetParticleCInverseMassMultiplier() const
	{
		return m_fInvMassMultiplierC;
	}
	float GetParticleDInverseMassMultiplier() const
	{
		return m_fInvMassMultiplierD;
	}

	bool CouplesToParticle(const int iParticleId) const
	{
		return ((GetParticleA()==iParticleId) || (GetParticleB()==iParticleId) || (GetParticleC()==iParticleId) || (GetParticleD()==iParticleId));
	}

	void GetOtherParticles(const int iParticleId, int& iParticle1, int& iParticle2, int& iParticle3) const
	{
		Assert(CouplesToParticle(iParticleId));
		if(GetParticleA()==iParticleId)
		{
			iParticle1=GetParticleB();
			iParticle2=GetParticleC();
			iParticle3=GetParticleD();
		}
		else if(GetParticleB()==iParticleId)
		{
			iParticle1=GetParticleA();
			iParticle2=GetParticleC();
			iParticle3=GetParticleD();		
		}
		else if(GetParticleC()==iParticleId)
		{
			iParticle1=GetParticleA();
			iParticle2=GetParticleB();
			iParticle3=GetParticleD();		
		}
		else if(GetParticleD()==iParticleId)
		{
			iParticle1=GetParticleA();
			iParticle2=GetParticleB();
			iParticle3=GetParticleC();		
		}
		else
		{
			Assert(false);
			iParticle1=-1;
			iParticle2=-1;
			iParticle3=-1;
		}
	}

	bool IsRootConstraint() const  
	{
		return (0==m_fInvMassA || 0==m_fInvMassB || 0==m_fInvMassC || 0==m_fInvMassD);
	}

	int GetRootParticle() const
	{
		Assert(IsRootConstraint());
		if(0==m_fInvMassA)
		{
			return GetParticleA();
		}
		else if(0==m_fInvMassB)
		{
			Assert(0==m_fInvMassB);
			return GetParticleB();
		}
		else if(0==m_fInvMassC)
		{
			Assert(0==m_fInvMassC);
			return GetParticleC();
		}
		else
		{
			Assert(0==m_fInvMassD);
			return GetParticleD();
		}
	}

	void SetInfiniteMass(const int iParticleId, const bool ASSERT_ONLY(bTreatInfiniteMass))
	{
		Assert(bTreatInfiniteMass);
		Assert(CouplesToParticle(iParticleId));
		if(GetParticleA()==iParticleId)
		{
			m_fInvMassMultiplierA=0.0f;
		}
		else if(GetParticleB()==iParticleId)
		{
			m_fInvMassMultiplierB=0.0f;
		}
		else if(GetParticleC()==iParticleId)
		{
			m_fInvMassMultiplierC=0.0f;
		}
		else if(GetParticleD()==iParticleId)
		{
			m_fInvMassMultiplierD=0.0f;
		}
		Assert(1.0f==m_fInvMassMultiplierA || 1.0f==m_fInvMassMultiplierB || 1.0f==m_fInvMassMultiplierC || 1.0f==m_fInvMassMultiplierD);
	}

private:
	
	float m_fInvMassA;
	float m_fInvMassB;
	float m_fInvMassC;
	float m_fInvMassD;
	float m_fInvMassMultiplierA;//0 for infinite mass particles, 1 otherwise.
	float m_fInvMassMultiplierB;//0 for infinite mass particles, 1 otherwise.
	float m_fInvMassMultiplierC;//0 for infinite mass particles, 1 otherwise.
	float m_fInvMassMultiplierD;//0 for infinite mass particles, 1 otherwise.
	s16 m_iConstraintIndex;
	s16 m_iParticleAId;
	s16 m_iParticleBId;
	s16 m_iParticleCId;
	s16 m_iParticleDId;
};

class CClothConstraintGraph
{
public:

	CClothConstraintGraph()
		: m_iNumConstraints(0),
		  m_iNumLevels(0)
	{
	}
	~CClothConstraintGraph()
	{
	}

	//Reset and add constraints before computing the graph.
	void Reset()
	{
		m_iNumConstraints=0;
		m_iNumLevels=0;
	}
	void AddConstraint(const int iConstraintIndex, const int iParticleA, const int iParticleB, const float fInvMassA, const float fInvMassB)
	{
		Assert(m_iNumConstraints<MAX_NUM_CONSTRAINTS);
		if(m_iNumConstraints<MAX_NUM_CONSTRAINTS)
		{
			m_aConstraints[m_iNumConstraints].Set(iConstraintIndex,iParticleA,iParticleB,fInvMassA,fInvMassB);
			m_iNumConstraints++;
		}
	}
	//Compute the graph before iterating over the constraints in a directed manner from root to tips. 
	void ComputeGraph();

	//Traverse the graph from root to tips.
	//Get the number of levels in the graph.
	int GetNumLevels() const
	{
		return m_iNumLevels;
	}
	//Get the number of constraints in a particular level.
	int GetNumConstraints(const int iLevel) const
	{
		return m_aiLevelSizes[iLevel];
	}
	//Get the two bodies of the jth Constraint of the ith level and determine if either is to be 
	//treated as infinite mass.
	void GetConstraintBodies(const int iLevel, const int jConstraint,  int& iConstraintIndex, int& iParticleA, int& iParticleB, float& fInverseMassAMultiplier, float& fInverseMassBMultiplier) const
	{
		Assert(iLevel<m_iNumLevels);
		Assert(jConstraint<m_aiLevelSizes[iLevel]);
		const int index=m_aiLevelOffsets[iLevel]+jConstraint;
		iConstraintIndex=m_apOrderedConstraints[index]->GetConstraintIndex();
		iParticleA=m_apOrderedConstraints[index]->GetParticleA();
		iParticleB=m_apOrderedConstraints[index]->GetParticleB();
		fInverseMassAMultiplier=m_apOrderedConstraints[index]->GetParticleAInverseMassMultiplier();
		fInverseMassBMultiplier=m_apOrderedConstraints[index]->GetParticleBInverseMassMultiplier();
	}
	int GetConstraintIndex(const int iLevel, const int jConstraint) const
	{
		Assert(iLevel<m_iNumLevels);
		Assert(jConstraint<m_aiLevelSizes[iLevel]);
		const int index=m_aiLevelOffsets[iLevel]+jConstraint;
		return m_apOrderedConstraints[index]->GetConstraintIndex();
	}
	int GetParticleIndexA(const int iLevel, const int jConstraint) const
	{
		Assert(iLevel<m_iNumLevels);
		Assert(jConstraint<m_aiLevelSizes[iLevel]);
		const int index=m_aiLevelOffsets[iLevel]+jConstraint;
		return m_apOrderedConstraints[index]->GetParticleA();
	}
	int GetParticleIndexB(const int iLevel, const int jConstraint) const
	{
		Assert(iLevel<m_iNumLevels);
		Assert(jConstraint<m_aiLevelSizes[iLevel]);
		const int index=m_aiLevelOffsets[iLevel]+jConstraint;
		return m_apOrderedConstraints[index]->GetParticleB();
	}

	bool IsLegal() const;

private:

	enum
	{
		MAX_NUM_CONSTRAINTS=512
	};
	int m_iNumConstraints;
	CClothConstraintGraphElement m_aConstraints[MAX_NUM_CONSTRAINTS];

	int m_iNumLevels;
	int m_aiLevelSizes[MAX_NUM_CONSTRAINTS];
	int m_aiLevelOffsets[MAX_NUM_CONSTRAINTS];
	const CClothConstraintGraphElement* m_apOrderedConstraints[MAX_NUM_CONSTRAINTS];

	enum
	{
		MAX_NUM_BODIES=512
	};


	void ComputeRootParticles(CClothConstraintGraphElement** papConstraints, int iNumConstraints, int* paiParticleIds, int& iNumParticles) const;

	void FillArrayHoles(CClothConstraintGraphElement** papConstraint, const int iArraySize, int& iNewArraySize) const;
};

#endif //_Constraint_GRAPH_H
