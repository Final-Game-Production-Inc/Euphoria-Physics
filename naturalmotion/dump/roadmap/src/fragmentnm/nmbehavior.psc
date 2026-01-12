<?xml version="1.0"?> 
<ParserSchema xmlns="http://www.rockstargames.com/RageParserSchema"
>


<structdef autoregister="true" constructable="false" type="rage::NMValue">
	<string name="m_name" size="64" type="member"/>
</structdef>

<structdef autoregister="true" base="rage::NMValue" type="rage::NMValueInt">
	<int name="m_value"/>
</structdef>

<structdef autoregister="true" base="rage::NMValue" type="rage::NMValueBool">
	<bool name="m_value"/>
</structdef>

<structdef autoregister="true" base="rage::NMValue" type="rage::NMValueFloat">
	<float name="m_value"/>
</structdef>

<structdef autoregister="true" base="rage::NMValue" type="rage::NMValueVector3">
	<Vector3 name="m_value"/>
</structdef>

<structdef autoregister="true" base="rage::NMValue" type="rage::NMValueString">
	<string name="m_value" size="64" type="member"/>
</structdef>

<structdef autoregister="true" type="rage::NMParam">
	<string name="m_name" size="64" type="member"/>
	<string name="m_type" size="8" type="member"/>
	<string name="m_init" size="64" type="member"/>
	<string name="m_description" size="256" type="member"/>
	<float init="-1000000" name="m_min"/>
	<float init="1000000" name="m_max"/>
	<float init="0.1f" name="m_step"/>
</structdef>

<structdef autoregister="true" type="rage::NMBehavior">
	<string name="m_name" size="64" type="member"/>
	<string name="m_description" size="256" type="member"/>
	<array name="m_params" type="atArray">
		<pointer policy="owner" type="rage::NMParam"/>
	</array>
</structdef>

<structdef autoregister="true" type="rage::NMBehaviorPool">
	<array name="m_behaviors" type="atArray">
		<pointer policy="owner" type="rage::NMBehavior"/>
	</array>
</structdef>

<structdef autoregister="true" onPostLoad="VerifyParamValues" type="rage::NMBehaviorInst">
	<string name="m_name" size="64" type="member"/>
	<string name="m_description" size="256" type="member"/>
	<pointer fromString="rage::NMBehaviorInst::FindBehavior" name="m_pBehavior" policy="external_named" toString="rage::NMBehaviorInst::GetBehaviorName" type="rage::NMBehavior"/>
	<array name="m_values" type="atArray">
		<pointer policy="owner" type="rage::NMValue"/>
	</array>
</structdef>

</ParserSchema>