// 
// atl/delegate.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef ATL_DELEGATE_H
#define ATL_DELEGATE_H

#include "atl/atpreprocessor.h"
#include "atl/inlist.h"
#include "system/interlocked.h"

/*
    Delegate
    ========
    A delegate (AKA functor) is a type of callback that can call back to
    member functions as well as free functions.

    This implementation of delegate uses some tricks inspired from:
    http://www.codeproject.com/cpp/FastDelegate.asp

    Example of creating and calling a delegate:

    class Foo
    {
    public:
        void DoSomething( int i, float f )
        {
            Displayf( "%d, %f", i, f );
        }
    };

    void TestDelegate()
    {
        //Declare a delegate that takes 2 parameters and returns void.
        atDelegate< void ( int, float ) > delegate;

        //Declare an instance of Foo
        Foo foo;

        //Bind the delegate to an object/member function pair.
        delegate.Bind( &foo, &Foo::DoSomething );

        //This will print the following to standard output:
        //123, 1.25
        delegate( 123, 1.25f );
    }

    Delegator
    =========
    A delegator works a bit like an event in C#.  Delegates can be registered
    with a delegator that has the same signature as the delegate.  When the
    delegator is invoked it will invoke each delegator that is registered with
    it.

    Example of creating and using a delegator:

    class Foo
    {
    public:
        void DoSomething( int i, float f )
        {
            Displayf( "Foo: %d, %f", i, f );
        }
    };

    class Bar
    {
    public:
        void DoSomething( int i, float f )
        {
            Displayf( "Bar: %d, %f", i, f );
        }
    };

    void TestDelegator
    {
        //Declare a delegate that takes 2 parameters and returns void.
        //A delegator's Dispatch() and operator() methods always return
        //void, so it doesn't make sense to declare a delegator that
        //returns anything other then void.
        atDelegator< void ( int, float ) > delegator;

        //Declare a delegates that take 2 parameters and returns void.
        atDelegate< void ( int, float ) > delegate0;
        atDelegate< void ( int, float ) > delegate1;

        Foo foo;
        Bar bar;

        //Bind the delegates to an object/member function pairs.
        delegate0.Bind( &foo, &Foo::DoSomething );
        delegate1.Bind( &bar, &Bar::DoSomething );

        //Register the delegates with the delegator.
        delegator.AddDelegate( &delegate0 );
        delegator.AddDelegate( &delegate1 );

        //This will print the following to standard output:
        //Foo: 123, 1.25
        //Bar: 123, 1.25
        delegator( 123, 1.25f );
    }

*/

namespace rage
{
namespace delegate_detail
{

//This is the type we will use for our generic target pointer.
struct GenericClass
{
    //Simply return a pointer to ourselves.  This is used below in the
    //BindHelper class to determine the actual value of "this" that should
    //be passed to member functions.
    GenericClass* GetThis() { return this; }
};

/*struct MicrosoftVirtualMFP
{
    void ( GenericClass::*codeptr )(); // points to the actual member function
    int delta;		// #bytes to be added to the 'this' pointer
    int vtable_index; // or 0 if no virtual inheritance
};*/

//Removes const/volatile qualification from a type.
template< typename T >
struct RemoveCV { typedef T Type; };

template< typename T >
struct RemoveCV< const T > { typedef T Type; };

template< typename T >
struct RemoveCV< volatile T > { typedef T Type; };

template< typename T >
struct RemoveCV< const volatile T > { typedef T Type; };

template< typename X, typename Y, typename MFTYPE, typename GEN_MFTYPE >
GenericClass* BindHelper( X* that, Y*, MFTYPE memFunc, GEN_MFTYPE& boundFunc )
{
    //This union is used to extract the actual function pointer from the
    //member function pointer.  Member function pointers are often
    //larger than a plain old function pointer because they sometimes
    //(in the cases of multiple or virtual inheritance) contain an offset
    //value that is applied to the "this" pointer before calling the
    //member function.
    //In all compilers that I know of, the actual function pointer is
    //the first value in the structure containing member function data.
#if !__SPU
#pragma pack( push, 1 )
#endif
    union
    {
        MFTYPE memFunc;
        GEN_MFTYPE boundFunc;
        GenericClass* ( Y::*YProbe )();
        GenericClass* ( GenericClass::*GenProbe )();
        //MicrosoftVirtualMFP msMFP;
    } u;
#if !__SPU
#pragma pack( pop )
#endif

    CompileTimeAssert( sizeof( u ) >= sizeof( memFunc ) );

    //Extract the actual function pointer from the member function pointer.
    u.memFunc = memFunc;
    boundFunc = u.boundFunc;

    //Here is where we get the actual value of the "this" pointer that
    //should be passed to the function we extracted above.
    u.GenProbe = &GenericClass::GetThis;
    //In case X is a const type, remove the const qualification so
    //we can call YProbe (a non-const function).
    return ( const_cast< typename RemoveCV< X >::Type* >( that )->*u.YProbe )();

    //We're essentially tricking the compiler into calling a member
    //function that was defined for another class (i.e. GenericClass).
    //Additionally, because the compiler /thinks/ its calling a Y method
    //on an X object, it implicitly handles the cast from a X* to a Y*
    //which, for multiple and/or virtual inheritance, may or may not
    //involve adding an offset to the "this" pointer.
    //You might thing we could simply use a static_cast to convert from X*
    //to Y*, but we can't.
    //There is a very simple way to demonstrate this using Microsoft
    //Visual C++.
    //
    //Declare 4 structs:
    //
    //  struct ClassA{ void FuncA(){} int m_A };
    //  struct ClassB : public virtual ClassA { void FuncB(){} int m_B };
    //  struct ClassC : public virtual ClassA { void FuncC(){} int m_C };
    //  struct ClassD : public ClassB, public Class C { void FuncD(){} int m_D };
    //
    //Now compile and run the following:
    //
    //1.  Delegate0< void > dlgt;
    //2.  ClassD D;
    //3.  dlgt.Bind( &D, &ClassD::FuncC );
    //4.  void ( ClassD::*pmf )() = &ClassD::FuncC;
    //5.  dlgt.Bind( &D, pmf );
    //
    //On line 3 trace into the BindHelper::Bind function.  You'll see that the
    //X template param has resolved to ClassD and the Y template param has
    //resolved to ClassC, the value of u.msMFP.delta is zero.  Becaus X is
    //ClassD and Y is ClassC, the call to ( that->*u.YProbe )() implicitly
    //casts the "this" pointer from a ClassD* to a ClassC*.
    //On line 5 trace into the BindHelper::Bind function.  You'll see that the
    //X template param has again resolved to ClassD, but this time the Y
    //template param has also resolved to ClassD.  There will be no implicit
    //cast (because X and Y are the same class), but now the value of
    //u.msMFP.delta is non-zero and that will be used to offset the "this"
    //pointer.
    //If we had simply used static_cast, this second case (where X and Y are
    //the same class) would have resulted in an incorrect "this" pointer.
}

}   //namespace delegate_detail

template< typename T > class atDelegate;
template< typename T > class atDelegator;
template< typename T > class atClosure;

#undef DLGT_PARAM_COUNT

#define DLGT_PARAM_COUNT    0
#include "delegatetemplate.h"
#undef DLGT_PARAM_COUNT

#define DLGT_PARAM_COUNT    1
#include "delegatetemplate.h"
#undef DLGT_PARAM_COUNT

#define DLGT_PARAM_COUNT    2
#include "delegatetemplate.h"
#undef DLGT_PARAM_COUNT

#define DLGT_PARAM_COUNT    3
#include "delegatetemplate.h"
#undef DLGT_PARAM_COUNT

#define DLGT_PARAM_COUNT    4
#include "delegatetemplate.h"
#undef DLGT_PARAM_COUNT

#define DLGT_PARAM_COUNT    5
#include "delegatetemplate.h"
#undef DLGT_PARAM_COUNT

#define DLGT_PARAM_COUNT    6
#include "delegatetemplate.h"
#undef DLGT_PARAM_COUNT

#define DLGT_PARAM_COUNT    7
#include "delegatetemplate.h"
#undef DLGT_PARAM_COUNT

#define DLGT_PARAM_COUNT    8
#include "delegatetemplate.h"
#undef DLGT_PARAM_COUNT

#define DLGT_PARAM_COUNT    9
#include "delegatetemplate.h"
#undef DLGT_PARAM_COUNT

#define DLGT_PARAM_COUNT    10
#include "delegatetemplate.h"
#undef DLGT_PARAM_COUNT

#define DLGT_PARAM_COUNT    11
#include "delegatetemplate.h"
#undef DLGT_PARAM_COUNT

#define DLGT_PARAM_COUNT    12
#include "delegatetemplate.h"
#undef DLGT_PARAM_COUNT

#define DLGT_PARAM_COUNT    13
#include "delegatetemplate.h"
#undef DLGT_PARAM_COUNT

#define DLGT_PARAM_COUNT    14
#include "delegatetemplate.h"
#undef DLGT_PARAM_COUNT

#define DLGT_PARAM_COUNT    15
#include "delegatetemplate.h"
#undef DLGT_PARAM_COUNT

#define DLGT_PARAM_COUNT    16
#include "delegatetemplate.h"
#undef DLGT_PARAM_COUNT

}   //namespace rage

#endif  //ATL_DELEGATE_H
