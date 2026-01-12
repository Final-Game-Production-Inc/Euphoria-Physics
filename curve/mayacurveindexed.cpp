// 
// curve/mayacurveindexed.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#include "mayacurveindexed.h"

#if ENABLE_UNUSED_CURVE_CODE

#include "data/safestruct.h"

/*
#include "data/resource.h"
#include "data/resourcehelpers.h"
#include "data/struct.h"
#include "file/token.h"
*/
using namespace rage;

const Vector3 *mayaCurveIndexed::s_pControlVertices = NULL;
const mayaKnotVector *mayaCurveIndexed::s_pKnotVectors = NULL;

mayaCurveIndexed::mayaCurveIndexed(datResource &rsc) : mayaCurve(rsc)
{
	rsc.PointerFixup(m_pCVIndexes);
}

#if	__DECLARESTRUCT
void mayaCurveIndexed::DeclareStruct(datTypeStruct &s)
{
	mayaCurve::DeclareStruct(s);

	SSTRUCT_BEGIN_BASE(mayaCurveIndexed, mayaCurve)
	SSTRUCT_FIELD_VP(mayaCurveIndexed, m_pCVIndexes)
	SSTRUCT_FIELD(mayaCurveIndexed, m_KVIndex)
	SSTRUCT_IGNORE(mayaCurveIndexed, m_Padding)
	SSTRUCT_END(mayaCurveIndexed)
}
#endif

#endif // ENABLE_UNUSED_CURVE_CODE
