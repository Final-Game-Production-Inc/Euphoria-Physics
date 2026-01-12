//
// pharticulated/phasespacevector.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHARTICULATED_PHASESPACEVECTOR_H
#define PHARTICULATED_PHASESPACEVECTOR_H

#include "vectormath/classes.h"
#include "vectormath/legacyconvert.h"

namespace rage {


// A Spatial Vector represents (a) a velocity including rotation
//						or	   (b) a force including a torque

class phPhaseSpaceVector
{
public:
	Vec3V omega;			// Rotation vector
	Vec3V trans;			// Velocity or position vector

	// For FORCES:   omega is linear force;   trans is torque around the origin

public:
	phPhaseSpaceVector() {}
	phPhaseSpaceVector( const phPhaseSpaceVector& init );
	phPhaseSpaceVector& operator= ( const phPhaseSpaceVector& rhs );

	void SetZero();
	bool IsZero() const;

	// Norms
	float NormSq() const {  Vec3f t, o; LoadAsScalar(t, trans); LoadAsScalar(o, omega); return (MagSquared(t)+MagSquared(o)); }
	ScalarV NormSqV() const {  return (MagSquared(trans)+MagSquared(omega)); }

	// Addition and scalar multiplication (All-vector pipeline).
	phPhaseSpaceVector& operator+=( const phPhaseSpaceVector& spVel );
	phPhaseSpaceVector& operator-=( const phPhaseSpaceVector& spVel );
	phPhaseSpaceVector& operator*=( float s );
	phPhaseSpaceVector& operator*=( Vector3::Vector3Param s );
	// All-scalar pipeline.
	void Add( const phPhaseSpaceVector& spVel );
	void AddScaled( const phPhaseSpaceVector& spVel, float alpha );
	void AddScaled( const phPhaseSpaceVector& toAddTo, const phPhaseSpaceVector& spVel, float alpha );
	// All-vector pipeline.
	void AddV( const phPhaseSpaceVector& spVel );
	void AddScaled( const phPhaseSpaceVector& spVel, Vector3::Vector3Param alpha );
	void SetScaled( const phPhaseSpaceVector& spVel, Vector3::Vector3Param alpha );
	void AddScaled( const phPhaseSpaceVector& toAddTo, const phPhaseSpaceVector& spVel, Vector3::Vector3Param alpha );
	void SubtractFrom ( const phPhaseSpaceVector& spVel );		// Sets this = (spVel-this)
	void Scale (float scaleFactor) {  trans = rage::Scale(trans, Vec3VFromF32(scaleFactor)); omega = rage::Scale(omega, Vec3VFromF32(scaleFactor)); }

	// TODO: This 
	void Scale (const phPhaseSpaceVector& norm, float scaleFactor) {  trans = rage::Scale(norm.trans, Vec3VFromF32(scaleFactor)); omega = rage::Scale(norm.omega,Vec3VFromF32(scaleFactor)); }

	__forceinline
	void Negate() {  omega = rage::Negate(omega); trans = rage::Negate(trans); } 

	// Dot Product
	Vector3 operator^=( const phPhaseSpaceVector& spVel ) const;

	// Crossproduct
	void CrossProductLeft( const phPhaseSpaceVector& u );		//this = u*this;
	phPhaseSpaceVector& operator*=( const phPhaseSpaceVector& spVel );

	// Coordinate transformations
	void ApplyTranslationXform( const Vector3& u );		// Represents with origin at u
	void ApplyRotationXform( const Matrix34& v );			// Represents in rotated coordinate system

	// General rigid transformations (duals of coordinate transforms)
	void ApplyTransformation( const Matrix34& A );	// Transforms accordingly
	void ApplyTransformationInverse( const Matrix34& A );	// Transforms accordingly.

	bool operator==( const phPhaseSpaceVector& b ) {  return ((IsEqualAll(omega, b.omega) & IsEqualAll(trans,b.trans)) != 0); }

};

// Addition
inline phPhaseSpaceVector operator-( const phPhaseSpaceVector& u, const phPhaseSpaceVector& v );
// Cross Product
inline phPhaseSpaceVector operator*( const phPhaseSpaceVector& u, const phPhaseSpaceVector& v );
// Dot Product
inline Vector3 operator^( const phPhaseSpaceVector& u, const phPhaseSpaceVector& v );
inline Vector3 InnerProduct( const phPhaseSpaceVector& u, const phPhaseSpaceVector& v );

// ************************************************************************
// Inlined functions for phPhaseSpaceVectors
// ************************************************************************

// Constructors & Initializers

__forceinline phPhaseSpaceVector::phPhaseSpaceVector( const phPhaseSpaceVector& init )
: omega( init.omega ), trans(init.trans) 
{
}

__forceinline phPhaseSpaceVector& phPhaseSpaceVector::operator= ( const phPhaseSpaceVector& rhs )
{
	omega = rhs.omega;
	trans = rhs.trans;
	return *this;
}

__forceinline void phPhaseSpaceVector::SetZero()
{
	trans.SetIntrin128(_vzerofp);
	omega.SetIntrin128(_vzerofp);
}

__forceinline bool phPhaseSpaceVector::IsZero() const 
{
	return IsZeroAll(trans) && IsZeroAll(omega);
}

// Addition

//////////////////////
// FLOAT PIPELINE
//////////////////////
inline void phPhaseSpaceVector::Add( const phPhaseSpaceVector& spVel )
{

	// All-scalar.
	Vec3f s_omega;
	Vec3f s_trans;
	Vec3f s_spVelOmega;
	Vec3f s_spVelTrans;
	LoadAsScalar( s_omega, omega );
	LoadAsScalar( s_trans, trans );
	LoadAsScalar( s_spVelOmega, spVel.omega );
	LoadAsScalar( s_spVelTrans, spVel.trans );
	s_omega = rage::Add(s_omega, s_spVelOmega);
	s_trans = rage::Add(s_trans, s_spVelTrans);
	StoreAsScalar( omega, s_omega );
	StoreAsScalar( trans, s_trans );
}

inline void phPhaseSpaceVector::AddScaled( const phPhaseSpaceVector& toAddTo, const phPhaseSpaceVector& spVel, float alpha )
{

	// All-scalar.
	Vec3f s_omega;
	Vec3f s_trans;
	Vec3f s_spVelOmega;
	Vec3f s_spVelTrans;
	LoadAsScalar( s_omega, toAddTo.omega );
	LoadAsScalar( s_trans, toAddTo.trans );
	LoadAsScalar( s_spVelOmega, spVel.omega );
	LoadAsScalar( s_spVelTrans, spVel.trans );
	s_omega = rage::AddScaled(s_omega, s_spVelOmega, Vec3FromF32(alpha));
	s_trans = rage::AddScaled(s_trans, s_spVelTrans, Vec3FromF32(alpha));
	StoreAsScalar( omega, s_omega );
	StoreAsScalar( trans, s_trans );
}

inline void phPhaseSpaceVector::AddScaled( const phPhaseSpaceVector& spVel, float alpha )
{

	// All-scalar.
	Vec3f s_omega;
	Vec3f s_trans;
	Vec3f s_spVelOmega;
	Vec3f s_spVelTrans;
	LoadAsScalar( s_omega, omega );
	LoadAsScalar( s_trans, trans );
	LoadAsScalar( s_spVelOmega, spVel.omega );
	LoadAsScalar( s_spVelTrans, spVel.trans );
	s_omega = rage::AddScaled(s_omega, s_spVelOmega, Vec3FromF32(alpha));
	s_trans = rage::AddScaled(s_trans, s_spVelTrans, Vec3FromF32(alpha));
	StoreAsScalar( omega, s_omega );
	StoreAsScalar( trans, s_trans );
}

//////////////////////
// VECTOR PIPELINE
//////////////////////
inline void phPhaseSpaceVector::AddV( const phPhaseSpaceVector& spVel )
{
	omega += spVel.omega;
	trans += spVel.trans;
}

inline void phPhaseSpaceVector::AddScaled( const phPhaseSpaceVector& spVel, Vector3::Vector3Param alpha )
{

	// All-vector.
	Vec3V v_alpha = RCC_VEC3V( alpha );
	omega = rage::AddScaled(omega, spVel.omega, v_alpha);
	trans = rage::AddScaled(trans, spVel.trans, v_alpha);
}

inline void phPhaseSpaceVector::SetScaled( const phPhaseSpaceVector& spVel, Vector3::Vector3Param alpha )
{

	// All-vector.
	Vec3V v_alpha = RCC_VEC3V( alpha );
	omega = rage::Scale(spVel.omega, v_alpha);
	trans = rage::Scale(spVel.trans, v_alpha);
}

inline void phPhaseSpaceVector::AddScaled( const phPhaseSpaceVector& toAddTo, const phPhaseSpaceVector& spVel, Vector3::Vector3Param alpha )
{

	// All-vector.
	Vec3V v_alpha( Vector3( alpha ).xyzw );
	omega = rage::AddScaled(toAddTo.omega, spVel.omega, v_alpha);
	trans = rage::AddScaled(toAddTo.trans, spVel.trans, v_alpha);
}



inline void phPhaseSpaceVector::SubtractFrom( const phPhaseSpaceVector& spVel )
{
	omega = spVel.omega - omega;
	trans = spVel.trans - trans;
}

inline phPhaseSpaceVector& phPhaseSpaceVector::operator+=( const phPhaseSpaceVector& spVel )
{
	omega += spVel.omega;
	trans += spVel.trans;
	return *this;
}

inline phPhaseSpaceVector& phPhaseSpaceVector::operator-=( const phPhaseSpaceVector& spVel )
{
	omega -= spVel.omega;
	trans -= spVel.trans;
	return *this;
}

inline phPhaseSpaceVector& phPhaseSpaceVector::operator*=( float f )
{

	omega = rage::Scale( omega, Vec3VFromF32(f) );
	trans = rage::Scale( trans, Vec3VFromF32(f) );
	return *this;
}


inline phPhaseSpaceVector& phPhaseSpaceVector::operator*=( Vector3::Vector3Param f )
{

	Vec3V v_f( Vector3(f).xyzw );
	omega = rage::Scale( omega, v_f );
	trans = rage::Scale( trans, v_f );
	return *this;
}


inline phPhaseSpaceVector operator-( const phPhaseSpaceVector& u, const phPhaseSpaceVector& v )
{
	phPhaseSpaceVector ret(u);
	ret -= v;
	return ret;
}

// Dot Products

// TODO: Refactor this to return a ScalarV_Out or a V4ReturnSplatted128.
__forceinline Vector3 operator^( const phPhaseSpaceVector& u, const phPhaseSpaceVector& v )
{

	ScalarV d1 = Dot( u.omega, v.trans );
	ScalarV d2 = Dot( u.trans, v.omega );
	Vec3V sum = Vec3V( Add( d1, d2 ) );

	return VEC3V_TO_VECTOR3( sum );
}

__forceinline Vector3 InnerProduct( const phPhaseSpaceVector& u, const phPhaseSpaceVector& v )
{
	return ( u^v ); 
}

__forceinline Vector3 phPhaseSpaceVector::operator^=( const phPhaseSpaceVector& spVec ) const
{
	return (*this)^spVec;
}

// Transformations

inline void phPhaseSpaceVector::ApplyTranslationXform( const Vector3& u )
{

	trans = AddCrossed( trans, omega, VECTOR3_TO_VEC3V(u) );				// Add crossproduct
}

// Represents in rotated coordinate system
inline void phPhaseSpaceVector::ApplyRotationXform( const Matrix34& R )
{

	Mat34V v_r = RCC_MAT34V( R );
	trans = UnTransform3x3Ortho( v_r, trans );
	omega = UnTransform3x3Ortho( v_r, omega );
}

// Cross Products
	
inline
void phPhaseSpaceVector::CrossProductLeft( const phPhaseSpaceVector& u )		//this = u*this;
{

	// Trans = u.omega*trans + u.trans*omega
	trans = Cross( u.omega, trans );
	trans = AddCrossed( trans, u.trans, omega );
	// Omega = u.omega * omega
	omega = Cross( u.omega, omega );
}

inline
phPhaseSpaceVector& phPhaseSpaceVector::operator*=( const phPhaseSpaceVector& u )
{

	// Trans = omega*u.trans + trans*u.omega
	trans = Cross(trans, u.omega);
	trans = AddCrossed( trans, omega, u.trans );
	// Omega = omega*u.omega
	omega = Cross(omega, u.omega);
	
	return *this;
}

inline
phPhaseSpaceVector operator*( const phPhaseSpaceVector& u, const phPhaseSpaceVector& v )
{
	phPhaseSpaceVector ret(v);
	ret.CrossProductLeft( u );
	return ret;
}


} // namespace rage

#endif	// ARTICULATED_PHASESPACEVECTOR_H
