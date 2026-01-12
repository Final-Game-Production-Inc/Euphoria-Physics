#ifndef NM_TYPES_H
#define NM_TYPES_H

#include "ClientPlatformIncludes.h"

/**
 * Detect the compiler currently in use
 */

#if defined(_MSC_VER)
# define NM_COMPILER_MSVC
# if (_MSC_VER >= 1400)
# define NM_MSVC_8
# endif
#elif defined(__MWERKS__)
# define NM_COMPILER_METROWERKS
#elif defined(__SNC__)
# define NM_COMPILER_SNC
#elif defined(__GNUC__)
# define NM_COMPILER_GCC
#else
# error NM: No NM_COMPILER_... defined / detected
#endif


/**
 * Automatically configure NM_PLATFORM_? from compiler defines
 */

#if defined(WIN32) || defined(_WIN32)

#if defined(_WIN64)
# define NM_IA64
#endif

# if defined(_XBOX_VER)
#  ifndef NM_PLATFORM_X360
#   define NM_PLATFORM_X360
#  endif
# elif defined(_DURANGO)
#  ifndef NM_PLATFORM_DURANGO
#   define NM_PLATFORM_DURANGO
#  endif
#  define NM_IA64
# else
#  ifndef NM_PLATFORM_WIN32
#   define NM_PLATFORM_WIN32
#  endif
# endif

#elif defined(__PPU__)

# ifndef NM_PLATFORM_CELL_PPU
#  define NM_PLATFORM_CELL_PPU
# endif

#elif defined(__ORBIS__)

# ifndef NM_PLATFORM_ORBIS
#  define NM_PLATFORM_ORBIS
# endif
# define NM_IA64

#elif defined(__SPU__)

# ifndef NM_PLATFORM_CELL_SPU
#  define NM_PLATFORM_CELL_SPU
# endif

#elif defined(RVL_SDK) && defined(NM_COMPILER_METROWERKS)

# ifndef NM_PLATFORM_WII
#  define NM_PLATFORM_WII
# endif

#else

# error NM: No NM_PLATFORM_... defined / detected

#endif


/**
 * Alignment macros; as MSVC / GCC require the tag at different ends of a declaration
 * so;
 * NM_ALIGN_PREFIX(16) class Vec3 { ... } NM_ALIGN_SUFFIX(16)
 */
#if defined(NM_PLATFORM_X360)
# define NM_ALIGN_PREFIX(sz) __declspec(align(sz))
# define NM_ALIGN_SUFFIX(sz)
#elif defined(NM_PLATFORM_CELL_PPU) || defined(NM_PLATFORM_CELL_SPU)
# define NM_ALIGN_PREFIX(sz)
# define NM_ALIGN_SUFFIX(sz) __attribute__ ((aligned(sz)))
#else
# define NM_ALIGN_PREFIX(sz)
# define NM_ALIGN_SUFFIX(sz)
#endif


// For function arguments that aren't used
#define NM_UNUSED(x) /*x*/

/**
 * \brief Utility classes and functions.
 *
 * Utility classes and functions.
 *
 * See the \ref Maths module, \ref Strings module and \ref DataStreams documentation.
 */
namespace NMutils
{
  typedef rage::u8         NMU8;
  typedef rage::u16        NMU16;
  typedef rage::u32        NMU32;
  typedef rage::u64        NMU64;
  typedef rage::s8         NMI8;
  typedef rage::s16        NMI16;
  typedef rage::s32        NMI32;
  typedef rage::s64        NMI64;
}

// on MS compiler (pc, 360) we can use pragmas-in-macros, which let us write a tidy push/pop alias that is removed on other compilers
// without countless more #ifdefs in the rest of the source
#ifdef NM_MSVC_8

# define MSVCBeginWarningMacroBlock(_WarningMods) \
  __pragma(warning(push)) \
  __pragma(warning(_WarningMods))

# define MSVCEndWarningMacroBlock() \
  __pragma(warning(pop))

#else // all non-MS compilers

# define MSVCBeginWarningMacroBlock(_WarningMods)
# define MSVCEndWarningMacroBlock()

#endif // NM_MSVC_8

// define any warnings we want to muck with, so that we have meaningful names in the code rather than just mysterious numbers

// structure was padded due to __declspec(align())
#define MSVCWarning_StructurePadding  4324



#endif // NM_TYPES_H

