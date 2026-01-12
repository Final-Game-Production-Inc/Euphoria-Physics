//
// physics/archetype.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "archetype.h"

#if __BANK
#include "bank/bank.h"
#include "bank/slider.h"
#endif

#include "data/resource.h"
#include "data/resourcehelpers.h"
#include "file/asset.h"
#include "file/token.h"
#include "phbound/boundcomposite.h"
#include "phcore/config.h"
#include "phcore/phmath.h"
#include "string/string.h"

using namespace rage;


////////////////////////////////////////////////////////////////
// static functions

const char * phArchetype::sClassTypeString = "BASE";
const char * phArchetypePhys::sClassTypeString = "PHYS";
const char * phArchetypeDamp::sClassTypeString = "DAMP";


const char* phArchetype::GetClassTypeString () const
{
	switch(GetClassType())
	{
	default: // intentional fall thougth.
		Assert(0);
	case ARCHETYPE:	     return phArchetype::sClassTypeString;
	case ARCHETYPE_PHYS: return phArchetypePhys::sClassTypeString;
	case ARCHETYPE_DAMP: return phArchetypeDamp::sClassTypeString;
	}
}


phArchetype* phArchetype::Load (const char* fileName, phBound* bound)
{
	fiStream* stream;
	if (ASSET.Exists(fileName,"phys") &&
		(stream = ASSET.Open(fileName,"phys"))!=NULL)
	{
		fiAsciiTokenizer token;
		token.Init(fileName,stream);

		phArchetype* archetype = Load(token,bound);

		stream->Close();

		return archetype;
	}
	else
	{
		// Failed to load a fileName.phys file
		if(!phBound::GetBoundFlag(phBound::DONT_WARN_MISS_PHYS))
		{
			Warningf("phArchetype::Load() -- .phys file %s does not exist",fileName);
		}
		return NULL;
	}
}


phArchetype* phArchetype::Load (fiAsciiTokenizer& token, phBound* bound)
{
	phArchetype* archetype = phArchetype::CreateOfType(token);

	AssertMsg(archetype , "phArchetype:Load - failed to create the correct type of archetype");

	archetype->LoadData(token,bound);

	return archetype;
}


phArchetype* phArchetype::CreateOfType (int classType)
{
	switch (classType)
	{
		case ARCHETYPE:
		{
			return rage_new phArchetype;
		}
		case ARCHETYPE_PHYS:
		{
			return rage_new phArchetypePhys;
		}
		case ARCHETYPE_DAMP:
		{
			return rage_new phArchetypeDamp;
		}
	}

	return NULL;
}


phArchetype* phArchetype::CreateOfType (fiAsciiTokenizer& token)
{
	token.GetDelimiter("version:");

	int version = token.GetInt();
	if (version!=100)
	{
		Errorf("phArchetype:CreateOfType - unknown version number, %d",version);
		return NULL;
	}

	token.GetDelimiter("type:");

	if (token.CheckIToken(phArchetype::sClassTypeString))
	{
		return rage_new phArchetype;
	}
	else if (token.CheckIToken(phArchetypePhys::sClassTypeString))
	{
		return rage_new phArchetypePhys;
	}
	else if (token.CheckIToken(phArchetypeDamp::sClassTypeString))
	{
		return rage_new phArchetypeDamp;
	}

	return NULL;
}


////////////////////////////////////////////////////////////////

u32 phArchetype::sDefaultTypeFlags = DEFAULT_TYPE;
u32 phArchetype::sDefaultIncludeFlags = INCLUDE_FLAGS_ALL;
u16 phArchetype::sDefaultPropertyFlags = 0;


void phArchetype::SetDefaultTypeFlag (u32 mask, bool value)
{
	if (value)
	{
		sDefaultTypeFlags |= mask;
	}
	else
	{
		sDefaultTypeFlags &= ~mask;
	}
}


void phArchetype::SetDefaultIncludeFlag (u32 mask, bool value)
{
	if (value)
	{
		sDefaultIncludeFlags |= mask;
	}
	else
	{
		sDefaultIncludeFlags &= ~mask;
	}
}


void phArchetype::SetDefaultPropertyFlag (u16 mask, bool value)
{
	if (value)
	{
		sDefaultPropertyFlags |= mask;
	}
	else
	{
		sDefaultPropertyFlags &= ~mask;
	}
}


////////////////////////////////////////////////////////////////
// phArchetype

phArchetype::phArchetype ()
{
	m_Type = ARCHETYPE;
	m_RefCount = 0;
	m_Bound = NULL;
	m_Filename = NULL;
	Erase();
}


void phArchetype::Erase ()
{
	if (phConfig::IsRefCountingEnabled())
	{
		AssertMsg(m_RefCount==0 , "phArchetype:Erase - clearing an archetype with outstanding references");
	}
	SetBound(NULL);
	m_TypeFlags = sDefaultTypeFlags;
	m_IncludeFlags = sDefaultIncludeFlags;
	m_PropertyFlags = sDefaultPropertyFlags;
}


phArchetype::~phArchetype ()
{
	if (phConfig::IsRefCountingEnabled())
	{
		AssertMsg(m_RefCount==0 , "phArchetype:~phArchetype - deleting an archetype with outstanding references");
	}
	SetBound(NULL);
}


////////////////////////////////////////////////////////////////
// resources

void phArchetype::LoadData (fiAsciiTokenizer& token, phBound* bound)
{
	const int bufLen = 128;
	char buffer[bufLen];

	if (!bound)
	{
		phBound * newBound=NULL;
		if(token.CheckToken("boundsource:"))
		{
			token.GetToken(buffer,bufLen);

			if (stricmp(buffer,"file")==0)
			{
				token.GetToken(buffer,bufLen);
				newBound = phBound::Load(buffer);
			}
			else if (stricmp(buffer,"local")==0)
			{
				token.GetDelimiter("{");
				newBound = phBound::Load(token);
				token.GetDelimiter("}");
			}
			else
			{
				Quitf(ERR_PHY_ARCH,"phArchetype:Load - boundsource unknown, '%s'",buffer);
			}
		}
		else
		{
			newBound = phBound::Load(token.filename);
		}

		SetBound(newBound);
	}
	else
	{
		SetBound(bound);
	}

	if (token.CheckIToken("TypeFlags:"))
	{
		m_TypeFlags = token.GetInt();
	}

	if (token.CheckIToken("IncludeFlags:"))
	{
		m_IncludeFlags = token.GetInt();
	}

	if (token.CheckIToken("PropertyFlags:"))
	{
		m_PropertyFlags = (u16)token.GetInt();
	}
}


#if (!__FINAL && !IS_CONSOLE) || (__BANK && IS_CONSOLE)

bool phArchetype::Save (const char* name, eBoundDestination dest, const char* boundFilename)
{
	// Open the phys file if it already exists.
	fiStream* s = ASSET.Open(name,"phys",false,false);

	if (!s)
	{
		// The phys file does not already exist, so make one.
		s = ASSET.Create(name,"phys");
	}

	if (!s)
	{
		// The phys file couldn't be opened or created.
		Errorf("phArchetype:Save() - Can't open file '%s'",name);
		return false;
	}

	fiAsciiTokenizer token;
	token.Init(name,s);
	Save(token,dest,boundFilename);
	s->Close();
	return true;
}


bool phArchetype::Save (fiAsciiTokenizer & token, eBoundDestination WIN32PC_ONLY(dest),
						const char* UNUSED_PARAM(boundFilename))
{
	token.Put("version: 100\n");
	token.Put("type: ");
	token.Put(GetClassTypeString());
	token.Put("\n");

#if !IS_CONSOLE
	// When saving a physics archetype on a console, the bound is not saved (bounds can never be saved on
	// consoles, but physics archetypes can (for tuning physical parameters in widgets).

	if (dest==BOUND_FILE)
	{
		Quitf("phArchetype:Save - BOUND_FILE not implemented yet (do we want this?)");
	}
	else if (dest==BOUND_LOCAL)
	{
		token.Put("boundsource: local\n");
		Assert(m_Bound);
		token.PutDelimiter("{\n");
		phBound::Save(token,m_Bound);
		token.PutDelimiter("}\n");
	}
#endif	// end of #if !__CONSOLE

	SaveData(token);

	return true;
}


void phArchetype::SaveData (fiAsciiTokenizer& token)
{
	if (m_TypeFlags!=phArchetype::sDefaultTypeFlags)
	{
		token.PutDelimiter("\nTypeFlags: ");
		token.Put((int)m_TypeFlags);
	}

	if (m_IncludeFlags!=phArchetype::sDefaultIncludeFlags)
	{
		token.PutDelimiter("\nIncludeFlags: ");
		token.Put((int)m_IncludeFlags);
	}

	if (m_PropertyFlags!=phArchetype::sDefaultPropertyFlags)
	{
		token.PutDelimiter("\nPropertyFlags: ");
		token.Put((int)m_PropertyFlags);
	}
}
#endif	// #if (!__FINAL && !IS_CONSOLE) || (__BANK && IS_CONSOLE)


void phArchetype::CopyData (const phArchetype* original)
{
	// Copy the basic archetype class information.
	m_TypeFlags = original->GetTypeFlags();
	m_IncludeFlags = original->GetIncludeFlags();
	m_PropertyFlags = original->GetPropertyFlags();
	m_Filename = original->GetFilename();
}


void phArchetype::Copy (const phArchetype* original)
{
	// Copy the given archetype into this archetype.
	Assert(GetClassType()==original->GetClassType());
	CopyData(original);

	if (m_Bound)
	{
		// Copy this original archetype's bound into the this archetype's bound.
		m_Bound->Copy(original->GetBound());

		// Make sure the new bound has no references other than ours
		Assert(m_Bound->GetRefCount()==1 || !phConfig::IsRefCountingEnabled());
	}
	else if (original->GetBound())
	{
		// The original archetype has a bound pointer, and this one does not, so make a new bound.
		SetBound(original->GetBound()->Clone());

		// Take full ownership of the cloned bound, by dereferencing it. After this,
		// the new bound will have a reference count of 1, so deleting this archetype will delete the bound
		m_Bound->Release();
	}


	// Make the new archetype's reference count 0.
	m_RefCount = 0;
}


phArchetype* phArchetype::Clone () const
{
	phArchetype* clone = phArchetype::CreateOfType(GetClassType());
	AssertMsg(clone , "invalid physics archetype class type");
	clone->Copy(this);
	return clone;
}


int phArchetype::Release (bool deleteAtZero)
{
	if (!phConfig::IsRefCountingEnabled())
	{
		return 1;
	}

	AssertMsg(m_RefCount , "phArchetype:Release - already has 0 references");

	m_RefCount--;

	if (m_RefCount==0)
	{
		if (deleteAtZero)
		{
			delete this;
		}
		else
		{
			this->Erase();
		}
		return 0;
	}
	else
	{
		return m_RefCount;
	}
}


void phArchetype::SetBound (phBound* bound)
{
	if (bound!=NULL)
	{
		// Add a reference count to the new bound.
		bound->AddRef();
	}

	phBound* oldBound = m_Bound;

	// Replace the old bound with the new bound.
	m_Bound = bound;

	if (oldBound!=NULL)
	{
		// Remove a reference count from the old bound.
		oldBound->Release();
	}
}


void phArchetype::AddTypeFlags (u32 mask)
{
	m_TypeFlags |= mask;
}

void phArchetype::RemoveTypeFlags (u32 mask)
{
	m_TypeFlags &= ~mask;
}


void phArchetype::AddIncludeFlags (u32 mask)
{
	m_IncludeFlags |= mask;
}

void phArchetype::RemoveIncludeFlags (u32 mask)
{
	m_IncludeFlags &= ~mask;
}


void phArchetype::SetPropertyFlag (u16 mask, bool value)
{
	if (value)
	{
		m_PropertyFlags |= mask;
	}
	else
	{
		m_PropertyFlags &= ~mask;
	}
}


////////////////////////////////////////////////////////////////
// phArchetypePhys

float phArchetypePhys::sDefaultDensity = 0.3f;
float phArchetypePhys::sDefaultGravityFactor = 1.0f;


phArchetypePhys::phArchetypePhys ()
{
	m_Type = ARCHETYPE_PHYS;
	SetMassInternal(1.0f);
	SetInvMassInternal(1.0f);
	SetAngInertiaInternal(Vec3V(V_ONE));
	SetInvAngInertiaInternal(Vec3V(V_ONE));
	m_GravityFactor = sDefaultGravityFactor;
	m_MaxSpeed = DEFAULT_MAX_SPEED;
	m_MaxAngSpeed = DEFAULT_MAX_ANG_SPEED;
	m_BuoyancyFactor = 1.0f;
}


void phArchetypePhys::Erase ()
{
	phArchetype::Erase();
	SetMassInternal(1.0f);
	SetInvMassInternal(1.0f);
	SetAngInertiaInternal(Vec3V(V_ONE));
	SetInvAngInertiaInternal(Vec3V(V_ONE));
	m_GravityFactor = sDefaultGravityFactor;
	m_BuoyancyFactor = 1.0f;
}


void phArchetypePhys::LoadData (fiAsciiTokenizer& token, phBound* bound)
{
	// base class load
	phArchetype::LoadData(token,bound);

	// mass
	if (token.CheckIToken("mass:"))
	{
		SetMassInternal(token.GetFloat());
	}
	else if (token.CheckIToken("density:"))
	{
		float density = token.GetFloat();

		if (m_Bound)
		{
			SetMassInternal(m_Bound->GetVolume()*density*1000.0f); // 1000 converts from g/cm^3 to kg/m^3
		}
	}
	else
	{
		if (m_Bound)
		{
			SetMassInternal(m_Bound->GetVolume()*sDefaultDensity*1000.0f); // 1000 converts from g/cm^3 to kg/m^3
		}
	}

	SetMassOnly(GetMassInternal());

	// See if the angular inertia is specified in the archetype file.
	if (token.CheckIToken("angInertia:"))
	{
		// Get the angular inertia from the file.
		Vec3V angInertia;
		token.GetVector(RC_VECTOR3(angInertia));
		SetAngInertiaInternal(angInertia);
	}
	else if (token.CheckIToken("inertiaBox:"))
	{
		// Get the inertia box from the file, and use it to calculate the angular inertia.
		Vector3 inertiaBox;
		token.GetVector(inertiaBox);
		float twelfthMass = GetMassInternal()*0.083333333333f;
		Vec3V angInertia = Vec3V(	twelfthMass*(square(inertiaBox.y)+square(inertiaBox.z)),
									twelfthMass*(square(inertiaBox.x)+square(inertiaBox.z)),
									twelfthMass*(square(inertiaBox.x)+square(inertiaBox.y)));
		SetAngInertiaInternal(angInertia);
	}
	else
	{
		if (m_Bound)
		{
			// Calculate the angular inertia.
			SetAngInertiaInternal(m_Bound->GetComputeAngularInertia(GetMassInternal()));
		}

		if (token.CheckIToken("inertiaScale:"))
		{
			// Scale the angular inertia.
			SetAngInertiaInternal(Scale(GetAngInertiaInternal(),ScalarVFromF32(token.GetFloat())));
		}
	}

	// Set the angular inertia, clamp it and find its inverse.
	SetAngInertia(GetAngInertiaInternal());

	if (token.CheckIToken("gravityFactor:"))
	{
		// Get the gravity factor from the file.
		m_GravityFactor = token.GetFloat();
	}
	if(token.CheckToken("buoyancyFactor:"))
	{
		m_BuoyancyFactor = token.GetFloat();
	}
}


#if (!__FINAL && !IS_CONSOLE) || (__BANK && IS_CONSOLE)
// Archetypes can be saved under the same conditions as bounds (!__FINAL && !IS_CONSOLE), and they
// can also be saved from widgets in console builds.

void phArchetypePhys::SaveData (fiAsciiTokenizer& token)
{
	phArchetype::SaveData(token);
	const float tolerance=0.01f;
	if(m_Bound)
	{
		float density=GetMassInternal()/m_Bound->GetVolume();
		float maxDensity=GetDefaultDensity();
		float minDensity=maxDensity*(1.0f-tolerance);
		maxDensity*=(1.0f+tolerance);
		if(density<minDensity || density>maxDensity)
		{
			// m_Mass is out of the default range, so save it.
			token.PutDelimiter("\nmass: ");
			token.Put(GetMassInternal());
		}
		Vector3 defaultAngInertia(VEC3V_TO_VECTOR3(m_Bound->GetComputeAngularInertia(GetMassInternal())));
		Vector3 currentAngInertia = VEC3V_TO_VECTOR3(GetAngInertiaInternal());
		if(currentAngInertia.x<defaultAngInertia.x*(1.0f-tolerance) || currentAngInertia.x>defaultAngInertia.x*(1.0f+tolerance)
			|| currentAngInertia.y<defaultAngInertia.y*(1.0f-tolerance) || currentAngInertia.y>defaultAngInertia.y*(1.0f+tolerance)
			|| currentAngInertia.z<defaultAngInertia.z*(1.0f-tolerance) || currentAngInertia.z>defaultAngInertia.z*(1.0f+tolerance))
		{
			// m_AngInertia is out of the default range, so save it.
			token.PutDelimiter("\nangInertia: ");
			token.Put(GetAngInertiaInternal());
		}
	}
	else
	{
		// There's no bound with which to do calculations, so save m_Mass and m_AngInertia.
		token.PutDelimiter("\nmass: ");
		token.Put(GetMassInternal());
		token.PutDelimiter("\nangInertia: ");
		token.Put(GetAngInertiaInternal());
	}
	float maxGravityFactor=GetDefaultGravityFactor();
	float minGravityFactor=maxGravityFactor*(1.0f-tolerance);
	maxGravityFactor*=(1.0f+tolerance);
	if(m_GravityFactor<minGravityFactor || m_GravityFactor>maxGravityFactor)
	{
		token.PutDelimiter("\ngravityFactor: ");
		token.Put(m_GravityFactor);
	}
	if(fabs(1.0f - m_BuoyancyFactor) > 0.001f)
	{
		token.PutDelimiter("buoyancyFactor: ");
		token.Put(m_BuoyancyFactor);
	}
}
#endif	// end of #if (!__FINAL && !IS_CONSOLE) || (__BANK && IS_CONSOLE)


void phArchetypePhys::CalcInvMass ()
{
	SetInvMassInternal((GetMassInternal()!=0.0f ? 1.0f/GetMassInternal() : 0.0f));
}


void phArchetypePhys::CalcInvAngInertia ()
{
	SetInvAngInertiaInternal(InvertSafe(GetAngInertiaInternal(),Vec3V(V_FLT_MAX)));
	Assertf(m_AngInertia.IsClose(m_AngInertia, SMALL_FLOAT), "Invalid inertia <%3.2f, %3.2f, %3.2f> of model %s", m_AngInertia.GetX(), m_AngInertia.GetY(), m_AngInertia.GetZ(), GetFilename());
}


void phArchetypePhys::CalcAngInertia ()
{
	FastAssert(m_Bound);
	SetAngInertia(m_Bound->GetComputeAngularInertia(GetMassInternal()));
}


void phArchetypePhys::SetMassOnly (float mass)
{
	SetMassInternal(mass);
	CalcInvMass();
}


void phArchetypePhys::SetMassOnly (ScalarV_In mass)
{
	SetMassInternal(mass);
	CalcInvMass();
}


void phArchetypePhys::SetMass (float mass)
{
	SetMassOnly(mass);
	if (m_Bound)
	{
		if (mass>0.0f)
		{
			CalcAngInertia();
		}
		else
		{
			SetAngInertia(Vec3V(V_ZERO));
		}
	}
}


void phArchetypePhys::SetMass (ScalarV_In mass)
{
	SetMassOnly(mass);
	if (m_Bound)
	{
		if (IsGreaterThanAll(mass,ScalarV(V_ZERO)))
		{
			CalcAngInertia();
		}
		else
		{
			SetAngInertia(ORIGIN);
		}
	}
}


void phArchetypePhys::SetGravityFactor (float gravityFactor)
{
	Assert(gravityFactor >= 0.0f && gravityFactor < 100.0f);
	m_GravityFactor = gravityFactor;
}


void phArchetypePhys::SetDensity (float density)
{
	FastAssert(m_Bound);
	SetMass(density*m_Bound->GetVolume());
}


void phArchetypePhys::ComputeCompositeAngInertia (const float* massList, const Vec3V* angInertiaList)
{
	Assert(m_Bound && m_Bound->GetType()==phBound::COMPOSITE);
	// The first argument of zero is density that is ignored when the mass list exists.
	SetAngInertia(static_cast<phBoundComposite*>((phBound*)m_Bound)->ComputeCompositeAngInertia(0.0f,massList,angInertiaList));
}


void phArchetypePhys::SetAngInertia (const Vector3 &ang)
{
	SetAngInertia(RCC_VEC3V(ang));
}


void phArchetypePhys::SetAngInertia (Vec3V_In angInertia)
{
	Vec3V clampedAngInertia = angInertia;
	phMathInertia::ClampAngInertia(RC_VECTOR3(clampedAngInertia));
	SetAngInertiaInternal(angInertia);
	CalcInvAngInertia();
}


void phArchetypePhys::GetInverseInertiaMatrix (Mat33V_In instMatrix, Mat33V_InOut invInertia) const
{
	phMathInertia::GetInverseInertiaMatrix(instMatrix,GetInvAngInertiaInternal().GetIntrin128(),invInertia);
}


void phArchetypePhys::CopyData (const phArchetype* original)
{
	phArchetype::CopyData(original);
	SetMassInternal(original->GetMass());
	SetInvMassInternal(original->GetInvMass());
	SetAngInertiaInternal(VECTOR3_TO_VEC3V(original->GetAngInertia()));
	SetInvAngInertiaInternal(VECTOR3_TO_VEC3V(original->GetInvAngInertia()));
	Assertf(m_AngInertia.IsClose(m_AngInertia, SMALL_FLOAT), "Invalid inertia <%3.2f, %3.2f, %3.2f> of model %s", m_AngInertia.GetX(), m_AngInertia.GetY(), m_AngInertia.GetZ(), GetFilename());
	m_GravityFactor = original->GetGravityFactor();
	Assert(m_GravityFactor >= 0.0f && m_GravityFactor < 100.0f);
	const phArchetypePhys* originalPhys = static_cast<const phArchetypePhys*>(original);
	m_BuoyancyFactor = originalPhys->GetBuoyancyFactor();
	m_MaxSpeed = original->GetMaxSpeed();
	m_MaxAngSpeed = original->GetMaxAngSpeed();
}

void phArchetypePhys::Copy (const phArchetype* original)
{
	Assert(original->GetClassType()>=ARCHETYPE_PHYS);
	phArchetype::Copy(original);
}


////////////////////////////////////////////////////////////////
// phArchetypeDamp

phArchetypeDamp::phArchetypeDamp ()
{
	m_Type = ARCHETYPE_DAMP;

	for(int typeIndex=0;typeIndex<NUM_DAMP_TYPES;typeIndex++)
	{
		m_DampingConstant[typeIndex].Zero();
	}
}


void phArchetypeDamp::Erase ()
{
	phArchetypePhys::Erase();

	for (int typeIndex=0; typeIndex<NUM_DAMP_TYPES; typeIndex++)
	{
		m_DampingConstant[typeIndex].Zero();
	}
}


void phArchetypeDamp::LoadData (fiAsciiTokenizer& token, phBound* bound)
{
	// base class load
	phArchetypePhys::LoadData(token,bound);

	Vector3 dampVect;

	if(token.CheckToken("damping"))
	{
		token.GetDelimiter("{");
		if(token.CheckToken("linearC:"))
		{
			token.GetVector(dampVect);
			ActivateDamping(phArchetypeDamp::LINEAR_C,dampVect);
		}
		if(token.CheckToken("linearV:"))
		{
			token.GetVector(dampVect);
			ActivateDamping(phArchetypeDamp::LINEAR_V,dampVect);
		}
		if(token.CheckToken("linearV2:"))
		{
			token.GetVector(dampVect);
			ActivateDamping(phArchetypeDamp::LINEAR_V2,dampVect);
		}
		if(token.CheckToken("angularC:"))
		{
			token.GetVector(dampVect);
			ActivateDamping(phArchetypeDamp::ANGULAR_C,dampVect);
		}
		if(token.CheckToken("angularV:"))
		{
			token.GetVector(dampVect);
			ActivateDamping(phArchetypeDamp::ANGULAR_V,dampVect);
		}
		if(token.CheckToken("angularV2:"))
		{
			token.GetVector(dampVect);
			ActivateDamping(phArchetypeDamp::ANGULAR_V2,dampVect);
		}
		token.GetDelimiter("}");
	}
}


#if (!__FINAL && !IS_CONSOLE) || (__BANK && IS_CONSOLE)
// Archetypes can be saved under the same conditions as bounds (!__FINAL && !IS_CONSOLE), and they
// can also be saved from widgets in console builds.

void phArchetypeDamp::SaveData (fiAsciiTokenizer& token)
{
	phArchetypePhys::SaveData(token);
	token.PutDelimiter("\ndamping {");
	if(m_DampingConstant[LINEAR_C].IsNonZero())
	{
		token.PutDelimiter("\n\tlinearC: ");
		token.Put(m_DampingConstant[LINEAR_C]);
	}
	if(m_DampingConstant[LINEAR_V].IsNonZero())
	{
		token.PutDelimiter("\n\tlinearV: ");
		token.Put(m_DampingConstant[LINEAR_V]);
	}
	if(m_DampingConstant[LINEAR_V2].IsNonZero())
	{
		token.PutDelimiter("\n\tlinearV2: ");
		token.Put(m_DampingConstant[LINEAR_V2]);
	}
	if(m_DampingConstant[ANGULAR_C].IsNonZero())
	{
		token.PutDelimiter("\n\tangularC: ");
		token.Put(m_DampingConstant[ANGULAR_C]);
	}
	if(m_DampingConstant[ANGULAR_V].IsNonZero())
	{
		token.PutDelimiter("\n\tangularV: ");
		token.Put(m_DampingConstant[ANGULAR_V]);
	}
	if(m_DampingConstant[ANGULAR_V2].IsNonZero())
	{
		token.PutDelimiter("\n\tangularV2: ");
		token.Put(m_DampingConstant[ANGULAR_V2]);
	}
	token.PutDelimiter("\n}\n");
}
#endif	// end of #if (!__FINAL && !IS_CONSOLE) || (__BANK && IS_CONSOLE)


void phArchetypeDamp::CopyData (const phArchetype* original)
{
	phArchetypePhys::CopyData(original);
	const phArchetypeDamp* originalDamp = static_cast<const phArchetypeDamp*>(original);
	for (int index=0;index<NUM_DAMP_TYPES;index++)
	{
		m_DampingConstant[index].Set(originalDamp->GetDampingConstant(index));
	}
}

void phArchetypeDamp::Copy (const phArchetype* original)
{
	Assert(original->GetClassType()>=ARCHETYPE_DAMP);
	phArchetypePhys::Copy(original);
}


#if __BANK

void phArchetype::AddWidgets(bkBank& bank)
{
	bank.PushGroup("Property",false);
	bank.AddSlider("Property Flags: ",&m_PropertyFlags,0,PROPERTY_FLAGS_ALL,1);
	bank.AddToggle("PRIORITY_PUSHER",&m_PropertyFlags,phArchetype::PROPERTY_PRIORITY_PUSHER);

	bank.AddButton("Save",datCallback(CFA1(phArchetype::SaveFromWidgets),this));
	bank.PopGroup();
}


void phArchetypePhys::AddWidgets(bkBank& bank)
{
	bank.PushGroup("Phys",false);
#if PHARCHETYPEPHYS_PACK_MASS_IN_ANG_INERTIA_W
	bank.AddSlider("m_Mass",&m_AngInertiaXYZMassW[3],0.0f,bkSlider::FLOAT_MAX_VALUE,10.0f,datCallback(CFA1(phArchetypePhys::WidgetSetMass),this));
	bank.AddSlider("m_AngInertia",(Vector3*)&m_AngInertiaXYZMassW,0.0f,bkSlider::FLOAT_MAX_VALUE,10.0f,datCallback(CFA1(phArchetypePhys::WidgetSetAngInertia),this));
#else // PHARCHETYPEPHYS_PACK_MASS_IN_ANG_INERTIA_W
	bank.AddSlider("m_Mass",&m_Mass,0.0f,bkSlider::FLOAT_MAX_VALUE,10.0f,datCallback(CFA1(phArchetypePhys::WidgetSetMass),this));
	bank.AddSlider("m_AngInertia",&m_AngInertia,0.0f,bkSlider::FLOAT_MAX_VALUE,10.0f,datCallback(CFA1(phArchetypePhys::WidgetSetAngInertia),this));
#endif // PHARCHETYPEPHYS_PACK_MASS_IN_ANG_INERTIA_W
	bank.AddSlider("m_GravityFactor",&m_GravityFactor,0.0f,100.0f,0.1f);
	bank.AddSlider("m_BuoyancyFactor",&m_BuoyancyFactor,0.0f,10.0f,0.01f);
	bank.AddSlider("m_MaxSpeed", &m_MaxSpeed,0.0f, 1000.0f, 0.1f);
	bank.PopGroup();

	phArchetype::AddWidgets(bank);
}


void phArchetypeDamp::AddWidgets(bkBank& bank)
{
	bank.PushGroup("Damping",false);
	const char* typeName[]={"Linear_Con","Linear_Vel","Linear_Vel2","Angular_Con","Angular_Vel","Angular_Vel2"};
	for(int index=0;index<NUM_DAMP_TYPES;index++)
	{
		bank.AddTitle(typeName[index]);
		//bank.AddToggle(typeName[index],(bool*)&m_DampingActive[index]);
		bank.AddSlider("damping",&m_DampingConstant[index],0.0f,100.0f,0.1f);
	}
	bank.PopGroup();

	phArchetypePhys::AddWidgets(bank);
}

#endif



#if !defined(RESOURCE_COMPILER)

void phArchetypeBase::VirtualConstructFromPtr (class datResource & rsc, phArchetypeBase *ArcheType)
{

	int type = ArcheType->m_Type;

	switch	(type)
	{
		case	phArchetype::ARCHETYPE:
			::new ((void *)ArcheType) phArchetype(rsc);
			break;

		case	phArchetype::ARCHETYPE_PHYS:
			::new ((void *)ArcheType) phArchetypePhys(rsc);
			break;

		case	phArchetype::ARCHETYPE_DAMP:
			::new ((void *)ArcheType) phArchetypeDamp(rsc);
			break;

		default:
			AssertMsg(0 , "phArchetypeBase::ResourcePageIn - unsupported or unknown phArchetype type");
			break;
	}
}

void phArchetype::Place(phArchetype *that,datResource &rsc)
{
	VirtualConstructFromPtr(rsc, that);
}

phArchetype::phArchetype (datResource &rsc) : phArchetypeBase(rsc)
{
}

#if __DECLARESTRUCT
void phArchetype::DeclareStruct(datTypeStruct &s)
{
	pgBase::DeclareStruct(s);
	STRUCT_BEGIN(phArchetype);
	STRUCT_FIELD(m_Type);
	STRUCT_FIELD(m_Filename);
	STRUCT_FIELD(m_Bound);
	STRUCT_FIELD(m_TypeFlags);
	STRUCT_FIELD(m_IncludeFlags);
	STRUCT_FIELD(m_PropertyFlags);
	STRUCT_FIELD(m_RefCount);
	// STRUCT_CONTAINED_ARRAY(m_Pad);
	STRUCT_END();
}
#endif // __DECLARESTRUCT

phArchetypePhys::phArchetypePhys (datResource &rsc) : phArchetype(rsc)
{
}

#if __DECLARESTRUCT
void phArchetypePhys::DeclareStruct(datTypeStruct &s)
{
	phArchetype::DeclareStruct(s);
	STRUCT_BEGIN(phArchetypePhys);
#if PHARCHETYPEPHYS_PACK_MASS_IN_ANG_INERTIA_W
	STRUCT_FIELD(m_AngInertiaXYZMassW);
	STRUCT_FIELD(m_InvAngInertiaXYZInvMassW);
#else // PHARCHETYPEPHYS_PACK_MASS_IN_ANG_INERTIA_W
	STRUCT_CONTAINED_ARRAY(m_ArchetypePhysPad);
	STRUCT_FIELD(m_Mass);
	STRUCT_FIELD(m_InvMass);
#endif // PHARCHETYPEPHYS_PACK_MASS_IN_ANG_INERTIA_W
	STRUCT_FIELD(m_GravityFactor);
	STRUCT_FIELD(m_MaxSpeed);
	STRUCT_FIELD(m_MaxAngSpeed);
	STRUCT_FIELD(m_BuoyancyFactor);
#if !PHARCHETYPEPHYS_PACK_MASS_IN_ANG_INERTIA_W
	STRUCT_FIELD(m_AngInertia);
	STRUCT_FIELD(m_InvAngInertia);
#endif // !PHARCHETYPEPHYS_PACK_MASS_IN_ANG_INERTIA_W
	STRUCT_END();
}
#endif // __DECLARESTRUCT

phArchetypeDamp::phArchetypeDamp (datResource &rsc) : phArchetypePhys(rsc)
{
}

#if __DECLARESTRUCT
void phArchetypeDamp::DeclareStruct(datTypeStruct &s)
{
	phArchetypePhys::DeclareStruct(s);
	STRUCT_BEGIN(phArchetypeDamp);
	STRUCT_FIELD(m_DampingConstant[0]);
	STRUCT_FIELD(m_DampingConstant[1]);
	STRUCT_FIELD(m_DampingConstant[2]);
	STRUCT_FIELD(m_DampingConstant[3]);
	STRUCT_FIELD(m_DampingConstant[4]);
	STRUCT_FIELD(m_DampingConstant[5]);
	STRUCT_END();
}
#endif // __DECLARESTRUCT

#endif
