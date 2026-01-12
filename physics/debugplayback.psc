<?xml version="1.0"?>
<ParserSchema xmlns="http://www.rockstargames.com/RageParserSchema">

<structdef type="ItemBase" autoregister="true">
</structdef>

<structdef type="Callstack" onPreSave="CacheStrings" autoregister="true">
	<array name="m_SymbolArray" type="atArray">
		<string type="ConstString"/>
	</array>
	<array name="m_AddressArray" type="atArray">
		<size_t/>
	</array>
	<size_t name="m_iSignature"/>
  <bool name="m_bIsScript"/>
</structdef>

<structdef type="DataBase" base="ItemBase" autoregister="true" onPreLoad="OnPreLoad" onPostLoad="OnPostLoad">
</structdef>

<structdef type="EventBase" base="ItemBase" constructable="false" autoregister="true">
	<u32 name="m_iEventIndex"/>
	<size_t name="m_Callstack"/>
</structdef>

<structdef type="SimpleLabelEvent" base="EventBase" autoregister="true">
	<string name="m_Label" type="atHashString"/>
	<string name="m_Description" type="ConstString"/>
</structdef>

<structdef type="SimpleLineEvent" base="EventBase" autoregister="true">
  <string name="m_Label" type="atHashString"/>
  <Vec3V name="m_Start"/>
  <Vec3V name="m_End"/>
  <u32 name="m_ColorStart"/>
  <u32 name="m_ColorEnd"/>
</structdef>

<structdef type="SimpleMatrixEvent" base="EventBase" autoregister="true">
  <string name="m_Label" type="atHashString"/>
  <float name="m_fScale"/>
  <Mat34V name="m_Matrix"/>
</structdef>

  <structdef type="SimpleSphereEvent" base="EventBase" autoregister="true">
    <string name="m_Label" type="atHashString"/>
    <Vec3V name="m_Pos"/>
    <float name="m_Radius"/>
    <u32 name="m_Color"/>
  </structdef>
  
<structdef type="Frame::FrameData" autoregister="true">
	<array name="m_pEvents" type="atArray">
		<pointer type="EventBase" policy="owner"/>
	</array>
	<u32 name="m_FrameIndex"/>
</structdef>

<structdef type="DebugRecorder::DebugRecorderData" autoregister="true">
	<array name="m_Frames" type="atArray">
		<struct type="Frame::FrameData"/>
	</array>
	<array name="m_RecordedDataArray" type="atArray">
		<pointer type="DataBase" policy="owner"/>
	</array>
	<array name="m_RecordedDataIDArray" type="atArray">
		<size_t/>
	</array>
	<array name="m_SavedCallstacks" type="atArray">
		<pointer type="Callstack" policy="owner"/>
	</array>
	<int name="m_SelectedFrameIndex"/>
	<int name="m_SelectedEventIndex"/>
	<int name="m_AnchorFrameIndex"/>
	<int name="m_AnchorEventIndex"/>
</structdef>

<structdef type="DebugRecorder" autoregister="true">
	<pointer name="mp_SaveData" type="DebugRecorder::DebugRecorderData" policy="owner"/>
</structdef>

</ParserSchema>