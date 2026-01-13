// 
// system/dependency.h 
// 
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved. 
// 

#ifndef SYSTEM_DEPENDENCY_H
#define SYSTEM_DEPENDENCY_H

#include "dependency_config.h"

#include "math/amath.h"
#include "system/bit.h"
#include "system/codefrag_spu.h"

namespace rage
{

////////////////////////////////////////////////////////////////////////////////

// PURPOSE: Dependency task
// Describes an atomic packet of work, including inputs, outputs
// plus relation to past/subsequent packets
struct ALIGNAS(16) sysDependency
{
	enum eChain
	{
		kChainPpu = 0,
		kChainSpu,
		kChainNum,
	};

	enum ePriority
	{
		kPriorityLow = 0,
		kPriorityMed,
		kPriorityHigh,
		kPriorityCritical,
		kPriorityNum,
	};

	enum eParent
	{
		kParentStrong = 0,
		kParentWeak,
	};

	// PURPOSE: Signature of function to invoke to process dependency
	typedef bool (Callback)(const sysDependency&);

	// PURPOSE: Initializer for ppu dependency
	void Init(Callback* cb, u8 id=0, u32 flags=0);

	// PURPOSE: Initializer for spu dependency
	void Init(const spuCodeFragment& cb, u8 id=0, u32 flags=0);

	// PURPOSE: Set child of this dependency
	void AddChild(sysDependency& child);

	// PURPOSE: Add a weak child to this dependency (not a strict child, instead weak graph link)
	void AddWeakChild(sysDependency& child);

	// PURPOSE: Maximum number of parameters supported
	static const u32 sm_MaxNumParameters = 10;

	// PURPOSE: Maximum number of data supported
	static const u32 sm_MaxNumData = 8;

	// PURPOSE: Maximum number of parent supported
	static const u32 sm_MaxNumParents = 2;

	union
	{
		float m_AsFloat;
		bool m_AsBool;
		int m_AsInt;
		u32 m_AsUInt;
		void* m_AsPtr;
		const void* m_AsConstPtr;
		struct { u16 m_Low, m_High; } m_AsShort;
	} m_Params[sm_MaxNumParameters];

	sysDependency* m_Parents[sm_MaxNumParents];
	Callback* m_Callback;

	u32 m_NumPending;
	u32 m_Flags;
	u32 m_DataSizes[sm_MaxNumData];
	u16 m_CodeSize;
	u8 m_Priority;
	u8 m_Chain : 1;
	u8 m_Id : 5;
	u8 m_JobFailureCount : 2;
} ;

////////////////////////////////////////////////////////////////////////////////

namespace sysDepFlag
{
enum {	INPUT0=BIT0, INPUT1=BIT1, INPUT2=BIT2, INPUT3=BIT3, INPUT4=BIT4, INPUT5=BIT5, INPUT6=BIT6, INPUT7=BIT7,
		OUTPUT0=BIT8, OUTPUT1=BIT9, OUTPUT2=BIT10, OUTPUT3=BIT11, OUTPUT4=BIT12, OUTPUT5=BIT13, OUTPUT6=BIT14, OUTPUT7=BIT15,
		ALLOC0=BIT16 };
}

////////////////////////////////////////////////////////////////////////////////

__forceinline void sysDependency::Init(Callback* cb, u8 NOTFINAL_ONLY(id), u32 flags)
{
	NOTFINAL_ONLY(m_Id = id);
	m_NumPending = 0;
	m_Parents[kParentStrong] = m_Parents[kParentWeak] = NULL;
	m_Chain = kChainPpu;
	m_Callback = cb;
	m_Flags = flags;
	m_JobFailureCount = 0;
}

////////////////////////////////////////////////////////////////////////////////

__forceinline void sysDependency::Init(const spuCodeFragment& cb, u8 NOTFINAL_ONLY(id), u32 flags)
{
	NOTFINAL_ONLY(m_Id = id);
	m_NumPending = 0;
	m_Parents[kParentStrong] = m_Parents[kParentWeak] = NULL;
	m_Chain = SYS_USE_SPU_DEPENDENCY ? kChainSpu : kChainPpu;
	m_Callback = reinterpret_cast<Callback*>(cb.pCodeFrag);
	Assign(m_CodeSize, cb.codeFragSize);
	m_Flags = flags;
	m_JobFailureCount = 0;
}

////////////////////////////////////////////////////////////////////////////////

__forceinline void sysDependency::AddChild(sysDependency& child)
{
	FastAssert(!child.m_Parents[kParentStrong]);
	child.m_Parents[kParentStrong] = this;
	++m_NumPending;
}

////////////////////////////////////////////////////////////////////////////////

__forceinline void sysDependency::AddWeakChild(sysDependency& child)
{
	FastAssert(!child.m_Parents[kParentWeak]);
	child.m_Parents[kParentWeak] = this;
	++m_NumPending;
}

////////////////////////////////////////////////////////////////////////////////

} // namespace rage

#endif  // SYSTEM_DEPENDENCY_H
