// 
// data/qaseritem.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#include "qaseritem.h"

#if __QA

#include "string/string.h"
#include "system/timer.h"
#include "bitbuffer.h"

namespace rage
{

int
qaSerItem::ResetRandomSeed( const int seed )
{
    QALog( "qaSerItem: Using random seed:%u", seed );

    sm_Random.Reset( seed );

    return seed;
}

void
qaSerItem::Randomize( qaSerItem* item )
{
    static char* s_Strings[] =
    {
        "The quick brown fox jumped over the lazy dog",
        "Twas brillig,",
        "and the slithy toves did gyre and gimble in the wabe:",
        "All mimsy were the borogoves,",
        "And the mome raths outgrabe",
        //String longer than 127
        "It was the best of times, it was the worst of times"
        "It was the best of times, it was the worst of times"
        "It was the best of times, it was the worst of times"
        "It was the best of times, it was the worst of times"
    };

    static u8 s_Bitbuf[] = { "Four score and seven years ago our fathers brought forth on this continent, a new nation..." };
    static u8 s_Bytebuf[] = { "I like big butts and I cannot lie.  You other fellas can't deny..." };

    static const int NUM_STRINGS = sizeof( s_Strings ) / sizeof( s_Strings[ 0 ] );

    item->m_Type = Type( rndRanged32( 0, NUM_TYPES - 1 ) );

    switch( item->m_Type )
    {
        case TYPE_INT:
        case TYPE_UNSIGNED:
            item->m_NumBits = rndRanged32( 2, 32 );
            {
                unsigned val = rndUns32();

                val &= ~( 0xFFFFFFFF << item->m_NumBits );

                if( TYPE_INT == item->m_Type )
                {
                    const bool neg = !!( rndUns32() & 0x01 );

                    val >>= 1;

                    item->iValue = neg ? -int( val ) : int( val );
                }
                else
                {
                    item->uValue = val;
                }
            }
            break;

        case TYPE_S64:
        case TYPE_U64:
            item->m_NumBits = rndRanged32( 32, 64 );
            {
                const u64 lo = rndUns32();
                const u64 hi = rndUns32();
                u64 val = ( hi << 32 ) | lo;

				val &= ~( 0xFFFFFFFFFFFFFFFFLL << item->m_NumBits );

                if( TYPE_S64 == item->m_Type )
                {
                    const bool neg = !!( rndUns32() & 0x01 );

                    val >>= 1;

                    item->s64Value = neg ? -s64( val ) : s64( val );
                }
                else
                {
                    item->u64Value = val;
                }
            }
            break;

        case TYPE_FLOAT:
            item->m_NumBits = 32;
            item->fValue = rndFloat();
            break;

        case TYPE_DOUBLE:
            item->m_NumBits = 64;
            item->dValue = rndDouble();
            break;

        case TYPE_BOOL:
            item->m_NumBits = 1;
            item->bValue = !( rndUns32() & 0x01 );
            break;

        case TYPE_STRING:
            safecpy( item->str,
                        s_Strings[ rndRanged32( 0, NUM_STRINGS - 1 ) ],
                        sizeof( item->str ) );
            item->m_NumBits = datBitBuffer::StrBitLen( item->str );
            break;

        case TYPE_BITBUF:

            do
            {
                item->m_Offset = rndRanged32( 0, sizeof( s_Bitbuf ) * 8 );
                item->m_NumBits = rndRanged32( 0, sizeof( s_Bitbuf ) * 8 );

                if( item->m_Offset + item->m_NumBits > int( sizeof( s_Bitbuf ) * 8 ) )
                {
                    item->m_NumBits = ( sizeof( s_Bitbuf ) * 8 ) - item->m_Offset;
                    Assert( item->m_NumBits >= 0 );
                }
            }
            while( 0 == item->m_NumBits );

            CompileTimeAssert( sizeof( item->bitbuf ) >= sizeof( s_Bitbuf ) );

            sysMemCpy( item->bitbuf, s_Bitbuf, sizeof( s_Bitbuf ) );
            break;

        case TYPE_BYTEBUF:

            do
            {
                //item->m_Offset = rndRanged32( 0, sizeof( s_Bytebuf ) * 8 );
                item->m_NumBits = rndRanged32( 0, sizeof( s_Bytebuf ) * 8 );

                if( item->m_Offset + item->m_NumBits > int( sizeof( s_Bytebuf ) * 8 ) )
                {
                    item->m_NumBits = ( sizeof( s_Bitbuf ) * 8 ) - item->m_Offset;
                    Assert( item->m_NumBits >= 0 );
                }
            }
            while( item->m_NumBits < 8 );

            item->m_NumBytes = item->m_NumBits >> 3;
            item->m_NumBits = item->m_NumBytes << 3;

            CompileTimeAssert( sizeof( item->bytebuf ) >= sizeof( s_Bytebuf ) );

            sysMemCpy( item->bytebuf, s_Bytebuf, sizeof( s_Bytebuf ) );
            break;

        default:
            Assert( false );
            break;
    }
}

bool
qaSerItem::CheckBitBuf( const qaSerItem* orig, const qaSerItem* test )
{
    Assert( TYPE_BITBUF == orig->m_Type );

    bool success = false;

    if( AssertVerify( orig->m_NumBits <= ( int ) sizeof( orig->bitbuf ) << 3 ) )
    {
        success = qaSerItem::CheckBitBufImpl( orig, test );
    }

    return success;
}

bool
qaSerItem::CheckByteBuf( const qaSerItem* orig, const qaSerItem* test )
{
    Assert( TYPE_BYTEBUF == orig->m_Type );

    bool success = false;

    if( AssertVerify( ( orig->m_NumBits >> 3 ) <= ( int ) sizeof( orig->bytebuf ) ) )
    {
        success = qaSerItem::CheckBitBufImpl( orig, test );
    }

    return success;
}

bool
qaSerItem::CheckBitBufImpl( const qaSerItem* orig, const qaSerItem* test )
{
    CompileTimeAssert( sizeof( orig->bitbuf ) == sizeof( orig->bytebuf ) );

    bool success = true;

    int origIdx = orig->m_Offset;

    for( int i = 0; i < orig->m_NumBits; ++i, ++origIdx )
    {
        const unsigned oBit =
            orig->bitbuf[ origIdx >> 3 ] >> ( 0x07 - ( origIdx & 0x07 ) );
        const unsigned tBit = test->bitbuf[ i >> 3 ] >> ( 0x07 - ( i & 0x07 ) );

        if( ( oBit & 0x01 ) != ( tBit & 0x01 ) )
        {
            success = false;
            break;
        }
    }

    return success;
}

}   //namespace rage

#endif  //__QA
