REM we need to add a few bits and bobs when compiling NM CBU jobs
ECHO Overriding compile flags for NM CBU job - %0
set CPPFLAGS=%CPPFLAGS% -I %RAGE_DIR:\=/%/naturalmotion/include -DART_MONOLITHIC -DART_MONOLITHIC_STATIC -DNMUTILS_NODLL -DROCKSTAR_GAME_EMBEDDED

