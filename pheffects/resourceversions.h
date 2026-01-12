// 
// pheffects/resourceversions.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef PHEFFECTS_RESOURCEVERSIONS_H
#define PHEFFECTS_RESOURCEVERSIONS_H

// Version 1 - put some clothes on man!
// Version 2 - Changed datatypes for performance
// Version 3 - Pinning implemented
// Version 4 - Reordered pinned verts to start, removed cruft
// Version 5 - Static friction
// Version 6 - external colliders
// Version 7 - body relative movement 
// Version 8 - improved body relative movement 
// Version 9 - improved friction, now body relative 
// Version 12 - added deformation damping to edges 
// Version 13 - edge collisions optimized and reenabled 
// Version 14 - edge strain refactored, double pinning bug fixed
// Version 15 - collision callback added 
// Version 16 - over relaxation interation  
// Version 17 - refactored external collisions 
// Version 18 - added option for bend springs 
// Version 19 - modified collidable edges implementation 
// Version 20 - Added vertex collission lists for each bound
// Version 21 - Made friction more robust by avoiding divide by zero 
// Version 22 - edges sorted properly into buckets of 8 with no interaction 
int clthResourceVersion = 21;

#endif
