// 
// atl/teststlcompliance.cpp
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 
//
// This program is intended as a tester for the SLT compliance of various ATL
// containers as well as a basic example for the use of STL algorithms.


#include <iterator>
#include <algorithm>
#include <vector>

#include "system/main.h"
#include "atl/array.h"


using namespace rage;

template<class _ARRAY_CLASS> void TestIterators(_ARRAY_CLASS &array);
template<class _ARRAY_CLASS> void TestInsertEraseAndClear(_ARRAY_CLASS &array);
template<class _ARRAY_CLASS> void TestSortingAndSearching(_ARRAY_CLASS &array);

void atArrayTest();
void atFixedArrayTest();
void atRangeArrayTest();


const int					numElements=10;


int Main()
{
	atArrayTest();
	atFixedArrayTest();
	atRangeArrayTest();

	return 0;
}

void atArrayTest()
{
	typedef atArray<int>		Integer_atArray;
	Integer_atArray				intAtArray;

	intAtArray.Resize(numElements);

	Displayf("\n\n\natArray:\n");

	TestIterators(intAtArray);
	TestInsertEraseAndClear(intAtArray);
	TestSortingAndSearching(intAtArray);
}

void atFixedArrayTest()
{
	typedef atFixedArray<int, numElements>	Integer_FixedArray;
	Integer_FixedArray			intFixedArray;

	intFixedArray.Resize(numElements);

	Displayf("\n\n\natFixedArray:\n");

	TestIterators(intFixedArray);
	TestInsertEraseAndClear(intFixedArray);
	TestSortingAndSearching(intFixedArray);
}

void atRangeArrayTest()
{
	typedef atRangeArray<int, numElements>	Integer_RangeArray;
	Integer_RangeArray			intRangeArray;

	Displayf("\n\n\natRangeArray:\n");

	TestIterators(intRangeArray);
	TestSortingAndSearching(intRangeArray);
}

/*
	Define a number of helper functions to abstract API differences
	between the various array classes so that the test templates work for
	more than one array type:
*/
template<class _A,int _B,class _C> void ReserveWrapper(atArray<_A,_B,_C> &array, u32 capacity) {	array.Reserve(capacity); }		//Covers the one array that does actually support (and need) reserving).
template<class _ARRAY_CLASS> void ReserveWrapper(_ARRAY_CLASS &/*array*/, u32 /*capacity*/){}	//Do nothing for generic array - covers all fixed size arrays.

template<class _A,int _B> void ResizeWrapper(atRangeArray<_A,_B> &/*array*/, u32 /*count*/) {}	//Makes sure the function does nothing for atRangeArray as it can't be resized.
template<class _ARRAY_CLASS> void ResizeWrapper(_ARRAY_CLASS &array, u32 count){ array.Resize(count); }				//Resize all other arrays.

template<class _A,int _B> void ResetWrapper(atRangeArray<_A,_B> &/*array*/) {}					//Makes sure the function does nothing for atRangeArray as it can't be reset.
template<class _ARRAY_CLASS> void ResetWrapper(_ARRAY_CLASS &array){ array.Reset(); }			//Reset all other arrays.



template<class _ARRAY_CLASS> void TestIterators(_ARRAY_CLASS &array)
{
	const _ARRAY_CLASS			&constArray= array;

	Assert(array.max_size()>=numElements);

	for(int i=0; i<numElements; i++)
		array[i]= i;

	Displayf("\niterating from first to last element using indices:\n");
	for(int i=0; i<numElements; i++)
		Displayf("index: %d, value: %d", i, array[i]);

	Displayf("\niterating from first to last element using forward iterator:\n");
	for(typename _ARRAY_CLASS::iterator it=array.begin(); it!=array.end(); it++)				//NOTE: end() returns iterator pointing just beyond last element.
		Displayf("value: %d", *it);

	Displayf("\niterating from first to last element using const forward iterator:\n");
	for(typename _ARRAY_CLASS::const_iterator it=array.begin(); it!=array.end(); it++)			//Note the != in it!=array.end(). This way the loop works for all kinds of containers. < works only for pointer like iterators and containers like vector. For example < does not work for std::list as there is no guarantee the last node has a higher address than the first node or any other node.
		Displayf("value: %d", *it);


	Displayf("\niterating from last to first element using reverse iterator:\n");
	for(typename _ARRAY_CLASS::reverse_iterator rit=array.rbegin(); rit!=array.rend(); rit++)
		Displayf("value: %d", *rit);

	Displayf("\niterating from last to first element using const reverse iterator:\n");
	for(typename _ARRAY_CLASS::const_reverse_iterator rit=constArray.rbegin(); rit!=constArray.rend(); rit++)
		Displayf("value: %d", *rit);
}


template<class _ARRAY_CLASS> void TestInsertEraseAndClear(_ARRAY_CLASS &array)
{
	array.Reset();
	ReserveWrapper(array, 3);
	array.insert(array.begin(), 3);
	array.insert(array.begin(), 1);
	array.insert(array.begin()+1, 2);
	array.insert(array.end(), 4);																//Causes re-allocation (assuming the container does dynamic allocation) - see ReserveWrapper().

	array.Reset();																				//Force empty array so that next insertion will force re-allocation.
	array.insert(array.begin(), 1);																//Causes re-allocation.

	array.Reset();
	ReserveWrapper(array,2);
	array.insert(array.end(), 1);
	AssertVerify(*(array.insert(array.end(), 3))==3);											//Make sure the returned iterator points to the right element.
	AssertVerify(*(array.insert(array.begin()+1, 2))==2);										//Causes re-allocation. Also make sure the returned iterator points to the right element.
	array.insert(array.end(), 4);

	array.Reset();
	ReserveWrapper(array,1);
	array.insert(array.begin(), 2);
	AssertVerify(*(array.insert(array.begin(), 1))==1);											//Causes re-allocation. Also make sure the returned iterator points to the right element.

	array.Reset();
	ReserveWrapper(array,3);
	array.insert(array.begin(), 3, 2);
	array.insert(array.begin(), 2, 1);															//Causes re-allocation.
	array.insert(array.end(), 2, 3);															//Causes re-allocation.

	typedef std::vector<int>	Integer_vector;
	Integer_vector				intStdArray;

	intStdArray.push_back(1);
	intStdArray.push_back(2);
	intStdArray.push_back(3);

	array.Reset();
	ReserveWrapper(array,6);
	array.insert(array.begin(), intStdArray.begin(), intStdArray.end());
	array.insert(array.begin()+1, intStdArray.begin()+1, intStdArray.end());

	array.Reset();
	array.insert(array.begin(), intStdArray.begin(), intStdArray.end());						//Causes re-allocation.

	_ARRAY_CLASS				array2;
	array2.insert(array2.begin(), array.begin(), array.end());									//Do the random access iterators work properly with insert?

	array.Reset();
	array.insert(array.begin(), intStdArray.begin(), intStdArray.end());
	array.insert(array.begin(), intStdArray.begin(), intStdArray.end());
	array.erase(array.begin());																	//from the beginning
	array.erase(array.end()-1);																	//from the end
	array.erase(array.begin()+2);																//somewhere in the middle

	array.Reset();
	array.insert(array.begin(), intStdArray.begin(), intStdArray.end());
	array.insert(array.begin(), intStdArray.begin(), intStdArray.end());
	array.insert(array.begin(), intStdArray.begin(), intStdArray.end());
	array.erase(array.begin(), array.begin()+2);												//from the beginning
	array.erase(array.end()-2, array.end());													//from the end
	array.erase(array.begin()+2, array.begin()+4);												//somewhere in the middle

	array.clear();
}


template<class _ARRAY_CLASS> void TestSortingAndSearching(_ARRAY_CLASS &array)
{
	_ARRAY_CLASS				array2;

	ResetWrapper(array);
	ResizeWrapper(array, numElements);
	int							index= 0;
	array[index++]=4;
	array[index++]=7;
	array[index++]=2;
	array[index++]=3;
	array[index++]=8;
	array[index++]=1;
	array[index++]=9;
	array[index++]=0;
	array[index++]=5;
	array[index++]=6;
	Assert(index==numElements);

	typename _ARRAY_CLASS::iterator		result;
	result= std::find(array.begin(), array.end(), 9);											//find() operates on unsorted data.
	AssertVerify(result!=array.end() && *result==9);

	std::sort(array.begin(), array.begin()+ numElements/2);										//Sort first half of array.
	std::sort(array.begin()+ numElements/2, array.end());										//Sort second half of array.

	ResizeWrapper(array2, numElements);															//std::merge() expects the destination range to be already existent.
	std::merge(array.begin(), array.begin()+ numElements/2, array.begin()+ numElements/2, array.end(), array2.begin());	//Merge 2 sorted ranges into one sorted range. In this case the 2 ranges are the first and second half of the same array.
	std::reverse(array2.begin(), array2.end());													//Reverses the contents of the container.
	std::random_shuffle(array2.begin(), array2.end());											//Randomizes the elements of the container.

	std::sort(array.begin(), array.end());														//Sort whole thing.

	AssertVerify(std::binary_search(array.begin(), array.end(), 9));							//Looks for the existence of the value in the range.
	AssertVerify(std::binary_search(array.begin(), array.end(), 11)==false);

	result= std::lower_bound(array.begin(), array.end(), 7);									//Search within ordered range. Actually searches for the first element that's equal or greater than 7.
	AssertVerify(result!=array.end() && *result==7);

	result= std::upper_bound(array.begin(), array.end(), 4);									//Search within ordered range. Searches for the first element that's greater than 4.
	AssertVerify(result!=array.end() && *(result-1)==4);
}

