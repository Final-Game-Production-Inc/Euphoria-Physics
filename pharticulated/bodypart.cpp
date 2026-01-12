// 
// pharticulated/bodypart.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "bodypart.h"
#include "articulatedbody.h"

#include "data/resourcehelpers.h"
#include "physics/collider.h"

namespace rage {

	void phArticulatedBodyPart::RejuvenateMatrix ()
	{
#if REJUVENATE_TEST
		// See how far away from normal and orthogonal the matrix gets.
		// When rejuvenate testing is turned on, this will only change the matrix if the error
		// limit is reached, to find out how necessary this is.
		bool errorLimitReached = !m_Matrix.IsOrthonormal(REJUVENATE_ERROR);
		if (errorLimitReached)
#endif
		{
			if ( IsFiniteAll(m_Matrix.GetCol0()) && ( IsZeroAll(m_Matrix.GetCol0()) | IsZeroAll(m_Matrix.GetCol1()) | IsZeroAll(m_Matrix.GetCol2()) ) == 0 ) // if(each vector is nonzero)
			{
				m_Matrix.SetCol1( Normalize( m_Matrix.GetCol1() ) );
				m_Matrix.SetCol2( Cross( m_Matrix.GetCol0(), m_Matrix.GetCol1() ) );
				m_Matrix.SetCol2( Normalize( m_Matrix.GetCol2() ) );
				m_Matrix.SetCol0( Cross(m_Matrix.GetCol1(), m_Matrix.GetCol2() ) );
			}
			else
			{
				// protection against very bad matrices...
				m_Matrix.SetIdentity3x3();
				Warningf("RejuvenateMatrix - bad rotation matrix");
			}
			if ( !IsFiniteAll(m_Matrix.GetCol3()) ||
				 !IsLessThanAll(m_Matrix.GetCol3(),Vec3V(V_FLT_LARGE_6)))
			{
				m_Matrix.SetCol3(Vec3V(V_ZERO));
				Warningf("RejuvenateMatrix - bad translation");
			}
		}
	}

	// CalcRBI: Computes the spatial inertia vector for the link by itself,
	//    that is the inertia of the link disconnected from the rest of the
	//    articulated body.
	// Stores the result as an articulated body inertia; however, it could have
	//	  also been stored as a rigid body inertia.
	void phArticulatedBodyPart::CalcRBI(phArticulatedBodyInertia& ret, Vec4V_In massAndAngularInertia)
	{
		ScalarV mass = massAndAngularInertia.GetW();
		ret.GetM().SetDiagonal( RCC_VECTOR3(mass) );

		Vec3V mp = Scale( mass, m_Matrix.GetCol3() );

		ret.GetH().CrossProduct( VEC3V_TO_VECTOR3(mp) );
		Mat33V dcross;
		RC_MATRIX33(dcross).CrossProduct( Vector3( m_Matrix.GetCol3Intrin128ConstRef() ) );

		Mat33V Imatrix = MATRIX33_TO_MAT33V(ret.GetI());		// Will hold inertia around origin
		Mat33V MD;
		Vec3V angInertia = massAndAngularInertia.GetXYZ();
		MD.SetCol0( Scale( m_Matrix.GetCol0(), angInertia ) );
		MD.SetCol1( Scale( m_Matrix.GetCol1(), angInertia ) );
		MD.SetCol2( Scale( m_Matrix.GetCol2(), angInertia ) );
		UnTransformOrtho( Imatrix, m_Matrix.GetMat33(), MD );
		Multiply( dcross, RCC_MAT33V(ret.GetH()), dcross );
		Subtract( Imatrix, Imatrix, dcross );

		RC_MAT33V(ret.GetI()) = Imatrix;
	}

#if PHARTICULATED_DEBUG_SERIALIZE
	void phArticulatedBodyPart::DebugSerialize(fiAsciiTokenizer& t)
	{
		t.PutDelimiter("m_Matrix: "); t.Put(m_Matrix); t.PutDelimiter("\n");

		t.PutDelimiter("m_LinkInertiaInverse.m_Mass: "); t.Put(m_LinkInertiaInverse.m_Mass); t.PutDelimiter("\n");
		t.PutDelimiter("m_LinkInertiaInverse.m_CrossInertia: "); t.Put(m_LinkInertiaInverse.m_CrossInertia); t.PutDelimiter("\n");
		t.PutDelimiter("m_LinkInertiaInverse.m_InvInertia: "); t.Put(m_LinkInertiaInverse.m_InvInertia); t.PutDelimiter("\n");
	}
#endif // PHARTICULATED_DEBUG_SERIALIZE


	void phArticulatedBodyPart::Copy (const phArticulatedBodyPart& original)
	{
		// Copy the original.
		*this = original;
	}

} // namespace rage