// 
// phcore/materialmgrflag.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 
#ifndef PHCORE_MATERIALMGRFLAG_H
#define PHCORE_MATERIALMGRFLAG_H

#include "materialmgr.h"

#include "atl/array.h"
#include "atl/map.h"
#include "data/base.h"
#include "file/asset.h"
#include "file/device.h"
#include "file/token.h"
#include "string/string.h"
#if HACK_GTA4
#include <limits>
#endif

namespace rage {

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
class phMaterialMgrFlag : public phMaterialMgr
{
public:
	// PURPOSE: Returns the id corresponding to a material name, by searching in a hash table.
	// NOTES: If the material you asked for isn't found, MATERIAL_NOT_FOUND is returned
	//        instead.
	// NOTE: The base class behavior only calls the derived classes FindMaterialId method,
	//		 however phMaterialMgrFlag additionally removes any flags from the name before
	//       calling FindMaterialId and adds the flags to the Id afterwards.
	virtual Id FindMaterialId(const char* name) const;

	// PURPOSE: Stores the material name into name
	// NOTE: The base class behavior only uses the result of GetMaterial(id).GetName(),
	//		 however phMaterialMgrFlag additionally appends any flags to the name.
	virtual void GetMaterialName(Id id, char* name, int size) const;

	/*
	PURPOSE:
		Material flags are stored by name in the bound and material files, and before
		loading a bound or material file the names of the application-defined flags must
		be registered with this function.
	*/
	void RegisterFlag(const char* name, phMaterialFlags flag);

	/*
	PURPOSE:
		Looks up the flag value by name.
	RETURNS:
		The flag value or 0 if the flag name was not registered with RegisterFlag().
	*/
	phMaterialFlags GetFlagByName(const char* name) const;

	// PURPOSE: Extracts the material array indx from the phMaterial ID
	Id GetBaseMaterialId(Id id) const
	{
		return (id & ((1 << m_BitsForIndex) - 1));
	}

	// PURPOSE: Extracts the phMaterialFlags from the phMaterial ID
	virtual phMaterialFlags GetFlags(Id id) const
	{
		return (id >> m_BitsForIndex);
	}

	// PURPOSE: Sets the phMaterialFlags in the phMaterial ID
	Id SetFlags(Id id, phMaterialFlags flags) const
	{
		return ((id & ((1 << m_BitsForIndex) - 1)) | (flags << m_BitsForIndex));
	}

	// PURPOSE: Adds some phMaterialFlags to the phMaterial ID
	Id AddFlags(Id id, phMaterialFlags flags) const
	{
		return (id | (flags << m_BitsForIndex));
	}

	// PURPOSE: Clears some phMaterialFlags from the phMaterial ID
	Id ClearFlags(Id id, phMaterialFlags flags) const
	{
		return (id & ~(flags << m_BitsForIndex));
	}

#if !__SPU
	// PURPOSE: Sets the default flags for the material in the phMaterial ID
	Id SetDefaultFlags(Id id) const
	{
		const phMaterialMgr::Id iBaseMaterialId=GetBaseMaterialId(id);
		Assert(iBaseMaterialId<(phMaterialMgr::Id)m_DefaultFlags.size());
		// Assert(iBaseMaterialId < (phMaterialMgr::Id)std::numeric_limits<int>::max());
		const int iBaseMaterialId32=static_cast<int>(iBaseMaterialId);
		return (id == DEFAULT_MATERIAL_ID) ? DEFAULT_MATERIAL_ID : SetFlags(id, m_DefaultFlags[iBaseMaterialId32]);
	}
#endif

	// PURPOSE: Returns an iterator over the flag name map list.  This is
	//			used by functions that write out the flag names.
	atMap<ConstString, phMaterialFlags>::ConstIterator GetFlagIter() const
	{
		return m_FlagNameMap.CreateIterator();
	}

	// PURPOSE: Returns the number of registered flags.
	int GetNumFlags() const
	{
		return m_FlagNameMap.GetNumUsed();
	}

	// PURPOSE: Some tools want to load and store flags by name without caring
	//          about the value.  Enabling autoAssignFlags tells the material manager
	//			to automatically generate values for flag names that have not been
	//			registered.  If you enable this, you must not register any flags yourself.
	void SetAutoAssignFlags(bool autoAssignFlags) { m_AutoAssignFlags = autoAssignFlags; }

	// PURPOSE:	Get the current value of the UseDefaultMaterialIfNotFound option.
	// RETURNS:	The value of m_UseDefaultMaterialIfNotFound.
	bool GetUseDefaultMaterialIfNotFound() const
	{
		return m_UseDefaultMaterialIfNotFound;
	}

	// PURPOSE:	Set the value of the UseDefaultMaterialIfNotFound option.
	//			When this is true, materials with unknown names are treated
	//			as if they were the default material, which allows them to have
	//			flags. Without this option, for unknown materials the flags will
	//			be skipped too.
	// PARAMS:	b			- The new value of m_UseDefaultMaterialIfNotFound.
	void SetUseDefaultMaterialIfNotFound(bool b)
	{
		m_UseDefaultMaterialIfNotFound = b;
	}

#if __PS3
	virtual void FillInSpuWorkUnit(PairListWorkUnitInput& wuInput);
#endif

#if __BANK
	void SetSelectedMaterialMask(unsigned mask);
	unsigned GetSelectedMaterialMask() { return m_uSelectedMaterialMask; };
	virtual Color32 GetDebugColor(phMaterialMgr::Id id, phMaterialFlags polyFlags, phMaterialFlags highlightFlags) const;

	// Walks all materials and adds widgets for each of them, along with a toggle for overriding the physics widgets to make it easier to see the materials.
	void AddMaterialFlagWidgets(bkBank& bank);
#endif

protected:
	phMaterialMgrFlag();
	~phMaterialMgrFlag();

	virtual void BeginLoad(int reservedSlots);

	virtual void LoadMaterial (phMaterial& material, fiAsciiTokenizer& token);
	void LoadDefaultFlags(const phMaterial& material, fiAsciiTokenizer& token);

	virtual void SaveMaterial (const phMaterial& material, fiAsciiTokenizer& token) const;
	void SaveDefaultFlags(const phMaterial& material, fiAsciiTokenizer& token) const;

#if __BANK
	virtual void AddMaterialWidgets(phMaterial& material, bkBank& bank);
#endif

	atArray<phMaterialFlags> m_DefaultFlags;

	int m_BitsForIndex;
	atMap<ConstString, phMaterialFlags> m_FlagNameMap;
	u8  m_CurrentFlagBit;
	bool m_AutoAssignFlags;

	// PURPOSE:	When this is true, materials with unknown names are treated
	//			as if they were the default material, which allows them to have
	//			flags. Without this option, for unknown materials the flags will
	//			be skipped too.
	bool m_UseDefaultMaterialIfNotFound;


#if __BANK
	// The mask representing the currently selected set of materials.
	unsigned m_uSelectedMaterialMask;

	// This bool array is used by the widgets
	bool m_abActiveMaterials[sizeof(phMaterialFlags) * 8];
	phMaterialFlags m_aMaterialFlags[sizeof(phMaterialFlags) * 8];	// the flags for each material cannot be inferred from the order the iterator provides them.

	// The callback that is called when one of the material toggle widgets is hit.
	void UpdateSelectedMaterial();

	// This reacts to the toggle widget and enables the various widgets required to see the material bounds.
	void EnableBoundsDisplay();

	// Used by the widget that allows the user to turn on all the drawing widgets required to see materials by name.
	bool m_bPhysicsBoundsEnabledByMaterial;

	// A modifier for the HSV palette control
	float m_fColorPalette;
#endif

};

#define MATERIALMGRFLAG (dynamic_cast<phMaterialMgrFlag&>(MATERIALMGR))

} // namespace rage

#endif // PHCORE_MATERIALMGR_H
