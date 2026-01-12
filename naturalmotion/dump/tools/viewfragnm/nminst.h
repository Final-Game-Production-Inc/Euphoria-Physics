// 
// nmviewer/nminst.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef NMVIEWER_NMINST_H
#define NMVIEWER_NMINST_H

#include "fragmentnm/instance.h"

namespace rage
{
    class fragType;
    class Matrix34;
    class NMBehaviorInst;
    class phCollider;
    class phContact;
    class phImpact;
    class phImpactData;
}

namespace ragesamples
{

class sampleFragInstNM : public fragInstNM
{
public:
    sampleFragInstNM( const fragType* type, const Matrix34& matrix );

    virtual phInst* PrepareForActivation( phCollider** collider, phInst* otherInst );
	virtual bool PrepareForDeactivation( bool colliderManagedBySim );

    NMBehaviorInst* GetBehaviorInst() const { return m_pBehaviorInst; }
    void SetBehaviorInst( NMBehaviorInst *pBehaviorInst ) { m_pBehaviorInst = pBehaviorInst; }

    bool TriggerOnActivate() const { return m_triggerOnActivate; }
    void SetTriggerOnActivate( bool trigger ) { m_triggerOnActivate = trigger; }

    void CreateAndPostARTMessage();

private:
    NMBehaviorInst *m_pBehaviorInst;
    bool m_triggerOnActivate;
};

} // namespace ragesamples

#endif // NMVIEWER_NMINST_H
