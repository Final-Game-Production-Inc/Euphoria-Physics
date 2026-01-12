//
// crskeleton/jointdata.cpp
//
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved.
//

#include "jointdata.h"

#include "skeletondata.h"

#include "atl/array_struct.h"
#include "bank/bank.h"
#include "data/safestruct.h"
#include "file/token.h"
#include "math/angmath.h"
#include "math/simplemath.h"
#include "parser/manager.h"
#include "vector/colors.h"

#include "jointdata_parser.h"

namespace rage
{

int crJointRotationLimit::sm_ControlPointFocus = 0;

const float crJointRotationLimit::sm_BinarySearchResolution = 0.01f * DtoR;

////////////////////////////////////////////////////////////////////////////////

#if __BANK
void crJointRotationLimit::SyncControlPoints()
{
	if (!m_UsePerControlTwistLimits)
	{
		for (int i = 0; i < sm_MaxControlPoints; i++)
		{
			m_ControlPoints[i].m_MinTwist = m_TwistLimitMin;
			m_ControlPoints[i].m_MaxTwist = m_TwistLimitMax;
		}
	}

	if (m_NumControlPoints < 2)
	{
		m_ControlPoints[1] = m_ControlPoints[0];
	}

	if (m_NumControlPoints < 4)
	{
		m_ControlPoints[2] = m_ControlPoints[0], m_ControlPoints[3] = m_ControlPoints[1];
	}

	if (m_NumControlPoints < 8)
	{
		float a2 = square(m_ControlPoints[0].m_MaxSwing), b2 = square(m_ControlPoints[1].m_MaxSwing);
		m_ControlPoints[4].m_MaxSwing = (a2 + b2) > 0 ? sqrtf(2.f * a2 * b2 / (a2 + b2)) : 0;

		a2 = b2, b2 = square(m_ControlPoints[2].m_MaxSwing);
		m_ControlPoints[5].m_MaxSwing = (a2 + b2) > 0 ? sqrtf(2.f * a2 * b2 / (a2 + b2)) : 0;

		a2 = b2, b2 = square(m_ControlPoints[3].m_MaxSwing);
		m_ControlPoints[6].m_MaxSwing = (a2 + b2) > 0 ? sqrtf(2.f * a2 * b2 / (a2 + b2)) : 0;

		a2 = b2, b2 = square(m_ControlPoints[0].m_MaxSwing);
		m_ControlPoints[7].m_MaxSwing = (a2 + b2) > 0 ? sqrtf(2.f * a2 * b2 / (a2 + b2)) : 0;

		m_ControlPoints[4].m_MinTwist = InterpolateAngle(0.5f, m_ControlPoints[0].m_MinTwist, m_ControlPoints[1].m_MinTwist);
		m_ControlPoints[4].m_MaxTwist = InterpolateAngle(0.5f, m_ControlPoints[0].m_MaxTwist, m_ControlPoints[1].m_MaxTwist);
		m_ControlPoints[5].m_MinTwist = InterpolateAngle(0.5f, m_ControlPoints[1].m_MinTwist, m_ControlPoints[2].m_MinTwist);
		m_ControlPoints[5].m_MaxTwist = InterpolateAngle(0.5f, m_ControlPoints[1].m_MaxTwist, m_ControlPoints[2].m_MaxTwist);
		m_ControlPoints[6].m_MinTwist = InterpolateAngle(0.5f, m_ControlPoints[2].m_MinTwist, m_ControlPoints[3].m_MinTwist);
		m_ControlPoints[6].m_MaxTwist = InterpolateAngle(0.5f, m_ControlPoints[2].m_MaxTwist, m_ControlPoints[3].m_MaxTwist);
		m_ControlPoints[7].m_MinTwist = InterpolateAngle(0.5f, m_ControlPoints[3].m_MinTwist, m_ControlPoints[0].m_MinTwist);
		m_ControlPoints[7].m_MaxTwist = InterpolateAngle(0.5f, m_ControlPoints[3].m_MaxTwist, m_ControlPoints[0].m_MaxTwist);
	}
}
#endif // __BANK

////////////////////////////////////////////////////////////////////////////////

void crJointRotationLimit::Reset()
{
	m_BoneID = -1;
	m_JointDOFs = JOINT_3_DOF;
	m_UseTwistLimits = false;
	m_UseEulerAngles = false;
	m_UsePerControlTwistLimits = false;
	m_NumControlPoints = 1;
	m_TwistAxis = Vec3V(V_X_AXIS_WZERO);
	m_TwistLimitMin = -PI;
	m_TwistLimitMax = PI;
	m_SoftLimitScale = 1.f;
	m_ZeroRotation = QuatV(V_IDENTITY);
	m_ZeroRotationEulers = Vec3V(V_ZERO);

	for (int i = 0; i < sm_MaxControlPoints; i++)
	{
		m_ControlPoints[i].m_MaxSwing = PI;
		m_ControlPoints[i].m_MinTwist = -PI;
		m_ControlPoints[i].m_MaxTwist = PI;
	}
}

////////////////////////////////////////////////////////////////////////////////

void crJointRotationLimit::InitFromEulers(Vec3V_In offset, Vec3V_In minRot, Vec3V_In maxRot)
{
	if (IsNearZero(minRot[1]) && IsNearZero(minRot[2]) && IsNearZero(maxRot[1]) && IsNearZero(maxRot[2]))
		m_JointDOFs = JOINT_1_DOF, m_TwistAxis = Vec3V(V_X_AXIS_WZERO);
	else if (IsNearZero(minRot[0]) && IsNearZero(minRot[2]) && IsNearZero(maxRot[0]) && IsNearZero(maxRot[2]))
		m_JointDOFs = JOINT_1_DOF, m_TwistAxis = Vec3V(V_Y_AXIS_WZERO);
	else if (IsNearZero(minRot[0]) && IsNearZero(minRot[1]) && IsNearZero(maxRot[0]) && IsNearZero(maxRot[1]))
		m_JointDOFs = JOINT_1_DOF, m_TwistAxis = Vec3V(V_Z_AXIS_WZERO);
	else
		m_JointDOFs = JOINT_3_DOF, m_TwistAxis = Normalize(offset);

	m_TwistLimitMin = Dot(minRot, m_TwistAxis).Getf();
	m_TwistLimitMax = Dot(maxRot, m_TwistAxis).Getf();
	m_SoftLimitScale = 1.f;

	m_UseTwistLimits = true;
	m_UsePerControlTwistLimits = false;

	if (m_JointDOFs == JOINT_3_DOF)
	{
		Vec3V primarySwingAxis, secondarySwingAxis;
		MakeOrthonormals(m_TwistAxis, primarySwingAxis, secondarySwingAxis);

		m_ZeroRotationEulers = minRot + maxRot;
		m_ZeroRotationEulers *= ScalarV(V_HALF);
		m_ZeroRotationEulers = SubtractScaled(m_ZeroRotationEulers, m_TwistAxis, Dot(m_ZeroRotationEulers, m_TwistAxis));
		m_ZeroRotation = QuatVFromEulersXYZ(m_ZeroRotationEulers);

		Mat34V zeroRotationMatrix(m_TwistAxis, primarySwingAxis, secondarySwingAxis, Vec3V(V_ZERO));

		QuatV q = QuatVFromMat33V(zeroRotationMatrix.GetMat33());
		m_ZeroRotation = Multiply(m_ZeroRotation, q);

		Vec3V maxDelta = maxRot - m_ZeroRotationEulers;

		m_ControlPoints[0].m_MaxSwing = fabsf(Dot(maxDelta, primarySwingAxis).Getf());
		m_ControlPoints[1].m_MaxSwing = fabsf(Dot(maxDelta, secondarySwingAxis).Getf());

		m_NumControlPoints = IsNearZero(m_ControlPoints[0].m_MaxSwing - m_ControlPoints[1].m_MaxSwing) ? 1 : 2;
	}
}

////////////////////////////////////////////////////////////////////////////////

void crJointRotationLimit::InitAsEulers(Vec3V_In minRot, Vec3V_In maxRot)
{
	m_UseEulerAngles = true;

	JointControlPoint& p0 = m_ControlPoints[0];
	p0.m_MaxSwing = minRot.GetXf();
	p0.m_MinTwist = minRot.GetYf();
	p0.m_MaxTwist = minRot.GetZf();
	JointControlPoint& p1 = m_ControlPoints[1];
	p1.m_MaxSwing = maxRot.GetXf();
	p1.m_MinTwist = maxRot.GetYf();
	p1.m_MaxTwist = maxRot.GetZf();
}

////////////////////////////////////////////////////////////////////////////////

bool crJointRotationLimit::ConvertToEulers(Vec3V_InOut minRot, Vec3V_InOut maxRot) const
{
	if (m_JointDOFs == JOINT_3_DOF && !m_UseEulerAngles)
		return false;

	if (m_UseEulerAngles)
	{
		const JointControlPoint& p0 = m_ControlPoints[0];
		minRot = Vec3V(p0.m_MaxSwing, p0.m_MinTwist, p0.m_MaxTwist);
		const JointControlPoint& p1 = m_ControlPoints[1];
		maxRot = Vec3V(p1.m_MaxSwing, p1.m_MinTwist, p1.m_MaxTwist);
	}
	else
	{	
		Vec3V twistAxis = m_TwistAxis;
		QuatV twistMin = QuatVFromAxisAngle(twistAxis, ScalarVFromF32(m_TwistLimitMin));
		minRot = QuatVToEulersXYZ(twistMin);
		QuatV twistMax = QuatVFromAxisAngle(twistAxis, ScalarVFromF32(m_TwistLimitMax));
		maxRot = QuatVToEulersXYZ(twistMax);
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////

#if __PFDRAW

const float PFD_JointLength = 0.1f;

void TwistSwingDecomp(QuatV_In q, Vec3V_In twistAxis, QuatV_InOut twist, QuatV_InOut swing)
{
	Vec3V tmp = Normalize(Vec3V(q.GetXf(), q.GetYf(), q.GetZf()));

	float dot = fabsf(Dot(tmp, twistAxis).Getf());

	if (fabsf(dot - 1) < SMALL_FLOAT)
	{
		twist = q;
		swing = QuatV(V_IDENTITY);
	}
	else if (dot < SMALL_FLOAT)
	{
		twist = QuatV(V_IDENTITY);
		swing = q;
	}
	else
	{
		tmp = Transform(q, twistAxis);
		swing = QuatVFromVectors(twistAxis, tmp);
		twist = Multiply(InvertNormInput(swing), q);
	}
}

////////////////////////////////////////////////////////////////////////////////

void crJointRotationLimit::Draw(Mat34V_In parentMatrix, Mat34V_In jointMatrix, Mat34V_In jointLocalMatrix,  Mat34V_In childLocalMatrix) const
{
	static const ScalarV s_oneDOFThreshold(0.01f);

	ScalarV length = ScalarVFromF32(PFD_JointLength);

	Mat34V identitySpace;
	identitySpace.Set3x3(parentMatrix);
	identitySpace.SetCol3(jointMatrix.GetCol3());

	Vec3V childOffset = Scale(childLocalMatrix.GetCol3(),  length / Mag(childLocalMatrix.GetCol3()));

	Vec3V primarySwingAxis, secondarySwingAxis;
	MakeOrthonormals(m_TwistAxis, primarySwingAxis, secondarySwingAxis);

	if (m_JointDOFs == JOINT_1_DOF)
	{
		Vec3V planarChildOffset = SubtractScaled(childOffset, m_TwistAxis, Dot(childOffset, m_TwistAxis));

		// If this is true then the bend axis is parallel to the bone and this
		// is a twist-only joint (a roll bone), and we need to draw a line
		// perpendicular to the bone to show the current twist amount
		if (IsLessThanAll(MagSquared(planarChildOffset), s_oneDOFThreshold))
		{
			Vec3V currentTwist = Transform(jointLocalMatrix, primarySwingAxis);
			currentTwist = SubtractScaled(currentTwist, m_TwistAxis, Dot(currentTwist, m_TwistAxis));

			Draw1DOF(identitySpace, m_TwistAxis, primarySwingAxis, m_TwistLimitMin, m_TwistLimitMax, &currentTwist);
		}
		else
		{
			Draw1DOF(identitySpace, m_TwistAxis, planarChildOffset, m_TwistLimitMin, m_TwistLimitMax);
		}
	}
	else
	{
		if (m_UseTwistLimits)
		{
			QuatV swing, twist;
			QuatV joint = QuatVFromMat33V(jointLocalMatrix.GetMat33());
			TwistSwingDecomp(joint, m_TwistAxis, twist, swing);

			Mat34V twistSpace;
			Mat34VFromQuatV(twistSpace, swing);
			Transform3x3(twistSpace,identitySpace,twistSpace);
			twistSpace.SetCol3(identitySpace.GetCol3());

			Vec3V currentTwist = Transform(twist, primarySwingAxis);

			float min, max;

			if (sm_ControlPointFocus == 0)
			{
				Vec3V endPoint = Transform(joint, m_TwistAxis);
				endPoint = UnTransform(m_ZeroRotation, endPoint);

				GetTwistLimitsAtPoint(endPoint, min, max);
			}
			else
				min = m_ControlPoints[sm_ControlPointFocus - 1].m_MinTwist, max = m_ControlPoints[sm_ControlPointFocus - 1].m_MaxTwist;

			Draw1DOF(twistSpace, m_TwistAxis, primarySwingAxis, min, max, &currentTwist);
		}

		Mat34V swingSpace;
		Mat34VFromQuatV(swingSpace, m_ZeroRotation);
		Transform3x3(swingSpace,identitySpace,swingSpace);
		swingSpace.SetCol3(identitySpace.GetCol3());

		Vec3V current = Transform(jointLocalMatrix, Vec3V(V_X_AXIS_WZERO));
		Draw3DOF(swingSpace, &current);
	}
}

////////////////////////////////////////////////////////////////////////////////

void crJointRotationLimit::Draw1DOF(Mat34V_In worldMtx, Vec3V_In axis, Vec3V_In offset, float min, float max, const Vec3V* current) const
{
	static const int NUM_ARC_POINTS = 25;

	Vec3V scaled;

	ScalarV length = ScalarVFromF32(PFD_JointLength);

	grcWorldMtx(worldMtx);

	if (current == NULL)
	{
		grcColor(Color_white);

		grcBegin(drawLines, 2);
		scaled = Scale(axis, ScalarV(V_HALF) * length);
		grcVertex3f(scaled);
		scaled = Scale(axis, ScalarV(V_NEGHALF) * length);
		grcVertex3f(scaled);
		grcEnd();
	}

	grcColor(Color_green);

	grcBegin(drawLineStrip, NUM_ARC_POINTS + 4);
	grcVertex3f(VEC3_ZERO);

	scaled = Scale(offset, length / Mag(offset));

	Vec3V pointAlongArc;

	for (int i = 0; i < NUM_ARC_POINTS; i++)
	{
		ScalarV angle(i * (max - min) / (NUM_ARC_POINTS - 1) + min);
		QuatV twist = QuatVFromAxisAngle(axis, angle);
		pointAlongArc = Transform(twist, scaled);
		if (i == 0)	grcVertex3f(pointAlongArc);
		pointAlongArc = Scale(pointAlongArc, ScalarV(V_HALF));
		grcVertex3f(pointAlongArc);
	}

	pointAlongArc = Scale(pointAlongArc, ScalarV(V_TWO));
	grcVertex3f(pointAlongArc);
	grcVertex3f(VEC3_ZERO);
	grcEnd();

	if (current)
	{
		scaled = Scale(*current, length / Mag(*current));

		grcColor(Color_red);

		grcBegin(drawLines, 2);
		grcVertex3f(VEC3_ZERO);
		grcVertex3f(scaled);
		grcEnd();		
	}
}

////////////////////////////////////////////////////////////////////////////////

void crJointRotationLimit::Draw3DOF(Mat34V_In worldMtx, const Vec3V* current) const
{
	static const int NUM_ARC_POINTS = 16;

	ScalarV length = ScalarVFromF32(PFD_JointLength);
	int i;

	Vec3V* pointsAlongArc = Alloca(Vec3V, NUM_ARC_POINTS);

	for (i = 0; i < NUM_ARC_POINTS; i++)
	{
		float theta = i * 2.0f * PI / NUM_ARC_POINTS;
		float phi = CalcPhiAtTheta(theta);

		SphericalToPoint(pointsAlongArc[i], theta, phi, length);
	}

	grcWorldMtx(worldMtx);

	grcColor(Color_white);

	grcBegin(drawLineStrip, NUM_ARC_POINTS + 1);
	for (i = 0; i < NUM_ARC_POINTS; i++)
		grcVertex3f(pointsAlongArc[i]);
	grcVertex3f(pointsAlongArc[0]);
	grcEnd();

	grcBegin(drawLines, 2 * NUM_ARC_POINTS);
	for (i = 0; i < NUM_ARC_POINTS; i++)
	{
		grcVertex3f(VEC3_ZERO);
		grcVertex3f(pointsAlongArc[i]);
	}
	grcEnd();

	grcColor(Color32(0.71f,0.80f,0.80f,0.3f));

	grcBegin(drawTris, NUM_ARC_POINTS * 3);
	
	for (i = 0; i < NUM_ARC_POINTS - 1; i++)
	{
		grcVertex3f(VEC3_ZERO);
		grcVertex3f(pointsAlongArc[i]);
		grcVertex3f(pointsAlongArc[i + 1]);
	}
	grcVertex3f(pointsAlongArc[i - 1]);
	grcVertex3f(pointsAlongArc[i]);
	grcVertex3f(pointsAlongArc[0]);
	grcEnd();

	for (i = 0; i < sm_MaxControlPoints; i++)
	{
		float theta = i * 2.0f * PI / sm_MaxControlPoints;
		float phi = CalcPhiAtTheta(theta);

		Vec3V p;
		SphericalToPoint(p, theta, phi, length);
		p = Transform(worldMtx, p);

		static int points[sm_MaxControlPoints] = { 1, 5, 2, 6, 3, 7, 4, 8 };
		grcColor((sm_ControlPointFocus == points[i]) ? Color_red : Color_white);
		grcDrawSphere(0.005f, p, 8, false, true);
	}

	grcWorldMtx(worldMtx);

	if (current)
	{
		Vec3V scaled = Scale(*current, length / Mag(*current));

		grcColor(Color_red);

		grcBegin(drawLines, 2);
		grcVertex3f(VEC3_ZERO);
		grcVertex3f(scaled);
		grcEnd();		
	}
}

////////////////////////////////////////////////////////////////////////////////

void crJointRotationLimit::Draw3DOFSphere(Mat34V_In worldMtx, const Vec3V* current) const
{
	static const int NUM_ARC_POINTS = 16;

	ScalarV length = ScalarVFromF32(PFD_JointLength);

	Vec3V* pointsAlongArc = Alloca(Vec3V, NUM_ARC_POINTS);

	for (int i = 0; i < NUM_ARC_POINTS; i++)
	{
		float theta = i * 2.0f * PI / NUM_ARC_POINTS;
		float phi = CalcPhiAtTheta(theta);

		SphericalToPoint(pointsAlongArc[i], theta, phi, length);
	}

	grcWorldMtx(worldMtx);

	grcColor(Color_white);

	grcBegin(drawLineStrip, NUM_ARC_POINTS + 1);
	for (int i = 0; i < NUM_ARC_POINTS; i++)
		grcVertex3f(pointsAlongArc[i]);
	grcVertex3f(pointsAlongArc[0]);
	grcEnd();

	grcColor(Color32(0.71f,0.80f,0.80f,0.3f));
	grcDrawSphere(length.Getf(), worldMtx.GetCol3(), NUM_ARC_POINTS, false, true);

	for (int i = 0; i < sm_MaxControlPoints; i++)
	{
		float theta = i * 2.0f * PI / sm_MaxControlPoints;
		float phi = CalcPhiAtTheta(theta);

		Vec3V p;
		SphericalToPoint(p, theta, phi, length);
		p = Transform(worldMtx, p);

		static int points[sm_MaxControlPoints] = { 1, 5, 2, 6, 3, 7, 4, 8 };
		grcColor((sm_ControlPointFocus == points[i]) ? Color_red : Color_white);
		grcDrawSphere(0.005f, p, 8, false, true);
	}

	grcWorldMtx(worldMtx);

	if (current)
	{
		Vec3V scaled = Scale(*current, length / Mag(*current));

		grcColor(Color_red);

		grcBegin(drawLines, 2);
		grcVertex3f(VEC3_ZERO);
		grcVertex3f(scaled);
		grcEnd();		
	}
}
#endif // __PFDRAW

////////////////////////////////////////////////////////////////////////////////

void crJointRotationLimit::ApplyLimits(Mat34V_InOut inOut, const Mat34V* prevFrame) const
{
	QuatV q = QuatVFromMat33V( inOut.GetMat33() );
	if (prevFrame)
	{
		QuatV r = QuatVFromMat33V( prevFrame->GetMat33() );
		ApplyLimits( q, &r );
	}
	else
	{
		ApplyLimits( q );
	}
	Mat34VFromQuatV( inOut, q, inOut.GetCol3() );
}

////////////////////////////////////////////////////////////////////////////////

void crJointRotationLimit::ApplyLimits(Mat34V_InOut inOut, const QuatV* prevFrame) const
{
	QuatV q = QuatVFromMat33V(inOut.GetMat33());
	ApplyLimits( q, prevFrame);

	Mat34VFromQuatV( inOut, q, inOut.GetCol3() );
}

////////////////////////////////////////////////////////////////////////////////

void crJointRotationLimit::ApplyLimits(QuatV_InOut inOut, const QuatV* prevFrame) const
{
	Assert(!m_UseEulerAngles);

	ScalarV twistAngle = QuatVTwistAngle( inOut, m_TwistAxis );

	if (m_JointDOFs == JOINT_1_DOF)
	{
		twistAngle =  ScalarVFromF32( AngleClamp( twistAngle.Getf(), m_TwistLimitMin, m_TwistLimitMax) );
		inOut = QuatVFromAxisAngle( m_TwistAxis, twistAngle );
		return;
	}

	Vec3V endPoint = Transform( inOut, m_TwistAxis );
	endPoint = UnTransform( m_ZeroRotation, endPoint );

	if (prevFrame)
	{
		Vec3V v = Transform( *prevFrame, m_TwistAxis );
		v = UnTransform( m_ZeroRotation, v );
		FindNearestPoint( endPoint, &v );
	}
	else
	{
		FindNearestPoint( endPoint );
	}

	twistAngle = ScalarVFromF32( LimitTwistAngle( endPoint, twistAngle.Getf() ) );

	endPoint = Transform( m_ZeroRotation, endPoint );
	inOut = QuatVFromVectors( m_TwistAxis, endPoint );

	QuatV twist = QuatVFromAxisAngle( m_TwistAxis, twistAngle );
	inOut = Multiply( inOut, twist);
}

////////////////////////////////////////////////////////////////////////////////

void crJointRotationLimit::ApplyEulerLimits(QuatV_InOut inOut, u32 dofs) const
{
	Assert(m_UseEulerAngles);

	JointControlPoint p0 = m_ControlPoints[0], p1 = m_ControlPoints[1];
	Vec3V min = Vec3V(p0.m_MaxSwing, p0.m_MinTwist, p0.m_MaxTwist);
	Vec3V max = Vec3V(p1.m_MaxSwing, p1.m_MinTwist, p1.m_MaxTwist);
	Vec3V euler;
	// DARCHARD: This I'm not so sure about. If there's no limit for a DOF, we have to make sure
	//	there limits in the join data are max'd out (should be done when creating the joint limit).

	if(dofs & crBoneData::HAS_ROTATE_LIMITS)
	{
		euler = QuatVToEulersXYZ(inOut);
		euler[0] = AngleClamp(euler[0], min[0], max[0]);
		inOut = QuatVFromEulersXYZ(euler);
	}
	if(dofs & crBoneData::HAS_ROTATE_LIMITS)
	{
		euler = QuatVToEulersYXZ(inOut);
		euler[1] = AngleClamp(euler[1], min[1], max[1]);
		inOut = QuatVFromEulersYXZ(euler);
	}
	if(dofs & crBoneData::HAS_ROTATE_LIMITS)
	{
		euler = QuatVToEulersZXY(inOut);
		euler[2] = AngleClamp(euler[2], min[2], max[2]);
		inOut = QuatVFromEulersZXY(euler);
	}
}

////////////////////////////////////////////////////////////////////////////////

void crJointRotationLimit::ApplyEulerLimits(QuatV_InOut inOut) const
{
	Assert(m_UseEulerAngles);

	Vec3V e = QuatVToEulersXYZ(inOut);
	JointControlPoint p0 = m_ControlPoints[0], p1 = m_ControlPoints[1];
	Vec3V min = Vec3V(p0.m_MaxSwing, p0.m_MinTwist, p0.m_MaxTwist);
	Vec3V max = Vec3V(p1.m_MaxSwing, p1.m_MinTwist, p1.m_MaxTwist);
	e = Clamp(e, min, max);
	inOut = QuatVFromEulersXYZ(e);
}

////////////////////////////////////////////////////////////////////////////////

void crJointRotationLimit::GetTwistLimitsAtPoint(Vec3V_In endPoint, float& min, float& max) const
{
	if (!m_UsePerControlTwistLimits || m_NumControlPoints == 1)
	{
		min = m_TwistLimitMin, max = m_TwistLimitMax;
		return;
	}

	float theta, phi;
	PointToSpherical(endPoint, theta, phi);

	theta = CanonicalizeAngleFast(theta);
	phi /= CalcPhiAtTheta(theta);

	int p1 = 0, p2 = 1;

	if (m_NumControlPoints >= 8)
	{
		static const int points[10] = { 2, 6, 3, 7, 0, 4, 1, 5, 2, 6 }; // MAGIC (don't ask)

		int index = int((theta + PI) / (0.25f * PI));
		Assert(index >= 0 && index <= 8);

		p1 = points[index];
		p2 = points[index + 1];

		theta = (theta + PI) / (0.25f * PI) - index;
	}
	else if (m_NumControlPoints >= 4)
	{
		static const int points[6] = { 2, 3, 0, 1, 2, 3 }; // MAGIC (don't ask)

		int index = int((theta + PI) / (0.5f * PI));
		Assert(index >= 0 && index <= 4);

		p1 = points[index];
		p2 = points[index + 1];

		theta = (theta + PI) / (0.5f * PI) - index;
	}
	else
	{
		static const int points[4] = { 0, 1, 0, 1 }; // MAGIC (don't ask)

		int index = int((theta + PI) / PI);
		Assert(index >= 0 && index <= 2);

		p1 = points[index];
		p2 = points[index + 1];

		theta = (theta + PI) / PI - index;
	}

	theta = Clamp(theta, 0.f, 1.f);
	phi = Clamp(phi, 0.f, 1.f);

	min = InterpolateAngle(phi, m_TwistLimitMin, InterpolateAngle(theta, m_ControlPoints[p1].m_MinTwist, m_ControlPoints[p2].m_MinTwist));
	max = InterpolateAngle(phi, m_TwistLimitMax, InterpolateAngle(theta, m_ControlPoints[p1].m_MaxTwist, m_ControlPoints[p2].m_MaxTwist));
}

////////////////////////////////////////////////////////////////////////////////

float crJointRotationLimit::LimitTwistAngle(Vec3V_In endPoint, float twistAngle) const
{
	if (!m_UseTwistLimits)
		return twistAngle;

	float min, max;
	GetTwistLimitsAtPoint(endPoint, min, max);
	return AngleClamp(twistAngle, min, max);
}

////////////////////////////////////////////////////////////////////////////////

void crJointRotationLimit::PointToSpherical(Vec3V_In point, float& theta, float& phi)
{
	theta = atan2f(point[1], point[2]);
	phi = acosf(point[0]);
}

////////////////////////////////////////////////////////////////////////////////

void crJointRotationLimit::SphericalToPoint(Vec3V_InOut point, float theta, float phi, ScalarV_In r)
{
	float sinPhi = sinf(phi);

	point[0] = cosf(phi);
	point[1] = sinf(theta) * sinPhi;
	point[2] = cosf(theta) * sinPhi;

	point = Scale(point, r);
}

////////////////////////////////////////////////////////////////////////////////

float crJointRotationLimit::CalcPhiAtTheta(float theta) const
{
	if (m_NumControlPoints == 1)
		return m_ControlPoints[0].m_MaxSwing;

	float a2, b2;

	theta = CanonicalizeAngleFast(theta);

	if (m_NumControlPoints >= 8)
	{
		static const int points[10] = { 2, 6, 3, 7, 0, 4, 1, 5, 2, 6 }; // MAGIC (don't ask)
		static const bool swap[9] = { false, false, true, true, false, false, true, true, false };

		int index = int((theta + PI) / (0.25f * PI));
		Assert(index >= 0 && index <= 8);

		a2 = square(m_ControlPoints[points[index]].m_MaxSwing);
		b2 = square(m_ControlPoints[points[index + 1]].m_MaxSwing);

		if (index & 1)
			a2 = a2 * b2 / (2.f * b2 - a2);
		else
			b2 = a2 * b2 / (2.f * a2 - b2);

		if (swap[index])
			std::swap(a2, b2);
	}
	else if (m_NumControlPoints >= 4)
	{
		a2 = square(m_ControlPoints[(fabsf(theta) >= 0.5f * PI) ? 2 : 0].m_MaxSwing);
		b2 = square(m_ControlPoints[(theta >= 0.f) ? 1 : 3].m_MaxSwing);
	}
	else
	{
		a2 = square(m_ControlPoints[0].m_MaxSwing);
		b2 = square(m_ControlPoints[1].m_MaxSwing);
	}

	float s2 = square(sinf(theta));

	return sqrtf(a2 * b2 / (a2 * s2 + b2 * (1.f - s2)));
}

////////////////////////////////////////////////////////////////////////////////

void crJointRotationLimit::FindNearestPoint(Vec3V_InOut endPoint, const Vec3V* prevFrame) const
{
	float origTheta, origPhi;
	PointToSpherical(endPoint, origTheta, origPhi);

	float bestPhi = CalcPhiAtTheta(origTheta);

	if (origPhi > bestPhi)
	{
		if (m_NumControlPoints == 1)
		{
			SphericalToPoint(endPoint, origTheta, bestPhi);
		}
		else
		{
			float cosOrigPhi = cosf(origPhi), sinOrigPhi = sinf(origPhi);
			float prevTheta = 0, prevPhi = 0, cosPrevPhi = 0, sinPrevPhi = 0;

			float bestTheta = origTheta;
			float stepSize = 0.5f * PI;
			float bestDist = acosf(sinOrigPhi * sinf(bestPhi) + cosOrigPhi * cosf(bestPhi));

			if (0 && prevFrame)
			{
				PointToSpherical(*prevFrame, prevTheta, prevPhi);
				cosPrevPhi = cosf(prevPhi), sinPrevPhi = sinf(prevPhi);
				bestDist += 0.1f * (acosf(sinPrevPhi * sinf(bestPhi) + cosPrevPhi * cosf(bestPhi)));
			}

			while (fabsf(stepSize) > sm_BinarySearchResolution)
			{
				while (true)
				{
					float theta = bestTheta + stepSize;
					float phi = CalcPhiAtTheta(theta);
					float dist = acosf(sinOrigPhi * sinf(phi) * cosf(origTheta - theta) + cosOrigPhi * cosf(phi));

					if (0 && prevFrame)
						dist += 0.1f * (acosf(sinPrevPhi * sinf(phi) * cosf(prevTheta - theta) + cosPrevPhi * cosf(phi)));

					if (dist < bestDist)
						bestTheta = theta, bestPhi = phi, bestDist = dist;
					else
						break;
				}

				stepSize *= -0.5f;
			}

			SphericalToPoint(endPoint, bestTheta, bestPhi);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

float CosSphericalDist(float theta1, float phi1, float theta2, float phi2)
{
	return cosf(phi1) * cosf(phi2) * cosf(theta1 - theta2) + sinf(phi1) * sinf(phi2);
}

////////////////////////////////////////////////////////////////////////////////

#if __BANK && !__SPU
void crJointRotationLimit::AddWidgets(bkBank& bank)
{
	bank.AddSlider("Select Control Point", &sm_ControlPointFocus, 0, 8, 1);
	PARSER.AddWidgets(bank, this);
}
#endif

////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_PLACE(crJointData);

////////////////////////////////////////////////////////////////////////////////

#if __DECLARESTRUCT
datSwapper_ENUM(crJointRotationLimit::JointDOFs);

////////////////////////////////////////////////////////////////////////////////

void crJointRotationLimit::JointControlPoint::DeclareStruct(datTypeStruct &s)
{
	SSTRUCT_BEGIN(JointControlPoint)
	SSTRUCT_FIELD(JointControlPoint, m_MaxSwing)
	SSTRUCT_FIELD(JointControlPoint, m_MinTwist)
	SSTRUCT_FIELD(JointControlPoint, m_MaxTwist)
	SSTRUCT_END(JointControlPoint)
}

////////////////////////////////////////////////////////////////////////////////

void crJointRotationLimit::DeclareStruct(datTypeStruct &s)
{
	SSTRUCT_BEGIN(crJointRotationLimit)
	SSTRUCT_IGNORE(crJointRotationLimit, m_VirtualPtr)
	SSTRUCT_FIELD(crJointRotationLimit, m_BoneID)
	SSTRUCT_FIELD(crJointRotationLimit, m_NumControlPoints)
	SSTRUCT_FIELD(crJointRotationLimit, m_JointDOFs)
	SSTRUCT_FIELD(crJointRotationLimit, m_ZeroRotation)
	SSTRUCT_FIELD(crJointRotationLimit, m_ZeroRotationEulers)
	SSTRUCT_FIELD(crJointRotationLimit, m_TwistAxis)
	SSTRUCT_FIELD(crJointRotationLimit, m_TwistLimitMin)
	SSTRUCT_FIELD(crJointRotationLimit, m_TwistLimitMax)
	SSTRUCT_FIELD(crJointRotationLimit, m_SoftLimitScale)
	SSTRUCT_CONTAINED_ARRAY(crJointRotationLimit, m_ControlPoints)
	SSTRUCT_FIELD(crJointRotationLimit, m_UseTwistLimits)
	SSTRUCT_FIELD(crJointRotationLimit, m_UseEulerAngles)
	SSTRUCT_FIELD(crJointRotationLimit, m_UsePerControlTwistLimits)
	SSTRUCT_IGNORE(crJointRotationLimit, m_Padding)
	SSTRUCT_END(crJointRotationLimit)
}

////////////////////////////////////////////////////////////////////////////////

void crJointTranslationLimit::DeclareStruct(datTypeStruct &s)
{
	SSTRUCT_BEGIN(crJointTranslationLimit)
	SSTRUCT_IGNORE(crJointTranslationLimit, m_VirtualPtr)
	SSTRUCT_FIELD(crJointTranslationLimit, m_BoneID)
	SSTRUCT_IGNORE(crJointTranslationLimit, m_Padding)
	SSTRUCT_FIELD(crJointTranslationLimit, m_LimitMin)
	SSTRUCT_FIELD(crJointTranslationLimit, m_LimitMax)
	SSTRUCT_END(crJointTranslationLimit)
}

////////////////////////////////////////////////////////////////////////////////

void crJointScaleLimit::DeclareStruct(datTypeStruct &s)
{
	SSTRUCT_BEGIN(crJointScaleLimit)
	SSTRUCT_IGNORE(crJointScaleLimit, m_VirtualPtr)
	SSTRUCT_FIELD(crJointScaleLimit, m_BoneID)
	SSTRUCT_IGNORE(crJointScaleLimit, m_Padding)
	SSTRUCT_FIELD(crJointScaleLimit, m_LimitMin)
	SSTRUCT_FIELD(crJointScaleLimit, m_LimitMax)
	SSTRUCT_END(crJointScaleLimit)
}

////////////////////////////////////////////////////////////////////////////////

void crJointData::DeclareStruct(datTypeStruct &s)
{
	pgBase::DeclareStruct(s);

	SSTRUCT_BEGIN_BASE(crJointData, pgBase)
	SSTRUCT_DYNAMIC_ARRAY(crJointData, m_RotationLimits, m_NumRotationLimits)
	SSTRUCT_DYNAMIC_ARRAY(crJointData, m_TranslationLimits, m_NumTranslationLimits)
	SSTRUCT_DYNAMIC_ARRAY(crJointData, m_ScaleLimits, m_NumScaleLimits)
	SSTRUCT_FIELD(crJointData, m_Name)
	SSTRUCT_FIELD(crJointData, m_NumRotationLimits)
	SSTRUCT_FIELD(crJointData, m_NumTranslationLimits)
	SSTRUCT_FIELD(crJointData, m_NumScaleLimits)
	SSTRUCT_FIELD(crJointData, m_RefCount)
	SSTRUCT_END(crJointData)
}

#endif // __DECLARESTRUCT

////////////////////////////////////////////////////////////////////////////////

crJointTranslationLimit::crJointTranslationLimit()
: m_BoneID(-1)
, m_LimitMin(V_ZERO)
, m_LimitMax(V_ZERO)
{
}

////////////////////////////////////////////////////////////////////////////////

void crJointTranslationLimit::Init(Vec3V_In min, Vec3V_In max)
{
	m_LimitMin = min;
	m_LimitMax = max;
}

////////////////////////////////////////////////////////////////////////////////

void crJointTranslationLimit::ApplyLimits(Vec3V_InOut inOut, u32 dofs) const
{
	if(dofs & crBoneData::HAS_TRANSLATE_LIMITS)
	{
		inOut[0] = Clamp(inOut[0], m_LimitMin[0], m_LimitMax[0]);
	}
	if(dofs & crBoneData::HAS_TRANSLATE_LIMITS)
	{
		inOut[1] = Clamp(inOut[1], m_LimitMin[1], m_LimitMax[1]);
	}
	if(dofs & crBoneData::HAS_TRANSLATE_LIMITS)
	{
		inOut[2] = Clamp(inOut[2], m_LimitMin[2], m_LimitMax[2]);
	}
}

////////////////////////////////////////////////////////////////////////////////

crJointScaleLimit::crJointScaleLimit()
: m_BoneID(-1)
, m_LimitMin(V_ZERO)
, m_LimitMax(V_ZERO)
{
}

////////////////////////////////////////////////////////////////////////////////

void crJointScaleLimit::Init(Vec3V_In min, Vec3V_In max)
{
	m_LimitMin = min;
	m_LimitMax = max;
}

////////////////////////////////////////////////////////////////////////////////

void crJointScaleLimit::ApplyLimits(Vec3V_InOut inOut, u32 dofs) const
{
	if (dofs & crBoneData::HAS_SCALE_LIMITS)
	{
		inOut[0] = Clamp(inOut[0], m_LimitMin[0], m_LimitMax[0]);
	}
	if (dofs & crBoneData::HAS_SCALE_LIMITS)
	{
		inOut[1] = Clamp(inOut[1], m_LimitMin[1], m_LimitMax[1]);
	}
	if (dofs & crBoneData::HAS_SCALE_LIMITS)
	{
		inOut[2] = Clamp(inOut[2], m_LimitMin[2], m_LimitMax[2]);
	}
}

////////////////////////////////////////////////////////////////////////////////

crJointData::crJointData()
: m_RotationLimits(NULL)
, m_TranslationLimits(NULL)
, m_ScaleLimits(NULL)
, m_NumRotationLimits(0)
, m_NumTranslationLimits(0)
, m_NumScaleLimits(0)
, m_RefCount(1)
{
}

////////////////////////////////////////////////////////////////////////////////

crJointData::~crJointData()
{
	delete [] m_RotationLimits;
	delete [] m_TranslationLimits;
	delete [] m_ScaleLimits;
}

////////////////////////////////////////////////////////////////////////////////

crJointData::crJointData(datResource& rsc)
: m_Name(rsc)
{
	rsc.PointerFixup(m_RotationLimits);
	rsc.PointerFixup(m_TranslationLimits);
	rsc.PointerFixup(m_ScaleLimits);
}

////////////////////////////////////////////////////////////////////////////////

#if CR_DEV && !__SPU
crJointData* crJointData::AllocateAndLoad(const char* filename)
{
	Assert(filename);
	crJointData* jointData = rage_new crJointData;
	if (!jointData->Load(filename))
	{
		Errorf("crJointData::AllocateAndLoad - failed to load '%s'", filename);
		delete jointData;
		return NULL;
	}

	return jointData;
}

////////////////////////////////////////////////////////////////////////////////

bool crJointData::Load(const char* filename)
{
	fiStream* f = ASSET.Open(filename, "jlimits", false, true);
	if (f==NULL)
	{
		Errorf("crJointData::Load - failed to open file '%s'", filename);
		return false;
	}

	sysMemStartTemp();
	parTree* tree = PARSER.LoadTree(f);
	sysMemEndTemp();

	bool result = true;

	if (!PARSER.LoadObject(tree->GetRoot(), *this))
	{
		Errorf("crJointData::Load - failed to parse '%s'", filename);
		result = false;
	}

	sysMemStartTemp();
	delete tree;
	sysMemEndTemp();

	m_Name = filename;

	f->Close();
	return result;
}

////////////////////////////////////////////////////////////////////////////////

bool crJointData::Save(const char* filename)
{
	fiStream* f = ASSET.Create(filename, "jlimits");
	if (f==NULL)
	{
		return false;
	}

	PARSER.SaveObject(f, this);

	f->Close();

	return true;
}

////////////////////////////////////////////////////////////////////////////////

bool crJointData::InitFromSkeletonData(const char* filename, eSkelDataInit init)
{
	fiStream* f = ASSET.Open(filename, "skel", false, true);
	if(f==0)
	{
		Errorf("crJointData::InitFromSkeletonData - failed to open file '%s'", filename);
		return false;
	}

	fiTokenizer T(filename,f);
	char buffer[256];

	int version;
	if (T.CheckIToken("version:"))
	{
		version = T.GetInt();
		Assertf((version>=101) && (version <= 109), "crJointData::InitFromSkeletonData - skeleton '%s' has unknown version %d.\r\nProbably caused by skeleton having been exported with copy of REX built using newer version of Rage than this executable.\r\n",filename,version);
	}
	else
	{
		// pre-versioning, < 01/29/2002
		version=100;
	}

	if(T.CheckIToken("AuthoredOrientation"))
	{
		T.GetInt();
	}

	T.MatchToken("NumBones");
	int nb = T.GetInt();
	AssertMsg(nb <= 65535-1 , "Too many bones");

	// find first bone
	T.GetToken(buffer,sizeof(buffer));
	bool valid = strncmp(buffer,"bone",4) == 0;

	bool success = false;
	if (valid)
	{
		// Let's do this instead, when we load the bone data, we always create the limits
		// using the index we know about. When we're all done we create a reduced set
		// of those limits that actually have the limit information.

		sysMemStartTemp();
		crJointRotationLimit* rotationLimits = rage_new crJointRotationLimit[nb];
		crJointTranslationLimit* translationLimits = rage_new crJointTranslationLimit[nb];
		crJointScaleLimit* scaleLimits = rage_new crJointScaleLimit[nb];
		sysMemEndTemp();

		int index = 0;

		switch (version) {
			case 100:
				success = LoadBone_v100(T, rotationLimits, translationLimits, scaleLimits, index, init);
				break;
			case 101:
			case 102:
			case 103:
				success = LoadBone_v101(T, rotationLimits, translationLimits, scaleLimits, index, init);
				break;
			case 104:
			case 105:
			case 106:
			case 107:
			case 108:
			case 109:
				// intentional fall through
				success = LoadBone_v104Plus(T, rotationLimits, translationLimits, scaleLimits, index, init);
				break;
		}

		// Do something with the limits now we have them

		if (m_NumRotationLimits)
		{
			m_RotationLimits = rage_new crJointRotationLimit[m_NumRotationLimits];
			crJointRotationLimit* curr = m_RotationLimits;
			for (int i=0; i<nb; ++i)
			{
				crJointRotationLimit& limit = rotationLimits[i];
				if (limit.GetBoneID() != -1)
				{
					Assert(curr < m_RotationLimits+m_NumRotationLimits);

					curr->m_BoneID = limit.m_BoneID;
					curr->m_NumControlPoints = limit.m_NumControlPoints;
					curr->m_JointDOFs = limit.m_JointDOFs;
					curr->m_ZeroRotation = limit.m_ZeroRotation;
					curr->m_ZeroRotationEulers = limit.m_ZeroRotationEulers;
					curr->m_TwistAxis = limit.m_TwistAxis;
					curr->m_TwistLimitMin = limit.m_TwistLimitMin;
					curr->m_TwistLimitMax = limit.m_TwistLimitMax;
					curr->m_SoftLimitScale = limit.m_SoftLimitScale;
					for (int c=0; c<crJointRotationLimit::sm_MaxControlPoints; ++c)
					{
						curr->m_ControlPoints[c].m_MaxSwing = limit.m_ControlPoints[c].m_MaxSwing;
						curr->m_ControlPoints[c].m_MinTwist = limit.m_ControlPoints[c].m_MinTwist;
						curr->m_ControlPoints[c].m_MaxTwist = limit.m_ControlPoints[c].m_MaxTwist;
					}
					curr->m_UseTwistLimits = limit.m_UseTwistLimits;
					curr->m_UseEulerAngles = limit.m_UseEulerAngles;
					curr->m_UsePerControlTwistLimits = limit.m_UsePerControlTwistLimits;

					++curr;
				}
			}
		}

		if (m_NumTranslationLimits)
		{
			m_TranslationLimits = rage_new crJointTranslationLimit[m_NumTranslationLimits];
			crJointTranslationLimit* curr = m_TranslationLimits;
			for (int i=0; i<nb; ++i)
			{
				crJointTranslationLimit& limit = translationLimits[i];
				if (limit.GetBoneID() != -1)
				{
					Assert(curr < m_TranslationLimits+m_NumTranslationLimits);

					curr->m_BoneID = limit.m_BoneID;
					curr->m_LimitMin = limit.m_LimitMin;
					curr->m_LimitMax = limit.m_LimitMax;

					++curr;
				}
			}
		}

		if (m_NumScaleLimits)
		{
			m_ScaleLimits = rage_new crJointScaleLimit[m_NumScaleLimits];
			crJointScaleLimit* curr = m_ScaleLimits;
			for (int i=0; i<nb; ++i)
			{
				crJointScaleLimit& limit = scaleLimits[i];
				if (limit.GetBoneID() != -1)
				{
					Assert(curr < m_ScaleLimits+m_NumScaleLimits);

					curr->m_BoneID = limit.m_BoneID;
					curr->m_LimitMin = limit.m_LimitMin;
					curr->m_LimitMax = limit.m_LimitMax;
				}
			}
		}

		sysMemStartTemp();
		delete[] rotationLimits;
		delete[] translationLimits;
		delete[] scaleLimits;
		sysMemEndTemp();
	}

	f->Close();

	return success;
}

////////////////////////////////////////////////////////////////////////////////

bool crJointData::LoadBone_v100(fiTokenizer& tok, crJointRotationLimit* rotLimits, crJointTranslationLimit* transLimits, crJointScaleLimit* scaleLimits, int& index, eSkelDataInit init)
{
	char temp[128];
	tok.GetToken(temp,sizeof(temp));

	Vec3V offset(V_ZERO);

	tok.MatchToken("{");
	tok.MatchVector("offset",RC_VECTOR3(offset));

	Vec3V rotMin(-PI, -PI, -PI), rotMax(PI, PI, PI);

	u32 dof = 0;

	if(tok.CheckToken("rotmin"))
	{
		dof = crBoneData::HAS_ROTATE_LIMITS;
		tok.GetVector(RC_VECTOR3(rotMin));
	}
	if(tok.CheckToken("rotmax"))
	{
		dof = crBoneData::HAS_ROTATE_LIMITS;
		tok.GetVector(RC_VECTOR3(rotMax));
	}

	if (dof & crBoneData::HAS_ROTATE_LIMITS)
	{
		crJointRotationLimit& limit = rotLimits[index];
		limit.SetBoneID(index);
		switch (init)
		{
		case kInitFromEulers:
			limit.InitFromEulers(offset, rotMin, rotMax);
			break;
		case kInitAsEulers:
			limit.InitAsEulers(rotMin, rotMax);
			break;
		}

		++m_NumRotationLimits;
	}

	while(1)
	{
		tok.GetToken(temp,sizeof(temp));
		if(strcmp(temp,"bone")==0)
		{
			if (!LoadBone_v100(tok,rotLimits,transLimits,scaleLimits,++index,init))
				return false;
		}
		else if(strcmp(temp,"}")==0)
		{
			break;
		}
		else
		{
			Errorf("crJointData:LoadBone_v100() - error in file");
			return false;
		}
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////

static bool DoReadDOF(fiTokenizer &tok,u32 &status,u32 dofMask,u32 limitMask,float &lockValue,float &limitMin,float &limitMax)
{
	bool retVal=true;
	status |= dofMask;
	if (tok.CheckIToken("lock"))
	{
		retVal=false;
		status &= ~dofMask;
		lockValue = tok.GetFloat();
	}
	if (tok.CheckIToken("limit"))
	{
		status |= limitMask;
		limitMin = tok.GetFloat();
		limitMax = tok.GetFloat();
	}
	return retVal;
}

////////////////////////////////////////////////////////////////////////////////

bool crJointData::LoadBone_v101(fiTokenizer& tok, crJointRotationLimit* rotLimits, crJointTranslationLimit* transLimits, crJointScaleLimit* scaleLimits, int& index, eSkelDataInit init)
{
	char buffer[128];
	float dummy;

	tok.GetToken(buffer,sizeof(buffer));
	tok.MatchToken("{");

	// offset
	Vec3V offset = Vec3V(V_ZERO);
	if (tok.CheckIToken("offset"))
	{
		tok.GetVector(RC_VECTOR3(offset));
	}

	Vec3V minRot(-PI, -PI, -PI), maxRot(PI, PI, PI);
	Vec3V minTrans(V_ZERO), maxTrans(V_ZERO);
	Vec3V minScale(V_ZERO), maxScale(V_ZERO);

	u32 dof = 0;

	u32 idx = index;

	while (1)
	{
		tok.GetToken(buffer,sizeof(buffer));

		if (strncmp(buffer,"bone",4)==0)
		{
			if (!LoadBone_v101(tok,rotLimits,transLimits,scaleLimits,++index,init))
				return false;
		}
		else if (stricmp(buffer,"}")==0)
		{
			break;
		}
		// rotation x
		else if (stricmp(buffer,"rotX")==0)
		{
			DoReadDOF(tok,dof,crBoneData::ROTATE_X,crBoneData::HAS_ROTATE_LIMITS,dummy,minRot[0],maxRot[0]);
		}
		// rotation y
		else if (stricmp(buffer,"rotY")==0)
		{
			DoReadDOF(tok,dof,crBoneData::ROTATE_Y,crBoneData::HAS_ROTATE_LIMITS,dummy,minRot[1],maxRot[1]);
		}
		// rotation z
		else if (stricmp(buffer,"rotZ")==0)
		{
			DoReadDOF(tok,dof,crBoneData::ROTATE_Z,crBoneData::HAS_ROTATE_LIMITS,dummy,minRot[2],maxRot[2]);
		}
		// translation x
		else if (stricmp(buffer,"transX")==0)
		{
			DoReadDOF(tok,dof,crBoneData::TRANSLATE_X,crBoneData::HAS_TRANSLATE_LIMITS,offset[0],minTrans[0],maxTrans[0]);
		}
		// translation y
		else if (stricmp(buffer,"transY")==0)
		{
			DoReadDOF(tok,dof,crBoneData::TRANSLATE_Y,crBoneData::HAS_TRANSLATE_LIMITS,offset[1],minTrans[1],maxTrans[1]);
		}
		// translation z
		else if (stricmp(buffer,"transZ")==0)
		{
			DoReadDOF(tok,dof,crBoneData::TRANSLATE_Z,crBoneData::HAS_TRANSLATE_LIMITS,offset[2],minTrans[2],maxTrans[2]);
		}
		// scale x
		else if (stricmp(buffer,"scaleX")==0)
		{
			DoReadDOF(tok,dof,crBoneData::SCALE_X,crBoneData::HAS_SCALE_LIMITS,dummy,minScale[0],maxScale[0]);
		}
		// scale y
		else if (stricmp(buffer,"scaleY")==0)
		{
			DoReadDOF(tok,dof,crBoneData::SCALE_Y,crBoneData::HAS_SCALE_LIMITS,dummy,minScale[1],maxScale[1]);
		}
		// scale z
		else if (stricmp(buffer,"scaleZ")==0)
		{
			DoReadDOF(tok,dof,crBoneData::SCALE_Z,crBoneData::HAS_SCALE_LIMITS,dummy,minScale[2],maxScale[2]);
		}
		// visibility
		// This will turn off visibility. Default is visible.
		else if (stricmp(buffer,"visibility")==0)
		{
			tok.GetInt();
		}
	}

	if (dof & crBoneData::HAS_ROTATE_LIMITS)
	{
		crJointRotationLimit& limit = rotLimits[idx];
		limit.SetBoneID(idx);
		switch (init)
		{
		case kInitFromEulers:
			limit.InitFromEulers(offset, minRot, maxRot);
			break;
		case kInitAsEulers:
			limit.InitAsEulers(minRot, maxRot);
			break;
		}

		++m_NumRotationLimits;
	}

	if (dof & crBoneData::HAS_TRANSLATE_LIMITS)
	{
		crJointTranslationLimit& limit = transLimits[idx];
		limit.SetBoneID(idx);
		limit.Init(minTrans, maxTrans);

		++m_NumTranslationLimits;
	}

	if (dof & crBoneData::HAS_SCALE_LIMITS)
	{
		crJointScaleLimit& limit = scaleLimits[idx];
		limit.SetBoneID(idx);
		limit.Init(minScale, maxScale);

		++m_NumScaleLimits;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////

bool crJointData::LoadBone_v104Plus(fiTokenizer& tok, crJointRotationLimit* rotLimits, crJointTranslationLimit* transLimits, crJointScaleLimit* scaleLimits, int& index, eSkelDataInit init)
{
	char buffer[128];
	float dummy;

	// index and name
	tok.GetToken(buffer,sizeof(buffer));
	tok.MatchToken("{");

	u32 idx = index;
	// bone id
	u16 boneId = u16(idx);
	if(tok.CheckIToken("boneid"))
	{
		boneId = u16(tok.GetInt());
	}

	if(tok.CheckIToken("mirror"))
	{
		tok.GetInt();
	}

	// offset
	Vec3V offset = Vec3V(V_ZERO);
	if (tok.CheckIToken("offset"))
	{
		tok.GetVector(RC_VECTOR3(offset));
	}

	// euler
	if (tok.CheckIToken("euler"))
	{
		Vec3V dummy;
		tok.GetVector(RC_VECTOR3(dummy));
	}

	// joint orient
	if(tok.CheckIToken("orient"))
	{
		Vec3V dummy;
		tok.GetVector(RC_VECTOR3(dummy));
	}

	// scale orient
	if(tok.CheckIToken("sorient"))
	{
		Vec3V dummy;
		tok.GetVector(RC_VECTOR3(dummy));
	}

	// scale
	if (tok.CheckIToken("scale"))
	{
		Vec3V dummy;
		tok.GetVector(RC_VECTOR3(dummy));
	}

	Vec3V minRot(-PI, -PI, -PI), maxRot(PI, PI, PI);
	Vec3V minTrans(V_ZERO), maxTrans(V_ZERO);
	Vec3V minScale(V_ZERO), maxScale(V_ZERO);

	u32 dof = 0;
	while (1)
	{
		tok.GetToken(buffer,sizeof(buffer));

		if (strncmp(buffer,"bone",4)==0)
		{
			if (!LoadBone_v104Plus(tok,rotLimits,transLimits,scaleLimits,++index,init))
				return false;
		}
		else if (stricmp(buffer,"}")==0)
		{
			break;
		}
		// rotation x
		else if (stricmp(buffer,"rotX")==0)
		{
			DoReadDOF(tok,dof,crBoneData::ROTATE_X,crBoneData::HAS_ROTATE_LIMITS,dummy,minRot[0],maxRot[0]);
		}
		// rotation y
		else if (stricmp(buffer,"rotY")==0)
		{
			DoReadDOF(tok,dof,crBoneData::ROTATE_Y,crBoneData::HAS_ROTATE_LIMITS,dummy,minRot[1],maxRot[1]);
		}
		// rotation z
		else if (stricmp(buffer,"rotZ")==0)
		{
			DoReadDOF(tok,dof,crBoneData::ROTATE_Z,crBoneData::HAS_ROTATE_LIMITS,dummy,minRot[2],maxRot[2]);
		}
		// translation x
		else if (stricmp(buffer,"transX")==0)
		{
			DoReadDOF(tok,dof,crBoneData::TRANSLATE_X,crBoneData::HAS_TRANSLATE_LIMITS,dummy,minTrans[0],maxTrans[0]);
		}
		// translation y
		else if (stricmp(buffer,"transY")==0)
		{
			DoReadDOF(tok,dof,crBoneData::TRANSLATE_Y,crBoneData::HAS_TRANSLATE_LIMITS,dummy,minTrans[1],maxTrans[1]);
		}
		// translation z
		else if (stricmp(buffer,"transZ")==0)
		{
			DoReadDOF(tok,dof,crBoneData::TRANSLATE_Z,crBoneData::HAS_TRANSLATE_LIMITS,dummy,minTrans[2],maxTrans[2]);
		}
		// scale x
		else if (stricmp(buffer,"scaleX")==0)
		{
			DoReadDOF(tok,dof,crBoneData::SCALE_X,crBoneData::HAS_SCALE_LIMITS,dummy,minScale[0],maxScale[0]);
		}
		// scale y
		else if (stricmp(buffer,"scaleY")==0)
		{
			DoReadDOF(tok,dof,crBoneData::SCALE_Y,crBoneData::HAS_SCALE_LIMITS,dummy,minScale[1],maxScale[2]);
		}
		// scale z
		else if (stricmp(buffer,"scaleZ")==0)
		{
			DoReadDOF(tok,dof,crBoneData::SCALE_Z,crBoneData::HAS_SCALE_LIMITS,dummy,minScale[2],maxScale[2]);
		}
		else if (stricmp(buffer,"visibility")==0)
		{
			tok.GetInt();
		}
	}

	if (dof & crBoneData::HAS_ROTATE_LIMITS)
	{
		crJointRotationLimit& limit = rotLimits[idx];
		limit.SetBoneID(boneId);
		switch (init)
		{
		case kInitFromEulers:
			limit.InitFromEulers(offset, minRot, maxRot);
			break;
		case kInitAsEulers:
			limit.InitAsEulers(minRot, maxRot);
			break;
		}

		++m_NumRotationLimits;
	}

	if (dof & crBoneData::HAS_TRANSLATE_LIMITS)
	{
		crJointTranslationLimit& limit = transLimits[idx];
		limit.SetBoneID(boneId);
		limit.Init(minTrans, maxTrans);

		++m_NumTranslationLimits;
	}

	if (dof & crBoneData::HAS_SCALE_LIMITS)
	{
		crJointScaleLimit& limit = scaleLimits[idx];
		limit.SetBoneID(boneId);
		limit.Init(minScale, maxScale);

		++m_NumScaleLimits;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////

u32 crJointData::LoadDof_v100(fiTokenizer& tok, u32* dofs, int& index)
{
	char temp[128];
	tok.GetToken(temp,sizeof(temp));

	Vec3V dummy;
	u32 dof = 0;

	tok.MatchToken("{");
	tok.MatchVector("offset",RC_VECTOR3(dummy));

	Vec3V rotMin(-PI, -PI, -PI), rotMax(PI, PI, PI);

	if(tok.CheckToken("rotmin"))
	{
		dof = crBoneData::HAS_ROTATE_LIMITS;
		tok.GetVector(RC_VECTOR3(rotMin));
	}
	if(tok.CheckToken("rotmax"))
	{
		dof = crBoneData::HAS_ROTATE_LIMITS;
		tok.GetVector(RC_VECTOR3(rotMax));
	}

	while(1)
	{
		tok.GetToken(temp,sizeof(temp));
		if(strcmp(temp,"bone")==0)
		{
			int idx = index;
			dofs[idx] = LoadDof_v100(tok, dofs, ++index);
		}
		else if(strcmp(temp,"}")==0)
		{
			break;
		}
		else
		{
			Errorf("crJointData:LoadBone_v100() - error in file");
			return false;
		}
	}

	return dof;
}

////////////////////////////////////////////////////////////////////////////////

u32 crJointData::LoadDof_v101(fiTokenizer& tok, u32* dofs, int& index)
{
	char buffer[128];
	float dummy;

	tok.GetToken(buffer,sizeof(buffer));
	tok.MatchToken("{");

	// offset
	if (tok.CheckIToken("offset"))
	{
		Vec3V dummy;
		tok.GetVector(RC_VECTOR3(dummy));
	}

	u32 dof = 0;

	while (1)
	{
		tok.GetToken(buffer,sizeof(buffer));

		if (strncmp(buffer,"bone",4)==0)
		{
			int idx = index;
			dofs[idx] = LoadDof_v101(tok, dofs, ++index);
		}
		else if (stricmp(buffer,"}")==0)
		{
			break;
		}
		// rotation x
		else if (stricmp(buffer,"rotX")==0)
		{
			DoReadDOF(tok,dof,crBoneData::ROTATE_X,crBoneData::HAS_ROTATE_LIMITS,dummy,dummy,dummy);
		}
		// rotation y
		else if (stricmp(buffer,"rotY")==0)
		{
			DoReadDOF(tok,dof,crBoneData::ROTATE_Y,crBoneData::HAS_ROTATE_LIMITS,dummy,dummy,dummy);
		}
		// rotation z
		else if (stricmp(buffer,"rotZ")==0)
		{
			DoReadDOF(tok,dof,crBoneData::ROTATE_Z,crBoneData::HAS_ROTATE_LIMITS,dummy,dummy,dummy);
		}
		// translation x
		else if (stricmp(buffer,"transX")==0)
		{
			DoReadDOF(tok,dof,crBoneData::TRANSLATE_X,crBoneData::HAS_TRANSLATE_LIMITS,dummy,dummy,dummy);
		}
		// translation y
		else if (stricmp(buffer,"transY")==0)
		{
			DoReadDOF(tok,dof,crBoneData::TRANSLATE_Y,crBoneData::HAS_TRANSLATE_LIMITS,dummy,dummy,dummy);
		}
		// translation z
		else if (stricmp(buffer,"transZ")==0)
		{
			DoReadDOF(tok,dof,crBoneData::TRANSLATE_Z,crBoneData::HAS_TRANSLATE_LIMITS,dummy,dummy,dummy);
		}
		// scale x
		else if (stricmp(buffer,"scaleX")==0)
		{
			DoReadDOF(tok,dof,crBoneData::SCALE_X,crBoneData::HAS_SCALE_LIMITS,dummy,dummy,dummy);
		}
		// scale y
		else if (stricmp(buffer,"scaleY")==0)
		{
			DoReadDOF(tok,dof,crBoneData::SCALE_Y,crBoneData::HAS_SCALE_LIMITS,dummy,dummy,dummy);
		}
		// scale z
		else if (stricmp(buffer,"scaleZ")==0)
		{
			DoReadDOF(tok,dof,crBoneData::SCALE_Z,crBoneData::HAS_SCALE_LIMITS,dummy,dummy,dummy);
		}
		// visibility
		// This will turn off visibility. Default is visible.
		else if (stricmp(buffer,"visibility")==0)
		{
			tok.GetInt();
		}
	}

	return dof;
}

////////////////////////////////////////////////////////////////////////////////

u32 crJointData::LoadDof_v104Plus(fiTokenizer& tok, u32* dofs, int& index)
{
	char buffer[128];
	float dummy;

	// index and name
	tok.GetToken(buffer,sizeof(buffer));
	tok.MatchToken("{");

	// bone id
	if(tok.CheckIToken("boneid"))
	{
		tok.GetInt();
	}

	if(tok.CheckIToken("mirror"))
	{
		tok.GetInt();
	}

	// offset
	if (tok.CheckIToken("offset"))
	{
		Vec3V dummy;
		tok.GetVector(RC_VECTOR3(dummy));
	}

	// euler
	if (tok.CheckIToken("euler"))
	{
		Vec3V dummy;
		tok.GetVector(RC_VECTOR3(dummy));
	}

	// joint orient
	if(tok.CheckIToken("orient"))
	{
		Vec3V dummy;
		tok.GetVector(RC_VECTOR3(dummy));
	}

	// scale orient
	if(tok.CheckIToken("sorient"))
	{
		Vec3V dummy;
		tok.GetVector(RC_VECTOR3(dummy));
	}

	// scale
	if (tok.CheckIToken("scale"))
	{
		Vec3V dummy;
		tok.GetVector(RC_VECTOR3(dummy));
	}

	u32 dof = 0;

	while (1)
	{
		tok.GetToken(buffer,sizeof(buffer));

		if (strncmp(buffer,"bone",4)==0)
		{
			int idx = index;
			dofs[idx] = LoadDof_v104Plus(tok, dofs, ++index);
		}
		else if (stricmp(buffer,"}")==0)
		{
			break;
		}
		// rotation x
		else if (stricmp(buffer,"rotX")==0)
		{
			DoReadDOF(tok,dof,crBoneData::ROTATE_X,crBoneData::HAS_ROTATE_LIMITS,dummy,dummy,dummy);
		}
		// rotation y
		else if (stricmp(buffer,"rotY")==0)
		{
			DoReadDOF(tok,dof,crBoneData::ROTATE_Y,crBoneData::HAS_ROTATE_LIMITS,dummy,dummy,dummy);
		}
		// rotation z
		else if (stricmp(buffer,"rotZ")==0)
		{
			DoReadDOF(tok,dof,crBoneData::ROTATE_Z,crBoneData::HAS_ROTATE_LIMITS,dummy,dummy,dummy);
		}
		// translation x
		else if (stricmp(buffer,"transX")==0)
		{
			DoReadDOF(tok,dof,crBoneData::TRANSLATE_X,crBoneData::HAS_TRANSLATE_LIMITS,dummy,dummy,dummy);
		}
		// translation y
		else if (stricmp(buffer,"transY")==0)
		{
			DoReadDOF(tok,dof,crBoneData::TRANSLATE_Y,crBoneData::HAS_TRANSLATE_LIMITS,dummy,dummy,dummy);
		}
		// translation z
		else if (stricmp(buffer,"transZ")==0)
		{
			DoReadDOF(tok,dof,crBoneData::TRANSLATE_Z,crBoneData::HAS_TRANSLATE_LIMITS,dummy,dummy,dummy);
		}
		// scale x
		else if (stricmp(buffer,"scaleX")==0)
		{
			DoReadDOF(tok,dof,crBoneData::SCALE_X,crBoneData::HAS_SCALE_LIMITS,dummy,dummy,dummy);
		}
		// scale y
		else if (stricmp(buffer,"scaleY")==0)
		{
			DoReadDOF(tok,dof,crBoneData::SCALE_Y,crBoneData::HAS_SCALE_LIMITS,dummy,dummy,dummy);
		}
		// scale z
		else if (stricmp(buffer,"scaleZ")==0)
		{
			DoReadDOF(tok,dof,crBoneData::SCALE_Z,crBoneData::HAS_SCALE_LIMITS,dummy,dummy,dummy);
		}
		else if (stricmp(buffer,"visibility")==0)
		{
			tok.GetInt();
		}
	}

	return dof;
}

#endif // CR_DEV && !_SPU

////////////////////////////////////////////////////////////////////////////////

const crJointRotationLimit* crJointData::FindJointRotationLimit(int boneId) const
{
	if (const crJointRotationLimit* limit = m_RotationLimits) {
		for (int i=0, numLimits=m_NumRotationLimits; i<numLimits; ++i, ++limit)
		{
			if(limit->GetBoneID() == boneId)
			{
				return limit;
			}
		}
	}

	return NULL;
}

////////////////////////////////////////////////////////////////////////////////

crJointRotationLimit* crJointData::FindJointRotationLimit(int boneId)
{
	if (crJointRotationLimit* limit = m_RotationLimits) {
		for (int i=0, numLimits=m_NumRotationLimits; i<numLimits; ++i, ++limit)
		{
			if (limit->GetBoneID() == boneId)
			{
				return limit;
			}
		}
	}

	return NULL;
}

////////////////////////////////////////////////////////////////////////////////

const crJointTranslationLimit* crJointData::FindJointTranslationLimit(int boneId) const
{
	if (const crJointTranslationLimit* limit = m_TranslationLimits) {
		for (int i=0, numLimits=m_NumTranslationLimits; i<numLimits; ++i, ++limit)
		{
			if (limit->GetBoneID() == boneId)
			{
				return limit;
			}
		}
	}

	return NULL;
}

////////////////////////////////////////////////////////////////////////////////

crJointTranslationLimit* crJointData::FindJointTranslationLimit(int boneId)
{
	if (crJointTranslationLimit* limit = m_TranslationLimits) {
		for (int i=0, numLimits=m_NumTranslationLimits; i<numLimits; ++i, ++limit)
		{
			if (limit->GetBoneID() == boneId)
			{
				return limit;
			}
		}
	}

	return NULL;
}

////////////////////////////////////////////////////////////////////////////////

const crJointScaleLimit* crJointData::FindJointScaleLimit(int boneId) const
{
	if (const crJointScaleLimit* limit = m_ScaleLimits) {
		for (int i=0, numLimits=m_NumScaleLimits; i<numLimits; ++i, ++limit)
		{
			if (limit->GetBoneID() == boneId)
			{
				return limit;
			}
		}
	}

	return NULL;
}

////////////////////////////////////////////////////////////////////////////////

crJointScaleLimit* crJointData::FindJointScaleLimit(int boneId)
{
	if (crJointScaleLimit* limit = m_ScaleLimits) {
		for (int i=0, numLimits=m_NumScaleLimits; i<numLimits; ++i, ++limit)
		{
			if (limit->GetBoneID() == boneId)
			{
				return limit;
			}
		}
	}

	return NULL;
}

////////////////////////////////////////////////////////////////////////////////

bool crJointData::ConvertToEulers(const crBoneData& boneData, Vec3V_InOut minRot, Vec3V_InOut maxRot) const
{
	if (const crJointRotationLimit* jointLimit = FindJointRotationLimit(boneData.GetBoneId()))
	{
		return jointLimit->ConvertToEulers(minRot, maxRot);
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////

bool crJointData::GetTranslationLimits(const crBoneData& boneData, Vec3V_InOut minTrans, Vec3V_InOut maxTrans) const
{
	if (const crJointTranslationLimit* jointLimit = FindJointTranslationLimit(boneData.GetBoneId()))
	{
		minTrans = jointLimit->m_LimitMin;
		maxTrans = jointLimit->m_LimitMax;

		return true;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////

bool crJointData::GetScaleLimits(const crBoneData& boneData, Vec3V_InOut minScale, Vec3V_InOut maxScale) const
{
	if (const crJointScaleLimit* jointLimit = FindJointScaleLimit(boneData.GetBoneId()))
	{
		minScale = jointLimit->m_LimitMin;
		maxScale = jointLimit->m_LimitMax;

		return true;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////

static bool HasRotationLimits (float limitMin, float limitMax)
{
	return (limitMin!=limitMax) && (limitMin>-1.99f*PI || limitMax<1.99f*PI);
}

////////////////////////////////////////////////////////////////////////////////

static bool HasTranslationLimits (float limitMin, float limitMax)
{
	return (limitMin!=limitMax);
}

////////////////////////////////////////////////////////////////////////////////

bool crJointData::HasLimits(const crBoneData& bone, bool& outXLimits, bool& outYLimits, bool& outZLimits, bool& outXTLimits, bool& outYTLimits, bool& outZTLimits) const
{
	u32 dofs = bone.GetDofs();

	const crJointRotationLimit* rotLimit = FindJointRotationLimit(bone.GetBoneId());
	if(rotLimit && (dofs & crBoneData::HAS_ROTATE_LIMITS))
	{
		outXLimits = (dofs & crBoneData::ROTATE_X)!=0;
		outYLimits = (dofs & crBoneData::ROTATE_Y)!=0;
		outZLimits = (dofs & crBoneData::ROTATE_Z)!=0;

		Vec3V min, max;
		if(rotLimit->ConvertToEulers(min, max))
		{
			outXLimits &= HasRotationLimits(min.GetXf(),max.GetXf());
			outYLimits &= HasRotationLimits(min.GetYf(),max.GetYf());
			outZLimits &= HasRotationLimits(min.GetZf(),max.GetZf());
		}
	}
	else
	{
		outXLimits = outYLimits = outZLimits = false;
	}

	const crJointTranslationLimit* transLimit = FindJointTranslationLimit(bone.GetBoneId());
	if(transLimit && (dofs & crBoneData::HAS_TRANSLATE_LIMITS))
	{
		outXTLimits = (dofs & crBoneData::TRANSLATE_X)!=0;
		outYTLimits = (dofs & crBoneData::TRANSLATE_Y)!=0;
		outZTLimits = (dofs & crBoneData::TRANSLATE_Z)!=0;

		outXTLimits &= HasTranslationLimits(transLimit->m_LimitMin.GetXf(),transLimit->m_LimitMax.GetXf());
		outYTLimits &= HasTranslationLimits(transLimit->m_LimitMin.GetYf(),transLimit->m_LimitMax.GetYf());
		outZTLimits &= HasTranslationLimits(transLimit->m_LimitMin.GetZf(),transLimit->m_LimitMax.GetZf());
	}
	else
	{
		outXTLimits = outYTLimits = outZTLimits = false;
	}

	return outXLimits || outYLimits || outZLimits || outXTLimits || outYTLimits || outZTLimits;
}

////////////////////////////////////////////////////////////////////////////////

} // namespace rage
