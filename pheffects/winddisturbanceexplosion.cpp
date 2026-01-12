// 
// pheffects/winddisturbanceexplosion.cpp 
// 
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved. 
// 

///////////////////////////////////////////////////////////////////////////////
//  INCLUDES
///////////////////////////////////////////////////////////////////////////////

#include "winddisturbanceexplosion.h"
#include "wind_optimisations.h"

// rage
#if __BANK
#include "grcore/debugdraw.h"
#endif

WIND_OPTIMISATIONS()

namespace rage
{
	///////////////////////////////////////////////////////////////////////////////
	//  CODE
	///////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////
	//  CLASS phWindExplosion
	///////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////
	//  Constructor
	///////////////////////////////////////////////////////////////////////////////

	phWindExplosion::phWindExplosion						()
	{
		m_vPos = Vec3V(V_ZERO);
		m_type = WIND_DIST_GLOBAL;

		m_Radius_Force.ZeroComponents();

//		m_currTime = 0.0f;
//		m_currRadius = m_settings.startRadius;
//		m_currForce = m_settings.startForce;
		
		m_processed = false;
	}


	///////////////////////////////////////////////////////////////////////////////
	//  Constructor
	///////////////////////////////////////////////////////////////////////////////

	phWindExplosion::phWindExplosion						(phWindDistType_e type, Vec3V_In pos, const phExplosionSettings& settings)
	{
		m_vPos = pos;
		m_type = type;

		m_Radius_Force = Vec2V(settings.radius, settings.force);

//		m_currTime = 0.0f;
//		m_currRadius = m_settings.startRadius;
//		m_currForce = m_settings.startForce;

		m_processed = false;
	}


	///////////////////////////////////////////////////////////////////////////////
	//  Update
	///////////////////////////////////////////////////////////////////////////////

	bool			phWindExplosion::Update							(float UNUSED_PARAM(dt))
	{
		return m_processed;
// 		m_currTime += dt;
// 		if (m_currTime>=m_settings.totalLife)
// 		{
// 			return true;
// 		}
// 
// 		float timeRatio = m_currTime/m_settings.totalLife;
// 
// 		m_currRadius = m_settings.startRadius + ((m_settings.endRadius-m_settings.startRadius)*timeRatio);
// 		m_currForce = m_settings.startForce + ((m_settings.endForce-m_settings.startForce)*timeRatio);
// 
// 		return false;
	}


	///////////////////////////////////////////////////////////////////////////////
	//  Apply
	///////////////////////////////////////////////////////////////////////////////

	void			phWindExplosion::Apply							(phWindField* RESTRICT UNUSED_PARAM(pDisturbanceField), u32 UNUSED_PARAM(startGridX), u32 UNUSED_PARAM(endGridX))
	{
		m_processed = true;
	}


	///////////////////////////////////////////////////////////////////////////////
	//  GetVelocity
	///////////////////////////////////////////////////////////////////////////////

	Vec3V_Out			phWindExplosion::GetVelocity				(Vec3V_In vPos, ScalarV_In UNUSED_PARAM(windSpeed), ScalarV_In UNUSED_PARAM(waterSpeed))
	{
		Vec3V vVel(V_ZERO);

		Vec3V vDir = vPos - m_vPos;
		ScalarV radSq = square(m_Radius_Force.GetX());
		if (IsLessThanOrEqualAll(MagSquared(vDir), radSq))
		{
			vDir = NormalizeSafe(vDir, Vec3V(V_X_AXIS_WZERO));
			vVel += vDir*m_Radius_Force.GetY();
		}

		return vVel;
	}


	///////////////////////////////////////////////////////////////////////////////
	//  DebugDraw
	///////////////////////////////////////////////////////////////////////////////

#if __BANK
	void			phWindExplosion::DebugDraw							()
	{
		grcDebugDraw::Sphere(GetPos(), m_Radius_Force.GetXf(), Color32(0.0f, 1.0f, 0.0f, 1.0f), false);
	}
#endif // __BANK



	///////////////////////////////////////////////////////////////////////////////
	//  CLASS phWindExplosionGroup
	///////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////
	//  Constructor
	///////////////////////////////////////////////////////////////////////////////

	phWindExplosionGroup::phWindExplosionGroup					()											
	{
		m_disturbances.Reset();
	}

	///////////////////////////////////////////////////////////////////////////////
	//  Init
	///////////////////////////////////////////////////////////////////////////////

	void phWindExplosionGroup::Init					(int groupSize)											
	{
		m_disturbances.Resize(groupSize);
	}

	///////////////////////////////////////////////////////////////////////////////
	//  Reset
	///////////////////////////////////////////////////////////////////////////////

	void phWindExplosionGroup::Reset					()											
	{
		for(int i = 0; i < m_disturbances.GetCount(); i++)
		{
			m_disturbances[i].SetIsActive(false);
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	//  Update
	///////////////////////////////////////////////////////////////////////////////

	void			phWindExplosionGroup::Update							(float dt)
	{
		// remove processed explosions from the array
		for (int i=0; i<m_disturbances.GetCount(); i++)
		{
			// only if they've been processed already
			if (this->m_disturbances[i].GetIsActive())
			{
				if (this->m_disturbances[i].Update(dt))
				{
					this->m_disturbances[i].SetIsActive(false);
				}
			}
		}
	}


	///////////////////////////////////////////////////////////////////////////////
	//  Apply
	///////////////////////////////////////////////////////////////////////////////

	void			phWindExplosionGroup::Apply							(phWindField* RESTRICT pDisturbanceField, int batch)
	{
		// calc the start and end of this batch
		u32 startGridX = WINDFIELD_NUM_ELEMENTS_X * batch / WIND_NUM_BATCHES;
		u32 endGridX = WINDFIELD_NUM_ELEMENTS_X * (batch+1) / WIND_NUM_BATCHES;

		// loop through the active disturbances
		for (int i=0; i<m_disturbances.GetCount(); i++)
		{			
			if (this->m_disturbances[i].GetIsActive())
			{
				this->m_disturbances[i].Apply(pDisturbanceField, startGridX, endGridX);
			}
		}
	}
	
	///////////////////////////////////////////////////////////////////////////////
	//  AddDisturbance
	///////////////////////////////////////////////////////////////////////////////

	int 			phWindExplosionGroup::AddDisturbance				(const phWindDisturbanceBase* pDisturbance)
	{
		for (int i=0; i<m_disturbances.GetCount(); i++)
		{
			if (m_disturbances[i].GetIsActive()==false)
			{
				m_disturbances[i] = *(static_cast<const phWindExplosion*>(pDisturbance));
				m_disturbances[i].SetIsActive(true);
				return i;
			}
		}

		return -1;
	}

	///////////////////////////////////////////////////////////////////////////////
	//  RemoveDisturbance
	///////////////////////////////////////////////////////////////////////////////

	void			phWindExplosionGroup::RemoveDisturbance			(int index)
	{
		Assertf(index>=0 && index<m_disturbances.GetCount(), "Trying to remove a disturbance out of the array range");
		m_disturbances[index].SetIsActive(false);
	}

	///////////////////////////////////////////////////////////////////////////////
	//  GetNumActiveDisturbances
	///////////////////////////////////////////////////////////////////////////////

	int				phWindExplosionGroup::GetNumActiveDisturbances			()	
	{
		int count = 0;
		for (int i=0; i<m_disturbances.GetCount(); i++)
		{
			if (m_disturbances[i].GetIsActive())
			{
				count++;
			}
		}

		return count;
	}

#if __BANK
	///////////////////////////////////////////////////////////////////////////////
	//  DebugDraw
	///////////////////////////////////////////////////////////////////////////////

	void			phWindExplosionGroup::DebugDraw					()
	{
		// loop through the active disturbances
		for (int i=0; i<m_disturbances.GetCount(); i++)
		{
			if (m_disturbances[i].GetIsActive())
			{
				m_disturbances[i].DebugDraw();
			}
		}
	}
#endif



	///////////////////////////////////////////////////////////////////////////////
	//  CLASS phWindDirExplosion
	///////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////
	//  Constructor
	///////////////////////////////////////////////////////////////////////////////

	phWindDirExplosion::phWindDirExplosion						()
	{
		m_vPos = Vec3V(V_ZERO);
		m_type = WIND_DIST_GLOBAL;

		m_vDir.ZeroComponents();
		m_Length_Radius_Force.ZeroComponents();

		m_processed = false;
	}


	///////////////////////////////////////////////////////////////////////////////
	//  Constructor
	///////////////////////////////////////////////////////////////////////////////

	phWindDirExplosion::phWindDirExplosion						(phWindDistType_e type, Vec3V_In pos, const phDirExplosionSettings& settings)
	{
		m_vPos = pos;
		m_type = type;

		m_vDir = settings.vDir;
		m_Length_Radius_Force = Vec3V(settings.length, settings.radius, settings.force);

		m_processed = false;
	}


	///////////////////////////////////////////////////////////////////////////////
	//  Update
	///////////////////////////////////////////////////////////////////////////////

	bool			phWindDirExplosion::Update							(float UNUSED_PARAM(dt))
	{
		return m_processed;
	}


	///////////////////////////////////////////////////////////////////////////////
	//  Apply
	///////////////////////////////////////////////////////////////////////////////

	void			phWindDirExplosion::Apply							(phWindField* RESTRICT UNUSED_PARAM(pDisturbanceField), u32 UNUSED_PARAM(startGridX), u32 UNUSED_PARAM(endGridX))
	{
		m_processed = true;
	}


	///////////////////////////////////////////////////////////////////////////////
	//  GetVelocity
	///////////////////////////////////////////////////////////////////////////////

	Vec3V_Out			phWindDirExplosion::GetVelocity					(Vec3V_In vPos, ScalarV_In UNUSED_PARAM(windSpeed), ScalarV_In UNUSED_PARAM(waterSpeed))
	{
		Vec3V vVel(V_ZERO);

 		Vec3V vDirExplosionPos = m_vPos;
		ScalarV length = m_Length_Radius_Force.GetX();
		ScalarV radius = m_Length_Radius_Force.GetY();
		ScalarV force = m_Length_Radius_Force.GetZ();
		Vec3V vDirExplosionPosEnd = AddScaled(vDirExplosionPos, m_vDir, length);

		ScalarV distSqr = phWindDisturbanceHelper::DistSqrFromPointToLineSegmentV(vPos, vDirExplosionPos, vDirExplosionPosEnd);

		if (IsLessThanOrEqualAll(distSqr, radius*radius))
		{
			Vec3V vCylinderVec = vDirExplosionPosEnd - vDirExplosionPos;
			Vec3V vPosVec = vPos - vDirExplosionPos;
			ScalarV dot = Dot(vCylinderVec, vPosVec);
			dot = Saturate(dot);

			vVel += Scale(m_vDir, force * dot);
		}

		return vVel;
	}


	///////////////////////////////////////////////////////////////////////////////
	//  DebugDraw
	///////////////////////////////////////////////////////////////////////////////

#if __BANK
	void			phWindDirExplosion::DebugDraw							()
	{
		ScalarV half(V_HALF);
		ScalarV length = m_Length_Radius_Force.GetX();
		ScalarV radius = m_Length_Radius_Force.GetY();
		Vec3V vCentrePos = GetPos() + Scale(m_vDir, half*length);

		Vec3V vRight(V_X_AXIS_WZERO);
		Vec3V vForward = Cross(m_vDir, vRight);
		vForward = Normalize(vForward);
		vRight = Cross(vForward, m_vDir);
		vRight = Normalize(vRight);

		Mat34V vMtx;
		vMtx.SetCol0(vRight);
		vMtx.SetCol1(m_vDir);
		vMtx.SetCol2(vForward);
		vMtx.SetCol3(vCentrePos);

		grcDrawCylinder(length.Getf(), radius.Getf(), RCC_MATRIX34(vMtx), 16, true);
	}
#endif // __BANK



	///////////////////////////////////////////////////////////////////////////////
	//  CLASS phWindDirExplosionGroup
	///////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////
	//  Constructor
	///////////////////////////////////////////////////////////////////////////////

	phWindDirExplosionGroup::phWindDirExplosionGroup					()											
	{
		m_disturbances.Reset();
	}

	///////////////////////////////////////////////////////////////////////////////
	//  Init
	///////////////////////////////////////////////////////////////////////////////

	void phWindDirExplosionGroup::Init					(int groupSize)											
	{
		m_disturbances.Resize(groupSize);
	}

	///////////////////////////////////////////////////////////////////////////////
	//  Reset
	///////////////////////////////////////////////////////////////////////////////

	void phWindDirExplosionGroup::Reset					()											
	{
		for(int i = 0; i < m_disturbances.GetCount(); i++)
		{
			m_disturbances[i].SetIsActive(false);
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	//  Update
	///////////////////////////////////////////////////////////////////////////////

	void			phWindDirExplosionGroup::Update							(float dt)
	{
		// remove processed hemisphees from the array
		for (int i=0; i<m_disturbances.GetCount(); i++)
		{
			// only if they've been processed already
			if (this->m_disturbances[i].GetIsActive())
			{
				if (this->m_disturbances[i].Update(dt))
				{
					this->m_disturbances[i].SetIsActive(false);
				}
			}
		}
	}


	///////////////////////////////////////////////////////////////////////////////
	//  Apply
	///////////////////////////////////////////////////////////////////////////////

	void			phWindDirExplosionGroup::Apply							(phWindField* RESTRICT pDisturbanceField, int batch)
	{
		// calc the start and end of this batch
		u32 startGridX = WINDFIELD_NUM_ELEMENTS_X * batch / WIND_NUM_BATCHES;
		u32 endGridX = WINDFIELD_NUM_ELEMENTS_X * (batch+1) / WIND_NUM_BATCHES;

		// loop through the active disturbances
		for (int i=0; i<m_disturbances.GetCount(); i++)
		{			
			if (this->m_disturbances[i].GetIsActive())
			{
				this->m_disturbances[i].Apply(pDisturbanceField, startGridX, endGridX);
			}
		}
	}
	
	///////////////////////////////////////////////////////////////////////////////
	//  AddDisturbance
	///////////////////////////////////////////////////////////////////////////////

	int 			phWindDirExplosionGroup::AddDisturbance				(const phWindDisturbanceBase* pDisturbance)
	{
		for (int i=0; i<m_disturbances.GetCount(); i++)
		{
			if (m_disturbances[i].GetIsActive()==false)
			{
				m_disturbances[i] = *(static_cast<const phWindDirExplosion*>(pDisturbance));
				m_disturbances[i].SetIsActive(true);
				return i;
			}
		}

		return -1;
	}

	///////////////////////////////////////////////////////////////////////////////
	//  RemoveDisturbance
	///////////////////////////////////////////////////////////////////////////////

	void			phWindDirExplosionGroup::RemoveDisturbance			(int index)
	{
		Assertf(index>=0 && index<m_disturbances.GetCount(), "Trying to remove a disturbance out of the array range");
		m_disturbances[index].SetIsActive(false);
	}

	///////////////////////////////////////////////////////////////////////////////
	//  GetNumActiveDisturbances
	///////////////////////////////////////////////////////////////////////////////

	int				phWindDirExplosionGroup::GetNumActiveDisturbances			()	
	{
		int count = 0;
		for (int i=0; i<m_disturbances.GetCount(); i++)
		{
			if (m_disturbances[i].GetIsActive())
			{
				count++;
			}
		}

		return count;
	}

#if __BANK
	///////////////////////////////////////////////////////////////////////////////
	//  DebugDraw
	///////////////////////////////////////////////////////////////////////////////

	void			phWindDirExplosionGroup::DebugDraw					()
	{
		// loop through the active disturbances
		for (int i=0; i<m_disturbances.GetCount(); i++)
		{
			if (m_disturbances[i].GetIsActive())
			{
				m_disturbances[i].DebugDraw();
			}
		}
	}
#endif
















} // namespace rage
