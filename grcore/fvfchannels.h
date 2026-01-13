// 
// grcore/fvfchannels.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 
#ifndef GRCORE_FVFCHANNELS_H
#define GRCORE_FVFCHANNELS_H

// PURPOSE: Listing of fvf "channels" available
// Intentionally not in rage namespace because we "splice" it into the grcFvf class.
enum grcFvfChannels {
	grcfcPosition,			//  0  0x0001
	grcfcWeight,			//  1  0x0002
	grcfcBinding,			//  2  0x0004
	grcfcNormal,			//  3  0x0008
	grcfcDiffuse,			//  4  0x0010
	grcfcSpecular,			//  5  0x0020
	grcfcTexture0,			//  6  0x0040
	grcfcTexture1,			//  7  0x0080
	grcfcTexture2,			//  8  0x0100
	grcfcTexture3,			//  9  0x0200
	grcfcTexture4,			// 10  0x0400
	grcfcTexture5,			// 11  0x0800
	grcfcTexture6,			// 12  0x1000
	grcfcTexture7,			// 13  0x2000
	grcfcTangent0,			// 14  0x4000
	grcfcTangent1,			// 15  0x8000
	grcfcBinormal0,			// 16  0x10000
	grcfcBinormal1,			// 17  0x20000
	grcfcCount
};
enum grcFvfChannelBits {
	grcfcPositionMask	= 0x0001,
	grcfcWeightMask		= 0x0002,
	grcfcBindingMask	= 0x0004,
	grcfcNormalMask		= 0x0008,
	grcfcDiffuseMask	= 0x0010,
	grcfcSpecularMask	= 0x0020,
	grcfcTexture0Mask	= 0x0040,
	grcfcTexture1Mask	= 0x0080,
	grcfcTexture2Mask	= 0x0100,
	grcfcTexture3Mask	= 0x0200,
	grcfcTexture4Mask	= 0x0400,
	grcfcTexture5Mask	= 0x0800,
	grcfcTexture6Mask	= 0x1000,
	grcfcTexture7Mask	= 0x2000,
	grcfcTangent0Mask	= 0x4000,
	grcfcTangent1Mask	= 0x8000,
	grcfcBinormal0Mask	= 0x10000,
	grcfcBinormal1Mask	= 0x20000,

	grcfcPositionHalf4Mask = 0x40000000,
	grcfcPositionFloat4Mask = 0x20000000,
};

#if __ASSERT
CompileTimeAssert(grcfcCount == 18);
#endif

#endif
