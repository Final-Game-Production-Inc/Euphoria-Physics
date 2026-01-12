// 
// data/safestruct.h 
// 
// Copyright (C) 1999-2007 Rockstar Games.  All Rights Reserved. 
// 

#ifndef DATA_SAFESTRUCT_H 
#define DATA_SAFESTRUCT_H 

#include "struct.h"
#include "system/nelem.h"

namespace rage {

//PURPOSE : Performs safe structure definitions for resourcing using compile time asserts and type traits
//
// REMARKS:
// Works in only on windows and xbox at the current time.
//
// Currently checks for all these at compile time
//		* Padding at the end
	//		* incorrect order of members
	//		* gaps in structure
	//		* Base class not specified
	//		* Given base class is not an actual base class
	//		* 2D arrays, arrays, and virtual pointer fields are all pointers
	//		* all other members are not
	//		* checks that contained arrays are actually contained arrays
	//		* reports size differences of expected and actual sizes to help correct errors
	//
	//		uses empty classes to give decent error messages.
	//
	//
	//	EXAMPLE: use just like normal structure definations SSTRUCT_BEGIN, SSTRUCT_FIELD_* , SSTRUCT_END . Note no semicolons should be placed at
	//			the ends of the lines and the class type should be specified on each field.
	//
	//	<CODE>
	//		SSTRUCT_BEGIN_BASE( mySmallClass, myBaseClass)
	//			 SSTRUCT_FIELD(mySmallClass, firstValue )
	//			 SSTRUCT_FIELD(mySmallClass, secondValue )
	//			 SSTRUCT_CONTAINED_ARRAY( mySmallClass, paddArray )
	//		SSTRUCT_END
	//</CODE>
	//  Notice how there are no semicolons and the type is repeated in each macro
	//
#if !__PS3 && !__PSP2 && !__64BIT && !HAS_PADDED_POINTERS

	template<class T> struct VP_IS_NOT_A_POINTER;
	template<class T> struct VP_IS_NOT_A_POINTER<T*>{};

	template<class T> VP_IS_NOT_A_POINTER<T> IsPointer(const T&) { return  VP_IS_NOT_A_POINTER<T>();}


	template<class T> struct FIELDS_SHOULD_NOT_BE_A_POINTER{};
	template<class T> struct FIELDS_SHOULD_NOT_BE_A_POINTER<T*>;

	template<class T> FIELDS_SHOULD_NOT_BE_A_POINTER<T> IsNotAPointer(const T&) { return  FIELDS_SHOULD_NOT_BE_A_POINTER<T>();}

	template<bool Result, class Message> struct STATIC_ASSERTION_FAILURE;
	template<class MESSAGE> struct STATIC_ASSERTION_FAILURE<true,MESSAGE>{};

	template<int ExpectedSize_Minus_ActualSize, class Message> struct STATIC_SIZE_MISMATCH;
	template<class MESSAGE> struct STATIC_SIZE_MISMATCH<0,MESSAGE>{};

	template<bool, class> struct INITAL_OFFSET_IS_INCORRECT_BASE_CLASS_MAY_NOT_BE_DEFINED;

	template<class MESSAGE> struct INITAL_OFFSET_IS_INCORRECT_BASE_CLASS_MAY_NOT_BE_DEFINED<true,MESSAGE>{};

	struct GAP_IN_RESOURCE_FOUND {};
	struct UNEXPECTED_PADDING_AT_THE_END {};
	struct COMPILE_TIME_ASSERT_FAILED {};
	struct BASE_CLASS_IS_NOT_CORRECT {};
	struct CANT_CAST_TO_TYPE_WITH_DIFFERENT_SIZE {};

#define OFFSET_OF( S, M )		OffsetOf( S, M)

#define SSTRUCT_IS_BASE_OF( BASECLASS, TYPE )	__is_base_of( BASECLASS, TYPE )



#define SSTRUCT_START_EL( TYPE, A ) OFFSET_OF( TYPE, A ), ::rage::GAP_IN_RESOURCE_FOUND >);  
#define SSTRUCT_END_EL( TYPE, A ) sizeof( ::rage::STATIC_SIZE_MISMATCH<  ( OFFSET_OF( TYPE, A) + sizeof( A )) -


#define SSTRUCT_BEGIN_BASE( TYPE, BASECLASS)	 sizeof(::rage::STATIC_ASSERTION_FAILURE<  SSTRUCT_IS_BASE_OF( BASECLASS, TYPE ), ::rage::BASE_CLASS_IS_NOT_CORRECT>); STRUCT_BEGIN( TYPE); sizeof( ::rage::INITAL_OFFSET_IS_INCORRECT_BASE_CLASS_MAY_NOT_BE_DEFINED< sizeof(BASECLASS) ==
#define SSTRUCT_BEGIN_BASE2( TYPE, BASECLASS1, BASECLASS2 )	 \
	sizeof(::rage::STATIC_ASSERTION_FAILURE<  SSTRUCT_IS_BASE_OF( BASECLASS1, TYPE ), ::rage::BASE_CLASS_IS_NOT_CORRECT>);\
	sizeof(::rage::STATIC_ASSERTION_FAILURE<  SSTRUCT_IS_BASE_OF( BASECLASS2, TYPE ), ::rage::BASE_CLASS_IS_NOT_CORRECT>); \
	STRUCT_BEGIN( TYPE); \
	sizeof( ::rage::INITAL_OFFSET_IS_INCORRECT_BASE_CLASS_MAY_NOT_BE_DEFINED< sizeof(BASECLASS1) + sizeof(BASECLASS2) ==
#define SSTRUCT_BEGIN( TYPE)	STRUCT_BEGIN( TYPE); sizeof( ::rage::INITAL_OFFSET_IS_INCORRECT_BASE_CLASS_MAY_NOT_BE_DEFINED< __has_virtual_destructor(TYPE) * sizeof(void*) ==
#define SSTRUCT_FIELD(TYPE, A)    SSTRUCT_START_EL( TYPE,A);   sizeof(::rage::IsNotAPointer( A )); STRUCT_FIELD(A); SSTRUCT_END_EL( TYPE, A )
#define SSTRUCT_FIELD_VP(TYPE, A)    SSTRUCT_START_EL( TYPE,A);   sizeof(::rage::IsPointer( A )); STRUCT_FIELD_VP(A); SSTRUCT_END_EL( TYPE, A )
#define SSTRUCT_FIELD_AS(TYPE, A, NEWTYPE) SSTRUCT_START_EL( TYPE,A);  sizeof(::rage::STATIC_ASSERTION_FAILURE< sizeof(A) == sizeof(NEWTYPE), ::rage::CANT_CAST_TO_TYPE_WITH_DIFFERENT_SIZE>); STRUCT_FIELD(*reinterpret_cast<NEWTYPE*>(&A)); SSTRUCT_END_EL( TYPE, A )
#define SSTRUCT_END(TYPE)		sizeof( TYPE), ::rage::UNEXPECTED_PADDING_AT_THE_END >); STRUCT_END();

#define SSTRUCT_CONTAINED_ARRAY_COUNT(TYPE,field,ct)			SSTRUCT_START_EL( TYPE,field); CHECK_IS_ARRAY(field );  STRUCT_CONTAINED_ARRAY_COUNT(field,ct); SSTRUCT_END_EL( TYPE, field )
#define SSTRUCT_CONTAINED_ARRAY_COUNT_VP(TYPE,field,ct)			SSTRUCT_START_EL( TYPE,field); CHECK_IS_ARRAY(field );  STRUCT_CONTAINED_ARRAY_COUNT_VP(field,ct); SSTRUCT_END_EL( TYPE, field )
#define SSTRUCT_CONTAINED_2D_ARRAY_COUNT( TYPE,field, ct1, ct2) SSTRUCT_START_EL( TYPE, field); sizeof(::rage::IsPointer( field )); STRUCT_CONTAINED_2D_ARRAY_COUNT(field,ct1, ct2); SSTRUCT_END_EL( TYPE, field )
#define SSTRUCT_CONTAINED_2D_ARRAY_COUNT_VP( TYPE,field, ct1, ct2) SSTRUCT_START_EL( TYPE, field); sizeof(::rage::IsPointer( field )); STRUCT_CONTAINED_2D_ARRAY_COUNT_VP(field,ct1, ct2); SSTRUCT_END_EL( TYPE, field )
#define SSTRUCT_CONTAINED_ARRAY(TYPE,field)						SSTRUCT_START_EL( TYPE, field); CHECK_IS_ARRAY(field ); STRUCT_CONTAINED_ARRAY(field); SSTRUCT_END_EL( TYPE, field )
#define SSTRUCT_CONTAINED_ARRAY_VP(TYPE,field)					SSTRUCT_START_EL( TYPE, field); CHECK_IS_ARRAY(field ); STRUCT_CONTAINED_ARRAY_VP(field); SSTRUCT_END_EL( TYPE, field )
#define SSTRUCT_DYNAMIC_ARRAY(TYPE,field,ct)					SSTRUCT_START_EL( TYPE, field); /*sizeof(IsPointer( field ));*/ STRUCT_DYNAMIC_ARRAY(field,ct); SSTRUCT_END_EL( TYPE, field )
#define SSTRUCT_DYNAMIC_ARRAY_NOCOUNT(TYPE,field,ct)			SSTRUCT_START_EL( TYPE, field); /*sizeof(IsPointer( field ));*/ STRUCT_DYNAMIC_ARRAY_NOCOUNT(field,ct); SSTRUCT_END_EL( TYPE, field )
#define SSTRUCT_IGNORE(TYPE,field)								SSTRUCT_START_EL( TYPE, field); STRUCT_IGNORE(field); SSTRUCT_END_EL( TYPE, field )
#define SSTRUCT_SKIP(TYPE, field, ct)							SSTRUCT_START_EL( TYPE, field); STRUCT_SKIP(field, ct); sizeof( ::rage::STATIC_ASSERTION_FAILURE<  ( OFFSET_OF( TYPE, field) + ct) ==

#else

#define SSTRUCT_BEGIN_BASE( TYPE, BASECLASS)	STRUCT_BEGIN( TYPE);
#define SSTRUCT_BEGIN_BASE2( TYPE, BASECLASS1, BASECLASS2 )	 STRUCT_BEGIN( TYPE); 
#define SSTRUCT_BEGIN( TYPE)					STRUCT_BEGIN( TYPE);
#define SSTRUCT_FIELD(TYPE, A)					STRUCT_FIELD(A); 
#define SSTRUCT_FIELD_VP(TYPE, A)				STRUCT_FIELD_VP(A); 
#define SSTRUCT_FIELD_AS(TYPE, A, NEWTYPE)		STRUCT_FIELD(*reinterpret_cast<NEWTYPE*>(&A));
#define SSTRUCT_END(TYPE)						STRUCT_END();

#define SSTRUCT_CONTAINED_ARRAY_COUNT(TYPE,field,ct)			STRUCT_CONTAINED_ARRAY_COUNT(field,ct); 
#define SSTRUCT_CONTAINED_ARRAY_COUNT_VP(TYPE,field,ct)			STRUCT_CONTAINED_ARRAY_COUNT_VP(field,ct);
#define SSTRUCT_CONTAINED_2D_ARRAY_COUNT( TYPE,field, ct1, ct2) STRUCT_CONTAINED_2D_ARRAY_COUNT(field,ct1, ct2); 
#define SSTRUCT_CONTAINED_2D_ARRAY_COUNT_VP( TYPE,field, ct1, ct2) STRUCT_CONTAINED_2D_ARRAY_COUNT_VP(field,ct1, ct2); 
#define SSTRUCT_CONTAINED_ARRAY(TYPE,field)						STRUCT_CONTAINED_ARRAY(field); 
#define SSTRUCT_CONTAINED_ARRAY_VP(TYPE,field)					STRUCT_CONTAINED_ARRAY_VP(field);
#define SSTRUCT_DYNAMIC_ARRAY(TYPE,field,ct)					STRUCT_DYNAMIC_ARRAY(field,ct); 
#define SSTRUCT_DYNAMIC_ARRAY_NOCOUNT(TYPE,field,ct)			STRUCT_DYNAMIC_ARRAY_NOCOUNT(field,ct); 
#define SSTRUCT_IGNORE(TYPE,field)								STRUCT_IGNORE(field);
#define SSTRUCT_SKIP(TYPE, field, ct)							STRUCT_SKIP(field, ct);

#endif

} // namespace rage

#endif // DATA_SAFESTRUCT_H 
