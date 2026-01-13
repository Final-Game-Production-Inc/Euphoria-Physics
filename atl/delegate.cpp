// 
// atl/delegate.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#include "delegate.h"

///////////////////////////////////////////////////////////////////////////////
// Unit tests
//
//  Test delegates with a wide variety of inheritance (single, multiple,
//  virtual, etc.).
//  Different types of inheritance result in different actions applied by
//  the compiler during a call to a member function.  For example, with
//  multiple inheritance, the compiler applies an offset to the "this"
//  pointer before calling a base class's member function.
//  Each class has a member function that sets a member variable. After
//  calling the member function through the delegate, the member variable
//  is validated.  This should ensure that the "this" pointer in the delegate
//  is having the proper offsets applied before calling the memeber function.
///////////////////////////////////////////////////////////////////////////////

#define QA_ASSERT_ON_FAIL   0
#include "qa/qa.h"

#if __QA

using namespace rage;

struct ClassA
{
    ClassA()
        : m_A( 321 )
    {
    }

    virtual ~ClassA()
    {
    }

    virtual void FuncA( const int i )
    {
        m_A = i;
    }

    int m_A;
};

struct ClassB : public virtual ClassA
{
    virtual void FuncB( const int i )
    {
        m_B = i;
    }

    int m_B;
};

struct ClassC : public virtual ClassA
{
    ClassC()
        : m_C( 1234 )
    {
    }

    virtual void FuncC( const int i )
    {
        m_C = i;
    }

    int m_C;
};

struct ClassD : public ClassB, public ClassC
{
    virtual void FuncD( const int i )
    {
        m_D = i;
    }

    int m_D;
};

struct ClassF;

struct ClassE : public virtual ClassC
{
    virtual void FuncE( const int i )
    {
        m_E = i;
    }

    virtual void FuncC( const int i )
    {
        m_C = i;
    }

    //Pointer to member function for ClassF.
    typedef void ( ClassF::*PmfF )( const int );

    PmfF m_PmfF;

    int m_E;
};

struct ClassF : public ClassD, public ClassE
{
    void FuncF( const int i )
    {
        m_F = i;
    }

    int m_F;
};

class qa_Delegate : public qaItem
{
public:

	void Init()
    {
    }

    void Shutdown()
    {
    }

	void Update( qaResult& result );
};

class qa_Delegator : public qaItem
{
public:

    typedef atDelegator< void ( const int, const float ) > Delegator;

    struct Foo
    {
        Foo()
        {
            m_Dlgt.Bind( this, &Foo::OnEvent );

            this->Reset();
        }

        void Reset()
        {
            m_I = 0; m_F = 0;
        }

        void OnEvent( const int i, const float f )
        {
            m_I = i; m_F = f;

            //Remove ourself from the delegator while it's iterating.
            m_Dlgt.Cancel();
        }

        Delegator::Delegate m_Dlgt;

        int m_I;
        float m_F;
    };

    struct Bar
    {
        Bar()
        {
            this->Reset();
        }

        void Reset()
        {
            m_I = 0; m_F = 0;
        }

        void OnEvent( const int i, const float f )
        {
            m_I = i; m_F = f;
        }

        int m_I;
        float m_F;
    };

    struct Baz
    {
        Baz()
        {
            this->Reset();
        }

        void Reset()
        {
            m_I = 0; m_F = 0;
        }

        void OnEvent( const int i, const float f )
        {
            m_I = i; m_F = f;
        }

        int m_I;
        float m_F;
    };

	void Init()
    {
    }

    void Shutdown()
    {
    }

	void Update( qaResult& result );
};

#define DLGT_CHECK( t, c, f, v )\
    {\
        Class##t x;\
        atDelegate1< void, const int > dlgt;\
        dlgt.Bind( &x, &Class##c::Func##f );\
        dlgt( v );\
        QA_CHECK( v == x.Class##c::m_##f );\
    }

static int qa_FreeFunc( const int i )
{
    return i + 1;
}

void
qa_Delegate::Update( qaResult& result )
{
    DLGT_CHECK( A, A, A, 100 );

#if !defined(_MSC_VER) || _MSC_VER < 1500
	{
		// this code crashes on XeDK 7879.
        ClassB x;
        atDelegate1< void, const int > dlgt;
        dlgt.Bind( &x, &ClassB::FuncB );
        dlgt( 101 ); // <-- boom!
        QA_CHECK( 101 == x.ClassB::m_B );
	}

	// DLGT_CHECK( B, B, B, 101 );
    DLGT_CHECK( B, B, A, 102 );
    DLGT_CHECK( B, A, A, 103 );

    DLGT_CHECK( C, C, C, 104 );
    DLGT_CHECK( C, C, A, 105 );
    DLGT_CHECK( C, A, A, 106 );

    DLGT_CHECK( D, D, D, 106 );
    DLGT_CHECK( D, D, C, 108 );
    DLGT_CHECK( D, D, B, 109 );
    DLGT_CHECK( D, D, A, 110 );
    DLGT_CHECK( D, C, C, 114 );
    DLGT_CHECK( D, C, A, 115 );
    DLGT_CHECK( D, B, B, 111 );
    DLGT_CHECK( D, B, A, 112 );
    DLGT_CHECK( D, A, A, 113 );

    DLGT_CHECK( E, E, E, 117 );
    DLGT_CHECK( E, E, C, 118 );
    DLGT_CHECK( E, E, A, 119 );
    DLGT_CHECK( E, C, C, 120 );
    DLGT_CHECK( E, C, A, 121 );
    DLGT_CHECK( E, A, A, 122 );

    DLGT_CHECK( F, F, F, 123 );
    DLGT_CHECK( F, F, D, 124 );
    //DLGT_CHECK( F, F, C, 125 );   //This one generates an ambiguous access of C
    DLGT_CHECK( F, F, B, 126 );
    DLGT_CHECK( F, F, A, 127 );
    DLGT_CHECK( F, D, D, 128 );
    //DLGT_CHECK( F, D, C, 129 );   //This one generates an ambiguous access of C
    DLGT_CHECK( F, D, B, 130 );
    DLGT_CHECK( F, D, A, 131 );
    //DLGT_CHECK( F, C, C, 132 );   //This one generates an ambiguous access of C
    //DLGT_CHECK( F, C, A, 133 );   //This one generates an ambiguous access of C
    DLGT_CHECK( F, B, B, 134 );
    DLGT_CHECK( F, B, A, 135 );
    DLGT_CHECK( F, A, A, 136 );
#endif

    //Test using a PMF (pointer to mem func) that is defined before
    //it's target class is defined.
    {
        atDelegate< void ( const int ) > dlgt;
        ClassF F;
        F.m_PmfF = &ClassF::FuncF;
        dlgt.Bind( &F, F.m_PmfF );
        dlgt( 999 );
        QA_CHECK( 999 == F.m_F );
    }

    //Test free functions.
    {
        atDelegate< int ( const int ) > dlgt;
        dlgt.Bind( &qa_FreeFunc );
        QA_CHECK( dlgt( 777 ) == 778 );
    }

    TST_PASS;
}

void
qa_Delegator::Update( qaResult& result )
{
    Delegator delegator;

    //Foo adds itself to the delegator
    Foo foo;
    Bar bar;
    Baz baz;

    Delegator::Delegate dlgtBar;//( &bar, &Bar::OnEvent );
    Delegator::Delegate dlgtBaz;//( &baz, &Baz::OnEvent );
    dlgtBar.Bind( &bar, &Bar::OnEvent );
    dlgtBaz.Bind( &baz, &Baz::OnEvent );

    //Calls Bar::OnEvent().
    delegator.AddDelegate( &dlgtBar );
    foo.Reset(); bar.Reset(); baz.Reset();
    delegator( 1, 2 );
    QA_CHECK( 0 == foo.m_I && 0 == foo.m_F );
    QA_CHECK( 1 == bar.m_I && 2 == bar.m_F );
    QA_CHECK( 0 == baz.m_I && 0 == baz.m_F );

    //Calls Bar::OnEvent().
    //Calls Baz::OnEvent().
    delegator.AddDelegate( &dlgtBaz );
    foo.Reset(); bar.Reset(); baz.Reset();
    delegator( 3, 4 );
    QA_CHECK( 0 == foo.m_I && 0 == foo.m_F );
    QA_CHECK( 3 == bar.m_I && 4 == bar.m_F );
    QA_CHECK( 3 == baz.m_I && 4 == baz.m_F );

    //Calls Foo::OnEvent() which removes itself.  Ensure bar and baz delegates
    //are still invoked.
    delegator.RemoveDelegate( &dlgtBar );
    delegator.RemoveDelegate( &dlgtBaz );
    delegator.AddDelegate( &foo.m_Dlgt );
    delegator.AddDelegate( &dlgtBar );
    delegator.AddDelegate( &dlgtBaz );
    foo.Reset(); bar.Reset(); baz.Reset();
    delegator( 5, 6 );
    QA_CHECK( 5 == foo.m_I && 6 == foo.m_F );
    QA_CHECK( 5 == bar.m_I && 6 == bar.m_F );
    QA_CHECK( 5 == baz.m_I && 6 == baz.m_F );

    //At this point foo has removed itself from the delegator.
    foo.Reset(); bar.Reset(); baz.Reset();
    delegator( 7, 8 );
    QA_CHECK( 0 == foo.m_I && 0 == foo.m_F );
    QA_CHECK( 7 == bar.m_I && 8 == bar.m_F );
    QA_CHECK( 7 == baz.m_I && 8 == baz.m_F );

    delegator.RemoveDelegate( &dlgtBar );
    //Calls Baz::OnEvent() - adds 3 to i and f.
    foo.Reset(); bar.Reset(); baz.Reset();
    delegator( 9, 10 );
    QA_CHECK( 0 == foo.m_I && 0 == foo.m_F );
    QA_CHECK( 0 == bar.m_I && 0 == bar.m_F );
    QA_CHECK( 9 == baz.m_I && 10 == baz.m_F );

    delegator.RemoveDelegate( &dlgtBaz );
    //Calls nothing.
    foo.Reset(); bar.Reset(); baz.Reset();
    delegator( 11, 12 );
    QA_CHECK( 0 == foo.m_I && 0 == foo.m_F );
    QA_CHECK( 0 == bar.m_I && 0 == bar.m_F );
    QA_CHECK( 0 == baz.m_I && 0 == baz.m_F );

    TST_PASS;
}

QA_ITEM_FAMILY( qa_Delegate, (), () );
QA_ITEM_FAMILY( qa_Delegator, (), () );

QA_ITEM( qa_Delegate, (), qaResult::PASS_OR_FAIL );
QA_ITEM( qa_Delegator, (), qaResult::PASS_OR_FAIL );

#endif  //__QA
