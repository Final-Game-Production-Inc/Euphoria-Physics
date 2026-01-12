<?xml version="1.0"?>
<ParserSchema xmlns="http://www.rockstargames.com/RageParserSchema">

<structdef type="NMImpacts" base="PhysicsEvent" autoregister="true">
	<struct name="m_OtherId" type="phInstId"/>
	<string name="m_StringId" type="atHashString"/>
	<bool name="m_bResult"/>
</structdef>

<structdef type="RecordedContact" base="DataCollectionEvent" autoregister="true">
	<Vec3V name="m_MyContactNormal"/>
	<Vec3V name="m_MyContactPos"/>
	<Vec3V name="m_OtherContactNormal"/>
	<Vec3V name="m_OtherContactPos"/>
	<struct name="m_rOther" type="phInstId"/>
	<size_t name="m_PolyID"/>
	<u32 name="m_iElement"/>
	<u32 name="m_iOtherElement"/>
	<u8 name="m_iComponent"/>
	<u8 name="m_iOtherComponent"/>
	<bool name="m_bDisabled"/>
	<bool name="m_bIsConstraint"/>
</structdef>

<structdef type="RecordedContactWithMatrices" base="RecordedContact" autoregister="true">
	<Mat34V name="m_MyMatrix"/>
	<Mat34V name="m_OtherMatrix"/>
</structdef>

<structdef type="PolyData" base="DataBase" autoregister="true">
	<array name="m_VertArray" type="atArray">
		<Vec3V/>
	</array>
</structdef>

</ParserSchema>