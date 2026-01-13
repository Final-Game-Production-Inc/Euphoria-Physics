//
// grcore/fvf.h
//
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved.
//

#ifndef GRCORE_FVF_H
#define GRCORE_FVF_H

#include "system/bit.h"
#include "atl/array.h"
#include "atl/bitset.h"
#include "data/struct.h"

namespace rage {

const int grcTexStageCount = 8;
const int grcTanBiCount = 2;

//
// PURPOSE
//	Class wrapper around fvf formats -- used to prevent confusion with gfx fvf formats
//	We are switching to this because a standard int can not contain enough data to
//	properly represent an fvf stream (which explains why d3d has pulled away from it also)
//	This fvf class uses the concepts of channels and data size/type.  A channel represents
//	actual data sent, such as positions or normals.  A data size/type represents how that
//	data is stored in a vertex buffer/dma packet.
//
//	For platforms supporting multiple streams, one fvf will represent one stream.
// <FLAG Component>
class grcFvf {
public:
	grcFvf();
	grcFvf(const grcFvf& pSrc, bool makeDynamic = false);
	grcFvf(class datResource &);
	DECLARE_PLACE(grcFvf);

	// PURPOSE
	// Data size enums, these correspond to various d3d types
	// *** EXTREMELY IMPORTANT!  If you add more data size types here, make sure the
	//	c_BitsPerDataSize const below is set to compensate for # of bits used.  Also,
	//	make sure the sm_TypeSizes array is updated ***
	enum grcDataSize {
		grcdsHalf,
		grcdsHalf2,
		grcdsHalf3,
		grcdsHalf4,

		grcdsFloat,
		grcdsFloat2,
		grcdsFloat3,
		grcdsFloat4,

		grcdsUBYTE4,
		grcdsColor,	
		grcdsPackedNormal,

#if __PS3
		grcdsEDGE0,				// Reserved for Edge spu-only data formats
		grcdsEDGE1,				// Reserved for Edge spu-only data formats
		grcdsEDGE2,				// Reserved for Edge spu-only data formats
#else
		grcdsShort_unorm,
		grcdsShort2_unorm,
		grcdsByte2_unorm,
#endif

		grcdsShort2,
		grcdsShort4,

		grcdsCount
	};

	static u32 GetDataSizeFromType(grcDataSize size) { return sm_TypeSizes[size]; }

	enum grcStreamFrequencyMode
	{
		grcsfmDivide,
		grcsfmModulo,
		grcsfmIsInstanceStream,
		grcsfmCount
	};

	static const u32 s_DefaultDivider = 1 - __PPU;

#include "grcore/fvfchannels.h"

	// Use below to enable/disable channel status
	//	Also, use a size of grcdsCount to indicate that you just want the default.
	
	/*	PURPOSE
			Enable/disable the position channel, and set the "stride" of this channel
		PARAMS
			enable - the desired status of this channel
			size - The stride of this channel
			pretransformed - Position has been transformed on CPU, useful for blits
	*/
	void SetPosChannel( bool enable, grcDataSize size=grcdsCount, bool pretransformed=false );

	/*	PURPOSE
			Enable/disable the blendweight channel, and set the "stride" of this channel
		PARAMS
			enable - the desired status of this channel
			size - The stride of this channel
	*/
	void SetBlendWeightChannel( bool enable, grcDataSize size=grcdsCount );

	/*	PURPOSE
			Enable/disable the bindings channel, and set the "stride" of this channel
		PARAMS
			enable - the desired status of this channel
			size - The stride of this channel
	*/
	void SetBindingsChannel( bool enable, grcDataSize size=grcdsCount );

	/*	PURPOSE
			Enable/disable the normal channel, and set the "stride" of this channel
		PARAMS
			enable - the desired status of this channel
			size - The stride of this channel
	*/
	void SetNormalChannel( bool enable, grcDataSize size=grcdsCount );

	/*	PURPOSE
			Enable/disable the diffuse channel, and set the "stride" of this channel
		PARAMS
			enable - the desired status of this channel
			size - The stride of this channel
	*/
	void SetDiffuseChannel( bool enable, grcDataSize size=grcdsCount );

	/*	PURPOSE
			Enable/disable the specular channel, and set the "stride" of this channel
		PARAMS
			enable - the desired status of this channel
			size - The stride of this channel
	*/
	void SetSpecularChannel( bool enable, grcDataSize size=grcdsCount );

	/*	PURPOSE
			Enable/disable the texture uv channel, and set the "stride" of this channel
		PARAMS
			stage - The texture channel to set (generally 0 - 7)
			enable - the desired status of this channel
			size - The stride of this channel
	*/
	void SetTextureChannel( int stage, bool enable, grcDataSize size=grcdsCount );

	/*	PURPOSE
			Enable/disable the tangent space channel, and set the "stride" of this channel
		PARAMS
			stage - The tangent channel to set (generally 0 - 7)
			enable - the desired status of this channel
			size - The stride of this channel
	*/
	void SetTangentChannel( int stage, bool enable, grcDataSize size=grcdsCount );

	/*	PURPOSE
			Enable/disable the binormal vector channel, and set the "stride" of this channel
		PARAMS
			stage - The tangent channel to set (generally 0 - 7)
			enable - the desired status of this channel
			size - The stride of this channel
	*/
	void SetBinormalChannel( int stage, bool enable, grcDataSize size=grcdsCount );

	/*	PURPOSE
			Enable/disable a particular, and set the "stride" of this channel
		PARAMS
			channel - the semantics of this channel
			enable - the desired status of this channel
			size - The stride of this channel
	*/
	void SetChannel( grcFvfChannels channel, bool enable, grcDataSize size=grcdsCount );

	/*	PURPOSE
			Determine if a channel is active
		PARAMS
			channel - The channel to query status of
		RETURNS
			True if channel is enabled, false otherwise
	*/
	bool IsChannelActive(grcFvfChannels channel) const { u32 msk = (1 << channel); return (m_Fvf & msk) == msk; }
	
	// Get the status of the various channels
	// RETURNS: The active status of the position channel
	bool GetPosChannel() const					{ return IsChannelActive(grcfcPosition); }
	// RETURNS: The active status of the blendweight channel
	bool GetBlendWeightChannel() const			{ return IsChannelActive(grcfcWeight); }
	// RETURNS: The active status of the skin bindings channel
	bool GetBindingsChannel() const				{ return IsChannelActive(grcfcBinding); }
	// RETURNS: The active status of the normalchannel
	bool GetNormalChannel() const				{ return IsChannelActive(grcfcNormal); }
	// RETURNS: The active status of the diffuse channel
	bool GetDiffuseChannel() const				{ return IsChannelActive(grcfcDiffuse); }
	// RETURNS: The active status of the specular channel
	bool GetSpecularChannel() const				{ return IsChannelActive(grcfcSpecular); }
	// RETURNS: The active status of the texture UV channel for the specified texture stage
	bool GetTextureChannel( int stage ) const;
	// RETURNS: The active status of the tangent space channel for the specified texture stage
	bool GetTangentChannel( int stage ) const;
	// RETURNS: The active status of the binormal vector channel for the specified texture stage
	bool GetBinormalChannel( int stage ) const;

	// PURPOSE: Disable every channel in this fvf
	void ClearAllChannels()						{ m_Fvf = 0; m_FvfSize = 0; m_ChannelCount = 0; }

	// RETURNS: The number of bytes needed for this fvf
	inline s32 GetTotalSize() const					{ return m_FvfSize; }

	// RETURNS: The size type for the specified channel
	inline grcDataSize GetDataSizeType( grcFvfChannels channel ) const;		// implemented below
	static grcDataSize GetDynamicDataSizeType(grcFvfChannels channel);

	// RETURNS: The size of a particular channel (in bytes)
	inline s32 GetSize( grcFvfChannels channel ) const;						// implemneted below

	// RETURNS: The offset for this channel of data
	s32 GetOffset( grcFvfChannels channel ) const;

	// PURPOSE: Same as GetOffset, but won't assert when the channel is inactive
	// RETURNS: The offset for this channel of data (undefined return value when channel inactive)
	s32 GetOffsetNoAssert( grcFvfChannels channel ) const;

	// PURPSOE: Disable any channels that are currently set in the other fvf, useful for multiple streams
	void DisableSetChannels( const grcFvf &otherFvf );

	// PURPOSE: Build a string that represents this fvf (for hashing purposes)
	//	DOES **NOT** ADD ENDING NULL TO STRING!!!!
	s32 BuildFvfString( char *out, s32 maxLen ) const;

	/*	PURPOSE
			Query to see if the position channel contains pre-transformed data
		RETURNS 
			True if position channel configured for pretransform, false if not
			Results are undefined if no position channel is used
	*/
	bool IsPreTransform() const { return ((m_Flags & FLG_PREXFORM) == FLG_PREXFORM); }

	// PURPOSE: Set the dynamic order flag
	// PARAMS:
	//		dynamic - zero for standard order, non zero for dynamic order
	// NOTES: The dynamic order is as follows:
	//		Position - float4
	//		Normal - float4
	//		Tangent0 - float4
	//		Tangent1 - float4
	//		Tangent2 - float4
	//		Tangent3 - float4
	//		Tangent4 - float4
	//		Tangent5 - float4
	//		Tangent6 - float4
	//		Tangent7 - float4
	//		TexCoord0 - float4
	//		TexCoord1 - float4
	//		TexCoord2 - float4
	//		TexCoord3 - float4
	//		TexCoord4 - float4
	//		TexCoord5 - float4
	//		TexCoord6 - float4
	//		TexCoord7 - float4
	//		Weights - float4
	//		Binormal0 - float4
	//		Binormal1 - float4
	//		Binormal2 - float4
	//		Binormal3 - float4
	//		Binormal4 - float4
	//		Binormal5 - float4
	//		Binormal6 - float4
	//		Binormal7 - float4
	//		Binding - u32
	//		Diffuse - u32
	//		Specular - u32
	//		UnusedPadding - u32
	// SEE ALSO: IsDynamicOrder
	void SetDynamicOrder(u8 dynamic)				{ m_DynamicOrder = dynamic; }

	// PURPOSE: Test if this Fvf is in dynamic order or standard order
	// SEE ALSO: SetDynamicOrder
	bool IsDynamicOrder() const						{ return (m_DynamicOrder != 0); }

	u32 GetChannelCount() const						{ return m_ChannelCount; }

#if __DECLARESTRUCT
	void DeclareStruct(class datTypeStruct &s);
#endif

protected:
	void SetPreXform(bool x) { x ? m_Flags |= FLG_PREXFORM : m_Flags &= ~FLG_PREXFORM; }
	void SetChannelDataSize(grcFvfChannels channel, grcDataSize size);
	void EnableChannel(grcFvfChannels channel, bool enable)
	{
		u32 channelMask = 1 << channel;
		bool channelSet = (m_Fvf & channelMask) != 0;
		if (enable && !channelSet)
		{
			m_ChannelCount++;
			m_Fvf |= channelMask;
		}
		else if (channelSet && !enable)
		{
			m_ChannelCount--;
			m_Fvf &= ~channelMask;
		}
	}

	void RecomputeTotalSize();

	u32 m_Fvf;								// The fvf channels currently used.  Could be u16 now.
	u8 m_FvfSize;							// The total size for this entire fvf
	u8 m_Flags;								// Various flags to use (i.e. transformed positions, etc.)
	u8 m_DynamicOrder;						// This vertex format is in padded dynamic order instead of the standard order
	u8 m_ChannelCount;
	u64 m_FvfChannelSizes;

	// Flags used for any odd states we have to support
	//	FLG_PREXFORM = positions are pre-transformed (really only useful for PC builds)
	enum { FLG_PREXFORM = BIT0 };

	static const s32 sm_TypeSizes[];
};

// Make sure stuff doesn't get shovelled in here again.
CompileTimeAssert(sizeof(grcFvf) == 16);

inline grcFvf::grcDataSize grcFvf::GetDataSizeType( grcFvfChannels channel ) const {
	return (grcDataSize) ((m_FvfChannelSizes >> (channel * 4)) & 15);
}

inline s32 grcFvf::GetSize( grcFvfChannels channel ) const {
	if ( IsChannelActive(channel) == false )
	{
		return 0;
	}

	grcDataSize sizeType = GetDataSizeType(channel);
	Assert(!IsDynamicOrder() || sizeType == GetDynamicDataSizeType(channel));

	return sm_TypeSizes[sizeType];
}


}	// namespace rage

#endif		// RMCORE_FVF_H
