// 
// pheffects/winddisturbancehemisphere.h 
// 
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved. 
// 

#ifndef PHEFFECTS_WIND_HEMISPHERE_H 
#define PHEFFECTS_WIND_HEMISPHERE_H 


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

	//  phHemisphereSettings  //////////////////////////////////////////////////////
	struct phHemisphereSettings
	{
		Vec3V				vDir;
		float 				radius;
		float				force;
	};


	///////////////////////////////////////////////////////////////////////////////
	//  CLASSES
	///////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////
	//  phWindHemisphere
	///////////////////////////////////////////////////////////////////////////////

	class phWindHemisphere : public phWindDisturbanceBase
	{
		///////////////////////////////////
		// FUNCTIONS 
		///////////////////////////////////

		public: ///////////////////////////

								phWindHemisphere			();
								phWindHemisphere			(phWindDistType_e type, Vec3V_In pos, const phHemisphereSettings& settings);

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

			Vec3V					m_vDir;
			Vec2V					m_Radius_Force;

			bool					m_processed;

	};


	///////////////////////////////////////////////////////////////////////////////
	//  phWindHemisphereGroup
	///////////////////////////////////////////////////////////////////////////////

	class phWindHemisphereGroup : public phWindDisturbanceGroupBase
	{
		///////////////////////////////////
		// DEFINES 
		///////////////////////////////////



		///////////////////////////////////
		// FUNCTIONS 
		///////////////////////////////////

		public: ///////////////////////////

									phWindHemisphereGroup		();
									~phWindHemisphereGroup		() {m_disturbances.Reset();}

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

			atArray<phWindHemisphere>	m_disturbances;


	};

} // end namespace




#endif // PHEFFECTS_WIND_HEMISPHERE_H 
