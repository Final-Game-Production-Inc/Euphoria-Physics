<?xml version="1.0"?>
<ParserSchema xmlns="http://www.rockstargames.com/RageParserSchema">

<structdef type="rage::NMValue" constructable="false" autoregister="true">
	<string name="m_name" type="member" size="64"/>
</structdef>

<structdef type="rage::NMValueInt" base="rage::NMValue" autoregister="true">
	<int name="m_value"/>
</structdef>

<structdef type="rage::NMValueBool" base="rage::NMValue" autoregister="true">
	<bool name="m_value"/>
</structdef>

<structdef type="rage::NMValueFloat" base="rage::NMValue" autoregister="true">
	<float name="m_value"/>
</structdef>

<structdef type="rage::NMValueVector3" base="rage::NMValue" autoregister="true">
	<Vector3 name="m_value"/>
</structdef>

<structdef type="rage::NMValueString" base="rage::NMValue" autoregister="true">
	<string name="m_value" type="member" size="64"/>
</structdef>

<structdef type="rage::NMParam" autoregister="true">
	<string name="m_name" type="member" size="64"/>
	<string name="m_type" type="member" size="8"/>
	<string name="m_init" type="member" size="64"/>
	<string name="m_description" type="atHashString"/>
	<float name="m_min" init="-1000000"/>
	<float name="m_max" init="1000000"/>
	<float name="m_step" init="0.1f"/>
</structdef>

<structdef type="rage::NMBehavior" autoregister="true">
	<string name="m_name" type="member" size="64"/>
	<string name="m_description" type="atHashString"/>
	<array name="m_params" type="atArray">
		<pointer type="rage::NMParam" policy="owner"/>
	</array>
  <bool name="m_allowDuplicates" init="false"/>
  <bool name="m_taskMessage" init="false"/>
</structdef>

<structdef type="rage::NMBehaviorPool" autoregister="true">
	<array name="m_behaviors" type="atArray">
		<pointer type="rage::NMBehavior" policy="owner"/>
	</array>
</structdef>

<structdef type="rage::NMBehaviorInst" onPostLoad="VerifyParamValues" autoregister="true">
	<string name="m_name" type="member" size="64"/>
	<string name="m_description" type="atHashString"/>
	<pointer name="m_pBehavior" type="rage::NMBehavior" policy="external_named" toString="rage::NMBehaviorInst::GetBehaviorName" fromString="rage::NMBehaviorInst::FindBehavior"/>
	<array name="m_values" type="atArray">
		<pointer type="rage::NMValue" policy="owner"/>
	</array>
</structdef>

</ParserSchema>
