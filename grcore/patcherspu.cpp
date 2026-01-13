// 
// grcore/patcherspu.cpp 
// 
// Copyright (C) 1999-2007 Rockstar Games.  All Rights Reserved. 
// 

#if __SPU
#define grcTexture spuTexture
#define g_VertexShaderInputs (pSpuGcmState->VertexShaderInputs)
#include "grmodel/grmodelspu.h"			// HACK
#include "effect_config.h"
#include "system/magicnumber.h"
#include "system/taskheader.h"

#include <spu_printf.h>

#include "grcore/grcorespu_gcmcallback.h"

#include "profile/element.h"
#include "grcore/effectcache.cpp"
#include "grcore/effect_common.cpp"
#include "grcore/effect_gcm.cpp"
#if SPU_GCM_FIFO
#include "grcore/edge_jobs.cpp"
#include "grcore/grcorespu.cpp"
#include "grmodel/grmodelspu.cpp"	// HACK HACK
#endif

#include "data/datcompress.cpp"

const int TAG = 8;

#define dprintf(x)	// spu_printf x

#define GCM_CONTEXT	(&ctxt)

using namespace rage;

namespace rage
{
	extern bool g_PatcherModifiedData;
}

void patcherspu (sysTaskParameters& taskParams) 
{
#if SPU_GCM_FIFO

#include "grcorespu_header.h"

#include "patcherspu_case.h"

#include "grcorespu_case.h"

#include "grcorespu_gamestate_case.h"

#include "grmodel/grmodelspu_case.h"		// HACK

#include "grcorespu_footer.h"

#endif
}

#endif		// __SPU


