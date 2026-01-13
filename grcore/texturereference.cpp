// 
// grcore/texturereference.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 
#include "texturereference.h"
#include "string/string.h"
#include "data/resource.h"
#include "paging/dictionary.h"

#if __WIN32PC && !__FINAL
#include "file/asset.h"
#include "system/magicnumber.h"
#include "dds.h"
#endif

using namespace rage;

//////// grcTextureReferenceBase /////////

bool grcTextureReferenceBase::Copy(const grcImage *pImage) {
	grcTexture *r = GetReference();
	return r? r->Copy(pImage) : false;
}

bool grcTextureReferenceBase::IsSRGB() const {
	const grcTexture *r = GetReference();
	return r? r->IsSRGB() : false;
}

bool grcTextureReferenceBase::LockRect(int layer, int mipLevel,grcTextureLock &lock, u32 uLockFlags) const {
	const grcTexture *r = GetReference();
	return r? r->LockRect(layer,mipLevel,lock,uLockFlags) : false;
}

void grcTextureReferenceBase::UnlockRect(const grcTextureLock &lock) const {
	const grcTexture *r = GetReference();
	if (r)
		r->UnlockRect(lock);
}

const grcTextureObject* grcTextureReferenceBase::GetTexturePtr() const {
	const grcTexture *r = GetReference();
	if (r)
		return r->GetTexturePtr();
	else
		return grcTexture::None->GetTexturePtr();
}

grcTextureObject* grcTextureReferenceBase::GetTexturePtr() {
	grcTexture *r = GetReference();
	if (r)
		return r->GetTexturePtr();
	else
#if __RESOURCECOMPILER
		return NULL;
#else
		return const_cast<grcTexture*>(grcTexture::None)->GetTexturePtr();
#endif
}

grcTextureReferenceBase::~grcTextureReferenceBase() {
}


grcTextureReference::~grcTextureReference() {
	if (m_Reference)
		m_Reference->Release();
}

const char *grcTextureReferenceBase::GetName() const {
	return m_Name;
}

int grcTextureReferenceBase::GetWidth() const {
	const grcTexture *r = GetReference();
	return r? r->GetWidth() : 0;
}

int grcTextureReferenceBase::GetDepth() const {
	const grcTexture *r = GetReference();
	return r? r->GetDepth() : 0;
}

int grcTextureReferenceBase::GetMipMapCount() const {
	const grcTexture *r = GetReference();
	return r? r->GetMipMapCount() : 0;
}

int grcTextureReferenceBase::GetArraySize() const {
	const grcTexture *r = GetReference();
	return r? r->GetArraySize() : 0;
}

int grcTextureReferenceBase::GetBitsPerPixel() const {
	const grcTexture *r = GetReference();
	return r? r->GetBitsPerPixel() : 0;
}

int grcTextureReferenceBase::GetHeight() const {
	const grcTexture *r = GetReference();
	return r? r->GetHeight() : 0;
}

void grcTextureReferenceBase::Download() const {
	const grcTexture *r = GetReference();
	if (r)
		r->Download();
}

int grcTextureReferenceBase::GetDownloadSize() const {
	const grcTexture *r = GetReference();
	return r? r->GetDownloadSize() : 0;
}

grcTextureReferenceBase::grcTextureReferenceBase(datResource &rsc) : grcTexture(rsc) {
	rsc.PointerFixup(m_Name);
}

grcTextureReferenceBase::grcTextureReferenceBase(const char *name,u8 type) : grcTexture(type) {
	m_Name = StringDuplicate(name);
}

bool grcTextureReferenceBase::IsValid() const
{
	const grcTexture *r = GetReference();
	if (r != NULL)
	{
		return r->IsValid();
	}
	return true;
}

#if __DECLARESTRUCT
void grcTextureReferenceBase::DeclareStruct(class datTypeStruct &s)
{
	grcTexture::DeclareStruct(s);
	STRUCT_BEGIN(grcTextureReferenceBase);
	STRUCT_END();
}
#endif

////////// grcTextureReference ///////////

grcTexture* grcTextureReference::GetReference() {
#if __WIN32PC
	Assert(m_Reference != (grcTexture*)0xCDCDCDCD);
	Assert((m_Reference == NULL) || m_Reference->IsValid());
#endif // __WIN32PC
	return (m_Reference) ? m_Reference->GetReference() : 0;
}

const grcTexture* grcTextureReference::GetReference() const {
#if __WIN32PC
	Assert(m_Reference != (grcTexture*)0xCDCDCDCD);
	Assert((m_Reference == NULL) || m_Reference->IsValid());
#endif // __WIN32PC
	return (m_Reference) ? m_Reference->GetReference() : 0;
}

void grcTextureReference::SetReference(grcTexture *ref) {
	Assertf(ref != this, "Texture Reference '%s' is trying to self reference", GetName());	

	if (ref)
		ref->AddRef();
	
	if (m_Reference)
		m_Reference->Release();
	
	m_Reference = ref;
	m_CachedTexturePtr = GetTexturePtr();
#if __WIN32PC
	Assert(ref != (grcTexture*)0xCDCDCDCD);
	Assert((ref == NULL) || ref->IsValid());
#endif // __WIN32PC
}

void grcTextureReference::SetEffectInfo(grcEffect* effect, grcEffectVar variable) const
{
#if __WIN32PC
	Assert(m_Reference != (grcTexture*)0xCDCDCDCD);
	Assert((m_Reference == NULL) || m_Reference->IsValid());
#endif // __WIN32PC
    if (m_Reference) 
        m_Reference->SetEffectInfo(effect,variable);
}

grcTextureReference::grcTextureReference(const char *name,grcTexture *ref) :  grcTextureReferenceBase(name,grcTexture::REFERENCE) {
	m_Reference = ref;
	if (ref)
		ref->AddRef();
	m_CachedTexturePtr = GetTexturePtr();
#if __WIN32PC
	Assert(m_Reference != (grcTexture*)0xCDCDCDCD);
	Assert((m_Reference == NULL) || m_Reference->IsValid());
#endif // __WIN32PC
}

grcTextureReference::grcTextureReference(datResource &rsc) : grcTextureReferenceBase(rsc) {
	if (!rsc.IsDefragmentation()) {
		Assert(!m_Reference);
		
		grcTexture* ref = NULL;
		const char* name = GetName();
		if (name)
		{
			// if the reference has a name then check to see if the name is registered
			ref = grcTextureFactory::GetInstance().LookupTextureReference(name);
		}

		// If it's a pre-registered reference, hook up to it right now
		if (ref) {
			// LookupTextureReference returns a new object so we don't need an AddRef here.
			m_Reference = ref;
		} 
	}

	m_CachedTexturePtr = GetTexturePtr();
}

#if 0

////////// grcTextureDictionaryReference ///////////////

grcTextureDictionaryReference::grcTextureDictionaryReference(const char *name,pgDictionary<grcTexture> *dict,grcTexture *ref) : grcTextureReference(name,ref) {
	(m_Dictionary = dict)->AddRef();
}


grcTextureDictionaryReference::~grcTextureDictionaryReference() {
	m_Dictionary->Release();
}

#endif