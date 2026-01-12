/*	$OpenBSD: pbkdf2.h,v 1.1 2008/06/14 06:28:27 djm Exp $	*/

/*-
 * Copyright (c) 2008 Damien Bergamini <damien.bergamini@free.fr>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Password-Based Key Derivation Function 2 (PKCS #5 v2.0).
 * Code based on IEEE Std 802.11-2007, Annex H.4.2.
 */

/*
	Purpose (based on Wikipedia entry):

	PBKDF2 applies a pseudorandom function, such as a cryptographic hash,
	cipher, or HMAC to the input password or passphrase, along with a salt
	value and repeats the process many times (1000 is a recommended minimum
	number of iterations) to produce a derived key, which can then be used
	as a cryptographic key in subsequent operations.
	
	For example, this can be used to generate an AES key from a password,
	or from the contents of a file, etc. Decryption is then only possible
	if the same passphrase is supplied to re-generate the key.
*/

#ifndef PBKDF2_H
#define PBKDF2_H

namespace rage {

// uncomment this to test pkcs5_pbkdf2, sha1, and hmac_sha1 code against known vectors
// #define PKCS5_PBKDF2_TEST

// PURPOSE:	Derives a key from a passphrase (see comments above)
// PARAMS:	passphrase - the passphrase/password/arbitrary data from which to generate a key
//			passphrase_len - length in bytes of the passphrase
//			salt - data that must be passed along with passphrase in order to re-generate the key.
//				   the salt need not be private data, and is used to strengthen the key from certain types of attacks
//			salt_len - length in bytes of the salt
//			key - the key to return. This must be allocated before calling this function.
//			key_len - the desired key length to be generated
//			rounds - the number of iterations to perform (higher = more secure but takes longer), should be at least 1000.
// RETURNS: false if parameters are invalid, true otherwise. Upon return, 'key' will contain the generated key.
bool
pkcs5_pbkdf2(const char *passphrase,
			 const u32 passphrase_len,
			 const char *salt,
			 const u32 salt_len,
			 u8 *key,
			 const u32 key_len,
			 const u32 rounds = 2000);

#ifdef PKCS5_PBKDF2_TEST
int
pbkdf2_test();
#endif

} // namespace rage

#endif // PBKDF2_H
