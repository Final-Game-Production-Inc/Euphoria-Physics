//
// curve/curvenurbs.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "curve.h"

#if ENABLE_UNUSED_CURVE_CODE

namespace rage {

IMPLEMENT_PLACE(CurveNurbsKnot);
IMPLEMENT_PLACE(CurveNurbsBlendFunctionConst);

#if	__DECLARESTRUCT

void	CurveNurbsKnot::DeclareStruct	(datTypeStruct &s)
{
	STRUCT_BEGIN(CurveNurbsKnot);
	STRUCT_FIELD(Kv);
	STRUCT_FIELD(Kt);
	STRUCT_END();
}

void	CurveNurbsBlendFunctionConst::DeclareStruct	(datTypeStruct &s)
{
	STRUCT_BEGIN(CurveNurbsBlendFunctionConst);
	STRUCT_FIELD(fFuncConst1);
	STRUCT_FIELD(fFuncConst2a);	
	STRUCT_FIELD(fFuncConst2b);
	STRUCT_FIELD(fFuncConst2c);
	STRUCT_FIELD(fFuncConst3a);
	STRUCT_FIELD(fFuncConst3b);
	STRUCT_FIELD(fFuncConst3c);
	STRUCT_FIELD(fFuncConst4);
	STRUCT_END();
}

#endif

} // namespace rage

#endif // ENABLE_UNUSED_CURVE_CODE
