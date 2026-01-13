/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.600.051
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#if !defined(_SCE_GNMX_LCUE_COMPUTECONTEXT_METHODS_H)
#define _SCE_GNMX_LCUE_COMPUTECONTEXT_METHODS_H

#if !defined(DOXYGEN_IGNORE)

#undef _SCE_GNMX_COMPUTECONTEXT_METHODS_H
#include "grcore/gnmx/computecontext_methods.h"
#undef _SCE_GNMX_COMPUTECONTEXT_METHODS_H

#endif

/** @brief Resets the Gnm::DispatchCommandBuffer, and swaps buffers in the LCUE for a new frame.
			
	Call this at the beginning of every frame.

	The Gnm::DispatchCommandBuffer will be reset to empty (<c>m_cmdptr = m_beginptr</c>)
*/
void reset()
{
	m_dcb.resetBuffer();
	swapBuffers();
}

#endif // _SCE_GNMX_LCUE_COMPUTECONTEXT_METHODS_H