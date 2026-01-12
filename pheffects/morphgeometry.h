//
// pheffects/morphgeometry.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHEFFECTS_MORPHGEOMETRY_H
#define PHEFFECTS_MORPHGEOMETRY_H

#include "data/struct.h"
#include "phbound/boundgeom.h"

namespace rage {


////////////////////////////////////////////////////////////////
// phMorphGeometry

/*
PURPOSE
	A class to represent a physics bound that is morphable between any of the bounds supplied.
*/

class phMorphGeometry : public phBoundGeometry
{
public:
	// PURPOSE: New bound type necessary to be different than any of the existing bound types.
	enum BoundType
	{
		GEOMETRY_MORPH = phBound::NUM_BOUND_TYPES,
		NUM_BOUND_TYPES
	};

	phMorphGeometry ();
	~phMorphGeometry ();

	// PURPOSE: Initialize the morph bound, allocate space for pointers numBounds bounds.
	void Init (int numBounds);

	////////////////////////////////////////////////////////////

	// bounds
	// PURPOSE: Returns the bound at index.
	phBoundGeometry * GetBound (int index) const						{ FastAssert(index>=0 && index<m_NumBounds); return m_MorphBounds[index]; }

	// PURPOSE: Gets weight of the bound at index.
	float GetWeight (int index) const							{ FastAssert(index>=0 && index<m_NumBounds); return m_Weights[index]; }

	// PURPOSE: Sets weight of the bound at index.
	void SetWeight (int index, float weight) 					{ FastAssert(index>=0 && index<m_NumBounds); m_Weights[index] = weight; }

	// PURPOSE: Gets weight of the bound at index.
	int GetNumBounds () const									{ return m_NumBounds; }

	// PURPOSE: Sets the entry at index to the new bound passed into this function.
	void SetBound (int index, phBoundGeometry* bound);

	// PURPOSE: Clears the entry at index.
	void RemoveBound (int index)								{ SetBound(index,NULL); }

	// PURPOSE: Updates and morphs the morphbounds into the morphtarget bound.
	void Update();

	// PURPOSE: Initializes the morphtarget bound to the same number of vertices as the bound passed in.
	void InitMorphTargetBound(const phBoundGeometry* original);

	// PURPOSE: New virtual constructor needs to be passed to phBound::SetCustomConstructor().
	static phBound* NewTypeSpecific(const char* typestring);

	// PURPOSE: New virtual constructor needs to be passed to phBound::SetCustomResourceConstructor(). Returns true if bound is a phMorphGeometry.
	static bool  NewRscTypeSpecific(phBound* bound, datResource& rsc);

	////////////////////////////////////////////////////////////
	// resources
	phMorphGeometry (datResource & rsc);							// construct in resource

protected:
	///////////////////////////////////////////////////////////
	// load / save
	bool Load_v110 (fiAsciiTokenizer & token);						// load, ASCII, version 110

#if !__FINAL && !IS_CONSOLE
	bool Save_v110 (fiAsciiTokenizer & token);						// save, ASCII, version 110
#endif


protected:

	//============================================================================
	// Data

	// PURPOSE: A list of pointers to the sub-bounds that compose this morph bound.
	phBoundGeometry ** m_MorphBounds;

	// PURPOSE: A list of weights for the sub-bounds that compose this morph bound.
	float* m_Weights;

	// PURPOSE: A list of vertices, one per sub-bound to morph each vertex from.
	Vector3* m_MorphVerts;

	// PURPOSE: Number of sub-bounds that this morph bound is composed of.
	u16 m_NumBounds;

	// PURPOSE: array of indices that need to be morphed between frames.
	int* m_MorphIndices;
	int  m_NumMorphIndices;

	// Added post gta4 - Want to keep and move to rage\dev
	// Improves volume calculation on geometry bounds with open edges
#if !__SPU
	PH_NON_SPU_VIRTUAL bool HasOpenEdges() const {return false;}
#endif
};

} // namespace rage

#endif
