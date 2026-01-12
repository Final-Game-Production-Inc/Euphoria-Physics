// 
// pharticulated/savedstate.h 
// 
// Copyright (C) 1999-2008 Rockstar Games.  All Rights Reserved. 
// 

#ifndef PHARTICULATED_SAVEDSTATE_H 
#define PHARTICULATED_SAVEDSTATE_H 

#include "vectormath/classes.h"

namespace rage {

class phArticulatedBody;
class phArticulatedBodyPart;
class phArticulatedBodyPartSavedState;

class phArticulatedBodySavedState
{
public:
	phArticulatedBodySavedState();
	~phArticulatedBodySavedState();

	void SaveState(phArticulatedBody& body);
	void RestoreState(phArticulatedBody& body);

private:
	phArticulatedBodyPartSavedState* m_Parts;
	int m_NumParts;
};

class phArticulatedBodyPartSavedState
{
public:
	void SaveState(phArticulatedBody *body, int partIndex);
	void RestoreState(phArticulatedBody *body, int partIndex);

private:
	Mat34V m_Matrix;
	Vec3V m_LinVelocity;
	Vec3V m_AngVelocity;
};

} // namespace rage

#endif // PHARTICULATED_SAVEDSTATE_H 
