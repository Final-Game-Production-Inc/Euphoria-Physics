//
// system/lambda.cpp
//
// Copyright (C) 2017-2017 Rockstar Games.  All Rights Reserved.
//

#include "system/lambda.h"

namespace rage {

#if LAMBDA_USE_VARIADIC_TEMPLATES
	/*static*/ void LambdaPrivate::FunctionCopier(void *dstFuncPtrPtr, const void *srcFuncPtrPtr LAMBDA_ASSERT_OR_TRAP_ONLY(, size_t maxSizeBytes)) {
		LAMBDA_CHECK_COPY_CAPTURE_SIZE(sizeof(void(*)()), maxSizeBytes);
		*((void(**)())dstFuncPtrPtr) = *((void(**)())srcFuncPtrPtr);
	}
#else
	namespace LambdaPrivate {
		void FunctionCopier(void *dstFuncPtrPtr, const void *srcFuncPtrPtr LAMBDA_ASSERT_OR_TRAP_ONLY(, size_t maxSizeBytes)) {
			LAMBDA_CHECK_COPY_CAPTURE_SIZE(sizeof(void(*)()), maxSizeBytes);
			*((void(**)())dstFuncPtrPtr) = *((void(**)())srcFuncPtrPtr);
		}
	}
#endif

} // namespace rage
