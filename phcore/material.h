//
// phcore/material.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHCORE_MATERIAL_H
#define PHCORE_MATERIAL_H

#include "data/base.h"
#include "phcore/constants.h"
#include "vector/color32.h"

namespace rage {

class bkBank;
class fiAsciiTokenizer;
class fiStream;
class datResource;

/*
PURPOSE:
	Each material can have a default set of application-defined flags associated with it.
	Additionally, some bound types may provide a way of overriding the material defined
	default.  The flag are stored by name in the bound and material files, and before
	loading a bound or material file the names of the application-defined flags must
	be registered with the phMaterialMgr::AddFlag() function.
*/
#if PH_MATERIAL_ID_64BIT	// HACK_GTA4 - new, post gta4 option
typedef u64 phMaterialFlags;
#else
typedef u32 phMaterialFlags;
#endif

/*
PURPOSE:
Index into material array contained in a bound
This is not the same as a phMaterialMgr::Id,
but is an index into an array of phMaterialMgr::Id's
*/
typedef int phMaterialIndex;

/*
PURPOSE:
	Every object in the physics system is composed of one or more phMaterials,
	specified in their phBound. Within the core physics system, they are used for
	only two purposes: friction and elasticity. Those two values control some
	aspects of the collision response between physical objects.

PARAMETERS:
	Friction - A coefficient that determines how much tangential force can be generated
	           per newton of normal force. When this limit is exceeded by the collision
			   force, the objects slide relative to each other.
    Elasticity - When objects collide in reality, some amount of the energy of the collision
	             is stored in the object by deformation. Depending on the material of the object,
				 quite a bit of the energy can be returned as motion in the opposite direction.
				 An elasticity of 1.0 indicates perfectly elastic collisions, meaning that all
				 the energy is returned as motion. An elasticity of 0.0 indicates that all the
				 energy was dissipated as sound, heat, stored as potential energy, or whatever.

<FLAG Component>
*/
class phMaterial : public datBase
{
public:
	static const int MAX_NAME_LENGTH = 100;

	phMaterial ();
	phMaterial (datResource &rsc);

	virtual ~phMaterial ();

	enum {	MATERIAL = 0 };	// class type

	void SetClassType (int type);
	void SetName (const char* name);

	// PURPOSE:	Set the friction of this material.
	// PARAMS:
	//	friction - the new friction for this material
	// NOTES:
	//	Friction can vary from 0 (slippery) to infinity (sticky). Anything over 1 is pretty sticky.
	//	The friction used in collisions is the friction from the two colliding objects multiplied together.
	void SetFriction (float friction);

	// PURPOSE: Set the elasticity of this material.
	// PARAMS:
	//	elasticity - the new elasticity for this material
	// NOTES:
	//	Elasticity can vary from 0 (soft) to 1 (bouncy).
	//	The elasticity used in collisions is the elasticity from the two colliding objects multipled together.
	void SetElasticity (float elasticity);

	int GetClassType () const;
	const char* GetName () const;

	// PURPOSE:	Get the friction of this material.
	// RETURN:	the friction for this material
	// NOTES:
	//	Friction can vary from 0 (slippery) to infinity (sticky). Anything over 1 is pretty sticky.
	//	The friction used in collisions is the friction from the two colliding objects multiplied together.
	float GetFriction () const;

	// PURPOSE: Get the elasticity of this material.
	// RETURN:	the elasticity for this material
	// NOTES:
	//	Elasticity can vary from 0 (soft) to 1 (bouncy).
	//	The elasticity used in collisions is the elasticity from the two colliding objects multipled together.
	float GetElasticity () const;

	void LoadHeader (fiAsciiTokenizer& token);

	// PURPOSE: Loads tuning data from the tokenizer.
	// RETURN: true if any data was parsed
	// NOTES:
	//  If the physics material tune file has the potential for tuning data to appear in a different order than is written
	//  by SaveData() (perhaps hand edited), this function should be called by the application's parsing loop 
	//  until it returns false or there are no more tokens to parse.
	virtual bool LoadData (fiAsciiTokenizer& token);
	
	void SaveHeader (fiAsciiTokenizer& token) const;
	virtual void SaveData (fiAsciiTokenizer& token) const;

	#if __BANK
	virtual void AddWidgets (bkBank& bank);
	#endif

	// PURPOSE:
	//	Retrieve the color to use for debug drawing this particular material.
	// RETURN:
	//  The color red. Derived classes will set this up to something more useful, probably based on tuning data
	//  specific to their application.
	virtual Color32 GetDebugColor() const;

protected:
	static const int sm_ClassType;											// id of this class type
	static const char* sm_ClassTypeString;									// name of this class type

	int			m_Type;				// stored here for loading from resources
	const char*	m_Name;
	float		m_Friction;
	float		m_Elasticity;

	u8			m_Pad[12];
} ;

inline void phMaterial::SetClassType (int type)
{
	m_Type = type;
}

inline int phMaterial::GetClassType () const
{
	return m_Type;
}

inline const char* phMaterial::GetName () const
{
	return m_Name;
}

inline float phMaterial::GetFriction () const
{
	return m_Friction;
}

inline float phMaterial::GetElasticity () const
{
	return m_Elasticity;
}

} // namespace rage

#endif
