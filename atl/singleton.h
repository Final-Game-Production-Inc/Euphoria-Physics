//
// atl/singleton.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "system/new.h"

#ifndef ATL_SINGLETON_H
#define ATL_SINGLETON_H

#define ATL_SINGLETON_USE_MARKERS 0

#if ATL_SINGLETON_USE_MARKERS
#define DATA_MARKER_INTENTIONAL_HEADER_INCLUDE 1
#include "data/marker.h"
#define ATL_SINGLETON_FUNC() RAGE_FUNC()
#else
#define ATL_SINGLETON_FUNC()
#endif

// PURPOSE : Use this template container class to make any class into a
//               singleton.  Normal usage:
//	
//				 <CODE>
//               class MyClass
//               {
//               public:
//                 // normal class stuff, but don't put ctor/dtor here.
//                 void InitClass();
//                 void ShutdownClass();
//                 int  GetData()         { return _someData; }
//               protected:
//                 // Make the ctor(s)/dtor protected, so this can only be
//                 // instantiated as a singleton.  Note: singleton will still
//                 // work on classes that do not follow this (public ctors)
//                 // but violation of the singleton is possible then, since non-
//                 // singleton versions of the class can be instantiated.
//                 MyClass() : _someData(5) {}
//                 MyClass(int arg) : _someData(arg) {} // etc...
//                 // don't implement the copy constructor, because singletons
//                 // shouldn't be copied!
//                 MyClass(const MyClass& mc) {}
//                 ~MyClass() {}
//               private:
//                 int _someData;
//               };
//
//               // now create a singleton of MyClass
//               typedef atSingleton<MyClass> MyClassSingleton;
//				</CODE>
//
//              Later, in your program code, you can instantiate the
//              singleton and access its members like so:
//
//				<CODE>
//              void somefunc()
//              {
//                // instantiate the MyClassSingleton
//                MyClassSingleton::Instantiate();
//                // could also call MyClassSingleton::Instantiate(10);
//                // since we have a constructor of that form in MyClass.
//
//                // access the methods in MyClass:
//                int data1 = MyClassSingleton::InstancePtr()->GetData();
//                // or...
//                int data2 = MyClassSingleton::InstanceRef().GetData();
//
//                // now destroy the singleton
//                MyClassSingleton::Destroy();
//             }
//				</CODE>
//
template <class _Type>
class atSingleton : protected _Type
{
public:
    //
    // PURPOSE
	//	creates the singleton instance for class _Type.
    //  Assures (by assertion) that the instance will only be created
    //  once.  This works for default constructors
    //
    static void Instantiate()
    {
        FastAssert(!sm_Instance);
		ATL_SINGLETON_FUNC();
        sm_Instance = rage_new atSingleton();
    }

    //------------------------------------------------------------------------------
    // Function    : ResourceInstantiate
    // Description : 
    //------------------------------------------------------------------------------
	//
	// PURPOSE
	//	This is used during resource loading to set the pointer to
	//	an already created object. Can only be used once (unless
	//	the instance is destroyed by calling Destroy()).
	// PARAMS
	//	inst - the instance that is used as the singleton instance
	//
    static void ResourceInstantiate(_Type& inst)
    {
        FastAssert(!sm_Instance);
        sm_Instance = &static_cast<atSingleton<_Type>&>(inst);
    }

	// PURPOSE: Deletes the singleton instance
    static void Destroy()           { delete sm_Instance; sm_Instance = NULL;  }

  	// PURPOSE: returns a pointer/reference to the singleton instance.
	// Access members of the instance as you would members of any
	// object.
	// RETURNS: a pointer to the singleton
    static _Type* InstancePtr()         { FastAssert(sm_Instance); return  sm_Instance; }
	// PURPOSE: returns a pointer/reference to the singleton instance.
	// Access members of the instance as you would members of any
	// object.
	// RETURNS: a reference to the singleton
	static _Type& InstanceRef()         { FastAssert(sm_Instance); return *sm_Instance; }

	// PURPOSE: returns true if an instance of this singleton exists
	// RETURNS: returns true if an instance of this singleton exists
	static bool IsInstantiated()	{ return (sm_Instance!=NULL); }

	// PURPOSE: returns a pointer/reference to the singleton instance.
	// Access members of the instance as you would members of any
	// object.
	// RETURNS: a reference to the singleton
	static _Type& GetInstance()         { return InstanceRef(); }

	//
	// PURPOSE
	//	This might be a source of confusion.  These templatized
	//  functions are used to instantiate classes of type _Type that
	//  have constructors with arguments.  For n arguments, you
	//  to add a function below with n arguments.  Note, these will
	//  only be created if they are used, since they are templates.
	//  I've added 4 below, for 1-4 arguments.  If you get a
	//  compilation error, add one for the number of arguments you
	//  need.  Also need a atSingleton protected constructor with
	//  the same number of arguments.
	// PARAMS
	//	a - constructor argument #1
	//	b - constructor argument #2
	//	c - constructor argument #3
	//	d - constructor argument #4
	//
    template<class _A> static void Instantiate(const _A& a)
    {
        FastAssert(!sm_Instance);
		ATL_SINGLETON_FUNC();
        sm_Instance = rage_new atSingleton(a);
    }
	// <COMBINE Instantiate@_A>
    template<class A, class B>
        static void Instantiate(const A& a, const B& b)
    {
        FastAssert(!sm_Instance);
		ATL_SINGLETON_FUNC();
        sm_Instance = rage_new atSingleton(a, b);
    }
	// <COMBINE Instantiate@_A>
    template<class A, class B, class C>
        static void Instantiate(const A& a, const B& b, const C& c)
    {
        FastAssert(!sm_Instance);
		ATL_SINGLETON_FUNC();
        sm_Instance = rage_new atSingleton(a, b, c);
    }
	// <COMBINE Instantiate@_A>
    template<class A, class B, class C, class D> 
        static void Instantiate(const A& a, const B& b, const C& c, const D& d)
    {
        FastAssert(!sm_Instance);
		ATL_SINGLETON_FUNC();
        sm_Instance = rage_new atSingleton(a, b, c, d);
    }
    
protected:
    // although the instance is of type atSingleton<_Type>, the Instance***() funcs 
    // above implicitly cast it to type _Type.
    static atSingleton* sm_Instance;
    
private:
    //------------------------------------------------------------------------------
    // Function    : atSingleton (default constructor)
    // Description : This is hidden, so that the singleton can only be instantiated
    //               via the public static Instantiate function.
    //------------------------------------------------------------------------------
    atSingleton() : _Type() {}
    
    //------------------------------------------------------------------------------
    // Function    : atSingleton (N-argument constructor)
    // Description : Used by the templatized public Instantiate() functions above.
    //               Create a new one if you need more arguments.
    //
    // TODO! These constructors don't work with the PS2 gnu compiler, that is why
    //       they are commented out.
    //------------------------------------------------------------------------------
  
    template<class A> 
        atSingleton(const A& a) : _Type(a) {}
    template<class A, class B> 
        atSingleton(const A& a, const B& b) : _Type(a, b) {}
    template<class A, class B, class C> 
        atSingleton(const A& a, const B& b, const C& c) : _Type(a, b, c) {}
    template<class A, class B, class C, class D> 
        atSingleton(const A& a, const B& b, const C &c, const D& d) : _Type(a, b, c, d) {}
    
    //------------------------------------------------------------------------------
    // Function    : atSingleton (copy constructor)
    // Description : Hidden, because you can't copy a singleton.
    //------------------------------------------------------------------------------
    atSingleton(const atSingleton&) {}
    
    //------------------------------------------------------------------------------
    // Function    : ~atSingleton
    // Description : Hidden: destroy via the public static Destroy() method.
    //------------------------------------------------------------------------------
    ~atSingleton()                  {}
};

// declare the static instance pointer
template<class _Type> atSingleton<_Type>* atSingleton<_Type>::sm_Instance = NULL;

#endif //ATL_SINGLETON_H
