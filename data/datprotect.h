#ifndef DAT_PROTECT_H
#define DAT_PROTECT_H

namespace rage {

// Contains context data that is passed to the various functions below.
// Ideally the context data will contain completely private data if available
// (i.e. data that is not stored anywhere on the user's machine).
// If no private data is available, it can still be used to tie protected
// data to a specific user, title, or any other piece of data that is available
// or reconstructable at runtime.
//
// For example, if the signed-in user id is supplied as part of the private data,
// that same user id would need to be supplied to decrypt or verify it later -
// preventing a user from using a valid save file created by someone else.
//
// WARNING: be very careful if you're attempting to pass an Xbox XUID to tie data
// to a specific Xbox user, since XUIDs change depending on whether the user is
// online or offline. If data is protected using an online XUID, the user won't
// be able to use the data again unless he is signed online (requiring an internet
// connection).
class datProtectContext
{
public:
	// PARAMS:	lockToTitle - Generally set to true, unless you need to access
	//						  the protected data from multiple titles.
	//						  If true, the data is also tied to the Rage AES key
	//						  that is title / platform specific and embedded into
	//						  the exe/xex. Data protected with this set to true
	//						  is not readable by other titles. A hacker would need
	//						  to obtain the embedded key in order to alter the
	//						  data, adding an additional layer of security.
	//			privateData - see comments above. Can be NULL.
	//			privateDataSize - size in bytes of privateData. Can be 0.
	datProtectContext(bool lockToTitle,
					  const u8* privateData,
					  u32 privateDataSize)
	: m_PrivateData(privateData)
	, m_PrivateDataSize(privateDataSize)
	, m_LockToTitle(lockToTitle)
	{

	}

	const u8* m_PrivateData;
	u32 m_PrivateDataSize;
	bool m_LockToTitle : 1;
};

const u16 datProtectSignatureSize = 44;

// PURPOSE:	Calculates a signature that may be used to verify the integrity and
//			authenticity of a block of data.
//			For example, it can be used to detect modifications to save game data, and to
//			prevent a different user from using a valid save file created by someone else
//			(by passing a unique user identifier along with the context object).
// PARAMS:	dataToProtect - the data to sign
//			sizeOfDataToProtect - length in bytes of the data to be signed
//			context - the same context data must be passed to the datVerifySignature function
//					  in order to determine the data's integrity and authenticity
//					  (see comments for the datProtectContext class, above).
//			signature - upon return, the signature will be filled in. The same signature must
//						be passed to datVerifySignature. If you're signing a file, the signature
//						can be stored in the same file, or in a different location, even online.
void datSign(const u8* dataToProtect,
			 const u32 sizeOfDataToProtect,
			 const datProtectContext &context,
			 u8 (&signature)[datProtectSignatureSize]);


// PURPOSE:	Verifies the integrity and authenticity of a block of data.
// PARAMS:	dataToProtect - the data that was passed to datSign
//			sizeOfDataToProtect - length in bytes of the signed data
//			context - the same context data that was passed to datSign
//			signature - the signature that was calculated by datSign
// RETURNS: false if the integrity or authenticity of the data has been compromised, true otherwise
bool datVerifySignature(const u8* dataToProtect,
						const u32 sizeOfDataToProtect,
						const datProtectContext &context,
						const u8 (&signature)[datProtectSignatureSize]);


// PURPOSE:	Encrypts and signs a block of data.
//			Internally uses datSign to verify the integrity and authenticity of a block of data,
//			as well as encrypting the data so it cannot be read until unprotected.
// PARAMS:	dataToProtect - the data to protect (data is encrypted in-place)
//			sizeOfDataToProtect - length in bytes of the data to be protected
//			context - the same context data must be passed to the datUnprotect function.
//					  Context data can be used to tie data to a specific user or title, so even
//					  if the data is unmodified, it won't be decryptable by a different user
//					  (see comments for the datProtectContext class, above).
//			signature - upon return, the signature will be filled in. The same signature must
//						be passed to datVerifySignature. If you're signing a file, the signature
//						can be stored in the same file, or in a different location, even online.
// NOTES:	1. if sizeOfDataToProtect < 16, the data will NOT be encrypted, but the signature
//			will still be able to verify the integrity and authenticity of the data.
//			2. sizeOfDataToProtect does not have to be a multiple of 16. All bytes will be
//			encrypted as long as sizeOfDataToProtect >= 16.
void datProtect(u8* dataToProtect,
				const u32 sizeOfDataToProtect,
				const datProtectContext &context,
				u8 (&signature)[datProtectSignatureSize]);


// PURPOSE:	Decrypts data that was previously encrypted via datProtect.
//			Also verifies the integrity and authenticity of the data.
// PARAMS:	protectedData - the data that was return from datProtect
//							(does not include the signature)
//			sizeOfProtectedData - length in bytes of the protected data
//								  (does not include the length of the signature)
//			context - the same context data that was passed to datProtect
//			signature - the signature that was calculated by datProtect
// RETURNS: false if the integrity or authenticity of the data has been compromised, true otherwise.
//			If false, the data will not be decrypted properly and should be ignored.
bool datUnprotect(u8* protectedData,
				  const u32 sizeOfProtectedData,
				  const datProtectContext &context,
				  const u8 (&signature)[datProtectSignatureSize]);


// set ENABLE_DAT_PROTECT_TEST to 1 and call datProtectTest to test:
//	1. datSign/datVerifySignature
//	2. datProtect/datUnprotect
#define ENABLE_DAT_PROTECT_TEST 0
#if ENABLE_DAT_PROTECT_TEST
void datProtectTest();
#endif

}	// namespace rage

#endif // DAT_PROTECT_H
