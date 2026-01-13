//
// atl/ptr.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef ATL_PTR_H
#define ATL_PTR_H


#include "referencecounter.h"

#include "data/struct.h"

namespace rage {

#if __WIN32
    #pragma warning(disable: 4786)	                                            //identifier was truncated to '255' characters in the debug information message is suppressed
#endif

/*
PURPOSE

This template implements smart pointers to classes derived from
atReferenceCounter.

REMARKS
* What is a smart pointer? *

A smart pointer is a little wrapper class around a regular pointer. Its
purpose is to make reference counting easier and safer. Whenever something
gets assigned to a smart pointer its reference count is increased and when
the smart pointer goes out of scope or is assigned a new value the reference
counter is automatically decreased. This makes it virtually impossible to get
reference counting wrong or to forget about it. Due to the design of the
wrapper class it can be used just like a regular pointer. All the necessary
operators (like *, -> or =) are overloaded to make it as transparent as possible.
Only when declaring the pointer and when freeing an object you will notice
the difference.

* What are smart pointers good for? *

The obvious advantage is the easier and safer use of reference counting. No
more danger of missing calls to AddRef() or Release(). 
But there is more to it. Since smart pointers are complete classes with
constructors and destructors they are perfect candidates to be used with type
save containers (e.g. lists, arrays, hash tables, ...). Whenever an element
is added to or deleted from the container the reference counter of the object
the smart pointer points to is automatically updated. When deleting the
container there is no need to iterate through it and to manually free its
elements. Copying all elements from one array to an other can be as simple as
using the assignment operator of the container - the reference counters are
automatically updated.

* What about the memory overhead? *

The smart pointer class consists of about a dozen tiny functions, mostly one
liners, so that instanciating the template is very cheap. A few lines of code
sprinkled throughout a program to achive a similar result are most likely just
as or even more expensive.
An actual instance of a smart pointer takes exactly as much or as little
memory as a regular pointer. This is possible, because the only data member of
a smart pointer is a pointer and the class has no virtual functions.

* And the performance overhead? *

The implementation below was choosen because it focuses on as little overhead
as possible. Still there is some CPU overhead. Increasing and decreasing the
reference counter is exactly the same as doing it manually, but it's likely to
happen more often - it completely depends on how the smart pointers are used -
see below. Dereferencing the object a smart pointer points to is usually one
indirection more than with a regular pointer (depends on the compiler). So
it's some extra cost but very little and often completely neglectable compared
to the cost of calling and executing a function. Still you may notice it in
an inner loop.

*So there is a performance cost - can it be avoided?*

Yes! In most cases it's good enough if one member variable of a class or a
temporary variable on the stack is a smart pointer. This ensures that the
reference stays valid as long as a class exists or a function is executed.
That being said, there is no reason not to declare a regular pointer and
assign the smart pointer's contents to it. The regular pointer can then be
used within an inner loop while the smart pointer keeps track of the
references. This method combines the safety of smart pointers with the full
performance of regular pointers.

* Anything else I should be aware of? *

This implementation requires that the class the smart pointer points to is
derived from atReferenceCounter (or at least features functions AddRef()
and Release() with identical behaviour). There are smart pointer
implementations that work with any arbitrary data type (even basic data
types like int), but come with significantly more overhead. www.boost.org
offers such an implementation and also a performance comparison of various
implementation strategies.
Be aware of cyclic references. If object A references object B and vice versa
these objects will never be automatically freed unless one of the references
is manually released. This sort of memory leak is sometimes hard to find.
Therefore atReferenceCounter offers some debug functionality to make it
easier. It is a good idea to start using it when you start using smart
pointers - a new leak is much easier to find.




EXAMPLES

<CODE>
    atPtr<Sample> sptr  = rage_new Sample;     
</CODE>
    
sptr is now a pointer to Sample and Sample knows there is a pointer
to it. If sptr goes out of scope or is assigned to a new object Sample
knows that there is one reference less and if there is no
further reference to Sample it is deleted automatically.

It is possible to use sptr like a regular pointer:

<CODE>
    sptr->CallSomeMethod();
    variable= sptr->variable;
    sampleInstance= *sptr;
    sptr= rage_new Sample;
    sptr= &sampleInstance;
    if(sptr == &sampleInstance)
        ;
</CODE>


A good way to make the usage of a class easier is to define nested types for
smart pointers to the class.

<CODE>
    class Widget : public atReferenceCounter
    {
    public:
        typedef atPtr<Widget>       SPtr;       //Smartpointer to a Widget
        typedef atPtr<const Widget> SPtrToConst;//Smartpointer to a const Widget
    }
</CODE>


The user of Widget can now write

<CODE>
    Widget::SPtr            mySmartPointer;
</CODE>

to declare a smart pointer to Widget. This is especially helpful when using
atDerivedPtr and atDerivedPtrToConst.



PARAMS

_Type       - The type of class you want to define a smart pointer for.



NOTES

The delete operator is intentionally not overloaded because it would not act
the way you expect (the object would not be deleted but the reference count
would be decreased instead). If you want to release the reference to an
object you can assign 0 to the smartpointer, call its method free() or simply
let the smartpointer go out of scope.

You must not delete a reference counted object.



SEE ALSO
    atReferenceCounter, atDerivedPtr, atDerivedPtrToConst
<FLAG Component>
*/
template <typename _Type> class atPtr
{
public:
    typedef _Type           ValueType;                                          //A synonym for the template parameter _Type.
    typedef _Type*          ValuePtrType;
    typedef atPtr       SPtrType;


    /*
    PURPOSE
        Default constructor.
    */
/*    atPtr() : p(0)
    {
    }
*/
    /*
    PURPOSE
        Constructor that takes a normal pointer as argument.
    */
    atPtr(_Type* p_=0) : p(p_)
    {
        if(p != 0)
            p->AddRef();
    }    

    /*
    PURPOSE
        Copy constructor - takes a smartpointer (of any related type) as
                        argument.
    */
/*    template<typename Y> atPtr(const atPtr<Y> &p_) : p(p_.p)
    { 
        if(p != 0)
            p->AddRef();
    }*/

    /*
    PURPOSE
        Copy constructor - takes a smartpointer as argument
    */
    atPtr(const atPtr<_Type> &p_) : p(p_.p)
    {
        if(p != 0)
            p->AddRef();
    }

    /*
    PURPOSE
        Destructor: releases the reference to the object the smartpointer
        points to.

    NOTES
        This destructor is intentionally not virtual. The assumption is that
        the only classes derived from this class are the ones defined below
        and that these do not have any data members that must be freed. Not
        having a single virtual function means an instance of this class does
        only need as much memory as a regular pointer!
    */
    ~atPtr()
    {
        if(p != 0)
            p->Release();
    }

    /*
    PURPOSE
        Allows to access the stored pointer itself.

    EXAMPLES
        Ptr= (Object_type *)SPtr;      //assign to a regular pointer


        if(SPtr)                        //test if a pointer is assigned or not
            ....
    */
    operator _Type*() const
    { 
        return p; 
    }    

    /*
    PURPOSE
        Allows to access the value the smartpointer points to (just like with
        a regular pointer).

    EXAMPLE
        *SPtr= newValue;
    */
    _Type& operator*()
    {
        return *p;
    }


    /*
    PURPOSE
        Overloads the -> operator to work like it does with a normal pointer.
    */
    _Type* operator->() const
    {
        return p; 
    }
   
    
    /*
    PURPOSE
        Assign one smartpointer to another.
    */
    atPtr& operator=(const atPtr<_Type> &p_)
    {
        return assign(p_.p);
    }

    /*
    PURPOSE
        Allows to assign a normal pointer to the smartpointer.
    */
    atPtr& operator=(_Type* p_)
    {
        return assign(p_);
    }

    
    /*
    PURPOSE
        Assign one smartpointer (of any related type) to another
    */
/*    template<typename Y> atPtr& operator=(const atPtr<Y> &p_) 
    { 
        return assign(p_.p);
    }
*/
    /*
    PURPOSE
        Allows to compare a smartpointer with a normal pointer like both would
        be normal pointers.
    */
/*    bool operator==(_Type* p_)
    {
        return p == p_;
    }*/

    /*
    PURPOSE
        Allows to compare two smartpointers.
    */
/*    inline friend bool operator==(const atPtr<_Type> &First, const atPtr<_Type> &Second)
    {
        return First.p == Second.p;
    }*/

    /*
    PURPOSE
        In case the "real" pointer is on the left.
    */
/*    inline friend bool operator==(const _Type *pPtr, const atPtr<_Type> &Second)
    {
        return pPtr == Second.p;
    }*/

    /*
    PURPOSE
        Is it a NULL pointer ?
    */
    inline bool IsNull()
    {
        return (bool)(p==NULL);
    }

    /*
    PURPOSE
        Test if one smartpointer is smaller than the other.
        This is important for ordered data containers.
    */
    inline friend bool operator<(const atPtr<_Type> &First, const atPtr<_Type> &Second)                 //define comparison operators to allow the usage with stl or atl containers
    {
        if(First.p < Second.p)
            return true;
        else
            return false;
    }

    /*
    PURPOSE
        Test if one smartpointer is greater than the other.
        This is important for ordered data containers.
    */
    inline friend bool operator>(const atPtr<_Type> &First, const atPtr<_Type> &Second)                 //define comparison operators to allow the usage with stl or atl containers
    {
        if(First.p > Second.p)
            return true;
        else
            return false;
    }


    /*
    PURPOSE
        != test between two smartpointers.
    */
    bool operator!=(const atPtr<_Type> &p_)
    {
        if(p != p_.p)
            return true;
        else
            return false;
    }

    /*
    PURPOSE
        != test between a smartpointer and a normal pointer.
    */
    bool operator!=(_Type* p_)
    {
        if(p != p_)
            return true;
        else
            return false;
    }

    /*
    PURPOSE
        Frees this reference to the object. The object will be deleted if its
        reference count equals 0. This smart pointer is set to NULL.
        If the pointer is already null nothing will happen.

    NOTES
        Use this method instead of delete (or simply let the smartpointer go
        out of scope).
    */
    void free()
    {
        operator= ((_Type*)0);
    }





protected:
    /*
    PURPOSE
        Assigns the smartpointer a new value and takes care of reference counting.
    */
    inline atPtr& assign(_Type* p_)
    {
        if(p_ != p)
        {
            if(p != 0)
                p->Release();

            p = p_;

            if(p != 0)
                p->AddRef();
        }

        return *this;
    }

public:
//protected:
    _Type*                  p;                                                  //Stores the actual pointer.
};


/*
PURPOSE
	Simple wrapper to allow classes starting with a reference count of 1 to work with atPtr
*/
template<class T>
atPtr<T>	atCreateResourceAtPtr( T* resoucePtr )
{
	atPtr<T>	p( resoucePtr );
	resoucePtr->Release();
	return p;
}


/*
PURPOSE
    A smart pointer template for dealing with derived classes.

REMARKS
    Use this template if you:

    * have a base class and a smart pointer belonging to it.
    * have a derived class you want to define a smart pointer for
    * want the smart pointer to the derived class to know it is also a
        pointer to the base class.
    * can afford a little bit of extra memory and cpu usage.

    A hierarchy of smart pointer classes is build which matches the class
    hierachy they point to. This way the smart pointers can be used more
    like normal pointers and a lot of casting can be avoided.


PARAMS
    _Type                      - The class for which to define a smart pointer.
    _TSPtrToParentType       - Smart pointer to the class  is derived from.


EXAMPLES

<CODE>
    class DerivedWidget : public Widget
    {
    public:
        // Smartpointer to this class
        typedef atDerivedPtr<DerivedWidget, Widget::SPtr>                             SPtr;

        // Smartpointer to a const of this class
        typedef atDerivedPtrToConst<DerivedWidget, Widget::SPtr, Widget::SPtrToConst> SPtrToConst;
    };
</CODE>

    The example assumes Widget is directly or indirectly derived from
    atReferenceCounter and that Widget defines nested smart pointer types in a
    similar way as the example above itself does.


SEE ALSO
    atReferenceCounter, atPtr, atDerivedPtrToConst
<FLAG Component>
*/
template <typename _Type, typename _TSPtrToParentType> class atDerivedPtr : public _TSPtrToParentType
{
public:
    typedef     _Type       ValueType;                                          //A synonym for the template parameter _Type.
    typedef     _Type*      ValuePtrType;

    atDerivedPtr() : _TSPtrToParentType(/*(typename _TSPtrToParentType::ValuePtrType)0*/)
    {
    }

    atDerivedPtr(_Type* p_) : _TSPtrToParentType((typename _TSPtrToParentType::ValuePtrType)p_)
    {
    }    

    atDerivedPtr(const atDerivedPtr &p_) : _TSPtrToParentType((typename _TSPtrToParentType::ValuePtrType)p_.p)
    {
    }


    /*
    PURPOSE
        Allows to access the stored pointer itself.

    EXAMPLE
        Ptr= (Ptr *)SPtr;
    */
/*    operator _Type*() const
    { 
        return (_Type *)p; 
    }    
*/
    /*
        Allows to access the value the smartpointer points to (just like with
        a regular pointer).

    EXAMPLES
        *SPtr= newValue;
    */
    _Type& operator*()
    {
        return *((_Type *)_TSPtrToParentType::p);
    }


    _Type* operator->() const
    {
        return (_Type *)_TSPtrToParentType::p; 
    }


    atDerivedPtr& operator=(const atDerivedPtr &p_)                       //The assignment operator can not be inherited.
    {
        return (atDerivedPtr &)assign((_Type*)p_._TSPtrToParentType::p);
    }


    atDerivedPtr& operator=(_Type* p_)                                        //The assignment operator can not be inherited.
    {
        return (atDerivedPtr &)assign(p_);
    }

    /*
    PURPOSE
        Allows to compare a smartpointer with a normal pointer like both would
        be normal pointers.
    */
/*    bool operator==(_Type * p_)
    {
        if(p == p_)
            return true;
        else
            return false;
    }*/
};


/*
PURPOSE
    Basically the same as atDerivedPtr but defines smart pointers to
    const instances.


PARAMS
    _Type                          - The class for which to define a smart
                                pointer to constant instances.
    TSPtrToNonConstParentType   - Smart pointer to a NON CONST instance of
                                the class _Type is derived from.
    TSPtrToConstParentType      - Smart pointer to a CONST instance of the
                                class _Type is derived from.

SEE ALSO
    atReferenceCounter, atPtr, atDerivedPtr
<FLAG Component>
*/
template <typename _Type, typename TSPtrToNonConstParentType, typename TSPtrToConstParentType> class atDerivedPtrToConst : public TSPtrToConstParentType
{
public:
    typedef     _Type       ValueType;                                          //A synonym for the template parameter _Type.
    typedef     _Type*      ValuePtrType;


    atDerivedPtrToConst() : TSPtrToConstParentType(/*(typename TSPtrToConstParentType::ValuePtrType)0*/)
    {
    }

    atDerivedPtrToConst(const _Type* p_) : TSPtrToConstParentType((typename TSPtrToConstParentType::ValuePtrType)p_)
    {
    }    

    /*
    PURPOSE
        Construct from a non constant smart pointer.
    */
    atDerivedPtrToConst(const atDerivedPtr<_Type, TSPtrToNonConstParentType> &p_) : TSPtrToConstParentType((typename TSPtrToConstParentType::ValuePtrType)p_.p)
    {
    }

    /*
    PURPOSE
        Construct from a constant smart pointer.
    */
    atDerivedPtrToConst(const atDerivedPtrToConst &p_) : TSPtrToConstParentType((typename TSPtrToConstParentType::ValuePtrType)p_.p)
    {
    }


    /*
    PURPOSE
        Allows to access the stored pointer itself.

    EXAMPLES
        Ptr= (Ptr *)SPtr;
    */
    operator const _Type*() const
    { 
        return (const _Type *)TSPtrToConstParentType::p; 
    }    

    /*
    PURPOSE
        Allows to access the value the smartpointer points to (just like with
        a regular pointer).

    EXAMPLES
        *SPtr= newValue;
    */
    const _Type& operator*()
    {
        return *((const _Type *)TSPtrToConstParentType::p);
    }


    const _Type* operator->() const
    {
        return (const _Type *)TSPtrToConstParentType::p; 
    }


    /*
    PURPOSE
        Assign a non const smart pointer.
    */
    atDerivedPtrToConst& operator=(const atDerivedPtr<_Type, TSPtrToNonConstParentType> &p_)
    {
        return (atDerivedPtrToConst &)assign((const _Type*)p_.TSPtrToConstParentType::p);
    }

    /*
    PURPOSE
        Assign a const smart pointer.
    */
    atDerivedPtrToConst& operator=(const atDerivedPtrToConst &p_)
    {
        return (atDerivedPtrToConst &)assign((const _Type*)p_.TSPtrToConstParentType::p);
    }

    /*
    PURPOSE
        Assign a regular pointer.
    */
    atDerivedPtrToConst& operator=(const _Type* p_)
    {
        return (atDerivedPtrToConst &)assign(p_);
    }
};




struct atResourceDestructor
{
	template<class T>
	static void Release(T* val )
	{ 
		val->Release(); 
	}
};
struct atMemDestructor
{
	template<class T>
	static void Release(T* val )
	{ 
		delete val;
	}
};
/*
PURPOSE
	Is a simple scoped ptr where the asset is only every used as const after being created.
	Very useful for resources such as rendertargets and textures.
*/
template<class T, class DESTRUCT = atResourceDestructor>
class atScopedPtr
{
	T*	m_ptr;
	atScopedPtr( const atScopedPtr& );

	void release()
	{
		if ( m_ptr )
		{
			DESTRUCT::Release(m_ptr);
		}
	}

public:
	atScopedPtr() : m_ptr( 0 )	{}
	atScopedPtr( T* v ) : m_ptr( v )	{}
	~atScopedPtr() 
	{ 
		release();
	}
	atScopedPtr& operator=( T* val )
	{
		if (val != m_ptr)
		{
			release();
			m_ptr = val;
		}
		return *this;
	}
	atScopedPtr& operator=( atScopedPtr& ptr )
	{
		if (ptr.m_ptr != m_ptr)
		{
			release();
			m_ptr = ptr.m_ptr;
		}
		return *this;
	}

	const T& operator*() const { return *m_ptr; }
	T& operator*() { return *m_ptr; }
	const T* operator->() const { return m_ptr; }
	T* operator->() { return m_ptr; }
	const T* GetPtr() const { return m_ptr; }
	T* GetPtr() { return m_ptr; }

	operator const T*() const { return m_ptr; }
	operator T*() {	return m_ptr; }

	friend bool operator==(const atScopedPtr &First, const atScopedPtr &Second)
	{
		return First.m_ptr == Second.m_ptr;
	}
};
namespace localShared
{
	struct References
	{
		int StrongRef;
		int WeakRef;

		References( int s, int w ) : StrongRef( s), WeakRef( w ) {}
		bool IsReferenced() const { return ( StrongRef + WeakRef ) > 0; }
		bool Exists() const { return StrongRef != 0; }

#if __DECLARESTRUCT
		void DeclareStruct(datTypeStruct &s)
		{
			STRUCT_BEGIN(References);
			STRUCT_FIELD(StrongRef);
			STRUCT_FIELD(WeakRef);
			STRUCT_END();
		}
#endif // __DECLARESTRUCT
	};
};

// PURPOSE
//		Smart reference ptr with external reference count
//		Advantages over atPtr  -> more flexible as doesn't need class to be derived off atReferenceCounter, also has weak pointers		
//		Disadvantages -> needs an extra allocation, deallocation and can be slightly slower due to memory accesses.
//
//		For a good discussion of usage and practical experience in a game with Both Shared and weak pointers 
//		see http://www.cbloom.com/3d/techdocs/smart_pointers.txt
//			
//
//SEE ALSO
//	atReferenceCounter,atPtr

template <typename _Type, class DESTRUCT = atMemDestructor> class atSharedPtr 
{
public:
	typedef _Type            ValueType;                                          //A synonym for the template parameter _Type.
	typedef _Type*            ValuePtrType;
	typedef atSharedPtr       SPtrType;
	
	//	PURPOSE
	//	Constructor that takes a normal pointer as argument.
	//
	explicit atSharedPtr(_Type* p_=0) : p(p_), m_RefCount(0)
	{
		if(p != 0)
			CreateRefCount();
	}    

	//	PURPOSE
	// Copy constructor - takes a smartpointer as argument
	//
	atSharedPtr(const atSharedPtr<_Type> &p_) : p(p_.p), m_RefCount( p_.m_RefCount )
	{
		if(p != 0)
			AddRef();
	}

	atSharedPtr(datResource &rsc)
		: p(rsc)
		, m_RefCount(rsc)
	{
	}

	void Place(datResource &rsc) { ::new (this) atSharedPtr<_Type>(rsc); }

	// PURPOSE
	// Destructor: releases the reference to the object the smartpointer
	// points to.
	~atSharedPtr()
	{
		Release();
	}

	// PURPOSE
	//Allows to access the stored pointer itself.
	//
	operator _Type*() const			{ 		return p;    	}    
	_Type& operator*()				{ 		return *p; 	}
	_Type* operator->() const		{		return p; 	}


	/*
	PURPOSE
	Assign one smartpointer to another.
	*/
	atSharedPtr& operator=(const atSharedPtr<_Type> &p_)
	{
		return assign(p_.p, p_.m_RefCount );
	}

	/*
	PURPOSE
	Allows to assign a normal pointer to the smartpointer.
	*/
	atSharedPtr& operator=(_Type* p_)
	{
		return assign(p_, p_ ? rage_new localShared::References(0,0)  : 0 );
	}

	/*
	PURPOSE
	Is it a NULL pointer ?
	*/
	inline bool IsNull()
	{
		return (p != NULL);
	}

	//	PURPOSE
	// != test between two smartpointers.
	bool operator!=(const atPtr<_Type> &p_)
	{
		return (p != p_.p);
	}

#if __DECLARESTRUCT
	void DeclareStruct(datTypeStruct &s)
	{
		STRUCT_BEGIN(atSharedPtr);
		STRUCT_FIELD(p);
		STRUCT_FIELD(m_RefCount);
		STRUCT_END();
	}
#endif // __DECLARESTRUCT

	//	PURPOSE
	// != test between a smartpointer and a normal pointer.
	bool operator!=(_Type* p_)
	{
		return (p != p_);
	}
	
	 int GetStrongRefCount() const 	{ return m_RefCount ? m_RefCount->StrongRef : 0; }
	 int GetWeakRefCount() const 	{ return m_RefCount ? m_RefCount->WeakRef : 0; }

private:
	/*
	PURPOSE
	Assigns the smartpointer a new value and takes care of reference counting.
	*/
	inline atSharedPtr& assign(_Type* p_, localShared::References* refCount )
	{
		if(p_ != p)
		{
			if(p != 0)
				Release();

			p = p_;
			m_RefCount = refCount;
			if(p != 0)
				AddRef();
		}

		return *this;
	}
	void CreateRefCount()	
	{ 
		if (!m_RefCount) 		{ m_RefCount = rage_new localShared::References(1,0); } 
	}

	void AddRef()			
	{ 
		FastAssert(m_RefCount);
		m_RefCount->StrongRef++;
	} 
	void Release()
	{
		if ( m_RefCount) 
		{
			if  ( (--m_RefCount->StrongRef) == 0 )
			{
				DESTRUCT::Release( (_Type *) p);
				if ( !m_RefCount->IsReferenced() )
				{
					delete m_RefCount;
				}
			}

		}
	}

	

	datOwner<_Type>                  p;                                                  //Stores the actual pointer.	
	datOwner<localShared::References>				m_RefCount;
};

template<class _Type2>
inline int	atSharedGetRefCount( atSharedPtr<_Type2>& ptr )	{ return ptr.GetStrongRefCount();}


// PURPOSE
//		Weak pointer which returns null if the object it points to no longer exists
//		Can only take in a SharedPtr
//
//SEE ALSO
//	atSharedPtr, atReferenceCounter,atPtr

template <typename _Type, class DESTRUCT = atMemDestructor>  class atWeakPtr
{
public:
	friend class atSharedPtr<_Type, DESTRUCT>;

	typedef _Type            ValueType;                                          //A synonym for the template parameter _Type.
	typedef _Type*            ValuePtrType;
	typedef atWeakPtr			SPtrType;
	

	//	PURPOSE
	//	Constructor that takes a normal pointer as argument.
	//
	atWeakPtr( atSharedPtr<_Type> &s ) : p( s.p ), m_RefCount( s.m_RefCount )
	{
		AssertMsg( m_RefCount != 0 , "Weak Pointer can not be assigned to an empty pointer" );
		AddRef();
	}    

	//	PURPOSE
	// Copy constructor - takes a smartpointer as argument
	//
	atWeakPtr(const atWeakPtr<_Type> &p_) : p(p_.p), m_RefCount( p_.m_RefCount )
	{
		AddRef();
	}
	
	// PURPOSE
	// Destructor: releases the reference to the object the smartpointer
	// points to.
	~atWeakPtr()
	{
		Release();
	}

	atWeakPtr& operator=(const atWeakPtr<_Type> &p_)
	{
		m_RefCount->WeakRef--;
		return assign(p_.p, p_.m_RefCount);
	}
	atWeakPtr& operator=(const atSharedPtr<_Type> &p_)
	{
		return assign(p_.p, p_.m_RefCount);
	}
	
	// PURPOSE
	//Allows to access the stored pointer itself.
	//
	operator _Type*() const			{ 		return m_RefCount->Exists() ? p : 0;    	}    
	_Type& operator*()				{ 		FastAssert( m_RefCount->Exists()); return  *p; 	}
	_Type* operator->() const		{		return m_RefCount->Exists() ? p : 0; 	}
	
	inline bool IsNull()	{		return m_RefCount->Exists() ? p : 0;	}

	//	PURPOSE
	// != test between two smartpointers.
	bool operator!=(const atWeakPtr<_Type> &p_)
	{
		return (p != p_.p);
	}

public:
	inline atWeakPtr& assign(_Type* p_, localShared::References* refCount )
	{
		if(p != 0)
			Release();

		p = p_;
		m_RefCount = refCount;

		if(p != 0)
			AddRef();
		return *this;
	}
	void AddRef()
	{
		m_RefCount->WeakRef++;
	}
	void Release()
	{
		m_RefCount->WeakRef--;
		if ( !m_RefCount->IsReferenced() )
		{
			delete m_RefCount;
		}
	}
	//protected:
	_Type*									p;                                                  //Stores the actual pointer.
	localShared::References*				m_RefCount;
};

}	// namespace rage


#endif //ATL_PTR_H
