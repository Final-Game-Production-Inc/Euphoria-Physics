// 
// grcore/shmoo.h
// 
// Copyright (C) 1999-2007 Rockstar Games.  All Rights Reserved. 
// 

#ifndef GRCORE_SHMOO_H
#define GRCORE_SHMOO_H

#include "atl/string.h"
#include "grcore/effect_typedefs.h"

#include <map>

namespace rage
{
	class grmShader;
	class bkBank;

	class grcShmoo
	{
	public:
		grcShmoo(const char* name, u32 numInstances);
		~grcShmoo();

		void Begin(const grmShader* shader, int pass);
		void End();

		void SetEnabled(bool enabled) { m_Enabled = enabled; }
		bool IsEnabled() const { return m_Enabled; }

	#if __BANK
		void AddWidgets(bkBank& bank);
	#endif // __BANK

	public:
		struct Result
		{
			Result() 
				: RegisterCount(0xff)
				, PassIndex(0xffffffff)
			{
			}
			ConstString TechniqueName;
			u32 PassIndex;
			u8 RegisterCount;
		};

		const ConstString & GetName() const { return m_Name; }
		void GetResults(atArray<Result> &results, float minDelta = 0.01f ) const;
		static void ApplyResults(grmShader* shader, atArray<Result> &results);
		
	private:
		static const u32 kNumSamples = 64;
		static const u32 kNumSamplesIgnore = 3; // Number of worst/best case samples to ignore
		static const u8 kMaxRegister = 48;
		static const u64 kMaxSampleValue = 100000000000000000ull;

		struct Instance
		{
			Instance()
				: SampleCount(0)
				, BestSample(~0ull)
				, WorstSample(0ull)
				, DefaultSample(0ull)
				, LastRegisterCount(0xff)
				, BestRegisterCount(0xff)
				, WorstRegisterCount(0xff)
				, Index(0xffffffff)
			{
			}
			u64 Samples[kNumSamples];
			u32 SampleCount;
			u64 BestSample;
			u64 WorstSample;
			u64 DefaultSample;
			u8 LastRegisterCount;
			u8 BestRegisterCount;
			u8 WorstRegisterCount;
			ConstString TechniqueName;
			u32 Index;
		};
		struct InstanceKey
		{
			grcEffectTechnique Technique;
			int Pass;

			bool operator<(const InstanceKey& key) const
			{
				if (Technique < key.Technique)
					return true;
				else if (Technique == key.Technique)
					return Pass < key.Pass;
				else
					return false;
			}
		};
		typedef std::map<InstanceKey, Instance> InstanceMap;
		InstanceMap m_Instances;
		InstanceKey m_CurrentInstance;
		u32 m_InstanceCount;
		u32 m_InstanceIdx;
		u32 m_TimeStampLabels;
		u32 m_SetStartTimeStampLabel;
		u32 m_Flip;
		ConstString m_Name;
		bool m_Enabled;
		bool m_InProgress;
		bool m_Dump;
		u8 m_PrevMinRegisterCount;
		u8 m_InitialRegisterCount;
	};

} // namespace rage

#endif // GRCORE_SHMOO_H
