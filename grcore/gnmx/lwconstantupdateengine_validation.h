#include <sdk_version.h>
#if   (SCE_ORBIS_SDK_VERSION & 0xfff00000) == 0x01600000
#	include "1.6/lwconstantupdateengine_validation.h"
#elif (SCE_ORBIS_SDK_VERSION & 0xfff00000) == 0x01700000
#	include "1.7/lwconstantupdateengine_validation.h"
#else
#	error "Orbis SDK version not currently supported"
#endif
