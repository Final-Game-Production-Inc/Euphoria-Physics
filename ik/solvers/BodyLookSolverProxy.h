// 
// ik/solvers/BodyLookSolverProxy.h
// 
// Copyright (C) 2013 Rockstar Games.  All Rights Reserved.
// 

#ifndef BODYLOOKSOLVERPROXY_H
#define BODYLOOKSOLVERPROXY_H

#include "SolverProxy.h"

#include "fwtl/pool.h"
#include "ik/IkRequest.h"
#include "scene/RegdRefTypes.h"

class CPed;

class CBodyLookIkSolverProxy : public CIkSolverProxy
{
	friend class CBodyLookIkSolver;

public:
	CBodyLookIkSolverProxy(CPed* pPed);
	~CBodyLookIkSolverProxy();

	FW_REGISTER_CLASS_POOL(CBodyLookIkSolverProxy);

	void Reset();
	void Iterate(float dt, crSkeleton& skeleton);

	void PreIkUpdate(float deltaTime);
	void PostIkUpdate(float deltaTime);

	bool AllocateSolver(CPed* pPed);

	void Request(const CIkRequestBodyLook& request);

	void Abort();
	bool IsAborted() const									{ return m_bAbort; }

	void SetActive(bool bActive)							{ m_bActive = bActive; }
	bool IsActive() const									{ return m_bActive; }

	const CEntity* GetTargetEntity() const					{ return m_pTargetEntity.Get(); }
	bool IsTargetEntityValid() const						{ return m_bTargetEntityValid; }
	Vec3V_Out GetTargetPosition() const						{ return m_vTargetPosition; }

	u32 GetHashKey() const									{ return m_Request.GetHashKey(); }
	u32 GetFlags() const									{ return m_Request.GetFlags(); }
	s32 GetLookAtFlags() const								{ return m_Request.GetLookAtFlags(); }
	s32 GetPriority() const									{ return (s32)m_Request.GetRequestPriority(); }
	u16 GetHoldTime() const									{ return m_Request.GetHoldTime(); }

	u8 GetTurnRate() const;
	u8 GetHeadRotLim(LookIkRotationAngle rotAngle) const;

#if __IK_DEV
	void DebugRender();
#endif //__IK_DEV

private:
	void UpdateTargetPosition();
	void UpdateActiveState();

	CIkRequestBodyLook	m_Request;

	Vec3V				m_vTargetPosition;

	RegdConstEnt		m_pTargetEntity;
	CPed*				m_pPed;

	u16					m_uCurrTime;

	u16					m_bActive				: 1;
	u16					m_bAbort				: 1;
	u16					m_bTargetEntityValid	: 1;
};

#endif // BODYLOOKSOLVERPROXY_H
