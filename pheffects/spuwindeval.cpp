// 
// pheffects/spuwindeval.cpp 
// 
// Copyright (C) 1999-2009 Rockstar Games.  All Rights Reserved. 
// 

#include "spuwindeval.h"
#include "../../suite/src/rmptfx/ptxrandomtable.h"

#include "wind.h"

#if __SPU
#ifndef SPU_DONT_INCLUDE_GLOBAL_DISTURBANCES
	#include "winddisturbancedownwash.cpp"
	#include "winddisturbancesphere.cpp"
	#include "winddisturbanceexplosion.cpp"
	#include "winddisturbancehemisphere.cpp"
	#include "winddisturbancethermal.cpp"
#endif // SPU_DONT_INCLUDE_GLOBAL_DISTURBANCES
#endif


#include "system/dma.h"

namespace rage 
{
	void phSpuWindEval::Init(phSpuWindEvalData& data)
	{
		m_pData = &data;
		m_pSpuDisturbanceData = NULL;
	}

#if !__SPU
	void phSpuWindEvalData::SetData(phWind& wind)
	{
		m_elapsedTime = wind.m_elapsedTime;

		for (int i=0; i<2; i++)
		{
			m_settings[i].speed = wind.m_settings[i].speed;
			m_settings[i].globalVariationSinPosMult = wind.m_settings[i].globalVariationSinPosMult;
			m_settings[i].globalVariationSinTimeMult = wind.m_settings[i].globalVariationSinTimeMult;
			m_settings[i].globalVariationCosPosMult = wind.m_settings[i].globalVariationCosPosMult;
			m_settings[i].globalVariationCosTimeMult = wind.m_settings[i].globalVariationCosTimeMult;
			m_settings[i].positionalVariationMult = wind.m_settings[i].positionalVariationMult;
			m_settings[i].positionalVariationIsFluid = wind.m_settings[i].positionalVariationIsFluid;

			m_windState[i].vBaseVelocity = wind.m_windState[i].vBaseVelocity;
			m_windState[i].vGustVelocity = wind.m_windState[i].vGustVelocity;
			m_windState[i].vGlobalVariationVelocity = wind.m_windState[i].vGlobalVariationVelocity;
		}

		m_disturbanceField = &wind.m_disturbanceField.GetDataBase();

		m_basePosWldX = phWindField::GetBasePosWldX();
		m_basePosWldY = phWindField::GetBasePosWldY();
		m_basePosWldZ = phWindField::GetBasePosWldZ();

		m_waterLevelWld = phWindField::GetWaterLevelWld();
	}

	void phSpuWindEvalData::SetDisturbancePtrs(phWind& wind)
	{
		m_disturbanceGroups.m_downwashGroup = &(wind.m_downwashGroup);
		m_disturbanceGroups.m_sphereGroup = &(wind.m_sphereGroup);
		m_disturbanceGroups.m_explosionGroup = &(wind.m_explosionGroup);
		m_disturbanceGroups.m_dirExplosionGroup = &(wind.m_dirExplosionGroup);
		m_disturbanceGroups.m_hemisphereGroup = &(wind.m_hemisphereGroup);
		m_disturbanceGroups.m_thermalGroup = &(wind.m_thermalGroup);
	}
#endif

	void phSpuWindEval::GetLocalVelocity(Vec3V_In vPos, Vec3V_InOut vWindVel, bool includeWindFieldDisturbance, bool includeGlobalDisturbances)
	{
		WindType_e windType = WIND_TYPE_AIR;
		if (vPos.GetZf()<=m_pData->m_waterLevelWld)
		{
			windType = WIND_TYPE_WATER;
		}

		includeGlobalDisturbances = false;

		ScalarV windSpeed = ScalarVFromF32(m_pData->m_settings[WIND_TYPE_AIR].speed);
		ScalarV waterSpeed = ScalarVFromF32(m_pData->m_settings[WIND_TYPE_WATER].speed);

		vWindVel = m_pData->m_windState[windType].vBaseVelocity;

		// add on gusts
		vWindVel += m_pData->m_windState[windType].vGustVelocity;

		// add on global variation (on a sine wave)
		float gustMultX = (FPSin((vPos.GetXf()*m_pData->m_settings[windType].globalVariationSinPosMult)+(m_pData->m_elapsedTime*m_pData->m_settings[windType].globalVariationSinTimeMult))+1.0f) * 0.5f;
		float gustMultY = (FPSin((vPos.GetYf()*m_pData->m_settings[windType].globalVariationCosPosMult)+(m_pData->m_elapsedTime*m_pData->m_settings[windType].globalVariationCosTimeMult))+1.0f) * 0.5f;
		vWindVel.SetXf(vWindVel.GetXf() + m_pData->m_windState[windType].vGlobalVariationVelocity.GetXf() * gustMultX);
		vWindVel.SetYf(vWindVel.GetYf() + m_pData->m_windState[windType].vGlobalVariationVelocity.GetYf() * gustMultY);
		vWindVel.SetZf(vWindVel.GetZf() + m_pData->m_windState[windType].vGlobalVariationVelocity.GetZf());

		// add on positional variation
		vWindVel += GetPositionalVariationVelocity(windType, vPos);

		// add any global disturbance field velocity
		if (includeWindFieldDisturbance)
		{
#ifndef SPU_DONT_INCLUDE_GLOBAL_DISTURBANCES
			if (includeGlobalDisturbances)
			{
				if (m_pData->m_disturbanceGroups.m_downwashGroup)
				{
					vWindVel += m_pData->m_disturbanceGroups.m_downwashGroup->GetVelocity(vPos, windSpeed, waterSpeed);
				}
				if (m_pData->m_disturbanceGroups.m_sphereGroup)
				{
					vWindVel += m_pData->m_disturbanceGroups.m_sphereGroup->GetVelocity(vPos, windSpeed, waterSpeed);
				}
				if (m_pData->m_disturbanceGroups.m_explosionGroup)
				{
					vWindVel += m_pData->m_disturbanceGroups.m_explosionGroup->GetVelocity(vPos, windSpeed, waterSpeed);
				}
				if (m_pData->m_disturbanceGroups.m_dirExplosionGroup)
				{
					vWindVel += m_pData->m_disturbanceGroups.m_dirExplosionGroup->GetVelocity(vPos, windSpeed, waterSpeed);
				}
				if (m_pData->m_disturbanceGroups.m_hemisphereGroup)
				{
					vWindVel += m_pData->m_disturbanceGroups.m_hemisphereGroup->GetVelocity(vPos, windSpeed, waterSpeed);
				}
				if (m_pData->m_disturbanceGroups.m_thermalGroup)
				{
					vWindVel += m_pData->m_disturbanceGroups.m_thermalGroup->GetVelocity(vPos, windSpeed, waterSpeed);
				}
			}
#endif // SPU_DONT_INCLUDE_GLOBAL_DISTURBANCES

			// add on velocity from the disturbance field
			u32	gridX, gridY, gridZ;
			if (CalcGridPos((int)vPos.GetXf(), (int)vPos.GetYf(), (int)vPos.GetZf(), gridX, gridY, gridZ))
			{
#if __SPU
				const int dmaTag = 8;

				// get data from PPU (truncate the old addrs since we got a 16 bytes aligned + extra addr before)
				u32 ppuAddress = 0x0;
				u32 addrOffset = 0x0;

				ppuAddress = (u32)FindDataAddress(gridX, gridY, gridZ, m_pData->m_disturbanceField);
				addrOffset = ppuAddress & 0xf;
				ppuAddress &= ~0xf;
				sysDmaGet(&m_spuDisturbanceDataStorage, ppuAddress, sizeof(m_spuDisturbanceDataStorage), dmaTag);
				m_pSpuDisturbanceData = (phWindPoint*)(m_spuDisturbanceDataStorage + addrOffset);

				sysDmaWaitTagStatusAll(1 << dmaTag);

				vWindVel += m_pSpuDisturbanceData->GetVelocity();
#else
				vWindVel += FindDataAddress(gridX, gridY, gridZ, m_pData->m_disturbanceField)->GetVelocity();
#endif
			}
		}
	}

	Vec3V_Out phSpuWindEval::GetPositionalVariationVelocity(int windType, Vec3V_In vPos)
	{
		Vec3V vQueryPos = vPos;
		if (!m_pData->m_settings[windType].positionalVariationIsFluid)
		{
			// if we are using fluid positional variation then add to the wind field at this grid element
			vQueryPos.SetXf((float)(int)vQueryPos.GetXf());
			vQueryPos.SetYf((float)(int)vQueryPos.GetYf());
			vQueryPos.SetZf((float)(int)vQueryPos.GetZf());
		}

		u8 randIndex = (u8)((int)(vQueryPos.GetXf() * vQueryPos.GetYf() * vQueryPos.GetZf()) % PTXRANDTABLE_SIZE);

		Vec3V vRandDir(ptxRandomTable::GetValue(randIndex)-0.5f, ptxRandomTable::GetValue((u8)(randIndex+1))-0.5f, ptxRandomTable::GetValue((u8)(randIndex+2))-0.5f);
		vRandDir = Normalize(vRandDir);
		vRandDir *= ScalarVFromF32(m_pData->m_settings[windType].speed*m_pData->m_settings[windType].positionalVariationMult);

		return vRandDir;
	}

	typedef phWindPoint ptxWindArray[WINDFIELD_NUM_ELEMENTS_X][WINDFIELD_NUM_ELEMENTS_Y][WINDFIELD_NUM_ELEMENTS_Z];

	inline phWindPoint* phSpuWindEval::FindDataAddress(u32 x, u32 y, u32 z, phWindPoint* baseAddr)
	{
		int idx = x * (WINDFIELD_NUM_ELEMENTS_Y * WINDFIELD_NUM_ELEMENTS_Z) + y * (WINDFIELD_NUM_ELEMENTS_Z) + z;
		return baseAddr + idx;
	}

	inline bool phSpuWindEval::CalcGridPos(int WorldX, int WorldY, int WorldZ, u32 &GridX, u32 &GridY, u32 &GridZ)
	{
		if ((WorldX >= m_pData->m_basePosWldX && WorldX < m_pData->m_basePosWldX + WINDFIELD_NUM_ELEMENTS_X * WINDFIELD_ELEMENT_SIZE_X) &&
			(WorldY >= m_pData->m_basePosWldY && WorldY < m_pData->m_basePosWldY + WINDFIELD_NUM_ELEMENTS_Y * WINDFIELD_ELEMENT_SIZE_Y) &&
			(WorldZ >= m_pData->m_basePosWldZ && WorldZ < m_pData->m_basePosWldZ + WINDFIELD_NUM_ELEMENTS_Z * WINDFIELD_ELEMENT_SIZE_Z))
		{
			GridX = ((u32)(WorldX / WINDFIELD_ELEMENT_SIZE_X)) % WINDFIELD_NUM_ELEMENTS_X;
			GridY = ((u32)(WorldY / WINDFIELD_ELEMENT_SIZE_Y)) % WINDFIELD_NUM_ELEMENTS_Y;
			GridZ = ((u32)(WorldZ / WINDFIELD_ELEMENT_SIZE_Z)) % WINDFIELD_NUM_ELEMENTS_Z;
			return true;
		}
		return false;
	}

} // namespace rage
