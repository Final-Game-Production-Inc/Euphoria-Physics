// 
// pheffects/winddisturbancethermal.cpp 
// 
// Copyright (C) 1999-2008 Rockstar Games.  All Rights Reserved. 
// 

///////////////////////////////////////////////////////////////////////////////
//  INCLUDES
///////////////////////////////////////////////////////////////////////////////

#include "winddisturbancethermal.h"
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
	//  CLASS phWindThermal
	///////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////
	//  Constructor
	///////////////////////////////////////////////////////////////////////////////

	phWindThermal::phWindThermal					()
	{
		m_vPos = Vec3V(V_ZERO);
		m_type = WIND_DIST_GLOBAL;

		m_Height_CurrRadiusSqr_CurrStrength = Vec3V(10.0f, 0.0f, 0.0f);
		m_radius = 3.0f;
		m_maxStrength = 1.0f;
		m_deltaStrength = 0.1f;
		m_shrinkChance = 1.0f;
		m_state = THERMAL_STATE_GROWING;
	}


	///////////////////////////////////////////////////////////////////////////////
	//  Constructor
	///////////////////////////////////////////////////////////////////////////////

	phWindThermal::phWindThermal					(phWindDistType_e type, Vec3V_In pos, const phThermalSettings& settings)
	{
		m_vPos = pos;
		m_type = type;
		m_Height_CurrRadiusSqr_CurrStrength.SetXf(settings.height);
		m_radius = settings.radius;
		m_maxStrength = settings.maxStrength;
		m_deltaStrength = settings.deltaStrength;
		m_shrinkChance = settings.shrinkChance;

		// if the delta strength is zero then go straight to max strength
		if (m_deltaStrength==0.0f)
		{
			m_Height_CurrRadiusSqr_CurrStrength.SetZf(settings.maxStrength);
			m_state = THERMAL_STATE_ON;
		}
		// otherwise start off at zero and grow up to the max strength
		else
		{
			m_Height_CurrRadiusSqr_CurrStrength.SetZ(ScalarV(V_ZERO));
			m_state = THERMAL_STATE_GROWING;
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	//  Update
	///////////////////////////////////////////////////////////////////////////////

	bool			phWindThermal::Update							(float dt)
	{
		// update differently depending on the state
		switch(m_state)
		{
		case THERMAL_STATE_GROWING:
			{
				// the thermal is growing - increase its strength
				SetCurrStrength( GetCurrStrength() +(dt*m_deltaStrength));

				// check if its reached its max strength yet
				if (GetCurrStrength() > m_maxStrength)
				{
					SetCurrStrength(m_maxStrength);
					m_state = THERMAL_STATE_ON;
				}

				break;
			}

		case THERMAL_STATE_ON:
			{	
				// the thermal is fully grown - randomly start to shrink it
				if (g_DrawRand.GetFloat()<m_shrinkChance*dt)
				{
					// if the max strength is set to zero then turn off instantly
					if (m_deltaStrength==0.0f)
					{
						SetCurrStrength( 0.0f);
						m_state = THERMAL_STATE_OFF;
						return true;
					}
					// otherwise start shrinking
					else
					{
						m_state = THERMAL_STATE_SHRINKING;
					}
				}

				break;
			}

		case THERMAL_STATE_SHRINKING:
			{
				// the thermal is shrinking - decrease its strength
				SetCurrStrength( GetCurrStrength()-(dt*m_deltaStrength));

				// check if its finished yet
				if (GetCurrStrength()<=0.0f)
				{
					m_state = THERMAL_STATE_OFF;
					return true;
				}

				break;
			}

		default:
			break;
		}

		float currRadius = m_radius * (GetCurrStrength()/m_maxStrength);
		m_Height_CurrRadiusSqr_CurrStrength.SetYf(currRadius*currRadius);

		return false;
	}


	///////////////////////////////////////////////////////////////////////////////
	//  DebugDraw
	///////////////////////////////////////////////////////////////////////////////

#if __BANK
	void			phWindThermal::DebugDraw						()
	{

		float height = m_Height_CurrRadiusSqr_CurrStrength.GetXf();

		// calc the centre pos
		Vec3V vCentrePos = m_vPos;
		vCentrePos.SetZf(vCentrePos.GetZf() + height * 0.5f);

		// draw the capsule representing the thermal
		Color32 col;
		if (m_isCulled)
		{
			col = Color32(1.0f, 0.0f, 0.0f, 1.0f);
		}
		else
		{
			col = Color32(1.0f, 1.0f, 0.0f, 1.0f);
		}

		Vec3V vStartPos = vCentrePos;
		vStartPos.SetZf(vStartPos.GetZf()-(height*0.5f));
		Vec3V vEndPos = vCentrePos;
		vEndPos.SetZf(vEndPos.GetZf()+(height*0.5f));
		grcDebugDraw::Capsule(vStartPos, vEndPos, sqrt(m_Height_CurrRadiusSqr_CurrStrength.GetYf()), col, false);
	}
#endif // __BANK


	///////////////////////////////////////////////////////////////////////////////
	//  GetVelocity
	///////////////////////////////////////////////////////////////////////////////

	Vec3V_Out			phWindThermal::GetVelocity						(Vec3V_In vPos, ScalarV_In UNUSED_PARAM(windSpeed), ScalarV_In UNUSED_PARAM(waterSpeed))
	{
		Vec3V vVel(V_ZERO);

		ScalarV height = m_Height_CurrRadiusSqr_CurrStrength.GetX();
		ScalarV currRadiusSqr = m_Height_CurrRadiusSqr_CurrStrength.GetY();
		ScalarV currStrength = m_Height_CurrRadiusSqr_CurrStrength.GetZ();

		// check if the we're in the height range of this thermal cylinder
		if (IsGreaterThanAll(vPos.GetZ(), m_vPos.GetZ()) &&
			IsLessThanAll(vPos.GetZ(), m_vPos.GetZ() + height))
		{
			// check if we're within the radius of this thermal cylinder
			ScalarV vecX = vPos.GetX()-m_vPos.GetX();
			ScalarV vecY = vPos.GetY()-m_vPos.GetY();
			ScalarV distSqr = vecX*vecX + vecY*vecY;
			if (IsLessThanAll(distSqr, currRadiusSqr))
			{
				ScalarV ratio = distSqr/currRadiusSqr;
				vVel.SetZ(AddScaled(vVel.GetZ(), ratio, currStrength));
			}
		}

		return vVel;
	}


	///////////////////////////////////////////////////////////////////////////////
	//  CLASS phWindThermalGroup
	///////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////
	//  Constructor
	///////////////////////////////////////////////////////////////////////////////

					phWindThermalGroup::phWindThermalGroup			()											
	{
		m_autoData.type = WIND_DIST_GLOBAL;

		m_autoData.radiusMin = 3.0f;
		m_autoData.radiusMax = 5.0f;
		m_autoData.heightMin = 10.0f;
		m_autoData.heightMax = 15.0f;
		m_autoData.strengthMin = 0.5f;
		m_autoData.strengthMax = 3.0f;
		m_autoData.strengthDelta = 0.3f;

		m_autoData.createChance = 0.3f;
		m_autoData.shrinkChance = 0.1f;
		m_disturbances.Reset();
	}

	///////////////////////////////////////////////////////////////////////////////
	//  Init
	///////////////////////////////////////////////////////////////////////////////

	void			phWindThermalGroup::Init					(int groupSize)											
	{
		m_disturbances.Resize(groupSize);
	}

	///////////////////////////////////////////////////////////////////////////////
	//  Reset
	///////////////////////////////////////////////////////////////////////////////

	void			phWindThermalGroup::Reset					()											
	{
		for(int i = 0; i < m_disturbances.GetCount(); i++)
		{
			m_disturbances[i].SetIsActive(false);
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	//  Update
	///////////////////////////////////////////////////////////////////////////////

	void			phWindThermalGroup::Update							(float dt)
	{
		// create new disturbances randomly if the automated flag is set
		if (this->m_automated)
		{
			// check if there is room to create a new disturbance
			if (this->GetNumActiveDisturbances()<m_disturbances.GetCapacity())
			{
				// check if we should create a new disturbance
				if (g_DrawRand.GetFloat()<m_autoData.createChance*dt)	
				{
					float radius = g_DrawRand.GetRanged(m_autoData.radiusMin, m_autoData.radiusMax);
					float height = g_DrawRand.GetRanged(m_autoData.heightMin, m_autoData.heightMax);
					float maxStrength = g_DrawRand.GetRanged(m_autoData.strengthMin, m_autoData.strengthMax);

					Vec3V pos = Vec3V(phWindField::GetBasePosWldX() + g_DrawRand.GetRanged(0.0f, (float)(WINDFIELD_NUM_ELEMENTS_X*WINDFIELD_ELEMENT_SIZE_X)), 
						phWindField::GetBasePosWldY() + g_DrawRand.GetRanged(0.0f, (float)(WINDFIELD_NUM_ELEMENTS_Y*WINDFIELD_ELEMENT_SIZE_Y)),
						phWindField::GetBasePosWldZ() + g_DrawRand.GetRanged(0.0f, (float)(WINDFIELD_NUM_ELEMENTS_Z*WINDFIELD_ELEMENT_SIZE_Z))-height*0.5f);

					phThermalSettings thermalSettings;
					thermalSettings.radius = radius;
					thermalSettings.height = height;
					thermalSettings.maxStrength = maxStrength;
					thermalSettings.deltaStrength = m_autoData.strengthDelta;

					thermalSettings.shrinkChance = m_autoData.shrinkChance;

					phWindThermal thermal(m_autoData.type, pos, thermalSettings);
					this->AddDisturbance(&thermal);
				}
			}
		}

		// update and possibly remove thermals (and set some cull info)
		int numNonCulled = 0;
		for (int i=0; i<m_disturbances.GetCount(); i++)
		{
			this->m_disturbances[i].SetIsCulled(true);

			if (this->m_disturbances[i].GetIsActive())
			{
				if (this->m_disturbances[i].Update(dt))
				{
					// thermal is finished - remove
					this->m_disturbances[i].SetIsActive(false);
				}
				else
				{
					// still active - set to not be culled for the moment - this could be updated shortly
					this->m_disturbances[i].SetIsCulled(false);
					numNonCulled++;
				}
			}
		}

		// set the cull info
		const int MAX_DISTURBANCES = 64;
		float distsSqr[MAX_DISTURBANCES];
		Assert(m_disturbances.GetCount() <= MAX_DISTURBANCES);

		if (this->m_cullRange>0.0f)
		{
			Vec3V vFocalPos = Vec3V(phWindField::GetBasePosWldX() + 0.5f*(float)(WINDFIELD_NUM_ELEMENTS_X*WINDFIELD_ELEMENT_SIZE_X), 
				phWindField::GetBasePosWldY() + 0.5f*(float)(WINDFIELD_NUM_ELEMENTS_Y*WINDFIELD_ELEMENT_SIZE_Y),
				phWindField::GetBasePosWldZ() + 0.5f*(float)(WINDFIELD_NUM_ELEMENTS_Z*WINDFIELD_ELEMENT_SIZE_Z));

			for (int i=0; i<m_disturbances.GetCount(); i++)
			{
				if (this->m_disturbances[i].GetIsActive())
				{
					Vec3V vThermalPos = this->m_disturbances[i].GetPos();
					Vec3V vThermalPosTop = vThermalPos;
					vThermalPosTop.SetZf(vThermalPosTop.GetZf() + this->m_disturbances[i].GetHeight());
					distsSqr[i] = phWindDisturbanceHelper::DistSqrFromPointToLineSegment(vFocalPos, vThermalPos, vThermalPosTop);

					if (distsSqr[i]>this->m_cullRange*this->m_cullRange)
					{
						this->m_disturbances[i].SetIsCulled(true);
						numNonCulled--;
					}
				}
			}
		}

		if (this->m_cullMax>0 && numNonCulled>this->m_cullMax)
		{
			// too many non culled disturbances - remove the furthest one each time (not optimal but just getting working for now)
			int numToCull = numNonCulled-this->m_cullMax;
			for (int i=0; i<numToCull; i++)
			{
				float maxDistSqr = 0.0f;
				int cullId = 0;

				for (int j=0; j<m_disturbances.GetCount(); j++)
				{
					if (this->m_disturbances[j].GetIsCulled()==false && distsSqr[j]>maxDistSqr)
					{
						maxDistSqr = distsSqr[j];
						cullId = j;
					}
				}

				// we need to cull the thermal at j this pass
				this->m_disturbances[cullId].SetIsCulled(true);
			}
		}
	}


	///////////////////////////////////////////////////////////////////////////////
	//  Apply
	///////////////////////////////////////////////////////////////////////////////

	void			phWindThermalGroup::Apply							(phWindField* RESTRICT UNUSED_PARAM(pDisturbanceField), int UNUSED_PARAM(batch))
	{
	}

	///////////////////////////////////////////////////////////////////////////////
	//  AddDisturbance
	///////////////////////////////////////////////////////////////////////////////

	int 			phWindThermalGroup::AddDisturbance				(const phWindDisturbanceBase* pDisturbance)
	{
		for (int i=0; i<m_disturbances.GetCount(); i++)
		{
			if (m_disturbances[i].GetIsActive()==false)
			{
				m_disturbances[i] = *(static_cast<const phWindThermal*>(pDisturbance));
				m_disturbances[i].SetIsActive(true);
				return i;
			}
		}

		return -1;
	}

	///////////////////////////////////////////////////////////////////////////////
	//  RemoveDisturbance
	///////////////////////////////////////////////////////////////////////////////

	void			phWindThermalGroup::RemoveDisturbance			(int index)
	{
		Assertf(index>=0 && index<m_disturbances.GetCount(), "Trying to remove a disturbance out of the array range");
		m_disturbances[index].SetIsActive(false);
	}

	///////////////////////////////////////////////////////////////////////////////
	//  GetNumActiveDisturbances
	///////////////////////////////////////////////////////////////////////////////

	int				phWindThermalGroup::GetNumActiveDisturbances			()	
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

	void			phWindThermalGroup::DebugDraw						()
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
