@echo off
setlocal
cd %~dp0
p4 edit edge_vehicledamage.o
%SCE_PS3_ROOT%\host-win32\bin\spa -O3 --warningsaserrors --verboseswp --nopicwarn -o edge_vehicledamage.o -p edge_vehicledamage.asm edge_vehicledamage.spa
