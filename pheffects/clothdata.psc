<?xml version="1.0"?>
<ParserSchema xmlns="http://www.rockstargames.com/RageParserSchema">

<structdef type="::rage::phVec3V">
	<array name="m_Data" type="member" size="4">
		<u32/>
	</array>
</structdef>

<structdef type="::rage::phClothData">
  <array name="m_VertexPinnedPositions" type="atArray" align="16">
		<struct type="::rage::phVec3V"/>
	</array>    
	<array name="m_VertexPositions" type="atArray" align="16">
		<struct type="::rage::phVec3V"/>
	</array>	
	<array name="m_VertexPrevPositions" type="atArray" align="16">
		<struct type="::rage::phVec3V"/>
	</array>

  <u16 name="m_NumVerts" />
  <u16 name="m_NumPinVerts" />

</structdef>

<structdef type="::rage::phClothDataDebug">
	<array name="m_VertexPositions" type="atArray">
		<Vec3V/>
	</array>
	<array name="m_VertexPrevPositions" type="atArray">
		<Vec3V/>
	</array>

  <u16 name="m_NumVerts"/>
  <u16 name="m_NumPinVerts"/>

</structdef>

</ParserSchema>