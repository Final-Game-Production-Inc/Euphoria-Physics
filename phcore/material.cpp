//
// phcore/material.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "material.h"

#include "bank/bank.h"
#include "data/resource.h"
#include "file/asset.h"
#include "file/stream.h"
#include "file/token.h"
#include "string/string.h"
#include "vector/colors.h"

namespace rage {

const int phMaterial::sm_ClassType = phMaterial::MATERIAL;
const char* phMaterial::sm_ClassTypeString = "BASE";

phMaterial::phMaterial ()
{
	const float DEFAULT_ELASTICITY = 0.1f;
	const float DEFAULT_FRICTION = 0.5f;
	const char* DEFAULT_NAME = "default";

	m_Type = MATERIAL;
	SetElasticity(DEFAULT_ELASTICITY);
	SetFriction(DEFAULT_FRICTION);
	SetName(DEFAULT_NAME);

	sysMemZeroBytes<sizeof(m_Pad)>(m_Pad);
}

phMaterial::~phMaterial ()
{
}


void phMaterial::SetName (const char* name)
{
	m_Name = name;
}


void phMaterial::SetFriction (float friction)
{
	m_Friction = friction;
}


void phMaterial::SetElasticity (float elasticity)
{
	m_Elasticity = elasticity;
}


void phMaterial::LoadHeader (fiAsciiTokenizer& token)
{
	if (token.CheckIToken("type:", true))
	{
		token.IgnoreToken();
	}

	if (token.CheckIToken("mtl", false))
	{
		// The name is optional, but if you provide one it'd better match the file name!
		char redundantName[phMaterial::MAX_NAME_LENGTH];
		token.MatchToken("mtl");
		token.GetToken(redundantName, phMaterial::MAX_NAME_LENGTH);
		redundantName[phMaterial::MAX_NAME_LENGTH-1]='\0';

		Assert(stricmp(redundantName, m_Name)==0);
	}
}

void phMaterial::SaveHeader (fiAsciiTokenizer& ) const
{
}

bool phMaterial::LoadData (fiAsciiTokenizer& token)
{
	bool parsed = false;
	if (token.CheckIToken("elasticity:"))
	{
		m_Elasticity = token.GetFloat();
		parsed = true;
	}

	if (token.CheckIToken("friction:"))
	{
		m_Friction = token.GetFloat();
		parsed = true;
	}
	return parsed;
}

void phMaterial::SaveData (fiAsciiTokenizer& token) const
{
	token.PutDelimiter("\telasticity: ");
	token.Put(m_Elasticity);
	token.PutDelimiter("\n");

	token.PutDelimiter("\tfriction: ");
	token.Put(m_Friction);
	token.PutDelimiter("\n");
}


#if __BANK
void phMaterial::AddWidgets (bkBank& bank)
{
	bank.AddSlider("Friction", &m_Friction, 0.0f, 20.0f, 0.01f);
	bank.AddSlider("Elasticity", &m_Elasticity, 0.0f, 20.0f, 0.01f);
}
#endif


phMaterial::phMaterial (datResource &)
{
}

Color32 phMaterial::GetDebugColor() const 
{ 
	return Color_red; 
}


} // namespace rage

