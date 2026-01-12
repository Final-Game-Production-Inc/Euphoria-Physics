@echo off
del *.frag
for /F "tokens=1,2*" %%i in (funcs.txt) do @echo //  > %%i.frag
for /F "tokens=1,2*" %%i in (funcs.txt) do @echo // phsolver/%%i.frag  >> %%i.frag
for /F "tokens=1,2*" %%i in (funcs.txt) do @echo //  >> %%i.frag
for /F "tokens=1,2*" %%i in (funcs.txt) do @echo // Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  >> %%i.frag
for /F "tokens=1,2*" %%i in (funcs.txt) do @echo //  >> %%i.frag
for /F "tokens=1,2*" %%i in (funcs.txt) do @echo. >> %%i.frag
for /F "tokens=1,2*" %%i in (funcs.txt) do @echo #include "%%j.cpp">> %%i.frag
for /F "tokens=1,2*" %%i in (funcs.txt) do @echo. >> %%i.frag
for /F "tokens=1,2*" %%i in (funcs.txt) do @echo using namespace rage;>> %%i.frag
for /F "tokens=1,2*" %%i in (funcs.txt) do @echo. >> %%i.frag
for /F "tokens=1,2*" %%i in (funcs.txt) do @echo SPUFRAG_DECL(void, %%j, phManifold^&, const phForceSolverGlobals^&);>> %%i.frag
for /F "tokens=1,2*" %%i in (funcs.txt) do @echo SPUFRAG_IMPL(void, %%j, phManifold^& manifold, const phForceSolverGlobals^& globals)>> %%i.frag
for /F "tokens=1,2*" %%i in (funcs.txt) do @echo {>> %%i.frag
for /F "tokens=1,2*" %%i in (funcs.txt) do @echo 	%%i(manifold, globals);>> %%i.frag
for /F "tokens=1,2*" %%i in (funcs.txt) do @echo }>> %%i.frag
for /F "tokens=1,2*" %%i in (funcs.txt) do @echo. >> %%i.frag
for /F "tokens=1,2*" %%i in (funcs.txt) do @echo #define DEBUG_SPU_JOB_NAME "%%i" > %%i_frag.cpp
for /F "tokens=1,2*" %%i in (funcs.txt) do @echo #include "%%i.frag" >> %%i_frag.cpp
