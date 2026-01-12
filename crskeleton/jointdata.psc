<?xml version="1.0"?>
<ParserSchema xmlns="http://www.rockstargames.com/RageParserSchema">
							
<structdef type="::rage::crJointRotationLimit::JointControlPoint" autoregister="true">
	<float name="m_MaxSwing" min="0" max="PI" step="0.01f"/>
	<float name="m_MinTwist" min="-2.f * PI" max="2.f * PI" step="0.01f"/>
	<float name="m_MaxTwist" min="-2.f * PI" max="2.f * PI" step="0.01f"/>
</structdef>

<const name="::rage::crJointRotationLimit::sm_MaxControlPoints" value="8"/>
  
<structdef type="::rage::crJointRotationLimit" onPostLoad="PostLoad" onPreSave="PreSave" autoregister="true">
	<int name="m_BoneID" hideWidgets="true"/>
	<int name="m_JointDOFs" min="1" max="3" step="2"/>
	<bool name="m_UseTwistLimits"/>
	<bool name="m_UseEulerAngles"/>
	<bool name="m_UsePerControlTwistLimits"/>
	<int name="m_NumControlPoints" min="1" max="8"/>
	<Vector3 name="m_TwistAxis" min="-1" max="1"/>
	<float name="m_TwistLimitMin" min="-2.f * PI" max="2.f * PI" step="0.01f"/>
	<float name="m_TwistLimitMax" min="-2.f * PI" max="2.f * PI" step="0.01f"/>
	<float name="m_SoftLimitScale" min="0.f" max="1.f" step="0.01f"/>
	<Vector3 name="m_ZeroRotationEulers"/>
	<array name="m_ControlPoints" type="member" size="::rage::crJointRotationLimit::sm_MaxControlPoints">
		<struct name="ControlPoint" type="::rage::crJointRotationLimit::JointControlPoint"/>
	</array>
</structdef>

<structdef type="::rage::crJointTranslationLimit" autoregister="true">
	<Vector3 name="m_LimitMin"/>
	<Vector3 name="m_LimitMax"/>
</structdef>

<structdef type="::rage::crJointScaleLimit" autoregister="true">
	<Vector3 name="m_LimitMin"/>
	<Vector3 name="m_LimitMax"/>
</structdef>

<structdef type="::rage::crJointData" autoregister="true">
	<array name="m_RotationLimits" type="pointer" sizeVar="m_NumRotationLimits">
		<struct type="::rage::crJointRotationLimit"/>
	</array>
	<array name="m_TranslationLimits" type="pointer" sizeVar="m_NumTranslationLimits">
		<struct type="::rage::crJointTranslationLimit"/>
	</array>
	<array name="m_ScaleLimits" type="pointer" sizeVar="m_NumScaleLimits">
		<struct type="::rage::crJointScaleLimit"/>
	</array>
</structdef>

</ParserSchema>