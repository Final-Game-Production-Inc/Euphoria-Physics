#undef _SCE_GNMX_COMPUTECONTEXT_METHODS_H
#include <gnmx\computecontext_methods.h>

inline void reset()
{
	m_dcb.resetBuffer();
	swapBuffers();
}

