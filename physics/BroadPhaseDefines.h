#ifndef _BROADPHASE_DEFINES_H_
#define _BROADPHASE_DEFINES_H_

namespace rage
{


#define BPHASH_SCRATCHOFFSET 0x70000000
#define BPHASH_RECOMMENDED_SCRATCHPAD_BYTE_SIZE 4092*4  // this # seems to give decent enough hashing values

	//#define BPHASH_HASHTABLE_SIZE 1024*64  //!me figure out how many handles this will support
#define BPHASH_MIN_HASHTABLE_SIZE 2039 //16381
#define BPHASH_MAX_HASHTABLE_SIZE 2039 //16381

#define BPHASH_ZUP 0

// 16 bit keys give you twice as many entries into the same sized hash table, but 
// the extra level of indirection costs you a little performance
#define BPHASH_16BITKEYS

// in powers of two, the scaling factor between grid levels ( i.e. set to 2 to make smaller grid cell a quarter bigger grid )
#define BPHASH_SCALESHIFT 2
// = 2^BPHASH_SCALESHIFT as float
#define BPHASH_SCALE 4.0f

#ifdef BPHASH_16BITKEYS
	typedef unsigned short tHashEntry;
	#define BPHASH_NULL 		0xffff
	#define BPHASH_NULL32 		0xffffffff
	// the most you can fit on the scratchpad with 16 bit keys before filling up the hash table
	#define BPHASH_MAX_ENTRIES  1800
#else
typedef unsigned int tHashEntry;

	// a good value for null since a collision would only happen when at max grid position of coarsest resolution.
	// if you have a coordinate like this you are set up totally wrong anyhow 
	#define BPHASH_NULL 		0xffffffff 
	#define BPHASH_NULL32		BPHASH_NULL
	
#endif

}

#endif

