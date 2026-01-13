//
// system/lambda.h
//
// Copyright (C) 2015-2018 Rockstar Games.  All Rights Reserved.
//

#ifndef SYSTEM_LAMBDA_H
#define SYSTEM_LAMBDA_H

#include "ipc.h"
#include "new.h"
#include "diag/trap.h"

namespace rage {

#define LAMBDA_COPY_DEFAULT_MAX_SIZE_BYTES      64

#if __ASSERT
#	define LAMBDA_ASSERT_OR_TRAP_ONLY(...)          __VA_ARGS__
#elif ENABLE_TRAPS
#	define LAMBDA_ASSERT_OR_TRAP_ONLY(...)          __VA_ARGS__
#else
#	define LAMBDA_ASSERT_OR_TRAP_ONLY(...)
#endif

#if __ASSERT
#	define LAMBDA_CHECK_COPY_CAPTURE_SIZE(SIZEOF_LAMBDA_TYPE, MAX_SIZE_BYTES)   FatalAssert(SIZEOF_LAMBDA_TYPE <= MAX_SIZE_BYTES)
#	define LAMBDA_CHECK_IS_VALID(B)                                             FatalAssert(B)
#else
#	define LAMBDA_CHECK_COPY_CAPTURE_SIZE(SIZEOF_LAMBDA_TYPE, MAX_SIZE_BYTES)   TrapGT(SIZEOF_LAMBDA_TYPE, MAX_SIZE_BYTES)
#	define LAMBDA_CHECK_IS_VALID(B)                                             TrapZ(!!(B))
#endif


enum LambdaRefStackCheck {
	LAMBDA_REF_DISABLE_CHECK_STACK_OBJECT,
	LAMBDA_REF_CHECK_STACK_OBJECT,
};


// Variadic template version, not supported until Visual Studio 2013.
#if (1 && (RSG_ORBIS || (defined(_MSC_VER) && _MSC_VER >= 1800)))
#	define LAMBDA_USE_VARIADIC_TEMPLATES    1
#else
#	define LAMBDA_USE_VARIADIC_TEMPLATES    0
#endif

#if LAMBDA_USE_VARIADIC_TEMPLATES

	template<class FUNCTION_SIGNATURE, unsigned MAX_SIZE_BYTES=~0u, LambdaRefStackCheck STACK_CHECK=LAMBDA_REF_CHECK_STACK_OBJECT>  class LambdaRef;
	template<class FUNCTION_SIGNATURE, unsigned MAX_SIZE_BYTES=LAMBDA_COPY_DEFAULT_MAX_SIZE_BYTES>                                  class LambdaCopy;

	// So that LambdaRef and LambdaCopy don't need to be templated on the lambda
	// type, they use templated function pointers.  These can then be optimized
	// out by the compiler, so that effectively the lambda function is being
	// called through just a regular function pointer.
	//
	// Compare this to std::function implementation which uses a polymorphic
	// class, so the lambda function is being called through a virtual function
	// table.
	//
	class LambdaPrivate {
	private:
		template<class FUNCTION_SIGNATURE, unsigned MAX_SIZE_BYTES, LambdaRefStackCheck STACK_CHECK>    friend class LambdaRef;
		template<class FUNCTION_SIGNATURE, unsigned MAX_SIZE_BYTES>                                     friend class LambdaCopy;

		// Rather than use a regular CompileTimeAssert, this helper class will
		// tell you the sizes in the error message.
		template<unsigned LAMBDA_SIZE_BYTES, unsigned MAX_SIZE_BYTES>
		struct CheckSize {
			typedef char Check[LAMBDA_SIZE_BYTES <= MAX_SIZE_BYTES ? 1 : -1];
		};
		//@@: range RANGE_LAMBDA_CALLER {
		template<class LAMBDA, class RETURN, class... PARAMS>
		static RETURN Caller(const void *capture, PARAMS... params) {
			// Notice that the const is being cast away from capture here.  That is to enable support for mutable lambdas.
			return ((LAMBDA*)capture)->operator()(params...);
		}
		//@@: } RANGE_LAMBDA_CALLER
		template<class LAMBDA>
		static void Copier(void *dstCapture, const void *srcCapture LAMBDA_ASSERT_OR_TRAP_ONLY(, size_t maxSizeBytes)) {
			LAMBDA_CHECK_COPY_CAPTURE_SIZE(sizeof(LAMBDA), maxSizeBytes);
			rage_placement_new(dstCapture) LAMBDA(*(const LAMBDA*)srcCapture);
		}

		template<class LAMBDA>
		static void Destructor(const void *capture) {
			((LAMBDA*)capture)->~LAMBDA();
			(void)capture;                  // if LAMBDA has no dtor, then some compilers give an unreferenced parameter warning
		}

		template<class RETURN, class... PARAMS>
		static RETURN FunctionCaller(const void *funcPtrPtr, PARAMS... params) {
			return (*((RETURN(**)(PARAMS...))funcPtrPtr))(params...);
		}

		static void FunctionCopier(void *dstFuncPtrPtr, const void *srcFuncPtrPtr LAMBDA_ASSERT_OR_TRAP_ONLY(, size_t maxSizeBytes));
	};

	// Lightweight version of std::function, used for passing lambda objects as
	// arguments to functions.  Unlike std::function, the lambda object is not
	// copied, so the callee function cannot hold onto a reference to it.
	//
	// WARNING: Because of the fact that LambdaRef only stores a pointer to a
	// lambda object, not take a copy of it, nasty bugs can be introduced when
	// the object that is being pointed to goes out of scope.  The naively
	// innocent looking code,
	//
	//      extern void Foo(LambdaRef<void()>);
	//      int x = 3;
	//      LambdaRef<void()> lr = [=](){bar(x);};
	//      Foo(lr);
	//
	// is wrong.  It is wrong for the same reason that
	//
	//      int *ptr = &int(3);
	//
	// is wrong. The code is saving the address of an rvalue.  In the LambdaRef
	// example, the lambda object is gone by the time Foo is called, so lr has a
	// dangling pointer; the behaviour is undefined.  In some optimized builds,
	// Microsoft compilers even skip calling the lambda constructor, so x will
	// not be copied.
	//
	// The correct way to write that code would be either
	//
	//      extern void Foo(LambdaRef<void()>);
	//      int x = 3;
	//      auto lr = [=](){bar(x);};
	//      Foo(lr);
	//
	// or
	//
	//      extern void Foo(LambdaRef<void()>);
	//      int x = 3;
	//      Foo([=](){bar(x);});
	//
	//
	//
	// One reason to use LambdaRef over std::function is that it will not cause
	// a memory allocation when the lambda object is too large to fit.  That
	// issue can be worked around by using std::ref, but that is error prone, as
	// callers may forget.  See
	// blog.demofox.org/2015/02/25/avoiding-the-performance-hazzards-of-stdfunction/.
	//
	// Another reason is that in Visual Studio 2013, the <functional> header
	// file will not compile with __vectorcall, see
	// connect.microsoft.com/VisualStudio/feedback/details/804357.
	//
	// Implementation concept based loosely on Microsoft's std::function and
	// Boost::function.
	//
	// Note that LambdaRef provides a MAX_SIZE_BYTES value (defaulting to ~0u)
	// purely for compile time error checking if the LambdaRef is going to be
	// copied into a LambdaCopy.
	//
	template<unsigned MAX_SIZE_BYTES, LambdaRefStackCheck STACK_CHECK, class RETURN, class... PARAMS>
	class LambdaRef<RETURN (PARAMS...), MAX_SIZE_BYTES, STACK_CHECK> {
	private:
		template<class FUNCTION_SIGNATURE, unsigned MAX_SIZE_BYTES_2> friend class LambdaCopy;

		// When the LambdaRef is constructed from a lambda, m_capture will point
		// to the lambda object, m_caller will point to a function that calls
		// the lambda's operator(), m_copier is a function that can copy the
		// lambda object, and m_destructor a function that can delete a lambda
		// object.
		//
		// When the LambdaRef is constructed from a function pointer, m_capture,
		// m_copier and m_destructor will all be NULL, and m_caller will be the
		// function pointer.
		//
		union {
			struct {
				const void *m_capture;
				RETURN (*m_caller)(const void*, PARAMS...);
				void (*m_copier)(void*, const void* LAMBDA_ASSERT_OR_TRAP_ONLY(, size_t));
				void (*m_destructor)(const void*);
			} m_lambda;
			struct {
				const void *m_capture;                                                      // NULL
				RETURN (*m_caller)(PARAMS...);
				void (*m_copier)(void*, const void* LAMBDA_ASSERT_OR_TRAP_ONLY(, size_t));  // NULL
				void (*m_destructor)(const void*);                                          // NULL
			} m_function;
		} u;

		typedef RETURN FUNCTION_SIGNATURE(PARAMS...);

	public:
		inline LambdaRef() {
			u.m_lambda.m_capture    = NULL;
			u.m_lambda.m_caller     = NULL;
			u.m_lambda.m_copier     = NULL;
			u.m_lambda.m_destructor = NULL;
			if (STACK_CHECK == LAMBDA_REF_CHECK_STACK_OBJECT) {
				// LambdaRef should only ever be temporary stack based objects,
				// since they do not take a copy of the lambda object passed to
				// them, only a reference.
				LAMBDA_CHECK_IS_VALID(sysIpcIsStackAddress(this));
			}
		}

		template<class LAMBDA>
		inline void Set(const LAMBDA &lambda) {
			(void)sizeof(LambdaPrivate::CheckSize<sizeof(LAMBDA), MAX_SIZE_BYTES>);
			u.m_lambda.m_capture    = &lambda;
			u.m_lambda.m_caller     = LambdaPrivate::Caller<LAMBDA, RETURN, PARAMS...>;
			u.m_lambda.m_copier     = LambdaPrivate::Copier<LAMBDA>;
			u.m_lambda.m_destructor = LambdaPrivate::Destructor<LAMBDA>;
		}

		template<class LAMBDA>
		/*implicit*/ inline LambdaRef(const LAMBDA &lambda) {
			if (STACK_CHECK == LAMBDA_REF_CHECK_STACK_OBJECT) {
				LAMBDA_CHECK_IS_VALID(sysIpcIsStackAddress(this));
			}
			Set(lambda);
		}

		template<class LAMBDA>
		inline LambdaRef &operator=(const LAMBDA &lambda) {
			Set(lambda);
			return *this;
		}

		inline void Set(RETURN (*funcPtr)(PARAMS...)) {
			u.m_function.m_capture    = NULL;
			u.m_function.m_caller     = funcPtr;
			u.m_function.m_copier     = NULL;
			u.m_function.m_destructor = NULL;
		}

		/*implicit*/ inline LambdaRef(RETURN (*funcPtr)(PARAMS...)) {
			Set(funcPtr);
		}

		inline LambdaRef &operator=(RETURN (*funcPtr)(PARAMS...)) {
			Set(funcPtr);
			return *this;
		}

		template<unsigned MAX_SIZE_BYTES_2>
		inline void Set(const LambdaCopy<RETURN (PARAMS...), MAX_SIZE_BYTES_2> &lambdaCopy);

		template<unsigned MAX_SIZE_BYTES_2>
		/*implicit*/ inline LambdaRef(const LambdaCopy<FUNCTION_SIGNATURE, MAX_SIZE_BYTES_2> &lambdaCopy) {
			if (STACK_CHECK == LAMBDA_REF_CHECK_STACK_OBJECT) {
				LAMBDA_CHECK_IS_VALID(sysIpcIsStackAddress(this));
			}
			Set(lambdaCopy);
		}

		template<unsigned MAX_SIZE_BYTES_2>
		inline LambdaRef &operator=(const LambdaCopy<FUNCTION_SIGNATURE, MAX_SIZE_BYTES_2> &lambdaCopy) {
			if (STACK_CHECK == LAMBDA_REF_CHECK_STACK_OBJECT) {
				LAMBDA_CHECK_IS_VALID(sysIpcIsStackAddress(this));
			}
			Set(lambdaCopy);
			return *this;
		}

		inline RETURN operator()(PARAMS... params) const {
			LAMBDA_CHECK_IS_VALID(u.m_lambda.m_caller);
			if (IsLambda())
				return u.m_lambda.m_caller(u.m_lambda.m_capture, params...);
			else
				return u.m_function.m_caller(params...);
		}

		inline operator bool() const {
			return IsValid();
		}

		inline bool IsValid() const {
            return !!u.m_lambda.m_caller;
		}

		inline void Invalidate() {
            u.m_lambda.m_caller = NULL;
		}

		inline bool IsLambda() const {
			return !!u.m_lambda.m_capture;
		}

		inline bool IsFunctionPointer() const {
			return !IsLambda();
		}
	};

	// Convenience class for LambdaRef with LAMBDA_REF_DISABLE_CHECK_STACK_OBJECT.
	//
	// There should be very few cases where this is ever needed.  In all but
	// maybe a few exceptional cases, LambdaRef should be used and the LambdaRef
	// object should be on the stack.
	//
	template<class FUNCTION_SIGNATURE_2, unsigned MAX_SIZE_BYTES=~0u>
	class LambdaRef_NoStackCheck : public LambdaRef<FUNCTION_SIGNATURE_2, MAX_SIZE_BYTES, LAMBDA_REF_DISABLE_CHECK_STACK_OBJECT> {
		using LambdaRef<FUNCTION_SIGNATURE_2, MAX_SIZE_BYTES, LAMBDA_REF_DISABLE_CHECK_STACK_OBJECT>::LambdaRef;
	};

	// Similar to LambdaRef, but does copy the lambda object.
	//
	// Unlike std::function, this will fail to compile if directly set to a
	// lambda object that is larger than MAX_SIZE_BYTES, as opposed to doing a
	// dynamic memory allocation.  Though if set via a LambdaRef, then the size
	// check is a runtime rather than compile time check.
	//
	// Note that capture needs to be 16-byte aligned in case there is something
	// in there that the lambda's copy constructor uses a vector operation on.
	// But adding ALIGNAS(16) simply does not work correctly and can be ignored
	// by both Clang and Microsoft compilers.  So instead m_captureBuf is sized
	// 16-bytes more than necessary, and the helper CapturePtr() will align
	// within that buffer.
	//
	template<unsigned MAX_SIZE_BYTES, class RETURN, class... PARAMS>
	class LambdaCopy<RETURN (PARAMS...), MAX_SIZE_BYTES> {
	private:
		template<class FUNCTION_SIGNATURE, unsigned MAX_SIZE_BYTES_2, LambdaRefStackCheck STACK_CHECK> friend class LambdaRef;

		// When the LambdaCopy is constructed from a lambda, m_capture will
		// point to the lambda object, m_caller will point to a function that
		// calls the lambda's operator(), m_copier is a function that can copy
		// the lambda object, and m_destructor a function that can delete a
		// lambda object.
		//
		// When the LambdaRef is constructed from a function pointer, m_capture,
		// m_copier and m_destructor will all be NULL, and m_caller will be the
		// function pointer.
		//
		char m_captureBuf[((MAX_SIZE_BYTES+sizeof(void*)-1)&~(sizeof(void*)-1))+16];
		RETURN (*m_caller)(const void*, PARAMS...);
		void (*m_copier)(void*, const void* LAMBDA_ASSERT_OR_TRAP_ONLY(, size_t));
		void (*m_destructor)(const void*);

		typedef RETURN FUNCTION_SIGNATURE(PARAMS...);

		inline const void *CapturePtr() const {
			return (void*)(((uptr)m_captureBuf+15)&~15);
		}

		inline void *CapturePtr() {
			return const_cast<void*>(const_cast<const LambdaCopy*>(this)->CapturePtr());
		}

		template<class LAMBDA>
		inline void SetInternal(const LAMBDA &lambda) {
			(void)sizeof(LambdaPrivate::CheckSize<sizeof(LAMBDA), MAX_SIZE_BYTES>);
			rage_placement_new(CapturePtr()) LAMBDA(lambda);
			m_caller     = LambdaPrivate::Caller<LAMBDA, RETURN, PARAMS...>;
			m_copier     = LambdaPrivate::Copier<LAMBDA>;
			m_destructor = LambdaPrivate::Destructor<LAMBDA>;
		}

		inline void SetInternal(RETURN (*funcPtr)(PARAMS...)) {
			CompileTimeAssert(MAX_SIZE_BYTES >= sizeof(funcPtr));
			*((RETURN(**)(PARAMS...))CapturePtr()) = funcPtr;
			m_caller     = LambdaPrivate::FunctionCaller<RETURN, PARAMS...>;
			m_copier     = LambdaPrivate::FunctionCopier;
			m_destructor = NULL;
		}

		template<unsigned MAX_SIZE_BYTES_2, LambdaRefStackCheck STACK_CHECK>
		inline void SetRefInternal(const LambdaRef<FUNCTION_SIGNATURE, MAX_SIZE_BYTES_2, STACK_CHECK> &lambdaRef) {
			if (lambdaRef.u.m_lambda.m_capture) {
				lambdaRef.u.m_lambda.m_copier(CapturePtr(), lambdaRef.u.m_lambda.m_capture LAMBDA_ASSERT_OR_TRAP_ONLY(, MAX_SIZE_BYTES));
				m_caller     = lambdaRef.u.m_lambda.m_caller;
				m_copier     = lambdaRef.u.m_lambda.m_copier;
				m_destructor = lambdaRef.u.m_lambda.m_destructor;
			}
			else {
				CompileTimeAssert(MAX_SIZE_BYTES >= sizeof(RETURN(*)(PARAMS...)));
				*((RETURN(**)(PARAMS...))CapturePtr()) = lambdaRef.u.m_function.m_caller;
				m_caller     = LambdaPrivate::FunctionCaller<RETURN, PARAMS...>;
				m_copier     = LambdaPrivate::FunctionCopier;
				m_destructor = NULL;
			}
		}

	public:
		inline LambdaCopy() : m_caller(NULL), m_copier(NULL), m_destructor(NULL) {
		}

		inline LambdaCopy(const LambdaCopy &other) {
			auto copier = other.m_copier;
			if (copier)
				copier(CapturePtr(), other.CapturePtr() LAMBDA_ASSERT_OR_TRAP_ONLY(, MAX_SIZE_BYTES));
			m_caller     = other.m_caller;
			m_copier     = copier;
			m_destructor = other.m_destructor;
		}

		inline ~LambdaCopy() {
			if (m_destructor)
				m_destructor(CapturePtr());
		}

		template<class LAMBDA>
		inline void Set(const LAMBDA &lambda) {
			this->~LambdaCopy();
			SetInternal(lambda);
		}

		template<unsigned MAX_SIZE_BYTES_2, LambdaRefStackCheck STACK_CHECK>
		inline void SetRef(const LambdaRef<FUNCTION_SIGNATURE, MAX_SIZE_BYTES_2, STACK_CHECK> &lambdaRef) {
			this->~LambdaCopy();
			SetRefInternal(lambdaRef);
		}

		template<unsigned MAX_SIZE_BYTES_2>
		inline void SetRef(const LambdaRef_NoStackCheck<FUNCTION_SIGNATURE, MAX_SIZE_BYTES_2> &lambdaRef) {
			SetRef(static_cast<const LambdaRef<FUNCTION_SIGNATURE, MAX_SIZE_BYTES_2, LAMBDA_REF_DISABLE_CHECK_STACK_OBJECT>&>(lambdaRef));
		}

		template<class LAMBDA>
		/*implicit*/ inline LambdaCopy(const LAMBDA &lambda) {
			SetInternal(lambda);
		}

		template<unsigned MAX_SIZE_BYTES_2, LambdaRefStackCheck STACK_CHECK>
		/*implicit*/ inline LambdaCopy(const LambdaRef<FUNCTION_SIGNATURE, MAX_SIZE_BYTES_2, STACK_CHECK> &lambdaRef) {
			SetRefInternal(lambdaRef);
		}

		inline LambdaCopy &operator=(const LambdaCopy &other) {
			this->~LambdaCopy();
			rage_placement_new(this) LambdaCopy(other);
			return *this;
		}

		template<class LAMBDA>
		inline LambdaCopy &operator=(const LAMBDA &lambda) {
			Set(lambda);
			return *this;
		}

		template<unsigned MAX_SIZE_BYTES_2, LambdaRefStackCheck STACK_CHECK>
		inline LambdaCopy &operator=(const LambdaRef<FUNCTION_SIGNATURE, MAX_SIZE_BYTES_2, STACK_CHECK> &lambdaRef) {
			SetRef(lambdaRef);
			return *this;
		}

		template<unsigned MAX_SIZE_BYTES_2>
		inline LambdaCopy &operator=(const LambdaRef_NoStackCheck<FUNCTION_SIGNATURE, MAX_SIZE_BYTES_2> &lambdaRef) {
			return operator=(static_cast<const LambdaRef<FUNCTION_SIGNATURE, MAX_SIZE_BYTES_2, LAMBDA_REF_DISABLE_CHECK_STACK_OBJECT>&>(lambdaRef));
		}

		inline RETURN operator()(PARAMS... params) const {
			LAMBDA_CHECK_IS_VALID(m_caller);
			return m_caller(CapturePtr(), params...);
		}

		inline operator bool() const {
			return IsValid();
		}

		inline bool IsValid() const {
            return !!m_caller;
		}

		inline void Invalidate() {
            m_caller = NULL;
		}

		inline bool IsFunctionPointer() const {
			return !m_destructor;
		}

		inline bool IsLambda() const {
			return !IsFunctionPointer();
		}
	};

	template<unsigned MAX_SIZE_BYTES, LambdaRefStackCheck STACK_CHECK, class RETURN, class... PARAMS>
	template<unsigned MAX_SIZE_BYTES_2>
	inline void LambdaRef<RETURN (PARAMS...), MAX_SIZE_BYTES, STACK_CHECK>::Set(const LambdaCopy<RETURN (PARAMS...), MAX_SIZE_BYTES_2> &lambdaCopy) {
		(void)sizeof(LambdaPrivate::CheckSize<MAX_SIZE_BYTES_2, MAX_SIZE_BYTES>);
		if (lambdaCopy.IsLambda()) {
			u.m_lambda.m_capture      = lambdaCopy.CapturePtr();
			u.m_lambda.m_caller       = lambdaCopy.m_caller;
			u.m_lambda.m_copier       = lambdaCopy.m_copier;
			u.m_lambda.m_destructor   = lambdaCopy.m_destructor;
		}
		else {
			u.m_function.m_capture    = NULL;
			u.m_function.m_caller     = *(RETURN(**)(PARAMS...))(lambdaCopy.CapturePtr());
			u.m_function.m_copier     = NULL;
			u.m_function.m_destructor = NULL;
		}
	}

// Lame non-variadic template version.
// Delete this once variadic templates are supported on all platforms.
#else

	template<class FUNCTION_SIGNATURE, unsigned MAX_SIZE_BYTES=~0u, LambdaRefStackCheck STACK_CHECK=LAMBDA_REF_CHECK_STACK_OBJECT>  class LambdaRef;
	template<class FUNCTION_SIGNATURE, unsigned MAX_SIZE_BYTES=~0u>                                                                 class LambdaRef_NoStackCheck;
	template<class FUNCTION_SIGNATURE, unsigned MAX_SIZE_BYTES=LAMBDA_COPY_DEFAULT_MAX_SIZE_BYTES>                                  class LambdaCopy;

	namespace LambdaPrivate {
		template<unsigned LAMBDA_SIZE_BYTES, unsigned MAX_SIZE_BYTES>
		struct CheckSize {
			typedef char Check[LAMBDA_SIZE_BYTES <= MAX_SIZE_BYTES ? 1 : -1];
		};

		template<class LAMBDA>
		static void Copier(void *dstCapture, const void *srcCapture LAMBDA_ASSERT_OR_TRAP_ONLY(, size_t maxSizeBytes)) {
			LAMBDA_CHECK_COPY_CAPTURE_SIZE(sizeof(LAMBDA), maxSizeBytes);
			rage_placement_new(dstCapture) LAMBDA(*(const LAMBDA*)srcCapture);
		}

		template<class LAMBDA>
		static void Destructor(const void *capture) {
			((LAMBDA*)capture)->~LAMBDA();
			(void)capture;
		}

		void FunctionCopier(void *dstFuncPtrPtr, const void *srcFuncPtrPtr LAMBDA_ASSERT_OR_TRAP_ONLY(, size_t maxSizeBytes));
	}

#	define LAMBDA_COMMA                         ,
#	define LAMBDA_CLASS_TYPE(TYPE, NAME)        class TYPE
#	define LAMBDA_TYPE_NAME(TYPE, NAME)         TYPE NAME
#	define LAMBDA_TYPE(TYPE, NAME)              TYPE
#	define LAMBDA_NAME(TYPE, NAME)              NAME

#	define LAMBDA_SPECIALIZATIONS(NUM_PARAMS, TYPE_ARGS_FUNC)                                                                                   \
                                                                                                                                                \
		namespace LambdaPrivate {                                                                                                               \
			template<class LAMBDA, class RETURN TYPE_ARGS_FUNC(LAMBDA_COMMA, LAMBDA_CLASS_TYPE)>                                                \
			static RETURN Caller(const void *capture TYPE_ARGS_FUNC(LAMBDA_COMMA, LAMBDA_TYPE_NAME)) {                                          \
				return ((LAMBDA*)capture)->operator()(TYPE_ARGS_FUNC(, LAMBDA_NAME));                                                           \
			}                                                                                                                                   \
                                                                                                                                                \
			template<class RETURN TYPE_ARGS_FUNC(LAMBDA_COMMA, LAMBDA_CLASS_TYPE)>                                                              \
			static RETURN FunctionCaller(const void *funcPtrPtr TYPE_ARGS_FUNC(LAMBDA_COMMA, LAMBDA_TYPE_NAME)) {                               \
				return (*((RETURN(**)(TYPE_ARGS_FUNC(, LAMBDA_TYPE)))funcPtrPtr))(TYPE_ARGS_FUNC(, LAMBDA_NAME));                               \
			}                                                                                                                                   \
		}                                                                                                                                       \
                                                                                                                                                \
		template<unsigned MAX_SIZE_BYTES, LambdaRefStackCheck STACK_CHECK, class RETURN TYPE_ARGS_FUNC(LAMBDA_COMMA, LAMBDA_CLASS_TYPE)>        \
		class LambdaRef<RETURN (TYPE_ARGS_FUNC(, LAMBDA_TYPE)), MAX_SIZE_BYTES, STACK_CHECK> {                                                  \
		private:                                                                                                                                \
			template<class FUNCTION_SIGNATURE, unsigned MAX_SIZE_BYTES_2> friend class LambdaCopy;                                              \
                                                                                                                                                \
			union {                                                                                                                             \
				struct {                                                                                                                        \
					const void *m_capture;                                                                                                      \
					RETURN (*m_caller)(const void* TYPE_ARGS_FUNC(LAMBDA_COMMA, LAMBDA_TYPE));                                                  \
					void (*m_copier)(void*, const void* LAMBDA_ASSERT_OR_TRAP_ONLY(, size_t));                                                  \
					void (*m_destructor)(const void*);                                                                                          \
				} m_lambda;                                                                                                                     \
				struct {                                                                                                                        \
					const void *m_capture;                                                                                                      \
					RETURN (*m_caller)(TYPE_ARGS_FUNC(, LAMBDA_TYPE));                                                                          \
					void (*m_copier)(void*, const void* LAMBDA_ASSERT_OR_TRAP_ONLY(, size_t));                                                  \
					void (*m_destructor)(const void*);                                                                                          \
				} m_function;                                                                                                                   \
			} u;                                                                                                                                \
                                                                                                                                                \
			typedef RETURN FUNCTION_SIGNATURE(TYPE_ARGS_FUNC(, LAMBDA_TYPE));                                                                   \
                                                                                                                                                \
		public:                                                                                                                                 \
			inline LambdaRef() {                                                                                                                \
				u.m_lambda.m_capture    = NULL;                                                                                                 \
				u.m_lambda.m_caller     = NULL;                                                                                                 \
				u.m_lambda.m_copier     = NULL;                                                                                                 \
				u.m_lambda.m_destructor = NULL;                                                                                                 \
				if (STACK_CHECK == LAMBDA_REF_CHECK_STACK_OBJECT) {                                                                             \
					LAMBDA_CHECK_IS_VALID(sysIpcIsStackAddress(this));                                                                          \
				}                                                                                                                               \
			}                                                                                                                                   \
                                                                                                                                                \
			template<class LAMBDA>                                                                                                              \
			inline void Set(const LAMBDA &lambda) {                                                                                             \
				(void)sizeof(LambdaPrivate::CheckSize<sizeof(LAMBDA), MAX_SIZE_BYTES>);                                                         \
				u.m_lambda.m_capture    = &lambda;                                                                                              \
				u.m_lambda.m_caller     = LambdaPrivate::Caller<LAMBDA, RETURN TYPE_ARGS_FUNC(LAMBDA_COMMA, LAMBDA_TYPE)>;                      \
				u.m_lambda.m_copier     = LambdaPrivate::Copier<LAMBDA>;                                                                        \
				u.m_lambda.m_destructor = LambdaPrivate::Destructor<LAMBDA>;                                                                    \
			}                                                                                                                                   \
                                                                                                                                                \
			template<class LAMBDA>                                                                                                              \
			inline LambdaRef(const LAMBDA &lambda) {                                                                                            \
				if (STACK_CHECK == LAMBDA_REF_CHECK_STACK_OBJECT) {                                                                             \
					LAMBDA_CHECK_IS_VALID(sysIpcIsStackAddress(this));                                                                          \
				}                                                                                                                               \
				Set(lambda);                                                                                                                    \
			}                                                                                                                                   \
                                                                                                                                                \
			template<class LAMBDA>                                                                                                              \
			inline LambdaRef &operator=(const LAMBDA &lambda) {                                                                                 \
				Set(lambda);                                                                                                                    \
				return *this;                                                                                                                   \
			}                                                                                                                                   \
                                                                                                                                                \
			inline void Set(RETURN (*funcPtr)(TYPE_ARGS_FUNC(, LAMBDA_TYPE))) {                                                                 \
				u.m_function.m_capture    = NULL;                                                                                               \
				u.m_function.m_caller     = funcPtr;                                                                                            \
				u.m_function.m_copier     = NULL;                                                                                               \
				u.m_function.m_destructor = NULL;                                                                                               \
			}                                                                                                                                   \
                                                                                                                                                \
			/*implicit*/ inline LambdaRef(RETURN (*funcPtr)(TYPE_ARGS_FUNC(, LAMBDA_TYPE))) {                                                   \
				Set(funcPtr);                                                                                                                   \
			}                                                                                                                                   \
                                                                                                                                                \
			inline LambdaRef &operator=(RETURN (*funcPtr)(TYPE_ARGS_FUNC(, LAMBDA_TYPE))) {                                                     \
				Set(funcPtr);                                                                                                                   \
				return *this;                                                                                                                   \
			}                                                                                                                                   \
                                                                                                                                                \
			template<unsigned MAX_SIZE_BYTES_2>                                                                                                 \
			inline void Set(const LambdaCopy<RETURN (TYPE_ARGS_FUNC(, LAMBDA_TYPE)), MAX_SIZE_BYTES_2> &lambdaCopy);                            \
                                                                                                                                                \
			template<unsigned MAX_SIZE_BYTES_2>                                                                                                 \
			/*implicit*/ inline LambdaRef(const LambdaCopy<FUNCTION_SIGNATURE, MAX_SIZE_BYTES_2> &lambdaCopy) {                                 \
				if (STACK_CHECK == LAMBDA_REF_CHECK_STACK_OBJECT) {                                                                             \
					LAMBDA_CHECK_IS_VALID(sysIpcIsStackAddress(this));                                                                          \
				}                                                                                                                               \
				Set(lambdaCopy);                                                                                                                \
			}                                                                                                                                   \
                                                                                                                                                \
			template<unsigned MAX_SIZE_BYTES_2>                                                                                                 \
			inline LambdaRef &operator=(const LambdaCopy<FUNCTION_SIGNATURE, MAX_SIZE_BYTES_2> &lambdaCopy) {                                   \
				if (STACK_CHECK == LAMBDA_REF_CHECK_STACK_OBJECT) {                                                                             \
					LAMBDA_CHECK_IS_VALID(sysIpcIsStackAddress(this));                                                                          \
				}                                                                                                                               \
				Set(lambdaCopy);                                                                                                                \
				return *this;                                                                                                                   \
			}                                                                                                                                   \
                                                                                                                                                \
			inline RETURN operator()(TYPE_ARGS_FUNC(, LAMBDA_TYPE_NAME)) const {                                                                \
				LAMBDA_CHECK_IS_VALID(u.m_lambda.m_caller);                                                                                     \
				if (IsLambda())                                                                                                                 \
					return u.m_lambda.m_caller(u.m_lambda.m_capture TYPE_ARGS_FUNC(LAMBDA_COMMA, LAMBDA_NAME));                                 \
				else                                                                                                                            \
					return u.m_function.m_caller(TYPE_ARGS_FUNC(, LAMBDA_NAME));                                                                \
			}                                                                                                                                   \
																																				\
			inline operator bool() const {																										\
				return IsValid();                                                                                                               \
			}                                                                                                                                   \
                                                                                                                                                \
			inline bool IsValid() const {                                                                                                       \
	            return !!u.m_lambda.m_caller;                                                                                                   \
			}                                                                                                                                   \
																																				\
			inline void Invalidate() {                                                                                                          \
				u.m_lambda.m_caller = NULL;                                                                                                     \
			}                                                                                                                                   \
                                                                                                                                                \
			inline bool IsLambda() const {                                                                                                      \
				return !!u.m_lambda.m_capture;                                                                                                  \
			}                                                                                                                                   \
                                                                                                                                                \
			inline bool IsFunctionPointer() const {                                                                                             \
				return !IsLambda();                                                                                                             \
			}                                                                                                                                   \
		};                                                                                                                                      \
                                                                                                                                                \
		template<unsigned MAX_SIZE_BYTES, class RETURN TYPE_ARGS_FUNC(LAMBDA_COMMA, LAMBDA_CLASS_TYPE)>                                         \
		class LambdaCopy<RETURN (TYPE_ARGS_FUNC(, LAMBDA_TYPE)), MAX_SIZE_BYTES> {                                                              \
		private:                                                                                                                                \
			template<class FUNCTION_SIGNATURE, unsigned MAX_SIZE_BYTES_2, LambdaRefStackCheck STACK_CHECK> friend class LambdaRef;              \
	                                                                                                                                            \
			char m_captureBuf[((MAX_SIZE_BYTES+sizeof(void*)-1)&~(sizeof(void*)-1))+16];                                                        \
			RETURN (*m_caller)(const void* TYPE_ARGS_FUNC(LAMBDA_COMMA, LAMBDA_TYPE));                                                          \
			void (*m_copier)(void*, const void* LAMBDA_ASSERT_OR_TRAP_ONLY(, size_t));                                                          \
			void (*m_destructor)(const void*);                                                                                                  \
	                                                                                                                                            \
			typedef RETURN FUNCTION_SIGNATURE(TYPE_ARGS_FUNC(, LAMBDA_TYPE));                                                                   \
                                                                                                                                                \
			inline const void *CapturePtr() const {                                                                                             \
				return (void*)(((uptr)m_captureBuf+15)&~15);                                                                                    \
			}                                                                                                                                   \
                                                                                                                                                \
			inline void *CapturePtr() {                                                                                                         \
				return const_cast<void*>(const_cast<const LambdaCopy*>(this)->CapturePtr());                                                    \
			}                                                                                                                                   \
                                                                                                                                                \
			template<class LAMBDA>                                                                                                              \
			inline void SetInternal(const LAMBDA &lambda) {                                                                                     \
				(void)sizeof(LambdaPrivate::CheckSize<sizeof(LAMBDA), MAX_SIZE_BYTES>);                                                         \
				rage_placement_new(CapturePtr()) LAMBDA(lambda);                                                                                \
				m_caller     = LambdaPrivate::Caller<LAMBDA, RETURN TYPE_ARGS_FUNC(LAMBDA_COMMA, LAMBDA_TYPE)>;                                 \
				m_copier     = LambdaPrivate::Copier<LAMBDA>;                                                                                   \
				m_destructor = LambdaPrivate::Destructor<LAMBDA>;                                                                               \
			}                                                                                                                                   \
	                                                                                                                                            \
			inline void SetInternal(RETURN (*funcPtr)(TYPE_ARGS_FUNC(, LAMBDA_TYPE))) {                                                         \
				CompileTimeAssert(MAX_SIZE_BYTES >= sizeof(funcPtr));                                                                           \
				*((RETURN(**)(TYPE_ARGS_FUNC(, LAMBDA_TYPE)))CapturePtr()) = funcPtr;                                                           \
				m_caller     = LambdaPrivate::FunctionCaller<RETURN TYPE_ARGS_FUNC(LAMBDA_COMMA, LAMBDA_TYPE)>;                                 \
				m_copier     = LambdaPrivate::FunctionCopier;                                                                                   \
				m_destructor = NULL;                                                                                                            \
			}                                                                                                                                   \
                                                                                                                                                \
			template<unsigned MAX_SIZE_BYTES_2, LambdaRefStackCheck STACK_CHECK>                                                                \
			inline void SetRefInternal(const LambdaRef<FUNCTION_SIGNATURE, MAX_SIZE_BYTES_2, STACK_CHECK> &lambdaRef) {                         \
				if (lambdaRef.u.m_lambda.m_capture) {                                                                                           \
					lambdaRef.u.m_lambda.m_copier(CapturePtr(), lambdaRef.u.m_lambda.m_capture LAMBDA_ASSERT_OR_TRAP_ONLY(, MAX_SIZE_BYTES));   \
					m_caller     = lambdaRef.u.m_lambda.m_caller;                                                                               \
					m_copier     = lambdaRef.u.m_lambda.m_copier;                                                                               \
					m_destructor = lambdaRef.u.m_lambda.m_destructor;                                                                           \
				}                                                                                                                               \
				else {                                                                                                                          \
					CompileTimeAssert(MAX_SIZE_BYTES >= sizeof(RETURN(*)(TYPE_ARGS_FUNC(, LAMBDA_TYPE))));                                      \
					*((RETURN(**)(TYPE_ARGS_FUNC(, LAMBDA_TYPE)))CapturePtr()) = lambdaRef.u.m_function.m_caller;                               \
					m_caller     = LambdaPrivate::FunctionCaller<RETURN TYPE_ARGS_FUNC(LAMBDA_COMMA, LAMBDA_TYPE)>;                             \
					m_copier     = LambdaPrivate::FunctionCopier;                                                                               \
					m_destructor = NULL;                                                                                                        \
				}                                                                                                                               \
			}                                                                                                                                   \
	                                                                                                                                            \
		public:                                                                                                                                 \
			inline LambdaCopy() : m_caller(NULL), m_copier(NULL), m_destructor(NULL) {                                                          \
			}                                                                                                                                   \
	                                                                                                                                            \
			inline LambdaCopy(const LambdaCopy &other) {                                                                                        \
				auto copier = other.m_copier;                                                                                                   \
				if (copier)                                                                                                                     \
					copier(CapturePtr(), other.CapturePtr() LAMBDA_ASSERT_OR_TRAP_ONLY(, MAX_SIZE_BYTES));                                      \
				m_caller     = other.m_caller;                                                                                                  \
				m_copier     = copier;                                                                                                          \
				m_destructor = other.m_destructor;                                                                                              \
			}                                                                                                                                   \
	                                                                                                                                            \
			inline ~LambdaCopy() {                                                                                                              \
				if (m_destructor)                                                                                                               \
					m_destructor(CapturePtr());                                                                                                 \
			}                                                                                                                                   \
	                                                                                                                                            \
			template<class LAMBDA>                                                                                                              \
			inline void Set(const LAMBDA &lambda) {                                                                                             \
				this->~LambdaCopy();                                                                                                            \
				SetInternal(lambda);                                                                                                            \
			}                                                                                                                                   \
	                                                                                                                                            \
			template<unsigned MAX_SIZE_BYTES_2, LambdaRefStackCheck STACK_CHECK>                                                                \
			inline void SetRef(const LambdaRef<FUNCTION_SIGNATURE, MAX_SIZE_BYTES_2, STACK_CHECK> &lambdaRef) {                                 \
				this->~LambdaCopy();                                                                                                            \
				SetRefInternal(lambdaRef);                                                                                                      \
			}                                                                                                                                   \
	                                                                                                                                            \
			template<unsigned MAX_SIZE_BYTES_2>                                                                                                 \
			inline void SetRef(const LambdaRef_NoStackCheck<FUNCTION_SIGNATURE, MAX_SIZE_BYTES_2> &lambdaRef) {                                 \
				SetRef(static_cast<const LambdaRef<FUNCTION_SIGNATURE, MAX_SIZE_BYTES_2, LAMBDA_REF_DISABLE_CHECK_STACK_OBJECT>&>(lambdaRef));  \
			}                                                                                                                                   \
	                                                                                                                                            \
			template<class LAMBDA>                                                                                                              \
			inline LambdaCopy(const LAMBDA &lambda) {                                                                                           \
				SetInternal(lambda);                                                                                                            \
			}                                                                                                                                   \
	                                                                                                                                            \
			template<unsigned MAX_SIZE_BYTES_2, LambdaRefStackCheck STACK_CHECK>                                                                \
			inline LambdaCopy(const LambdaRef<FUNCTION_SIGNATURE, MAX_SIZE_BYTES_2, STACK_CHECK> &lambdaRef) {                                  \
				SetRefInternal(lambdaRef);                                                                                                      \
			}                                                                                                                                   \
	                                                                                                                                            \
			inline LambdaCopy &operator=(const LambdaCopy &other) {                                                                             \
				this->~LambdaCopy();                                                                                                            \
				rage_placement_new(this) LambdaCopy(other);                                                                                     \
				return *this;                                                                                                                   \
			}                                                                                                                                   \
	                                                                                                                                            \
			template<class LAMBDA>                                                                                                              \
			inline LambdaCopy &operator=(const LAMBDA &lambda) {                                                                                \
				Set(lambda);                                                                                                                    \
				return *this;                                                                                                                   \
			}                                                                                                                                   \
	                                                                                                                                            \
			template<unsigned MAX_SIZE_BYTES_2, LambdaRefStackCheck STACK_CHECK>                                                                \
			inline LambdaCopy &operator=(const LambdaRef<FUNCTION_SIGNATURE, MAX_SIZE_BYTES_2, STACK_CHECK> &lambdaRef) {                       \
				SetRef(lambdaRef);                                                                                                              \
				return *this;                                                                                                                   \
			}                                                                                                                                   \
	                                                                                                                                            \
			template<unsigned MAX_SIZE_BYTES_2>                                                                                                 \
			inline LambdaCopy &operator=(const LambdaRef_NoStackCheck<FUNCTION_SIGNATURE, MAX_SIZE_BYTES_2> &lambdaRef) {                       \
				return operator=(static_cast<const LambdaRef<FUNCTION_SIGNATURE, MAX_SIZE_BYTES_2, LAMBDA_REF_DISABLE_CHECK_STACK_OBJECT>&>(lambdaRef)); \
			}                                                                                                                                   \
	                                                                                                                                            \
			inline RETURN operator()(TYPE_ARGS_FUNC(, LAMBDA_TYPE_NAME)) const {                                                                \
				LAMBDA_CHECK_IS_VALID(m_caller);                                                                                                \
				return m_caller(CapturePtr() TYPE_ARGS_FUNC(LAMBDA_COMMA, LAMBDA_NAME));                                                        \
			}                                                                                                                                   \
																																				\
			inline operator bool() const {																										\
				return IsValid();																												\
			}																																	\
	                                                                                                                                            \
			inline bool IsValid() const {                                                                                                       \
	            return !!m_caller;                                                                                                              \
			}                                                                                                                                   \
																																				\
			inline void Invalidate() {																											\
	            m_caller = NULL;																												\
		    }                                                                                                                                   \
                                                                                                                                                \
			inline bool IsFunctionPointer() const {                                                                                             \
				return !m_destructor;                                                                                                           \
			}                                                                                                                                   \
                                                                                                                                                \
			inline bool IsLambda() const {                                                                                                      \
				return !IsFunctionPointer();                                                                                                    \
			}                                                                                                                                   \
		};                                                                                                                                      \
	                                                                                                                                            \
		template<unsigned MAX_SIZE_BYTES, LambdaRefStackCheck STACK_CHECK, class RETURN TYPE_ARGS_FUNC(LAMBDA_COMMA, LAMBDA_CLASS_TYPE)>        \
		template<unsigned MAX_SIZE_BYTES_2>                                                                                                     \
		inline void LambdaRef<RETURN (TYPE_ARGS_FUNC(, LAMBDA_TYPE)), MAX_SIZE_BYTES, STACK_CHECK>::Set(                                        \
			const LambdaCopy<RETURN (TYPE_ARGS_FUNC(, LAMBDA_TYPE)), MAX_SIZE_BYTES_2> &lambdaCopy) {                                           \
			(void)sizeof(LambdaPrivate::CheckSize<MAX_SIZE_BYTES_2, MAX_SIZE_BYTES>);                                                           \
			if (lambdaCopy.IsLambda()) {                                                                                                        \
				u.m_lambda.m_capture      = lambdaCopy.CapturePtr();                                                                            \
				u.m_lambda.m_caller       = lambdaCopy.m_caller;                                                                                \
				u.m_lambda.m_copier       = lambdaCopy.m_copier;                                                                                \
				u.m_lambda.m_destructor   = lambdaCopy.m_destructor;                                                                            \
			}                                                                                                                                   \
			else {                                                                                                                              \
				u.m_function.m_capture    = NULL;                                                                                               \
				u.m_function.m_caller     = *(RETURN(**)(TYPE_ARGS_FUNC(, LAMBDA_TYPE)))(lambdaCopy.CapturePtr());                              \
				u.m_function.m_copier     = NULL;                                                                                               \
				u.m_function.m_destructor = NULL;                                                                                               \
			}                                                                                                                                   \
		}


#	define LAMBDA_TYPE_ARGS_FUNC(COMMA, FUNC)
	LAMBDA_SPECIALIZATIONS(0, LAMBDA_TYPE_ARGS_FUNC)
#	undef  LAMBDA_TYPE_ARGS_FUNC
#	define LAMBDA_TYPE_ARGS_FUNC(COMMA, FUNC)  COMMA FUNC(T0, a0)
	LAMBDA_SPECIALIZATIONS(1, LAMBDA_TYPE_ARGS_FUNC)
#	undef  LAMBDA_TYPE_ARGS_FUNC
#	define LAMBDA_TYPE_ARGS_FUNC(COMMA, FUNC)  COMMA FUNC(T0, a0), FUNC(T1, a1)
	LAMBDA_SPECIALIZATIONS(2, LAMBDA_TYPE_ARGS_FUNC)
#	undef  LAMBDA_TYPE_ARGS_FUNC
#	define LAMBDA_TYPE_ARGS_FUNC(COMMA, FUNC)  COMMA FUNC(T0, a0), FUNC(T1, a1), FUNC(T2, a2)
	LAMBDA_SPECIALIZATIONS(3, LAMBDA_TYPE_ARGS_FUNC)
#	undef  LAMBDA_TYPE_ARGS_FUNC
#	define LAMBDA_TYPE_ARGS_FUNC(COMMA, FUNC)  COMMA FUNC(T0, a0), FUNC(T1, a1), FUNC(T2, a2), FUNC(T3, a3)
	LAMBDA_SPECIALIZATIONS(4, LAMBDA_TYPE_ARGS_FUNC)
#	undef  LAMBDA_TYPE_ARGS_FUNC
#	define LAMBDA_TYPE_ARGS_FUNC(COMMA, FUNC)  COMMA FUNC(T0, a0), FUNC(T1, a1), FUNC(T2, a2), FUNC(T3, a3), FUNC(T4, a4)
	LAMBDA_SPECIALIZATIONS(5, LAMBDA_TYPE_ARGS_FUNC)
#	undef  LAMBDA_TYPE_ARGS_FUNC
#	define LAMBDA_TYPE_ARGS_FUNC(COMMA, FUNC)  COMMA FUNC(T0, a0), FUNC(T1, a1), FUNC(T2, a2), FUNC(T3, a3), FUNC(T4, a4), FUNC(T5, a5)
	LAMBDA_SPECIALIZATIONS(6, LAMBDA_TYPE_ARGS_FUNC)
#	undef  LAMBDA_TYPE_ARGS_FUNC
#	define LAMBDA_TYPE_ARGS_FUNC(COMMA, FUNC)  COMMA FUNC(T0, a0), FUNC(T1, a1), FUNC(T2, a2), FUNC(T3, a3), FUNC(T4, a4), FUNC(T5, a5), FUNC(T6, a6)
	LAMBDA_SPECIALIZATIONS(7, LAMBDA_TYPE_ARGS_FUNC)
#	undef  LAMBDA_TYPE_ARGS_FUNC
#	define LAMBDA_TYPE_ARGS_FUNC(COMMA, FUNC)  COMMA FUNC(T0, a0), FUNC(T1, a1), FUNC(T2, a2), FUNC(T3, a3), FUNC(T4, a4), FUNC(T5, a5), FUNC(T6, a6), FUNC(T7, a7)
	LAMBDA_SPECIALIZATIONS(8, LAMBDA_TYPE_ARGS_FUNC)
#	undef  LAMBDA_TYPE_ARGS_FUNC

#	undef  LAMBDA_COMMA
#	undef  LAMBDA_CLASS_TYPE
#	undef  LAMBDA_TYPE_NAME
#	undef  LAMBDA_TYPE
#	undef  LAMBDA_NAME
#	undef  LAMBDA_SPECIALIZATION

	template<class FUNCTION_SIGNATURE_2, unsigned MAX_SIZE_BYTES>
	class LambdaRef_NoStackCheck : public LambdaRef<FUNCTION_SIGNATURE_2, MAX_SIZE_BYTES, LAMBDA_REF_DISABLE_CHECK_STACK_OBJECT> {
	private:
		typedef LambdaRef<FUNCTION_SIGNATURE_2, MAX_SIZE_BYTES, LAMBDA_REF_DISABLE_CHECK_STACK_OBJECT> Base;
	public:
		// Inheriting constructors with using keyword doesn't seem to work on old Microsoft compilers
		inline LambdaRef_NoStackCheck() {
		}
		template<class LAMBDA> /*implicit*/ inline LambdaRef_NoStackCheck(const LAMBDA &lambda) : Base(lambda) {
		}
	};

#endif

} // namespace rage

#endif // SYSTEM_LAMBDA_H
