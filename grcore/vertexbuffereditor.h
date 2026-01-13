// 
// grcore/vertexbuffereditor.h 
// 
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved. 
// 

#ifndef GRCORE_VERTEXBUFFEREDITOR_H
#define GRCORE_VERTEXBUFFEREDITOR_H

#include "grcore/vertexbuffer.h"

#include "vector/color32.h"
#include "vector/vector2.h"
#include "vector/vector3.h"

namespace rage
{
	class grcFvf;
	class grcVertexBuffer;
	class Matrix34;
	struct mshVertex;
	struct Matrix43;

	class grcBufferEditor // Please keep pure virtual
	{
	public:

		virtual ~grcBufferEditor() {}

#if !__SPU
		virtual bool Lock(u32 uLockType = 0, u32 uOffset = 0, u32 uSize = 0) = 0;
		virtual bool LockReadOnly() = 0;
		virtual void Unlock() = 0;
		virtual bool isLocked(u32 uLockType = 0) const = 0;
#endif // !__SPU
		virtual void* GetLockPtr() const = 0;

		void Zero();
 		void Skin(const Matrix43* matrices, int matrixCount, const u16* matrixPalette, int startVertex, int vertexCount, grcVertexBuffer* output);
		void SwapVertices(int v0, int v1);
		Vector3	GetPosition(int vertIndex);
		Vector3	GetNormal(int vertIndex);
		Vector2	GetUV(int vertIndex, int uvSet);
		Color32	GetDiffuse(int vertIndex);
		void SetSpecular(int vertIndex, const Color32& color);
		Color32 GetSpecular(int vertIndex);
		Vector4	GetTangent(int vertIndex, int uvSet);
		Vector3	GetBinormal(int vertIndex, int uvSet);
		Color32	GetBlendIndices(int vertIndex);
		Vector4	GetBlendWeights(int vertIndex);
		Vector3 GetPositionDynamic(int vertIndex);
		Vector3	GetUVDynamic(int vertIndex, int uvSet);
		Vector4 GetTangentDynamic(int vertIndex, int uvSet);
		Vector4	GetBlendWeightsDynamic(int vertIndex);
		void SetVertex(int vertIndex, const mshVertex& vert, u8* reverseMap = 0);
		void SetPosition(int vertIndex, const Vector3& pos);
		void SetPositionAndBinding(int vertIndex, const Vector4& pos);
		void SetNormal(int vertIndex, const Vector3& nrm);
		void SetUV(int vertIndex, int uvSet, const Vector2& uv, bool bFlipV = true);
		void SetUV(int vertIndex, int uvSet, const Vector3& uv);
		void SetUV(int vertIndex, int uvSet, const Vector4& uv);
		void SetUV(int vertIndex, int uvSet, const Color32& uv);
			
		void SetCPV(int vertIndex, const Color32& cpv);
		Color32 GetCPV(int vertIndex );
		void SetTangent(int vertIndex, int uvSet, const Vector3& tangent);
		void SetTangent(int vertIndex, int uvSet, const Vector4& tangent);
		void SetTangent(int vertIndex, int uvSet, const Vector3& tangent, const Vector3& binormal, const Vector3* normal = 0);
		void SetTangentFast( u8* pTangentPtr, const int uvSet, const Vector3& tangent, const Vector3& binormal, const Vector3* normal, bool hasBinormalChannel );
		void SetBiNormal(int vertIndex, int uvSet, const Vector3& binormal);
		void SetBlendIndices(int vertIndex, Color32 indices);
		void SetBlendWeights(int vertIndex, const Vector4& weights);
		void SetPositionDynamic(int vertIndex, const Vector3& pos);
		void SetNormalDynamic(int vertIndex, const Vector3& nrm);
		void WriteVertexData(const void* pData, int nSize) const;
		u32 GetPositionOffset() const;
		u32 GetNormalOffset() const;
		u32 GetBlendIndexOffset() const;

		const u8* GetVertexMemoryBlockConst( int vertIndex ) const;
		u8* GetVertexMemoryBlock( int vertIndex ) const;

		static void SetShortUVRange(int uvSet, float range);
		void SetShortUV( s16 * s, float x, float y, int uvSet);

		static u32 PackNormal(const Vector3 &normal);
		static void UnpackNormal(Vector3 &ret,u32 packedNormal);

	protected:

		static const int kMaxTexCoordSets = 8;
		static float sm_ShortTexCoordRange[kMaxTexCoordSets];

		virtual void SetFvf(grcFvf* fvf) = 0;
		virtual const grcFvf* GetFvf() const = 0;
		virtual void* GetVertexData() const = 0;
		virtual void SetVertexData(u8* data) = 0;
		virtual void SetDynamic() = 0;
		virtual s32 GetVertexCount() const = 0;
		virtual u32 GetVertexStride() const = 0;
		virtual bool IsReadOnly() const = 0;
		virtual bool IsDynamic() const = 0;
	};

	class grcVertexBufferEditor : public grcBufferEditor
	{
	public:
		explicit grcVertexBufferEditor(grcVertexBuffer* vertexBuffer, bool lock = true, bool readOnly = false);
		grcVertexBufferEditor(grcVertexBuffer* vertexBuffer, u32 uLockType, u32 uOffset, u32 uSize);
		virtual ~grcVertexBufferEditor();

#if __WIN32 && !RSG_DURANGO
		void MakeDynamic();
#else
		void MakeDynamic()
		{
			Quitf("Not yet implemented");
		}
#endif // __WIN32

		grcVertexBuffer* GetVertexBuffer() { return m_VertexBuffer; }
		const grcVertexBuffer* GetVertexBuffer() const { return m_VertexBuffer; }

#if !__SPU
		bool Lock(u32 uLockType = 0, u32 uOffset = 0, u32 uSize = 0);
		bool LockReadOnly();
		void Unlock();
		bool isLocked(u32 uLockType = 0) const;
#endif // !__SPU
		void* GetLockPtr() const;
		bool isValid() { return (GetVertexData() != NULL) ? true : false; }
		void Set(const grcVertexBuffer* pSource);

	protected:
		void SetFvf(grcFvf* fvf)		{ m_VertexBuffer->SetFvf(fvf); }
		const grcFvf* GetFvf() const	{ return m_VertexBuffer->GetFvf(); }
		void* GetVertexData() const		{ return m_VertexBuffer->GetVertexData(); }
		void SetVertexData(u8* data)	{ m_VertexBuffer->SetVertexData(data); }
		void SetDynamic()				{ m_VertexBuffer->SetDynamic(); }
		s32 GetVertexCount() const		{ return m_VertexBuffer->GetVertexCount(); }
		u32 GetVertexStride() const		{ return m_VertexBuffer->GetVertexStride(); }
		bool IsReadOnly() const;
		bool IsDynamic() const			{ return m_VertexBuffer->IsDynamic(); }

		grcVertexBuffer* m_VertexBuffer;

	private:
		// SPU code doesn't actually work directly on the vertex buffer in xdr,
		// instead it works on a local store copy only
		u32 m_uLockType;
		u8* m_CachedLockPtrWithFvfVertexPositionOffset;
		bool m_FvfSizeIsEight;

		grcVertexBufferEditor(const grcVertexBufferEditor&);
		grcVertexBufferEditor &operator=(const grcVertexBufferEditor&);
	};

	class grcSystemBufferEditor : public grcBufferEditor
	{
	public:
		grcSystemBufferEditor(void *pvBuffer, u32 uByteSize, const grcFvf* poFVF);
#if !__SPU
		bool Lock(u32 UNUSED_PARAM(uLockType) = 0, u32 UNUSED_PARAM(uOffset) = 0, u32 UNUSED_PARAM(uSize) = 0) { Assert(m_Buffer != NULL); return true; }
		bool LockReadOnly() { Assert(m_Buffer != NULL); return true; }
		void Unlock() {}
		bool isLocked(u32 UNUSED_PARAM(uLockType) = 0) const { Assert(m_Buffer != NULL); return true; }
#endif // !__SPU
		void* GetLockPtr() const { Assert(m_Buffer != NULL); return m_Buffer; }
		void Set(const void* pSource);

	protected:
		void SetFvf(grcFvf* fvf)		{ m_Fvf = *fvf; m_Stride = fvf->GetTotalSize(); }
		void* GetVertexData() const		{ return m_Buffer; }
		void SetVertexData(u8* data)	{ m_Buffer = data; }
		void SetDynamic() {}
		const grcFvf* GetFvf() const	{ return &m_Fvf; }
		s32 GetVertexCount() const		{ return (s32)(m_ByteSize / m_Stride); }
		u32 GetVertexStride() const		{ return m_Stride; }
		bool IsReadOnly() const			{ return false; }
		bool IsDynamic() const			{ return false; }

		void*	m_Buffer;
		u32     m_ByteSize;
		grcFvf	m_Fvf;
		u32		m_Stride;

	};

	inline bool grcVertexBufferEditor::IsReadOnly() const
	{
#if !__SPU
		return grcVertexBuffer::AreLockFlagsReadOnly(m_uLockType) ? true : false;
#else
		return false;
#endif // !__SPU
	}

	inline void* grcVertexBufferEditor::GetLockPtr() const
	{
#if !__SPU
		Assert(isLocked());
#endif // !__SPU
		return m_VertexBuffer->GetFastLockPtr();
	}

#if !__SPU
	inline bool grcVertexBufferEditor::Lock(u32 uLockType, u32 uOffset, u32 uSize) 
	{ 
		Assert(m_VertexBuffer != NULL);
		Assert(!isLocked());
		bool isLocked = !!m_VertexBuffer->Lock(uLockType, uOffset, uSize); 
		m_uLockType = uLockType;

		m_CachedLockPtrWithFvfVertexPositionOffset = reinterpret_cast<u8*>(m_VertexBuffer->GetFastLockPtr());
		const grcFvf* fvf = m_VertexBuffer->GetFvf();
		m_CachedLockPtrWithFvfVertexPositionOffset += fvf->GetOffset(grcFvf::grcfcPosition);

		m_FvfSizeIsEight = fvf->GetSize(grcFvf::grcfcPosition) == 8;

		return isLocked;
	}
	inline bool grcVertexBufferEditor::LockReadOnly() 
	{ 
		return Lock(grcsRead);
	}
	inline void grcVertexBufferEditor::Unlock() 
	{ 
		Assert(m_VertexBuffer != NULL);
		Assert(isLocked());

		if(IsReadOnly())	
			m_VertexBuffer->UnlockRO();
		else
			m_VertexBuffer->UnlockRW();

		m_CachedLockPtrWithFvfVertexPositionOffset = NULL;
		m_FvfSizeIsEight = false;
		m_uLockType = 0;
	}

	inline bool grcVertexBufferEditor::isLocked(u32 ASSERT_ONLY(uLockType)) const
	{
		Assert( uLockType == 0 || uLockType == m_uLockType);
		return (m_uLockType != 0) ? true : false;
	}
#endif

#if __SPU
	inline u32 PackNormalComponent(float value,u32 size,u32 shift) {
		double scale = (double)((1 << (size-1)) - 1);
		return (u32(((double)value) * scale) & ((1 << size)-1)) << shift;
	}
#else
	inline u32 PackNormalComponent(float value,u32 size,u32 shift) {
		float scale = (float)((1 << (size-1)) - 1);
		return (u32(value * scale) & ((1 << size)-1)) << shift;
	}
#endif

	inline u32 PackNormal_8_8_8(const Vector3 &v) {
		return PackNormalComponent(v.x,8,0) | PackNormalComponent(v.y,8,8) | PackNormalComponent(v.z,8,16);
	}

	inline u32 PackNormal_11_11_10(const Vector3 &v) {
		return PackNormalComponent(v.x,11,0) | PackNormalComponent(v.y,11,11) | PackNormalComponent(v.z,10,22);
	}
} // namespace rage

#endif // GRCORE_VERTEXBUFFEREDITOR_H
