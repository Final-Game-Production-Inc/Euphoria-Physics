//rage/dev/rage/base/src/grcore/resourceversions.h#8 - edit change 66036 (text)
// 
// grcore/resourceversions.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef GRCORE_RESOURCEVERSIONS_H
#define GRCORE_RESOURCEVERSIONS_H

// Change this whenever rmcore structures change
// Higher level code that resources rmcore objects should
// add this value to its own private resource version number.
// Version 7 due to removal of bucketmask from several structures and general API cleanup
// Version 8 removes the rmcModelInfo data structures from drawables
// Version 9 is because the definition of rmcBlendSetCustom changed.
// Version 10 is because we had to add padding to rmcShaderGroup for alignment
// Version 11 is to track the significant rmcModel internal changes
// Version 12 is due to reorganizing rmcTexturePS2 to eliminate the mega hack
// Version 13 is due to bucketmask being re-added to rmcLodGroup
// Version 14 is due to changing rmcShader's variables to bitfields
// Version 15 is due to rearranging the bits
// Version 16 is due to deriving several classes from Base
// Version 17 is due to being able to remove normals/tex1's from xbox meshes
// Version 18 is due to rmcShaderGroup locals becoming a linked list
// Version 19 is due to rmcModel format changing in regards to FVF object
// Version 20 is due to an FVF data format change to handle pretransformed verts
// Version 21 is due to an optional bounds spheres added for each geometry packet
// Version 22 is due to removeing redundant Vector4 allocated for grmModelGeoms with only a single packet
// Version 23 is due to changeing "doublebuffer" vertex buffers to triple buffered
// Version 24 is to enable larger constant buffers for skinned objects (peds and vehicles mainly) and changed light format to include packing data.
const int rmcResourceBaseVersion = 24;

#endif
