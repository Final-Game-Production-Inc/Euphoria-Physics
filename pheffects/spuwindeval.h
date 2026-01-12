// 
// pheffects/spuwindeval.h 
// 
// Copyright (C) 1999-2009 Rockstar Games.  All Rights Reserved. 
// 

#ifndef PHEFFECTS_SPUWINDEVAL_H 
#define PHEFFECTS_SPUWINDEVAL_H 

#include "windfield.h"

namespace rage {

	class phWind;

// PURPOSE: Routines for evaluating the wind field on SPUs

	struct WindEvalSettings
	{
		float speed;
		float globalVariationSinPosMult;
		float globalVariationSinTimeMult;
		float globalVariationCosPosMult;
		float globalVariationCosTimeMult;
		float positionalVariationMult;		
		bool positionalVariationIsFluid;		
	};

	struct WindEvalState
	{
		Vec3V vBaseVelocity;				
		Vec3V vGustVelocity;				
		Vec3V vGlobalVariationVelocity;		
	};

	struct WindEvalDisturbanceGroups
	{
		WindEvalDisturbanceGroups()
			: m_downwashGroup(NULL)
			, m_sphereGroup(NULL)
			, m_explosionGroup(NULL)
			, m_dirExplosionGroup(NULL)
			, m_hemisphereGroup(NULL)
			, m_thermalGroup(NULL)
		{
		}

		// SetDisturbancePtrs sets these to the PPU addresses of the groups.
		// By the time GetLocalVelocity is called, these are expected to contain local store pointers to the
		// disturbance group data.
		// The wind code itself is not responsible for doing the DMAs to load the groups. Therefore if you don't
		// want to use disturbances, don't call SetDisturbancePtrs.
		class phWindDownwashGroup*			m_downwashGroup;
		class phWindSphereGroup*			m_sphereGroup;
		class phWindExplosionGroup*			m_explosionGroup;
		class phWindDirExplosionGroup*		m_dirExplosionGroup;
		class phWindHemisphereGroup*		m_hemisphereGroup;
		class phWindThermalGroup*			m_thermalGroup;
		// Other groups we're not using right now 
		// phWindCycloneGroup		m_cycloneGroup;
		// phWindBoxGroup			m_boxGroup;
	};

	struct ALIGNAS(16) phSpuWindEvalData
	{
		float m_elapsedTime;	

		WindEvalSettings m_settings[2];
		WindEvalState m_windState[2];

		phWindPoint* m_disturbanceField;

		int m_basePosWldX;
		int m_basePosWldY;						
		int m_basePosWldZ;	

		float m_waterLevelWld;

		WindEvalDisturbanceGroups m_disturbanceGroups;
#if !__SPU
		void SetData(phWind& wind);
		void SetDisturbancePtrs(phWind& wind);
#endif
	} ;

	struct ALIGNAS(16) phSpuWindEval
	{
		public:
			phSpuWindEval() : m_pData(NULL) {}

			void Init(phSpuWindEvalData& data);
			void GetLocalVelocity(Vec3V_In vPos, Vec3V_InOut vWindVel, bool includeWindFieldDisturbance, bool includeGlobalDisturbances);

		protected:

			Vec3V_Out GetPositionalVariationVelocity(int windType, Vec3V_In vPos);
			phWindPoint* FindDataAddress(u32 x, u32 y, u32 z, phWindPoint* baseAddr);
			bool CalcGridPos(int WorldX, int WorldY, int WorldZ, u32 &GridX, u32 &GridY, u32 &GridZ);

			// These have to be big enough to hold a phWindPoint starting from any address (and possibly crossing a 16 byte boundary)
			char m_spuDisturbanceDataStorage[32];
			phWindPoint* m_pSpuDisturbanceData;		// points into m_spuDisturbanceDataStorage

			phSpuWindEvalData* m_pData;
#ifndef _M_X64
			u32 m_pad[2];
#endif

	} ;


} // namespace rage

#endif // PHEFFECTS_SPUWINDEVAL_H 
