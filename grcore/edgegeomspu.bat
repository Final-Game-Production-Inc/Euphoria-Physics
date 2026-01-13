
REM 	ABS_RAG_DIR is already created from a previously executed batch file.
REM	If it does not exist, then we have to abort.
IF "%ABS_RAGE_DIR%"=="" (
	ECHO ABS_RAGE_DIR is not specified!  Aborting...
	EXIT \B
)

spu-lv2-gcc -c -xassembler-with-cpp %ABS_RAGE_DIR%/base/src/data/datdecompressinplace.spuasm -o %OUTDIR%\datdecompressinplace.spuobj
set OBJ_FILES=%OBJ_FILES% %OUTDIR%\datdecompressinplace.spuobj

spu-lv2-gcc -c warptransformloop.s -o %OUTDIR%\warptransformloop.spuobj
set OBJ_FILES=%OBJ_FILES% %OUTDIR%\warptransformloop.spuobj

spu-lv2-gcc -c warppointtransformloop.s -o %OUTDIR%\warppointtransformloop.spuobj
set OBJ_FILES=%OBJ_FILES% %OUTDIR%\warppointtransformloop.spuobj

set OBJ_FILES=%OBJ_FILES% edge_vehicledamage.o

@rem pushd %ABS_RAGE_DIR%
@rem cd tools\base\exes
@rem set MAKE=%CD%\make.exe
@rem popd

@rem pushd ..\edge\geom
@rem %MAKE%
@rem popd

@rem if %DIR%==psn_debug set LIBS=%LIBS% -L %RAGE_DIR%/base/src/vcproj/RageGraphics/psn_edge -ledgegeom_dbg
@rem if %DIR%==psn_prerelease set LIBS=%LIBS% -L %RAGE_DIR%/base/src/vcproj/RageGraphics/psn_edge -ledgegeom_dbg
@rem if %DIR%==psn_beta set LIBS=%LIBS% -L %RAGE_DIR%/base/src/vcproj/RageGraphics/psn_edge -ledgegeom_dbg
@rem if %DIR%==psn_release set LIBS=%LIBS% -L %RAGE_DIR%/base/src/vcproj/RageGraphics/psn_edge -ledgegeom
@rem if %DIR%==psn_bankrelease set LIBS=%LIBS% -L %RAGE_DIR%/base/src/vcproj/RageGraphics/psn_edge -ledgegeom
@rem if %DIR%==psn_final set LIBS=%LIBS% -L %RAGE_DIR%/base/src/vcproj/RageGraphics/psn_edge -ledgegeom
@rem if %DIR%==psn_profile set LIBS=%LIBS% -L %RAGE_DIR%/base/src/vcproj/RageGraphics/psn_edge -ledgegeom

@set COMPILE_SET_0_OPT_LVL=-Os
