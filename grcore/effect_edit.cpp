//
// grcore/effect_edit.cpp
//
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved.
//
#include "effect.h"
#include "texture.h"
#include "file/asset.h"

#if __BANK
#include "bank/bank.h"
#include "bank/bkmgr.h"
#include "bank/text.h"
#endif

namespace rage {

#if __BANK
void grcMaterialLibrary::AddWidgets(bkBank& B) const
{
	for (int i=0; i<GetCount(); i++) {
		if( GetEntry(i) )
		{
			B.PushGroup(GetEntry(i)->MaterialName);
			GetEntry(i)->AddWidgets(B);
			B.PopGroup();
		}
	}
}
#endif


#if __BANK
void grcInstanceData::AddWidgets(bkBank &B) const
{
	for (int pass=0; pass<2; pass++) {
		bool created = false;
		for (int i=0; i<Count; i++) {
			const char *name;
			grcEffect::VarType type;
			grcEffectVar var = Basis->GetVarByIndex(i);
#if EFFECT_PRESERVE_STRINGS
			int annotationCount;
			bool isGlobal;
			const char *actualName;
			Basis->GetVarDesc(var,name,type,annotationCount,isGlobal,&actualName);
#else
			u32 nameHash, actualNameHash;
			char nameHashBuf[128];
			Basis->GetVarDesc(var,nameHash,type,&actualNameHash);
			formatf(nameHashBuf,"hash_%x",nameHash);
			name = nameHashBuf;
#endif
			/* VT_NONE, VT_INT, VT_FLOAT, VT_VECTOR2,				// 0-3
				VT_VECTOR3, VT_VECTOR4, VT_TEXTURE, VT_BOOL,		// 4-7
				VT_MATRIX34, VT_MATRIX44, VT_STRING,				// 8-A */

			name = Basis->GetAnnotationData(var,"UIName",(char*)0);
			if (!name)
				continue;

			// Set the group to better organize the data
			if (Basis->m_Locals[i].Usage & USAGE_MATERIAL) {
				if (pass == 0)
					continue;
				if (!created) {
					created = true;
					B.PushGroup("Material Variables", false);
				}
			}
			else {
				if (pass == 1)
					continue;
				if (!created) {
					created = true;
					B.PushGroup("Instance Variables", false);
				}
			}
			
			float uiMin = Basis->GetAnnotationData(var,"UIMin",0.0f);
			float uiMax = Basis->GetAnnotationData(var,"UIMax",1.0f);
			float uiStep = Basis->GetAnnotationData(var,"UIStep",0.001f);
			bool isColor = !stricmp(Basis->GetAnnotationData(var,"UIWidget","not color"),"color");
			if (!isColor)
				isColor = (strstr(name,"Color") != 0 || strstr(name,"color") != 0) && uiMin == 0.0f && uiMax == 1.0f;

			switch (type) {
				case grcEffect::VT_FLOAT: B.AddSlider(name, Data()[i].Float,uiMin,uiMax,uiStep); break;
				case grcEffect::VT_VECTOR2: B.AddSlider(name, (Vector2*)Data()[i].Float,uiMin,uiMax,uiStep); break;
				case grcEffect::VT_VECTOR3: 
					if (isColor)
						B.AddColor(name, (Vector3*)Data()[i].Float,uiStep);
					else
						B.AddSlider(name, (Vector3*)Data()[i].Float,uiMin,uiMax,uiStep);
					break;
				case grcEffect::VT_VECTOR4: 
					if (isColor)
						B.AddColor(name, (Vector4*)Data()[i].Float,uiStep); 
					else
						B.AddSlider(name, (Vector4*)Data()[i].Float,uiMin,uiMax,uiStep); 
					break;
				case grcEffect::VT_TEXTURE: {
					char *texName = (char*)(Data()[i].Texture? Data()[i].Texture->GetName():"NULL");
					B.AddText(name, texName, StringLength(texName) + 1,true);
					break;
				}
				default: break;
			}
		}

		if (created)
			B.PopGroup();
	}
}

const char* grcInstanceData::GetVariableName(int varNum) const
{
	const char *name;
	grcEffect::VarType type;
	grcEffectVar var = Basis->GetVarByIndex(varNum);
#if EFFECT_PRESERVE_STRINGS
	int annotationCount;
	bool isGlobal;
	const char *actualName;
	Basis->GetVarDesc(var,name,type,annotationCount,isGlobal,&actualName);
	return actualName;
#else
	u32 nameHash, actualNameHash;
	char nameHashBuf[128];
	Basis->GetVarDesc(var,nameHash,type,&actualNameHash);
	formatf(nameHashBuf,"hash_%x",nameHash);
	name = nameHashBuf;
	return name;
#endif
}

bool grcInstanceData::HasTexture(int varNum, const char* previousTextureName) const
{
	grcEffect::VarType type;
	grcEffectVar var = Basis->GetVarByIndex(varNum);
#if EFFECT_PRESERVE_STRINGS
	const char *name;
	int annotationCount;
	bool isGlobal;
	const char *actualName;
	Basis->GetVarDesc(var,name,type,annotationCount,isGlobal,&actualName);
#else
	u32 nameHash, actualNameHash;
	char nameHashBuf[128];
	Basis->GetVarDesc(var,nameHash,type,&actualNameHash);
	formatf(nameHashBuf,"hash_%x",nameHash);
#endif

	switch (type) 
	{
		case grcEffect::VT_TEXTURE: {

			char *texName = (char*)(Data()[varNum].Texture? Data()[varNum].Texture->GetName():NULL);
			if ( texName != NULL )
			{
				const char* texFileName = strlwr(texName);
				texFileName = fiAssetManager::FileName(texName);
				char filename[RAGE_MAX_PATH];
				fiAssetManager::RemoveExtensionFromPath(filename, RAGE_MAX_PATH, texFileName);

				if ( previousTextureName == NULL 
					|| (previousTextureName != NULL && stricmp(filename, previousTextureName) == 0 ) )
				{
					return true;
				}
			}
			break;
		}
		default: break;
	}

	return false;
}

grcTexChangeData::grcTexChangeData()
{
	m_instance         = NULL;
	m_varNum           = -1;
	m_userData         = -1;
	m_ready            = false;
	m_textBox          = NULL;
	m_textBoxString[0] = '\0';
}

grcTexChangeData::grcTexChangeData(grcInstanceData* instance, int varNum, const atString& filename, int userData)
{
	m_instance         = instance;
	m_varNum           = varNum;
	m_filename         = filename;
	m_userData         = userData;
	m_ready            = false;
	m_textBox          = NULL;
	m_textBoxString[0] = '\0';
}

grcTexChangeData::~grcTexChangeData()
{
}

grcTexChangeData::UpdateTextureFuncType grcTexChangeData::s_UpdateTextureFunc = NULL;

void grcTexChangeData::UpdateTextureParams()
{
	if (m_ready)
	{
		strncpy(m_textBoxString, m_filename, m_filename.length());
		m_textBox->SetStringPointer(m_textBoxString);
		
		grcTexture *texture = NULL;
		const char* ext = strrchr(m_filename, '.');

		if (ext && stricmp(ext + 1, grcTexture::GetTxdExtension()) == 0) // is it a TXD?
		{
			texture = grcTexture::RunCustomLoadFunc(m_filename);
		}
		else // no, it's probably just a DDS
		{
			texture = grcTextureFactory::GetInstance().Create(m_filename);
		}

		grcEffect &effect = m_instance->GetBasis();

		effect.SetVar(*m_instance, effect.GetVarByIndex(m_varNum), texture);

		if (s_UpdateTextureFunc)
		{
			s_UpdateTextureFunc(texture, m_filename, m_userData);
		}

		// Reset
		m_ready = false;
	}
}

void grcTexChangeData::ReloadTextureFile()
{
	m_ready = true;
}

void grcTexChangeData::LoadTextureFile()
{
	char c_str_extlist[32] = "";
	char c_str_filename[RAGE_MAX_PATH] = "";
	sprintf(c_str_extlist, "*.dds;*.%s", grcTexture::GetTxdExtension());
	bool fileSelected = BANKMGR.OpenFile(c_str_filename, sizeof(c_str_filename), c_str_extlist, false, "Open DDS/TXD");

	// Open file with the given name
	if (fileSelected)
	{
		m_ready = true;
		m_filename = c_str_filename;
	}
}


static void SamplerStateEditReload(CallbackData cb)
{
	atArray<grcSamplerStateEditData>* samplerStateEditData = (atArray<grcSamplerStateEditData>*)cb;
	Assert(samplerStateEditData);

	const u32 count = samplerStateEditData->GetCount();
	for(u32 i=0; i<count; i++)
	{
		grcSamplerStateEditData &edit = (*samplerStateEditData)[i];
		if(!edit.m_enabled)
			return;

		grcInstanceData		&data  = *edit.m_data;
		grcEffect			&basis = edit.m_data->GetBasis();
		grcSamplerStateHandle oldHandle = basis.GetSamplerState(data, edit.m_var);
		grcStateBlock::DestroySamplerState(oldHandle); 
		basis.SetSamplerState(data, edit.m_var, grcStateBlock::CreateSamplerState(edit.m_desc));
	}
}

int grcInstanceData::GetNumTextures() const
{
	int count = 0;
	
	for (int i=0; i<Count; i++) {
		const char *name;
		grcEffect::VarType type;
		grcEffectVar var = Basis->GetVarByIndex(i);
		name = Basis->GetAnnotationData(var,"UIName",(char*)0);
		if (!name)
			continue;

#if EFFECT_PRESERVE_STRINGS
			const char *actualName;
			int annotationCount;
			bool isGlobal;
			Basis->GetVarDesc(var,name,type,annotationCount,isGlobal,&actualName);
#else
			u32 nameHash, actualNameHash;
			char nameHashBuf[128];
			Basis->GetVarDesc(var,nameHash,type,&actualNameHash);
			formatf(nameHashBuf,"hash_%x",nameHash);
			name = nameHashBuf;
#endif

		switch (type) {
			case grcEffect::VT_TEXTURE: 
				count++;
				break;
			default: 
				break;
		}
	}

	return count;
}

void grcInstanceData::AddWidgets_WithLoad(bkBank &B, atArray<grcTexChangeData> &texArray, int userData, atArray<grcSamplerStateEditData> *samplerEditData)
{
	for (int pass=0; pass<2; pass++) {
		bool created = false;
		for (int i=0; i<Count; i++) {
			const char *name;
			grcEffect::VarType type;
			grcEffectVar var = Basis->GetVarByIndex(i);
#if EFFECT_PRESERVE_STRINGS
			const char *actualName;
			int annotationCount;
			bool isGlobal;
			Basis->GetVarDesc(var,name,type,annotationCount,isGlobal,&actualName);
#else
			u32 nameHash, actualNameHash;
			char nameHashBuf[128];
			Basis->GetVarDesc(var,nameHash,type,&actualNameHash);
			formatf(nameHashBuf,"hash_%x",nameHash);
			name = nameHashBuf;
#endif

			name = Basis->GetAnnotationData(var,"UIName",(char*)0);
			if (!name)
				continue;

			// Set the group to better organize the data
			if (Basis->m_Locals[i].Usage & USAGE_MATERIAL) {
				if (pass == 0)
					continue;
				if (!created) {
					created = true;
					B.PushGroup("Material Variables", false);
				}
			}
			else {
				if (pass == 1)
					continue;
				if (!created) {
					created = true;
					B.PushGroup("Instance Variables", false);
				}
			}

			float uiMin = Basis->GetAnnotationData(var,"UIMin",0.0f);
			float uiMax = Basis->GetAnnotationData(var,"UIMax",1.0f);
			float uiStep = Basis->GetAnnotationData(var,"UIStep",0.001f);
			bool isColor = !stricmp(Basis->GetAnnotationData(var,"UIWidget","not color"),"color");
			if (!isColor)
				isColor = (strstr(name,"Color") != 0 || strstr(name,"color") != 0) && uiMin == 0.0f && uiMax == 1.0f;

			switch (type) {
				case grcEffect::VT_FLOAT: B.AddSlider(name, Data()[i].Float,uiMin,uiMax,uiStep); break;
				case grcEffect::VT_VECTOR2: B.AddSlider(name, (Vector2*)Data()[i].Float,uiMin,uiMax,uiStep); break;
				case grcEffect::VT_VECTOR3: 
					{
						if (isColor)
							B.AddColor(name, (Vector3*)Data()[i].Float,uiStep);
						else
							B.AddSlider(name, (Vector3*)Data()[i].Float,uiMin,uiMax,uiStep);
						break;
					}

				case grcEffect::VT_VECTOR4: 
					{

						if (isColor)
							B.AddColor(name, (Vector4*)Data()[i].Float,uiStep); 
						else
							B.AddSlider(name, (Vector4*)Data()[i].Float,uiMin,uiMax,uiStep); 
						break;
					}

				case grcEffect::VT_TEXTURE: 
					{
						char *texName = (char*)(Data()[i].Texture? Data()[i].Texture->GetName():"NULL");

						// Get just the filename and add in path for N:
						char texture_name[RAGE_MAX_PATH];
						formatf(texture_name, sizeof(texture_name), "%s", texName);
						fiAssetManager::RemoveExtensionFromPath(texture_name, sizeof(texture_name), texture_name);

						atString fileName;
						fileName += "N:/RSGEDI/GTA/GTAExport/texturesGTA5/";
						fileName += fiAssetManager::FileName(texture_name);
		
						texArray.PushAndGrow(grcTexChangeData(this, i, fileName, userData));
						grcTexChangeData &currData = texArray[texArray.GetCount() - 1];

						if (texName)
							strncpy(currData.m_textBoxString, texName, sizeof(currData.m_textBoxString));
						currData.m_textBox = B.AddROText(name, "", 255);
						currData.m_textBox->SetStringPointer(currData.m_textBoxString);
						
						// if the atArray moved (usually from growing), we need to adjust the textbox pointers.
						char buttoName[RAGE_MAX_PATH];
						sprintf(buttoName, "Load New %s", name);
						B.AddButton(buttoName, datCallback(MFA(grcTexChangeData::LoadTextureFile), &currData), "Load a new texture");
						B.AddButton("Reload texture", datCallback(MFA(grcTexChangeData::ReloadTextureFile), &currData), "Reload current texture");
						
						if(samplerEditData)
						{
							B.PushGroup("Sampler state edit");
								grcSamplerStateEditData& samplerEdit = samplerEditData->Grow();
								samplerEdit.m_enabled	= false;
								samplerEdit.m_data		= this;
								samplerEdit.m_var		= var;
								grcSamplerStateHandle h = GetBasis().GetSamplerState(*this, var);
								grcStateBlock::GetSamplerStateDesc(h, samplerEdit.m_desc);
								
								B.AddToggle("Enable", &samplerEdit.m_enabled);
								B.AddSlider("Min Mip Level", &samplerEdit.m_desc.MinLod,		0.0f, 12.0f, 1.0f,	datCallback(SamplerStateEditReload, samplerEditData));
								B.AddSlider("Max Mip Level", &samplerEdit.m_desc.MaxLod,		0.0f, 12.0f, 1.0f,	datCallback(SamplerStateEditReload, samplerEditData));
								B.AddSlider("Mip Lod Bias",	 &samplerEdit.m_desc.MipLodBias,	-8.0f, 8.0f, 1.0f,	datCallback(SamplerStateEditReload, samplerEditData));
							B.PopGroup();
						}
						break;
					}
				default: break;
			}
		}

		if (created)
			B.PopGroup();
	}
}
#endif // __BANK

}	// namespace rage