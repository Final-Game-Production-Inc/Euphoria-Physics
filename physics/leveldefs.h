//
// physics/leveldefs.h
//
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved.
//

#ifndef PHYSICS_LEVELDEFS_H
#define PHYSICS_LEVELDEFS_H

#define PHLEVEL_NODE_TYPE	0

namespace rage {
#if PHLEVEL_NODE_TYPE == 0
	class phLooseOctreeNode;
	typedef phLooseOctreeNode phLevelNode;
#elif PHLEVEL_NODE_TYPE == 1
	class phLooseQuadtreeNode;
	typedef phLooseQuadtreeNode phLevelNode;
#else
#error "Invalid value for PHLEVEL_NODE_TYPE"
#endif

	template <class __NodeType> class phLevelNodeTree;
	typedef phLevelNodeTree<phLevelNode> phLevelNew;
}

#endif // ndef PHYSICS_LEVELDEFS_H
