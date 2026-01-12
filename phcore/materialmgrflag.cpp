// 
// phcore/materialmgrflag.cpp
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 
#include "materialmgrflag.h"

#include "material.h"

#include "atl/bintree.h"
#include "bank/bank.h"
#include "file/asset.h"
#include "math/random.h"
#include "grprofile/drawmanager.h"
#include "bank/bkmgr.h"
#include "vector/colors.h"
#include "vector/colorvector3.h"

#if __PS3
#include "phbullet/CollisionWorkUnit.h"
#endif

namespace rage {

EXT_PFD_DECLARE_GROUP(Physics);
EXT_PFD_DECLARE_GROUP(Bounds);
EXT_PFD_DECLARE_GROUP(BoundMaterials);
EXT_PFD_DECLARE_ITEM_SLIDER_INT(HighlightFlags);
EXT_PFD_DECLARE_ITEM(MaterialUseFlagColors);
EXT_PFD_DECLARE_ITEM_SLIDER(MaterialColorPalette);
EXT_PFD_DECLARE_ITEM_SLIDER(BoundDrawDistance);
EXT_PFD_DECLARE_ITEM(Solid);

phMaterialMgrFlag::phMaterialMgrFlag() 
: m_BitsForIndex(8)
, m_CurrentFlagBit(0)
, m_AutoAssignFlags(false)
, m_UseDefaultMaterialIfNotFound(false) 
#if __BANK
, m_fColorPalette(0.0f)
, m_bPhysicsBoundsEnabledByMaterial(false)
#endif
{
#if __BANK
	SetSelectedMaterialMask(0);
#endif
	m_MaterialIndexMask = (1 << m_BitsForIndex) - 1;
}

phMaterialMgrFlag::~phMaterialMgrFlag()
{
}

void phMaterialMgrFlag::BeginLoad(int reservedSlots) 
{ 
	m_DefaultFlags.Resize(reservedSlots); 
	if(m_DefaultFlags.GetCount() > 0)
	{
		memset(&m_DefaultFlags[0],0,sizeof(m_DefaultFlags[0])*m_DefaultFlags.GetCount());
	}
}

#if __PS3
void phMaterialMgrFlag::FillInSpuWorkUnit(PairListWorkUnitInput& wuInput)
{
	wuInput.m_materialMask = (1 << m_BitsForIndex) - 1;
}
#endif

#if __BANK
#if __PFDRAW
Color32 phMaterialMgrFlag::GetDebugColor(phMaterialMgr::Id id, phMaterialFlags polyFlags, phMaterialFlags highlightFlags) const
{
	if(!PFD_MaterialUseFlagColors.GetEnabled())
	{
		return phMaterialMgr::GetDebugColor(GetMaterial(id));
	}
#else
Color32 phMaterialMgrFlag::GetDebugColor(phMaterialMgr::Id /*id*/, phMaterialFlags polyFlags, phMaterialFlags highlightFlags) const
{
#endif

	// Determine if this particular material has multiple flags on it that match the highlight flags so that it can
	// be shown as a multiple-match color (red).
	phMaterialFlags match = polyFlags & highlightFlags;
	int index = 0;
	bool found = false;

	// Count the number of material bits that match the selected highlight bits; if more than 1 then use the multiple-match color (red)
	while(match)
	{
		if(match & 1)
		{
			// if this is the second bit discovered, then multiple bits match.
			if(found)
			{
				return Color_red;
			}
			found = true;
		}
		match >>= 1;
		index++;
	}

	// The material has only one matching bit, so return a color related to which flag it is.
	float h = fmodf((float) (index) / (float) (GetNumFlags() + 1) + m_fColorPalette, 1.0f);

	ColorVector3 colorVector3;
	colorVector3.Set(h, 1.0f, 1.0f);
	colorVector3.HSVtoRGB();
	Color32 color(colorVector3);

	return color;
}
#endif


void phMaterialMgrFlag::GetMaterialName(Id id, char* name, int size) const
{
	phMaterialMgr::GetMaterialName(id, name, size);

	phMaterialFlags flags = GetFlags(id);

	atMap<ConstString, phMaterialFlags>::ConstIterator i = m_FlagNameMap.CreateIterator();

	for ( ; !i.AtEnd(); ++i)
	{
		if (i.GetData() & flags)
		{
			safecat(name, ",", size);
			safecat(name, i.GetKey().m_String, size);
		}
	}
}

phMaterialMgr::Id phMaterialMgrFlag::FindMaterialId(const char* name) const
{
	phMaterialMgr::Id id = MATERIAL_NOT_FOUND;
	phMaterialFlags flags = 0;

	char buf[256];

	bool first = true;

	const char* s = name;
	for (size_t used = 0; used < sizeof(buf); s++)
	{
		if (*s != '\0' && *s != ',')
		{
			buf[used++] = *s;
			continue;
		}

		if (used == 0)
			continue;

		buf[used] = '\0';

		if (first)
		{
			// After this, we are no longer parsing the first item in the string. /FF
			first = false;

			id = phMaterialMgr::FindMaterialId(buf);

			if (id == MATERIAL_NOT_FOUND)
			{
				if(m_UseDefaultMaterialIfNotFound)
				{
					// Treat this as if it were the default material and continue. This
					// means that we can still set the flags correctly even though we
					// didn't recognize the material. /FF
					id = DEFAULT_MATERIAL_ID;
				}
				else
				{
					return id;
				}
			}
			else
			{
				flags = (static_cast<int>(id) < m_DefaultFlags.GetCount()) ? m_DefaultFlags[static_cast<int>(id)] : 0;
			}
		}
		else
		{
			flags |= GetFlagByName(buf);
		}

		if (*s == '\0')
			break;

		used = 0;
	}

	if (id == MATERIAL_NOT_FOUND)
		return id;

	return SetFlags(id, flags);
}

void phMaterialMgrFlag::RegisterFlag(const char* name, phMaterialFlags flag)
{
	AssertMsg(!m_AutoAssignFlags, "You must not register flags after auto-assignment is enabled.");

	char norm[256];
	StringNormalize(norm, name, sizeof(norm));

	phMaterialFlags* check = m_FlagNameMap.Access(norm);
	if (check)
		Assert(*check == flag);
	else
		m_FlagNameMap.Insert(ConstString(norm), flag);

#if __ASSERT
	while (static_cast<phMaterialFlags>((1U << m_CurrentFlagBit++)) < flag)
		AssertMsg(m_CurrentFlagBit < sizeof(phMaterialFlags) * 8, "Ran out of bits for material flags.");
#endif
}

phMaterialFlags phMaterialMgrFlag::GetFlagByName(const char* name) const
{
	char norm[256];
	StringNormalize(norm, name, sizeof(norm));

	const phMaterialFlags* flag = m_FlagNameMap.Access(norm);
	if (flag)
	{
		return *flag;
	}
	else if (m_AutoAssignFlags)
	{
		AssertMsg(m_CurrentFlagBit < sizeof(Id) * 8 - m_BitsForIndex, "Ran out of bits for material flags.");
		phMaterialFlags newFlag = (1 << const_cast<u8&>(m_CurrentFlagBit)++);
		Printf("Assigning flag \"%s\" value 0x%08x\n", name, static_cast<u32>(newFlag));
		const_cast<phMaterialMgrFlag*>(this)->m_FlagNameMap.Insert(ConstString(norm), newFlag);
		return newFlag;
	}
	else
	{
		Warningf("Unknown material flag \"%s\" specified", name);
		return 0;
	}
}

#if __BANK

void phMaterialMgrFlag::AddMaterialFlagWidgets(bkBank& bank)
{
	// Walks all materials and adds widgets for each of them, along with a toggle for overriding the physics widgets to make it easier to see the materials.
	bank.AddToggle("Enable Bounds Display", &m_bPhysicsBoundsEnabledByMaterial, datCallback(MFA(phMaterialMgrFlag::EnableBoundsDisplay),this), "Convenience toggle for the state of rage - ProfileDraw.Physics, ..Physics.Bounds, ..Physics.Bounds.BoundMaterials and also turns on solid drawing for bounds.");

	bank.AddSlider("Color Palette", &m_fColorPalette, 0.0f, 1.0f, 0.01f);
	atMap<ConstString, phMaterialFlags>::ConstIterator iterator = GetFlagIter();
	int index = 0;
	while(!iterator.AtEnd())
	{
		const ConstString str = iterator.GetKey();
		m_abActiveMaterials[index] = false;
		m_aMaterialFlags[index] = iterator.GetData();
		bank.AddToggle(str, &m_abActiveMaterials[index], datCallback(MFA(phMaterialMgrFlag::UpdateSelectedMaterial), this));
		index++;
		iterator.Next();
	}
}

void phMaterialMgrFlag::UpdateSelectedMaterial()
{
	// walk all the bools for the activated materials and set up the HighlightFlags mask to include any selected ones.
	int count = GetNumFlags();
	int index = 0;
	m_uSelectedMaterialMask = 0;
	while (index < count)
	{
		if(m_abActiveMaterials[index])
		{
			m_uSelectedMaterialMask |= m_aMaterialFlags[index];
		}
		index++;
	}

	PFD_ITEM_SLIDER_SET_VALUE(HighlightFlags, m_uSelectedMaterialMask);
}

void phMaterialMgrFlag::EnableBoundsDisplay()
{
	// This reacts to the toggle widget and enables the various widgets required to see the material bounds.
	PFD_ITEM_ENABLE(MaterialUseFlagColors, m_bPhysicsBoundsEnabledByMaterial);
	PFD_GROUP_ENABLE(Physics, m_bPhysicsBoundsEnabledByMaterial);
	PFD_GROUP_ENABLE(Bounds, m_bPhysicsBoundsEnabledByMaterial);
	PFD_GROUP_ENABLE(BoundMaterials, m_bPhysicsBoundsEnabledByMaterial);
	PFD_ITEM_ENABLE(Solid, m_bPhysicsBoundsEnabledByMaterial);
}
#endif

#if __BANK
void phMaterialMgrFlag::SetSelectedMaterialMask(unsigned selectedMask)
{
	m_uSelectedMaterialMask = selectedMask;
	unsigned index = 0;
	unsigned bitCount = sizeof(phMaterialFlags) * 8;
	while (index < bitCount)
	{
		m_abActiveMaterials[index] = ((m_aMaterialFlags[index] & selectedMask) != 0);
		index++;
	}
}
#endif


void phMaterialMgrFlag::LoadMaterial (phMaterial& material, fiAsciiTokenizer& token)
{
	material.LoadHeader(token);

	if(token.CheckIToken("{", true))
	{
		if(token.CheckIToken("version:",true))
		{
			char temp[32];
			token.GetToken(temp,32);
		}

		material.LoadData(token);

		LoadDefaultFlags(material, token);

		token.Pop();
	}
}


void phMaterialMgrFlag::SaveMaterial (const phMaterial& material, fiAsciiTokenizer& token) const
{
	material.SaveHeader(token);

	token.StartBlock();

	material.SaveData(token);

	SaveDefaultFlags(material, token);

	token.EndBlock();
}


void phMaterialMgrFlag::LoadDefaultFlags(const phMaterial& material, fiAsciiTokenizer& token)
{
#if PH_MATERIAL_ID_64BIT	// Gordon to check if this is needed - might be because atArray doesn't take u64 index into array
	const Id iMaterialId=GetMaterialId(material);
	Assert(iMaterialId<(phMaterialMgr::Id)m_DefaultFlags.size());
	Assert(iMaterialId<(phMaterialMgr::Id)std::numeric_limits<int>::max());
	const int iMaterialId32=static_cast<int>(iMaterialId);
	phMaterialFlags& flags = m_DefaultFlags[iMaterialId32];
#else
	phMaterialFlags& flags = m_DefaultFlags[GetMaterialId(material)];
#endif

	flags = 0;

	while (token.CheckIToken("flag:"))
	{
		char name[256];
		token.GetToken(name, sizeof(name));
		flags |= GetFlagByName(name);
	}
}


void phMaterialMgrFlag::SaveDefaultFlags(const phMaterial& material, fiAsciiTokenizer& token) const
{
#if PH_MATERIAL_ID_64BIT	// Gordon to check if this is needed - might be because atArray doesn't take u64 index into array
	const Id iMaterialId=GetMaterialId(material);
	Assert(iMaterialId<(phMaterialMgr::Id)m_DefaultFlags.size());
	Assert(iMaterialId<(phMaterialMgr::Id)std::numeric_limits<int>::max());
	const int iMaterialId32=static_cast<int>(iMaterialId);
	const phMaterialFlags& flags = m_DefaultFlags[iMaterialId32];
#else
	const phMaterialFlags& flags = m_DefaultFlags[GetMaterialId(material)];
#endif

	atMap<ConstString, phMaterialFlags>::ConstIterator i = m_FlagNameMap.CreateIterator();
	for ( ; !i.AtEnd(); ++i)
	{
		if (flags & i.GetData())
		{
			token.PutDelimiter("\tflag: ");
			token.Put(i.GetKey());
			token.PutDelimiter("\n");
		}
	}
}

#if __BANK

void phMaterialMgrFlag::AddMaterialWidgets(phMaterial& material, bkBank& bank)
{
	phMaterialMgr::AddMaterialWidgets(material, bank);

#if !PH_MATERIAL_ID_64BIT	//need to add widget code for u64
	atMap<ConstString, phMaterialFlags>::Iterator i = m_FlagNameMap.CreateIterator();
	if (!i.AtEnd())
	{
		bank.PushGroup("Default Flags", false);
		for ( ; !i.AtEnd(); ++i)
			bank.AddToggle(i.GetKey(), &m_DefaultFlags[GetMaterialId(material)], i.GetData());
		bank.PopGroup();
	}
#endif
}

#endif // __BANK

} // namespace rage


