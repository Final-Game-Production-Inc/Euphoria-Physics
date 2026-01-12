// 
// pheffects/winddisturbancedownwash.cpp 
// 
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved. 
// 

///////////////////////////////////////////////////////////////////////////////
//  INCLUDES
///////////////////////////////////////////////////////////////////////////////

#include "winddisturbancedownwash.h"
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
	//  CLASS phWindDownwash
	///////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////
	//  Constructor
	///////////////////////////////////////////////////////////////////////////////

					phWindDownwash::phWindDownwash					()
	{
		m_vPos = Vec3V(V_ZERO);
		m_type = WIND_DIST_GLOBAL;

		vDir_Radius = Vec4V(Vec3V(V_ZERO), ScalarV(V_ZERO));
		vForce_GroundZ_ZFadeThreshMin_ZFadeThreshMax = Vec4V(0.0f, -100000.0f, 0.0f, 0.0f);

		m_processed = false;
	}


	///////////////////////////////////////////////////////////////////////////////
	//  Constructor
	///////////////////////////////////////////////////////////////////////////////

					phWindDownwash::phWindDownwash					(phWindDistType_e type, Vec3V_In pos, const phDownwashSettings& settings)
	{
		m_vPos = pos;
		m_type = type;

		vDir_Radius = Vec4V(settings.vDir, ScalarV(settings.radius));
		vForce_GroundZ_ZFadeThreshMin_ZFadeThreshMax = Vec4V(
			settings.force,
			settings.groundZ,
			settings.zFadeThreshMin,
			settings.zFadeThreshMax);

		m_processed = false;
	}


	///////////////////////////////////////////////////////////////////////////////
	//  Update
	///////////////////////////////////////////////////////////////////////////////

	bool			phWindDownwash::Update							(float UNUSED_PARAM(dt))
	{
		return m_processed;
	}


	///////////////////////////////////////////////////////////////////////////////
	//  Apply
	///////////////////////////////////////////////////////////////////////////////

	void			phWindDownwash::Apply							(phWindField* RESTRICT UNUSED_PARAM(pDisturbanceField), u32 UNUSED_PARAM(startGridX), u32 UNUSED_PARAM(endGridX))
	{
		m_processed = true;
	}


	///////////////////////////////////////////////////////////////////////////////
	//  GetVelocity
	///////////////////////////////////////////////////////////////////////////////

	Vec3V_Out		phWindDownwash::GetVelocity						(Vec3V_In vPos, ScalarV_In UNUSED_PARAM(windSpeed), ScalarV_In UNUSED_PARAM(waterSpeed))
	{
		Vec3V vVel(V_ZERO);

		Vec3V vDir = vPos - m_vPos;
		Vec3V settingsDir = vDir_Radius.GetXYZ();
		ScalarV radius = vDir_Radius.GetW();
		ScalarV radSq = square(radius);

		ScalarV distSqr = MagSquared(vDir);

		if (IsLessThanOrEqualAll(distSqr, radSq))
		{
			ScalarV angle = Dot(vDir, settingsDir);
			if (IsGreaterThanAll(angle, ScalarV(V_ZERO)))
			{
				// below the rotor
				ScalarV distSqrRatio = ScalarV(V_ONE) - (distSqr/radSq);
// 				static float zFadeThreshMax = 10.0f;
// 				static float zFadeThreshMin = 5.0f;
				ScalarV groundZ = vForce_GroundZ_ZFadeThreshMin_ZFadeThreshMax.GetY();
				ScalarV heightAboveGround = vPos.GetZ() - groundZ;
				ScalarV zFadeThreshMin = vForce_GroundZ_ZFadeThreshMin_ZFadeThreshMax.GetZ();
				ScalarV zFadeThreshMax = vForce_GroundZ_ZFadeThreshMin_ZFadeThreshMax.GetW();
				ScalarV heightZFade = (heightAboveGround-zFadeThreshMin) / (zFadeThreshMax-zFadeThreshMin);
				heightZFade = Saturate(heightZFade);
				vDir.SetZ(vDir.GetZ()*heightZFade);
				vDir = NormalizeSafe(vDir, Vec3V(V_X_AXIS_WZERO));
				ScalarV force = vForce_GroundZ_ZFadeThreshMin_ZFadeThreshMax.GetX();
				vVel += Scale(vDir, force*angle*distSqrRatio);
			}
		}

		return vVel;
	}


	///////////////////////////////////////////////////////////////////////////////
	//  DebugDraw
	///////////////////////////////////////////////////////////////////////////////

#if __BANK
	void			phWindDownwash::DebugDraw						()
	{

		grcDebugDraw::PartialSphere(GetPos(), vDir_Radius.GetWf(), vDir_Radius.GetXYZ(), PI*0.5f, Color32(0.0f, 1.0f, 0.0f, 1.0f), false);
	}
#endif // __BANK



	///////////////////////////////////////////////////////////////////////////////
	//  CLASS phWindDownwashGroup
	///////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////
	//  Constructor
	///////////////////////////////////////////////////////////////////////////////

					phWindDownwashGroup::phWindDownwashGroup		()											
	{
		m_disturbances.Reset();
	}

	///////////////////////////////////////////////////////////////////////////////
	//  Init
	///////////////////////////////////////////////////////////////////////////////

	void			phWindDownwashGroup::Init						(int groupSize)											
	{
		m_disturbances.Resize(groupSize);
	}

	///////////////////////////////////////////////////////////////////////////////
	//  Reset
	///////////////////////////////////////////////////////////////////////////////

	void			phWindDownwashGroup::Reset						()											
	{
		for(int i = 0; i < m_disturbances.GetCount(); i++)
		{
			m_disturbances[i].SetIsActive(false);
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	//  Update
	///////////////////////////////////////////////////////////////////////////////

	void			phWindDownwashGroup::Update						(float dt)
	{
		// remove processed downwashes from the array
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

	void			phWindDownwashGroup::Apply						(phWindField* RESTRICT pDisturbanceField, int batch)
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

	int 			phWindDownwashGroup::AddDisturbance				(const phWindDisturbanceBase* pDisturbance)
	{
		for (int i=0; i<m_disturbances.GetCount(); i++)
		{
			if (m_disturbances[i].GetIsActive()==false)
			{
				m_disturbances[i] = *(static_cast<const phWindDownwash*>(pDisturbance));
				m_disturbances[i].SetIsActive(true);
				return i;
			}
		}

		return -1;
	}

	///////////////////////////////////////////////////////////////////////////////
	//  RemoveDisturbance
	///////////////////////////////////////////////////////////////////////////////

	void			phWindDownwashGroup::RemoveDisturbance			(int index)
	{
		Assertf(index>=0 && index<m_disturbances.GetCount(), "Trying to remove a disturbance out of the array range");
		m_disturbances[index].SetIsActive(false);
	}

	///////////////////////////////////////////////////////////////////////////////
	//  GetNumActiveDisturbances
	///////////////////////////////////////////////////////////////////////////////

	int				phWindDownwashGroup::GetNumActiveDisturbances	()	
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

	void			phWindDownwashGroup::DebugDraw					()
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
