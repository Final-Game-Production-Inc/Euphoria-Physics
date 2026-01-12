// 
// curve/curve.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef CURVE_CURVE_H
#define CURVE_CURVE_H

#include "phcore/constants.h"
#if ENABLE_UNUSED_CURVE_CODE

#include "atl/array.h"
#include "atl/array_struct.h"
#include "data/base.h"
#include "file/token.h"
#include "string/string.h"
#include "vector/vector3.h"
#include "vector/vectorn.h"

namespace rage {

enum { CURVE_TYPE_BASE, CURVE_TYPE_NURBS, CURVE_TYPE_CATROM };

class ConstString;
class fiAsciiTokenizer;
class datResource;
class datSerialize;


//=============================================================================
// cvCurve
//
// PURPOSE
//   cvCurve is a base class for any curve defined by a set of _Vector points. 
//   Virtual accessors are provided, and derived curve classes can override them 
//   to control how the curve is defined by the set of points.  
//   Examples of derived classes are cvCurveNurbs and cvCurveCatrom.
// NOTES
//   - No default implementation is provided for the base curve class methods. 
//     Implementation in derived classes is required. 
// <FLAG Component>
//
template<class _Vector> class cvCurve
{
public:
	cvCurve ();

    cvCurve (datResource & rsc);

	virtual ~cvCurve ();

	DECLARE_PLACE	(cvCurve<_Vector>);

#if	__DECLARESTRUCT
	virtual void	DeclareStruct	(datTypeStruct &s);
#endif

	void AllocateVertices(int maxVertices);

	void PostInit();

	//=========================================================================
	// Accessors

	// Accessor to return the type
	int GetType() const { return m_Type; }

	int	GetNumVertices() const;

	int	GetVertexCapacity() const;

	//increase the size of the vertex array to the amount specified by "totalNumVertices"
	void GrowNumVertices(int totalNumVertices);

	void SetNumVertices(int numVertices);

	_Vector& GetVertex (int index);

	const _Vector& GetVertex (int index) const;

	_Vector& AppendVertex ();

	void SetLooping (bool looping);

	//=========================================================================
	// IO

	void Load(fiAsciiTokenizer& token, int version);
	
	void Save(fiAsciiTokenizer& token, int version);
	
	void SaveCurve(fiAsciiTokenizer& token, int version);

	virtual void Serialize(datSerialize& archive);

	//=========================================================================
	// Evaluation

	// PURPOSE: Sets pos to the point on the curve at (seg,t).
	virtual float SolveSegment (_Vector& posOut, int seg, float t, float slope=0.0f, _Vector* direction=0);

	// PURPOSE: A simpler version of SolveSegment that just gives you a position 
	// (and optionally a normal) if you give it a t-value.
	virtual void SolveSegmentSimple (_Vector& posOut, int seg, float t, _Vector* tangentOut, _Vector* unitTangentOut);

	// PURPOSE: This will likely be faster than calling SolveSegmentSimple multiple times 
	// with the same t-value, because it can cache values that it has calculated rather 
	// than recalculating them with each call.
	virtual void SolveSegmentSimpleBatch(int numCurves, cvCurve<_Vector>** curves, _Vector* posOut, int seg, float t,
		_Vector** tangents);

	// PRUPOSE: Update (segOut,tOut) to the point on the curve (dist) ahead of their current position:
	virtual void Move (int& segOut, float& tOut, float dist, float slope=0.0f) const;

protected:

	atArray<_Vector> m_Vertices;

    u8 m_Type; // This has to be first for resourcing to work

	bool m_Looping;

	// PURPOSE: Pad to fill up the remaining bytes
	u8 pad[14];

private:
	virtual void AllocateVerticesDerived() {}

	virtual void PostInitVerts () {}
};


// singleton
#define CURVEMGR	(cvCurveMgrBase::GetInstance())

class cvCurveMgrBase : public datBase
{
public:
    virtual bool Load (const char* fileName, const char* fileExt="path") = 0;
	virtual bool Load (fiAsciiTokenizer & token) = 0;
    virtual bool Save (const char* fileName, const char* fileExt="path") = 0;
	virtual bool Save (fiAsciiTokenizer & token) = 0;

    virtual cvCurve<Vector3> * AddCurve(fiAsciiTokenizer & token) = 0;

    virtual int     GetCurveCount()                     const = 0;
    virtual cvCurve<Vector3> * GetCurve(int curve)                 const = 0;
    virtual int     GetCurveIndex(const cvCurve<Vector3> * pCurve) const = 0;

	static cvCurveMgrBase* GetInstance () { return sm_Inst; }

protected:
	static cvCurveMgrBase* sm_Inst;
};


//=============================================================================
// Wrapper function to help make tokenizer reading/writing of VectorNs generic.

template<int N>
void GetVectorGeneric(fiBaseTokenizer& , VectorN<N> &)
{
	// TODO: Implement if we keep this i/o
	FastAssert(0);
}

template<class _Vector>
void GetVectorGeneric(fiBaseTokenizer& t, _Vector &v)
{
	t.GetVector(v);
}

template<int N>
void PutVectorGeneric(fiBaseTokenizer& , VectorN<N> &)
{
	// TODO: Implement if we keep this i/o
	FastAssert(0);
}

template<class _Vector>
void PutVectorGeneric(fiBaseTokenizer& T, _Vector &v)
{
	T.Put(v);
}


//=============================================================================
// Implementations

template<class _Vector>
int cvCurve<_Vector>::GetNumVertices() const
{
	return m_Vertices.GetCount();
}
template<class _Vector>
int cvCurve<_Vector>::GetVertexCapacity() const
{
	return m_Vertices.GetCapacity();
}

template<class _Vector>
void cvCurve<_Vector>::GrowNumVertices(int numVertices)
{
	if(numVertices>m_Vertices.GetCapacity())
	{
		m_Vertices.Resize(m_Vertices.GetCapacity());
		m_Vertices.Grow(numVertices - m_Vertices.GetCapacity());
	}
	
	if(m_Vertices.GetCapacity() != m_Vertices.GetCount())
		m_Vertices.Resize(numVertices);
}

template<class _Vector>
void cvCurve<_Vector>::SetNumVertices(int numVertices)
{
	Assertf(numVertices<=m_Vertices.GetCapacity(),"Requested %d vertices, but only %d were allocated for this curve.  Call GrowNumVertices() to expand the size", numVertices,m_Vertices.GetCapacity());
	m_Vertices.Resize(numVertices);
}

template<class _Vector>
_Vector & cvCurve<_Vector>::GetVertex(int i)
{
	return m_Vertices[i];
}

template<class _Vector>
const _Vector & cvCurve<_Vector>::GetVertex(int i) const
{
	return m_Vertices[i];
}

template<class _Vector>
_Vector & cvCurve<_Vector>::AppendVertex()
{
	return m_Vertices.Append();
}

template<class _Vector>
void cvCurve<_Vector>::SetLooping(bool looping)
{
	m_Looping = looping;
}	

template<class _Vector>
cvCurve<_Vector>::cvCurve()
{
	m_Looping = false;
	m_Type = CURVE_TYPE_BASE;
}

template<class _Vector> 
cvCurve<_Vector>::~cvCurve()
{
}

template<class _Vector> 
void cvCurve<_Vector>::AllocateVertices(int maxVertices)
{
	FastAssert(m_Vertices.GetCapacity()==0);
	m_Vertices.Reserve(maxVertices);
	m_Vertices.Resize(0);
	AllocateVerticesDerived();
}

template<class _Vector> 
float cvCurve<_Vector>::SolveSegment (_Vector& UNUSED_PARAM(posOut), int UNUSED_PARAM(seg), float UNUSED_PARAM(t),
									  float UNUSED_PARAM(slope), _Vector* UNUSED_PARAM(tangent))
{
	AssertMsg(0 , "Base cvCurve class does not define an actual curve.");
	return 0.0f;
}

template<class _Vector> 
void cvCurve<_Vector>::SolveSegmentSimple (_Vector& UNUSED_PARAM(posOut), int UNUSED_PARAM(seg), float UNUSED_PARAM(t), _Vector* UNUSED_PARAM(tangentOut), _Vector* UNUSED_PARAM(unitTangentOut))
{
	AssertMsg(0 , "Base cvCurve class does not define an actual curve.");
}

template<class _Vector> 
void cvCurve<_Vector>::SolveSegmentSimpleBatch(int UNUSED_PARAM(numCurves), cvCurve** UNUSED_PARAM(curves), _Vector* UNUSED_PARAM(positionsOut), 
											   int UNUSED_PARAM(seg), float UNUSED_PARAM(t), _Vector** UNUSED_PARAM(tangentsOut))
{
	AssertMsg(0 , "A curve manager is needed to load curve types.");
}

template<class _Vector> 
void cvCurve<_Vector>::Move (int& UNUSED_PARAM(seg), float& UNUSED_PARAM(t), float UNUSED_PARAM(dist), float UNUSED_PARAM(slope)) const
{
	AssertMsg(0 , "Base cvCurve class does not define an actual curve.");
}

template<class _Vector> 
void cvCurve<_Vector>::PostInit()
{
	PostInitVerts();
}

template<class _Vector>
void cvCurve<_Vector>::Serialize(datSerialize& archive)
{
	archive << m_Type;
	archive << m_Looping;
	archive << m_Vertices;
}

template<class _Vector> 
void cvCurve<_Vector>::Load (fiAsciiTokenizer& token, int version)
{
	char name[128];
	int numVerts = 0;
	if (token.CheckToken("type:"))
	{
		token.GetInt();
	}
	token.GetToken(name, 128);
	char *numVertsStr = strrchr(name, '[');
	*numVertsStr = 0;
	numVertsStr++;
	numVerts = atoi(numVertsStr);
	AllocateVertices(numVerts);
	SetNumVertices(numVerts);

	char closedStr[16];

	const char* openCurly = "{";
	const char* closeCurly = "}";
	bool looping = false;
	if( version>1 )
	{
		// Get a "open" or "closed" token before the opening curly brace.
		token.GetToken(closedStr, 16);
		if( !stricmp(closedStr, "closed") )
			looping=true;
		else if( !stricmp(closedStr, "open") )
			looping=false;
		else
			Quitf( "Unrecognized token '%s'", closedStr );
	}

	token.GetDelimiter(openCurly);

	for(int v=0; v<numVerts; v++)
	{
		_Vector & vertex = GetVertex(v);
		GetVectorGeneric(token,vertex);
	}
	token.GetDelimiter(closeCurly);

	// Is this a looping curve?
	if( version>1 )
	{
		m_Looping = looping;
	}
	else
	{
		const float epsilon = 0.0001f;
		if( m_Vertices[numVerts-1].Dist2(m_Vertices[numVerts-2])<epsilon &&
			m_Vertices[numVerts-2].Dist2(m_Vertices[numVerts-3])<epsilon &&
			m_Vertices[numVerts-3].Dist2(m_Vertices[numVerts-4])<epsilon )
		{
			m_Looping = true;
			// Weird hack: set the last three verts to be equal to the
			// first three.  Somehow, this gives us the same curve you
			// see in Maya.
			for(int j=numVerts-3; j<numVerts; j++)
			{
				m_Vertices[j] = m_Vertices[j-numVerts+3];
			}
		}
		else
		{
			m_Looping = false;
		}
	}

	PostInit();
}

template<class _Vector> 
void cvCurve<_Vector>::Save (fiAsciiTokenizer& token, int version)
{
	token.StartLine();
	token.PutStr("curve[%d] ", m_Vertices.GetCount());

	if (version>1)
	{
		// Set a "open" or "closed" token before the opening curly brace.
		if (m_Looping)
		{
			token.Put("closed ");
		}
		else
		{
			token.Put("open ");
		}
	}
	token.EndLine();

	token.StartBlock();
	{
		int numVerts = m_Vertices.GetCount();
		for(int vertIndex=0;vertIndex<numVerts;vertIndex++)
		{
			token.StartLine();
			PutVectorGeneric(token,m_Vertices[vertIndex]);
			//token.Put(m_Vertices[vertIndex]);
			token.EndLine();
		}
	}
	token.EndBlock();
}

template<class _Vector> 
void cvCurve<_Vector>::SaveCurve (fiAsciiTokenizer & token, int version)
{
	// Write the header.
	token.StartLine();
	token.PutStr("type: %d", GetType());
	token.EndLine();

	Save(token,version);
}

template<class _Vector> 
cvCurve<_Vector>::cvCurve (datResource &rsc)
: m_Vertices(rsc, true)
{
}

#if	__DECLARESTRUCT

template<class _Vector> 
void	cvCurve<_Vector>::DeclareStruct	(datTypeStruct &s)
{
	STRUCT_BEGIN(cvCurve<_Vector>);
	STRUCT_FIELD(m_Vertices);
	STRUCT_FIELD(m_Type);
	STRUCT_FIELD(m_Looping);
	STRUCT_CONTAINED_ARRAY(pad);
	STRUCT_END();
}

#endif

}	// namespace rage

#include "curvenurbs.h"
#include "curvecatrom.h"

namespace rage {

template<class _Vector>
void cvCurve<_Vector>::Place(cvCurve<_Vector> *that,datResource & rsc )
{
	switch (that->GetType())
	{
	case CURVE_TYPE_NURBS:
		::new (that) cvCurveNurbs<_Vector>(rsc);
		break;
	case CURVE_TYPE_CATROM:
		::new (that) cvCurveCatRom(rsc);
		break;
	default:		
		Errorf("Unknown curve type %d\n", that->GetType());
		AssertMsg(0 , "cvCurve<_Vector>::Place - unsupported or unknown curve type");
		break;
	}
}

}	// namespace rage

#endif // ENABLE_UNUSED_CURVE_CODE

#endif // CURVE_CURVE_H
