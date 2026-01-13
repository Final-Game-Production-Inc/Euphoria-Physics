// 
// system/nelem.h 
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
// 

//PURPOSE
//  Gives the number of elements (length) of an array.
//  The array must be declared as an array, not a pointer:
//  int a[ 10 ]; // THIS
//  int* b = a;  // NOT THIS
//
//	Uses templates to give a type safe result
//
//  NELEM( a );  //This works
//  NELEM( b );  //This returns a compile time error
//
// Old version #define NELEM( a ) int( sizeof( a ) / sizeof( ( a )[ 0 ] ) )
#ifndef SYSTEM_NELEM_H
#define SYSTEM_NELEM_H

#include "typeinfo.h"

#define CHECK_IS_ARRAY(x )( 0 * sizeof( ::rage::union_cast<const ::Passed_Argument_to_NELEM_is_not_an_array*>(x) ) +  \
							0 * sizeof( ::Passed_Argument_to_NELEM_is_not_an_array::check_type((x), &(x)) ) )

// implementation details
class Passed_Argument_to_NELEM_is_not_an_array
{
public:
	class Is_pointer;  // intentionally incomplete type
	class Is_array {};  
	template<typename T>
	static Is_pointer check_type(volatile const T*, volatile const T* volatile const*);
	static Is_array check_type(const volatile void*, volatile const void*);
};

// public interface
#define NELEM(x)  int( (CHECK_IS_ARRAY(x )) +  (sizeof(x) / sizeof((x)[0]) )  )

#ifndef COUNTOF
#define COUNTOF NELEM
#endif //COUNTOF

#endif
