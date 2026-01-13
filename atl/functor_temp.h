//
// atl/functor_temp.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

//lint -etemplate(1764) -etemplate(550)

/************************* FunctorN template *******************/
template <___TEMPLATE_RET ___TEMPLATE_ARG_LIST > class ___FUNCTOR_NAME : public FunctorBase
{
public:
	___FUNCTOR_NAME(DummyInit * = 0) : FunctorBase() {} //lint !e1931
	___RET_TYPE operator()(___FUNC_DEC_ARG_LIST) const //lint !e66
	{
		___RET Thunk(*this,___FUNC_ARG_LIST);
	} //lint !e1746
	
	static ___RET_TYPE NullFn(___ARG_TYPE_LIST) {___RET ___RET_ARG_CTOR;} //lint !e66
	static ___FUNCTOR_NAME NullFunctor(); // {return ___MakeFunctor(NullFn);}

protected:
	typedef ___RET_TYPE (*TypeThunk)(const FunctorBase &,___ARG_TYPE_LIST); //lint !e114 !e66
	___FUNCTOR_NAME(TypeThunk t,const void *c,PFunc f,const void *mf,size_t sz):
	FunctorBase(c,f,mf,sz),Thunk(t){} //lint !e747
private:
	TypeThunk Thunk;
};

/************************* MemberTranslatorN template *******************/
template <___TEMPLATE_RET ___TEMPLATE_ARG_LIST,class _callee, class _memFunc>
class ___MEMBER_TRANS_NAME:public ___FUNCTOR_NAME<___RET_ARG_TYPE_LIST ___ARG_TYPE_LIST>
{
public:
	___MEMBER_TRANS_NAME(_callee &c,const _memFunc &m):
		___FUNCTOR_NAME<___RET_ARG_TYPE_LIST ___ARG_TYPE_LIST>(thunk,&c,0,&m,sizeof(_memFunc))
	{
		} //lint !e1746
	static ___RET_TYPE thunk(const FunctorBase &ftor,___FUNC_DEC_ARG_LIST) //lint !e66
	{
		_callee *callee = (_callee *)ftor.GetCallee();
		_memFunc &memFunc = *(_memFunc*)(void *)(ftor.GetMemFunc());
		___RET (callee->*memFunc)(___FUNC_ARG_LIST); //lint !e1004 !e10
	} //lint !e1746
}; //lint !e1712

/************************* FunctionTranslatorN template *******************/
template <___TEMPLATE_RET ___TEMPLATE_ARG_LIST,class _func>
class ___FUNCTION_TRANS_NAME:public ___FUNCTOR_NAME<___RET_ARG_TYPE_LIST ___ARG_TYPE_LIST>
{
public:
	___FUNCTION_TRANS_NAME(_func f):___FUNCTOR_NAME<___RET_ARG_TYPE_LIST ___ARG_TYPE_LIST>(thunk,0,(FunctorBase::PFunc)f,0,0) //lint !e1931 !e747
	{
	}
	static ___RET_TYPE thunk(const FunctorBase &ftor,___FUNC_DEC_ARG_LIST) //lint !e66
	{
		___RET (_func(ftor.GetFunc()))(___FUNC_ARG_LIST);
	}
}; //lint !e1712

/************************* MakeFunctor templates *******************/
template <class _callee,class _Trt,class _callType,___TEMPLATE_ARG_LIST>
inline ___MEMBER_TRANS_NAME<___MAKE_RET_ARG_TYPE_LIST ___ARG_TYPE_LIST,_callee,_Trt (_callType::*)(___ARG_TYPE_LIST)>
___MAKE_FUNCTOR_NAME(_callee &c,_Trt (_callType::* const & f)(___ARG_TYPE_LIST))
{
	typedef _Trt (_callType::*MemFunc)(___ARG_TYPE_LIST);
	return ___MEMBER_TRANS_NAME<___MAKE_RET_ARG_TYPE_LIST ___ARG_TYPE_LIST,_callee,MemFunc>(c,f);
}

template <class _callee,class _Trt,class _callType,___TEMPLATE_ARG_LIST>
inline ___MEMBER_TRANS_NAME<___MAKE_RET_ARG_TYPE_LIST ___ARG_TYPE_LIST,const _callee,_Trt (_callType::*)(___ARG_TYPE_LIST)const>
___MAKE_FUNCTOR_NAME(const _callee &c,_Trt (_callType::* const & f)(___ARG_TYPE_LIST)const)
{
	typedef _Trt (_callType::*MemFunc)(___ARG_TYPE_LIST)const;
	return ___MEMBER_TRANS_NAME<___MAKE_RET_ARG_TYPE_LIST ___ARG_TYPE_LIST,const _callee,MemFunc>(c,f);
}

template <class _Trt, ___TEMPLATE_ARG_LIST>
inline ___FUNCTION_TRANS_NAME<___MAKE_RET_ARG_TYPE_LIST ___ARG_TYPE_LIST,_Trt (*)(___ARG_TYPE_LIST)> //lint !e40 !e129
___MAKE_FUNCTOR_NAME(_Trt (*f)(___ARG_TYPE_LIST))
{
	return ___FUNCTION_TRANS_NAME<___MAKE_RET_ARG_TYPE_LIST ___ARG_TYPE_LIST,_Trt (*)(___ARG_TYPE_LIST)>(f);
}

template <___TEMPLATE_RET ___TEMPLATE_ARG_LIST > 
inline 
___FUNCTOR_NAME<___RET_ARG_TYPE_LIST ___ARG_TYPE_LIST> 
___FUNCTOR_NAME < ___RET_ARG_TYPE_LIST ___ARG_TYPE_LIST > :: NullFunctor()
{
	return ___MakeFunctor(___FUNCTOR_NAME<___RET_ARG_TYPE_LIST ___ARG_TYPE_LIST> :: NullFn);
}


#undef ___FUNCTOR_NAME			
#undef ___MEMBER_TRANS_NAME	
#undef ___FUNCTION_TRANS_NAME
#undef ___TEMPLATE_ARG_LIST
#undef ___FUNC_DEC_ARG_LIST
#undef ___FUNC_ARG_LIST	
#undef ___ARG_TYPE_LIST	
#undef ___TEMPLATE_RET
#undef ___RET
#undef ___RET_TYPE
#undef ___RET_ARG_TYPE_LIST
#undef ___MAKE_FUNCTOR_NAME
#undef ___MAKE_RET_ARG_TYPE_LIST
#undef ___RET_ARG_CTOR

