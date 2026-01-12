//
// pheffects/clothdata.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHEFFECTS_CLOTHDATA_H
#define PHEFFECTS_CLOTHDATA_H

#include "clothconnectivitydata.h"
#include "paging/ref.h"
#include "math/float16.h"
#include "vectormath/vectortypes.h"

#include "parser/manager.h"


RAGE_DECLARE_CHANNEL(Cloth)


#define clothAssertf(cond,fmt,...)				RAGE_ASSERTF(Cloth,cond,fmt,##__VA_ARGS__)
#define clothFatalAssertf(cond,fmt,...)			RAGE_FATALASSERTF(Cloth,cond,fmt,##__VA_ARGS__)
#define clothVerifyf(cond,fmt,...)				RAGE_VERIFYF(Cloth,cond,fmt,##__VA_ARGS__)
#define clothErrorf(fmt,...)					RAGE_ERRORF(Cloth,fmt,##__VA_ARGS__)
#define clothWarningf(fmt,...)					RAGE_WARNINGF(Cloth,fmt,##__VA_ARGS__)
#define clothDisplayf(fmt,...)					RAGE_DISPLAYF(Cloth,fmt,##__VA_ARGS__)
#define clothDebugf1(fmt,...)					RAGE_DEBUGF1(Cloth,fmt,##__VA_ARGS__)
#define clothDebugf2(fmt,...)					RAGE_DEBUGF2(Cloth,fmt,##__VA_ARGS__)
#define clothDebugf3(fmt,...)					RAGE_DEBUGF3(Cloth,fmt,##__VA_ARGS__)
#define clothLogf(severity,fmt,...)				RAGE_LOGF(Cloth,severity,fmt,##__VA_ARGS__)
#define clothCondLogf(cond,severity,fmt,...)	RAGE_CONDLOGF(cond,Cloth,severity,fmt,##__VA_ARGS__)

#define NO_PIN_VERTS_IN_VERLET					(!__RESOURCECOMPILER)
#define NO_VERTS_IN_VERLET						(1 && NO_PIN_VERTS_IN_VERLET)
#define NO_BOUND_CENTER_RADIUS					(1)
#define CLOTH_INSTANCE_FROM_DATA				(NO_VERTS_IN_VERLET && !__FINAL)


namespace rage {


#if __PS3 || __WIN32PC || RSG_DURANGO || RSG_ORBIS
	#define MAGIC_ZERO		1.15203098e-019f
#else
	#define MAGIC_ZERO		0.0f
#endif


// TODO: may be not best solution yet !
// we need an option to write out vector4 as binary
struct phVec3V
{
	u32 m_Data[4];

	PAR_SIMPLE_PARSABLE;
};

class phClothDataDebug : public pgBase
{
public:
	phClothDataDebug() {}

protected:
	atArray<Vec3V>	m_VertexPositions;
	atArray<Vec3V>	m_VertexPrevPositions;

	u16 m_NumVerts;
	u16 m_NumPinVerts;

	PAR_SIMPLE_PARSABLE;
};


class phVerletCloth;

class phClothData : public pgBase
{
	friend class phVerletCloth;
public:

	DECLARE_PLACE(phClothData);

	phClothData(class datResource& rsc);

#if __DECLARESTRUCT
	void DeclareStruct(datTypeStruct &s);
#endif

	phClothData ();
	virtual ~phClothData ();

#if CLOTH_INSTANCE_FROM_DATA
	void InstanceFromData(int vertsCapacity);
#endif


#if NO_PIN_VERTS_IN_VERLET
	void InstanceFromTemplate( const phClothData* copyme );
#else
	void InstanceFromTemplate( const phClothData* copyme, const int numPinVerts );
#endif 

	void Init (const Vec3V *vertices, int numVertices, Vec3V_In gravity, bool allocNormals, int extraVerts );

	Vec3V* GetVertexPointer ();
	Vec3V* GetVertexPrevPointer ();
	Vec3V* GetNormalsPointer ();
	Vec3V* GetPinVertexPointer();

	const Vec3V* GetVertexPointer () const;
	const Vec3V* GetNormalsPointer () const;
	const Vec3V* GetVertexPrevPointer () const;
	const Vec3V* GetPinVertexPointer () const;

	Vec3V_Out GetVertexPosition (int vertexIndex) const;
	Vec3V_Out GetVertexPrevPosition (int vertexIndex) const;
	Vec3V_Out GetVertexInitialNormal (int vertexIndex) const;

	void SetVertexPrevPosition (int vertexIndex, Vec3V_In position);
	void SetVertexPosition (int vertexIndex, Vec3V_In position);

// TODO: this fn shouldn't be in this class
	void TransformVertexPositions( Mat34V_In transform, const int vertIdxStart );

	void SwapVertex (int oldIndex, int newIndex, phClothConnectivityData* connectivity=NULL, bool hasNormals = false);
	int FindVertsNearSegment( int numVertices, Vec3V_In segmentStart, Vec3V_In segmentEnd, ScalarV_In radiusSquared, int* verts, int maxNearVerts, Vec3V_In vOffset) const;

	int GetVertexOppositeEdge (int triIndex, int edgeIndex, const phClothConnectivityData& connectivity) const;

	void VerifyMesh( phClothConnectivityData *connectivity = NULL );

	enum enCLOTH_DATA_FLAGS
	{
		FLAG_COMPRESSION		= 0,
		FLAG_NO_COMPRESSION		= 1 << 0,
		FLAG_IS_ROPE			= 1 << 1,
	};

	void SetFlags( enCLOTH_DATA_FLAGS flags ) { m_Flags = flags; }
	void SetIsRope () { m_Flags = (m_Flags | FLAG_IS_ROPE);	}
	s32  IsFlagSet( enCLOTH_DATA_FLAGS flag ) { return (s32)(m_Flags & flag); }	
	bool IsRope () { return !!(m_Flags & FLAG_IS_ROPE);	}
	void  SetSwitchDistanceUp( float distance ) { m_SwitchDistanceUp = distance; }
	void  SetSwitchDistanceDown( float distance ) { m_SwitchDistanceDown = distance; }
	float GetSwitchDistanceUp() const {	return m_SwitchDistanceUp; }	
	float GetSwitchDistanceDown() const { return m_SwitchDistanceDown; }
#if NO_VERTS_IN_VERLET
	int GetVertexCount() const { return m_NumVerts; }
#endif
	int GetVertexCapacity() const { return m_VertexPositions.GetCapacity(); }
	int GetPrevPositionVertexCount() const { return m_VertexPrevPositions.GetCount(); }
	int GetNormalsCount() const { return m_VertexInitialNormals.GetCount(); }
	int GetPinVertexCapacity() const { return m_VertexPinnedPositions.GetCapacity(); } 

#if NO_PIN_VERTS_IN_VERLET
 #if NO_VERTS_IN_VERLET
	int GetNumPinVerts() const { return (int)m_NumPinVerts; }
	void SetNumPinVerts(int numPinVerts) { m_NumPinVerts = (u16)numPinVerts; }
 #else
	int GetNumPinVerts() const { return m_NumPinVerts; }
	void SetNumPinVerts(int numPinVerts) { m_NumPinVerts = numPinVerts; }
 #endif
#endif 

#if NO_VERTS_IN_VERLET
	int GetNumVerts() const { return (int)m_NumVerts; }
	void SetNumVerts(int numVerts) { m_NumVerts = (u16)numVerts; }
#endif

	void ResizePositions(int vertCount) { m_VertexPositions.Resize(vertCount); }
	void ResizePrevPositions(int vertCount) { m_VertexPrevPositions.Resize(vertCount); }

	void ResizePrevPositionsArray(int vertCount) 
	{ 
		m_VertexPrevPositions.Resize(vertCount);
		for (int i=0; i < vertCount; ++i)
		{
			m_VertexPrevPositions[i] = Vec3V(V_ZERO);
		}
	}

#if !__SPU	

	void Load(const char* filePath, const char* clothName);	
	void Save(void* clothName);

	static const char* sm_tuneFilePath;
	static const char* sm_MetaDataFileAppendix;

 #if __BANK && !__RESOURCECOMPILER
	void RegisterRestInterface(const char* controllerName);
	void UnregisterRestInterface(const char* controllerName);
 #endif

protected:
#endif

	atArray<Vec3V>	m_VertexPinnedPositions;

	atArray<Vec3V>	m_VertexInitialNormals;		// needed only for character cloth
	atArray<Vec3V>	m_VertexPositions;

	atArray<Vec3V>	m_VertexPrevPositions;		// needed only for ropes ... and damage offsets on cloth attached to vehicles :(

// TODO: only position and extra positions arrays are needed. the extra positions array will be used in different way for different contexts
// - env cloth : not used
// - char cloth : used for initial normals
// - rope : used for previous positions ... and still think about using compressed deltas

	u16 m_NumVerts;
	u16 m_NumPinVerts;

#if RSG_CPU_X64
	char m_Padding[ sizeof(pgRef<const phClothData>) - sizeof(int)];
#endif

	float m_SwitchDistanceUp;
	float m_SwitchDistanceDown;
	s32 m_Flags;

	PAR_SIMPLE_PARSABLE;
};


inline Vec3V* phClothData::GetNormalsPointer ()
{
	SPU_VALIDATE2( this, &(m_VertexInitialNormals[0]));
	return &(m_VertexInitialNormals[0]);
}

inline const Vec3V* phClothData::GetNormalsPointer() const
{
	SPU_VALIDATE2( this, &(m_VertexInitialNormals[0]));
	return &(m_VertexInitialNormals[0]);
}

inline Vec3V* phClothData::GetVertexPointer ()
{
	SPU_VALIDATE2( this, &(m_VertexPositions[0]));
	return &(m_VertexPositions[0]);
}

inline const Vec3V* phClothData::GetVertexPointer() const
{
	SPU_VALIDATE2( this, &(m_VertexPositions[0]));
	return &(m_VertexPositions[0]);
}

inline Vec3V* phClothData::GetVertexPrevPointer()
{
	SPU_VALIDATE2( this, &(m_VertexPrevPositions[0]));
	return &(m_VertexPrevPositions[0]);
}

inline Vec3V* phClothData::GetPinVertexPointer()
{
	SPU_VALIDATE2( this, &(m_VertexPinnedPositions[0]));
	return &(m_VertexPinnedPositions[0]);
}

inline const Vec3V* phClothData::GetVertexPrevPointer() const
{
	SPU_VALIDATE2( this, &(m_VertexPrevPositions[0]));
	return (const Vec3V*)&m_VertexPrevPositions[0];
}

inline const Vec3V* phClothData::GetPinVertexPointer() const
{
	SPU_VALIDATE2( this, &(m_VertexPinnedPositions[0]));
	return (const Vec3V*)&m_VertexPinnedPositions[0];
}

inline Vec3V_Out phClothData::GetVertexPosition (int vertexIndex) const
{
	SPU_VALIDATE2( this, &(m_VertexPositions[0]));
	return m_VertexPositions[vertexIndex];
}

inline Vec3V_Out phClothData::GetVertexPrevPosition(int vertexIndex) const
{
	SPU_VALIDATE(&(m_VertexPrevPositions[0]));
	return m_VertexPrevPositions[vertexIndex];
}

inline void phClothData::SetVertexPrevPosition (int vertexIndex, Vec3V_In position)
{
	SPU_VALIDATE(&(m_VertexPrevPositions[0]));
	m_VertexPrevPositions[vertexIndex] = position;
}

inline void phClothData::SetVertexPosition (int vertexIndex, Vec3V_In position)
{
	SPU_VALIDATE(&(m_VertexPositions[0]));
	m_VertexPositions[vertexIndex] = position;
}

inline Vec3V_Out phClothData::GetVertexInitialNormal (int vertexIndex) const
{
	SPU_VALIDATE(&(m_VertexInitialNormals[0]));
	return m_VertexInitialNormals[vertexIndex];
}


#if __SPU
static const vec_double2 PackScale = {	-(float)((1 << ((10) - 1)) - 1) / (float)(1 << 22),
										-(float)((1 << ((10) - 1)) - 1) / (float)(1 << 22)	};
#else
static const __vector4 PackScale = {
	-(float)((1 << ((10) - 1)) - 1) / (float)(1 << 22),
	-(float)((1 << ((10) - 1)) - 1) / (float)(1 << 22),
	-(float)((1 << ((10) - 1)) - 1) / (float)(1 << 22),
	-(float)((1 << (2)) - 1)		/ (float)(1 << 22)};
#endif

static const __vector4	UnpackScaleOffset = {
	-(float)(1 << 22) / (float)((1 << (10 - 1)) - 1),											// Signed scale
	-(float)(1 << 23) / (float)((1 << 2) - 1),													// Unsigned scale
	-(float)(1 << 22) / (float)((1 << (10 - 1)) - 1) * 3.0f,									// Signed offset
	-(float)(1 << 23) / (float)((1 << 2) - 1)};													// Unsigned offset



inline __vector4 Pack_Core32( __vector4& destV, __vector4 VtoPack )
{
#if UNIQUE_VECTORIZED_TYPE && __XENON

		const __vector4 three( Vec3V(V_THREE).GetIntrin128() );			// avoid L2 cache misses
		const __vector4 scaledV = __vnmsubfp( VtoPack, PackScale, three );
		return __vpkd3d( destV, scaledV, VPACK_NORMPACKED32, VPACK_32, 0 );

#elif UNIQUE_VECTORIZED_TYPE && __PS3	

		static const _uvector4 PACK_MASKS			= {	0x40400000, 0x000001ff, 0x00400000, 0 };
		const _uvector4 threeV			= (_uvector4)__vspltw(PACK_MASKS, 0);	// = { 0x40400000, 0x40400000, 0x40400000, 0x40400000 };
		const _uvector4 mantissaMASK	= (_uvector4)__vspltw(PACK_MASKS, 1);	// = { 0x000001ff, 0x000001ff, 0x000001ff, 0x000001ff };
		const _uvector4 signMASK		= (_uvector4)__vspltw(PACK_MASKS, 2);	// = { 0x00400000, 0x00400000, 0x00400000, 0x00400000 };
		const _uvector4 emptyV			= (_uvector4)__vspltw(PACK_MASKS, 3);

		const _uvector4 shift6  = (_uvector4)__vspltisw(0x6);				// = { 0x6,0x6,0x6,0x6 };
		const _uvector4 shift4  = (_uvector4)__vspltisw(0x4);				// = { 0x4,0x4,0x4,0x4 };
		const _uvector4 shift13 = (_uvector4)__vspltisw(0xD);				// = { 0xD,0xD,0xD,0xD };
		const _uvector4 shift10 = (_uvector4)__vspltisw(0xA);				// = { 0xA,0xA,0xA,0xA };


		static const _uvector4 maskZ = { 0, 0, 0, 0x3FF };					// mask lowest 10 bits
		const _uvector4 maskY = (_uvector4)__vslw( maskZ, shift10 );		// mask bits 20 to 11	// = { 0, 0, 0, 0xFFC00 }
		const _uvector4 maskX = (_uvector4)__vslw( maskY, shift10 );		// mask bits 30 to 21	// = { 0, 0, 0, 0x3FF00000 }

		const vec_uchar16 permShiftRight32b = vec_lvsr( 4, (u32*)NULL );	// permute vector for shifting right by 4 bytes		
		const vec_uchar16 permShiftRight16b = vec_lvsr( 2, (u32*)NULL );	// permute vector for shifting right by 2 bytes
		const vec_uchar16 permShiftRight48b = vec_lvsr( 6, (u32*)NULL );	// permute vector for shifting right by 6 bytes
		

	#if __SPU
		const vec_double2 PackScale = {	-(float)((1 << ((10) - 1)) - 1) / (float)(1 << 22),
										-(float)((1 << ((10) - 1)) - 1) / (float)(1 << 22)	};

		const vec_double2 VtoPack12		= spu_extend( (vec_float4)VtoPack );								// X and Z
		static const vec_uchar16 maskYW		= { 0x04, 0x05, 0x06, 0x07, 0x04, 0x05, 0x06, 0x07, 0x1C, 0x1D, 0x1E, 0x1F, 0x1C, 0x1D, 0x1E, 0x1F };
		static const vec_uchar16 maskXYZW	= { 0x00, 0x01, 0x02, 0x03, 0x10, 0x11, 0x12, 0x13, 0x08, 0x09, 0x0A, 0x0B, 0x18, 0x19, 0x1A, 0x1B };
		const vec_double2 VtoPack34		= spu_extend( (vec_float4)vec_perm( VtoPack, VtoPack, maskYW) );	// Y and W	// w doesn't matter here
		const vec_double2 threeVDouble	= spu_extend( (vec_float4)threeV );

		const vec_float4 scaledD12		= spu_roundtf( spu_nmsub(VtoPack12, PackScale, threeVDouble) );
		const vec_float4 scaledD34		= spu_roundtf( spu_nmsub(VtoPack34, PackScale, threeVDouble) );
		
		__vector4 scaledV =  vec_perm( scaledD12, scaledD34, maskXYZW );
	#else
		__vector4 scaledV = __vnmsubfp( VtoPack, PackScale, (__vector4)threeV );
	#endif
		// get the lower 9 bits		// get the sign and shift it
		//  ( v & 0x000001ff ) | ( ( v & 0x00400000 ) >> 13 )

		_uvector4 mantissaV = (_uvector4)__vand( scaledV, (__vector4)mantissaMASK );
		_uvector4 signV		= (_uvector4)__vand( scaledV, (__vector4)signMASK );

		
// TODO: 
// Note: eventually one permute and _vor can be saved here. Revise in the next/future optimization
		__vector4 packedV = (__vector4)__vor( mantissaV, __vsrw( signV, shift13 ) );
		
		_uvector4 packedVZ  = (_uvector4)vec_perm( emptyV, (_uvector4)packedV,  permShiftRight32b );			
		_uvector4 packedVY  = (_uvector4)vec_perm( emptyV, (_uvector4)packedVZ, permShiftRight16b );						
		_uvector4 packedVX  = (_uvector4)vec_perm( emptyV, (_uvector4)packedVZ, permShiftRight48b );		

		packedVY  = (_uvector4)__vsrw( packedVY, shift6 );						// shift right 6 bits in the same word
		packedVX  = (_uvector4)__vslw( packedVX, shift4 );						// shift left 4 bits in the same word

/* // OLD - to delete
		static _uvector4 maskXYZ		= { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0 };
		destV = (__vector4)__vand( (_uvector4)destV, maskXYZ ) ;	
		destV = (__vector4)__vor( (_uvector4)destV, __vand( packedVZ, maskZ ) );		
*/
// Note: use ONE (1) permute instead of the __vand and the first __vor (see above)
		static const vec_uchar16 permuteMASK = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x1C, 0x1D, 0x1E, 0x1F };
		destV = (__vector4) vec_perm( (_uvector4)destV, (_uvector4)__vand( packedVZ, maskZ ), permuteMASK );

		destV = (__vector4)__vor( (_uvector4)destV, __vand( packedVY, maskY ) );	 		
		return (__vector4)__vor( (_uvector4)destV, __vand( packedVX, maskX ) );

#else  // __WIN32PC


// on PC doesn't make difference between _ivector4, _uvector4, __vector4 ... it is all __vector4 i.e. float

		static const u32 three_Hex			= 0x40400000;
		static const u32 mantissaMASK_Hex	= 0x000001ff;
		static const u32 signMASK_Hex		= 0x00400000;
		static const _uvector4 threeV		= _mm_load_ps1( (float*)&three_Hex );
		static const _uvector4 mantissaMASK	= _mm_load_ps1( (float*)&mantissaMASK_Hex );
		static const _uvector4 signMASK		= _mm_load_ps1( (float*)&signMASK_Hex );

		_uvector4 emptyV			= { 0,0,0,0 };
/*
		static _uvector4 maskZ = { 0, 0, 0, 0x3FF };								// mask "lowest" 10 bits
		static _uvector4 maskY = { 0, 0, 0, 0xFFC00 };								// mask bits 20 to 11
		static _uvector4 maskX = { 0, 0, 0, 0x3FF00000 };							// mask bits 30 to 21
*/
		static u32 maskZ_Hex = 0x3FF;
		static u32 maskY_Hex = 0xFFC00;
		static u32 maskX_Hex = 0x3FF00000;

		// load the low word and shuffle to the high word
		static _uvector4 maskZ = _mm_shuffle_ps( emptyV, _mm_load_ss( (float*)&maskZ_Hex), _MM_SHUFFLE(0, 3, 0, 1));	// mask "lowest" 10 bits
		static _uvector4 maskY = _mm_shuffle_ps( emptyV, _mm_load_ss( (float*)&maskY_Hex), _MM_SHUFFLE(0, 3, 0, 1));	// mask bits 20 to 11
		static _uvector4 maskX = _mm_shuffle_ps( emptyV, _mm_load_ss( (float*)&maskX_Hex), _MM_SHUFFLE(0, 3, 0, 1));	// mask bits 30 to 21

		__vector4 scaledV = __vnmsubfp( VtoPack, PackScale, (__vector4)threeV );

		// get the lower 9 bits		// get the sign and shift it
		//  ( v & 0x000001ff ) | ( ( v & 0x00400000 ) >> 13 )

		_uvector4 mantissaV = (_uvector4)__vand( scaledV, (__vector4)mantissaMASK );
		_uvector4 signV		= (_uvector4)__vand( scaledV, (__vector4)signMASK );

		__m128i shiftedSign = _mm_srli_epi32(*(__m128i*)&signV, 13);
		__vector4 packedV = (__vector4)__vor( mantissaV, *(__m128*)&shiftedSign );

		_uvector4 packedVZ  = _mm_shuffle_ps( emptyV, packedV, _MM_SHUFFLE(2, 3, 0, 1));
		_uvector4 packedVY  = _mm_shuffle_ps( emptyV, packedV, _MM_SHUFFLE(1, 3, 0, 1));
		_uvector4 packedVX  = _mm_shuffle_ps( emptyV, packedV, _MM_SHUFFLE(0, 3, 0, 1));

		// actually on PC we need to shift the opposite way 
		__m128i packedVY_i  = _mm_slli_epi32 (*(__m128i*)&packedVY, 10);
		__m128i packedVX_i  = _mm_slli_epi32 (*(__m128i*)&packedVX, 20);

		packedVY  = (_uvector4) *(__m128*)&packedVY_i;
		packedVX  = (_uvector4) *(__m128*)&packedVX_i;

// ugly
		M128_UNION_CAST(destV).m128_f32[3] = 0.0f;

		destV = (__vector4)__vor( (_uvector4)destV, __vand( packedVZ, maskZ ) );
		destV = (__vector4)__vor( (_uvector4)destV, __vand( packedVY, maskY ) );	 		
		destV = (__vector4)__vor( (_uvector4)destV, __vand( packedVX, maskX ) );

		return destV;

#endif

}

inline __vector4 Unpack_Core32( __vector4 VtoUnpack )
{
#if UNIQUE_VECTORIZED_TYPE && __XENON

		const __vector4 three = __vpermwi(UnpackScaleOffset, 0xAB);		// permute constant is 1010 1011 - use Signed offset (Z) and splat in XYZ in three
		const __vector4 UnpackScale = __vpermwi(UnpackScaleOffset, 0x1);// permute constant is 0000 0001 - use Signed scale (X) and splat in XYZ in UnpackScale
		const __vector4 unpackedV = __vupkd3d( VtoUnpack, VPACK_NORMPACKED32 );
		return __vnmsubfp( unpackedV, UnpackScale, three );

#elif UNIQUE_VECTORIZED_TYPE && __PS3

		static const uint32_t X = 0x00010203;
		static const uint32_t Z = 0x08090A0B;
		static const vec_uchar16 xControl = (vec_uchar16)((vec_uint4){ X, X, X, X });
		static const vec_uchar16 zControl = (vec_uchar16)((vec_uint4){ Z, Z, Z, Z });

		const __vector4 threePack = vec_perm( UnpackScaleOffset, UnpackScaleOffset,  zControl );	// use Signed offset (Z) and splat in XYZ in three
		const __vector4 UnpackScale = vec_perm(UnpackScaleOffset, UnpackScaleOffset, xControl);		// use Signed scale (X) and splat in XYZ in UnpackScale

		static _uvector4 maskZUnpack = { 0, 0, 0x3FF, 0  };							// mask the lowest 10 bits of Z component
		const _uvector4 maskYUnpack = __vsldoi( maskZUnpack, maskZUnpack, 0x4 );	// mask the lowest 10 bits of X component	// = { 0x3FF, 0, 0, 0  };
		const _uvector4 maskXUnpack = __vsldoi( maskYUnpack, maskYUnpack, 0x4 );	// mask the lowest 10 bits of Y component	// = { 0, 0x3FF, 0, 0  };
		
		const vec_uchar16 permShiftLeftX = vec_lvsl( 10, (u32*)NULL );				// permute vector for shifting left 10 bytes to place X
		const vec_uchar16 permShiftLeftY = vec_lvsl( 8, (u32*)NULL );				// permute vector for shifting left 8 bytes  to place Y
		const vec_uchar16 permShiftLeftZ = vec_lvsl( 4, (u32*)NULL );				// permute vector for shifting left 4 bytes  to place Z

		const _uvector4 shift4				= (_uvector4)__vspltisw( 0x4 );			// = { 0x4,0x4,0x4,0x4 };
		const _uvector4 shift10				= (_uvector4)__vspltisw( 0xA );			// = { 0xA, 0xA, 0xA, 0xA };				

		static _uvector4 UNPACK_MASKS		= { 0x00000200, ~(0x00000200), 0x40000000, 0x003ffe00 };
		const _uvector4 signMASK_10Bit		= (_uvector4)__vspltw(UNPACK_MASKS, 0);	//= { 0x00000200, 0x00000200, 0x00000200, 0x00000200 };
		const _uvector4 signREMOVE_10Bit	= (_uvector4)__vspltw(UNPACK_MASKS, 1);	//= { ~(0x00000200), ~(0x00000200), ~(0x00000200), ~(0x00000200) };
		const _uvector4 mantissaMASKU		= (_uvector4)__vspltw(UNPACK_MASKS, 2);	//= { 0x40000000, 0x40000000, 0x40000000, 0x40000000 };
		const _uvector4 signSHIFT			= (_uvector4)__vspltw(UNPACK_MASKS, 3);	//= { 0x003ffe00, 0x003ffe00, 0x003ffe00, 0x003ffe00 };
		const _uvector4 emptyV				= (_uvector4)__vspltisw(0);
// TODO: 
// Note: eventually one permute can be saved here. Revise in the next/future optimization
		_uvector4 packedVX  = (_uvector4)vec_perm( (_uvector4)VtoUnpack, emptyV, permShiftLeftX );
		_uvector4 packedVY  = (_uvector4)vec_perm( (_uvector4)VtoUnpack, emptyV, permShiftLeftY );
		_uvector4 packedVZ  = (_uvector4)vec_perm( (_uvector4)VtoUnpack, emptyV, permShiftLeftZ );

		packedVX  = (_uvector4)__vsrw( packedVX, shift4 );							// shift right 4 bits in the same word 	
		packedVY  = (_uvector4)__vsrw( packedVY, shift10 );							// shift right 10 bits in the same word 

		_uvector4	unpackV = __vor( __vand( packedVX, maskXUnpack ), __vand( packedVY, maskYUnpack ) );	
		unpackV = __vor( unpackV, __vand( packedVZ, maskZUnpack ) );

		_uvector4 signSHIFTED		= vec_add( __vand( unpackV, signMASK_10Bit ), signSHIFT );
		_uvector4 mantissaAndNum	=   __vor( __vand( unpackV, signREMOVE_10Bit), mantissaMASKU );
		_uvector4 toUnpack			=   __vor( mantissaAndNum, signSHIFTED );

	#if __SPU
		static const vec_uchar16 maskYW		= { 0x04, 0x05, 0x06, 0x07, 0x04, 0x05, 0x06, 0x07, 0x1C, 0x1D, 0x1E, 0x1F, 0x1C, 0x1D, 0x1E, 0x1F };
		static const vec_uchar16 maskXYZW	= { 0x00, 0x01, 0x02, 0x03, 0x10, 0x11, 0x12, 0x13, 0x08, 0x09, 0x0A, 0x0B, 0x18, 0x19, 0x1A, 0x1B };

		const vec_double2 toUnpack12		= spu_extend( (vec_float4)toUnpack );										// X and Z		
		const vec_double2 toUnpack34		= spu_extend( (vec_float4)vec_perm( toUnpack, toUnpack, maskYW) );			// Y and W
		const vec_double2 threePackD		= spu_extend( (vec_float4)threePack );
		const vec_double2 UnpackScaleDouble = spu_extend( (vec_float4)UnpackScale );

		const vec_float4 scaledD12			= spu_roundtf( spu_nmsub(toUnpack12, UnpackScaleDouble, threePackD) );
		const vec_float4 scaledD34			= spu_roundtf( spu_nmsub(toUnpack34, UnpackScaleDouble, threePackD) );
		
		return (__vector4) vec_perm( scaledD12, scaledD34, maskXYZW );
	#else
		return __vnmsubfp( (__vector4)toUnpack, UnpackScale, threePack );
	#endif

#else

// on PC doesn't make difference between _ivector4, _uvector4, __vector4 ... it is all __vector4 i.e. float

		static const __vector4 threePack	= _mm_shuffle_ps( UnpackScaleOffset, UnpackScaleOffset, _MM_SHUFFLE(2, 2, 2, 2) );
		static const __vector4 UnpackScale	= _mm_shuffle_ps( UnpackScaleOffset, UnpackScaleOffset, _MM_SHUFFLE(0, 0, 0, 0) );

/*
		static _uvector4 maskXUnpack = { 0x3FF, 0, 0, 0  };					// mask the lowest 10 bits of X component
		static _uvector4 maskYUnpack = { 0, 0x3FF, 0, 0  };					// mask the lowest 10 bits of Y component
		static _uvector4 maskZUnpack = { 0, 0, 0x3FF, 0  };					// mask the lowest 10 bits of Z component
*/
		_uvector4 maskXUnpack = { 0, 0, 0, 0 };
		_uvector4 maskYUnpack = { 0, 0, 0, 0 };
		_uvector4 maskZUnpack = { 0, 0, 0, 0 };
// ugly. fix it. use some shuffle ?!
		M128_UNION_CAST(maskXUnpack).m128_u32[0] = 0x3FF;
		M128_UNION_CAST(maskYUnpack).m128_u32[1] = 0x3FF;
		M128_UNION_CAST(maskZUnpack).m128_u32[2] = 0x3FF;

		static const u32 signMASK_10Bit_Hex		= 0x00000200;
		static const u32 signREMOVE_10Bit_Hex	= (u32)~(0x00000200);
		static const u32 mantissaMASKU_Hex		= 0x40000000;
		static const u32 signSHIFT_Hex			= 0x003ffe00;

		static _uvector4 signMASK_10Bit		= _mm_load_ps1( (float*)&signMASK_10Bit_Hex );
		static _uvector4 signREMOVE_10Bit	= _mm_load_ps1( (float*)&signREMOVE_10Bit_Hex );
		static _uvector4 mantissaMASKU		= _mm_load_ps1( (float*)&mantissaMASKU_Hex );
		static _uvector4 signSHIFT			= _mm_load_ps1( (float*)&signSHIFT_Hex );

		_uvector4 emptyV2 = { 0.0f, 0.0f, 0.0f, 0.0f };

		_uvector4 packedVX2  = (_uvector4)_mm_shuffle_ps( (_uvector4)VtoUnpack, emptyV2, _MM_SHUFFLE(3, 3, 3, 3) );
		_uvector4 packedVY2  = (_uvector4)_mm_shuffle_ps( (_uvector4)VtoUnpack, emptyV2, _MM_SHUFFLE(3, 3, 3, 3) );
		_uvector4 packedVZ2  = (_uvector4)_mm_shuffle_ps( emptyV2, (_uvector4)VtoUnpack, _MM_SHUFFLE(3, 3, 3, 3) );

		__m128i packedVX2_i  = _mm_srli_epi32 (*(__m128i*)&packedVX2, 20);
		__m128i packedVY2_i  = _mm_srli_epi32 (*(__m128i*)&packedVY2, 10);
/*
		packedVX2  = (_uvector4)__vsrw( packedVX2, shift4 );										// shift right 4 bits in the same word 	
		packedVY2  = (_uvector4)__vsrw( packedVY2, shift10 );										// shift right 10 bits in the same word 
*/

		packedVX2  = (_uvector4) *(__m128*)&packedVX2_i;
		packedVY2  = (_uvector4) *(__m128*)&packedVY2_i;		

		_uvector4	unpackV = __vor( __vand( packedVX2, maskXUnpack ), __vand( packedVY2, maskYUnpack ) );	
		unpackV = __vor( unpackV, __vand( packedVZ2, maskZUnpack ) );

		_uvector4 _sign_ = __vand( unpackV, signMASK_10Bit );

		// ! _mm_xor_ps on PC treats the values like floats, wtf !
		//			_uvector4 signSHIFTED		= _mm_xor_ps( _sign_, signSHIFT );

// ugly. fix it
		_uvector4 signSHIFTED;
		// why isn't this using a 128 bit integer vector add?
		M128_UNION_CAST(signSHIFTED).m128_u32[0] = M128_UNION_CAST(_sign_).m128_u32[0] + M128_UNION_CAST(signSHIFT).m128_u32[0];
		M128_UNION_CAST(signSHIFTED).m128_u32[1] = M128_UNION_CAST(_sign_).m128_u32[1] + M128_UNION_CAST(signSHIFT).m128_u32[1];
		M128_UNION_CAST(signSHIFTED).m128_u32[2] = M128_UNION_CAST(_sign_).m128_u32[2] + M128_UNION_CAST(signSHIFT).m128_u32[2];
		M128_UNION_CAST(signSHIFTED).m128_u32[3] = M128_UNION_CAST(_sign_).m128_u32[3] + M128_UNION_CAST(signSHIFT).m128_u32[3];

		_uvector4 mantissaAndNum	=   __vor( __vand( unpackV, signREMOVE_10Bit), mantissaMASKU );
		_uvector4 toUnpack			=   __vor( mantissaAndNum, signSHIFTED );

		return __vnmsubfp( (__vector4)toUnpack, UnpackScale, threePack );
#endif
}



// TODO: move to float16.h ??

inline __vector4 Pack_CoreFloat16( __vector4& destinationV, __vector4 VtoPack )
{
#if __XENON
//	return __vpkd3d( destinationV, VtoPack, VPACK_FLOAT16_4, VPACK_64LO, 0 );
	return __vpkd3d( destinationV, VtoPack, VPACK_FLOAT16_4, VPACK_64HI, 0 );
#else
	return Vec::V4Float16Vec4PackIntoZW( destinationV ,VtoPack );
//	return Vec::V4Float16Vec4PackIntoXY( destinationV ,VtoPack );
#endif
}

inline __vector4 Unpack_CoreFloat16( __vector4 VtoUnpack )
{
#if __XENON
// NOTE: the floats are packed into ZW ... use vsldoi if the floats are packed in XY bits
//	__vector4 packed2 = __vsldoi( VtoUnpack, VtoUnpack, 8 );
	return __vupkd3d( VtoUnpack/*packed2*/, VPACK_FLOAT16_4 );

#else 
	// WORK IN PROGRESS: non-sense code to please the compiler
	Assert(0);
	return VtoUnpack;	
#endif
}



inline __vector4 PackVFloat16( __vector4& destinationV, __vector4 VtoPack )
{
	return Pack_CoreFloat16( destinationV, VtoPack );
}

inline __vector4 UnpackVFloat16( __vector4 VtoUnpack )
{
	return Unpack_CoreFloat16( VtoUnpack );
}

inline __vector4 PackV1010102( __vector4& destinationV, __vector4 VtoPack )
{
	return Pack_Core32( destinationV, VtoPack );
}

inline __vector4 UnpackV1010102( __vector4 VtoUnpack )
{
	return Unpack_Core32( VtoUnpack );
}

// TODO: Pack funcs here are dup on those in vertexbuffereditor

inline float UnpackNormalComponentF(u32 value,u32 size,u32 shift) 
{
	float scale = (float)((1 << (size-1)) - 1);
	// Assumes 32bit integer here.  This recovers the original sign.
	u32 signShift = (32 - size - shift);
	int signedValue = ((int)value << signShift) >> (signShift + shift);
	return signedValue * FPInvertFast(scale);
}

#if __PS3 && HACK_GTA4
inline void UnpackNormal_11_11_10V(Vector3 &ret,volatile u32 packedNormal) {
#else
inline void UnpackNormal_11_11_10V(Vector3 &ret,u32 packedNormal) {
#endif	
	ret.x = UnpackNormalComponentF(packedNormal,11,0);
	ret.y = UnpackNormalComponentF(packedNormal,11,11);
	ret.z = UnpackNormalComponentF(packedNormal,10,22);
}

inline void UnpackNormal_8_8_8V(Vector3 &ret,u32 packedNormal) 
{
	ret.x = UnpackNormalComponentF(packedNormal,8,0);
	ret.y = UnpackNormalComponentF(packedNormal,8,8);
	ret.z = UnpackNormalComponentF(packedNormal,8,16);
}


} // namespace rage

#endif // end of #ifndef PHEFFECTS_CLOTHDATA_H
