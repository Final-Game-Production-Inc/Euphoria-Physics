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



#ifndef SIMPLEX_SOLVER_INTERFACE_H
#define SIMPLEX_SOLVER_INTERFACE_H

#include "vector/vector3.h"

#define NO_VIRTUAL_INTERFACE
#ifdef NO_VIRTUAL_INTERFACE
#include "VoronoiSimplexSolver.h"
#define SimplexSolverInterface VoronoiSimplexSolver
#else
/// for simplices from 1 to 4 vertices
/// for example Johnson-algorithm or alternative approaches based on
/// voronoi regions or barycentric coordinates
class SimplexSolverInterface
{
	public:
		virtual ~SimplexSolverInterface() {};

	virtual void reset() = 0;

	virtual void AddVertex(const rage::Vector3::Param w, const rage::Vector3::Param p, const rage::Vector3::Param q, int vertexIndexP, int vertexIndexQ) = 0;
	
	virtual bool closest(rage::Vector3::Param v) = 0;

	virtual flost maxVertex() = 0;

	virtual bool fullSimplex() const = 0;

	virtual int getSimplex(rage::Vector3 *pBuf, rage::Vector3 *qBuf, rage::Vector3 *yBuf) const = 0;

	virtual bool inSimplex(rage::Vector3::Param w) = 0;
	
	virtual void backup_closest(rage::Vector3::Ref v) = 0;

	virtual bool emptySimplex() const = 0;

	virtual void compute_points(rage::Vector3::Ref p1, rage::Vector3::Ref p2) = 0;

	virtual int numVertices() const =0;


};
#endif
#endif //SIMPLEX_SOLVER_INTERFACE_H

