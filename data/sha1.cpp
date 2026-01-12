/*	$OpenBSD: sha1.c,v 1.21 2008/07/29 19:32:50 miod Exp $	*/

/*
 * SHA-1 in C
 * By Steve Reid <steve@edmweb.com>
 * 100% Public Domain
 *
 * Test Vectors (from FIPS PUB 180-1)
 * "abc"
 *   A9993E36 4706816A BA3E2571 7850C26C 9CD0D89D
 * "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"
 *   84983E44 1C3BD26E BAAE4AA1 F95129E5 E54670F1
 * A million repetitions of "a"
 *   34AA973C D4C4DAA4 F61EEB2B DBAD2731 6534016F
 */

#include <string.h>
#include "sha1.h"
#include "system/memops.h"

namespace rage {

#define rol(value, bits) (((value) << (bits)) | ((value) >> (32 - (bits))))

/*
 * blk0() and blk() perform the initial expand.
 * I got the idea of expanding during the round function from SSLeay
 */
#if !__BE
# define blk0(i) (block->l[i] = (rol(block->l[i],24)&0xFF00FF00) \
    |(rol(block->l[i],8)&0x00FF00FF))
#else
# define blk0(i) block->l[i]
#endif

#define blk(i) (block->l[i&15] = rol(block->l[(i+13)&15]^block->l[(i+8)&15] \
    ^block->l[(i+2)&15]^block->l[i&15],1))

/*
 * (R0+R1), R2, R3, R4 are the different operations (rounds) used in SHA1
 */
#define R0(v,w,x,y,z,i) z+=((w&(x^y))^y)+blk0(i)+0x5A827999+rol(v,5);w=rol(w,30);
#define R1(v,w,x,y,z,i) z+=((w&(x^y))^y)+blk(i)+0x5A827999+rol(v,5);w=rol(w,30);
#define R2(v,w,x,y,z,i) z+=(w^x^y)+blk(i)+0x6ED9EBA1+rol(v,5);w=rol(w,30);
#define R3(v,w,x,y,z,i) z+=(((w|x)&y)|(w&x))+blk(i)+0x8F1BBCDC+rol(v,5);w=rol(w,30);
#define R4(v,w,x,y,z,i) z+=(w^x^y)+blk(i)+0xCA62C1D6+rol(v,5);w=rol(w,30);

typedef union {
	u8 c[64];
	u32 l[16];
} CHAR64LONG16;

#ifdef __sh__
static void do_R01(u32 *a, u32 *b, u32 *c, u32 *d, u32 *e, CHAR64LONG16 *);
static void do_R2(u32 *a, u32 *b, u32 *c, u32 *d, u32 *e, CHAR64LONG16 *);
static void do_R3(u32 *a, u32 *b, u32 *c, u32 *d, u32 *e, CHAR64LONG16 *);
static void do_R4(u32 *a, u32 *b, u32 *c, u32 *d, u32 *e, CHAR64LONG16 *);

#define nR0(v,w,x,y,z,i) R0(*v,*w,*x,*y,*z,i)
#define nR1(v,w,x,y,z,i) R1(*v,*w,*x,*y,*z,i)
#define nR2(v,w,x,y,z,i) R2(*v,*w,*x,*y,*z,i)
#define nR3(v,w,x,y,z,i) R3(*v,*w,*x,*y,*z,i)
#define nR4(v,w,x,y,z,i) R4(*v,*w,*x,*y,*z,i)

static void
do_R01(u32 *a, u32 *b, u32 *c, u32 *d, u32 *e, CHAR64LONG16 *block)
{
    nR0(a,b,c,d,e, 0); nR0(e,a,b,c,d, 1); nR0(d,e,a,b,c, 2); nR0(c,d,e,a,b, 3);
    nR0(b,c,d,e,a, 4); nR0(a,b,c,d,e, 5); nR0(e,a,b,c,d, 6); nR0(d,e,a,b,c, 7);
    nR0(c,d,e,a,b, 8); nR0(b,c,d,e,a, 9); nR0(a,b,c,d,e,10); nR0(e,a,b,c,d,11);
    nR0(d,e,a,b,c,12); nR0(c,d,e,a,b,13); nR0(b,c,d,e,a,14); nR0(a,b,c,d,e,15);
    nR1(e,a,b,c,d,16); nR1(d,e,a,b,c,17); nR1(c,d,e,a,b,18); nR1(b,c,d,e,a,19);
}

static void
do_R2(u32 *a, u32 *b, u32 *c, u32 *d, u32 *e, CHAR64LONG16 *block)
{
    nR2(a,b,c,d,e,20); nR2(e,a,b,c,d,21); nR2(d,e,a,b,c,22); nR2(c,d,e,a,b,23);
    nR2(b,c,d,e,a,24); nR2(a,b,c,d,e,25); nR2(e,a,b,c,d,26); nR2(d,e,a,b,c,27);
    nR2(c,d,e,a,b,28); nR2(b,c,d,e,a,29); nR2(a,b,c,d,e,30); nR2(e,a,b,c,d,31);
    nR2(d,e,a,b,c,32); nR2(c,d,e,a,b,33); nR2(b,c,d,e,a,34); nR2(a,b,c,d,e,35);
    nR2(e,a,b,c,d,36); nR2(d,e,a,b,c,37); nR2(c,d,e,a,b,38); nR2(b,c,d,e,a,39);
}

static void
do_R3(u32 *a, u32 *b, u32 *c, u32 *d, u32 *e, CHAR64LONG16 *block)
{
    nR3(a,b,c,d,e,40); nR3(e,a,b,c,d,41); nR3(d,e,a,b,c,42); nR3(c,d,e,a,b,43);
    nR3(b,c,d,e,a,44); nR3(a,b,c,d,e,45); nR3(e,a,b,c,d,46); nR3(d,e,a,b,c,47);
    nR3(c,d,e,a,b,48); nR3(b,c,d,e,a,49); nR3(a,b,c,d,e,50); nR3(e,a,b,c,d,51);
    nR3(d,e,a,b,c,52); nR3(c,d,e,a,b,53); nR3(b,c,d,e,a,54); nR3(a,b,c,d,e,55);
    nR3(e,a,b,c,d,56); nR3(d,e,a,b,c,57); nR3(c,d,e,a,b,58); nR3(b,c,d,e,a,59);
}

static void
do_R4(u32 *a, u32 *b, u32 *c, u32 *d, u32 *e, CHAR64LONG16 *block)
{
    nR4(a,b,c,d,e,60); nR4(e,a,b,c,d,61); nR4(d,e,a,b,c,62); nR4(c,d,e,a,b,63);
    nR4(b,c,d,e,a,64); nR4(a,b,c,d,e,65); nR4(e,a,b,c,d,66); nR4(d,e,a,b,c,67);
    nR4(c,d,e,a,b,68); nR4(b,c,d,e,a,69); nR4(a,b,c,d,e,70); nR4(e,a,b,c,d,71);
    nR4(d,e,a,b,c,72); nR4(c,d,e,a,b,73); nR4(b,c,d,e,a,74); nR4(a,b,c,d,e,75);
    nR4(e,a,b,c,d,76); nR4(d,e,a,b,c,77); nR4(c,d,e,a,b,78); nR4(b,c,d,e,a,79);
}
#endif

/*
 * Hash a single 512-bit block. This is the core of the algorithm.
 */
void
Sha1::Transform(u32 state[5], const u8 buffer[Sha1::SHA1_BLOCK_LENGTH])
{
	u32 a, b, c, d, e;
	u8 workspace[Sha1::SHA1_BLOCK_LENGTH];
	CHAR64LONG16 *block = (CHAR64LONG16 *)workspace;
	//@@: location SHA1_TRANSFORM_MEMCPY
	(void)sysMemCpy(block, buffer, Sha1::SHA1_BLOCK_LENGTH);

	/* Copy m_Context.state[] to working vars */
	a = state[0];
	b = state[1];
	c = state[2];
	d = state[3];
	e = state[4];

#ifdef __sh__
	do_R01(&a, &b, &c, &d, &e, block);
	do_R2(&a, &b, &c, &d, &e, block);
	do_R3(&a, &b, &c, &d, &e, block);
	do_R4(&a, &b, &c, &d, &e, block);
#else
	/* 4 rounds of 20 operations each. Loop unrolled. */
	R0(a,b,c,d,e, 0); R0(e,a,b,c,d, 1); R0(d,e,a,b,c, 2); R0(c,d,e,a,b, 3);
	R0(b,c,d,e,a, 4); R0(a,b,c,d,e, 5); R0(e,a,b,c,d, 6); R0(d,e,a,b,c, 7);
	R0(c,d,e,a,b, 8); R0(b,c,d,e,a, 9); R0(a,b,c,d,e,10); R0(e,a,b,c,d,11);
	R0(d,e,a,b,c,12); R0(c,d,e,a,b,13); R0(b,c,d,e,a,14); R0(a,b,c,d,e,15);
	R1(e,a,b,c,d,16); R1(d,e,a,b,c,17); R1(c,d,e,a,b,18); R1(b,c,d,e,a,19);
	R2(a,b,c,d,e,20); R2(e,a,b,c,d,21); R2(d,e,a,b,c,22); R2(c,d,e,a,b,23);
	R2(b,c,d,e,a,24); R2(a,b,c,d,e,25); R2(e,a,b,c,d,26); R2(d,e,a,b,c,27);
	R2(c,d,e,a,b,28); R2(b,c,d,e,a,29); R2(a,b,c,d,e,30); R2(e,a,b,c,d,31);
	R2(d,e,a,b,c,32); R2(c,d,e,a,b,33); R2(b,c,d,e,a,34); R2(a,b,c,d,e,35);
	R2(e,a,b,c,d,36); R2(d,e,a,b,c,37); R2(c,d,e,a,b,38); R2(b,c,d,e,a,39);
	R3(a,b,c,d,e,40); R3(e,a,b,c,d,41); R3(d,e,a,b,c,42); R3(c,d,e,a,b,43);
	R3(b,c,d,e,a,44); R3(a,b,c,d,e,45); R3(e,a,b,c,d,46); R3(d,e,a,b,c,47);
	R3(c,d,e,a,b,48); R3(b,c,d,e,a,49); R3(a,b,c,d,e,50); R3(e,a,b,c,d,51);
	R3(d,e,a,b,c,52); R3(c,d,e,a,b,53); R3(b,c,d,e,a,54); R3(a,b,c,d,e,55);
	R3(e,a,b,c,d,56); R3(d,e,a,b,c,57); R3(c,d,e,a,b,58); R3(b,c,d,e,a,59);
	R4(a,b,c,d,e,60); R4(e,a,b,c,d,61); R4(d,e,a,b,c,62); R4(c,d,e,a,b,63);
	R4(b,c,d,e,a,64); R4(a,b,c,d,e,65); R4(e,a,b,c,d,66); R4(d,e,a,b,c,67);
	R4(c,d,e,a,b,68); R4(b,c,d,e,a,69); R4(a,b,c,d,e,70); R4(e,a,b,c,d,71);
	R4(d,e,a,b,c,72); R4(c,d,e,a,b,73); R4(b,c,d,e,a,74); R4(a,b,c,d,e,75);
	R4(e,a,b,c,d,76); R4(d,e,a,b,c,77); R4(c,d,e,a,b,78); R4(b,c,d,e,a,79);
#endif

	/* Add the working vars back into m_Context.state[] */
	state[0] += a;
	state[1] += b;
	state[2] += c;
	state[3] += d;
	state[4] += e;

	/* Wipe variables */
	a = b = c = d = e = 0;
}

Sha1::Sha1()
{
	Init();
	m_KeyLen = 0;
}

Sha1::Sha1(const u8 *key, const u32 key_len)

{
	Clear(key, key_len);
}

void
Sha1::Clear()
{
    Init();
    m_KeyLen = 0;
}

void
Sha1::Clear(const u8 *key, const u32 key_len)
{
    Init();

	if((key == NULL) || (key_len == 0))
	{
		m_KeyLen = 0;
		return;
	}
	
	if (key_len <= Sha1::SHA1_BLOCK_LENGTH) {
		sysMemCpy(m_Key, key, key_len);
		m_KeyLen = key_len;
	} else {
		Update(key, key_len);
		u8 tk[Sha1::SHA1_DIGEST_LENGTH];
		m_KeyLen = 0;
		Final(tk);
		m_KeyLen = Sha1::SHA1_DIGEST_LENGTH;
		sysMemCpy(m_Key, tk, m_KeyLen);
	}

	memset(m_Kpad, 0, sizeof(m_Kpad));
	sysMemCpy(m_Kpad, m_Key, m_KeyLen);

	for (u8 i = 0; i < Sha1::SHA1_BLOCK_LENGTH; i++) {
		m_Kpad[i] ^= 0x36;
	}

	Init();
	Update(m_Kpad, Sha1::SHA1_BLOCK_LENGTH);
}

void
Sha1::Init()
{
	/* SHA1 initialization constants */
	m_Context.count = 0;
	m_Context.state[0] = 0x67452301;
	m_Context.state[1] = 0xEFCDAB89;
	m_Context.state[2] = 0x98BADCFE;
	m_Context.state[3] = 0x10325476;
	m_Context.state[4] = 0xC3D2E1F0;
}

/*
 * Run your data through this.
 */
void
Sha1::Update(const u8 *data, u32 len)
{
	u32 i;

	u32 j = (u32)((m_Context.count >> 3) & 63);

	m_Context.count += (len << 3);

	if ((j + len) > 63) {
		(void)sysMemCpy(&m_Context.buffer[j], data, (i = 64-j));
	
		Transform(m_Context.state, m_Context.buffer);

		for ( ; i + 63 < len; i += 64)
		{
			Transform(m_Context.state, (u8 *)&data[i]);
		}

		j = 0;
	} else {
		i = 0;
	}

	(void)sysMemCpy(&m_Context.buffer[j], &data[i], len - i);
}

/*
 * Add padding and return the message digest.
 */
void
Sha1::Pad()
{
	u8 finalcount[8];

	for (u32 i = 0; i < 8; i++) {
		finalcount[i] = (u8)((m_Context.count >>
		    ((7 - (i & 7)) * 8)) & 255);	/* Endian independent */
	}

	Update((u8 *)"\200", 1);

	while ((m_Context.count & 504) != 448) {
		Update((u8 *)"\0", 1);
	}

	Update(finalcount, 8); /* Should cause a Transform() */
}

void
Sha1::Final(u8 (&digest)[Sha1::SHA1_DIGEST_LENGTH])
{
	Pad();

	for (u32 i = 0; i < Sha1::SHA1_DIGEST_LENGTH; i++) {
		digest[i] = (u8)((m_Context.state[i>>2] >> ((3-(i & 3)) * 8) ) & 255);
	}

	memset(&m_Context, 0, sizeof(m_Context));

	if(m_KeyLen > 0) {
		memset(m_Kpad, 0, sizeof(m_Kpad));
		sysMemCpy(m_Kpad, m_Key, m_KeyLen);

		for (u8 i = 0; i < Sha1::SHA1_BLOCK_LENGTH; i++) {
			m_Kpad[i] ^= 0x5c;
		}

		Init();
		Update(m_Kpad, Sha1::SHA1_BLOCK_LENGTH);
		Update(digest, Sha1::SHA1_DIGEST_LENGTH);
		m_KeyLen = 0; // to prevent infinite recursion;
		Final(digest);
	}
}

}	// namespace rage
