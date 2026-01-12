//
// pheffects/clothverletinst.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHEFFECTS_CLOTH_VERLET_INST_H
#define PHEFFECTS_CLOTH_VERLET_INST_H

#include "cloth_verlet.h"

#include "physics/inst.h"
#include "physics/instbehavior.h"
#include "system/taskheader.h"

namespace rage
{

#define CLOTH_MAX_COLLISION_OBJECTS 16


class phClothVerletBehavior : public phInstBehavior
{
public:
	phClothVerletBehavior();
	~phClothVerletBehavior();

	DECLARE_PLACE(phClothVerletBehavior);
	phClothVerletBehavior(class datResource& rsc);

#if __DECLARESTRUCT
	void DeclareStruct(datTypeStruct &s);
#endif	

	virtual void Update (float /*TimeStep*/) {}				
	virtual void Reset() { m_CollisionInst.Resize(0); }
	virtual bool IsActive() const { return true; }		
	virtual bool CollideObjects(Vec::V3Param128 /*timeStep*/, phInst* /*myInst*/, phCollider* /*myCollider*/, phInst* /*otherInst*/, phCollider* /*otherCollider*/, phInstBehavior* /*otherInstBehavior*/) { return false; }

	bool UpdateVerletBound(phVerletCloth* cloth);
	void UpdateRopeBound(phVerletCloth* cloth, const phInst* clothInstance=NULL);
	void SetUserDataAddress(u32 userDataAddr) { m_UserDataAddress = userDataAddr; }
	void SetActivateOnHit(bool activeonhit)	{ m_ActivateOnHit = activeonhit; }
	void SetActivateOnHitOverridden(bool overrideActivateOnHit) { m_ActivateOnHitOverridden = overrideActivateOnHit; }
	bool ActivateWhenHit() const { return m_ActivateOnHit; }
	bool IsMotionSeparated() const { return m_ActivateOnHit; }	
	bool IsActivateOnHitOverridden() const { return m_ActivateOnHitOverridden; }	
	u32  GetUserDataAddress() const { return m_UserDataAddress; }

	phInstDatRefArray& GetCollisionInstRef() { return m_CollisionInst; }

private:
	u32	 m_UserDataAddress;				// used only on the SPU
	phInstDatRefArray m_CollisionInst; 

	ATTR_UNUSED char m_padding1[10];	
	bool m_ActivateOnHit;				// if true then linear motion is separated from angular motion
	bool m_ActivateOnHitOverridden;		
};

}  // namespace rage

#endif // PHEFFECTS_CLOTH_VERLET_INST_H
