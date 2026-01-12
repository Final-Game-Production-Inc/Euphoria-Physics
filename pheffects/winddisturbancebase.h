// 
// pheffects/winddisturbancebase.h 
// 
// Copyright (C) 1999-2008 Rockstar Games.  All Rights Reserved. 
// 

#ifndef PHEFFECTS_WINDDISTURBANCEBASE_H 
#define PHEFFECTS_WINDDISTURBANCEBASE_H 

///////////////////////////////////////////////////////////////////////////////
//  INCLUDES
///////////////////////////////////////////////////////////////////////////////	

#include "atl/array.h"
#include "pheffects/windfield.h"
#include "vectormath/classes.h"


namespace rage
{

	///////////////////////////////////////////////////////////////////////////////
	//  ENUMERATIONS
	///////////////////////////////////////////////////////////////////////////////

	enum phWindDistType_e
	{
		WIND_DIST_GLOBAL = 0,
		WIND_DIST_AIR,
		WIND_DIST_WATER
	};


	///////////////////////////////////////////////////////////////////////////////
	//  CLASSES
	///////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////
	//  phWindDisturbanceHelper
	///////////////////////////////////////////////////////////////////////////////

	class phWindDisturbanceHelper
	{
		///////////////////////////////////
		// FUNCTIONS 
		///////////////////////////////////

		public: ///////////////////////////

			static Vec3V_Out		ClosestPointOnLineSegment		(Vec3V_In vP, Vec3V_In vP1, Vec3V_In vP2)
			{
				Vec3V vDiff = vP-vP1;
				Vec3V vDir = vP2-vP1;
				ScalarV vT = Dot(vDiff, vDir)/Dot(vDir, vDir);
				vT = Saturate(vT);
				return vP1 + vT * vDir;
			}

			static float		DistSqrFromPointToLineSegment	(Vec3V_In vP, Vec3V_In vP1, Vec3V_In vP2)
			{
				Vec3V vVec = vP - ClosestPointOnLineSegment(vP, vP1, vP2);
				return MagSquared(vVec).Getf();
			}

			static ScalarV		DistSqrFromPointToLineSegmentV	(Vec3V_In vP, Vec3V_In vP1, Vec3V_In vP2)
			{
				Vec3V vVec = vP - ClosestPointOnLineSegment(vP, vP1, vP2);
				return MagSquared(vVec);
			}
	};

	///////////////////////////////////////////////////////////////////////////////
	//  phWindDisturbanceBase
	///////////////////////////////////////////////////////////////////////////////

	class phWindDisturbanceBase
	{
		///////////////////////////////////
		// FUNCTIONS 
		///////////////////////////////////

		public: ///////////////////////////

								phWindDisturbanceBase		(Vec3V_In vPos = Vec3V(V_ZERO)) 
															: m_vPos(vPos), 
															  m_type(WIND_DIST_GLOBAL),
															  m_isActive(false),
															  m_isCulled(false)						{}
	virtual						~phWindDisturbanceBase		()										{}

			void 				SetIsActive					(bool val)								{m_isActive = val;}
			bool 				GetIsActive					()										{return m_isActive;}
			void 				SetIsCulled					(bool val)								{m_isCulled = val;}
			bool 				GetIsCulled					()										{return m_isCulled;}
			Vec3V&				GetPos()					{return m_vPos;}

#if __BANK
	virtual	void				DebugDraw					()										{}
#endif


		///////////////////////////////////
		// VARIABLES
		///////////////////////////////////

		protected: ////////////////////////

			Vec3V				m_vPos;
			phWindDistType_e	m_type;
			bool				m_isActive;
			bool				m_isCulled;
	};


	///////////////////////////////////////////////////////////////////////////////
	//  phWindDisturbanceGroupBase
	///////////////////////////////////////////////////////////////////////////////

	class phWindDisturbanceGroupBase
	{
		///////////////////////////////////
		// FUNCTIONS 
		///////////////////////////////////

		public: ///////////////////////////

								phWindDisturbanceGroupBase		() 
																: m_automated(true),
																  m_cullRange(0.0f),
																  m_cullMax(0)						{}
	virtual						~phWindDisturbanceGroupBase		()									{}

	virtual	void				Init							(int groupSize) = 0;
	virtual	void 				Update							(float dt) = 0;
	virtual	void 				Apply							(phWindField* RESTRICT pDisturbanceField, int batch) = 0;
	__forceinline Vec3V_Out		GetVelocity						(Vec3V_In UNUSED_PARAM(vPos), ScalarV_In UNUSED_PARAM(windSpeed), ScalarV_In UNUSED_PARAM(waterSpeed))		{return Vec3V(V_ZERO);}

	virtual	int 				AddDisturbance					(const phWindDisturbanceBase* pDisturbance) = 0;
	virtual	void				RemoveDisturbance				(int index) = 0;	
	virtual	int					GetNumActiveDisturbances				() = 0;	

			void 				SetAutomated					(bool val)							{m_automated = val;}
			void				SetCullInfo						(float range, int max)				{m_cullRange = range; m_cullMax = max;}

#if __BANK
	virtual	void				DebugDraw						() = 0;
#endif

		///////////////////////////////////
		// VARIABLES
		///////////////////////////////////

		protected: ////////////////////////

			bool				m_automated;										// is the group is in charge of automating new disturbances

			float				m_cullRange;										// the distance outside which disturbances are considered culled (0.0f = don't use)	
			int					m_cullMax;											// the max number of non culled disturbances (0 = don't use)

	};


} // namespace rage

#endif // PHEFFECTS_WINDDISTURBANCEBASE_H 
