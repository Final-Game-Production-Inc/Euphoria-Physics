//
// pharticulated/bodyinertia.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHARTICULATED_BODYINERTIA_H
#define PHARTICULATED_BODYINERTIA_H

// ********************************************
// Defines  (a) RigidBodyInertia  (NOT DONE)
//			(b) ArticulatedBodyInertia
// ********************************************


#include "phasespacevector.h"
#include "vector/matrix33.h"
#include "vector/matrix34.h"
#include "vectormath/classes.h"
#include "vectormath/legacyconvert.h"


namespace rage {


class phArticulatedBodyInertia
{
public:
	phArticulatedBodyInertia() {}
	phArticulatedBodyInertia( const Matrix33& M, const Matrix33& H, const Matrix33& I);
	phArticulatedBodyInertia( const phArticulatedBodyInertia& init );

	void SetM ( const Matrix33& newM ) { m_Mass = RCC_MAT33V(newM); }
	void SetH ( const Matrix33& newH ) { m_CrossInertia = RCC_MAT33V(newH); }
	void SetI ( const Matrix33& newI ) { m_InvInertia = RCC_MAT33V(newI); }

	Matrix33& GetM() { return RC_MATRIX33(m_Mass); }
	Matrix33& GetH() { return RC_MATRIX33(m_CrossInertia); }
	Matrix33& GetI() { return RC_MATRIX33(m_InvInertia); }
	const Matrix33& GetM() const { return RCC_MATRIX33(m_Mass); }
	const Matrix33& GetH() const { return RCC_MATRIX33(m_CrossInertia); }
	const Matrix33& GetI() const { return RCC_MATRIX33(m_InvInertia); }

	phArticulatedBodyInertia& operator+= ( const phArticulatedBodyInertia& a );
	void SubtractFrom( const phArticulatedBodyInertia& a );	// Sets this := (a - this)

	phArticulatedBodyInertia& operator*= ( float scale );

	void Transform (const phPhaseSpaceVector& src, phPhaseSpaceVector& dest) const;
	void TransformTrans (const Vector3& srcTrans, phPhaseSpaceVector& dest) const;

	void Inverse ( phArticulatedBodyInertia& ret ) const;	// Computes the inverse (which is also symmetric)

	// Sets equal to u v^S / scale
	void SetOuterProductScaled( const phPhaseSpaceVector& u, const float& scale );

	static void InverseSym( Mat33V_In original, Mat33V_InOut inverse );

	// (Don't use this one, it's just an optimization; it's called from inside the above one.)
	static void InverseSym_Imp( MAT33V_DECL(original), Mat33V_InOut inverse );

public:
	Mat33V m_Mass;				// A positive definite, symmetric matrix
	Mat33V m_CrossInertia;
	Mat33V m_InvInertia;		// Also symmetric and positive definite
};

// ***********************************************************************************
//  Inlined methods for ArticulatedBodyInertia
// ***********************************************************************************

// Constructors

inline phArticulatedBodyInertia::phArticulatedBodyInertia( const Matrix33& theM, const Matrix33& theH, const Matrix33& theI)
: m_Mass(RCC_MAT33V(theM)), m_CrossInertia(RCC_MAT33V(theH)), m_InvInertia(RCC_MAT33V(theI))
{
}

inline phArticulatedBodyInertia::phArticulatedBodyInertia( const phArticulatedBodyInertia& init )
: m_Mass(init.m_Mass), m_CrossInertia(init.m_CrossInertia), m_InvInertia(init.m_InvInertia)
{
}

inline phArticulatedBodyInertia& phArticulatedBodyInertia::operator+= ( const phArticulatedBodyInertia& a )
{
	m_Mass += a.m_Mass;
	m_CrossInertia += a.m_CrossInertia;
	m_InvInertia += a.m_InvInertia;
	return *this;
}

// Sets this := (a - this)
inline void phArticulatedBodyInertia::SubtractFrom( const phArticulatedBodyInertia& a )
{
	m_Mass = a.m_Mass - m_Mass;
	m_InvInertia = a.m_InvInertia - m_InvInertia;
	m_CrossInertia = a.m_CrossInertia - m_CrossInertia;
}

inline phArticulatedBodyInertia& phArticulatedBodyInertia::operator*= ( float scale )
{

	Vec3V scaleFact = Vec3VFromF32( scale );
	Scale(m_Mass, m_Mass, scaleFact);
	Scale(m_InvInertia, m_InvInertia, scaleFact);
	Scale(m_CrossInertia, m_CrossInertia, scaleFact);
	return *this;
}

inline void phArticulatedBodyInertia::TransformTrans (const Vector3& srcTrans, phPhaseSpaceVector& dest) const
{
	dest.trans = UnTransformOrtho( m_CrossInertia, RCC_VEC3V(srcTrans) );
	dest.omega = UnTransformOrtho( m_Mass, RCC_VEC3V(srcTrans) );
}

VEC3_INLINE void phArticulatedBodyInertia::Transform (const phPhaseSpaceVector& src, phPhaseSpaceVector& dest) const
{

	// Save inputs locally.
	Mat33V v_CrossInertia	= m_CrossInertia;
	Mat33V v_Mass			= m_Mass;
	Mat33V v_InvInertia		= m_InvInertia;
	Vec3V v_srcOmega		= src.omega;
	Vec3V v_srcTrans		= src.trans;
	// Store outputs locally.
	Vec3V v_destOmega;
	Vec3V v_destTrans;

	v_destOmega = Multiply( v_CrossInertia, v_srcOmega );
	v_destTrans = UnTransformOrtho( v_CrossInertia, v_srcTrans );

	Vec3V temp;
	temp = UnTransformOrtho( v_Mass, v_srcTrans );
	v_destOmega = Add(v_destOmega, temp);
	temp = UnTransformOrtho( v_InvInertia, v_srcOmega );
	v_destTrans = Add( v_destTrans, temp );

	dest.omega = v_destOmega;
	dest.trans = v_destTrans;
}

// Calculate inverse under assumption matrix is symmetric
// Only uses lower part of the matrix.  No checking done for symmetry
__forceinline void phArticulatedBodyInertia::InverseSym( Mat33V_In original, Mat33V_InOut inverse )
{
	InverseSym_Imp( MAT33V_ARG(original), inverse );
}

} // namespace rage


#endif // ARTICULATED_BODY_INERTIA_H
