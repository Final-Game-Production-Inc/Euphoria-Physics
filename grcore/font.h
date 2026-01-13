//
// grcore/font.h
//
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved.
//

#ifndef GRCORE_FONT_H
#define GRCORE_FONT_H

#include "effect.h"
#include "vector/color32.h"

namespace rage {

class grcImage;
class grcTexture;

// #define gfxDrawFont(x,y,s) grcFont::GetCurrent().Draw(x,y,s)

/*
PURPOSE
	grcFont provides basic font rendering support.  It supports
	proportional- and fixed-width fonts.
<FLAG Component>
*/
class grcFont {
public:
	struct Glyph { 
		u16 x, y;
		u16 kerningSlot;
		u8 kerningCount;
		u8 width, height; 
		s8 xoffset, yoffset, xadvance;
	};
	struct Kerning {
		u16 second;
		s16 xadvance;	// Could turn on packing to save a byte here
	};
	struct FontMetrics {
		Glyph *m_Glyphs;
		s16 *m_GlyphLut;
		Kerning *m_Kerning;
		u8 *m_TextureData;
		size_t m_TextureDataSize;
		int m_CharWidth, m_CharHeight, m_CharCount;
		bool m_HasOutline;
	};

	// PURPOSE:	Constructor
	grcFont();

	// PURPOSE: Destructor
	~grcFont();

	// PURPOSE: Returns true if a current font is set.
	static bool HasCurrent();

	// PURPOSE: Returns reference to the current font
	// NOTES:	rmcSetup sets this up as an 8x8 fixed-width font.
	static inline grcFont& GetCurrent();

	// PURPOSE: Sets new current font
	// PARAMS:	newFont - New font.
	// RETURNS:	Previously bound font (may be NULL)
	static inline grcFont* SetCurrent(grcFont *newFont);

    // PURPOSE: Get the internal font scaling factor
    inline float GetInternalScale() const;

    // PURPOSE: Sets the internal font scaling factor
    // PARAMS:  scale - new scale factor
    inline void SetInternalScale(const float scale);

    // PURPOSE: Get the internal font color
    inline Color32 GetInternalColor() const;

    // PURPOSE: Sets the internal font color
    // PARAMS:  color - new color
    inline void SetInternalColor(const Color32 color);

	// PURPOSE: Returns character cell width of this font
	inline int GetWidth() const;

	// PURPOSE: Returns pixel width of a string in this font
	// PARAMS:	s - string to measure
	//			sl - strlen(s) (caller probably already knows it, and we use a fast path if it's
	//				a fixed-width font)
	inline int GetStringWidth(const char *s,int sl) const;

	// PURPOSE: Returns character cell height of this font.
	inline int GetHeight() const;

	// PURPOSE:	Begin a sequence of character drawing.
	//			Call this yourself first if you call DrawChar directly.
	//			Draw and Drawf call this internally.
	void DrawCharBegin() const;

	// PURPOSE:	End a sequence of character drawing.
	//			Call this yourself afterward if you call DrawChar directly.
	//			Draw and Drawf call this internally.
	void DrawCharEnd() const;

	// PURPOSE: Renders a single character at the specified position
	// PARAMS:	x - column
	//			y - row
	//			z - depth value
	//			color - color
	//			scaleX - horizontal scale factor
	//			scaleY - vertical scale factor
	//			ch - Character to render
	// NOTES:	You must bracket your one or more DrawChar calls with DrawCharBegin/End.
	void DrawChar(float x,float y,float z,Color32 color,float scaleX,float scaleY,u16 ch) const;

	// PURPOSE: Renders a character string at the specified position
	// PARAMS:	x - column
	//			y - row
	//			z - depth value
	//			color - color
	//			scaleX - horizontal scale factor
	//			scaleY - vertical scale factor
	//			string - String to render (UTF-8 supported)
	// NOTES:	\n characters in the string reset the left margin to the initial x coordinate
	//			and adjust y downward by the character cell height.  You can also do custom tabstops
	//			by inserting \b## sequences; one or more decimal digits after the \b will adjust
	//			the x coordinate to the left margin plus that value times the scale times the character
	//			cell width.  (Any nondigit will terminate the sequence, so if you wanted to print a number
	//			right after that just insert a space and lower the tabstop value slightly).
	void DrawScaled(float x,float y,float z,Color32 color,float scaleX,float scaleY,const char *string) const;

	// PURPOSE: Renders a string at the specified position
	// PARAMS:	x - column
	//			y - row
	//			string - String to render (UTF-8 supported)
	void Draw(float x,float y,const char *string) const;

	// PURPOSE: Renders a string at the specified position
	// PARAMS:	x - column
	//			y - row
	//			color - text color
	//			string - String to render (UTF-8 supported)
	void Draw(float x,float y,Color32 color,const char *string) const;

	// PURPOSE:	Renders a printf-style string using the current color at the specified position
	// PARAMS:	x, y - Screen position
	//			fmt - printf-style format string (UTF-8 supported)
	void Drawf(float x,float y,const char *fmt,...) const;

	// PURPOSE:	Renders a printf-style string using the current color at the specified position
	//			using the specified color
	// PARAMS:	x, y - Screen position
	//			color - text color
	//			fmt - printf-style format string (UTF-8 supported)
	void Drawf(float x,float y, Color32 color, const char *fmt,...) const;

	// PURPOSE: Creates a fixed-width or proportional-width font from a specified bitmap
	// PARAMS:	bits - Pointer to font bitmap.  There are ((charWidth+7)/8)*charHeight bytes per
	//				character stored in character order; NUL is not included, but all other
	//				control characters are included.
	//			charWidth - Width of the character cell.  Also used as the constant per-character
	//				width if "widths" is NULL.
	//			charHeight - Height of each character cell
	//			firstChar - First character in font
	//			charCount - Number of characters in the font.
	// RETURNS:	Pointer to newly created font object.
	static grcFont* CreateFontFromBitmap(const u8* bits,int charWidth,int charHeight,int firstChar,int charCount);

	// PURPOSE:	Creates a fixed-width font from a file
	// PARAMS:	filename - Name of the font image to load.  Must contain 128 cells, 16 characters per line.
	// RETURNS:	Pointer to newly created font object
	static grcFont* CreateFontFromFile(const char *filename);

	// PURPOSE:	Creates a proportional-width font from a dds file and a fnt file in AngelCode format
	// PARAMS:	folder - Directory containing the .fnt and .dds files
	//			filename - Base name (.fnt assumed if not supplied) of the descriptor file
	// RETURNS:	Pointer to newly created font object
	static grcFont* CreateFontFromFiles(const char *folder,const char *filename);

	// PURPOSE:	Creates a proportional-width font from an embedded dds file and a FontMetric structure
	// RETURNS:	Pointer to newly created font object
	static grcFont* CreateFontFromMetrics(FontMetrics &fontMetrics);

	// PURPOSE:	Creates a fixed-width font from an image
	// PARAMS:	image - Image to create font from (should be in DXT1 format)
	//			charWidth - Width of a character cell
	//			charHeight - Height of a character cell
	//			firstChar - First character in string
	//			charCount - Total number of characters in font
	static grcFont* CreateFontFromImage(grcImage *image,int charWidth,int charHeight,int firstChar,int charCount);

	// PURPOSE: Indicates whether a font has outline information or not (and therefore should be drawn with alpha
	//			and will not need a quad behind to in order to be legible)
	bool HasOutline() const { return m_HasOutline; }

protected:
	int GetStringWidthInternal(const char *s,int sl) const;
	int GetGlyphIndex(u16 ch) const { return ch<m_CharCount? m_GlyphLut[ch] : -1; }
	grcTexture *m_Texture;
	int m_CharWidth, m_CharHeight, m_CharCount;
	Glyph *m_Glyphs;
	s16 *m_GlyphLut;	// -1 if no glyph present, else index into m_Glyphs.  Could use a 2D table here if we start having huge and/or sparse fonts.
	Kerning *m_Kerning;
	float m_InvWidth, m_InvHeight, m_InternalScale[NUMBER_OF_RENDER_THREADS];
    Color32 m_InternalColor[NUMBER_OF_RENDER_THREADS];
	mutable int m_InBegin[NUMBER_OF_RENDER_THREADS];
	bool m_HasOutline, m_StaticMetrics;

	static grcFont* sm_Current[NUMBER_OF_RENDER_THREADS];
};


inline bool grcFont::HasCurrent() {
	return (sm_Current[g_RenderThreadIndex] != NULL);
}


inline grcFont& grcFont::GetCurrent() {
	const unsigned rti = g_RenderThreadIndex;
	FastAssert(sm_Current[rti]);
	return *sm_Current[rti];
}

inline grcFont* grcFont::SetCurrent(grcFont *newFont) {
	const unsigned rti = g_RenderThreadIndex;
	grcFont *old = sm_Current[rti];
	if (g_IsSubRenderThread)
		sm_Current[rti] = newFont;
	else
		for (unsigned i=0; i<NUMBER_OF_RENDER_THREADS; ++i)
			sm_Current[i] = newFont;
	return old;
}

inline int grcFont::GetWidth() const {
	return int(m_CharWidth * m_InternalScale[g_RenderThreadIndex]);
}

inline int grcFont::GetStringWidth(const char *s,int sl) const {
	return GetStringWidthInternal(s,sl);
}

inline int grcFont::GetHeight() const {
	return int(m_CharHeight * m_InternalScale[g_RenderThreadIndex]);
}

inline float grcFont::GetInternalScale() const {
    return m_InternalScale[g_RenderThreadIndex];
}

inline void grcFont::SetInternalScale(const float scale) {
	if (g_IsSubRenderThread)
		m_InternalScale[g_RenderThreadIndex] = scale;
	else
		for (unsigned i=0; i<NUMBER_OF_RENDER_THREADS; ++i)
			m_InternalScale[i] = scale;
}

inline Color32 grcFont::GetInternalColor() const {
    return m_InternalColor[g_RenderThreadIndex];
}

inline void grcFont::SetInternalColor(const Color32 color) {
	if (g_IsSubRenderThread)
		m_InternalColor[g_RenderThreadIndex] = color;
	else
		for (unsigned i=0; i<NUMBER_OF_RENDER_THREADS; ++i)
			m_InternalColor[i] = color;
}

}	// namespace rage

#endif
