// 
// system/typeinfo.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef SYSTEM_TYPEINFO_H
#define SYSTEM_TYPEINFO_H

#if RSG_ORBIS
#include <stdint.h>		// for size_t
#endif

#ifdef _CPPRTTI	// Predefined by MSC, defined manually on PS3
# define SafeCast(type,ptr)	::rage::smart_cast<type*>(ptr)
# include <typeinfo>
# define GetRttiName(ptr)	typeid(*ptr).name()
namespace rage {
#if __XENON
typedef type_info type_info;
#elif __WIN32PC
#if _HAS_EXCEPTIONS
	typedef ::std::type_info type_info;
#endif
#else
typedef ::std::type_info type_info;
#endif
} // namespace rage
#else
# define SafeCast(type,ptr)	static_cast<type*>(ptr)
# define GetRttiName(ptr)	NULL
#endif

namespace rage
{

template<class Val, int ALIGNMENT>
inline Val aligned_cast( void* ptr) { AssertMsg(((size_t)ptr&(ALIGNMENT -1 ))==0,"A ptr needs to be align so this operation is incorrect"); return reinterpret_cast<Val>(ptr); }




//PURPOSE
// Determines if a type is a pointer.
template< typename T >
struct typeinfoIsPointerType
{
	static const int Value = 0;
};

template< typename T >
struct typeinfoIsPointerType<T*>
{
	static const int Value = 1;
};

//PURPOSE
//  Used in place of static_cast but where you want to make sure
//  it's a valid cast using dynamic_cast in debug builds.
//NOTES
//  smart_cast does not work with reference types.
template< typename T, typename U >
inline
T smart_cast( U u )
{
	//smart_cast only works with pointer types.
	CompileTimeAssert( typeinfoIsPointerType< T >::Value == 1 );
#if __ASSERT && defined(_CPPRTTI)
	FastAssert(static_cast< T >( u ) == dynamic_cast< T >( u ));
#endif
	return static_cast< T >( u );
}

//PURPOSE
//  Similar to smart_cast except verify_cast will also assert 
//  if the result of the cast is a NULL pointer.
//NOTES
//  verify_cast does not work with reference types.
template< typename T, typename U >
inline
T verify_cast( U u )
{
	//verify_cast only works with pointer types.
	CompileTimeAssert( typeinfoIsPointerType< T >::Value == 1 );
#if __ASSERT
	T t = smart_cast< T >( u ); FastAssert( t ); return t;
#else  //__ASSERT
	return smart_cast< T >( u );
#endif //__ASSERT
}

// PURPOSE
//	Casting via a union. The 'To' and 'From' types should be of the same size (it CompileTimeAssert()'s for this).
//	Added as a strict-aliasing helper (needed for gcc 4.1.1 when -fno-strict-aliasing is not specified).
//
// NOTES
//
//	'To' should be a pointer, if the intent is to fix strict aliasing (though this function works for casting normal types).
//	'To' needs to be explicitly specified, but 'From' need not be (although it should be a pointer as well, if 'To' is!).
//	Usage example:
//		int* i = ...;
//		float* f;
//		f = union_cast<float*>( i ); // This eliminates the "strict-aliasing rule" warning that an equivalent reinterpret_cast gives.
template <typename To, typename From>
__forceinline
To union_cast( From fromPtr )
{
	CompileTimeAssert( sizeof(From) == sizeof(To) );
	// The return value will either be partly undefined, or the input will be partly truncated!

	union
	{
		From fr;
		To to;
	} Temp;
	Temp.fr = fromPtr;
	return Temp.to;
}

}   //namespace rage

#endif  //SYSTEM_TYPEINFO_H
