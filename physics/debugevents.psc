<?xml version="1.0"?>
<ParserSchema xmlns="http://www.rockstargames.com/RageParserSchema">

<structdef type="rage::debugPlayback::phInstId" autoregister="true">
	<u32 name="m_AsU32"/>
</structdef>

<structdef type="PhysicsEvent" base="EventBase" constructable="false" autoregister="true">
	<Vec3V name="m_Pos"/>
	<struct name="m_rInst" type="phInstId"/>
</structdef>

<structdef type="PhysicsDebugStack" base="PhysicsEvent" autoregister="true">
</structdef>

<structdef type="InstLineEvent" base="PhysicsEvent" autoregister="true">
  <string name="m_Label" type="atHashString"/>
  <Vec3V name="m_Start"/>
  <Vec3V name="m_End"/>
  <u32 name="m_ColorStart"/>
  <u32 name="m_ColorEnd"/>
</structdef>

<structdef type="InstSphereEvent" base="PhysicsEvent" autoregister="true">
  <string name="m_Label" type="atHashString"/>
  <float name="m_Radius"/>
  <u32 name="m_Color"/>
</structdef>
  
<structdef type="PhysicsFrame" base="EventBase" autoregister="true">
	<u32 name="m_Frame"/>
</structdef>

<structdef type="NewObjectData" base="DataBase" autoregister="true">
	<struct name="m_rInst" type="phInstId"/>
	<Mat34V name="m_Matrix"/>
	<string name="m_Description" type="ConstString"/>
</structdef>

<structdef type="ApplyForceEvent" base="PhysicsEvent" autoregister="true">
	<s16 name="m_iComponent"/>
	<Vec3V name="m_ForcePos"/>
	<Vec3V name="m_Force"/>
	<float name="m_fMass"/>
</structdef>

<structdef type="ApplyImpulseEvent" base="PhysicsEvent" autoregister="true">
	<Vec3V name="m_Impulse"/>
	<Vec3V name="m_ImpulsePos"/>
</structdef>

<structdef type="ApplyImpulseCGEvent" base="ApplyImpulseEvent" autoregister="true">
</structdef>

<structdef type="ApplyForceCGEvent" base="ApplyForceEvent" autoregister="true">
</structdef>

<structdef type="ApplyTorqueEvent" base="PhysicsEvent" autoregister="true">
	<float name="m_fMass"/>
	<Vec3V name="m_Torque"/>
</structdef>

<structdef type="SetVelocityEvent" base="PhysicsEvent" autoregister="true">
	<Vec3V name="m_Velocity"/>
</structdef>

<structdef type="SetAngularVelocityEvent" base="PhysicsEvent" autoregister="true">
	<Vec3V name="m_AngVelocity"/>
</structdef>

<structdef type="SetMatrixEvent" base="PhysicsEvent" autoregister="true">
	<Mat34V name="m_Matrix"/>
</structdef>

<structdef type="SetTaggedFloatEvent" base="PhysicsEvent" autoregister="true">
	<string name="m_FloatName" type="atHashString"/>
	<float name="m_fValue"/>
</structdef>

<structdef type="SetTaggedVectorEvent" base="PhysicsEvent" autoregister="true">
	<string name="m_ValueName" type="atHashString"/>
  <int name="m_eVectorType"/>
  <Vec3V name="m_Value"/>
</structdef>

<structdef type="SetTaggedMatrixEvent" base="PhysicsEvent" autoregister="true">
	<string name="m_ValueName" type="atHashString"/>
	<Mat34V name="m_Value"/>
</structdef>

<structdef type="DataCollectionEvent::ChildData" constructable="false" autoregister="true">
	<string name="m_Identifier" type="atHashString"/>
</structdef>

<structdef type="DataCollectionEvent::ChildData_String" base="DataCollectionEvent::ChildData" autoregister="true">
	<string name="m_String" type="atHashString"/>
</structdef>

<structdef type="DataCollectionEvent::ChildData_Float" base="DataCollectionEvent::ChildData" autoregister="true">
	<float name="m_fValue"/>
</structdef>

<structdef type="DataCollectionEvent::ChildData_Int" base="DataCollectionEvent::ChildData" autoregister="true">
	<int name="m_iValue"/>
</structdef>

<structdef type="DataCollectionEvent::ChildData_Bool" base="DataCollectionEvent::ChildData" autoregister="true">
	<bool name="m_bValue"/>
</structdef>

<structdef type="DataCollectionEvent::ChildData_Vec3" base="DataCollectionEvent::ChildData" autoregister="true">
	<float name="m_fX"/>
	<float name="m_fY"/>
	<float name="m_fZ"/>
</structdef>

<structdef type="DataCollectionEvent" base="PhysicsEvent" autoregister="true">
	<array name="m_Data" type="atArray">
		<pointer type="DataCollectionEvent::ChildData" policy="owner"/>
	</array>
</structdef>

<structdef type="SetTaggedDataCollectionEvent" base="DataCollectionEvent" autoregister="true">
	<string name="m_CollectionName" type="atHashString"/>
</structdef>

<structdef type="UpdateLocation" base="PhysicsEvent" autoregister="true">
	<bool name="m_bHasLastMatrix"/>
	<Mat34V name="m_NewMatrix"/>
	<Mat34V name="m_LastMatrix"/>
</structdef>

<structdef type="InstLabelEvent" base="PhysicsEvent" autoregister="true">
	<string name="m_Label" type="atHashString"/>
	<string name="m_Description" type="ConstString"/>
</structdef>

</ParserSchema>