//
// crskeleton/skeleton.h
//
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved.
//

#ifndef CRSKELETON_SKELETON_H
#define CRSKELETON_SKELETON_H

#include "diag/stats.h"
#include "diag/trap.h"
#include "paging/ref.h"
#include "grprofile/drawcore.h"
#include "vectormath/mat34v.h"

namespace rage
{

class crSkeletonData;
struct Matrix43;

////////////////////////////////////////////////////////////////////////////////

// PURPOSE: A crSkeleton is an instance of a particular skeleton.  It holds the current
// pose information of a particular character in a game.  crFrame::Pose sets
// the local matrices of all the crBones, and then crSkeleton::Update computes
// the current object matrices for each bone. The matrices are then attached
// into the MatrixSet for drawing.
class crSkeleton
{
public:

	// PURPOSE: Default constructor
	crSkeleton();

	// PURPOSE: Resource constructor
	crSkeleton(datResource&);

	// PURPOSE: Destructor
	~crSkeleton();

#if __DECLARESTRUCT
	void DeclareStruct(datTypeStruct& s);
#endif // __DECLARESTRUCT
	
	DECLARE_PLACE(crSkeleton);

	// PURPOSE: Initialization, attaches a skeleton to a skeleton data structure
	// PARAMS:  skelData - skeleton data, parentMtx - parent matrix pointer, NULL for none
	// NOTES: Init must be called before most operations on a skeleton can be performed, reference counts the skeleton data structure
	void Init(const crSkeletonData& data, const Mat34V* parentMtx=NULL);

	// PURPOSE: Detached tracked references, for fragment code.
	void UnInit();

	// PURPOSE: Shuts down the skeleton, deallocates all memory, detaches from skeleton data
	void Shutdown();

	// PURPOSE: Resets all the local matrices with their default rotations and translations
	// NOTES: Global matrices remain unchanged
	void Reset();

	// PURPOSE: Resets the local matrices of a certain bone
	// NOTES: Global matrices remain unchanged
	void PartialReset(u32 boneIdx);

	// PURPOSE: Copy matrices from another skeleton
	void CopyFrom(const crSkeleton& other);

	// PURPOSE: Compute the object matrices of the skeleton from the local matrices
	void Update();

	// PURPOSE: Transform all current object matrices
	void Transform(Mat34V_In mtx);

	// PURPOSE: Transform the object matrices on a subsection of the skeleton
	void PartialTransform(u32 boneIdx, Mat34V_In mtx, bool inclusive=true);

	// PURPOSE: Compute the object matrices of a sub-section of the skeleton
	void PartialUpdate(u32 boneIdx, bool inclusive=true);

	// PURPOSE: Compute the local matrices of the skeleton from the object matrices
	void InverseUpdate();

	// PURPOSE: Compute the local matrices of a sub-section of the skeleton.
	void PartialInverseUpdate(u32 boneIdx, bool inclusive=true);

	// PURPOSE: Copy matrices for rendering into the provided buffer
	// PARAMS:  isSkinned - true if the model is skinned (skin space will be used), outMtxs - output array of matrices
	void Attach(bool isSkinned, Matrix43* outMtxs) const;

	// PURPOSE: Number of bones this skeleton supports
	u32 GetBoneCount() const;

	// PURPOSE: Get the current skeleton data
	const crSkeletonData& GetSkeletonData() const;

	// PURPOSE: Gets the parent matrix
	const Mat34V* GetParentMtx() const;

	// PURPOSE: Set the parent matrix
	void SetParentMtx(const Mat34V* parent);

	// PURPOSE: Get local matrix of a specific bone
	Mat34V_Ref GetLocalMtx(u32 boneIdx);
	Mat34V_ConstRef GetLocalMtx(u32 boneIdx) const;

	// PURPOSE: Get object matrix of a specific bone
	Mat34V_Ref GetObjectMtx(u32 boneIdx);
	Mat34V_ConstRef GetObjectMtx(u32 boneIdx) const;

	// PURPOSE: Get object matrices array
	const Mat34V* GetObjectMtxs() const;	
	Mat34V* GetObjectMtxs();

	// PURPOSE: Get local matrices array
	const Mat34V* GetLocalMtxs() const;	
	Mat34V* GetLocalMtxs();

	// PURPOSE: Get global matrix of a specific bone
	// NOTES: It is faster to access bone matrix in object or local space
	void GetGlobalMtx(u32 boneIdx, Mat34V_InOut outMtx) const;

	// PURPOSE: Set object matrix of a specific bone from a world matrix
	// NOTES: Can be slow because of parent matrix inversion
	void SetGlobalMtx(u32 boneIdx, Mat34V_In mtx);

	// PURPOSE: Finding the bone to terminate iterating a partial sequence on
	u32 GetTerminatingPartialBone(u32 boneIdx) const;

	// PURPOSE: Set the skeleton data
	// NOTES: Must have the same number of bones, 
	// the same hierarchy and the same bone IDs as the original crSkeletonData
	// (i.e. the crSkeletonData::GetSignature's must match).
	void SetSkeletonData(const crSkeletonData& skelData);

	// PURPOSE: Copy the skeleton data
	// NOTES: Must have the same number of bones, 
	// the same hierarchy and the same bone IDs as the original crSkeletonData
	// (i.e. the crSkeletonData::GetSignature's must match) and you really need
	// to know why you do this!
	void CopySkeletonData(const crSkeletonData& skelData);

#if !__NO_OUTPUT
	size_t GetMemorySize() const;
#endif

#if __PFDRAW
	// PURPOSE: Debug drawing of the skeleton.
	void ProfileDraw() const;
#endif // __DEV

protected:
	pgRef<const crSkeletonData> m_SkeletonData;
	datRef<const Mat34V> m_Parent;
	datOwner<Mat34V> m_Locals;
	datRef<Mat34V> m_Objects;
	u32 m_NumBones;
};

////////////////////////////////////////////////////////////////////////////////

inline u32 crSkeleton::GetBoneCount() const
{
	return m_NumBones;
}

////////////////////////////////////////////////////////////////////////////////

inline const crSkeletonData& crSkeleton::GetSkeletonData() const
{
	FastAssert(m_SkeletonData);
	return *m_SkeletonData;
}

////////////////////////////////////////////////////////////////////////////////

inline const Mat34V* crSkeleton::GetParentMtx() const
{
	return m_Parent;
}

////////////////////////////////////////////////////////////////////////////////

inline void crSkeleton::SetParentMtx(const Mat34V* parentMtx)
{
	m_Parent = parentMtx;
}

////////////////////////////////////////////////////////////////////////////////

inline Mat34V_Ref crSkeleton::GetLocalMtx(u32 boneIdx)
{
	TrapGE(boneIdx, m_NumBones);
	return m_Locals[boneIdx];
}

////////////////////////////////////////////////////////////////////////////////

inline Mat34V_ConstRef crSkeleton::GetLocalMtx(u32 boneIdx) const
{
	TrapGE(boneIdx, m_NumBones);
	return m_Locals[boneIdx];
}

////////////////////////////////////////////////////////////////////////////////

inline Mat34V_Ref crSkeleton::GetObjectMtx(u32 boneIdx)
{
	TrapGE(boneIdx, m_NumBones);
	return m_Objects[boneIdx];
}

////////////////////////////////////////////////////////////////////////////////

inline Mat34V_ConstRef crSkeleton::GetObjectMtx(u32 boneIdx) const
{
	TrapGE(boneIdx, m_NumBones);
	return m_Objects[boneIdx];
}

////////////////////////////////////////////////////////////////////////////////

inline Mat34V* crSkeleton::GetObjectMtxs()
{
	return m_Objects;
}

////////////////////////////////////////////////////////////////////////////////

inline const Mat34V* crSkeleton::GetObjectMtxs() const
{
	return m_Objects;
}

////////////////////////////////////////////////////////////////////////////////

inline Mat34V* crSkeleton::GetLocalMtxs()
{
	return m_Locals;
}

////////////////////////////////////////////////////////////////////////////////

inline const Mat34V* crSkeleton::GetLocalMtxs() const
{
	return m_Locals;
}

////////////////////////////////////////////////////////////////////////////////

}	// namespace rage

#endif // CRSKELETON_SKELETON_H
