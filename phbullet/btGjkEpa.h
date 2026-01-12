/*
Bullet Continuous Collision Detection and Physics Library
Copyright (c) 2003-2006 Erwin Coumans  http://continuousphysics.com/Bullet/

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, 
including commercial applications, and to alter it and redistribute it freely, 
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

/*
GJK-EPA collision solver by Nathanael Presson
Nov.2006
*/


#ifndef _05E48D53_04E0_49ad_BB0A_D74FE62E7366_
#define _05E48D53_04E0_49ad_BB0A_D74FE62E7366_

#include "phcore/constants.h"
#if ENABLE_UNUSED_PHYSICS_CODE

#include "phbound/bound.h"

#include "vector/vector3.h"

class btStackAlloc;

///btGjkEpaSolver contributed under zlib by Nathanael Presson
struct	btGjkEpaSolver
{
struct	sResults
	{
	enum eStatus
		{
		Separated,		/* Shapes doesnt penetrate												*/ 
		Penetrating,	/* Shapes are penetrating												*/ 
		GJK_Failed,		/* GJK phase fail, no big issue, shapes are probably just 'touching'	*/ 
		EPA_Failed,		/* EPA phase fail, bigger problem, need to save parameters, and debug	*/ 
		}		status;
    rage::Vector3	witnesses[2];
	rage::Vector3	normal;
	float	depth;
	int	epa_iterations;
	int	gjk_iterations;
	};
static bool	Collide(rage::phBound* shape0,const rage::Matrix34& wtrs0,
                    rage::phBound* shape1,const rage::Matrix34& wtrs1,
					float	radialmargin,
					sResults&	results);
};

#endif // ENABLE_UNUSED_PHYSICS_CODE

#endif
