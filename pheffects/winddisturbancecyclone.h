// 
// pheffects/winddisturbancecyclone.h 
// 
// Copyright (C) 1999-2008 Rockstar Games.  All Rights Reserved. 
// 

#ifndef PHEFFECTS_WIND_CYCLONE_H 
#define PHEFFECTS_WIND_CYCLONE_H 


///////////////////////////////////////////////////////////////////////////////
//  INCLUDES
///////////////////////////////////////////////////////////////////////////////	

#include "winddisturbancebase.h"

// rage
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

	enum eCycloneMode	
	{	
		CYCLONE_STATE_OFF			= 0,
		CYCLONE_STATE_GROWING,
		CYCLONE_STATE_ON,
		CYCLONE_STATE_SHRINKING 
	};


	///////////////////////////////////////////////////////////////////////////////
	//  STRUCTURES
	///////////////////////////////////////////////////////////////////////////////

	struct phCycloneSettings
	{	
		float					range;
		float					maxStrength;
		float					deltaStrength;
		float					forceMult;
		float					shrinkChance;
	};

	struct phCycloneAutoData	
	{
		phWindDistType_e		type;		
		float					rangeMin;
		float					rangeMax;
		float					strengthMin;
		float					strengthMax;
		float					strengthDelta;
		float					forceMult;
		float					createChance;
		float					shrinkChance;

	};


	///////////////////////////////////////////////////////////////////////////////
	//  CLASSES
	///////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////
	//  phWindCyclone
	///////////////////////////////////////////////////////////////////////////////

	class phWindCyclone : public phWindDisturbanceBase
	{

		///////////////////////////////////
		// FUNCTIONS 
		///////////////////////////////////

		public: ///////////////////////////

								phWindCyclone				();
								phWindCyclone				(phWindDistType_e type, Vec3V_In pos, const phCycloneSettings& settings);

			bool				Update						(float dt);
			__forceinline Vec3V_Out	GetVelocity				(Vec3V_In vPos, ScalarV_In windSpeedV, ScalarV_In waterSpeedV)
			{
				float windSpeed = windSpeedV.Getf();
				float waterSpeed = waterSpeedV.Getf();
				register Vec3V vVel(V_ZERO);

				// calc the velocity of the cyclone at this position
				Vec3V vDelta = vPos - m_vPos;
				vDelta.SetZ(ScalarV(V_ZERO));
				float dist = Mag(vDelta).Getf();
				if (dist <= m_settings.range)
				{
					float relDist = dist / m_settings.range;

					bool underWater = vPos.GetZf()<phWindField::GetWaterLevelWld();
					if ((m_type==WIND_DIST_AIR && underWater) ||
						(m_type==WIND_DIST_WATER && !underWater))
					{
						return vVel;
					}

					float strengthMult = windSpeed;
					if (underWater)
					{
						strengthMult = waterSpeed;
					}

					float force = rage::Min(relDist, 1.0f-relDist);
					force *= m_currStrength*strengthMult;

					Vec3V vDelta2 = vDelta * ScalarVFromF32(force / dist);
					vVel += ScalarVFromF32(force) * Vec3V(vDelta2.GetY(), -vDelta2.GetX(), ScalarV(V_ZERO)) * ScalarVFromF32(m_settings.forceMult);
				}

				return vVel;
			}


#if __BANK
			void				DebugDraw					();
#endif

		///////////////////////////////////
		// VARIABLES
		///////////////////////////////////

		private: //////////////////////////

			phCycloneSettings	m_settings;

			float				m_currStrength;	
			eCycloneMode		m_state;

	};


	///////////////////////////////////////////////////////////////////////////////
	//  phWindCycloneGroup
	///////////////////////////////////////////////////////////////////////////////

	class phWindCycloneGroup : public phWindDisturbanceGroupBase
	{
		///////////////////////////////////
		// DEFINES 
		///////////////////////////////////



		///////////////////////////////////
		// FUNCTIONS 
		///////////////////////////////////

		public: ///////////////////////////

				 				phWindCycloneGroup			();
								~phWindCycloneGroup			() {m_disturbances.Reset();}

			void				Init						(int groupSize);

			void				Reset						();

			void 				Update						(float dt);
			void 				Apply						(phWindField* RESTRICT pDisturbanceField, int batch);

			Vec3V_Out			GetVelocity					(Vec3V_In vPos, ScalarV_In windSpeed, ScalarV_In waterSpeed);

			int 				AddDisturbance				(const phWindDisturbanceBase* pDisturbance);
			void				RemoveDisturbance			(int index);	
			int					GetNumActiveDisturbances			();

			void				SetAutoData					(const phCycloneAutoData& val)							{m_autoData = val;}

#if __BANK
			void				DebugDraw					();
#endif

		///////////////////////////////////
		// VARIABLES 
		///////////////////////////////////

		private: //////////////////////////

			phCycloneAutoData		m_autoData;
			atArray<phWindCyclone>	m_disturbances;

	};

} // namespace rage


#endif // PHEFFECTS_WIND_CYCLONE_H 
