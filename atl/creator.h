// 
// atl/creator.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef ATL_CREATOR_H
#define ATL_CREATOR_H

#include "system/new.h"

namespace rage {

// PURPOSE: This is a function template that can be used to create factory functions.
// When called, the function will return a new object of type _Type.
template<class _Base, class _Type>
_Base& atCreator() 
{
	return *(rage_new _Type);
}

// PURPOSE: This is a function template that can be used to create factory functions.
// When called, the function will return a new object of type _Type.
template<class _Base, class _Type>
_Base* atPtrCreator() 
{
	return rage_new _Type;
}

template<class _Base, class _Type>
void atPlaceCreator(_Base& obj)
{
	::new(&obj) _Type;
}

template<class _Base, class _Type>
void atPlacePtrCreator(_Base obj)
{
	::new(obj) _Type;
}


#define __MULTIARG_ATCREATOR								\
template<class _Base, class _Type, __TEMPLATE_ARG_LIST>		\
_Base& atCreator(__FUNC_DEC_ARG_LIST)						\
{															\
	return *(rage_new _Type(__FUNC_ARG_LIST));					\
}

#define __MULTIARG_ATPTRCREATOR								\
template<class _Base, class _Type, __TEMPLATE_ARG_LIST>		\
_Base* atPtrCreator(__FUNC_DEC_ARG_LIST)					\
{															\
	return rage_new _Type(__FUNC_ARG_LIST);						\
}

#define __MULTIARG_ATPLACECREATOR							\
template<class _Base, class _Type, __TEMPLATE_ARG_LIST>		\
void atPlaceCreator(_Base& obj, __FUNC_DEC_ARG_LIST)		\
{															\
	::new(&obj) _Type(__FUNC_ARG_LIST);						\
}

#define __MULTIARG_ATPLACEPTRCREATOR						\
template<class _Base, class _Type, __TEMPLATE_ARG_LIST>		\
void atPlacePtrCreator(_Base& obj, __FUNC_DEC_ARG_LIST)		\
{															\
	::new(obj) _Type(__FUNC_ARG_LIST);						\
}

#define __ALL_ATCREATOR_TEMPLATES							\
	__MULTIARG_ATCREATOR									\
	__MULTIARG_ATPTRCREATOR									\
	__MULTIARG_ATPLACECREATOR								\
	__MULTIARG_ATPLACEPTRCREATOR							\

#define __TEMPLATE_ARG_LIST			typename _P1
#define __FUNC_DEC_ARG_LIST			_P1 p1
#define __FUNC_ARG_LIST				p1
__ALL_ATCREATOR_TEMPLATES
#undef __FUNC_ARG_LIST
#undef __FUNC_DEC_ARG_LIST
#undef __TEMPLATE_ARG_LIST

#define __TEMPLATE_ARG_LIST			typename _P1, typename _P2
#define __FUNC_DEC_ARG_LIST			_P1 p1, _P2 p2
#define __FUNC_ARG_LIST				p1, p2
__ALL_ATCREATOR_TEMPLATES
#undef __FUNC_ARG_LIST
#undef __FUNC_DEC_ARG_LIST
#undef __TEMPLATE_ARG_LIST

#define __TEMPLATE_ARG_LIST			typename _P1, typename _P2, typename _P3
#define __FUNC_DEC_ARG_LIST			_P1 p1, _P2 p2, _P3 p3
#define __FUNC_ARG_LIST				p1, p2, p3
__ALL_ATCREATOR_TEMPLATES
#undef __FUNC_ARG_LIST
#undef __FUNC_DEC_ARG_LIST
#undef __TEMPLATE_ARG_LIST

#define __TEMPLATE_ARG_LIST			typename _P1, typename _P2, typename _P3, typename _P4
#define __FUNC_DEC_ARG_LIST			_P1 p1, _P2 p2, _P3 p3, _P4 p4
#define __FUNC_ARG_LIST				p1, p2, p3, p4
__ALL_ATCREATOR_TEMPLATES
#undef __FUNC_ARG_LIST
#undef __FUNC_DEC_ARG_LIST
#undef __TEMPLATE_ARG_LIST

#define __TEMPLATE_ARG_LIST			typename _P1, typename _P2, typename _P3, typename _P4, typename _P5
#define __FUNC_DEC_ARG_LIST			_P1 p1, _P2 p2, _P3 p3, _P4 p4, _P5 p5
#define __FUNC_ARG_LIST				p1, p2, p3, p4, p5
__ALL_ATCREATOR_TEMPLATES
#undef __FUNC_ARG_LIST
#undef __FUNC_DEC_ARG_LIST
#undef __TEMPLATE_ARG_LIST

} // namespace rage

#endif // ATL_CREATOR_H
