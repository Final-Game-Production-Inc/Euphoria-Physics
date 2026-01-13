// 
// atl/iterativeCombSorter.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef ATL_ITERATIVECOMBSORT_H
#define ATL_ITERATIVECOMBSORT_H

#include "math\amath.h"

namespace CombSortGapTable
{
	static const int gapTab[] = {1,1,2,3,4,5,6,7,8,  // 1.247330950103979 1.063708091090
		9,    //*       9.70    1.5423
		11,    //       12.10    1.6818
		13,    //       15.09    1.7877
		17,    //       18.82    1.9284
		23,    //       23.47    2.0376
		29,    //       29.28    2.1011
		37,    //       36.52    2.1582
		47,    //       45.55    2.2063
		59,    //       56.82    2.2473
		71,    //       70.87    2.2811
		89,    //       88.40    2.3215
		109,    //      110.27    2.3559
		137,    //      137.54    2.3919
		173,    //      171.55    2.4242
		211,    //      213.98    2.4504
		269,    //      266.91    2.4797
		331,    //      332.93    2.5026
		419,    //      415.27    2.5267
		521,    //      517.98    2.5469
		647,    //      646.09    2.5663
		809,    //      805.89    2.5850
		1009,    //     1005.21    2.6024
		1249,    //     1253.82    2.6185
		1567,    //     1563.93    2.6346
		1951,    //     1950.74    2.6491
		2437,    //     2433.22    2.6630
		3037,    //     3035.03    2.6759
		3779,    //     3785.69    2.6882
		4723,    //     4722.01    2.7001
		5897,    //     5889.91    2.7113
		7349,    //     7346.67    2.7217
		9161,    //     9163.73    2.7318
		11437,    //    11430.20    2.7414
		14251,    //    14257.24    2.7505
		17783,    //    17783.50    2.7593
		22189,    //    22181.91    2.7676
		27673,    //    27668.18    2.7756
		34511,    //    34511.38    2.7832
		43049,    //    43047.11    2.7905
		53693,    //    53693.99    2.7975
		66973,    //    66974.18    2.8043
		83537,    //    83538.97    2.8108
		104207,    //   104200.74    2.8170
		129971,    //   129972.80    2.8230
		162119,    //   162119.10    2.8288
		202219,    //   202216.17    2.8343
		252233,    //   252230.49    2.8397
		314623,    //   314614.90    2.8449
		392423,    //   392428.90    2.8499
		489487,    //   489488.71    2.8547
		610553,    //   610554.42    2.8594
		761561,    //   761563.43    2.8639
		949931,    //   949921.64    2.8683
		1184867,    //  1184866.66    2.8725
		1477913,    //  1477920.85    2.8766
		1843447,    //  1843456.42    2.8806
		2299397,    //  2299400.25    2.8845
		2868113,    //  2868113.10    2.8882
		3577487,    //  3577486.23    2.8918
		4462309,    //  4462309.30    2.8954
		5565979,    //  5565976.50    2.8988
		6942601,    //  6942614.76    2.9021
		8659741,    //  8659738.26    2.9054
		10801561,    // 10801559.56    2.9085
		13473113,    // 13473119.54    2.9116
		16805447,    // 16805439.00    2.9146
		20961937,    // 20961944.19    2.9175
		26146481,    // 26146481.77    2.9203
		32613319,    // 32613315.95    2.9231
		40679603,    // 40679598.37    2.9258
		50740915,    //*50740922.08    2.9284
		63290713,    // 63290722.55    2.9309
		78944477,    // 78944477.09    2.9334
		98469893,    // 98469889.61    2.9359
		2147483647};
};

/*
template< class COMPARATOR >
void CombKeySorter( Vector4& a, Vector4& b , COMPARATOR& m_compare )
{
	// x is taken to be the key
	Vector4 keyA;
	Vector4 keyB;
	keyA.SplatX( a);
	keyB.SplatX( b );

	Vector4 SelectMask = m_compare( keyA, keyB );//.IsLessThanV( keyB );
	a = SelectMask.Select( a, b );
	b = SelectMask.Select( b, a  );
};
*/
template<class T, class COMPARATOR>
void CombKeySorter( T& a, T& b, COMPARATOR& m_compare )
{
	if ( !m_compare( a, b ))		
	{
		std::swap( a, b );
	}
}
class IterativeCombSorter
{
public:
	IterativeCombSorter() : m_gapIndex( -1 )
	{
	}

	void Start( int size )
	{
		int i = 0;
		while ( CombSortGapTable::gapTab[i] < size )
		{
			i++;
		}
		m_gapIndex = i > 0 ? i-1 : 0;
		m_restartIndex = m_gapIndex / 2;
	}

	template<class T, class COMPARATOR > 
	bool SortPass( T& arr ,  COMPARATOR& m_compare , int size )
	{
		FastAssert( m_gapIndex >= 0 && "Sorting is not setup" );
		bool	sorted = false;
		int gap = CombSortGapTable::gapTab[ m_gapIndex ];

		for ( int i = 0; i < size - gap; i++ )
		{
			CombKeySorter( arr[i], arr[ i + gap ], m_compare );
			//if ( !m_compare( arr[i], arr[i + gap ] ))		
			//{
			//	std::swap( arr[i], arr[i + gap ] );
			//}
		}
		m_gapIndex--;
		if ( m_gapIndex < 0 )
		{
			sorted = true;
			m_gapIndex = m_restartIndex;
		}
		return sorted;
	}
	template<class T, class COMPARATOR > 
	void SortPassNPass( int N, T& arr ,  COMPARATOR& m_compare , int size )
	{
		while ( N-- && !SortPass( arr, m_compare, size ));
	}

	template<class T, class COMPARATOR >
	void Sort( T& arr , COMPARATOR& func , int size ) 
	{
		Start( size );
		while( !SortPass( arr , func, size) );
	}
private:
	int				m_gapIndex;
	int				m_restartIndex;
};

#endif
