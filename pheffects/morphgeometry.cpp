//
// pheffects/morphgeometry.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "morphgeometry.h"

#include "data/resource.h"
#include "data/resourcehelpers.h"
#include "file/token.h"
#include "phbound/boundpolyhedron.h"
#include "phbound/boundtaperedcapsule.h"
#include "phcore/material.h"
#include "phcore/materialmgr.h"
#include "phcore/phmath.h"
#include "phcore/segment.h"
#include "grprofile/drawmanager.h"
#include "string/string.h"
#include "system/memops.h"

namespace rage {

EXT_PFD_DECLARE_ITEM(ComponentIndices);

////////////////////////////////////////////////////////////////

phMorphGeometry::phMorphGeometry ()
{
	m_MorphBounds = NULL;
	phBound::SetCentroidOffset(Vec3V(V_ZERO));
	m_Type = GEOMETRY_MORPH;
}


void phMorphGeometry::Init (int numBounds)
{
	Assert(numBounds>0 && numBounds<0xFFFF);
	m_NumBounds = static_cast<u16>(numBounds);

	m_MorphBounds = rage_new phBoundGeometry*[m_NumBounds];
	m_Weights = rage_new float[m_NumBounds];
	m_MorphVerts = rage_new Vector3[m_NumBounds];
	memset(m_MorphBounds,0,sizeof(phBoundGeometry*) * m_NumBounds);
	memset(m_Weights,0,sizeof(float) * m_NumBounds);
}


phMorphGeometry::~phMorphGeometry()
{
	if (m_MorphBounds)
	{
		for (int i=0; i<m_NumBounds; i++)
		{
			SetBound(i, NULL);
		}
	}

	delete[] m_MorphIndices;	
	delete[] m_MorphBounds;	
}


/////////////////////////////////////////////////////////////////
// load / save
bool  phMorphGeometry::NewRscTypeSpecific(phBound* Bound, datResource& rsc)
{
	if(Bound->GetType() == GEOMETRY_MORPH)
	{
		::new ((void *)Bound) phMorphGeometry(rsc);
		return true;
	}  //!me ugly, change this.
#if USE_TAPERED_CAPSULE
	else if( Bound->GetType() == phBound::TAPERED_CAPSULE)
	{
		::new ((void *)Bound) phBoundTaperedCapsule(rsc);
		return true;
	}
#endif

	return false;
}

phBound* phMorphGeometry::NewTypeSpecific(const char* typestring)
{
	if (stricmp(typestring,"MORPHGEOMETRY") == 0)
		return rage_new phMorphGeometry;
	else
		return NULL;
}

bool phMorphGeometry::Load_v110 (fiAsciiTokenizer & token)
{
	token.MatchToken("NumBounds:");
	Init(token.GetInt());

	// centroid offset
	Vector3 temp;
	if (token.CheckToken("centroid:"))
	{
		token.GetVector(temp);
		SetCentroidOffset(RCC_VEC3V(temp));
	}
	
	// center of gravity offset
	// bool cgOffsetInBoundFile = false;
	if (token.CheckToken("cg:"))
	{
		token.GetVector (temp);
		SetCGOffset(RCC_VEC3V(temp));
		// cgOffsetInBoundFile = true;
	}

	phBoundGeometry* partBound=NULL;
#if __DEV
	int checkNumVerts = 0;														// in dev builds we want to make sure that all bounds have the same amount of verts
#endif
	for(int i=0; i<m_NumBounds; i++)
	{
		token.MatchToken("bound:");
		int partIndex=token.GetInt();
		if (token.CheckToken("name:"))											// is the bound in another file?
		{
			char partName[RAGE_MAX_PATH];
			token.GetToken(partName,sizeof(partName));
			partBound = dynamic_cast<phBoundGeometry*> (phBound::Load(partName));
		}
		else																	// nope, bound is part of this file, so lets just load it
		{
			partBound = dynamic_cast<phBoundGeometry*> (phBound::Load(token));
		}
		Assert(partBound);														// Make sure the bound we tried to load exists.
		SetBound(partIndex,partBound);
#if __DEV
		if(!checkNumVerts)														// did we get the number of verts yet?
			checkNumVerts = partBound->GetNumVertices();						// nope so lets get the first bounds number of verts
		else																	// yup, got number of verts already
			if(checkNumVerts != partBound->GetNumVertices())					// so lets make sure the new bound has the same number of verts
				Quitf("phBoundGeometry %s differs in number of vertices!",token.GetName());	// lets quit out since we cannot morph between bounds that have different number of vertices.
#endif
	}
	InitMorphTargetBound(partBound);													// Initialize our morphtarget bound from one of the source bounds.
	CalculateExtents();															// calculate its extents

	return true;
}


#if !__FINAL && !IS_CONSOLE
bool phMorphGeometry::Save_v110 (fiAsciiTokenizer & token)
{
	int actualNum = 0;

	int index;
	for(index=0;index<m_NumBounds;index++)
	{
		if (m_MorphBounds[index])
		{
			++actualNum;
		}
	}

	token.PutDelimiter("\n");
	token.PutDelimiter("NumBounds: ");
	token.Put(actualNum);
	token.PutDelimiter("\n");

	if(!IsEqualAll(GetCentroidOffset(),Vec3V(V_ZERO)))
	{
		token.PutDelimiter("centroid: ");
		token.Put(GetCentroidOffset());
		token.PutDelimiter("\n");
	}

	if(!IsEqualAll(GetCGOffset(),Vec3V(V_ZERO)))
	{
		token.PutDelimiter("cg: ");
		token.Put(GetCGOffset());
		token.PutDelimiter("\n");
	}

	token.PutDelimiter("\n");

	for(index=0;index<m_NumBounds;index++)
	{
		if (m_MorphBounds[index])
		{
			token.PutDelimiter("\n");
			token.PutDelimiter("\n");
			token.PutDelimiter("bound: ");
			token.Put(index);
			token.PutDelimiter("\n");
			phBound::Save(token,m_MorphBounds[index],VERSION_110);
		}
	}
	return true;
}
#endif	// end of #if !__FINAL && !IS_CONSOLE



/*
Purpose: Set the physics bound pointer for this composite bound.
Parameters: bound - pointer to the phBound that will be used by this archetype.
Notes:
- The reference count of the new bound is incremented if the new bound is not NULL.
- The reference count of the old bound is decremented if the old bound is not NULL. */
void phMorphGeometry::SetBound (int i, phBoundGeometry * bound)
{
	if (bound!=NULL)
	{
		// Add a reference count to the new bound.
		bound->AddRef();
	}

	if (m_MorphBounds[i]!=NULL)
	{
		// Remove a reference count from the old bound.
		m_MorphBounds[i]->Release();
	}

	// Replace the old bound with the new bound.
	m_MorphBounds[i] = bound;
}

void phMorphGeometry::InitMorphTargetBound (const phBoundGeometry* original)
{
#if __ASSERT																	// in __DEV builds lets make sure all geometry has the same number of verts.
	int numVerts = m_MorphBounds[0]->GetNumVertices();
	for(int j=1; j<m_NumBounds; j++)
	{
		Assert(m_MorphBounds[j]->GetNumVertices() == numVerts);		
	}
#endif
	int* tempMorphIndices = rage_new int[m_MorphBounds[0]->GetNumVertices()];
	m_NumMorphIndices = 0;
    for(int i=0; i<m_MorphBounds[0]->GetNumVertices(); i++)
	{
		bool needToMorph = false;
		for(int j=0; j<m_NumBounds; j++)									// first lets check if we need to morph this vert.
		{
			m_MorphVerts[0].Set(VEC3V_TO_VECTOR3(m_MorphBounds[0]->GetVertex(i)));
			m_MorphVerts[j].Set(VEC3V_TO_VECTOR3(m_MorphBounds[j]->GetVertex(i)));
			m_MorphVerts[0].Subtract(m_MorphVerts[j]);
			if(!needToMorph)
			{
				needToMorph = m_MorphVerts[0].Mag2() > 0.0001f;
			}
		}
		if(needToMorph)
			tempMorphIndices[m_NumMorphIndices++] = i;
	}
	m_MorphIndices = rage_new int[m_NumMorphIndices];
	sysMemCpy(m_MorphIndices, tempMorphIndices, m_NumMorphIndices*sizeof(int));
	delete [] tempMorphIndices;
	phBoundGeometry::Copy(original);
	m_Type = GEOMETRY_MORPH;
}



// Pointers to the Bounds and LocalMatrices arrays in phMorphGeometry keep their original values
// in the clone and each phMorphGeometry keeps its own lists.  Pointers to LastMatrices and
// CurrentMatrices also maintain their original values, but their arrays are not copied to the clone
// because the default values are the same as the LocalMatrices pointer.
void phMorphGeometry::Update()
{
	Vector3 finalVert;
	for(int i=0; i<m_NumMorphIndices; i++)
	{
		finalVert.Set(0.0f);
		for(int j=0; j<m_NumBounds; j++)									// first lets check if we need to morph this vert.
		{
			Vector3 temp; 
			temp.Set(VEC3V_TO_VECTOR3(m_MorphBounds[j]->GetVertex(m_MorphIndices[i])));
			temp.Scale(m_Weights[j]);
			finalVert.Add(temp);
		}
		SetVertex(m_MorphIndices[i],RCC_VEC3V(finalVert));
	}
	CalculatePolyNormals();
}

phMorphGeometry::phMorphGeometry (datResource & rsc)
	: phBoundGeometry(rsc)
{
	rsc.PointerFixup(m_MorphBounds);
	rsc.PointerFixup(m_MorphIndices);

	for (int i=0; i<m_NumBounds; i++)
	{
		ObjectFixup(rsc, m_MorphBounds[i]);
	}
}


} // namespace rage
