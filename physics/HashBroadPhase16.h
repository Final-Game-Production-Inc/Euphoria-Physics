#ifndef _HashBroadPhase16_
#define _HashBroadPhase16_

#include "phcore/constants.h"
#if ENABLE_UNUSED_PHYSICS_CODE

//#include <stdlib.h>
//#include <stdio.h>
//#include "AM_Classes.h"
//#include "Vector4.h"
#include <algorithm>
#include <stdlib.h>		// for qsort

#include "BroadPhaseDefines.h"

#include "vector/vector3_config.h"

namespace rage
{

//*************

// possible improvements:
//		Change tHashEntry width to 16bits ( i.e. pointers to scratch pad locations )
//		Seperate hash tables for x and y dimensions.
//		Store more pairs, then do one big set of aabb overlap tests ( microcode )
//		Hash table will be invalidated if tHashEntry *values passed in to addBatch is changed ( memory location as well )... be nice if this wasn't the case.  
//*************

// use addBatch with a pre-sorted list for best performance

class CHashBroadPhase16
{
	public:		

//	typedef BPVector BPVector;
		

		class VECTOR_ALIGN BPVector
		{
			public:
				float x,y,z,w;

				inline float GetX() const { return x; }
				inline float GetW() const { return w; }

#if BPHASH_ZUP
				inline float GetY() const { return y; }
				inline float GetZ() const { return z; }
#else
				inline float GetZ() const { return y; }
				inline float GetY() const { return z; }
#endif

		}  
#if (__PS3) && VECTORIZED
		
#endif
		;
		

		
		class AABB
		{
			public:
			
				inline float CalcXYExtent() const
				{
					float ext = m_max.GetX() - m_min.GetX();
					float extY = m_max.GetY() - m_min.GetY();
					if( ext < extY )
					{
						ext = extY;
					}
					
					return ext;
				}
			
				// return true if there is an overlap between aabbs
				inline bool aabbVsAabb( const BPVector& minA, const BPVector& maxA, const BPVector& minB, const BPVector& maxB ) const
				{
					if( maxA.GetX() < minB.GetX() ) return false;
					if( maxA.GetY() < minB.GetY() ) return false;
					if( maxA.GetZ() < minB.GetZ() ) return false;

					if( maxB.GetX() < minA.GetX() ) return false;
					if( maxB.GetY() < minA.GetY() ) return false;
					if( maxB.GetZ() < minA.GetZ() ) return false;
					
					return true;
				}

				inline bool testOverlap( const AABB &aabbT ) const
				{
					return aabbVsAabb( m_min, m_max, aabbT.m_min, aabbT.m_max );
				}
			
				// return true if this AABB completely contains the other AABB
				bool testContains( const AABB &aabbT ) const
				{
					return aabbVsAabb( aabbT.m_max, aabbT.m_min, m_min, m_max );
				}			
			
				BPVector m_min;
				BPVector m_max;
		};
		
		class AABBEntity
		{
			public:
				AABB m_aabb;
				u16 m_value;
		};

		
		//  
		class CCollisionPair
		{
			public:
				const AABBEntity *m_a;
				const AABBEntity *m_b;
				
		};		
		
		
		inline int afloor(float x) const
		{
					
			if( x<0.0f )
				 return int(x)-1;

			return int(x);
		}			
		

		// will attempt to make hashtable size a prime # between the bounds specified
		// size is in # of tHashEntry's
		CHashBroadPhase16( int nHashBufferMin = BPHASH_MIN_HASHTABLE_SIZE, int nHashBufferMax = BPHASH_MAX_HASHTABLE_SIZE ); // int nHashBuffer = BPHASH_RECOMMENDED_SCRATCHPAD_BYTE_SIZE, unsigned int scratchAddress = BPHASH_SCRATCHOFFSET )
	
	
		inline unsigned int incrementHashIndex( unsigned int idx, unsigned int v = 1 ) const
		{
			return (idx + v) % m_nHashTable;
		}
		

		inline tHashEntry getValue( unsigned int idx ) const
		{
			return m_hashTable[idx];
		}


		inline void writeValue( tHashEntry val, unsigned int idx ) const
		{
			m_hashTable[idx] = val;
		}

		
		inline bool isValue( tHashEntry val ) const
		{
			return ( val != BPHASH_NULL ); 
		}
		
		
		inline bool isEmpty( unsigned int idx ) const
		{
			return (getValue(idx) == BPHASH_NULL);
		}		
		
		// initialize values, clear the hashtable
		void init( float width, float xOrigin, float yOrigin, unsigned int gridLevels, int startGridResolution);
		
		inline void clear()
		{
#ifdef BPHASH_16BITKEYS	
			m_nextAabbEntityMap = 1;
			// this way tHashEntries with value null will => null when resolved to AABBEntity
			m_aabbEntityMap[0] = NULL;				
#endif

			for( unsigned int iH = 0; iH < m_nHashTable; iH++ )
			{
				m_hashTable[ iH ] = BPHASH_NULL;
			}

		}

		
		static inline void copyQuad( unsigned char* src, unsigned char* dest, unsigned int nBytes )
		{
		
			for( unsigned int iH = 0; iH < nBytes; iH++ )
			{
				dest[ iH ] = src[ iH ];
			}				

		}
		
		// given a grid position and grid level, return all elements at that location
		int queryBucket( tHashEntry value, unsigned int x, unsigned int y, unsigned int level, CCollisionPair *queryHits, unsigned int maxHits, CCollisionPair *previousCellHits, unsigned int& idx ) const;

		int queryBucketNoAabbCheck( unsigned int x, unsigned int y, unsigned int level, CCollisionPair *queryHits, unsigned int maxHits, CCollisionPair *previousCellHits ) const;

		// given a grid position and grid level, return all elements at that location and insert a new element
		int insertBucket( tHashEntry value, unsigned int x, unsigned int y, unsigned int level, CCollisionPair *queryHits, unsigned int maxHits, CCollisionPair *previousCellHits );

		bool alreadyHit( const AABBEntity *ent, CCollisionPair *previousCellHits, int nPrev ) const
		{
			while( nPrev > 0 )
			{
				if( previousCellHits[nPrev-1].m_b == ent )
				{
					return true;
				}
				nPrev--;
			}
			
			return false;
		}

		// can only increase the resolution, but this is ok, since all our aabbs are added in sorted order with coarsest first.
		inline int calcResolutionIncrease( const AABB &aabb, int currentRez ) const
		{
			float ext = aabb.CalcXYExtent();
			
			while( currentRez > 0 && 1.0f > m_cellWidthRecip[ currentRez ]*ext*BPHASH_SCALE )
			{
				currentRez--;
			}
			
			return currentRez;
		}
		
		inline int getBucketX( int rez, float x ) const
		{
			const float cellWidthRecip = m_cellWidthRecip[ rez ];
			const int iXa = afloor((x - m_xO)*cellWidthRecip);
			
			return iXa;
		}
		
		
		inline int getBucketY( int rez, float y ) const
		{
			const float cellWidthRecip = m_cellWidthRecip[ rez ];
			const int iYa = afloor((y - m_yO)*cellWidthRecip);
			
			return iYa;
		}
		
		struct Rect
		{
			public:
				int iXa, iYa, iXb, iYb;
		} ;

		
		// get the range in bucket indexes the aabb covers for the specified resolution
		inline void getXYrange( const AABB &aabb, int rez, Rect *rect ) const
		{
			
			const float cellWidthRecip = m_cellWidthRecip[ rez ];
		
			int iXa, iXb, iYa, iYb;

			{
				
				const int iXmax = afloor((m_width)*cellWidthRecip);
				
				iXa = afloor((aabb.m_min.GetX() - m_xO)*cellWidthRecip);
				if( iXa < 0 )
				{
					iXa = 0;
				}
 							
				iXb = afloor((aabb.m_max.GetX() - m_xO)*cellWidthRecip);
				if( iXb > iXmax )
				{
					iXb = iXmax;
				}
				
				//				
				// determine y cell range
				//

				const int iYmax = afloor((m_width)*cellWidthRecip);
				
				iYa = afloor(( aabb.m_min.GetY() - m_yO )*cellWidthRecip);
				if( iYa < 0 )
				{
					iYa = 0;
				}
				
				iYb = afloor(( aabb.m_max.GetY() - m_yO )*cellWidthRecip);
				if( iYb > iYmax )
				{
					iYb = iYmax;
				}
				
				rect->iXa = iXa;
				rect->iXb = iXb;
				rect->iYa = iYa;
				rect->iYb = iYb;
			}
		}
		
#ifdef BPHASH_16BITKEYS				

		// get an entry that to be inserted into the broadphase
		inline tHashEntry getHashEntry( AABBEntity *ent )
		{
			 tHashEntry idx = m_nextAabbEntityMap++;
			 m_aabbEntityMap[ idx ] = ent;
			 return idx;
		}
		
		// get an entity from a entry that has been added to the broadphase
		inline AABBEntity *getEntity( tHashEntry hashEntry ) const
		{
			return m_aabbEntityMap[hashEntry];
		} 		
		
		// for queries use this to get rid of a temporary hash entry you may have needed to create for the query
		inline void popHashEntry()
		{
			m_nextAabbEntityMap--;
		}

#else
		
		inline tHashEntry getHashEntry( const AABBEntity *ent ) const
		{
			return (tHashEntry)(ent);
		}
		
		inline AABBEntity *getEntity( tHashEntry hashEntry ) const
		{
			return (AABBEntity *)(hashEntry);
		}
		
		inline void popHashEntry()
		{
		}		 			
#endif

		// add an entry to the broadphase.
		// get the entry with getHashEntry
		int add( tHashEntry entryToAdd, unsigned int rez, CCollisionPair *pairsOut, int pairsOutBuffSize );

		// query the aabb for hits of an entity that has been added to the broadphase
		int queryAabb( tHashEntry entryToQuery, unsigned int rez, CCollisionPair *pairsOut, int pairsOutBuffSize );

		// query the aabb of an entity that has not been inserted in to the broadphase
		int queryAabbNoInsert( AABBEntity *aabb, unsigned int rez, CCollisionPair *pairsOut, int pairsOutBuffSize );


		// the prefered way of using this system.
		// bounding volumes must be sorted by size, largest first and must overlap at least part of the broadphase area.
		inline int addBatchSorted( AABBEntity *sortedBoundingVolumeIn, int nBoundingVolume, CCollisionPair *pairsOut, int pairsOutBuffSize )
		{
			int nPairsOut = 0;
			int rez = m_hashLevels-1;
			int iBV;
			for( iBV = 0; iBV < nBoundingVolume; iBV++ )
			{
				rez = calcResolutionIncrease( sortedBoundingVolumeIn[iBV].m_aabb, rez );
				tHashEntry entry = getHashEntry( sortedBoundingVolumeIn + iBV );
				int newPairs = add( entry, rez, pairsOut, pairsOutBuffSize );
				pairsOut += newPairs;
				nPairsOut += newPairs;
				pairsOutBuffSize -= newPairs;
			}
			
			return nPairsOut;
		}
		
		// must overlap at least part of the broadphase area.
		void insert( tHashEntry entryToInsert );		

		void removeLastAdded( tHashEntry key );

		// get all the entities the ray intersects... seems to be fairly slow... 
		int queryRay( const BPVector &start, const BPVector &end, unsigned int rez, CCollisionPair *pairsOut, int pairsOutBuffSize ) const;

		// the hash function.  I haven't really checked how good it is, but it seems ok ( I've seen some clumping around low #"s though )
		inline unsigned int getHashIndex( int x, int y, int level ) const
		{	
			int n = 0x8da6b343 * x + 0xd8163841 * y +  + 0xcb1ab31f * level; // + 0xcb1ab31f * z;
			n = n % m_nHashTable;
			if (n < 0) n += m_nHashTable;
			return n;

			//return ( (x * 65521) ^ (y * 131071) ^ (level*32749) ) % m_nHashTable;
			//!me need a better hash function!
//			return (( level << 28 ) ^ ( y << 14 ) ^ x ) % m_nHashTable;
		}

//		static int CompareRecordWidth(const AABBEntity & a, const AABBEntity & b)
		static int CompareRecordWidth(const void * a, const void * b)
		{
//			const AABB &aabbA = (a).m_aabb;
//			const AABB &aabbB = (b).m_aabb;
			const AABB &aabbA = ((const AABBEntity*)(a))->m_aabb;
			const AABB &aabbB = ((const AABBEntity*)(b))->m_aabb;
			
			
			float extA = aabbA.CalcXYExtent();
			float extB = aabbB.CalcXYExtent();
			
			if (extA > extB)
				return -1;
			if (extA < extB)
				return +1;
			return 0;
		}

		
		static void sortByWidth( AABBEntity *records, unsigned int nRecords )
		{
		
//			std::sort( records, records + nRecords, CompareRecordWidth );
			qsort( records, nRecords, sizeof( AABBEntity ), CompareRecordWidth );
		}
		
		
		#define MAXHASHLEVELS 7
		
		// important that these two are in same order!  m_x0 then m_y0. should be aligned on 16 byte boundary
		float m_xO;
		float m_yO;
		float m_width;

		unsigned int m_hashLevels;

		tHashEntry *m_hashTable;
		unsigned int m_nHashTable;


		float m_cellWidthRecip[ MAXHASHLEVELS ];
	
#ifdef BPHASH_16BITKEYS	
		// a map between hashentries and entities with aabbs
		AABBEntity *m_aabbEntityMap[BPHASH_MAX_ENTRIES];
		tHashEntry m_nextAabbEntityMap;
#endif
	
} ; // CHashBroadPhase16

}

#endif // ENABLE_UNUSED_PHYSICS_CODE

#endif  // _HashBroadPhase16_

