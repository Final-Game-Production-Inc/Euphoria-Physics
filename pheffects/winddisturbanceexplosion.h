// 
// pheffects/winddisturbanceexplosion.h 
// 
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved. 
// 

#ifndef PHEFFECTS_WIND_EXPLOSION_H 
#define PHEFFECTS_WIND_EXPLOSION_H 


///////////////////////////////////////////////////////////////////////////////
//  INCLUDES
///////////////////////////////////////////////////////////////////////////////	

#include "pheffects/winddisturbancebase.h"

// rage
#include "math/random.h"
#include "vectormath/classes.h"
#include "vectormath/legacyconvert.h"


///////////////////////////////////////////////////////////////////////////////
//  DEFINES
///////////////////////////////////////////////////////////////////////////////	

namespace rage
{

	///////////////////////////////////////////////////////////////////////////////
	//  STRUCTURES
	///////////////////////////////////////////////////////////////////////////////	

	//  phExplosionSettings  //////////////////////////////////////////////////////
	struct phExplosionSettings
	{
		float 				radius;
		float				force;
	};

	//  phDirExplosionSettings  ///////////////////////////////////////////////////
	struct phDirExplosionSettings
	{
		Vec3V				vDir;
		float				length;
		float 				radius;
		float				force;
	};


	///////////////////////////////////////////////////////////////////////////////
	//  CLASSES
	///////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////
	//  phWindExplosion
	///////////////////////////////////////////////////////////////////////////////

	class phWindExplosion : public phWindDisturbanceBase
	{
		///////////////////////////////////
		// FUNCTIONS 
		///////////////////////////////////

		public: ///////////////////////////

								phWindExplosion				();
								phWindExplosion				(phWindDistType_e type, Vec3V_In pos, const phExplosionSettings& settings);

			bool				Update						(float dt);
			void				Apply						(phWindField* RESTRICT pDisturbanceField, u32 startGridX, u32 endGridX);
			Vec3V_Out			GetVelocity					(Vec3V_In vPos, ScalarV_In windSpeed, ScalarV_In waterSpeed);

#if __BANK
			void				DebugDraw					();
#endif

		///////////////////////////////////
		// VARIABLES
		///////////////////////////////////

		private: //////////////////////////

//			phExplosionSettings	m_settings;

//			float				m_currTime;
//			float				m_currRadius;
//			float				m_currForce;

			bool				m_processed;

			Vec2V				m_Radius_Force;
	};


	///////////////////////////////////////////////////////////////////////////////
	//  phWindExplosionGroup
	///////////////////////////////////////////////////////////////////////////////

	class phWindExplosionGroup : public phWindDisturbanceGroupBase
	{
		///////////////////////////////////
		// DEFINES 
		///////////////////////////////////



		///////////////////////////////////
		// FUNCTIONS 
		///////////////////////////////////

		public: ///////////////////////////

										phWindExplosionGroup		();
										~phWindExplosionGroup		() {m_disturbances.Reset();}

			void						Init						(int groupSize);
			void						Reset						();

			void						Update						(float dt);
			void						Apply						(phWindField* RESTRICT pDisturbanceField, int batch);

			__forceinline Vec3V_Out		GetVelocity					(Vec3V_In vPos, ScalarV_In windSpeed, ScalarV_In waterSpeed)
			{
				register Vec3V vVel(V_ZERO);

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

			int 						AddDisturbance				(const phWindDisturbanceBase* pDisturbance);
			void						RemoveDisturbance			(int index);	
			int							GetNumActiveDisturbances	();

#if __BANK
			void						DebugDraw					();
#endif


		///////////////////////////////////
		// VARIABLES 
		///////////////////////////////////

			atArray<phWindExplosion>	m_disturbances;


	};



	///////////////////////////////////////////////////////////////////////////////
	//  phWindDirExplosion
	///////////////////////////////////////////////////////////////////////////////

	class phWindDirExplosion : public phWindDisturbanceBase
	{
		///////////////////////////////////
		// FUNCTIONS 
		///////////////////////////////////

		public: ///////////////////////////

								phWindDirExplosion			();
								phWindDirExplosion			(phWindDistType_e type, Vec3V_In pos, const phDirExplosionSettings& settings);

			bool				Update						(float dt);
			void				Apply						(phWindField* RESTRICT pDisturbanceField, u32 startGridX, u32 endGridX);
			Vec3V_Out			GetVelocity					(Vec3V_In vPos, ScalarV_In windSpeed, ScalarV_In waterSpeed);

#if __BANK
			void				DebugDraw					();
#endif

		///////////////////////////////////
		// VARIABLES
		///////////////////////////////////

		private: //////////////////////////

			bool					m_processed;

//			phDirExplosionSettings	m_settings;
			Vec3V					m_vDir;
			Vec3V					m_Length_Radius_Force;


	};


	///////////////////////////////////////////////////////////////////////////////
	//  phWindDirExplosionGroup
	///////////////////////////////////////////////////////////////////////////////

	class phWindDirExplosionGroup : public phWindDisturbanceGroupBase
	{
		///////////////////////////////////
		// DEFINES 
		///////////////////////////////////



		///////////////////////////////////
		// FUNCTIONS 
		///////////////////////////////////

		public: ///////////////////////////

									phWindDirExplosionGroup		();
									~phWindDirExplosionGroup		() {m_disturbances.Reset();}

			void					Init						(int groupSize);
			void					Reset						();

			void					Update						(float dt);
			void					Apply						(phWindField* RESTRICT pDisturbanceField, int batch);

			__forceinline Vec3V_Out	GetVelocity					(Vec3V_In vPos, ScalarV_In windSpeed, ScalarV_In waterSpeed)
			{
				register Vec3V vVel(V_ZERO);

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

			int 					AddDisturbance				(const phWindDisturbanceBase* pDisturbance);
			void					RemoveDisturbance			(int index);	
			int						GetNumActiveDisturbances	();

#if __BANK
			void					DebugDraw					();
#endif


		///////////////////////////////////
		// VARIABLES 
		///////////////////////////////////

			atArray<phWindDirExplosion>	m_disturbances;


	};















} // end namespace




#endif // PHEFFECTS_WIND_EXPLOSION_H 
