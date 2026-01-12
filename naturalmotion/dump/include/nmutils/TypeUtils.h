#ifndef NM_TYPE_UTILS_H
#define NM_TYPE_UTILS_H

#include "nmutils/NMTypes.h"

namespace NMutils
{
  #define XOR_BYTESWAP(a, b) (((a) == (b)) || (((a) ^= (b)), ((b) ^= (a)), ((a) ^= b)))

  // a generic way to swap bits based on type size
  template<typename T> T endianSwap(T t)
  {
    T swapReturn = t;
    unsigned char* c = reinterpret_cast<unsigned char*>(&swapReturn);
    for (size_t s = 0; s < sizeof(t) / 2; ++s) {
      XOR_BYTESWAP(c[s], c[sizeof(t) - s - 1]);
    }
    return swapReturn;
  }

  // sdbm variant hash, also used by berkeley -db
  inline int hashString(const char* str)
  {
    if (!str)
      return 0;

    int hash = 0;
    const char *c = str;

    while (*c != '\0')
    {
      hash = (*c) + (hash << 6) + (hash << 16) - hash;
      ++c;
    }

    return hash;
  }

  // return true if little, false if big
  inline bool platformIsLittleEndian()
  {
    int x = 1;
    if (*(char *) &x == 1)
      return true;
    else
      return false;
  }
}

#endif // NM_TYPE_UTILS_H
