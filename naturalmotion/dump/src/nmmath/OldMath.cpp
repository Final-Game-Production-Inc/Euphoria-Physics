#include "nmmath/OldMath.h"
#include "nmutils/NMTypes.h"
#include <stdlib.h>
#include <limits>
#include <math.h>

//*****************************************************************************
//
// +++++ THIS IS DEPRECTATED. DO NOT USE. USE NMPLATFORM MATHS INSTEAD +++++++++
//
//*****************************************************************************

namespace NMutils
{
  float NMVector3Normalize(NMVector3 v)
  {
    float mag = 0, k = 0;
    int j;

    for (j = 0; j < 3; j++)
      k += v[j] * v[j];

    if (k > 0)
    {
      mag = sqrtf(k);
      k = 1.0f / mag;

      v[0] *= k, v[1] *= k, v[2] *= k;
    }
    else
    {
      mag = 0;
      v[0] = 1, v[1] = 0, v[2] = 0;
    }

    return mag;
  }

  void NMMatrix3ToQuaternion(NMVector4 q, const NMMatrix3 m)
  {
    const float *const t = (float *) m;
    float    tr, s, qt[4];
    int       i, j, k;

    static const int nxt[3] = { 1, 2, 0 };

    tr = t[0] + t[4] + t[8];

    /* check the diagonals. */

    if (tr > (float) (0.0)) {
      s = sqrtf(tr + (float) (1.0));

      /* q[0] = s / 2.0; */
      q[0] = s * (float) (0.5);

      s = (float) (0.5) / s;

      q[1] = (t[5] - t[7]) * s;
      q[2] = (t[6] - t[2]) * s;
      q[3] = (t[1] - t[3]) * s;
    } else {
      /* diagonal is negative */

      i = 0;

      if (t[4] > t[0])
        i = 1;

      if (t[8] > t[(i * 3) + i])
        i = 2;

      j = nxt[i];
      k = nxt[j];

      s = t[(i * 3) + i]
      - t[(j * 3) + j]
      - t[(k * 3) + k]
      + (float) (1.0);

      s = sqrtf(s);

      qt[i] = s * (float) (0.5);

      if (s != (float) (0.0))
        s = (float) (0.5) / s;

      qt[3] = (t[(j * 3) + k] - t[(k * 3) + j]) * s;
      qt[j] = (t[(i * 3) + j] + t[(j * 3) + i]) * s;
      qt[k] = (t[(i * 3) + k] + t[(k * 3) + i]) * s;

      q[1] = qt[0];
      q[2] = qt[1];
      q[3] = qt[2];
      q[0] = qt[3];
    }
  }

  void NMMatrix4Copy(NMMatrix4 A, const NMMatrix4 B)
  {
    A[0][0] = B[0][0], A[0][1] = B[0][1],
    A[0][2] = B[0][2], A[0][3] = B[0][3],

    A[1][0] = B[1][0], A[1][1] = B[1][1],
    A[1][2] = B[1][2], A[1][3] = B[1][3],

    A[2][0] = B[2][0], A[2][1] = B[2][1],
    A[2][2] = B[2][2], A[2][3] = B[2][3],

    A[3][0] = B[3][0], A[3][1] = B[3][1],
    A[3][2] = B[3][2], A[3][3] = B[3][3];
  }

  void NMMatrix4TMOrthoNormalize(NMMatrix4 tm)
  {
    float dot;
    NMVector3Normalize(tm[0]);
    dot = NMVector3Dot(tm[0], tm[1]);
    NMVector3MultiplyAdd(tm[1], -dot, tm[0]);
    NMVector3Normalize(tm[1]);
    NMVector3Cross(tm[2], tm[0], tm[1]);
  }
}

