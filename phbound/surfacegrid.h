//
// phbound/surfacegrid.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//
#ifndef PHBOUND_SURFACEGRID_H
#define PHBOUND_SURFACEGRID_H

// If this is turned on, there is a static float within the GetElevation() below
// that will allow you to experiment with adjusting the output elevation. This was
// useful once for diagnosing bounding box issues. -ecosky
#define DEBUG_SURFACE_GRID_OFFSET 0


// This causes spam related to the assignment of surface grid coordinates to be 
// produced both within the phSurfaceGrid and the phLiquidMgr.
#define DEBUG_SURFACE_GRID_COORDINATES 0


// This causes spam related to the attachment and detachment of a phSurfaceGrid 
// to its (potential) neighbors.
#define DEBUG_SURFACE_GRID_ATTACH 0

namespace rage {

	// PURPOSE:	This works just like the IsNan() function in 'math/nan.h', but
	//			it uses the integer pipeline instead of the float pipeline. The
	//			regular IsNan() basically just did "return f != f", and the resulting
	//			floating point compare was very expensive when used for branching
	//			(not sure if it matters that the actual value often was NaN, I don't
	//			know if the FPU is slower at comparisons involving those).
	// PARAMS:	f		- Pointer to the floating point value to check, in memory
	//					  (intentionally a pointer, not a reference, to make users
	//					  maybe think a little about it before using it on something
	//					  that may be in a register).
	// NOTES:	If the value you want to check is already in a floating point register,
	//			it is probably not any faster to use this than to use the regular IsNan(),
	//			since you would probably get a load-hit-store when it stores it out to memory
	//			and reads it back again.
	inline bool IsNanInMemory(const float *f)
	{
		// Note: It's possible that there are ways to optimize the code below further,
		// by maybe doing some more bit shifting, masking, XOR'ing or whatever. But,
		// I have verified that this code executes much much faster than the regular IsNan()
		// in a Xenon Release build anyway.
		// Also, it is possible that the user knows that his particular NaN would use a particular
		// exact bit value, in which case it may be unnecessary to check the full range, but when I
		// first tried doing it that way and checking against the 0x7f800001 produced by MakeNan(),
		// I ran into numbers that for one reason or another were 0x7fc00001, so be careful./FF

		// The 32 bits of an IEEE single precision float are
		//	S EEEEEEEE FFFFFFFFFFFFFFFFFFFFFFF
		// and the condition for it representing a NaN is
		//	If E=255 and F is nonzero, then V=NaN ("Not a number") 		
		// (see http://www.psc.edu/general/software/packages/ieee/ieee.html)

		// Get away from thinking about this as a float. /FF
		const u32 *asIntPtr = reinterpret_cast<const u32*>(f);

		// Mask off the sign bit, since it's value doesn't matter
		// for determining if it's a NaN. /FF
		const u32 mask = 0x7fffffff;
		const u32 asIntMasked = (*asIntPtr) & mask;

		// Now, check if it's between 0x7f800001 and 0x7fffffff (which is the
		// same as the mask used above). /FF
		return asIntMasked >= 0x7f800001 && asIntMasked <= mask;
	}

	////////////////////////////////////////////////////////////////
	// phSurfaceGrid

	// PURPOSE
	//   Class to represent the data for a height field surface.
	// NOTES
	//   This class is actually designed to be used as part of an dynamic, animated height field, hence the two sets of grid data (and the two pointers).
	//   This class also which records surface grids are adjacent and connected to it. 
	//
	//  This class supports the use of an "Offset Grid" which is a 2D array of floats that match the pair of 2D array of floats this class maintains.
	//	Functions for getting elevation will add in the offset grid value for the specified coordinate, allowing the grid to map to varying terrain
	//	 such as a river flowing down hill.
	//
	//  There are also functions for interacting with "Local" elevation values which are useful for the external code (such as wave simulators) to 
	//   do what they need to do without having to worry about the offset grid messing things up.
	//
	//	At one point it was suggested that this data could be moved up to the bound that is using the grid, but certain managers (ie liquid) 
	//   have a finite number of phSurfaceGrid objects that they share between any number of high-LOD bounds; keeping the grid separate 
	//   allows the memory footprint of the grid to be limited to those bounds that are at high LOD. The management of the adjacent neighbors
	//	 has also been simplified, with the bulk of the code related to attaching/detaching now existing within this class. The manager needs to
	//	 only track grids by grid-coordinates (ie one phSurfaceGrid is one coordinate cell) and notify the grids of neighbors and this class
	//   will do the connecting from there.
	//

	class phSurfaceGrid
	{
	public:

		// Currently, this class is fixed to use a 32x32 grid. This should probably be changed to a template parameter, but
		// at the moment there are quite a few dependencies on the 32x32 arrangement.
		enum
		{
			kSurfaceGridLength = 32,
			kSurfaceGridPointCount = kSurfaceGridLength * kSurfaceGridLength,
		};

		// The Neighbor enumeration is merely used to identify which adjacent phSurfaceGrid entries are in each slot.
		enum Neighbor
		{
			kNeighborNextX = 0,
			kNeighborPrevX,
			kNeighborNextZ,
			kNeighborPrevZ
		};

		// Purpose:
		// Support class for safely accessing/manipulating the various types of 2d grid data.
		template <typename T>
		class GridData : public atRangeArray<T, kSurfaceGridPointCount>
		{
		public:
			GridData() {}

			GridData(datResource& rsc)
				: atRangeArray<T, kSurfaceGridPointCount>(rsc)
			{}

			void SetValues(const T& val)
			{
				for(int i = 0; i < kSurfaceGridPointCount; i++)
				{
					(*this)[i] = val;
				}
			}

			__forceinline static int GetIndex(int x, int z)
			{
				return z * kSurfaceGridLength + x;
			}
			__forceinline const T& GetValue(int x, int z) const
			{
				return (*this)[GetIndex(x,z)];
			}
			__forceinline T& GetValue(int x, int z)
			{
				return (*this)[GetIndex(x,z)];
			}
			__forceinline void SetValue(int x, int z, const T& val)
			{
				(*this)[GetIndex(x,z)] = val;
			}
		};	 
		
		inline phSurfaceGrid();
		
		~phSurfaceGrid()
		{
			DetachNeighbors();
		}

		// PURPOSE:
		//	Set the "Grid World Space" coordinates of this cell. Note that each phSurfaceGrid consumes 1 unit of area in this coordinate system.
		//  The use of integral coordinates for the cells facilitates the connection of adjacent phSurfaceGrids.
		inline void SetCellCoordinates(int x, int z)
		{
#if DEBUG_SURFACE_GRID_COORDINATES
			Displayf("phSurfaceGrid:SetCellCoordinates %08x -> %d %d", this, x,z);
#endif
			m_CellX = x;
			m_CellZ = z;
		}

		inline int GetCellX() const { return m_CellX; }
		inline int GetCellZ() const { return m_CellZ; }



		inline float GetMaxOffset() const { return m_fMaxOffset; }
		inline float GetWorldSpaceOffset() const { return m_fWorldSpaceOffset; }


		inline void SetOffsetAndVelocityGrid(const GridData<float>* grid, const GridData<Vector2>* velocityGrid, float fMaxElevation, float fMinElevation)
		{
			m_pOffsetGrid = grid;
			m_pVelocityGrid = velocityGrid;
			m_fMaxOffset = fMaxElevation - fMinElevation;
			m_fWorldSpaceOffset = fMinElevation;
		}

		inline void Reset()
		{
			m_pOffsetGrid = NULL;
			m_pVelocityGrid = NULL;
			DetachNeighbors();
			m_fMaxOffset = 0.0f;
			m_fWorldSpaceOffset = 0.0f;
		}

		inline const GridData<float>* GetOffsetGrid() const
		{
			return m_pOffsetGrid;
		}

		inline const GridData<Vector2>* GetVelocityGrid() const
		{
			return m_pVelocityGrid;
		}

		inline void Clear();
		inline void SwapGrids();

		// PURPOSE:
		//  Do the math needed to refer to grid elements by a single index so the code can
		//	avoid doing extra work to look up [x][z] values.
		inline int GetIndex(int x, int z) const
		{
			Assert(x >= 0 && x < kSurfaceGridLength);
			Assert(z >= 0 && z < kSurfaceGridLength);
			return x + z * kSurfaceGridLength;
		}

		// PURPOSE:
		//  Same as GetIndex, but without any error checking.
		inline int GetIndexUnsafe(int x, int z) const
		{
			return x + z * kSurfaceGridLength;
		}

		__forceinline float GetOffset(int index) const
		{
			return (*m_pOffsetGrid)[index];
		}
		__forceinline bool IsOffsetValid(int index) const
		{

			Assert(index >= 0 && index < kSurfaceGridPointCount);
			if(m_pOffsetGrid)
			{
				return GetOffset(index) > 0.0f;
			}
			return false;
		}

		// PURPOSE:
		// Same as IsOffsetValid, but without any error checking.
		__forceinline bool IsOffsetValidUnsafe(int index) const
		{
			if(m_pOffsetGrid)
			{
				return GetOffset(index) >= 0.0f;
			}
			return false;
		}

		__forceinline bool IsOffsetValid(int x, int z) const
		{
			return IsOffsetValid(GetIndex(x,z));
		}

		// PURPOSE:
		//  Return true if the specified grid index is valid.
		//  WARNING: The caller is responsible for range checking and ensuring this has an offset grid.
		__forceinline bool IsOffsetValidFast(int index) const
		{
			FastAssert(index >= 0 && index < kSurfaceGridPointCount);
			FastAssert(m_pOffsetGrid);
			return GetOffset(index) >= 0.0f; // invalid points are below zero in local space.
		}

		// PURPOSE:
		// Same as IsOffsetValidFast, but without any error checking.
		__forceinline bool IsOffsetValidFastUnsafe(int index) const
		{
			return GetOffset(index) >= 0.0f; // invalid points are below zero in local space.
		}

		// PURPOSE:
		//	Return the elevation of the grid at the specified coordinate, including the offset elevation.
		__forceinline float GetElevation(int index) const
		{
			FastAssert(index >= 0 && index < kSurfaceGridPointCount);
			if(m_pOffsetGrid)
			{
				float value = GetOffset(index);
#if DEBUG_SURFACE_GRID_OFFSET
				static float debugOffset = 0.0f;
				return m_CurrentGrid[index] + value + debugOffset;
#else
				return m_CurrentGrid[index] + value;
#endif

			}
			return m_CurrentGrid[index];
		}

		// PURPOSE:
		//	Return the elevation of the grid at the specified coordinate, including the offset elevation.
		// NOTE:
		//  Consider using the version of this function that takes an index if you already have the index available.
		__forceinline float GetElevation(int x, int z) const
		{
			return GetElevation(GetIndex(x,z));
		}


		// PURPOSE:
		//	Return the elevation of the grid at the specified coordinate, including the offset elevation.
		//	Sets the referenced bool to true if there is an offset grid, and the point at the offset grid is not-NaN.
		__forceinline float GetElevation(int index, bool& rValidOffsetPoint) const
		{
			FastAssert(index >= 0 && index < kSurfaceGridPointCount);
			if(m_pOffsetGrid)
			{
				float value = GetOffset(index);
				rValidOffsetPoint = value >= 0.0f;
#if DEBUG_SURFACE_GRID_OFFSET
				static float debugOffset = 0.0f;
				return m_CurrentGrid[index] + value + debugOffset;
#else
				return m_CurrentGrid[index] + value;
#endif

			}
			rValidOffsetPoint = false;
			return m_CurrentGrid[index];
		}

		// PURPOSE:
		//	Return the elevation of the grid at the specified coordinate, including the offset elevation.
		//	Sets the referenced bool to true if there is an offset grid, and the point at the offset grid is not-NaN.
		// NOTE:
		//  Consider using the version of this function that takes an index if you already have the index available.
		__forceinline float GetElevation(int x, int z, bool& rValidOffsetPoint) const
		{
			return GetElevation(GetIndex(x,z), rValidOffsetPoint);
		}

		// PURPOSE:
		//	Return the elevation of the grid at the specified coordinate, without including any offset values.
		__forceinline float GetLocalElevation(int index) const
		{
			Assert(index >= 0 && index < kSurfaceGridPointCount);
			return m_CurrentGrid[index];
		}


		// PURPOSE:
		// Same as GetLocalElevation, but without any error checking.
		__forceinline float GetLocalElevationUnsafe(int index) const
		{
			return m_CurrentGrid[index];
		}

		// PURPOSE:
		//	Return the elevation of the grid at the specified coordinate, without including any offset values.
		// NOTE:
		//  Consider using the version of this function that takes an index if you already have the index available.
		__forceinline float GetLocalElevation(int x, int z) const
		{
			return GetLocalElevation(GetIndex(x,z));
		}


		// PURPOSE:
		// Same as GetLocalElevation, but without any error checking.
		__forceinline float GetLocalElevationUnsafe(int x, int z) const
		{
			return GetLocalElevationUnsafe(GetIndexUnsafe(x,z));
		}

		// PURPOSE:
		//	Add the specified amount to the local (raw) elevation value for the specified index.
		// NOTE:
		//	This is an ADD not a SET function.
		__forceinline void ModifyLocalElevation(int index, float amount)
		{
			FastAssert(index >= 0 && index < kSurfaceGridPointCount);
			m_CurrentGrid[index] += amount;
		}

		// PURPOSE:
		//	Add the specified amount to the local (raw) elevation value for the specified index.
		// NOTE:
		//	This is an ADD not a SET function.
		//  Consider using the version of this function that takes an index if you already have the index available.
		__forceinline void ModifyLocalElevation(int x, int z, float amount)
		{
			ModifyLocalElevation(GetIndex(x,z), amount);
		}

		// PURPOSE:
		//	Return the elevation of the grid at the specified coordinate IN THE NEXT GRID array,
		//  without including any offset values. 
		__forceinline float GetNextLocalElevation(int index) const
		{
			FastAssert(index >= 0 && index < kSurfaceGridPointCount);
			return m_NextGrid[index];
		}


		// PURPOSE:
		// Same as GetNextLocalElevation, but without any error checking.
		__forceinline float GetNextLocalElevationUnsafe(int index) const
		{
			return m_NextGrid[index];
		}

		// PURPOSE:
		//	Return the elevation of the grid at the specified coordinate IN THE NEXT GRID array,
		//  without including any offset values. 
		// NOTE:
		//  Consider using the version of this function that takes an index if you already have the index available.
		__forceinline float GetNextLocalElevation(int x, int z) const
		{
			return GetNextLocalElevation(GetIndex(x,z));
		}


		// PURPOSE:
		//	Set the raw/local elevation of the grid at the specified coordinate IN THE NEXT GRID array.
		__forceinline void SetNextLocalElevation(int index, float elevation)
		{
			FastAssert(index >= 0 && index < kSurfaceGridPointCount);
			m_NextGrid[index] = elevation;
		}


		// PURPOSE:
		// Same as SetNextLocalElevation, but without any error checking.
		__forceinline void SetNextLocalElevationUnsafe(int index, float elevation)
		{
			m_NextGrid[index] = elevation;
		}

		// PURPOSE:
		//	Set the raw/local elevation of the grid at the specified coordinate IN THE NEXT GRID array.
		// NOTE:
		//  Consider using the version of this function that takes an index if you already have the index available.
		__forceinline void SetNextLocalElevation(int x, int z, float elevation)
		{
			SetNextLocalElevation(GetIndex(x,z), elevation);
		}


		// PURPOSE:
		// Same as SetNextLocalElevation, but without any error checking.
		__forceinline void SetNextLocalElevationUnsafe(int x, int z, float elevation)
		{
			SetNextLocalElevationUnsafe(GetIndexUnsafe(x,z), elevation);
		}


		// PURPOSE:
		//	Find the elevation at the given point, looking at neighbors if necessary to deal with edge cases.
		// NOTE:
		//  The edge case logic is simplistic and will only look for immediate neighbors.
		//	If a neighbor is not present at the specified location, it will return the edge elevation.
		float FindElevation(int x, int z) const
		{
			if(x < 0)
			{
				if(m_NeighborGrids[kNeighborPrevX])
				{
					return m_NeighborGrids[kNeighborPrevX]->FindRelativeElevation(x + kSurfaceGridLength, z, m_fWorldSpaceOffset);
				}
				x = 0;
			} 
			else if(x >= kSurfaceGridLength)
			{
				if(m_NeighborGrids[kNeighborNextX])
				{
					return m_NeighborGrids[kNeighborNextX]->FindRelativeElevation(x - kSurfaceGridLength, z, m_fWorldSpaceOffset);
				}
				x = kSurfaceGridLength - 1;
			}
			
			if(z < 0)
			{
				if(m_NeighborGrids[kNeighborPrevZ])
				{
					return m_NeighborGrids[kNeighborPrevZ]->FindRelativeElevation(x, z + kSurfaceGridLength, m_fWorldSpaceOffset);
				}
				z = 0;
			} 
			else if(z >= kSurfaceGridLength)
			{
				if(m_NeighborGrids[kNeighborNextZ])
				{
					return m_NeighborGrids[kNeighborNextZ]->FindRelativeElevation(x, z - kSurfaceGridLength, m_fWorldSpaceOffset);
				}
				z = kSurfaceGridLength - 1;
			}
			return GetElevation(x,z);
		}

		inline float FindElevationSafe(int x, int z) const
		{
			float elevation = FindElevation(x,z);

			// If the point is nan then there is no water here. Return the min elevation.
			if(IsNanInMemory(&elevation))
			{
				return m_fWorldSpaceOffset;
			}
			return elevation;
		}

		// PURPOSE:
		//	Return the elevation relative to another grid's world space elevation offset.
		inline float FindRelativeElevation(int x, int z, float otherWorldSpaceOffset) const
		{
			return FindElevation(x, z) + m_fWorldSpaceOffset - otherWorldSpaceOffset;
		}

		// PURPOSE:
		//	Find the elevation at the given point, looking at neighbors if necessary to deal with edge cases.
		//	Sets the referenced bool to true if there is an offset grid, and the point at the offset grid is not-NaN.
		// NOTE:
		//  The edge case logic is simplistic and will only look for immediate neighbors.
		//	If a neighbor is not present at the specified location, it will return the edge elevation.
		float FindElevation(int x, int z, bool& rValidOffsetPoint) const
		{
			if(x < 0)
			{
				if(m_NeighborGrids[kNeighborPrevX])
				{
					return m_NeighborGrids[kNeighborPrevX]->FindRelativeElevation(x + kSurfaceGridLength, z, m_fWorldSpaceOffset, rValidOffsetPoint);
				}
				x = 0;
			} 
			else if(x >= kSurfaceGridLength)
			{
				if(m_NeighborGrids[kNeighborNextX])
				{
					return m_NeighborGrids[kNeighborNextX]->FindRelativeElevation(x - kSurfaceGridLength, z, m_fWorldSpaceOffset, rValidOffsetPoint);
				}
				x = kSurfaceGridLength - 1;
			}

			if(z < 0)
			{
				if(m_NeighborGrids[kNeighborPrevZ])
				{
					return m_NeighborGrids[kNeighborPrevZ]->FindRelativeElevation(x, z + kSurfaceGridLength, m_fWorldSpaceOffset, rValidOffsetPoint);
				}
				z = 0;
			} 
			else if(z >= kSurfaceGridLength)
			{
				if(m_NeighborGrids[kNeighborNextZ])
				{
					return m_NeighborGrids[kNeighborNextZ]->FindRelativeElevation(x, z - kSurfaceGridLength, m_fWorldSpaceOffset, rValidOffsetPoint);
				}
				z = kSurfaceGridLength - 1;
			}
			return GetElevation(x,z, rValidOffsetPoint);
		}

		// PURPOSE:
		//	Return the elevation relative to another grid's world space elevation offset.
		inline float FindRelativeElevation(int x, int z, float otherWorldSpaceOffset, bool& rValidOffsetPoint) const
		{
			return FindElevation(x, z, rValidOffsetPoint) + m_fWorldSpaceOffset - otherWorldSpaceOffset;
		}


		// PURPOSE:
		//	Find the LOCAL elevation at the given point (no offset value added), looking at neighbors if necessary to deal with edge cases.
		// NOTE:
		//	The edge case logic is simplistic and will only look for immediate neighbors.
		//	If a neighbor is not present at the specified location, it will return the edge elevation.
		float FindLocalElevation(int x, int z) const
		{
			if(x < 0)
			{
				if(m_NeighborGrids[kNeighborPrevX])
				{
					return m_NeighborGrids[kNeighborPrevX]->FindLocalElevation(x + kSurfaceGridLength, z);
				}
				x = 0;
			} 
			else if(x >= kSurfaceGridLength)
			{
				if(m_NeighborGrids[kNeighborNextX])
				{
					return m_NeighborGrids[kNeighborNextX]->FindLocalElevation(x - kSurfaceGridLength, z);
				}
				x = kSurfaceGridLength - 1;
			}

			if(z < 0)
			{
				if(m_NeighborGrids[kNeighborPrevZ])
				{
					return m_NeighborGrids[kNeighborPrevZ]->FindLocalElevation(x, z + kSurfaceGridLength);
				}
				z = 0;
			} 
			else if(z >= kSurfaceGridLength)
			{
				if(m_NeighborGrids[kNeighborNextZ])
				{
					return m_NeighborGrids[kNeighborNextZ]->FindLocalElevation(x, z - kSurfaceGridLength);
				}
				z = kSurfaceGridLength - 1;
			}
			return GetLocalElevation(x,z);
		}


		// PURPOSE:
		// Same as FindLocalElevation, but without any error checking.
		float FindLocalElevationUnsafe(int x, int z) const
		{
			if(x < 0)
			{
				if(m_NeighborGrids[kNeighborPrevX])
				{
					return m_NeighborGrids[kNeighborPrevX]->FindLocalElevationUnsafe(x + kSurfaceGridLength, z);
				}
				x = 0;
			} 
			else if(x >= kSurfaceGridLength)
			{
				if(m_NeighborGrids[kNeighborNextX])
				{
					return m_NeighborGrids[kNeighborNextX]->FindLocalElevationUnsafe(x - kSurfaceGridLength, z);
				}
				x = kSurfaceGridLength - 1;
			}

			if(z < 0)
			{
				if(m_NeighborGrids[kNeighborPrevZ])
				{
					return m_NeighborGrids[kNeighborPrevZ]->FindLocalElevationUnsafe(x, z + kSurfaceGridLength);
				}
				z = 0;
			} 
			else if(z >= kSurfaceGridLength)
			{
				if(m_NeighborGrids[kNeighborNextZ])
				{
					return m_NeighborGrids[kNeighborNextZ]->FindLocalElevationUnsafe(x, z - kSurfaceGridLength);
				}
				z = kSurfaceGridLength - 1;
			}
			return GetLocalElevationUnsafe(x,z);
		}

		// PURPOSE:
		//	Get the velocity for a specific index.
		inline Vector2 GetVelocity(int index)
		{
			if(m_pVelocityGrid)
			{
				return (*m_pVelocityGrid)[index];
			}
			return Vector2(0.0f,0.0f);
		}

		// PURPOSE:
		//	Get the velocity for a specific cell coordinate.
		inline Vector2 GetVelocity(int x, int z) const
		{
			if(m_pVelocityGrid)
			{
				return (*m_pVelocityGrid)[GetIndex(x,z)];
			}
			return Vector2(0.0f,0.0f);
		}


		// PURPOSE:
		//	Find the normal at the given point, looking at neighbors if necessary to deal with edge cases.
		//  The calculation finds the average normal of the fan of four triangles that share the specified
		//  coordinate as the center point of a fan.
		//
		// NOTE: The edge case logic is simplistic and will only look for immediate neighbors.
		//   If a neighbor is not present at the specified location, it will duplicate the edge value for
		//   the normal calculation.
		//
		// TODO:
		//   This function could be optimized to deal with edge cases more efficiently.
		//
		inline void CalculateNormal(int x, int z, float spacing, Vector3& outNormal) const
		{
			// Find the elevation of the four points around the specified point.
			float left = FindLocalElevation(x-1, z);
			float right = FindLocalElevation(x+1, z);
			float top = FindLocalElevation(x, z-1);
			float bottom = FindLocalElevation(x, z+1);
			float center = FindLocalElevation(x,z);

			Vector3 vectorLeft(-spacing, left - center, 0.0f);
			Vector3 vectorRight(spacing, right - center, 0.0f);
			Vector3 vectorTop(0.0f, top - center, -spacing);
			Vector3 vectorBottom(0.0f, bottom - center, spacing);

			Vector3 normalLeftTop;
			normalLeftTop.Cross(vectorTop, vectorLeft);
			Vector3 normalLeftBottom;
			normalLeftBottom.Cross(vectorLeft, vectorBottom);
			Vector3 normalRightBottom;
			normalRightBottom.Cross(vectorBottom, vectorRight);
			Vector3 normalRightTop;
			normalRightTop.Cross(vectorRight, vectorTop);

			outNormal = normalLeftTop + normalLeftBottom + normalRightBottom + normalRightTop;
			outNormal.Normalize();
		}

		// PURPOSE:
		//	Just an accessor to the neighbor grid array that allows the use of enumerations for readability.
		inline phSurfaceGrid *GetNeighbor(Neighbor n) const
		{
			return m_NeighborGrids[n];
		}

		// PURPOSE:
		//	Just an accessor to the neighbor grid array that allows the use of enumerations for readability.
		inline void SetNeighbor(Neighbor n, phSurfaceGrid* pNeighbor)
		{
			m_NeighborGrids[n] = pNeighbor;
		}

		// PURPOSE:
		//	Attempt to attach the specified grid to this one. Uses the cell coordinates to determine if
		//  the grids are adjacent.
		bool TryAttachNeighbor(phSurfaceGrid* pNeighbor)
		{
			Assert(pNeighbor);

			// First check to see if it might get connected on the x axis.
			if(pNeighbor->GetCellX() == m_CellX)
			{
				int z = pNeighbor->GetCellZ();
				if(z == m_CellZ - 1)
				{
#if DEBUG_SURFACE_GRID_ATTACH
					Displayf("phSurfaceGrid:TryAttachNeighbor PrevX: this=%08x %d %d, neighbor=%08x %d %d", this, GetCellX(), GetCellZ(), pNeighbor, pNeighbor->GetCellX(),pNeighbor->GetCellZ());
#endif
					Assert(m_NeighborGrids[kNeighborPrevZ] == NULL);
					Assert(pNeighbor->GetNeighbor(kNeighborNextZ) == NULL);
					m_NeighborGrids[kNeighborPrevZ] = pNeighbor;
					pNeighbor->SetNeighbor(kNeighborNextZ, this);
					return true;
				}

				if(z == m_CellZ + 1)
				{
#if DEBUG_SURFACE_GRID_ATTACH
					Displayf("phSurfaceGrid:TryAttachNeighbor NextX: this=%08x %d %d, neighbor=%08x %d %d", this, GetCellX(), GetCellZ(), pNeighbor, pNeighbor->GetCellX(),pNeighbor->GetCellZ());
#endif
					Assert(m_NeighborGrids[kNeighborNextZ] == NULL);
					Assert(pNeighbor->GetNeighbor(kNeighborPrevZ) == NULL);
					m_NeighborGrids[kNeighborNextZ] = pNeighbor;
					pNeighbor->SetNeighbor(kNeighborPrevZ, this);
					return true;
				}
				return false;
			}

			// Now check to see if it might get connected on the z axis.
			if(pNeighbor->GetCellZ() == m_CellZ)
			{
				int x = pNeighbor->GetCellX();
				if(x == m_CellX - 1)
				{
#if DEBUG_SURFACE_GRID_ATTACH
					Displayf("phSurfaceGrid:TryAttachNeighbor PrevZ: this=%08x %d %d, neighbor=%08x %d %d", this, GetCellX(), GetCellZ(), pNeighbor, pNeighbor->GetCellX(),pNeighbor->GetCellZ());
#endif
					Assert(m_NeighborGrids[kNeighborPrevX] == NULL);
					Assert(pNeighbor->GetNeighbor(kNeighborNextX) == NULL);
					m_NeighborGrids[kNeighborPrevX] = pNeighbor;
					pNeighbor->SetNeighbor(kNeighborNextX, this);
					return true;
				}

				if(x == m_CellX + 1)
				{
#if DEBUG_SURFACE_GRID_ATTACH
					Displayf("phSurfaceGrid:TryAttachNeighbor NextZ: this=%08x %d %d, neighbor=%08x %d %d", this, GetCellX(), GetCellZ(), pNeighbor, pNeighbor->GetCellX(),pNeighbor->GetCellZ());
#endif
					Assert(m_NeighborGrids[kNeighborNextX] == NULL);
					Assert(pNeighbor->GetNeighbor(kNeighborPrevX) == NULL);
					m_NeighborGrids[kNeighborNextX] = pNeighbor;
					pNeighbor->SetNeighbor(kNeighborPrevX, this);
					return true;
				}
			}
#if DEBUG_SURFACE_GRID_ATTACH
			Displayf("phSurfaceGrid:TryAttachNeighbor Fail : this=%08x %d %d, neighbor=%08x %d %d", this, GetCellX(), GetCellZ(), pNeighbor, pNeighbor->GetCellX(),pNeighbor->GetCellZ());
#endif
			return false;
		}

		// Purpose:
		//	Unlink this surface grid from any neighbors, generally this occurs when the grid is being released/recycled/destroyed.
		void DetachNeighbors()
		{
#if DEBUG_SURFACE_GRID_ATTACH
			Displayf("phSurfaceGrid:DetachNeighbors() this=%08x %d %d NextX=%08x PrevX=%08x NextZ=%08x PrevZ=%08x", 
				this, GetCellX(), GetCellZ(), m_NeighborGrids[kNeighborNextX], m_NeighborGrids[kNeighborPrevX],m_NeighborGrids[kNeighborNextZ],m_NeighborGrids[kNeighborPrevZ]);
#endif
			// It used to be at high LOD, let's disable its water grid.
			if(m_NeighborGrids[kNeighborNextX] != NULL)
			{
				FastAssert(m_NeighborGrids[kNeighborNextX]->m_NeighborGrids[kNeighborPrevX] == this);
				m_NeighborGrids[kNeighborNextX]->m_NeighborGrids[kNeighborPrevX] = NULL;
				m_NeighborGrids[kNeighborNextX] = NULL;
			}
			if(m_NeighborGrids[kNeighborPrevX] != NULL)
			{
				FastAssert(m_NeighborGrids[kNeighborPrevX]->m_NeighborGrids[kNeighborNextX] == this);
				m_NeighborGrids[kNeighborPrevX]->m_NeighborGrids[kNeighborNextX] = NULL;
				m_NeighborGrids[kNeighborPrevX] = NULL;
			}
			if(m_NeighborGrids[kNeighborNextZ] != NULL)
			{
				FastAssert(m_NeighborGrids[kNeighborNextZ]->m_NeighborGrids[kNeighborPrevZ] == this);
				m_NeighborGrids[kNeighborNextZ]->m_NeighborGrids[kNeighborPrevZ] = NULL;
				m_NeighborGrids[kNeighborNextZ] = NULL;
			}
			if(m_NeighborGrids[kNeighborPrevZ] != NULL)
			{
				FastAssert(m_NeighborGrids[kNeighborPrevZ]->m_NeighborGrids[kNeighborNextZ] == this);
				m_NeighborGrids[kNeighborPrevZ]->m_NeighborGrids[kNeighborNextZ] = NULL;
				m_NeighborGrids[kNeighborPrevZ] = NULL;
			}
		}

	protected:
		// The neighboring grids to this grid.  Any can be NULL.
		// Right now, we don't have any way to distinguish between a neighbor that exists but doesn't have a grid
		//   and a neighbor that just doesn't exist.
		phSurfaceGrid *m_NeighborGrids[4];	

		// The actual grid arrays. There are two of them, Current and Next, which allow the users of these objects
		// to build the next grid using values from the first.
		// TODO: Convert these to GridData types for safety.
		float m_GridPoints0[kSurfaceGridLength][kSurfaceGridLength];
		float m_GridPoints1[kSurfaceGridLength][kSurfaceGridLength];

		// One of these will point to m_GridPoints0, the other will point to m_GridPoints1.
		float *m_CurrentGrid, *m_NextGrid;	

		// This is a separate grid that can be 'added' to the above grid.
		const GridData<float>* m_pOffsetGrid;	

		// This is another grid that can be set which will provide water velocity data.
		const GridData<Vector2>* m_pVelocityGrid;
		// The cell coordinates are used by the logic that attaches neighboring surfaces together.
		int m_CellX;
		int m_CellZ;

		// The max offset is used to track how large the largest offset grid value is.
		float m_fMaxOffset;

		// The min offset is used by track the lowest offset grid value, used by the phBoundSurface to set the centroid offset.
		float m_fWorldSpaceOffset;
	};


	phSurfaceGrid::phSurfaceGrid()
	{
		m_CurrentGrid = (float*) m_GridPoints0;
		m_NextGrid = (float*) m_GridPoints1;

		m_NeighborGrids[0] = NULL;
		m_NeighborGrids[1] = NULL;
		m_NeighborGrids[2] = NULL;
		m_NeighborGrids[3] = NULL;

		m_pOffsetGrid = NULL;
		m_pVelocityGrid = NULL;
		m_fMaxOffset = 0.0f;
		m_fWorldSpaceOffset = 0.0f;
	}


	void phSurfaceGrid::Clear()
	{
		for(int GridIndex = 0; GridIndex < kSurfaceGridPointCount; ++GridIndex)
		{
			m_CurrentGrid[GridIndex] = 0.0f;
			m_NextGrid[GridIndex] = 0.0f;
		}
	}

	void phSurfaceGrid::SwapGrids()
	{
		float *pTemp = m_CurrentGrid;
		m_CurrentGrid = m_NextGrid;
		m_NextGrid = pTemp;
	}

} // namespace rage

#endif
