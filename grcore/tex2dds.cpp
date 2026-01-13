// 
// grcore/tex2dds.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 
// THIS CODE WILL NOT BUILD.  ROLL BACK TO A VERSION OF RAGE WR142 OR EARLIER.

#error "THIS CODE WILL NOT BUILD.  ROLL BACK TO A VERSION OF RAGE WR142 OR EARLIER."

#define TEX2DDS
#include "image.h"
#include "file/stream.h"
#include "file/asset.h"

#if __WIN32PC

#pragma comment(lib,"xgraphics.lib")

#pragma warning(disable: 4668)
#include <windows.h>
//#include <d3d8-xbox.h>
//#include <xgraphics.h>
#pragma warning(error: 4668)

using namespace rage;

namespace rage {
	grcImage* grcImage::LoadTEX(const char *filename) {
		fiSafeStream S(ASSET.Open(filename,"tex",true,true));
		if (!S)
			return NULL;

		enum {
			TEX_FMT_NULL, TEX_FMT_P_8, TEX_FMT_AP_88, TEX_FMT_RGB_332
			, TEX_FMT_ARGB_8332, TEX_FMT_RGB_565, TEX_FMT_ARGB_1555, TEX_FMT_ARGB_4444
			, TEX_FMT_I_8, TEX_FMT_AI_44, TEX_FMT_AI_88, TEX_FMT_A_8
			, TEX_FMT_YIQ_422, TEX_FMT_AYIQ_8422, TEX_FMT_PA_8, TEX_FMT_P_4
			, TEX_FMT_PA_4, TEX_FMT_RGB_888, TEX_FMT_RGBA_8888, TEX_FMT_Z16
			, TEX_FMT_Z24, TEX_FMT_Z32, TEX_FMT_DXT1, TEX_FMT_DXT2
			, TEX_FMT_DXT3, TEX_FMT_DXT4, TEX_FMT_DXT5, TEX_FMT_SCEE_16BIT_4BIT
			, TEX_FMT_SCEE_16BIT_2BIT, TEX_FMT_SCEE_24BIT_4BIT, TEX_FMT_SCEE_24BIT_2BIT, TEX_FMT_SCEE_32BIT_4BIT
			, TEX_FMT_SCEE_32BIT_2BIT, TEX_FMT_COUNT
		};

		struct TexHeader {
			unsigned short Width;
			unsigned short Height;
			unsigned short Type;
			unsigned short NumMips;
			unsigned short NPalette;
			unsigned short RepS;
			unsigned short RepT;
		};

		TexHeader hdr;
		S->ReadShort(&hdr.Width,7);
		if (hdr.Type != TEX_FMT_PA_8 && hdr.Type != TEX_FMT_PA_4 && hdr.Type != TEX_FMT_P_4 && hdr.Type != TEX_FMT_P_8 && hdr.Type != TEX_FMT_A_8)
			Quitf("Unsupported tex format %d",hdr.Type);
		// Read the color lookup table
		int npal = ((hdr.Type == TEX_FMT_PA_8 || hdr.Type == TEX_FMT_P_8)? 256 : (hdr.Type == TEX_FMT_PA_4 || hdr.Type == TEX_FMT_P_4)? 16 : 0);
		Color32 lut[256];
		for (int i=0; i<npal; i++) {
			u8 b = (u8)S->GetCh();
			u8 g = (u8)S->GetCh();
			u8 r = (u8)S->GetCh();
			u8 a = (u8)S->GetCh();
			lut[i].Set(r,g,b,a);
		}
		// Just convert to 32bpp for now.
		// .tex format is upside-down so fix it!
		grcImage *result = NULL;
		if (npal) {
			result = grcImage::Create(hdr.Width,hdr.Height,1,grcImage::A8R8G8B8,STANDARD,hdr.NumMips-1,0);
			if (npal == 256) {
				grcImage *mip = result;
				do {
					Color32 *cp = (mip->GetWidth() * (mip->GetHeight()-1)) + mip->GetColor32();
					for (int row=0; row<mip->GetHeight(); row++) {
						for (int col=0; col<mip->GetWidth(); col++) {
							u8 texel = (u8)S->FastGetCh();
							cp[col] = lut[texel];
						}
						cp -= mip->GetWidth();
					}
					mip = mip->GetNext();
				} while (mip);
			}
			else /*if (npal == 16)*/ {
				grcImage *mip = result;
				do {
					Color32 *cp = (mip->GetWidth() * (mip->GetHeight()-1)) + mip->GetColor32();
					for (int row=0; row<mip->GetHeight(); row++) {
						for (int col=0; col<mip->GetWidth(); col+=2) {
							u8 texel = (u8)S->FastGetCh();
							cp[col] = lut[texel&15];
							cp[col+1] = lut[texel>>4];
						}
						cp -= mip->GetWidth();
					}
					mip = mip->GetNext();
				} while (mip);
			}
		}
		else if (hdr.Type == TEX_FMT_A_8) {
			result = grcImage::Create(hdr.Width,hdr.Height,1,grcImage::A8,STANDARD,hdr.NumMips-1,0);
			grcImage *mip = result;
			do {
				u8 *a = (mip->GetWidth() * (mip->GetHeight()-1)) + mip->GetBits();
				for (int row=0; row<mip->GetHeight(); row++) {
					for (int col=0; col<mip->GetWidth(); col++) {
						u8 texel = (u8) S->FastGetCh();
						a[col] = texel;
					}
					a -= mip->GetWidth();
				}
				mip = mip->GetNext();
			} while (mip);
		}
		return result;
	}
}

int main(int argc,char **argv) {
	bool flip = false;
	for (int i=1; i<argc; i++) {
		if (!strcmp(argv[i],"-flip")) {
			flip = true;
			continue;
		}
		else if (!strcmp(argv[i],"-noflip")) {
			flip = false;
			continue;
		}
		grcImage *src = grcImage::LoadTEX(argv[i]);
		if (!src)
			continue;
		if (flip)
			src->FlipVertical();
		int area = src->GetArea();
		Color32 *texel = src->GetColor32();
		bool hasAlpha = false;
		do {
			if (texel->GetAlpha() != 255) {
				hasAlpha = true;
				break;
			}
			++texel;
		} while (--area);
		grcImage *dest = grcImage::Create(src->GetWidth(), src->GetHeight(), src->GetDepth(), hasAlpha? grcImage::DXT5 : grcImage::DXT1,grcImage::STANDARD,src->GetExtraMipCount(),0);
		grcImage *srcMip = src;
		grcImage *destMip = dest;
		do {
			/* XGCompressRect(destMip->GetBits(),hasAlpha? D3DFMT_DXT5 : D3DFMT_DXT1,destMip->GetWidth() << (1 + hasAlpha),destMip->GetWidth(),destMip->GetHeight(),srcMip->GetBits(),
				D3DFMT_LIN_A8R8G8B8,srcMip->GetStride(),0.0f,0); */
			srcMip = srcMip->GetNext();
			destMip = destMip->GetNext();
		} while (srcMip && destMip);
		src->Release();
		char ddsname[256];
		strcpy(ddsname,argv[i]);
		if (strchr(ddsname,'.'))
			strcpy(strchr(ddsname,'.'),".dds");
		grcDisplayf("Saving %s as %s",argv[i],ddsname);
		dest->SaveDDS(ddsname);
		dest->Release();
	}
	return 0;
}

#endif
