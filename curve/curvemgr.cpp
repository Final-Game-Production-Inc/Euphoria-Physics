//
// curve/curvemgr.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "curve.h"
#include "curvemgr.h"

#if ENABLE_UNUSED_CURVE_CODE

#include "file/asset.h"
#include "file/stream.h"
#include "file/token.h"

#include <string.h>

#define CURVE_FILE_VERSION_NUMBER 3

namespace rage {

void cvCurveMgr::CreateInstance ()
{
	sm_Inst = rage_new cvCurveMgr;
}

void cvCurveMgr::DestroyInstance ()
{
	if(sm_Inst)
	{
		delete sm_Inst;
	}

	sm_Inst = 0;
}

void cvCurveMgr::SetInstance (cvCurveMgr * pCurveMgr)
{
	sm_Inst = pCurveMgr;
}

cvCurveMgr::cvCurveMgr ()
{
    m_bResourced = false;
}


cvCurveMgr::~cvCurveMgr ()
{
	for (int curveIndex=0;curveIndex<m_CurveArray.GetCount();curveIndex++)
	{
		delete m_CurveArray[curveIndex];
	}
}

cvCurve<Vector3>* cvCurveMgr::CreateCurve (int curveType)
{
	switch (curveType)
	{
		case CURVE_TYPE_NURBS:
		{
			return rage_new cvCurveNurbs<Vector3>;
		}
		case CURVE_TYPE_CATROM:
		{
			return rage_new cvCurveCatRom;
		}
		default:
		{
			return rage_new cvCurve<Vector3>;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

bool cvCurveMgr::Load (const char* fileName, const char* fileExt)
{
    // Get the file name and extension.
	char file[64];
	if (!strcmp(fileName,""))
	{
		// No file name was specified, so use "layout".
		strcpy(file,"layout");
	}
	else
	{
		// Use the given file name.
		strcpy(file,fileName);
	}
	char extension[64];
	if (!strcmp(fileExt,""))
	{
		// No file extension was specified, so use "paths".
		strcpy(extension,"paths");
	}
	else
	{
		// Use the given file extension.
		strcpy(extension,fileExt);
	}

	// Open the file.
	fiStream* fileStream = ASSET.Open(file,extension);
	if (!fileStream)
	{
		return false;
	}

	// Parse the file.
	char curveFile[128];
	strcpy(curveFile,file);
	strcat(curveFile,extension);
	fiAsciiTokenizer token;
    token.Init(curveFile,fileStream);

    // Call the base init function
    Load(token);

    // Close the file
    fileStream->Close();
    fileStream = NULL;

    return true;

}

bool cvCurveMgr::Load (fiAsciiTokenizer & token)
{
	// Read the header:
	if (!token.CheckToken("version:"))
        return false;
	int version = token.GetInt();
	if (version != CURVE_FILE_VERSION_NUMBER)
	{
		Errorf("Invalid curve file expected version %d got version %d", CURVE_FILE_VERSION_NUMBER, version);
		return false;
	}

	token.MatchToken("numcurves:");
	int curveCount = token.GetInt();

    // Allocate the curves
    m_CurveArray.Resize(curveCount);
    for ( int i=0; i<m_CurveArray.GetCount(); i++ )
    {
        m_CurveArray[i] = NULL;
    }

	// Now read each curve:
	for( int i=0; i<m_CurveArray.GetCount(); i++ )
	{
        token.MatchToken("type:");
        int curveType = token.GetInt();

        m_CurveArray[i] = CreateCurve(curveType);
		m_CurveArray[i]->Load(token,version);                
	}

	return true;
}


////////////////////////////////////////////////////////////////////////////////

bool cvCurveMgr::Save (const char* fileName, const char* fileExt)
{
    // Get the file name and extension.
	char file[64];
	if (!strcmp(fileName,""))
	{
		// No file name was specified, so use "layout".
		strcpy(file,"layout");
	}
	else
	{
		// Use the given file name.
		strcpy(file,fileName);
	}
	char extension[64];
	if (!strcmp(fileExt,""))
	{
		// No file extension was specified, so use "paths".
		strcpy(extension,"paths");
	}
	else
	{
		// Use the given file extension.
		strcpy(extension,fileExt);
	}

	// Open the file.
	fiStream* fileStream = ASSET.Create(file,extension);
	if (!fileStream)
	{
		return false;
	}

	// Parse the file.
	char curveFile[128];
	strcpy(curveFile,file);
	strcat(curveFile,extension);
	fiAsciiTokenizer token;
    token.Init(curveFile,fileStream);

    // Call the base save function
    Save(token);

    // Close the file
    fileStream->Close();
    fileStream = NULL;

    return true;

}

bool cvCurveMgr::Save (fiAsciiTokenizer & token)
{
	// Write the header:
    int version = CURVE_FILE_VERSION_NUMBER;
    token.StartLine();
	token.PutStr("version: %d", version);
    token.EndLine();

    token.StartLine();
	token.PutStr("numcurves: %d", m_CurveArray.GetCount());
    token.EndLine();

	// Now write each curve:
	for( int i=0; i<m_CurveArray.GetCount(); i++ )
	{   
		m_CurveArray[i]->SaveCurve(token,version);
	}

	return true;
}

cvCurve<Vector3> * cvCurveMgr::AddCurve (fiAsciiTokenizer & token)
{
    // Make sure we do not add a curve to a resourced 
    AssertMsg(!m_bResourced , "Can not add curve to a resourced cvCurveMgr.\n");

    int version = CURVE_FILE_VERSION_NUMBER;

    // Check the type
    token.MatchToken("type:");
    int curveType = token.GetInt();

    // Create the curve
    cvCurve<Vector3> * curve = CreateCurve(curveType);
    curve->Load(token, version);

    // Add the curve to the manager
    m_CurveArray.Grow() = curve;

    return curve;
}

//
// Resource code (must be at the bottom of the file)
//
#include	"data/resourcehelpers.h"

IMPLEMENT_PLACE	(cvCurveMgr);

cvCurveMgr::cvCurveMgr (datResource &rsc)

: 

m_CurveArray(rsc, true)

{
	sm_Inst = this;
    m_bResourced = true;
}

#if	__DECLARESTRUCT

void	cvCurveMgr::DeclareStruct	(rage::datTypeStruct &s)
{
	STRUCT_BEGIN(mcLevelBounds);
	STRUCT_FIELD(m_CurveArray);
	STRUCT_FIELD(m_bResourced);
	STRUCT_CONTAINED_ARRAY(pad);
	STRUCT_END();
}

#endif

} // namespace rage

#endif // ENABLE_UNUSED_CURVE_CODE
