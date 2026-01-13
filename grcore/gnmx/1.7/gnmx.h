/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.700.081
* Copyright (C) 2011 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#if defined(SCE_GNM_HP3D)
#include <gnm_for_vsh/gnmx.h>
#else 
#if !defined(_SCE_GNMX_H)
#define _SCE_GNMX_H

#include "grcore/gnmx/common.h"

#if defined(SCE_GNMX_ENABLE_GFX_LCUE)
#include "grcore/gnmx/lwconstantupdateengine_cuetolcue.h"
#endif

#include "grcore/gnmx/computecontext.h"
#include "grcore/gnmx/constantupdateengine.h"
#include "grcore/gnmx/fetchshaderhelper.h"
#include "grcore/gnmx/gfxcontext.h"
#include "grcore/gnmx/helpers.h"
#include "grcore/gnmx/shaderbinary.h"
#include "grcore/gnmx/computequeue.h"

#endif // !defined(_SCE_GNMX_H)
#endif // defined(SCE_GNM_HP3D) 
