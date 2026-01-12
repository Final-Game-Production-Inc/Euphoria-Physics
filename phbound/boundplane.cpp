//
// phbound/boundplane.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "boundplane.h"

#include "support.h"

#include "data/resource.h"
#include "data/struct.h"



namespace rage {

CompileTimeAssert(sizeof(phBoundPlane) <= phBound::MAX_BOUND_SIZE);
CompileTimeAssert(sizeof(phBoundPlane) <= 128);


phBoundPlane::phBoundPlane()
{
	m_Type = phBound::PLANE;
}


phBoundPlane::~phBoundPlane()
{
}

#if !__SPU

phBoundPlane::phBoundPlane (datResource & rsc) 
	: phBound(rsc)
{
}

#if __DECLARESTRUCT
void phBoundPlane::DeclareStruct(datTypeStruct &s)
{
	phBound::DeclareStruct(s);
	STRUCT_BEGIN(phBoundPlane);
	STRUCT_END();
}
#endif // __DECLARESTRUCT

#endif // !__SPU

} // namespace rage
