//
// pharticulated/joint.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "joint.h"

#include "articulatedbody.h"
#include "jointdispatch.h"
#include "bodypart.h"
#include "joint1dof.h"
#include "joint3dof.h"

#include "file/token.h"
#include "system/memory.h"
#include "system/typeinfo.h"
#include "vectormath/legacyconvert.h"


namespace rage {


	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////

	float phJoint::sm_StiffnessModulus = 0.0f;

	IMPLEMENT_PLACE(phJointType);

	float phJoint::GetJointAngularVelocityOnAxis (phArticulatedBody *body, Vec3V_In axis) const
	{
		ScalarV axisDotParentAngVel = Dot(axis,body->GetAngularVelocity(GetParentLinkIndex()));
		ScalarV axisDotChildAngVel = Dot(axis,body->GetAngularVelocity(GetChildLinkIndex()));
		return Subtract(axisDotChildAngVel, axisDotParentAngVel).Getf();
	}

	void phJoint::ApplyTorque (phArticulatedBody *body, Vec::V3Param128 torque, Vec::V3Param128 timeStep) const 
	{
		body->ApplyTorque(GetChildLinkIndex(), torque, timeStep);
		Vec3V negatedTorque = Negate(Vec3V(torque));
		body->ApplyTorque(GetParentLinkIndex(), negatedTorque.GetIntrin128(), timeStep);
	}

	void phJoint::ApplyAngImpulse (phArticulatedBody *body, Vec::V3Param128 angImpulse) const
	{
		body->ApplyAngImpulse(GetChildLinkIndex(), angImpulse);
		Vec::Vector_4V negatedAngImpulse = Vec::V4Negate(angImpulse);
		body->ApplyAngImpulse(GetParentLinkIndex(), negatedAngImpulse);
	}

	phJointType::phJointType (datResource &/*rsc*/) 
	{
	}

	phJointType::phJointType()
	{
		m_ParentLinkIndex = INVALID_AB_LINK_INDEX;
		m_ChildLinkIndex = INVALID_AB_LINK_INDEX;
		m_EnforceAccededLimits = false;
#ifdef USE_SOFT_LIMITS
		m_InvTimeToSeparateSoft = 4.0f;
#endif
	}

	phJoint::phJoint()
		: m_DriveState(DRIVE_STATE_FREE)
		, m_Stiffness(0.0f)
		, m_MinStiffness(0.0f)
	{
		sysMemZeroBytes<sizeof(m_UpInertia)>( union_cast<u32*>(&m_UpInertia) );
		sysMemZeroBytes<sizeof(m_DownInertia)>(&m_DownInertia);

		sysMemZeroBytes<sizeof(m_VelocityToPropDown)>(&m_VelocityToPropDown);
	}

	void phJoint::SetMinStiffness( float minStiffness ) 
	{ 
		m_MinStiffness = FPClamp(minStiffness, 0.0f, 1.0f); 

		// Enforce the minimum stiffness
		m_Stiffness = FPClamp(m_Stiffness, m_MinStiffness, 1.0f); 
	}

	ScalarV_Out phJoint::ResponseToUnitTorqueOnAxis (phArticulatedBody *body, Vec3V_In unitAxis) const
	{
		phPhaseSpaceVector childResponse;
		childResponse.omega = UnTransformOrtho( GetChildLink(body)->GetLinkInertiaInverse().m_Mass, unitAxis );
		childResponse.trans = UnTransformOrtho( GetChildLink(body)->GetLinkInertiaInverse().m_CrossInertia, unitAxis );
		phPhaseSpaceVector parentResponse;
		parentResponse.omega = UnTransformOrtho( GetParentLink(body)->GetLinkInertiaInverse().m_Mass, unitAxis );
		parentResponse.trans = UnTransformOrtho( GetParentLink(body)->GetLinkInertiaInverse().m_CrossInertia, unitAxis );
		phPhaseSpaceVector temp;
		TransformByAupdown(childResponse, temp);
		parentResponse.Negate();						// Parent's response ("up part" only) to the negated torque
		TransformByAdownupAndAdd(parentResponse, childResponse);
		parentResponse.omega += temp.omega;				// Only care about omega part.
		childResponse.omega -= parentResponse.omega;
		return Dot(unitAxis,childResponse.omega);
	}


	ScalarV_Out phJoint::ResponseToUnitForceOnAxis (phArticulatedBody *body, Vec3V_In unitAxis) const
	{
		phPhaseSpaceVector childResponse;
		childResponse.omega = Multiply( GetChildLink(body)->GetLinkInertiaInverse().m_CrossInertia, unitAxis );
		childResponse.trans = UnTransformOrtho( GetChildLink(body)->GetLinkInertiaInverse().m_InvInertia, unitAxis );
		phPhaseSpaceVector parentResponse;
		parentResponse.omega = Multiply( GetParentLink(body)->GetLinkInertiaInverse().m_CrossInertia, unitAxis );
		parentResponse.trans = UnTransformOrtho( GetParentLink(body)->GetLinkInertiaInverse().m_InvInertia, unitAxis );
		phPhaseSpaceVector temp;
		TransformByAupdown(childResponse,temp);
		parentResponse.Negate();						// Parent's response ("up part" only) to the negated torque
		TransformByAdownupAndAdd(parentResponse,childResponse);
		parentResponse.trans += temp.trans;				// Only care about trans part.
		childResponse.trans -= parentResponse.trans;
		return Dot(unitAxis,childResponse.trans);
	}

	// Returns a righthanded orthonormal basis to complement vector u
	void phJoint::GetOrtho( Vec3V_In u, Vec3V_InOut v, Vec3V_InOut w)
	{
		// ORIGINAL CODE:
		//if ( u.x > 0.5f || u.x < -0.5f || u.y > 0.5f || u.y<-0.5f )
		//{
		//	v.Set ( u.y, -u.x, 0.0f );
		//}
		//else
		//{
		//	v.Set ( 0.0f, u.z, -u.y);
		//}
		//v.Normalize();
		//w = u;
		//w.Cross(v);
		//w.Normalize();

		Vec::Vector_4V v_u = u.GetIntrin128();
		Vec::Vector_4V v_neg_u = Vec::V4Negate( v_u );

		// We want to compare (a > b) with:
		//     ---------------------------
		// a = | u.x | -0.5 | u.y | -0.5 |
		//     ---------------------------
		// with:
		//     ---------------------------
		// b = | 0.5 |  u.x | 0.5 |  u.y |
		//     ---------------------------

		Vec::Vector_4V v_zero = Vec::V4VConstant(V_ZERO);
		Vec::Vector_4V v_half = Vec::V4VConstant(V_HALF);
		Vec::Vector_4V v_negHalf = Vec::V4VConstant(V_NEGHALF);
		Vec::Vector_4V a = Vec::V4MergeXY( v_u, v_negHalf );
		Vec::Vector_4V b = Vec::V4MergeXY( v_half, v_u );
		Vec::Vector_4V isGt = Vec::V4IsGreaterThanV( a, b );

		// Combine the results.
		Vec::Vector_4V resX = Vec::V4SplatX( isGt );
		Vec::Vector_4V resY = Vec::V4SplatY( isGt );
		Vec::Vector_4V resZ = Vec::V4SplatZ( isGt );
		Vec::Vector_4V resW = Vec::V4SplatW( isGt );
		Vec::Vector_4V res = Vec::V4Or( Vec::V4Or( resX, resY ), Vec::V4Or( resZ, resW ) );

		// Generate result A to select from.
		Vec::Vector_4V v_uy = Vec::V4SplatY( v_u );
		Vec::Vector_4V v_uy_0_uy_0 = Vec::V4MergeXY( v_uy, v_zero );
		Vec::Vector_4V resultA = Vec::V4MergeXY( v_uy_0_uy_0, v_neg_u );

		// Generate result B to select from.
		Vec::Vector_4V v_uz = Vec::V4SplatZ( v_u );
		Vec::Vector_4V v_0_nux_0_nuy = Vec::V4MergeXY( v_zero, v_neg_u );
		Vec::Vector_4V resultB = Vec::V4MergeZW( v_0_nux_0_nuy, v_uz );

		// Select the result.
		Vec::Vector_4V result = Vec::V4SelectFT( res, resultB, resultA );

		// Normalize and return.
		Vec::Vector_4V normalizedResult = Vec::V3Normalize( result );
		Vec::Vector_4V outW = Vec::V3Cross( v_u, normalizedResult );
		outW = Vec::V3Normalize( outW );

		v.SetIntrin128( normalizedResult );
		w.SetIntrin128( outW );
	}

	// Returns a vector v orthonormal to unit vector u
	Vec3V_Out phJoint::GetOrtho( Vec3V_In u )
	{
		// ORIGINAL CODE:
		//if ( u.x > 0.5f || u.x < -0.5f || u.y > 0.5f || u.y < -0.5f )
		//{
		//	v.Set ( u.y, -u.x, 0.0f );
		//}
		//else
		//{
		//	v.Set ( 0.0f, u.z, -u.y);
		//}
		//v.Normalize();

		Vec::Vector_4V v_u = u.GetIntrin128();
		Vec::Vector_4V v_neg_u = Vec::V4Negate( v_u );

		// We want to compare (a > b) with:
		//     ---------------------------
		// a = | u.x | -0.5 | u.y | -0.5 |
		//     ---------------------------
		// with:
		//     ---------------------------
		// b = | 0.5 |  u.x | 0.5 |  u.y |
		//     ---------------------------

		Vec::Vector_4V v_zero = Vec::V4VConstant(V_ZERO);
		Vec::Vector_4V v_half = Vec::V4VConstant(V_HALF);
		Vec::Vector_4V v_negHalf = Vec::V4VConstant(V_NEGHALF);
		Vec::Vector_4V a = Vec::V4MergeXY( v_u, v_negHalf );
		Vec::Vector_4V b = Vec::V4MergeXY( v_half, v_u );
		Vec::Vector_4V isGt = Vec::V4IsGreaterThanV( a, b );

		// Combine the results.
		Vec::Vector_4V resX = Vec::V4SplatX( isGt );
		Vec::Vector_4V resY = Vec::V4SplatY( isGt );
		Vec::Vector_4V resZ = Vec::V4SplatZ( isGt );
		Vec::Vector_4V resW = Vec::V4SplatW( isGt );
		Vec::Vector_4V res = Vec::V4Or( Vec::V4Or( resX, resY ), Vec::V4Or( resZ, resW ) );

		// Generate result A to select from.
		Vec::Vector_4V v_uy = Vec::V4SplatY( v_u );
		Vec::Vector_4V v_uy_0_uy_0 = Vec::V4MergeXY( v_uy, v_zero );
		Vec::Vector_4V resultA = Vec::V4MergeXY( v_uy_0_uy_0, v_neg_u );

		// Generate result B to select from.
		Vec::Vector_4V v_uz = Vec::V4SplatZ( v_u );
		Vec::Vector_4V v_0_nux_0_nuy = Vec::V4MergeXY( v_zero, v_neg_u );
		Vec::Vector_4V resultB = Vec::V4MergeZW( v_0_nux_0_nuy, v_uz );

		// Select the result.
		Vec::Vector_4V result = Vec::V4SelectFT( res, resultB, resultA );

		// Normalize and return.
		Vec::Vector_4V normalizedResult = Vec::V3Normalize( result );
		return Vec3V( normalizedResult );
	}

	void phJoint::VrRotateAlign( Mat34V_InOut outMat, Vec3V_In fromVec, Vec3V_In toVec )
	{
		Vec3V v_toVec = toVec;
		Vec3V v_fromVec = fromVec;
		Vec3V v_crossVec = Cross(v_fromVec, v_toVec);
		ScalarV sine2 = MagSquared( v_crossVec );
		if( IsLessThanAll( sine2, ScalarV(V_FLT_SMALL_12) ) != 0 )
		{
			outMat = Mat34V(V_IDENTITY);
			return;
		}

		ScalarV sine = Sqrt( sine2 );
		v_crossVec = InvScale( v_crossVec, sine );
		ScalarV scale = InvSqrtSafe( Scale( MagSquared(v_fromVec), MagSquared(v_toVec) ), ScalarV(V_FLT_LARGE_8) );
		sine = Scale(sine, scale);
		ScalarV cosine = Scale(Dot(v_fromVec, v_toVec), scale);
		VrRotate( outMat, cosine, sine, v_crossVec );
	}

	void phJoint::VrRotate( Mat34V_InOut outMat, ScalarV_In c, ScalarV_In s, Vec3V_In u )
	{
		RotationMapR3Set( outMat, u, s, c );
	}


	void phJoint::RotationMapR3Set( Mat34V_InOut ret, Vec3V_In u, ScalarV_In s, ScalarV_In c )
	{
		Assert( FPAbs( MagSquared(u).Getf()-1.0f ) < 2.0e-6f );

		//float mc = 1.0f-c.Getf();
		//float xmc = u.GetXf()*mc;
		//float xymc = xmc*u.GetYf();
		//float xzmc = xmc*u.GetZf();
		//float yzmc = u.GetYf()*u.GetZf()*mc;
		//float xs = u.GetXf()*s.Getf();
		//float ys = u.GetYf()*s.Getf();
		//float zs = u.GetZf()*s.Getf();
		//ret.SetCol0( Vec3V(u.GetXf()*u.GetXf()*mc+c.Getf(), xymc-zs, xzmc+ys) );
		//ret.SetCol1( Vec3V(xymc+zs,u.GetYf()*u.GetYf()*mc+c.Getf(),yzmc-xs) );
		//ret.SetCol2( Vec3V(xzmc-ys, yzmc+xs, u.GetZf()*u.GetZf()*mc+c.Getf()) );

		// Code below is same as from Mat33FromAxisAngle(), except sine is negated to be equivalent to original code here.
		// Left-handed rotation I think. :-/
		Vec::Vector_4V _xxxx = Vec::V4SplatX( u.GetIntrin128() );
		Vec::Vector_4V _yyyy = Vec::V4SplatY( u.GetIntrin128() );
		Vec::Vector_4V _zzzz = Vec::V4SplatZ( u.GetIntrin128() );
		Vec::Vector_4V oneMinusCos = Vec::V4Subtract( Vec::V4VConstant(V_ONE), c.GetIntrin128() );
		Vec::Vector_4V sinTimesAxis = Vec::V4Scale( u.GetIntrin128(), s.GetIntrin128() );
		Vec::Vector_4V negSinTimesAxis = Vec::V4Negate( sinTimesAxis );
		Vec::Vector_4V tempVect0 = Vec::V4PermuteTwo<Vec::X1,Vec::Z2,Vec::Y1,Vec::X1>( sinTimesAxis, negSinTimesAxis );
		Vec::Vector_4V tempVect1 = Vec::V4PermuteTwo<Vec::Z1,Vec::X1,Vec::X2,Vec::X1>( sinTimesAxis, negSinTimesAxis );
		Vec::Vector_4V tempVect2 = Vec::V4PermuteTwo<Vec::Y2,Vec::X1,Vec::X1,Vec::X1>( sinTimesAxis, negSinTimesAxis );
		tempVect0 = Vec::V4SelectFT( Vec::V4VConstant(V_MASKX), tempVect0, c.GetIntrin128() );
		tempVect1 = Vec::V4SelectFT( Vec::V4VConstant(V_MASKY), tempVect1, c.GetIntrin128() );
		tempVect2 = Vec::V4SelectFT( Vec::V4VConstant(V_MASKZ), tempVect2, c.GetIntrin128() );
		ret.SetCol0Intrin128( Vec::V4AddScaled( tempVect0, Vec::V4Scale( u.GetIntrin128(), _xxxx ), oneMinusCos ) );
		ret.SetCol1Intrin128( Vec::V4AddScaled( tempVect1, Vec::V4Scale( u.GetIntrin128(), _yyyy ), oneMinusCos ) );
		ret.SetCol2Intrin128( Vec::V4AddScaled( tempVect2, Vec::V4Scale( u.GetIntrin128(), _zzzz ), oneMinusCos ) );
	}

	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////

#if __DECLARESTRUCT
	void phJointType::DeclareStruct(datTypeStruct &s)
	{
		pgBase::DeclareStruct(s);
		SSTRUCT_BEGIN_BASE(phJointType, pgBase)
			SSTRUCT_FIELD(phJointType, m_DefaultStiffness)
			SSTRUCT_FIELD(phJointType, m_EnforceAccededLimits)
			SSTRUCT_FIELD(phJointType, m_JointType)
			SSTRUCT_FIELD(phJointType, m_ParentLinkIndex)
			SSTRUCT_FIELD(phJointType, m_ChildLinkIndex)
			SSTRUCT_FIELD(phJointType, m_OrientParent)
			SSTRUCT_FIELD(phJointType, m_OrientChild)
#ifdef USE_SOFT_LIMITS
			SSTRUCT_FIELD(phJointType, m_InvTimeToSeparateSoft)
#endif
			SSTRUCT_END(phJointType)
	}
#endif // __DECLARESTRUCT

	void phJointType::VirtualConstructFromPtr (datResource & rsc, phJointType* joint, u8 jointType)
	{
		switch (jointType)
		{
		case phJoint::JNT_1DOF:
			::new (joint) phJoint1DofType(rsc);
			break;
		case phJoint::JNT_3DOF:
			::new (joint) phJoint3DofType(rsc);
			break;
		default:
			Errorf("Unknown joint type %d\n", jointType);
			AssertMsg(0 , "phJointType::VirtualConstructFromPtr - unsupported or unknown joint type");
			break;
		}
	}

	void phJoint::VirtualConstructFromPtr (datResource & rsc, phJoint* joint)
	{
		switch (joint->GetJointType())
		{
		case phJoint::JNT_1DOF:
			::new ((void *)joint) phJoint1Dof(rsc);
			break;
		case phJoint::JNT_3DOF:
			::new ((void *)joint) phJoint3Dof(rsc);
			break;
		default:
			Errorf("Unknown joint type %d\n", joint->GetJointType());
			AssertMsg(0 , "phJoint::VirtualConstructFromPtr - unsupported or unknown joint type");
			break;
		}
	}

	// Computes the response in a joint orientation to a given torque.
	// If the torque is applied to the child link and the opposite
	//	torque applied to the parent link, then there is a change in
	//  joint orientation.  
	// This routine returns the net change in orientation, namely, the 
	//	change in orienation of the child minus the orientation of the parent.
	// The returned net change is expressed as a rotation vector.
	// It is written as a general purpose routine for all joints, but mostly
	//	only makes sense to use this for 3-Dof rotational joints.
	void phJoint::TotalResponseToTorque( phArticulatedBody *body, Vec3V_In torque, Vec3V_InOut response ) const
	{
		Vec3V v_torque = torque;

		phPhaseSpaceVector childResponse;
		childResponse.omega = UnTransformOrtho( GetChildLink(body)->GetLinkInertiaInverse().m_Mass, v_torque );
		childResponse.trans = UnTransformOrtho( GetChildLink(body)->GetLinkInertiaInverse().m_CrossInertia, v_torque );
		phPhaseSpaceVector parentResponse;
		parentResponse.omega = UnTransformOrtho( GetParentLink(body)->GetLinkInertiaInverse().m_Mass, v_torque );
		parentResponse.trans = UnTransformOrtho( GetParentLink(body)->GetLinkInertiaInverse().m_CrossInertia, v_torque );

		phPhaseSpaceVector temp;
		TransformByAupdown( childResponse, temp );
		parentResponse.Negate();						// Parent's response ("up part" only) to the negated torque
		TransformByAdownupAndAdd( parentResponse, childResponse );
		parentResponse.omega += temp.omega;				// Only care about omega part.
		childResponse.omega -= parentResponse.omega;

		response = childResponse.omega;
	}

	phArticulatedBodyPart* phJoint::GetParentLink(const phArticulatedBody *body) const		
	{ 
		Assert(m_Type);
		Assert(m_Type->m_ParentLinkIndex < body->GetNumBodyParts());
		return const_cast<phArticulatedBodyPart*>(&body->GetLink(m_Type->m_ParentLinkIndex)); 
	}

	phArticulatedBodyPart* phJoint::GetChildLink(const phArticulatedBody *body) const		
	{ 
		Assert(m_Type);
		Assert(m_Type->m_ChildLinkIndex < body->GetNumBodyParts());
		return const_cast<phArticulatedBodyPart*>(&body->GetLink(m_Type->m_ChildLinkIndex)); 
	}

#if PHARTICULATED_DEBUG_SERIALIZE
	void phJoint::DebugSerialize(fiAsciiTokenizer& t)
	{
		t.PutDelimiter("m_JointType: "); t.Put(m_JointType); t.PutDelimiter("\n");
		t.PutDelimiter("m_Stiffness: "); t.Put(m_Stiffness); t.PutDelimiter("\n");
		t.PutDelimiter("m_JointType: "); t.Put(m_JointType); t.PutDelimiter("\n");

		t.PutDelimiter("m_UpInertia.m_Mass: "); t.Put(m_UpInertia.m_Mass); t.PutDelimiter("\n");
		t.PutDelimiter("m_UpInertia.m_CrossInertia: "); t.Put(m_UpInertia.m_CrossInertia); t.PutDelimiter("\n");
		t.PutDelimiter("m_UpInertia.m_InvInertia: "); t.Put(m_UpInertia.m_InvInertia); t.PutDelimiter("\n");

		t.PutDelimiter("m_DownInertia.m_Mass: "); t.Put(m_DownInertia.m_Mass); t.PutDelimiter("\n");
		t.PutDelimiter("m_DownInertia.m_CrossInertia: "); t.Put(m_DownInertia.m_CrossInertia); t.PutDelimiter("\n");
		t.PutDelimiter("m_DownInertia.m_InvInertia: "); t.Put(m_DownInertia.m_InvInertia); t.PutDelimiter("\n");

		t.PutDelimiter("m_DownPush.omega: "); t.Put(m_DownPush.omega); t.PutDelimiter("\n");
		t.PutDelimiter("m_DownPush.trans: "); t.Put(m_DownPush.trans); t.PutDelimiter("\n");

		t.PutDelimiter("m_VelocityToPropDown.omega: "); t.Put(m_VelocityToPropDown.omega); t.PutDelimiter("\n");
		t.PutDelimiter("m_VelocityToPropDown.trans: "); t.Put(m_VelocityToPropDown.trans); t.PutDelimiter("\n");

		t.PutDelimiter("m_OrientParent: "); t.Put(m_OrientParent); t.PutDelimiter("\n");
		t.PutDelimiter("m_PositionParent: "); t.Put(m_PositionParent); t.PutDelimiter("\n");
		t.PutDelimiter("m_PositionChild: "); t.Put(m_PositionChild); t.PutDelimiter("\n");

		t.PutDelimiter("m_DriveState: "); t.Put(m_DriveState); t.PutDelimiter("\n");
		t.PutDelimiter("m_EnforceAccededLimits: "); t.Put(m_EnforceAccededLimits); t.PutDelimiter("\n");

		switch (m_JointType)
		{
		case phJoint::JNT_1DOF:
			static_cast<phJoint1Dof*>(this)->DebugSerialize(t);
			return;
		case phJoint::JNT_3DOF:
			static_cast<phJoint3Dof*>(this)->DebugSerialize(t);
			return;
		case phJoint::PRISM_JNT:
			return;
		}
	}
#endif // PHARTICULATED_DEBUG_SERIALIZE

} // namespace rage
