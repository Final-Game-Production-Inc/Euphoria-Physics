#ifndef VECTORMATH_CLASSES_SOA_H
#define VECTORMATH_CLASSES_SOA_H

//=======================================================================================================================================
//
//
// INCLUDES
//
//
//=======================================================================================================================================

#include "vectormath.h"

//================================================
// SoA
//================================================

#include "vecbool1v_soa.h"
#include "vecbool2v_soa.h"
#include "vecbool3v_soa.h"
#include "vecbool4v_soa.h"
#include "scalarv_soa.h"
#include "vec2v_soa.h"
#include "vec3v_soa.h"
#include "vec4v_soa.h"
#include "quatv_soa.h"
#include "mat33v_soa.h"
#include "mat34v_soa.h"
#include "mat44v_soa.h"



//=======================================================================================================================================
//
//
// FREE FUNCTIONS (class arguments)
//
//
//=======================================================================================================================================

// Vectorized 4-at-a-time Vec*V free functions.
#include "classfreefuncsv_soa.h"


namespace rage
{
	// V4 SoA reinterpret type conversions (Vec4V <--> QuatV <--> VecBool1V).
	template <typename _4tupleTo, typename _4tupleFrom>
	void Cast( _4tupleTo& _to, const _4tupleFrom& from );
}


//=======================================================================================================================================
//
//
// FREE FUNCTION IMPLEMENTATIONS
//
//
//=======================================================================================================================================

// Vectorized 4-at-a-time Vec*V free functions.
#include "classfreefuncsv_soa.inl"

namespace rage
{
	template <typename _To, typename _From>
	__forceinline void Cast( _To& _to, const _From& from )
	{
		_to = _To(
			from.GetXIntrin128(),
			from.GetYIntrin128(),
			from.GetZIntrin128(),
			from.GetWIntrin128()
			);
	}
}


#endif // VECTORMATH_CLASSES_SOA_H
