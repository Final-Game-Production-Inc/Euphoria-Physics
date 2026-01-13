//
// atl/functor.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef ATL_FUNCTOR_H
#define ATL_FUNCTOR_H

#include <stddef.h>		// for size_t

namespace rage {

//lint -e115

/*
	PURPOSE:

	<c>Functor<0...N></c> is a new and improved set of callback classes.
	This code is derived from <EXTLINK http://www.tutok.sk/fastgl/callback.html>Rich 
	Hickey's discussion on Functors.</EXTLINK>
    
	* Advantages *

	1. <b>type safety</b> - the set of Functor class enforce type 
	   safety, so you don't have to worry about incorrect
	   casts causing random crashes that are possible with 
	   the datCallback class.

	2. <b>no wierd macros</b> - Functors have no need for the 
	   MFA or CFA macros.
		
	3. <b>heirarchy independence</b> - if you want to call
	   member functions with datCallback, you have to 
	   derive your class from Base.  Functors don't have 
	   this requirement.
	
	4. <b>multiple arguments</b> - you can have more than one argument to a functor.

	5. <b>return types</b> - are supported.

    * Disadvantages *
		
	1. though it doesn't use as much template code as some other 
	   designs, Functor will increase code size compared to 
	   using datCallback.

	In Andrei Alexandrescu's <i>Modern C++ Design</i>, a more generalized 
	Functor is described that also allows binding of 
	parameters.  I chose Hickey's since Alexandrescu's relied 
	on partial template specialization (which is not supported
	in Visual C++ 6 or 7), and because Hickey's functors 
	can be passed by value without any dynamic memory allocation
	occurring.

	EXAMPLE:

	<CODE>
	class testClass
	{
	public:
		void Func0()					{Printf("in Func0\n");}
		void Func1(const char* s)		{Printf("in Func1: %s\n",s);}
		int  FuncRet1(const char* s)	{Printf("in Func2: %s\n",s); return 1;}
	};

	void main()
	{
		testClass testClassInst;

		// demonstrates a setup for a functor with no arguments:
		Functor0						testFunc0	= MakeFunctor(testClassInst,&testClass::Func0);
		testFunc0();  // will print "in Func0"

		// demonstrates a setup for a functor with 1 argument:
		Functor1<const char*>			testFunc1	= MakeFunctor(testClassInst,&testClass::Func1);
		testFunc1("look at me!");  // will print "in Func1: look at me!"

		// demonstrates a setup for a functor with 1 argument and a return value:
		Functor1Ret<int,const char*>	testFunc2	= MakeFunctorRet(testClassInst,&testClass::FuncRet1);
		int retValue=testFunc2("look at me!");  // will print "in Func1: look at me!"
		Printf("retValue: %d",retValue);		// will print "retValue: 1"
	}
	</CODE>
*/
class FunctorBase
{
private:
	class DummyA
	{
	public:
		virtual ~DummyA() {}
	};
	class DummyB
	{
	public:
		virtual ~DummyB() {}
	};

	// this class is used for the sole purpose of 
	// inquiring the max size of a pointer to member function
	// as multiple inheritance seems to create a bigger pointer
	// under Win32:
	class GetPtrMemSize : public DummyA, public DummyB
	{
	public:
		virtual ~GetPtrMemSize() {}
		virtual void DummyFunc() {}
	};

	friend bool operator==(const FunctorBase &lhs,const FunctorBase &rhs);
	friend bool operator!=(const FunctorBase &lhs,const FunctorBase &rhs)
	{
		return !operator==(lhs,rhs);
	}

public:
	typedef void (GetPtrMemSize::*PMemFunc)();
	typedef void (*PFunc)();
	enum {MEM_FUNC_SIZE = sizeof(PMemFunc)};

	FunctorBase():Func(0),Callee(0)
	{
	}

	FunctorBase(const void *c,PFunc f,const void *mf,size_t sz); //lint !e114

	//for evaluation in conditions
	operator bool() const	{return Func||Callee;} //lint !e1930

	class DummyInit //lint !e114
	{
	};


	PFunc		GetFunc() const		{return Func;}
	void*		GetCallee() const	{return Callee;}
	const char*	GetMemFunc() const	{return MemFunc;}


	////////////////////////////////////////////////////////////////
	// Note: this code depends on all ptr-to-mem-funcs being same size
	// If that is not the case then make memFunc as large as largest
	////////////////////////////////////////////////////////////////

	union
	{
	PFunc	Func;
	char	MemFunc[MEM_FUNC_SIZE];
	};
	void*		Callee;
};

/************************* Functor0 *******************/


//<ALIAS Topic1>
class Functor0 : public FunctorBase
{
public:
	Functor0(DummyInit * = 0) : FunctorBase() {} //lint !e1931
	void operator()(void) const // operator for calling the function
	{
		Thunk(*this);
	}

	static void NullFn() {} // the null functor instance
	inline static Functor0 NullFunctor();
	
protected:
	typedef void (*TypeThunk)(const FunctorBase &); //lint !e114
	Functor0(TypeThunk t,const void *c,PFunc f,const void *mf,size_t sz):
		FunctorBase(c,f,mf,sz),Thunk(t){}
private: //lint !e114
	TypeThunk Thunk; //lint !e114
};

//DOM-IGNORE-BEGIN

/************************* MemberTranslator0 *******************/
template <class _Callee, class _MemFunc>
class MemberTranslator0:public Functor0
{
public:
	MemberTranslator0(_Callee &c,const _MemFunc &m):
		Functor0(thunk,&c,0,&m,sizeof(_MemFunc))
	{
	}
	static void thunk(const FunctorBase &ftor)
	{
		_Callee *callee = (_Callee *)ftor.GetCallee();
		_MemFunc &memFunc = *(_MemFunc*)(void *)(ftor.GetMemFunc());
		(callee->*memFunc)();
	}
private:
	MemberTranslator0();
};

/************************* FunctionTranslator0 *******************/
template <class _Func>
class FunctionTranslator0:public Functor0
{
public:
	FunctionTranslator0(_Func f):Functor0(thunk,0,(FunctorBase::PFunc)f,0,0) //lint !e1931 !e747
	{
	}
	static void thunk(const FunctorBase &ftor)
	{
		(_Func(ftor.GetFunc()))();
	}

	bool operator==(FunctionTranslator0& func0)
	{
		return operator==(*this, func0);
	}
}; //lint !e1712

/************************* MakeFunctor for Functor0 *******************/
template <class _Callee,class _Trt,class _CallType>
inline MemberTranslator0<_Callee,_Trt (_CallType::*)(void)>
___MakeFunctor(_Callee &c,_Trt (_CallType::* const & f)(void)) //lint !e1717
{
	typedef _Trt (_CallType::*MemFunc)();
	return MemberTranslator0<_Callee,MemFunc>(c,f);
}

template <class _Callee,class _Trt,class _CallType>
inline MemberTranslator0<const _Callee,_Trt (_CallType::*)(void)const>
___MakeFunctor(const _Callee &c,_Trt (_CallType::* const & f)(void)const)
{
	typedef _Trt (_CallType::*MemFunc)()const;
	return MemberTranslator0<const _Callee,MemFunc>(c,f);
}

template <class _Trt>
inline FunctionTranslator0<_Trt (*)(void)>
___MakeFunctor(_Trt (*f)(void))
{
	return FunctionTranslator0<_Trt (*)()>(f); //lint !e1717
}
/************************* NullFunctor for Functor0 *******************/
inline Functor0 Functor0::NullFunctor() {return ___MakeFunctor(Functor0::NullFn);}

/************************* Functor0Ret *******************/
template <class _RetType> class Functor0Ret : public FunctorBase
{
public:
	Functor0Ret(DummyInit * = 0){} //lint !e1931
	_RetType operator()() const
	{
		return Thunk(*this);
	}
	
	static _RetType NullFn() {return _RetType();}
	static Functor0Ret NullFunctor() {return ___MakeFunctor(NullFn);}

protected:
	typedef _RetType (*TypeThunk)(const FunctorBase &); //lint !e114
	Functor0Ret(TypeThunk t,const void *c,PFunc f,const void *mf,size_t sz):
		FunctorBase(c,f,mf,sz),Thunk(t){}
private:
	TypeThunk Thunk;
};

/************************* MemberTranslator0Ret *******************/
template <class _RetType,class _Callee, class _MemFunc>
class MemberTranslator0Ret:public Functor0Ret<_RetType>
{
public:
	 MemberTranslator0Ret(_Callee &c,const _MemFunc &m):
		Functor0Ret<_RetType>(thunk,&c,0,&m,sizeof(_MemFunc))
	{
	}
	static _RetType thunk(const FunctorBase &ftor)
	{
		_Callee *callee = (_Callee *)ftor.GetCallee();
		_MemFunc &memFunc = *(_MemFunc*)(void *)(ftor.GetMemFunc());
		return (callee->*memFunc)();
	}
};

/************************* FunctionTranslator0Ret *******************/
template <class _RetType,class _Func>
class FunctionTranslator0Ret:public Functor0Ret<_RetType>
{
public:
	FunctionTranslator0Ret(_Func f):Functor0Ret<_RetType>(thunk,0,(FunctorBase::PFunc)f,0,0) //lint !e1931
	{
	}

	static _RetType thunk(const FunctorBase &ftor)
	{
		return (_Func(ftor.GetFunc()))();
	}
};

/************************* MakeFunctorRet *******************/
template <class _Callee,class _Trt,class _CallType>
inline MemberTranslator0Ret<_Trt,_Callee,_Trt (_CallType::*)(void)>
___MakeFunctorRet(_Callee &c,_Trt (_CallType::* const & f)(void))
{
	typedef _Trt (_CallType::*MemFunc)();
	return MemberTranslator0Ret<_Trt,_Callee,MemFunc>(c,f);
}

template <class _Callee,class _Trt,class _CallType>
inline MemberTranslator0Ret<_Trt,const _Callee,_Trt (_CallType::*)(void) const>
___MakeFunctorRet(const _Callee &c,_Trt (_CallType::* const & f)(void) const)
{
	typedef _Trt (_CallType::*MemFunc)()const;
	return MemberTranslator0Ret<_Trt,const _Callee,MemFunc>(c,f);
}

template <class _Trt>
inline FunctionTranslator0Ret<_Trt,_Trt (*)(void)>
___MakeFunctorRet(_Trt (*f)(void))
{
	return FunctionTranslator0Ret<_Trt,_Trt (*)(void)>(f);
}


// Functor1:	
#define ___FUNCTOR_NAME				Functor1
#define ___MEMBER_TRANS_NAME		MemberTranslator1
#define ___FUNCTION_TRANS_NAME		FunctionTranslator1
#define	___TEMPLATE_ARG_LIST		class _P1
#define	___FUNC_DEC_ARG_LIST		_P1 p1
#define	___FUNC_ARG_LIST			p1
#define	___ARG_TYPE_LIST			_P1
#define ___RET_TYPE					void
#define ___RET					
#define ___TEMPLATE_RET
#define ___RET_ARG_TYPE_LIST
#define ___MAKE_FUNCTOR_NAME		___MakeFunctor
#define ___MAKE_RET_ARG_TYPE_LIST	
#define ___RET_ARG_CTOR				
#include "functor_temp.h"

// Functor1Ret:
#define ___FUNCTOR_NAME				Functor1Ret
#define ___MEMBER_TRANS_NAME		MemberTranslator1Ret
#define ___FUNCTION_TRANS_NAME		FunctionTranslator1Ret
#define	___TEMPLATE_ARG_LIST		class _P1
#define	___FUNC_DEC_ARG_LIST		_P1 p1
#define	___FUNC_ARG_LIST			p1
#define	___ARG_TYPE_LIST			_P1
#define ___RET_TYPE					_RetType
#define ___RET						return 
#define ___TEMPLATE_RET				class _RetType,
#define ___RET_ARG_TYPE_LIST		_RetType,
#define ___MAKE_FUNCTOR_NAME		___MakeFunctorRet
#define ___MAKE_RET_ARG_TYPE_LIST	_Trt,
#define ___RET_ARG_CTOR				_RetType()
#include "functor_temp.h"

// Functor2:
#define ___FUNCTOR_NAME				Functor2
#define ___MEMBER_TRANS_NAME		MemberTranslator2
#define ___FUNCTION_TRANS_NAME		FunctionTranslator2
#define	___TEMPLATE_ARG_LIST		class _P1,class _P2
#define	___FUNC_DEC_ARG_LIST		_P1 p1,_P2 p2
#define	___FUNC_ARG_LIST			p1,p2
#define	___ARG_TYPE_LIST			_P1,_P2
#define ___RET_TYPE					void
#define ___RET					
#define ___TEMPLATE_RET
#define ___RET_ARG_TYPE_LIST
#define ___MAKE_FUNCTOR_NAME		___MakeFunctor
#define ___MAKE_RET_ARG_TYPE_LIST	
#define ___RET_ARG_CTOR				
#include "functor_temp.h"

// Functor2Ret:
#define ___FUNCTOR_NAME				Functor2Ret
#define ___MEMBER_TRANS_NAME		MemberTranslator2Ret
#define ___FUNCTION_TRANS_NAME		FunctionTranslator2Ret
#define	___TEMPLATE_ARG_LIST		class _P1,class _P2
#define	___FUNC_DEC_ARG_LIST		_P1 p1,_P2 p2
#define	___FUNC_ARG_LIST			p1,p2
#define	___ARG_TYPE_LIST			_P1,_P2
#define ___RET_TYPE					_RetType
#define ___RET						return 
#define ___TEMPLATE_RET				class _RetType,
#define ___RET_ARG_TYPE_LIST		_RetType,
#define ___MAKE_FUNCTOR_NAME		___MakeFunctorRet
#define ___MAKE_RET_ARG_TYPE_LIST	_Trt,
#define ___RET_ARG_CTOR				_RetType()
#include "functor_temp.h"

// Functor3:
#define ___FUNCTOR_NAME				Functor3
#define ___MEMBER_TRANS_NAME		MemberTranslator3
#define ___FUNCTION_TRANS_NAME		FunctionTranslator3
#define	___TEMPLATE_ARG_LIST		class _P1,class _P2,class _P3
#define	___FUNC_DEC_ARG_LIST		_P1 p1,_P2 p2,_P3 p3
#define	___FUNC_ARG_LIST			p1,p2,p3
#define	___ARG_TYPE_LIST			_P1,_P2,_P3
#define ___RET_TYPE					void
#define ___RET					
#define ___TEMPLATE_RET
#define ___RET_ARG_TYPE_LIST
#define ___MAKE_FUNCTOR_NAME		___MakeFunctor
#define ___MAKE_RET_ARG_TYPE_LIST	
#define ___RET_ARG_CTOR				
#include "functor_temp.h"

// Functor3Ret:
#define ___FUNCTOR_NAME				Functor3Ret
#define ___MEMBER_TRANS_NAME		MemberTranslator3Ret
#define ___FUNCTION_TRANS_NAME		FunctionTranslator3Ret
#define	___TEMPLATE_ARG_LIST		class _P1,class _P2,class _P3
#define	___FUNC_DEC_ARG_LIST		_P1 p1,_P2 p2,_P3 p3
#define	___FUNC_ARG_LIST			p1,p2,p3
#define	___ARG_TYPE_LIST			_P1,_P2,_P3
#define ___RET_TYPE					_RetType
#define ___RET						return 
#define ___TEMPLATE_RET				class _RetType,
#define ___RET_ARG_TYPE_LIST		_RetType,
#define ___MAKE_FUNCTOR_NAME		___MakeFunctorRet
#define ___MAKE_RET_ARG_TYPE_LIST	_Trt,
#define ___RET_ARG_CTOR				_RetType()
#include "functor_temp.h"

// Functor4:
#define ___FUNCTOR_NAME				Functor4
#define ___MEMBER_TRANS_NAME		MemberTranslator4
#define ___FUNCTION_TRANS_NAME		FunctionTranslator4
#define	___TEMPLATE_ARG_LIST		class _P1,class _P2,class _P3,class _P4
#define	___FUNC_DEC_ARG_LIST		_P1 p1,_P2 p2,_P3 p3,_P4 p4
#define	___FUNC_ARG_LIST			p1,p2,p3,p4
#define	___ARG_TYPE_LIST			_P1,_P2,_P3,_P4
#define ___RET_TYPE					void
#define ___RET					
#define ___TEMPLATE_RET
#define ___RET_ARG_TYPE_LIST
#define ___MAKE_FUNCTOR_NAME		___MakeFunctor
#define ___MAKE_RET_ARG_TYPE_LIST	
#define ___RET_ARG_CTOR				
#include "functor_temp.h"

// Functor4Ret:
#define ___FUNCTOR_NAME				Functor4Ret
#define ___MEMBER_TRANS_NAME		MemberTranslator4Ret
#define ___FUNCTION_TRANS_NAME		FunctionTranslator4Ret
#define	___TEMPLATE_ARG_LIST		class _P1,class _P2,class _P3,class _P4
#define	___FUNC_DEC_ARG_LIST		_P1 p1,_P2 p2,_P3 p3,_P4 p4
#define	___FUNC_ARG_LIST			p1,p2,p3,p4
#define	___ARG_TYPE_LIST			_P1,_P2,_P3,_P4
#define ___RET_TYPE					_RetType
#define ___RET						return 
#define ___TEMPLATE_RET				class _RetType,
#define ___RET_ARG_TYPE_LIST		_RetType,
#define ___MAKE_FUNCTOR_NAME		___MakeFunctorRet
#define ___MAKE_RET_ARG_TYPE_LIST	_Trt,
#define ___RET_ARG_CTOR				_RetType()
#include "functor_temp.h"

// Functor5:
#define ___FUNCTOR_NAME				Functor5
#define ___MEMBER_TRANS_NAME		MemberTranslator5
#define ___FUNCTION_TRANS_NAME		FunctionTranslator5
#define	___TEMPLATE_ARG_LIST		class _P1,class _P2,class _P3,class _P4,class _P5
#define	___FUNC_DEC_ARG_LIST		_P1 p1,_P2 p2,_P3 p3,_P4 p4,_P5 p5
#define	___FUNC_ARG_LIST			p1,p2,p3,p4,p5
#define	___ARG_TYPE_LIST			_P1,_P2,_P3,_P4,_P5
#define ___RET_TYPE					void
#define ___RET					
#define ___TEMPLATE_RET
#define ___RET_ARG_TYPE_LIST
#define ___MAKE_FUNCTOR_NAME		___MakeFunctor
#define ___MAKE_RET_ARG_TYPE_LIST	
#define ___RET_ARG_CTOR				
#include "functor_temp.h"

// Functor5Ret:
#define ___FUNCTOR_NAME				Functor5Ret
#define ___MEMBER_TRANS_NAME		MemberTranslator5Ret
#define ___FUNCTION_TRANS_NAME		FunctionTranslator5Ret
#define	___TEMPLATE_ARG_LIST		class _P1,class _P2,class _P3,class _P4,class _P5
#define	___FUNC_DEC_ARG_LIST		_P1 p1,_P2 p2,_P3 p3,_P4 p4,_P5 p5
#define	___FUNC_ARG_LIST			p1,p2,p3,p4,p5
#define	___ARG_TYPE_LIST			_P1,_P2,_P3,_P4,_P5
#define ___RET_TYPE					_RetType
#define ___RET						return 
#define ___TEMPLATE_RET				class _RetType,
#define ___RET_ARG_TYPE_LIST		_RetType,
#define ___MAKE_FUNCTOR_NAME		___MakeFunctorRet
#define ___MAKE_RET_ARG_TYPE_LIST	_Trt,
#define ___RET_ARG_CTOR				_RetType()
#include "functor_temp.h"

// add Functor5,Functor6,Functor7...FunctorN if you need 'em

//////// lint fixes:
//lint -esym(1025,___MakeFunctor)
//lint -esym(1025,___MakeFunctorRet)

//DOM-IGNORE-END

#define MakeFunctor			/*lint --e(1703) --e(1514) --e(64) --e(1058) --e(118)*/ ___MakeFunctor
#define MakeFunctorRet		/*lint --e(1703) --e(1514) --e(64) --e(1058) --e(118)*/ ___MakeFunctorRet

//lint +e115

}	// namespace rage

#endif



