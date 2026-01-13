// 
// system/endian.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#include "endian.h"
#include "math/nan.h"

//#define QA_ASSERT_ON_FAIL 1
#include "qa/qa.h"

#if __QA

using namespace rage;

class sysEndianQA : public qaItem
{
public:

    sysEndianQA();

    void Init();
    void Shutdown();

    void Update( qaResult& result );

private:
};

sysEndianQA::sysEndianQA()
{
}

void
sysEndianQA::Init()
{
}

void
sysEndianQA::Shutdown()
{
}

void
sysEndianQA::Update( qaResult& result )
{
    u64 valu64 = 0x0102030405060708ULL;
    s64 vals64 = 0x0102030405060708ULL;
    u32 valu32 = 0x01020304;
    s32 vals32 = 0x01020304;
    u16 valu16 = 0x0102;
    s16 vals16 = 0x0102;
    float valf = 1.345f;
    double vald = 78.931;

    u64 swap_valu64 = sysEndian::Swap( valu64 );
    s64 swap_vals64 = sysEndian::Swap( vals64 );
    u32 swap_valu32 = sysEndian::Swap( valu32 );
    s32 swap_vals32 = sysEndian::Swap( vals32 );
    u16 swap_valu16 = sysEndian::Swap( valu16 );
    s16 swap_vals16 = sysEndian::Swap( vals16 );
    float swap_valf = sysEndian::Swap( valf );
    double swap_vald = sysEndian::Swap( vald );

    QA_CHECK( swap_valu64 == 0x0807060504030201ULL );
    QA_CHECK( swap_vals64 == 0x0807060504030201ULL );
    QA_CHECK( swap_valu32 == 0x04030201 );
    QA_CHECK( swap_vals32 == 0x04030201 );
    QA_CHECK( swap_valu16 == 0x0201 );
    QA_CHECK( swap_vals16 == 0x0201 );

    QA_CHECK( swap_valu64 == sysEndian::BtoL( valu64 ) );
    QA_CHECK( swap_vals64 == sysEndian::BtoL( vals64 ) );
    QA_CHECK( swap_valu32 == sysEndian::BtoL( valu32 ) );
    QA_CHECK( swap_vals32 == sysEndian::BtoL( vals32 ) );
    QA_CHECK( swap_valu16 == sysEndian::BtoL( valu16 ) );
    QA_CHECK( swap_vals16 == sysEndian::BtoL( vals16 ) );
    QA_CHECK( swap_valf == sysEndian::BtoL( valf ) );
    QA_CHECK( swap_vald == sysEndian::BtoL( vald ) );

    QA_CHECK( swap_valu64 == sysEndian::LtoB( valu64 ) );
    QA_CHECK( swap_vals64 == sysEndian::LtoB( vals64 ) );
    QA_CHECK( swap_valu32 == sysEndian::LtoB( valu32 ) );
    QA_CHECK( swap_vals32 == sysEndian::LtoB( vals32 ) );
    QA_CHECK( swap_valu16 == sysEndian::LtoB( valu16 ) );
    QA_CHECK( swap_vals16 == sysEndian::LtoB( vals16 ) );
    QA_CHECK( swap_valf == sysEndian::LtoB( valf ) );
    QA_CHECK( swap_vald == sysEndian::LtoB( vald ) );

    if( sysEndian::IsBig() )
    {
        QA_CHECK( valu64 == sysEndian::BtoN( valu64 ) );
        QA_CHECK( vals64 == sysEndian::BtoN( vals64 ) );
        QA_CHECK( valu32 == sysEndian::BtoN( valu32 ) );
        QA_CHECK( vals32 == sysEndian::BtoN( vals32 ) );
        QA_CHECK( valu16 == sysEndian::BtoN( valu16 ) );
        QA_CHECK( vals16 == sysEndian::BtoN( vals16 ) );
        QA_CHECK( valf == sysEndian::BtoN( valf ) );
        QA_CHECK( vald == sysEndian::BtoN( vald ) );

        QA_CHECK( valu64 == sysEndian::NtoB( valu64 ) );
        QA_CHECK( vals64 == sysEndian::NtoB( vals64 ) );
        QA_CHECK( valu32 == sysEndian::NtoB( valu32 ) );
        QA_CHECK( vals32 == sysEndian::NtoB( vals32 ) );
        QA_CHECK( valu16 == sysEndian::NtoB( valu16 ) );
        QA_CHECK( vals16 == sysEndian::NtoB( vals16 ) );
        QA_CHECK( valf == sysEndian::NtoB( valf ) );
        QA_CHECK( vald == sysEndian::NtoB( vald ) );

        QA_CHECK( swap_valu64 == sysEndian::LtoN( valu64 ) );
        QA_CHECK( swap_vals64 == sysEndian::LtoN( vals64 ) );
        QA_CHECK( swap_valu32 == sysEndian::LtoN( valu32 ) );
        QA_CHECK( swap_vals32 == sysEndian::LtoN( vals32 ) );
        QA_CHECK( swap_valu16 == sysEndian::LtoN( valu16 ) );
        QA_CHECK( swap_vals16 == sysEndian::LtoN( vals16 ) );
        QA_CHECK( swap_valf == sysEndian::LtoN( valf ) );
        QA_CHECK( swap_vald == sysEndian::LtoN( vald ) );
        
        QA_CHECK( swap_valu64 == sysEndian::NtoL( valu64 ) );
        QA_CHECK( swap_vals64 == sysEndian::NtoL( vals64 ) );
        QA_CHECK( swap_valu32 == sysEndian::NtoL( valu32 ) );
        QA_CHECK( swap_vals32 == sysEndian::NtoL( vals32 ) );
        QA_CHECK( swap_valu16 == sysEndian::NtoL( valu16 ) );
        QA_CHECK( swap_vals16 == sysEndian::NtoL( vals16 ) );
        QA_CHECK( swap_valf == sysEndian::NtoL( valf ) );
        QA_CHECK( swap_vald == sysEndian::NtoL( vald ) );
    }
    else
    {
        QA_CHECK( swap_valu64 == sysEndian::BtoN( valu64 ) );
        QA_CHECK( swap_vals64 == sysEndian::BtoN( vals64 ) );
        QA_CHECK( swap_valu32 == sysEndian::BtoN( valu32 ) );
        QA_CHECK( swap_vals32 == sysEndian::BtoN( vals32 ) );
        QA_CHECK( swap_valu16 == sysEndian::BtoN( valu16 ) );
        QA_CHECK( swap_vals16 == sysEndian::BtoN( vals16 ) );
        QA_CHECK( swap_valf == sysEndian::BtoN( valf ) );
        QA_CHECK( swap_vald == sysEndian::BtoN( vald ) );
          
        QA_CHECK( swap_valu64 == sysEndian::NtoB( valu64 ) );
        QA_CHECK( swap_vals64 == sysEndian::NtoB( vals64 ) );
        QA_CHECK( swap_valu32 == sysEndian::NtoB( valu32 ) );
        QA_CHECK( swap_vals32 == sysEndian::NtoB( vals32 ) );
        QA_CHECK( swap_valu16 == sysEndian::NtoB( valu16 ) );
        QA_CHECK( swap_vals16 == sysEndian::NtoB( vals16 ) );
        QA_CHECK( swap_valf == sysEndian::NtoB( valf ) );
        QA_CHECK( swap_vald == sysEndian::NtoB( vald ) );

        QA_CHECK( valu64 == sysEndian::LtoN( valu64 ) );
        QA_CHECK( vals64 == sysEndian::LtoN( vals64 ) );
        QA_CHECK( valu32 == sysEndian::LtoN( valu32 ) );
        QA_CHECK( vals32 == sysEndian::LtoN( vals32 ) );
        QA_CHECK( valu16 == sysEndian::LtoN( valu16 ) );
        QA_CHECK( vals16 == sysEndian::LtoN( vals16 ) );
        QA_CHECK( valf == sysEndian::LtoN( valf ) );
        QA_CHECK( vald == sysEndian::LtoN( vald ) );
        
        QA_CHECK( valu64 == sysEndian::NtoL( valu64 ) );
        QA_CHECK( vals64 == sysEndian::NtoL( vals64 ) );
        QA_CHECK( valu32 == sysEndian::NtoL( valu32 ) );
        QA_CHECK( vals32 == sysEndian::NtoL( vals32 ) );
        QA_CHECK( valu16 == sysEndian::NtoL( valu16 ) );
        QA_CHECK( vals16 == sysEndian::NtoL( vals16 ) );
        QA_CHECK( valf == sysEndian::NtoL( valf ) );
        QA_CHECK( vald == sysEndian::NtoL( vald ) );
    }

    TST_PASS;
}

QA_ITEM_FAMILY( sysEndianQA, (), () );

QA_ITEM( sysEndianQA, (), qaResult::PASS_OR_FAIL );

#endif  //_QA
