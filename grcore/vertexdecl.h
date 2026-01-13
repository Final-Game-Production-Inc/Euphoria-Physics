//
// grcore/vertexdecl.h
//
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved.
//

#ifndef GRCORE_VERTEXDECL_H
#define GRCORE_VERTEXDECL_H

#include "grcore/config.h"
#include "grcore/fvf.h"

#if __XENON
struct D3DVertexDeclaration;
#elif __WIN32PC
struct IDirect3DVertexDeclaration9;
#elif RSG_ORBIS
#include <gnm/buffer.h>
#endif

namespace rage 
{

struct grcSmallVertexElement {
	u16 DECLARE_BITFIELD_4( 
		stream,2,		// Stream index (0-3)
		offset,6,		// Offset into stream of this data (in words, not bytes)
		type,4,			// grcDataSize
		usage,4			// grcFvfChannels enumerant
		);
};

#if __D3D11

struct d3d11InputElementDesc 
{
	const char *SemanticName;
	unsigned int SemanticIndex;
	unsigned int Format;
	unsigned int InputSlot;
	unsigned int AlignedByteOffset;
	unsigned int InputSlotClass;
	unsigned int InstanceDataStepRate;
};

struct grcVertexDeclaration
{
	int Release() const;
	void AddRef() const;
	static const unsigned int c_MaxStreams = 4;		// Really 16 under DX11...
	int elementCount;
	mutable int refCount;
	unsigned int Stream0Size;
	d3d11InputElementDesc desc[0];
};

#elif __D3D9

struct grcVertexDeclaration
{
	int Release(); // implemented in device_d3d.cpp
	void AddRef();
	static const unsigned int c_MaxAttributes = 16;
	static const unsigned int c_MaxStreams = 4;
#if __XENON
	D3DVertexDeclaration * D3dDecl;
#elif __WIN32PC
	IDirect3DVertexDeclaration9 * D3dDecl;
#endif
	unsigned short Divider[c_MaxStreams];
	unsigned int Stream0Size;
};

#elif __OPENGL || __GCM

struct grcVertexDeclaration
{
	// Vertex attribute array is at 0x1680+4*i, high bit is set for SYSTEM memory.
	struct AttributeFormat {	// Register 0x1740 + 4*i
		unsigned DECLARE_BITFIELD_4(type,4,count,4,stride,8,divider,16);
	};
	static const unsigned int c_MaxAttributes = 16;
	static const unsigned int c_MaxStreams = 4;

	union {
		AttributeFormat Format[c_MaxAttributes];
		unsigned FormatU[c_MaxAttributes];
	};
	unsigned char	Offset[c_MaxAttributes];
	unsigned char	Stream[c_MaxAttributes];
	unsigned		Stream0Size;
	int				RefCount;
	unsigned		StreamFrequencyMode;		// must live at offset 8 within quadword.
	unsigned		IsPadded;

	void AddRef() { ++RefCount; }
	int Release(); // implemented in device_gcm.cpp
} ;

#elif RSG_ORBIS

struct grcVertexDeclaration
{
	static const unsigned int c_MaxAttributes = 16;		// arbitrary limit on Orbis
#if __ASSERT
	sce::Gnm::DataFormat DataFormats[c_MaxAttributes];
#endif
	uint32_t VertexDword3[c_MaxAttributes];
	u8 Offsets[c_MaxAttributes];
	u8 Semantics[c_MaxAttributes];						// grcfcPosition, grcfcWeight, etc.
	u8 Streams[c_MaxAttributes];
	u8 Dividers[c_MaxAttributes];
	unsigned short Stream0Size, ElementCount;
	mutable int RefCount;
	void AddRef() const { ++RefCount; }
	int Release() const;
};

#elif __PSP2

enum grcGxmAttributeFormat {
	SCE_GXM_ATTRIBUTE_FORMAT_U8,		///< 8-bit unsigned integer
	SCE_GXM_ATTRIBUTE_FORMAT_S8,		///< 8-bit signed integer
	SCE_GXM_ATTRIBUTE_FORMAT_U16,		///< 16-bit unsigned integer
	SCE_GXM_ATTRIBUTE_FORMAT_S16,		///< 16-bit signed integer
	SCE_GXM_ATTRIBUTE_FORMAT_U8N,		///< 8-bit unsigned integer normalized to [0,1] range
	SCE_GXM_ATTRIBUTE_FORMAT_S8N,		///< 8-bit signed integer normalized to [-1,1] range
	SCE_GXM_ATTRIBUTE_FORMAT_U16N,		///< 16-bit unsigned integer normalized to [0,1] range
	SCE_GXM_ATTRIBUTE_FORMAT_S16N,		///< 16-bit signed integer normalized to [-1,1] range
	SCE_GXM_ATTRIBUTE_FORMAT_F16,		///< 16-bit half precision floating point
	SCE_GXM_ATTRIBUTE_FORMAT_F32		///< 32-bit single precision floating point
};

struct grcGxmVertexAttribute {
	u16 streamIndex, offset;
	u8 format, componentCount;
	u16 regIndex;				// not used here
};

struct grcGxmVertexStream {
	u16 stride;
	u16 indexSource; // SceGxmIndexSource; always zero for us (16bit, no instancing yet)
};

struct grcVertexDeclaration
{
	static const unsigned int c_MaxAttributes = 16;
	static const unsigned int c_MaxStreams = 4;

	grcGxmVertexAttribute Attributes[c_MaxAttributes];
	grcGxmVertexStream Streams[c_MaxStreams];

	u16			Stream0Size;
	u16			AttributeCount, StreamCount;
	u16			RefCount;

	void AddRef() { ++RefCount; }
	int Release() { if (--RefCount==0) { delete this; return 0; } else return RefCount; }
};

#else

// PSP
struct grcVertexDeclaration {
	unsigned fvf;	// as per Draw calls
	int	refCount;

	void AddRef() { ++refCount; }
	int	Release() { if (--refCount==0) { delete this; return 0; } else return refCount; }
};

#endif // __D3D

struct grcVertexElement
{
	enum eVertexElementType
	{
		grcvetPosition,
		grcvetPositionT,
		grcvetNormal,
		grcvetBinormal,
		grcvetTangent,
		grcvetTexture,
		grcvetBlendWeights,
		grcvetBindings,
		grcvetColor,

		grcvetCount
	};

	// TODO: add streamOffsetOverride here, which will allow Luke's cool "overlap two half3's as half4's" trick to be formally supported.
	// Unfortunately that will require an overhaul of the grcFvf class, and/or a way to create vertex buffers without specifying an FVF.
	grcVertexElement()
		: stream(0)
		, type(grcvetPosition)
		, channel(0)
		, size(0)
		, format(grcFvf::grcdsFloat3)
		, streamFrequencyMode(grcFvf::grcsfmDivide)
		, streamFrequencyDivider(0)
	{
	}

	grcVertexElement(unsigned int stream, eVertexElementType type, unsigned int channel, unsigned int size, grcFvf::grcDataSize format, grcFvf::grcStreamFrequencyMode streamFrequencyMode = grcFvf::grcsfmDivide, u16 streamFrequencyDivider = 0)
		: stream(stream)
		, type(type)
		, channel(channel)
		, size(size)
		, format(format)
		, streamFrequencyMode(streamFrequencyMode)
		, streamFrequencyDivider(streamFrequencyDivider)
	{
#if __OPENGL || __GCM
		FastAssert(stream < grcVertexDeclaration::c_MaxStreams);
#else
		FastAssert(stream < 16); // Sigh... hard coded magic number... sigh
#endif // __OPENGL || __GCM

#if __D3D11 || RSG_ORBIS
		//On DX11/Orbis we can have more channels, not sure this is the correct limit but its enough for now.
		FastAssert(channel < 16);
#else
		FastAssert(channel < 8); // Sigh... hard coded magic number... sigh
#endif
	}

	unsigned int					stream;			// Stream number
	eVertexElementType				type;			// Semantic type of the element
	unsigned int					channel;		// Channel of the element 0 - 7
	unsigned int					size;			// Size of the element in bytes
	grcFvf::grcDataSize				format;			// Format of the element data
	grcFvf::grcStreamFrequencyMode	streamFrequencyMode;
	u32								streamFrequencyDivider;
	//Note: Please make sure not to leave any padding uninitialized because this structure is hashed by grmModelFactory.
};

} // namespace rage

#endif // GRCORE_VERTEXDECL_H
