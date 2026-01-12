/*
Copyright (c) 2003-2006 Gino van den Bergen / Erwin Coumans  http://continuousphysics.com/Bullet/

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, 
including commercial applications, and to alter it and redistribute it freely, 
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/


#ifndef SIMD_TRANSFORM_UTIL_H
#define SIMD_TRANSFORM_UTIL_H

#include "vector/quaternion.h"
#include "vector/matrix34.h"
#include "vector/vector3.h"

#include "vectormath/classes.h"
#include "vectormath/legacyconvert.h"

#define SIMDSQRT12 SimdScalar(0.7071067811865475244008443621048490)

#define SimdRecipSqrt(x) ((float)(1.0f/SimdSqrt(float(x))))		/* reciprocal square root */

namespace rage {

/// Utils related to temporal transforms
class SimdTransformUtil
{

public:

	__forceinline
	static void IntegrateTransform(MAT34V_DECL(curTrans),
								   Vec::V3Param128_After3Args linvel,
								   Vec::V3Param128_After3Args quat0,
								   Vec::V3Param128_After3Args quatDelta,
								   Vec::V3Param128_After3Args timeStep,
								   Mat34V_InOut predictedTransform	)
	{
		// Save locals.
		Mat34V v_curTrans = MAT34V_ARG_GET(curTrans);
		ScalarV v_timestep(timeStep); // This splat may not be necessary...
		QuatV v_quat(Vec::V4QuatNormalize(Vec::V4AddScaled(quat0, quatDelta, timeStep)));

		Vec3V v_outTrans = AddScaled(v_curTrans.GetCol3(), Vec3V(linvel), Vec3V(v_timestep));

		Mat34V v_outMat;
		Mat34VFromQuatV(v_outMat, v_quat, v_outTrans);
		predictedTransform = v_outMat;
	}

	__forceinline
	static void CalculateQuaternionDelta(MAT33V_DECL(transform0), MAT33V_DECL2(transform1), Vec::V4Ref128 quat0, Vec::V4Ref128 quatDelta)
	{
		using namespace rage;

		Mat33V v_transform0(MAT33V_ARG_GET(transform0));
		Mat33V v_transform1(MAT33V_ARG_GET(transform1));

		QuatV v_quat0 = QuatVFromMat33V(v_transform0);
		QuatV v_quat1 = QuatVFromMat33V(v_transform1);

		quat0 = v_quat0.GetIntrin128();
		quatDelta = Vec::V4Subtract(v_quat1.GetIntrin128(), v_quat0.GetIntrin128());
	}

	__forceinline
	static Vec3V_Out CalculateAngularVelocity(MAT33V_DECL(transform0), MAT33V_DECL2(transform1))
	{
		using namespace rage;

		Mat33V v_transform0(MAT33V_ARG_GET(transform0));
		Mat33V v_transform1(MAT33V_ARG_GET(transform1));

		Mat33V dmat;
		UnTransformOrtho( dmat, v_transform0, v_transform1 );
		QuatV dorn = QuatVFromMat33V( dmat );

		Vec3V axis;
		ScalarV angle;
		QuatVToAxisAngle( axis, angle, dorn );

		return Scale( axis, angle );
	}

	__forceinline
	static Vec3V_Out CalculateLinearVelocity(Vec::V3Param128 position0, Vec::V3Param128 position1)
	{
		return Vec3V(position1) - Vec3V(position0);
	}


};

} // namespace rage

#endif //SIMD_TRANSFORM_UTIL_H

