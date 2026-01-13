/*
cl -I \xenon\include\win32 xenon_texture_sizes.cpp
	\xenon\lib\win32\xgraphics.lib \xenon\lib\win32\d3d9.lib
*/
#define STRICT
#include <windows.h>
#include <d3d9.h>
#include <xgraphics.h>

#include <stdio.h>

void main() {
	D3DTexture dummy;
	UINT uBaseSize, uMipSize;

	static D3DFORMAT formats[] = { D3DFMT_DXT1, D3DFMT_DXT5, D3DFMT_A8R8G8B8 };
	static const char *fnames[] = {"DXT1","DXT5","A8R8G8B8" };

	for (int f=0; f<3; f++) {
		for (int levels=4; levels<12; levels++) {
			int width = 1<<levels;
			int height = width;
			XGSetTextureHeader(width, height, levels, 0, formats[f], D3DPOOL_MANAGED, 
				0, XGHEADER_CONTIGUOUS_MIP_OFFSET, 0, &dummy, &uBaseSize, &uMipSize);
			printf("%s,%d,%d+%d=%d\n",
				fnames[f],width,uBaseSize,uMipSize,uBaseSize+uMipSize);
		}
	}
}
