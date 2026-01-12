<?xml version="1.0"?>
<ParserSchema xmlns="http://www.rockstargames.com/RageParserSchema">

<structdef type="::rage::phBoundComposite" base="::rage::phBound">
  
  <array name="m_Bounds" type="pointer" sizeVar="m_MaxNumBounds">
    <pointer type="::rage::phBound" policy="owner"/>
  </array>
  <array name="m_CurrentMatrices" type="pointer" sizeVar="m_MaxNumBounds">
    <Mat34V />
  </array>
  <array name="m_LastMatrices" type="pointer" sizeVar="m_MaxNumBounds">
    <Mat34V />
  </array>
  <array name="m_LocalBoxMinMaxs" type="pointer" sizeVar="m_MaxNumBounds">
    <Vec3V />
  </array>
  <array name="m_TypeAndIncludeFlags" type="pointer" sizeVar="m_MaxNumBounds">
    <u32 />
  </array>
  <array name="m_OwnedTypeAndIncludeFlags" type="pointer" sizeVar="m_MaxNumBounds">
    <u32 />
  </array>
  <u16 name="m_MaxNumBounds" />
	<u16 name="m_NumBounds" />
  
</structdef>
  

</ParserSchema>