// 
// nmviewer/nminst.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#include "nminst.h"

#include "fragment/type.h"
#include "fragmentnm/messageparams.h"
#include "fragmentnm/nmbehavior.h"
#include "physics/inst.h"
#include "vector/matrix34.h"


using namespace rage;

namespace ragesamples
{

sampleFragInstNM::sampleFragInstNM( const fragType* type, const Matrix34& matrix )
: fragInstNM(type, matrix)
, m_pBehaviorInst(NULL)
, m_triggerOnActivate(true)
{
}

static int s_iCount = 0;

phInst* sampleFragInstNM::PrepareForActivation( phCollider** collider, phInst* otherInst, const phConstraintBase * constraint )
{
	++s_iCount;

	phInst* result = fragInstNM::PrepareForActivation( collider, otherInst, constraint );

	if ( s_iCount==1 && m_triggerOnActivate && (m_AgentId != -1) )
	{
		CreateAndPostARTMessage();
	}

	--s_iCount;

	return result;
}

bool sampleFragInstNM::PrepareForDeactivation( bool colliderManagedBySim )
{
	ART::MessageParams msg;
	PostARTMessage("stopAllBehaviours", &msg);

	++s_iCount;

	fragInstNM::PrepareForDeactivation(colliderManagedBySim);

	--s_iCount;

	return true;
}


void sampleFragInstNM::CreateAndPostARTMessage()
{
    if ( m_pBehaviorInst != NULL )
    {
        ART::MessageParams msg;
		// There may or may not be a "start" parameter.  If not, add one.
		if( m_pBehaviorInst->GetValue("start") == 0 )
			msg.addBool("start", true);
        m_pBehaviorInst->ConfigureMessage( &msg );
        PostARTMessage( m_pBehaviorInst->GetBehavior()->GetName(), &msg );
        Displayf( "Posted new behavior message: %s", m_pBehaviorInst->GetBehavior()->GetName() );
    }
}

} // namespace ragesamples
