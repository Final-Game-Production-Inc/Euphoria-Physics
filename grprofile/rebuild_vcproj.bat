@echo off
if "%1"=="VS2010" goto VS2010
call %RS_TOOLSROOT%\script\coding\projbuild\rebuild_library.bat grprofile
if "%1"=="VS2008" goto END
:VS2010
call %RS_TOOLSROOT%\script\util\projgen\rebuildGameLibrary.bat makefile.txt %2 %3
:END
