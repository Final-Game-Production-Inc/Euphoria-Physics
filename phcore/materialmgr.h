// 
// phcore/materialmgr.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 
#ifndef PHCORE_MATERIALMGR_H
#define PHCORE_MATERIALMGR_H

#include "material.h"

#include "atl/array.h"
#include "atl/map.h"
#include "data/base.h"
#include "file/asset.h"
#include "file/device.h"
#include "file/token.h"
#include "phCore/constants.h"
#include "string/string.h"

struct PairListWorkUnitInput;

#define PHMATERIALMGR_MIN_ELASTICITY 0.0f
#define PHMATERIALMGR_MAX_ELASTICITY 0.9f

namespace rage {

class bkBank;
class fiAsciiTokenizer;
struct phMaterialPair;

/*
  PURPOSE
	The material manager keeps track of the list of phMaterial's in the physics system.
	Typically, all the materials that will be needed are loaded once, at boot time. Each one
	gets a name and an id. As the phBound's are loaded, they look up the materials by name,
	and then store an id to be used to get quick access to the material during collisions.

  NOTES
	One place in a game needs to instantiate the material manager, by including materialmgrimpl.h
	and calling phMaterialMgrImpl::CreateAndLoad. After that, all access to the material manager
	is done through the interface in this file.

	You are allowed to maintain multiple material managers, or to Delete the material manager and
	CreateAndLoad a new one, for instance if you want a different set of materials for each level.
	Beware, though, that when a phBound is loaded, it stores only the material ids for its
	materials. Therefore, any phBound that used materials whose ids were not the same, or not
	found, in the new material manager, will not act as expected. See phMaterialMgrImpl::CreateAndLoad
	for more information.

<FLAG Component>
*/
class phMaterialMgr : public datBase
{
public:
	// PURPOSE
	//   A unique value assigned to materials as they are loaded. At this time, Ids
	//   are re-used between material managers.
#if PH_MATERIAL_ID_64BIT	// EUGENE_APPROVED - just want to keep note of option to use 64 bits?
	typedef u64 Id;
#else
	typedef u32 Id;
#endif

	// PURPOSE: The Id assigned when no material was assigned to a bound by the artist.
	static const Id DEFAULT_MATERIAL_ID = 0;

	// PURPOSE: The Id assigned when the requested material was not found.
	static const Id MATERIAL_NOT_FOUND = Id(-1);

	virtual void Load(int reservedSlots = 0) = 0;

	phMaterialMgr();
	virtual ~phMaterialMgr();

	// PURPOSE: Returns true if the material manager has been instantiated.
	static bool IsInstanceValid();

	// PURPOSE: Returns the currently active material manager
	// NOTES: The active instance is the one used to load bounds and get material properties
	//        for collisions
	static phMaterialMgr& GetInstance();

	// PURPOSE: Changes the currently active material manager
	// NOTES: The active instance is the one used to load bounds and get material properties
	//        for collisions
	static void SetInstance(phMaterialMgr* manager);

	// PURPOSE: Destroys the currently active material manager
	// NOTES: After this, bounds cannot be loaded and the simulator cannot update until
	//        either SetInstance or phMaterialMgrImpl::CreateAndLoad has been called.
	virtual void Destroy() = 0;

	// PURPOSE: Returns the number of materials currently owned by the material manager
	int GetNumMaterials() const;

	// PURPOSE: Returns a reference to a material by searching for the name in a hash table
	virtual const phMaterial& FindMaterial(const char* name) const = 0;

	// PURPOSE: Returns a reference to a material by id
	// NOTE: This function relies on SetMaterialInformation being called
	const phMaterial& GetMaterial(Id id) const;

	// PURPOSE: Find the combined friction and elasticity of two objects
	// PARAMS:
	//  materialIdA - Material Id of the first object
	//  materialIdB - Material Id of the second object
	//  combinedFriction - result parameter that will be filled with the combined friction of the two objects
	//  combinedElasticity - result parameter that will be filled with the combined elasticity of the two objects
	void FindCombinedFrictionAndElasticity(Id materialIdA, Id materialIdB, float& combinedFriction, float& combinedElasticity);

	// PURPOSE: Combine two different frictions into one
	// PARAMS: 
	//  frictionA - friction of first object
	//  frictionB - friction of second object
	// RETURN:
	//  Combined friction of the two objects
	static float CombineFriction(float frictionA, float frictionB);

	// PURPOSE: Combine two different elasticities into one
	// PARAMS: 
	//  elasticityA - elasticity of first object
	//  elasticityB - elasticity of second object
	// RETURN:
	//  Combined elasticity of the two objects
	static float CombineElasticity(float elasticityA, float elasticityB);

	// PURPOSE: Returns the id corresponding to a material name, by searching in a hash table.
	// NOTES: If the material you asked for isn't found, MATERIAL_NOT_FOUND is returned
	//        instead.
	// NOTE: The default behavior only calls the derived classes FindMaterialId method,
	//		 however phMaterialMgrFlag additionally removes any flags from the name before
	//       calling FindMaterialId and adds the flags to the Id afterwards.
	virtual Id FindMaterialId(const char* name) const
	{
		return FindMaterialIdImpl(name);
	}

	// PURPOSE: Stores the material name into name
	// NOTE: The default behavior only uses the result of GetMaterial(id).GetName(),
	//		 however phMaterialMgrFlag additionally appends any flags to the name.
	virtual void GetMaterialName(Id id, char* name, int size) const;

	// PURPOSE: Returns the id corresponding to a material
	// NOTES: If the material isn't controlled by the material manager, MATERIAL_NOT_FOUND
	//        is returned instead.
	virtual Id GetMaterialId(const phMaterial& material) const = 0;

	// PURPOSE: Returns default material.
	// NOTES: The default material is used whenever something goes wrong with the material
	//        specification...either a material name was not found in a phBound source asset or a
	//        material id was out of bounds in a loaded or resourcified phBound.
	virtual const phMaterial& GetDefaultMaterial() const = 0;

	// PURPOSE: Returns a color for this material to be drawn with in profile drawing
	// NOTES: The color is randomly assigned to this material
	Color32 GetDebugColor(const phMaterial& material) const;

	// PURPOSE: Returns the phMaterialFlags flags for a given material.
	// NOTES: This is generally overridden by the MATERIALMGRFLAG to provide something other than zero flags.
	virtual phMaterialFlags GetFlags(Id /*materialId*/) const { return 0; }

	// PURPOSE: Returns a color for this material to be drawn with in profile drawing.
	// NOTES: This is generally overridden by the MATERIALMGRFLAG to provide something other than Color_red.
	virtual Color32 GetDebugColor(phMaterialMgr::Id id, phMaterialFlags polyFlags, phMaterialFlags highlightFlags) const;

	// PURPOSE: Allocate a material manually
	// NOTES: If some extra materials were reserved when phMaterialMgrImpl::CreateAndLoad was
	//        called, this allows you to grab one of them. This is normally the only way to
	//        modify a material, since all the other access methods return const phMaterials.
	virtual phMaterial& AllocateMaterial(const char* name) = 0;

#if __PS3
	virtual void FillInSpuWorkUnit(PairListWorkUnitInput& wuInput);
#endif

#if __BANK
	// PURPOSE: Create material widgets
	// NOTES: For development purposes, this adds widgets for inspecting and modifying materials,
	//        as well as saving and loading them.
	void AddWidgets(bkBank& bank);
#endif

protected:
	// PURPOSE: Set the derived material type
	// PARAMS:
	//   materials - pointer to contiguous array of derived materials
	//   mask - mask to apply to material Ids to get their array index
	template <typename MaterialType>
	void SetMaterialInformation(const MaterialType* materials, Id mask)
	{
		m_MaterialSize = static_cast<u32>(sizeof(MaterialType));
		m_MaterialIndexMask = mask;
		m_Materials = materials;
	}

	// PURPOSE: Returns the id corresponding to a material name, by searching in a hash table.
	// NOTES: If the material you asked for isn't found, MATERIAL_NOT_FOUND is returned
	//        instead.
	virtual Id FindMaterialIdImpl(const char* name) const = 0;

	virtual void BeginLoad(int UNUSED_PARAM(reservedSlots)) { }

	virtual void LoadMaterial (phMaterial& material, fiAsciiTokenizer& token);

	virtual void SaveMaterial (const phMaterial& material, fiAsciiTokenizer& token) const;

#if __BANK
	bool BankSaveMaterial (phMaterial* material);
	bool BankLoadMaterial (phMaterial* material);

	virtual void AddMaterialWidgets(phMaterial& material, bkBank& bank);
#endif

	ConstString m_AssetFolder;
	Id m_MaterialIndexMask;
	u32 m_NumMaterials;
	u32 m_MaterialSize;
	const void* m_Materials;

	atArray<phMaterialPair> m_MaterialOverridePairs;

private:
	void SaveAll() const;

	static phMaterialMgr* sm_Instance;
};

#define MATERIALMGR (::rage::phMaterialMgr::GetInstance())

inline int phMaterialMgr::GetNumMaterials() const
{
	return m_NumMaterials;
}

inline bool phMaterialMgr::IsInstanceValid()
{
	return sm_Instance != NULL;
}

inline phMaterialMgr& phMaterialMgr::GetInstance()
{
	return *sm_Instance;
}

inline void phMaterialMgr::SetInstance(phMaterialMgr* manager)
{
	sm_Instance = manager;
}

__forceinline const phMaterial& phMaterialMgr::GetMaterial(Id id) const
{
	Id materialIndex = m_MaterialIndexMask & id;
	if(!Verifyf(materialIndex < m_NumMaterials, "Material index: %" I64FMT "i out of range %i. Material Id:0x%" I64FMT "x, Mask: 0x%" I64FMT "x", materialIndex, m_NumMaterials, id, m_MaterialIndexMask))
		materialIndex = 0;
	return reinterpret_cast<const phMaterial&>(reinterpret_cast<const u8*>(m_Materials)[m_MaterialSize*materialIndex]);
}

__forceinline float phMaterialMgr::CombineFriction(float frictionA, float frictionB)
{
	return frictionA*frictionB;
}
__forceinline float phMaterialMgr::CombineElasticity(float elasticityA, float elasticityB)
{
	return Clamp(elasticityA*elasticityB, PHMATERIALMGR_MIN_ELASTICITY, PHMATERIALMGR_MAX_ELASTICITY);
}

struct phMaterialPair
{
	phMaterialMgr::Id m_MaterialIndexA;
	phMaterialMgr::Id m_MaterialIndexB;
	float m_CombinedFriction;
	float m_CombinedElasticity;
};

} // namespace rage

#endif // PHCORE_MATERIALMGR_H
