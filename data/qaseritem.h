// 
// data/qaseritem.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef DATA_QASERITEM_H
#define DATA_QASERITEM_H

#include "qa/qa.h"

#if __QA

#include "math/random.h"

namespace rage
{

//PURPOSE
//  qaSerItem is used for testing serialization classes (bitbuffer, etc.).
//  Each instance of a qaSerItem contains a value of a random type and
//  a random number of bits.
//  Within a unit test instances of qaSerItem are serialized, then
//  deserialized, and then their data is checked against the original
//  data.

struct qaSerItem
{
    //Data types
    enum Type
    {
        TYPE_INVALID    = -1,
        TYPE_INT,
        TYPE_UNSIGNED,
        TYPE_S64,
        TYPE_U64,
        TYPE_FLOAT,
        TYPE_DOUBLE,
        TYPE_BOOL,
        TYPE_STRING,
        TYPE_BITBUF,
        TYPE_BYTEBUF,

        NUM_TYPES
    };

    qaSerItem()
        : m_Type( TYPE_INVALID )
        , m_NumBits( 0 )
        , m_NumBytes( 0 )
        , m_Offset( 0 )
    {
        qaSerItem::Randomize( this );
    }

    Type m_Type;
    int m_NumBits;
    int m_NumBytes;
    int m_Offset;

    //Union that can contain any of the types in the Type enum.
    union
    {
        int iValue;
        unsigned uValue;
        s64 s64Value;
        u64 u64Value;
        float fValue;
        double dValue;
        bool bValue;
        char str[ 256 ];
        u8 bitbuf[ 128 ];
        u8 bytebuf[ 128 ];
    };

    //PURPOSE
    //  Generates a random u32.
    static unsigned rndUns32()
    {
        const unsigned u0 = unsigned( sm_Random.GetInt() );
        const unsigned u1 = unsigned( sm_Random.GetInt() );
        return ( ( u0 & 0xFFFF ) << 16 ) | ( u1 & 0xFFFF );
    }

    //PURPOSE
    //  Generates a random float.
    static float rndFloat()
    {
        return ( sm_Random.GetFloat() - 0.5f ) * ~0u;
    }

    //PURPOSE
    //  Generates a random double.
    static double rndDouble()
    {
        return ( sm_Random.GetFloat() - 0.5 ) * ~u64( 0 );
    }

    //PURPOSE
    //  Generates a random u32 within a given range.
    static int rndRanged32( const unsigned m, const unsigned M )
    {
        FastAssert( m <= M );

        const unsigned u = rndUns32();
        return ( ( u % ( M - m + 1 ) ) + m );
    }

    //PURPOSE
    //  Resets the random seed in the RNG used to generate values.
    static int ResetRandomSeed( const int seed );

    //PURPOSE
    //  Populates the item with a random value of a random type.
    static void Randomize( qaSerItem* item );

    //PURPOSE
    //  Serializes an array of items to a buffer.  The buffer must
    //  implement a specific API (see the implementation of Write()).
    template< typename T >
    static void Write( T* buf,
                       const qaSerItem* items,
                       const int numItems,
                       int* numItemsWritten,
                       int* numBitsWritten );

    //PURPOSE
    //  Checks an array of items against a buffer.  The buffer must
    //  implement a specific API (see the implementation of Check()).
    template< typename T >
    static bool Check( const T* buf,
                       const qaSerItem* items,
                       const int numItems );

    //PURPOSE
    //  Compares the contents of two items and returns true if the items
    //  are equivalent.
    //NOTES
    //  The types of the items must be TYPE_BITBUF.
    static bool CheckBitBuf( const qaSerItem* orig,
                             const qaSerItem* test );

    //PURPOSE
    //  Compares the contents of two items and returns true if the items
    //  are equivalent.
    //NOTES
    //  The types of the items must be TYPE_BYTEBUF.
    static bool CheckByteBuf( const qaSerItem* orig,
                              const qaSerItem* test );

    static bool CheckBitBufImpl( const qaSerItem* orig,
                                 const qaSerItem* test );

    static mthRandom sm_Random;
};

template< typename T >
void
qaSerItem::Write( T* buf,
                   const qaSerItem* items,
                   const int numItems,
                   int* numItemsWritten,
                   int* numBitsWritten )
{
    FastAssert( numItems > 0 );

    *numItemsWritten = *numBitsWritten = 0;

    buf->SetCursorPos( 0 );

    //Write items to the buffer

    for( int i = 0; i < numItems; ++i, ++*numItemsWritten )
    {
        const qaSerItem* s = &items[ i ];

        if( !buf->CanWriteBits( s->m_NumBits ) )
        {
            break;
        }

        if( qaSerItem::TYPE_INT == s->m_Type )
        {
            buf->WriteInt( s->iValue, s->m_NumBits );
        }
        else if( qaSerItem::TYPE_UNSIGNED == s->m_Type )
        {
            buf->WriteUns( s->uValue, s->m_NumBits );
        }
        else if( qaSerItem::TYPE_S64 == s->m_Type )
        {
            buf->WriteInt( s->s64Value, s->m_NumBits );
        }
        else if( qaSerItem::TYPE_U64 == s->m_Type )
        {
            buf->WriteUns( s->u64Value, s->m_NumBits );
        }
        else if( qaSerItem::TYPE_FLOAT == s->m_Type )
        {
            buf->WriteFloat( s->fValue );
        }
        else if( qaSerItem::TYPE_DOUBLE == s->m_Type )
        {
            buf->WriteDouble( s->dValue );
        }
        else if( qaSerItem::TYPE_BOOL == s->m_Type )
        {
            buf->WriteBool( s->bValue );
        }
        else if( qaSerItem::TYPE_STRING == s->m_Type )
        {
            buf->WriteStr( s->str, sizeof( s->str ) );
        }
        else if( qaSerItem::TYPE_BITBUF == s->m_Type )
        {
            buf->WriteBits( s->bitbuf, s->m_NumBits, s->m_Offset );
        }
        else if( qaSerItem::TYPE_BYTEBUF == s->m_Type )
        {
            buf->WriteBytes( s->bytebuf, s->m_NumBytes );
        }
        else
        {
            Assert( false );
        }

        *numBitsWritten += s->m_NumBits;

        FastAssert( *numBitsWritten == buf->GetNumBitsWritten() );
    }
}

template< typename T >
bool
qaSerItem::Check( const T* buf,
                   const qaSerItem* items,
                   const int numItems )
{
    FastAssert( numItems > 0 );

    buf->SetReadPos( 0 );

    qaSerItem tmp;

    for( int i = 0; i < numItems; ++i )
    {
        const qaSerItem* s = &items[ i ];

        if( qaSerItem::TYPE_INT == s->m_Type )
        {
            if( !buf->ReadInt( tmp.iValue, s->m_NumBits )
                || tmp.iValue != s->iValue )
                return false;
        }
        else if( qaSerItem::TYPE_UNSIGNED == s->m_Type )
        {
            if( !buf->ReadUns( tmp.uValue, s->m_NumBits )
                || tmp.uValue != s->uValue )
                return false;
        }
        else if( qaSerItem::TYPE_S64 == s->m_Type )
        {
            if( !buf->ReadInt( tmp.s64Value, s->m_NumBits )
                || tmp.s64Value != s->s64Value )
                return false;
        }
        else if( qaSerItem::TYPE_U64 == s->m_Type )
        {
            if( !buf->ReadUns( tmp.u64Value, s->m_NumBits )
                || tmp.u64Value != s->u64Value )
                return false;
        }
        else if( qaSerItem::TYPE_FLOAT == s->m_Type )
        {
            if( !buf->ReadFloat( tmp.fValue )
                || tmp.fValue != s->fValue )
                return false;
        }
        else if( qaSerItem::TYPE_DOUBLE == s->m_Type )
        {
            if( !buf->ReadDouble( tmp.dValue )
                || tmp.dValue != s->dValue )
                return false;
        }
        else if( qaSerItem::TYPE_BOOL == s->m_Type )
        {
            if( !buf->ReadBool( tmp.bValue )
                || tmp.bValue != s->bValue )
                return false;
        }
        else if( qaSerItem::TYPE_STRING == s->m_Type )
        {
            if( !buf->ReadStr( tmp.str, sizeof( tmp.str ) )
                || 0 != ::strcmp( tmp.str, s->str ) )
                return false;
        }
        else if( qaSerItem::TYPE_BITBUF == s->m_Type )
        {
            if( !buf->ReadBits( tmp.bitbuf, s->m_NumBits )
                || !qaSerItem::CheckBitBuf( s, &tmp ) )
                return false;
        }
        else if( qaSerItem::TYPE_BYTEBUF == s->m_Type )
        {
            if( !buf->ReadBytes( tmp.bytebuf, s->m_NumBytes )
                || !qaSerItem::CheckByteBuf( s, &tmp ) )
                return false;
        }
        else
        {
            FastAssert( false );
        }
    }

    return true;
}

}   //namespace rage

#endif  //__QA

#endif  //DATA_QASERITEM_H
