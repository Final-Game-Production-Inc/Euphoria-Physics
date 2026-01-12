p4 edit ..\..\..\lib\datdecompressinplace_spu.obj
del ..\..\..\lib\datdecompressinplace_spu.obj
spu-lv2-gcc -c -fpic -xassembler-with-cpp datdecompressinplace.spuasm -o ..\..\..\lib\datdecompressinplace_spu.obj
