//
// grcore/texturereference.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef GRCORE_TEXTUREREFERENCE_H
#define GRCORE_TEXTUREREFERENCE_H

#include "paging/ref.h"
#include "grcore/texture.h"

namespace rage {

/*
	grcTextureReferenceBase defines the base class for all referencable
	textures; it leaves GetReference undefined for specialization by
	subclasses, but redefines all other member functions to properly
	forward the calls to the object returned by GetReference.
*/
class grcTextureReferenceBase: public grcTexture {
public:
	grcTextureReferenceBase(const char *name,u8 type);
	grcTextureReferenceBase(class datResource &rsc);
	~grcTextureReferenceBase();

	virtual grcTexture* GetReference() = 0;
	virtual const grcTexture* GetReference() const = 0;

	const char *GetName() const;
	int GetWidth() const;
	int GetHeight() const;
	int GetDepth() const;
	int GetMipMapCount() const;
	int GetArraySize() const;
	int GetBitsPerPixel() const;
	void Download() const;
	int GetDownloadSize() const;
	bool Copy(const grcImage*);
	bool IsSRGB() const;

	ChannelBits FindUsedChannels() const {ChannelBits bits(false); return bits;}

	virtual bool LockRect(int layer, int mipLevel,grcTextureLock &lock, u32 uLockFlags = (grcsRead | grcsWrite)) const;
	virtual void UnlockRect(const grcTextureLock &/*lock*/) const;

	const grcTextureObject* GetTexturePtr() const;
	grcTextureObject* GetTexturePtr();

	virtual bool IsValid() const;

#if __DECLARESTRUCT
	virtual void DeclareStruct(class datTypeStruct &s);
#endif
};


/*
	grcTextureReference is a special class which holds a reference to an externally
	defined texture.  This is the only way a texture in a resource can reference a
	texture defined outside that resource; higher level code will register a name
	and the address of the original texture.  Then the object within a resource will
	automatically get a reference to that original texture, by name, that can be
	safely resolved at a later time.
	<FLAG Component>
*/
class grcTextureReference: public grcTextureReferenceBase {
public:
	grcTextureReference(const char *name,grcTexture *ref);
	grcTextureReference(datResource&);
	~grcTextureReference();
	void SetReference(grcTexture *ref);
	grcTexture *GetReference();
	const grcTexture *GetReference() const;

    void SetEffectInfo(grcEffect*, grcEffectVar) const;

private:
	pgRef<grcTexture> m_Reference;
};

#if 0

template <class T_> class pgDictionary;

class grcTextureDictionaryReference: public grcTextureReference {
public:
	grcTextureDictionaryReference(const char *name,pgDictionary<grcTexture> *dict,grcTexture *ref);
	grcTextureDictionaryReference(datResource&);
	~grcTextureDictionaryReference();

private:
	pgDictionary<grcTexture> *m_Dictionary;
};

#endif

}	// namespace rage

#endif	// GRCORE_TEXTUREREFERENCE_H
