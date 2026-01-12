// 
// pheffects/winddisturbancesphere.cpp 
// 
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved. 
// 

///////////////////////////////////////////////////////////////////////////////
//  INCLUDES
///////////////////////////////////////////////////////////////////////////////

#include "winddisturbancesphere.h"
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
	//  CLASS phWindSphere
	///////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////
	//  Constructor
	///////////////////////////////////////////////////////////////////////////////

	phWindSphere::phWindSphere						()
	{
		m_vPos = Vec3V(V_ZERO);
		m_type = WIND_DIST_GLOBAL;

		m_ForceMult_Radius = Vec2V(V_X_AXIS_WZERO);
		m_vVelocity.ZeroComponents();

		m_processed = false;
	}


	///////////////////////////////////////////////////////////////////////////////
	//  Constructor
	///////////////////////////////////////////////////////////////////////////////

	phWindSphere::phWindSphere						(phWindDistType_e type, Vec3V_In pos, const phSphereSettings& settings)
	{
		m_vPos = pos;
		m_type = type;
		m_vVelocity = settings.vVelocity;
		m_ForceMult_Radius = Vec2V(settings.forceMult, settings.radius);

		m_processed = false;
	}


	///////////////////////////////////////////////////////////////////////////////
	//  Update
	///////////////////////////////////////////////////////////////////////////////

	bool			phWindSphere::Update							(float UNUSED_PARAM(dt))
	{
		return m_processed;
	}


	///////////////////////////////////////////////////////////////////////////////
	//  Apply
	///////////////////////////////////////////////////////////////////////////////

	void			phWindSphere::Apply							(phWindField* RESTRICT pDisturbanceField, u32 UNUSED_PARAM(startGridX), u32 UNUSED_PARAM(endGridX))
	{
		ScalarV radius = m_ForceMult_Radius.GetY();
		Vec3V vMin = GetPos() - Vec3V(radius);
		Vec3V vMax = GetPos() + Vec3V(radius);

		int worldMinX = ((int)vMin.GetXf()) / WINDFIELD_ELEMENT_SIZE_X * WINDFIELD_ELEMENT_SIZE_X;
		int worldMinY = ((int)vMin.GetYf()) / WINDFIELD_ELEMENT_SIZE_Y * WINDFIELD_ELEMENT_SIZE_Y;
		int worldMinZ = ((int)vMin.GetZf()) / WINDFIELD_ELEMENT_SIZE_Z * WINDFIELD_ELEMENT_SIZE_Z;

		int worldMaxX = ((int)vMax.GetXf()) / WINDFIELD_ELEMENT_SIZE_X * WINDFIELD_ELEMENT_SIZE_X;
		int worldMaxY = ((int)vMax.GetYf()) / WINDFIELD_ELEMENT_SIZE_Y * WINDFIELD_ELEMENT_SIZE_Y;
		int worldMaxZ = ((int)vMax.GetZf()) / WINDFIELD_ELEMENT_SIZE_Z * WINDFIELD_ELEMENT_SIZE_Z;

		worldMinX = rage::Max(worldMinX, phWindField::GetBasePosWldX());
		worldMinY = rage::Max(worldMinY, phWindField::GetBasePosWldY());
		worldMinZ = rage::Max(worldMinZ, phWindField::GetBasePosWldZ());

		worldMaxX = rage::Min(worldMaxX, phWindField::GetBasePosWldX() + WINDFIELD_ELEMENT_SIZE_X*(WINDFIELD_NUM_ELEMENTS_X-1));
		worldMaxY = rage::Min(worldMaxY, phWindField::GetBasePosWldY() + WINDFIELD_ELEMENT_SIZE_Y*(WINDFIELD_NUM_ELEMENTS_Y-1));
		worldMaxZ = rage::Min(worldMaxZ, phWindField::GetBasePosWldZ() + WINDFIELD_ELEMENT_SIZE_Z*(WINDFIELD_NUM_ELEMENTS_Z-1));

		// adjust the z to take water into account
		int worldWaterZ = (int)phWindField::GetWaterLevelWld();
		worldWaterZ = Clamp(worldWaterZ, worldMinZ, worldMaxZ);

		if (m_type==WIND_DIST_AIR)
		{
			worldMinZ = rage::Max(worldMinZ, worldWaterZ);
			worldMaxZ = rage::Max(worldMaxZ, worldWaterZ);
		} 
		else if (m_type==WIND_DIST_WATER)
		{
			worldMinZ = rage::Min(worldMinZ, worldWaterZ);
			worldMaxZ = rage::Min(worldMaxZ, worldWaterZ);
		}

		u32 gridX, gridY, gridZ;
		
		// pre-calc vectorised pos and increments
		const float fWorldMinX = (float)worldMinX;
		const float fWorldMinY = (float)worldMinY;
		const float fWorldMinZ = (float)worldMinZ;
		Vec3V vWorldPos = Vec3V(fWorldMinX, fWorldMinY, fWorldMinZ);
		const Vec3V vWorldIncrementX = Vec3V(V_X_AXIS_WZERO);
		const Vec3V vWorldIncrementY = Vec3V(V_Y_AXIS_WZERO);
		const Vec3V vWorldIncrementZ = Vec3V(V_Z_AXIS_WZERO);

		const Vec3V vSphereRadius = Vec3V(radius);

		for (int worldX=worldMinX; worldX<=worldMaxX; worldX++)
		{
			phWindField::CalcGridPosXFast(worldX, gridX);
			vWorldPos.SetYf(fWorldMinY);

			for (int worldY=worldMinY; worldY<=worldMaxY; worldY++)
			{
				phWindField::CalcGridPosYFast(worldY, gridY);
				vWorldPos.SetZf(fWorldMinZ);

				for (int worldZ=worldMinZ; worldZ<=worldMaxZ; worldZ++)
				{
					phWindField::CalcGridPosZFast(worldZ, gridZ);

					// calc attenuation
					Vec3V vTmpAttenuation = GetPos() - vWorldPos;
					vTmpAttenuation = vTmpAttenuation/vSphereRadius;

					ScalarV vAtt = Dot(vTmpAttenuation,vTmpAttenuation);
					vAtt = ScalarV(V_ONE) - vAtt;
					vAtt = Saturate(vAtt);

					Vec3V vVel(vAtt);
					vVel = vVel*m_vVelocity;

					// apply the disturbance to the field
					// TODO: this is slow as we convert the current fixed point velocity to a float, add the vector to it and then convert back to an int
					//       could the entire loop be made fixed point?
					pDisturbanceField->AddVelocity(gridX, gridY, gridZ, vVel);
					vWorldPos = vWorldPos + vWorldIncrementZ;
				}
				vWorldPos = vWorldPos + vWorldIncrementY;
			}
			vWorldPos = vWorldPos + vWorldIncrementX;
		}

		// this box has now been applied
		m_processed = true;
	}

	///////////////////////////////////////////////////////////////////////////////
	//  DebugDraw
	///////////////////////////////////////////////////////////////////////////////

#if __BANK
	void			phWindSphere::DebugDraw							()
	{
		grcDebugDraw::Sphere(GetPos(), m_ForceMult_Radius.GetYf(), Color32(0.0f, 1.0f, 0.0f, 1.0f), false);
	}
#endif // __BANK



	///////////////////////////////////////////////////////////////////////////////
	//  CLASS phWindSphereGroup
	///////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////
	//  Constructor
	///////////////////////////////////////////////////////////////////////////////

	phWindSphereGroup::phWindSphereGroup					()											
	{
		m_disturbances.Reset();
	}

	///////////////////////////////////////////////////////////////////////////////
	//  Init
	///////////////////////////////////////////////////////////////////////////////

	void phWindSphereGroup::Init					(int groupSize)											
	{
		m_disturbances.Resize(groupSize);
	}

	///////////////////////////////////////////////////////////////////////////////
	//  Reset
	///////////////////////////////////////////////////////////////////////////////

	void phWindSphereGroup::Reset					()											
	{
		for(int i = 0; i < m_disturbances.GetCount(); i++)
		{
			m_disturbances[i].SetIsActive(false);
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	//  Update
	///////////////////////////////////////////////////////////////////////////////

	void			phWindSphereGroup::Update							(float dt)
	{
		// remove processed boxes from the array
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

	void			phWindSphereGroup::Apply								(phWindField* RESTRICT pDisturbanceField, int batch)
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

	int 			phWindSphereGroup::AddDisturbance				(const phWindDisturbanceBase* pDisturbance)
	{
		for (int i=0; i<m_disturbances.GetCount(); i++)
		{
			if (m_disturbances[i].GetIsActive()==false)
			{
				m_disturbances[i] = *(static_cast<const phWindSphere*>(pDisturbance));
				m_disturbances[i].SetIsActive(true);
				return i;
			}
		}

		return -1;
	}

	///////////////////////////////////////////////////////////////////////////////
	//  RemoveDisturbance
	///////////////////////////////////////////////////////////////////////////////

	void			phWindSphereGroup::RemoveDisturbance			(int index)
	{
		Assertf(index>=0 && index<m_disturbances.GetCount(), "Trying to remove a disturbance out of the array range");
		m_disturbances[index].SetIsActive(false);
	}

	///////////////////////////////////////////////////////////////////////////////
	//  GetNumActiveDisturbances
	///////////////////////////////////////////////////////////////////////////////
	int				phWindSphereGroup::GetNumActiveDisturbances			()	
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

	void			phWindSphereGroup::DebugDraw					()
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