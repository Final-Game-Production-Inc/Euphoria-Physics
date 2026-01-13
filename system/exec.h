// 
// system/exec.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 
#ifndef SYSTEM_EXEC_H
#define SYSTEM_EXEC_H

namespace rage {

//PURPOSE:  Handy hardcoded Windows command that will allow users
// to execute an asynchronous command with various parameters like
// max/min windows, working directories, etc.
#define START_COMMAND "START"

// PURPOSE:	Execute a program on the Host PC
// PARAMS:	commandLine - command line string to pass to command processor
// RETURNS:	Exit code from system call
// NOTES:	If you want to launch a process without waiting for it to complete,
//			remember that the "start" command will invoke a detached process.
int sysExec(const char *commandLine);

// PURPOSE:	Retrieves an environment variable from the Host PC
// PARAMS:	env - Env var to retrieve
//			buf - Destination buffer to fill
//			bufSize - sizeof(buf)
// RETURNS:	false if env var wasn't found, else true and buf is valid.
//			If false is returned, buf is unchanged, so you can initialize it
//			with a default value if you want.
bool sysGetEnv(const char *env,char *buf,int bufSiz);

struct sysLibrary;
sysLibrary* sysLoadLibrary(const char *path);
void sysFreeLibrary(sysLibrary*);
void* sysGetProcAddress(sysLibrary*, const char *name);

}

#endif	// SYSTEM_EXEC_H
