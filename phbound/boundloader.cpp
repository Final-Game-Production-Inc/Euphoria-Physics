//
// phbound/boundloader.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "bound.h"
#include "boundbox.h"
#include "boundbvh.h"
#include "boundcapsule.h"
#include "boundcomposite.h"
#include "boundcurvedgeom.h"
#include "boundcylinder.h"
#include "boundgeom.h"
#include "boundgrid.h"
#include "boundribbon.h"
#include "boundsphere.h"
#include "boundsurface.h"
#include "boundtaperedcapsule.h"
#include "bounddisc.h"

#include "data/resource.h"
#include "data/resourcehelpers.h"
#include "file/asset.h"
#include "file/token.h"
#include "paging/rscbuilder.h"
#include "string/string.h"
#include "system/memory.h"

namespace rage {

Functor1Ret<phBound*,const char*>	phBound::sm_CustomConstructor = NULL;

phBound* phBound::CreateOfType (int boundType)
{
	switch (boundType)
	{
		case SPHERE:
		{
			return rage_new phBoundSphere;
		}
		case CAPSULE:
		{
			return rage_new phBoundCapsule;
		}
		case BOX:
		{
			return rage_new phBoundBox;
		}
		case GEOMETRY:
		{
			return rage_new phBoundGeometry;
		}
#if USE_GEOMETRY_CURVED
		case GEOMETRY_CURVED:
		{
			return rage_new phBoundCurvedGeometry;
		}
#endif
		case DISC:
		{
			return rage_new phBoundDisc;
		}
		case CYLINDER:
		{
			return rage_new phBoundCylinder;
		}
#if USE_GRIDS
		case GRID:
		{
			return rage_new phBoundGrid;
		}
#endif
#if USE_RIBBONS
		case RIBBON:
		{
			return rage_new phBoundRibbon;
		}
#endif
		case BVH:
		{
			return rage_new phBoundBVH;
		}
		case COMPOSITE:
		{
			return rage_new phBoundComposite;
		}
	}

	return NULL;
}

//MakeFunctorRet(testClassInst,&testClass::FuncRet1);
//int retValue=testFunc2("look at me!");  // will print "in Func1: look at me!"

phBound *phBound::Load(const char * filename)
{
#if __RESOURCECOMPILER
	strcpy(s_currentBoundFilename, filename);
#endif // __RESOURCECOMPILER
#if __TOOL
	AssertMsg(!pgRscBuilder::IsBuilding(), "Resources cannot be generated in a __TOOL build");
#endif // __TOOL
	phBound * bound = 0;
	fiStream * file;
	FileVersion version;

	bool folderPushed = false;

	char normalizeBuffer[RAGE_MAX_PATH];
	const char* normalized = normalizeBuffer;
	StringNormalize(normalizeBuffer, filename, 256);

	if (strchr(normalized, '/'))
	{
		char path[RAGE_MAX_PATH];
		fiAssetManager::RemoveNameFromPath(path, RAGE_MAX_PATH, normalized);
		ASSET.PushFolder(path);

		folderPushed = true;
		normalized = fiAssetManager::FileName(normalized);
	}

	// Try to find the bound file with the given name in the bound folder.
	file = ASSET.Open(normalized,"bnd",true,true);
	if (!file)
	{
		// Try to find the bound file named "bound" in the folder with the given file name.
		char filenameBound[RAGE_MAX_PATH];
		strcpy(filenameBound, normalized);
		strcat(filenameBound, "/bound");
		file = ASSET.Open(filenameBound,"bnd",true,true);
	}

	if (!file)
	{
		// Try to find the bound file with the given name in the folder with the same name.
		char filenameBound[RAGE_MAX_PATH];
		strcpy(filenameBound, normalized);
		strcat(filenameBound, "/");
		strcat(filenameBound, normalized);
		file = ASSET.Open(filenameBound,"bnd",true,true);
	}

	if (!file)
	{
		// Try to find the bound file with the given file name in the bound folder in the folder
		// with the given file name.
		char filenameBound[RAGE_MAX_PATH];
		strcpy(filenameBound,normalized);
		strcat(filenameBound,"/bound/bound");
		file = ASSET.Open(filenameBound,"bnd",true,true);
	}

//	int mem1 = sysMemGetMemoryUsed();

	if (file)
	{
		// ASCII bound exists
		fiAsciiTokenizer token;

		token.Init(normalized,file);

		const int BUFFER_SIZE = 30;
		char buffer[BUFFER_SIZE];

		token.MatchToken("version:");
		token.GetToken(buffer,BUFFER_SIZE);

		if (stricmp(buffer,"1.01")==0)
		{
			version = VERSION_101;
		}
		else if (stricmp(buffer,"1.1")==0 || stricmp(buffer,"1.10")==0)
		{
			version = VERSION_110;
		}
		else
		{
			if (phBound::MessagesEnabled())
			{
				Warningf("phBound:Load - unknown version");
			}

			if (folderPushed)
			{
				ASSET.PopFolder();
			}

			return NULL;
		}

		if (version==VERSION_110)
		{
			token.MatchToken("type:");
			token.GetToken(buffer,BUFFER_SIZE);

			if (stricmp(buffer,"SPHERE") == 0)
			{
				bound = rage_new phBoundSphere;
			}
			else if (stricmp(buffer,"HOTDOG") == 0)
			{
				bound = rage_new phBoundCapsule;
			}
			else if (stricmp(buffer,"CAPSULE") == 0)
			{
				bound = rage_new phBoundCapsule;
			}
#if USE_TAPERED_CAPSULE
			else if (stricmp(buffer,"TAPERED_CAPSULE")==0)
			{
				bound = rage_new phBoundTaperedCapsule;
			}
#endif
			else if (stricmp(buffer,"BOX") == 0)
			{
				bound = rage_new phBoundBox;
			}
			else if (stricmp(buffer,"GEOMETRY") == 0)
			{
				bound = rage_new phBoundGeometry;
			}
#if USE_GEOMETRY_CURVED
			else if (stricmp(buffer,"GEOMETRY_CURVED") == 0)
			{
				bound = rage_new phBoundCurvedGeometry;
			}
#endif
			else if (stricmp(buffer, "DISC") == 0)
			{
				bound = rage_new phBoundDisc;
			}
			else if (stricmp(buffer, "CYLINDER") == 0)
			{
				bound = rage_new phBoundCylinder;
			}
			else if (stricmp(buffer,"OCTREE") == 0)
			{
			    bound = rage_new phBoundBVH;
			}
			else if (stricmp(buffer,"QUADTREE") == 0)
			{
				bound = rage_new phBoundBVH;
			}
#if USE_GRIDS
			else if (stricmp(buffer,"OTGRID") == 0 ||
                     stricmp(buffer,"GRID") == 0)
			{
				bound = rage_new phBoundGrid;
			}
#endif
			else if (stricmp(buffer,"COMPOSITE") == 0)
			{
				bound = rage_new phBoundComposite;
			}
#if USE_RIBBONS
			else if (stricmp(buffer,"RIBBON") == 0)
			{
				bound = rage_new phBoundRibbon;
			}
#endif
			else if (stricmp(buffer,"BVH") == 0)
			{
				bound = rage_new phBoundBVH;
			}
#if USE_SURFACES
			else if (stricmp(buffer, "SURFACE") == 0)
			{
				bound = rage_new phBoundSurface;
			}
#endif
			else
			{
				if(!sm_CustomConstructor || (bound = sm_CustomConstructor(buffer))==0)
				{
					/*
					if (phBound::MessagesEnabled())
					{
						Warningf("phBound:Load - unknown type '%s'",buffer);
					}
					*/
					Errorf("phBound:Load - unknown type '%s'",buffer);

					if (folderPushed)
					{
						ASSET.PopFolder();
					}

					return NULL;
				}
			}

			if (bound && !bound->LoadData(token,version))
			{
				delete bound;
				bound = NULL;
			}

			file->Close();
		}
		else
		{
			if (phBound::MessagesEnabled())
			{
				Warningf("phBound:Load - unknown file version");
			}

			bound = NULL;
		}

	}
	else if ((file = ASSET.Open(normalized,"bbnd",true,true))!=NULL)
	{
		// Binary tokenizer no longer supported.
		Assert(0);
	}
	else
	{
		if (GetBoundFlag(WARN_MISSING_BOUNDS))
		{
			Warningf("phBound:Load - couldn't open filename '%s'",filename);
		}
		bound = NULL;
	}

	if (folderPushed)
	{
		ASSET.PopFolder();
	}

//	int mem2 = sysMemGetMemoryUsed();
//	Displayf("Memory usage for bound '%s' is %d bytes.", filename, mem2 - mem1);

	return bound;
}


/////////////////////////////////////////////////////////////////
// load / save, data only

phBound * phBound::Load (fiAsciiTokenizer & token)
{
#if __TOOL
	AssertMsg(!pgRscBuilder::IsBuilding(), "Resources cannot be generated in a __TOOL build");
#endif // __TOOL

	const int BUFFER_SIZE = 30;
	char buffer[BUFFER_SIZE];

	token.MatchToken("version:");
	token.GetToken(buffer,BUFFER_SIZE);

	FileVersion version;
	if (stricmp(buffer,"1.01")==0)
	{
		version = VERSION_101;
	}
	else if (stricmp(buffer,"1.1")==0 || stricmp(buffer,"1.10")==0)
	{
		version = VERSION_110;
	}
	else
	{
		if (phBound::MessagesEnabled())
		{
			Warningf("phBound:Load - unknown version");
		}

		return NULL;
	}

	if (version==VERSION_110)
	{
		token.MatchToken("type:");
		token.GetToken(buffer,BUFFER_SIZE);

		sysMemUseMemoryBucket bucket(sm_MemoryBucket);
		phBound* bound=NULL;

		if (stricmp(buffer,"SPHERE") == 0)
		{
			bound = rage_new phBoundSphere;
		}
		else if (stricmp(buffer,"HOTDOG") == 0)
		{
			bound = rage_new phBoundCapsule;
		}
		else if (stricmp(buffer,"CAPSULE") == 0)
		{
			bound = rage_new phBoundCapsule;
		}
		else if (stricmp(buffer,"BOX") == 0)
		{
			bound = rage_new phBoundBox;
		}
		else if (stricmp(buffer,"GEOMETRY") == 0)
		{
			bound = rage_new phBoundGeometry;
		}
#if USE_GEOMETRY_CURVED
		else if (stricmp(buffer,"GEOMETRY_CURVED") == 0)
		{
			bound = rage_new phBoundCurvedGeometry;
		}
#endif
		else if (stricmp(buffer,"DISC") == 0)
		{
			bound = rage_new phBoundDisc;
		}
		else if (stricmp(buffer,"CYLINDER") == 0)
		{
			bound = rage_new phBoundCylinder;
		}
		else if (stricmp(buffer,"OCTREE") == 0)
		{
			bound = rage_new phBoundBVH;
		}
		else if (stricmp(buffer,"QUADTREE") == 0)
		{
			bound = rage_new phBoundBVH;
		}
#if USE_GRIDS
		else if (stricmp(buffer,"OTGRID") == 0 ||
                 stricmp(buffer,"GRID") == 0)
		{
			bound = rage_new phBoundGrid;
		}
#endif
		else if (stricmp(buffer,"COMPOSITE") == 0)
		{
			bound = rage_new phBoundComposite;
		}
#if USE_RIBBONS
		else if (stricmp(buffer,"RIBBON") == 0)
		{
			bound = rage_new phBoundRibbon;
		}
#endif
		else if (stricmp(buffer,"BVH") == 0)
		{
			bound = rage_new phBoundBVH;
		}
		else
		{
			if (phBound::MessagesEnabled())
			{
				Warningf("phBound:Load - unknown type '%s'",buffer);
			}

			return NULL;
		}

		if (bound && !bound->LoadData(token,version))
		{
			delete bound;
			bound = NULL;
		}

		return bound;
	}
	else
	{
		if (phBound::MessagesEnabled())
		{
			Warningf("phBound:Load - unknown file version");
		}

		return NULL;
	}
}


////////////////////////////////////////////////////////////////
// save helper functions

void WriteHeader_110 (fiAsciiTokenizer & token, phBound * boundToSave)
{
	token.PutDelimiter("version: 1.10\n");

	switch (boundToSave->GetType())
	{
	case phBound::SPHERE:
		token.Put ("type: sphere\n");
		break;
	case phBound::CAPSULE:
		token.Put ("type: capsule\n");
		break;
	case phBound::BOX:
		token.Put ("type: box\n");
		break;
	case phBound::GEOMETRY:
		token.Put ("type: geometry\n");
		break;
#if USE_GEOMETRY_CURVED
	case phBound::GEOMETRY_CURVED:
		token.Put ("type: geometry_curved\n");
		break;
#endif
	case phBound::DISC:
		token.Put ("type: disc\n");
		break;
	case phBound::CYLINDER:
		token.Put ("type: cylinder\n");
		break;
#if USE_GRIDS
	case phBound::OCTREEGRID:
		token.Put ("type: grid\n");
		break;
#endif
#if USE_RIBBONS
	case phBound::RIBBON:
		token.Put ("type: ribbon\n");
		break;
#endif
	case phBound::COMPOSITE:
		token.Put ("type: composite\n");
		break;
	case phBound::BVH:
		token.Put ("type: bvh\n");
		break;
#if USE_SURFACES
	case phBound::SURFACE:
		token.Put ("type: surface\n");
		break;
#endif
	default:
		Quitf(ERR_PHY_BOUND,"WriteHeader_110: unknown bound type");
		break;
	}
}


#if !__FINAL && !IS_CONSOLE
bool phBound::Save (const char * filename, phBound * boundToSave, FileVersion version, FileMode mode)
{
	Assert(filename!=NULL && boundToSave!=NULL);

	// write the header (version/type)
	// and pass to the file save function
	fiStream * file;
	file = ASSET.Create(filename,(mode==Binary)?"bbnd":"bnd",false);
	if (!file)
	{
		if (phBound::MessagesEnabled())
		{
			Warningf("phBound:Save - couldn't open file %s",filename);
		}

		return false;
	}

	bool retVal;

	if (mode==ASCII)
	{
		fiAsciiTokenizer token;
		token.Init(filename,file);
		retVal = Save(token, boundToSave, version);
		file->Close( );
		return retVal; 
	}
	else // (mode==Binary)
	{
		// Binary tokenizer no longer supported.
		Assert(0);
		return 0; 
	}
}


bool phBound::Save (fiAsciiTokenizer & token, phBound * boundToSave, FileVersion version)
{
	// write the header (version/type)
	switch (version)
	{
	case VERSION_110:
		WriteHeader_110(token,boundToSave);
		return boundToSave->SaveData(token,version);
	default:
		if (phBound::MessagesEnabled())
		{
			Warningf("phBound:Save ASCII - unknown version");
		}

		return false;
	}
}


#endif	// end of #if !__FINAL && !IS_CONSOLE

////////////////////////////////////////////////////////////////
// resource page-in

void phBound::Place(void *that,datResource & rsc )
{
	VirtualConstructFromPtr(rsc, (phBound*) that);
}


void phBound::ResourcePageIn (datResource & rsc)
{
	phPolygon::EnableResourceConstructor(true);
	ResourcePageInDoWork(rsc);
	phPolygon::EnableResourceConstructor(false);

}


void phBound::ResourcePageInDoWork (datResource & rsc)
{
	phBound *bound = (phBound*) rsc.GetBase();

	switch (bound->m_Type)
	{
	case SPHERE:
		::new (bound) phBoundSphere(rsc);
		break;
	case CAPSULE:
		::new (bound) phBoundCapsule(rsc);
		break;
	case BOX:
		::new (bound) phBoundBox(rsc);
		break;
	case GEOMETRY:
		::new (bound) phBoundGeometry(rsc);
		break;
#if USE_GEOMETRY_CURVED
	case GEOMETRY_CURVED:
		::new (bound) phBoundCurvedGeometry(rsc);
		break;
#endif
	case DISC:
		::new (bound) phBoundDisc(rsc);
		break;
	case CYLINDER:
		::new (bound) phBoundCylinder(rsc);
		break;
#if USE_GRIDS
	case GRID:
		::new (bound) phBoundGrid(rsc);
		break;
#endif
#if USE_RIBBONS
	case RIBBON:
		::new (bound) phBoundRibbon(rsc);
		break;
#endif
	case COMPOSITE:
		::new (bound) phBoundComposite(rsc);
		break;
	default:
		Errorf("Unsupported or unknown bound type %d\n", bound->GetType());
		AssertMsg(0 , "phBound::ResourcePageInDoWork - unsupported or unknown bound type");
		break;
	}
}

} // namespace rage
