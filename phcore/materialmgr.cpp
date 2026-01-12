// 
// phcore/materialmgr.cpp
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 
#include "materialmgr.h"

#include "material.h"

#include "atl/bintree.h"
#include "bank/bank.h"
#include "file/asset.h"
#include "math/random.h"
#include "grprofile/drawmanager.h"
#include "vector/colorvector3.h"
#include "vector/colors.h"

#if __PS3
#include "phBullet/CollisionWorkUnit.h"
#endif

namespace rage {

EXT_PFD_DECLARE_ITEM_SLIDER(MaterialColorPalette);
EXT_PFD_DECLARE_ITEM(MaterialUseColorPalette);

phMaterialMgr* phMaterialMgr::sm_Instance = NULL;

phMaterialMgr::phMaterialMgr()
:	m_MaterialSize(0),
	m_NumMaterials(0),
	m_MaterialIndexMask((Id)-1),
	m_Materials(NULL)
{
}
phMaterialMgr::~phMaterialMgr()
{
}

void phMaterialMgr::FindCombinedFrictionAndElasticity(Id materialIdA, Id materialIdB, float& combinedFriction, float& combinedElasticity)
{
	// TODO: Remove the unnecessary branches here

	// Ensure that the extra bits in the ID are clear
	Id materialIndexA = materialIdA & m_MaterialIndexMask;
	Id materialIndexB = materialIdB & m_MaterialIndexMask;

	// Ordering the material ids here saves us from extra branches in the loop and doesn't affect the combined friction or elasticity
	if(materialIndexA > materialIndexB)
		SwapEm(materialIndexA,materialIndexB);

	// Check if this pair of materials has a material override
	for(int materialPairIndex = 0; materialPairIndex < m_MaterialOverridePairs.GetCount(); ++materialPairIndex)
	{
		const phMaterialPair& materialPair = m_MaterialOverridePairs[materialPairIndex];
		if(materialPair.m_MaterialIndexA == materialIndexA && materialPair.m_MaterialIndexB == materialIndexB)
		{
			combinedFriction = materialPair.m_CombinedFriction;
			combinedElasticity = materialPair.m_CombinedElasticity;
			return;
		}
	}

	// There was no override for this pair, just combine the materials normally
	const phMaterial& materialA = GetMaterial(materialIdA);
	const phMaterial& materialB = GetMaterial(materialIdB);
	combinedFriction = CombineFriction(materialA.GetFriction(), materialB.GetFriction());
	combinedElasticity = CombineElasticity(materialA.GetElasticity(), materialB.GetElasticity());
}


void phMaterialMgr::GetMaterialName(Id id, char* name, int size) const
{
	char buf[10];
	formatf(buf, 10, "_%d", (int)id);
	safecpy(name, GetMaterial(id).GetName(), size);
	safecat(name, buf, size);
}

Color32 phMaterialMgr::GetDebugColor(phMaterialMgr::Id id, phMaterialFlags /*polyFlags*/, phMaterialFlags /*highlightFlags*/) const 
{ 
	return GetDebugColor(GetMaterial(id)); 
}

Color32 phMaterialMgr::GetDebugColor(const phMaterial& material) const
{
#if __PFDRAW
	if(!PFD_MaterialUseColorPalette.GetEnabled())
	{
		return material.GetDebugColor();
	}
#endif

	Id value = GetMaterialId(material);
#if __PFDRAW
	float h = fmodf(((float) value + PFD_MaterialColorPalette.GetValue()) / (float) GetNumMaterials(), 1.0f);
#else
	float h = fmodf((float) value / (float) GetNumMaterials(), 1.0f);
#endif

	ColorVector3 colorVector3;
	colorVector3.Set(h, 1.0f, 1.0f);
	colorVector3.HSVtoRGB();
	Color32 color(colorVector3);

	return color;
}

#if __PS3
void phMaterialMgr::FillInSpuWorkUnit(PairListWorkUnitInput& wuInput)
{
	wuInput.m_MaterialOverridePairs = m_MaterialOverridePairs.GetElements();
	wuInput.m_NumMaterialOverridePairs = m_MaterialOverridePairs.GetCount();
}
#endif

#if __BANK
void phMaterialMgr::AddWidgets(bkBank& bank)
{
	bank.AddButton("Save All",datCallback(MFA(phMaterialMgr::SaveAll),this));

	// We want to insert the materials alphabetically, so dump them in an atBinTree before adding the widgets
	typedef atBinTree<ConstString, const phMaterial*> SortTree;

	SortTree sorted;

	for (int materialId = 1; materialId < GetNumMaterials(); ++materialId)
	{
		const phMaterial* material = &GetMaterial(materialId);
		char lowName[phMaterial::MAX_NAME_LENGTH];
		StringNormalize(lowName, material->GetName(), sizeof(lowName));

		sorted.Insert(lowName, material);
	}

	bank.PushGroup(GetDefaultMaterial().GetName(), false);
	AddMaterialWidgets(const_cast<phMaterial&>(GetDefaultMaterial()), bank);
	bank.PopGroup();

	// Now go through them in sorted order to add them to the bank
	for (SortTree::Iterator it = sorted.CreateIterator(); !it.AtEnd(); it.Next())
	{
		bank.PushGroup(it.GetData()->GetName(), false);
		AddMaterialWidgets(const_cast<phMaterial&>(*it.GetData()), bank);
		bank.PopGroup();
	}

	sorted.DeleteAll();
}

bool phMaterialMgr::BankSaveMaterial (phMaterial* material)
{
	Assert(material);

	ASSET.PushFolder("$");
	ASSET.PushFolder(m_AssetFolder);
	fiStream * s = ASSET.Create(material->GetName(), "mtl");
	ASSET.PopFolder();
	ASSET.PopFolder();
	if (s)
	{
		fiTokenizer T(material->GetName(), s);
		SaveMaterial(*material, T);
		s->Close();
		return true;
	}
	return false;
}

bool phMaterialMgr::BankLoadMaterial (phMaterial* material)
{
	Assert(material);

	ASSET.PushFolder(m_AssetFolder);
	fiStream * s = ASSET.Open(material->GetName(), "mtl");
	ASSET.PopFolder();
	if (s)
	{
		fiTokenizer T(material->GetName(), s);
		LoadMaterial(*material, T);
		s->Close();
		return true;
	}
	return false;
}

void phMaterialMgr::AddMaterialWidgets(phMaterial& material, bkBank& bank)
{
	if (&GetDefaultMaterial() != &material)
	{
		bank.AddButton("Save",datCallback(MFA1(phMaterialMgr::BankSaveMaterial),this,&material));
		bank.AddButton("Load",datCallback(MFA1(phMaterialMgr::BankLoadMaterial),this,&material));
	}

	material.AddWidgets(bank);
}

#endif // __BANK

void phMaterialMgr::SaveAll() const
{
	ASSET.PushFolder(m_AssetFolder);

	for (int materialId = 0; materialId < GetNumMaterials(); ++materialId)
	{
		const phMaterial* material = &GetMaterial(materialId);

		if (fiStream* materialStream = ASSET.Create(material->GetName(), "mtl", false))
		{
			fiAsciiTokenizer token;
			token.Init(material->GetName(), materialStream);

			SaveMaterial(*material, token);

			materialStream->Close();
		}
	}

	ASSET.PopFolder();
}

void phMaterialMgr::LoadMaterial (phMaterial& material, fiAsciiTokenizer& token)
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

		token.Pop();
	}
}

void phMaterialMgr::SaveMaterial (const phMaterial& material, fiAsciiTokenizer& token) const
{
	material.SaveHeader(token);

	token.StartBlock();

	material.SaveData(token);

	token.EndBlock();
}

} // namespace rage
