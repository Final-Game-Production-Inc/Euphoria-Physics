// 
// data/aes.h 
// 
// Copyright (C) 1999-2008 Rockstar Games.  All Rights Reserved. 
// 
#ifndef DATA_AES_H
#define DATA_AES_H

#include "file/file_config.h"

namespace rage {

#if ((RSG_CPU_X86 || RSG_CPU_X64) && RSG_PC) && !__RGSC_DLL && !__FINAL
#define TRANSFORMIT		1
#endif

#define AES_KEY_ID_MASK		0x0FFFFFFF
#define AES_KEY_ID_GTA4		0x0FFFFFFF
#define AES_KEY_ID_MC4		0x0FFFFFFE
#define AES_KEY_ID_RDR2		0x0FFFFFFD
#define AES_KEY_ID_JIMMY	0x0FFFFFFC
#define AES_KEY_ID_MP3		0x0FFFFFFB
#define AES_KEY_ID_BULLY2	0x0FFFFFFA
#define AES_KEY_ID_GTA5_PC	0x0FFFFFF9
#define AES_KEY_ID_GTA5_PS3	0x0FFFFFF8
#define AES_KEY_ID_GTA5_360	0x0FFFFFF7

#if __RGSC_DLL
	#define AES_KEY_ID_RGSC		0x0FFFFFF6
#endif

#define AES_KEY_ID_GTA5SAVE_PC	0x0FFFFFF5
#define AES_KEY_ID_GTA5SAVE_PS4			0x0FFFFFF4
#define AES_KEY_ID_GTA5SAVE_XBOXONE		0x0FFFFFF3
#define AES_KEY_ID_GTA5SAVE_PS3	0x0FFFFFF4
#define AES_KEY_ID_GTA5SAVE_360	0x0FFFFFF3
#define AES_KEY_ID_FROM_CLOUD 0x0FFFFFF2
#define AES_KEY_ID_GTA5SAVE_MIGRATION 0x0FFFFFF1
#define AES_KEY_ID_RAGE		0x0FFFFF00

#if	RSG_CPU_X64 || RSG_CPU_X86
	#define AES_KEY_ID_WBC_GTA5_PC			0x0FEFFFFF
	#define AES_MULTIKEY_ID_GTA5_PS4		0x0FFEFFFF
	#define AES_MULTIKEY_ID_GTA5_XBOXONE	0x0FFFEFFF
	#define	TFIT_NUM_KEYS					0x65
#endif


#if HACK_MC4
#define AES_KEY_ID_DEFAULT	AES_KEY_ID_MC4
#elif HACK_RDR2
#define AES_KEY_ID_DEFAULT	AES_KEY_ID_RDR2
#elif HACK_MP3
#define AES_KEY_ID_DEFAULT	AES_KEY_ID_MP3
#elif HACK_BULLY2
#define AES_KEY_ID_DEFAULT	AES_KEY_ID_BULLY2
#elif HACK_JIMMY
#define AES_KEY_ID_DEFAULT	AES_KEY_ID_JIMMY
#elif __RGSC_DLL
#define AES_KEY_ID_DEFAULT	AES_KEY_ID_RGSC
#elif HACK_GTA4
	#if RSG_PC
		#define AES_KEY_ID_DEFAULT	AES_KEY_ID_WBC_GTA5_PC
	#elif RSG_ORBIS
		#define AES_KEY_ID_DEFAULT	AES_MULTIKEY_ID_GTA5_PS4
	#elif RSG_DURANGO
		#define AES_KEY_ID_DEFAULT	AES_MULTIKEY_ID_GTA5_XBOXONE
	#elif __PS3
		#define AES_KEY_ID_DEFAULT	AES_KEY_ID_GTA5_PS3
	#elif __XENON
		#define AES_KEY_ID_DEFAULT	AES_KEY_ID_GTA5_360
	#endif
#else
	#define AES_KEY_ID_DEFAULT	AES_KEY_ID_RAGE
#endif


#if __WIN32PC
	#define AES_KEY_ID_SAVEGAME	AES_KEY_ID_GTA5SAVE_PC
#elif RSG_ORBIS
	#define AES_KEY_ID_SAVEGAME	AES_KEY_ID_GTA5SAVE_PS4
#elif RSG_DURANGO
	#define AES_KEY_ID_SAVEGAME	AES_KEY_ID_GTA5SAVE_XBOXONE
#elif __PS3
	#define AES_KEY_ID_SAVEGAME	AES_KEY_ID_GTA5SAVE_PS3
#elif __XENON
	#define AES_KEY_ID_SAVEGAME	AES_KEY_ID_GTA5SAVE_360
#endif

typedef unsigned long ulong32;

#define INTEL_AES_INSTRUCTIONS	(__WIN32PC && !__RGSC_DLL && !__TOOL)

typedef union Symmetric_key {
	struct _rijndael {
		ALIGNAS(16) ulong32 eK[60], dK[60];
		int Nr;
		int extra_rounds;
	} rijndael;
} symmetric_key;

class AES
{
public:
	// PURPOSE: Init the encoder with a particular AES_KEY_ID_... (or 0 for the default for your project)
	AES(unsigned = 0);

	// PURPOSE: Init the encoder with a specific key sequence (mostly to avoid breaking rdr2 dongles)
	AES(const unsigned char*);

	~AES();

	// PURPOSE: Overload the Encrypt method with a KeyID to select the correct encrypting function
	static bool Encrypt(unsigned int keyId, unsigned int selector, void *data, unsigned int size);

	// PURPOSE: Overload the Decrypt method with a KeyID to select the correct encrypting function
	static bool Decrypt(unsigned int keyId, unsigned int selector,  void *data, unsigned int size);

	// PURPOSE:	Encrypt data in-place.  Size must be multiple of 16.  If it isn't, the remainder MODULO 16 bytes of data are ignored.
	bool Encrypt(void *data, unsigned int size);

	// PURPOSE:	Decrypt data in-place.  Size must be multiple of 16.  If it isn't, the remainder MODULO 16 bytes of data are ignored.
	bool Decrypt(void *data, unsigned int size);

	// PURPOSE: Retrieve the key ID the encoder was configured for.
	unsigned GetKeyId() { return key_id; }

	//PURPOSE: Setup cloud key 
	static void CreateCloudAes(const unsigned char* data);
	static void ReleaseCloudAes() { delete ms_cloudAes; }
	static AES* GetCloudAes() { return ms_cloudAes; }

	//PURPOSE: Explicit TransformIT decryption
	#if RSG_PC && (RSG_CPU_X86 || RSG_CPU_X64 || __RESOURCECOMPILER || RSG_TOOL) && !__RGSC_DLL
		static bool isTransformITKeyId(unsigned int keyId);
		static bool TransformITDecrypt(unsigned int selector, void *data, unsigned int size);
	#else
		static bool isTransformITKeyId(unsigned int) { return false; }
		static bool TransformITDecrypt(unsigned int, void *, unsigned int) { return false; }
	#endif

protected:
	symmetric_key key;
	unsigned key_id;

    static AES* ms_cloudAes;
};

}	// namespace rage


#endif