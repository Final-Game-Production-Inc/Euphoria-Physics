//
// phcore/surface.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHCORE_SURFACE_H
#define PHCORE_SURFACE_H

#include "material.h"

namespace rage {

class fiAsciiTokenizer;

class bkBank;

/*
PURPOSE
phSurface is a material class for driving surfaces, with information for interactions with wheels.
<FLAG Component>
*/
class phSurface : public phMaterial {
public:
			phSurface	();
			phSurface	(datResource &rsc);
	virtual ~phSurface ();

	enum { SURFACE=(phMaterial::MATERIAL+1) };		// class type

	void	SetGrip			(float val)	{ Grip = val; }
	void	SetDrag			(float val)	{ Drag = val; }
	void	SetWidth		(float val)	{ Width = val; }
	void	SetHeight		(float val)	{ Height = val; }
	void	SetSpace		(float val)	{ Space = val; }
	void	SetDepth		(float val)	{ Depth = val; }
	void	SetPtxIndex		(int i, s16 val) { PtxIndex[i] = val;}
	void	SetPtxThreshold (int i, float val) { PtxThreshold[i] = val;}

	float	GetGrip			() const	{return Grip;}
	float	GetDrag			() const	{return Drag;}
	float	GetWidth		() const	{return Width;}
	float	GetHeight		() const	{return Height;}
	float	GetSpace		() const	{return Space;}
	float	GetDepth		() const	{return Depth;}
	int		GetPtxIndex (int iType) const	{return PtxIndex[iType];}
	float	GetPtxThreshold (int iType) const	{return PtxThreshold[iType];}


	// PURPOSE: Call the phMaterial.LoadData(), then parse phSurface tuning data from the tokenizer.
	// RETURN: true if any data was parsed
	// NOTES:
	//  If the physics material tune file has the potential for tuning data to appear in a different order than is written
	//  by SaveData() (perhaps hand edited), this function should be called by the application's parsing loop 
	//  until it returns false or there are no more tokens to parse.
	virtual bool LoadData	(fiAsciiTokenizer &t);

	virtual void SaveData	(fiAsciiTokenizer &t) const;

#if __BANK
	void AddWidgets (bkBank& bank);
#endif

protected:
	static const int ClassType;												// id of this class type
	static const char* ClassTypeString;										// name of this class type

	float	Grip;
	float	Drag;
	float	Width;
	float	Height;
	float	Space;
	float	Depth;
	s16		PtxIndex[2];
	float	PtxThreshold[2];
};

} // namespace rage

#endif	// PHCORE_SURFACE_H
