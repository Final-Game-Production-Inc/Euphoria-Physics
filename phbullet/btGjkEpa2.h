#ifndef _68DA1F85_90B7_4bb0_A705_83B4040A75C6_
#define _68DA1F85_90B7_4bb0_A705_83B4040A75C6_

#include "phcore/constants.h"
#if ENABLE_EPA_PENETRATION_SOLVER_CODE

#include "phbound/bound.h"

#include "vector/vector3.h"

///btGjkEpaSolver contributed under zlib by Nathanael Presson
struct	btGjkEpaSolver2
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
	};

static int		StackSizeRequirement();

static bool		Penetration(const rage::phBound* shape0,const rage::Matrix34& wtrs0,
							const rage::phBound* shape1,const rage::Matrix34& wtrs1,
							const rage::Vector3& guess,
							sResults&	results);
};

#endif // ENABLE_EPA_PENETRATION_SOLVER_CODE

#endif
