// 
// grcore/texture_gnm.h 
// 
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved. 
// 

#ifndef GRCORE_TEXTURE_GNM_H
#define GRCORE_TEXTURE_GNM_H

#include "grcore/config.h"

#if __GNM

#include "atl/bitset.h"
#include "texture.h"
#include "wrapper_gcm.h"
#include "device.h"
#include "orbisdurangoresourcebase.h"

#include <gnm/texture.h>

namespace sce { namespace Gnm { class Texture; class RenderTarget; class DepthRenderTarget; } }

namespace rage {

	class grcImage;
	class grcRenderTargetGNM;
	class datResource;

	struct grcTextureGNMFormats
	{
		static sce::Gnm::DataFormat grctf_to_Orbis[];
		static sce::Gnm::DataFormat grcImage_to_Orbis[];
	};

	class grcTextureGNM: public grcOrbisDurangoTextureBase {
	public:
		grcTextureGNM(const char *filename,grcTextureFactory::TextureCreateParams *params);
		grcTextureGNM(grcImage *image,grcTextureFactory::TextureCreateParams *params);
		grcTextureGNM(u32 width,u32 height,u32 format,void *pBuffer,grcTextureFactory::TextureCreateParams *params);
		grcTextureGNM(u32 width,u32 height,u32 depth,u32 format,void *pBuffer,grcTextureFactory::TextureCreateParams *params);
		grcTextureGNM(const sce::Gnm::RenderTarget &sceRenderTarget, const char type);
		grcTextureGNM(class datResource&);
		~grcTextureGNM();

		u32 GetLinesPerPitch() const;
		virtual bool IsGammaEnabled() const;
		virtual void SetGammaEnabled(bool enabled);
		virtual const grcTextureObject *GetTexturePtr() const { return &GetGnmTexture(); }
		virtual grcTextureObject *GetTexturePtr() { return &GetGnmTexture(); }
		virtual grcDevice::MSAAMode GetMSAA() const	{ return grcDevice::MSAAMode(GetGnmTexture()); }

		virtual u32 GetImageFormat() const; // cast this to grcImage::Format

		virtual bool LockRect(int /*layer*/, int /*mipLevel*/,grcTextureLock &/*lock*/, u32 uLockFlags = (grcsRead | grcsWrite)) const;
		virtual void UnlockRect(const grcTextureLock &/*lock*/) const;
		bool Copy(const grcImage *image);
		virtual bool Copy(const grcTexture *pTexture, s32 mipLevel = -1);

		//MIPS AND MULTIPLE LAYERS ARE CURRENTLY NOT SUPPORTED
		void Resize(u32 width, u32 height);

		inline u32 GetMemoryOffset(u32 layer, u32 mip) const
		{
			u64 offset, pitch;
			FastAssert(layer == 0);
			FastAssert(mip <= GetMipMapCount());

			if(mip < GetMipMapCount())
				GetLockInfo(mip, offset, pitch);
			else
			{
				GetLockInfo(GetMipMapCount()-1, offset, pitch);
				offset += pitch*GetRowCount(GetMipMapCount()-1);
			}
			return (u32)offset;
		}


		// RETURNS: a raw pointer to the pixel data
		void* GetBits();
		// Removes the top mip
		void RemoveTopMip();

		void TileInPlace(); //helper function for tiling textures that have untiled data written to them
	private:
		struct USER_MEMORY
		{
			sce::Gnm::Texture	m_GnmTexture;
			uint64_t			m_Unused[2];
		};
		CompileTimeAssert(sizeof(USER_MEMORY) <= sizeof(datPadding<GRC_ORBIS_DURANGO_TEXTURE_USER_MEM_SIZE, u64>));
		USER_MEMORY *GetUserMemory() { return (USER_MEMORY *)&m_UserMemory; }
		USER_MEMORY *GetUserMemory() const { return (USER_MEMORY *)&m_UserMemory; }
		sce::Gnm::Texture &GetGnmTexture() { return GetUserMemory()->m_GnmTexture; }
		sce::Gnm::Texture &GetGnmTexture() const { return GetUserMemory()->m_GnmTexture; }

		void RecalculatePitch();
		void Create(u32 width,u32 height,u32 depth, GRC_TEMP_XG_FORMAT format,grcTextureFactory::TextureCreateParams *params);
		sce::Gnm::SizeAlign InitGnmTexture();
		void ComputeMipOffsets();
		void CopyBits(int mip,const void *bits);
	};

}	// namespace rage

#endif		// __GNM

#endif
