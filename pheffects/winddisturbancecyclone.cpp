// 
// pheffects/winddisturbancecyclone.cpp 
// 
// Copyright (C) 1999-2008 Rockstar Games.  All Rights Reserved. 
// 

///////////////////////////////////////////////////////////////////////////////
//  INCLUDES
///////////////////////////////////////////////////////////////////////////////

#include "winddisturbancecyclone.h"
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
	//  CLASS phWindCyclone
	///////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////
	//  Constructor
	///////////////////////////////////////////////////////////////////////////////

					phWindCyclone::phWindCyclone					()
	{
		m_vPos = Vec3V(V_ZERO);
		m_type = WIND_DIST_GLOBAL;

		m_settings.range = 0.0f;
		m_settings.maxStrength = 1.0f;
		m_settings.deltaStrength = 0.1f;
		m_settings.forceMult = 1.0f;
		m_settings.shrinkChance = 1.0f;

		m_currStrength = 0.0f;
		m_state = CYCLONE_STATE_GROWING;
	}


	///////////////////////////////////////////////////////////////////////////////
	//  Constructor
	///////////////////////////////////////////////////////////////////////////////

					phWindCyclone::phWindCyclone					(phWindDistType_e type, Vec3V_In vPos, const phCycloneSettings& settings)
	{
		m_vPos = vPos;
		m_type = type;
		m_settings = settings;

		m_currStrength = 0.0f;
		m_state = CYCLONE_STATE_GROWING;
	}


	///////////////////////////////////////////////////////////////////////////////
	//  Update
	///////////////////////////////////////////////////////////////////////////////

	bool			phWindCyclone::Update							(float dt)
	{
		// update differently depending on the state
		switch(m_state)
		{
		case CYCLONE_STATE_GROWING:
			{
				// the cyclone is growing - increase its strength
				m_currStrength = m_currStrength+(dt*m_settings.deltaStrength);

				// check if its reached its max strength yet
				if (m_currStrength > m_settings.maxStrength)
				{
					m_currStrength = m_settings.maxStrength;
					m_state = CYCLONE_STATE_ON;
				}

				break;
			}

		case CYCLONE_STATE_ON:
			{	
				// the cyclone is fully grown - randomly start to shrink it

				if (g_DrawRand.GetFloat()<m_settings.shrinkChance*dt)
				{
					m_state = CYCLONE_STATE_SHRINKING;
				}

				break;
			}

		case CYCLONE_STATE_SHRINKING:
			{
				// the cyclone is shrinking - decrease its strength
				m_currStrength = m_currStrength-(dt*m_settings.deltaStrength);

				// check if its finished yet
				if (m_currStrength<=0.0f)
				{
					return true;

				}

				break;
			}

		default:
			break;
		}

		return false;
	}
	
	///////////////////////////////////////////////////////////////////////////////
	//  DebugDraw
	///////////////////////////////////////////////////////////////////////////////

#if __BANK
	void			phWindCyclone::DebugDraw						()
	{
		// calc the centre pos
		Vec3V vCentrePos = m_vPos;
		vCentrePos.SetZf(phWindField::GetBasePosWldZ() + (0.5f*(WINDFIELD_NUM_ELEMENTS_Z*WINDFIELD_ELEMENT_SIZE_Z)));

		// scale the radius with the strength
		float currRadius = m_settings.range * (m_currStrength/m_settings.maxStrength);

		// draw the capsule representing the thermal
		float height = WINDFIELD_NUM_ELEMENTS_Z*WINDFIELD_ELEMENT_SIZE_Z;
		Vec3V vStartPos = vCentrePos;
		vStartPos.SetZf(vStartPos.GetZf()-(height*0.5f));
		Vec3V vEndPos = vCentrePos;
		vEndPos.SetZf(vEndPos.GetZf()+(height*0.5f));
		grcDebugDraw::Capsule(vStartPos, vEndPos, currRadius, Color32(1.0f, 0.0f, 0.0f, 1.0f), false);
	}
#endif // __BANK


	///////////////////////////////////////////////////////////////////////////////
	//  CLASS phWindCycloneGroup
	///////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////
	//  Constructor
	///////////////////////////////////////////////////////////////////////////////

	phWindCycloneGroup::phWindCycloneGroup				()											
	{
		m_autoData.type = WIND_DIST_GLOBAL;
		m_autoData.rangeMin = 5.0f;
		m_autoData.rangeMax = 15.0f;
		m_autoData.strengthMin = 0.5f;
		m_autoData.strengthMax = 3.0f;
		m_autoData.strengthDelta = 0.3f;
		m_autoData.forceMult = 10.0f;
		m_autoData.createChance = 0.3f;
		m_autoData.shrinkChance = 0.1f;
		m_disturbances.Reset();
	}

	///////////////////////////////////////////////////////////////////////////////
	//  Init
	///////////////////////////////////////////////////////////////////////////////

	void phWindCycloneGroup::Init					(int groupSize)											
	{
		m_disturbances.Resize(groupSize);
	}

	///////////////////////////////////////////////////////////////////////////////
	//  Init
	///////////////////////////////////////////////////////////////////////////////

	void phWindCycloneGroup::Reset					()											
	{
		for(int i = 0; i < m_disturbances.GetCount(); i++)
		{
			m_disturbances[i].SetIsActive(false);
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	//  Update
	///////////////////////////////////////////////////////////////////////////////

	void			phWindCycloneGroup::Update							(float dt)
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
					Vec3V pos = Vec3V(phWindField::GetBasePosWldX() + g_DrawRand.GetRanged(0.0f, (float)(WINDFIELD_NUM_ELEMENTS_X*WINDFIELD_ELEMENT_SIZE_X)), 
						phWindField::GetBasePosWldY() + g_DrawRand.GetRanged(0.0f, (float)(WINDFIELD_NUM_ELEMENTS_Y*WINDFIELD_ELEMENT_SIZE_Y)),
						phWindField::GetBasePosWldZ() + 0.0f);

					float range = g_DrawRand.GetRanged(m_autoData.rangeMin, m_autoData.rangeMax);
					float maxStrength = g_DrawRand.GetRanged(m_autoData.strengthMin, m_autoData.strengthMax);

					phCycloneSettings cycSettings;
					cycSettings.range = range;
					cycSettings.maxStrength = maxStrength;
					cycSettings.deltaStrength = m_autoData.strengthDelta;
					cycSettings.forceMult = m_autoData.forceMult;
					cycSettings.shrinkChance = m_autoData.shrinkChance;

					phWindCyclone cyc(m_autoData.type, pos, cycSettings);
					this->AddDisturbance(&cyc);
				}
			}
		}

		// update and possibly remove cyclones
		for (int i=0; i<m_disturbances.GetCount(); i++)
		{
			if (this->m_disturbances[i].GetIsActive())
			{
				if (this->m_disturbances[i].Update(dt))
				{
					// cyclone is finished - remove
					this->m_disturbances[i].SetIsActive(false);
				}
			}
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	//  Apply
	///////////////////////////////////////////////////////////////////////////////

	void			phWindCycloneGroup::Apply							(phWindField* RESTRICT UNUSED_PARAM(pDisturbanceField), int UNUSED_PARAM(batch))
	{
	}


	///////////////////////////////////////////////////////////////////////////////
	//  GetVelocity
	///////////////////////////////////////////////////////////////////////////////

	Vec3V_Out		phWindCycloneGroup::GetVelocity						(Vec3V_In vPos, ScalarV_In windSpeed, ScalarV_In waterSpeed)
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

	///////////////////////////////////////////////////////////////////////////////
	//  AddDisturbance
	///////////////////////////////////////////////////////////////////////////////

	int 			phWindCycloneGroup::AddDisturbance					(const phWindDisturbanceBase* pDisturbance)
	{
		for (int i=0; i<m_disturbances.GetCount(); i++)
		{
			if (m_disturbances[i].GetIsActive()==false)
			{
				m_disturbances[i] = *(static_cast<const phWindCyclone*>(pDisturbance));
				m_disturbances[i].SetIsActive(true);
				return i;
			}
		}

		return -1;
	}

	///////////////////////////////////////////////////////////////////////////////
	//  RemoveDisturbance
	///////////////////////////////////////////////////////////////////////////////

	void			phWindCycloneGroup::RemoveDisturbance				(int index)
	{
		Assertf(index>=0 && index<m_disturbances.GetCount(), "Trying to remove a disturbance out of the array range");
		m_disturbances[index].SetIsActive(false);
	}

	///////////////////////////////////////////////////////////////////////////////
	//  GetNumActiveDisturbances
	///////////////////////////////////////////////////////////////////////////////
	int				phWindCycloneGroup::GetNumActiveDisturbances			()	
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

	void			phWindCycloneGroup::DebugDraw						()
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
