#if defined(_XDK_VER) && _XDK_VER < 2638
#pragma message("Your Xenon SDK install is incorrect.  Only _XDK_VER>=2638 are supported.")
#pragma message("Please edit lines 1 and 8 of this file to match your actual installation.")
#pragma message("We do not have access to $XEDK/include/win32/xdk.h on normal PC builds so this has to")
#pragma message("be configured manually for now.")
#error "Your Xenon SDK install is incorrect.  Only _XDK_VER>=2638 are supported"
#elif !defined(_XDK_VER)
#define _XDK_VER 2920		// If you get a redefined symbol here, edit it to match your XeDK.
#endif
