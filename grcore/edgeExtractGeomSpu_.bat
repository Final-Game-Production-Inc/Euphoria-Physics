
@rem spu-lv2-gcc -c -xassembler-with-cpp %RAGE_DIR%/base/src/data/datdecompressinplace.spuasm -o %OUTDIR%\datdecompressinplace.spuobj
@rem set OBJ_FILES=%OBJ_FILES% %OUTDIR%\datdecompressinplace.spuobj

@rem spu-lv2-gcc -c warptransformloop.s -o %OUTDIR%\warptransformloop.spuobj
@rem set OBJ_FILES=%OBJ_FILES% %OUTDIR%\warptransformloop.spuobj

@rem spu-lv2-gcc -c warppointtransformloop.s -o %OUTDIR%\warppointtransformloop.spuobj
@rem set OBJ_FILES=%OBJ_FILES% %OUTDIR%\warppointtransformloop.spuobj

@rem pushd %RAGE_DIR%
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
