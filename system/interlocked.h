// 
// system/interlocked.h 
// 
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved. 
// 

#ifndef SYSTEM_INTERLOCKED_H
#define SYSTEM_INTERLOCKED_H

#include <stddef.h>

namespace rage
{

//PURPOSE
//  Atomically exchanges the destination value with the exchange value.
//RETURNS
//  The initial value of destination.
u32 sysInterlockedExchange(volatile u32* destination, u32 exchange);
u64 sysInterlockedExchange(volatile u64* destination, u64 exchange);

//PURPOSE
//  Atomically compares the destination value with the comparand and if they
//  are equal exchanges the destination value with the exchange value.
//RETURNS
//  The initial value of destination.
u32 sysInterlockedCompareExchange(volatile u32* destination, u32 exchange, u32 comparand);
u64 sysInterlockedCompareExchange(volatile u64* destination, u64 exchange, u64 comparand);

//PURPOSE
//  Atomically exchanges the destination value with the exchange value.
//RETURNS
//  The initial value of destination.
void* sysInterlockedExchangePointer(void* volatile* destination, void* exchange);

//PURPOSE
//  Atomically compares the destination value with the comparand and if they
//  are equal exchanges the destination value with the exchange value.
//RETURNS
//  The initial value of destination.
void* sysInterlockedCompareExchangePointer(void* volatile* destination, void* exchange, void* comparand);

//PURPOSE
//  Atomically compares the destination value with the comparand and if they
//  are equal exchanges the destination value with the exchange value.
//RETURNS
//  The initial value of destination.
template<class T>
T* sysInterlockedCompareExchange(T* volatile* destination, T* exchange, T* comparand);

//PURPOSE
//  Atomically increments the destination value.
//RETURNS
//  The incremented value.
u32 sysInterlockedIncrement(volatile u32* destination);
u16 sysInterlockedIncrement(volatile u16* destination);
u8  sysInterlockedIncrement(volatile u8*  destination);

//PURPOSE
//  Atomically increments the destination value.
//  May be a tiny bit faster than sysInterlockedIncrement(volatile T*) on some platforms,
//  but caller MUST garuntee not to overflow or underflow the value.
//RETURNS
//  The incremented value.
u16 sysInterlockedIncrement_NoWrapping(volatile u16* destination);
u8  sysInterlockedIncrement_NoWrapping(volatile u8*  destination);

//PURPOSE
//  Atomically decrements the destination value.
//RETURNS
//  The decremented value.
u32 sysInterlockedDecrement(volatile u32* destination);
u16 sysInterlockedDecrement(volatile u16* destination);
u8  sysInterlockedDecrement(volatile u8*  destination);

//PURPOSE
//  Atomically decrements the destination value.
//  May be a tiny bit faster than sysInterlockedDecrement(volatile T*) on some platforms,
//  but caller MUST garuntee not to overflow or underflow the value.
//RETURNS
//  The incremented value.
u16 sysInterlockedDecrement_NoWrapping(volatile u16* destination);
u8  sysInterlockedDecrement_NoWrapping(volatile u8*  destination);

//PURPOSE
//  Atomically adds to the destination value.
//RETURNS
//  The original value.
u32 sysInterlockedAdd(volatile u32* destination, s32 value);
u64 sysInterlockedAdd(volatile u64* destination, s64 value);
void* sysInterlockedAddPointer(volatile void** destination, ptrdiff_t value);

//PURPOSE
//  Atomically bitwise ORs to the destination value.
//RETURNS
//  The original value.
u32 sysInterlockedOr(volatile u32* destination, u32 value);
u64 sysInterlockedOr(volatile u64* destination, u64 value);

//PURPOSE
//  Atomically bitwise ANDs to the destination value.
//RETURNS
//  The original value.
u32 sysInterlockedAnd(volatile u32* destination, u32 value);

//PURPOSE
//  Atomically reads the target value
//RETURNS
//  The original value.
u32 sysInterlockedRead(const volatile u32* target);
u64 sysInterlockedRead(const volatile u64* target);

//PURPOSE
//  Atomically reads the target pointer
//RETURNS
//  The original value.
void* sysInterlockedReadPointer(void* const volatile* target);

} //namespace rage

#include "interlocked.inl"

#endif  //SYSTEM_INTERLOCKED_H
