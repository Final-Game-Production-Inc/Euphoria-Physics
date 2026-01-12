// 
// data/autoid.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef DATA_AUTOID_H
#define DATA_AUTOID_H

namespace rage
{

void AutoIdInit();
void AutoIdShutdown();

class AutoIdMgr;
class AutoIdRegistry;

// Security - remove the auto id names from the shipping exe.
// Projects such as FiveM used this info to figure out our network message names/contents.
#define STRIP_AUTO_ID_NAMES (__NO_OUTPUT)

enum
{
    //Maximum number of chars in the full name of an auto id.
    //Includes null terminator.
    AUTOID_MAX_LENOF_FULLNAME  = 255,
};

class AutoIdDescriptor
{
    friend class AutoIdRegistry;

public:

    AutoIdDescriptor( const unsigned level,
#if !STRIP_AUTO_ID_NAMES
                      const char* name,
#endif
                      const AutoIdDescriptor* family,
					  int priority );

    AutoIdDescriptor( const unsigned level,
                      const unsigned mandatedId,
#if !STRIP_AUTO_ID_NAMES
                      const char* name,
#endif
                      const AutoIdDescriptor* family );

    virtual ~AutoIdDescriptor() {}

    const AutoIdDescriptor* GetFamily() const { return m_Family; }

    unsigned GetId() const { return m_Id; }

    unsigned GetLevel() const { return m_Level; }

	bool IsIdMandated() const	{ return m_HasMandatedId; }

    int GetOrdinal() const;

    struct Compare
    {
        bool operator()( const AutoIdDescriptor* a,
                         const AutoIdDescriptor* b ) const;
    };

    struct ById
    {
        bool operator()( const AutoIdDescriptor* a,
                         const AutoIdDescriptor* b ) const;
    };

#if !STRIP_AUTO_ID_NAMES
	const char* GetName() const { return m_Name; }

    const char* GetFullName( char* buf,
                             const int sizeofBuf ) const;

    const char* GetReverseFullName( char* buf,
                                    const int sizeofBuf ) const;
#endif

private:
    unsigned m_Level;
    unsigned m_Id;
    int m_Ordinal;
#if STRIP_AUTO_ID_NAMES
    char m_Name[10];
#else
	char m_RandomName[10];
	const char* m_Name;
#endif
    const AutoIdDescriptor* m_Family;
    AutoIdDescriptor* m_Next;
    bool m_HasMandatedId;
	char m_Priority;
};

typedef AutoIdDescriptor AutoIdFamily;

class AutoIdRegistry
{
    friend class AutoIdMgr;

public:

    AutoIdRegistry();

    ~AutoIdRegistry();

    void Register( AutoIdDescriptor* idDesc );

    void Unregister( AutoIdDescriptor* idDesc );

    const AutoIdDescriptor* GetDescriptorFromId( const unsigned id ) const;

    const AutoIdFamily* GetFamilyFromId( const unsigned id ) const;

    bool GetFamilyIdFromId( const unsigned id,
                            unsigned* familyId ) const;

    int GetOrdinalFromId( const unsigned id ) const;

    void AssignIds();

    void ClearIds();

    int GetIdCount();

#if !STRIP_AUTO_ID_NAMES
    const char* GetNameFromId( const unsigned id ) const;

    const char* GetFullNameFromId( const unsigned id,
                                   char* buf,
                                   const int sizeofBuf ) const;
#endif

private:

    AutoIdDescriptor* m_Head;
    AutoIdDescriptor** m_OrderedIds;
    int m_Count;
    int m_MandatedIdCount;
    unsigned m_FirstConsecutiveId;
    unsigned m_LastConsecutiveId;
    AutoIdRegistry* m_Next;
};

template< typename T >
class AutoIdRegistrar
{
public:

    static void Register( AutoIdDescriptor* idDesc )
    {
        Registry()->Register( idDesc );
    }

    static void Unregister( AutoIdDescriptor* idDesc )
    {
        Registry()->Unregister( idDesc );
    };

    static const AutoIdDescriptor* GetDescriptorFromId( const unsigned id )
    {
        return Registry()->GetDescriptorFromId( id );
    }

    static const AutoIdFamily* GetFamilyFromId( const unsigned id )
    {
        return Registry()->GetFamilyFromId( id );
    }

    static bool GetFamilyIdFromId( const unsigned id,
                                   unsigned* familyId )
    {
        return Registry()->GetFamilyIdFromId( id, familyId );
    }

    static int GetOrdinalFromId( const unsigned id )
    {
        return Registry()->GetOrdinalFromId( id );
    }

    static int GetIdCount()
    {
        return Registry()->GetIdCount();
    }

    static bool IsValidId( const unsigned id )
    {
        return ( 0 != Registry()->GetDescriptorFromId( id ) );
    }

#if !STRIP_AUTO_ID_NAMES
    static const char* GetNameFromId( const unsigned id )
    {
        return Registry()->GetNameFromId( id );
    }

    static const char* GetFullNameFromId( const unsigned id,
                                          char* buf,
                                          const int sizeofBuf )
    {
        return Registry()->GetFullNameFromId( id, buf, sizeofBuf );
    }
#endif

private:

    static AutoIdRegistry* Registry()
    {
        static AutoIdRegistry s_Reg;
        return &s_Reg;
    }
};

template< typename T >
class AutoIdDescriptor_T : public AutoIdDescriptor
{
public:

    AutoIdDescriptor_T( const unsigned level,
#if !STRIP_AUTO_ID_NAMES
                        const char* name,
#endif
                        const AutoIdDescriptor* family,
						int priority )
#if !STRIP_AUTO_ID_NAMES
        : AutoIdDescriptor( level, name, family, priority )
#else
		: AutoIdDescriptor( level, family, priority )
#endif
    {
        AutoIdRegistrar< T >::Register( this );
    }

    AutoIdDescriptor_T( const unsigned level,
                        const unsigned mandatedId,
#if !STRIP_AUTO_ID_NAMES
                        const char* name,
#endif
                        const AutoIdDescriptor* family )
#if !STRIP_AUTO_ID_NAMES
        : AutoIdDescriptor( level, mandatedId, name, family )
#else
		: AutoIdDescriptor( level, mandatedId, family )
#endif
    {
        AutoIdRegistrar< T >::Register( this );
    }

    virtual ~AutoIdDescriptor_T()
    {
        AutoIdRegistrar< T >::Unregister( this );
    }
};

class AutoIdMgr
{
public:

    static void Init();

    static void Shutdown();

    static void Register( AutoIdRegistry* registry );

    static void Unregister( AutoIdRegistry* registry );

    //PURPOSE
    //  Dumps ids and names to debug output.
#if !__NO_OUTPUT
    static void DumpIds();
#endif

private:

    struct IdRegCollection
    {
        IdRegCollection()
            : m_Head( 0 )
            , m_Count( 0 )
        {
        }

        AutoIdRegistry* m_Head;
        int m_Count;
    };

    static IdRegCollection* Registries()
    {
        static IdRegCollection s_RegCollection;
        return &s_RegCollection;
    }
};

}   //namespace rage

//PURPOSE
//  Common code in all AUTOID_DECL_* macros.

#if STRIP_AUTO_ID_NAMES
#define AUTOID_DECL_COMMON( name )\
    static const name::AutoIdDesc__ AutoId__;\
    static const name::AutoIdDesc__* GetAudoIdDesc() { return &AutoId__; }\
    static unsigned GetAutoId() { return AutoId__.GetId(); }
#else
#define AUTOID_DECL_COMMON( name )\
    static const name::AutoIdDesc__ AutoId__;\
    static const name::AutoIdDesc__* GetAudoIdDesc() { return &AutoId__; }\
    static unsigned GetAutoId() { return AutoId__.GetId(); }\
    static const char* GetAutoIdName() { return #name; }\
    static const char* GetAutoIdFullName( char* buf, const int sizeofBuf ) { return AutoId__.GetFullName( buf, sizeofBuf ); }
#endif

//PURPOSE
//  For each AUTOID_DECL_*, place an instance of AUTOID_IMPL in a .cpp file.
#define AUTOID_IMPL( name )\
    const name::AutoIdDesc__ name::AutoId__;

//PURPOSE
//  Declare the root node of an Id family tree.
//PARAMS
//  name        - Name of the id.

#if STRIP_AUTO_ID_NAMES
#define AUTOID_DECL_ROOT( name )\
    typedef name AutoIdRoot;\
    enum { AutoIdLevel  = 0 };\
    static const ::rage::AutoIdDescriptor* GetAutoIdDescriptorFromId( const unsigned id ) { return AutoIdRegistrar< AutoIdRoot >::GetDescriptorFromId( id ); }\
    static bool GetAutoIdFamilyIdFromId( const unsigned id, unsigned* famId ) { return AutoIdRegistrar< AutoIdRoot >::GetFamilyIdFromId( id, famId ); }\
    static bool IsValidAutoId( const unsigned id ) { return AutoIdRegistrar< AutoIdRoot >::IsValidId( id ); }\
    struct AutoIdDesc__ : public ::rage::AutoIdDescriptor_T< AutoIdRoot >\
    {\
        AutoIdDesc__() : ::rage::AutoIdDescriptor_T< AutoIdRoot >( AutoIdLevel, NULL, 1 ) {}\
    };\
    AUTOID_DECL_COMMON( name );
#else
#define AUTOID_DECL_ROOT( name )\
    typedef name AutoIdRoot;\
    enum { AutoIdLevel  = 0 };\
    static const ::rage::AutoIdDescriptor* GetAutoIdDescriptorFromId( const unsigned id ) { return AutoIdRegistrar< AutoIdRoot >::GetDescriptorFromId( id ); }\
    static const char* GetAutoIdNameFromId( const unsigned id ) { return AutoIdRegistrar< AutoIdRoot >::GetNameFromId( id ); }\
    static const char* GetAutoIdFullNameFromId( const unsigned id, char* buf, const int sizeofBuf ) { return AutoIdRegistrar< AutoIdRoot >::GetFullNameFromId( id , buf, sizeofBuf ); }\
    static bool GetAutoIdFamilyIdFromId( const unsigned id, unsigned* famId ) { return AutoIdRegistrar< AutoIdRoot >::GetFamilyIdFromId( id, famId ); }\
    static bool IsValidAutoId( const unsigned id ) { return AutoIdRegistrar< AutoIdRoot >::IsValidId( id ); }\
    struct AutoIdDesc__ : public ::rage::AutoIdDescriptor_T< AutoIdRoot >\
    {\
        AutoIdDesc__() : ::rage::AutoIdDescriptor_T< AutoIdRoot >( AutoIdLevel, #name, NULL, 1 ) {}\
    };\
    AUTOID_DECL_COMMON( name );
#endif

//PURPOSE
//  Declare a node in an Id family tree.
//PARAMS
//  name        - Name of the id.
//  family      - Name of th id family.
#if STRIP_AUTO_ID_NAMES
#define AUTOID_DECL( name, family )\
    typedef family::AutoIdRoot AutoIdRoot;\
    enum { AutoIdLevel  = family::AutoIdLevel + 1 };\
    struct AutoIdDesc__ : public ::rage::AutoIdDescriptor_T< AutoIdRoot >\
    {\
        AutoIdDesc__() : ::rage::AutoIdDescriptor_T< AutoIdRoot >( AutoIdLevel, family::GetAudoIdDesc(), 1 ) {}\
    };\
    AUTOID_DECL_COMMON( name );
#else
#define AUTOID_DECL( name, family )\
    typedef family::AutoIdRoot AutoIdRoot;\
    enum { AutoIdLevel  = family::AutoIdLevel + 1 };\
    struct AutoIdDesc__ : public ::rage::AutoIdDescriptor_T< AutoIdRoot >\
    {\
        AutoIdDesc__() : ::rage::AutoIdDescriptor_T< AutoIdRoot >( AutoIdLevel, #name, family::GetAudoIdDesc(), 1 ) {}\
    };\
    AUTOID_DECL_COMMON( name );
#endif

//PURPOSE
//  Declare a node in an Id family tree with a specific priority.
//PARAMS
//  name        - Name of the id.
//  family      - Name of th id family.
//  priority    - Priority to use, must be 1 or greater.

#if STRIP_AUTO_ID_NAMES
#define AUTOID_DECL_PRIORITY( name, family, priority )\
	typedef family::AutoIdRoot AutoIdRoot;\
	enum { AutoIdLevel  = family::AutoIdLevel + 1 };\
	struct AutoIdDesc__ : public ::rage::AutoIdDescriptor_T< AutoIdRoot >\
	{\
		AutoIdDesc__() : ::rage::AutoIdDescriptor_T< AutoIdRoot >( AutoIdLevel, family::GetAudoIdDesc(), priority ) {}\
	};\
	AUTOID_DECL_COMMON( name );
#else
#define AUTOID_DECL_PRIORITY( name, family, priority )\
	typedef family::AutoIdRoot AutoIdRoot;\
	enum { AutoIdLevel  = family::AutoIdLevel + 1 };\
	struct AutoIdDesc__ : public ::rage::AutoIdDescriptor_T< AutoIdRoot >\
	{\
		AutoIdDesc__() : ::rage::AutoIdDescriptor_T< AutoIdRoot >( AutoIdLevel, #name, family::GetAudoIdDesc(), priority ) {}\
	};\
	AUTOID_DECL_COMMON( name );
#endif


//PURPOSE
//  Declare a node in an Id family tree with a mandated id.
//PARAMS
//  name        - Name of the id.
//  family      - Name of th id family.
//  id          - Mandated id.
#if STRIP_AUTO_ID_NAMES
#define AUTOID_DECL_ID( name, family, id )\
    typedef family::AutoIdRoot AutoIdRoot;\
    enum { AutoIdLevel  = family::AutoIdLevel + 1 };\
    struct AutoIdDesc__ : public ::rage::AutoIdDescriptor_T< AutoIdRoot >\
    {\
        AutoIdDesc__() : ::rage::AutoIdDescriptor_T< AutoIdRoot >( AutoIdLevel, id, family::GetAudoIdDesc() ) {}\
    };\
    AUTOID_DECL_COMMON( name );
#else
#define AUTOID_DECL_ID( name, family, id )\
    typedef family::AutoIdRoot AutoIdRoot;\
    enum { AutoIdLevel  = family::AutoIdLevel + 1 };\
    struct AutoIdDesc__ : public ::rage::AutoIdDescriptor_T< AutoIdRoot >\
    {\
        AutoIdDesc__() : ::rage::AutoIdDescriptor_T< AutoIdRoot >( AutoIdLevel, id, #name, family::GetAudoIdDesc() ) {}\
    };\
    AUTOID_DECL_COMMON( name );
#endif

#endif  //DATA_AUTOID_H
