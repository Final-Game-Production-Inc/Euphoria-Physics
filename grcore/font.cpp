//
// grcore/font.cpp
//
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved.
//

#include "font.h"

#include "device.h"
#include "image.h"
#include "im.h"
#include "texture.h"
#include "quads.h"

#if RSG_DURANGO
#include "grcore/texture_durango.h"
#endif

#include "file/asset.h"
#include "file/device.h"
#include "string/string.h"
#include "string/unicode.h"
#include "system/alloca.h"

#include <stdarg.h>
#include <stdio.h>

using namespace rage;

grcFont* grcFont::sm_Current[NUMBER_OF_RENDER_THREADS];


grcFont::grcFont() 
	: m_Texture(NULL)
	, m_CharWidth(0)
	, m_CharHeight(0)
	, m_CharCount(0)
	, m_Glyphs(0) 
	, m_GlyphLut(0)
	, m_Kerning(0)
	, m_InvWidth(0)
	, m_InvHeight(0)
	, m_HasOutline(false)
	, m_StaticMetrics(false)
{
	for (unsigned i=0; i<NUMBER_OF_RENDER_THREADS; ++i) {
		m_InternalScale[i] = 1.0f;
	    m_InternalColor[i] = Color32(255,255,255);
		m_InBegin[i] = 0;
	}
}


grcFont::~grcFont() {
	if (m_Texture)
		m_Texture->Release();
	if (!m_StaticMetrics) {
		delete [] m_GlyphLut;
		delete [] m_Glyphs;
		delete [] m_Kerning;
	}
}

void grcFont::DrawCharBegin() const {
	if (++m_InBegin[g_RenderThreadIndex] == 1)
		grcBindTexture(m_Texture);
}


void grcFont::DrawCharEnd() const {
	if (--m_InBegin[g_RenderThreadIndex] == 0)
		grcBindTexture(NULL);
}


inline float Shim(float v)
{
	const float fudge = 1<<23;
#if __XENON
	return ((v + fudge) - fudge) - 0.5f;
#else
	return (v + fudge) - fudge;
#endif
}

void grcFont::DrawChar(float x,float y,float z,Color32 color,float scaleX,float scaleY,u16 ch) const {
	if (GetGlyphIndex(ch) == -1) {
		if (grcInQuads())	// Make sure we draw something if we're doing a nested quad
			grcDrawQuadf(0,0,0,0, 0, 0,0,0,0,color);
		return;
	}

	Glyph &g = m_Glyphs[m_GlyphLut[ch]];
	float fu = float(g.x);
	float fv = float(g.y);
	float fw = float(g.width);
	float fh = float(g.height);
	float fxo = float(g.xoffset);
	float fyo = float(g.yoffset);

	float u = fu * m_InvWidth;
	float v = fv * m_InvHeight;
	float du = fw * m_InvWidth;
	float dv = fh * m_InvHeight;

	grcBeginQuads(1);		// Note that nested calls are intentionally ignored.
	grcDrawQuadf(
		Shim(x + fxo * scaleX),
		Shim(y + fyo * scaleY),
		Shim(x + (fxo + fw) * scaleX), 
		Shim(y + (fyo + fh) * scaleY),z,u,v,u+du,v+dv,color);
	grcEndQuads();
}


void grcFont::DrawScaled(float x,float y,float z,Color32 color,float scaleX,float scaleY,const char *string) const {
	if (!string)
		string = "(null)";

	int length = CountUtf8Characters(string);
	if (!length)
		return;

	// Pre-scan the string looking for spaces to pull out
	const char *prescan = string;
	int actualLength = 0;
	while (*prescan) {
		u16 ch;
		prescan += Utf8ToWideChar(prescan,ch);
		if (ch == '\b') {
			while (*prescan >= '0' && *prescan <= '9')
				++prescan;
			continue;
		}
		else if (ch != '\n' && ch != 32 && GetGlyphIndex(ch) != -1)
			++actualLength;
	}
	Assert(actualLength <= length);

	float start = x;
	DrawCharBegin();
	float internalScale = m_InternalScale[g_RenderThreadIndex];
	scaleX *= internalScale;
	scaleY *= internalScale;

	if (actualLength <= grcBeginQuadsMax)
		grcBeginQuads(actualLength);
	int prevKerningSlot = 0, prevKerningCount = 0;
	while (*string) {
		u16 ch;
		string += Utf8ToWideChar(string,ch);
		if (ch == '\n') {
			x = start;
			y += scaleY * float(m_CharHeight);
		}
		else if (ch == '\b') {
			int tab = 0;
			while (*string >= '0' && *string <= '9') {
				tab = (tab * 10) + (*string++ - '0');
			}
			x = start + scaleX * tab * m_CharWidth;
		}
		else {
			if (GetGlyphIndex(ch) != -1) {
				// Don't waste time rendering a space but make sure we use an appropriate step for it.
				if (ch != 32)
					DrawChar(x,y,z,color,scaleX,scaleY,ch);
				Glyph &g = m_Glyphs[m_GlyphLut[ch]];
				int advance = g.xadvance;
				while (prevKerningCount--) {
					if (m_Kerning[prevKerningSlot].second == ch) {
						advance += m_Kerning[prevKerningSlot].xadvance;
						break;
					}
					else
						++prevKerningSlot;
				}
				x += scaleX * advance;
				prevKerningSlot = g.kerningSlot;
				prevKerningCount = g.kerningCount;
			}
			else {
				x += scaleX * float(m_CharWidth);
				prevKerningCount = 0;
			}
		}
	}
	if (actualLength <= grcBeginQuadsMax)
		grcEndQuads();

	DrawCharEnd();
}


int grcFont::GetStringWidthInternal(const char *s,int sl) const {
	int result = 0;
	int prevKerningSlot = 0, prevKerningCount = 0;
	while (sl) {
		char16 ch;
		int consumed = Utf8ToWideChar(s,ch);
		if (!consumed) {
			Errorf("Couldn't convert string '%s' UTF8->WideChar", s);
			return 0;
		}
		s += consumed;
		sl -= consumed;
		if (GetGlyphIndex(ch) != -1) {
			Glyph &g = m_Glyphs[GetGlyphIndex(ch)];
			result += g.xadvance;
			while (prevKerningCount--) {
				if (m_Kerning[prevKerningSlot].second == ch) {
					result += m_Kerning[prevKerningSlot].xadvance;
					break;
				}
				else
					prevKerningSlot++;
			}
			prevKerningSlot = g.kerningSlot;
			prevKerningCount = g.kerningCount;
		}
		else {
			result += m_CharWidth;
			prevKerningCount = 0;
		}
	}
	return int(result * m_InternalScale[g_RenderThreadIndex]);
}


void grcFont::Draw(float x,float y,const char *string) const {
	DrawScaled(x,y,0,m_InternalColor[g_RenderThreadIndex],1.0f,1.0f,string);
}

void grcFont::Draw(float x,float y,Color32 color, const char *string) const {
	DrawScaled(x,y,0,color,1.0f,1.0f,string);
}


void grcFont::Drawf(float x,float y,const char *fmt,...) const {
	char buffer[256];
	va_list args;
	va_start(args,fmt);
	vformatf(buffer,sizeof(buffer),fmt,args);
	Draw(x,y,m_InternalColor[g_RenderThreadIndex],buffer);
	va_end(args);
}

void grcFont::Drawf(float x,float y, Color32 color, const char *fmt,...) const {
	char buffer[256];
	va_list args;
	va_start(args,fmt);
	vformatf(buffer,sizeof(buffer),fmt,args);
	Draw(x,y,color,buffer);
	va_end(args);
}


grcFont* grcFont::CreateFontFromBitmap(const u8 * bits,int charWidth,int charHeight,int firstChar,int charCount) {
	const int texWidth = 128, texHeight = 64;	// TODO: Needs work
	
	grcImage *image = grcImage::Create(texWidth,texHeight,1,grcImage::DXT1,grcImage::STANDARD,0,0);
	int charsPerRow = texWidth / charWidth;

	for (int c=0; c<charCount; c++) {
		int x = (c % charsPerRow) * charWidth;
		int y = (c / charsPerRow) * charHeight;
		const u32 white = ~0U, black = 0;
		for (int row=0; row<charHeight; row++) {
			for (int col=0; col<charWidth; col++) {
				image->SetPixel(x+col,y+row,bits[col>>3] & (128 >> (col&7))? white : black);
			}
			bits += Max<int>(1, charWidth>>3);
		}
	}

	grcFont *font = CreateFontFromImage(image,charWidth,charHeight,firstChar,charCount);
	image->Release();
	return font;
}


grcFont* grcFont::CreateFontFromImage(grcImage *image,int charWidth,int charHeight,int firstChar,int charCount) {
	// TODO - make image const, requires fixing up grcTexture API
	grcFont *f = rage_new grcFont;
	BANK_ONLY(grcTexture::SetCustomLoadName("grcFont");)
	f->m_Texture = grcTextureFactory::GetInstance().Create(image);
	DURANGO_ONLY( ((grcDurangoPlacementTexture*)f->m_Texture)->UpdateGPUCopy());
	BANK_ONLY(grcTexture::SetCustomLoadName(NULL);)
	f->m_CharWidth = charWidth;
	f->m_CharHeight = charHeight;
	f->m_CharCount = charCount;
	int charsPerRow = image->GetWidth() / charWidth;
	f->m_InvWidth = 1.0f / image->GetWidth();
	f->m_InvHeight = 1.0f / image->GetHeight();

	f->m_Glyphs = rage_new Glyph[charCount];
	f->m_GlyphLut = rage_new s16[firstChar + charCount];
	for (int i=0; i<firstChar; i++)
		f->m_GlyphLut[i] = -1;
	for (int i=0; i<charCount; i++) {
		f->m_GlyphLut[i + firstChar] = s16(i);
		Glyph &g = f->m_Glyphs[i];
		g.x = u16((i % charsPerRow) * charWidth);
		g.y = u16((i / charsPerRow) * charHeight);
		g.width = u8(charWidth);
		g.height = u8(charHeight);
		g.kerningSlot = 0;
		g.kerningCount = 0;
		g.xoffset = g.yoffset = 0;
		g.xadvance = s8(charWidth);
	}

	return f;
}


grcFont* grcFont::CreateFontFromFile(const char *filename) {
	grcImage *image = grcImage::Load(filename);
	if (image) {
		const int perRow = 16, rowCount = 8;
		grcFont *font = CreateFontFromImage(image,image->GetWidth()/perRow,image->GetHeight()/rowCount,32,perRow*rowCount);
		// if (font)
		//	font->m_InternalScale = 0.75f;
		image->Release();
		return font;
	}
	else
		return NULL;
}

grcFont* grcFont::CreateFontFromMetrics(FontMetrics &fontMetrics) {
	/* How to create a new embedded font:
		1. Make sure the font works correctly via CreateFontFromFiles
		2. Enable the #if 1 block near the end of that function, and run again.
		3. Move that header file to the correct place (default is here in grcore) and use it.
		4. Disable the #if 1 block again!
	*/
	grcFont *f = rage_new grcFont;
	char buf[64];
	fiDevice::MakeMemoryFileName(buf,sizeof(buf),fontMetrics.m_TextureData,fontMetrics.m_TextureDataSize,false,"font .dds");
	grcImage *image = grcImage::LoadDDS(buf);
	BANK_ONLY(grcTexture::SetCustomLoadName("grcFont");)
	f->m_Texture = grcTextureFactory::GetInstance().Create(image);
	BANK_ONLY(grcTexture::SetCustomLoadName(NULL);)
	f->m_InvWidth = 1.0f / image->GetWidth();
	f->m_InvHeight = 1.0f / image->GetHeight();
	image->Release();
	f->m_CharWidth = fontMetrics.m_CharWidth;
	f->m_CharHeight = fontMetrics.m_CharHeight;
	f->m_CharCount = fontMetrics.m_CharCount;
	f->m_Glyphs = fontMetrics.m_Glyphs;
	f->m_GlyphLut = fontMetrics.m_GlyphLut;
	f->m_Kerning = fontMetrics.m_Kerning;
	f->m_HasOutline = fontMetrics.m_HasOutline;
	f->m_StaticMetrics = true;
	return f;
}

struct tempKern { 
	u16 first, second; 
	s16 xadvance; 
};

struct compare {
	bool operator()( const tempKern& a, const tempKern& b ) const {
		return a.first < b.first || (a.first == b.first && a.second < b.second);
	}
};

grcFont* grcFont::CreateFontFromFiles(const char *folder,const char *filename) {
	ASSET.PushFolder(folder);
	fiStream *txt = ASSET.Open(filename,"fnt");
	if (!txt) {
		ASSET.PopFolder();
		return NULL;
	}

	// http://www.angelcode.com/products/bmfont/
	// info face="Verdana" size=16 bold=0 italic=0 charset="" unicode=0 stretchH=100 smooth=1 aa=1 padding=0,0,0,0 spacing=1,1
	// common lineHeight=20 base=16 scaleW=128 scaleH=128 pages=1 packed=0
	// page id=0 file="Verdana16.png"
	// chars count=95
	int size, lineHeight, base, charCount,scaleW,scaleH,pages;
	char buf[256], face[64];
	fgets(buf,sizeof(buf),txt);
	bool outline = strstr(buf,"outline=1") != 0;
	sscanf(buf,"info face=%s size=%d",face,&size);
	fgets(buf,sizeof(buf),txt);
	sscanf(buf,"common lineHeight=%d base=%d scaleW=%d scaleH=%d pages=%d",&lineHeight,&base,&scaleW,&scaleH,&pages);
	AssertMsg(pages==1,"Font loader requires font to fit all on one texture page");
	fgets(buf,sizeof(buf),txt);
	strtok(buf,"\""); 
	char *texName = strtok(NULL,"\"");
	char texBuf[64];
	strcpy(texBuf,texName);
	grcImage *image = grcImage::Load(texBuf);
	ASSET.PopFolder();
	if (!image) {
		txt->Close();
		return NULL;
	}

	grcFont *f = rage_new grcFont;
	f->m_Texture = grcTextureFactory::GetInstance().Create(image);
	f->m_InvWidth = 1.0f / scaleW;
	f->m_InvHeight = 1.0f / scaleH;
	image->Release();

	fgets(buf,sizeof(buf),txt);
	sscanf(buf,"chars count=%d\n",&charCount);

	f->m_CharWidth = size;
	f->m_CharHeight = lineHeight;
	f->m_Glyphs = rage_new Glyph[charCount];
	f->m_HasOutline = outline;

	// Reserve a temporary structure for building the final lut
	s16 *tempLut = Alloca(s16, charCount);
	s16 highest = -1;

	// Read character cell info
	for (int i=0; i<charCount; i++) {
		// char id=125 x=1 y=1 width=8 height=17 xoffset=1 yoffset=3 xadvance=10 page=0 chnl=0 letter="}"
		// DANGER: rage fscanf only supports up to eight stored parameters, we're right at that limit!
		int id, x, y, width, height, xoffset, yoffset, xadvance;
		fgets(buf,sizeof(buf),txt);
		sscanf(buf,"char id=%d x=%d y=%d width=%d height=%d xoffset=%d yoffset=%d xadvance=%d",
			&id,&x,&y,&width,&height,&xoffset,&yoffset,&xadvance);
		Glyph &g = f->m_Glyphs[i];
		tempLut[i] = s16(id);
		if (tempLut[i] > highest)
			highest = tempLut[i];
		g.x = u16(x);
		g.y = u16(y);
		g.width = u8(width);
		g.height = u8(height);
		g.kerningSlot = 0;
		g.kerningCount = 0;
		g.xoffset = s8(xoffset); 
		g.yoffset = s8(yoffset);
		g.xadvance = s8(xadvance);
	}

	// Initialize the LUT
	f->m_CharCount = highest + 1;
	f->m_GlyphLut = rage_new s16[highest + 1];
	for (int i=0; i<=highest; i++)
		f->m_GlyphLut[i] = -1;
	for (int i=0; i<charCount; i++)
		f->m_GlyphLut[tempLut[i]] = s16(i);

	// Read kerning information
	// kernings count=106
	// kerning first=107 second=45 amount=-1
	int kerningsCount = 0;
	fgets(buf,sizeof(buf),txt);
	tempKern *tk = 0;
	if (sscanf(buf,"kernings count=%d\n",&kerningsCount) && kerningsCount) {
		f->m_Kerning = rage_new Kerning[kerningsCount];
		// kerning first=107 second=45 amount=-1
		// Inputs aren't necessarily sorted!  Do two passes.
		tk = Alloca(tempKern,kerningsCount);
		for (int i=0; i<kerningsCount; i++) {
			int first, second, amount;
			fgets(buf,sizeof(buf),txt);
			sscanf(buf,"kerning first=%d second=%d amount=%d\n",&first,&second,&amount);
			tk[i].first = u16(first);
			tk[i].second = u16(second);
			tk[i].xadvance = s16(amount);
		}
		std::sort(tk,tk+kerningsCount,compare());
		for (int i=0; i<kerningsCount; i++) {
			if (!f->m_Glyphs[f->m_GlyphLut[tk[i].first]].kerningCount)
				f->m_Glyphs[f->m_GlyphLut[tk[i].first]].kerningSlot = u16(i);
			f->m_Glyphs[f->m_GlyphLut[tk[i].first]].kerningCount++;
			f->m_Kerning[i].second = tk[i].second;
			f->m_Kerning[i].xadvance = tk[i].xadvance;
		}
	}
	else
		f->m_Kerning = NULL;
	txt->Close();

#if 0
	ASSET.PushFolder(folder);
	fiStream *dds = ASSET.Open(texBuf,"");
	ASSET.PopFolder();

	char fnbuf[128];
	formatf(fnbuf,"x:\\gta5\\src\\dev\\rage\\base\\src\\grcore\\%s_fnt.h",filename);
	fiStream *output = fiStream::Create(fnbuf);
	fprintf(output,"// Generated by code, do not edit.\r\n\r\n");
	fprintf(output,"static unsigned char %s_dds[] = {\r\n\t",filename);
	int stored = 0, d;
	while ((d = dds->FastGetCh()) != -1)
		fprintf(output,"%d,%s",d,(stored++&15)==15?"\r\n\t":"");
	fprintf(output,"};\r\n\r\n");
	dds->Close();

	fprintf(output,"static ::rage::grcFont::Glyph %s_Glyphs[%d]={\r\n",filename,charCount);
	for (int i=0; i<charCount; i++) {
		Glyph &g = f->m_Glyphs[i];
		fprintf(output,"\t{ %u, %u, %u, %u, %u, %u, %d, %d, %d },\r\n",g.x,g.y,g.kerningSlot,g.kerningCount,g.width,g.height,g.xoffset,g.yoffset,g.xadvance);
	};
	fprintf(output,"};\r\n\r\n");
	fprintf(output,"static ::rage::s16 %s_GlyphLut[%d]={\r\n\t",filename,highest+1);
	for (int i=0; i<=highest; i++)
		fprintf(output,"%d,%s",f->m_GlyphLut[i],(i&15)==15?"\r\n\t":"");
	fprintf(output,"\r\n};\r\n\r\n");
	if (kerningsCount) {
		fprintf(output,"static ::rage::grcFont::Kerning %s_Kerning[]={\r\n",filename);
		for (int i=0; i<kerningsCount; i++)
			fprintf(output,"\t{ %u, %d } /* %u: %d %d %d */,\r\n",f->m_Kerning[i].second,f->m_Kerning[i].xadvance,i,tk[i].first,tk[i].second,tk[i].xadvance);
		fprintf(output,"};\r\n\r\n");
	}
	fprintf(output,"::rage::grcFont::FontMetrics %s_Metrics = {\r\n",filename);
	fprintf(output,"\t%s_Glyphs, %s_GlyphLut, %s%s, %s_dds, sizeof(%s_dds), %d, %d, %d, %s\r\n};\r\n",
		filename, filename,
		kerningsCount? filename : "NULL",
		kerningsCount? "_Kerning" : "",
		filename,filename,
		f->m_CharWidth, f->m_CharHeight, f->m_CharCount,
		f->m_HasOutline? "true":"false");
	output->Close();
#endif

	return f;
}
