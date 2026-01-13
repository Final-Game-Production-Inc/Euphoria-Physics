// 
// grcore/texturecontrol.h
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef GRCORE_TEXTURECONTROL_H
#define GRCORE_TEXTURECONTROL_H

#define __BANK_TEXTURE_CONTROL_ENABLE (1)
#define __BANK_TEXTURE_CONTROL (__BANK_TEXTURE_CONTROL_ENABLE && __BANK && __DEV && __CONSOLE && !__RESOURCECOMPILER)

#if __BANK_TEXTURE_CONTROL

namespace rage {

class bkBank;
class grcImage;

class grcTextureControl
{
public:
	enum eTextureGPUFormat
	{
		tgf_DEFAULT = 0,
#if __XENON
		tgf_1_REVERSE                ,
		tgf_1                        ,
		tgf_8                        ,
		tgf_1_5_5_5                  ,
		tgf_5_6_5                    ,
		tgf_6_5_5                    ,
		tgf_8_8_8_8                  ,
		tgf_2_10_10_10               ,
		tgf_8_A                      ,
		tgf_8_B                      ,
		tgf_8_8                      ,
		tgf_Cr_Y1_Cb_Y0_REP          ,
		tgf_Y1_Cr_Y0_Cb_REP          ,
		tgf_16_16_EDRAM              ,
		tgf_8_8_8_8_A                ,
		tgf_4_4_4_4                  ,
		tgf_10_11_11                 ,
		tgf_11_11_10                 ,
		tgf_DXT1                     ,
		tgf_DXT2_3                   ,
		tgf_DXT4_5                   ,
		tgf_16_16_16_16_EDRAM        ,
		tgf_24_8                     ,
		tgf_24_8_FLOAT               ,
		tgf_16                       ,
		tgf_16_16                    ,
		tgf_16_16_16_16              ,
		tgf_16_EXPAND                ,
		tgf_16_16_EXPAND             ,
		tgf_16_16_16_16_EXPAND       ,
		tgf_16_FLOAT                 ,
		tgf_16_16_FLOAT              ,
		tgf_16_16_16_16_FLOAT        ,
		tgf_32                       ,
		tgf_32_32                    ,
		tgf_32_32_32_32              ,
		tgf_32_FLOAT                 ,
		tgf_32_32_FLOAT              ,
		tgf_32_32_32_32_FLOAT        ,
		tgf_32_AS_8                  ,
		tgf_32_AS_8_8                ,
		tgf_16_MPEG                  ,
		tgf_16_16_MPEG               ,
		tgf_8_INTERLACED             ,
		tgf_32_AS_8_INTERLACED       ,
		tgf_32_AS_8_8_INTERLACED     ,
		tgf_16_INTERLACED            ,
		tgf_16_MPEG_INTERLACED       ,
		tgf_16_16_MPEG_INTERLACED    ,
		tgf_DXN                      ,
		tgf_8_8_8_8_AS_16_16_16_16   ,
		tgf_DXT1_AS_16_16_16_16      ,
		tgf_DXT2_3_AS_16_16_16_16    ,
		tgf_DXT4_5_AS_16_16_16_16    ,
		tgf_2_10_10_10_AS_16_16_16_16,
		tgf_10_11_11_AS_16_16_16_16  ,
		tgf_11_11_10_AS_16_16_16_16  ,
		tgf_32_32_32_FLOAT           ,
		tgf_DXT3A                    ,
		tgf_DXT5A                    ,
		tgf_CTX1                     ,
		tgf_DXT3A_AS_1_1_1_1         ,
		tgf_8_8_8_8_GAMMA_EDRAM      ,
		tgf_2_10_10_10_FLOAT_EDRAM   ,
#elif __PS3
		tgf_B8                       ,
		tgf_A1R5G5B5                 ,
		tgf_A4R4G4B4                 ,
		tgf_R5G6B5                   ,
		tgf_A8R8G8B8                 ,
		tgf_COMPRESSED_DXT1          ,
		tgf_COMPRESSED_DXT23         ,
		tgf_COMPRESSED_DXT45         ,
		tgf_G8B8                     ,
		tgf_R6G5B5                   ,
		tgf_DEPTH24_D8               ,
		tgf_DEPTH24_D8_FLOAT         ,
		tgf_DEPTH16                  ,
		tgf_DEPTH16_FLOAT            ,
		tgf_X16                      ,
		tgf_Y16_X16                  ,
		tgf_R5G5B5A1                 ,
		tgf_COMPRESSED_HILO8         ,
		tgf_COMPRESSED_HILO_S8       ,
		tgf_W16_Z16_Y16_X16_FLOAT    ,
		tgf_W32_Z32_Y32_X32_FLOAT    ,
		tgf_X32_FLOAT                ,
		tgf_D1R5G5B5                 ,
		tgf_D8R8G8B8                 ,
		tgf_Y16_X16_FLOAT            ,
		tgf_COMPRESSED_B8R8_G8R8     ,
		tgf_COMPRESSED_R8B8_R8G8     ,
#else
		// ..
#endif
		tgf_COUNT,
		tgf_FORCE32 = 0x7fffffff
	};

	enum eTextureByteSwap
	{
		tbs_DEFAULT = 0,
		tbs_1_1, // force no byte swap
		tbs_1_2, // force 1-in-2 byte swap
		tbs_1_4, // force 1-in-4 byte swap
		tbs_2_4, // force 2-in-4 byte swap
		tbs_COUNT,
		tbs_FORCE32 = 0x7fffffff
	};

	enum eTextureControlFlag
	{
		tcf_DEFAULT = 0,
		tcf_FALSE,
		tcf_TRUE,
		tcf_COUNT,
		tcf_FORCE32 = 0x7fffffff
	};

	enum eTextureNumFormat
	{
		tnf_DEFAULT = 0,
		tnf_FRACTION,
		tnf_INTEGER,
		tnf_COUNT,
		tnf_FORCE32 = 0x7fffffff
	};

	enum eTextureComponentMapping
	{
		tcm_DEFAULT = 0,
		tcm_UNSIGNED,
		tcm_SIGNED,
		tcm_BIAS,
		tcm_GAMMA,
		tcm_COUNT,
		tcm_FORCE32 = 0x7fffffff
	};

	enum eTextureComponentSwizzle
	{
		tcs_DEFAULT = 0,
		tcs_R,
		tcs_G,
		tcs_B,
		tcs_A,
		tcs_0,
		tcs_1,
		tcs_COUNT,
		tcs_FORCE32 = 0x7fffffff
	};

	enum eTexturePreInitByteSwap
	{
		pbs_NONE, // 1-1
		pbs_1_2,
		pbs_1_4,
		pbs_2_4,
		pbs_2_8,
		pbs_1_2_4, // swap 1-in-2 bytes every 4 bytes
		pbs_1_2_8, // swap 1-in-2 bytes every 8 bytes
		pbs_1_4_8, // swap 1-in-4 bytes every 8 bytes
		pbs_2_4_8, // swap 2-in-4 bytes every 8 bytes
		pbs_COUNT,
		pbs_FORCE32 = 0x7fffffff
	};

	enum eTexturePreInitDataSize
	{
		pds_NONE = 0,
		pds_1, // 1 byte
		pds_2, // 2 bytes
		pds_4, // 4 bytes
		pds_COUNT,
		pds_FORCE32 = 0x7fffffff
	};

	bool                     m_enabled;
	bool                     m_update; // update fields from incoming texture's GPU format, instead of applying fields
	eTextureGPUFormat        m_GPUFormat; // internal format (can be interchanged, as long as it's the same bits per pixel)
	eTextureByteSwap         m_byteSwap;
	eTextureControlFlag      m_linear;
	eTextureControlFlag      m_expand; // use _EXPAND/_AS_16_16_16_16 formats on XENON
	eTextureNumFormat        m_numFormat;
	eTextureComponentMapping m_mapping[4]; // x,y,z,w
	eTextureComponentSwizzle m_swizzle[4]; // x,y,z,w
	bool                     m_remapOrderXXXY;  // flag indicating CELL_GCM_TEXTURE_REMAP_ORDER_XXXY (as opposed to XYXY) should be used on PS3
	eTexturePreInitByteSwap  m_preInitByteSwap; // software byte-swap applied to image pixel data
	eTexturePreInitDataSize  m_preInitDataSize;
	u32                      m_preInitData;
	u32                      m_preInitDataStart;
	u32                      m_preInitDataStep;

	grcTextureControl();

	static const char** GetUIStrings_eTextureGPUFormat       ();
	static const char** GetUIStrings_eTextureByteSwap        ();
	static const char** GetUIStrings_eTextureComponentSwizzle();
	static const char** GetUIStrings_eTextureComponentMapping();
	static const char** GetUIStrings_eTextureControlFlag     ();
	static const char** GetUIStrings_eTextureNumFormat       ();
	static const char** GetUIStrings_eTexturePreInitByteSwap ();
	static const char** GetUIStrings_eTexturePreInitDataSize ();

	void AddWidgets(bkBank& bk); // this just adds the "Create" button
	void CreateWidgets(bkBank& bk);
	static void Reset();

	void PreInitImage (grcImage* image) const;
	void DisplayFormat(grcImage* image);
	void DisplayFormat(grcImage* image, u32  format, u32  remap);
	bool UpdateFormat (grcImage* image, u32& format, u32& remap);
};

extern grcTextureControl gTextureControl;

} // namespace rage

#endif // __BANK_TEXTURE_CONTROL
#endif // GRCORE_TEXTURECONTROL_H
