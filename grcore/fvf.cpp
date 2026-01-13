//
// grcore/fvf.cpp
//
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved.
//

#include "fvf.h"

#include "data/struct.h"
#include "atl/array_struct.h"
#include "math/amath.h"
#include "system/nelem.h"
#include "system/platform.h"
#include "data/safestruct.h"

namespace rage
{

// Leave array size off so we can guarantee it is exactly the correct size
const s32 grcFvf::sm_TypeSizes[] = {
	2,			// grcdsHalf
	4,			// grcdsHalf2
	6,			// grcdsHalf3
	8,			// grcdsHalf4

	4,			// grcdsFloat
	8,			// grcdsFloat2
	12,			// grcdsFloat3
	16,			// grcdsFloat4

	4,			// grcdsUBYTE4
	4,			// grcdsColor	
	4,			// grcdsPackedNormal

#if __PS3
	0,			// grcdsEDGE0
	0,			// grcdsEDGE1
	0,			// grcdsEDGE2
#else
	2,			//grcdsShort_unorm
	4,			//grcdsShort2_unorm
	2,			//grcdsByte2_unorm
#endif

	4,			// grcdsShort2
	8,			// grcdsShort4
};

static const grcFvf::grcFvfChannels s_DynamicOrder[] =
{
	grcFvf::grcfcPosition,
	grcFvf::grcfcNormal,
	grcFvf::grcfcTangent0,
	grcFvf::grcfcTangent1,
	grcFvf::grcfcTexture0,
	grcFvf::grcfcTexture1,
	grcFvf::grcfcTexture2,
	grcFvf::grcfcTexture3,
	grcFvf::grcfcTexture4,
	grcFvf::grcfcTexture5,
	grcFvf::grcfcTexture6,
	grcFvf::grcfcTexture7,
	grcFvf::grcfcWeight,
	grcFvf::grcfcBinormal0,
	grcFvf::grcfcBinormal1,
	grcFvf::grcfcBinding, // Everything before here is 16 bytes to maintain vector alignment
	grcFvf::grcfcDiffuse,
	grcFvf::grcfcSpecular,
};
CompileTimeAssert(NELEM(s_DynamicOrder) == grcFvf::grcfcCount);

IMPLEMENT_PLACE(grcFvf);

grcFvf::grcFvf() : m_Fvf(0), m_FvfSize(0), m_Flags(0), m_DynamicOrder(0) {
	CompileTimeAssert(grcdsCount == NELEM(grcFvf::sm_TypeSizes));
	CompileTimeAssert(grcdsCount == 16);
	AssertMsg( ((grcfcTexture7 - grcfcTexture0) + 1) >= grcTexStageCount, "Need to support more texture stages");
	// Assert( "Need to support more tangent stages" && ((grcfcTangent7 - grcfcTangent0) + 1) >= grcTexStageCount);
	// Assert( "Need to support more binormal stages" && ((grcfcBinormal7 - grcfcBinormal0) + 1) >= grcTexStageCount);
	m_FvfChannelSizes = 0;
	m_ChannelCount = 0;
}

grcFvf::grcFvf(datResource &)
{
	Assert(m_FvfChannelSizes);
}

grcFvf::grcFvf(const grcFvf& cloneMe, bool makeDynamic /*= false*/)
{
	if (makeDynamic)
	{
		m_Fvf = m_FvfSize = m_Flags = 0;
		m_FvfChannelSizes = 0;
		m_ChannelCount = 0;

		SetDynamicOrder(1);
		for (int i = 0; i < grcFvf::grcfcCount; ++i)
		{
			grcFvfChannels channel = (grcFvfChannels)i;
			if (cloneMe.IsChannelActive(channel))
			{
				SetChannelDataSize(channel, GetDynamicDataSizeType(channel));
				EnableChannel(channel, true);
			}
		}
		RecomputeTotalSize();
	}
	else
	{
		m_Flags = cloneMe.m_Flags;
		m_Fvf = cloneMe.m_Fvf;
		m_FvfSize = cloneMe.m_FvfSize;
		m_DynamicOrder = cloneMe.m_DynamicOrder;
		m_FvfChannelSizes = cloneMe.m_FvfChannelSizes;
		m_ChannelCount = cloneMe.m_ChannelCount;
	}
}

grcFvf::grcDataSize grcFvf::GetDynamicDataSizeType(grcFvfChannels channel)
{
	switch (channel)
	{
	case grcfcPosition:
	case grcfcWeight:
	case grcfcNormal:
	case grcfcTexture0:
	case grcfcTexture1:
	case grcfcTexture2:
	case grcfcTexture3:
	case grcfcTexture4:
	case grcfcTexture5:
	case grcfcTexture6:
	case grcfcTexture7:
	case grcfcTangent0:
	case grcfcTangent1:
	case grcfcBinormal0:
	case grcfcBinormal1:
		return grcdsFloat4;
	case grcfcDiffuse:
	case grcfcSpecular:
		return grcdsColor;
	case grcfcBinding:
		if (g_sysPlatform == platform::PS3 || g_sysPlatform == platform::ORBIS || RSG_ORBIS)
			return grcdsUBYTE4;
		else
			return grcdsColor;
	case grcfcCount:
		Errorf("Invalid FVF channel");
	}

	return grcFvf::grcdsCount;
}

void grcFvf::SetPosChannel( bool enable, grcDataSize size, bool pretransformed ) {
	if (IsDynamicOrder())
		size = grcdsFloat4;
	else if ( size == grcdsCount ) {
		if (pretransformed)
			size = grcdsFloat4;
		else
			size = grcdsFloat3;
	}

	SetChannelDataSize( grcfcPosition, size );
	EnableChannel( grcfcPosition, enable );
	RecomputeTotalSize();
	SetPreXform(pretransformed);
} 

void grcFvf::SetBlendWeightChannel( bool enable, grcDataSize size ) {
	if (IsDynamicOrder())
		size = grcdsFloat4;
	else if ( size == grcdsCount )
		size = grcdsFloat2;
	SetChannelDataSize( grcfcWeight, size );
	EnableChannel( grcfcWeight, enable );
	RecomputeTotalSize();
} 

void grcFvf::SetBindingsChannel( bool enable, grcDataSize size ) {
	if ( size == grcdsCount ) {
		if (g_sysPlatform == platform::PS3 || g_sysPlatform == platform::ORBIS || RSG_ORBIS)
			size = grcFvf::grcdsUBYTE4;
		else
			size = grcdsColor;
	}
	SetChannelDataSize( grcfcBinding, size );
	EnableChannel( grcfcBinding, enable );
	RecomputeTotalSize();
} 

void grcFvf::SetNormalChannel( bool enable, grcDataSize size ) {
	if ( IsDynamicOrder() )
		size = grcdsFloat4;
	else if (size == grcdsCount)
		size = grcdsFloat3;
	SetChannelDataSize( grcfcNormal, size );
	EnableChannel( grcfcNormal, enable );
	RecomputeTotalSize();
} 

void grcFvf::SetDiffuseChannel( bool enable, grcDataSize size ) {
	if ( size == grcdsCount )
		size = grcdsColor;
	SetChannelDataSize( grcfcDiffuse, size );
	EnableChannel( grcfcDiffuse, enable );
	RecomputeTotalSize();
} 

void grcFvf::SetSpecularChannel( bool enable, grcDataSize size ) {
	if ( size == grcdsCount )
		size = grcdsColor;
	SetChannelDataSize( grcfcSpecular, size );
	EnableChannel( grcfcSpecular, enable );
	RecomputeTotalSize();
}

void grcFvf::SetChannel( grcFvfChannels channel, bool enable, grcDataSize size ) {
	Assert( size != grcdsCount );
	SetChannelDataSize( channel, size );
	EnableChannel( channel, enable );
	RecomputeTotalSize();
}

bool grcFvf::GetTextureChannel( int stage ) const	
{ 
	Assert(stage < grcTexStageCount); 
	return IsChannelActive(static_cast<grcFvfChannels>(grcfcTexture0 + stage)); 
}

bool grcFvf::GetTangentChannel( int stage ) const	
{ 
	Assert(stage < grcTanBiCount); 
	return IsChannelActive(static_cast<grcFvfChannels>(grcfcTangent0 + stage)); 
}

bool grcFvf::GetBinormalChannel( int stage ) const	
{ 
	Assert(stage < grcTanBiCount); 
	return IsChannelActive(static_cast<grcFvfChannels>(grcfcBinormal0 + stage)); 
}

void grcFvf::SetTextureChannel( int stage, bool enable, grcDataSize size ) {
	AssertMsg( stage < grcTexStageCount && stage >= 0, "Unsupported texture stage" );

	if ( IsDynamicOrder() )
		size = grcdsFloat4;
	else if ( size == grcdsCount )
		size = grcdsFloat2;

	grcFvfChannels channel = static_cast<grcFvfChannels>(grcfcTexture0 + stage);
	SetChannelDataSize( channel, size );
	EnableChannel( channel, enable );
	RecomputeTotalSize();
} 

void grcFvf::SetTangentChannel( int stage, bool enable, grcDataSize size ) {
	AssertMsg( stage < grcTanBiCount && stage >= 0, "Unsupported tangent stage" );
	
	if ( IsDynamicOrder() )
		size = grcdsFloat4;
	else if ( size == grcdsCount )
		size = grcdsFloat3;
	
	grcFvfChannels channel = static_cast<grcFvfChannels>(grcfcTangent0 + stage);
	SetChannelDataSize( channel, size );
	EnableChannel( channel, enable );
	RecomputeTotalSize();
} 

void grcFvf::SetBinormalChannel( int stage, bool enable, grcDataSize size ) {
	AssertMsg( stage < grcTanBiCount && stage >= 0, "Unsupported binormal stage" );

	if ( IsDynamicOrder() )
		size = grcdsFloat4;
	else if ( size == grcdsCount )
		size = grcdsFloat3;

	grcFvfChannels channel = static_cast<grcFvfChannels>(grcfcBinormal0 + stage);
	SetChannelDataSize( channel, size );
	EnableChannel( channel, enable );
	RecomputeTotalSize();
} 

s32 grcFvf::GetOffsetNoAssert( grcFvfChannels channel ) const {
	s32 offset = 0;
	if (IsDynamicOrder())
	{
		for (int i = 0; i < grcfcCount; ++i) {
			grcFvfChannels currentChannel = s_DynamicOrder[i]; // remap
			if (IsChannelActive(currentChannel))
			{
				if ( currentChannel == channel )
				{
					return offset;
				}
				offset += GetSize(currentChannel);
			}
		}
	}
	else
	{
		for (int i = 0; i < grcfcCount; ++i) {
			grcFvfChannels currentChannel = (grcFvfChannels)i;
			if ( currentChannel == channel )
			{
				return offset;
			}
			offset += GetSize(currentChannel);
		}
	}
	return offset;
}

s32 grcFvf::GetOffset( grcFvfChannels channel ) const {
	Assert(IsChannelActive(channel));
	return GetOffsetNoAssert(channel);
}

void grcFvf::DisableSetChannels( const grcFvf &otherFvf ) {
	m_Fvf &= ~otherFvf.m_Fvf;
	RecomputeTotalSize();
}

void grcFvf::SetChannelDataSize(grcFvfChannels channel, grcDataSize size) {
	u32 shift = channel * 4;
	if (shift < 64) {		// Binormals are teh sux0r.
		u64 mask = (u64)15 << shift;
		m_FvfChannelSizes = (m_FvfChannelSizes & ~mask) | (u64(size) << shift);
	}
}


void grcFvf::RecomputeTotalSize() {
	int totalSize = 0;
	for (s32 i = 0; i < grcfcCount; ++i) {
		if ( IsChannelActive(static_cast<grcFvfChannels>(i)) )
			totalSize += GetSize(static_cast<grcFvfChannels>(i));
	}
	AssertMsg(totalSize < 256, "FVF code needs to change to handle vertex sizes this large");
	if (m_DynamicOrder)
		totalSize = (totalSize + 15) & ~15;
	m_FvfSize = static_cast<u8>(totalSize);
}

#if 0
s32 grcFvf::BuildFvfString( char *out, s32 ) const {
	char *start = out;
	const u8 *in = reinterpret_cast<const u8 *>(&m_Fvf);
	for (int i = 0; i < (int) sizeof(m_Fvf); i++) {
		*out = (*in == 0) ? 0xff : *in;
		in++;
		out++;
	}
	in = reinterpret_cast<const u8 *>(&m_FvfChannelSizes);
	for (int i = 0; i < (int) 8; ++i) {
		*out = (*in == 0) ? 0xff : *in;
		in++;
		out++;
	}
	Assertf((s32) (out - start)<=size, "BuildFvfString needs a bigger string (%d given, %d used)", size, (s32) (out - start));
	return (s32) (out - start);
}
#endif

#if __DECLARESTRUCT
void grcFvf::DeclareStruct(datTypeStruct &s) {
	SSTRUCT_BEGIN(grcFvf)
	SSTRUCT_FIELD(grcFvf, m_Fvf)
	SSTRUCT_FIELD(grcFvf, m_FvfSize)
	SSTRUCT_FIELD(grcFvf, m_Flags)
	SSTRUCT_FIELD(grcFvf, m_DynamicOrder)
	SSTRUCT_FIELD(grcFvf, m_ChannelCount)
	SSTRUCT_FIELD(grcFvf, m_FvfChannelSizes)
	SSTRUCT_END(grcFvf)
}
#endif

} // namespace rage
