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

/*
	Integrated in Rage from http://opensource.apple.com/source/CF/CF-299/Base.subproj/uuid.c
	 + Changes has been made in order to change the MAC address from any sub-system by calling uuid_set_address()
	   If no address is provided then a random mac address is used.
	 + Added a encryption/decryption functions to obfuscate the MAC Address that was in the uuid
	 + Changed the way time is being checked to use rage code. We now use rage::sysTimer::GetSystemMsTime() 
*/

#include "uuid.h"

#include "system/timer.h"

#include <string.h>

namespace rage
{

namespace UuidUtils
{

/*
 *    Preferences file management
 */

static bool GMacAddressInitialized = false;
static uuid_address_t GSavedENetAddr = {{0, 0, 0, 0, 0, 0}};
static uuid_address_t GEncryptKey = {{0xab, 0xee, 0x42, 0x24, 0x99, 0x9c}};
static uuid_time_t GLastTime = {0, 0};            /* Clock state info */
static unsigned short GTimeAdjust = 0;
static unsigned short GClockSeq = 0;

/***************************************************************************
 *
 * Local definitions
 *
 **************************************************************************/

static const long      uuid_c_version          = 1;

/*
 * declarations used in UTC time calculations
 */
 
static uuid_time_t          time_now = {0, 0};     /* utc time as of last query        */
//static uuid_time_t          time_last;    /* utc time last time I looked      */
//static unsigned short       time_adjust;  /* 'adjustment' to ensure uniqness  */
//static unsigned short       clock_seq;    /* 'adjustment' for backwards clocks*/

/*
 * true_random variables
 */

static unsigned long     rand_m = 0;         /* multiplier                       */
static unsigned long     rand_ia = 0;        /* adder #1                         */
static unsigned long     rand_ib = 0;        /* adder #2                         */
static unsigned long     rand_irand = 0;     /* random value                     */

/*
 * saved copy of our IEEE 802 address for quick reference
 */

OSErr uuid_set_address(uuid_address_t *addr)
{
	GMacAddressInitialized = true;
	memmove(&GSavedENetAddr, addr, sizeof (uuid_address_t));
	return false;
}

OSErr uuid_get_address(uuid_address_t *addr)
{
    
    /*
     * just return address we determined previously if we've
     * already got one
     */

	memmove(addr, &GSavedENetAddr, sizeof (uuid_address_t));

	return false;
}

OSErr encryptMacAddress(uuid_address_t *addr)
{
	unsigned int i;
	for (i = 0; i < 6; i++) {
		addr->eaddr[i] = addr->eaddr[i] ^ GEncryptKey.eaddr[i];
	}
	return false;
}

OSErr decryptMacAddress(uuid_address_t *addr)
{
	unsigned int i;
	for (i = 0; i < 6; i++) {
		addr->eaddr[i] = GEncryptKey.eaddr[i] ^ addr->eaddr[i];
	}
	return false;
}

OSErr GenRandomEthernet(uuid_address_t *addr) {
    unsigned int i;
    for (i = 0; i < 6; i++) {
        addr->eaddr[i] = (unsigned char)(true_random() & 0xff);
    }
    return false;
}

/*****************************************************************************
 *
 *  Macro definitions
 *
 ****************************************************************************/

/*
 * ensure we've been initialized
 */
static int uuid_init_done = false;

/*
 * U U I D _ _ G E T _ O S _ T I M E
 *
 * Get OS time - contains platform-specific code.
 */

static const double utc_conversion_factor = 429.4967296; // 2^32 / 10^7

void uuid__get_os_time (uuid_time_t * uuid_time)
{
    unsigned64_t utc;
	unsigned64_t os_basetime_diff;

	double at = rage::sysTimer::GetSystemMsTime() * 1000;
    double utc_at = at / utc_conversion_factor;
    
    /* Convert 'at' in double seconds to 100ns units in utc */
    utc.hi = (unsigned long)utc_at;
    utc_at -= (double)utc.hi;
    utc_at *= utc_conversion_factor;
    utc_at *= 10000000.0;
    utc.lo = (unsigned long)utc_at;

    /*
     * Offset between DTSS formatted times and Unix formatted times.
     */
    os_basetime_diff.lo = uuid_c_os_base_time_diff_lo;
    os_basetime_diff.hi = uuid_c_os_base_time_diff_hi;
    UADD_UVLW_2_UVLW (&utc, &os_basetime_diff, uuid_time);

}

OSErr init()
{
    /*
     * init the random number generator
     */
    
    true_random_init();

    /*
     *    Read the preferences data from the Macintosh pref file
     */
    
    ReadPrefData();

	GenRandomEthernet(&GSavedENetAddr);

    /*
     *    Get the time. Note that I renamed 'time_last' to
     *    GLastTime to indicate that I'm using it elsewhere as
     *    a shared library global.
     */
        
    if ((GLastTime.hi == 0) && (GLastTime.lo == 0)) {
        uuid__get_os_time (&GLastTime);
        GClockSeq = true_random();
    }
    uuid_init_done = true;
    return 0;
}

unsigned long _CFGenerateUUID(Uuid_t *uuid)
{
    OSErr                    err;
    uuid_address_t            eaddr;
    int               got_no_time = false;

    if (!uuid_init_done) {
        err = init();
        if (err) return err;
    }
    /*
     * get our hardware network address
     */
     
    if (0 != (err = uuid_get_address(&eaddr))) return err;

    do
    {
        /*
         * get the current time
         */
        uuid__get_os_time (&time_now);

        /*
         * do stuff like:
         *
         *  o check that our clock hasn't gone backwards and handle it
         *    accordingly with clock_seq
         *  o check that we're not generating uuid's faster than we
         *    can accommodate with our time_adjust fudge factor
         */
        switch (time_cmp (&time_now, &GLastTime))
        {
            case uuid_e_less_than:
                new_clock_seq (&GClockSeq);
                GTimeAdjust = 0;
                break;
            case uuid_e_greater_than:
                GTimeAdjust = 0;
				got_no_time = false;
                break;
            case uuid_e_equal_to:
                if (GTimeAdjust == MAX_TIME_ADJUST)
                {
                    /*
                     * spin your wheels while we wait for the clock to tick
                     */
                    got_no_time = true;
                }
                else
                {
                    GTimeAdjust++;
                }
                break;
            default:
                return (unsigned long) kUUIDInternalError;
        }
    } while (got_no_time);

    GLastTime.lo = time_now.lo;
    GLastTime.hi = time_now.hi;

    if (GTimeAdjust != 0)
    {
        UADD_UW_2_UVLW (&GTimeAdjust, &time_now, &time_now);
    }

    /*
     * now construct a uuid with the information we've gathered
     * plus a few constants
     */
    uuid->time_low = time_now.lo;
    uuid->time_mid = time_now.hi & TIME_MID_MASK;

    uuid->time_hi_and_version =
        (time_now.hi & TIME_HIGH_MASK) >> TIME_HIGH_SHIFT_COUNT;
    uuid->time_hi_and_version |= UUID_VERSION_BITS;

    uuid->clock_seq_low = GClockSeq & CLOCK_SEQ_LOW_MASK;
    uuid->clock_seq_hi_and_reserved =
        (GClockSeq & CLOCK_SEQ_HIGH_MASK) >> CLOCK_SEQ_HIGH_SHIFT_COUNT;

    uuid->clock_seq_hi_and_reserved |= UUID_RESERVED_BITS;

    memmove (uuid->node, &eaddr, sizeof (uuid_address_t));

    return 0;
}

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

uuid_compval_t time_cmp(uuid_time_t *time1,uuid_time_t *time2)
{
    /*
     * first check the hi parts
     */
    if (time1->hi < time2->hi) return (uuid_e_less_than);
    if (time1->hi > time2->hi) return (uuid_e_greater_than);

    /*
     * hi parts are equal, check the lo parts
     */
    if (time1->lo < time2->lo) return (uuid_e_less_than);
    if (time1->lo > time2->lo) return (uuid_e_greater_than);

    return (uuid_e_equal_to);
}


/*
** T R U E _ R A N D O M _ I N I T
**
** Note: we "seed" the RNG with the bits from the clock and the PID
**
**/

void true_random_init (void)
{
    uuid_time_t         t;
    unsigned short          *seedp, seed=0;


    /*
     * optimal/recommended starting values according to the reference
     */
    static unsigned long   rand_m_init     = 971;
    static unsigned long   rand_ia_init    = 11113;
    static unsigned long   rand_ib_init    = 104322;
    static unsigned long   rand_irand_init = 4181;

    rand_m = rand_m_init;
    rand_ia = rand_ia_init;
    rand_ib = rand_ib_init;
    rand_irand = rand_irand_init;

    /*
     * Generating our 'seed' value
     *
     * We start with the current time, but, since the resolution of clocks is
     * system hardware dependent (eg. Ultrix is 10 msec.) and most likely
     * coarser than our resolution (10 usec) we 'mixup' the bits by xor'ing
     * all the bits together.  This will have the effect of involving all of
     * the bits in the determination of the seed value while remaining system
     * independent.  Then for good measure to ensure a unique seed when there
     * are multiple processes creating UUID's on a system, we add in the PID.
     */
    uuid__get_os_time(&t);
    seedp = (unsigned short *)(&t);
    seed ^= *seedp++;
    seed ^= *seedp++;
    seed ^= *seedp++;
    seed ^= *seedp++;
    rand_irand += seed;
}

/*
** T R U E _ R A N D O M
**
** Note: we return a value which is 'tuned' to our purposes.  Anyone
** using this routine should modify the return value accordingly.
**/

unsigned short true_random (void)
{
    rand_m += 7;
    rand_ia += 1907;
    rand_ib += 73939;

    if (rand_m >= 9973) rand_m -= 9871;
    if (rand_ia >= 99991) rand_ia -= 89989;
    if (rand_ib >= 224729) rand_ib -= 96233;

    rand_irand = (rand_irand * rand_m) + rand_ia + rand_ib;

    return (HI_WORD (rand_irand) ^ (rand_irand & RAND_MASK));
}

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

void new_clock_seq (unsigned short*clkseq)
{
    /*
     * A clkseq value of 0 indicates that it hasn't been initialized.
     */
    if (*clkseq == 0)
    {
#ifdef UUID_NONVOLATILE_CLOCK
        *clkseq = uuid__read_clock();           /* read nonvolatile clock */
        if (*clkseq == 0)                       /* still not init'd ???   */
        {
            *clkseq = true_random();      /* yes, set random        */
        }
#else
        /*
         * with a volatile clock, we always init to a random number
         */
        *clkseq = true_random();
#endif
    }

    CLOCK_SEQ_BUMP (clkseq);
    if (*clkseq == 0)
    {
        *clkseq = *clkseq + 1;
    }

#ifdef UUID_NONVOLATILE_CLOCK
    uuid_write_clock (clkseq);
#endif
}



/*    ReadPrefData
 *
 *        Read the preferences data into my global variables
 */

OSErr ReadPrefData(void)
{
    /*
     *    Zero out the saved preferences information
     */
    
    memset((void *)&GSavedENetAddr, 0, sizeof(GSavedENetAddr));
    memset((void *)&GLastTime, 0, sizeof(GLastTime));
    GTimeAdjust = 0;
    GClockSeq = 0;

    return 0;
}

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

#undef HI_WORD
#undef RAND_MASK
#undef TIME_MID_MASK
#undef TIME_HIGH_MASK
#undef TIME_HIGH_SHIFT_COUNT
#undef MAX_TIME_ADJUST
#undef CLOCK_SEQ_LOW_MASK
#undef CLOCK_SEQ_HIGH_MASK
#undef CLOCK_SEQ_HIGH_SHIFT_COUNT
#undef CLOCK_SEQ_FIRST
#undef CLOCK_SEQ_LAST
#undef CLOCK_SEQ_BIT_BANG
#undef CLOCK_SEQ_BUMP
#undef UUID_VERSION_BITS
#undef UUID_RESERVED_BITS
#undef IS_OLD_UUID
#undef EmptyArg
#undef UUID_VERIFY_INIT
#undef CHECK_STRUCTURE
#undef bCHECK_STRUCTURE
#undef vCHECK_STRUCTURE
#undef rCHECK_STRUCTURE
#undef uuid_c_os_base_time_diff_lo
#undef uuid_c_os_base_time_diff_hi
#undef UUID_C_100NS_PER_SEC
#undef UUID_C_100NS_PER_USEC
#undef UADD_UVLW_2_UVLW
#undef UADD_UW_2_UVLW
#undef IFR_NEXT