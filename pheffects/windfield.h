// 
// pheffects/windfield.h 
// 
// Copyright (C) 1999-2008 Rockstar Games.  All Rights Reserved. 
// 

#ifndef PHEFFECTS_WINDFIELD_H 
#define PHEFFECTS_WINDFIELD_H 


///////////////////////////////////////////////////////////////////////////////
//  INCLUDES
///////////////////////////////////////////////////////////////////////////////	

#include "vector/vector3.h"
#include "vectormath/vec3v.h"
#include "vectormath/classes.h"
#include "vectormath/vectormath.h"


///////////////////////////////////////////////////////////////////////////////
//  DEFINES
///////////////////////////////////////////////////////////////////////////////	

#define WIND_NUM_BATCHES				(8)

// Z-up
#define WINDFIELD_NUM_ELEMENTS_X		(32)
#define WINDFIELD_NUM_ELEMENTS_Y		(32)
#define WINDFIELD_NUM_ELEMENTS_Z		(8)
// Y-up (RAGE sample_wind)
// #define WINDFIELD_NUM_ELEMENTS_X		(32)
// #define WINDFIELD_NUM_ELEMENTS_Y		(8)
// #define WINDFIELD_NUM_ELEMENTS_Z		(32)

#define WINDFIELD_ELEMENT_SIZE_X		(1)
#define WINDFIELD_ELEMENT_SIZE_Y		(1)
#define WINDFIELD_ELEMENT_SIZE_Z		(1)


namespace rage
{
	///////////////////////////////////////////////////////////////////////////////
	//  STRUCTURES
	///////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////
	//  phWindPoint
	///////////////////////////////////////////////////////////////////////////////

	struct phWindPoint
	{
		// functions
		Vec3V_Out GetVelocity() const
		{
 #if __PPU || __XENON

			//unpack s16 to s32 by sticking zeros before s16
			// divide by 256 (by doing IntToFloatRaw<8> - which automatically divides by pow(2,8))
			// and then convert to float
			// Using Load Unaligned Safe
			return IntToFloatRaw<8>(Vec3V(Vec::V4UnpackLowSignedShort(Vec::V4LoadUnalignedSafe<6>(&m_velX))));
 #else
			const float s = 1.0f / 256.0f;
			return Vec3V(m_velX*s, m_velY*s, m_velZ*s);
#endif
		}

		void SetVelocity(int velX, int velY, int velZ)
		{
			m_velX = (s16)Clamp(velX * 256.0f, -32768.0f, 32767.0f);
			m_velY = (s16)Clamp(velY * 256.0f, -32768.0f, 32767.0f);
			m_velZ = (s16)Clamp(velZ * 256.0f, -32768.0f, 32767.0f);
		}

		void SetVelocity(Vec3V_In vVel)
		{
			m_velX = (s16)Clamp(vVel.GetXf() * 256.0f, -32768.0f, 32767.0f);
			m_velY = (s16)Clamp(vVel.GetYf() * 256.0f, -32768.0f, 32767.0f);
			m_velZ = (s16)Clamp(vVel.GetZf() * 256.0f, -32768.0f, 32767.0f);
		}

		void Multiply(float f)
		{
			int m = (int)(f*256.0f);
			m_velX = (s16)((m_velX * m) >> 8);
			m_velZ = (s16)((m_velZ * m) >> 8);
			m_velY = (s16)((m_velY * m) >> 8);
		}

		void MultiplyFixed(int m)
		{
			m_velX = (s16)((m_velX * m) >> 8);
			m_velZ = (s16)((m_velZ * m) >> 8);
			m_velY = (s16)((m_velY * m) >> 8);
		}

		s16* GetBase() 
		{
			return &m_velX;
		}

		s16 GetVelX() {return m_velX;}
		s16 GetVelY() {return m_velY;}
		s16 GetVelZ() {return m_velZ;}

	private:

		// variables
		s16 m_velX;
		s16 m_velY;
		s16 m_velZ;


	};


	///////////////////////////////////////////////////////////////////////////////
	//  phWindField
	///////////////////////////////////////////////////////////////////////////////

	struct phWindField
	{
		// functions
		void			Init();

		// static functions
static	void			CalcBasePos							(Vec3V_In vFocalPtWld);

static	void			CalcGridPosFast						(int worldX, int worldY, int worldZ, u32& gridX, u32& gridY, u32& gridZ);
static	bool			CalcGridPos							(int worldX, int worldY, int worldZ, u32& gridX, u32& gridY, u32& gridZ);

static	void			CalcGridPosXFast					(int worldX, u32& gridX);
static	void			CalcGridPosYFast					(int worldY, u32& gridY);
static	void			CalcGridPosZFast					(int worldZ, u32& gridZ);

static	void			CalcWorldPosX						(u32 gridX, int& worldX);
static	void			CalcWorldPosY						(u32 gridY, int& worldY);
static	void			CalcWorldPosZ						(u32 gridZ, int& worldZ);

		// accessors
inline	phWindPoint&	GetDataBase							()														{return m_data[0][0][0];}
inline	phWindPoint&	GetData								(int x, int y, int z)									{return m_data[x][y][z];}
inline	unsigned int	GetWidth							()	const												{return WINDFIELD_NUM_ELEMENTS_X;}
inline	unsigned int	GetHeight							()	const												{return WINDFIELD_NUM_ELEMENTS_Y;}
inline	unsigned int	GetDepth							()	const												{return WINDFIELD_NUM_ELEMENTS_Z;}

inline	Vec3V_Out		GetVelocity							(u32 x, u32 y, u32 z)									{return m_data[x][y][z].GetVelocity();}
inline	void			SetVelocity							(u32 x, u32 y, u32 z, Vec3V_In vVel)					{m_data[x][y][z].SetVelocity(vVel);}
inline	void			AddVelocity							(u32 x, u32 y, u32 z, Vec3V_In vVel)					{m_data[x][y][z].SetVelocity(m_data[x][y][z].GetVelocity()+vVel);}
inline	void			MultiplyVelocity					(u32 x, u32 y, u32 z, float mult)						{m_data[x][y][z].Multiply(mult);}
inline	void			MultiplyVelocityFixed				(u32 x, u32 y, u32 z, int mult)							{m_data[x][y][z].MultiplyFixed(mult);}

		// static accessors
static	int				GetBasePosWldX						()														{return m_basePosWldX;}
static	int				GetBasePosWldY						()														{return m_basePosWldY;}
static	int				GetBasePosWldZ						()														{return m_basePosWldZ;}
static	float			GetWaterLevelWld					()														{return m_waterLevelWld;}
static	void			SetWaterLevelWld					(float val)												{m_waterLevelWld = val;}


	private:

		// variables
		phWindPoint		m_data								[WINDFIELD_NUM_ELEMENTS_X][WINDFIELD_NUM_ELEMENTS_Y][WINDFIELD_NUM_ELEMENTS_Z];

static	int				m_basePosWldX;						// the negative-most position of the grid in world space
static	int				m_basePosWldY;						
static	int				m_basePosWldZ;						

static	float			m_waterLevelWld;


	};


	///////////////////////////////////////////////////////////////////////////////
	//  INLINE FUNCTIONS
	///////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////
	//  Init
	///////////////////////////////////////////////////////////////////////////////

	inline void				phWindField::Init						()
	{
		for (int x=0; x<WINDFIELD_NUM_ELEMENTS_X; x++)
		{
			for (int y=0; y<WINDFIELD_NUM_ELEMENTS_Y; y++)
			{
				for (int z=0; z<WINDFIELD_NUM_ELEMENTS_Z; z++)
				{
					m_data[x][y][z].SetVelocity(0, 0, 0);
				}
			}
		}
	}


	///////////////////////////////////////////////////////////////////////////////
	//  CalcBasePos
	///////////////////////////////////////////////////////////////////////////////

	inline void				phWindField::CalcBasePos				(Vec3V_In vFocalPtWld)
	{
		m_basePosWldX = ((int)vFocalPtWld.GetXf()) - (WINDFIELD_NUM_ELEMENTS_X * WINDFIELD_ELEMENT_SIZE_X / 2);
		m_basePosWldY = ((int)vFocalPtWld.GetYf()) - (WINDFIELD_NUM_ELEMENTS_Y * WINDFIELD_ELEMENT_SIZE_Y / 2);
		m_basePosWldZ = ((int)vFocalPtWld.GetZf()) - (WINDFIELD_NUM_ELEMENTS_Z * WINDFIELD_ELEMENT_SIZE_Z / 2);
	}


	///////////////////////////////////////////////////////////////////////////////
	//  CalcGridPosFast
	///////////////////////////////////////////////////////////////////////////////
	// - returns the indices in the grid from the coordinates on the map
	// - caller must know the world coordinates are in the grid area
	///////////////////////////////////////////////////////////////////////////////

	inline void				phWindField::CalcGridPosFast			(int worldX, int worldY, int worldZ, u32& gridX, u32& gridY, u32& gridZ)
	{
		Assert(worldX>=m_basePosWldX && worldX<m_basePosWldX+(WINDFIELD_NUM_ELEMENTS_X*WINDFIELD_ELEMENT_SIZE_X));
		Assert(worldY>=m_basePosWldY && worldY<m_basePosWldY+(WINDFIELD_NUM_ELEMENTS_Y*WINDFIELD_ELEMENT_SIZE_Y));
		Assert(worldZ>=m_basePosWldZ && worldZ<m_basePosWldZ+(WINDFIELD_NUM_ELEMENTS_Z*WINDFIELD_ELEMENT_SIZE_Z));

		gridX = ((u32)(worldX/WINDFIELD_ELEMENT_SIZE_X)) % WINDFIELD_NUM_ELEMENTS_X;
		gridY = ((u32)(worldY/WINDFIELD_ELEMENT_SIZE_Y)) % WINDFIELD_NUM_ELEMENTS_Y;
		gridZ = ((u32)(worldZ/WINDFIELD_ELEMENT_SIZE_Z)) % WINDFIELD_NUM_ELEMENTS_Z;
	}


	///////////////////////////////////////////////////////////////////////////////
	//  CalcGridPos
	///////////////////////////////////////////////////////////////////////////////
	// - returns the indices in the grid from the coordinates on the map
	// - returns true if the coordinates are actually in the grid area
	// - force inlining to make sure no LHS occurs
	///////////////////////////////////////////////////////////////////////////////

	__forceinline bool			phWindField::CalcGridPos				(int worldX, int worldY, int worldZ, u32& gridX, u32& gridY, u32& gridZ)
	{
		if ((worldX>=m_basePosWldX && worldX<m_basePosWldX+(WINDFIELD_NUM_ELEMENTS_X*WINDFIELD_ELEMENT_SIZE_X)) &&
			(worldY>=m_basePosWldY && worldY<m_basePosWldY+(WINDFIELD_NUM_ELEMENTS_Y*WINDFIELD_ELEMENT_SIZE_Y)) &&
			(worldZ>=m_basePosWldZ && worldZ<m_basePosWldZ+(WINDFIELD_NUM_ELEMENTS_Z*WINDFIELD_ELEMENT_SIZE_Z)))
		{
			gridX = ((u32)(worldX / WINDFIELD_ELEMENT_SIZE_X)) % WINDFIELD_NUM_ELEMENTS_X;
			gridY = ((u32)(worldY / WINDFIELD_ELEMENT_SIZE_Y)) % WINDFIELD_NUM_ELEMENTS_Y;
			gridZ = ((u32)(worldZ / WINDFIELD_ELEMENT_SIZE_Z)) % WINDFIELD_NUM_ELEMENTS_Z;

			return true;
		}

		return false;
	}


	///////////////////////////////////////////////////////////////////////////////
	//  CalcGridPosXFast
	///////////////////////////////////////////////////////////////////////////////
	// - returns the indices in the grid from the coordinates on the map
	// - caller must know the world coordinates are in the grid area
	///////////////////////////////////////////////////////////////////////////////

	inline void				phWindField::CalcGridPosXFast			(int worldX, u32& gridX)
	{
		Assert(worldX>=m_basePosWldX && worldX<m_basePosWldX+(WINDFIELD_NUM_ELEMENTS_X*WINDFIELD_ELEMENT_SIZE_X));
		gridX = ((u32)(worldX/WINDFIELD_ELEMENT_SIZE_X)) % WINDFIELD_NUM_ELEMENTS_X;
	}


	///////////////////////////////////////////////////////////////////////////////
	//  CalcGridPosYFast
	///////////////////////////////////////////////////////////////////////////////
	// - returns the indices in the grid from the coordinates on the map
	// - caller must know the world coordinates are in the grid area
	///////////////////////////////////////////////////////////////////////////////

	inline void				phWindField::CalcGridPosYFast			(int worldY, u32& gridY)
	{
		Assert(worldY>=m_basePosWldY && worldY<m_basePosWldY+(WINDFIELD_NUM_ELEMENTS_Y*WINDFIELD_ELEMENT_SIZE_Y));
		gridY = ((u32)(worldY/WINDFIELD_ELEMENT_SIZE_Y)) % WINDFIELD_NUM_ELEMENTS_Y;
	}


	///////////////////////////////////////////////////////////////////////////////
	//  CalcGridPosZFast
	///////////////////////////////////////////////////////////////////////////////
	// - returns the indices in the grid from the coordinates on the map
	// - caller must know the world coordinates are in the grid area
	///////////////////////////////////////////////////////////////////////////////

	inline void				phWindField::CalcGridPosZFast			(int worldZ, u32& gridZ)
	{
		Assert(worldZ>=m_basePosWldZ && worldZ<m_basePosWldZ+(WINDFIELD_NUM_ELEMENTS_Z*WINDFIELD_ELEMENT_SIZE_Z));
		gridZ = ((u32)(worldZ/WINDFIELD_ELEMENT_SIZE_Z)) % WINDFIELD_NUM_ELEMENTS_Z;
	}


	///////////////////////////////////////////////////////////////////////////////
	//  CalcWorldPosX
	///////////////////////////////////////////////////////////////////////////////
	// - given a grid coordinate this function returns the world coordinate
	///////////////////////////////////////////////////////////////////////////////

	inline void				phWindField::CalcWorldPosX				(u32 gridX, int& worldX)
	{
		u32 basePointGridX = ((u32)(m_basePosWldX/WINDFIELD_ELEMENT_SIZE_X)) % WINDFIELD_NUM_ELEMENTS_X;
		worldX = m_basePosWldX + ((gridX-basePointGridX) % WINDFIELD_NUM_ELEMENTS_X) * WINDFIELD_ELEMENT_SIZE_X;
	}


	///////////////////////////////////////////////////////////////////////////////
	//  CalcWorldPosY
	///////////////////////////////////////////////////////////////////////////////
	// - given a grid coordinate this function returns the world coordinate
	///////////////////////////////////////////////////////////////////////////////

	inline void				phWindField::CalcWorldPosY				(u32 gridY, int& worldY)
	{
		u32 basePointGridY = ((u32)(m_basePosWldY/WINDFIELD_ELEMENT_SIZE_Y)) % WINDFIELD_NUM_ELEMENTS_Y;
		worldY = m_basePosWldY + ((gridY-basePointGridY) % WINDFIELD_NUM_ELEMENTS_Y) * WINDFIELD_ELEMENT_SIZE_Y;
	}


	///////////////////////////////////////////////////////////////////////////////
	//  CalcWorldPosZ
	///////////////////////////////////////////////////////////////////////////////
	// - given a grid coordinate this function returns the world coordinate
	///////////////////////////////////////////////////////////////////////////////

	inline void				phWindField::CalcWorldPosZ				(u32 gridZ, int& worldZ)
	{
		u32 basePointGridZ = ((u32)(m_basePosWldZ/WINDFIELD_ELEMENT_SIZE_Z)) % WINDFIELD_NUM_ELEMENTS_Z;
		worldZ = m_basePosWldZ + ((gridZ-basePointGridZ) % WINDFIELD_NUM_ELEMENTS_Z) * WINDFIELD_ELEMENT_SIZE_Z;
	}


} // namespace rage

#endif // PHEFFECTS_WINDFIELD_H 
