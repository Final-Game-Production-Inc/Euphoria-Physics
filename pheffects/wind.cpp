//
// pheffects/wind.cpp
//
// Copyright (C) 1999-2008 Rockstar Games.  All Rights Reserved.
//

///////////////////////////////////////////////////////////////////////////////
//  INCLUDES
///////////////////////////////////////////////////////////////////////////////

#include "wind.h"
#include "wind_optimisations.h"
#include "grcore/debugdraw.h"
#include "vector/colors.h"
#include "vectormath/legacyconvert.h"
#include "profile/timebars.h"
#include "math/vecrand.h"
#include "system/cache.h"

#if __BANK
#include "bank\bkmgr.h"
#include "bank\bank.h"
#include "bank\combo.h"
#include "bank\slider.h"
#endif

WIND_OPTIMISATIONS()

namespace rage
{
	///////////////////////////////////////////////////////////////////////////////
	//  DEFINES
	///////////////////////////////////////////////////////////////////////////////

	#define NUM_VARIATION_ENTRIES			(64)
	#define VARIATION_ENTRY_MASK			(NUM_VARIATION_ENTRIES - 1)


	///////////////////////////////////////////////////////////////////////////////
	//  STATIC MEMBERS OF phWindField
	///////////////////////////////////////////////////////////////////////////////

	int phWindField::m_basePosWldX = 0;
	int phWindField::m_basePosWldY = 0;
	int phWindField::m_basePosWldZ = 0;

	float phWindField::m_waterLevelWld = 0;


	///////////////////////////////////////////////////////////////////////////////
	//  GLOBAL VARIABLES
	///////////////////////////////////////////////////////////////////////////////

	float g_variationDir			[NUM_VARIATION_ENTRIES] =
	{ 
		 0.5f,	-0.3f,	 0.8f,	 0.0f,	-0.4f,	-0.8f,	 0.3f,	-0.1f, 
		-0.9f,	-0.5f,	 0.7f,	 0.7f,	 0.3f,	 0.7f,	 0.0f,	-0.5f,
		 0.2f,	-1.0f,	 0.3f,	 0.0f,	 0.4f,	 0.6f,	-0.2f,	 0.1f,  
		 1.0f,	 0.9f,	 0.0f,	-0.4f,	-0.5f,	-0.1f,	 0.3f,	 0.7f,
		-0.4f,	 1.0f,	 0.7f,	 1.0f,	 0.2f,	-0.5f,	-0.7f,	 0.8f, 
		 0.4f,	-0.1f,	 0.0f,	 0.3f,	 0.5f,	-0.7f,	-0.3f,	 0.5f,
		 0.2f,	 0.5f,	-0.3f,	-0.3f,	 0.0f,	 0.0f,	 1.0f,	 0.4f, 
		-0.4f,	-0.5f,	-0.3f,	 0.4f,	 0.9f,	 0.6f,	 0.1f,	 0.0f
	};

	float g_variationMult			[NUM_VARIATION_ENTRIES] =
	{ 
		1.0f,	0.75f,	1.0f,	0.6f,	0.7f,	1.0f,	1.0f,	1.0f, 
		1.0f,	0.9f,	0.5f,	1.0f,	1.0f,	0.85f,	1.0f,	1.0f,
		0.6f,	1.0f,	1.0f,	0.9f,	0.8f,	1.0f,	0.9f,	1.0f, 
		1.0f,	0.5f,	0.7f,	1.0f,	1.0f,	1.0f,	0.7f,	1.0f,	
		1.0f,	0.75f,	1.0f,	1.0f,	0.8f,	0.7f,	1.0f,	1.0f, 
		1.0f,	1.0f,	0.7f,	0.6f,	1.0f,	1.0f,	1.0f,	1.0f,	
		0.8f,	1.0f,	0.7f,	1.0f,	1.0f,	1.0f,	0.8f,	0.7f, 
		0.9f,	1.0f,	1.0f,	1.0f,	0.6f,	1.0f,	1.0f,	1.0f
	};

	static mthVecRand g_vecRand;

	///////////////////////////////////////////////////////////////////////////////
	//  CODE
	///////////////////////////////////////////////////////////////////////////////

	//copy from framework's vectormathutil.h because we cannot include framework code into rage base code
	__forceinline Vec4V_Out SinApprox(Vec4V_In x) // super-fast quadratic approx to Sin(x), accurate to within +/-0.056
	{
		const Vec4V t = x*Vec4V(V_ONE_OVER_PI)*Vec4V(V_HALF);
		const Vec4V a = SubtractScaled(Vec4V(V_ONE), Vec4V(V_TWO), t - RoundToNearestIntNegInf(t));
		const Vec4V b = Vec4V(V_ONE) - Abs(a);
		const Vec4V y = Vec4V(V_FOUR)*a*b;
		return y;
	}
	///////////////////////////////////////////////////////////////////////////////
	//  CLASS phWind
	///////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////
	//  Constructor
	///////////////////////////////////////////////////////////////////////////////

					phWind::phWind								()
	{
		// main data
		m_elapsedTime = 0.0f;
		m_vElapsedTime = ScalarV(V_ZERO);

		for (int i=0; i<WIND_TYPE_NUM; i++)
		{
			// settings
			m_settings[i].angle = 0.0f;
			m_settings[i].speed = 0.0f;
			m_settings[i].maxSpeed = 0.0f;
			m_settings[i].globalVariationMult = 0.25f;
			m_settings[i].globalVariationSinPosMult = 1.0f;
			m_settings[i].globalVariationSinTimeMult = 1.0f;
			m_settings[i].globalVariationCosPosMult = 1.0f;
			m_settings[i].globalVariationCosTimeMult = 1.0f;
			m_settings[i].positionalVariationMult = 0.1f;
			m_settings[i].positionalVariationIsFluid = false;
			m_settings[i].gustChance = 0.0f;
			m_settings[i].gustSpeedMultMin = 0.0f;
			m_settings[i].gustSpeedMultMax = 0.0f;
			m_settings[i].gustTimeMin = 0.0f;
			m_settings[i].gustTimeMax = 0.0f;

			// wind state
			m_windState[i].vBaseVelocity = Vec3V(V_ZERO);
			m_windState[i].vGustVelocity = Vec3V(V_ZERO);
			m_windState[i].vGlobalVariationVelocity = Vec3V(V_ZERO);

			// gust state
			m_gustState[i].gustDir = 0.0f;
			m_gustState[i].gustSpeed = 0.0f;
			m_gustState[i].gustTotalTime = 0.0f;
			m_gustState[i].gustCurrTime = 0.0f;
		}
		
		// disturbances
		m_disturbanceFallOff = 10.0f;

		// debug
#if __BANK
		m_drawScale = 0.1f;
		m_numGlobalWindQueries = 0;
		m_numLocalWindQueries = 0;
		m_debugDrawWindFieldFull = false;
		m_debugDrawWindFieldNoDisturbances = false;
		m_debugDrawDisturbanceField = false;
		m_debugDrawDisturbanceBounds = false;
		m_debugDrawWindQueries = false;
		m_pauseUpdate = false;
		m_dontApplyGusts = false;
		m_dontApplyVariation = false;
		m_dontApplyDisturbances = false;
#endif
	}

	///////////////////////////////////////////////////////////////////////////////
	//  InitClass
	///////////////////////////////////////////////////////////////////////////////

	void			phWind::InitClass							()
	{
		g_wind::Instantiate();
	}


	///////////////////////////////////////////////////////////////////////////////
	//  ShutdownClass
	///////////////////////////////////////////////////////////////////////////////

	void			phWind::ShutdownClass						()
	{
		g_wind::Destroy();
	}


	///////////////////////////////////////////////////////////////////////////////
	//  InitSession
	///////////////////////////////////////////////////////////////////////////////

	void			phWind::InitSession							()
	{
		//@@: location PHWIND_INITSESSION
		m_disturbanceField.Init();

		m_downwashGroup.Reset();
		m_sphereGroup.Reset();
		m_explosionGroup.Reset();
		m_dirExplosionGroup.Reset();
		m_hemisphereGroup.Reset();
		m_thermalGroup.Reset();
	}


	///////////////////////////////////////////////////////////////////////////////
	//  Update
	///////////////////////////////////////////////////////////////////////////////

	void			phWind::Update								(float dt, u32 unpausedElapsedTimeInMs, u32 frameCount, bool skipCalcs)
	{
#if __BANK
		m_numGlobalWindQueries = 0;
		m_numLocalWindQueries = 0;

		if (m_pauseUpdate)
		{
			return;
		}
#endif
		m_elapsedTime += dt;
		m_vElapsedTime = ScalarV(m_elapsedTime);

		if( !skipCalcs )
		{
			CalcBaseVelocity();
			CalcGustVelocity(dt);
			CalcGlobalVariationVelocity(unpausedElapsedTimeInMs);
		}

		// update the disturbance fields
		int batch = frameCount % WIND_NUM_BATCHES;
		UpdateDisturbances(dt, frameCount, batch);
	}


	///////////////////////////////////////////////////////////////////////////////
	//  CalcBaseVelocity
	///////////////////////////////////////////////////////////////////////////////

	void			phWind::CalcBaseVelocity					()
	{		
		for (int i=0; i<WIND_TYPE_NUM; i++)
		{
			Vec3V vWindDir = Vec3V(FPSin(m_settings[i].angle), FPCos(m_settings[i].angle), 0.0f);
			m_windState[i].vBaseVelocity = ScalarVFromF32(m_settings[i].speed) * vWindDir;
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	//  CalcGustVelocity
	///////////////////////////////////////////////////////////////////////////////

	void			phWind::CalcGustVelocity					(float dt)
	{
		for (int i=0; i<WIND_TYPE_NUM; i++)
		{
			// randomly start a gust
			if (m_gustState[i].gustCurrTime==0.0f)
			{
				m_windState[i].vGustVelocity = Vec3V(V_ZERO);

				if (g_DrawRand.GetRanged(0.0f, 100.0f)<m_settings[i].gustChance)
				{
					m_gustState[i].gustDir = g_DrawRand.GetRanged(-PI, PI);
					m_gustState[i].gustSpeed = g_DrawRand.GetRanged(m_settings[i].gustSpeedMultMin, m_settings[i].gustSpeedMultMax) * m_settings[i].speed;
					m_gustState[i].gustTotalTime = g_DrawRand.GetRanged(m_settings[i].gustTimeMin, m_settings[i].gustTimeMax);
					m_gustState[i].gustCurrTime = m_gustState[i].gustTotalTime;
				}	
			}

			// apply active gust
			if (m_gustState[i].gustCurrTime>0.0f)
			{
				// calc the current speed of the gust
				float maxGustTime = m_gustState[i].gustTotalTime*0.5f;
				float currGustSpeed = m_gustState[i].gustSpeed * (1.0f - (Abs(maxGustTime - m_gustState[i].gustCurrTime) / maxGustTime));

				// calc the gust velocity
				Vec3V vGustDir = Vec3V(FPSin(m_gustState[i].gustDir), FPCos(m_gustState[i].gustDir), 0.0f);
				Vec3V vGustVel = ScalarVFromF32(currGustSpeed) * vGustDir;

				// apply the gust to the global wind
				m_windState[i].vGustVelocity = vGustVel;

				// clamp the gust velocity so that it never gets faster than the max wind speed
				Vec3V vGlobalWindBaseGustVel = m_windState[i].vBaseVelocity + m_windState[i].vGustVelocity;
				float baseGustSpeed = Mag(vGlobalWindBaseGustVel).Getf();
				if (baseGustSpeed>m_settings[i].maxSpeed)
				{
					vGlobalWindBaseGustVel = Normalize(vGlobalWindBaseGustVel);
					vGlobalWindBaseGustVel *= ScalarVFromF32(m_settings[i].maxSpeed);
					m_windState[i].vGustVelocity = vGlobalWindBaseGustVel - m_windState[i].vBaseVelocity;
				}

				m_gustState[i].gustCurrTime = Max(m_gustState[i].gustCurrTime-dt, 0.0f);
			}	
		}
	}


	///////////////////////////////////////////////////////////////////////////////
	//  CalcGlobalVariationVelocity
	///////////////////////////////////////////////////////////////////////////////

	void			phWind::CalcGlobalVariationVelocity			(u32 unpausedElapsedTimeInMs)
	{
		// lookup the current variation multiplier from the table
		int varIndex;
		varIndex = ((int)(unpausedElapsedTimeInMs) >> 11) & VARIATION_ENTRY_MASK;
		float varInterp;
		varInterp = ((int)(unpausedElapsedTimeInMs) & 2047) / 2048.0f;
		varInterp = 0.5f - 0.5f * cosf(varInterp * PI);
		float varMult = (g_variationMult[varIndex] * (1.0f - varInterp) + g_variationMult[(varIndex+1)&VARIATION_ENTRY_MASK] * varInterp);

		for (int i=0; i<WIND_TYPE_NUM; i++)
		{
			Vec3V vVarDir;
// 			if ((WindType_e)i==WIND_TYPE_WATER)
// 			{
// 				// calculate underwater variation direction
// 				vVarDir.SetXf(cosf(unpausedElapsedTimeInMs/1000.0f));
// 				vVarDir.SetYf(sinf(unpausedElapsedTimeInMs/1000.0f));
// 				vVarDir.SetZ(ScalarV(V_ZERO));
// 			}
// 			else
			{
				// lookup the current variation direction from the table
				varIndex = ((int)(unpausedElapsedTimeInMs) >> 10) & VARIATION_ENTRY_MASK;
				varInterp = ((int)(unpausedElapsedTimeInMs) & 1023) / 1024.0f;
				varInterp = 0.5f - 0.5f * cosf(varInterp * PI);
				vVarDir.SetXf(g_variationDir[(varIndex+0)&VARIATION_ENTRY_MASK] * (1.0f - varInterp) + g_variationDir[(varIndex+1)&VARIATION_ENTRY_MASK] * varInterp);
				vVarDir.SetYf(g_variationDir[(varIndex+3)&VARIATION_ENTRY_MASK] * (1.0f - varInterp) + g_variationDir[(varIndex+4)&VARIATION_ENTRY_MASK] * varInterp);
				vVarDir.SetZf(g_variationDir[(varIndex+6)&VARIATION_ENTRY_MASK] * (1.0f - varInterp) + g_variationDir[(varIndex+7)&VARIATION_ENTRY_MASK] * varInterp);
			}

			// dampen the effect of varDir's z axis so there isn't as much up/down variation
			vVarDir.SetZf(vVarDir.GetZf()*0.3f);

			// the variation direction is in the range (-1.0, 1.0) - scale by the current wind speed
			vVarDir *= ScalarVFromF32(m_settings[i].speed);

			// now scale by the game variation mult (0.0, 1.0) (and scale by the local variation multiplier)
			vVarDir *= ScalarVFromF32(m_settings[i].globalVariationMult * varMult);

			// calc the global varied wind velocity by adding the variation direction to the global wind velocity
			m_windState[i].vGlobalVariationVelocity = vVarDir;
		}
	}


	///////////////////////////////////////////////////////////////////////////////
	//  GetPositionalVariationVelocity
	///////////////////////////////////////////////////////////////////////////////

	Vec3V_Out			phWind::GetPositionalVariationVelocity		(WindType_e windType, Vec3V_In vPos)
	{
		Vec3V vQueryPos = vPos;
		if (!m_settings[windType].positionalVariationIsFluid)
		{
			// if we are using fluid positional variation then add to the wind field at this grid element
			// this is achieved by doing a truncate operation
			vQueryPos = RoundToNearestIntZero(vQueryPos);
		}

		g_vecRand.SetSeed(Vec4V(vQueryPos, ScalarV(V_ONE)));
		Vec3V vRandDir = g_vecRand.GetSignedVec3V();

		vRandDir = Normalize(vRandDir);
		vRandDir *= m_settings[windType].vSpeedPosVariationMult;

		return vRandDir;
	}

	void phWind::GetPositionalVariationVelocityF(WindType_e windType, 
		int iPosX, int iPosY, int iPosZ,
		float posX, float posY, float posZ, float& posXOut, float& posYOut, float& posZOut )
	{
		// if we are using fluid positional variation then add to the wind field at this grid element		
		int seed = !m_settings[windType].positionalVariationIsFluid 
				? (iPosX * iPosY * iPosZ)
				: (int)(posX * posY * posZ);

		int storedSeed = g_DrawRand.GetSeed();
		g_DrawRand.Reset(seed);

		float vRandDirX = g_DrawRand.GetRanged(-1.0f, 1.0f);
		float vRandDirY = g_DrawRand.GetRanged(-1.0f, 1.0f);
		float vRandDirZ = g_DrawRand.GetRanged(-1.0f, 1.0f);

		const float invMag = FPInvSqrt( vRandDirX*vRandDirX + vRandDirY*vRandDirY + vRandDirZ*vRandDirZ );
		vRandDirX *= invMag;
		vRandDirY *= invMag;
		vRandDirZ *= invMag;

		const float variationMult = m_settings[windType].speed*m_settings[windType].positionalVariationMult;
		vRandDirX *= variationMult;
		vRandDirY *= variationMult;
		vRandDirZ *= variationMult;

		g_DrawRand.Reset(storedSeed);

		posXOut = vRandDirX;
		posYOut = vRandDirY;
		posZOut = vRandDirZ;
	}


	///////////////////////////////////////////////////////////////////////////////
	//  UpdateDisturbances
	///////////////////////////////////////////////////////////////////////////////

	PPU_ONLY(static bank_bool s_useOptimisedDampen = true);

	void			phWind::UpdateDisturbances					(float dt, u32 frameCount, int batch)
	{
#if __PPU
		if (s_useOptimisedDampen)
			DampenDisturbanceField_Opt(dt, frameCount);
		else
#endif
			DampenDisturbanceField(dt, frameCount);

		m_downwashGroup.Update(dt);
		m_downwashGroup.Apply(&m_disturbanceField, batch);

		m_sphereGroup.Update(dt);
		m_sphereGroup.Apply(&m_disturbanceField, batch);

		m_explosionGroup.Update(dt);
		m_explosionGroup.Apply(&m_disturbanceField, batch);

		m_dirExplosionGroup.Update(dt);
		m_dirExplosionGroup.Apply(&m_disturbanceField, batch);

		m_hemisphereGroup.Update(dt);
		m_hemisphereGroup.Apply(&m_disturbanceField, batch);

		m_thermalGroup.Update(dt);
		m_thermalGroup.Apply(&m_disturbanceField, batch);
	}


	///////////////////////////////////////////////////////////////////////////////
	//  DampenDisturbanceField
	///////////////////////////////////////////////////////////////////////////////
	// dampened the disturbance field containing the wind generated by cars
	// this happens in batches depending on the frame rate
	///////////////////////////////////////////////////////////////////////////////
	//if this size changes, then make same change in the for loop below
#define NUM_DISTURBANCE_COMPONENTS 32

	void			phWind::DampenDisturbanceField				(float dt, u32 frameCount)
	{
		//This could potentially work for the PS3 also, but there's a better optimized function for PS3
		PF_PUSH_TIMEBAR_DETAIL("Dampen Disturbance Field");
		//using s16 as this value never exceeds 255
		s16 mult = s16(powf((1.0f/m_disturbanceFallOff), dt) * 256.0f);

		const int batches = 4;
		int frame = frameCount % batches; 
		int startX = WINDFIELD_NUM_ELEMENTS_X * frame / batches;
		int endX = WINDFIELD_NUM_ELEMENTS_X * (frame+1) / batches;

		// Avoiding triple loop update as it causes LHS issues on the 360. Issue is we update only 6 bytes at a time
		// but the write back into memory happens in word size. Since all the data is contiguous, we get a lot of misaligned
		// loads and stores and thus the LHS.

		// Rewriting loop as accessing a single dimensional array of the s16 type which makes sure we do the update
		// in an aligned manner.
		s16* windVelocityComponents = (s16*) m_disturbanceField.GetData(startX,0,0).GetBase();

		//start prefetch pointer 2 cache lines ahead 
		s16* windVelocityBasePrefetch = windVelocityComponents + 128;
		const int totalNumElements = (endX - startX) * WINDFIELD_NUM_ELEMENTS_Y * WINDFIELD_NUM_ELEMENTS_Z * 3;

		//Make sure that number of elements in array is divisible by number of components used
		CompileTimeAssert(((WINDFIELD_NUM_ELEMENTS_Y * WINDFIELD_NUM_ELEMENTS_Z * 3) % NUM_DISTURBANCE_COMPONENTS) == 0);
		for(int i=0; i<totalNumElements; i+=NUM_DISTURBANCE_COMPONENTS)
		{
			//calculate starting index in this iteration
			PrefetchDC(windVelocityBasePrefetch + i);
			s16* currentElementPtr = &windVelocityComponents[i];
			//updating all NUM_DISTURBANCE_COMPONENTS elements by hand without loop
			currentElementPtr[0] =  s16((currentElementPtr[0] * mult)>>8);
			currentElementPtr[1] =  s16((currentElementPtr[1] * mult)>>8);
			currentElementPtr[2] =  s16((currentElementPtr[2] * mult)>>8);
			currentElementPtr[3] =  s16((currentElementPtr[3] * mult)>>8);
			currentElementPtr[4] =  s16((currentElementPtr[4] * mult)>>8);
			currentElementPtr[5] =  s16((currentElementPtr[5] * mult)>>8);
			currentElementPtr[6] =  s16((currentElementPtr[6] * mult)>>8);
			currentElementPtr[7] =  s16((currentElementPtr[7] * mult)>>8);
			currentElementPtr[8] =  s16((currentElementPtr[8] * mult)>>8);
			currentElementPtr[9] =  s16((currentElementPtr[9] * mult)>>8);
			currentElementPtr[10] =  s16((currentElementPtr[10] * mult)>>8);
			currentElementPtr[11] =  s16((currentElementPtr[11] * mult)>>8);
			currentElementPtr[12] =  s16((currentElementPtr[12] * mult)>>8);
			currentElementPtr[13] =  s16((currentElementPtr[13] * mult)>>8);
			currentElementPtr[14] =  s16((currentElementPtr[14] * mult)>>8);
			currentElementPtr[15] =  s16((currentElementPtr[15] * mult)>>8);
			currentElementPtr[16] =  s16((currentElementPtr[16] * mult)>>8);
			currentElementPtr[17] =  s16((currentElementPtr[17] * mult)>>8);
			currentElementPtr[18] =  s16((currentElementPtr[18] * mult)>>8);
			currentElementPtr[19] =  s16((currentElementPtr[19] * mult)>>8);
			currentElementPtr[20] =  s16((currentElementPtr[20] * mult)>>8);
			currentElementPtr[21] =  s16((currentElementPtr[21] * mult)>>8);
			currentElementPtr[22] =  s16((currentElementPtr[22] * mult)>>8);
			currentElementPtr[23] =  s16((currentElementPtr[23] * mult)>>8);
			currentElementPtr[24] =  s16((currentElementPtr[24] * mult)>>8);
			currentElementPtr[25] =  s16((currentElementPtr[25] * mult)>>8);
			currentElementPtr[26] =  s16((currentElementPtr[26] * mult)>>8);
			currentElementPtr[27] =  s16((currentElementPtr[27] * mult)>>8);
			currentElementPtr[28] =  s16((currentElementPtr[28] * mult)>>8);
			currentElementPtr[29] =  s16((currentElementPtr[29] * mult)>>8);
			currentElementPtr[30] =  s16((currentElementPtr[30] * mult)>>8);
			currentElementPtr[31] =  s16((currentElementPtr[31] * mult)>>8);
		}

		PF_POP_TIMEBAR_DETAIL();
	}


	///////////////////////////////////////////////////////////////////////////////
	//  DampenDisturbanceField_Opt
	///////////////////////////////////////////////////////////////////////////////
	// dampened the disturbance field containing the wind generated by cars
	// this happens in batches depending on the frame rate
	///////////////////////////////////////////////////////////////////////////////

#if __PPU
	void			phWind::DampenDisturbanceField_Opt				(float dt, u32 frameCount)
	{
		// Note that 360 is missing the VXM integer multiply instructions we use here.
		// It would have to unpack, convert to float, multiply, and reconvert back to integers again.
		float mult = powf((1.0f/m_disturbanceFallOff), dt);
		Assert(((size_t)(&m_disturbanceField.GetDataBase()) & 15) == 0);
		int multFixed = (mult * 256.0f);
		// There's got to be a better way to do this, but it's outside the loop anyway.
		vector short multI = (vector short)(multFixed,multFixed,multFixed,multFixed,multFixed,multFixed,multFixed,multFixed);
		// The bytes are numbered from left to right in XYZW order.  0x0? is the first parameter to the vec_perm,
		// and 0x1? is the second parameter to the vec_perm.  Because we're using 8.8 fixed point for the multiplier,
		// and mule and mulo both produce full 32 bit results from two 16 bit inputs, we need to pick the middle two bytes
		// out of the results; also, we need to reinterleave the even and odd results at the same time.  Verified in the
		// debugger with a hand-written test case that everything lines up properly.
		//                              evenMid0   oddMid0    evenMid1    oddMid1   evenMid2   oddMid2    evenMid3   oddMid3
		vector unsigned char mixer = { 0x01,0x02, 0x11,0x12, 0x05,0x06, 0x15,0x16, 0x09,0x0A, 0x19,0x1A, 0x0D,0x0E, 0x1D,0x1E };

		const int batches = 4;
		int frame = frameCount % batches; 
		int startX = WINDFIELD_NUM_ELEMENTS_X * frame / batches;
		int endX = WINDFIELD_NUM_ELEMENTS_X * (frame+1) / batches;

		for (int x=startX; x<endX; x++)
		{
			for (int y=0; y<WINDFIELD_NUM_ELEMENTS_Y; y++)
			{
				// If this changes, we need to modify the number of elements in flight here.
				CompileTimeAssert(WINDFIELD_NUM_ELEMENTS_Z == 8);
				vector short *z = (vector short*) &m_disturbanceField.GetData(x,y,0);
				vector int z0e = vec_mule(z[0],multI);
				vector int z0o = vec_mulo(z[0],multI);
				vector int z1e = vec_mule(z[1],multI);
				vector int z1o = vec_mulo(z[1],multI);
				vector int z2e = vec_mule(z[2],multI);
				vector int z2o = vec_mulo(z[2],multI);
				z[0] = vec_perm((vector short)z0e,(vector short)z0o,mixer);
				z[1] = vec_perm((vector short)z1e,(vector short)z1o,mixer);
				z[2] = vec_perm((vector short)z2e,(vector short)z2o,mixer);
			}
		}
	}
#endif

	///////////////////////////////////////////////////////////////////////////////
	//  GetGlobalVelocity
	///////////////////////////////////////////////////////////////////////////////
	void			phWind::GetGlobalVelocity(WindType_e type, Vec3V_InOut vWindVel, bool bUseGustVelocity, bool bUseGlobalVariationVelocity)
	{
		const WindState& windState = m_windState[type];
		vWindVel = windState.vBaseVelocity + (bUseGustVelocity?windState.vGustVelocity:Vec3V(V_ZERO)) + (bUseGlobalVariationVelocity?windState.vGlobalVariationVelocity:Vec3V(V_ZERO));

#if __BANK
		m_numGlobalWindQueries++;
#endif
	}

	///////////////////////////////////////////////////////////////////////////////
	//  GetLocalVelocity
	///////////////////////////////////////////////////////////////////////////////
	void			phWind::GetLocalVelocity						(Vec3V_In vPos, Vec3V_InOut vWindVel, bool includeWindFieldDisturbance, bool includeGlobalDisturbances, bool ignoreNonDisturbances, bool includeGridPosWindFieldDisturbance)
	{
		Vec3V vint = FloatToIntRaw<0>(vPos);
		int iPosX = vint.GetXi();
		int iPosY = vint.GetYi();
		int iPosZ = vint.GetZi(); 

		WindType_e windType = IsLessThanOrEqualAll(vPos.GetZ(), ScalarVFromF32(phWindField::GetWaterLevelWld())) ? WIND_TYPE_WATER : WIND_TYPE_AIR;
		vWindVel = Vec3V(V_ZERO);
	
		const WindState& windState = m_windState[windType];

		if (ignoreNonDisturbances==false)
		{
			// add on the base velocity
			vWindVel += windState.vBaseVelocity;
			Assertf(IsFiniteAll(vWindVel), "vWindVel is not finite (1)");

#if __BANK
			if (m_dontApplyGusts==false)
#endif
			{
				// add on gusts
				vWindVel += windState.vGustVelocity;
				Assertf(IsFiniteAll(vWindVel), "vWindVel is not finite (2)");
			}

#if __BANK
			if (m_dontApplyVariation==false)
#endif
			{
				const WindSettings& windSettings = m_settings[windType];
				Assertf(IsFiniteAll(vPos.GetXY()), "vPos is not finite");
				Assertf(IsFiniteAll(windSettings.vGlobalVariationSinCosMult.GetXY()), "windSettings.vGlobalVariationSinCosMultXY is not finite");
				Assertf(IsFiniteAll(windSettings.vGlobalVariationSinCosMult.GetZW()), "windSettings.vGlobalVariationSinCosMultZW is not finite");
				Assertf(IsFiniteAll(m_vElapsedTime), "m_vElapsedTime is not finite");
				Vec2V gustMultXY = windSettings.vGlobalVariationSinCosMult.GetXY() * vPos.GetXY();
				gustMultXY += windSettings.vGlobalVariationSinCosMult.GetZW() * m_vElapsedTime;
				gustMultXY = (SinApprox(Vec4V(gustMultXY, Vec2V(V_ZERO))).GetXY() + Vec2V(V_ONE)) * Vec2V(V_HALF);
				Vec3V gustMult(gustMultXY, ScalarV(V_ONE));	
				Assertf(IsFiniteAll(gustMultXY), "gustMultXY is not finite");
				Assertf(IsFiniteAll(windState.vGlobalVariationVelocity), "windState.vGlobalVariationVelocity is not finite");
				Vec3V vAddVel = windState.vGlobalVariationVelocity * gustMult;
				Assertf(IsFiniteAll(vAddVel), "vAddVel is not finite - m_vElapsedTime:%.3f", m_vElapsedTime.Getf());
				vWindVel += vAddVel;
				Assertf(IsFiniteAll(vWindVel), "vWindVel is not finite (3)");

				vWindVel += GetPositionalVariationVelocity(windType, vPos);
				Assertf(IsFiniteAll(vWindVel), "vWindVel is not finite (4)");
			}
		}

		// add on velocity from the disturbance field
#if __BANK
		if (m_dontApplyDisturbances==false)
#endif
		{
			if (includeWindFieldDisturbance)
			{
				if (includeGlobalDisturbances)
				{
					ScalarV airSpeed = ScalarVFromF32(m_settings[WIND_TYPE_AIR].speed);
					ScalarV waterSpeed = ScalarVFromF32(m_settings[WIND_TYPE_WATER].speed);

					vWindVel += m_downwashGroup.GetVelocity(vPos, airSpeed, waterSpeed);
					Assertf(IsFiniteAll(vWindVel), "vWindVel is not finite (5)");
					vWindVel += m_sphereGroup.GetVelocity(vPos, airSpeed, waterSpeed);
					Assertf(IsFiniteAll(vWindVel), "vWindVel is not finite (6)");
					vWindVel += m_explosionGroup.GetVelocity(vPos, airSpeed, waterSpeed);
					Assertf(IsFiniteAll(vWindVel), "vWindVel is not finite (7)");
					vWindVel += m_dirExplosionGroup.GetVelocity(vPos, airSpeed, waterSpeed);
					Assertf(IsFiniteAll(vWindVel), "vWindVel is not finite (8)");
					vWindVel += m_hemisphereGroup.GetVelocity(vPos, airSpeed, waterSpeed);
					Assertf(IsFiniteAll(vWindVel), "vWindVel is not finite (9)");
					vWindVel += m_thermalGroup.GetVelocity(vPos, airSpeed, waterSpeed);
					Assertf(IsFiniteAll(vWindVel), "vWindVel is not finite (10)");
				}

				if (includeGridPosWindFieldDisturbance)
				{
					// add on velocity from the disturbance field
					u32 gridX, gridY, gridZ;
					if (phWindField::CalcGridPos(iPosX, iPosY, iPosZ, gridX, gridY, gridZ))
					{
						vWindVel += m_disturbanceField.GetVelocity(gridX, gridY, gridZ);
						Assertf(IsFiniteAll(vWindVel), "vWindVel is not finite (1)");
					}
				}
			}
		}

		Assertf(IsFiniteAll(vWindVel), "vWindVel is not finite (5)");

#if __BANK
		if (m_debugDrawWindQueries)
		{
			Color32 colA(0.0f, 0.0f, 1.0f, 1.0f);

			if (includeWindFieldDisturbance)
			{
				colA.SetRed(255);
			}

			if (includeGlobalDisturbances)
			{
				colA.SetGreen(255);
			}

			Color32 colB(colA.GetRedf(), colA.GetGreenf(), colA.GetBluef(), 0.0f);
		
			grcDebugDraw::Line(vPos, vPos+(vWindVel*ScalarVFromF32(m_drawScale)), colA, colB);
		}

		m_numLocalWindQueries++;
#endif
	}


	///////////////////////////////////////////////////////////////////////////////
	//  GetLocalVelocity_DownwashOnly
	///////////////////////////////////////////////////////////////////////////////
	void			phWind::GetLocalVelocity_DownwashOnly					(Vec3V_In vPos, Vec3V_InOut vWindVel)
	{
		vWindVel = Vec3V(V_ZERO);

		ScalarV airSpeed = ScalarVFromF32(m_settings[WIND_TYPE_AIR].speed);
		ScalarV waterSpeed = ScalarVFromF32(m_settings[WIND_TYPE_WATER].speed);

		vWindVel += m_downwashGroup.GetVelocity(vPos, airSpeed, waterSpeed);
		Assertf(IsFiniteAll(vWindVel), "vWindVel is not finite (5)");

#if __BANK
		if (m_debugDrawWindQueries)
		{
			Color32 colA(0.0f, 0.0f, 1.0f, 1.0f);
			colA.SetRed(255);
			colA.SetGreen(255);
			Color32 colB(colA.GetRedf(), colA.GetGreenf(), colA.GetBluef(), 0.0f);
			grcDebugDraw::Line(vPos, vPos+(vWindVel*ScalarVFromF32(m_drawScale)), colA, colB);
		}
		m_numLocalWindQueries++;
#endif
	}


	///////////////////////////////////////////////////////////////////////////////
	//  AddWidgets
	///////////////////////////////////////////////////////////////////////////////

#if __BANK
	void			phWind::AddWidgets							(bkBank& bank)
	{
		bank.PushGroup("Rage Wind", false);
		{
			PPU_ONLY(bank.AddToggle("Optimized Dampen", &s_useOptimisedDampen));
			bank.AddSlider("Draw Scale", &m_drawScale, 0.0f, 10.0f, 0.005f);
			bank.AddToggle("Draw Debug Wind Field (Full)", &m_debugDrawWindFieldFull);
			bank.AddToggle("Draw Debug Wind Field (No Disturbances)", &m_debugDrawWindFieldNoDisturbances);
			bank.AddToggle("Draw Debug Disturbance Field", &m_debugDrawDisturbanceField);
			bank.AddToggle("Draw Debug Disturbance Bounds", &m_debugDrawDisturbanceBounds);
			bank.AddToggle("Draw Debug Wind Queries", &m_debugDrawWindQueries);
			bank.AddSlider("Num Global Wind Queries", &m_numGlobalWindQueries, 0, 10000, 0);
			bank.AddSlider("Num Local Wind Queries", &m_numLocalWindQueries, 0, 10000, 0);
			bank.AddToggle("Pause Update", &m_pauseUpdate);
			bank.AddToggle("Don't Apply Gusts", &m_dontApplyGusts);
			bank.AddToggle("Don't Apply Variation", &m_dontApplyVariation);
			bank.AddToggle("Don't Apply Disturbances", &m_dontApplyDisturbances);
			bank.AddButton("Clear Wind Fields", datCallback(MFA(phWind::InitSession), this));
		}
		bank.PopGroup();

	}
#endif // __BANK


	///////////////////////////////////////////////////////////////////////////////
	//  DebugDraw
	///////////////////////////////////////////////////////////////////////////////

#if __BANK
	void			DebugBox									(const int basePosX, const int basePosY, const int basePosZ)
	{
		Color32 col(255, 0, 0, 255);
		Vec3V vBoxMin = Vec3V((float)basePosX, (float)basePosY, (float)basePosZ);
		Vec3V vBoxMax = vBoxMin;
		Vec3V vBoxAdd((float)(WINDFIELD_NUM_ELEMENTS_X*WINDFIELD_ELEMENT_SIZE_X), (float)(WINDFIELD_NUM_ELEMENTS_Y*WINDFIELD_ELEMENT_SIZE_Y), (float)(WINDFIELD_NUM_ELEMENTS_Z*WINDFIELD_ELEMENT_SIZE_Z));
		vBoxMax += vBoxAdd;

		grcDebugDraw::BoxAxisAligned(vBoxMin, vBoxMax, col, false);
	}

	void			phWind::DebugDraw							()
	{
		bool drawnBox = false;

		if (m_debugDrawDisturbanceField)
		{ 
			Color32 colA(128, 128, 128, 255);
			Color32 colB(255, 255, 255, 255);

			for (int x=0; x<WINDFIELD_NUM_ELEMENTS_X; x++)
			{
				for (int y=0; y<WINDFIELD_NUM_ELEMENTS_Y; y++)
				{
					for (int z=0; z<WINDFIELD_NUM_ELEMENTS_Z; z++)
					{
						// calc the world pos of this location
						int worldX = phWindField::GetBasePosWldX() + (x*WINDFIELD_ELEMENT_SIZE_X);
						int worldY = phWindField::GetBasePosWldY() + (y*WINDFIELD_ELEMENT_SIZE_Y);
						int worldZ = phWindField::GetBasePosWldZ() + (z*WINDFIELD_ELEMENT_SIZE_Z);

						// calc the grid indices of this world pos
						u32 gridX, gridY, gridZ;
						phWindField::CalcGridPosFast(worldX, worldY, worldZ, gridX, gridY, gridZ);

						Vec3V vWindVel = m_disturbanceField.GetVelocity(gridX, gridY, gridZ);
						vWindVel *= ScalarVFromF32(m_drawScale);	

						Vec3V vBaseCoors = Vec3V((float)(phWindField::GetBasePosWldX() + x*WINDFIELD_ELEMENT_SIZE_X), (float)(phWindField::GetBasePosWldY() + y*WINDFIELD_ELEMENT_SIZE_Y), (float)(phWindField::GetBasePosWldZ() + z*WINDFIELD_ELEMENT_SIZE_Z));
						Vec3V vEndCoors = vBaseCoors + vWindVel;
						grcDebugDraw::Line(RCC_VECTOR3(vBaseCoors), RCC_VECTOR3(vEndCoors), colA, colB);
					}
				}
			}

			if (!drawnBox)
			{
				DebugBox(phWindField::GetBasePosWldX(), phWindField::GetBasePosWldY(), phWindField::GetBasePosWldZ());
				drawnBox = true;
			}
		}

		if (m_debugDrawDisturbanceBounds)
		{
			m_downwashGroup.DebugDraw();
			m_sphereGroup.DebugDraw();
			m_explosionGroup.DebugDraw();
			m_dirExplosionGroup.DebugDraw();
			m_hemisphereGroup.DebugDraw();
			m_thermalGroup.DebugDraw();
		}

		if (m_debugDrawWindFieldFull || m_debugDrawWindFieldNoDisturbances)
		{	
			// make sure both can't be checked
			if (m_debugDrawWindFieldFull && m_debugDrawWindFieldNoDisturbances)
			{
				m_debugDrawWindFieldNoDisturbances = false;
			}

			Color32 colA(0, 128, 128, 255);
			Color32 colB(0, 255, 255, 255);

			Color32 colC(128, 0, 128, 128);
			Color32 colD(255, 0, 255, 255);

			for (int x=0; x<WINDFIELD_NUM_ELEMENTS_X; x++)
			{
				for (int y=0; y<WINDFIELD_NUM_ELEMENTS_Y; y++)
				{
					for (int z=0; z<WINDFIELD_NUM_ELEMENTS_Z; z++)
					{
						// calc the world pos of this location
						int worldX = phWindField::GetBasePosWldX() + (x*WINDFIELD_ELEMENT_SIZE_X);
						int worldY = phWindField::GetBasePosWldY() + (y*WINDFIELD_ELEMENT_SIZE_Y);
						int worldZ = phWindField::GetBasePosWldZ() + (z*WINDFIELD_ELEMENT_SIZE_Z);

						// calc the grid indices of this world pos
						u32 gridX, gridY, gridZ;
						phWindField::CalcGridPosFast(worldX, worldY, worldZ, gridX, gridY, gridZ);

						Vec3V vWindVel;
						if (m_debugDrawWindFieldFull)
						{
							GetLocalVelocity(Vec3V((float)worldX, (float)worldY, (float)worldZ), vWindVel);
						}
						else if (m_debugDrawWindFieldNoDisturbances)
						{
							GetLocalVelocity(Vec3V((float)worldX, (float)worldY, (float)worldZ), vWindVel, false, false);
						}
						vWindVel *= ScalarVFromF32(m_drawScale);	

						Vec3V vBaseCoors = Vec3V((float)(phWindField::GetBasePosWldX() + x*WINDFIELD_ELEMENT_SIZE_X), (float)(phWindField::GetBasePosWldY() + y*WINDFIELD_ELEMENT_SIZE_Y), (float)(phWindField::GetBasePosWldZ() + z*WINDFIELD_ELEMENT_SIZE_Z));
						Vec3V vEndCoors = vBaseCoors + vWindVel;

						if (worldZ<=phWindField::GetWaterLevelWld())
						{
							grcDebugDraw::Line(RCC_VECTOR3(vBaseCoors), RCC_VECTOR3(vEndCoors), colC, colD);
						}
						else
						{
							grcDebugDraw::Line(RCC_VECTOR3(vBaseCoors), RCC_VECTOR3(vEndCoors), colA, colB);
						}
					}
				}
			}

			if (!drawnBox)
			{
				DebugBox(phWindField::GetBasePosWldX(), phWindField::GetBasePosWldY(), phWindField::GetBasePosWldZ());
				drawnBox = true;
			}
		}
	}
#endif // __BANK

} // namespace rage
