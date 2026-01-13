// 
// grcore/shmoo.cpp
// 
// Copyright (C) 1999-2007 Rockstar Games.  All Rights Reserved. 
//

#include "grcore/shmoo.h"


#if __PPU
#include "bank/bank.h"
#include "grcore/wrapper_gcm.h"
#include "grmodel/shader.h"

#include <algorithm>

namespace rage
{
#if __BANK
	// from device_gcm.cpp
	extern u8 g_MinRegisterCount;

	grcShmoo::grcShmoo(const char* name, u32 instanceCount)
		: m_InstanceCount(instanceCount)
		, m_InstanceIdx(0)
		, m_Name(name)
		, m_Enabled(false)
		, m_InProgress(false)
		, m_Dump(false)
	{
		m_TimeStampLabels = gcm::TimeStampRegistrar::Allocate(m_InstanceCount * 4);
		Assert(gcm::TimeStampRegistrar::IsValid(m_TimeStampLabels));
	}

	grcShmoo::~grcShmoo()
	{
		gcm::TimeStampRegistrar::Free(m_TimeStampLabels, m_InstanceCount * 4);
	}

	void grcShmoo::Begin(const grmShader* shader, int pass)
	{

		if (!m_Enabled || m_TimeStampLabels == 0xffffffff)
		{
			return;
		}
		Assert(shader);

		m_CurrentInstance.Technique = shader->GetCurrentTechnique();
		Assert(m_CurrentInstance.Technique != grcetNONE);
		m_CurrentInstance.Pass = pass;
		Instance& inst = m_Instances[m_CurrentInstance];
#if EFFECT_PRESERVE_STRINGS
		inst.TechniqueName = shader->GetCurrentTechniqueName();
#endif

		m_InitialRegisterCount = shader->GetInitialRegisterCount(pass);
		if (inst.LastRegisterCount == 0xff)
		{
			Assert(inst.Index == 0xffffffff);
			inst.Index = m_InstanceIdx++;
			Assert(inst.Index < m_InstanceCount);
			Assert(inst.Index != 0xffffffff);
		}
		else
		{
			m_PrevMinRegisterCount = g_MinRegisterCount;
			g_MinRegisterCount = inst.LastRegisterCount;
		}

		m_Flip = GRCDEVICE.GetFrameCounter() & 1;
		m_SetStartTimeStampLabel = m_TimeStampLabels + m_Flip * (2 * m_InstanceCount) + (2 * inst.Index);
		Assert(m_SetStartTimeStampLabel < GCM_REPORT_COUNT);
		GCM_DEBUG(GCM::cellGcmSetTimeStamp(GCM_CONTEXT, m_SetStartTimeStampLabel));

		m_InProgress = true;
	}

	void grcShmoo::End()
	{
		if (!m_Enabled || !m_InProgress || m_TimeStampLabels == 0xffffffff)
		{
			m_InProgress = false;
			return;
		}

		Assert(m_Flip == (GRCDEVICE.GetFrameCounter() & 1));

		Instance& inst = m_Instances[m_CurrentInstance];

		Assert(m_SetStartTimeStampLabel + 1 < GCM_REPORT_COUNT);
		GCM_DEBUG(GCM::cellGcmSetTimeStamp(GCM_CONTEXT, m_SetStartTimeStampLabel + 1));
		if (inst.LastRegisterCount != 0xff)
		{
			g_MinRegisterCount = m_PrevMinRegisterCount;
			u32 getStartTimeStampLabel = m_TimeStampLabels + (1 - m_Flip) * (2 * m_InstanceCount) + (2 * inst.Index);
			u64 getStartTimeStamp = cellGcmGetTimeStampLocation(getStartTimeStampLabel,CELL_GCM_LOCATION_MAIN);
			u64 getEndTimeStamp = cellGcmGetTimeStampLocation(getStartTimeStampLabel + 1,CELL_GCM_LOCATION_MAIN);
			inst.Samples[inst.SampleCount++] = getEndTimeStamp - getStartTimeStamp;
			if (inst.SampleCount == kNumSamples)
			{
				std::sort(inst.Samples, inst.Samples + kNumSamples);
				u64 averageSample = 0ull;
				for (u32 i = kNumSamplesIgnore; i < kNumSamples - kNumSamplesIgnore; ++i)
				{
					averageSample += inst.Samples[i];
				}
				averageSample /= (kNumSamples - 2 * kNumSamplesIgnore);

				if (averageSample < kMaxSampleValue)
				{
					if (averageSample > inst.WorstSample)
					{
						inst.WorstSample = averageSample;
						inst.WorstRegisterCount = inst.LastRegisterCount;
					}
					if (inst.LastRegisterCount == m_InitialRegisterCount)
					{
						inst.DefaultSample = averageSample;
					}
					if (averageSample < inst.BestSample)
					{
						inst.BestSample = averageSample;
						if (inst.BestRegisterCount != inst.LastRegisterCount)
						{
							inst.BestRegisterCount = inst.LastRegisterCount;

							float worstSample = static_cast<float>(static_cast<double>(inst.WorstSample) / 1000000.0);
							float bestSample = static_cast<float>(static_cast<double>(inst.BestSample) / 1000000.0);
							float defaultSample = static_cast<float>(static_cast<double>(inst.DefaultSample) / 1000000.0);
							Printf("Technique: \"%s\" (pass: %d)\n\tBEST: register count=%d sample=%f\n\tWORST: register count=%d sample=%f\n\tDEFAULT: register count=%d sample=%f\n",
								inst.TechniqueName.c_str(), m_CurrentInstance.Pass,
								inst.BestRegisterCount, bestSample,
								inst.WorstRegisterCount, worstSample,
								m_InitialRegisterCount, defaultSample);

#if 0
							u64 worstTotal = 0ull;
							u64 bestTotal = 0ull;
							u64 defaultTotal = 0ull;
							for (InstanceMap::const_iterator i = m_Instances.begin(); i != m_Instances.end(); ++i)
							{
								const Instance& inst = i->second;
								worstTotal += inst.WorstSample;
								if (inst.BestSample != ~0ull)
								{
									bestTotal += inst.BestSample;
								}
								defaultTotal += inst.DefaultSample;
							}
							Printf("\tTOTALS worst=%f best=%f default=%f\n",
								static_cast<float>(static_cast<double>(worstTotal) / 1000000.0),
								static_cast<float>(static_cast<double>(bestTotal) / 1000000.0),
								static_cast<float>(static_cast<double>(defaultTotal) / 1000000.0));
#endif // 0
						}
					}

					inst.LastRegisterCount = Max<u8>((inst.LastRegisterCount + 1) % kMaxRegister, m_InitialRegisterCount);
				}

				inst.SampleCount = 0;
			}
		}
		else
		{
			inst.LastRegisterCount = m_InitialRegisterCount;
		}

		if (m_Dump)
		{
			for (InstanceMap::const_iterator i = m_Instances.begin(); i != m_Instances.end(); ++i)
			{
				const Instance& inst = i->second;
				if ( inst.LastRegisterCount != 0xff )
				{
					const float bestSample = static_cast<float>(static_cast<double>(inst.BestSample) / 1000000.0);
					const float defaultSample = static_cast<float>(static_cast<double>(inst.DefaultSample) / 1000000.0);
					const float savings = defaultSample - bestSample;
					Printf("Technique: %s pass: %d --- CGC_FLAGS(\"-regcount %d\") saves %fms\n", inst.TechniqueName.c_str(), i->first.Pass, inst.BestRegisterCount, savings);
				}
			}
			m_Dump = false;
		}

		m_InProgress = false;
	}

	void grcShmoo::AddWidgets(bkBank& bank)
	{
		char buffer[1024];
		snprintf(buffer, 1024, "Enable shmooing (%s)", m_Name.c_str());
		bank.AddToggle(buffer, &m_Enabled);
		snprintf(buffer, 1024, "Dump shmoos (%s)", m_Name.c_str());
		bank.AddToggle(buffer, &m_Dump);
	}
	
	void grcShmoo::GetResults(atArray<Result> &results, float minDelta ) const
	{
		for (InstanceMap::const_iterator i = m_Instances.begin(); i != m_Instances.end(); ++i)
		{
			const Instance& inst = i->second;
			if ( inst.LastRegisterCount != 0xff )
			{
				const float bestSample = static_cast<float>(static_cast<double>(inst.BestSample) / 1000000.0);
				const float defaultSample = static_cast<float>(static_cast<double>(inst.DefaultSample) / 1000000.0);
				const float savings = defaultSample - bestSample;
				
				if( savings > minDelta )
				{
					Result & res = results.Grow();
					res.TechniqueName = inst.TechniqueName;
					res.PassIndex = i->first.Pass;
					res.RegisterCount = inst.BestRegisterCount;
				}
			}
		}
	}
#endif // __BANK
	
	void grcShmoo::ApplyResults(grmShader* shader, atArray<Result> &results)
	{
		for(int i=0;i<results.GetCount();i++)
		{
			const Result & res = results[i];
			
			shader->SetInitialRegisterCount(res.TechniqueName, res.PassIndex, res.RegisterCount);
		}
	}


} // namespace rage

#endif // __PPU
