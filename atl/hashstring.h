//
// atl/hashstring.h
//
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved.
//

#ifndef ATL_HASHSTRING_H
#define ATL_HASHSTRING_H

#include "string/stringhash.h"

#include <string.h>

// Output format helper
// Use like Displayf("This hash value is " HASHFMT ", which is cool with me.", HASHOUT(hashValue));
// Note that the spaces around HASHFMT are VERY IMPORTANT for the finicky PS compilers.

#define HASHFMT "%s (0x%08x)"
#define HASHOUT(x) (x).TryGetCStr(), (x).GetHash()

namespace rage {


class datResource;

struct atHashStringStats
{
	u32		m_refdStringCount;
	u32		m_refdStringChars;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// Procedure for adding new namespaces:
//
// 1) Add an enum to atHashStringNamespaces
// 2) Add typedefs with nicer names than atNamespacedHashString<HSNS_FOO> and atNamespacedHashValue(HSNS_FOO) (see bottom of file)
// 3) In the .cpp file, edit InitNamespaces and set up your new namespace's properties
// 4) Add the new namespace to %RS_TOOLSROOT%\bin\coding\python\namespaces.txt
// 5) Update the %RS_TOOLSROOT%\script\coding\autoexp\* files to use the new namespace.
// 6) After checking in, tell everyone to re-run %RS_TOOLSROOT%\script\coding\autoexp\InstallAutoExpDotDat.bat

enum atHashStringNamespaces
{
	HSNS_ATHASHSTRING,
	HSNS_ATFINALHASHSTRING,
	HSNS_ATLITERALHASHSTRING,
	HSNS_DIAGSTRING,			// Strings for diagnostic messages - typically dynamically generated
	HSNS_STATNAMESTRING,		// Strings for multiplayer stats

	HSNS_NUM_NAMESPACES,

	HSNS_INVALID = 255,
	HSNS_MAX_NAMESPACES = 256,
};
CompileTimeAssert(HSNS_NUM_NAMESPACES < HSNS_MAX_NAMESPACES);

namespace atHashStringNamespaceSupport
{
	extern void InitNamespaces();
	extern void ShutdownNamespaces();

	extern void AddString(atHashStringNamespaces nameSpace, const u32 hash, const char* str);
	extern u32 ComputeHash(atHashStringNamespaces nameSpace, const char* str);
	extern const char* GetString(atHashStringNamespaces nameSpace, const u32 hash);
	extern const char* TryGetString(atHashStringNamespaces nameSpace, const u32 hash);
	extern atHashStringStats FindStats(atHashStringNamespaces nameSpace);
	extern const char* GetNamespaceName(atHashStringNamespaces nameSpace);
	extern const char* GetNamespaceValueName(atHashStringNamespaces nameSpace);

#if __BANK
	extern void SaveAllHashStrings();
#endif

	// PURPOSE: For .cpp files that could potentially do static initialization before hashstring.cpp does, they need to
	// create a static instance of this class. For example: namespace { atHashStringNamespaceSupport g_InitNamespaces; }
	struct EarlyInit
	{
		EarlyInit() { InitNamespaces(); }
		~EarlyInit() { ShutdownNamespaces(); }
	};
}

// These are just dummy base classes that are here to DISallow some conversions between unrelated types
class atNamespacedHashStringBase
{
public:
	// Functions that are in common with hash value
	void SetHash(const u32 hash)					{ m_hash = hash; }
	u32 GetHash() const								{ return m_hash; }

	void Clear()									{ m_hash = 0; }

	bool IsNull() const								{ return m_hash == 0; }
	bool IsNotNull() const							{ return m_hash != 0; }

protected:
	u32		m_hash;
};

class atNamespacedHashValueBase
{
public:
	// Functions that are in common with hash string
	void SetHash(const u32 hash)					{ m_hash = hash; }
	u32 GetHash() const								{ return m_hash; }

	void Clear()									{ m_hash = 0; }

	bool IsNull() const								{ return m_hash == 0; }
	bool IsNotNull() const							{ return m_hash != 0; }

protected:
	u32		m_hash;
};

template<atHashStringNamespaces Namespace> class atNamespacedHashValue;

template<atHashStringNamespaces Namespace>
class atNamespacedHashString : public atNamespacedHashStringBase
{
	typedef atNamespacedHashStringBase Base;
public:
	atNamespacedHashString()								{m_hash = 0;}
	atNamespacedHashString(const atNamespacedHashString<Namespace>& other) {m_hash = other.m_hash;}
	explicit atNamespacedHashString(const u32 hash)			{m_hash = hash;}
	explicit atNamespacedHashString(const char* str)		{ SetFromString(str); }
	explicit atNamespacedHashString(datResource&)			{ }
	atNamespacedHashString(const char* str, const u32 hash)	{ SetHashWithString(hash, str); }

	void SetFromString(const char* const str)		{ m_hash = atHashStringNamespaceSupport::ComputeHash( Namespace, str ); atHashStringNamespaceSupport::AddString( Namespace, m_hash, str ); }

	void operator=(const atNamespacedHashString<Namespace>& op) { m_hash = op.m_hash; }
	void operator=(const char* str ) { SetFromString(str); }

	bool operator ==(const atNamespacedHashString<Namespace>& op) const			{ return m_hash == op.m_hash; }
	bool operator ==(const char* op) const			{ return m_hash == atHashStringNamespaceSupport::ComputeHash( Namespace, op ); }
	bool operator ==(const atNamespacedHashValue<Namespace>& op) const;

	bool operator !=(const atNamespacedHashString<Namespace>& op) const			{ return m_hash != op.m_hash; }
	bool operator !=(const char* op) const			{ return m_hash != atHashStringNamespaceSupport::ComputeHash( Namespace, op ); }
	bool operator !=(const atNamespacedHashValue<Namespace>& op) const;

	static atNamespacedHashString Null()			{ return atNamespacedHashString(); }
	static u32 ComputeHash(const char* str)			{ return atHashStringNamespaceSupport::ComputeHash(Namespace, str); }
	static int GetNamespaceIndex()					{ return Namespace; }

	// Functions that are unique to hash strings

	void SetHashWithString(const u32 hash, const char* str);

	const char* GetCStr() const					{ return atHashStringNamespaceSupport::GetString(Namespace, m_hash); }
	const char* TryGetCStr() const				{ return atHashStringNamespaceSupport::TryGetString( Namespace, m_hash ); }
	
	u32 GetLength() const						{ const char* str = GetCStr(); return str ? ( (u32) strlen(str) ) : 0; }
	
	static const char* TryGetString(const u32 hash) { return atHashStringNamespaceSupport::TryGetString( Namespace, hash ); }
	static atHashStringStats GetStats() { return atHashStringNamespaceSupport::FindStats(Namespace); }
private:
	// These are NOT ALLOWED:
	atNamespacedHashString(const atNamespacedHashStringBase& other); // Error here means you might be trying to convert a hash name from one namespace to another
	atNamespacedHashString(const atNamespacedHashValueBase& other);  // Are you converting a hash VALUE into a hash STRING? Need to do an explicit conversion via u32s for that.
	atNamespacedHashString(const class atHashValue& other);			 // Are you converting a hash VALUE into a hash STRING? Need to do an explicit conversion via u32s for that.

};


template<atHashStringNamespaces Namespace>
class atNamespacedHashValue : public atNamespacedHashStringBase
{
public:
	atNamespacedHashValue()							{m_hash = 0;}
	atNamespacedHashValue(const atNamespacedHashValue<Namespace>& other) {m_hash = other.m_hash; }
	atNamespacedHashValue(const atNamespacedHashString<Namespace>& other) {m_hash = other.GetHash(); } // Implicit conversion from string to value
	explicit atNamespacedHashValue(const u32 hash)	{m_hash = hash;}
	explicit atNamespacedHashValue(const char* str) { SetFromString(str); }
	explicit atNamespacedHashValue(datResource&) {}
	atNamespacedHashValue(const char* ASSERT_ONLY(str), const u32 hash)	{ Assertf(hash == atHashStringNamespaceSupport::ComputeHash( Namespace, str ), "atNamespacedHashValue - hash for %s isn't %u in namespace %s", str, hash, atHashStringNamespaceSupport::GetNamespaceName(Namespace)); m_hash = hash; }

	void SetFromString(const char* str)				{ m_hash = atHashStringNamespaceSupport::ComputeHash( Namespace, str ); }

	void operator=(const atNamespacedHashValue<Namespace>& op) { m_hash = op.m_hash; }
	void operator=(const atNamespacedHashString<Namespace>& op) { m_hash = op.GetHash(); }
	void operator=(const char* str ) { SetFromString(str); }

	bool operator ==(const atNamespacedHashValue<Namespace>& op) const			{ return m_hash == op.m_hash; }
	bool operator ==(const char* op) const			{ return m_hash == atHashStringNamespaceSupport::ComputeHash( Namespace, op ); }
	bool operator !=(const atNamespacedHashValue<Namespace>& op) const			{ return m_hash != op.m_hash; }
	bool operator !=(const char* op) const			{ return m_hash != atHashStringNamespaceSupport::ComputeHash( Namespace, op ); }

	static atNamespacedHashValue Null()				{ return atNamespacedHashValue(); }
	static u32 ComputeHash(const char* str)			{ return atHashStringNamespaceSupport::ComputeHash(Namespace, str); }
	static int GetNamespaceIndex()					{ return Namespace; }

private:
	// These are NOT ALLOWED
	atNamespacedHashValue(const atNamespacedHashStringBase& other);
	atNamespacedHashValue(const atNamespacedHashValueBase& other);
};

template<atHashStringNamespaces Namespace> bool atNamespacedHashString<Namespace>::operator ==(const atNamespacedHashValue<Namespace>& op) const { return GetHash() == op.GetHash(); }
template<atHashStringNamespaces Namespace> bool atNamespacedHashString<Namespace>::operator !=(const atNamespacedHashValue<Namespace>& op) const { return GetHash() != op.GetHash(); }

template<atHashStringNamespaces Namespace> void atNamespacedHashString<Namespace>::SetHashWithString(const u32 hash, const char* str)
{
	Assertf(hash == atHashStringNamespaceSupport::ComputeHash( Namespace, str ), "atNamespacedHashValue - was expecting hash 0x%08x instead of 0x%08x for %s in namespace %d %s", atHashStringNamespaceSupport::ComputeHash( Namespace, str ), hash, str, Namespace, atHashStringNamespaceSupport::GetNamespaceName(Namespace)); 
	m_hash = hash; 
	atHashStringNamespaceSupport::AddString( Namespace, m_hash, str ); 
}

#if !__FINAL

// This is a subclass and not a typedef because we need to add some new implicit conversions for backwards compatibility reasons
class atNonFinalHashString : public atNamespacedHashString<HSNS_ATHASHSTRING>
{
	typedef atNamespacedHashString<HSNS_ATHASHSTRING> Base;
public:
	// Boilerplate code to make things still work
	atNonFinalHashString()									: Base() {}
	atNonFinalHashString(const Base& other)					: Base(other) {} 
	explicit atNonFinalHashString(datResource& rsc)			: Base(rsc) {}
	atNonFinalHashString(const char* str, const u32 hash)	: Base(str, hash) {}

	using Base::operator =;
	using Base::operator ==;
	using Base::operator !=;

	static atNonFinalHashString Null()						{ return atNonFinalHashString(); }

	// Unique stuff for this subclass
	/*implicit!!!*/ atNonFinalHashString(const u32 hash)	: Base(hash) {}
	/*implicit!!!*/ atNonFinalHashString(const char* str)	: Base(str) {}
	explicit atNonFinalHashString(const class atHashValue&);

	void operator =(const u32 op)							{ SetHash(op); }
	void operator =(const class atHashValue& op);

	bool operator ==(const u32 op) const					{ return GetHash() == op; }
	bool operator ==(const int op) const					{ return GetHash() == (u32)op; }
	bool operator ==(const class atFinalHashString& op) const;

	bool operator !=(const u32 op) const 					{ return GetHash() != op; }
	bool operator !=(const int op) const 					{ return GetHash() != (u32)op; }
	bool operator !=(const class atFinalHashString& op) const;

	operator u32() const									{ return GetHash(); }
};
#endif	//	!__FINAL

// This is a subclass and not a typedef because we need to add some new implicit conversions for backwards compatibility reasons
class atFinalHashString : public atNamespacedHashString<HSNS_ATFINALHASHSTRING>
{
	typedef atNamespacedHashString<HSNS_ATFINALHASHSTRING> Base;
public:
	// Boilerplate code to make things still work
	atFinalHashString()									: Base() {}
	atFinalHashString(const Base& other)				: Base(other) {} 
	explicit atFinalHashString(const u32 hash)			: Base(hash) {}
	explicit atFinalHashString(const char* str)			: Base(str) {}
	explicit atFinalHashString(datResource& rsc)		: Base(rsc) {}
	atFinalHashString(const char* str, const u32 hash)	: Base(str, hash) {}

	using Base::operator =;
	using Base::operator ==;
	using Base::operator !=;

	static atFinalHashString Null()					{ return atFinalHashString(); }

	// Unique stuff for this subclass
	explicit atFinalHashString(const class atHashValue&);

	operator u32() const							{ return GetHash(); }

	bool operator ==(const u32 op) const					{ return GetHash() == op; }
	bool operator ==(const int op) const					{ return GetHash() == (u32)op; }
	bool operator ==(const class atHashValue& op) const;

	bool operator !=(const u32 op) const 					{ return GetHash() != op; }
	bool operator !=(const int op) const 					{ return GetHash() != (u32)op; }
	bool operator !=(const class atHashValue& op) const;

#if !__FINAL
	bool operator ==(const class atNonFinalHashString& op) const;
	bool operator !=(const class atNonFinalHashString& op) const;
#endif
};

class atHashValue : public atNamespacedHashValue<HSNS_ATHASHSTRING>
{
	typedef atNamespacedHashValue<HSNS_ATHASHSTRING> Base;
public:
	// Boilerplate code to make things still work
	atHashValue()										: Base() {}
	atHashValue(const Base& other)						: Base(other) {}
	atHashValue(datResource& rsc)						: Base(rsc) { }
	atHashValue(const char* const str, const u32 hash)	: Base(str, hash) {}

	using Base::operator =;
	using Base::operator ==;
	using Base::operator !=;

	static atHashValue Null()							{ return atHashValue(); }

	// Unique stuff for this subclass
	/*implicit!!!*/	atHashValue(const u32 hash)			: Base(hash) {}
	/*implicit!!!*/ atHashValue(const char* const str)	: Base(str) {}
	operator u32() const								{ return GetHash(); }

	atHashValue(const atFinalHashString& other)			: Base(other.GetHash()) {}

	void operator =(const u32 op)							{ SetHash(op); }
	void operator =(const atFinalHashString& other)			{ SetHash(other.GetHash()); }

	bool operator ==(const u32 op) const					{ return GetHash() == op; }
	bool operator ==(const int op) const					{ return GetHash() == (u32)op; }
	bool operator ==(const atFinalHashString& op) const		{ return GetHash() == op.GetHash(); }

	bool operator !=(const u32 op) const 					{ return GetHash() != op; }
	bool operator !=(const int op) const 					{ return GetHash() != (u32)op; }
	bool operator !=(const atFinalHashString& op) const		{ return GetHash() != op.GetHash(); }

#if !__FINAL
	atHashValue(const atNonFinalHashString& other)			: Base(other) {}
	void operator =(const atNonFinalHashString& other)		{ SetHash(other.GetHash()); }
	bool operator ==(const atNonFinalHashString& op) const	{ return GetHash() == op.GetHash(); }
	bool operator !=(const atNonFinalHashString& op) const	{ return GetHash() != op.GetHash(); }
#endif	//	!__FINAL

	const char* TryGetCStr() const									{ return NULL; }
	static const char* TryGetString(const u32 UNUSED_PARAM(hash)) 	{ return NULL; }

#if __FINAL_LOGGING
	const char* GetCStr() const										{ return "FINAL_LOGGING"; }
#endif
};

#if !__FINAL
inline atNonFinalHashString::atNonFinalHashString(const class atHashValue& op) : atNonFinalHashString::Base(op.GetHash()) {}
inline void atNonFinalHashString::operator =(const class atHashValue& op) { SetHash(op.GetHash()); }
inline bool atNonFinalHashString::operator ==(const class atFinalHashString& op) const { return GetHash() == op.GetHash(); }
inline bool atNonFinalHashString::operator !=(const class atFinalHashString& op) const { return GetHash() != op.GetHash(); }
#endif

inline atFinalHashString::atFinalHashString(const class atHashValue& op) : atFinalHashString::Base(op.GetHash()) {}
inline bool atFinalHashString::operator ==(const class atHashValue& op) const { return GetHash() == op.GetHash(); }
inline bool atFinalHashString::operator !=(const class atHashValue& op) const { return GetHash() != op.GetHash(); }
#if !__FINAL
inline bool atFinalHashString::operator ==(const class atNonFinalHashString& op) const { return GetHash() == op.GetHash(); }
inline bool atFinalHashString::operator !=(const class atNonFinalHashString& op) const { return GetHash() != op.GetHash(); }
#endif

#if !__FINAL
CompileTimeAssert( sizeof(atNonFinalHashString) == 4 );
#endif	//	!__FINAL

CompileTimeAssert( sizeof(atFinalHashString) == 4);
CompileTimeAssert( sizeof(atHashValue) == 4 );



#if !__FINAL
typedef atNonFinalHashString	atHashString;
#else
typedef atHashValue			atHashString;
#endif // __FINAL

#if __DEV
typedef atHashString	atHashWithStringDev;
#else
typedef atHashValue		atHashWithStringDev;
#endif // __FINAL

#if __BANK
typedef atHashString	atHashWithStringBank;
#else
typedef atHashValue		atHashWithStringBank;
#endif // __FINAL

typedef atHashString    atHashWithStringNotFinal;

typedef atNamespacedHashString<HSNS_ATLITERALHASHSTRING> atLiteralHashString;
typedef atNamespacedHashValue<HSNS_ATLITERALHASHSTRING> atLiteralHashValue;

typedef atNamespacedHashString<HSNS_DIAGSTRING>	atDiagHashString;
typedef atNamespacedHashValue<HSNS_DIAGSTRING> atDiagHashValue;

typedef atNamespacedHashString<HSNS_STATNAMESTRING> atStatNameString;
typedef atNamespacedHashValue<HSNS_STATNAMESTRING> atStatNameValue;

}

#endif // !defined ATL_HASHSTRING_H
