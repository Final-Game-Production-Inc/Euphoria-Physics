// 
// data/autoid.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 
#include "autoid.h"

//#include "netdiag.h"
#include "string/string.h"
#include "system/nelem.h"
#include "system/param.h"
#include "math/random.h"

#include <time.h>
#include <stdlib.h>
#include <algorithm>
#include "system/rsg_algorithm.h" //for lower bound

#if !__FINAL
PARAM(dumpautoids, "[net] Dump all the auto-IDs that were assigned");
#endif // !__FINAL

namespace rage
{

static int s_InitCount = 0;

static void GenerateRandomAutoIdName(char (&name)[10])
{
	static bool s_InitRng = false;
	static mthRandom s_AutoIdRng;
	static const char s_Alphabet[] =
	{
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"0123456789"
	};
	static const unsigned numChars = COUNTOF(s_Alphabet) - 1;

	if(!s_InitRng)
	{
		s_InitRng = true;

		// seed with a known value so everyone gets the same names
		s_AutoIdRng.Reset(0x58BD19E0);
	}

	for(unsigned i = 0; i < COUNTOF(name); ++i)
	{
		int rndIndex = s_AutoIdRng.GetRanged(0, numChars - 1);
		name[i] = s_Alphabet[rndIndex];
	}
}

void
AutoIdInit()
{
    if( !s_InitCount )
    {
        AutoIdMgr::Init();
    }

    ++s_InitCount;
}

void
AutoIdShutdown()
{
    Assert( s_InitCount > 0 );

    --s_InitCount;

    if( !s_InitCount )
    {
        AutoIdMgr::Shutdown();
    }
}

///////////////////////////////////////////////////////////////////////////////
// AutoIdDescriptor
///////////////////////////////////////////////////////////////////////////////

AutoIdDescriptor::AutoIdDescriptor( const unsigned level,
#if !STRIP_AUTO_ID_NAMES
                                    const char* name,
#endif
                                    const AutoIdDescriptor* family,
									int priority)
    : m_Level( level )
    , m_Id( 0 )
    , m_Ordinal(0)
    , m_Family( family )
    , m_Next( 0 )
    , m_HasMandatedId( false )
	, m_Priority( (char) priority )
{
#if STRIP_AUTO_ID_NAMES
	GenerateRandomAutoIdName(m_Name);
#else
	m_Name = name;
	GenerateRandomAutoIdName(m_RandomName);
#endif
}

AutoIdDescriptor::AutoIdDescriptor( const unsigned level,
                                    const unsigned mandatedId,
#if !STRIP_AUTO_ID_NAMES
                                    const char* name,
#endif
                                    const AutoIdDescriptor* family )
    : m_Level( level )
    , m_Id( mandatedId )
    , m_Ordinal(0)
    , m_Family( family )
    , m_Next( 0 )
    , m_HasMandatedId( true )
	, m_Priority( 0 )
{
#if STRIP_AUTO_ID_NAMES
	GenerateRandomAutoIdName(m_Name);
#else
	m_Name = name;
	GenerateRandomAutoIdName(m_RandomName);
#endif
}

#if !STRIP_AUTO_ID_NAMES
const char*
AutoIdDescriptor::GetFullName( char* buf, const int sizeofBuf ) const
{
    const AutoIdFamily* family = this->GetFamily();

    if( !family || this == family )
    {
        safecpy( buf, this->GetName(), sizeofBuf );
    }
    else
    {
        family->GetFullName( buf, sizeofBuf );
        safecat( buf, ".", sizeofBuf );
        safecat( buf, this->GetName(), sizeofBuf );
    }

    return buf;
}

const char*
AutoIdDescriptor::GetReverseFullName( char* buf, const int sizeofBuf ) const
{
    safecpy( buf, this->GetName(), sizeofBuf );

    const AutoIdFamily* family = this->GetFamily();

    while( family && this != family )
    {
        safecat( buf, ".", sizeofBuf );
        safecat( buf, family->GetName(), sizeofBuf );
        family = family->GetFamily();
    }

    return buf;
}
#endif

int
AutoIdDescriptor::GetOrdinal() const
{
    return m_Ordinal;
}

///////////////////////////////////////////////////////////////////////////////
// AutoIdRegistry
///////////////////////////////////////////////////////////////////////////////

AutoIdRegistry::AutoIdRegistry()
    : m_Head( 0 )
    , m_OrderedIds( 0 )
    , m_Count( 0 )
    , m_MandatedIdCount( 0 )
    , m_FirstConsecutiveId(0)
    , m_LastConsecutiveId(0)
    , m_Next( 0 )
{
    AutoIdMgr::Register( this );
}

AutoIdRegistry::~AutoIdRegistry()
{
    this->ClearIds();
}

void
AutoIdRegistry::Register( AutoIdDescriptor* idDesc )
{
    Assert( !m_OrderedIds );
    Assert( !idDesc->m_Next );

    idDesc->m_Next = m_Head;
    m_Head = idDesc;

    if( idDesc->m_HasMandatedId )
    {
        ++m_MandatedIdCount;
    }

    ++m_Count;
}

void
AutoIdRegistry::Unregister( AutoIdDescriptor* idDesc )
{
    Assert( idDesc->m_Next || m_Head == idDesc );

    AutoIdDescriptor* prev = 0;
    AutoIdDescriptor* cur = m_Head;

    while( idDesc != cur )
    {
        prev = cur;
        cur = cur->m_Next;
    }

    if( cur )
    {
        if( !prev )
        {
            m_Head = cur->m_Next;
        }
        else
        {
            prev->m_Next = cur->m_Next;
        }

        cur->m_Next = 0;

        if( idDesc->m_HasMandatedId )
        {
            --m_MandatedIdCount;
            Assert( m_MandatedIdCount >= 0 );
        }

        --m_Count;
        Assert( m_Count >= 0 );
    }
}

struct AutoIdLowerBoundPred
{
    bool operator()( const AutoIdDescriptor* desc, const unsigned id ) const
    {
        return desc->GetId() < id;
    }
};

const AutoIdDescriptor*
AutoIdRegistry::GetDescriptorFromId( const unsigned id ) const
{
    AssertMsg( m_OrderedIds , "Forgot to call AutoIdInit()" );

    const AutoIdDescriptor* desc = 0;

    if( id >= m_FirstConsecutiveId && id <= m_LastConsecutiveId )
    {
        desc = m_OrderedIds[id-m_FirstConsecutiveId];
    }
    else
    {
        //This could be optimized by excluding consecutive ids from the search.
        AutoIdDescriptor** first = m_OrderedIds;
        AutoIdDescriptor** stop = m_OrderedIds + m_Count;

        AutoIdDescriptor** p =
            std::lower_boundRSG( first, stop, id, AutoIdLowerBoundPred() );

        if( p != stop && id == ( *p )->m_Id )
        {
            desc = *p;
        }
    }

    return desc;
}

#if !STRIP_AUTO_ID_NAMES
const char*
AutoIdRegistry::GetNameFromId( const unsigned id ) const
{
    const AutoIdDescriptor* desc = this->GetDescriptorFromId( id );

    return desc ? desc->GetName() : 0;
}

const char*
AutoIdRegistry::GetFullNameFromId( const unsigned id,
                                   char* buf,
                                   const int sizeofBuf ) const
{
    const AutoIdDescriptor* desc = this->GetDescriptorFromId( id );

    return desc ? desc->GetFullName( buf, sizeofBuf ) : 0;
}
#endif

const AutoIdFamily*
AutoIdRegistry::GetFamilyFromId( const unsigned id ) const
{
    const AutoIdDescriptor* desc = this->GetDescriptorFromId( id );

    return desc ? desc->GetFamily() : 0;
}

bool
AutoIdRegistry::GetFamilyIdFromId( const unsigned id,
                                   unsigned* familyId ) const
{
    const AutoIdFamily* family = this->GetFamilyFromId( id );

    const bool success = 0 != family;

    if( success )
    {
        *familyId = family->GetId();
    }

    return success;
}

int
AutoIdRegistry::GetOrdinalFromId( const unsigned id ) const
{
    const AutoIdDescriptor* desc = this->GetDescriptorFromId( id );

    return desc ? desc->GetOrdinal() : -1;
}

bool
AutoIdDescriptor::Compare::operator()( const AutoIdDescriptor* a,
                                       const AutoIdDescriptor* b ) const
{
    //Sort so all mandated ids come before auto-generated ids.
    //Mandated descriptors will be ordered by id, non-mandated will
    //be ordered by name.

    if( a->m_HasMandatedId && b->m_HasMandatedId )
    {
        Assertf(a->m_Id != b->m_Id || a == b,
                "Duplicate message ids: \"%s\"/\"%s\"",
                a->GetName(),
                b->GetName());

        return int( a->m_Id < b->m_Id );
    }
    else if( a->m_HasMandatedId )
    {
        return true;
    }
    else if( b->m_HasMandatedId )
    {
        return false;
    }
    else if( a == b )
    {
        return false;
    }
    else
    {
		// Sort by priority next.
		if (a->m_Priority < b->m_Priority)
		{
			return true;
		}

		if (a->m_Priority > b->m_Priority)
		{
			return false;
		}

#if !STRIP_AUTO_ID_NAMES
#if __FINAL_LOGGING
		// final logging builds have the auto id names for logging purposes, but we want the order to be
		// the same as in the shipping exe so we can connect to multiplayer sessions with the public
		const int cmp = ::strcmp( a->m_RandomName, b->m_RandomName );
#else
		// Finally, sort by name.
		char bufA[ AUTOID_MAX_LENOF_FULLNAME ];
		char bufB[ AUTOID_MAX_LENOF_FULLNAME ];

		//Use reverse full name because the most variation in full
		//names will come at the end of the name.
		//e.g. Pressed.Keyboard.Ui vs Ui.Keyboard.Pressed

		a->GetReverseFullName( bufA, sizeof( bufA ) );
        b->GetReverseFullName( bufB, sizeof( bufB ) );
		const int cmp = ::strcmp( bufA, bufB );
		Assertf( cmp != 0,  "Duplicate message ids: \"%s\"/\"%s\"", bufA, bufB );
#endif
#else
		const int cmp = ::strcmp( a->m_Name, b->m_Name );
		if(cmp == 0)
		{
			Quitf(0, "Duplicate message ids: \"%s\"/\"%s\"", a->m_Name, b->m_Name );
		}
#endif

        return cmp < 0;
    }
}

bool
AutoIdDescriptor::ById::operator()(const AutoIdDescriptor* a,
                                    const AutoIdDescriptor* b) const
{
    return a->m_Id < b->m_Id;
}

void
AutoIdRegistry::AssignIds()
{
    if( !m_OrderedIds )
    {
        m_OrderedIds = rage_new AutoIdDescriptor*[ m_Count ];

        if( AssertVerify( m_OrderedIds ) )
        {
            AutoIdDescriptor* cur = m_Head;

            for( int i = 0; cur; ++i, cur = cur->m_Next )
            {
				m_OrderedIds[ i ] = cur;

				for(int j = 0; j < i; ++j)
				{
#if STRIP_AUTO_ID_NAMES
					if(strcmp(m_OrderedIds[j]->m_Name, m_OrderedIds[i]->m_Name) == 0)
#else
					if(strcmp(m_OrderedIds[j]->m_RandomName, m_OrderedIds[i]->m_RandomName) == 0)
#endif
					{
						Quitf(0, "Duplicate Auto Ids");
					}
				}
            }

            //Group all mandated ids at the beginning of the buffer and
            //all non-mandated ids at the end.
            //The mandated group is ordered by id, the non-mandated group is
            //ordered by name.
            //Assign ids to the non-mandated group, making sure the ids aren't
            //used by the mandated group.
            std::sort( m_OrderedIds, m_OrderedIds + m_Count, AutoIdDescriptor::Compare() );

            AutoIdDescriptor** mstart = &m_OrderedIds[0];
            AutoIdDescriptor** mend = &m_OrderedIds[m_MandatedIdCount];
            unsigned nextId = 0;
            for(int i = m_MandatedIdCount; i < m_Count; ++i)
            {
                //Make sure the id isn't used by the mandated ids.
                while(mstart < mend)
                {
                    if(nextId < (*mstart)->m_Id)
                    {
                        break;
                    }
                    else
                    {
                        Assert(nextId == (*mstart)->m_Id);
                        nextId = (*mstart)->m_Id + 1;
                        ++mstart;
                    }
                }

                m_OrderedIds[i]->m_Id = nextId++;
            }

            //Re-sort to final ordering
            std::sort(m_OrderedIds, m_OrderedIds + m_Count, AutoIdDescriptor::ById());

            m_FirstConsecutiveId = m_LastConsecutiveId = m_OrderedIds[0]->m_Id;
            for(int i = 0; i < m_Count; ++i)
            {
                if(m_OrderedIds[i]->m_Id == m_LastConsecutiveId + 1)
                {
                    ++m_LastConsecutiveId;
                }

                m_OrderedIds[i]->m_Ordinal = i;
            }

#if __ASSERT
            for( int idx = 1; idx < m_Count; ++idx )
            {
                char bufA[ AUTOID_MAX_LENOF_FULLNAME ];
                char bufB[ AUTOID_MAX_LENOF_FULLNAME ];

                Assertf( m_OrderedIds[ idx ]->GetId() != m_OrderedIds[ idx - 1 ]->GetId(),
                            "Duplicate IDs: \"%s\"[%d] and \"%s\"[%d]",
                            m_OrderedIds[ idx ]->GetFullName( bufA, sizeof( bufA ) ),
                            m_OrderedIds[ idx ]->GetId(),
                            m_OrderedIds[ idx - 1 ]->GetFullName( bufB, sizeof( bufB ) ),
                            m_OrderedIds[ idx - 1 ]->GetId() );
            }
#endif  //__ASSERT
        }
    }

#if !__FINAL
	if (PARAM_dumpautoids.Get())
	{
		for(int i = 0; i < m_Count; ++i)
		{
			AutoIdDescriptor *desc = m_OrderedIds[i];
			char fullName[256];
			desc->GetFullName(fullName, sizeof(fullName));
			Displayf("AutoID: %s: %d %s", fullName, desc->GetId(), desc->IsIdMandated() ? "(mandated)" : "");
		}
	}
#endif // !_FINAL

}

void
AutoIdRegistry::ClearIds()
{
    delete [] m_OrderedIds;
    m_OrderedIds = 0;
}

int
AutoIdRegistry::GetIdCount()
{
    return m_Count;
}

///////////////////////////////////////////////////////////////////////////////
// AutoIdMgr
///////////////////////////////////////////////////////////////////////////////

void
AutoIdMgr::Init()
{
    AutoIdRegistry* cur = Registries()->m_Head;

    for( ; cur; cur = cur->m_Next )
    {
        cur->AssignIds();
    }
}

void
AutoIdMgr::Shutdown()
{
    AutoIdRegistry* cur = Registries()->m_Head;

    for( ; cur; cur = cur->m_Next )
    {
        cur->ClearIds();
    }
}

void
AutoIdMgr::Register( AutoIdRegistry* registry )
{
    Assert( !registry->m_Next );

    registry->m_Next = Registries()->m_Head;
    Registries()->m_Head = registry;

    ++Registries()->m_Count;
}

void
AutoIdMgr::Unregister( AutoIdRegistry* registry )
{
    Assert( registry->m_Next || Registries()->m_Head == registry );

    AutoIdRegistry* prev = 0;
    AutoIdRegistry* cur = Registries()->m_Head;

    while( registry != cur )
    {
        prev = cur;
        cur = cur->m_Next;
    }

    if( cur )
    {
        if( !prev )
        {
            Registries()->m_Head = cur->m_Next;
        }
        else
        {
            prev->m_Next = cur->m_Next;
        }

        cur->m_Next = 0;
    }

    --Registries()->m_Count;
    Assert( Registries()->m_Count >= 0 );
}

#if !__NO_OUTPUT
void
AutoIdMgr::DumpIds()
{
    AutoIdInit();
    const AutoIdRegistry* cur = Registries()->m_Head;

    for(; cur; cur = cur->m_Next)
    {
        for(int i = 0; i < cur->m_Count; ++i)
        {
            const AutoIdDescriptor* desc = cur->m_OrderedIds[i];
            char buf[256];

            Displayf("0x%08x: %s", desc->GetId(), desc->GetFullName(buf, COUNTOF(buf)));
        }
    }
    AutoIdShutdown();
}
#endif  //__NO_OUTPUT

}   //namespace rage

///////////////////////////////////////////////////////////////////////////////
// Unit tests
///////////////////////////////////////////////////////////////////////////////

#ifndef QA_ASSERT_ON_FAIL
#define QA_ASSERT_ON_FAIL   0
#endif
#include "qa/qa.h"

#if __QA

using namespace rage;

#include <string.h>

class qa_AutoId : public qaItem
{
public:

    qa_AutoId()
    {
    }

    void Init()
    {
        AutoIdInit();
    }

    void Shutdown()
    {
        AutoIdShutdown();
    }

	void Update( qaResult& result );

    enum
    {
        ID3     = 3,
        ID4     = 4,
        ID6     = 6,
        ID8     = 0xaa76,
        ID9     = 0xba76,
        ID10    = 0xac76,
        ID11    = 0xaa16,
        ID12    = 0x0a76,
        ID13    = 0xa376,
        ID14    = 0x1234,
        ID15    = 0x7890,
    };

    struct A0 { AUTOID_DECL_ROOT( A0 ); };
    struct A1 { AUTOID_DECL( A1, A0 ); };
    struct A2 { AUTOID_DECL_ID( A2, A1, ID3 ); };
    struct A3 { AUTOID_DECL( A3, A2 ); };
    struct A4 { AUTOID_DECL( A4, A0 ); };
    struct A5 { AUTOID_DECL_ID( A5, A0, ID4 ); };
    struct A6 { AUTOID_DECL( A6, A0 ); };
    struct A7 { AUTOID_DECL_ID( A7, A0, ID6 ); };
    struct A8 { AUTOID_DECL_ID( A8, A7, ID8 ); };
    struct A9 { AUTOID_DECL_ID( A9, A8, ID9 ); };
    struct A10 { AUTOID_DECL_ID( A10, A9, ID10 ); };
    struct A11 { AUTOID_DECL_ID( A11, A0, ID11 ); };
    struct A12 { AUTOID_DECL_ID( A12, A0, ID12 ); };
    struct A13 { AUTOID_DECL_ID( A13, A0, ID13 ); };
    struct A14 { AUTOID_DECL_ID( A14, A0, ID14 ); };
};

AUTOID_IMPL( qa_AutoId::A0 );
AUTOID_IMPL( qa_AutoId::A1 );
AUTOID_IMPL( qa_AutoId::A2 );
AUTOID_IMPL( qa_AutoId::A3 );
AUTOID_IMPL( qa_AutoId::A4 );
AUTOID_IMPL( qa_AutoId::A5 );
AUTOID_IMPL( qa_AutoId::A6 );
AUTOID_IMPL( qa_AutoId::A7 );
AUTOID_IMPL( qa_AutoId::A8 );
AUTOID_IMPL( qa_AutoId::A9 );
AUTOID_IMPL( qa_AutoId::A10 );
AUTOID_IMPL( qa_AutoId::A11 );
AUTOID_IMPL( qa_AutoId::A12 );
AUTOID_IMPL( qa_AutoId::A13 );
AUTOID_IMPL( qa_AutoId::A14 );

void
qa_AutoId::Update( qaResult& result )
{
    const char* name;
    unsigned id;

    //Check that we can retrieve names from auto-generated ids.
    name = A0::GetAutoIdNameFromId( A0::GetAutoId() );
    QA_CHECK( name && 0 == ::strcmp( name, "A0" ) );

    name = A0::GetAutoIdNameFromId( A1::GetAutoId() );
    QA_CHECK( name && 0 == ::strcmp( name, "A1" ) );

    name = A0::GetAutoIdNameFromId( A2::GetAutoId() );
    QA_CHECK( name && 0 == ::strcmp( name, "A2" ) );

    name = A0::GetAutoIdNameFromId( A3::GetAutoId() );
    QA_CHECK( name && 0 == ::strcmp( name, "A3" ) );

    name = A0::GetAutoIdNameFromId( A4::GetAutoId() );
    QA_CHECK( name && 0 == ::strcmp( name, "A4" ) );

    name = A0::GetAutoIdNameFromId( A5::GetAutoId() );
    QA_CHECK( name && 0 == ::strcmp( name, "A5" ) );

    name = A0::GetAutoIdNameFromId( A6::GetAutoId() );
    QA_CHECK( name && 0 == ::strcmp( name, "A6" ) );

    name = A0::GetAutoIdNameFromId( A7::GetAutoId() );
    QA_CHECK( name && 0 == ::strcmp( name, "A7" ) );

    //Check that we can retrive names from mandated ids.
    name = A0::GetAutoIdNameFromId( ID8 );
    QA_CHECK( name && 0 == ::strcmp( name, "A8" ) );

    name = A0::GetAutoIdNameFromId( ID9 );
    QA_CHECK( name && 0 == ::strcmp( name, "A9" ) );

    name = A0::GetAutoIdNameFromId( ID10 );
    QA_CHECK( name && 0 == ::strcmp( name, "A10" ) );

    name = A0::GetAutoIdNameFromId( ID11 );
    QA_CHECK( name && 0 == ::strcmp( name, "A11" ) );

    name = A0::GetAutoIdNameFromId( ID12 );
    QA_CHECK( name && 0 == ::strcmp( name, "A12" ) );

    name = A0::GetAutoIdNameFromId( ID13 );
    QA_CHECK( name && 0 == ::strcmp( name, "A13" ) );

    name = A0::GetAutoIdNameFromId( ID14 );
    QA_CHECK( name && 0 == ::strcmp( name, "A14" ) );

    name = A0::GetAutoIdNameFromId( ID15 );
    QA_CHECK( !name );

    //Test retrieving family ids.
    id = A3::GetAutoId();
    QA_CHECK( A0::GetAutoIdFamilyIdFromId( id, &id )
              && A2::GetAutoId() == id );

    QA_CHECK( A0::GetAutoIdFamilyIdFromId( id, &id )
              && A1::GetAutoId() == id );

    QA_CHECK( A0::GetAutoIdFamilyIdFromId( id, &id )
              && A0::GetAutoId() == id );

    //A0 has no family id.
    QA_CHECK( !A0::GetAutoIdFamilyIdFromId( id, &id ) );

    //Test retrieving family ids from mandated ids.
    id = A10::GetAutoId();
    QA_CHECK( A0::GetAutoIdFamilyIdFromId( id, &id )
              && A9::GetAutoId() == id );

    QA_CHECK( A0::GetAutoIdFamilyIdFromId( id, &id )
              && A8::GetAutoId() == id );

    QA_CHECK( A0::GetAutoIdFamilyIdFromId( id, &id )
              && A7::GetAutoId() == id );

    QA_CHECK( A0::GetAutoIdFamilyIdFromId( id, &id )
              && A0::GetAutoId() == id );

    TST_PASS;
}

QA_ITEM_FAMILY( qa_AutoId, (), () );

QA_ITEM_FAST( qa_AutoId, (), qaResult::PASS_OR_FAIL );

#endif  //__QA
