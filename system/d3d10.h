#if !__OPTIMIZED
#define D3D_DEBUG_INFO
#endif // !__OPTIMIZED

#if __WIN32PC

#ifndef _SYSTEM_d3d10_H_
#define _SYSTEM_d3d10_H_

#pragma warning(disable:4668)
#include <d3d10_1.h>
#pragma warning(error:4668)

#endif // _SYSTEM_d3d10_H_

#endif // __WIN32PC
