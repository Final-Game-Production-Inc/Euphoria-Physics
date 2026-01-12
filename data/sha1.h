/*	$OpenBSD: sha1.h,v 1.23 2004/06/22 01:57:30 jfb Exp $	*/

/*
 * SHA-1 in C
 * By Steve Reid <steve@edmweb.com>
 * 100% Public Domain
 */

/*
 * Modified from the original to be object-oriented, and combines HMAC algorithm
 */

#ifndef _SHA1_H
#define _SHA1_H

namespace rage {

/*
   Provides an implementation of the cryptographically strong Secure Hashing Algorithm 1 (SHA-1).

   Also provides an implementation of HMAC-SHA1 (Hash-based Message Authentication Code), which
   is used for calculating a message authentication code (MAC) using a cryptographic hash
   function in combination with a secret key. It may be used to simultaneously verify both the
   data integrity and the authenticity of a message/file.

   Both take an arbitrary block of data as input and output a fixed-size (160-bit) digest.
 
   Example usage:
   {
	 Sha1 sha1;
	 u8 digest[Sha1::SHA1_DIGEST_LENGTH] = {0};

	 unsigned char data[] = "Hi There";
	 sha1.Update(data, sizeof(data)-1);
	 sha1.Final(digest);

	 for(u8 i = 0; i < sizeof(digest); i += 4)
	 {
		printf("%02X%02X%02X%02X", digest[i], digest[i+1], digest[i+2], digest[i+3]);
	 }
   }

   This class can be used to incrementally hash large datasets (such as the contents of a file).
   The resulting digest will be the same as when the entire dataset is passed at once.

   Example usage (1 million a's):
   {
	 Sha1 sha1;
	 u8 digest[Sha1::SHA1_DIGEST_LENGTH] = {0};
	 unsigned char data[] = "a";

	 for(u32 i = 0; i < 1000000; i++)
	 {
		sha1.Update(data, 1);
	 }

	 sha1.Final(digest);
   }

   Example usage using HMAC:
   {
	 u8 key[16] = {0x0b, 0x0b, 0x0b, 0x0b,
				   0x0b, 0x0b, 0x0b, 0x0b,
				   0x0b, 0x0b, 0x0b, 0x0b,
				   0x0b, 0x0b, 0x0b, 0x0b};
	 Sha1 hmac_sha1(key, sizeof(key));
	 u8 digest[Sha1::SHA1_DIGEST_LENGTH] = {0};
	 unsigned char data[] = "Hi There";
	 hmac_sha1.Update(data, sizeof(data)-1);
	 hmac_sha1.Final(digest);
   }
*/

class Sha1
{
public:
	static const u8 SHA1_DIGEST_LENGTH = 20; // 160-bit digest

	Sha1();

	// if a key is provided, then it becomes HMAC-SHA1 (see comments above)
	// if a NULL key is passed, then it reverts to regular SHA-1
	Sha1(const u8 *key, const u32 key_len);

    void Clear();
    // Allows this sha1 instance to be reset to an HMAC-SHA1
    void Clear(const u8 *key, const u32 key_len);

	void Update(const u8 *data, u32 len);
	void Final(u8 (&digest)[SHA1_DIGEST_LENGTH]);

private:
	static const u8 SHA1_BLOCK_LENGTH = 64;	// 512-bit block

	void Init();
	void Pad();
	void Transform(u32 state[5], const u8 buffer[SHA1_BLOCK_LENGTH]);

	struct SHA1_CTX
	{
		u32 state[5];
		u64 count;
		u8 buffer[SHA1_BLOCK_LENGTH];
	};

	SHA1_CTX m_Context;
	u8 m_Kpad[Sha1::SHA1_BLOCK_LENGTH];
	u8 m_Key[Sha1::SHA1_BLOCK_LENGTH];
	u32 m_KeyLen;
};

}	// namespace rage

#endif // _SHA1_H
