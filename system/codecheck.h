// 
// system/codecheck.h 
// 
// Copyright (C) 1999-2007 Rockstar Games.  All Rights Reserved. 
// 

#ifndef SYSTEM_CODECHECK_H 
#define SYSTEM_CODECHECK_H 

#if __WIN32
#include <sal.h>
#endif

namespace rage {


//PURPOSE : Handy macro to reset everything to zero in a POD
#define SETTOZERO( a )   memset( a, 0, sizeof( a ))


#if __ASSERT

// add more return value handlers
template<class T>  inline bool codecheckRetVal( T a ) 
{
#if !__PPU && !__SPU && !RSG_ORBIS
	CompileTimeAssert(0 && "Cannot Return a value without a Return value handler" );
#else
	// unused template code is still executed on the gcc so we have to use an Assert
	Assert( 0 && "Cannot Return a value without a Return value handler" );
#endif
	return false;
}
template<class T> inline bool codecheckRetVal( T* a ) 
{
	return a != NULL;
}
template<> inline  bool codecheckRetVal( int a )
{
	return a == 0;
}
template<> inline bool codecheckRetVal( bool a ) {	return a; }

template<class T> inline  T CheckReturnValue( T val , const char* msg )
{
	if ( !rage::codecheckRetVal( val ) ) 
	{
		AssertMsg( false, msg );
	}
	return  val;
}

//PURPOSE : Asserts if the return value doesn't match a successful return value
//
// REMARKS: This only works with ignoreable asserts, or emailable asserts.
//
// <GROUP codecheck>
#define AssertRetVal( a ) CheckReturnValue( a , #a )

//PURPOSE : if assert fails then will not do scoped block afterwards
// REMARKS: This only works with ignoreable asserts, or emailable asserts.
//
// <GROUP codecheck>
#define If_Assert(a ) Assert( a ) ; if ( (a) ) 
#define If_Assertf(a, ...) Assertf(a, __VA_ARGS__) ; if ( (a) 

//PURPOSE : if assert fails then will not do scoped block afterwards
// REMARKS: These are like If_Assert and If_Assertf, but the if is executed even in non __ASSERT builds
//			This is handly for protecting against run time overflows.
//
// <GROUP codecheck>
#define If_Always_With_Assert(a )		Assert( a ) ; if ( (a) ) 
#define If_Always_With_Assertf(a, ...)	Assertf(a, __VA_ARGS__) ; if ( (a) )

//PURPOSE : if assert fails then will do the following command
// EXAMPLE : 
//  <CODE> If_NotAssert( ptr , return ); // if ptr is null in assert builds it will return
// </CODE>
// <GROUP codecheck>
#define If_NotAssert(a , b)  Assert( a ) ; if ( !(a) ) { b ;} 


//PURPOSE : if assert fails then will return false
// <GROUP codecheck>
#define AssertReturn(a )	If_NotAssert(a , return false )



// PURPOSE: Asserts if the return value to a function is not checked.
// FASTRETURNCHECK only works on PS3, and is done entirely at compile time and
// is therefore more suitable for accessors and vector library functions.
// <GROUP codecheck>
#if __PS3
#define RETURNCHECK( a )		a  __attribute__ ((warn_unused_result))
#else
#define RETURNCHECK( a )		sysErrorReturn<a>
#endif
// PURPOSE : Error return value checker.
// DESCRIPTION : Will fire an assert if the value is not checked. This is achieved by 
// overloaded the conversion operator so when the class is converted to the given value 
// it will not assert in the destructor. Also checking return values.
//
// EXAMPLE : Change you function to be
//  <CODE> sysErrorReturn<bool>	CheckReturnValueFunction();
//  ...
//  CheckReturnValueFunction();		// Will Assert Here
//  if ( CheckReturnValueFunction() ) 
//	{
//		return;		// Will not assert
//	}
// </CODE>
// <GROUP codecheck>
template<class T>
class sysErrorReturn
{
	T    m_errorVal;
	bool m_checked;

public:
	sysErrorReturn( const T& errorVal ) : m_errorVal( errorVal ), m_checked(false )
	{}
	~sysErrorReturn()
	{
		AssertMsg( m_checked , "Didn't check return value from function" );
	}
	operator T() 
	{
		m_checked = true;
		return m_errorVal;
	}
};

#else  // __ASSERT

#define AssertReturn(a )

#define If_Always_With_Assert(a )		if ( (a) ) 
#define If_Always_With_Assertf(a, ...)	if ( (a) )

#define If_Assert(a )			
#define If_Assertf(a, ...)
#define AssertRetVal( a )		a 
#define If_NotAssert( a , b)		
#define RETURNCHECK( a )		a
#endif // __ASSERT

#if RSG_PS3 || RSG_ORBIS
#define FASTRETURNCHECK( a )	a  __attribute__ ((warn_unused_result))
#else // __PS3
#define FASTRETURNCHECK( a )	a
#endif // __PS3

#if __PS3
#define NOT_REACHED
#elif defined(_MSC_VER)
#define NOT_REACHED __assume(0)
#elif defined(__has_builtin) && __has_builtin(__builtin_unreachable)
#define NOT_REACHED __builtin_unreachable()
#endif

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// Win32 Static Analysis Macros
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#if __WIN32

#define BEGIN_SUPPRESS_SA_WARNING(x)		\
	CompileTimeAssert(x >= 6000 && x <= 6999); /* This macro should only be used for static analysis warnings. */ \
	__pragma(warning(push));		\
	__pragma(warning(disable: x));	
#define END_SUPPRESS_SA_WARNING()	\
	__pragma(warning(pop));			

#else

#define BEGIN_SUPPRESS_SA_WARNING(x)
#define END_SUPPRESS_SA_WARNING()

// If you use SAL annotations, #define them to nothing here so other builds still work.
#define _In_ 
#define _In_opt_
#define _Out_
#define _Out_opt_
#define _Ret_
#define _Ret_opt_

#endif


} // namespace rage

#endif // SYSTEM_CODECHECK_H 
