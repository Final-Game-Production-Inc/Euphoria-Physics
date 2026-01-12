#include "datprotect.h"
#include "sha1.h"
#include "pbkdf2.h"
#include "aes.h"
#include "math/random.h"
#include "system/timer.h"
#include "system/memops.h"
#include <time.h>

namespace rage {

void
datGetDigest(const u8* dataToProtect,
			 const u32 sizeOfDataToProtect,
			 u8 (&digest)[Sha1::SHA1_DIGEST_LENGTH],
			 const datProtectContext &context)
{
	// make passphrase from private data (SHA-1)
	Sha1 genPassphrase;

	if(context.m_PrivateData && context.m_PrivateDataSize)
	{
		genPassphrase.Update(context.m_PrivateData, context.m_PrivateDataSize);
	}

	genPassphrase.Update((const u8*)&sizeOfDataToProtect, sizeof(sizeOfDataToProtect));

	u8 passphrase[Sha1::SHA1_DIGEST_LENGTH];
	genPassphrase.Final(passphrase);

	// cryptographically hash the plaintext data with the passphrase as the key (HMAC-SHA1)
	Sha1 hashPlaintext(passphrase, sizeof(passphrase));
	hashPlaintext.Update(dataToProtect, sizeOfDataToProtect);
	hashPlaintext.Final(digest);
}

void
datSign(const u8* dataToProtect,
		const u32 sizeOfDataToProtect,
		const datProtectContext &context,
		u8 (&signature)[datProtectSignatureSize])
{
	u8 digest[Sha1::SHA1_DIGEST_LENGTH];
	datGetDigest(dataToProtect, sizeOfDataToProtect, digest, context);

	// derive an aes key from the digest (PBKDF2)
	// since the digest is only 20 bytes, and AES works on blocks of 16 bytes
	// we'll add 32-20 = 12 bytes of salt data to the signature
	mthRandom r(sysTimer::GetSystemMsTime());
	u32 random_number = r.GetInt();
#if __PPU || __PSP2 || RSG_ORBIS
	u64 timestamp = time(NULL);
#else
	__time64_t timestamp = _time64(NULL);
#endif

	u8 sig[32];
	CompileTimeAssert(sizeof(digest) == 20);
	CompileTimeAssert(sizeof(random_number) == 4);
	CompileTimeAssert(sizeof(timestamp) == 8);
	sysMemCpy(sig, digest, sizeof(digest));
	sysMemCpy(sig + sizeof(digest), &random_number, sizeof(random_number));
	sysMemCpy(sig + sizeof(digest) + sizeof(random_number), &timestamp, sizeof(timestamp));

	u8 key[32]; // 256-bit AES key
	u32 salt[] = {0x1508E96FUL, 0x47B847D1UL, 0x3A658C71UL, random_number};
	pkcs5_pbkdf2((const char*)sig, sizeof(sig), (const char*)salt, sizeof(salt), key, sizeof(key), 2000);

	// encrypt the signature using the derived key and then with the Rage key (AES)
	AES aes_derivedKey(key);
	aes_derivedKey.Encrypt(sig, sizeof(sig));

	if(context.m_LockToTitle)
	{
		AES aes_rageKey;
		aes_rageKey.Encrypt(sig, sizeof(sig));
	}

	// store the sig, random_number, and timestamp with the file
	sysMemCpy(signature, sig, sizeof(sig));
	sysMemCpy(signature + sizeof(sig), &random_number, sizeof(random_number));
	sysMemCpy(signature + sizeof(sig) + sizeof(random_number), &timestamp, sizeof(timestamp));
}

bool
datVerifySignature(const u8* dataToProtect,
				   const u32 sizeOfDataToProtect,
				   const datProtectContext &context,
				   const u8 (&signature)[datProtectSignatureSize])
{
	u8 digest[Sha1::SHA1_DIGEST_LENGTH];
	datGetDigest(dataToProtect, sizeOfDataToProtect, digest, context);

	// derive an aes key from the digest (PBKDF2)
	u8 sig[32];
	sysMemCpy(sig, digest, sizeof(digest));
	sysMemCpy(sig + sizeof(digest), signature + sizeof(sig), sizeof(u32));
	sysMemCpy(sig + sizeof(digest) + sizeof(u32), signature + sizeof(sig) + sizeof(u32), sizeof(u64));

	u32 random_number;
	sysMemCpy(&random_number, signature + sizeof(sig), sizeof(random_number));

	u8 key[32]; // 256-bit AES key
	u32 salt[] = {0x1508E96FUL, 0x47B847D1UL, 0x3A658C71UL, random_number};
	pkcs5_pbkdf2((const char*)sig, sizeof(sig), (const char*)salt, sizeof(salt), key, sizeof(key), 2000);

	// decrypt the stored signature using the Rage key and then with the derived key (AES)
	u8 stored[32];
	sysMemCpy(stored, signature, sizeof(stored));

	if(context.m_LockToTitle)
	{
		AES aes_rageKey;
		aes_rageKey.Decrypt(stored, sizeof(stored));
	}

	AES aes_derivedKey(key);
	aes_derivedKey.Decrypt(stored, sizeof(stored));

	// compare the calculated signature with the decrypted stored signature
	return memcmp(stored, sig, sizeof(sig)) == 0;
}

void 
datProtect(u8* dataToProtect,
		   const u32 sizeOfDataToProtect,
		   const datProtectContext &context,
		   u8 (&signature)[datProtectSignatureSize])
{
	// sign
	datSign(dataToProtect, sizeOfDataToProtect, context, signature);

	// derive an aes key from the signature (PBKDF2)
	u8 key[32]; // 256-bit AES key
	u32 salt[] = {0xE109A542UL, 0xF60A133BUL, 0x81AC0255UL, 0xCC39401BUL};
	pkcs5_pbkdf2((const char*)signature, sizeof(signature), (const char*)salt, sizeof(salt), key, sizeof(key), 2000);

	// encrypt the key using the Rage key
	if(context.m_LockToTitle)
	{
		AES aes_rageKey;
		aes_rageKey.Encrypt(key, sizeof(key));
	}

	// encrypt the data using the derived key (AES)
	AES aes_derivedKey(key);
	//@@: location DATPROTECT_DERIVED_KEY_ENCRYPT
	aes_derivedKey.Encrypt(dataToProtect, sizeOfDataToProtect);

	// re-encrypt the last 16 bytes since the AES algorithm will ignore the last (size % 16) bytes
	if(((sizeOfDataToProtect % 16) != 0) && (sizeOfDataToProtect > 16))
	{
		aes_derivedKey.Encrypt(dataToProtect + sizeOfDataToProtect - 16, 16);
	}

	// hash cyphertext data with the private data as the key
	const u8 *hmacKey = context.m_PrivateData;
	u32 hmacKeyLen = context.m_PrivateDataSize;
	Sha1 hashCyphertext(hmacKey, hmacKeyLen);
	u8 digest[Sha1::SHA1_DIGEST_LENGTH];
	hashCyphertext.Update(dataToProtect, sizeOfDataToProtect);
	hashCyphertext.Update((const u8*)&sizeOfDataToProtect, sizeof(sizeOfDataToProtect));
	hashCyphertext.Final(digest);

	// derive aes key from cyphertext hash
	u8 key2[32]; // 256-bit AES key
	u32 salt2[] = {0x0FC919E8UL, 0x9A17C45FUL, 0xE716D46CUL, 0x3A159C75UL};
	pkcs5_pbkdf2((const char*)digest, sizeof(digest), (const char*)salt2, sizeof(salt2), key2, sizeof(key2), 2000);

	// encrypt signature with derived key
	AES aes_derivedKey2(key2);
	aes_derivedKey2.Encrypt(signature, sizeof(signature));
}

bool datUnprotect(u8* protectedData,
				  const u32 sizeOfProtectedData,
				  const datProtectContext &context,
				  const u8 (&signature)[datProtectSignatureSize])
{
	u8 signatureCpy[datProtectSignatureSize];
	sysMemCpy(signatureCpy, signature, sizeof(signatureCpy));

	// hash cyphertext data with the private data as the key
	const u8 *hmacKey = context.m_PrivateData;
	u32 hmacKeyLen = context.m_PrivateDataSize;
	Sha1 hashCyphertext(hmacKey, hmacKeyLen);
	u8 digest[Sha1::SHA1_DIGEST_LENGTH];
	hashCyphertext.Update(protectedData, sizeOfProtectedData);
	hashCyphertext.Update((const u8*)&sizeOfProtectedData, sizeof(sizeOfProtectedData));
	//@@: location DATPROTECT_FINALIZE_HASH_CIPHER
	hashCyphertext.Final(digest);

	// derive aes key from cyphertext digest
	u8 key[32]; // 256-bit AES key
	u32 salt[] = {0x0FC919E8UL, 0x9A17C45FUL, 0xE716D46CUL, 0x3A159C75UL};
	pkcs5_pbkdf2((const char*)digest, sizeof(digest), (const char*)salt, sizeof(salt), key, sizeof(key), 2000);

	// decrypt signature with derived key
	AES aes_derivedKey(key);
	aes_derivedKey.Decrypt(signatureCpy, sizeof(signatureCpy));

	// derive an aes key from the signature (PBKDF2)
	u8 key2[32]; // 256-bit AES key
	u32 salt2[] = {0xE109A542UL, 0xF60A133BUL, 0x81AC0255UL, 0xCC39401BUL};
	pkcs5_pbkdf2((const char*)signatureCpy, sizeof(signatureCpy), (const char*)salt2, sizeof(salt2), key2, sizeof(key2), 2000);

	// encrypt the key with the Rage key
	if(context.m_LockToTitle)
	{
		AES aes_rageKey;
		aes_rageKey.Encrypt(key2, sizeof(key2));
	}

	// decrypt cyphertext with rage key
	//@@: location DATUNPROTECT_CREATE_DERIVED_KEY_TWO 
	AES aes_derivedKey2(key2);

	// the last 16 bytes are re-encrypted since the AES algorithm will ignore the last (size % 16) bytes
	if(((sizeOfProtectedData % 16) != 0) && (sizeOfProtectedData > 16))
	{
		aes_derivedKey2.Decrypt(protectedData + sizeOfProtectedData - 16, 16);
	}

	// decrypt cyphertext with derived key
	aes_derivedKey2.Decrypt(protectedData, sizeOfProtectedData);

	// verify signature
	return datVerifySignature(protectedData, sizeOfProtectedData, context, signatureCpy);
}

#if ENABLE_DAT_PROTECT_TEST
void datProtectTest()
{
	static sysTimer timer;

	const u32 maxSize = 20 * 1024 * 1024;
	u8 *pool = rage_new u8[maxSize * 2];
	if(!AssertVerify(pool))
	{
		return;
	}

	for(u32 i = 0; i < 1000; i++)
	{
		mthRandom r(sysTimer::GetSystemMsTime());
		u32 size = i+1;
		//u32 size = r.GetRanged(1, maxSize);

		u8* data = pool;
		u8* data2 = pool + maxSize;

		for(u32 j = 0; j < size; j++)
		{
			data[j] = (u8)r.GetRanged(0x0, 0xFF);
		}

		sysMemCpy(data2, data, size);

		bool tamperData = r.GetBool();
		bool tamperSignature = r.GetBool();
		bool lockToTitle = r.GetBool();
		bool lockToProfileId = r.GetBool();

		u32 profileId = 28175236;
		u8 *privateData = NULL;
		u32 privateDataLen = 0;
		if(lockToProfileId)
		{
			privateData = (u8*)&profileId;
			privateDataLen = sizeof(profileId);
		}

		datProtectContext context(lockToTitle, privateData, privateDataLen);

		Displayf("Test %d: size %u, tamperData:%s, tamperSignature:%s, lockToTitle:%s, lockToProfileId:%s ",
			i + 1, size, tamperData ? "yes" : "no", tamperSignature ? "yes" : "no", lockToTitle ? "yes" : "no", lockToProfileId ? "yes" : "no");

		u8 signature[datProtectSignatureSize];
		datSign(data, size, context, signature);

		bool result = datVerifySignature(data, size, context, signature);
		Assert(result);
		Displayf("\tsignature %s", result ? "pass" : "FAIL");

		u8 signature2[datProtectSignatureSize];
		timer.Reset();
		datProtect(data, size, context, signature2);
		float ms = (timer.GetUsTime()) / 1000;
		Displayf("\tdatProtect took %f ms (%f ms per KB)", ms, ms / (size / 1024));

		if(tamperData)
		{
			u32 index = r.GetRanged(0, size - 1);
			data[index] = (u8)r.GetRangedDifferent(data[index], 0x0, 0xFF);
		}

		if(tamperSignature)
		{
			u32 index = r.GetRanged(0, sizeof(signature2) - 1);
			signature2[index] = (u8)r.GetRangedDifferent(signature2[index], 0x0, 0xFF);
		}

		timer.Reset();
		result = datUnprotect(data, size, context, signature2);
		ms = (timer.GetUsTime()) / 1000;
		Displayf("\tdatUnprotect took %f ms (%f ms per KB)", ms, ms / (size / 1024));
		Assert((result == true && tamperData == false && tamperSignature == false) ||
			(result == false && (tamperData == true || tamperSignature == true)));

		bool identical = memcmp(data, data2, size) == 0;
		Assert((result == identical) || (size < 16));

		if(tamperData || tamperSignature)
		{
			Assert((identical == false) || (size < 16));
			if(size < 16)
			{
				Displayf("\tencryption %s, identical: %s", result ? "FAILED TO DETECT MODIFICATION" : "modification detected", identical ? "yes (size < 16)" : "no (good)");
			}
			else
			{
				Displayf("\tencryption %s, identical: %s", result ? "FAILED TO DETECT MODIFICATION" : "modification detected", identical ? "YES (but shouldn't be)" : "no (good)");
			}
		}
		else
		{
			Displayf("\tencryption %s, identical: %s", result ? "pass" : "FAIL", identical ? "yes" : "NO");
		}
	}

	delete[] pool;
}
#endif

}	// namespace rage
