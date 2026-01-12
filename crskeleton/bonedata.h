//
// crskeleton/bonedata.h
//
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved.
//

#ifndef CRSKELETON_BONEDATA_H
#define CRSKELETON_BONEDATA_H

#include "cranimation/animation_config.h"

#include "data/struct.h"
#include "string/string.h"
#include "system/bit.h"
#include "vectormath/mat34v.h"
#include "vectormath/quatv.h"
#include "vectormath/vec3v.h"

namespace rage
{

class datResource;
class fiTokenizer;
class fiStream;

#define ENABLE_MIRROR_TEST (0)

////////////////////////////////////////////////////////////////////////////////

// PURPOSE:
//	The crBoneData holds data that is general to all instances of a
//	particular matrix that belongs to skeletons of the type of described
//	by a particular crSkeletonData.  Several crBoneData are owned by
//	a crSkeletonData, the connections between them determine the structure of the skeleton.
class crBoneData
{
	friend class crSkeletonData;

public:
	
	// PURPOSE: Default constructor
	crBoneData();

	// PURPOSE: Resource constructor
	crBoneData(datResource&);

	// PURPOSE: Destructor
	~crBoneData();

#if __DECLARESTRUCT
	void DeclareStruct(class datTypeStruct &s);
#endif // __DECLARESTRUCT

#if CR_DEV
	// PURPOSE: Load the bone data from a file.
	// PARAMS: tok - the input tokenizer, bones - the entire list of bones, next - the next bone data to be loaded, index - the bone data index value.
	// RETURN: true if the loading process successes.
	bool Load_v100(fiTokenizer &tok, crBoneData **next, int &index);
	bool Load_v101(fiTokenizer &tok, crBoneData **next, int &index);
	bool Load_v104Plus(fiTokenizer &tok, crBoneData **next, int &index);

	// PUPOSE: Save the bone data for a file
	// PARAMS: f - the input stream, indent - level of layout indentation for this bone data
	// RETURN: true if the bone data was written
	bool Save(fiStream* f, int indent = 0) const;
#endif // CR_DEV

	// PURPOSE: Get bone data name.
	// RETURNS: Bone name, can be NULL if no name assigned.
	const char* GetName() const;
	
	// PURPOSE: Get bone data degree-of-freedom flags
	// RETURNS: Degree-of-freedom bit field (see enumeration in crBoneData for details)
	// NOTES: Use to check availability of dofs, and components thereof, plus the validity of min/max limits.
	u16 GetDofs() const;

	void SetDofs(u16 dofs);

	// PURPOSE: Get default translation.
	// RETURNS: Default translation vector of bone. This is the offset between this bone and it's parent.
	Vec3V_ConstRef GetDefaultTranslation() const;

	// PURPOSE: Get default local bone rotation
	// RETURNS: Default rotation on the bone as a quaternion
	QuatV_ConstRef GetDefaultRotation() const;

	// PURPOSE: Get default scale.
	// RETURNS: Default scale vector.
	// NOTES: Scale is not properly implemented in Rage yet, this is a placeholder.
	Vec3V_ConstRef GetDefaultScale() const;

	// PURPOSE: Get the bone index.
	// RETURNS: The bone index [0..number of bones in skeleton-1]
	// NOTES: The bone index is the bone's position in the [flattened] hierarchy, the root is always 0,
	// then the hierarchy is walked (children first, then siblings) and is numbered in that sequence.
	int GetIndex() const;

	// PURPOSE: Get the mirror bone's index.
	// RETURNS: The index of the bone that is a mirror to this bone.
	// NOTES: Used to mirror animations / frames. If the mirror index == bone index, then this bone has no mirror / is self mirroring.
	int GetMirrorIndex() const;

	// PURPOSE: Check what degrees of freedom are on this bone.
	// RETURNS: True if any of the degrees of freedom in the mask exist on this bone.
	bool HasDofs(u32 dofMask) const;

	// PURPOSE: Get the bone id of this bone.
	// RETURNS: The bone id (or if bone ids not used, returns the bone's index).
	// NOTES: Bone ids a used in frames and animations to allow skeletons which are similar, but may have different optional/additional bones to share the same animations.
	// It allows additional bones to be added to a skeleton, without the need to re-export all the intermediate assets.
	// This is accomplished by indexing the dof with frames and animations using the bone id (which is typically generated from a hash of the bone name), rather than the
	// bone index (which is the index of the bone in the flattened skeleton hierarchy).
	// The root bone is a special case that is guaranteed to have a bone id and index of 0. When moving data between frames and skeletons (ie posing) the bone ids are mapped to indices.
	u16 GetBoneId() const;

	// PURPOSE: Get the bone's child.
	// RETURNS: Pointer to the bone's first child, or NULL if no children.
	const crBoneData* GetChild() const;
	
	// PURPOSE: Get the bone's next sibling.
	// RETURNS: Pointer to the bone's next sibling, or NULL if no more siblings.
	const crBoneData* GetNext() const;

	// PURPOSE: Get the bone's parent.
	// RETURNS: Pointer to the bone's parent, or NULL if no parent.
	const crBoneData* GetParent() const;

	// PURPOSE: Calculates the combined joint and scale orientation on a hierarchy of bones.
	// PARAMS: outMtx - the resulting matrix
	// NOTES: Walks up the parent bones combining all the joint orientations, scale and translation.
	// Not cheap; you may want	to cache the result...
	void CalcCumulativeJointScaleOrients(Mat34V_InOut outMtx) const;

	// PURPOSE: Calculate a non-chiral signature
	u64 GetSignatureNonChiral() const;

	// PURPOSE: Degrees of freedom ({rotate,translate,scale} X {x,y,z}) and limits
	enum
	{
		ROTATE_X =				BIT( 0),							// can rotate on x-axis
		ROTATE_Y =				BIT( 1),							// can rotate on y-axis
		ROTATE_Z =				BIT( 2),							// can rotate on z-axis
		HAS_ROTATE_LIMITS =		BIT( 3),							// is rotation limited?
		TRANSLATE_X =			BIT( 4),							// can translate in x-axis
		TRANSLATE_Y =			BIT( 5),							// can translate in y-axis
		TRANSLATE_Z =			BIT( 6),							// can translate in z-axis
		HAS_TRANSLATE_LIMITS =	BIT( 7),							// is translation limited?
		SCALE_X =				BIT( 8),							// can scale in x-axis
		SCALE_Y =				BIT( 9),							// can scale in y-axis
		SCALE_Z =				BIT(10),							// can scale in z-axis
		HAS_SCALE_LIMITS =		BIT(11),							// is scale limited?
		HAS_CHILD =				BIT(12),							// children?
		IS_SKINNED =			BIT(13),							// bone is skinned to
		ROTATION =				(ROTATE_X | ROTATE_Y | ROTATE_Z),
		TRANSLATION =			(TRANSLATE_X | TRANSLATE_Y |TRANSLATE_Z),
		SCALE =					(SCALE_X | SCALE_Y | SCALE_Z),
	};

private:
	const crBoneData* GetIndexedBone(int idx) const;
	crBoneData* GetIndexedBone(int idx);
	void AddChild(crBoneData* child);

	QuatV m_DefaultRotation;
	Vec3V m_DefaultTranslation;
	Vec3V m_DefaultScale;

	s16 m_NextIndex;
	s16 m_ParentIndex;
	ConstString m_Name;
	u16 m_Dofs;
	u16 m_Index;
	u16 m_BoneId;
	u16 m_MirrorIndex;
};

////////////////////////////////////////////////////////////////////////////////

inline const char* crBoneData::GetName() const
{
	return m_Name;
}

////////////////////////////////////////////////////////////////////////////////

inline u16 crBoneData::GetDofs()const
{
	return m_Dofs;
}

////////////////////////////////////////////////////////////////////////////////

inline void crBoneData::SetDofs(u16 dofs)
{
	m_Dofs=dofs;
}

////////////////////////////////////////////////////////////////////////////////

inline Vec3V_ConstRef crBoneData::GetDefaultTranslation() const
{
	return m_DefaultTranslation;
}

////////////////////////////////////////////////////////////////////////////////

inline QuatV_ConstRef crBoneData::GetDefaultRotation() const
{
	return m_DefaultRotation;
}

////////////////////////////////////////////////////////////////////////////////

inline Vec3V_ConstRef crBoneData::GetDefaultScale() const
{
	return m_DefaultScale;
}

////////////////////////////////////////////////////////////////////////////////

inline int crBoneData::GetIndex() const
{
	return m_Index;
}

////////////////////////////////////////////////////////////////////////////////

inline int crBoneData::GetMirrorIndex() const
{
	return m_MirrorIndex;
}

////////////////////////////////////////////////////////////////////////////////

inline bool crBoneData::HasDofs(u32 dofMask) const
{
	return (m_Dofs & dofMask) != 0;
}

////////////////////////////////////////////////////////////////////////////////

inline u16 crBoneData::GetBoneId() const
{
	return m_BoneId;
}

////////////////////////////////////////////////////////////////////////////////

inline const crBoneData* crBoneData::GetChild() const
{
	return m_Dofs & HAS_CHILD ? this+1 : NULL;
}

////////////////////////////////////////////////////////////////////////////////

inline const crBoneData* crBoneData::GetNext() const
{
	return GetIndexedBone(m_NextIndex);
}

////////////////////////////////////////////////////////////////////////////////

inline const crBoneData* crBoneData::GetParent() const	
{
	return GetIndexedBone(m_ParentIndex);
}

///////////////////////////////////////////////////////////////////////////////

__forceinline const crBoneData* crBoneData::GetIndexedBone(int idx) const
{
	return idx>=0 ? (this + (idx-m_Index)) : NULL;
}

///////////////////////////////////////////////////////////////////////////////

__forceinline crBoneData* crBoneData::GetIndexedBone(int idx)
{
	return idx>=0 ? (this + (idx-m_Index)) : NULL;
}

///////////////////////////////////////////////////////////////////////////////

// PURPOSE: Iterator that enumerates through the hierarchy of bones.
class crBoneDataIterator
{
public:
	crBoneDataIterator(const crBoneData* root) : m_Root(root), m_Current(root) {}

	// PURPOSE: Returns the current bone in the hierarchy.
	// RETURNS: Returns current bone the iterator is enumerating.
	// NOTES: Doesn't change the iterator's position.
	// Will return root if called immediately after construction or GetRoot(true) call.
	const crBoneData* GetCurrent() const;

	// PURPOSE: Returns the root bone in the hierarchy.
	// PARAMS: updateIteratorPos - if true, resets iterator to start.
	// RETURNS: Returns current bone the iterator is enumerating.
	const crBoneData* GetRoot(bool updateIteratorPos = true);

	// PURPOSE: Returns the next bone in the hierarchy, or NULL if no more bones found below the root.
	// PARAMS: updateIteratorPos - if true, increments iterator to next bone.
	// RETURNS: Returns next bone the iterator is enumerating, or NULL if no next.
	// NOTES: Performs ordered enumeration of all children, grand-children etc of root, but
	// never returns root itself.  Terminates with return of NULL.
	const crBoneData* GetNext(bool updateIteratorPos = true);

	// PURPOSE: Returns the previous bone in the hierarchy, or NULL if no more bones found below the root.
	// PARAMS: updateIteratorPos - if true, increments iterator to previous bone.
	// RETURNS: Returns previous bone the iterator is enumerating, or NULL if no previous.
	// NOTES: Performs reverse ordered enumeration of all children, grand-children etc of root.
	// Terminates with return of root, and then NULL.
	const crBoneData* GetPrev(bool updateIteratorPos = true);

	// PURPOSE: Returns the first bone in the hierarchy, or NULL if no bones found below the root.
	// PARAMS: updateIteratorPos - if true, resets iterator to point at first bone (or root if no first bone).
	// RETURNS: Returns first bone in the enumeration order, or NULL if no bones below root.
	const crBoneData* GetFirst(bool updateIteratorPos = true);

	// PURPOSE: Returns the last bone in the hierarchy, or NULL if no bones found below the root.
	// PARAMS: updateIteratorPos - if true, resets iterator to point at first bone (or root if no first bone).
	// RETURNS: Returns last bone in the enumeration order, or NULL if no bones below root.
	const crBoneData* GetLast(bool updateIteratorPos = true);

protected:
	const crBoneData* InternalNext(const crBoneData* current, const crBoneData* root);

	const crBoneData* m_Root;
	const crBoneData* m_Current;
};

///////////////////////////////////////////////////////////////////////////////

inline const crBoneData* crBoneDataIterator::InternalNext(const crBoneData* current, const crBoneData* root)
{
	const crBoneData* bone = current->GetChild();

	if( bone )
		return bone;

	if( current != root )
	{
		bone = current->GetNext();
		if( bone )
			return bone;
		bone = current->GetParent();

		while( bone && !bone->GetNext() )
		{
			if( bone == root )
				return NULL;

			bone = bone->GetParent();
		}
	}	
	return ( bone && ( bone != root )) ? bone->GetNext() : NULL;
}

///////////////////////////////////////////////////////////////////////////////

inline const crBoneData* crBoneDataIterator::GetCurrent() const
{
	return m_Current;
}

///////////////////////////////////////////////////////////////////////////////

inline const crBoneData* crBoneDataIterator::GetRoot(bool updateIteratorPos)
{
	if(updateIteratorPos)
	{
		m_Current = m_Root;
	}
	return m_Root;
}

///////////////////////////////////////////////////////////////////////////////

inline const crBoneData* crBoneDataIterator::GetNext(bool updateIteratorPos)
{
	if(m_Current == NULL)
	{
		return NULL;
	}

	const crBoneData* bd = InternalNext(m_Current, m_Root);
	if(updateIteratorPos)
	{
		m_Current = bd;
	}
	return bd;
}

///////////////////////////////////////////////////////////////////////////////

inline const crBoneData* crBoneDataIterator::GetPrev(bool updateIteratorPos)
{
	if(m_Current == NULL)
	{
		return NULL;
	}

	// OPTIMIZE - very inefficient way of calculating the prev bone
	const crBoneData* bdPrev = NULL;
	const crBoneData* bd = m_Root;
	while(bd != m_Current)
	{
		bdPrev = bd;
		bd = InternalNext(bd, m_Root);
	}

	if(updateIteratorPos)
	{
		m_Current = bdPrev;
	}
	return bdPrev;
}

///////////////////////////////////////////////////////////////////////////////

inline const crBoneData* crBoneDataIterator::GetFirst(bool updateIteratorPos)
{
	const crBoneData* bd = InternalNext(m_Root, m_Root);
	if(updateIteratorPos)
	{
		m_Current = bd;
	}
	return bd;
}

///////////////////////////////////////////////////////////////////////////////

inline const crBoneData* crBoneDataIterator::GetLast(bool updateIteratorPos)
{
	// OPTIMIZE - very inefficient way of calculating the last bone
	const crBoneData* bdPrev = NULL;
	const crBoneData* bd = m_Root;
	while(bd)
	{
		bdPrev = bd;
		bd = InternalNext(bd, m_Root);
	}
	
	if(updateIteratorPos)
	{
		m_Current = bdPrev;
	}
	return bdPrev;
}

}	// namespace rage

///////////////////////////////////////////////////////////////////////////////

#endif // CRSKELETON_BONEDATA_H
