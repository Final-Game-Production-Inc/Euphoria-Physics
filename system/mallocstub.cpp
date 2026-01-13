// 
// system/mallocstub.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

// Note that STLport has some lingering calls to malloc, as does CyaSSL, neither of which appear to be actually
// used in practice.  Replacing them isn't safe because realloc is used by 360 startup code to resize the atexit
// table used by globally constructed/destructed objects.

// So, just at least try to catch strdup calls.

// extern "C" void malloc_should_not_be_used();
// extern "C" void free_should_not_be_used();
extern "C" void use_StringDuplicate_instead_of_strdup();

/* extern "C" __declspec(noalias) __declspec(restrict) void* malloc(size_t) {
	malloc_should_not_be_used();
	return 0;
} */

/* extern "C" __declspec(noalias) void free(void*) {
	free_should_not_be_used();
} */

extern "C" char *strdup(const char*) {
	use_StringDuplicate_instead_of_strdup();
	return 0;
}
