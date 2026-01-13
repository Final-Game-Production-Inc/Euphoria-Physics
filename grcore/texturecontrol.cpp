// 
// grcore/texturecontrol.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#include "grcore/image.h"
#include "grcore/texture.h"
#include "grcore/texturecontrol.h"

#if __BANK_TEXTURE_CONTROL

#include "bank/button.h"

#if	__XENON
#define DBG 0
#include "system/xtl.h"
#include <xgraphics.h>
#undef DBG
#endif // __XENON

namespace rage {

#ifndef SWITCH_CASE
#define SWITCH_CASE(f) case f: return #f
#endif

#ifndef STRING
#define STRING(x) #x
#endif

// ================================================================================================

grcTextureControl::grcTextureControl()
{
	memset(this, 0, sizeof(*this));
}

const char** grcTextureControl::GetUIStrings_eTextureGPUFormat()
{
	static const char* strings[] =
	{
		STRING(DEFAULT                  ),
#if __XENON
		STRING(1_REVERSE                ),
		STRING(1                        ),
		STRING(8                        ),
		STRING(1_5_5_5                  ),
		STRING(5_6_5                    ),
		STRING(6_5_5                    ),
		STRING(8_8_8_8                  ),
		STRING(2_10_10_10               ),
		STRING(8_A                      ),
		STRING(8_B                      ),
		STRING(8_8                      ),
		STRING(Cr_Y1_Cb_Y0_REP          ),
		STRING(Y1_Cr_Y0_Cb_REP          ),
		STRING(16_16_EDRAM              ),
		STRING(8_8_8_8_A                ),
		STRING(4_4_4_4                  ),
		STRING(10_11_11                 ),
		STRING(11_11_10                 ),
		STRING(DXT1                     ),
		STRING(DXT2_3                   ),
		STRING(DXT4_5                   ),
		STRING(16_16_16_16_EDRAM        ),
		STRING(24_8                     ),
		STRING(24_8_FLOAT               ),
		STRING(16                       ),
		STRING(16_16                    ),
		STRING(16_16_16_16              ),
		STRING(16_EXPAND                ),
		STRING(16_16_EXPAND             ),
		STRING(16_16_16_16_EXPAND       ),
		STRING(16_FLOAT                 ),
		STRING(16_16_FLOAT              ),
		STRING(16_16_16_16_FLOAT        ),
		STRING(32                       ),
		STRING(32_32                    ),
		STRING(32_32_32_32              ),
		STRING(32_FLOAT                 ),
		STRING(32_32_FLOAT              ),
		STRING(32_32_32_32_FLOAT        ),
		STRING(32_AS_8                  ),
		STRING(32_AS_8_8                ),
		STRING(16_MPEG                  ),
		STRING(16_16_MPEG               ),
		STRING(8_INTERLACED             ),
		STRING(32_AS_8_INTERLACED       ),
		STRING(32_AS_8_8_INTERLACED     ),
		STRING(16_INTERLACED            ),
		STRING(16_MPEG_INTERLACED       ),
		STRING(16_16_MPEG_INTERLACED    ),
		STRING(DXN                      ),
		STRING(8_8_8_8_AS_16_16_16_16   ),
		STRING(DXT1_AS_16_16_16_16      ),
		STRING(DXT2_3_AS_16_16_16_16    ),
		STRING(DXT4_5_AS_16_16_16_16    ),
		STRING(2_10_10_10_AS_16_16_16_16),
		STRING(10_11_11_AS_16_16_16_16  ),
		STRING(11_11_10_AS_16_16_16_16  ),
		STRING(32_32_32_FLOAT           ),
		STRING(DXT3A                    ),
		STRING(DXT5A                    ),
		STRING(CTX1                     ),
		STRING(DXT3A_AS_1_1_1_1         ),
		STRING(8_8_8_8_GAMMA_EDRAM      ),
		STRING(2_10_10_10_FLOAT_EDRAM   ),
#elif __PS3
		STRING(B8                       ),
		STRING(A1R5G5B5                 ),
		STRING(A4R4G4B4                 ),
		STRING(R5G6B5                   ),
		STRING(A8R8G8B8                 ),
		STRING(COMPRESSED_DXT1          ),
		STRING(COMPRESSED_DXT23         ),
		STRING(COMPRESSED_DXT45         ),
		STRING(G8B8                     ),
		STRING(R6G5B5                   ),
		STRING(DEPTH24_D8               ),
		STRING(DEPTH24_D8_FLOAT         ),
		STRING(DEPTH16                  ),
		STRING(DEPTH16_FLOAT            ),
		STRING(X16                      ),
		STRING(Y16_X16                  ),
		STRING(R5G5B5A1                 ),
		STRING(COMPRESSED_HILO8         ),
		STRING(COMPRESSED_HILO_S8       ),
		STRING(W16_Z16_Y16_X16_FLOAT    ),
		STRING(W32_Z32_Y32_X32_FLOAT    ),
		STRING(X32_FLOAT                ),
		STRING(D1R5G5B5                 ),
		STRING(D8R8G8B8                 ),
		STRING(Y16_X16_FLOAT            ),
		STRING(COMPRESSED_B8R8_G8R8     ),
		STRING(COMPRESSED_R8B8_R8G8     ),
#else
		// ..
#endif
		NULL
	};
	return strings;
}

const char** grcTextureControl::GetUIStrings_eTextureByteSwap()
{
	static const char* strings[] =
	{
		STRING(DEFAULT),
		STRING(1_1    ),
		STRING(1_2    ),
		STRING(1_4    ),
		STRING(2_4    ),
		NULL
	};
	return strings;
}

const char** grcTextureControl::GetUIStrings_eTextureControlFlag()
{
	static const char* strings[] =
	{
		STRING(DEFAULT),
		STRING(FALSE  ),
		STRING(TRUE   ),
		NULL
	};
	return strings;
}

const char** grcTextureControl::GetUIStrings_eTextureNumFormat()
{
	static const char* strings[] =
	{
		STRING(DEFAULT ),
		STRING(FRACTION),
		STRING(INTEGER ),
		NULL
	};
	return strings;
}

const char** grcTextureControl::GetUIStrings_eTextureComponentMapping()
{
	static const char* strings[] =
	{
		STRING(DEFAULT ),
		STRING(UNSIGNED),
		STRING(SIGNED  ),
		STRING(BIAS    ),
		STRING(GAMMA   ),
		NULL
	};
	return strings;
}

const char** grcTextureControl::GetUIStrings_eTextureComponentSwizzle()
{
	static const char* strings[] =
	{
		STRING(DEFAULT),
		STRING(R      ),
		STRING(G      ),
		STRING(B      ),
		STRING(A      ),
		STRING(0      ),
		STRING(1      ),
		NULL
	};
	return strings;
}

const char** grcTextureControl::GetUIStrings_eTexturePreInitByteSwap()
{
	static const char* strings[] =
	{
		STRING(NONE ),
		STRING(1_2  ),
		STRING(1_4  ),
		STRING(2_4  ),
		STRING(2_8  ),
		STRING(1_2_4),
		STRING(1_2_8),
		STRING(1_4_8),
		STRING(2_4_8),
		NULL
	};
	return strings;
}

const char** grcTextureControl::GetUIStrings_eTexturePreInitDataSize()
{
	static const char* strings[] =
	{
		STRING(NONE),
		STRING(1   ),
		STRING(2   ),
		STRING(4   ),
		NULL
	};
	return strings;
}

static bkBank*   gTextureControl_bank      = NULL;
static bkGroup*  gTextureControl_bankGroup = NULL;
static bkButton* gTextureControl_button    = NULL;
static void      gTextureControl_CreateWidgets()
{
	if (gTextureControl_bank      != NULL &&
		gTextureControl_bankGroup != NULL)
	{
		gTextureControl_bank->SetCurrentGroup(*gTextureControl_bankGroup);
		gTextureControl_button->Destroy();
		gTextureControl.CreateWidgets(*gTextureControl_bank);

		gTextureControl_bank      = NULL;
		gTextureControl_bankGroup = NULL;
	}
}

void grcTextureControl::AddWidgets(bkBank& bk)
{
	gTextureControl_bank      = &bk;
	gTextureControl_bankGroup = bk.PushGroup("Texture Control", false);
	gTextureControl_button    = bk.AddButton("Create Texture Control Widgets", gTextureControl_CreateWidgets);

	bk.PopGroup();
}

void grcTextureControl::CreateWidgets(bkBank& bk)
{
	bk.AddToggle   ("m_enabled"         ,       &m_enabled         );
	bk.AddToggle   ("m_update"          ,       &m_update          );
	bk.AddSeparator();
	bk.AddCombo    ("m_GPUFormat"       , (int*)&m_GPUFormat       , tgf_COUNT, GetUIStrings_eTextureGPUFormat       ());
#if __XENON
	bk.AddCombo    ("m_byteSwap"        , (int*)&m_byteSwap        , tbs_COUNT, GetUIStrings_eTextureByteSwap        ());
#endif
	bk.AddCombo    ("m_linear"          , (int*)&m_linear          , tcf_COUNT, GetUIStrings_eTextureControlFlag     ());
#if __XENON
	bk.AddCombo    ("m_expand"          , (int*)&m_expand          , tcf_COUNT, GetUIStrings_eTextureControlFlag     ());
	bk.AddCombo    ("m_numFormat"       , (int*)&m_numFormat       , tnf_COUNT, GetUIStrings_eTextureNumFormat       ());
	bk.AddSeparator();
	bk.AddCombo    ("m_mapping[x]"      , (int*)&m_mapping[0]      , tcm_COUNT, GetUIStrings_eTextureComponentMapping());
	bk.AddCombo    ("m_mapping[y]"      , (int*)&m_mapping[1]      , tcm_COUNT, GetUIStrings_eTextureComponentMapping());
	bk.AddCombo    ("m_mapping[z]"      , (int*)&m_mapping[2]      , tcm_COUNT, GetUIStrings_eTextureComponentMapping());
	bk.AddCombo    ("m_mapping[w]"      , (int*)&m_mapping[3]      , tcm_COUNT, GetUIStrings_eTextureComponentMapping());
#endif
	bk.AddSeparator();
	bk.AddCombo    ("m_swizzle[x]"      , (int*)&m_swizzle[0]      , tcs_COUNT, GetUIStrings_eTextureComponentSwizzle());
	bk.AddCombo    ("m_swizzle[y]"      , (int*)&m_swizzle[1]      , tcs_COUNT, GetUIStrings_eTextureComponentSwizzle());
	bk.AddCombo    ("m_swizzle[z]"      , (int*)&m_swizzle[2]      , tcs_COUNT, GetUIStrings_eTextureComponentSwizzle());
	bk.AddCombo    ("m_swizzle[w]"      , (int*)&m_swizzle[3]      , tcs_COUNT, GetUIStrings_eTextureComponentSwizzle());
#if __PS3
	bk.AddToggle   ("m_remapOrderXXXY"  ,       &m_remapOrderXXXY  );
#endif
	bk.AddSeparator();
	bk.AddCombo    ("m_preInitByteSwap" , (int*)&m_preInitByteSwap , pbs_COUNT, GetUIStrings_eTexturePreInitByteSwap ());
	bk.AddCombo    ("m_preinitDataSize" , (int*)&m_preInitDataSize , pds_COUNT, GetUIStrings_eTexturePreInitDataSize ());
	bk.AddSlider   ("m_preInitData"     ,       &m_preInitData     , 0, 0xffffffff, 1);
	bk.AddSlider   ("m_preInitDataStart",       &m_preInitDataStart, 0, 64, 1);
	bk.AddSlider   ("m_preInitDataStep" ,       &m_preInitDataStep , 0, 64, 1);
	bk.AddSeparator();
	bk.AddButton   ("RESET"             , Reset);
}

grcTextureControl gTextureControl;

void grcTextureControl::Reset()
{
	const bool bEnabled = gTextureControl.m_enabled;
	const bool bUpdate  = gTextureControl.m_update;

	memset(&gTextureControl, 0, sizeof(gTextureControl));

	gTextureControl.m_enabled = bEnabled;
	gTextureControl.m_update  = bUpdate;
}

template <typename T> static __forceinline void ByteSwap2(void* data_)
{
	T* data = reinterpret_cast<T*>(data_);
	const T temp[2] = {data[0], data[1]};

	data[0] = temp[1];
	data[1] = temp[0];
}

template <typename T> static __forceinline void ByteSwap4(void* data_)
{
	T* data = reinterpret_cast<T*>(data_);
	const T temp[4] = {data[0], data[1], data[2], data[3]};

	data[0] = temp[3];
	data[1] = temp[2];
	data[2] = temp[1];
	data[3] = temp[0];
}

void grcTextureControl::PreInitImage(grcImage* image) const
{
	if (image)
	{
		u8*       data = image->GetBits();
		const int size = image->GetSize();

		// byte-swap
		{
			switch (m_preInitByteSwap)
			{
			case pbs_NONE  : break;
			case pbs_1_2   : for (int i = 0; i <= size - 2; i += 2) { ByteSwap2<u8 >(data + i); } break;
			case pbs_1_4   : for (int i = 0; i <= size - 4; i += 4) { ByteSwap4<u8 >(data + i); } break;
			case pbs_2_4   : for (int i = 0; i <= size - 4; i += 4) { ByteSwap2<u16>(data + i); } break;
			case pbs_2_8   : for (int i = 0; i <= size - 8; i += 8) { ByteSwap4<u16>(data + i); } break;
			case pbs_1_2_4 : for (int i = 0; i <= size - 4; i += 4) { ByteSwap2<u8 >(data + i); } break;
			case pbs_1_2_8 : for (int i = 0; i <= size - 8; i += 8) { ByteSwap2<u8 >(data + i); } break;
			case pbs_1_4_8 : for (int i = 0; i <= size - 8; i += 8) { ByteSwap4<u8 >(data + i); } break;
			case pbs_2_4_8 : for (int i = 0; i <= size - 8; i += 8) { ByteSwap2<u16>(data + i); } break;
			default        : break;
			}
		}

		// data
		{
			int i0 = Max<int>(0, m_preInitDataStart);
			int xi = Max<int>(1, m_preInitDataStep );

			switch (m_preInitDataSize)
			{
			case pds_NONE : break;
			case pds_1    : xi = Max<int>(1, xi); for (int i = i0; i <= size - 1; i += xi) { *(u8 *)(data + i) = (u8 )m_preInitData; } break;
			case pds_2    : xi = Max<int>(2, xi); for (int i = i0; i <= size - 2; i += xi) { *(u16*)(data + i) = (u16)m_preInitData; } break;
			case pds_4    : xi = Max<int>(4, xi); for (int i = i0; i <= size - 4; i += xi) { *(u32*)(data + i) = (u32)m_preInitData; } break;
			default       : break;
			}
		}

		PreInitImage(image->GetNext());
		PreInitImage(image->GetNextLayer());
	}
}

void grcTextureControl::DisplayFormat(grcImage* image)
{
	Displayf("grcImage format: %s%s", grcImage::GetFormatString(image->GetFormat()), image->IsSRGB() ? " (sRGB)" : "");

	if (image->GetSize() >= 32)
	{
		const u8* data = image->GetBits();

		for (int y = 0; y < 4; y++)
		{
			Displayf("    data [%02x.%02x.%02x.%02x . %02x.%02x.%02x.%02x . %02x.%02x.%02x.%02x . %02x.%02x.%02x.%02x]",
				data[0x00 + y*16], data[0x01 + y*16], data[0x02 + y*16], data[0x03 + y*16],
				data[0x04 + y*16], data[0x05 + y*16], data[0x06 + y*16], data[0x07 + y*16],
				data[0x08 + y*16], data[0x09 + y*16], data[0x0a + y*16], data[0x0b + y*16],
				data[0x0c + y*16], data[0x0d + y*16], data[0x0e + y*16], data[0x0f + y*16]
				);
		}
	}
}

// ================================================================================================
#if __XENON

static const char* GPUFormatString_GPUTEXTUREFORMAT(u32 x)
{
	switch (x)
	{
	SWITCH_CASE(GPUTEXTUREFORMAT_1_REVERSE                );
	SWITCH_CASE(GPUTEXTUREFORMAT_1                        );
	SWITCH_CASE(GPUTEXTUREFORMAT_8                        );
	SWITCH_CASE(GPUTEXTUREFORMAT_1_5_5_5                  );
	SWITCH_CASE(GPUTEXTUREFORMAT_5_6_5                    );
	SWITCH_CASE(GPUTEXTUREFORMAT_6_5_5                    );
	SWITCH_CASE(GPUTEXTUREFORMAT_8_8_8_8                  );
	SWITCH_CASE(GPUTEXTUREFORMAT_2_10_10_10               );
	SWITCH_CASE(GPUTEXTUREFORMAT_8_A                      );
	SWITCH_CASE(GPUTEXTUREFORMAT_8_B                      );
	SWITCH_CASE(GPUTEXTUREFORMAT_8_8                      );
	SWITCH_CASE(GPUTEXTUREFORMAT_Cr_Y1_Cb_Y0_REP          );
	SWITCH_CASE(GPUTEXTUREFORMAT_Y1_Cr_Y0_Cb_REP          );
	SWITCH_CASE(GPUTEXTUREFORMAT_16_16_EDRAM              );
	SWITCH_CASE(GPUTEXTUREFORMAT_8_8_8_8_A                );
	SWITCH_CASE(GPUTEXTUREFORMAT_4_4_4_4                  );
	SWITCH_CASE(GPUTEXTUREFORMAT_10_11_11                 );
	SWITCH_CASE(GPUTEXTUREFORMAT_11_11_10                 );
	SWITCH_CASE(GPUTEXTUREFORMAT_DXT1                     );
	SWITCH_CASE(GPUTEXTUREFORMAT_DXT2_3                   );
	SWITCH_CASE(GPUTEXTUREFORMAT_DXT4_5                   );
	SWITCH_CASE(GPUTEXTUREFORMAT_16_16_16_16_EDRAM        );
	SWITCH_CASE(GPUTEXTUREFORMAT_24_8                     );
	SWITCH_CASE(GPUTEXTUREFORMAT_24_8_FLOAT               );
	SWITCH_CASE(GPUTEXTUREFORMAT_16                       );
	SWITCH_CASE(GPUTEXTUREFORMAT_16_16                    );
	SWITCH_CASE(GPUTEXTUREFORMAT_16_16_16_16              );
	SWITCH_CASE(GPUTEXTUREFORMAT_16_EXPAND                );
	SWITCH_CASE(GPUTEXTUREFORMAT_16_16_EXPAND             );
	SWITCH_CASE(GPUTEXTUREFORMAT_16_16_16_16_EXPAND       );
	SWITCH_CASE(GPUTEXTUREFORMAT_16_FLOAT                 );
	SWITCH_CASE(GPUTEXTUREFORMAT_16_16_FLOAT              );
	SWITCH_CASE(GPUTEXTUREFORMAT_16_16_16_16_FLOAT        );
	SWITCH_CASE(GPUTEXTUREFORMAT_32                       );
	SWITCH_CASE(GPUTEXTUREFORMAT_32_32                    );
	SWITCH_CASE(GPUTEXTUREFORMAT_32_32_32_32              );
	SWITCH_CASE(GPUTEXTUREFORMAT_32_FLOAT                 );
	SWITCH_CASE(GPUTEXTUREFORMAT_32_32_FLOAT              );
	SWITCH_CASE(GPUTEXTUREFORMAT_32_32_32_32_FLOAT        );
	SWITCH_CASE(GPUTEXTUREFORMAT_32_AS_8                  );
	SWITCH_CASE(GPUTEXTUREFORMAT_32_AS_8_8                );
	SWITCH_CASE(GPUTEXTUREFORMAT_16_MPEG                  );
	SWITCH_CASE(GPUTEXTUREFORMAT_16_16_MPEG               );
	SWITCH_CASE(GPUTEXTUREFORMAT_8_INTERLACED             );
	SWITCH_CASE(GPUTEXTUREFORMAT_32_AS_8_INTERLACED       );
	SWITCH_CASE(GPUTEXTUREFORMAT_32_AS_8_8_INTERLACED     );
	SWITCH_CASE(GPUTEXTUREFORMAT_16_INTERLACED            );
	SWITCH_CASE(GPUTEXTUREFORMAT_16_MPEG_INTERLACED       );
	SWITCH_CASE(GPUTEXTUREFORMAT_16_16_MPEG_INTERLACED    );
	SWITCH_CASE(GPUTEXTUREFORMAT_DXN                      );
	SWITCH_CASE(GPUTEXTUREFORMAT_8_8_8_8_AS_16_16_16_16   );
	SWITCH_CASE(GPUTEXTUREFORMAT_DXT1_AS_16_16_16_16      );
	SWITCH_CASE(GPUTEXTUREFORMAT_DXT2_3_AS_16_16_16_16    );
	SWITCH_CASE(GPUTEXTUREFORMAT_DXT4_5_AS_16_16_16_16    );
	SWITCH_CASE(GPUTEXTUREFORMAT_2_10_10_10_AS_16_16_16_16);
	SWITCH_CASE(GPUTEXTUREFORMAT_10_11_11_AS_16_16_16_16  );
	SWITCH_CASE(GPUTEXTUREFORMAT_11_11_10_AS_16_16_16_16  );
	SWITCH_CASE(GPUTEXTUREFORMAT_32_32_32_FLOAT           );
	SWITCH_CASE(GPUTEXTUREFORMAT_DXT3A                    );
	SWITCH_CASE(GPUTEXTUREFORMAT_DXT5A                    );
	SWITCH_CASE(GPUTEXTUREFORMAT_CTX1                     );
	SWITCH_CASE(GPUTEXTUREFORMAT_DXT3A_AS_1_1_1_1         );
	SWITCH_CASE(GPUTEXTUREFORMAT_8_8_8_8_GAMMA_EDRAM      );
	SWITCH_CASE(GPUTEXTUREFORMAT_2_10_10_10_FLOAT_EDRAM   );
	}

	return "GPUTEXTUREFORMAT_UNKNOWN";
}

static const char* GPUFormatString_GPUENDIAN(u32 x)
{
	switch ((GPUENDIAN)x)
	{
	SWITCH_CASE(GPUENDIAN_NONE  );
	SWITCH_CASE(GPUENDIAN_8IN16 );
	SWITCH_CASE(GPUENDIAN_8IN32 );
	SWITCH_CASE(GPUENDIAN_16IN32);
	}

	return "GPUENDIAN_UNKNOWN";
}

enum GPUTILED
{
	GPUTILED_FALSE = 0,
	GPUTILED_TRUE  = 1,
};

static const char* GPUFormatString_GPUTILED(u32 x)
{
	switch ((GPUTILED)x)
	{
		SWITCH_CASE(GPUTILED_FALSE);
		SWITCH_CASE(GPUTILED_TRUE );
	}

	return "GPUTILED_UNKNOWN";
}

static const char* GPUFormatString_GPUSIGN(u32 x)
{
	switch ((GPUSIGN)x)
	{
	SWITCH_CASE(GPUSIGN_UNSIGNED);
	SWITCH_CASE(GPUSIGN_SIGNED  );
	SWITCH_CASE(GPUSIGN_BIAS    );
	SWITCH_CASE(GPUSIGN_GAMMA   );
	}

	return "GPUSIGN_UNKNOWN";
}

static const char* GPUFormatString_GPUNUMFORMAT(u32 x)
{
	switch ((GPUNUMFORMAT)x)
	{
		SWITCH_CASE(GPUNUMFORMAT_FRACTION);
		SWITCH_CASE(GPUNUMFORMAT_INTEGER );
	}

	return "GPUNUMFORMAT_UNKNOWN";
}

static const char* GPUFormatString_GPUSWIZZLE(u32 x)
{
	switch ((GPUSWIZZLE)x)
	{
	SWITCH_CASE(GPUSWIZZLE_X   );
	SWITCH_CASE(GPUSWIZZLE_Y   );
	SWITCH_CASE(GPUSWIZZLE_Z   );
	SWITCH_CASE(GPUSWIZZLE_W   );
	SWITCH_CASE(GPUSWIZZLE_0   );
	SWITCH_CASE(GPUSWIZZLE_1   ); 
	SWITCH_CASE(GPUSWIZZLE_KEEP);
	}

	return "GPUSWIZZLE_UNKNOWN";
}

static int GetGPUFormatBitsPerPixel(u32 GPUFormat)
{
	switch (GPUFormat)
	{
	case GPUTEXTUREFORMAT_1_REVERSE                 : return   1;
	case GPUTEXTUREFORMAT_1                         : return   1;
	case GPUTEXTUREFORMAT_8                         : return   8;
	case GPUTEXTUREFORMAT_1_5_5_5                   : return  16;
	case GPUTEXTUREFORMAT_5_6_5                     : return  16;
	case GPUTEXTUREFORMAT_6_5_5                     : return  16;
	case GPUTEXTUREFORMAT_8_8_8_8                   : return  32;
	case GPUTEXTUREFORMAT_2_10_10_10                : return  32;
	case GPUTEXTUREFORMAT_8_A                       : return   8;
	case GPUTEXTUREFORMAT_8_B                       : return   8;
	case GPUTEXTUREFORMAT_8_8                       : return  16;
	case GPUTEXTUREFORMAT_Cr_Y1_Cb_Y0_REP           : return  16;
	case GPUTEXTUREFORMAT_Y1_Cr_Y0_Cb_REP           : return  16;
	case GPUTEXTUREFORMAT_16_16_EDRAM               : return  32;
	case GPUTEXTUREFORMAT_8_8_8_8_A                 : return  32;
	case GPUTEXTUREFORMAT_4_4_4_4                   : return  16;
	case GPUTEXTUREFORMAT_10_11_11                  : return  32;
	case GPUTEXTUREFORMAT_11_11_10                  : return  32;
	case GPUTEXTUREFORMAT_DXT1                      : return   4;
	case GPUTEXTUREFORMAT_DXT2_3                    : return   8;
	case GPUTEXTUREFORMAT_DXT4_5                    : return   8;
	case GPUTEXTUREFORMAT_16_16_16_16_EDRAM         : return  64;
	case GPUTEXTUREFORMAT_24_8                      : return  32;
	case GPUTEXTUREFORMAT_24_8_FLOAT                : return  32;
	case GPUTEXTUREFORMAT_16                        : return  16;
	case GPUTEXTUREFORMAT_16_16                     : return  32;
	case GPUTEXTUREFORMAT_16_16_16_16               : return  64;
	case GPUTEXTUREFORMAT_16_EXPAND                 : return  16;
	case GPUTEXTUREFORMAT_16_16_EXPAND              : return  32;
	case GPUTEXTUREFORMAT_16_16_16_16_EXPAND        : return  64;
	case GPUTEXTUREFORMAT_16_FLOAT                  : return  16;
	case GPUTEXTUREFORMAT_16_16_FLOAT               : return  32;
	case GPUTEXTUREFORMAT_16_16_16_16_FLOAT         : return  64;
	case GPUTEXTUREFORMAT_32                        : return  32;
	case GPUTEXTUREFORMAT_32_32                     : return  64;
	case GPUTEXTUREFORMAT_32_32_32_32               : return 128;
	case GPUTEXTUREFORMAT_32_FLOAT                  : return  32;
	case GPUTEXTUREFORMAT_32_32_FLOAT               : return  64;
	case GPUTEXTUREFORMAT_32_32_32_32_FLOAT         : return 128;
	case GPUTEXTUREFORMAT_32_AS_8                   : return   8;
	case GPUTEXTUREFORMAT_32_AS_8_8                 : return  16;
	case GPUTEXTUREFORMAT_16_MPEG                   : return  16;
	case GPUTEXTUREFORMAT_16_16_MPEG                : return  32;
	case GPUTEXTUREFORMAT_8_INTERLACED              : return   8;
	case GPUTEXTUREFORMAT_32_AS_8_INTERLACED        : return   8;
	case GPUTEXTUREFORMAT_32_AS_8_8_INTERLACED      : return  16;
	case GPUTEXTUREFORMAT_16_INTERLACED             : return  16;
	case GPUTEXTUREFORMAT_16_MPEG_INTERLACED        : return  16;
	case GPUTEXTUREFORMAT_16_16_MPEG_INTERLACED     : return  32;
	case GPUTEXTUREFORMAT_DXN                       : return   8;
	case GPUTEXTUREFORMAT_8_8_8_8_AS_16_16_16_16    : return  32;
	case GPUTEXTUREFORMAT_DXT1_AS_16_16_16_16       : return   4;
	case GPUTEXTUREFORMAT_DXT2_3_AS_16_16_16_16     : return   8;
	case GPUTEXTUREFORMAT_DXT4_5_AS_16_16_16_16     : return   8;
	case GPUTEXTUREFORMAT_2_10_10_10_AS_16_16_16_16 : return  32;
	case GPUTEXTUREFORMAT_10_11_11_AS_16_16_16_16   : return  32;
	case GPUTEXTUREFORMAT_11_11_10_AS_16_16_16_16   : return  32;
	case GPUTEXTUREFORMAT_32_32_32_FLOAT            : return  96;
	case GPUTEXTUREFORMAT_DXT3A                     : return   4;
	case GPUTEXTUREFORMAT_DXT5A                     : return   4;
	case GPUTEXTUREFORMAT_CTX1                      : return   4;
	case GPUTEXTUREFORMAT_DXT3A_AS_1_1_1_1          : return   4;
	case GPUTEXTUREFORMAT_8_8_8_8_GAMMA_EDRAM       : return  32;
	case GPUTEXTUREFORMAT_2_10_10_10_FLOAT_EDRAM    : return  32;
	}

	return 0;
}

void grcTextureControl::DisplayFormat(grcImage* image, u32 format, u32)
{
	DisplayFormat(image);

	Displayf("GPU format [0x%08x]:", format);
	Displayf("    %s", GPUFormatString_GPUTEXTUREFORMAT((format & D3DFORMAT_TEXTUREFORMAT_MASK) >> D3DFORMAT_TEXTUREFORMAT_SHIFT));
	Displayf("    %s", GPUFormatString_GPUENDIAN       ((format & D3DFORMAT_ENDIAN_MASK       ) >> D3DFORMAT_ENDIAN_SHIFT       ));
	Displayf("    %s", GPUFormatString_GPUTILED        ((format & D3DFORMAT_TILED_MASK        ) >> D3DFORMAT_TILED_SHIFT        ));
	Displayf("  x:%s", GPUFormatString_GPUSIGN         ((format & D3DFORMAT_SIGNX_MASK        ) >> D3DFORMAT_SIGNX_SHIFT        ));
	Displayf("  y:%s", GPUFormatString_GPUSIGN         ((format & D3DFORMAT_SIGNY_MASK        ) >> D3DFORMAT_SIGNY_SHIFT        ));
	Displayf("  z:%s", GPUFormatString_GPUSIGN         ((format & D3DFORMAT_SIGNZ_MASK        ) >> D3DFORMAT_SIGNZ_SHIFT        ));
	Displayf("  w:%s", GPUFormatString_GPUSIGN         ((format & D3DFORMAT_SIGNW_MASK        ) >> D3DFORMAT_SIGNW_SHIFT        ));
	Displayf("    %s", GPUFormatString_GPUNUMFORMAT    ((format & D3DFORMAT_NUMFORMAT_MASK    ) >> D3DFORMAT_NUMFORMAT_SHIFT    ));
	Displayf("  x:%s", GPUFormatString_GPUSWIZZLE      ((format & D3DFORMAT_SWIZZLEX_MASK     ) >> D3DFORMAT_SWIZZLEX_SHIFT     ));
	Displayf("  y:%s", GPUFormatString_GPUSWIZZLE      ((format & D3DFORMAT_SWIZZLEY_MASK     ) >> D3DFORMAT_SWIZZLEY_SHIFT     ));
	Displayf("  z:%s", GPUFormatString_GPUSWIZZLE      ((format & D3DFORMAT_SWIZZLEZ_MASK     ) >> D3DFORMAT_SWIZZLEZ_SHIFT     ));
	Displayf("  w:%s", GPUFormatString_GPUSWIZZLE      ((format & D3DFORMAT_SWIZZLEW_MASK     ) >> D3DFORMAT_SWIZZLEW_SHIFT     ));
}

bool grcTextureControl::UpdateFormat(grcImage* image, u32& format, u32&)
{
	//#define GPUSWIZZLE_ARGB (GPUSWIZZLE_Z | GPUSWIZZLE_Y<<3 | GPUSWIZZLE_X<<6 | GPUSWIZZLE_W<<9)
	//#define GPUSWIZZLE_ORGB (GPUSWIZZLE_Z | GPUSWIZZLE_Y<<3 | GPUSWIZZLE_X<<6 | GPUSWIZZLE_1<<9) // NOTE -- 'O' means opaque (i.e. 1) .. 'Z' means zero (0)
	//#define GPUSWIZZLE_ABGR (GPUSWIZZLE_X | GPUSWIZZLE_Y<<3 | GPUSWIZZLE_Z<<6 | GPUSWIZZLE_W<<9)
	//#define GPUSWIZZLE_OBGR (GPUSWIZZLE_X | GPUSWIZZLE_Y<<3 | GPUSWIZZLE_Z<<6 | GPUSWIZZLE_1<<9)
	//#define GPUSWIZZLE_OOGR (GPUSWIZZLE_X | GPUSWIZZLE_Y<<3 | GPUSWIZZLE_1<<6 | GPUSWIZZLE_1<<9)
	//#define GPUSWIZZLE_OZGR (GPUSWIZZLE_X | GPUSWIZZLE_Y<<3 | GPUSWIZZLE_0<<6 | GPUSWIZZLE_1<<9)
	//#define GPUSWIZZLE_RZZZ (GPUSWIZZLE_0 | GPUSWIZZLE_0<<3 | GPUSWIZZLE_0<<6 | GPUSWIZZLE_X<<9)
	//#define GPUSWIZZLE_OOOR (GPUSWIZZLE_X | GPUSWIZZLE_1<<3 | GPUSWIZZLE_1<<6 | GPUSWIZZLE_1<<9)
	//#define GPUSWIZZLE_ORRR (GPUSWIZZLE_X | GPUSWIZZLE_X<<3 | GPUSWIZZLE_X<<6 | GPUSWIZZLE_1<<9)
	//#define GPUSWIZZLE_GRRR (GPUSWIZZLE_X | GPUSWIZZLE_X<<3 | GPUSWIZZLE_X<<6 | GPUSWIZZLE_Y<<9)
	//#define GPUSWIZZLE_RGBA (GPUSWIZZLE_W | GPUSWIZZLE_Z<<3 | GPUSWIZZLE_Y<<6 | GPUSWIZZLE_X<<9)
	//
	//#define MAKED3DFMT2(TextureFormat, Endian, Tiled, TextureSignX, TextureSignY, TextureSignZ, TextureSignW, NumFormat, SwizzleX, SwizzleY, SwizzleZ, SwizzleW) \
	//( \
	//	0 \
	//	| (TextureFormat) << D3DFORMAT_TEXTUREFORMAT_SHIFT \
	//	| (Endian       ) << D3DFORMAT_ENDIAN_SHIFT        \
	//	| (Tiled        ) << D3DFORMAT_TILED_SHIFT         \
	//	| (TextureSignX ) << D3DFORMAT_SIGNX_SHIFT         \
	//	| (TextureSignY ) << D3DFORMAT_SIGNY_SHIFT         \
	//	| (TextureSignZ ) << D3DFORMAT_SIGNZ_SHIFT         \
	//	| (TextureSignW ) << D3DFORMAT_SIGNW_SHIFT         \
	//	| (NumFormat    ) << D3DFORMAT_NUMFORMAT_SHIFT     \
	//	| (SwizzleX     ) << D3DFORMAT_SWIZZLEX_SHIFT      \
	//	| (SwizzleY     ) << D3DFORMAT_SWIZZLEY_SHIFT      \
	//	| (SwizzleZ     ) << D3DFORMAT_SWIZZLEZ_SHIFT      \
	//	| (SwizzleW     ) << D3DFORMAT_SWIZZLEW_SHIFT      \
	//) \
	//// end.

	if (m_enabled)
	{
		PreInitImage(image);

		u32 currentGPUFormat = (format & D3DFORMAT_TEXTUREFORMAT_MASK) >> D3DFORMAT_TEXTUREFORMAT_SHIFT;

		if (m_update)
		{
			switch (currentGPUFormat)
			{
			case GPUTEXTUREFORMAT_1_REVERSE                 : m_GPUFormat = tgf_1_REVERSE                ; break;
			case GPUTEXTUREFORMAT_1                         : m_GPUFormat = tgf_1                        ; break;
			case GPUTEXTUREFORMAT_8                         : m_GPUFormat = tgf_8                        ; break;
			case GPUTEXTUREFORMAT_1_5_5_5                   : m_GPUFormat = tgf_1_5_5_5                  ; break;
			case GPUTEXTUREFORMAT_5_6_5                     : m_GPUFormat = tgf_5_6_5                    ; break;
			case GPUTEXTUREFORMAT_6_5_5                     : m_GPUFormat = tgf_6_5_5                    ; break;
			case GPUTEXTUREFORMAT_8_8_8_8                   : m_GPUFormat = tgf_8_8_8_8                  ; break;
			case GPUTEXTUREFORMAT_2_10_10_10                : m_GPUFormat = tgf_2_10_10_10               ; break;
			case GPUTEXTUREFORMAT_8_A                       : m_GPUFormat = tgf_8_A                      ; break;
			case GPUTEXTUREFORMAT_8_B                       : m_GPUFormat = tgf_8_B                      ; break;
			case GPUTEXTUREFORMAT_8_8                       : m_GPUFormat = tgf_8_8                      ; break;
			case GPUTEXTUREFORMAT_Cr_Y1_Cb_Y0_REP           : m_GPUFormat = tgf_Cr_Y1_Cb_Y0_REP          ; break;
			case GPUTEXTUREFORMAT_Y1_Cr_Y0_Cb_REP           : m_GPUFormat = tgf_Y1_Cr_Y0_Cb_REP          ; break;
			case GPUTEXTUREFORMAT_16_16_EDRAM               : m_GPUFormat = tgf_16_16_EDRAM              ; break;
			case GPUTEXTUREFORMAT_8_8_8_8_A                 : m_GPUFormat = tgf_8_8_8_8_A                ; break;
			case GPUTEXTUREFORMAT_4_4_4_4                   : m_GPUFormat = tgf_4_4_4_4                  ; break;
			case GPUTEXTUREFORMAT_10_11_11                  : m_GPUFormat = tgf_10_11_11                 ; break;
			case GPUTEXTUREFORMAT_11_11_10                  : m_GPUFormat = tgf_11_11_10                 ; break;
			case GPUTEXTUREFORMAT_DXT1                      : m_GPUFormat = tgf_DXT1                     ; break;
			case GPUTEXTUREFORMAT_DXT2_3                    : m_GPUFormat = tgf_DXT2_3                   ; break;
			case GPUTEXTUREFORMAT_DXT4_5                    : m_GPUFormat = tgf_DXT4_5                   ; break;
			case GPUTEXTUREFORMAT_16_16_16_16_EDRAM         : m_GPUFormat = tgf_16_16_16_16_EDRAM        ; break;
			case GPUTEXTUREFORMAT_24_8                      : m_GPUFormat = tgf_24_8                     ; break;
			case GPUTEXTUREFORMAT_24_8_FLOAT                : m_GPUFormat = tgf_24_8_FLOAT               ; break;
			case GPUTEXTUREFORMAT_16                        : m_GPUFormat = tgf_16                       ; break;
			case GPUTEXTUREFORMAT_16_16                     : m_GPUFormat = tgf_16_16                    ; break;
			case GPUTEXTUREFORMAT_16_16_16_16               : m_GPUFormat = tgf_16_16_16_16              ; break;
			case GPUTEXTUREFORMAT_16_EXPAND                 : m_GPUFormat = tgf_16_EXPAND                ; break;
			case GPUTEXTUREFORMAT_16_16_EXPAND              : m_GPUFormat = tgf_16_16_EXPAND             ; break;
			case GPUTEXTUREFORMAT_16_16_16_16_EXPAND        : m_GPUFormat = tgf_16_16_16_16_EXPAND       ; break;
			case GPUTEXTUREFORMAT_16_FLOAT                  : m_GPUFormat = tgf_16_FLOAT                 ; break;
			case GPUTEXTUREFORMAT_16_16_FLOAT               : m_GPUFormat = tgf_16_16_FLOAT              ; break;
			case GPUTEXTUREFORMAT_16_16_16_16_FLOAT         : m_GPUFormat = tgf_16_16_16_16_FLOAT        ; break;
			case GPUTEXTUREFORMAT_32                        : m_GPUFormat = tgf_32                       ; break;
			case GPUTEXTUREFORMAT_32_32                     : m_GPUFormat = tgf_32_32                    ; break;
			case GPUTEXTUREFORMAT_32_32_32_32               : m_GPUFormat = tgf_32_32_32_32              ; break;
			case GPUTEXTUREFORMAT_32_FLOAT                  : m_GPUFormat = tgf_32_FLOAT                 ; break;
			case GPUTEXTUREFORMAT_32_32_FLOAT               : m_GPUFormat = tgf_32_32_FLOAT              ; break;
			case GPUTEXTUREFORMAT_32_32_32_32_FLOAT         : m_GPUFormat = tgf_32_32_32_32_FLOAT        ; break;
			case GPUTEXTUREFORMAT_32_AS_8                   : m_GPUFormat = tgf_32_AS_8                  ; break;
			case GPUTEXTUREFORMAT_32_AS_8_8                 : m_GPUFormat = tgf_32_AS_8_8                ; break;
			case GPUTEXTUREFORMAT_16_MPEG                   : m_GPUFormat = tgf_16_MPEG                  ; break;
			case GPUTEXTUREFORMAT_16_16_MPEG                : m_GPUFormat = tgf_16_16_MPEG               ; break;
			case GPUTEXTUREFORMAT_8_INTERLACED              : m_GPUFormat = tgf_8_INTERLACED             ; break;
			case GPUTEXTUREFORMAT_32_AS_8_INTERLACED        : m_GPUFormat = tgf_32_AS_8_INTERLACED       ; break;
			case GPUTEXTUREFORMAT_32_AS_8_8_INTERLACED      : m_GPUFormat = tgf_32_AS_8_8_INTERLACED     ; break;
			case GPUTEXTUREFORMAT_16_INTERLACED             : m_GPUFormat = tgf_16_INTERLACED            ; break;
			case GPUTEXTUREFORMAT_16_MPEG_INTERLACED        : m_GPUFormat = tgf_16_MPEG_INTERLACED       ; break;
			case GPUTEXTUREFORMAT_16_16_MPEG_INTERLACED     : m_GPUFormat = tgf_16_16_MPEG_INTERLACED    ; break;
			case GPUTEXTUREFORMAT_DXN                       : m_GPUFormat = tgf_DXN                      ; break;
			case GPUTEXTUREFORMAT_8_8_8_8_AS_16_16_16_16    : m_GPUFormat = tgf_8_8_8_8_AS_16_16_16_16   ; break;
			case GPUTEXTUREFORMAT_DXT1_AS_16_16_16_16       : m_GPUFormat = tgf_DXT1_AS_16_16_16_16      ; break;
			case GPUTEXTUREFORMAT_DXT2_3_AS_16_16_16_16     : m_GPUFormat = tgf_DXT2_3_AS_16_16_16_16    ; break;
			case GPUTEXTUREFORMAT_DXT4_5_AS_16_16_16_16     : m_GPUFormat = tgf_DXT4_5_AS_16_16_16_16    ; break;
			case GPUTEXTUREFORMAT_2_10_10_10_AS_16_16_16_16 : m_GPUFormat = tgf_2_10_10_10_AS_16_16_16_16; break;
			case GPUTEXTUREFORMAT_10_11_11_AS_16_16_16_16   : m_GPUFormat = tgf_10_11_11_AS_16_16_16_16  ; break;
			case GPUTEXTUREFORMAT_11_11_10_AS_16_16_16_16   : m_GPUFormat = tgf_11_11_10_AS_16_16_16_16  ; break;
			case GPUTEXTUREFORMAT_32_32_32_FLOAT            : m_GPUFormat = tgf_32_32_32_FLOAT           ; break;
			case GPUTEXTUREFORMAT_DXT3A                     : m_GPUFormat = tgf_DXT3A                    ; break;
			case GPUTEXTUREFORMAT_DXT5A                     : m_GPUFormat = tgf_DXT5A                    ; break;
			case GPUTEXTUREFORMAT_CTX1                      : m_GPUFormat = tgf_CTX1                     ; break;
			case GPUTEXTUREFORMAT_DXT3A_AS_1_1_1_1          : m_GPUFormat = tgf_DXT3A_AS_1_1_1_1         ; break;
			case GPUTEXTUREFORMAT_8_8_8_8_GAMMA_EDRAM       : m_GPUFormat = tgf_8_8_8_8_GAMMA_EDRAM      ; break;
			case GPUTEXTUREFORMAT_2_10_10_10_FLOAT_EDRAM    : m_GPUFormat = tgf_2_10_10_10_FLOAT_EDRAM   ; break;
			default                                         : m_GPUFormat = tgf_DEFAULT                  ; break;
			}

			switch ((GPUENDIAN)((format & D3DFORMAT_ENDIAN_MASK) >> D3DFORMAT_ENDIAN_SHIFT))
			{
			case GPUENDIAN_NONE   : m_byteSwap = tbs_1_1    ; break;
			case GPUENDIAN_8IN16  : m_byteSwap = tbs_1_2    ; break;
			case GPUENDIAN_8IN32  : m_byteSwap = tbs_1_4    ; break;
			case GPUENDIAN_16IN32 : m_byteSwap = tbs_2_4    ; break;
			default               : m_byteSwap = tbs_DEFAULT; break;
			}

			for (int i = 0; i < 4; i++) // x,y,z,w
			{
				// mapping
				{
					const int shift = D3DFORMAT_SIGNX_SHIFT + (D3DFORMAT_SIGNY_SHIFT - D3DFORMAT_SIGNX_SHIFT)*i;
					const u32 mask  = D3DFORMAT_SIGNX_MASK << (D3DFORMAT_SIGNY_SHIFT - D3DFORMAT_SIGNX_SHIFT)*i;

					switch ((GPUSIGN)((format & mask) >> shift))
					{
					case GPUSIGN_UNSIGNED : m_mapping[i] = tcm_UNSIGNED; break;
					case GPUSIGN_SIGNED   : m_mapping[i] = tcm_SIGNED  ; break;
					case GPUSIGN_BIAS     : m_mapping[i] = tcm_BIAS    ; break;
					case GPUSIGN_GAMMA    : m_mapping[i] = tcm_GAMMA   ; break;
					default               : m_mapping[i] = tcm_DEFAULT ; break;
					}
				}

				// swizzle
				{
					const int shift = D3DFORMAT_SWIZZLEX_SHIFT + (D3DFORMAT_SWIZZLEY_SHIFT - D3DFORMAT_SWIZZLEX_SHIFT)*i;
					const u32 mask  = D3DFORMAT_SWIZZLEX_MASK << (D3DFORMAT_SWIZZLEY_SHIFT - D3DFORMAT_SWIZZLEX_SHIFT)*i;

					switch ((GPUSWIZZLE)((format & mask) >> shift))
					{
					case GPUSWIZZLE_X    : m_swizzle[i] = tcs_R      ; break;
					case GPUSWIZZLE_Y    : m_swizzle[i] = tcs_G      ; break;
					case GPUSWIZZLE_Z    : m_swizzle[i] = tcs_B      ; break;
					case GPUSWIZZLE_W    : m_swizzle[i] = tcs_A      ; break;
					case GPUSWIZZLE_0    : m_swizzle[i] = tcs_0      ; break;
					case GPUSWIZZLE_1    : m_swizzle[i] = tcs_1      ; break;
					case GPUSWIZZLE_KEEP : m_swizzle[i] = tcs_DEFAULT; break; // <-- fetch instructions only, should not actually hit this
					default              : m_swizzle[i] = tcs_DEFAULT; break;
					}
				}
			}

			switch ((GPUTILED)((format & D3DFORMAT_TILED_MASK) >> D3DFORMAT_TILED_SHIFT))
			{
			case GPUTILED_TRUE  : m_linear = tcf_FALSE  ; break;
			case GPUTILED_FALSE : m_linear = tcf_TRUE   ; break;
			default             : m_linear = tcf_DEFAULT; break;
			}

			switch (currentGPUFormat)
			{
			case GPUTEXTUREFORMAT_16_FLOAT                  : m_expand = tcf_FALSE  ; break;
			case GPUTEXTUREFORMAT_16_16_FLOAT               : m_expand = tcf_FALSE  ; break;
			case GPUTEXTUREFORMAT_16_16_16_16_FLOAT         : m_expand = tcf_FALSE  ; break;

			case GPUTEXTUREFORMAT_8_8_8_8                   : m_expand = tcf_FALSE  ; break;
			case GPUTEXTUREFORMAT_DXT1                      : m_expand = tcf_FALSE  ; break;
			case GPUTEXTUREFORMAT_DXT2_3                    : m_expand = tcf_FALSE  ; break;
			case GPUTEXTUREFORMAT_DXT4_5                    : m_expand = tcf_FALSE  ; break;
			case GPUTEXTUREFORMAT_2_10_10_10                : m_expand = tcf_FALSE  ; break;
			case GPUTEXTUREFORMAT_10_11_11                  : m_expand = tcf_FALSE  ; break;
			case GPUTEXTUREFORMAT_11_11_10                  : m_expand = tcf_FALSE  ; break;

			case GPUTEXTUREFORMAT_16_EXPAND                 : m_expand = tcf_TRUE   ; break;
			case GPUTEXTUREFORMAT_16_16_EXPAND              : m_expand = tcf_TRUE   ; break;
			case GPUTEXTUREFORMAT_16_16_16_16_EXPAND        : m_expand = tcf_TRUE   ; break;

			case GPUTEXTUREFORMAT_8_8_8_8_AS_16_16_16_16    : m_expand = tcf_TRUE   ; break;
			case GPUTEXTUREFORMAT_DXT1_AS_16_16_16_16       : m_expand = tcf_TRUE   ; break;
			case GPUTEXTUREFORMAT_DXT2_3_AS_16_16_16_16     : m_expand = tcf_TRUE   ; break;
			case GPUTEXTUREFORMAT_DXT4_5_AS_16_16_16_16     : m_expand = tcf_TRUE   ; break;
			case GPUTEXTUREFORMAT_2_10_10_10_AS_16_16_16_16 : m_expand = tcf_TRUE   ; break;
			case GPUTEXTUREFORMAT_10_11_11_AS_16_16_16_16   : m_expand = tcf_TRUE   ; break;
			case GPUTEXTUREFORMAT_11_11_10_AS_16_16_16_16   : m_expand = tcf_TRUE   ; break;

			default                                         : m_expand = tcf_DEFAULT; break; 
			}

			switch ((GPUNUMFORMAT)((format & D3DFORMAT_NUMFORMAT_MASK) >> D3DFORMAT_NUMFORMAT_SHIFT))
			{
			case GPUNUMFORMAT_FRACTION : m_numFormat = tnf_FRACTION; break;
			case GPUNUMFORMAT_INTEGER  : m_numFormat = tnf_INTEGER ; break;
			default                    : m_numFormat = tnf_DEFAULT ; break;
			}
		}
		else // apply
		{
			if (m_GPUFormat != tgf_DEFAULT) // apply GPU format
			{
				u32 desiredGPUFormat = 0;

				switch (m_GPUFormat)
				{
				case tgf_DEFAULT                   : break;
				case tgf_1_REVERSE                 : desiredGPUFormat = GPUTEXTUREFORMAT_1_REVERSE                ; break;
				case tgf_1                         : desiredGPUFormat = GPUTEXTUREFORMAT_1                        ; break;
				case tgf_8                         : desiredGPUFormat = GPUTEXTUREFORMAT_8                        ; break;
				case tgf_1_5_5_5                   : desiredGPUFormat = GPUTEXTUREFORMAT_1_5_5_5                  ; break;
				case tgf_5_6_5                     : desiredGPUFormat = GPUTEXTUREFORMAT_5_6_5                    ; break;
				case tgf_6_5_5                     : desiredGPUFormat = GPUTEXTUREFORMAT_6_5_5                    ; break;
				case tgf_8_8_8_8                   : desiredGPUFormat = GPUTEXTUREFORMAT_8_8_8_8                  ; break;
				case tgf_2_10_10_10                : desiredGPUFormat = GPUTEXTUREFORMAT_2_10_10_10               ; break;
				case tgf_8_A                       : desiredGPUFormat = GPUTEXTUREFORMAT_8_A                      ; break;
				case tgf_8_B                       : desiredGPUFormat = GPUTEXTUREFORMAT_8_B                      ; break;
				case tgf_8_8                       : desiredGPUFormat = GPUTEXTUREFORMAT_8_8                      ; break;
				case tgf_Cr_Y1_Cb_Y0_REP           : desiredGPUFormat = GPUTEXTUREFORMAT_Cr_Y1_Cb_Y0_REP          ; break;
				case tgf_Y1_Cr_Y0_Cb_REP           : desiredGPUFormat = GPUTEXTUREFORMAT_Y1_Cr_Y0_Cb_REP          ; break;
				case tgf_16_16_EDRAM               : desiredGPUFormat = GPUTEXTUREFORMAT_16_16_EDRAM              ; break;
				case tgf_8_8_8_8_A                 : desiredGPUFormat = GPUTEXTUREFORMAT_8_8_8_8_A                ; break;
				case tgf_4_4_4_4                   : desiredGPUFormat = GPUTEXTUREFORMAT_4_4_4_4                  ; break;
				case tgf_10_11_11                  : desiredGPUFormat = GPUTEXTUREFORMAT_10_11_11                 ; break;
				case tgf_11_11_10                  : desiredGPUFormat = GPUTEXTUREFORMAT_11_11_10                 ; break;
				case tgf_DXT1                      : desiredGPUFormat = GPUTEXTUREFORMAT_DXT1                     ; break;
				case tgf_DXT2_3                    : desiredGPUFormat = GPUTEXTUREFORMAT_DXT2_3                   ; break;
				case tgf_DXT4_5                    : desiredGPUFormat = GPUTEXTUREFORMAT_DXT4_5                   ; break;
				case tgf_16_16_16_16_EDRAM         : desiredGPUFormat = GPUTEXTUREFORMAT_16_16_16_16_EDRAM        ; break;
				case tgf_24_8                      : desiredGPUFormat = GPUTEXTUREFORMAT_24_8                     ; break;
				case tgf_24_8_FLOAT                : desiredGPUFormat = GPUTEXTUREFORMAT_24_8_FLOAT               ; break;
				case tgf_16                        : desiredGPUFormat = GPUTEXTUREFORMAT_16                       ; break;
				case tgf_16_16                     : desiredGPUFormat = GPUTEXTUREFORMAT_16_16                    ; break;
				case tgf_16_16_16_16               : desiredGPUFormat = GPUTEXTUREFORMAT_16_16_16_16              ; break;
				case tgf_16_EXPAND                 : desiredGPUFormat = GPUTEXTUREFORMAT_16_EXPAND                ; break;
				case tgf_16_16_EXPAND              : desiredGPUFormat = GPUTEXTUREFORMAT_16_16_EXPAND             ; break;
				case tgf_16_16_16_16_EXPAND        : desiredGPUFormat = GPUTEXTUREFORMAT_16_16_16_16_EXPAND       ; break;
				case tgf_16_FLOAT                  : desiredGPUFormat = GPUTEXTUREFORMAT_16_FLOAT                 ; break;
				case tgf_16_16_FLOAT               : desiredGPUFormat = GPUTEXTUREFORMAT_16_16_FLOAT              ; break;
				case tgf_16_16_16_16_FLOAT         : desiredGPUFormat = GPUTEXTUREFORMAT_16_16_16_16_FLOAT        ; break;
				case tgf_32                        : desiredGPUFormat = GPUTEXTUREFORMAT_32                       ; break;
				case tgf_32_32                     : desiredGPUFormat = GPUTEXTUREFORMAT_32_32                    ; break;
				case tgf_32_32_32_32               : desiredGPUFormat = GPUTEXTUREFORMAT_32_32_32_32              ; break;
				case tgf_32_FLOAT                  : desiredGPUFormat = GPUTEXTUREFORMAT_32_FLOAT                 ; break;
				case tgf_32_32_FLOAT               : desiredGPUFormat = GPUTEXTUREFORMAT_32_32_FLOAT              ; break;
				case tgf_32_32_32_32_FLOAT         : desiredGPUFormat = GPUTEXTUREFORMAT_32_32_32_32_FLOAT        ; break;
				case tgf_32_AS_8                   : desiredGPUFormat = GPUTEXTUREFORMAT_32_AS_8                  ; break;
				case tgf_32_AS_8_8                 : desiredGPUFormat = GPUTEXTUREFORMAT_32_AS_8_8                ; break;
				case tgf_16_MPEG                   : desiredGPUFormat = GPUTEXTUREFORMAT_16_MPEG                  ; break;
				case tgf_16_16_MPEG                : desiredGPUFormat = GPUTEXTUREFORMAT_16_16_MPEG               ; break;
				case tgf_8_INTERLACED              : desiredGPUFormat = GPUTEXTUREFORMAT_8_INTERLACED             ; break;
				case tgf_32_AS_8_INTERLACED        : desiredGPUFormat = GPUTEXTUREFORMAT_32_AS_8_INTERLACED       ; break;
				case tgf_32_AS_8_8_INTERLACED      : desiredGPUFormat = GPUTEXTUREFORMAT_32_AS_8_8_INTERLACED     ; break;
				case tgf_16_INTERLACED             : desiredGPUFormat = GPUTEXTUREFORMAT_16_INTERLACED            ; break;
				case tgf_16_MPEG_INTERLACED        : desiredGPUFormat = GPUTEXTUREFORMAT_16_MPEG_INTERLACED       ; break;
				case tgf_16_16_MPEG_INTERLACED     : desiredGPUFormat = GPUTEXTUREFORMAT_16_16_MPEG_INTERLACED    ; break;
				case tgf_DXN                       : desiredGPUFormat = GPUTEXTUREFORMAT_DXN                      ; break;
				case tgf_8_8_8_8_AS_16_16_16_16    : desiredGPUFormat = GPUTEXTUREFORMAT_8_8_8_8_AS_16_16_16_16   ; break;
				case tgf_DXT1_AS_16_16_16_16       : desiredGPUFormat = GPUTEXTUREFORMAT_DXT1_AS_16_16_16_16      ; break;
				case tgf_DXT2_3_AS_16_16_16_16     : desiredGPUFormat = GPUTEXTUREFORMAT_DXT2_3_AS_16_16_16_16    ; break;
				case tgf_DXT4_5_AS_16_16_16_16     : desiredGPUFormat = GPUTEXTUREFORMAT_DXT4_5_AS_16_16_16_16    ; break;
				case tgf_2_10_10_10_AS_16_16_16_16 : desiredGPUFormat = GPUTEXTUREFORMAT_2_10_10_10_AS_16_16_16_16; break;
				case tgf_10_11_11_AS_16_16_16_16   : desiredGPUFormat = GPUTEXTUREFORMAT_10_11_11_AS_16_16_16_16  ; break;
				case tgf_11_11_10_AS_16_16_16_16   : desiredGPUFormat = GPUTEXTUREFORMAT_11_11_10_AS_16_16_16_16  ; break;
				case tgf_32_32_32_FLOAT            : desiredGPUFormat = GPUTEXTUREFORMAT_32_32_32_FLOAT           ; break;
				case tgf_DXT3A                     : desiredGPUFormat = GPUTEXTUREFORMAT_DXT3A                    ; break;
				case tgf_DXT5A                     : desiredGPUFormat = GPUTEXTUREFORMAT_DXT5A                    ; break;
				case tgf_CTX1                      : desiredGPUFormat = GPUTEXTUREFORMAT_CTX1                     ; break;
				case tgf_DXT3A_AS_1_1_1_1          : desiredGPUFormat = GPUTEXTUREFORMAT_DXT3A_AS_1_1_1_1         ; break;
				case tgf_8_8_8_8_GAMMA_EDRAM       : desiredGPUFormat = GPUTEXTUREFORMAT_8_8_8_8_GAMMA_EDRAM      ; break;
				case tgf_2_10_10_10_FLOAT_EDRAM    : desiredGPUFormat = GPUTEXTUREFORMAT_2_10_10_10_FLOAT_EDRAM   ; break;
				default                            : break;
				}

				if (GetGPUFormatBitsPerPixel(currentGPUFormat) == GetGPUFormatBitsPerPixel(desiredGPUFormat))
				{
					currentGPUFormat = desiredGPUFormat;
				}
				else
				{
					Displayf("grcTextureControl::UpdateFormat: failed to apply GPU format");
					Displayf("    current format is %d bits per pixel", GetGPUFormatBitsPerPixel(currentGPUFormat));
					Displayf("    desired format is %d bits per pixel", GetGPUFormatBitsPerPixel(desiredGPUFormat));
				}
			}

			if (m_byteSwap != tbs_DEFAULT) // apply byte-swap (endian) setting
			{
				format &= ~D3DFORMAT_ENDIAN_MASK;

				switch (m_byteSwap)
				{
				case tbs_DEFAULT : break;
				case tbs_1_1     : format |= (GPUENDIAN_NONE   << D3DFORMAT_ENDIAN_SHIFT); break;
				case tbs_1_2     : format |= (GPUENDIAN_8IN16  << D3DFORMAT_ENDIAN_SHIFT); break;
				case tbs_1_4     : format |= (GPUENDIAN_8IN32  << D3DFORMAT_ENDIAN_SHIFT); break;
				case tbs_2_4     : format |= (GPUENDIAN_16IN32 << D3DFORMAT_ENDIAN_SHIFT); break;
				default          : break;
				}
			}

			for (int i = 0; i < 4; i++) // x,y,z,w
			{
				if (m_mapping[i] != tcm_DEFAULT)
				{
					const int shift = D3DFORMAT_SIGNX_SHIFT + (D3DFORMAT_SIGNY_SHIFT - D3DFORMAT_SIGNX_SHIFT)*i;
					const u32 mask  = D3DFORMAT_SIGNX_MASK << (D3DFORMAT_SIGNY_SHIFT - D3DFORMAT_SIGNX_SHIFT)*i;

					format &= ~mask;

					switch (m_mapping[i])
					{
					case tcm_DEFAULT  : break;
					case tcm_UNSIGNED : format |= (GPUSIGN_UNSIGNED << shift); break;
					case tcm_SIGNED   : format |= (GPUSIGN_SIGNED   << shift); break;
					case tcm_BIAS     : format |= (GPUSIGN_BIAS     << shift); break;
					case tcm_GAMMA    : format |= (GPUSIGN_GAMMA    << shift); break;
					default           : break;
					}
				}

				if (m_swizzle[i] != tcs_DEFAULT)
				{
					const int shift = D3DFORMAT_SWIZZLEX_SHIFT + (D3DFORMAT_SWIZZLEY_SHIFT - D3DFORMAT_SWIZZLEX_SHIFT)*i;
					const u32 mask  = D3DFORMAT_SWIZZLEX_MASK << (D3DFORMAT_SWIZZLEY_SHIFT - D3DFORMAT_SWIZZLEX_SHIFT)*i;

					format &= ~mask;

					switch (m_swizzle[i])
					{
					case tcs_DEFAULT : break;
					case tcs_R       : format |= (GPUSWIZZLE_X << shift); break;
					case tcs_G       : format |= (GPUSWIZZLE_Y << shift); break;
					case tcs_B       : format |= (GPUSWIZZLE_Z << shift); break;
					case tcs_A       : format |= (GPUSWIZZLE_W << shift); break;
					case tcs_0       : format |= (GPUSWIZZLE_0 << shift); break;
					case tcs_1       : format |= (GPUSWIZZLE_1 << shift); break;
					default          : break;
					}
				}
			}

			if (m_linear == tcf_TRUE)
			{
				format &= ~D3DFORMAT_TILED_MASK;
			}
			else if (m_linear == tcf_FALSE)
			{
				format |= D3DFORMAT_TILED_MASK;
			}

			if (m_expand == tcf_TRUE)
			{
				switch (currentGPUFormat)
				{
				case GPUTEXTUREFORMAT_16_FLOAT          : currentGPUFormat = GPUTEXTUREFORMAT_16_EXPAND                ; break;
				case GPUTEXTUREFORMAT_16_16_FLOAT       : currentGPUFormat = GPUTEXTUREFORMAT_16_16_EXPAND             ; break;
				case GPUTEXTUREFORMAT_16_16_16_16_FLOAT : currentGPUFormat = GPUTEXTUREFORMAT_16_16_16_16_EXPAND       ; break;

				case GPUTEXTUREFORMAT_8_8_8_8           : currentGPUFormat = GPUTEXTUREFORMAT_8_8_8_8_AS_16_16_16_16   ; break;
				case GPUTEXTUREFORMAT_DXT1              : currentGPUFormat = GPUTEXTUREFORMAT_DXT1_AS_16_16_16_16      ; break;
				case GPUTEXTUREFORMAT_DXT2_3            : currentGPUFormat = GPUTEXTUREFORMAT_DXT2_3_AS_16_16_16_16    ; break;
				case GPUTEXTUREFORMAT_DXT4_5            : currentGPUFormat = GPUTEXTUREFORMAT_DXT4_5_AS_16_16_16_16    ; break;
				case GPUTEXTUREFORMAT_2_10_10_10        : currentGPUFormat = GPUTEXTUREFORMAT_2_10_10_10_AS_16_16_16_16; break;
				case GPUTEXTUREFORMAT_10_11_11          : currentGPUFormat = GPUTEXTUREFORMAT_10_11_11_AS_16_16_16_16  ; break;
				case GPUTEXTUREFORMAT_11_11_10          : currentGPUFormat = GPUTEXTUREFORMAT_11_11_10_AS_16_16_16_16  ; break;
				}
			}
			else if (m_expand == tcf_FALSE)
			{
				switch (currentGPUFormat)
				{
				case GPUTEXTUREFORMAT_16_EXPAND                 : currentGPUFormat = GPUTEXTUREFORMAT_16_FLOAT         ; break;
				case GPUTEXTUREFORMAT_16_16_EXPAND              : currentGPUFormat = GPUTEXTUREFORMAT_16_16_FLOAT      ; break;
				case GPUTEXTUREFORMAT_16_16_16_16_EXPAND        : currentGPUFormat = GPUTEXTUREFORMAT_16_16_16_16_FLOAT; break;

				case GPUTEXTUREFORMAT_8_8_8_8_AS_16_16_16_16    : currentGPUFormat = GPUTEXTUREFORMAT_8_8_8_8          ; break;
				case GPUTEXTUREFORMAT_DXT1_AS_16_16_16_16       : currentGPUFormat = GPUTEXTUREFORMAT_DXT1             ; break;
				case GPUTEXTUREFORMAT_DXT2_3_AS_16_16_16_16     : currentGPUFormat = GPUTEXTUREFORMAT_DXT2_3           ; break;
				case GPUTEXTUREFORMAT_DXT4_5_AS_16_16_16_16     : currentGPUFormat = GPUTEXTUREFORMAT_DXT4_5           ; break;
				case GPUTEXTUREFORMAT_2_10_10_10_AS_16_16_16_16 : currentGPUFormat = GPUTEXTUREFORMAT_2_10_10_10       ; break;
				case GPUTEXTUREFORMAT_10_11_11_AS_16_16_16_16   : currentGPUFormat = GPUTEXTUREFORMAT_10_11_11         ; break;
				case GPUTEXTUREFORMAT_11_11_10_AS_16_16_16_16   : currentGPUFormat = GPUTEXTUREFORMAT_11_11_10         ; break;
				}
			}

			if (m_numFormat != tnf_DEFAULT)
			{
				format &= ~D3DFORMAT_NUMFORMAT_MASK;

				switch (m_numFormat)
				{
				case tnf_DEFAULT  : break;
				case tnf_FRACTION : format |= (GPUNUMFORMAT_FRACTION << D3DFORMAT_NUMFORMAT_SHIFT); break;
				case tnf_INTEGER  : format |= (GPUNUMFORMAT_INTEGER  << D3DFORMAT_NUMFORMAT_SHIFT); break;
				default           : break;
				}
			}

			format &= ~D3DFORMAT_TEXTUREFORMAT_MASK;
			format |= (currentGPUFormat << D3DFORMAT_TEXTUREFORMAT_SHIFT);
		}

		return true;
	}

	return false;
}

// ================================================================================================
#elif __PS3

static const char* GetGPUTextureFormatString(u32 x)
{
	switch (x)
	{
	SWITCH_CASE(CELL_GCM_TEXTURE_B8                   ); //= (0x81),
	SWITCH_CASE(CELL_GCM_TEXTURE_A1R5G5B5             ); //= (0x82),
	SWITCH_CASE(CELL_GCM_TEXTURE_A4R4G4B4             ); //= (0x83),
	SWITCH_CASE(CELL_GCM_TEXTURE_R5G6B5               ); //= (0x84),
	SWITCH_CASE(CELL_GCM_TEXTURE_A8R8G8B8             ); //= (0x85),
	SWITCH_CASE(CELL_GCM_TEXTURE_COMPRESSED_DXT1      ); //= (0x86),
	SWITCH_CASE(CELL_GCM_TEXTURE_COMPRESSED_DXT23     ); //= (0x87),
	SWITCH_CASE(CELL_GCM_TEXTURE_COMPRESSED_DXT45     ); //= (0x88),
	SWITCH_CASE(CELL_GCM_TEXTURE_G8B8                 ); //= (0x8B),
	SWITCH_CASE(CELL_GCM_TEXTURE_R6G5B5               ); //= (0x8F),
	SWITCH_CASE(CELL_GCM_TEXTURE_DEPTH24_D8           ); //= (0x90),
	SWITCH_CASE(CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT     ); //= (0x91),
	SWITCH_CASE(CELL_GCM_TEXTURE_DEPTH16              ); //= (0x92),
	SWITCH_CASE(CELL_GCM_TEXTURE_DEPTH16_FLOAT        ); //= (0x93),
	SWITCH_CASE(CELL_GCM_TEXTURE_X16                  ); //= (0x94),
	SWITCH_CASE(CELL_GCM_TEXTURE_Y16_X16              ); //= (0x95),
	SWITCH_CASE(CELL_GCM_TEXTURE_R5G5B5A1             ); //= (0x97),
	SWITCH_CASE(CELL_GCM_TEXTURE_COMPRESSED_HILO8     ); //= (0x98),
	SWITCH_CASE(CELL_GCM_TEXTURE_COMPRESSED_HILO_S8   ); //= (0x99),
	SWITCH_CASE(CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT); //= (0x9A),
	SWITCH_CASE(CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT); //= (0x9B),
	SWITCH_CASE(CELL_GCM_TEXTURE_X32_FLOAT            ); //= (0x9C),
	SWITCH_CASE(CELL_GCM_TEXTURE_D1R5G5B5             ); //= (0x9D),
	SWITCH_CASE(CELL_GCM_TEXTURE_D8R8G8B8             ); //= (0x9E),
	SWITCH_CASE(CELL_GCM_TEXTURE_Y16_X16_FLOAT        ); //= (0x9F),
	SWITCH_CASE(CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8 ); //= (0xAD),
	SWITCH_CASE(CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8 ); //= (0xAE),
	}

	return "CELL_GCM_TEXTURE_UNKNOWN";
}

static const char* GetGPUTextureRemapString(u32 x)
{
	switch (x)
	{
	SWITCH_CASE(CELL_GCM_TEXTURE_REMAP_R);
	SWITCH_CASE(CELL_GCM_TEXTURE_REMAP_G);
	SWITCH_CASE(CELL_GCM_TEXTURE_REMAP_B);
	SWITCH_CASE(CELL_GCM_TEXTURE_REMAP_A);
	SWITCH_CASE(CELL_GCM_TEXTURE_REMAP_0);
	SWITCH_CASE(CELL_GCM_TEXTURE_REMAP_1);
	}

	return "CELL_GCM_TEXTURE_REMAP_UNKNOWN";
}

static int GetGPUFormatBitsPerPixel(u32 GPUFormat)
{
	switch (GPUFormat)
	{
	case CELL_GCM_TEXTURE_B8                    : return   8;
	case CELL_GCM_TEXTURE_A1R5G5B5              : return  16;
	case CELL_GCM_TEXTURE_A4R4G4B4              : return  16;
	case CELL_GCM_TEXTURE_R5G6B5                : return  16;
	case CELL_GCM_TEXTURE_A8R8G8B8              : return  32;
	case CELL_GCM_TEXTURE_COMPRESSED_DXT1       : return   4;
	case CELL_GCM_TEXTURE_COMPRESSED_DXT23      : return   8;
	case CELL_GCM_TEXTURE_COMPRESSED_DXT45      : return   8;
	case CELL_GCM_TEXTURE_G8B8                  : return  16;
	case CELL_GCM_TEXTURE_R6G5B5                : return  16;
	case CELL_GCM_TEXTURE_DEPTH24_D8            : return  32;
	case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT      : return  32;
	case CELL_GCM_TEXTURE_DEPTH16               : return  16;
	case CELL_GCM_TEXTURE_DEPTH16_FLOAT         : return  16;
	case CELL_GCM_TEXTURE_X16                   : return  16;
	case CELL_GCM_TEXTURE_Y16_X16               : return  32;
	case CELL_GCM_TEXTURE_R5G5B5A1              : return  16;
	case CELL_GCM_TEXTURE_COMPRESSED_HILO8      : return  16; // ?
	case CELL_GCM_TEXTURE_COMPRESSED_HILO_S8    : return  16; // ?
	case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT : return  64;
	case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT : return 128;
	case CELL_GCM_TEXTURE_X32_FLOAT             : return  32;
	case CELL_GCM_TEXTURE_D1R5G5B5              : return  16;
	case CELL_GCM_TEXTURE_D8R8G8B8              : return  32;
	case CELL_GCM_TEXTURE_Y16_X16_FLOAT         : return  32;
	case CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8  : return  16; // 32 bits / 2x1 pixels
	case CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8  : return  16; // 32 bits / 2x1 pixels
	}

	return 0;
}

void grcTextureControl::DisplayFormat(grcImage* image, u32 format, u32 remap)
{
	DisplayFormat(image);

	Displayf("GCM format [0x%08x], remap [0x%08x]:", format, remap);
	Displayf("    %s", GetGPUTextureFormatString(gcm::StripTextureFormat(format)));
	Displayf("    LINEAR = %s", (format & CELL_GCM_TEXTURE_LINEAR_MASK) == CELL_GCM_TEXTURE_LN ? "TRUE" : "FALSE");
	Displayf("  x:%s", GetGPUTextureRemapString((remap >> 6)&CELL_GCM_TEXTURE_REMAP_MASK));
	Displayf("  y:%s", GetGPUTextureRemapString((remap >> 4)&CELL_GCM_TEXTURE_REMAP_MASK));
	Displayf("  z:%s", GetGPUTextureRemapString((remap >> 2)&CELL_GCM_TEXTURE_REMAP_MASK));
	Displayf("  w:%s", GetGPUTextureRemapString((remap >> 0)&CELL_GCM_TEXTURE_REMAP_MASK));
}

bool grcTextureControl::UpdateFormat(grcImage* image, u32& format, u32& remap)
{
	if (m_enabled)
	{
		PreInitImage(image);

		u32 currentGPUFormat = format & 0x9F;

		if ((format & 0xBF) == CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8 ||
			(format & 0xBF) == CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8)
		{
			currentGPUFormat = (format & 0xBF);
		}

		if (m_update)
		{
			// these are not supported ..
			{
				m_byteSwap   = tbs_DEFAULT;
				m_expand     = tcf_DEFAULT;
				m_numFormat  = tnf_DEFAULT;
				m_mapping[0] = tcm_DEFAULT;
				m_mapping[1] = tcm_DEFAULT;
				m_mapping[2] = tcm_DEFAULT;
				m_mapping[3] = tcm_DEFAULT;
			}

			m_remapOrderXXXY = false;

			switch (currentGPUFormat) // consider 'gcm::StripTextureFormat(format)'
			{
			case CELL_GCM_TEXTURE_B8                    : m_GPUFormat = tgf_B8                   ; break;
			case CELL_GCM_TEXTURE_A1R5G5B5              : m_GPUFormat = tgf_A1R5G5B5             ; break;
			case CELL_GCM_TEXTURE_A4R4G4B4              : m_GPUFormat = tgf_A4R4G4B4             ; break;
			case CELL_GCM_TEXTURE_R5G6B5                : m_GPUFormat = tgf_R5G6B5               ; break;
			case CELL_GCM_TEXTURE_A8R8G8B8              : m_GPUFormat = tgf_A8R8G8B8             ; break;
			case CELL_GCM_TEXTURE_COMPRESSED_DXT1       : m_GPUFormat = tgf_COMPRESSED_DXT1      ; break;
			case CELL_GCM_TEXTURE_COMPRESSED_DXT23      : m_GPUFormat = tgf_COMPRESSED_DXT23     ; break;
			case CELL_GCM_TEXTURE_COMPRESSED_DXT45      : m_GPUFormat = tgf_COMPRESSED_DXT45     ; break;
			case CELL_GCM_TEXTURE_G8B8                  : m_GPUFormat = tgf_G8B8                 ; break;
			case CELL_GCM_TEXTURE_R6G5B5                : m_GPUFormat = tgf_R6G5B5               ; break;
			case CELL_GCM_TEXTURE_DEPTH24_D8            : m_GPUFormat = tgf_DEPTH24_D8           ; break;
			case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT      : m_GPUFormat = tgf_DEPTH24_D8_FLOAT     ; break;
			case CELL_GCM_TEXTURE_DEPTH16               : m_GPUFormat = tgf_DEPTH16              ; break;
			case CELL_GCM_TEXTURE_DEPTH16_FLOAT         : m_GPUFormat = tgf_DEPTH16_FLOAT        ; break;
			case CELL_GCM_TEXTURE_X16                   : m_GPUFormat = tgf_X16                  ; break;
			case CELL_GCM_TEXTURE_Y16_X16               : m_GPUFormat = tgf_Y16_X16              ; break;
			case CELL_GCM_TEXTURE_R5G5B5A1              : m_GPUFormat = tgf_R5G5B5A1             ; break;
			case CELL_GCM_TEXTURE_COMPRESSED_HILO8      : m_GPUFormat = tgf_COMPRESSED_HILO8     ; break;
			case CELL_GCM_TEXTURE_COMPRESSED_HILO_S8    : m_GPUFormat = tgf_COMPRESSED_HILO_S8   ; break;
			case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT : m_GPUFormat = tgf_W16_Z16_Y16_X16_FLOAT; break;
			case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT : m_GPUFormat = tgf_W32_Z32_Y32_X32_FLOAT; break;
			case CELL_GCM_TEXTURE_X32_FLOAT             : m_GPUFormat = tgf_X32_FLOAT            ; break;
			case CELL_GCM_TEXTURE_D1R5G5B5              : m_GPUFormat = tgf_D1R5G5B5             ; break;
			case CELL_GCM_TEXTURE_D8R8G8B8              : m_GPUFormat = tgf_D8R8G8B8             ; break;
			case CELL_GCM_TEXTURE_Y16_X16_FLOAT         : m_GPUFormat = tgf_Y16_X16_FLOAT        ; break;
			case CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8  : m_GPUFormat = tgf_COMPRESSED_B8R8_G8R8 ; break;
			case CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8  : m_GPUFormat = tgf_COMPRESSED_R8B8_R8G8 ; break;
			}

			for (int i = 0; i < 4; i++) // x,y,z,w
			{
				const int shift = 2*(3 - i); // 6,4,2,0
				const u32 mask  = CELL_GCM_TEXTURE_REMAP_MASK << shift;

				switch ((remap & mask) >> shift)
				{
				case CELL_GCM_TEXTURE_REMAP_R : m_swizzle[i] = tcs_R      ; break;
				case CELL_GCM_TEXTURE_REMAP_G : m_swizzle[i] = tcs_G      ; break;
				case CELL_GCM_TEXTURE_REMAP_B : m_swizzle[i] = tcs_B      ; break;
				case CELL_GCM_TEXTURE_REMAP_A : m_swizzle[i] = tcs_A      ; break;
				case CELL_GCM_TEXTURE_REMAP_0 : m_swizzle[i] = tcs_0      ; break;
				case CELL_GCM_TEXTURE_REMAP_1 : m_swizzle[i] = tcs_1      ; break;
				default                       : m_swizzle[i] = tcs_DEFAULT; break;
				}
			}

			switch (format & CELL_GCM_TEXTURE_LINEAR_MASK)
			{
			case CELL_GCM_TEXTURE_SZ : m_linear = tcf_FALSE  ; break;
			case CELL_GCM_TEXTURE_LN : m_linear = tcf_TRUE   ; break;
			default                  : m_linear = tcf_DEFAULT; break;
			}
		}
		else // apply
		{
			if (m_GPUFormat != tgf_DEFAULT)
			{
				u32 desiredGPUFormat = 0;

				switch (m_GPUFormat)
				{
				case tgf_DEFAULT               : break;
				case tgf_B8                    : desiredGPUFormat = CELL_GCM_TEXTURE_B8                   ; break;
				case tgf_A1R5G5B5              : desiredGPUFormat = CELL_GCM_TEXTURE_A1R5G5B5             ; break;
				case tgf_A4R4G4B4              : desiredGPUFormat = CELL_GCM_TEXTURE_A4R4G4B4             ; break;
				case tgf_R5G6B5                : desiredGPUFormat = CELL_GCM_TEXTURE_R5G6B5               ; break;
				case tgf_A8R8G8B8              : desiredGPUFormat = CELL_GCM_TEXTURE_A8R8G8B8             ; break;
				case tgf_COMPRESSED_DXT1       : desiredGPUFormat = CELL_GCM_TEXTURE_COMPRESSED_DXT1      ; break;
				case tgf_COMPRESSED_DXT23      : desiredGPUFormat = CELL_GCM_TEXTURE_COMPRESSED_DXT23     ; break;
				case tgf_COMPRESSED_DXT45      : desiredGPUFormat = CELL_GCM_TEXTURE_COMPRESSED_DXT45     ; break;
				case tgf_G8B8                  : desiredGPUFormat = CELL_GCM_TEXTURE_G8B8                 ; break;
				case tgf_R6G5B5                : desiredGPUFormat = CELL_GCM_TEXTURE_R6G5B5               ; break;
				case tgf_DEPTH24_D8            : desiredGPUFormat = CELL_GCM_TEXTURE_DEPTH24_D8           ; break;
				case tgf_DEPTH24_D8_FLOAT      : desiredGPUFormat = CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT     ; break;
				case tgf_DEPTH16               : desiredGPUFormat = CELL_GCM_TEXTURE_DEPTH16              ; break;
				case tgf_DEPTH16_FLOAT         : desiredGPUFormat = CELL_GCM_TEXTURE_DEPTH16_FLOAT        ; break;
				case tgf_X16                   : desiredGPUFormat = CELL_GCM_TEXTURE_X16                  ; break;
				case tgf_Y16_X16               : desiredGPUFormat = CELL_GCM_TEXTURE_Y16_X16              ; break;
				case tgf_R5G5B5A1              : desiredGPUFormat = CELL_GCM_TEXTURE_R5G5B5A1             ; break;
				case tgf_COMPRESSED_HILO8      : desiredGPUFormat = CELL_GCM_TEXTURE_COMPRESSED_HILO8     ; break;
				case tgf_COMPRESSED_HILO_S8    : desiredGPUFormat = CELL_GCM_TEXTURE_COMPRESSED_HILO_S8   ; break;
				case tgf_W16_Z16_Y16_X16_FLOAT : desiredGPUFormat = CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT; break;
				case tgf_W32_Z32_Y32_X32_FLOAT : desiredGPUFormat = CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT; break;
				case tgf_X32_FLOAT             : desiredGPUFormat = CELL_GCM_TEXTURE_X32_FLOAT            ; break;
				case tgf_D1R5G5B5              : desiredGPUFormat = CELL_GCM_TEXTURE_D1R5G5B5             ; break;
				case tgf_D8R8G8B8              : desiredGPUFormat = CELL_GCM_TEXTURE_D8R8G8B8             ; break;
				case tgf_Y16_X16_FLOAT         : desiredGPUFormat = CELL_GCM_TEXTURE_Y16_X16_FLOAT        ; break;
				case tgf_COMPRESSED_B8R8_G8R8  : desiredGPUFormat = CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8 ; break;
				case tgf_COMPRESSED_R8B8_R8G8  : desiredGPUFormat = CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8 ; break;
				default                        : break;
				}

				if (GetGPUFormatBitsPerPixel(currentGPUFormat) == GetGPUFormatBitsPerPixel(desiredGPUFormat))
				{
					currentGPUFormat = desiredGPUFormat;
				}
				else
				{
					Displayf("grcTextureControl::UpdateFormat: failed to apply GPU format");
					Displayf("    current format is %d bits per pixel", GetGPUFormatBitsPerPixel(currentGPUFormat));
					Displayf("    desired format is %d bits per pixel", GetGPUFormatBitsPerPixel(desiredGPUFormat));
				}
			}

			bool bSupportsRemapBGRA = true;
			bool bSupportsRemapXXXY = false;

			if (currentGPUFormat == CELL_GCM_TEXTURE_COMPRESSED_HILO8   ||
				currentGPUFormat == CELL_GCM_TEXTURE_COMPRESSED_HILO_S8 ||
				currentGPUFormat == CELL_GCM_TEXTURE_X16                ||
				currentGPUFormat == CELL_GCM_TEXTURE_Y16_X16            ||
				currentGPUFormat == CELL_GCM_TEXTURE_Y16_X16_FLOAT)
			{
				bSupportsRemapBGRA = false;
				bSupportsRemapXXXY = true;
			}

			for (int i = 0; i < 4; i++) // x,y,z,w
			{
				if (m_swizzle[i] != tcs_DEFAULT)
				{
					if (bSupportsRemapBGRA)
					{
						const int shift = 2*(3 - i); // 6,4,2,0
						const u32 mask  = CELL_GCM_TEXTURE_REMAP_MASK << shift;

						remap &= ~mask;

						switch (m_swizzle[i])
						{
						case tcs_DEFAULT : break;
						case tcs_R       : remap |= (CELL_GCM_TEXTURE_REMAP_R << shift); break;
						case tcs_G       : remap |= (CELL_GCM_TEXTURE_REMAP_G << shift); break;
						case tcs_B       : remap |= (CELL_GCM_TEXTURE_REMAP_B << shift); break;
						case tcs_A       : remap |= (CELL_GCM_TEXTURE_REMAP_A << shift); break;
						case tcs_0       : remap |= (CELL_GCM_TEXTURE_REMAP_0 << shift); break;
						case tcs_1       : remap |= (CELL_GCM_TEXTURE_REMAP_1 << shift); break;
						default          : break;
						}
					}
					else
					{
						Displayf("grcTextureControl::UpdateFormat: m_swizzle not compatible with GPU format");
					}
				}
			}

			if (m_remapOrderXXXY)
			{
				if (bSupportsRemapXXXY)
				{
					remap |= (CELL_GCM_TEXTURE_REMAP_ORDER_XXXY << 16);
				}
				else
				{
					Displayf("grcTextureControl::UpdateFormat: m_remapOrderXXXY not compatible with GPU format");
				}
			}

			if (currentGPUFormat != CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8 &&
				currentGPUFormat != CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8)
			{
				if (m_linear != tcf_DEFAULT)
				{
					currentGPUFormat &= ~CELL_GCM_TEXTURE_LINEAR_MASK;

					switch (m_linear)
					{
					case tcf_DEFAULT : break;
					case tcf_FALSE   : format |= CELL_GCM_TEXTURE_SZ; break;
					case tcf_TRUE    : format |= CELL_GCM_TEXTURE_LN; break;
					default          : break;
					}
				}
			}
			else if (m_linear != tcf_DEFAULT)
			{
				Displayf("grcTextureControl::UpdateFormat: 2x1 block formats must have m_linear = DEFAULT");
			}

			format &= ~0x9F;
			format |= currentGPUFormat;
		}

		return true;
	}

	return false;
}

#endif // __PS3

} // namespace rage

#endif // __BANK_TEXTURE_CONTROL
