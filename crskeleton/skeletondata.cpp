//
// crskeleton/skeletondata.cpp
//
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved.
//

#include "skeletondata.h"

#include "bonedata.h"

#include "atl/map_struct.h"
#include "crmetadata/properties.h"
#include "data/resource.h"
#include "data/safestruct.h"
#include "data/struct.h"
#include "file/asset.h"
#include "file/token.h"
#include "system/criticalsection.h"
#include "system/memops.h"
#include "system/memory.h"
#include "vectormath/classes.h"
#include "zlib/zlib.h"

#include <set>

namespace rage
{

////////////////////////////////////////////////////////////////////////////////

crSkeletonData::crSkeletonData()
: m_Bones(NULL)
, m_ParentIndices(NULL)
, m_ChildParentIndices(NULL)
, m_CumulativeInverseTransforms(NULL)
, m_DefaultTransforms(NULL)
, m_NumBones(0)
, m_NumChildParents(0)
, m_RefCount(1)
, m_Properties(NULL)
, m_Signature(0)
, m_SignatureNonChiral(0)
, m_SignatureComprehensive(0)
{
}

////////////////////////////////////////////////////////////////////////////////

crSkeletonData::crSkeletonData(datResource &rsc)
: m_BoneIdTable(rsc)
, m_Bones(rsc)
, m_CumulativeInverseTransforms(rsc)
, m_DefaultTransforms(rsc)
, m_ParentIndices(rsc)
, m_ChildParentIndices(rsc)
, m_Properties(rsc)
{
	for (int i=0; i<m_NumBones; i++)
	{
		::new (&m_Bones[i]) crBoneData(rsc);
	}

#if ENABLE_MIRROR_TEST
	GenerateMirrorIndices();
#endif
}

////////////////////////////////////////////////////////////////////////////////

crSkeletonData::~crSkeletonData()
{
	delete [] m_Bones;
	delete [] m_ParentIndices;
	delete [] m_ChildParentIndices;
	delete [] m_CumulativeInverseTransforms;
	delete [] m_DefaultTransforms;
	delete m_Properties;

	m_Bones = NULL;
	m_ParentIndices = NULL;
	m_ChildParentIndices = NULL;
	m_CumulativeInverseTransforms = NULL;
	m_DefaultTransforms = NULL;
	m_Properties = NULL;
}

////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_PLACE(crSkeletonData);

////////////////////////////////////////////////////////////////////////////////

#if __DECLARESTRUCT
void crSkeletonData::DeclareStruct(class datTypeStruct &s)
{
	// crBoneData now has a ConstString, which has a destructor, so we need to do this!
	datSwapArrayCount(m_Bones,m_NumBones);

	pgBase::DeclareStruct(s);
	SSTRUCT_BEGIN_BASE(crSkeletonData, pgBase)
	SSTRUCT_FIELD(crSkeletonData, m_BoneIdTable)
	SSTRUCT_DYNAMIC_ARRAY(crSkeletonData, m_Bones, m_NumBones)
	SSTRUCT_DYNAMIC_ARRAY(crSkeletonData, m_CumulativeInverseTransforms, m_NumBones)
	SSTRUCT_DYNAMIC_ARRAY(crSkeletonData, m_DefaultTransforms, m_NumBones)
	SSTRUCT_DYNAMIC_ARRAY(crSkeletonData, m_ParentIndices, m_NumBones)
	SSTRUCT_DYNAMIC_ARRAY(crSkeletonData, m_ChildParentIndices, m_NumChildParents)
	SSTRUCT_FIELD(crSkeletonData, m_Properties)
	SSTRUCT_FIELD(crSkeletonData, m_Signature)
	SSTRUCT_FIELD(crSkeletonData, m_SignatureNonChiral)
	SSTRUCT_FIELD(crSkeletonData, m_SignatureComprehensive)
	SSTRUCT_FIELD(crSkeletonData, m_RefCount)
	SSTRUCT_FIELD(crSkeletonData, m_NumBones)
	SSTRUCT_FIELD(crSkeletonData, m_NumChildParents)
	SSTRUCT_IGNORE(crSkeletonData, m_Padding)
	SSTRUCT_END(crSkeletonData)
}
#endif // __DECLARESTRUCT

////////////////////////////////////////////////////////////////////////////////

#if CR_DEV
crSkeletonData::crSkeletonData(const crSkeletonData& src)
: m_Bones(NULL)
, m_ParentIndices(NULL)
, m_ChildParentIndices(NULL)
, m_CumulativeInverseTransforms(NULL)
, m_DefaultTransforms(NULL)
, m_NumBones(src.m_NumBones)
, m_NumChildParents(src.m_NumChildParents)
, m_RefCount(1)
, m_Properties(NULL)
, m_Signature(src.m_Signature)
, m_SignatureNonChiral(src.m_SignatureNonChiral)
, m_SignatureComprehensive(src.m_SignatureComprehensive)
, m_BoneIdTable(src.m_BoneIdTable)
{
	m_Bones = rage_new crBoneData[m_NumBones];
	// Can't just do a memory copy since there is a ConstString member in crBoneData
	for(int i=0; i<m_NumBones; ++i)
	{
		m_Bones[i] = src.m_Bones[i];
	}

	// default transforms
	m_DefaultTransforms = rage_new Mat34V[m_NumBones];
	sysMemCpy(m_DefaultTransforms, src.m_DefaultTransforms, m_NumBones*sizeof(Mat34V));

	// cumulative joint orients
	m_CumulativeInverseTransforms = rage_new Mat34V[m_NumBones];
	sysMemCpy(m_CumulativeInverseTransforms, src.m_CumulativeInverseTransforms, m_NumBones*sizeof(Mat34V));

	// setup parent indices
	m_ParentIndices = rage_new s16[m_NumBones];
	sysMemCpy(m_ParentIndices, src.m_ParentIndices, m_NumBones*sizeof(s16));

	m_ChildParentIndices = rage_aligned_new(16) u16[m_NumChildParents];
	sysMemCpy(m_ChildParentIndices, src.m_ChildParentIndices, m_NumChildParents*sizeof(u16));

	if(src.m_Properties)
	{
		m_Properties = src.m_Properties->Clone();
	}
}

crSkeletonData* crSkeletonData::AllocateAndLoad(const char* filename, int* outVersion)
{
	Assert(filename);
	crSkeletonData* skelData = rage_new crSkeletonData;
	if(!skelData->Load(filename, outVersion))
	{
		Errorf("crSkeletonData::AllocateAndLoad - failed to load '%s'", filename);
		delete skelData;
		return NULL;
	}

	return skelData;
}

////////////////////////////////////////////////////////////////////////////////

bool crSkeletonData::Load(const char *filename, int* outVersion)
{
	fiStream* f = ASSET.Open(filename, "skel", false, true);
	if(f==0)
	{
		Errorf("crSkeletonData::Load - failed to open file '%s'", filename);
		return false;
	}

	fiTokenizer T(filename,f);
	char buffer[256];

	int version;
	if (T.CheckIToken("version:"))
	{
		version = T.GetInt();
		if (outVersion)
		{
			*outVersion = version;
		}
		Assertf((version>=101) && (version <= 110), "crSkeletonData::Load - skeleton '%s' has unknown version %d.\r\nProbably caused by skeleton having been exported with copy of REX built using newer version of Rage than this executable.\r\n",filename,version);	
	}
	else
	{
		// pre-versioning, < 01/29/2002
		version=100;
	}

	if(T.CheckIToken("AuthoredOrientation"))
	{
		T.GetInt();
	}

	T.MatchToken("NumBones");
	int nb = T.GetInt();
	AssertMsg(nb <= 65535-1 , "Too many bones");
	m_NumBones = (u16)nb;

	// find first bone
	T.GetToken(buffer,sizeof(buffer));
	bool valid = strncmp(buffer,"bone",4) == 0;

	bool success = false;
	if(valid)
	{
		m_Bones=rage_new crBoneData[m_NumBones];

		crBoneData *next=m_Bones+1;
		int index=0;

		switch (version) {
			case 100:
				success=m_Bones->Load_v100(T,&next,index);
				break;
			case 101:
			case 102:
			case 103:
				success=m_Bones->Load_v101(T,&next,index);
				break;
			case 104:
			case 105:
			case 106:
			case 107:
			case 108:
			case 109:
			case 110:
				// intentional fall through
				success=m_Bones->Load_v104Plus(T,&next,index);
				break;
		}

		if(success)
		{
			Init();
		}
	}

	f->Close();

#if ENABLE_MIRROR_TEST
	GenerateMirrorIndices();
#endif

	return success;
}

////////////////////////////////////////////////////////////////////////////////

bool crSkeletonData::Save(const char* filename)
{
	fiStream* f = ASSET.Create(filename, "skel");
	if (f==0)
	{
		return false;
	}

	char buffer[512];

	formatf(buffer, "Version: 110\r\n");
	f->Write(buffer, (int)strlen(buffer));
	formatf(buffer, "AuthoredOrientation %d\r\n", 1); // Unused
	f->Write(buffer, (int)strlen(buffer));
	formatf(buffer, "NumBones %d\r\n", m_NumBones);
	f->Write(buffer, (int)strlen(buffer));

	m_Bones[0].Save(f);

	f->Close();

	return true;
}

////////////////////////////////////////////////////////////////////////////////

void crSkeletonData::Init()
{
	// setup bone ids
	bool hasBoneIds = false;
	for(int i=0; i<m_NumBones; i++)
	{
		crBoneData& b=m_Bones[i];
		if(b.GetBoneId() != i)
		{
			hasBoneIds = true;
			break;
		}
	}
	if(hasBoneIds)
	{
		m_BoneIdTable.Create(m_NumBones);
		for(int i=0; i<m_NumBones; i++)
		{
			m_BoneIdTable.Insert(m_Bones[i].GetBoneId(), i);
		}
	}

	// default transforms
	m_DefaultTransforms = rage_new Mat34V[m_NumBones];
	for(int i=0; i<m_NumBones; ++i)
	{
		const crBoneData& bd = m_Bones[i];
		Mat34V& mtx = m_DefaultTransforms[i];
		Mat34VFromQuatV(mtx, bd.GetDefaultRotation(), bd.GetDefaultTranslation());

		if(!IsEqualAll(Vec3V(V_ONE), bd.GetDefaultScale()))
		{
			Scale3x3(mtx, bd.GetDefaultScale(), mtx);
		}
	}

	// calculate cumulative joint orients
	m_CumulativeInverseTransforms = rage_new Mat34V[m_NumBones];
	for(int i=0; i<m_NumBones; ++i)
	{
		Mat34V cumulative;
		m_Bones[i].CalcCumulativeJointScaleOrients(cumulative);
		InvertTransformOrtho(m_CumulativeInverseTransforms[i], cumulative);
	}

	// setup parent indices
	m_ParentIndices = rage_new s16[m_NumBones];
	for(int i=0; i<m_NumBones; i++)
	{
		const crBoneData* bd = GetBoneData(i)->GetParent();
		m_ParentIndices[i] = s16(bd ? bd->GetIndex() : -1);
	}

	GenerateChildParentIndices();

	// calculate signature
	m_Signature = 0;
	for(int i=0; i<m_NumBones; ++i)
	{
		u64 idAndDofs = (u64(m_Bones[i].GetBoneId())<<32) | u64(m_Bones[i].GetDofs());

		m_Signature = crc32(m_Signature, (const u8*)&idAndDofs, sizeof(u64));
	}

	// calculate signature (non-chiral)
	m_SignatureNonChiral = 0;
	if(HasBoneIds())
	{
		for(atMap<u32,int>::Iterator it = m_BoneIdTable.CreateIterator(); !it.AtEnd(); it.Next())
		{
			u64 idAndDofs = m_Bones[it.GetData()].GetSignatureNonChiral();
			m_SignatureNonChiral = crc32(m_SignatureNonChiral, (const u8*)&idAndDofs, sizeof(u64));
		}
	}
	else
	{
		for(int idx=0; idx<m_NumBones; ++idx)
		{
			u64 idAndDofs = m_Bones[idx].GetSignatureNonChiral();
			m_SignatureNonChiral = crc32(m_SignatureNonChiral, (const u8*)&idAndDofs, sizeof(u64));
		}
	}

	// calculate signature (comprehensive)
	m_SignatureComprehensive = 0;
	for(int i=0; i<m_NumBones; ++i)
	{
		u64 idAndDofs = (u64(m_Bones[i].GetBoneId())<<32) | u64(m_Bones[i].GetDofs());

		m_SignatureComprehensive = crc32(m_SignatureComprehensive, (const u8*)&idAndDofs, sizeof(u64));

		m_SignatureComprehensive = crc32(m_SignatureComprehensive, (const u8*)&m_Bones[i].GetDefaultTranslation(), sizeof(Vec3V));
		m_SignatureComprehensive = crc32(m_SignatureComprehensive, (const u8*)&m_Bones[i].GetDefaultRotation(), sizeof(QuatV));
		m_SignatureComprehensive = crc32(m_SignatureComprehensive, (const u8*)&m_Bones[i].GetDefaultScale(), sizeof(Vec3V));
	}
}

////////////////////////////////////////////////////////////////////////////////

void crSkeletonData::GenerateMirrorIndices()
{
	// Based on R*N naming convention. Sub-string _L_ for left, _R_ for right

	for(int i=0; i<m_NumBones; ++i)
	{
		crBoneData *b=&m_Bones[i];
		b->m_MirrorIndex = b->m_Index;
	}

	char temp[128];
	for(int i=0;i<m_NumBones;i++)
	{
		crBoneData *b=&m_Bones[i];

		StringNormalize(temp, b->GetName(), sizeof(temp));
		if (char* pos = strstr(temp, "_l_"))
		{
			pos[1] = 'r';
		}
		else if (strncmp(temp, "l_", 2) == 0)
		{
			temp[0] = 'r';
		}
		else
		{
			continue;
		}

		crBoneData *m=0;
		for(int j=0;j<m_NumBones;j++)
		{
			if(stricmp(m_Bones[j].GetName(),temp)==0)
			{
				m=&m_Bones[j];
				break;
			}
		}

		if (m!=0)
		{
			b->m_MirrorIndex = m->m_Index;
			m->m_MirrorIndex = b->m_Index;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

void crSkeletonData::GenerateMirrorIndices2()
{
	for (int i = 0; i < m_NumBones; ++i)
	{
		crBoneData* b = &m_Bones[i];
		b->m_MirrorIndex = b->m_Index;
	}

	char temp[128];
	for (int i = 0; i < m_NumBones; ++i)
	{
		crBoneData* b = &m_Bones[i];
		int len = (int)strlen(b->GetName());
		if (len < 3)
		{
			continue;
		}
		if (b->GetName()[len-1] != 'l' || b->GetName()[len-2]!='_')
		{
			continue;
		}

		safecpy(temp, b->GetName());
		temp[len-1]='r';
		crBoneData* m = 0;
		for (int j = 0; j < m_NumBones; ++j)
		{
			if (stricmp(m_Bones[j].GetName(), temp) == 0)
			{
				m = &m_Bones[j];
			}
		}
		if (m == 0)
		{
			b->m_MirrorIndex = b->m_Index;
			continue;
		}
		b->m_MirrorIndex = m->m_Index;
		m->m_MirrorIndex = b->m_Index;
	}
}

////////////////////////////////////////////////////////////////////////////////

void crSkeletonData::GenerateChildParentIndices()
{
	const u32 groupSize = 4;
#if __RESOURCECOMPILER
	sysMemStartTemp( );
#endif
	std::set<u32>* setTodo = new std::set<u32>();	
	for(u32 i=1; i < m_NumBones; ++i)
	{
		setTodo->insert(i);
	}
#if __RESOURCECOMPILER
	sysMemEndTemp( );
#endif
	// generate child parent indices (greedy algorithm)
	atArray<u16> indices;
	while(!setTodo->empty()) {
		u32 quadCurrentSize = 0;
		u32 childIndices[groupSize];
		u32 parentIndices[groupSize];

		// find candidate joints...
		for(std::set<u32>::const_iterator ite = setTodo->begin(); (ite != setTodo->end()) && (quadCurrentSize < groupSize); ite++) {
			u32 idxCurrent = *ite;
			s32 idxParent = m_ParentIndices[idxCurrent];

			// joint must not have his parent in the "todo" set
			if (setTodo->find(idxParent) != setTodo->end())
				continue;

			if(idxParent == -1)
			{
				idxParent = 0;
			}

			childIndices[quadCurrentSize] = idxCurrent;
			parentIndices[quadCurrentSize] = u32(idxParent);
			quadCurrentSize++;
		}

		// there is no candidate. it means there is circular dependency somewhere
		Assert(quadCurrentSize > 0);

		// replicate last elements
		for( u32 i = quadCurrentSize; i < groupSize; i++ ) {
			childIndices[i] = childIndices[i-1];
			parentIndices[i] = parentIndices[i-1];
		}

		#if __RESOURCECOMPILER
		sysMemStartTemp( );
		#endif
		// remove from joint set
		for( u32 i = 0; i < quadCurrentSize; i++ ) {
			setTodo->erase(childIndices[i]);
		}
		
		// add the quad
		for( u32 i = 0; i < groupSize; i++ ) {
			indices.Grow() = u16(childIndices[i]);
			indices.Grow() = u16(parentIndices[i]);
		}
		#if __RESOURCECOMPILER
		sysMemEndTemp( );
		#endif
	}

	u32 numIndices = indices.GetCount();
	m_ChildParentIndices = rage_aligned_new(16) u16[numIndices];
	sysMemCpy(m_ChildParentIndices, indices.GetElements(), numIndices*sizeof(u16));
	m_NumChildParents = u16(numIndices);

	sysMemStartTemp( );
	indices.Reset();
	delete setTodo;
	setTodo = NULL;
	sysMemEndTemp( );

	
}
#endif // CR_DEV

////////////////////////////////////////////////////////////////////////////////

const crBoneData* crSkeletonData::FindBoneData(const char* name) const
{
	for(int i=0; i<m_NumBones; ++i)
	{
		const crBoneData& bd = m_Bones[i];
		if(stricmp(bd.GetName(), name) == 0)
		{
			return &bd;
		}
	}

	return NULL;
}

////////////////////////////////////////////////////////////////////////////////

u32 crSkeletonData::GetNumAnimatableDofs(u32 dofFlags) const
{
	// calculate the number of bones that have translation and rotation dofs
	u32 numDofs = 0;
	for(int i=0; i<m_NumBones; ++i)
	{
		const crBoneData* boneData = GetBoneData(i);
		if(boneData)
		{
			// NOTE: the DOFs in boneData are currently channel dofs NOT track dofs!
			if ((dofFlags&crBoneData::TRANSLATION) && boneData->HasDofs(crBoneData::TRANSLATION))
			{
				++numDofs;
			}

			if((dofFlags&crBoneData::ROTATION) && boneData->HasDofs(crBoneData::ROTATION))
			{
				++numDofs;
			}
			if((dofFlags&crBoneData::SCALE) && boneData->HasDofs(crBoneData::SCALE))
			{
				++numDofs;
			}
		}
	}

	return numDofs;
}

////////////////////////////////////////////////////////////////////////////////

bool crSkeletonData::ConvertBoneIdToIndex(u16 boneId, int& outBoneIdx) const
{
	if(HasBoneIds())
	{
		const int *boneIndex = m_BoneIdTable.Access(boneId);
		if(boneIndex)
		{
			outBoneIdx = *boneIndex;
			return true;
		}
	}
	else if(boneId < m_NumBones)
	{
		outBoneIdx = boneId;
		return true;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////

bool crSkeletonData::ConvertBoneIndexToId(int boneIdx, u16& outBoneId) const
{
	if(HasBoneIds())
	{
		if(boneIdx < m_NumBones)
		{
			outBoneId = m_Bones[boneIdx].GetBoneId();
			return true;
		}
		return false;
	}
	else if(boneIdx < m_NumBones)
	{
		outBoneId = static_cast<u16>(boneIdx);
		return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////

void crSkeletonData::ComputeObjectTransform(int boneIdx, Mat34V_InOut result) const
{
	const crBoneData* bd = GetBoneData(boneIdx);
	Mat34V sum(V_IDENTITY);
	while (bd)
	{
		Transform(sum, GetDefaultTransform(bd->GetIndex()), sum);
		bd = bd->GetParent();
	}
	result = sum;
}

////////////////////////////////////////////////////////////////////////////////

void crSkeletonData::ComputeGlobalTransform(int boneIdx, Mat34V_In parentMtx, Mat34V_InOut result) const
{
	Mat34V localTransform;
	ComputeObjectTransform(boneIdx, localTransform);
	Transform(result, parentMtx, localTransform);
}

////////////////////////////////////////////////////////////////////////////////

#define USE_BONE_NAME_REGISTRY (!__FINAL)

#if USE_BONE_NAME_REGISTRY

struct BoneNameRegistry
{
	static const int sm_MaxNumEntries = 512;
	static const int sm_MaxBoneNameLen = 64;

	BoneNameRegistry();
	~BoneNameRegistry();

	void AddName(u16 id, const char* name);
	const char* FindName(u16 id) const;

private:
	typedef atMapMemoryPool<u16, const char*, 4> MemoryPool;
	typedef atMap<u16, const char*, atMapHashFn<u16>, atMapEquals<u16>, MemoryPool::MapFunctor > Map;
	typedef Map::Entry MapEntry;

	mutable sysCriticalSectionToken m_CriticalSectionToken;
	atRangeArray<char[sm_MaxBoneNameLen], sm_MaxNumEntries> m_Names;
	MemoryPool m_RegistryPool;
	Map m_Registry;
};

static BoneNameRegistry s_BoneNameRegistry;

////////////////////////////////////////////////////////////////////////////////

BoneNameRegistry::BoneNameRegistry()
: m_Registry(atMapHashFn<u16>(), atMapEquals<u16>(), m_RegistryPool.m_Functor)
{
	m_RegistryPool.Create<MapEntry>(sm_MaxNumEntries);
	m_Registry.Create(sm_MaxNumEntries, false);
}

////////////////////////////////////////////////////////////////////////////////

BoneNameRegistry::~BoneNameRegistry()
{
	m_Registry.Kill();
}

////////////////////////////////////////////////////////////////////////////////

void BoneNameRegistry::AddName(u16 id, const char* name)
{
	if(id)
	{
		sysCriticalSection criticalSection(m_CriticalSectionToken);

		int idx = m_Registry.GetNumUsed();
		if(idx < sm_MaxNumEntries)
		{
			if(!m_Registry.Access(id))
			{
				char* n = m_Names[idx];
				safecpy(n, name, sm_MaxBoneNameLen);
				m_Registry.Insert(id, n);
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

const char* BoneNameRegistry::FindName(u16 id) const
{
	if(id)
	{
		sysCriticalSection criticalSection(m_CriticalSectionToken);

		const char*const* entry = m_Registry.Access(id);
		if(entry)
		{
			return *entry;
		}
	}

	return NULL;
}

#endif // USE_BONE_NAME_REGISTRY

////////////////////////////////////////////////////////////////////////////////

u16 crSkeletonData::ConvertBoneNameToId(const char* boneIdName)
{
	Assert(boneIdName);
	if(boneIdName[0] == '#')
	{
		return u16(atoi(boneIdName+1));
	}
	else
	{
#if USE_BONE_NAME_REGISTRY
		s_BoneNameRegistry.AddName(atHash16U(boneIdName), boneIdName);
#endif // USE_BONE_NAME_REGISTRY

		return atHash16U(boneIdName);
	}
}

////////////////////////////////////////////////////////////////////////////////

const char* crSkeletonData::DebugConvertBoneIdToName(u16 id)
{
	if(id)
	{
#if USE_BONE_NAME_REGISTRY
		return s_BoneNameRegistry.FindName(id);
#endif // USE_BONE_NAME_REGISTRY
	}
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////

bool crSkeletonData::DebugRegisterBoneIdToName(u16 id, const char* name)
{
	if(id && name)
	{
#if USE_BONE_NAME_REGISTRY
		if(!s_BoneNameRegistry.FindName(id))
		{
			s_BoneNameRegistry.AddName(id, name);
			return true;
		}
#endif // USE_BONE_NAME_REGISTRY
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////

void crSkeletonData::SetProperties(crProperties* properties)
{
	delete m_Properties;
	m_Properties = properties;
}

////////////////////////////////////////////////////////////////////////////////

} // namespace rage
