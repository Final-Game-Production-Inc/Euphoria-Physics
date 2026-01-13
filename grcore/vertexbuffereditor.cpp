#include "grcore/vertexbuffereditor.h"

#include "grcore/fvf.h"

#include "math/float16.h"

#include "mesh/mesh.h"

#include "vector/matrix34.h"
#include "grcore/matrix43.h"
#include "system/codecheck.h"
#include "shaderlib/skinning_method_config.h"

namespace rage
{

#if __VBSTATS
	namespace VertexBufferStats
	{
		PF_PAGE(VBPage,"Vertex Buffer Stats");

		PF_GROUP(VBGeneric);
		PF_LINK(VBPage, VBGeneric);
		PF_TIMER(Lock, VBGeneric);
		PF_TIMER(Unlock, VBGeneric);
		PF_TIMER(Zero, VBGeneric);

		PF_GROUP(SetFunctions);
		PF_LINK(VBPage,SetFunctions);
		PF_TIMER(SetVertex,SetFunctions);
		PF_TIMER(SetPosition,SetFunctions);
		PF_TIMER(SetNormal,SetFunctions);
		PF_TIMER(SetUV,SetFunctions);
		PF_TIMER(SetCPV,SetFunctions);
		PF_TIMER(SetSpecular,SetFunctions);
		PF_TIMER(SetTangent,SetFunctions);
		PF_TIMER(SetBiNormal,SetFunctions);

		PF_GROUP(GetFunctions);
		PF_LINK(VBPage, GetFunctions);
		PF_TIMER(GetPosition, GetFunctions);
		PF_TIMER(GetNormal, GetFunctions);
		PF_TIMER(GetUV, GetFunctions);
		PF_TIMER(GetCPV, GetFunctions);
		PF_TIMER(GetSpecular, GetFunctions);
		PF_TIMER(GetTangent, GetFunctions);
		PF_TIMER(GetBinormal, GetFunctions);
	};

	using namespace VertexBufferStats;
#endif // __VBSTATS

	float grcBufferEditor::sm_ShortTexCoordRange[kMaxTexCoordSets] = {32.0f,32.0f,32.0f,32.0f,32.0f,32.0f,32.0f,32.0f};

	u32 grcBufferEditor::PackNormal(const Vector3 &normal) {
		return
			g_sysPlatform==platform::XENON	? normal.Pack1010102()	:
			g_sysPlatform==platform::PS3	? PackNormal_11_11_10(normal)	:
			PackNormal_8_8_8(normal);
	}

	static inline float UnpackNormalComponent(u32 value,u32 size,u32 shift) 
	{
		float scale = (float)((1 << (size-1)) - 1);
		// Assumes 32bit integer here.  This recovers the original sign.
		u32 signShift = (32 - size - shift);
		int signedValue = ((int)value << signShift) >> (signShift + shift);
		return signedValue * FPInvertFast(scale);
	}

#if __PS3 && HACK_GTA4
	static inline void UnpackNormal_11_11_10(Vector3 &ret,volatile u32 packedNormal) {
#else
	static inline void UnpackNormal_11_11_10(Vector3 &ret,u32 packedNormal) {
#endif	
		ret.x = UnpackNormalComponent(packedNormal,11,0);
		ret.y = UnpackNormalComponent(packedNormal,11,11);
		ret.z = UnpackNormalComponent(packedNormal,10,22);
	}

#if __PS3 && HACK_GTA4
	// Code lifted from vector4_default.h
	static void UnpackNormal_10_10_10_2(Vector4 &ret,volatile u32 packed)
	{
		int ux = (packed & 0x3FF) << 22;
		int uy = ((packed >> 10) & 0x3FF) << 22;
		int uz = ((packed >> 20) & 0x3FF) << 22;
		int uw = (int)packed;
		float fx = (float)(ux >> 22);
		float fy = (float)(uy >> 22);
		float fz = (float)(uz >> 22);
		float fw = (float)(uw >> 30);

		const float scale = 0.0019569471624266144814090019569472f;	 // 1/511
		ret.Set(fx * scale, fy * scale, fz * scale, fw);
	}
#endif

	static void UnpackNormal_8_8_8(Vector3 &ret,u32 packedNormal)
	{
		ret.x = UnpackNormalComponent(packedNormal,8,0);
		ret.y = UnpackNormalComponent(packedNormal,8,8);
		ret.z = UnpackNormalComponent(packedNormal,8,16);
	}

	void grcBufferEditor::UnpackNormal(Vector3 &ret,u32 packedNormal) {
		if (g_sysPlatform==platform::XENON)
			ret.Unpack1010102(packedNormal);
		else
		if (g_sysPlatform==platform::PS3)
			UnpackNormal_11_11_10(ret,packedNormal);
		else
			UnpackNormal_8_8_8(ret,packedNormal);
	}

	grcVertexBufferEditor::grcVertexBufferEditor(grcVertexBuffer* vertexBuffer, bool lock /*= true*/, bool readOnly /*= false*/)
	: m_VertexBuffer(vertexBuffer)
#if !__SPU
	, m_uLockType(0)
#endif
	, m_CachedLockPtrWithFvfVertexPositionOffset(NULL)
	, m_FvfSizeIsEight(false)
	{
		Assert(vertexBuffer);
#if !__SPU
		m_uLockType = (vertexBuffer && lock) ? (readOnly ? grcsRead : grcsWrite) : 0;
		vertexBuffer->Lock(m_uLockType);
#endif // !__SPU
	}

	grcVertexBufferEditor::grcVertexBufferEditor(grcVertexBuffer* vertexBuffer, u32 uLockType, u32 uOffset, u32 uSize) 
		: m_VertexBuffer(vertexBuffer)
#if !__SPU
	, m_uLockType(uLockType)
#endif
		, m_CachedLockPtrWithFvfVertexPositionOffset(NULL)
		, m_FvfSizeIsEight(false)
	{
#if !__SPU
		Lock(uLockType, uOffset, uSize);
#endif // !__SPU
	}

	grcVertexBufferEditor::~grcVertexBufferEditor()
	{
#if !__SPU
		if (isLocked())
		{
			Unlock();
		}
#endif // !__SPU
	}



#if MESH_LIBRARY
	void grcBufferEditor::SetVertex(int vertIndex, const mshVertex& vert, u8* reverseMap)
	{
		const grcFvf* fvf = GetFvf();
		Assert(fvf);
		Assert(vertIndex < GetVertexCount());

		Assert(!IsReadOnly());

#if !__SPU
		Assert(isLocked());
#endif	//!__SPU

		u8* pPtr = static_cast<u8*>(GetLockPtr());

		pPtr += GetVertexStride() * vertIndex;

		if (fvf->GetDataSizeType(grcFvf::grcfcPosition) == grcFvf::grcdsHalf4)
		{
			// TODO -- Float16 optimise
			Float16 *f = (Float16*)pPtr;
			f[0].SetFloat16_FromFloat32_RndNearest(vert.Pos.x);
			f[1].SetFloat16_FromFloat32_RndNearest(vert.Pos.y);
			f[2].SetFloat16_FromFloat32_RndNearest(vert.Pos.z);
			f[3].SetFloat16_FromFloat32_RndNearest((float) reverseMap[vert.Binding.Mtx[0]]);
		}
		else if (fvf->GetDataSizeType(grcFvf::grcfcPosition) == grcFvf::grcdsFloat4)
		{
			float *f = (float*)pPtr;
			f[0] = (vert.Pos.x);
			f[1] = (vert.Pos.y);
			f[2] = (vert.Pos.z);
			f[3] = ((float) reverseMap[vert.Binding.Mtx[0]]);
		}
		else if (fvf->GetDataSizeType(grcFvf::grcfcPosition) == grcFvf::grcdsPackedNormal)  // super packed position 
		{
			u32* pNormalPtr = reinterpret_cast<u32*>(pPtr);
			If_Assert( vert.Pos.x > -1.0f && vert.Pos.x < 1.0f )
				If_Assert( vert.Pos.y > -1.0f && vert.Pos.y < 1.0f )
				If_Assert( vert.Pos.z > -1.0f && vert.Pos.z < 1.0f )
				pNormalPtr[0] = PackNormal(vert.Pos);
		}
		else
			sysMemCpy(pPtr, &vert.Pos, 12);

		if( fvf->GetBindingsChannel() )
		{
			if( vert.Binding.IsPassThrough )
			{
				if( IsDynamic() )
				{
					float fWeight0 = vert.Binding.Wgt[0];
					float fWeight1 = vert.Binding.Wgt[1];
					float fWeight2 = vert.Binding.Wgt[2];
					float fWeight3 = vert.Binding.Wgt[3];
					Vector4* pWeightPtr = (Vector4*)(pPtr + fvf->GetOffset(grcFvf::grcfcWeight));
					pWeightPtr->Set(fWeight0, fWeight1, fWeight2, fWeight3);

					Color32* pIndicesPtr = (Color32*)(pPtr + fvf->GetOffset(grcFvf::grcfcBinding));
					pIndicesPtr->Set(vert.Binding.Mtx[0],vert.Binding.Mtx[1],vert.Binding.Mtx[2],vert.Binding.Mtx[3]);
				}
				else
				{
					int w0 = int(vert.Binding.Wgt[0] * 255);
					int w1 = int(vert.Binding.Wgt[1] * 255);
					int w2 = int(vert.Binding.Wgt[2] * 255);
					int w3 = int(vert.Binding.Wgt[3] * 255);

					Color32* pWeights = (Color32*)(pPtr + fvf->GetOffset(grcFvf::grcfcWeight));
					pWeights[0].Set(w0, w1, w2, w3);
					pWeights[1].Set(vert.Binding.Mtx[0],vert.Binding.Mtx[1],vert.Binding.Mtx[2],vert.Binding.Mtx[3]);
				}			
			}
			else
			{
				Assert(reverseMap);
				if( IsDynamic() )
				{
					float fWeight1 = vert.Binding.Wgt[1];
					float fWeight2 = vert.Binding.Wgt[2];
					float fWeight3 = vert.Binding.Wgt[3];
					float fWeight0 = 1.0f - fWeight1 - fWeight2 - fWeight3;
					Vector4* pWeightPtr = (Vector4*)(pPtr + fvf->GetOffset(grcFvf::grcfcWeight));
					pWeightPtr->Set(fWeight0, fWeight1, fWeight2, fWeight3);

					Color32* pIndicesPtr = (Color32*)(pPtr + fvf->GetOffset(grcFvf::grcfcBinding));
					pIndicesPtr->Set(reverseMap[vert.Binding.Mtx[0]],reverseMap[vert.Binding.Mtx[1]],reverseMap[vert.Binding.Mtx[2]],reverseMap[vert.Binding.Mtx[3]]);
				}
				else
				{
					int w1 = int(vert.Binding.Wgt[1] * 255);
					int w2 = int(vert.Binding.Wgt[2] * 255);
					int w3 = int(vert.Binding.Wgt[3] * 255);
					// Make sure any excess weight gets allocated to the dominant bone.
					int w0 = 255 - w1 - w2 - w3;

					Color32* pWeights = (Color32*)(pPtr + fvf->GetOffset(grcFvf::grcfcWeight));
					pWeights[0].Set(w0, w1, w2, w3);

					// No need to use a matrix palette for these platforms
					if (g_sysPlatform == platform::DURANGO
					||  g_sysPlatform == platform::WIN64PC
					||  g_sysPlatform == platform::ORBIS)
					{
						pWeights[1].Set(vert.Binding.Mtx[0], vert.Binding.Mtx[1], vert.Binding.Mtx[2], vert.Binding.Mtx[3]);
					}
					else
					{
#if __RESOURCECOMPILER
						bool copyMainWeight = g_sysPlatform == platform::XENON && MTX_IN_VB;
#else
						const bool copyMainWeight = (__XENON && MTX_IN_VB);		// PS3 via RSX doesn't seem to matter either way.
#endif
						u8 defBind = reverseMap[vert.Binding.Mtx[0]];

						if (copyMainWeight)	// Copying the main weight is faster with explicit fetches
							pWeights[1].Set(defBind,w1? reverseMap[vert.Binding.Mtx[1]] : defBind,w2?reverseMap[vert.Binding.Mtx[2]]:defBind,w3?reverseMap[vert.Binding.Mtx[3]]:defBind);
						else				// ...but slower when using legacy vertex shader constant method.
							pWeights[1].Set(defBind,reverseMap[vert.Binding.Mtx[1]],reverseMap[vert.Binding.Mtx[2]],reverseMap[vert.Binding.Mtx[3]]);
					}
				}
			}
		}

		if( fvf->GetNormalChannel() ) 
		{
			if( !IsDynamic() && fvf->GetDataSizeType(grcFvf::grcfcNormal) == grcFvf::grcdsPackedNormal )
			{
				u32* pNormalPtr = reinterpret_cast<u32*>(pPtr + fvf->GetOffset(grcFvf::grcfcNormal));
				pNormalPtr[0] = PackNormal(vert.Nrm);
			}
			else
			{
				sysMemCpy(pPtr + fvf->GetOffset(grcFvf::grcfcNormal), &vert.Nrm, fvf->GetSize(grcFvf::grcfcNormal));
			}
		}

		if( fvf->GetDiffuseChannel() )
		{
			Color32 cpv(vert.Cpv);
			Color32 cpvDC(cpv.GetDeviceColor()); // PS3 uses rgba not argb

			sysMemCpy(pPtr + fvf->GetOffset(grcFvf::grcfcDiffuse), &cpvDC, fvf->GetSize(grcFvf::grcfcDiffuse));
		}

		if( fvf->GetSpecularChannel() )
		{
			Color32 cpv(vert.Cpv2);
			Color32 cpvDC(cpv.GetDeviceColor()); // PS3 uses rgba not argb

			sysMemCpy(pPtr + fvf->GetOffset(grcFvf::grcfcSpecular), &cpvDC, fvf->GetSize(grcFvf::grcfcSpecular));
		}

		for( int i = 0; i < grcTexStageCount; i++ )
		{
			if( fvf->GetTextureChannel(i) )
			{
				if (fvf->GetDataSizeType(grcFvf::grcfcTexture0) == grcFvf::grcdsHalf2) 
				{
					Float16 *f = (Float16*) (pPtr + fvf->GetOffset(static_cast<grcFvf::grcFvfChannels>(grcFvf::grcfcTexture0 + i)));
					f[0].SetFloat16_FromFloat32_RndNearest(vert.Tex[i].x);
					f[1].SetFloat16_FromFloat32_RndNearest(1.0f-vert.Tex[i].y);
				}
				else
				if (fvf->GetDataSizeType(grcFvf::grcfcTexture0) == grcFvf::grcdsShort2) 
				{
					Assert(i<kMaxTexCoordSets);
					s16 *s = (s16*) (pPtr + fvf->GetOffset(static_cast<grcFvf::grcFvfChannels>(grcFvf::grcfcTexture0 + i)));
					s[0] = s16((32767.0f*vert.Tex[i].x)/sm_ShortTexCoordRange[i]);
					s[1] = s16((32767.0f*(1.0f-vert.Tex[i].y))/sm_ShortTexCoordRange[i]);
				}
				else
				{
					Vector2* pST = (Vector2*)(pPtr + fvf->GetOffset(static_cast<grcFvf::grcFvfChannels>(grcFvf::grcfcTexture0 + i)));
					pST->x = vert.Tex[i].x;
					pST->y = 1 - vert.Tex[i].y;
				}
			}
		}

		for( int i = 0; i < grcTanBiCount; i++ )
		{
			if( fvf->GetTangentChannel(i) )
			{	
				Vector4 t4;
				Convert(t4, vert.TanBi[i].T);
				// Determine the sign of the binormal
				Vector3 bi(vert.TanBi[i].B);
				Vector3 txn(vert.TanBi[i].T);
				txn.Cross(vert.Nrm);

				// Doing a cross product will rarely yield a perfect match, so we have to do more magic

				// Grab the dominant term & use it for sign comparison
				Vector3 txnAbs;
				txnAbs.Abs(txn);
				float testA, testB;
				if ( txnAbs.x > txnAbs.y ) {
					if ( txnAbs.x > txnAbs.z ) {
						testA = txn.x;
						testB = bi.x;
					}
					else {
						testA = txn.z;
						testB = bi.z;
					}
				}
				else if ( txnAbs.y > txnAbs.z ) {
					testA = txn.y;
					testB = bi.y;
				}
				else {
					testA = txn.z;
					testB = bi.z;
				}
				// Finally, do the comparison
				if ( (testA >= 0.f && testB >= 0) || (testA < 0.f && testB < 0.f) ) {
					t4.w = -1.0f;
				}
				else {
					t4.w = 1.0f;
				}

				if (fvf->GetDataSizeType(static_cast<grcFvf::grcFvfChannels>(grcFvf::grcfcTangent0+i)) == grcFvf::grcdsPackedNormal) 
				{
					u32* pNormalPtr = reinterpret_cast<u32*>(pPtr + fvf->GetOffset(static_cast<grcFvf::grcFvfChannels>(grcFvf::grcfcTangent0+i)));
					pNormalPtr[0] = t4.Pack1010102();
				}
				else if (fvf->GetDataSizeType(static_cast<grcFvf::grcFvfChannels>(grcFvf::grcfcTangent0+i)) == grcFvf::grcdsHalf4) 
				{
					// TODO -- Float16 optimise
					Float16 *pNormalPtr = (Float16*)(pPtr + fvf->GetOffset(static_cast<grcFvf::grcFvfChannels>(grcFvf::grcfcTangent0+i)));
					pNormalPtr[0].SetFloat16_FromFloat32_RndNearest(t4.x);
					pNormalPtr[1].SetFloat16_FromFloat32_RndNearest(t4.y);
					pNormalPtr[2].SetFloat16_FromFloat32_RndNearest(t4.z);
					pNormalPtr[3].SetFloat16_FromFloat32_RndNearest(t4.w);
				}
				else 
				{
					Assertf((fvf->GetDataSizeType(static_cast<grcFvf::grcFvfChannels>(grcFvf::grcfcTangent0+i)) == grcFvf::grcdsFloat4), "Tangent %d GetDataSizeType %d", i, fvf->GetDataSizeType(static_cast<grcFvf::grcFvfChannels>(grcFvf::grcfcTangent0+i)));
					sysMemCpy(pPtr + fvf->GetOffset(static_cast<grcFvf::grcFvfChannels>(grcFvf::grcfcTangent0+i)), &t4, fvf->GetSize(static_cast<grcFvf::grcFvfChannels>(grcFvf::grcfcTangent0+i)));
				}
			}
		}

		for( int i = 0; i < grcTanBiCount; i++ )
		{
			if (fvf->GetBinormalChannel(i)) 
			{
				if (fvf->GetDataSizeType(static_cast<grcFvf::grcFvfChannels>(grcFvf::grcfcBinormal0+i)) == grcFvf::grcdsPackedNormal) 
				{
					u32* pNormalPtr = reinterpret_cast<u32*>(pPtr + fvf->GetOffset(static_cast<grcFvf::grcFvfChannels>(grcFvf::grcfcBinormal0+i)));
					pNormalPtr[0] = PackNormal(vert.TanBi[i].B);
				}
				else 
				{
					sysMemCpy(pPtr + fvf->GetOffset(static_cast<grcFvf::grcFvfChannels>(grcFvf::grcfcBinormal0+i)), &vert.TanBi[i].B, fvf->GetSize(static_cast<grcFvf::grcFvfChannels>(grcFvf::grcfcBinormal0+i)));
				}
			}
		}
	}
#endif

	void grcBufferEditor::SetPosition(int vertIndex, const Vector3& pos)
	{
#if __VBSTATS
		PF_FUNC(SetPosition);
#endif
		const grcFvf* fvf = GetFvf();
		if( fvf->GetPosChannel() )
		{
			Assert(vertIndex < GetVertexCount());

			Assert(!IsReadOnly());
			u8* pPtr = static_cast<u8*>(GetLockPtr());

			pPtr += GetVertexStride() * vertIndex;
			pPtr += fvf->GetOffset(grcFvf::grcfcPosition);
			
			if (fvf->GetDataSizeType(grcFvf::grcfcPosition) == grcFvf::grcdsHalf4)
			{
				Float16 *f = (Float16*)pPtr;
				f[0].SetFloat16_FromFloat32(pos.x);
				f[1].SetFloat16_FromFloat32(pos.y);
				f[2].SetFloat16_FromFloat32(pos.z);
				f[3].SetFloat16_Zero();
			}
			else if (fvf->GetDataSizeType(grcFvf::grcfcPosition) == grcFvf::grcdsHalf3)
			{
				Float16 *f = (Float16*)pPtr;
				f[0].SetFloat16_FromFloat32(pos.x);
				f[1].SetFloat16_FromFloat32(pos.y);
				f[2].SetFloat16_FromFloat32(pos.z);
			}
			else if (fvf->GetDataSizeType(grcFvf::grcfcPosition) == grcFvf::grcdsHalf2)
			{
				Float16 *f = (Float16*)pPtr;
				f[0].SetFloat16_FromFloat32(pos.x);
				f[1].SetFloat16_FromFloat32(pos.y);
			}
			else
			{
				sysMemCpy(pPtr, &pos, fvf->GetSize(grcFvf::grcfcPosition));
			}
		}
	}

	void grcBufferEditor::SetPositionDynamic(int vertIndex, const Vector3& pos)
	{
#if __VBSTATS
		PF_FUNC(SetPosition);
#endif
		const grcFvf* fvf = GetFvf();
		if( fvf->GetPosChannel() )
		{
			Assert(vertIndex < GetVertexCount());
			Assert(!IsReadOnly());
			u8* pPtr = static_cast<u8*>(GetLockPtr());

			pPtr += GetVertexStride() * vertIndex;
			pPtr += fvf->GetOffset(grcFvf::grcfcPosition);
			*(Vector3*)pPtr = pos;
		}
	}

	void grcBufferEditor::SetPositionAndBinding(int vertIndex, const Vector4& pos)
	{
#if __VBSTATS
		PF_FUNC(SetPosition);
#endif
		const grcFvf* fvf = GetFvf();
		if( fvf->GetPosChannel() )
		{
			Assert(vertIndex < GetVertexCount());
			AssertMsg(fvf->GetSize(grcFvf::grcfcPosition) == 8,"Must use Half4 format for this");
			Assert(!IsReadOnly());
			u8* pPtr = static_cast<u8*>(GetLockPtr());
			pPtr += GetVertexStride() * vertIndex;
			pPtr += fvf->GetOffset(grcFvf::grcfcPosition);
			Float16 *f = (Float16*)pPtr;

			// TODO -- Float16 optimise
			f[0].SetFloat16_FromFloat32(pos.x);
			f[1].SetFloat16_FromFloat32(pos.y);
			f[2].SetFloat16_FromFloat32(pos.z);
			f[3].SetFloat16_FromFloat32(pos.w);
		}
	}

	void grcBufferEditor::SetNormal(int vertIndex, const Vector3& nrm)
	{
#if __VBSTATS
		PF_FUNC(SetNormal);
#endif
		const grcFvf* fvf = GetFvf();
		if( fvf->GetNormalChannel() )
		{
			Assert(vertIndex < GetVertexCount());
			Assert(!IsReadOnly());
			u8* pPtr = static_cast<u8*>(GetLockPtr());
			pPtr += GetVertexStride() * vertIndex;
			pPtr += fvf->GetOffset(grcFvf::grcfcNormal);
			if( fvf->GetDataSizeType(grcFvf::grcfcNormal) == grcFvf::grcdsPackedNormal )
			{
				*reinterpret_cast<u32*>(pPtr) = PackNormal(nrm);
			}
			else
			{
				sysMemCpy(pPtr, &nrm, fvf->GetSize(grcFvf::grcfcNormal));
			}
		}
	}

	void grcBufferEditor::SetNormalDynamic(int vertIndex, const Vector3& nrm)
	{
		Assert(!__WIN32PC == IsDynamic());
		const grcFvf* fvf = GetFvf();
		if( fvf->GetNormalChannel() )
		{
			Assert(vertIndex < GetVertexCount());
			Assert(!IsReadOnly());
			u8* pPtr = static_cast<u8*>(GetLockPtr());

			pPtr += GetVertexStride() * vertIndex;
			pPtr += fvf->GetOffset(grcFvf::grcfcNormal);
			*(Vector3*)pPtr = nrm;
		}
	}

	void grcBufferEditor::SetCPV(int vertIndex, const Color32& color)
	{
#if __VBSTATS
		PF_FUNC(SetCPV);
#endif
		const grcFvf* fvf = GetFvf();
		if( fvf->GetDiffuseChannel() )
		{
			Assert(vertIndex < GetVertexCount());
			Assert(!IsReadOnly());
			u8* pPtr = static_cast<u8*>(GetLockPtr());
			pPtr += GetVertexStride() * vertIndex;
			pPtr += fvf->GetOffset(grcFvf::grcfcDiffuse);
			*(Color32*)pPtr = Color32( color.GetDeviceColor() );
		}
	}

	Color32 grcBufferEditor::GetCPV(int vertIndex )
	{
#if HACK_GTA4
		Color32 res(0,0,0,0);
#endif // HACK_GTA4
		const grcFvf* fvf = GetFvf();
		if( fvf->GetDiffuseChannel() )
		{
			Assert(vertIndex < GetVertexCount());

			u8* pPtr = static_cast<u8*>(GetLockPtr());

			pPtr += GetVertexStride() * vertIndex;
			pPtr += fvf->GetOffset(grcFvf::grcfcDiffuse);

#if HACK_GTA4
			res.SetFromDeviceColor( ((Color32*)pPtr)->GetColor() );
#else // HACK_GTA4
			return *(Color32*)pPtr;
#endif // HACK_GTA4
		}
#if HACK_GTA4
		return res;
#else // HACK_GTA4
		return Color32(0,0,0,0);
#endif // HACK_GTA4
	}

	void grcBufferEditor::SetTangent(int vertIndex, int uvSet, const Vector3& tangent)
	{
		const grcFvf* fvf = GetFvf();
		if( fvf->GetTangentChannel(uvSet) )
		{
			Assert(vertIndex < GetVertexCount());

			Assert(!IsReadOnly());
#if !__SPU
			Assert(isLocked());
#endif	//!__SPU


			u8* pVertPtr = static_cast<u8*>(GetLockPtr());

			pVertPtr += GetVertexStride() * vertIndex;
			u8* pTangentPtr = pVertPtr + fvf->GetOffset(static_cast<grcFvf::grcFvfChannels>(grcFvf::grcfcTangent0+uvSet));

			if ( (fvf->GetDataSizeType(grcFvf::grcfcTangent0) == grcFvf::grcdsPackedNormal) ) 
			{
				*reinterpret_cast<u32*>(pTangentPtr) = PackNormal(tangent);
			}
			else 
			{
				sysMemCpy(pTangentPtr, &tangent, fvf->GetSize(static_cast<grcFvf::grcFvfChannels>(grcFvf::grcfcTangent0+ uvSet)));
			}
		}
	}

	void grcBufferEditor::SetTangent(int vertIndex, int uvSet, const Vector4& tangent)
	{
		const grcFvf* fvf = GetFvf();
		if( fvf->GetTangentChannel(uvSet) )
		{
			Assert(vertIndex < GetVertexCount());

			Assert(!IsReadOnly());
#if !__SPU
			Assert(isLocked());
#endif	//!__SPU


			u8* pVertPtr = static_cast<u8*>(GetLockPtr());

			pVertPtr += GetVertexStride() * vertIndex;
			u8* pTangentPtr = pVertPtr + fvf->GetOffset(static_cast<grcFvf::grcFvfChannels>(grcFvf::grcfcTangent0+uvSet));

			if (fvf->GetDataSizeType(static_cast<grcFvf::grcFvfChannels>(grcFvf::grcfcTangent0+uvSet)) == grcFvf::grcdsPackedNormal) 
			{
				u32* pNormalPtr = reinterpret_cast<u32*>(pTangentPtr);
				pNormalPtr[0] = tangent.Pack1010102();
			}
			else if (fvf->GetDataSizeType(static_cast<grcFvf::grcFvfChannels>(grcFvf::grcfcTangent0+uvSet)) == grcFvf::grcdsHalf4) 
			{
				// TODO -- Float16 optimise
				Float16 *pNormalPtr = (Float16*)(pTangentPtr);
				pNormalPtr[0].SetFloat16_FromFloat32(tangent.x);
				pNormalPtr[1].SetFloat16_FromFloat32(tangent.y);
				pNormalPtr[2].SetFloat16_FromFloat32(tangent.z);
				pNormalPtr[3].SetFloat16_FromFloat32(tangent.w);
			}
			else 
			{
				Assert(fvf->GetDataSizeType(static_cast<grcFvf::grcFvfChannels>(grcFvf::grcfcTangent0+uvSet)) == grcFvf::grcdsFloat4);
				sysMemCpy(pTangentPtr, &tangent, fvf->GetSize(static_cast<grcFvf::grcFvfChannels>(grcFvf::grcfcTangent0+uvSet)));
			}
		}
	}

	void grcBufferEditor::SetTangent(int vertIndex, int uvSet, const Vector3& tangent, const Vector3& binormal, const Vector3* normal)
	{
#if __VBSTATS
		PF_FUNC(SetTangent);
#endif
		const grcFvf* fvf = GetFvf();
		if( fvf->GetTangentChannel(uvSet) )
		{
			Assert(vertIndex < GetVertexCount());

			Assert(!IsReadOnly());

#if !__SPU
			Assert(isLocked());
#endif	//!__SPU

			u8* pVertPtr = static_cast<u8*>(GetLockPtr());

			pVertPtr += GetVertexStride() * vertIndex;
			u8* pTangentPtr = pVertPtr + fvf->GetOffset(static_cast<grcFvf::grcFvfChannels>(grcFvf::grcfcTangent0+uvSet));

			if( !fvf->GetBinormalChannel(uvSet) )
			{
				// Determine the sign of the binromal
				//Vector3 bi(GetBinormal(vertIndex, uvSet));
				Vector3 bi = binormal;
				Vector3 txn(tangent);
				if( !normal )
					txn.Cross(GetNormal(vertIndex));
				else
					txn.Cross(*normal);
#if __XENON && VECTORIZED
				// Keeping this in the vector pipeline is a huge win on xenon but may not be on other platforms
				Vector3 vDot = txn.DotV(bi);
				Vector3 vTest = vDot.IsLessThanV4(VEC3_ZERO);
				Vector3 vBi = vTest.Select(-VEC3_ONEW, VEC3_ONEW);
				Vector3 vTan = (tangent & VEC3_ANDW) | vBi;
				*(__vector4*)pTangentPtr = vTan;
#else
				Vector4 t4;
				Convert(t4, tangent);
				t4.w = Selectf(txn.Dot(bi), -1.0f, 1.0f);
				if (fvf->GetDataSizeType(static_cast<grcFvf::grcFvfChannels>(grcFvf::grcfcTangent0+uvSet)) == grcFvf::grcdsPackedNormal) 
				{
					u32* pNormalPtr = reinterpret_cast<u32*>(pTangentPtr);
					pNormalPtr[0] = t4.Pack1010102();
				}
				else if (fvf->GetDataSizeType(static_cast<grcFvf::grcFvfChannels>(grcFvf::grcfcTangent0+uvSet)) == grcFvf::grcdsHalf4) 
				{
					// TODO -- Float16 optimise
					Float16 *pNormalPtr = (Float16*)(pTangentPtr);
					pNormalPtr[0].SetFloat16_FromFloat32(t4.x);
					pNormalPtr[1].SetFloat16_FromFloat32(t4.y);
					pNormalPtr[2].SetFloat16_FromFloat32(t4.z);
					pNormalPtr[3].SetFloat16_FromFloat32(t4.w);
				}
				else 
				{
					Assert(fvf->GetDataSizeType(static_cast<grcFvf::grcFvfChannels>(grcFvf::grcfcTangent0+uvSet)) == grcFvf::grcdsFloat4);
					sysMemCpy(pTangentPtr, &t4, fvf->GetSize(static_cast<grcFvf::grcFvfChannels>(grcFvf::grcfcTangent0+uvSet)));
				}
#endif	
			}
			else
			{
				if ((fvf->GetDataSizeType(grcFvf::grcfcTangent0) == grcFvf::grcdsPackedNormal)) 
				{
					*reinterpret_cast<u32*>(pTangentPtr) = PackNormal(tangent);
				}
				else 
				{
					sysMemCpy(pTangentPtr, &tangent, fvf->GetSize(static_cast<grcFvf::grcFvfChannels>(grcFvf::grcfcTangent0+ uvSet)));
				}
			}
		}
	}

	// 1. We know there is a tangentchannel for the respective uvset 
	// 2. We know the pTangentPtr ( we know vertex pointer, vertex stride and tangent stride )
	// 3. We know the normal
	void grcBufferEditor::SetTangentFast( u8* pTangentPtr, const int uvSet, const Vector3& tangent, const Vector3& binormal, const Vector3* normal, bool hasBinormalChannel )
	{
		Assert( pTangentPtr );
		Assert( normal );
#if __VBSTATS
		PF_FUNC(SetTangent);
#endif
		const grcFvf* fvf = GetFvf();
		const grcFvf::grcFvfChannels fvfTangentChannel = static_cast<grcFvf::grcFvfChannels>(grcFvf::grcfcTangent0+uvSet);

		if( !hasBinormalChannel )
		{
			// Determine the sign of the binormal
			Vector3 bi = binormal;
			Vector3 txn(tangent);
			txn.Cross(*normal);
#if __XENON && VECTORIZED
			// Keeping this in the vector pipeline is a huge win on xenon but may not be on other platforms
			Vector3 vDot	= txn.DotV(bi);
			Vector3 vTest	= vDot.IsLessThanV4(VEC3_ZERO);
			Vector3 vBi		= vTest.Select(-VEC3_ONEW, VEC3_ONEW);
			Vector3 vTan	= (tangent & VEC3_ANDW) | vBi;
			*(__vector4*)pTangentPtr = vTan;
#else

			Vector4 t4;
			Convert(t4, tangent);
			t4.w = Selectf(txn.Dot(bi), -1.0f, 1.0f);

			const grcFvf::grcDataSize fvfType = fvf->GetDataSizeType(fvfTangentChannel);
			if ( fvfType == grcFvf::grcdsPackedNormal) 
			{
				u32* pNormalPtr = reinterpret_cast<u32*>(pTangentPtr);
				pNormalPtr[0] = t4.Pack1010102();
			}
			else if( fvfType == grcFvf::grcdsHalf4) 
			{
				// TODO -- Float16 optimise
				Float16 *pNormalPtr = (Float16*)(pTangentPtr);
				pNormalPtr[0].SetFloat16_FromFloat32(t4.x);
				pNormalPtr[1].SetFloat16_FromFloat32(t4.y);
				pNormalPtr[2].SetFloat16_FromFloat32(t4.z);
				pNormalPtr[3].SetFloat16_FromFloat32(t4.w);
			}
			else 
			{
				Assert(fvfType == grcFvf::grcdsFloat4);
				sysMemCpy(pTangentPtr, &t4, fvf->GetSize(fvfTangentChannel));
			}
#endif	
		}
		else
		{
			if ((fvf->GetDataSizeType(grcFvf::grcfcTangent0) == grcFvf::grcdsPackedNormal)) 
			{
				*reinterpret_cast<u32*>(pTangentPtr) = PackNormal(tangent);
			}
			else 
			{
				sysMemCpy(pTangentPtr, &tangent, fvf->GetSize(fvfTangentChannel));
			}
		}
	}

	void grcBufferEditor::SetBiNormal(int vertIndex, int uvSet, const Vector3& binormal)
	{
#if __VBSTATS
		PF_FUNC(SetBiNormal);
#endif
		const grcFvf* fvf = GetFvf();
		if( fvf->GetBinormalChannel(uvSet) )
		{
			Assert(vertIndex < GetVertexCount());

			Assert(!IsReadOnly());
#if !__SPU
			Assert(isLocked());
#endif	//!__SPU


			u8* pVertPtr = static_cast<u8*>(GetLockPtr());

			pVertPtr += GetVertexStride() * vertIndex;
			u8* pBinormalPtr = pVertPtr + fvf->GetOffset(static_cast<grcFvf::grcFvfChannels>(grcFvf::grcfcBinormal0 + uvSet));

			if ((fvf->GetDataSizeType(grcFvf::grcfcBinormal0) == grcFvf::grcdsPackedNormal)) 
			{
				*reinterpret_cast<u32*>(pBinormalPtr) = PackNormal(binormal);
			}
			else 
			{
				sysMemCpy(pBinormalPtr, &binormal, fvf->GetSize(static_cast<grcFvf::grcFvfChannels>(grcFvf::grcfcBinormal0+ uvSet)));
			}
		}
	}


	void grcBufferEditor::SetBlendIndices(int vertIndex, Color32 indices)
	{
		const grcFvf* fvf = GetFvf();
		if( fvf->GetBindingsChannel() )
		{
			Assert(vertIndex < GetVertexCount());

			Assert(!IsReadOnly());
#if !__SPU
			Assert(isLocked());
#endif	//!__SPU


			u8* pPtr = static_cast<u8*>(GetLockPtr());

			pPtr += GetVertexStride() * vertIndex;

			Color32* pIndicesPtr = (Color32*)(pPtr + fvf->GetOffset(grcFvf::grcfcBinding));
			*pIndicesPtr = indices;
		}
	}

	void grcBufferEditor::SetBlendWeights(int vertIndex, const Vector4& weights)
	{
		const grcFvf* fvf = GetFvf();
		if( fvf->GetBindingsChannel() )
		{
			Assert(vertIndex < GetVertexCount());

			Assert(!IsReadOnly());
#if !__SPU
			Assert(isLocked());
#endif	//!__SPU


			u8* pPtr = static_cast<u8*>(GetLockPtr());
			pPtr += GetVertexStride() * vertIndex;

			if( IsDynamic() )
			{
				Vector4* pWeightPtr = (Vector4*)(pPtr + fvf->GetOffset(grcFvf::grcfcWeight));
				*pWeightPtr = weights;
			}
			else
			{
				Color32* pWeights = (Color32*)(pPtr + fvf->GetOffset(grcFvf::grcfcWeight));
				pWeights->Setf(weights.x, weights.y, weights.z, weights.w);
			}
		}
	}

	inline void SetFloat16UV( Float16* f, float x, float y )
	{
		// TODO -- Float16 optimise
		f[0].SetFloat16_FromFloat32(x);
		f[1].SetFloat16_FromFloat32(y);
	}
	
	inline void grcBufferEditor::SetShortUV( s16 * s, float x, float y, int uvSet)
	{
		Assert(uvSet<kMaxTexCoordSets);
		s[0] = s16((32767.0f*x)/sm_ShortTexCoordRange[uvSet]);
		s[1] = s16((32767.0f*y)/sm_ShortTexCoordRange[uvSet]);
	}

	void grcBufferEditor::SetUV(int vertIndex, int uvSet, const Vector2& uv, bool bFlipV)
	{
#if __VBSTATS
		PF_FUNC(SetUV);
#endif
		const grcFvf* fvf = GetFvf();
		if( fvf->GetTextureChannel(uvSet) )
		{
			Assert(vertIndex < GetVertexCount());

			Assert(!IsReadOnly());
#if !__SPU
			Assert(isLocked());
#endif	//!__SPU


			u8* pPtr = static_cast<u8*>(GetLockPtr());

			pPtr += GetVertexStride() * vertIndex;
			pPtr += fvf->GetOffset(static_cast<grcFvf::grcFvfChannels>(grcFvf::grcfcTexture0 + uvSet));
			int sizeType = fvf->GetDataSizeType(static_cast<grcFvf::grcFvfChannels>(grcFvf::grcfcTexture0 + uvSet));
			
			if( bFlipV )
			{

				if (sizeType == grcFvf::grcdsHalf2)
				{
					SetFloat16UV( (Float16*) pPtr, uv.x, 1.0f-uv.y );
				}
				else if (sizeType == grcFvf::grcdsShort2)
				{
					SetShortUV( (s16*) pPtr, uv.x, 1.0f-uv.y ,uvSet);
				}
				else if (sizeType == grcFvf::grcdsFloat4) // very common case (dynamic vertex buffers)
				{
					((Vector4*)pPtr)->Set(uv.x,1.0f-uv.y,0.0f,0.0f);
				}
				else
				{
					Assert(sizeType == grcFvf::grcdsFloat2 );
					((Vector2*)pPtr)->Set(uv.x,1.0f-uv.y);
				}
			}
			else
			{
				if (sizeType == grcFvf::grcdsHalf2) 
				{
					SetFloat16UV( (Float16*) pPtr, uv.x, uv.y );
				}
				else if (sizeType == grcFvf::grcdsShort2)
				{
					SetShortUV( (s16*) pPtr, uv.x, 1.0f-uv.y ,uvSet);
				}
				else if (sizeType == grcFvf::grcdsFloat4) // very common case (dynamic vertex buffers)
				{
					((Vector4*)pPtr)->Set(uv.x,uv.y,0.0f,0.0f);
				}
				else
				{
					Assert(sizeType == grcFvf::grcdsFloat2 );
					*((Vector2*)pPtr) = uv;
					//sysMemCpy(pPtr, &, fvf->GetSize(static_cast<grcFvf::grcFvfChannels>(grcFvf::grcfcTexture0 + uvSet)));
				}
			}
		}
	}
	void grcBufferEditor::SetUV(int vertIndex, int uvSet, const Vector4& uv)
	{
#if __VBSTATS
		PF_FUNC(SetUV);
#endif
		const grcFvf* fvf = GetFvf();
		if( fvf->GetTextureChannel(uvSet) )
		{
			Assert(vertIndex < GetVertexCount());
			Assert(fvf->GetDataSizeType(static_cast<grcFvf::grcFvfChannels>(grcFvf::grcfcTexture0 + uvSet)) == grcFvf::grcdsFloat4 );

			Assert(!IsReadOnly());
#if !__SPU
			Assert(isLocked());
#endif	//!__SPU


			u8* pPtr = static_cast<u8*>(GetLockPtr());

			pPtr += GetVertexStride() * vertIndex;
			pPtr += fvf->GetOffset(static_cast<grcFvf::grcFvfChannels>(grcFvf::grcfcTexture0 + uvSet));
			sysMemCpy(pPtr, &uv, fvf->GetSize(static_cast<grcFvf::grcFvfChannels>(grcFvf::grcfcTexture0 + uvSet)));
		}
	}
	void grcBufferEditor::SetUV(int vertIndex, int uvSet, const Vector3& uv)
	{
#if __VBSTATS
		PF_FUNC(SetUV);
#endif
		const grcFvf* fvf = GetFvf();
		Assert(fvf);

		if( fvf->GetTextureChannel(uvSet) )
		{
			Assert(vertIndex < GetVertexCount());
			ASSERT_ONLY(grcFvf::grcDataSize dataSize = fvf->GetDataSizeType(static_cast<grcFvf::grcFvfChannels>(grcFvf::grcfcTexture0 + uvSet)));
			Assertf(dataSize == grcFvf::grcdsFloat3 || dataSize == grcFvf::grcdsFloat4, "%d", dataSize);

			Assert(!IsReadOnly());
#if !__SPU
			Assert(isLocked());
#endif	//!__SPU


			u8* pPtr = static_cast<u8*>(GetLockPtr());
			pPtr += GetVertexStride() * vertIndex;
			pPtr += fvf->GetOffset(static_cast<grcFvf::grcFvfChannels>(grcFvf::grcfcTexture0 + uvSet));
			sysMemCpy(pPtr, &uv, fvf->GetSize(static_cast<grcFvf::grcFvfChannels>(grcFvf::grcfcTexture0 + uvSet)));
		}
	}
	void grcBufferEditor::SetUV(int vertIndex, int uvSet, const Color32& uv)
	{
#if __VBSTATS
		PF_FUNC(SetUV);
#endif
		const grcFvf* fvf = GetFvf();
		if( fvf->GetTextureChannel(uvSet) )
		{
			const grcFvf* fvf = GetFvf();
			Assert(fvf);
			Assert(vertIndex < GetVertexCount());
			ASSERT_ONLY(grcFvf::grcDataSize dataSize = fvf->GetDataSizeType(static_cast<grcFvf::grcFvfChannels>(grcFvf::grcfcTexture0 + uvSet)));
			Assertf(dataSize == grcFvf::grcdsColor, "%d", dataSize);

			Assert(!IsReadOnly());
#if !__SPU
			Assert(isLocked());
#endif	//!__SPU


			u8* pPtr = static_cast<u8*>(GetLockPtr());
			pPtr += GetVertexStride() * vertIndex;
			pPtr += fvf->GetOffset(static_cast<grcFvf::grcFvfChannels>(grcFvf::grcfcTexture0 + uvSet));
			sysMemCpy(pPtr, &uv, fvf->GetSize(static_cast<grcFvf::grcFvfChannels>(grcFvf::grcfcTexture0 + uvSet)));
		}
	}


	const u8* grcBufferEditor::GetVertexMemoryBlockConst( int vertIndex ) const
	{
		Assert(vertIndex < GetVertexCount());
		Assert(!IsReadOnly());
#if !__SPU
		Assert(isLocked());
#endif	//!__SPU


		u8* pPtr = static_cast<u8*>(GetLockPtr());
		pPtr += GetVertexStride() * vertIndex;
		return pPtr;
	}

	u8* grcBufferEditor::GetVertexMemoryBlock( int vertIndex ) const
	{
		Assert(vertIndex < GetVertexCount());

		Assert(!IsReadOnly());
#if !__SPU
		Assert(isLocked());
#endif	//!__SPU

		u8* pPtr = static_cast<u8*>(GetLockPtr());
		pPtr += GetVertexStride() * vertIndex;
		return pPtr;
	}

	void grcBufferEditor::WriteVertexData(const void* pData, int nSize) const
	{
		Assert(!IsReadOnly());
#if !__SPU
		Assert(isLocked());
#endif	//!__SPU


		u8* pPtr = static_cast<u8*>(GetLockPtr());
		sysMemCpy(pPtr, pData, nSize);
	}

	u32 grcBufferEditor::GetPositionOffset() const
	{
		Assert(GetFvf());
		return GetFvf()->GetOffset(grcFvf::grcfcPosition);
	}

	u32 grcBufferEditor::GetNormalOffset() const
	{
		Assert(GetFvf());
		return GetFvf()->GetOffset(grcFvf::grcfcNormal);
	}

	u32 grcBufferEditor::GetBlendIndexOffset() const
	{
		Assert(GetFvf());
		return GetFvf()->GetOffset(grcFvf::grcfcBinding);
	}

	void grcBufferEditor::Zero()
	{
#if __VBSTATS
		PF_FUNC(Zero);
#endif
		Assert(!IsReadOnly());
#if !__SPU
		Assert(isLocked());
#endif	//!__SPU


		u8* pPtr = static_cast<u8*>(GetLockPtr());
		// Set all the vertex data to zero
		int nSize = GetVertexStride() * GetVertexCount();
		sysMemSet(pPtr, 0, nSize);
	}

	template <class _Type> void Next(_Type* & ptr,int stride) { ptr = (_Type*)((char*) ptr + stride); }

	void grcBufferEditor::Skin(const Matrix43* matrices, int /*matrixCount*/, const u16* matrixPalette, int startVertex, int vertexCount, grcVertexBuffer* output)
	{
		Assert(!__WIN32PC == IsDynamic());
		Assert(startVertex < GetVertexCount());
		Assert(startVertex + vertexCount <= GetVertexCount());
		Assert(output);
		Assert(output->IsDynamic());

#if !__SPU
		Assert(isLocked());
		grcVertexBuffer::LockHelper helper2(output);
#endif
		grcVertexBufferEditor outputEditor(output);

		int srcStride = GetVertexStride();
		int dstStride = output->GetVertexStride();

		const grcFvf *srcFvf = GetFvf();
		bool hasTangent = srcFvf->IsChannelActive(grcFvf::grcfcTangent0);

		const grcFvf *dstFvf = output->GetFvf();
		const char *srcPtr = (char*) GetLockPtr() + startVertex * srcStride;
		char *dstPtr = (char*) output->GetLockPtr() + startVertex * dstStride;
		Assert(srcFvf->GetDataSizeType(grcFvf::grcfcPosition) == grcFvf::grcdsFloat4);
		Assert(dstFvf->GetDataSizeType(grcFvf::grcfcPosition) == grcFvf::grcdsFloat4);
		Assert(srcFvf->GetDataSizeType(grcFvf::grcfcNormal) == grcFvf::grcdsFloat4);
		Assert(dstFvf->GetDataSizeType(grcFvf::grcfcNormal) == grcFvf::grcdsFloat4);
		Assert(false == hasTangent || srcFvf->GetDataSizeType(grcFvf::grcfcTangent0) == grcFvf::grcdsFloat4);
		Assert(false == dstFvf->IsChannelActive(grcFvf::grcfcTangent0) || dstFvf->GetDataSizeType(grcFvf::grcfcTangent0) == grcFvf::grcdsFloat4);
		const Vector3 *pSrcPosition = (const Vector3*)(srcPtr + srcFvf->GetOffset(grcFvf::grcfcPosition));
		Vector3 *pDstPosition = (Vector3*)(dstPtr + dstFvf->GetOffset(grcFvf::grcfcPosition));
		const Vector3 *pSrcNormal = (const Vector3*)(srcPtr + srcFvf->GetOffset(grcFvf::grcfcNormal));
		Vector3 *pDstNormal = (Vector3*)(dstPtr + dstFvf->GetOffset(grcFvf::grcfcNormal));

		const Vector3 *pSrcTangent = hasTangent ? (const Vector3*)(srcPtr + srcFvf->GetOffset(grcFvf::grcfcTangent0)) : NULL;
		Vector3 *pDstTangent = hasTangent ? (Vector3*)(dstPtr + dstFvf->GetOffset(grcFvf::grcfcTangent0)) : NULL;

		if(GetFvf()->GetBindingsChannel())
		{
			Assert(srcFvf->GetDataSizeType(grcFvf::grcfcWeight) == grcFvf::grcdsFloat4);
			const Color32 *pSrcBlendIndices = (Color32*)(srcPtr + srcFvf->GetOffset(grcFvf::grcfcBinding));
			const Vector4 *pSrcBlendWeights = (Vector4*)(srcPtr + srcFvf->GetOffset(grcFvf::grcfcWeight));

			for( int i = startVertex; i < (startVertex + vertexCount); i++ )
			{
				Color32 blendIndices = *pSrcBlendIndices;  
				Next(pSrcBlendIndices,srcStride);
				Vector4 blendWeights = *pSrcBlendWeights;
				Next(pSrcBlendWeights,srcStride);
				int i0 = blendIndices.GetRed();
				int i1 = blendIndices.GetGreen();
				int i2 = blendIndices.GetBlue();
				int i3 = blendIndices.GetAlpha();

#if VECTORIZED_PADDING
				Vector4 vWeight0, vWeight1, vWeight2, vWeight3;
				vWeight0.SplatX(blendWeights);
				vWeight1.SplatY(blendWeights);
				vWeight2.SplatZ(blendWeights);
				vWeight3.SplatW(blendWeights);
				Vector3 vWeight30, vWeight31, vWeight32, vWeight33;
				vWeight0.GetVector3(vWeight30);
				vWeight1.GetVector3(vWeight31);
				vWeight2.GetVector3(vWeight32);
				vWeight3.GetVector3(vWeight33);

				Matrix34 skinMtx, mtx1, mtx2, mtx3;
				matrices[matrixPalette[i0]].ToMatrix34(RC_MAT34V(skinMtx));
				matrices[matrixPalette[i1]].ToMatrix34(RC_MAT34V(mtx1));
				matrices[matrixPalette[i2]].ToMatrix34(RC_MAT34V(mtx2));
				matrices[matrixPalette[i3]].ToMatrix34(RC_MAT34V(mtx3));

				skinMtx.ScaleFull(vWeight30);
				mtx1.ScaleFull(vWeight31);
				mtx2.ScaleFull(vWeight32);
				mtx3.ScaleFull(vWeight33);

				skinMtx.Add(mtx1);
				skinMtx.Add(mtx2);
				skinMtx.Add(mtx3);
#else
				Matrix34 skinMtx;
				matrices[matrixPalette[i0]].ToMatrix34(skinMtx);
				skinMtx.ScaleFull(blendWeights.x);
				Matrix34 temp;
				matrices[matrixPalette[i1]].ToMatrix34(temp);
				temp.ScaleFull(blendWeights.y);
				skinMtx.Add(temp);
				temp = matrices[matrixPalette[i2]];
				temp.ScaleFull(blendWeights.z);
				skinMtx.Add(temp);
				temp = matrices[matrixPalette[i3]];
				temp.ScaleFull(blendWeights.w);
				skinMtx.Add(temp);
#endif

				// Note, this code was only using Vector3 tangents before, which is probably wrong.
				// I'd expect a packed binormal sign in the .w channel, but it seems like that would
				// have been wiped out by the old code anyway.
				skinMtx.Transform(*pSrcPosition,*pDstPosition);
				skinMtx.Transform3x3(*pSrcNormal,*pDstNormal);
				if( hasTangent )
					skinMtx.Transform3x3(*pSrcTangent,*pDstTangent);
				Next(pSrcPosition,srcStride);
				Next(pSrcNormal,srcStride);
				Next(pSrcTangent,srcStride);

				Next(pDstPosition,dstStride);
				Next(pDstNormal,dstStride);
				Next(pDstTangent,dstStride);
			}
		}
		else
		{
			Matrix34 skinMtx;
			matrices[0].ToMatrix34(RC_MAT34V(skinMtx));
			for( int i = startVertex; i < (startVertex + vertexCount); i++ )
			{
				skinMtx.Transform(*pSrcPosition,*pDstPosition);
				skinMtx.Transform3x3(*pSrcNormal,*pDstNormal);
				
				if(hasTangent)
					skinMtx.Transform3x3(*pSrcTangent, *pDstTangent);

				Next(pSrcPosition,srcStride);
				Next(pSrcNormal,srcStride);
				Next(pSrcTangent,srcStride);

				Next(pDstPosition,dstStride);
				Next(pDstNormal,dstStride);
				Next(pDstTangent,dstStride);
			}
		}
	}

	void grcBufferEditor::SwapVertices(int v0, int v1)
	{
		Assert(v0 < GetVertexCount());
		Assert(v1 < GetVertexCount());
		Assert(!IsReadOnly());
#if !__SPU
		Assert(isLocked());
#endif	//!__SPU


		u8* pPtr = static_cast<u8*>(GetLockPtr());
		u32 vertexStride = GetVertexStride();

		u8* pV0Ptr = reinterpret_cast<u8*>(pPtr) + (vertexStride * v0);
		u8* pV1Ptr = reinterpret_cast<u8*>(pPtr) + (vertexStride * v1);

		u8* temp = Alloca(u8, vertexStride);
		sysMemCpy(temp, pV0Ptr, vertexStride);
		sysMemCpy(pV0Ptr, pV1Ptr, vertexStride);
		sysMemCpy(pV1Ptr, temp, vertexStride);
	}

	Vector3 grcBufferEditor::GetPosition(int vertIndex)
	{
#if __VBSTATS
		PF_FUNC(GetPosition);
#endif
		const grcFvf* fvf = GetFvf();
		Assert(fvf->IsChannelActive(grcFvf::grcfcPosition));
		Assert(vertIndex < GetVertexCount());

		Vector3 ret;
#if !__SPU
		Assert(isLocked());
#endif	//!__SPU


		u8* pPtr = static_cast<u8*>(GetLockPtr());
		pPtr += GetVertexStride() * vertIndex;
		pPtr += fvf->GetOffset(grcFvf::grcfcPosition);
		if (fvf->GetSize(grcFvf::grcfcPosition) == 8)
		{
			// TODO -- Float16 optimise
			Float16 *h = reinterpret_cast<Float16*>(pPtr);
			ret.x = h[0].GetFloat32_FromFloat16();
			ret.y = h[1].GetFloat32_FromFloat16();
			ret.z = h[2].GetFloat32_FromFloat16();
		}
		else if (fvf->GetSize(grcFvf::grcfcPosition) ==4) // packed normal format
		{
			UnpackNormal(ret,*reinterpret_cast<u32*>(pPtr));
		}
		else
		{
			ret.Set(*reinterpret_cast<float*>(&pPtr[0]), *reinterpret_cast<float*>(&pPtr[4]), *reinterpret_cast<float*>(&pPtr[8]));
		}

		return ret;
	}

	Vector3 grcBufferEditor::GetPositionDynamic(int vertIndex)
	{
		Assert(IsDynamic());
		Assert(GetFvf()->IsChannelActive(grcFvf::grcfcPosition));
		Assert(vertIndex < GetVertexCount());
#if !__SPU
		Assert(isLocked());
#endif	//!__SPU


		u8* pPtr = static_cast<u8*>(GetLockPtr());
		pPtr += GetVertexStride() * vertIndex;

		return *reinterpret_cast<Vector3*>(pPtr);
	}

	Vector3 grcBufferEditor::GetNormal(int vertIndex)
	{
#if __VBSTATS
		PF_FUNC(GetNormal);
#endif
		const grcFvf* fvf = GetFvf();
		Assert( fvf->GetNormalChannel() );
		Assert(vertIndex < GetVertexCount());

		Vector3 ret;
#if !__SPU
		Assert(isLocked());
#endif	//!__SPU


		u8* pPtr = static_cast<u8*>(GetLockPtr());
		pPtr += GetVertexStride() * vertIndex;
		pPtr += fvf->GetOffset(grcFvf::grcfcNormal);
		if( fvf->GetDataSizeType(grcFvf::grcfcNormal) == grcFvf::grcdsPackedNormal )
			UnpackNormal(ret,*reinterpret_cast<u32*>(pPtr));
		else
			ret = *reinterpret_cast<Vector3*>(pPtr);

		return ret;
	}

	Vector2 grcBufferEditor::GetUV(int vertIndex, int uvSet)
	{
#if __VBSTATS
		PF_FUNC(GetUV);
#endif
		Vector2 ret(0.0f, 0.0f);

		const grcFvf* fvf = GetFvf();
		Assert(fvf->GetTextureChannel(uvSet));
		Assert(vertIndex < GetVertexCount());

		if (fvf->GetTextureChannel(uvSet) && (vertIndex < GetVertexCount()))
		{
#if !__SPU
			Assert(isLocked());
#endif	//!__SPU


			u8* pPtr = static_cast<u8*>(GetLockPtr());
			pPtr += GetVertexStride() * vertIndex;
			pPtr += fvf->GetOffset(static_cast<grcFvf::grcFvfChannels>(grcFvf::grcfcTexture0 + uvSet));

			if (fvf->GetDataSizeType(grcFvf::grcfcTexture0) == grcFvf::grcdsHalf2) 
			{
				// TODO -- Float16 optimise
				Float16 *h = reinterpret_cast<Float16*>(pPtr);
				ret.x = h[0].GetFloat32_FromFloat16();
				ret.y = h[1].GetFloat32_FromFloat16();
			}
			else
			{
				ret = *reinterpret_cast<Vector2*>(pPtr);
			}
			ret.y = 1 - ret.y;
		}

		return ret;
	}

	Vector3 grcBufferEditor::GetUVDynamic(int vertIndex, int uvSet)
	{
		Assert(!__WIN32PC == IsDynamic());
		const grcFvf* fvf = GetFvf();
		Assert(fvf->GetTextureChannel(uvSet));
		Assert(vertIndex < GetVertexCount());
		static const Vector3 vFlipV(0.0f, 1.0f, 0.0f);

		Vector3 vUV(Vector3::ZeroType);

		if (fvf->GetTextureChannel(uvSet) && (vertIndex < GetVertexCount()))
		{
#if !__SPU
			Assert(isLocked());
#endif	//!__SPU


			u8* pPtr = static_cast<u8*>(GetLockPtr());
			pPtr += GetVertexStride() * vertIndex;
			pPtr += fvf->GetOffset(static_cast<grcFvf::grcFvfChannels>(grcFvf::grcfcTexture0 + uvSet));

			vUV = vFlipV - *reinterpret_cast<Vector3*>(pPtr);
		}

		return vUV;
	}

	Color32 grcBufferEditor::GetDiffuse(int vertIndex)
	{
#if __VBSTATS
		PF_FUNC(GetCPV);
#endif
		const grcFvf* fvf = GetFvf();
		Assert( fvf->GetDiffuseChannel() );
		Assert(vertIndex < GetVertexCount());

		Color32 ret;
#if !__SPU
		Assert(isLocked());
#endif	//!__SPU

		u8* pPtr = static_cast<u8*>(GetLockPtr());
		pPtr += GetVertexStride() * vertIndex;
		pPtr += fvf->GetOffset(grcFvf::grcfcDiffuse);
#if HACK_GTA4
		ret.SetFromDeviceColor( ((Color32*)pPtr)->GetColor() );
#else // HACK_GTA4
		ret = *reinterpret_cast<Color32*>(pPtr);
#endif // HACK_GTA4

		return ret;
	}

	void grcBufferEditor::SetSpecular(int vertIndex, const Color32& color)
	{
#if __VBSTATS
		PF_FUNC(SetSpecular);
#endif
		const grcFvf* fvf = GetFvf();
		if( fvf->GetSpecularChannel() )
		{
			Assert(vertIndex < GetVertexCount());

			Assert(!IsReadOnly());
#if !__SPU
			Assert(isLocked());
#endif	//!__SPU

			u8* pPtr = static_cast<u8*>(GetLockPtr());

			pPtr += GetVertexStride() * vertIndex;
			pPtr += fvf->GetOffset(grcFvf::grcfcSpecular);
			*(Color32*)pPtr = Color32( color.GetDeviceColor() );
		}
	}

	Color32 grcBufferEditor::GetSpecular(int vertIndex)
	{
#if __VBSTATS
		PF_FUNC(GetSpecular);
#endif
		const grcFvf* fvf = GetFvf();
		Assert( fvf->GetSpecularChannel() );
		Assert(vertIndex < GetVertexCount());

		Color32 ret;
#if !__SPU
		Assert(isLocked());
#endif	//!__SPU

		u8* pPtr = static_cast<u8*>(GetLockPtr());

		pPtr += GetVertexStride() * vertIndex;
		pPtr += fvf->GetOffset(grcFvf::grcfcSpecular);
#if HACK_GTA4
		ret.SetFromDeviceColor( ((Color32*)pPtr)->GetColor() );
#else // HACK_GTA4
		ret = *reinterpret_cast<Color32*>(pPtr);
#endif // HACK_GTA4

		return ret;
	}

	Vector4 grcBufferEditor::GetTangent(int vertIndex, int uvSet)
	{
#if __VBSTATS
		PF_FUNC(GetTangent);
#endif
		const grcFvf* fvf = GetFvf();
		Assert( fvf->GetTangentChannel(uvSet) );
		Assert(vertIndex < GetVertexCount());

		Vector4 ret;
#if !__SPU
		Assert(isLocked());
#endif	//!__SPU

		u8* pPtr = static_cast<u8*>(GetLockPtr());

		pPtr += GetVertexStride() * vertIndex;
		pPtr += fvf->GetOffset(static_cast<grcFvf::grcFvfChannels>(grcFvf::grcfcTangent0 + uvSet));
		if( (fvf->GetDataSizeType(grcFvf::grcfcTangent0) != grcFvf::grcdsFloat4) ) 
		{
			if (fvf->GetDataSizeType(static_cast<grcFvf::grcFvfChannels>(grcFvf::grcfcTangent0+uvSet)) == grcFvf::grcdsPackedNormal) 
#if __PS3 && HACK_GTA4
				UnpackNormal_10_10_10_2(ret,*reinterpret_cast<u32*>(pPtr));
#else
				ret.Unpack1010102(*reinterpret_cast<u32*>(pPtr));
#endif
			else {
				Assert(fvf->GetDataSizeType(static_cast<grcFvf::grcFvfChannels>(grcFvf::grcfcTangent0+uvSet)) == grcFvf::grcdsHalf4);
				Float16 *h = reinterpret_cast<Float16*>(pPtr);

				// TODO -- Float16 optimise
				ret.x = h[0].GetFloat32_FromFloat16();
				ret.y = h[1].GetFloat32_FromFloat16();
				ret.z = h[2].GetFloat32_FromFloat16();
				ret.w = h[3].GetFloat32_FromFloat16();
			}
		}
		else
			ret = *reinterpret_cast<Vector4*>(pPtr);

		return ret;
	}

	Vector3 grcBufferEditor::GetBinormal(int vertIndex, int uvSet)
	{
#if __VBSTATS
		PF_FUNC(GetBinormal);
#endif
		const grcFvf* fvf = GetFvf();
		Assert( fvf->GetBinormalChannel(uvSet) );
		Assert(vertIndex < GetVertexCount());

		Vector3 ret;
#if !__SPU
		Assert(isLocked());
#endif	//!__SPU

		u8* pPtr = static_cast<u8*>(GetLockPtr());

		pPtr += GetVertexStride() * vertIndex;
		pPtr += fvf->GetOffset(static_cast<grcFvf::grcFvfChannels>(grcFvf::grcfcBinormal0 + uvSet));
		ret = *reinterpret_cast<Vector3*>(pPtr);

		return ret;
	}

	Vector4 grcBufferEditor::GetTangentDynamic(int vertIndex, int uvSet)
	{
		Assert(!__WIN32PC == IsDynamic());
		const grcFvf* fvf = GetFvf();
		Assert( fvf->GetTangentChannel(uvSet) );
		Assert(vertIndex < GetVertexCount());
#if !__SPU
		Assert(isLocked());
#endif	//!__SPU

		u8* pPtr = static_cast<u8*>(GetLockPtr());

		pPtr += GetVertexStride() * vertIndex;
		pPtr += fvf->GetOffset(static_cast<grcFvf::grcFvfChannels>(grcFvf::grcfcTangent0 + uvSet));

		return *reinterpret_cast<Vector4*>(pPtr);
	}

	Color32 grcBufferEditor::GetBlendIndices(int vertIndex)
	{
		const grcFvf* fvf = GetFvf();
		Assert( fvf->GetBindingsChannel() );
		Assert(vertIndex < GetVertexCount());

		Color32 ret;
#if !__SPU
		Assert(isLocked());
#endif	//!__SPU

		u8* pPtr = static_cast<u8*>(GetLockPtr());

		pPtr += GetVertexStride() * vertIndex;
		pPtr += fvf->GetOffset(grcFvf::grcfcBinding);
		ret = *reinterpret_cast<Color32*>(pPtr);

		return ret;
	}

	Vector4 grcBufferEditor::GetBlendWeights(int vertIndex)
	{
		const grcFvf* fvf = GetFvf();
		Assert( fvf->GetBlendWeightChannel() );
		Assert(vertIndex < GetVertexCount());

		Vector4 ret;
#if !__SPU
		Assert(isLocked());
#endif	//!__SPU

		u8* pPtr = static_cast<u8*>(GetLockPtr());

		pPtr += GetVertexStride() * vertIndex;
		pPtr += fvf->GetOffset(grcFvf::grcfcWeight);
		if( IsDynamic() )
		{
			ret = *reinterpret_cast<Vector4*>(pPtr);
		}
		else
		{
			Color32* pColor = (Color32*)pPtr;
			ret.Set(pColor->GetRed(), pColor->GetGreen(), pColor->GetBlue(), pColor->GetAlpha());
			ret.Scale(0.00392156862f);			// 1 / 255
		}

		return ret;
	}

	Vector4 grcBufferEditor::GetBlendWeightsDynamic(int vertIndex)
	{
		Assert(!__WIN32PC == IsDynamic());
		const grcFvf* fvf = GetFvf();
		Assert( fvf->GetBlendWeightChannel() );
		Assert(vertIndex < GetVertexCount());
#if !__SPU
		Assert(isLocked());
#endif	//!__SPU

		u8* pPtr = static_cast<u8*>(GetLockPtr());

		pPtr += GetVertexStride() * vertIndex;
		pPtr += fvf->GetOffset(grcFvf::grcfcWeight);

		return *reinterpret_cast<Vector4*>(pPtr);
	}

	void grcVertexBufferEditor::Set(const rage::grcVertexBuffer* pSource)
	{
		Assert(GetVertexCount() == pSource->GetVertexCount());
		Assert(GetVertexStride() == pSource->GetVertexStride());

#if !__SPU
		grcVertexBuffer::LockHelper helper1(m_VertexBuffer);
		grcVertexBuffer::LockHelper helper2(pSource, true);
		const void *const pSrc = helper2.GetLockPtr();
#else
		const void *const pSrc = pSource->GetFastLockPtr();
#endif

		WriteVertexData(pSrc, GetVertexStride() * GetVertexCount());
	}

	grcSystemBufferEditor::grcSystemBufferEditor(void *pvBuffer, u32 uByteSize, const grcFvf* poFVF) 
		: m_Buffer(pvBuffer), m_ByteSize(uByteSize), m_Fvf(*poFVF)
	{ 
		Assert(pvBuffer != NULL);
		Assert(poFVF != NULL);
		m_Stride = poFVF->GetTotalSize();
	}

	void grcSystemBufferEditor::Set(const void* pvSource)
	{
		Warningf("Probably don't want to support this, this way.");

		WriteVertexData(pvSource, GetVertexStride() * GetVertexCount());
	}

	// set the range of the short UV coords (they will need to be rescaled in the shaders)
	void grcBufferEditor::SetShortUVRange(int uvSet, float range)
	{
		if (AssertVerify(uvSet<kMaxTexCoordSets))
			sm_ShortTexCoordRange[uvSet] = range;
	}


} // namespace rage
