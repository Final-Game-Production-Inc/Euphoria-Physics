//
// crskeleton/bonedata.cpp
//
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved.
//

#include "bonedata.h"

#include "data/resource.h"
#include "data/safestruct.h"
#include "file/stream.h"
#include "file/token.h"
#include "vectormath/legacyconvert.h"

namespace rage
{

////////////////////////////////////////////////////////////////////////////////

crBoneData::crBoneData()
: m_Name(NULL)
, m_Dofs(0)
, m_NextIndex(-1)
, m_ParentIndex(-1)
, m_Index(0)
, m_BoneId(0)
, m_MirrorIndex(0xffff)
, m_DefaultTranslation(V_ZERO)
, m_DefaultRotation(V_IDENTITY)
, m_DefaultScale(V_ONE)
{
}

////////////////////////////////////////////////////////////////////////////////

crBoneData::crBoneData(datResource &rsc)
: m_Name(rsc)
{
#if ENABLE_MIRROR_TEST
	m_MirrorIndex = m_Index;
#endif
}

////////////////////////////////////////////////////////////////////////////////

crBoneData::~crBoneData()
{
}

////////////////////////////////////////////////////////////////////////////////

#if __DECLARESTRUCT
void crBoneData::DeclareStruct(datTypeStruct &s)
{
	SSTRUCT_BEGIN(crBoneData)
	SSTRUCT_FIELD(crBoneData, m_DefaultRotation)
	SSTRUCT_FIELD(crBoneData, m_DefaultTranslation)
	SSTRUCT_FIELD(crBoneData, m_DefaultScale)
	SSTRUCT_FIELD(crBoneData, m_NextIndex)
	SSTRUCT_FIELD(crBoneData, m_ParentIndex)
	SSTRUCT_FIELD(crBoneData, m_Name)
	SSTRUCT_FIELD(crBoneData, m_Dofs)
	SSTRUCT_FIELD(crBoneData, m_Index)
	SSTRUCT_FIELD(crBoneData, m_BoneId)
	SSTRUCT_FIELD(crBoneData, m_MirrorIndex)
	SSTRUCT_END(crBoneData)
}
#endif // __DECLARESTRUCT

////////////////////////////////////////////////////////////////////////////////

#if CR_DEV
bool crBoneData::Load_v100(fiTokenizer &tok,crBoneData **next,int &index)
{
	m_Index=(u16)index;
	m_BoneId = m_Index;
	m_MirrorIndex = m_Index;
	char temp[128];
	tok.GetToken(temp,sizeof(temp));
	m_Name = temp;

	if (stricmp(m_Name,"root")==0)
	{
		m_Dofs = ROTATE_X | ROTATE_Y | ROTATE_Z | TRANSLATE_X | TRANSLATE_Y | TRANSLATE_Z;
	}
	else
	{
		m_Dofs = ROTATE_X | ROTATE_Y | ROTATE_Z;
	}

	tok.MatchToken("{");
	tok.MatchVector("offset",RC_VECTOR3(m_DefaultTranslation));

	while(1)
	{
		tok.GetToken(temp,sizeof(temp));
		if(strcmp(temp,"bone")==0)
		{
			crBoneData *b=*next;
			(*next)++;
			AddChild(b);
			b->Load_v100(tok,next,index);
		}
		else if(strcmp(temp,"}")==0)
		{
			break;
		}
		else
		{
			Errorf("crBoneData:Load_v100() - error in file");
			return false;
		}
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////

bool ReadDOF(fiTokenizer &tok,u16 &status,u32 dofMask,u32 limitMask,float &lockValue,float &limitMin,float &limitMax)
{
	bool retVal=true;
	status |= dofMask;
	if (tok.CheckIToken("lock"))
	{
		retVal=false;
		status &= ~dofMask;
		lockValue = tok.GetFloat();
	}
	if (tok.CheckIToken("limit"))
	{
		status |= limitMask;
		limitMin = tok.GetFloat();
		limitMax = tok.GetFloat();
	}
	return retVal;
}

////////////////////////////////////////////////////////////////////////////////

bool crBoneData::Load_v101(fiTokenizer &tok,crBoneData **next,int &index)
{
	char buffer[128];
	float dummy;
	bool foundChild;

	m_Index=(u16)index;
	m_BoneId = m_Index;
	m_MirrorIndex = m_Index;
	tok.GetToken(buffer,sizeof(buffer));
	m_Name = buffer;
	tok.MatchToken("{");

	// offset
	m_DefaultTranslation = Vec3V(V_ZERO);
	if (tok.CheckIToken("offset"))
	{
		tok.GetVector(RC_VECTOR3(m_DefaultTranslation));
	}

	Vec3V defaultRotation = Vec3V(V_ZERO);
	m_DefaultRotation = QuatV(V_IDENTITY);

	m_Dofs = 0;

	while (1)
	{
		foundChild=false;

		tok.GetToken(buffer,sizeof(buffer));

		if (strncmp(buffer,"bone",4)==0)
		{
			crBoneData *b=*next;
			(*next)++;
			AddChild(b);
			b->Load_v101(tok,next,index);
			foundChild=true;
		}
		else if (stricmp(buffer,"}")==0)
		{
			break;
		}
		// rotation x
		else if (stricmp(buffer,"rotX")==0)
		{
			ReadDOF(tok,m_Dofs,ROTATE_X,HAS_ROTATE_LIMITS,defaultRotation[0],dummy,dummy);
		}
		// rotation y
		else if (stricmp(buffer,"rotY")==0)
		{
			ReadDOF(tok,m_Dofs,ROTATE_Y,HAS_ROTATE_LIMITS,defaultRotation[1],dummy,dummy);
		}
		// rotation z
		else if (stricmp(buffer,"rotZ")==0)
		{
			ReadDOF(tok,m_Dofs,ROTATE_Z,HAS_ROTATE_LIMITS,defaultRotation[2],dummy,dummy);
		}
		// translation x
		else if (stricmp(buffer,"transX")==0)
		{
			ReadDOF(tok,m_Dofs,TRANSLATE_X,HAS_TRANSLATE_LIMITS,m_DefaultTranslation[0],dummy,dummy);
		}
		// translation y
		else if (stricmp(buffer,"transY")==0)
		{
			ReadDOF(tok,m_Dofs,TRANSLATE_Y,HAS_TRANSLATE_LIMITS,m_DefaultTranslation[1],dummy,dummy);
		}
		// translation z
		else if (stricmp(buffer,"transZ")==0)
		{
			ReadDOF(tok,m_Dofs,TRANSLATE_Z,HAS_TRANSLATE_LIMITS,m_DefaultTranslation[2],dummy,dummy);
		}
		// scale x
		else if (stricmp(buffer,"scaleX")==0)
		{
			ReadDOF(tok,m_Dofs,SCALE_X,HAS_SCALE_LIMITS,dummy,dummy,dummy);
		}
		// scale y
		else if (stricmp(buffer,"scaleY")==0)
		{
			ReadDOF(tok,m_Dofs,SCALE_Y,HAS_SCALE_LIMITS,dummy,dummy,dummy);
		}
		// scale z
		else if (stricmp(buffer,"scaleZ")==0)
		{
			ReadDOF(tok,m_Dofs,SCALE_Z,HAS_SCALE_LIMITS,dummy,dummy,dummy);
		}
		// visibility
		// This will turn off visibility. Default is visible.
		else if (stricmp(buffer,"visibility")==0)
		{
			tok.GetInt();
		}

		if (!foundChild && !HasDofs(ROTATION | TRANSLATION | SCALE))
		{
			Errorf("crBoneData:Load_v101(): Name is %s, buffer is %s - error in file",m_Name.m_String,buffer);
			//return false;
		}
	}

	m_DefaultRotation = QuatVFromEulersXYZ(defaultRotation);

	return true;
}

////////////////////////////////////////////////////////////////////////////////

bool crBoneData::Load_v104Plus(fiTokenizer &tok, crBoneData **next, int &index)
{
	char buffer[128];
	float dummy;
	// bool foundChild;

	// index and name
	m_Index=(u16)index;
	tok.GetToken(buffer,sizeof(buffer));
	m_Name = buffer;
	tok.MatchToken("{");

	// bone id
	m_BoneId = m_Index;
	if(tok.CheckIToken("boneid"))
	{
		m_BoneId = u16(tok.GetInt());
	}

	m_MirrorIndex = m_Index;
	if(tok.CheckIToken("mirror"))
	{
		m_MirrorIndex = u16(tok.GetInt());
	}

	// offset
	m_DefaultTranslation = Vec3V(V_ZERO);
	if (tok.CheckIToken("offset"))
	{
		tok.GetVector(RC_VECTOR3(m_DefaultTranslation));
	}

	// euler
	m_DefaultRotation = QuatV(V_IDENTITY);
	if (tok.CheckIToken("euler"))
	{
		Vec3V rotation;
		tok.GetVector(RC_VECTOR3(rotation));
		m_DefaultRotation = QuatVFromEulersXYZ(rotation);
	}

	// scale
	m_DefaultScale = Vec3V(V_ONE);
	if (tok.CheckIToken("scale"))
	{
		tok.GetVector(RC_VECTOR3(m_DefaultScale));
	}

	m_Dofs = 0;

	while (1)
	{
		// foundChild=false;

		tok.GetToken(buffer,sizeof(buffer));

		if (strncmp(buffer,"bone",4)==0)
		{
			crBoneData *b=*next;
			(*next)++;
			b->m_Index = u16(++index);
			AddChild(b);
			b->Load_v104Plus(tok,next,index);
		}
		else if (stricmp(buffer,"}")==0)
		{
			break;
		}
		// rotation x
		else if (stricmp(buffer,"rotX")==0)
		{
			ReadDOF(tok,m_Dofs,ROTATE_X,HAS_ROTATE_LIMITS,dummy,dummy,dummy);
		}
		// rotation y
		else if (stricmp(buffer,"rotY")==0)
		{
			ReadDOF(tok,m_Dofs,ROTATE_Y,HAS_ROTATE_LIMITS,dummy,dummy,dummy);
		}
		// rotation z
		else if (stricmp(buffer,"rotZ")==0)
		{
			ReadDOF(tok,m_Dofs,ROTATE_Z,HAS_ROTATE_LIMITS,dummy,dummy,dummy);
		}
		// translation x
		else if (stricmp(buffer,"transX")==0)
		{
			ReadDOF(tok,m_Dofs,TRANSLATE_X,HAS_TRANSLATE_LIMITS,dummy,dummy,dummy);
		}
		// translation y
		else if (stricmp(buffer,"transY")==0)
		{
			ReadDOF(tok,m_Dofs,TRANSLATE_Y,HAS_TRANSLATE_LIMITS,dummy,dummy,dummy);
		}
		// translation z
		else if (stricmp(buffer,"transZ")==0)
		{
			ReadDOF(tok,m_Dofs,TRANSLATE_Z,HAS_TRANSLATE_LIMITS,dummy,dummy,dummy);
		}
		// scale x
		else if (stricmp(buffer,"scaleX")==0)
		{
			ReadDOF(tok,m_Dofs,SCALE_X,HAS_SCALE_LIMITS,dummy,dummy,dummy);
		}
		// scale y
		else if (stricmp(buffer,"scaleY")==0)
		{
			ReadDOF(tok,m_Dofs,SCALE_Y,HAS_SCALE_LIMITS,dummy,dummy,dummy);
		}
		// scale z
		else if (stricmp(buffer,"scaleZ")==0)
		{
			ReadDOF(tok,m_Dofs,SCALE_Z,HAS_SCALE_LIMITS,dummy,dummy,dummy);
		}
		// visibility
		// This will turn off visibility. Default is visible.
		else if (stricmp(buffer,"visibility")==0)
		{
			tok.GetInt();
		}
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////

static void WriteIndent(fiStream* s, const int indent)
{
	for (int i = 0; i < indent; ++i)
	{
		s->Write("\t", 1);
	}
}

////////////////////////////////////////////////////////////////////////////////

bool crBoneData::Save(fiStream* f,  int indent) const
{
	char buffer[512];

	WriteIndent(f, indent);
	formatf(buffer, "bone%d %s {\r\n", m_Index, m_Name.c_str());
	f->Write(buffer, (int)strlen(buffer));

	// Don't bother writing out root boneid
	if (m_BoneId != 0)
	{
		WriteIndent(f, indent+1);
		formatf(buffer, "boneid %d\r\n", m_BoneId);
		f->Write(buffer, (int)strlen(buffer));
	}

	WriteIndent(f, indent+1);
	formatf(buffer, "mirror %d\r\n", m_MirrorIndex);
	f->Write(buffer, (int)strlen(buffer));

	if (!IsZeroAll(m_DefaultTranslation))
	{
		WriteIndent(f, indent+1);
		formatf(buffer, "offset %f %f %f\r\n", m_DefaultTranslation[0], m_DefaultTranslation[1], m_DefaultTranslation[2]);
		f->Write(buffer, (int)strlen(buffer));
	}
	Vec3V defaultRotation = QuatVToEulersXYZ(m_DefaultRotation);
	if (!IsZeroAll(defaultRotation))
	{
		WriteIndent(f, indent+1);
		formatf(buffer, "euler %f %f %f\r\n", defaultRotation[0], defaultRotation[1], defaultRotation[2]);
		f->Write(buffer, (int)strlen(buffer));
	}
	if (!IsZeroAll(m_DefaultScale))
	{
		WriteIndent(f, indent+1);
		formatf(buffer, "scale %f %f %f\r\n", m_DefaultScale[0], m_DefaultScale[1], m_DefaultScale[2]);
		f->Write(buffer, (int)strlen(buffer));
	}
	WriteIndent(f, indent+1);
	if (m_Dofs & TRANSLATE_X)
	{
		formatf(buffer, "transX ");
		f->Write(buffer, (int)strlen(buffer));
	}
	if (m_Dofs & TRANSLATE_Y)
	{
		formatf(buffer, "transY ");
		f->Write(buffer, (int)strlen(buffer));
	}
	if (m_Dofs & TRANSLATE_Z)
	{
		formatf(buffer, "transZ ");
		f->Write(buffer, (int)strlen(buffer));
	}
	if (m_Dofs & ROTATE_X)
	{
		formatf(buffer, "rotX ");
		f->Write(buffer, (int)strlen(buffer));
	}
	if (m_Dofs & ROTATE_Y)
	{
		formatf(buffer, "rotY ");
		f->Write(buffer, (int)strlen(buffer));
	}
	if (m_Dofs & ROTATE_Z)
	{
		formatf(buffer, "rotZ ");
		f->Write(buffer, (int)strlen(buffer));
	}
	if (m_Dofs & SCALE_X)
	{
		formatf(buffer, "scaleX ");
		f->Write(buffer, (int)strlen(buffer));
	}
	if (m_Dofs & SCALE_Y)
	{
		formatf(buffer, "scaleY ");
		f->Write(buffer, (int)strlen(buffer));
	}
	if (m_Dofs & SCALE_Z)
	{
		formatf(buffer, "scaleZ ");
		f->Write(buffer, (int)strlen(buffer));
	}

	formatf(buffer, "\r\n");
	f->Write(buffer, (int)strlen(buffer));

	if (const crBoneData* child = GetChild())
	{
		child->Save(f, indent+1);
	}

	WriteIndent(f, indent);
	formatf(buffer, "}\r\n");
	f->Write(buffer, (int)strlen(buffer));

	if (const crBoneData* next = GetNext())
	{
		next->Save(f, indent);
	}

	return true;
}
#endif // CR_DEV

////////////////////////////////////////////////////////////////////////////////

void crBoneData::AddChild(crBoneData* child)
{
	child->m_ParentIndex = m_Index;

	if (m_Dofs & HAS_CHILD)
	{
		crBoneData* b = this+1;
		while (crBoneData* next = b->GetIndexedBone(b->m_NextIndex))
		{
			b = next;
		}
		b->m_NextIndex = child->m_Index;
	}
	else
	{
		m_Dofs |= HAS_CHILD;
	}
}

////////////////////////////////////////////////////////////////////////////////

void crBoneData::CalcCumulativeJointScaleOrients(Mat34V_InOut outMtx) const
{
	outMtx = Mat34V(V_IDENTITY);

	const crBoneData* bd = this;
	while(bd)
	{
		Mat34V mtx;
		Mat34VFromQuatV(mtx, bd->GetDefaultRotation());
		Transform(outMtx, mtx, outMtx);
		Translate(outMtx, outMtx, bd->GetDefaultTranslation());

		bd = bd->GetParent();
	}
}

////////////////////////////////////////////////////////////////////////////////

u64 crBoneData::GetSignatureNonChiral() const
{
	return (u64(m_BoneId)<<32) | u64(((m_Dofs&(crBoneData::TRANSLATE_X|crBoneData::TRANSLATE_Y|crBoneData::TRANSLATE_Z))?0x1:0x0)|((m_Dofs&(crBoneData::ROTATE_X|crBoneData::ROTATE_Y|crBoneData::ROTATE_Z))?0x2:0x0)|((m_Dofs&(crBoneData::SCALE_X|crBoneData::SCALE_Y|crBoneData::SCALE_Z))?0x4:0x0));
}

////////////////////////////////////////////////////////////////////////////////

} // namespace rage
