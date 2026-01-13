
#ifndef FAST_FACTORY_H
#define FAST_FACTORY_H
#include <vector>
//
//	PURPOSE
//		Macros to make it easy to use templates to the compiler to build mulitple versions of a pieace of code
//		so that it can be executed without branchs. Should be extended to have functors inlined and other stuff.

typedef std::vector<int>		paramList;

#define FASTFACTORY_CREATOR( a )	FASTFACTORY_MAKE_##a



template<class BaseClass, class FUNCTOR>
BaseClass*	FastFactoryCreate( FUNCTOR f, bool a, bool b, bool c )
{
	paramList params;
	params.push_back( (int) c );
	params.push_back( (int) b );
	params.push_back( (int) a );
	return f( params);
}
template<class BaseClass, class FUNCTOR>
BaseClass*	FastFactoryCreate( FUNCTOR f, bool a, bool b, bool c , bool d)
{
	paramList params;
	params.push_back( (int) d );
	params.push_back( (int) c );
	params.push_back( (int) b );
	params.push_back( (int) a );
	return f( params);
}
template<class BaseClass, class FUNCTOR>
BaseClass*	FastFactoryCreate( FUNCTOR f, bool a, bool b, bool c , bool d, bool e )
{
	paramList params;
	params.push_back( (int) e );
	params.push_back( (int) d );
	params.push_back( (int) c );
	params.push_back( (int) b );
	params.push_back( (int) a );
	return f( params);
}

template<class BaseClass, class FUNCTOR>
BaseClass*	FastFactoryCreate( FUNCTOR fc, bool a, bool b, bool c , bool d, bool e, bool f )
{
	paramList params;
	params.push_back( (int) f );
	params.push_back( (int) e );
	params.push_back( (int) d );
	params.push_back( (int) c );
	params.push_back( (int) b );
	params.push_back( (int) a );
	return fc( params);
}



/*
PURPOSE:
Fast Factory specification macros for creating the branch heirachy
*/

#define FAST_FACTORY_BOOL0( baseClass )   \
	baseClass*    FASTFACTORY_MAKE_##baseClass(  paramList& p )\
					{\
					int isSet = p.back();\
					p.pop_back();\
					if ( isSet)	{\
					return FASTFACTORY_MAKE_1_##baseClass< 1>(  p );\
					}\
					return FASTFACTORY_MAKE_1_##baseClass< 0>(  p);\
					}


#define FAST_FACTORY_BOOL1( baseClass )   \
	template<int A>		\
	baseClass*    FASTFACTORY_MAKE_1_##baseClass(  paramList& p )\
{\
	int isSet = p.back();\
	p.pop_back();\
	if ( isSet)	{\
	return FASTFACTORY_MAKE_2_##baseClass< A, 1>(  p );\
	}\
	return FASTFACTORY_MAKE_2_##baseClass<A, 0>(  p);\
}

#define FAST_FACTORY_BOOL2( baseClass )   \
	template<int A, int B>		\
	baseClass*    FASTFACTORY_MAKE_2_##baseClass(  paramList& p )\
{\
	int isSet = p.back();\
	p.pop_back();\
	if ( isSet)	{\
	return FASTFACTORY_MAKE_3_##baseClass< A, B,  1>(  p );\
	}\
	return FASTFACTORY_MAKE_3_##baseClass<A, B, 0>(  p);\
}

#define FAST_FACTORY_BOOL3( baseClass )   \
	template<int A, int B, int C>		\
	baseClass*    FASTFACTORY_MAKE_3_##baseClass(  paramList& p )\
{\
	int isSet = p.back();\
	p.pop_back();\
	if ( isSet)	{\
	return FASTFACTORY_MAKE_4_##baseClass< A, B, C, 1>(  p );\
	}\
	return FASTFACTORY_MAKE_4_##baseClass<A, B, C, 0>(  p);\
}

#define FAST_FACTORY_BOOL4( baseClass )   \
	template<int A, int B, int C, int D>		\
	baseClass*    FASTFACTORY_MAKE_4_##baseClass(  paramList& p )\
{\
	int isSet = p.back();\
	p.pop_back();\
	if ( isSet)	{\
	return FASTFACTORY_MAKE_5_##baseClass< A, B, C, D, 1>(  p );\
	}\
	return FASTFACTORY_MAKE_5_##baseClass<A, B, C, D, 0>(  p);\
}

#define FAST_FACTORY_BOOL5( baseClass )   \
	template<int A, int B, int C, int D, int E>		\
	baseClass*    FASTFACTORY_MAKE_5_##baseClass(  paramList& p )\
{\
	int isSet = p.back();\
	p.pop_back();\
	if ( isSet)	{\
	return FASTFACTORY_MAKE_6_##baseClass< A, B, C, D, E, 1>(  p );\
	}\
	return FASTFACTORY_MAKE_6_##baseClass<A, B, C, D, E, 0>(  p);\
}
#define FAST_FACTORY_BOOL6( baseClass  )   \
	template<int A, int B, int C, int D, int E, int F>		\
	baseClass*    FASTFACTORY_MAKE_6_##baseClass(  paramList& p )\
{\
	int isSet = p.back();\
	p.pop_back();\
	if ( isSet)	{\
	return FASTFACTORY_MAKE_7_##baseClass< A, B, C, D, E, F, 1>(  p );\
	}\
	return FASTFACTORY_MAKE_7_##baseClass<A, B, C, D, E, F, 0>(  p);\
}

#define FAST_FACTORY_BOOL7( baseClass  )   \
	template<int A, int B, int C, int D, int E, int F, int G>		\
	baseClass*    FASTFACTORY_MAKE_7_##baseClass(  paramList& p )\
{\
	int isSet = p.back();\
	p.pop_back();\
	if ( isSet)	{\
	return FASTFACTORY_MAKE_8_##baseClass< A, B, C, D, E, F, G,  1>(  p );\
	}\
	return FASTFACTORY_MAKE_8_##baseClass<A, B, C, D, E, F, G, 0>(  p);\
}

//---------------------------------- Nested Functions ------------------------------------

#define FAST_FACTORY_NEST1_BOOL0( baseClass )   \
	baseClass*    FASTFACTORY_MAKE_##baseClass(  paramList& p )\
					{\
					int isSet = p.back();\
					p.pop_back();\
					if ( isSet)	{\
					return FASTFACTORY_MAKE_1_##baseClass< 1>(  p );\
					}\
					p.pop_back(); return FASTFACTORY_MAKE_2_##baseClass< 0, 0>(  p);\
					}


#define FAST_FACTORY_NEST1_BOOL1( baseClass )    \
	template<int A>		\
	baseClass*    FASTFACTORY_MAKE_1_##baseClass(  paramList& p )\
{\
	int isSet = p.back();\
	p.pop_back();\
	if ( isSet)	{\
	return FASTFACTORY_MAKE_2_##baseClass< A, 1>(  p );\
	}\
	p.pop_back(); return FASTFACTORY_MAKE_3_##baseClass<A, 0, 0 >(  p);\
}

#define FAST_FACTORY_NEST1_BOOL2( baseClass )    \
	template<int A, int B>		\
	baseClass*    FASTFACTORY_MAKE_2_##baseClass(  paramList& p )\
{\
	int isSet = p.back();\
	p.pop_back();\
	if ( isSet)	{\
	return FASTFACTORY_MAKE_3_##baseClass< A, B,  1>(  p );\
	}\
	p.pop_back(); return FASTFACTORY_MAKE_4_##baseClass<A, B, 0, 0 >(  p);\
}

#define FAST_FACTORY_NEST1_BOOL3( baseClass )    \
	template<int A, int B, int C>		\
	baseClass*    FASTFACTORY_MAKE_3_##baseClass(  paramList& p )\
{\
	int isSet = p.back();\
	p.pop_back();\
	if ( isSet)	{\
	return FASTFACTORY_MAKE_4_##baseClass< A, B, C, 1>(  p );\
	}\
	p.pop_back(); return FASTFACTORY_MAKE_5_##baseClass<A, B, C, 0, 0>(  p);\
}

#define FAST_FACTORY_NEST1_BOOL4( baseClass )   \
	template<int A, int B, int C, int D>		\
	baseClass*    FASTFACTORY_MAKE_4_##baseClass(  paramList& p )\
{\
	int isSet = p.back();\
	p.pop_back();\
	if ( isSet)	{\
	return FASTFACTORY_MAKE_5_##baseClass< A, B, C, D, 1>(  p );\
	}\
	p.pop_back(); return FASTFACTORY_MAKE_6_##baseClass<A, B, C, D, 0, 0 >(  p);\
}

#define FAST_FACTORY_NEST1_BOOL5( baseClass )    \
	template<int A, int B, int C, int D, int E>		\
	baseClass*    FASTFACTORY_MAKE_5_##baseClass(  paramList& p )\
{\
	int isSet = p.back();\
	p.pop_back();\
	if ( isSet)	{\
	return FASTFACTORY_MAKE_6_##baseClass< A, B, C, D, E, 1>(  p );\
	}\
	p.pop_back();  return FASTFACTORY_MAKE_7_##baseClass<A, B, C, D, E, 0, 0>(  p);\
}
#define FAST_FACTORY_NEST1_BOOL6( baseClass )    \
	template<int A, int B, int C, int D, int E, int F>		\
	baseClass*    FASTFACTORY_MAKE_6_##baseClass(  paramList& p )\
{\
	int isSet = p.back();\
	p.pop_back();\
	if ( isSet)	{\
	return FASTFACTORY_MAKE_7_##baseClass< A, B, C, D, E, F, 1>(  p );\
	}\
	p.pop_back(); return FASTFACTORY_MAKE_8_##baseClass<A, B, C, D, E, F, 0, 0>(  p);\
}


//----------------------------------------------------------------------

#define FAST_FACTORY_FINISH1( baseClass , concreteClass )\
	template<int A >		\
	baseClass*    FASTFACTORY_MAKE_1_##baseClass( paramList&) {	return rage_new concreteClass<A >(); }

#define FAST_FACTORY_FINISH2( baseClass , concreteClass )\
	template<int A, int B >		\
	baseClass*    FASTFACTORY_MAKE_2_##baseClass( paramList&) {	return rage_new concreteClass<A, B >(); }

#define FAST_FACTORY_FINISH3( baseClass , concreteClass )\
	template<int A, int B, int C>		\
	baseClass*    FASTFACTORY_MAKE_3_##baseClass( paramList&) {	return rage_new concreteClass<A, B, C>(); }

#define FAST_FACTORY_FINISH4( baseClass , concreteClass )\
	template<int A, int B, int C, int D>		\
	baseClass*    FASTFACTORY_MAKE_4_##baseClass( paramList&) {	return rage_new concreteClass<A, B, C, D>(); }

#define FAST_FACTORY_FINISH5( baseClass , concreteClass )\
	template<int A, int B, int C, int D, int E>		\
	baseClass*    FASTFACTORY_MAKE_5_##baseClass( paramList&) {	return rage_new concreteClass<A, B, C, D, E>(); }

#define FAST_FACTORY_FINISH6( baseClass , concreteClass)\
	template<int A, int B, int C, int D, int E, int F >		\
	baseClass*    FASTFACTORY_MAKE_6_##baseClass( paramList&) {	return rage_new concreteClass<A, B, C, D, E, F>(); }

#define FAST_FACTORY_FINISH7( baseClass , concreteClass )\
	template<int A, int B, int C, int D, int E, int F, int G >		\
	baseClass*    FASTFACTORY_MAKE_7_##baseClass( paramList&) {	return rage_new concreteClass<A, B, C, D, E, F, G>(); }

#define FAST_FACTORY_FINISH8( baseClass , concreteClass )\
	template<int A, int B, int C, int D, int E, int F, int G, int H >		\
	baseClass*    FASTFACTORY_MAKE_8_##baseClass( paramList& ) {	return rage_new concreteClass<A, B, C, D, E, F, G, H>(); }

#endif
