/*
 * 
 * (c) Copyright 1989 OPEN SOFTWARE FOUNDATION, INC.
 * (c) Copyright 1989 HEWLETT-PACKARD COMPANY
 * (c) Copyright 1989 DIGITAL EQUIPMENT CORPORATION
 * To anyone who acknowledges that this file is provided "AS IS"
 * without any express or implied warranty:
 *                 permission to use, copy, modify, and distribute this
 * file for any purpose is hereby granted without fee, provided that
 * the above copyright notices and this notice appears in all source
 * code copies, and that none of the names of Open Software
 * Foundation, Inc., Hewlett-Packard Company, or Digital Equipment
 * Corporation be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.  Neither Open Software Foundation, Inc., Hewlett-
 * Packard Company, nor Digital Equipment Corporation makes any
 * representations about the suitability of this software for any
 * purpose.
 * 
 */

/*	uuid.c
	Copyright 1999-2002, Apple, Inc. All rights reserved.
	Responsibility: Doug Davidson

	Universal Unique ID. Note this definition will result is a 16-byte
	structure regardless what platform it is on.
*/

#ifndef ATL_UUID_H
#define ATL_UUID_H

namespace rage
{

namespace UuidUtils
{

struct Uuid_t {
    unsigned long        time_low;
    unsigned short       time_mid;
    unsigned short       time_hi_and_version;
    unsigned char        clock_seq_hi_and_reserved;
    unsigned char        clock_seq_low;
    unsigned char        node[6];
};


typedef bool OSErr;

enum {
    kUUIDInternalError = -21001,
    kUUIDInvalidString = -21002
};

unsigned long _CFGenerateUUID(Uuid_t *uuid);

typedef struct
{
    unsigned char eaddr[6];      /* 6 bytes of ethernet hardware address */
} uuid_address_t;

typedef struct {
    unsigned long lo;
    unsigned long hi;
} uuid_time_t;

OSErr GenRandomEthernet(uuid_address_t *addr);

OSErr ReadPrefData(void);

/*
 * Internal structure of universal unique IDs (UUIDs).
 *
 * There are three "variants" of UUIDs that this code knows about.  The
 * variant #0 is what was defined in the 1989 HP/Apollo Network Computing
 * Architecture (NCA) specification and implemented in NCS 1.x and DECrpc
 * v1.  Variant #1 is what was defined for the joint HP/DEC specification
 * for the OSF (in DEC's "UID Architecture Functional Specification Version
 * X1.0.4") and implemented in NCS 2.0, DECrpc v2, and OSF 1.0 DCE RPC.
 * Variant #2 is defined by Microsoft.
 *
 * This code creates only variant #1 UUIDs.
 * 
 * The three UUID variants can exist on the same wire because they have
 * distinct values in the 3 MSB bits of octet 8 (see table below).  Do
 * NOT confuse the version number with these 3 bits.  (Note the distinct
 * use of the terms "version" and "variant".) Variant #0 had no version
 * field in it.  Changes to variant #1 (should any ever need to be made)
 * can be accomodated using the current form's 4 bit version field.
 * 
 * The UUID record structure MUST NOT contain padding between fields.
 * The total size = 128 bits.
 *
 * To minimize confusion about bit assignment within octets, the UUID
 * record definition is defined only in terms of fields that are integral
 * numbers of octets.
 *
 * Depending on the network data representation, the multi-octet unsigned
 * integer fields are subject to byte swapping when communicated between
 * dissimilar endian machines.  Note that all three UUID variants have
 * the same record structure; this allows this byte swapping to occur.
 * (The ways in which the contents of the fields are generated can and
 * do vary.)
 *
 * The following information applies to variant #1 UUIDs:
 *
 * The lowest addressed octet contains the global/local bit and the
 * unicast/multicast bit, and is the first octet of the address transmitted
 * on an 802.3 LAN.
 *
 * The adjusted time stamp is split into three fields, and the clockSeq
 * is split into two fields.
 *
 * |<------------------------- 32 bits -------------------------->|
 *
 * +--------------------------------------------------------------+
 * |                     low 32 bits of time                      |  0-3  .time_low
 * +-------------------------------+-------------------------------
 * |     mid 16 bits of time       |  4-5               .time_mid
 * +-------+-----------------------+
 * | vers. |   hi 12 bits of time  |  6-7               .time_hi_and_version
 * +-------+-------+---------------+
 * |Res|  clkSeqHi |  8                                 .clock_seq_hi_and_reserved
 * +---------------+
 * |   clkSeqLow   |  9                                 .clock_seq_low
 * +---------------+----------...-----+
 * |            node ID               |  8-16           .node
 * +--------------------------...-----+
 *
 * --------------------------------------------------------------------------
 *
 * The structure layout of all three UUID variants is fixed for all time.
 * I.e., the layout consists of a 32 bit int, 2 16 bit ints, and 8 8
 * bit ints.  The current form version field does NOT determine/affect
 * the layout.  This enables us to do certain operations safely on the
 * variants of UUIDs without regard to variant; this increases the utility
 * of this code even as the version number changes (i.e., this code does
 * NOT need to check the version field).
 *
 * The "Res" field in the octet #8 is the so-called "reserved" bit-field
 * and determines whether or not the uuid is a old, current or other
 * UUID as follows:
 *
 *      MS-bit  2MS-bit  3MS-bit      Variant
 *      ---------------------------------------------
 *         0       x        x       0 (NCS 1.5)
 *         1       0        x       1 (DCE 1.0 RPC)
 *         1       1        0       2 (Microsoft)
 *         1       1        1       unspecified
 *
 * --------------------------------------------------------------------------
 *
 * Internal structure of variant #0 UUIDs
 *
 * The first 6 octets are the number of 4 usec units of time that have
 * passed since 1/1/80 0000 GMT.  The next 2 octets are reserved for
 * future use.  The next octet is an address family.  The next 7 octets
 * are a host ID in the form allowed by the specified address family.
 *
 * Note that while the family field (octet 8) was originally conceived
 * of as being able to hold values in the range [0..255], only [0..13]
 * were ever used.  Thus, the 2 MSB of this field are always 0 and are
 * used to distinguish old and current UUID forms.
 *
 * +--------------------------------------------------------------+
 * |                    high 32 bits of time                      |  0-3  .time_high
 * +-------------------------------+-------------------------------
 * |     low 16 bits of time       |  4-5               .time_low
 * +-------+-----------------------+
 * |         reserved              |  6-7               .reserved
 * +---------------+---------------+
 * |    family     |   8                                .family
 * +---------------+----------...-----+
 * |            node ID               |  9-16           .node
 * +--------------------------...-----+
 *
 */

/***************************************************************************
 *
 * Local definitions
 *
 **************************************************************************/
 
/*
 * local defines used in uuid bit-diddling
 */
#define HI_WORD(w)                  ((w) >> 16)
#define RAND_MASK                   0x3fff      /* same as CLOCK_SEQ_LAST */

#define TIME_MID_MASK               0x0000ffff
#define TIME_HIGH_MASK              0x0fff0000
#define TIME_HIGH_SHIFT_COUNT       16

/*
 *    The following was modified in order to prevent overlap because
 *    our clock is (theoretically) accurate to 1us (or 1s in CarbonLib)
 */


#define MAX_TIME_ADJUST             9            /* Max adjust before tick */

#define CLOCK_SEQ_LOW_MASK          0xff
#define CLOCK_SEQ_HIGH_MASK         0x3f00
#define CLOCK_SEQ_HIGH_SHIFT_COUNT  8
#define CLOCK_SEQ_FIRST             1
#define CLOCK_SEQ_LAST              0x3fff      /* same as RAND_MASK */

/*
 * Note: If CLOCK_SEQ_BIT_BANG == true, then we can avoid the modulo
 * operation.  This should save us a divide instruction and speed
 * things up.
 */

#ifndef CLOCK_SEQ_BIT_BANG
#define CLOCK_SEQ_BIT_BANG          1
#endif

#if CLOCK_SEQ_BIT_BANG
#define CLOCK_SEQ_BUMP(seq)         ((*seq) = ((*seq) + 1) & CLOCK_SEQ_LAST)
#else
#define CLOCK_SEQ_BUMP(seq)         ((*seq) = ((*seq) + 1) % (CLOCK_SEQ_LAST+1))
#endif

#define UUID_VERSION_BITS           (uuid_c_version << 12)
#define UUID_RESERVED_BITS          0x80

#define IS_OLD_UUID(uuid) (((uuid)->clock_seq_hi_and_reserved & 0xc0) != 0x80)

/****************************************************************************
 *
 * local data declarations
 *
 ****************************************************************************/

typedef struct {
    unsigned long lo;
    unsigned long hi;
} unsigned64_t;

typedef enum
{
    uuid_e_less_than, uuid_e_equal_to, uuid_e_greater_than
} uuid_compval_t;




/****************************************************************************
 *
 * local function declarations
 *
 ****************************************************************************/

/*
 * I N I T
 *
 * Startup initialization routine for UUID module.
 */

OSErr init (void);

/*
 * T R U E _ R A N D O M _ I N I T
 */

void true_random_init (void);

/*
 * T R U E _ R A N D O M
 */
unsigned short true_random (void);


/*
 * N E W _ C L O C K _ S E Q
 *
 * Ensure clock_seq is up-to-date
 *
 * Note: clock_seq is architected to be 14-bits (unsigned) but
 *       I've put it in here as 16-bits since there isn't a
 *       14-bit unsigned integer type (yet)
 */ 
void new_clock_seq ( unsigned short * /*clock_seq*/);


/*
 * T I M E _ C M P
 *
 * Compares two UUID times (64-bit DEC UID UTC values)
 */
uuid_compval_t time_cmp (
        uuid_time_t *        /*time1*/,
        uuid_time_t *        /*time2*/
    );


/************************************************************************/
/*                                                                        */
/*    New Routines                                                        */
/*                                                                        */
/************************************************************************/

/*
**++
**
**  ROUTINE NAME:       uuid_get_address
**
**  SCOPE:              PUBLIC
**
**  DESCRIPTION:
**
**  Return our IEEE 802 address.
**
**  This function is not really "public", but more like the SPI functions
**  -- available but not part of the official API.  We've done this so
**  that other subsystems (of which there are hopefully few or none)
**  that need the IEEE 802 address can use this function rather than
**  duplicating the gore it does (or more specifically, the gore that
**  "uuid__get_os_address" does).
**
**  INPUTS:             none
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      addr            IEEE 802 address
**
**      status          return status value
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:       none
**
**--
**/

OSErr uuid_set_address(uuid_address_t *addr);

OSErr uuid_get_address(uuid_address_t *addr);

OSErr encryptMacAddress(uuid_address_t *addr);

OSErr decryptMacAddress(uuid_address_t *addr);

/*****************************************************************************
 *
 *  Macro definitions
 *
 ****************************************************************************/

#define EmptyArg
#define UUID_VERIFY_INIT(Arg)          \
    if (! uuid_init_done)           \
    {                               \
        init (status);              \
        if (*status != uuid_s_ok)   \
        {                           \
            return Arg;                 \
        }                           \
    }

/*
 * Check the reserved bits to make sure the UUID is of the known structure.
 */

#define CHECK_STRUCTURE(uuid) \
( \
    (((uuid)->clock_seq_hi_and_reserved & 0x80) == 0x00) || /* var #0 */ \
    (((uuid)->clock_seq_hi_and_reserved & 0xc0) == 0x80) || /* var #1 */ \
    (((uuid)->clock_seq_hi_and_reserved & 0xe0) == 0xc0)    /* var #2 */ \
)

/*
 * The following macros invoke CHECK_STRUCTURE(), check that the return
 * value is okay and if not, they set the status variable appropriately
 * and return either a boolean false, nothing (for void procedures),
 * or a value passed to the macro.  This has been done so that checking
 * can be done more simply and values are returned where appropriate
 * to keep compilers happy.
 *
 * bCHECK_STRUCTURE - returns boolean false
 * vCHECK_STRUCTURE - returns nothing (void)
 * rCHECK_STRUCTURE - returns 'r' macro parameter
 */

#define bCHECK_STRUCTURE(uuid, status) \
{ \
    if (!CHECK_STRUCTURE (uuid)) \
    { \
        *(status) = uuid_s_bad_version; \
        return (false); \
    } \
}

#define vCHECK_STRUCTURE(uuid, status) \
{ \
    if (!CHECK_STRUCTURE (uuid)) \
    { \
        *(status) = uuid_s_bad_version; \
        return; \
    } \
}

#define rCHECK_STRUCTURE(uuid, status, result) \
{ \
    if (!CHECK_STRUCTURE (uuid)) \
    { \
        *(status) = uuid_s_bad_version; \
        return (result); \
    } \
}


/*
 *  Define constant designation difference in Unix and DTSS base times:
 *  DTSS UTC base time is October 15, 1582.
 *  Unix base time is January 1, 1970.
 */
#define uuid_c_os_base_time_diff_lo     0x13814000
#define uuid_c_os_base_time_diff_hi     0x01B21DD2

#ifndef UUID_C_100NS_PER_SEC
#define UUID_C_100NS_PER_SEC            10000000
#endif

#ifndef UUID_C_100NS_PER_USEC
#define UUID_C_100NS_PER_USEC           10
#endif





/*
 * UADD_UVLW_2_UVLW - macro to add two unsigned 64-bit long integers
 *                      (ie. add two unsigned 'very' long words)
 *
 * Important note: It is important that this macro accommodate (and it does)
 *                 invocations where one of the addends is also the sum.
 *
 * This macro was snarfed from the DTSS group and was originally:
 *
 * UTCadd - macro to add two UTC times
 *
 * add lo and high order longword separately, using sign bits of the low-order
 * longwords to determine carry.  sign bits are tested before addition in two
 * cases - where sign bits match. when the addend sign bits differ the sign of
 * the result is also tested:
 *
 *        sign            sign
 *      addend 1        addend 2        carry?
 *
 *          1               1            true
 *          1               0            true if sign of sum clear
 *          0               1            true if sign of sum clear
 *          0               0            false
 */
#define UADD_UVLW_2_UVLW(add1, add2, sum)                               \
    if (!(((add1)->lo&0x80000000UL) ^ ((add2)->lo&0x80000000UL)))       \
    {                                                                   \
        if (((add1)->lo&0x80000000UL))                                  \
        {                                                               \
            (sum)->lo = (add1)->lo + (add2)->lo ;                       \
            (sum)->hi = (add1)->hi + (add2)->hi+1 ;                     \
        }                                                               \
        else                                                            \
        {                                                               \
            (sum)->lo  = (add1)->lo + (add2)->lo ;                      \
            (sum)->hi = (add1)->hi + (add2)->hi ;                       \
        }                                                               \
    }                                                                   \
    else                                                                \
    {                                                                   \
        (sum)->lo = (add1)->lo + (add2)->lo ;                           \
        (sum)->hi = (add1)->hi + (add2)->hi ;                           \
        if (!((sum)->lo&0x80000000UL))                                  \
            (sum)->hi++ ;                                               \
    }

/*
 * UADD_UW_2_UVLW - macro to add a 16-bit unsigned integer to
 *                   a 64-bit unsigned integer
 *
 * Note: see the UADD_UVLW_2_UVLW() macro
 *
 */
#define UADD_UW_2_UVLW(add1, add2, sum)                                 \
{                                                                       \
    (sum)->hi = (add2)->hi;                                             \
    if ((add2)->lo & 0x80000000UL)                                      \
    {                                                                   \
        (sum)->lo = (*add1) + (add2)->lo;                               \
        if (!((sum)->lo & 0x80000000UL))                                \
        {                                                               \
            (sum)->hi++;                                                \
        }                                                               \
    }                                                                   \
    else                                                                \
    {                                                                   \
        (sum)->lo = (*add1) + (add2)->lo;                               \
    }                                                                   \
}

/*
 * U U I D _ _ G E T _ O S _ T I M E
 *
 * Get OS time - contains platform-specific code.
 */

void uuid__get_os_time (uuid_time_t * uuid_time);

/*
**++
**
**  ROUTINE NAME:       init
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**
**  Startup initialization routine for the UUID module.
**
**  INPUTS:             none
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          return status value
**
**          uuid_s_ok
**          uuid_s_coding_error
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       sets uuid_init_done so this won't be done again
**
**--
**/

OSErr init();


/*****************************************************************************
 *
 *  LOCAL MATH PROCEDURES - math procedures used internally by the UUID module
 *
 ****************************************************************************/

/*
** T I M E _ C M P
**
** Compares two UUID times (64-bit UTC values)
**/

uuid_compval_t time_cmp(uuid_time_t *time1,uuid_time_t *time2);



/****************************************************************************
**
**    U U I D   T R U E   R A N D O M   N U M B E R   G E N E R A T O R
**
*****************************************************************************
**
** This random number generator (RNG) was found in the ALGORITHMS Notesfile.
**
** (Note 16.7, July 7, 1989 by Robert (RDVAX::)Gries, Cambridge Research Lab,
**  Computational Quality Group)
**
** It is really a "Multiple Prime Random Number Generator" (MPRNG) and is
** completely discussed in reference #1 (see below).
**
**   References:
**   1) "The Multiple Prime Random Number Generator" by Alexander Hass
**      pp. 368 to 381 in ACM Transactions on Mathematical Software,
**      December, 1987
**   2) "The Art of Computer Programming: Seminumerical Algorithms
**      (vol 2)" by Donald E. Knuth, pp. 39 to 113.
**
** A summary of the notesfile entry follows:
**
** Gries discusses the two RNG's available for ULTRIX-C.  The default RNG
** uses a Linear Congruential Method (very popular) and the second RNG uses
** a technique known as a linear feedback shift register.
**
** The first (default) RNG suffers from bit-cycles (patterns/repetition),
** ie. it's "not that random."
**
** While the second RNG passes all the emperical tests, there are "states"
** that become "stable", albeit contrived.
**
** Gries then presents the MPRNG and says that it passes all emperical
** tests listed in reference #2.  In addition, the number of calls to the
** MPRNG before a sequence of bit position repeats appears to have a normal
** distribution.
**
** Note (mbs): I have coded the Gries's MPRNG with the same constants that
** he used in his paper.  I have no way of knowing whether they are "ideal"
** for the range of numbers we are dealing with.
**
****************************************************************************/

/*
** T R U E _ R A N D O M _ I N I T
**
** Note: we "seed" the RNG with the bits from the clock and the PID
**
**/

void true_random_init (void);

/*
** T R U E _ R A N D O M
**
** Note: we return a value which is 'tuned' to our purposes.  Anyone
** using this routine should modify the return value accordingly.
**/

unsigned short true_random (void);

/*****************************************************************************
 *
 *  LOCAL PROCEDURES - procedures used staticly by the UUID module
 *
 ****************************************************************************/

/*
** N E W _ C L O C K _ S E Q
**
** Ensure *clkseq is up-to-date
**
** Note: clock_seq is architected to be 14-bits (unsigned) but
**       I've put it in here as 16-bits since there isn't a
**       14-bit unsigned integer type (yet)
**/

void new_clock_seq (unsigned short*clkseq);



/*    ReadPrefData
 *
 *        Read the preferences data into my global variables
 */

OSErr ReadPrefData(void);

#if 0
// currently unused

/*    WritePrefData
 *
 *        Write the preferences data back out to my global variables.
 *    This gets called a couple of times. First, this is called by
 *    my GetRandomEthernet routine if I generated a psudorandom MAC
 *    address. Second, this is called when the library is being
 *    terminated through the __terminate() CFM call.
 *
 *        Note this does it's best attempt at writing the data out,
 *    and relies on ReadPrefData to check for integrety of the actual
 *    saved file.
 */

void WritePrefData(void)
{
}

#endif

} // namespace UuidUtils

} // namespace rage

#endif // ATL_UUID_H