// 
// atl/atfunctor.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef ATL_ATFUNCTOR_H
#define ATL_ATFUNCTOR_H

#include "atpreprocessor.h"

/*
    Functors are a type of callback that provide the ability to not only
    call back free functions, but also to call back member functions.

    A functor can be used in almost any situation where a callback can be
    used.

    Functors can be declared for free functions and for member functions that
    return any type and that take any number of arguments.

    EXAMPLE:

    <CODE>
    class TestClass
    {
    public:
        void Func0()                    { Printf( "in Func0\n" ); }
        void Func1( const char* s )     { Printf( "in Func1: %s\n", s ); }
        int  FuncRet1( const char* s )  { Printf( "in FuncRet1: %s\n", s ); return 1; }
    };

    void FreeFunction()
    {
        Printf( "in FreeFunction\n" );
    }

    void main()
    {
        TestClass t;

        // Demonstrates a setup for a functor with no arguments:
        atFunctor0< void > f0;
        f0.Reset< TestClass, &TestClass::Func0 >( &t );

        f0();   // will print "in Func0"

        // Demonstrates a setup for a functor with 1 argument:
        atFunctor1< void, const char* > f1;
        f1.Reset< TestClass, &TestClass::Func1 >( &t );

        f1( "look at me!" );    // will print "in Func1: look at me!"

        // Demonstrates a setup for a functor with 1 argument and a return value:
        atFunctor1< int, const char* > f2;
        f2.Reset< TestClass, &TestClass::FuncRet1 >( &t );

        int retValue = f2( "look at me!" );     // will print "in FuncRet1: look at me!"
        Printf( "retValue: %d", retValue );     // will print "retValue: 1"

        // Demonstrates a setup for a free function functor with no arguments:
        atFunctor0< void > f3;
        f3.Reset< &FreeFunction >();

        f3();   // will print "in FreeFunction"
    }
    </CODE>
*/

#include "delegate.h"

#undef AT_PARAM_COUNT

#define AT_PARAM_COUNT   0
#include "atfunctortemplate.h"
#undef AT_PARAM_COUNT

#define AT_PARAM_COUNT  1
#include "atfunctortemplate.h"
#undef AT_PARAM_COUNT

#define AT_PARAM_COUNT  2
#include "atfunctortemplate.h"
#undef AT_PARAM_COUNT

#define AT_PARAM_COUNT  3
#include "atfunctortemplate.h"
#undef AT_PARAM_COUNT

#define AT_PARAM_COUNT  4
#include "atfunctortemplate.h"
#undef AT_PARAM_COUNT

#define AT_PARAM_COUNT  5
#include "atfunctortemplate.h"
#undef AT_PARAM_COUNT

#define AT_PARAM_COUNT  6
#include "atfunctortemplate.h"
#undef AT_PARAM_COUNT

#define AT_PARAM_COUNT  7
#include "atfunctortemplate.h"
#undef AT_PARAM_COUNT

#define AT_PARAM_COUNT  8
#include "atfunctortemplate.h"
#undef AT_PARAM_COUNT

#endif  //ATL_FUNCTOR_H
