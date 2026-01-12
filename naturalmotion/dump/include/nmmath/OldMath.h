#pragma once
#ifndef NM_OLDMATH_H
#define NM_OLDMATH_H

//*****************************************************************************
//
// +++++ THIS IS DEPRECTATED. DO NOT USE. USE NMPLATFORM MATHS INSTEAD +++++++++
//
//*****************************************************************************


namespace NMutils
{
  typedef float                   NMVector3[3];
  typedef float                  *NMVector3Ptr;
  typedef const float            *NMVector3ConstPtr;
  typedef float                   NMVector4[4];
  typedef float                  *NMVector4Ptr;
  typedef const float            *NMVector4ConstPtr;
  typedef NMVector3               NMMatrix3[3];
  typedef NMVector3              *NMMatrix3Ptr;
  typedef const NMVector3        *NMMatrix3ConstPtr;
  typedef NMVector4               NMMatrix4[4];
  typedef NMVector4              *NMMatrix4Ptr;
  typedef const NMVector4        *NMMatrix4ConstPtr;

  inline void NMVector3Set(NMVector3 c, float x, float y, float z)
  {
    c[0] = x, c[1] = y, c[2] = z;
  }

  inline void NMVector3Copy(NMVector3 c, const NMVector3 b)
  {
    c[0] = b[0], c[1] = b[1], c[2] = b[2];
  }

  inline float NMVector3Dot(const NMVector3 b, const NMVector3 c)
  {
    return b[0] * c[0] + b[1] * c[1] + b[2] * c[2];
  }

  inline void NMVector3Cross(NMVector3 a, const NMVector3 b, const NMVector3 c)
  {
    float a0 = b[1] * c[2] - b[2] * c[1];
    float a1 = b[2] * c[0] - b[0] * c[2];
    float a2 = b[0] * c[1] - b[1] * c[0];
    a[0] = a0;
    a[1] = a1;
    a[2] = a2;
  }

  inline void NMVector3MultiplyAdd(NMVector3 v, float a, const NMVector3 v1)
  {
    v[0] += a * v1[0],
    v[1] += a * v1[1],
    v[2] += a * v1[2];
  }

  inline float NMVector3MagnitudeSqr(const NMVector3 v)
  {
    float m = 0.0f;

    m += v[0] * v[0];
    m += v[1] * v[1];
    m += v[2] * v[2];
    return m;
  }

  inline float NMVector3DistanceSqr(const NMVector3 a, const NMVector3 b)
  {
    NMVector3 diff;
    diff[0] = a[0]-b[0], diff[1] = a[1]-b[1], diff[2] = a[2]-b[2];
    return NMVector3Dot(diff, diff);
  }

  float NMVector3Normalize(NMVector3 v);

  inline void NMVector4Set(NMVector4 c, float w, float x, float y, float z)
  {
    c[0] = w, c[1] = x, c[2] = y, c[3] = z;
  }

  inline void NMVector4SetZero(NMVector4 c)
  {
    c[0] = c[1] = c[2] = c[3] = 0;
  }

  inline void NMVector4Copy(NMVector4 c, const NMVector4 b)
  {
    c[0] = b[0], c[1] = b[1], c[2] = b[2], c[3] = b[3];
  }

  inline void NMQuaternionFromTwistAndSwing(NMVector4 q, const NMVector3 ts)
  {
    float b = 2/(1 + ts[1]*ts[1] + ts[2]*ts[2]);
    float c = 2/(1 + ts[0]*ts[0]);

    q[0] = (b-1)*(c-1);
    q[1] = ts[0]*(b-1)*c;
    q[2] = b*(c*ts[0]*ts[1]+(c-1)*ts[2]);
    q[3] = b*(c*ts[0]*ts[2]-(c-1)*ts[1]);
  }

  void NMMatrix3ToQuaternion(NMVector4 q, const NMMatrix3 m);
  void NMMatrix4Copy(NMMatrix4 A, const NMMatrix4 B);
  void NMMatrix4TMOrthoNormalize(NMMatrix4 tm);
}

#endif // NM_OLDMATH_H
