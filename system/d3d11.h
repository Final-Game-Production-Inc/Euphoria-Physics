#if !__OPTIMIZED
#define D3D_DEBUG_INFO
#endif // !__OPTIMIZED

#if RSG_PC || RSG_DURANGO

#ifndef _SYSTEM_d3d11_H_
#define _SYSTEM_d3d11_H_

#pragma warning(disable:4668)
#if __D3D11_MONO_DRIVER
#include <d3d11_x.h>
#elif __D3D11_1
#include <d3d11_1.h>
#else
#include <d3d11.h>
#endif
#pragma warning(error:4668)

#endif // _SYSTEM_d3d11_H_

#endif // __WIN32PC
