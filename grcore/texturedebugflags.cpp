//
// grcore/texturedebugflags.cpp
//
// Copyright (C) 2013 Rockstar Games.  All Rights Reserved.
//

#include "texturedebugflags.h"

#if __BANK || defined(ASSET_ANALYSIS_TOOL)

namespace rage {

std::string GetTextureConversionFlagsString(u8 flags)
{
	std::string str = "";

	switch (flags)
	{
	case TEXTURE_CONVERSION_FLAG_PROCESSED         : break; // leave string blank
	case TEXTURE_CONVERSION_FLAG_SKIPPED           : str = "SKIPPED"; break;
	case TEXTURE_CONVERSION_FLAG_FAILED_PROCESSING : str = "FAILED"; break;
	default:
		{
			if (flags == (TEXTURE_CONVERSION_FLAG_INVALID_METADATA | TEXTURE_CONVERSION_FLAG_PROCESSED))
			{
				str = "BADMETA";
			}
			else // other combinations
			{
				str = "[";

				if (flags & TEXTURE_CONVERSION_FLAG_PROCESSED        ) { str += "P"; }
				if (flags & TEXTURE_CONVERSION_FLAG_SKIPPED          ) { str += "S"; }
				if (flags & TEXTURE_CONVERSION_FLAG_FAILED_PROCESSING) { str += "F"; }
				if (flags & TEXTURE_CONVERSION_FLAG_INVALID_METADATA ) { str += "I"; }
				if (flags & TEXTURE_CONVERSION_FLAG_OPTIMISED_DXT    ) { str += "O"; }
				if (flags & TEXTURE_CONVERSION_FLAG_NO_PROCESSING    ) { str += "N"; }

				str += "]";
			}
		}
	}

	return str;
}

std::string GetTextureTemplateTypeString(u16 templateType)
{
	std::string str = "";

	if (templateType & TEXTURE_TEMPLATE_TYPE_VERSION_MASK)
	{
		switch (templateType & TEXTURE_TEMPLATE_TYPE_MASK)
		{
		case TEXTURE_TEMPLATE_TYPE_UNKNOWN           : str = "UNKNOWN"; break;
		case TEXTURE_TEMPLATE_TYPE_DEFAULT           : str = "DEFAULT"; break;
		case TEXTURE_TEMPLATE_TYPE_TERRAIN           : str = "TERRAIN"; break;
		case TEXTURE_TEMPLATE_TYPE_CLOUDDENSITY      : str = "CLOUDDENSITY"; break;
		case TEXTURE_TEMPLATE_TYPE_CLOUDNORMAL       : str = "CLOUDNORMAL"; break;
		case TEXTURE_TEMPLATE_TYPE_CABLE             : str = "CLOUD"; break;
		case TEXTURE_TEMPLATE_TYPE_FENCE             : str = "FENCE"; break;
		case TEXTURE_TEMPLATE_TYPE_ENVEFF            : str = "ENVEFF"; break;
		case TEXTURE_TEMPLATE_TYPE_SCRIPT            : str = "SCRIPT"; break;
		case TEXTURE_TEMPLATE_TYPE_WATERFLOW         : str = "WATERFLOW"; break;
		case TEXTURE_TEMPLATE_TYPE_WATERFOAM         : str = "WATERFOAM"; break;
		case TEXTURE_TEMPLATE_TYPE_WATERFOG          : str = "WATERFOG"; break;
		case TEXTURE_TEMPLATE_TYPE_WATEROCEAN        : str = "WATEROCEAN"; break;
		case TEXTURE_TEMPLATE_TYPE_WATER             : str = "WATER"; break;
		case TEXTURE_TEMPLATE_TYPE_FOAMOPACITY       : str = "FOAMOPACITY"; break;
		case TEXTURE_TEMPLATE_TYPE_FOAM              : str = "FOAM"; break;
		case TEXTURE_TEMPLATE_TYPE_DIFFUSEMIPSHARPEN : str = "DIFFUSEMIPSHARPEN"; break;
		case TEXTURE_TEMPLATE_TYPE_DIFFUSEDETAIL     : str = "DIFFUSEDETAIL"; break;
		case TEXTURE_TEMPLATE_TYPE_DIFFUSEDARK       : str = "DIFFUSEDARK"; break;
		case TEXTURE_TEMPLATE_TYPE_DIFFUSEALPHAOPAQUE: str = "DIFFUSEALPHAOPAQUE"; break;
		case TEXTURE_TEMPLATE_TYPE_DIFFUSE           : str = "DIFFUSE"; break;
		case TEXTURE_TEMPLATE_TYPE_DETAIL            : str = "DETAIL"; break;
		case TEXTURE_TEMPLATE_TYPE_NORMAL            : str = "NORMAL"; break;
		case TEXTURE_TEMPLATE_TYPE_SPECULAR          : str = "SPECULAR"; break;
		case TEXTURE_TEMPLATE_TYPE_EMISSIVE          : str = "EMISSIVE"; break;
		case TEXTURE_TEMPLATE_TYPE_TINTPALETTE       : str = "TINTPALETTE"; break;
		case TEXTURE_TEMPLATE_TYPE_SKIPPROCESSING    : str = "SKIPPROCESSING"; break;
		case TEXTURE_TEMPLATE_TYPE_DONOTOPTIMIZE     : str = "DONOTOPTIMIZE"; break;
		case TEXTURE_TEMPLATE_TYPE_TEST              : str = "TEST"; break;
		default                                      : str = "?"; break;
		}

		if (templateType & TEXTURE_TEMPLATE_TYPE_FLAG_NOT_HALF ) { str += "+NH"; }
		if (templateType & TEXTURE_TEMPLATE_TYPE_FLAG_HD_SPLIT ) { str += "+HD"; }
		if (templateType & TEXTURE_TEMPLATE_TYPE_FLAG_FULL     ) { str += "+FULL"; }
		if (templateType & TEXTURE_TEMPLATE_TYPE_FLAG_MAPS_HALF) { str += "+MH"; }
	}

	return str;
}

} // namespace rage

#endif // __BANK || defined(ASSET_ANALYSIS_TOOL)
