
// crskeleton/skeleton.cpp
//
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved.
//

#include "skeleton.h"

#include "jointdata.h"
#include "skeletondata.h"

#include "atl/array_struct.h"
#include "grcore/matrix43.h"
#include "grprofile/drawcore.h"
#include "grprofile/drawmanager.h"
#include "system/cache.h"
#include "system/memops.h"

namespace rage
{

////////////////////////////////////////////////////////////////////////////////

crSkeleton::crSkeleton()
: m_SkeletonData(NULL)
, m_Parent(NULL)
, m_Locals(NULL)
, m_Objects(NULL)
, m_NumBones(0)
{
}

////////////////////////////////////////////////////////////////////////////////

crSkeleton::crSkeleton(datResource& rsc)
: m_Locals(rsc)
, m_Objects(rsc)
{
}

////////////////////////////////////////////////////////////////////////////////

crSkeleton::~crSkeleton()
{
	Shutdown();
}

////////////////////////////////////////////////////////////////////////////////

void crSkeleton::UnInit()
{
	m_SkeletonData.Release();
}

////////////////////////////////////////////////////////////////////////////////

#if __DECLARESTRUCT
void crSkeleton::DeclareStruct(datTypeStruct& s)
{
	STRUCT_BEGIN(crSkeleton);
	STRUCT_FIELD(m_SkeletonData);
	STRUCT_FIELD(m_Parent);
	STRUCT_FIELD(m_Locals);
	STRUCT_FIELD(m_Objects);
	STRUCT_FIELD(m_NumBones);
	STRUCT_END();
}
#endif // __DECLARESTRUCT

////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_PLACE(crSkeleton);

////////////////////////////////////////////////////////////////////////////////

void crSkeleton::Init(const crSkeletonData& skelData, const Mat34V* parentMtx)
{
	// if required, release existing skeleton data
	skelData.AddRef();
	m_SkeletonData.Release();
	m_SkeletonData = &skelData;

	u32 numBones = m_SkeletonData->GetNumBones();
	Assert(numBones > 0);
	m_NumBones = numBones;
	m_Locals = rage_new Mat34V[numBones*2];
	m_Objects = m_Locals + numBones;

	m_Parent = parentMtx;

	Reset();

	Update();
}

////////////////////////////////////////////////////////////////////////////////

void crSkeleton::Shutdown()
{
	m_SkeletonData.Release();

	delete [] m_Locals;
	m_NumBones = 0;
	m_Locals = NULL;
	m_Objects = NULL;
	m_Parent = NULL;
}

////////////////////////////////////////////////////////////////////////////////

void crSkeleton::Reset()
{
	sysMemCpy(m_Locals, m_SkeletonData->GetDefaultTransforms(), sizeof(Mat34V)*m_NumBones);
}

////////////////////////////////////////////////////////////////////////////////

void crSkeleton::PartialReset(u32 boneIdx)
{
	u32 terminalIdx = GetTerminatingPartialBone(boneIdx);
	for (u32 i=boneIdx; i<terminalIdx; i++)
	{
		Mat34V & localMat = GetLocalMtx(i);
		localMat = GetSkeletonData().GetDefaultTransform(i);
	}
	PartialUpdate(boneIdx);
}

////////////////////////////////////////////////////////////////////////////////

void crSkeleton::CopyFrom(const crSkeleton& other)
{
	Assert(m_NumBones == other.m_NumBones);
	sysMemCpy(m_Locals, other.m_Locals, 2*m_NumBones*sizeof(Mat34V));
}

////////////////////////////////////////////////////////////////////////////////

void crSkeleton::Update()
{
	const s16* parentIndices = m_SkeletonData->GetParentIndices();
	PrefetchDC(parentIndices);

	const Mat34V* localMtx = m_Locals;
	Mat34V* objectMtx = m_Objects;
	u32 numBones = m_NumBones;
	for(u32 i=0; i<numBones; ++i, ++localMtx, ++objectMtx)
	{
		PrefetchDC(localMtx);
		PrefetchDC(objectMtx);
	}

	localMtx = m_Locals;
	objectMtx = m_Objects;
	*objectMtx = *localMtx;

	++localMtx;
	++objectMtx;
	++parentIndices;
	for(u32 i=1; i<numBones; ++i, ++localMtx, ++objectMtx, ++parentIndices)
	{
		rage::Transform(*objectMtx, m_Objects[*parentIndices], *localMtx);
	}
}

////////////////////////////////////////////////////////////////////////////////

void crSkeleton::Transform(Mat34V_In mtx)
{
	u32 numBones = m_NumBones;
	for(u32 i=0; i<numBones; ++i)
	{
		rage::Transform(m_Objects[i], mtx, m_Objects[i]);
	}
}

////////////////////////////////////////////////////////////////////////////////

void crSkeleton::PartialTransform(u32 boneIdx, Mat34V_In mtx, bool inclusive)
{
	u32 firstBone = boneIdx;
	u32 terminatingBone = GetTerminatingPartialBone(boneIdx);

	if(!inclusive)
	{
		firstBone++;
	}

	for(u32 i=firstBone; i<terminatingBone; ++i)
	{
		rage::Transform(m_Objects[i], mtx, m_Objects[i]);
	}
}

////////////////////////////////////////////////////////////////////////////////

void crSkeleton::PartialUpdate(u32 boneIdx, bool inclusive)
{
	const crBoneData* bd = m_SkeletonData->GetBoneData(boneIdx);
	if(bd->GetParent() || !inclusive)
	{
		u32 firstBone = boneIdx;
		u32 terminiatingBone = GetTerminatingPartialBone(firstBone);
		const s16* parentIndices = m_SkeletonData->GetParentIndices();

		if(!inclusive)
		{
			firstBone++;
		}

		for(u32 i=firstBone; i<terminiatingBone; ++i)
		{
			rage::Transform(m_Objects[i], m_Objects[parentIndices[i]], m_Locals[i]);
		}
	}
	else
	{
		Update();
	}
}

////////////////////////////////////////////////////////////////////////////////

void crSkeleton::InverseUpdate()
{
	m_Locals[0] = m_Objects[0];

	const s16* parentIndices = m_SkeletonData->GetParentIndices();

	u32 numBones = m_NumBones;
	for(u32 i=1; i<numBones; ++i)
	{
		UnTransformOrtho(m_Locals[i], m_Objects[parentIndices[i]], m_Objects[i]);
	}
}

////////////////////////////////////////////////////////////////////////////////

void crSkeleton::PartialInverseUpdate(u32 boneIdx, bool inclusive)
{
	const crBoneData* bd = m_SkeletonData->GetBoneData(boneIdx);
	if(bd->GetParent() || !inclusive)
	{
		u32 firstBone = boneIdx;
		u32 terminiatingBone = GetTerminatingPartialBone(firstBone);
		const s16* parentIndices = m_SkeletonData->GetParentIndices();
		
		if(!inclusive)
		{
			firstBone++;
		}

		for(u32 i=firstBone; i<terminiatingBone; ++i)
		{
			UnTransformOrtho(m_Locals[i], m_Objects[parentIndices[i]], m_Objects[i]);
		}
	}
	else
	{
		InverseUpdate();
	}
}

////////////////////////////////////////////////////////////////////////////////

void crSkeleton::Attach(bool isSkinned, Matrix43* outMtxs) const
{
	const Mat34V* objectMtx = m_Objects;
	u32 numBones = m_NumBones;

	PrefetchDC(objectMtx);
	PrefetchDC(outMtxs);

	if(isSkinned)
	{
		const Mat34V* invJointScaleMtx = m_SkeletonData->GetCumulativeInverseJoints();
		PrefetchDC(invJointScaleMtx);

		for(u32 i=0; i<numBones; ++i, ++outMtxs, ++objectMtx, ++invJointScaleMtx)
		{
			PrefetchDC(outMtxs+1);
			PrefetchDC(objectMtx+1);
			PrefetchDC(invJointScaleMtx+1);

			Mat34V mtx;
			rage::Transform(mtx, *objectMtx, *invJointScaleMtx);
			outMtxs->FromMatrix34(mtx);
		}
	}
	else
	{
		for(u32 i=0; i<numBones; ++i, ++objectMtx, ++outMtxs)
		{
			outMtxs->FromMatrix34(*objectMtx);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

void crSkeleton::GetGlobalMtx(u32 boneIdx, Mat34V_InOut outMtx) const
{
	TrapGE(boneIdx, m_NumBones);
	if(m_Parent)
	{
		rage::Transform(outMtx, *m_Parent, m_Objects[boneIdx]);
	}
	else
	{
		outMtx = m_Objects[boneIdx];
	}
}

////////////////////////////////////////////////////////////////////////////////

void crSkeleton::SetGlobalMtx(u32 boneIdx, Mat34V_In mtx)
{
	TrapGE(boneIdx, m_NumBones);
	if(m_Parent)
	{
		UnTransformOrtho(m_Objects[boneIdx], *m_Parent, mtx);
	}
	else
	{
		m_Objects[boneIdx] = mtx;
	}
}

////////////////////////////////////////////////////////////////////////////////

u32 crSkeleton::GetTerminatingPartialBone(u32 boneIdx) const
{
	TrapGE(boneIdx, m_NumBones);

	// the terminating bone is the first valid next bone found on the partial root bone, or any of its parents
	const crBoneData* bd = m_SkeletonData->GetBoneData(boneIdx);
	do
	{
		if(bd->GetNext())
		{
			return bd->GetNext()->GetIndex();
		}
		bd = bd->GetParent();
	}
	while(bd);

	// if no terminating bone was found, then termination is after the last possible bone
	return m_NumBones;
}

////////////////////////////////////////////////////////////////////////////////

void crSkeleton::SetSkeletonData(const crSkeletonData& skelData)
{
	if(Verifyf(skelData.GetNumBones() == m_SkeletonData->GetNumBones(), "Try to set a skeleton data with different number of bones: %d <-> %d", skelData.GetNumBones(), m_SkeletonData->GetNumBones()))
	{
		Assert(m_SkeletonData->GetSignature() == skelData.GetSignature());
		skelData.AddRef();
		m_SkeletonData.Release();
		m_SkeletonData = &skelData;
	}
}

////////////////////////////////////////////////////////////////////////////////

void crSkeleton::CopySkeletonData(const crSkeletonData& skelData)
{
	if(Verifyf(skelData.GetNumBones() == m_SkeletonData->GetNumBones(), "Try to copy a skeleton data with different number of bones: %d <-> %d", skelData.GetNumBones(), m_SkeletonData->GetNumBones()))
	{
		Assert(m_SkeletonData->GetSignature() == skelData.GetSignature());

		crBoneData* dstBoneData = const_cast<crBoneData*>(m_SkeletonData->GetBones());
		const crBoneData* srcBoneData = skelData.GetBones();

		Mat34V* dstInvJoints = const_cast<Mat34V*>(m_SkeletonData->GetCumulativeInverseJoints());
		const Mat34V* srcInvJoints = skelData.GetCumulativeInverseJoints();

		Mat34V* dstDefaultTransforms = const_cast<Mat34V*>(m_SkeletonData->GetDefaultTransforms());
		const Mat34V* srcDefaultTransforms = skelData.GetDefaultTransforms();

		s16* dstParentIndices = const_cast<s16*>(m_SkeletonData->GetParentIndices());
		const s16* srcParentIndices = skelData.GetParentIndices();

		atMap<u32,int>& dstBoneIdTable = const_cast<atMap<u32,int>&>(m_SkeletonData->GetBoneIdTable());
		const atMap<u32,int>& srcBoneIdTable = skelData.GetBoneIdTable();

		dstBoneIdTable = srcBoneIdTable;

		for (s32 i = 0; i < m_SkeletonData->GetNumBones(); ++i)
		{
			dstBoneData[i] = srcBoneData[i];
			dstInvJoints[i] = srcInvJoints[i];
			dstDefaultTransforms[i] = srcDefaultTransforms[i];
			dstParentIndices[i] = srcParentIndices[i];
		}

		u16* dstChildParentIndices = const_cast<u16*>(m_SkeletonData->GetChildParentIndices());
		const u16* srcChildParentIndices = skelData.GetChildParentIndices();

		for (u32 i = 0; i < m_SkeletonData->GetNumChildParents(); ++i)
		{
			dstChildParentIndices[i] = srcChildParentIndices[i];
		}

	}
}

////////////////////////////////////////////////////////////////////////////////

#if !__NO_OUTPUT
size_t crSkeleton::GetMemorySize() const
{
	return sizeof(crSkeleton) + (m_NumBones * 2 * sizeof(Mat34V));
}
#endif

////////////////////////////////////////////////////////////////////////////////

#if __PFDRAW

PFD_DECLARE_GROUP(Skeleton);

void crSkeleton::ProfileDraw() const
{
	if (PFDGROUP_Skeleton.Begin())
	{
		// TODO - Make these into widgets (probably from the calling function)
		static float jointRadius = 0.2f;
		static float axisSize = 0.4f;
		Color32 boneColor(255, 0, 255);
		Color32 jointColor(0, 255, 0);
		Color32 labelColor(30, 30, 80);
		Vector3 a, b;

		const crSkeletonData &skeletonData = GetSkeletonData();
		u32 totalBones = skeletonData.GetNumBones();
		for (u32 boneIndex = 0; boneIndex < totalBones; boneIndex++)
		{
			Matrix34 mBone;
			const crBoneData *boneData = skeletonData.GetBoneData(boneIndex);
			GetGlobalMtx(boneIndex, RC_MAT34V(mBone));
			a.Set(mBone.d);

			grcColor(jointColor);
			grcDrawSphere(jointRadius, mBone.d, 20, true, true);

			for (const crBoneData *child = boneData->GetChild(); child; child = child->GetNext())
			{
				GetGlobalMtx(child->GetIndex(), RC_MAT34V(mBone));

				grcWorldIdentity();
				b.Set(mBone.d);
				grcDrawLine(a, b, boneColor);

				grcColor(jointColor);
				grcDrawSphere(jointRadius, mBone.d, 20, true, true);

				grcColor(labelColor);
				char buf[256] = {0};
				grcDrawLabel(mBone.d, formatf(buf, "%s :%d", child->GetName(), child->GetIndex()));

				grcDrawAxis(axisSize, mBone);
			}
		}

		PFDGROUP_Skeleton.End();
	}
}
#endif //__PFDRAW

////////////////////////////////////////////////////////////////////////////////

} // namespace rage
