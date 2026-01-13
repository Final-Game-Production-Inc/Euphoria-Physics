//#if !__OPTIMIZED

// Be sure to undefine _DEBUG if it wasn't defined in the first place
#ifndef _DEBUG
	#define GAME_IS_NOT_USING_DEBUG
#endif // _DEBUG

#ifndef USE_D3D_DEBUG_LIBS
	#error "Please include system/xtl.h first"
#endif

#if USE_D3D_DEBUG_LIBS || !__OPTIMIZED
	#define D3D_DEBUG_INFO
	#ifndef _DEBUG
		#define _DEBUG 1
	#endif // _DEBUG
#endif // USE_D3D_DEBUG_LIBS
#include <d3d9.h>

#ifdef GAME_IS_NOT_USING_DEBUG
	#undef _DEBUG
	#undef GAME_IS_NOT_USING_DEBUG
#endif // GAME_IS_NOT_USING_DEBUG
