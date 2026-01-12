//
// pheffects/wind.h
//
// Copyright (C) 1999-2008 Rockstar Games.  All Rights Reserved.
//

// Deals with wind. A wind field is being maintained around the player. This will allow for eddies and stuff.

#ifndef PHEFFECTS_WIND_H
#define PHEFFECTS_WIND_H


///////////////////////////////////////////////////////////////////////////////
//  INCLUDES
///////////////////////////////////////////////////////////////////////////////	

#include "atl/singleton.h"
#include "bank/bkmgr.h"
#include "data/base.h"
#include "math/random.h"
#include "pheffects/windfield.h"
#include "pheffects/winddisturbancebase.h"
#include "vectormath/classes.h"

#include "pheffects/winddisturbancedownwash.h"
#include "pheffects/winddisturbanceexplosion.h"
#include "pheffects/winddisturbancehemisphere.h"
#include "pheffects/winddisturbancesphere.h"
#include "pheffects/winddisturbancethermal.h"

namespace rage
{
	///////////////////////////////////////////////////////////////////////////////
	//  ENUMERATIONS
	///////////////////////////////////////////////////////////////////////////////	

	enum WindType_e
	{
		WIND_TYPE_AIR					= 0,
		WIND_TYPE_WATER,

		WIND_TYPE_NUM
	};


	///////////////////////////////////////////////////////////////////////////////
	//  STRUCTURES
	///////////////////////////////////////////////////////////////////////////////

	struct WindSettings
	{
		float			angle;							// the wind angle passed in from the game
		float			speed;							// the wind speed passed in from the game
		float			maxSpeed;						// the maximum wind speed passed in from the game
		float			globalVariationMult;			// the amount of global wind variation passed in from the game (0.0 - 1.0)
		float			globalVariationSinPosMult;
		float			globalVariationSinTimeMult;
		float			globalVariationCosPosMult;
		float			globalVariationCosTimeMult;
		Vec4V			vGlobalVariationSinCosMult;		// contains globalVariation sinPos(x), cosPos(y), sinTime(z), cosTime(w)
		float			positionalVariationMult;		// the amount of positional wind variation passed in from the game (0.0 - 1.0)
		bool			positionalVariationIsFluid;		// the type of positional wind variation to use (false = per grid element, true = everywhere)
		float			gustChance;						// the chance of a wind gust occurring
		float			gustSpeedMultMin;				// the min speed of gusts as a multiplier of the current game wind speed
		float			gustSpeedMultMax;				// the min speed of gusts as a multiplier of the current game wind speed
		float			gustTimeMin;					// the min time of gusts 
		float			gustTimeMax;					// the max time of gusts 
		ScalarV			vSpeedPosVariationMult;			//Contains ScalarV value of (speed * positionalVariationMult). Used for avoiding LHS
	};
	
	struct WindState
	{
		Vec3V			vBaseVelocity;					// the global base velocity 
		Vec3V			vGustVelocity;					// the global gust velocity (to add to the global base velocity)
		Vec3V			vGlobalVariationVelocity;		// the global variation velocity (to add to the global base velocity)
	};

	struct GustState
	{
		float			gustDir;
		float			gustSpeed;
		float			gustTotalTime;
		float			gustCurrTime;
	};


	///////////////////////////////////////////////////////////////////////////////
	//  CLASSES
	///////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////
	//  phWind
	///////////////////////////////////////////////////////////////////////////////

	class phWind : public datBase
	{
		///////////////////////////////////
		// FRIENDS 
		///////////////////////////////////
		
		// needed for compiling the spu code
		friend struct phSpuWindEvalData;


		///////////////////////////////////
		// FUNCTIONS 
		///////////////////////////////////

		public: ///////////////////////////

			// main interface
							phWind								();

			static void 	InitClass							();
			static void 	ShutdownClass						();

			void			InitSession							();
			void			Update								(float dt, u32 unpausedElapsedTimeInMs, u32 frameCount, bool skipCalcs = false);

			// settings
#if !__SPU
	inline	void			SetFocalPoint						(Vec3V_In vFocalPt)									{phWindField::CalcBasePos(vFocalPt);}
	inline	void			SetWaterLevel						(float waterZWld)									{phWindField::SetWaterLevelWld(waterZWld);}
#endif

	inline	void			SetSpeed							(WindType_e type, float val)						{m_settings[type].speed = val; m_settings[type].vSpeedPosVariationMult = ScalarVFromF32(val * m_settings[type].positionalVariationMult);}
	inline	void			SetMaxSpeed							(WindType_e type, float val)						{m_settings[type].maxSpeed = val;}
	inline	void			SetAngle							(WindType_e type, float val)						{m_settings[type].angle = val;}
	inline	void			SetGlobalVariationMult				(WindType_e type, float val)						{m_settings[type].globalVariationMult = val;}
	__forceinline void		SetGlobalVariationWaveData			(WindType_e type, float valSinPos, float valSinTime, 
																 float valCosPos, float valCosTime)					{
																	 m_settings[type].globalVariationSinPosMult = valSinPos; 
																	 m_settings[type].globalVariationSinTimeMult = valSinTime; 
																	 m_settings[type].globalVariationCosPosMult = valCosPos; 
																	 m_settings[type].globalVariationCosTimeMult = valCosTime;
																	 m_settings[type].vGlobalVariationSinCosMult = Vec4V(
																		 m_settings[type].globalVariationSinPosMult, 
																		 m_settings[type].globalVariationCosPosMult,
																		 m_settings[type].globalVariationSinTimeMult, 
																		 m_settings[type].globalVariationCosTimeMult);
																}
	inline	void			SetPositionalVariationMult			(WindType_e type, float val)						{m_settings[type].positionalVariationMult = val; m_settings[type].vSpeedPosVariationMult = ScalarVFromF32(val * m_settings[type].speed);}
	inline	void			SetPositionalVariationIsFluid		(WindType_e type, bool val)							{m_settings[type].positionalVariationIsFluid = val;}
	inline	void			SetGustChance						(WindType_e type, float val)						{m_settings[type].gustChance = val;}
	inline	void			SetGustSpeedMults					(WindType_e type, float minVal, float maxVal)		{m_settings[type].gustSpeedMultMin = minVal; m_settings[type].gustSpeedMultMax = maxVal;}
	inline	void			SetGustTimes						(WindType_e type, float minVal, float maxVal)		{m_settings[type].gustTimeMin = minVal; m_settings[type].gustTimeMax = maxVal;}

	inline	void			SetDisturbanceFallOff				(float val)											{m_disturbanceFallOff = val;}

			// access global wind
			void			GetGlobalVelocity					(WindType_e type, Vec3V_InOut vWindVel, bool bUseGustVelocity = true, bool bUseGlobalVariationVelocity = true);

			// access local wind
			void			GetLocalVelocity					(Vec3V_In vPos, Vec3V_InOut vWindVel, bool includeWindFieldDisturbance=true, bool includeGlobalDisturbances=true, bool ignoreNonDisturbances=false, bool includeGridPosWindFieldDisturbance=true);
			void			GetLocalVelocity_DownwashOnly		(Vec3V_In vPos, Vec3V_InOut vWindVel);

			// accessors
	inline	Vec3V_Out		GetBaseVelocity						(WindType_e type)	const							{return m_windState[type].vBaseVelocity;}
	inline	Vec3V_Out		GetGustVelocity						(WindType_e type)	const							{return m_windState[type].vGustVelocity;}
	inline	Vec3V_Out		GetGlobalVariationVelocity			(WindType_e type)	const							{return m_windState[type].vGlobalVariationVelocity;}

	inline	float			GetSpeed							(WindType_e type)	const							{return m_settings[type].speed;}
	inline	float			GetPositionalVariationMult			(WindType_e type)	const							{return m_settings[type].positionalVariationMult;}

	inline	float			GetGlobalVariationSinPosMult		(WindType_e type)	const							{return m_settings[type].globalVariationSinPosMult;}
	inline	float			GetGlobalVariationSinTimeMult		(WindType_e type)	const							{return m_settings[type].globalVariationSinTimeMult;}
	inline	float			GetGlobalVariationCosPosMult		(WindType_e type)	const							{return m_settings[type].globalVariationCosPosMult;}
	inline	float			GetGlobalVariationCosTimeMult		(WindType_e type)	const							{return m_settings[type].globalVariationCosTimeMult;}

	inline	phWindField&	GetDisturbanceField					()													{return m_disturbanceField;}
	inline	phWindPoint&	GetDisturbanceFieldData				()													{return m_disturbanceField.GetDataBase();}
	inline	unsigned int	GetDisturbanceFieldWidth			() const											{return m_disturbanceField.GetWidth();}
	inline	unsigned int	GetDisturbanceFieldHeight			() const											{return m_disturbanceField.GetHeight();}
	inline	unsigned int	GetDisturbanceFieldDepth			() const											{return m_disturbanceField.GetDepth();}

			WindSettings&	GetWindSettings						(u32 windType)										{return m_settings[windType];}
			WindState&		GetWindState						(u32 windType)										{return m_windState[windType];}
			GustState&		GetGustState						(u32 windType)										{return m_gustState[windType];}

			// debug
#if __BANK
			void			AddWidgets							(bkBank& bank);
			void			DebugDraw							();
#endif

			phWindDownwashGroup		m_downwashGroup;
			phWindSphereGroup		m_sphereGroup;
			phWindExplosionGroup	m_explosionGroup;
			phWindDirExplosionGroup	m_dirExplosionGroup;
			phWindHemisphereGroup	m_hemisphereGroup;
			phWindThermalGroup		m_thermalGroup;
			// Other groups we're not using right now (these would need to be added to the .cpp too)
			// phWindCycloneGroup		m_cycloneGroup;
			// phWindBoxGroup			m_boxGroup;

		private: //////////////////////////

			// gusts

			// variation
			void			CalcBaseVelocity					();
			void			CalcGustVelocity					(float dt);
			void			CalcGlobalVariationVelocity			(u32 unpausedElapsedTimeInMs);
			Vec3V_Out		GetPositionalVariationVelocity		(WindType_e windType, Vec3V_In pos);
			void			GetPositionalVariationVelocityF		(WindType_e windType, int iPosX, int iPosY, int iPosZ, float posX, float posY, float posZ, float& posXOut, float& posYOut, float& posZOut );

			// disturbances
			void			UpdateDisturbances					(float dt, u32 frameCount, int batch);
			void			DampenDisturbanceField				(float dt, u32 frameCount);
			void			DampenDisturbanceField_Opt			(float dt, u32 frameCount);


		///////////////////////////////////
		// VARIABLES
		///////////////////////////////////

		private: //////////////////////////

			// main data
			float			m_elapsedTime;	
			ScalarV			m_vElapsedTime;

			WindSettings	m_settings[WIND_TYPE_NUM];
			WindState		m_windState[WIND_TYPE_NUM];
			GustState		m_gustState[WIND_TYPE_NUM];

			// disturbances
			phWindField		m_disturbanceField;


			float			m_disturbanceFallOff;

#if __BANK
			// debug
			float			m_drawScale;
			s32				m_numGlobalWindQueries;
			s32				m_numLocalWindQueries;
			bool			m_debugDrawWindFieldFull;
			bool			m_debugDrawWindFieldNoDisturbances;
			bool			m_debugDrawDisturbanceField;
			bool			m_debugDrawDisturbanceBounds;
			bool			m_debugDrawWindQueries;
			bool			m_pauseUpdate;
			bool			m_dontApplyGusts;
			bool			m_dontApplyVariation;
			bool			m_dontApplyDisturbances;
#endif

	};


	///////////////////////////////////////////////////////////////////////////////
	//  DEFINES
	///////////////////////////////////////////////////////////////////////////////

#if !__SPU
	typedef atSingleton<phWind> g_wind;
	#define WIND g_wind::InstanceRef()
#endif

} // namespace rage


#endif // PHEFFECTS_WIND_H
