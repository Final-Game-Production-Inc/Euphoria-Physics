// 
// system/exec.cpp 
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
// 
#include "exec.h"
#include "file/remote.h"

#include <stdlib.h>

#if __WIN32
#include "system/xtl.h"
#endif

namespace rage {

#if __WIN32PC
int sysExec(const char *commandLine) {
	return system(commandLine);
}
#elif (__XENON || __PPU || __PSP2 || RSG_DURANGO || RSG_ORBIS) && !__FINAL
int sysExec(const char *commandLine) {
	return fiRemoteExec(commandLine);
}
#else
int sysExec(const char * /*commandLine*/) {
	AssertMsg(false,"sysExec not supported on this platform.");
	return -1;
}
#endif

bool sysGetEnv(const char *NOTFINAL_ONLY(env),char *NOTFINAL_ONLY(dest),int NOTFINAL_ONLY(destSize)) {
#if __WIN32PC && !__FINAL
	env = getenv(env);
	if (env) {
		safecpy(dest,env,destSize);
		return true;
	}
	else
		return false;
#elif !__FINAL
	return fiRemoteGetEnv(env,dest,destSize) == 0;
#else
	return false;
#endif
}

#if __WIN32PC || __XENON
sysLibrary* sysLoadLibrary(const char *path) {
	return (sysLibrary*) ::LoadLibrary(path);
}

void sysFreeLibrary(sysLibrary* lib) {
	if (lib)
		::FreeLibrary((HMODULE)lib);
}

void* sysGetProcAddress(sysLibrary* lib,const char *name) {
	return ::GetProcAddress((HMODULE)lib,name);
}
#endif

}
