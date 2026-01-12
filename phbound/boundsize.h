#ifndef PHBOUND_BOUNDSIZE_H
#define PHBOUND_BOUNDSIZE_H

#include "boundsphere.h"
#include "boundcapsule.h"
#include "boundtaperedcapsule.h"
#include "boundbox.h"
#include "boundgeom.h"
#include "boundcurvedgeom.h"
#include "boundgrid.h"
#include "boundribbon.h"
#include "boundbvh.h"
#include "boundsurface.h"
#include "boundcomposite.h"
#include "phbullet/TriangleShape.h"
#include "bounddisc.h"
#include "boundcylinder.h"

namespace rage
{

inline int GetBoundSize(phBound::BoundType type)
{
	switch(type)
	{
		#undef BOUND_TYPE_INC
		#define BOUND_TYPE_INC(className,enumLabel,enumValue,stringName,isUsed) \
			case phBound::enumLabel: FastAssert(isUsed); return sizeof(className);
		#include "boundtypes.inc"
		#undef BOUND_TYPE_INC
		default:
			FastAssert(0&&"Invalid Bound Type.");
			return 0;
	}
}

// Simple template meta-program to compute the maximum bound size as a compile time constant.
template <const int A, const int B> struct GetMaxTM { enum { VAL = ((A > B) ? A : B) }; };
const int PHBOUND_MAX_BOUND_SIZE = 
#undef BOUND_TYPE_INC
#define BOUND_TYPE_INC(className,enumLabel,enumValue,stringName,isUsed) GetMaxTM<sizeof(className),
#include "boundtypes.inc"
0
#undef BOUND_TYPE_INC
#define BOUND_TYPE_INC(className,enumLabel,enumValue,stringName,isUsed) >::VAL
#include "boundtypes.inc"
#undef BOUND_TYPE_INC
;

} // namespace rage

#endif // PHBOUND_BOUNDSIZE_H

