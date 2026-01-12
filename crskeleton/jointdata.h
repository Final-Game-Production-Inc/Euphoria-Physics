//
// crskeleton/jointdata.h
//
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved.
//

#ifndef CRSKELETON_JOINTDATA_H
#define CRSKELETON_JOINTDATA_H

#include "cranimation/animation_config.h"
#include "data/struct.h"
#include "paging/base.h"
#include "parser/macros.h"
#include "grprofile/drawcore.h"
#include "vectormath/quatv.h"
#include "vectormath/vec3v.h"

namespace rage
{

class bkBank;
class crBoneData;
class fiTokenizer;

////////////////////////////////////////////////////////////////////////////////

/*
 * PURPOSE: Defines a joint limit constraints for a single bone.
 *
 * NOTES
 *
 * Current animation packages (Maya and Max) specify joint limits as a pair
 * of minimum and maximum Euler angles for each axis of rotation of a joint.
 * For 1 degree of freedom (1-DOF) joints, such as the elbow or knee, this is
 * a perfectly valid representation. For 3 degree of freedom (3-DOF) joints,
 * such as the shoulder or hip, Euler angle limits do not provide a useful
 * representation of the range of motion. Using Euler angles for 3-DOF joints
 * creates an unpredictable range of motion because the range of motion along
 * one axis will change as a result of the current orientation of the other
 * two axis. Further, by clamping each angle independently, unwanted twist
 * (rotation about the length of the bone) is frequently introduced.
 *
 * The new joint limit system attempts to improve upon this situation by
 * decomposing 3-DOF joints into two components: the swing and the twist, and
 * constraining them independently. The swing is the component of the rotation
 * that affects the position of the other end of the bone (the child joint).
 * The twist is the component of the rotation that only affects the
 * orientation of the other end of the bone (rotation about the length of the
 * bone). See the rage::Quaternion::TwistSwingDecomp() function for more
 * information. Additionally, the new joint limit system attempts to provide
 * a more powerful way to describe both the twist and the swing limits to
 * more accurately represent the true range of motion.
 *
 * A 1-DOF joint is described by the axis of rotation and the minimum and
 * maximum angles of rotation about the axis. To simplify the API and tool
 * interface, the values for a 1-DOF joint use the same variables and widgets
 * as the twist for a 3-DOF joint: TwistAxis, TwistLimitMin, and
 * TwistLimitMax.
 *
 * The twist limits for 3-DOF joints are input the same way as the limits for
 * a 1-DOF joint; You specify the TwistAxis, TwistLimitMin, and
 * TwistLimitMax. The first step of defining a 3-DOF joint is determining the
 * twist axis. For most cases you'll want to define the twist axis to be a
 * unit vector in the same direction as the bone offset. This works well for
 * the spine and shoulder. The head is an example where it's not completely
 * clear what the twist axis is. Using forwards for the head seems to work
 * well as it allows you to eliminate any twist that tilts the head to one
 * side, which usually isn't a motion you want from an IK solver.
 *
 * Swing is limited in polar coordinates by specifying a maximum swing angle
 * away from the X-axis of the zero swing orientation. The zero swing
 * orientation is the parent bone's coordinate space transformed by a
 * user-specified zero rotation quaternion, aptly named ZeroRotation.
 * Visually, this describe the orientation (relative to the parent bone) and
 * opening angle of a cone as shown in the spine bone example limits above.
 *
 * Specifying a single maximum swing angle results in a cone, however most
 * joints can't swing equally far in all directions, so the user may specify
 * different maximum swing angles at control points around the edge of the
 * cone. Currently the system allows the user to specify one, two, four, or
 * eight separate swing angles. Specifying two angles results in an
 * elliptical cone and specifying four angles results in a asymmetric
 * elliptical cone. Working with 8 angles must be done carefully to avoid
 * creating a concave region which would result in limit flipping when the
 * desired swing position is equidistant from two nearby points on the
 * boundary of the swing region.
 *
 * In addition to specifying twist as described above, there is an option to
 * use separate twist limits at each control point and using the primary
 * twist limit at the zero swing orientation. The actual twist limit used is
 * based on the current swing and calculated by linear interpolation between
 * the twist limits specified on the two nearest control points along the
 * edge of the swing space and the primary twist limits at the zero swing
 * orientation. This was added to improve the ability to avoid
 * self-penetration.
 */

class crJointRotationLimit
{
public:
	crJointRotationLimit() { Reset(); }

	crJointRotationLimit(datResource&) { }

	// PURPOSE: Initializes the joint limits to complete freedom of movement.
	void Reset();

	IMPLEMENT_PLACE_INLINE(crJointRotationLimit);

#if __DECLARESTRUCT
	void DeclareStruct(class datTypeStruct &s);
#endif

	static const int RORC_VERSION = 1;

	// PURPOSE: Used to specify the number of degrees of freedom for this joint.
	// NOTES: A knee or elbow is 1 degree of freedom while a hip or shoulder is 3.
	enum JointDOFs
	{
		JOINT_1_DOF = 1,
		JOINT_3_DOF = 3
	};

	// PURPOSE: Specifies the maximum swing angle and the twist limit for one
	// control point.
	struct JointControlPoint
	{
		float m_MaxSwing;
		float m_MinTwist;
		float m_MaxTwist;

#if __DECLARESTRUCT
		void DeclareStruct(class datTypeStruct &s);
#endif

		PAR_SIMPLE_PARSABLE;
	};

	// PURPOSE: The maximum number of control points that can be used to
	// define the swing region allowed by a 3-DOF joint.
	static const int sm_MaxControlPoints = 8;

#if __PFDRAW
	// PURPOSE: Used to graphically represent the joint limits.
	void Draw(Mat34V_In parentMatrix, Mat34V_In jointMatrix, Mat34V_In jointLocalMatrix, Mat34V_In childLocalMatrix) const;
#endif

	// PURPOSE: Applies the limits to the given quaternion, optionally biasing
	// towards using the limits nearest the previous frame's position.
	void				ApplyLimits(QuatV_InOut inOut, const QuatV* prevFrame = NULL) const;
	// PURPOSE: Applies the limits to the given matrix, optionally biasing
	// towards using the limits nearest the previous frame's position.
	void				ApplyLimits(Mat34V_InOut inOut, const Mat34V* prevFrame = NULL) const;
	// PURPOSE: Applies the limits to the given matrix, optionally biasing
	// towards using the limits nearest the previous frame's position.
	void				ApplyLimits(Mat34V_InOut inOut, const QuatV* prevFrame) const;
	// PURPOSE: Applies the limits to the given quaternion, 1 rotation dof at a time
	void				ApplyEulerLimits(QuatV_InOut inOut, u32 dofs) const;
	// PURPOSE: Applies the limits to the given quaternion
	void				ApplyEulerLimits(QuatV_InOut inOut) const;

	// PURPOSE: Attempt to construct an equivalent set of limits based on the Euler angle
	// limits exported from Maya into the .skel file.
	void InitFromEulers(Vec3V_In offset, Vec3V_In minRot, Vec3V_In maxRot);

	// PURPOSE: Use this structure to store existing Euler angle limits exported in the .skel file
	void InitAsEulers(Vec3V_In minRot, Vec3V_In maxRot);

	// PURPOSE: Returns the bone ID specifying which bone these limits apply to.
	int					GetBoneID()						const		{ return m_BoneID; }

	// PURPOSE: Returns the degrees of freedom for this joint.
	JointDOFs			GetJointDOFs()					const		{ return m_JointDOFs; }

	// PURPOSE: Returns whether to enforce twist limits for 3-DOF joints.
	bool				GetUseTwistLimits()				const		{ return m_UseTwistLimits; }

	// PURPOSE: Returns the axis of rotation for a 1-DOF joint or the
	// twist axis for a 3-DOF joint.
	Vec3V_Out			GetTwistAxis()					const		{ return m_TwistAxis; }

	// PURPOSE: Returns the smallest angle of rotation for a 1-DOF joint
	// or the smallest twist angle for a 3-DOF joint.
	float				GetTwistLimitMin()			const		{ return m_TwistLimitMin; }
	// PURPOSE: Returns the largest angle of rotation for a 1-DOF joint
	// or the largest twist angle for a 3-DOF joint.
	float				GetTwistLimitMax()			const		{ return m_TwistLimitMax; }

	// PURPOSE: Returns the quaternion defining the zero orientation of the
	// cone constraining the swing of this joint, relative to the parent
	// bone orientation.
	QuatV_Out			GetZeroRotation()				const		{ return m_ZeroRotation; }

	// PURPOSE: Returns control point n.
	JointControlPoint& GetControlPoint(int n) { FastAssert(n >= 0 && n < sm_MaxControlPoints); return m_ControlPoints[n]; }

	// PURPOSE: Returns control point n.
	const JointControlPoint& GetControlPoint(int n) const { FastAssert(n >= 0 && n < sm_MaxControlPoints); return m_ControlPoints[n]; }

	// PURPOSE: Returns the scale factor for the soft limits.
	float GetSoftLimitScale() const									{ return m_SoftLimitScale; }


	// PURPOSE: Sets the bone ID specifying which bone these limits apply to.
	void SetBoneID(int boneID)										{ m_BoneID = boneID; }

	// PURPOSE: Sets the degrees of freedom for this joint.
	void SetJointDOFs(JointDOFs jointDOFs)							{ m_JointDOFs = jointDOFs; }

	// PURPOSE: Sets whether to enforce twist limits for 3-DOF joints.
	void SetUseTwistLimits(bool useTwistLimits)						{ m_UseTwistLimits = useTwistLimits; }

	// PURPOSE: Sets the axis of rotation for a 1-DOF joint or the
	// twist axis for a 3-DOF joint.
	void SetTwistAxis(Vec3V_In twistAxis)						{ m_TwistAxis = Normalize(twistAxis); }

	// PURPOSE: Sets the smallest angle of rotation for a 1-DOF joint
	// or the smallest twist angle for a 3-DOF joint.
	void SetTwistLimitMin(float twistLimitMin)				{ m_TwistLimitMin = twistLimitMin; }
	// PURPOSE: Sets the largest angle of rotation for a 1-DOF joint
	// or the largest twist angle for a 3-DOF joint.
	void SetTwistLimitMax(float twistLimitMax)				{ m_TwistLimitMax = twistLimitMax; }

	// PURPOSE: Sets the quaternion defining the zero orientation of the
	// cone constraining the swing of this joint, relative to the parent
	// bone orientation.
	void SetZeroRotation(QuatV_In zeroRotation)				{ m_ZeroRotation = zeroRotation; }

	// PURPOSE: Returns the scale factor for the soft limits.
	void SetSoftLimitScale(float scale) 							{ m_SoftLimitScale = scale; }

	// PURPOSE: If possible, acquire the euler limits from this joint limit
	// RETURNS: true - Eulers were correctly attained
	bool ConvertToEulers(Vec3V_InOut minRot, Vec3V_InOut maxRot) const;

#if __BANK
	void AddWidgets(bkBank& bank);
#endif

private:
#if __PFDRAW
	void Draw1DOF(Mat34V_In worldMtx, Vec3V_In axis, Vec3V_In offset, float min, float max, const Vec3V* current = NULL) const;
	void Draw3DOF(Mat34V_In worldMtx, const Vec3V* current = NULL) const;
	void Draw3DOFSphere(Mat34V_In worldMtx, const Vec3V* current = NULL) const;
#endif

	// PURPOSE: Calculates the maximum swing at a given theta using
	// the control points specified for this joint limit.
	float CalcPhiAtTheta(float theta) const;

	// PURPOSE: Finds the nearest point on the sphere within the region defined
	// by this joint limit.
	void FindNearestPoint(Vec3V_InOut endPoint, const Vec3V* prevFrame = NULL) const;

	// PURPOSE: Converts the point to spherical coordinates (omits the radius).
	static void PointToSpherical(Vec3V_In point, float& theta, float& phi);

	// PURPOSE: Converts the spherical coordinates to a point in cartesian coordinates.
	static void SphericalToPoint(Vec3V_InOut point, float theta, float phi, ScalarV_In r = ScalarV(V_ONE));

	// PURPOSE: Constrains the twist angle to the twist limits at the given swing endpoint.
	float LimitTwistAngle(Vec3V_In endPoint, float twistAngle) const;

	// PURPOSE: Interpolates the twist limits given the current swing endpoint.
	void GetTwistLimitsAtPoint(Vec3V_In endPoint, float& min, float& max) const;

	void*		m_VirtualPtr;

	// PURPOSE: The bone ID specifying which bone these limits apply to.
	int			m_BoneID;

	// PURPOSE: Number of control points specified to describe the allowed
	// region of swing for a 3-DOF joint.
	int			m_NumControlPoints;

	// PURPOSE: The degrees of freedom for this joint.
	JointDOFs	m_JointDOFs;

	// PURPOSE: The quaternion defining the zero orientation of the
	// cone constraining the swing of this joint, relative to the parent
	// bone orientation.
	QuatV	m_ZeroRotation;

private:
	// Used to hold Euler angles during serialization.
	mutable Vec3V m_ZeroRotationEulers;

public:
	// PURPOSE: The axis of rotation for a 1-DOF joint or the
	// twist axis for a 3-DOF joint.
	Vec3V		m_TwistAxis;

	// PURPOSE: The smallest angle of rotation for a 1-DOF joint
	// or the smallest twist angle for a 3-DOF joint.
	float		m_TwistLimitMin;
	// PURPOSE: The largest angle of rotation for a 1-DOF joint
	// or the largest twist angle for a 3-DOF joint.
	float		m_TwistLimitMax;

	// PURPOSE: The percentage (0.f - 1.f) of the hard limit range
	// at which to start a gradual resistance to further movement.
	float		m_SoftLimitScale;

	// PURPOSE: Set of maximum swing values and twist limits that describe
	// the limits of this joint.
	JointControlPoint m_ControlPoints[sm_MaxControlPoints];
//	crJointControlPoint* m_ControlPoints;

	// PURPOSE: Whether to enforce twist limits for 3-DOF joints.
	bool        m_UseTwistLimits;

	// PURPOSE: Whether to use euler angles
	bool		m_UseEulerAngles;

	// PURPOSE: Whether to use separate twist limits at each control point.
	bool		m_UsePerControlTwistLimits;

	datPadding<1, u8> m_Padding;

	// PURPOSE: Used by the bank to highlight one of the control points and
	// to display the twist limits for that joint.
	static int sm_ControlPointFocus;

	// PURPOSE: The stepsize threshold at which the binary search for a
	// valid solution terminates.
	static const float sm_BinarySearchResolution;

	// Serialization callbacks to sync zero rotation Eulers with quaternion
	void PreSave()					{ m_ZeroRotationEulers = QuatVToEulersXYZ(m_ZeroRotation); }
	void PostLoad()					{ m_ZeroRotation = QuatVFromEulersXYZ(m_ZeroRotationEulers); }

#if __BANK
	// Widget callbacks
	void SyncControlPoints();
	void ZeroWidgetsChanged()		{ m_ZeroRotation = QuatVFromEulersXYZ(m_ZeroRotationEulers); }
	void TwistAxisChanged()			{ m_TwistAxis = Normalize(m_TwistAxis); }
	void DOFsChanged()				{ if (m_JointDOFs != 1 && m_JointDOFs != 3) m_JointDOFs = JOINT_3_DOF; }
#endif

	friend class crJointData;

	PAR_SIMPLE_PARSABLE
};

class crJointTranslationLimit
{
public:
	// PURPOSE: Default constructor
	crJointTranslationLimit();

	// PURPOSE: Resource constructor
	crJointTranslationLimit(datResource&) {}

	// PURPOSE: Init
	void Init(Vec3V_In min, Vec3V_In max);

	// PURPOSE:
	void SetBoneID(int boneID) { m_BoneID = boneID; }

	// PURPOSE:
	int GetBoneID() const { return m_BoneID; }

	// PURPOSE:
	void ApplyLimits(Vec3V_InOut inOut, u32 dofs) const;

	DECLARE_PLACE(crJointTranslationLimit);

#if __DECLARESTRUCT
	void DeclareStruct(class datTypeStruct& s);
#endif

	void* m_VirtualPtr;

	int m_BoneID;

	datPadding<2, u32> m_Padding;

	Vec3V	m_LimitMin;
	Vec3V	m_LimitMax;

	PAR_SIMPLE_PARSABLE;
};

class crJointScaleLimit
{
public:
	// PURPOSE: Default constructor
	crJointScaleLimit();

	// PURPOSE: Resource constructor
	crJointScaleLimit(datResource&) {}
	
	// PURPOSE: Init
	void Init(Vec3V_In min, Vec3V_In max);

	// PURPOSE:
	void SetBoneID(int boneID) { m_BoneID = boneID; }

	// PURPOSE:
	int GetBoneID() const { return m_BoneID; }

	// PURPOSE:
	void ApplyLimits(Vec3V_InOut inOut, u32 dofs) const;

	DECLARE_PLACE(crJointScaleLimit);

#if __DECLARESTRUCT
	void DeclareStruct(class datTypeStruct& s);
#endif

	void* m_VirtualPtr;

	int m_BoneID;

	datPadding<2, u32> m_Padding;

	Vec3V m_LimitMin;
	Vec3V m_LimitMax;

	PAR_SIMPLE_PARSABLE;
};

////////////////////////////////////////////////////////////////////////////////

// PURPOSE: Wraps an atArray of crJointData structure to make it easy to serialize
// using the parser module.

class crJointData : public pgBase
{
public:
	// PURPOSE: Default constructor
	crJointData();

	// PURPOSE: Resource constructor
	crJointData(datResource& rsc);

	// PURPOSE: Destructor
	virtual ~crJointData();

	DECLARE_PLACE(crJointData);

#if __DECLARESTRUCT
	void DeclareStruct(class datTypeStruct &s);
#endif

	static const int RORC_VERSION = 1;

#if CR_DEV
	// PURPOSE: Allocate and Load the skeleton data from a file
	// PARAMS: filename - the input file from which to get the joint data
	// RETURN: pointer of the crJointData object, will return NULL if load failed
	static crJointData* AllocateAndLoad(const char* filename);

	// PURPOSE: Load the joint data from a file
	// PARAMS: filename - the input file from which to get the joint data
	// RETURN: true - load succeeded, false - load failed
	bool Load(const char* filename);

	// PURPOSE: Save joint data to a .jlimits file
	// PARAMS: filename - name of .jlimits file to save
	// RETURNS: true - if save successful, false - save failed
	bool Save(const char* filename);

	// PURPOSE: Init from skel data options
	enum eSkelDataInit
	{
		kInitFromEulers = 0,
		kInitAsEulers = 1,
	};

	// PURPOSE:
	// RETURNS: true - success, false - failure
	// PARAMS: filename - filename to load
	bool InitFromSkeletonData(const char* filename, eSkelDataInit init = kInitAsEulers);

#endif // CR_DEV

	// PURPOSE: Increment reference count
	void AddRef() const;

	// PURPOSE: Decrement reference count
	// RETURNS: Number of remaining references
	// NOTES: Will deallocate and return 0 when the internal counter reaches zero
	int Release() const;

	// PURPOSE: Returns joint rotation limit data
	// PARAMS: boneId
	// RETURNS: Reference to the joint limit data if found, null otherwise
	const crJointRotationLimit* FindJointRotationLimit(int boneId) const;
	crJointRotationLimit* FindJointRotationLimit(int boneId);

	// PURPOSE: Returns joint rotation limit data
	// PARAMS: jointIndex - will assert if not in range
	// RETURN: Reference to the joint limit data
	const crJointRotationLimit& GetJointRotationLimit(int jointIndex) const;
	crJointRotationLimit& GetJointRotationLimit(int jointIndex);

	// PURPOSE: Returns joint rotation limit data
	// PARAMS: boneId
	// RETURNS: Reference to the joint limit data if found, null otherwise
	const crJointTranslationLimit* FindJointTranslationLimit(int boneId) const;
	crJointTranslationLimit* FindJointTranslationLimit(int boneId);

	// PURPOSE: Returns joint translation limit data
	// PARAMS: jointIndex - will assert if not in range
	// RETURN: Reference to the joint limit data
	const crJointTranslationLimit& GetJointTranslationLimit(int jointIndex) const;
	crJointTranslationLimit& GetJointTranslationLimit(int jointIndex);

	// PURPOSE: Returns joint rotation limit data
	// PARAMS: boneId
	// RETURNS: Reference to the joint limit data if found, null otherwise
	const crJointScaleLimit* FindJointScaleLimit(int boneId) const;
	crJointScaleLimit* FindJointScaleLimit(int boneId);

	// PURPOSE: Returns joint scale limit data
	// PARAMS: jointIndex - will assert if not in range
	// RETURN: Reference to the joint limit data
	const crJointScaleLimit& GetJointScaleLimit(int jointIndex) const;
	crJointScaleLimit& GetJointScaleLimit(int jointIndex);

	// PURPOSE: Helper for converting code from old limits to new
	// RETURNS: true - Eulers were correctly attained
	bool ConvertToEulers(const crBoneData& boneData, Vec3V_InOut minRot, Vec3V_InOut maxRot) const;

	// PURPOSE: Helper for converting code from old limits to new
	// RETURNS: true - Translations limits were found for bone
	bool GetTranslationLimits(const crBoneData& boneData, Vec3V_InOut minTrans, Vec3V_InOut maxTrans) const;

	// PURPOSE: Helper for converting code from old limits to new
	// RETURNS: true  - Scale limits were found for bone
	bool GetScaleLimits(const crBoneData& boneata, Vec3V_InOut minScale, Vec3V_InOut maxScale) const;

	bool HasLimits(const crBoneData& bone, bool& outXLimits, bool& outYLimits, bool& outZLimits, bool& outXTLimits, bool& outYTLimits, bool& outZTLimits) const;

private:

#if CR_DEV
	bool LoadBone_v100(fiTokenizer& tok, crJointRotationLimit* rotLimits, crJointTranslationLimit* transLimits, crJointScaleLimit* scaleLimits, int& index, eSkelDataInit init);
	bool LoadBone_v101(fiTokenizer& tok, crJointRotationLimit* rotLimits, crJointTranslationLimit* transLimits, crJointScaleLimit* scaleLimits, int& index, eSkelDataInit init);
	bool LoadBone_v104Plus(fiTokenizer& tok, crJointRotationLimit* rotLimits, crJointTranslationLimit* transLimits, crJointScaleLimit* scaleLimits, int& index, eSkelDataInit init);
	u32 LoadDof_v100(fiTokenizer& tok, u32* dofs, int& index);
	u32 LoadDof_v101(fiTokenizer& tok, u32* dofs, int& index);
	u32 LoadDof_v104Plus(fiTokenizer& tok, u32* dofs, int& index);
#endif // CR_DEV

	crJointRotationLimit* m_RotationLimits;
	crJointTranslationLimit* m_TranslationLimits;
	crJointScaleLimit* m_ScaleLimits;

	ConstString m_Name;

	u16 m_NumRotationLimits;
	u16 m_NumTranslationLimits;
	u16 m_NumScaleLimits;
	mutable u16 m_RefCount;

	PAR_PARSABLE
};

////////////////////////////////////////////////////////////////////////////////

inline void crJointData::AddRef() const
{
	++m_RefCount;
}

////////////////////////////////////////////////////////////////////////////////

inline int crJointData::Release() const
{
	if (!--m_RefCount)
	{
		delete this;
		return 0;
	}

	return m_RefCount;
}

////////////////////////////////////////////////////////////////////////////////

inline const crJointRotationLimit& crJointData::GetJointRotationLimit(int jointIndex) const
{
	FastAssert(0<=jointIndex && jointIndex<m_NumRotationLimits);
	return m_RotationLimits[jointIndex];
}

////////////////////////////////////////////////////////////////////////////////

inline crJointRotationLimit& crJointData::GetJointRotationLimit(int jointIndex)
{
	FastAssert(0<=jointIndex && jointIndex<m_NumRotationLimits);
	return m_RotationLimits[jointIndex];
}

////////////////////////////////////////////////////////////////////////////////

inline const crJointTranslationLimit& crJointData::GetJointTranslationLimit(int jointIndex) const
{
	FastAssert(0<=jointIndex && jointIndex<m_NumTranslationLimits);
	return m_TranslationLimits[jointIndex];
}

////////////////////////////////////////////////////////////////////////////////

inline crJointTranslationLimit& crJointData::GetJointTranslationLimit(int jointIndex)
{
	FastAssert(0<=jointIndex && jointIndex<m_NumTranslationLimits);
	return m_TranslationLimits[jointIndex];
}

////////////////////////////////////////////////////////////////////////////////

inline const crJointScaleLimit& crJointData::GetJointScaleLimit(int jointIndex) const
{
	FastAssert(0<=jointIndex && jointIndex<m_NumScaleLimits);
	return m_ScaleLimits[jointIndex];
}

////////////////////////////////////////////////////////////////////////////////

inline crJointScaleLimit& crJointData::GetJointScaleLimit(int jointIndex)
{
	FastAssert(0<=jointIndex && jointIndex<m_NumScaleLimits);
	return m_ScaleLimits[jointIndex];
}

////////////////////////////////////////////////////////////////////////////////

} // namespace rage

#endif // ifndef CRSKELETON_JOINTDATA_H
