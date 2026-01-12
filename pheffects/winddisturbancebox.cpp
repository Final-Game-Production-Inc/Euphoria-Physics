// 
// pheffects/winddisturbancebox.cpp 
// 
// Copyright (C) 1999-2008 Rockstar Games.  All Rights Reserved. 
// 

///////////////////////////////////////////////////////////////////////////////
//  INCLUDES
///////////////////////////////////////////////////////////////////////////////

#include "winddisturbancebox.h"
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
	//  CLASS phWindBox
	///////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////
	//  Constructor
	///////////////////////////////////////////////////////////////////////////////

					phWindBox::phWindBox						()
	{
		m_vPos = Vec3V(V_ZERO);
		m_type = WIND_DIST_GLOBAL;

		m_settings.forceMult = 1.0f;
		m_settings.vDimensions = Vec3V(V_ZERO);
		m_settings.vVelocity = Vec3V(V_ZERO);
		m_settings.vDirForward = Vec3V(V_ZERO);
		m_settings.vDirRight = Vec3V(V_ZERO);
		m_settings.vDirUp = Vec3V(V_ZERO);
		m_settings.useDir = false;

		m_processed = false;
	}


	///////////////////////////////////////////////////////////////////////////////
	//  Constructor
	///////////////////////////////////////////////////////////////////////////////

					phWindBox::phWindBox						(phWindDistType_e type, Vec3V_In vPos, const phBoxSettings& settings)
	{
		m_vPos = vPos;
		m_type = type;
		m_settings = settings;

		m_processed = false;
	}


	///////////////////////////////////////////////////////////////////////////////
	//  Update
	///////////////////////////////////////////////////////////////////////////////

	bool			phWindBox::Update							(float UNUSED_PARAM(dt))
	{
		return m_processed;
	}


	///////////////////////////////////////////////////////////////////////////////
	//  Apply
	///////////////////////////////////////////////////////////////////////////////

	void			phWindBox::Apply							(phWindField* RESTRICT pDisturbanceField, u32 UNUSED_PARAM(startGridX), u32 UNUSED_PARAM(endGridX))
	{
		Vec3V vMin, vMax;
		if (m_settings.useDir==false)
		{
			// don't use the matrix for orientation - presume it's axis aligned
			vMin = GetPos() - m_settings.vDimensions;
			vMax = GetPos() + m_settings.vDimensions;
		}
		else
		{
			// use the matrix for orientation - i.e. non axis aligned

			// calc the base pos of the box - the corner in all negative directions
			Vec3V vBasePos = m_vPos;
			vBasePos -= m_settings.vDirRight   * m_settings.vDimensions.GetX() * ScalarV(V_HALF);
			vBasePos -= m_settings.vDirForward * m_settings.vDimensions.GetY() * ScalarV(V_HALF);
			vBasePos -= m_settings.vDirUp      * m_settings.vDimensions.GetZ() * ScalarV(V_HALF);

			// calc the vectors that travel along the edges of the box
			Vec3V vVecX = m_settings.vDirRight   * m_settings.vDimensions.GetX();
			Vec3V vVecY = m_settings.vDirForward * m_settings.vDimensions.GetY();
			Vec3V vVecZ = m_settings.vDirUp      * m_settings.vDimensions.GetZ();

			// calc the axis aligned box around this box
			vMin = vBasePos;
			vMax = vBasePos;

			vMin += Min(Vec3V(V_ZERO), vVecX);
			vMin += Min(Vec3V(V_ZERO), vVecY);
			vMin += Min(Vec3V(V_ZERO), vVecZ);

			vMax += Max(Vec3V(V_ZERO), vVecX);
			vMax += Max(Vec3V(V_ZERO), vVecY);
			vMax += Max(Vec3V(V_ZERO), vVecZ);
		}

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
		for (int worldX=worldMinX; worldX<=worldMaxX; worldX++)
		{
			phWindField::CalcGridPosXFast(worldX, gridX);

			for (int worldY=worldMinY; worldY<=worldMaxY; worldY++)
			{
				phWindField::CalcGridPosYFast(worldY, gridY);

				for (int worldZ=worldMinZ; worldZ<=worldMaxZ; worldZ++)
				{
					// half the wind disturbance on the edge of the box
					Vec3V vMult(V_ONE);
					if (worldX == worldMinX || worldX == worldMaxX ||
						worldY == worldMinY || worldY == worldMaxY ||
						worldZ == worldMinZ || worldZ == worldMaxZ)
					{
						vMult = Vec3V(V_HALF);
					}

					// calc the grid position
					phWindField::CalcGridPosZFast(worldZ, gridZ);

					// apply the disturbance to the field
					pDisturbanceField->AddVelocity(gridX, gridY, gridZ, vMult*m_settings.vVelocity);
				}
			}
		}

		// this box has now been applied
		m_processed = true;
	}

	///////////////////////////////////////////////////////////////////////////////
	//  DebugDraw
	///////////////////////////////////////////////////////////////////////////////

#if __BANK
	void			phWindBox::DebugDraw							()
	{
		// calc the min pos of the box
		Vec3V vMinPos = GetPos();
		vMinPos -= m_settings.vDirRight   * m_settings.vDimensions.GetX() * ScalarV(V_HALF);
		vMinPos -= m_settings.vDirForward * m_settings.vDimensions.GetY() * ScalarV(V_HALF);
		vMinPos -= m_settings.vDirUp      * m_settings.vDimensions.GetZ() * ScalarV(V_HALF);

		// calc the max pos of the box
		Vec3V vMaxPos = GetPos();
		vMaxPos += m_settings.vDirRight   * m_settings.vDimensions.GetX() * ScalarV(V_HALF);
		vMaxPos += m_settings.vDirForward * m_settings.vDimensions.GetY() * ScalarV(V_HALF);
		vMaxPos += m_settings.vDirUp      * m_settings.vDimensions.GetZ() * ScalarV(V_HALF);

		// draw the capsule representing the thermal
		grcDebugDraw::BoxAxisAligned(vMinPos, vMaxPos, Color32(0.0f, 1.0f, 0.0f, 1.0f), false);
	}
#endif // __BANK



	///////////////////////////////////////////////////////////////////////////////
	//  CLASS phWindBoxGroup
	///////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////
	//  Constructor
	///////////////////////////////////////////////////////////////////////////////

	phWindBoxGroup::phWindBoxGroup					()											
	{
		m_disturbances.Reset();
	}

	///////////////////////////////////////////////////////////////////////////////
	//  Init
	///////////////////////////////////////////////////////////////////////////////

	void phWindBoxGroup::Init					(int groupSize)											
	{
		m_disturbances.Resize(groupSize);
	}

	///////////////////////////////////////////////////////////////////////////////
	//  Update
	///////////////////////////////////////////////////////////////////////////////

	void			phWindBoxGroup::Update							(float dt)
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

	void			phWindBoxGroup::Apply								(phWindField* RESTRICT pDisturbanceField, int batch)
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

	int 			phWindBoxGroup::AddDisturbance				(const phWindDisturbanceBase* pDisturbance)
	{
		for (int i=0; i<m_disturbances.GetCount(); i++)
		{
			if (m_disturbances[i].GetIsActive()==false)
			{
				m_disturbances[i] = *(static_cast<const phWindBox*>(pDisturbance));
				m_disturbances[i].SetIsActive(true);
				return i;
			}
		}

		return -1;
	}

	///////////////////////////////////////////////////////////////////////////////
	//  RemoveDisturbance
	///////////////////////////////////////////////////////////////////////////////

	void			phWindBoxGroup::RemoveDisturbance			(int index)
	{
		Assertf(index>=0 && index<m_disturbances.GetCount(), "Trying to remove a disturbance out of the array range");
		m_disturbances[index].SetIsActive(false);
	}

	///////////////////////////////////////////////////////////////////////////////
	//  GetNumActiveDisturbances
	///////////////////////////////////////////////////////////////////////////////
	int				phWindBoxGroup::GetNumActiveDisturbances			()	
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

	void			phWindBoxGroup::DebugDraw					()
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
