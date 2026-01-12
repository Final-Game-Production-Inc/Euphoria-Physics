// 
// data/marker.h 
// 
// Copyright (C) 1999-2008 Rockstar Games.  All Rights Reserved. 
// 

#ifndef DATA_MARKER_H 
#define DATA_MARKER_H 

#if __PS3
#include <stdint.h>
#endif

// NOTE:	Let's try not to include this file in other header files so that it's cheap to change back and forth.
//			Also, this means that the .cpp files can override the ENABLE_RAGE_MARKERS definition to locally enable
//			or disable the feature (of course, marker.cpp will need to be modified too if we're locally enabling
//			the feature).
#ifndef RAGE_ENABLE_MARKERS
#define RAGE_ENABLE_MARKERS		(__DEV)
#endif

#define RAGE_USE_DEJA	0 // (!__WIN32PC && !__FINAL && !__PSP2 && !(__XENON && !__OPTIMIZED))

#if RAGE_USE_DEJA
#include "data/DejaLib.h"
#endif

namespace rage {

#if RAGE_ENABLE_MARKERS

#ifdef __GNUC__
#define RAGE_FUNC_NAME			__PRETTY_FUNCTION__		// virtual void someClass::SomeMember(argType argName,...) (on GCC, __FUNCTION__ would just be SomeMember, which isn't very useful)
#else
#define RAGE_FUNC_NAME			__FUNCTION__			// someClass::SomeMember
#endif

#if RAGE_USE_DEJA

#define RAGE_FUNC()				    static deja_context_reg DejaContextReg( RAGE_FUNC_NAME, __FILE__, __LINE__ ); deja_context DejaContext( &DejaContextReg );

// Must have one of these in scope before passing the name to RAGE_OBJECT_INIT.
// The actual support in Deja is much more sophisticated than this, akin to our DeclareStruct functions.
#define RAGE_OBJECT_DECLARE(__type) \
	static void DejaDescriptor( const __type& O ) \
	{ \
		DEJA_TYPE( O, __type ); \
	}

#define RAGE_OBJECT_INIT(objRef)			DEJA_OBJECT_INIT(objRef)
#define RAGE_OBJECT_LABEL(objRef,fmt,...)	DEJA_OBJECT_LABEL(objRef,fmt,__VA_ARGS__)
#define RAGE_OBJECT_KILL(objRef)			DEJA_OBJECT_KILL(objRef)
#define RAGE_SCOPE_OBJECT(objRef)			DEJA_SCOPE_OBJECT(objRef)

#else		// !RAGE_USE_DEJA

#define RAGE_FUNC()				static ::rage::Context MacroJoin(func,__LINE__) = { RAGE_FUNC_NAME, (::rage::Context*)-1, 0 }; RAGE_MARKER(MacroJoin(func,__LINE__));
#define RAGE_CONTEXT(tag)		::rage::Context tag(#tag)
#define RAGE_MARKER(tag)		::rage::Marker MacroJoin(marker,__LINE__) (tag)
#define RAGE_MARKER_PUSH(tag)	Marker::Push(tag)
#define RAGE_MARKER_POP()		Marker::Pop()

#define RAGE_OBJECT_DECLARE(__type)

#define RAGE_OBJECT_INIT(objRef)
#define RAGE_OBJECT_LABEL(objRef,fmt,...)
#define RAGE_OBJECT_KILL(objRef)
#define RAGE_SCOPE_OBJECT(objRef)

// This is intentionally a POD to avoid a bunch of static constructed objects; the compiler has to generate a static bool, test it,
// and jump around construction, every time.  Even worse, if there was a destructor, it would have to call atexit and potentially
// overrun that internal list as well.
struct Context {
	const char *DisplayName;
	Context *Next;
	s32 Value;
	static Context *First;
} SPU_ONLY();

// This returns whatever value we're measuring -- cache misses, memory consumed, cpu cycle counts
extern u32 (*g_RageMeasureValue)(void);

struct Marker {
	__forceinline Marker(Context &tag) { RAGE_MARKER_PUSH(tag); }
	__forceinline ~Marker() { RAGE_MARKER_POP(); }

	static void Push(Context &tag);
	static void Pop();
};

#endif	// !RAGE_USE_DEJA

#else	// !RAGE_ENABLE_MARKERS

#define RAGE_OBJECT_DECLARE(__type)

#define RAGE_OBJECT_INIT(objRef)
#define RAGE_OBJECT_LABEL(objRef,fmt,...)
#define RAGE_OBJECT_KILL(objRef)
#define RAGE_SCOPE_OBJECT(objRef)

#define RAGE_FUNC()
#define RAGE_CONTEXT(tag)
#define RAGE_MARKER(tag)
#define RAGE_MARKER_PUSH(tag)
#define RAGE_MARKER_POP()

#endif	// !RAGE_ENABLE_MARKERS

} // namespace rage

#else	// DATA_MARKER_H

#if !defined(__UNITYBUILD) && !defined(DATA_MARKER_INTENTIONAL_HEADER_INCLUDE)
#error	"Please don't include data/marker.h from a header file; it should only be included once per .cpp file that wants it."
#endif

#endif // DATA_MARKER_H 
