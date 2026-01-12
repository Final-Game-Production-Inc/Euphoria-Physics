// 
// pheffects/winddisturbancethermal.h 
// 
// Copyright (C) 1999-2008 Rockstar Games.  All Rights Reserved. 
// 

#ifndef PHEFFECTS_WIND_THERMAL_H 
#define PHEFFECTS_WIND_THERMAL_H 


///////////////////////////////////////////////////////////////////////////////
//  INCLUDES
///////////////////////////////////////////////////////////////////////////////	

#include "winddisturbancebase.h"

#include "math/random.h"
#include "vector/vector3.h"
#include "vectormath/classes.h"
#include "vectormath/legacyconvert.h"


///////////////////////////////////////////////////////////////////////////////
//  DEFINES
///////////////////////////////////////////////////////////////////////////////	




namespace rage
{


	///////////////////////////////////////////////////////////////////////////////
	//  ENUMERATIONS
	///////////////////////////////////////////////////////////////////////////////	

	enum eThermalMode	
	{	
		THERMAL_STATE_OFF			= 0,
		THERMAL_STATE_GROWING,
		THERMAL_STATE_ON,
		THERMAL_STATE_SHRINKING 
	};


	///////////////////////////////////////////////////////////////////////////////
	//  STRUCTURES
	///////////////////////////////////////////////////////////////////////////////

	struct phThermalSettings
	{
		float					radius;
		float					height;
		float					maxStrength;
		float					deltaStrength;
		
		float					shrinkChance;
	};

	struct phThermalAutoData	
	{
		phWindDistType_e		type;		
		
		float					radiusMin;
		float					radiusMax;
		float					heightMin;
		float					heightMax;
		float					strengthMin;
		float					strengthMax;
		float					strengthDelta;

		float					createChance;
		float					shrinkChance;

	};


	///////////////////////////////////////////////////////////////////////////////
	//  CLASSES
	///////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////
	//  phWindThermal
	///////////////////////////////////////////////////////////////////////////////

	class phWindThermal : public phWindDisturbanceBase
	{

		///////////////////////////////////
		// FUNCTIONS 
		///////////////////////////////////

		public: ///////////////////////////

								phWindThermal				();
								phWindThermal				(phWindDistType_e type, Vec3V_In pos, const phThermalSettings& settings);

			bool				Update						(float dt);
			Vec3V_Out			GetVelocity					(Vec3V_In vPos, ScalarV_In windSpeed, ScalarV_In waterSpeed);

			float				GetHeight					()						{return m_Height_CurrRadiusSqr_CurrStrength.GetXf();}

#if __BANK
			void				DebugDraw					();
#endif

		///////////////////////////////////
		// VARIABLES
		///////////////////////////////////

		private: //////////////////////////
			void				SetCurrStrength(float f) { m_Height_CurrRadiusSqr_CurrStrength.SetZf(f); }
			float				GetCurrStrength() { return m_Height_CurrRadiusSqr_CurrStrength.GetZf(); }

			// These are a vector type because we need them as a vector in GetVelocity
			Vec3V				m_Height_CurrRadiusSqr_CurrStrength;

			// These are 4-byte types because we don't need them in GetVelocity
			float				m_radius;
			float				m_shrinkChance;
			float				m_maxStrength;
			float				m_deltaStrength;

			eThermalMode		m_state;

	};


	///////////////////////////////////////////////////////////////////////////////
	//  phWindThermalGroup
	///////////////////////////////////////////////////////////////////////////////

	class phWindThermalGroup : public phWindDisturbanceGroupBase
	{
		///////////////////////////////////
		// DEFINES 
		///////////////////////////////////



		///////////////////////////////////
		// FUNCTIONS 
		///////////////////////////////////

		public: ///////////////////////////

						phWindThermalGroup				();
						~phWindThermalGroup				() {m_disturbances.Reset();}

			void		Init							(int groupSize);
			void		Reset							();

			void 		Update							(float dt);
			void 		Apply							(phWindField* RESTRICT pDisturbanceField, int batch);

			__forceinline Vec3V_Out	GetVelocity			(Vec3V_In vPos, ScalarV_In windSpeed, ScalarV_In waterSpeed)
			{
				Vec3V vVel(V_ZERO);

				// loop through the active disturbances
				for (int i=0; i<this->m_disturbances.GetCount(); i++)
				{
					if (this->m_disturbances[i].GetIsActive() && this->m_disturbances[i].GetIsCulled()==false)
					{
						vVel += this->m_disturbances[i].GetVelocity(vPos, windSpeed, waterSpeed);
					}
				}

				return vVel;
			}

			int 		AddDisturbance					(const phWindDisturbanceBase* pDisturbance);
			void		RemoveDisturbance				(int index);	
			int			GetNumActiveDisturbances		();

			void		SetAutoData						(const phThermalAutoData& val)			{m_autoData = val;}

#if __BANK
			void		DebugDraw						();
#endif

		///////////////////////////////////
		// VARIABLES 
		///////////////////////////////////

			phThermalAutoData		m_autoData;
			atArray<phWindThermal>	m_disturbances;
	};

} // namespace rage


#endif // PHEFFECTS_WIND_THERMAL_H 
