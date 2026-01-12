//
// pharticulated/bodypart.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

// Articulated Body Link Type
// ======================
//
// a. Holds applied forces and torques.
// b. Its position
// c. Pointers to the children links
// d. Mass and inertia in local coordinates.
// e. Computes the cross inertias with adjacent links in two passes
// f. Computes articulated inertia in two passes
// g. Computes bias force including coriolis force. (Again, two passes).
// h. Updates velocities only (affects bias forces, but not inertias)
// i. Updates velocities and positions.

#ifndef PHARTICULATED_BODYPART_H
#define PHARTICULATED_BODYPART_H

#include "bodyinertia.h"

#include "data\safestruct.h"

#include "data/resource.h"
#include "phcore/config.h"
#include "vectormath/classes.h"
#include "vectormath/legacyconvert.h"

#include "paging/base.h"
#include "paging/ref.h"

#define PHARTICULATED_DEBUG_SERIALIZE 0

namespace rage {


	class fiAsciiTokenizer;

	class phArticulatedBody;

	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////

	// Instance data
	class phArticulatedBodyPart
	{
	public:

		phArticulatedBodyPart();
		~phArticulatedBodyPart() {}

		const phArticulatedBodyInertia& GetLinkInertiaInverse() const;

		const Vector3& GetPosition() const;
		Vec3V_Out& GetPositionV() const;

		// PURPOSE: Getet the coordinate matrix of this articulated body part.
		// RETURN:	a reference to the coordinate matrix of this articulated body part
		Mat34V_Out GetMatrix () const;
		Mat34V_Ref GetMatrix ();

		Mat34V_ConstRef GetMatrixRef() const { return m_Matrix; }

		void  NormalizeMatrix() { ReOrthonormalize3x3( m_Matrix, m_Matrix ); }

		// PURPOSE: Set the coordinate matrix of this articulated body part.
		// PARAMS:
		//	matrix - the new coordinate matrix of this articulated body part
		// NOTES:
		//	1. The coordinate matrix is in world space, but with a position relative to the root part.
		//	2. This matrix is left-handed, so the orientation is a 3x3 transpose of the equivalent instance or collider matrix.
		void SetMatrix (const Matrix34& matrix);

		// PURPOSE: Set the position of this articulated body part, relative to the root part in world coordinates.
		// PARAMS:
		//	position - the new position of this articulated body part, relative to the root part in world coordinates
		void SetPosition (const Vector3& position);

		// PURPOSE: Set the orientation part of the matrix of this articulated body part.
		// PARAMS:
		//	orientation - the new orientation of this articulated body part
		// NOTES:
		//	1. The coordinate matrix is in world space, but with a position relative to the root part.
		//	2. This matrix is left-handed, so the orientation is a 3x3 transpose of the equivalent instance or collider matrix.
		void SetOrientation (const Matrix34& orientation);

		void RejuvenateMatrix ();

		// ----

		void CalcRBI(phArticulatedBodyInertia& ret, Vec4V_In massAndAngularInertia);						// Computes the Rigid Body Inertia matrix

		void Copy (const phArticulatedBodyPart& original);


#if PHARTICULATED_DEBUG_SERIALIZE
		void DebugSerialize(fiAsciiTokenizer& t);
#endif

	private:

		// PURPOSE: the orientation and position relative to the instance matrix
		// NOTES:
		//   - Since currently the instance (/collider) matrix must be axis aligned, this 
		//     currently also gives you the world space orientation.
		//   - Note also that this matrix is transposed relative to the way matrices 
		//     are normally used in RAGE.
		Mat34V m_Matrix;

		// PURPOSE: the inverse of the inertia of the articulated body part in the coordinate system of the body
		phArticulatedBodyInertia m_LinkInertiaInverse;	

		friend class phArticulatedBody;
	};

	// ****************************************************************************
	// Inlined functions for phArticulatedBodyPart
	// ****************************************************************************

	inline const phArticulatedBodyInertia& phArticulatedBodyPart::GetLinkInertiaInverse() const
	{
		return m_LinkInertiaInverse;
	}

	inline const Vector3& phArticulatedBodyPart::GetPosition() const
	{
		return RCC_VECTOR3(m_Matrix.GetCol3Intrin128ConstRef());
	}

	inline const Vec3V_Out& phArticulatedBodyPart::GetPositionV() const
	{
		return m_Matrix.GetCol3ConstRef();
	}

	inline Mat34V_Out phArticulatedBodyPart::GetMatrix () const
	{
		return m_Matrix;
	}

	inline Mat34V_Ref phArticulatedBodyPart::GetMatrix () 
	{
		return m_Matrix;
	}

	inline void phArticulatedBodyPart::SetMatrix (const Matrix34& matrix)
	{
		m_Matrix = RCC_MAT34V(matrix);
	}

	inline void phArticulatedBodyPart::SetPosition (const Vector3& position)
	{
		m_Matrix.SetCol3Intrin128(VECTOR3_TO_INTRIN(position));
	}

	inline void phArticulatedBodyPart::SetOrientation (const Matrix34& orientation)
	{
		m_Matrix.Set3x3( (*(reinterpret_cast<const ::rage::Mat33V*>(&orientation))) );
	}

	inline phArticulatedBodyPart::phArticulatedBodyPart()
	{
	}

} // namespace ragesamples

#endif		// PHARTICULATED_BODYPART_H
