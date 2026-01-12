<?xml version="1.0"?>
<ParserSchema xmlns="http://www.rockstargames.com/RageParserSchema">

<structdef type="::rage::phEdgeData">
	<array name="m_vertIndices" type="member" size="2">
		<u16/>
	</array>
	<float name="m_EdgeLength2"/>
	<float name="m_Weight0"/>
	<float name="m_CompressionWeight"/>
</structdef>

<structdef type="::rage::phVerletCloth">
  <array name="m_CustomEdgeData" type="atArray" align="16">
    <struct type="::rage::phEdgeData"/>
  </array>
	<array name="m_EdgeData" type="atArray" align="16">
		<struct type="::rage::phEdgeData"/>
	</array>
  <int name="m_NumEdges"/>
</structdef>

</ParserSchema>