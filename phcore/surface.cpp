//
// phcore/surface.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "surface.h"

#include "bank/bank.h"
#include "data/resource.h"
#include "file/asset.h"
#include "file/token.h"
#include "string/string.h"
#include "system/memory.h"

namespace rage {

const int phSurface::ClassType = phSurface::SURFACE;
const char * phSurface::ClassTypeString = "SURFACE";

phSurface::phSurface() : phMaterial()
{
	m_Type = SURFACE;
	Grip=1.0f;
	Drag=0.0f;
	Width=1.0f;
	Height=0.0f;
	Space=0.0f;
	Depth=0.0f;
	PtxIndex[0]=-1;
	PtxIndex[1]=-1;
	PtxThreshold[0]=0.25f;
	PtxThreshold[1]=0.50f;
}


phSurface::~phSurface ()
{
}

bool phSurface::LoadData (fiAsciiTokenizer& token)
{
	bool parsed = phMaterial::LoadData(token);

	if (token.CheckIToken("grip:"))
	{
		Grip = token.GetFloat();
		parsed = true;
	}

	if (token.CheckIToken("drag:"))
	{
		Drag = token.GetFloat();
		parsed = true;
	}

	if (token.CheckIToken("width:"))
	{
		Width = token.GetFloat();
		parsed = true;
	}

	if (token.CheckIToken("height:"))
	{
		Height = token.GetFloat();
		parsed = true;
	}

	if (token.CheckIToken("Space:"))
	{
		Space = token.GetFloat();
		parsed = true;
	}

	if (token.CheckIToken("depth:"))
	{
		Depth = token.GetFloat();
		parsed = true;
	}

	if (token.CheckIToken("ptxindex:"))
	{
		PtxIndex[0] = (s16)token.GetInt();
		PtxIndex[1] = (s16)token.GetInt();
		parsed = true;
	}

	if (token.CheckIToken("ptxthreshold:"))
	{
		PtxThreshold[0] = token.GetFloat();
		PtxThreshold[1] = token.GetFloat();
		parsed = true;
	}
	return parsed;
}

void phSurface::SaveData(fiAsciiTokenizer &t) const
{
	phMaterial::SaveData(t);

	t.PutDelimiter("\tgrip: ");
	t.Put(Grip);
	t.PutDelimiter("\n");

	t.PutDelimiter("\tdrag: ");
	t.Put(Drag);
	t.PutDelimiter("\n");

	t.PutDelimiter("\twidth: ");
	t.Put(Width);
	t.PutDelimiter("\n");

	t.PutDelimiter("\theight: ");
	t.Put(Height);
	t.PutDelimiter("\n");

	t.PutDelimiter("\tspace: ");
	t.Put(Space);
	t.PutDelimiter("\n");

	t.PutDelimiter("\tdepth: ");
	t.Put(Depth);
	t.PutDelimiter("\n");

	t.PutDelimiter("\tptxindex: ");
	t.Put(PtxIndex[0]);
	t.Put(" ");
	t.Put(PtxIndex[1]);
	t.PutDelimiter("\n");

	t.PutDelimiter("\tptxthreshold: ");
	t.Put(PtxThreshold[0]);
	t.Put(" ");
	t.Put(PtxThreshold[1]);
	t.PutDelimiter("\n");
}

#if __BANK
void phSurface::AddWidgets(bkBank& B) {
	phMaterial::AddWidgets(B);

	B.AddSlider("Grip",&Grip,0.0f,100.0f,0.01f);
	B.AddSlider("Drag",&Drag,0.0f,100.0f,0.001f);
	B.AddSlider("Width",&Width,0.01f,100.0f,0.1f);
	B.AddSlider("Height",&Height,-1.0f,1.0f,0.01f);
	B.AddSlider("Space",&Space,0.0f,100.0f,0.1f);
	B.AddSlider("Depth",&Depth,0.0f,10.0f,0.01f);
	B.AddSlider("PtxIndex1",&PtxIndex[0],-1,32767,1);
	B.AddSlider("PtxThreshold1",&PtxThreshold[0],0,1,0.01f);
	B.AddSlider("PtxIndex2",&PtxIndex[1],-1,32767,1);
	B.AddSlider("PtxThreshold2",&PtxThreshold[1],0,1,0.01f);
}
#endif


phSurface::phSurface(datResource & rsc) : phMaterial(rsc)
{
}

} // namespace rage


// EOF
