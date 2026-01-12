// 
// phcore/materialmgrimpl.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 
#ifndef PHCORE_MATERIALMGRIMPL_H
#define PHCORE_MATERIALMGRIMPL_H

#include "materialmgr.h"

#if __PS3
#include "phbullet/CollisionWorkUnit.h"
#endif

namespace rage {

/* 
  PURPOSE
    Handle the ownership and assignment of materials
			
  NOTES
    This class is referred to when it is instantiated, and after that
    all interaction occurs through base the interface class, phMaterialMgr.
	Instantiation occurs through the CreateAndLoad function, which can only
	be called again if Destroy is called in between.
*/
template <class _Material, typename _Base = phMaterialMgr>
class phMaterialMgrImpl : public _Base
{
public:
	// PURPOSE
	//   Initialize a material manager and load the materials.
	// NOTES
	//   The current asset path is searched for materials. If a materials.list file is
	//   found at that path, it will be used as a list of material files to parse. If no
	//   file with that name is found, the directory will be searched for any files with
	//   the .mtl extension.
	//
	//   The asset path current when this function is called will also be used to save
	//   and load materials through widgets.
	static phMaterialMgrImpl* Create();
	virtual void Load(int reservedSlots = 0);
	
	virtual void Destroy();

	virtual const phMaterial& FindMaterial(const char* name) const;
	virtual phMaterialMgr::Id GetMaterialId(const phMaterial& material) const;

	virtual const phMaterial& GetDefaultMaterial() const;

	phMaterial& AllocateMaterial(const char* name);

#if __PS3
	virtual void FillInSpuWorkUnit(PairListWorkUnitInput& wuInput);
#endif

protected:
	virtual phMaterialMgr::Id FindMaterialIdImpl(const char* name) const;

private:
	phMaterialMgrImpl();
	phMaterialMgrImpl(datResource& rsc);
	virtual ~phMaterialMgrImpl();

	static void FixupMapKey(datResource& rsc, ConstString& key);
	static void FixupMapMaterial(datResource& rsc, _Material*& material);
	void LoadNewMaterial(fiStream* stream, const char* name);
	void findUniqueMaterialList(fiStream* stream, atArray<ConstString>* outUniqueMaterials);

#if __RESOURCECOMPILER
	u8 m_Pad[8];
#endif

	_Material*	m_DefaultMaterial;

	typedef atMap<ConstString, _Material*> MapType;	// TODO: Consider a u32 hash value
	MapType		m_Map;

	typedef atArray<_Material> ArrayType;
	ArrayType	m_Array;
};

template <class _Material, typename _Base>
phMaterialMgrImpl<_Material, _Base>* phMaterialMgrImpl<_Material, _Base>::Create()
{
	phMaterialMgrImpl* instance = rage_new phMaterialMgrImpl;

	_Base::SetInstance(instance);

	return instance;
}

template <class _Material, typename _Base>
void phMaterialMgrImpl<_Material, _Base>::Load(int reservedSlots)
{
	char fullPath[RAGE_MAX_PATH];
	ASSET.FullPath(fullPath, RAGE_MAX_PATH, "", "");

	// if FullPath gave us a string ending in a dot, chop it off
	int length = (int) strlen(fullPath);
	if (length > 0 && fullPath[length - 1] == '.')
	{
		fullPath[length - 1] = '\0';
	}

	this->phMaterialMgr::m_AssetFolder = fullPath;

	fiStream* stream = ASSET.Open("materials", "list", true);

	if (stream)
	{
		atArray<ConstString> uniqueMaterials;
		findUniqueMaterialList(stream, &uniqueMaterials);
		stream->Close();

		int numUniqueMaterials = uniqueMaterials.GetCount();
		reservedSlots = Max(reservedSlots,numUniqueMaterials);

		// Now, make room for the materials in the containers
		FastAssert(reservedSlots < 255);
		m_Map.Recompute(atHashNextSize(static_cast<u16>(reservedSlots)));
		m_Array.Reserve(reservedSlots + 1);
		m_DefaultMaterial = &static_cast<_Material&>(AllocateMaterial("default"));

		_Base::BeginLoad(numUniqueMaterials + 1);

		for(int i = 0; i < numUniqueMaterials; i++)
		{
			const char* materialName = uniqueMaterials[i].c_str();
			if (fiStream* materialStream = ASSET.Open(materialName, "mtl"))
			{
				LoadNewMaterial(materialStream, materialName);
				materialStream->Close();
			}
			else
			{
				Errorf("Material file %s.mtl in the unique materials list but not found in the directory.", materialName);
			}
		}
	}
	else
	{
		const fiDevice* device = fiDevice::GetDevice(fullPath, true);

		fiFindData findData;
		fiHandle findHandle = device? device->FindFileBegin(fullPath, findData) : fiHandleInvalid;

		if (fiIsValidHandle(findHandle))
		{
			do
			{
				// Don't consider non-.mtl files
				char* ext = strrchr(findData.m_Name, '.');
				if (ext && stricmp(ext + 1, "mtl")==0)
				{
					++reservedSlots;
					FastAssert(reservedSlots < 0xffff);
				}
			}
			while (device->FindFileNext(findHandle, findData));

			device->FindFileEnd(findHandle);

			// Now, make room for the materials in the containers
			FastAssert(reservedSlots < 0xffff);
			m_Map.Recompute(atHashNextSize(static_cast<u16>(reservedSlots)));
			m_Array.Reserve(reservedSlots + 1);
			m_DefaultMaterial = &static_cast<_Material&>(AllocateMaterial("default"));

			_Base::BeginLoad(reservedSlots + 1);

			// Rewind the search and actually read the materials as you go
			fiHandle findHandle = device->FindFileBegin(fullPath, findData);
			FastAssert(fiIsValidHandle(findHandle));

			do
			{
				if (fiStream* materialStream = ASSET.Open(findData.m_Name, "mtl", true))
				{
					// Remove .mtl from the file name to make the material name
					char materialName[RAGE_MAX_PATH];
					int length = (int) strlen(findData.m_Name);
					FastAssert(length > 4 && length < RAGE_MAX_PATH);
					strncpy(materialName, findData.m_Name, length - 4);
					materialName[length - 4] = '\0';

					LoadNewMaterial(materialStream, materialName);

					materialStream->Close();
				}
			}
			while (device->FindFileNext(findHandle, findData));

			device->FindFileEnd(findHandle);
		}
		else
		{
			// We didn't find any materials to load, but reserve the extra slots anyway
			FastAssert(reservedSlots < 0xffff);
			m_Map.Recompute(atHashNextSize(static_cast<u16>(reservedSlots)));
			m_Array.Reserve(reservedSlots + 1);
			m_DefaultMaterial = &static_cast<_Material&>(AllocateMaterial("default"));
		}
	}

	_Base::SetMaterialInformation(m_Array.GetElements(), _Base::m_MaterialIndexMask);
}

template <class _Material, typename _Base>
void phMaterialMgrImpl<_Material, _Base>::findUniqueMaterialList(fiStream* stream, atArray<ConstString>* outUniqueMaterials)
{
	fiAsciiTokenizer token;
	token.Init("materials", stream);

	// First, count the materials you find
	char materialName[phMaterial::MAX_NAME_LENGTH];
	while (token.GetToken(materialName, phMaterial::MAX_NAME_LENGTH))
	{
		if (!stricmp(materialName,"#include"))
		{
			//special identifier we support including other files to make up the main list.
			token.GetToken(materialName, phMaterial::MAX_NAME_LENGTH);
			fiStream* stream2 = ASSET.Open(materialName, "", true);
			if (stream2)
			{
				findUniqueMaterialList(stream2, outUniqueMaterials);
				stream2->Close();
			}
			else
			{
				Errorf("Material list [%s] was not able to be opened the materials in it were not loaded.", materialName);
			}
		}
		else
		{
			//make sure it is unique
			bool add = true;
			for(int mat = 0; mat < outUniqueMaterials->GetCount(); mat++)
			{
				if ((*outUniqueMaterials)[mat] == materialName)
				{
					add = false;
					break;
				}
			}

			if (add)
			{
				outUniqueMaterials->PushAndGrow(ConstString(materialName));
			}
		}
	}
}

template <class _Material, typename _Base>
void phMaterialMgrImpl<_Material, _Base>::LoadNewMaterial(fiStream* stream, const char* name)
{
	fiAsciiTokenizer token;
	token.Init(name, stream);

	if (stricmp(name, "default") == 0)
		_Base::LoadMaterial(*m_DefaultMaterial, token);
	else
		_Base::LoadMaterial(AllocateMaterial(name), token);
}

// <COMBINE phMaterialMgr::Destroy>
template <class _Material, typename _Base>
void phMaterialMgrImpl<_Material, _Base>::Destroy()
{
	delete this; // Remember, you can't access members after this point!

	phMaterialMgr::SetInstance(NULL);
}

// PURPOSE: Used during resource construction of the atMap
template <class _Material, typename _Base>
inline void phMaterialMgrImpl<_Material, _Base>::FixupMapKey(datResource& rsc, ConstString& key)
{
	::new(&key) ConstString(rsc);
}

// PURPOSE: Used during resource construction of the atMap
template <class _Material, typename _Base>
inline void phMaterialMgrImpl<_Material, _Base>::FixupMapMaterial(datResource& rsc, _Material*& material)
{
	rsc.PointerFixup(material);
}

// <COMBINE phMaterialMgr::AllocateMaterial>
template <class _Material, typename _Base>
phMaterial& phMaterialMgrImpl<_Material, _Base>::AllocateMaterial(const char* name)
{
	// The material itself is stored in the array
#if __TOOL
	// JWR - the runtime rely on a cached pointer to the start of the m_Array container for an optimisation.
	bool reallocationRequired = (m_Array.GetCount() == m_Array.GetCapacity());

	char* oldAddress = m_Array.GetCount() ? (char*)(&m_Array[0]) : 0;
	_Material& material = m_Array.Grow();
	if (ptrdiff_t diff = (char*)(&m_Array[0]) - oldAddress)
		for (MapType::Iterator i = m_Map.CreateIterator(); !i.AtEnd(); i.Next())
			(char*&)(i.GetData()) += diff;

	if(reallocationRequired)
	{
		// JWR - we need to update that pointer when the container grows.
		SetMaterialInformation(m_Array.GetElements(), _Base::m_MaterialIndexMask);	
	}
#else
	_Material& material = m_Array.Append();
#endif

	_Base::m_NumMaterials = m_Array.GetCount();

	// The map element is added for quick searching
	char normalized[phMaterial::MAX_NAME_LENGTH];
	StringNormalize(normalized, name, sizeof(normalized));

	// The name stored in the material is actually owned by the map
	material.SetName(m_Map.Insert(ConstString(normalized), &material).key.m_String);

	return material;
}

#if __PS3
template <class _Material, typename _Base>
void phMaterialMgrImpl<_Material, _Base>::FillInSpuWorkUnit(PairListWorkUnitInput& wuInput)
{
	wuInput.m_materialArray = m_Array.GetElements();
	wuInput.m_numMaterials = m_Array.GetCount();
	wuInput.m_materialStride = u32(sizeof(_Material));
	wuInput.m_materialMask = 0xffffffff;

	_Base::FillInSpuWorkUnit(wuInput);
}
#endif

template <class _Material, typename _Base>
phMaterialMgrImpl<_Material, _Base>::phMaterialMgrImpl()
{
}

template <class _Material, typename _Base>
phMaterialMgrImpl<_Material, _Base>::phMaterialMgrImpl(datResource& rsc)
{
	// Do we really want to placement new the map twice here?  The atArray ctor looks wrong.
	::new (&m_Map) MapType(rsc, FixupMapKey, FixupMapMaterial);
	::new (&m_Map) atArray<_Material, __alignof(_Material)>(rsc);
}

// <COMBINE phMaterialMgr::phMaterialMgrImpl>
template <class _Material, typename _Base>
phMaterialMgrImpl<_Material, _Base>::~phMaterialMgrImpl()
{
	// Both the containers are automatically Killed in their destructors
}


// <COMBINE phMaterialMgr::FindMaterial>
template <class _Material, typename _Base>
const phMaterial& phMaterialMgrImpl<_Material, _Base>::FindMaterial(const char* name) const
{
	char normalized[phMaterial::MAX_NAME_LENGTH];
	StringNormalize(normalized, name, sizeof(normalized));

	if (strcmp(normalized, m_DefaultMaterial->GetName()) == 0)
	{
		return *m_DefaultMaterial;
	}

	_Material* const* materialPointer = m_Map.Access(normalized);

	if (materialPointer)
	{
		FastAssert(*materialPointer);
		return **materialPointer;
	}
	else
	{
#if __TOOL
		return const_cast<phMaterialMgrImpl*>(this)->AllocateMaterial(name);
#else
		return *m_DefaultMaterial;
#endif
	}
}


// <COMBINE phMaterialMgr::FindMaterialId>
template <class _Material, typename _Base>
phMaterialMgr::Id phMaterialMgrImpl<_Material, _Base>::FindMaterialIdImpl(const char* name) const
{
	char normalized[phMaterial::MAX_NAME_LENGTH];
	StringNormalize(normalized, name, sizeof(normalized));

	if (strcmp(normalized, m_DefaultMaterial->GetName()) == 0)
	{
		return phMaterialMgr::DEFAULT_MATERIAL_ID;
	}

	_Material* const* material = m_Map.Access(normalized);

	if (material)
	{
		FastAssert(*material);
		return GetMaterialId(**material);
	}
	else
	{
#if __TOOL
		phMaterial& newMaterial = const_cast<phMaterialMgrImpl*>(this)->AllocateMaterial(normalized);

		return GetMaterialId(newMaterial);
#else
		return phMaterialMgr::MATERIAL_NOT_FOUND;
#endif
	}
}

// <COMBINE phMaterialMgr::GetMaterialId>
template <class _Material, typename _Base>
phMaterialMgr::Id phMaterialMgrImpl<_Material, _Base>::GetMaterialId(const phMaterial& material) const
{
	if (&material == m_DefaultMaterial || m_Array.GetCount() == 1)
	{
		return phMaterialMgr::DEFAULT_MATERIAL_ID;
	}

	size_t id = &static_cast<const _Material&>(material) - &m_Array[0];

	if (id < (size_t)m_Array.GetCount())
	{
		return (phMaterialMgr::Id) (id);
	}
	else
	{
		return phMaterialMgr::MATERIAL_NOT_FOUND;
	}
}

// <COMBINE phMaterialMgr::GetDefaultMaterial>
template <class _Material, typename _Base>
const phMaterial& phMaterialMgrImpl<_Material, _Base>::GetDefaultMaterial() const
{
	return *m_DefaultMaterial;
}

} // namespace rage

#endif // PHCORE_MATERIALMGRIMPL_H
