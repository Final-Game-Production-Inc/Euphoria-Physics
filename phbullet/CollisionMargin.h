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

#ifndef COLLISION_MARGIN_H
#define COLLISION_MARGIN_H

#include "vector/vector3.h"

//used by Gjk and some other algorithms

#define CONVEX_DISTANCE_MARGIN 0.04f// 0.1f//;//0.01f
#define CONCAVE_DISTANCE_MARGIN 0.005f
#define CONVEX_MINIMUM_MARGIN	0.025f

#define PENETRATION_CHECK_EXTRA_MARGIN (0.08f)

//extern const rage::Vector3 PENETRATION_CHECK_EXTRA_MARGIN_V;

//extern const rage::Vector3 CONVEX_DISTANCE_MARGIN_V;
//extern const rage::Vector3 CONCAVE_DISTANCE_MARGIN_V;



#endif //COLLISION_MARGIN_H

