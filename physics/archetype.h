//
// physics/archetype.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHYSICS_ARCHETYPE_H
#define PHYSICS_ARCHETYPE_H

#include "paging/owner.h"
#include "phbound/bound.h"
#include "phcore/config.h"
#include "system/bit.h"
#include "vector/vector3.h"
#include "vector/matrix33.h"

namespace rage {

// Defines for type flags, include flags and property flags - the flag sets that are all off
// are defined just to make them searchable in code.
#define	TYPE_FLAGS_ALL		0xFFFFFFFF
#define	INCLUDE_FLAGS_ALL	0xFFFFFFFF
#define	PROPERTY_FLAGS_ALL	0xFFFF
#define TYPE_FLAGS_NONE		0x00000000
#define INCLUDE_FLAGS_NONE	0x00000000
#define PROPERTY_FLAGS_NONE	0x0000

#if HACK_GTA4
	// 150m/s is 540kmph or approx 337mph - fast enough for GTA V given people can jump out of planes hurtling towards ground.
	// Want this to be as low as possible, to give the best chance of stopping things exploding, in a bad way
	// Is fine for this to be a game side option to change of course!
	#define	DEFAULT_MAX_SPEED		150.0f
#else
	#define	DEFAULT_MAX_SPEED		500.0f
#endif

// This could actually be ((2*PI) * MinFramesPerSec / 2)
// If we ignore visual craziness then the only real limit is the way we calculate rotational CCD interpolations
//	which means the maximum per frame rotation should be 180 degrees
//		(minus some epsilon if we want to remove ambiguity about what direction to take)
#define	DEFAULT_MAX_ANG_SPEED	2.0f*PI

class bkBank;
class fiAsciiTokenizer;
class phArchetypeMgr;

#if __SPU
#define PH_NON_SPU_VIRTUAL 
#define PH_NON_SPU_PURE_VIRTUAL
#define PH_NON_SPU_VIRTUAL_ONLY(X) X
#else
#define PH_NON_SPU_VIRTUAL virtual
#define PH_NON_SPU_PURE_VIRTUAL =0
#define PH_NON_SPU_VIRTUAL_ONLY(X)
#endif


///////////////////////////////////////////////////////////////
// phArchetypeBase
//

#define phArchetype_MAGIC										MAKE_MAGIC_NUMBER('A','R','C','0')

/* 
Purpose: Base class for phArchetype to ensure that virtual pointer is at the beginning of the structure
 (for resource writing). */
class phArchetypeBase : public pgBase
{
public:
			phArchetypeBase	() {};
			phArchetypeBase	(datResource & /*rsc*/) {};

	virtual ~phArchetypeBase ()									{ } // virtual destructor to force virtual pointer

	////////////////////////////////////////////////////////////
	// resources
	int GetResourceType() const { return m_Type; }

	static void VirtualConstructFromPtr (class datResource & rsc, phArchetypeBase *ArcheType);		// hides virtual construction as much as possible - jps...

protected:
	int	m_Type;

	private:
		static	class	datResourceRegister	sm_Register;
		static	void	ResourcePageIn	(class datResource &rsc);
};


///////////////////////////////////////////////////////////////
// phArchetype
//
// Each phArchetype contains a bound and flags for culling collision tests.
// Each phArchetypePhys also contains mass and angular inertia and their inverses.
// Each phArchetypeDamp also contains damping constants for active objects.
//

// PURPOSE
//   Holds the physical properties for any physical object.  It contains a pointer to a phBound (the physical
//   boundary of the object), type flags to specify the kind of object it is for collisions, include flags to specify
//   the kinds of objects it can collide with, a reference count to keep track of the number of physics instances
//   using this archetype, and a name for sharing, debugging and loading from resources.
// NOTES 
//   - phArchetype is only for physical objects that do not move; the derived class phArchetypePhys contains physical
//     properties used for motion.
//   - Games can derive their own physics archetype classes.
//   - Archetype information is stored in *.phys files, which by default are in the bound folder.
// <FLAG Component>

class phArchetype : public phArchetypeBase
{
public:
	////////////////////////////////////////////////////////////
	// archetype class type functions
	// Game-specific archetypes can derive from any of these classes.
	// Deriving from phArchetypeDamp is recommended because damping is useful and extra memory is minimal.
	enum { ARCHETYPE, ARCHETYPE_PHYS, ARCHETYPE_DAMP, ARCHETYPE_AGE_LAST=ARCHETYPE_DAMP };
	enum { INVALID_ARCHETYPE = -1 };
	// Get the class type.
	int GetClassType () const												{ return GetResourceType(); }
	// Get the class type name.
	const char* GetClassTypeString () const;
	static const char* sClassTypeString;						// name of this class type

	////////////////////////////////////////////////////////////
	// class functions
	// TypeFlags and IncludeFlags are all game-specific (never assigned in RAGE code except as defaults at initialization).
	// TypeFlags tell what type of object this is (prop, water, creature, player, vehicle, etc).
	// IncludeFlags tell what types of object this can collide with. TypeFlags of this and IncludeFlags of the other
	// and TypeFlags of the other and IncludeFlags of this must both match to get a collision.
	// Default TypeFlags are all off except for one bit, DEFAULT_TYPE, which is defined in RAGE only so that all objects
	// can collide by default. That bit can be used for another game-specific type.
	// Default IncludeFlags are all on.
	// PropertyFlags are used by AGE, and some are available for game use, to assign physical properties to objects,
	// such as whether or not they can have contact forces.

	static void SetDefaultTypeFlags (u32 flags)					{ sDefaultTypeFlags = flags; }

	// PURPOSE: static function to set the default type flags for all physics archetypes
	// PARAMS:
	//	mask - a u32 with bits turned on for the particular flag(s) to be modified
	//	value - boolean for the value to be given to the flag bits specified in mask
	static void SetDefaultTypeFlag (u32 mask, bool value);

	static void SetDefaultIncludeFlags (u32 flags)				{ sDefaultIncludeFlags = flags; }

	// PURPOSE: static function to set the default include flags for all physics archetypes
	// PARAMS:
	//	mask - a u32 with bits turned on for the particular flag(s) to be modified
	//	value - boolean for the value to be given to the flag bits specified in mask
	static void SetDefaultIncludeFlag (u32 mask, bool value);

	static void SetDefaultPropertyFlags (u16 flags)				{ sDefaultPropertyFlags = flags; }

	// PURPOSE: static function to set the default property flags for all physics archetypes
	// PARAMS:
	//	mask - a u8 with bits turned on for the particular flag(s) to be modified
	//	value - boolean for the value to be given to the flag bits specified in mask
	static void SetDefaultPropertyFlag (u16 mask, bool value);

	// PURPOSE:: static function to load archetype information from a file
	// PARAMS:
	//	fileName - the name of the file
	//	bound - optional pointer to the phBound to be used by the archetype
	// RETURN: a pointer to a new phArchetype; NULL if the load process failed
	static phArchetype* Load (const char* fileName, phBound* bound=NULL);

	// PURPOSE: static function to load archetype information from a tokenizer
	// PARAMS:
	//	token - tokenizer for the file that is being read
	//	bound - optional pointer to the phBound to be used by the archetype
	// RETURN: a pointer to a new phArchetype; NULL if the load process failed
	static phArchetype* Load (fiAsciiTokenizer& token, phBound* bound=NULL);

	// PURPOSE: Make a new phArchetype, phArchetypePhys or phArchetypeDamp according to the given type.
	// PARAMS:
	//	classType - tokenizer for the file that is being read
	// RETURN: a pointer to a new phArchetype; NULL if the given class type is invalid
	static phArchetype* CreateOfType (fiAsciiTokenizer& token);

	// PURPOSE: Make a new phArchetype, phArchetypePhys or phArchetypeDamp according to the specified type
	// PARAMS:
	//	int - the index number of the kind of archetype to create
	// RETURN: a pointer to a new phArchetype; NULL if the given class type is invalid
	static phArchetype* CreateOfType (int classType);

	////////////////////////////////////////////////////////////
	// phArchetype
	phArchetype ();												// constructor
	phArchetype	(datResource &rsc);
#if __DECLARESTRUCT
	PH_NON_SPU_VIRTUAL void DeclareStruct (datTypeStruct &s);
#endif // __DECLARESTRUCT
	DECLARE_PLACE(phArchetype);

	virtual ~phArchetype ();									// destructor

	// PURPOSE: Restore a phArchetype to its default (initialized) state.
	// NOTES:	When the reference count is reduced to zero, physics archetypes either delete themselves or call Erase().
	PH_NON_SPU_VIRTUAL void Erase ();

	// PURPOSE: Load archetype data from a tokenizer.
	// PARAMS:
	//	token - tokenizer for the file to be read
	//	bound - a pointer to the phBound to be used by the archetype.
	// NOTES:
	//	- This virtual function is overridden in derived phArchetype classes.
	//	- TypeFlags and IncludeFlags are optional.
	//	- If bound is NULL, the function will try to load a bound specified in the tokenizer.
	PH_NON_SPU_VIRTUAL void LoadData (fiAsciiTokenizer& token, phBound* bound);

#if (!__FINAL && !IS_CONSOLE) || (__BANK && IS_CONSOLE)
	/*
	Purpose: Save archetype data to a file tokenizer.
	Parameters: token - tokenizer for the file to be written.
	Notes:
	- Only non-default values are saved.
	- This virtual function is overridden in derived phArchetype classes. */
	PH_NON_SPU_VIRTUAL void SaveData (fiAsciiTokenizer & token);

	enum eBoundDestination { BOUND_DEFAULT, BOUND_FILE, BOUND_LOCAL };
																// destination of the bound data (BOUND_DEFAULT = use archetype filename, BOUND_LOCAL = put into archetype file, BOUND_FILE = put in file specified)
	// PURPOSE: Save physics archetype data to a rage_new file.
	// PARAMS:
	//	name - the name of the file to be written, dest - the destination for the bound
	//			(BOUND_DEFAULT = use archetype filename, BOUND_LOCAL = put into archetype file,
	//			 BOUND_FILE (not used) = put in file specified), boundFilename - not used.
	// RETURN: true if the save is successful, false if the file can not be opened for saving
	// NOTES:	Archetypes can be saved under the same conditions as bounds (!__FINAL && !IS_CONSOLE), and they
	//			can also be saved from widgets in console builds.
	bool Save (const char* name, eBoundDestination dest=BOUND_DEFAULT, const char* boundFilename=NULL);
																// save phArchetype to file
	// PURPOSE: Save physics archetype data to a file tokenizer.
	// PARAMS:
	//	token - tokenizer for the file to be written, dest - the destination for the bound
	//			(BOUND_DEFAULT = use archetype filename, BOUND_LOCAL = put into archetype file,
	//			BOUND_FILE (not used) = put in file specified), boundFilename - not used.
	// RETURN: true if the save is successful, false if dest==BOUND_FILE. 
	bool Save (fiAsciiTokenizer & token, eBoundDestination dest=BOUND_DEFAULT, const char* boundFilename=NULL);
#endif

	// PURPOSE: Copy this basic data of the physics archetype into the given physics archetype.
	// PARAMS:
	//	original - a pointer to the physics archetype from which this is to be copied
	PH_NON_SPU_VIRTUAL void CopyData (const phArchetype* original);

	// PURPOSE: Copy this physics archetype into the given physics archetype, and make a copy of the
	//			bound (rather than copying the bound pointer).
	// PARAMS:
	//	original - A pointer to the physics archetype from which this is to be copied.
	PH_NON_SPU_VIRTUAL void Copy (const phArchetype* original);

	// PURPOSE: Copy this physics archetype into the given physics archetype, and make a copy of the
	//			bound for the clone.
	// PARAMS:
	//	clone - A pointer to the physics archetype into which this is to be copied.
	PH_NON_SPU_VIRTUAL phArchetype* Clone () const;

	void operator=(const phArchetype& original)					{ Copy(&original); }

	////////////////////////////////////////////////////////////
	// reference counting
	void AddRef () const;

	// PURPOSE: Decrement the reference count of this archetype, and if the reference count goes to zero,
	//			either delete it or erase it.
	// PARAMS:
	//	deleteAtZero - true means the physics archetype will delete itself if its reference count goes
	//					to zero, false means it will zero itself if its reference count goes to zero.
	// RETURN: the remaining number of references.
	// NOTES:
	//	- Reference counting can be disabled, in which case this function does nothing.
	int Release (bool deleteOnZero=true);

	int GetRefCount () const;

	////////////////////////////////////////////////////////////
	// accessors
	phBound* GetBound () const;

	// PURPOSE: Set the physics bound pointer for this archetype.
	// PARAMS:
	//	bound - pointer to the phBound that will be used by this archetype.
	// NOTES:
	//	- The reference count of the new bound is incremented if the rage_new bound is not NULL.
	//	- The reference count of the old bound is decremented if the old bound is not NULL. */
	void SetBound (phBound* bound);

#if __SPU
	phBound** GetBoundPtr ();
#endif

	const char* GetFilename () const;
	void SetFilename (const char* name);

	// See above under "class functions" for an explanation of m_TypeFlags.
	u32 GetTypeFlags () const;
	void SetTypeFlags (u32 flags);
	
	u32 GetTypeFlag (u32 mask) const;

	// PURPOSE: Set one or more of the type flags.
	// PARAMS:
	//	mask - a u32 with bits turned on for the particular flag(s) to be modified.
	void AddTypeFlags (u32 mask);

	// PURPOSE: Unset one or more of the type flags.
	// PARAMS:
	//	mask - a u32 with bits turned on for the particular flag(s) to be modified.
	void RemoveTypeFlags (u32 mask);

	// See above under "class functions" for an explanation of m_IncludeFlags.
	u32 GetIncludeFlags () const;
	void SetIncludeFlags (u32 flags);
	u32 GetIncludeFlag (u32 mask) const;
	
	// PURPOSE: Set one or more of the include flags.
	// PARAMS:
	//	mask - a u32 with bits turned on for the particular flag(s) to be modified.
	void AddIncludeFlags (u32 mask);

	// PURPOSE: Unset one or more of the include flags.
	// PARAMS:
	//	mask - a u32 with bits turned on for the particular flag(s) to be modified.
	void RemoveIncludeFlags (u32 mask);

	// See above under "class functions" for an explanation of m_PropertyFlags.
	u16 GetPropertyFlags () const;
	void SetPropertyFlags (u16 flags);
	u16 GetPropertyFlag (u16 mask) const;
	
	// PURPOSE: Set one or more of the property flags.
	// PARAMS:
	//	mask - a u8 with bits turned on for the particular flag(s) to be modified
	//	value - boolean for the value to be given to the flag bits specified in mask
	void SetPropertyFlag (u16 mask, bool value=true);

	// PURPOSE: Tell whether the given include flags match this archetype's type flags, and the given type flags match this archetype's include
	//			flags (both must match to get a collision).
	// PARAMS:
	//	includeFlags - the include flags that must overlap this archetype's type flags
	//	typeFlags - the type flags that must overlap this archetype's include flags
	// RETURN: true if the given include flags match this archetype's type flags, and the given type flags match this archetype's include flags
	bool MatchFlags (u32 includeFlags, u32 typeFlags) const;

	////////////////////////////////////////////////////////////
	// physics interfaces
	// Default values are used in phArchetype to allow accessing without casting.
	// phArchetypePhys (for objects that can be physically active) stores values for the physical properties.
	PH_NON_SPU_VIRTUAL float GetMass () const								{ return 1.0f; }
	PH_NON_SPU_VIRTUAL float GetInvMass () const							{ return 1.0f; }
	PH_NON_SPU_VIRTUAL Vector3 GetAngInertia () const						{ return Vector3(VEC3_IDENTITY); }
	PH_NON_SPU_VIRTUAL Vector3 GetInvAngInertia () const					{ return Vector3(VEC3_IDENTITY); }
	PH_NON_SPU_VIRTUAL void SetMass (float )								{ }
	PH_NON_SPU_VIRTUAL void SetMass (ScalarV_In )							{ }
	PH_NON_SPU_VIRTUAL void SetAngInertia (const Vector3 &)					{ }
	PH_NON_SPU_VIRTUAL void SetAngInertia (Vec3V_In )					{ }
	PH_NON_SPU_VIRTUAL void SetGravityFactor (float )						{ }
	PH_NON_SPU_VIRTUAL float GetMaxSpeed () const { return DEFAULT_MAX_SPEED; }
	PH_NON_SPU_VIRTUAL float GetMaxAngSpeed () const { return DEFAULT_MAX_ANG_SPEED; }

	float GetGravityFactor () const;
	float GetGravityFactorImp () const						{ return 1.0f; }

	// Tell whether the archetype is a phArchetypeDamp.
	bool IsDamped () const									{ return GetResourceType() >= ARCHETYPE_DAMP; }

	////////////////////////////////////////////////////////////
	#if __BANK
	PH_NON_SPU_VIRTUAL void AddWidgets (bkBank & bank);
	static void SaveFromWidgets (phArchetype * archetype)		{ archetype->Save(archetype->m_Filename); }
	#endif

	////////////////////////////////////////////////////////////
	// property flags
	enum
	{
		PROPERTY_PRIORITY_PUSHER =	BIT(0),		// an object that cannot receive downward pushes (default is false)
		PROPERTY_UNUSED =			BIT(1),		// an object that can have contact forces, if contacts are enabled (default is true)
		PROPERTY_USER_0 =			BIT(2)		// first user defined type flag bit (remaining defaults are false)
	};

	// The default type is on by default for all archetypes so that any two objects will be collidable by default.
	// It can be used as some other type by projects that set type flags and include flags for all objects.
	enum { DEFAULT_TYPE = BIT(0) };

protected:
	////////////////////////////////////////////////////////////
	// class variables
	// See above under "class functions" for an explanation of the flags.
	static u32 sDefaultTypeFlags;								// the type flags for use at construction
	static u32 sDefaultIncludeFlags;							// the include flags for use at construction
	static u16 sDefaultPropertyFlags;							// the property flags for use at construction

	////////////////////////////////////////////////////////////
	// member variables
	datRef<const char> m_Filename;								// filename for the bank saving (can't be #if BANK'ed because of resources)
	pgOwner<phBound> m_Bound;									// the bound
	u32 m_TypeFlags;												// flags specifying the kind of archetype
	u32 m_IncludeFlags;											// flags specifying types of archetypes to interact with
	u16 m_PropertyFlags;											// flags specifying physical properties (uses contact forces, is priority pusher)

private:
	mutable u16 m_RefCount;										// number of references to this archetype

	PAD_FOR_GCC_X64_4;
} ;

#if !__SPU
inline void phArchetype::AddRef () const
{
	if (phConfig::IsRefCountingEnabled())
	{
		m_RefCount++;
	}
}

inline int phArchetype::GetRefCount () const
{
	return (phConfig::IsRefCountingEnabled()) ? m_RefCount : 1;
}
#endif // !__SPU

inline phBound* phArchetype::GetBound () const
{
	return m_Bound;
}

#if __SPU
inline phBound** phArchetype::GetBoundPtr ()
{
	return &m_Bound.ptr;
}
#endif

inline const char* phArchetype::GetFilename () const
{
	return m_Filename;
}

inline void phArchetype::SetFilename (const char* name)
{
	m_Filename = name;
}

inline u32 phArchetype::GetTypeFlags () const
{
	return m_TypeFlags;
}

inline void phArchetype::SetTypeFlags (u32 flags)
{
	m_TypeFlags = flags;
}

inline u32 phArchetype::GetTypeFlag (u32 mask) const
{
	return m_TypeFlags & mask;
}

inline u32 phArchetype::GetIncludeFlags () const
{
	return m_IncludeFlags;
}

inline void phArchetype::SetIncludeFlags (u32 flags)
{
	m_IncludeFlags = flags;
}

inline u32 phArchetype::GetIncludeFlag (u32 mask) const
{
	return m_IncludeFlags & mask;
}

inline u16 phArchetype::GetPropertyFlags () const
{
	return m_PropertyFlags;
}

inline void phArchetype::SetPropertyFlags (u16 flags)
{
	m_PropertyFlags = flags;
}

inline u16 phArchetype::GetPropertyFlag (u16 mask) const
{
	return (u16)(m_PropertyFlags & mask);
}

inline bool phArchetype::MatchFlags (u32 includeFlags, u32 typeFlags) const
{
	return ((includeFlags & m_TypeFlags) && (m_IncludeFlags & typeFlags));
}


////////////////////////////////////////////////////////////////
// phArchetypePhys
//

#define PHARCHETYPEPHYS_PACK_MASS_IN_ANG_INERTIA_W (0)

/* 
Purpose: Holds the physical properties for any movable physical object.  In addition to what is contained in its
 base class phArchetype, phArchetypePhys contains mass and angular inertia and their inverses, and a gravity scaling
 factor (by which the force from gravity is multiplied).
Notes: 
- phArchetypePhys is a base class for phArchetypeDamp, which contains additional information for damping motion.
- Games can derive their own physics archetype classes.
- Archetype information is stored in *.phys files, which by default are in the bound folder. 
<FLAG Component>
*/
class phArchetypePhys : public phArchetype
{
public:
	static const char* sClassTypeString;						// name of this class type

	phArchetypePhys ();											// constructor
	phArchetypePhys	(datResource &rsc);
#if __DECLARESTRUCT
	PH_NON_SPU_VIRTUAL void DeclareStruct(datTypeStruct &s);
#endif // __DECLARESTRUCT

	//	<COMBINE phArchetype::Erase>
	PH_NON_SPU_VIRTUAL void Erase ();

	//	<COMBINE phArchetype::LoadData>
	PH_NON_SPU_VIRTUAL void LoadData (fiAsciiTokenizer& token, phBound* bound);

#if (!__FINAL && !IS_CONSOLE) || (__BANK && IS_CONSOLE)
	PH_NON_SPU_VIRTUAL void SaveData (fiAsciiTokenizer& token);			// save phArchetypePhys data
#endif

	// PURPOSE: Calculate the inverse mass from the mass.
	void CalcInvMass ();

	// PURPOSE: Calculate the inverse angular inertia from the angular inertia.
	void CalcInvAngInertia ();

	// PURPOSE: Calculate the angular inertia from the mass.
	void CalcAngInertia ();

	// PURPOSE: Calculate the angular inertia with the given list of masses (for composite bounds only).
	// PARAMS:
	//	massList	- list of masses for the composite bound parts
	// NOTES:
	//		1.	This is only for physics archetypes that have composite bounds.
	void ComputeCompositeAngInertia (const float* massList, const Vec3V* angInertiaList);

	// PURPOSE: Set the mass without changing the angular inertia.
	// PARAMS:
	//	mass - the new mass
	void SetMassOnly (float mass);
	void SetMassOnly (ScalarV_In mass);

	// PURPOSE: Set the mass and angular inertia.
	// PARAMS:
	//	mass - the new mass
	PH_NON_SPU_VIRTUAL void SetMass (float mass);
	PH_NON_SPU_VIRTUAL void SetMass (ScalarV_In mass);

	// PURPOSE: Set the angular inertia (this does not change the mass).
	// PARAMS:
	//	angInertia - the new angular inertia
	PH_NON_SPU_VIRTUAL void SetAngInertia (const Vector3& angInertia);
	PH_NON_SPU_VIRTUAL void SetAngInertia (Vec3V_In angInertia);

	// PURPOSE: This is redundant with SetMass; remove when no longer in use.
	DEPRECATED void SetInertia (float mass) { SetMass(mass); }

	// Set the multiplier for gravity (to make objects fall faster or slower).
	PH_NON_SPU_VIRTUAL void SetGravityFactor (float gravityFactor);

	// Calculate the mass and angular inertia from the given density and the bound's volume.
	// PARAMS:
	//	density - the density of the object in kg/m^3
	// NOTES:	This requires the archetype to have a bound so that it can compute the volume distribution.
	void SetDensity (float density);

	// accessors for physical parameters
	PH_NON_SPU_VIRTUAL float GetMass () const										{ return GetMassInternal(); }
	PH_NON_SPU_VIRTUAL float GetInvMass () const									{ return GetInvMassInternal(); }
	PH_NON_SPU_VIRTUAL Vector3 GetAngInertia () const								{ return VEC3V_TO_VECTOR3(GetAngInertiaInternal()); }
	PH_NON_SPU_VIRTUAL Vector3 GetInvAngInertia () const							{ return VEC3V_TO_VECTOR3(GetInvAngInertiaInternal()); }
	
	// PURPOSE: Calculate the physics archetype's inverse angular inertia matrix.
	// PARAMS:
	//	instMatrix	- the local coordinate matrix of the archetype's instance
	//	invInertia	- pointer to a matrix in which to write the inverse inertia matrix
	void GetInverseInertiaMatrix (Mat33V_In instMatrix, Mat33V_InOut invInertia) const;

	float GetGravityFactorImp () const												{ return m_GravityFactor; }

	static float GetDefaultDensity ()							{ return sDefaultDensity; }
	static void SetDefaultDensity (float defaultDensity)		{ sDefaultDensity = defaultDensity; }

	static float GetDefaultGravityFactor ()						{ return sDefaultGravityFactor; }
	static void SetDefaultGravityFactor (float defaultGravityFactor) { sDefaultGravityFactor = defaultGravityFactor; }

	// PURPOSE: Get the maximum speed.
	// RETURN:	the maximum speed of this object
	// NOTES:	This is used to set the maximum speed of the collider when this object becomes active.
	PH_NON_SPU_VIRTUAL float GetMaxSpeed () const;

	// PURPOSE: Get the maximum angular speed.
	// RETURN:	the maximum angular speed of this object
	// NOTES:	This is used to set the maximum angular speed of the collider when this object becomes active.
	PH_NON_SPU_VIRTUAL float GetMaxAngSpeed () const;

	// PURPOSE: Set the maximum speed.
	// PARAMS:
	//	maxSpeed - the maximum speed of this object
	void SetMaxSpeed (float maxSpeed);

	// PURPOSE: Set the maximum angular speed.
	// PARAMS:
	//	maxSpeed - the maximum angular speed of this object
	void SetMaxAngSpeed (float maxAngSpeed);

	// PURPOSE: Get the buoyancy factor.
	// RETURN: 
	float GetBuoyancyFactor() const;

	void SetBuoyancyFactor(const float buoyancyFactor);

	// <COMBINE phArchetype::CopyData>
	PH_NON_SPU_VIRTUAL void CopyData (const phArchetype* original);

	// PURPOSE: Copy this physics archetype into the given physics archetype, and make a copy of the
	//			bound (rather than copying the bound pointer).
	// PARAMS:
	//	original	- A pointer to the physics archetype from which this is to be copied
	PH_NON_SPU_VIRTUAL void Copy (const phArchetype* original);

	#if __BANK
	PH_NON_SPU_VIRTUAL void AddWidgets(bkBank& bank);
	static void WidgetSetMass(phArchetypePhys* archPhys) { archPhys->SetMass(archPhys->GetMass()); }
	static void WidgetSetAngInertia(phArchetypePhys* archPhys) { archPhys->SetAngInertia(archPhys->GetAngInertia()); }
	#endif

protected:
#if PHARCHETYPEPHYS_PACK_MASS_IN_ANG_INERTIA_W
	__forceinline void SetMassInternal(float mass) { m_AngInertiaXYZMassW.SetWf(mass); }
	__forceinline void SetInvMassInternal(float invMass) { m_InvAngInertiaXYZInvMassW.SetWf(invMass); }
	__forceinline void SetMassInternal(ScalarV_In mass) { m_AngInertiaXYZMassW.SetW(mass); }
	__forceinline void SetInvMassInternal(ScalarV_In invMass) { m_InvAngInertiaXYZInvMassW.SetW(invMass); }
	__forceinline void SetAngInertiaInternal(Vec3V_In angInertia) { m_AngInertiaXYZMassW.SetXYZ(angInertia); }
	__forceinline void SetInvAngInertiaInternal(Vec3V_In invAngInertia) { m_InvAngInertiaXYZInvMassW.SetXYZ(invAngInertia); }

	__forceinline float GetMassInternal() const { return m_AngInertiaXYZMassW.GetWf(); }
	__forceinline float GetInvMassInternal() const { return m_InvAngInertiaXYZInvMassW.GetWf(); }
	__forceinline ScalarV_Out GetMassVInternal() const { return m_AngInertiaXYZMassW.GetW(); }
	__forceinline ScalarV_Out GetInvMassVInternal() const { return m_InvAngInertiaXYZInvMassW.GetW(); }
	__forceinline Vec3V_Out GetAngInertiaInternal() const { return m_AngInertiaXYZMassW.GetXYZ(); }
	__forceinline Vec3V_Out GetInvAngInertiaInternal() const { return m_InvAngInertiaXYZInvMassW.GetXYZ(); }
#else // PHARCHETYPEPHYS_PACK_MASS_IN_ANG_INERTIA_W
	__forceinline void SetMassInternal(float mass) { m_Mass = mass; }
	__forceinline void SetInvMassInternal(float invMass) { m_InvMass = invMass; }
	__forceinline void SetMassInternal(ScalarV_In mass) { m_Mass = mass.Getf(); }
	__forceinline void SetInvMassInternal(ScalarV_In invMass) { m_InvMass = invMass.Getf(); }
	__forceinline void SetAngInertiaInternal(Vec3V_In angInertia) { RC_VEC3V(m_AngInertia) = angInertia; }
	__forceinline void SetInvAngInertiaInternal(Vec3V_In invAngInertia) { RC_VEC3V(m_InvAngInertia) = invAngInertia; }

	__forceinline float GetMassInternal() const { return m_Mass; }
	__forceinline float GetInvMassInternal() const { return m_InvMass; }
	__forceinline ScalarV_Out GetMassVInternal() const { return ScalarVFromF32(m_Mass); }
	__forceinline ScalarV_Out GetInvMassVInternal() const { return ScalarVFromF32(m_InvMass); }
	__forceinline Vec3V_Out GetAngInertiaInternal() const { return RCC_VEC3V(m_AngInertia); }
	__forceinline Vec3V_Out GetInvAngInertiaInternal() const { return RCC_VEC3V(m_InvAngInertia); }
#endif // PHARCHETYPEPHYS_PACK_MASS_IN_ANG_INERTIA_W

protected:
	// PURPOSE: default density for objects
	static float sDefaultDensity;

	// PURPOSE: default gravity factor for objects
	static float sDefaultGravityFactor;

#if PHARCHETYPEPHYS_PACK_MASS_IN_ANG_INERTIA_W
	Vec4V m_AngInertiaXYZMassW;
	Vec4V m_InvAngInertiaXYZInvMassW;
#else // PHARCHETYPEPHYS_PACK_MASS_IN_ANG_INERTIA_W
	u8 m_ArchetypePhysPad[8];

	// PURPOSE: mass of the bound (set or calculated from density and bound shape)
	float m_Mass;

	// PURPOSE: 1.0f / m_Mass
	float m_InvMass;
#endif

	// PURPOSE: a factor by which you multiply the global gravity to get the gravity for this object
	float m_GravityFactor;

	// PURPOSE: the maximum speed of this object
	float m_MaxSpeed;

	// PURPOSE: the maximum angular speed of this object
	float m_MaxAngSpeed;

	// PURPOSE: represent the degree to which buoyant forces should be scaled to compensate for the bound + mass not accurately representing
	//   the object for the purposes of buoyancy (for example, you might use a box bound to represent a cage)
	float m_BuoyancyFactor;

#if !PHARCHETYPEPHYS_PACK_MASS_IN_ANG_INERTIA_W
	// PURPOSE:the angular inertia vector (set or calculated from mass and bound shape)
	Vector3 m_AngInertia;

	// PURPOSE: component inverse of m_AngInertia
	Vector3 m_InvAngInertia;
#endif // !PHARCHETYPEPHYS_PACK_MASS_IN_ANG_INERTIA_W
};


inline float phArchetypePhys::GetMaxSpeed () const
{
	return m_MaxSpeed;
}


inline float phArchetypePhys::GetMaxAngSpeed () const
{
	return m_MaxAngSpeed;
}


inline void phArchetypePhys::SetMaxSpeed (float maxSpeed)
{
	m_MaxSpeed = maxSpeed;
}


inline void phArchetypePhys::SetMaxAngSpeed (float maxAngSpeed)
{
	m_MaxAngSpeed = maxAngSpeed;
}

inline float phArchetypePhys::GetBuoyancyFactor() const
{
	return m_BuoyancyFactor;
}

inline void phArchetypePhys::SetBuoyancyFactor(const float buoyancyFactor)
{
	m_BuoyancyFactor = buoyancyFactor;
}



////////////////////////////////////////////////////////////////
// phArchetypeDamp
//
// The phArchetypeDamp class provides a simple mechanism for damping the linear
// and angular velocities of a collider independently in all 6 degrees of
// freedom. It allows the damping function to be specified with 3
// coefficients: velocity squared, velocity, and constant. The damping
// equation for each axis is of the form:
//
//		damp = V2*vel*vel + V*vel + C;
//
// Three coefficients are specified for each of the 6 degrees of freedom
// making 18 in all. Damping is scaled by the mass or angular momentum
// of the axis it applies to, so the same coefficients can be used for
// different sized objects.
//
// The recommended process for tuning damping is to first tune the
// V2 components, as they affect the top speed of the collider the most. The
// V component is used mainly to tune how the object slows down at low
// speeds, and the C component is most useful for tuning how the object
// comes to rest.
//

/* 
Purpose: Holds the physical properties for any movable physical object with motion damping.  In addition to what
 is contained in its base class phArchetypePhys, phArchetypeDamp contains a set of vectors to damp linear and
 angular motion.
Notes: 
- Games can derive their own physics archetype classes.
- Archetype information is stored in *.phys files, which by default are in the bound folder. 
<FLAG Component>
*/
BANK_ONLY(class phMouseInput;)
class phArchetypeDamp : public phArchetypePhys
{
BANK_ONLY(friend class phMouseInput;)
public:
	// Types of motion damping - constant (_C), proportional to velocity (_V), and proportional to velocity squared (_V2)
	// for linear and angular motion.
	enum MotionDampingType
	{
		LINEAR_C, LINEAR_V, LINEAR_V2, ANGULAR_C, ANGULAR_V, ANGULAR_V2
	};
	enum { NUM_DAMP_TYPES = ANGULAR_V2 + 1 };

	static const char* sClassTypeString;						// name of this class type

	phArchetypeDamp ();											// constructor
	phArchetypeDamp	(datResource &rsc);
#if __DECLARESTRUCT
	PH_NON_SPU_VIRTUAL void DeclareStruct(datTypeStruct &s);
#endif // __DECLARESTRUCT

	//	<COMBINE phArchetype::Erase>
	PH_NON_SPU_VIRTUAL void Erase ();

	//	<COMBINE phArchetype::LoadData>
	PH_NON_SPU_VIRTUAL void LoadData (fiAsciiTokenizer& token, phBound* bound);

#if (!__FINAL && !IS_CONSOLE) || (__BANK && IS_CONSOLE)
	// PURPOSE: Save physics archetype data to a file tokenizer.
	// PARAMS:
	//	token - tokenizer for the file to be written.
	// NOTES: - Only values farther than 1% from the default values are saved.
	PH_NON_SPU_VIRTUAL void SaveData (fiAsciiTokenizer& token);
#endif

	// PURPOSE:
	//	Turn on motion damping of the given type with the given values.
	void ActivateDamping (int type, const Vector3& constant);

	// PURPOSE:
	//	Get the damping values for the given type.
	const Vector3& GetDampingConstant (int type) const;

	// <COMBINE phArchetype::CopyData>
	PH_NON_SPU_VIRTUAL void CopyData (const phArchetype* original);

	// <COMBINE phArchetype::Copy>
	PH_NON_SPU_VIRTUAL void Copy (const phArchetype* original);

	#if __BANK
	PH_NON_SPU_VIRTUAL void AddWidgets(bkBank& bank);
	static void WidgetDampingActive(void* archDamp);
	#endif

protected:
	Vector3 m_DampingConstant[NUM_DAMP_TYPES];
};

inline float phArchetype::GetGravityFactor () const
{
	switch(GetClassType())
	{
	default:
		Assert(0); // intentional fall though
	case ARCHETYPE: 
		return GetGravityFactorImp();
	case ARCHETYPE_PHYS:
		return static_cast<const phArchetypePhys*>(this)->GetGravityFactorImp();
	case ARCHETYPE_DAMP:
#if HACK_GTA4
	case 3: // ARCHETYPE_GTA:
#endif
		return static_cast<const phArchetypeDamp*>(this)->GetGravityFactorImp();	
	}
}

inline void phArchetypeDamp::ActivateDamping (int type, const Vector3& constant)	
{ 
	m_DampingConstant[type].Set(constant); 
}

inline const Vector3& phArchetypeDamp::GetDampingConstant (int type) const
{ 
	return m_DampingConstant[type]; 
}

} // namespace rage

#endif // end of #ifndef PHBASE_ARCHETYPE_H
