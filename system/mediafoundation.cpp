#include "mediafoundation.h"

#if (RSG_PC || RSG_DURANGO) && !__RESOURCECOMPILER && (!defined(__RGSC_DLL) || !__RGSC_DLL)

// External libs
#if RSG_PC

#pragma comment(lib, "mf_vista")
#pragma comment(lib, "mfplat_vista")
#pragma comment(lib, "evr_vista")
#pragma comment(lib, "wmcodecdspuuid")
#pragma comment(lib, "Msdmo")

#elif RSG_DURANGO

#pragma comment(lib, "mf")
#pragma comment(lib, "mfplat")
#pragma comment(lib, "evr")

#endif

#pragma comment(lib, "mfreadwrite")
#pragma comment(lib, "strmiids")
#pragma comment(lib, "uuid")
#pragma comment(lib, "mfuuid")
#pragma comment(lib, "propsys")

#endif // (RSG_PC || RSG_DURANGO) && !__RESOURCECOMPILER && !__RGSC_DLL
