call ..\..\setnmlibs.bat
set PH_LIBS=%RAGE_PH_LIBS%
set SAMPLE_LIBS=sample_physics sample_motiontree sample_cranimation sample_crfragment %RAGE_SAMPLE_LIBS%
set BASE_LIBS=curve spatialdata event
set LIBS=%RAGE_CORE_LIBS% %PH_LIBS% %SAMPLE_LIBS% %BASE_LIBS% pharticulated %RAGE_GFX_LIBS% %RAGE_NM_LIBS%
set LIBS=%LIBS%  fragment breakableglass phglass cloth grrope fragmentnm %RAGE_CR_LIBS%
set TESTERS=sample_motiontreenm
set XPROJ=%RAGE_DIR%\base\src %RAGE_DIR%\base\samples %RAGE_DIR%\naturalmotion\src 
set XPROJ=%XPROJ% %RAGE_DIR%\suite\src %RAGE_DIR%\suite\samples
set XINCLUDE=%RAGE_DIR%\naturalmotion\include
set XLIBS=..\..\lib
