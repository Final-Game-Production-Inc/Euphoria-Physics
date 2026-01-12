@echo off
for /F "tokens=1,2*" %%i in (funcs.txt) do %SCE_PS3_ROOT%\host-win32\spu\bin\spu-lv2-size c:\spu_debug\%%i_psn_beta.frag.elf