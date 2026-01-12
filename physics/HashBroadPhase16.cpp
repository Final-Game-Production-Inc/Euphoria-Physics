
#include "HashBroadPhase16.h"

#if ENABLE_UNUSED_PHYSICS_CODE

#include "math/amath.h"
#include "vector/vector3.h"
#include "vector/geometry.h"
//#include "debug.h"

//!me
//#define _BP_DEBUG_

namespace rage {


	// prime[i] is the largest prime smaller than 2^i
#define NUM_PRIMES 31
	static long int s_btPrime[NUM_PRIMES] = {1L,2L,3L,7L,13L,31L,61L,127L,251L,509L,
		1021L,2039L,4093L,8191L,16381L,32749L,65521L,131071L,262139L,
		524287L,1048573L,2097143L,4194301L,8388593L,16777213L,33554393L,
		67108859L,134217689L,268435399L,536870909L,1073741789L};


CHashBroadPhase16::CHashBroadPhase16( int nHashBufferMin, int nHashBufferMax ) // int nHashBuffer = BPHASH_RECOMMENDED_SCRATCHPAD_BYTE_SIZE, unsigned int scratchAddress = BPHASH_SCRATCHOFFSET )
{
	int nHashBuffer = nHashBufferMax;
	for (int i=0; i<NUM_PRIMES; i++) 
	{
		if (s_btPrime[i] >= (nHashBufferMin))
		{	
			nHashBuffer = s_btPrime[i];
			break;
		}
	}

	m_hashTable = rage_new tHashEntry[nHashBuffer];
	m_nHashTable = nHashBuffer;

//nHashBuffer = 4096*4;
//	m_hashTable = (tHashEntry *)(new unsigned char[nHashBuffer]);
//	m_nHashTable = nHashBuffer/sizeof( tHashEntry );


	/*
	// performance dies when scratch full for some reason, so keep a little lower
	if( nHashBuffer >= BPHASH_RECOMMENDED_SCRATCHPAD_BYTE_SIZE )
	{
	nHashBuffer = BPHASH_RECOMMENDED_SCRATCHPAD_BYTE_SIZE;
	}

	m_nHashTable = nHashBuffer/sizeof( tHashEntry );

	m_hashTable = (tHashEntry *)(scratchAddress);		
	*/
}


	// initialize values, clear the hashtable
void CHashBroadPhase16::init( float width, float xOrigin, float yOrigin, unsigned int gridLevels, int startGridResolution )
{
	m_width = width;
	m_xO = xOrigin;
	m_yO = yOrigin;
	m_hashLevels = gridLevels;

	//!me this is fucked!
/*	{
		unsigned int iRez = 0;
		int levelScale = 1;
		while( iRez < m_hashLevels )
		{
			m_cellWidthRecip[iRez] = 1.0f/(smallestGridResolution*levelScale);
			levelScale = levelScale << BPHASH_SCALESHIFT;
			iRez++;
		}
	}
*/

	

				{	//smallestGridResolution = smallestGridResolution;
		//			int gridsPerWidth = int(width/smallestGridResolution);
		//			Assert( gridsPerWidth > 0 );
		//			int startGridResolution = gridsPerWidth >> (BPHASH_SCALESHIFT*(gridLevels));
		//			Assert( startGridResolution > 0 );
		//			startGridResolution = startGridResolution << BPHASH_SCALESHIFT;
	int iRez = m_hashLevels-1;
	while( iRez >= 0 )
	{
	m_cellWidthRecip[iRez] = startGridResolution/width;
	iRez--;
	startGridResolution = startGridResolution << BPHASH_SCALESHIFT;
	}
	}
	

	//*

	//			for( int iH = 0; iH < m_nHashTable; iH++ )
	//			{
	//				m_hashTable[ iH ] = 0xdead;
	//			}


	// trim set so it's an integer amount of quadwords
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


int CHashBroadPhase16::queryBucket( tHashEntry value, unsigned int x, unsigned int y, unsigned int level, CCollisionPair *queryHits, unsigned int maxHits, CCollisionPair *previousCellHits, unsigned int &idxEnd ) const
{
	unsigned int idx = getHashIndex( x, y, level );
	
	unsigned int nHit = 0;


#ifdef _BP_DEBUG_	
	unsigned int wrapcheck = 0;
#endif
	
	while( !isEmpty( idx ) )
	{
	
#ifdef _BP_DEBUG_	
		if( wrapcheck++ == m_nHashTable )
		{
			AssertMsg( 0, "ERROR BEEP BEEP WHOOOP WHOOOP!!! *******hash table full*********\n" );
			return 0;
		}
#endif				
	
		const tHashEntry valB = getValue( idx );
	
		if( valB != value )
		{
			const AABBEntity *entB = getEntity( valB ); 
			//Vector3 minB = *((const Vector3 *)(&(entB->m_aabb.m_min)));
			//Vector3 maxB = *((const Vector3 *)(&(entB->m_aabb.m_max)));
			const AABB &aabbB = entB->m_aabb;
			const bool iAlreadyHit = alreadyHit( entB, previousCellHits, (int)(queryHits - previousCellHits) );
			if( !iAlreadyHit )
			{
			
				const AABBEntity *entA = getEntity( value ); 
				const AABB &aabbA = entA->m_aabb;

//!me			if( !filter( entA, entB ) )
				{
//					Vector3 minA = *((const Vector3 *)(&(entA->m_aabb.m_min)));
//					Vector3 maxA = *((const Vector3 *)(&(entA->m_aabb.m_max)));
//					if( geomBoxes::TestAABBtoAABB( minA, maxA, minB, maxB ) )

					if( aabbA.testOverlap( aabbB ) )
					{
						// add hit
						if( nHit >= maxHits )
						{
							break;
						}
						
						queryHits->m_a = entA;
						queryHits->m_b = entB;
						queryHits++;
						nHit++;
					}				
				}
			}	
		}
		idx = incrementHashIndex(idx);
	}
		
	idxEnd = idx;
		
	return nHit;
	
}


int CHashBroadPhase16::queryBucketNoAabbCheck( unsigned int x, unsigned int y, unsigned int level, CCollisionPair *queryHits, unsigned int maxHits, CCollisionPair *previousCellHits ) const
{
	unsigned int idx = getHashIndex( x, y, level );
	
	unsigned int nHit = 0;


#ifdef _BP_DEBUG_	
	unsigned int wrapcheck = 0;
#endif
	
	while( !isEmpty( idx ) )
	{
	
#ifdef _BP_DEBUG_	
		if( wrapcheck++ == m_nHashTable )
		{
			AssertMsg( 0, "ERROR BEEP BEEP WHOOOP WHOOOP!!! *******hash table full*********\n" );
			return 0;
		}
#endif				
	
		const tHashEntry valB = getValue( idx );
	
		const AABBEntity *entB = getEntity( valB ); 
		const bool iAlreadyHit = alreadyHit( entB, previousCellHits, (int)(queryHits - previousCellHits) );
		if( !iAlreadyHit )
		{
		
			// add hit
			if( nHit >= maxHits )
			{
				break;
			}
			
			queryHits->m_a = NULL;
			queryHits->m_b = entB;
			queryHits++;
			nHit++;
		}	

		idx = incrementHashIndex(idx);
	}
		
	return nHit;
	
}


int CHashBroadPhase16::insertBucket( tHashEntry value, unsigned int x, unsigned int y, unsigned int level, CCollisionPair *queryHits, unsigned int maxHits, CCollisionPair *previousCellHits )
{

	unsigned int idx;
	unsigned int nHit;// = queryBucket( value, x, y, level, queryHits, maxHits, previousCellHits, idx, true );

	{
		idx = getHashIndex( x, y, level );
		
		nHit = 0;


	#ifdef _BP_DEBUG_	
		unsigned int wrapcheck = 0;
	#endif
		
		while( !isEmpty( idx ) )
		{
		
	#ifdef _BP_DEBUG_	
			if( wrapcheck++ == m_nHashTable )
			{
				AssertMsg( 0, "ERROR BEEP BEEP WHOOOP WHOOOP!!! *******hash table full*********\n" );
				return 0;
			}
	#endif				
		
			const tHashEntry valB = getValue( idx );
		
			if( valB != value )
			{
				const AABBEntity *entB = getEntity( valB ); 
				const AABB &aabbB = entB->m_aabb;
				//Vector3 minB = *((const Vector3 *)(&(entB->m_aabb.m_min)));
				//Vector3 maxB = *((const Vector3 *)(&(entB->m_aabb.m_max)));
				const bool iAlreadyHit = alreadyHit( entB, previousCellHits, (int)(queryHits - previousCellHits) );
				if( !iAlreadyHit )
				{
				
					const AABBEntity *entA = getEntity( value ); 
					const AABB &aabbA = entA->m_aabb;

//!me				if( !filter( entA, entB ) )
					{
					//	Vector3 minA = *((const Vector3 *)(&(entA->m_aabb.m_min)));
					//	Vector3 maxA = *((const Vector3 *)(&(entA->m_aabb.m_max)));
					//	if( geomBoxes::TestAABBtoAABB( minA, maxA, minB, maxB ) )

						if( aabbA.testOverlap( aabbB ) )
						{
							// add hit
							if( nHit >= maxHits )
							{
								break;
							}
							
							queryHits->m_a = entA;
							queryHits->m_b = entB;
							queryHits++;
							nHit++;
						}				
					}
				}	
			}
			else 
			{
				// we have encountered the same key we are querying.  
				// for insertion we will want to stop here, so we don't have duplicate keys in a hash collision situation
				break;
			}
			idx = incrementHashIndex(idx);
		}
					
	}
	
	writeValue( value, idx );
	
	return nHit;
	
}



int CHashBroadPhase16::add( tHashEntry valueA, unsigned int rez, CCollisionPair *queryHits, int queryHitsBuffSize )
{
	const AABBEntity *aabb = getEntity( valueA );
	const AABB &aabbA = aabb->m_aabb;
	int nPairs = 0;

	AssertMsg( aabb->m_aabb.m_min.GetX() < aabb->m_aabb.m_max.GetX(), "bad aabb added into broadphase" );
	AssertMsg( aabb->m_aabb.m_min.GetY() < aabb->m_aabb.m_max.GetY(), "bad aabb added into broadphase" );
	AssertMsg( aabb->m_aabb.m_min.GetZ() < aabb->m_aabb.m_max.GetZ(), "bad aabb added into broadphase" );

	Rect rect;
	getXYrange( aabbA, rez, &rect );

	CCollisionPair* prevCellHits = queryHits;
		
	int iY = rect.iYa;		
	while( iY <= rect.iYb )
	{
	
		int iX = rect.iXa;
		while( iX <= rect.iXb )
		{
			int nHits = 0;
			
			// gather all hash collisions for this cell
			nHits = insertBucket( valueA, iX, iY, rez, queryHits, queryHitsBuffSize, prevCellHits );

			queryHits += nHits;
			queryHitsBuffSize -= nHits;
			nPairs += nHits;
			 
			iX++;
		}	

		iY++;
	}			
	
	rez++;
	
	
	// check each level of this or higher resolution
	while( rez < m_hashLevels )
	{
		
		// a change in resolution = half the number of cells
		rect.iYa = rect.iYa >> BPHASH_SCALESHIFT;
		rect.iYb = rect.iYb >> BPHASH_SCALESHIFT;
		rect.iXa = rect.iXa >> BPHASH_SCALESHIFT;
		rect.iXb = rect.iXb >> BPHASH_SCALESHIFT;

		int iY = rect.iYa;		
		while( iY <= rect.iYb )
		{
		
			int iX = rect.iXa;
			while( iX <= rect.iXb )
			{
				int nHits = 0;
				
				// gather all hash collisions for this cell
				unsigned int idxEnd;
				nHits = queryBucket( valueA, iX, iY, rez, queryHits, queryHitsBuffSize, prevCellHits, idxEnd );

				queryHits += nHits;
				queryHitsBuffSize -= nHits;
				nPairs += nHits;
				 
				iX++;
			}	

			iY++;
		}			

		rez++;

	}

	return nPairs;
}




int CHashBroadPhase16::queryAabb( tHashEntry valueA, unsigned int rez, CCollisionPair *queryHits, int queryHitsBuffSize )
{
	int nPairs = 0;
	
	const AABBEntity *aabb = getEntity( valueA );
	const AABB &aabbA = aabb->m_aabb;
	// we actually insert a pointer to the element into the hashtable.  This allows us to get the AABBs cheaply.

	AssertMsg( aabb->m_aabb.m_min.GetX() < aabb->m_aabb.m_max.GetX(), "bad aabb queried into broadphase" );
	AssertMsg( aabb->m_aabb.m_min.GetY() < aabb->m_aabb.m_max.GetY(), "bad aabb queried into broadphase" );
	AssertMsg( aabb->m_aabb.m_min.GetZ() < aabb->m_aabb.m_max.GetZ(), "bad aabb queried into broadphase" );


	Rect rect;
	getXYrange( aabbA, rez, &rect );

	CCollisionPair* prevCellHits = queryHits;
		
	// check each level of this or higher resolution
	while( rez < m_hashLevels )
	{

		int iY = rect.iYa;		
		while( iY <= rect.iYb )
		{
		
			int iX = rect.iXa;
			while( iX <= rect.iXb )
			{
				int nHits = 0;
				
				// gather all hash collisions for this cell
				unsigned int idxEnd;
				nHits = queryBucket( valueA, iX, iY, rez, queryHits, queryHitsBuffSize, prevCellHits, idxEnd );

				queryHits += nHits;
				queryHitsBuffSize -= nHits;
				nPairs += nHits;
				 
				iX++;
			}	

			iY++;
		}			


		// a change in resolution = half the number of cells
		rect.iYa = rect.iYa >> BPHASH_SCALESHIFT;
		rect.iYb = rect.iYb >> BPHASH_SCALESHIFT;
		rect.iXa = rect.iXa >> BPHASH_SCALESHIFT;
		rect.iXb = rect.iXb >> BPHASH_SCALESHIFT;
		rez++;

	}
	
	return nPairs;
}


int CHashBroadPhase16::queryAabbNoInsert( AABBEntity *aabb, unsigned int rez, CCollisionPair *queryHits, int queryHitsBuffSize )
{
	tHashEntry tempEntry = getHashEntry( aabb );

	int nHits = queryAabb( tempEntry, rez, queryHits, queryHitsBuffSize );
	
	popHashEntry();
	
	return nHits;
}


void CHashBroadPhase16::insert( tHashEntry valueA )
{

#if 0
static bool bFirsttest0 = false;
if( bFirsttest0 )
{
bFirsttest0 = false;

BPVector va, vb;
DEBUGLOG( "%f : ", m_width );
for( int i = 0; i < m_hashLevels; i++ )
{
	DEBUGLOG( "%f, ", 1.0f/m_cellWidthRecip[i] );

}

DEBUGLOG( "\n" );

/*
va.x = -99.78600f;
va.y = -161.9730f;
va.z = 27.61945f;
vb.x = -104.7860f;
vb.y = -161.9730f;
vb.z = 27.61945f;
*/

va.x = m_xO + 10.0f;
va.y = m_yO + 10.0f;
vb.x = m_xO + 15.0f;
vb.y = m_yO + 10.0f;

DEBUGLOG( "(%f, %f)=====>(%f, %f)\n", va.x, va.y, vb.x, vb.y );
queryRay( va, vb, 0, NULL, 0 );



va.x = m_xO;
va.y = m_yO;
vb.x = m_xO+0.0001f;
vb.y = m_yO+0.0001f;
DEBUGLOG( "(%f, %f)=====>(%f, %f)\n", va.x, va.y, vb.x, vb.y );
queryRay( va, vb, 0, NULL, 0 );

DEBUGLOG( "(%f, %f)=====>(%f, %f)\n", vb.x, vb.y, va.x, va.y );
queryRay( vb, va, 0, NULL, 0 );

va.x = m_xO;
va.y = m_yO;
vb.x = m_xO+129.0f;
vb.y = m_yO+0.0001f;
DEBUGLOG( "(%f, %f)=====>(%f, %f)\n", va.x, va.y, vb.x, vb.y );
queryRay( va, vb, 0, NULL, 0 );
DEBUGLOG( "(%f, %f)=====>(%f, %f)\n", vb.x, vb.y, va.x, va.y );
queryRay( vb, va, 0, NULL, 0 );


va.x = m_xO + 12.0f;
va.y = m_yO + 15.0f;
vb.x = va.x + 16.0f;
vb.y = vb.y + 16.0f;
DEBUGLOG( "(%f, %f)=====>(%f, %f)\n", va.x, va.y, vb.x, vb.y );
queryRay( va, vb, 0, NULL, 0 );
DEBUGLOG( "(%f, %f)=====>(%f, %f)\n", vb.x, vb.y, va.x, va.y );
queryRay( vb, va, 0, NULL, 0 );


va.x = m_xO + 30.0f;
va.y = m_yO + 14.0f;
vb.x = va.x + 16.0f;
vb.y = vb.y + 16.0f;
DEBUGLOG( "(%f, %f)=====>(%f, %f)\n", va.x, va.y, vb.x, vb.y );
queryRay( va, vb, 0, NULL, 0 );
DEBUGLOG( "(%f, %f)=====>(%f, %f)\n", vb.x, vb.y, va.x, va.y );
queryRay( vb, va, 0, NULL, 0 );


va.x = m_xO + 12.1f;
va.y = m_yO + 12.1f;
vb.x = va.x + 8.0f;
vb.y = va.y - 8.0f;
DEBUGLOG( "(%f, %f)=====>(%f, %f)\n", va.x, va.y, vb.x, vb.y );
queryRay( va, vb, 0, NULL, 0 );
DEBUGLOG( "(%f, %f)=====>(%f, %f)\n", vb.x, vb.y, va.x, va.y );
queryRay( vb, va, 0, NULL, 0 );

}
#endif

	const AABBEntity *aabb = getEntity( valueA );	
	const AABB &aabbA = aabb->m_aabb;

	AssertMsg( aabb->m_aabb.m_min.GetX() < aabb->m_aabb.m_max.GetX(), "bad aabb inserted into broadphase" );
	AssertMsg( aabb->m_aabb.m_min.GetY() < aabb->m_aabb.m_max.GetY(), "bad aabb inserted into broadphase" );
	AssertMsg( aabb->m_aabb.m_min.GetZ() < aabb->m_aabb.m_max.GetZ(), "bad aabb inserted into broadphase" );

	int rez = m_hashLevels-1;
	rez = calcResolutionIncrease( aabb->m_aabb, rez );

	Rect rect;
	getXYrange( aabbA, rez, &rect );
		
	int iY = rect.iYa;		
	while( iY <= rect.iYb )
	{
	
		int iX = rect.iXa;
		while( iX <= rect.iXb )
		{			
			
#ifdef _BP_DEBUG_	
	unsigned int wrapcheck = 0;
#endif			
			
			unsigned int idx = getHashIndex( iX, iY, rez );
			while( !isEmpty( idx ) && getValue(idx) != valueA )
			{
			
#ifdef _BP_DEBUG_	
		if( wrapcheck++ == m_nHashTable )
		{
			AssertMsg( 0, "ERROR BEEP BEEP WHOOOP WHOOOP!!! *******hash table full*********\n" );
			return;
		}
#endif				
			
				idx = incrementHashIndex(idx);
			}

			m_hashTable[idx] = valueA;
			iX++;
		}	

		iY++;
	}			
			
}		


void CHashBroadPhase16::removeLastAdded( tHashEntry valueA )
{

	const AABBEntity *aabb = getEntity( valueA );	
	const AABB &aabbA = aabb->m_aabb;

	int rez = m_hashLevels-1;
	rez = calcResolutionIncrease( aabb->m_aabb, rez );

	Rect rect;
	getXYrange( aabbA, rez, &rect );
		
	int iY = rect.iYa;		
	while( iY <= rect.iYb )
	{
	
		int iX = rect.iXa;
		while( iX <= rect.iXb )
		{			
			
#ifdef _BP_DEBUG_	
	unsigned int wrapcheck = 0;
#endif			
			
			unsigned int idx = getHashIndex( iX, iY, rez );
			while( m_hashTable[idx] != valueA && m_hashTable[idx] != BPHASH_NULL )
			{
			
//DEBUGLOG( "iHash (%d):%d\n", valueA, idx );

#ifdef _BP_DEBUG_	
		if( wrapcheck++ == m_nHashTable )
		{
			AssertMsg2( 0, "ERROR BEEP BEEP WHOOOP WHOOOP!!! *******couldn't find requested hash element %x*********\n", valueA );
			return;
		}
#endif				
			
				idx = incrementHashIndex(idx);
			}

//			DEBUGLOG( "remove (%d):%d\n", valueA, m_hashTable + idx );
			m_hashTable[idx] = BPHASH_NULL;
			iX++;
		}	

		iY++;
	}			
			
#if 0 && defined(_BP_DEBUG_)
	int i;
	for( i = 0; i < m_nHashTable; i++ )
	{
		AssertMsg( m_hashTable[i] != valueA, "hash key not removed: %d @ %d\n", valueA, i );
	}

#endif


}		


int CHashBroadPhase16::queryRay( const BPVector &vecStart, const BPVector &vecEnd, unsigned int rez, CCollisionPair *pairsOut, int pairsOutBuffSize ) const
{

	float fx = vecStart.GetX();
	float fy = vecStart.GetY();
	const float fxe = vecEnd.GetX();
	const float fye = vecEnd.GetY();

	int x = getBucketX( rez, fx );
	int y = getBucketY( rez, fy );
	
	const float dx = (fxe-fx);
	const float dy = (fye-fy);

	float dxR;
	float dyR;
	
	if( dx == 0.0f )
	{
		dxR = 0.0f;
	}
	else
	{
		dxR = 1.0f/dx;
	}
	
	if( dy == 0.0f )
	{
		dyR = 0.0f;
	}
	else
	{
		dyR = 1.0f/dy;
	}

	int xe = getBucketX( rez, fxe );
	int ye = getBucketY( rez, fye );

	float gridSlopeAdjustmentX;
	float gridSlopeAdjustmentY;
	float errorX;
	float errorY;
	
	if( x == xe )
	{
		if( dx < 0.0f )
			errorX = (x)*(1.0f/m_cellWidthRecip[rez]) + m_xO - fx;
		else
			errorX =(x + 1.0f )*(1.0f/m_cellWidthRecip[rez]) + m_xO - fx;
		
		gridSlopeAdjustmentX = 0.0f;
	}
	else
	{
		gridSlopeAdjustmentX = ( x < xe ) ? 1.0f : -1.0f;
		errorX = (x + (gridSlopeAdjustmentX+1.0f)*0.5f )*(1.0f/m_cellWidthRecip[rez]) + m_xO - fx;
	}
	
	if( y == ye )
	{
		if( dy < 0.0f )
			errorY = (y)*(1.0f/m_cellWidthRecip[rez]) + m_yO - fy;
		else
			errorY = (y + 1.0f)*(1.0f/m_cellWidthRecip[rez]) + m_yO - fy;
		gridSlopeAdjustmentY = 0.0f;
	}
	else
	{
		gridSlopeAdjustmentY = ( y < ye ) ? 1.0f : -1.0f;
		errorY = (y + (gridSlopeAdjustmentY+1.0f)*0.5f )*(1.0f/m_cellWidthRecip[rez]) + m_yO - fy;
	}
	
	const int iXadj = int(gridSlopeAdjustmentX);
	const int iYadj = int(gridSlopeAdjustmentY);
	
	
	int nHits = 0;
	bool done = false;

	CCollisionPair *previousCellHits = pairsOut;

	int xp = -1; // forces a check of all resolution levels for first time
	int yp = -1;

	do
	{
		if( x == xe && y == ye )
		{
			done = true;
		}
		
		unsigned int thisRez = rez;
		int xs = x;
		int ys = y;
		int xps = xp;
		int yps = yp;
		// only check those cells at a resolution where it has changed from the last check
		while( thisRez < m_hashLevels && 
			 (( xps != xs ) ||
			 ( yps != ys )))
		{
			int thisHits = queryBucketNoAabbCheck( xs, ys, thisRez, pairsOut, pairsOutBuffSize, previousCellHits );
			pairsOut += thisHits;
			nHits += thisHits;
			pairsOutBuffSize -= thisHits;
			
	//		DEBUGLOG( "%d, %d, %d\n", xs, ys, thisRez );
			
			xs = xs >> BPHASH_SCALESHIFT;
			ys = ys >> BPHASH_SCALESHIFT;
			xps = xps >> BPHASH_SCALESHIFT;
			yps = yps >> BPHASH_SCALESHIFT;
			thisRez++;
		}
		
		xp = x;
		yp = y;
		
		// see if line will go to next x or y grid cell first.	
		// x = x + t * dx,  y = y + t * dy    want t*dx = errorX  or t*dy = errorY
		// want smaller 't' ( errorX/dx < errorY/dy )  =>  ( dy*errorX < dx*errorY )
		const float dyex = Abs(dy*errorX);
		const float dxey = Abs(dx*errorY);
		if( dyex < dxey )
		{
			if( dx == 0.0f )
			{
				Assert( dy == 0.0f );
				done = true;
			}
			else
			{
				x += iXadj;
				errorY -= errorX*dxR*dy;  // t = errorX/dx 
				errorX = (1.0f/m_cellWidthRecip[rez])*gridSlopeAdjustmentX;
			}

		}
		else
		{
			if( dy == 0.0f )
			{
				Assert( dx == 0.0f );
				done = true;
			}
			else
			{	
				y += iYadj;
				errorX -= errorY*dyR*dx;  // t = errorY/dy
				errorY = (1.0f/m_cellWidthRecip[rez])*gridSlopeAdjustmentY;
			}
		}
		
	}while( !done );

	return nHits;

}

}

#endif // ENABLE_UNUSED_PHYSICS_CODE
