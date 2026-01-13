//
// grcore/effect_update.cpp
//
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved.
//

#include "effect.h"
#include "effect_config.h"

#include "device.h"
#include "texture.h"

#include "file/device.h"
#include "system/stack.h"

#if CHECK_GPU_DATA
#include "../phbound/surfacegrid.h"
#endif	//CHECK_GPU_DATA


namespace rage {

#if CHECK_GPU_DATA
	static bool checkFloatArray(const void *const fArray, const int count, u8 constantType, const char*ASSERT_ONLY(effectName), const char* ASSERT_ONLY(varName))
	{
		if (!fArray)
		{
			return true;
		}
		const float *fp = reinterpret_cast<const float*>(fArray);

		int constantSize = count == 1 ?  g_FloatSizeByType[constantType] : g_Float4SizeByType[constantType] * 4;
		for(int i=0; i<count; ++i)
		{
			for(int j=0; j<constantSize; ++j,++fp)
			{
				if (IsNanInMemory(fp))
				{
					Assertf(false, "NaN detected in shader parameters! (effect = '%s', varName='%s', count = %d, constantType = 0x%02x)", effectName, varName, count, constantType);
					return false;
				}
			}
		}
		return true;
	}
#endif	//CHECK_GPU_DATA


grcEffectTechnique grcEffect::LookupTechnique(const char *name,bool ASSERT_ONLY(mustExist)) const
{
	grcEffectTechnique result = LookupTechniqueByHash(atStringHash(name));
	Assertf(result != grcetNONE || !mustExist,"LookupTechnique(%s) in '%s' failed on required technique",name,m_EffectName);
	return result;
}


grcEffectTechnique grcEffect::LookupTechniqueByHash(u32 hashCode) const
{
	for (int i=0; i<m_Techniques.GetCount(); i++)
		if (m_Techniques[i].NameHash == hashCode)
			return (grcEffectTechnique) (i+1);
	return grcetNONE;
}

grcCBuffer *grcEffect::LookupGlobalConstantBufferByHash(u32 hash)
{
	for (int i=0; i<sm_GlobalsCBuf.GetCount(); i++)
		if (sm_GlobalsCBuf[i].GetNameHash() == hash)
			return &sm_GlobalsCBuf[i];
	return NULL;
}

grcEffectGlobalVar grcEffect::LookupGlobalVar(const char *name,bool ASSERT_ONLY(mustExist))
{
	u32 hashCode = atStringHash(name);
	for (int i=0; i<sm_Globals.GetCount(); i++)
		if (sm_Globals[i].SemanticHash == hashCode || sm_Globals[i].NameHash == hashCode) {
			// grcDebugf2("grcEffect::LookupGlobalVar(%s) returns %x",name,(i+1));
			return (grcEffectGlobalVar)(i+1);
		}
	Assertf(!mustExist,"LookupGlobalVar(%s) failed on required global",name);
	// grcDebugf2("grcEffect::LookupGlobalVar(%s) failed.",name);
	return grcegvNONE;
}


grcEffectVar grcEffect::LookupVar(const char *name,bool mustExist) const
{
	grcEffectVar result = LookupVarByHash(atStringHash(name));
	if (result == grcevNONE && mustExist)
		grcWarningf("LookupVar(%s) failed on required local (maybe due to unused var stripping?)",name);
	return result;
}


grcEffectVar grcEffect::LookupVarByHash(u32 hashCode) const
{
	//Note: this always returns a sampler var if querying a texture
	for (int i=0; i<m_Locals.GetCount(); i++)
		if (m_Locals[i].SemanticHash == hashCode || m_Locals[i].NameHash == hashCode)
			return (grcEffectVar) (i+1);

	return grcevNONE;
}

void grcEffect::SetVarUsageFlag(grcEffectVar var, u32 offset, bool onOff)
{
	if(onOff)
		m_Locals[(int)var-1].Usage |= offset;
	else
		m_Locals[(int)var-1].Usage &= ~offset;
}
bool grcEffect::GetVarUsageFlag(grcEffectVar var, u32 offset)
{
	return 0 != (m_Locals[(int)var-1].Usage & offset);
}
bool grcEffect::IsVar(grcEffectVar var) const
{
	return var > 0 && var <= m_Locals.GetCount();
}

void grcEffect::GetLocalCommon(const grcInstanceData &instanceData,grcEffectVar handle,void *dest,size_t destSize) const
{
	if (handle) {
		int index = (int)handle - 1;
		memcpy(dest, (char*)instanceData.Data()[index].Any, destSize);
	}
}


#if EFFECT_PRESERVE_STRINGS
void grcEffect::GetVarDesc(grcEffectVar var,const char *&name,VarType &type,int &annotationCount,bool &isGlobal,const char **actualName) const
{
	Assert(var);
	const grcParameter &parameter = m_Locals[(int)var-1];
	name = parameter.Semantic;
	type = (VarType) parameter.Type;
	isGlobal = false;
	annotationCount = parameter.AnnotationCount;
	if (actualName)
		*actualName = parameter.Name;
}
#endif


void grcEffect::GetVarDesc(grcEffectVar var,u32 &name,VarType &type,u32 *actualName) const
{
	Assert(var);
	const grcParameter &parameter = m_Locals[(int)var-1];
	name = parameter.SemanticHash;
	type = (VarType) parameter.Type;
	if (actualName)
		*actualName = parameter.NameHash;
}

void grcEffect::SetGlobalVar(grcEffectGlobalVar handle,const IntVector& value)
{
	if (handle) {
#if (__D3D11 || RSG_ORBIS)
		grcVertexProgram::SetParameterI(sm_Globals[handle-1].Register,value,4,sm_Globals[handle-1].CBufferOffset,sm_Globals[handle-1].CBuf TRACK_CONSTANT_BUFFER_CHANGES_ONLY(, &sm_Globals[handle-1]));
#else // (__D3D11 || RSG_ORBIS)
		Vec4V v((float)value[0], (float)value[1], (float)value[2], (float)value[3]);
		SetGlobalVar(handle,v);
#endif // (__D3D11 || RSG_ORBIS)
	}
}


void grcEffect::SetGlobalVar(grcEffectGlobalVar handle,bool value)
{
	if (handle) {
#if GRCORE_ON_SPU <= 1
#if RSG_PC || RSG_DURANGO || RSG_ORBIS
		grcVertexProgram::SetFlag(sm_Globals[handle-1].Register,value,sm_Globals[handle-1].CBufferOffset,sm_Globals[handle-1].CBuf TRACK_CONSTANT_BUFFER_CHANGES_ONLY(, &sm_Globals[handle-1]));
# else
		grcVertexProgram::SetFlag(sm_Globals[handle-1].Register,value);
# endif
#else
		SPU_SIMPLE_COMMAND(grcVertexProgram__SetFlag,sm_Globals[handle-1].Register | (value<<7));
#endif
	}
}


void grcEffect::SetGlobalVar(grcEffectGlobalVar handle, const int value)
{
	if (handle)
	{
		grcParameter &param = sm_Globals[handle-1];
#if GRCORE_ON_SPU <= 1
#if RSG_PC || RSG_DURANGO || RSG_ORBIS
		grcVertexProgram::SetFlag( param.Register, value, param.CBufferOffset, param.CBuf );
# else
		grcVertexProgram::SetFlag( param.Register, value );
# endif
#else
		SPU_SIMPLE_COMMAND( grcVertexProgram__SetFlag, param.Register | (value<<7) );
#endif
	}
}

void grcEffect::SetGlobalVar(grcEffectGlobalVar handle,const float *value,int count)
{
	float *temp = Alloca(float, count*4), *t = temp;
	for (int i=0; i<count; i++) {
		*t++ = *value++;
		*t++ = 0.0f;
		*t++ = 0.0f;
		*t++ = 0.0f;
	}
	SetGlobalFloatCommon((u32)handle, temp, count, VT_FLOAT);
}

void grcEffect::SetGlobalVar(grcEffectGlobalVar handle,const Vec2f *value,int count)
{
	const float *v = (float*)value;
	float *temp = Alloca(float, count*4), *t = temp;
	for (int i=0; i<count; i++) {
		*t++ = *v++;
		*t++ = *v++;
		*t++ = 0.0f;
		*t++ = 0.0f;
	}
	SetGlobalFloatCommon((u32)handle, temp, count, VT_VECTOR2);
}

void grcEffect::SetLocalCommon(grcInstanceData &instanceData,grcEffectVar handle,const void *value,int stride,int count,bool isFloat) const
{
	// Ignore null handles
	if (handle && count>0) {
		int index = (int)handle-1;
		Assert(!instanceData.IsTexture(index));
#if GRCORE_ON_SPU > 1
		if (GCM_CONTEXT) {
			// stride is always sizeof(value)
			int byteCount = stride * count;
			SPU_COMMAND(grcEffect__SetVarFloatCommon,index,byteCount);
			cmd->dest = instanceData.Data()[index].Float;
			Assert(((u32)cmd->dest & 3) == 0);
			Assign(cmd->count,count);
			Assign(cmd->stride,stride);
			cmd->effect = this;
			u32 *dest = cmd->alignedPayload;
			Assert(((u32)dest & 15) == 0 || byteCount < 16);	// must be aligned if 16 bytes or larger
			const u32 *src = (u32*) value;
			do {
				*dest++ = *src++;
				byteCount -= 4;
			} while (byteCount);
		}
		else
#endif	//GRCORE_ON_SPU
		{
			ASSERT_ONLY(const grcParameter &parameter = m_Locals[index]);
#if EFFECT_PRESERVE_STRINGS
			Assertf(g_Float4SizeByType[parameter.Type] == ((stride + 12)>>4),"In shader %s: Local var '%s:%s' shader expected type %s", m_EffectName, parameter.Name.c_str(),parameter.Semantic.c_str(),GetTypeName((VarType)parameter.Type));
			Assertf(count <= parameter.Count,"Supplied array count %d larger than variable %s count %d",count,parameter.Name.c_str(),parameter.Count);
#else
			Assertf(g_Float4SizeByType[parameter.Type] == ((stride + 12)>>4),"In shader %s: Local var hash_%x shader expected type %s", m_EffectName, parameter.NameHash,GetTypeName((VarType)parameter.Type));
			Assertf(count <= parameter.Count,"Supplied array count %d larger than variable hash_%x count %d",count,parameter.NameHash,parameter.Count);
#endif

#if CHECK_GPU_DATA
			if (isFloat)	{
				checkFloatArray(value, count, parameter.Type, m_EffectName, parameter.Name.c_str());
			}
#else
			(void)isFloat;
#endif	//CHECK_GPU_DATA

#if MULTIPLE_RENDER_THREADS > 1
			// Setting a variable from the update thread updates all copies
			if (!g_IsSubRenderThread && (instanceData.Flags & grcInstanceData::FLAG_EXTRA_DATA)) {
				for (int i=0; i<NUMBER_OF_RENDER_THREADS; i++) {
					const float *origDest = (float*) instanceData.DataForThread(i)[index].Any;
					u32 *dest = (u32*) origDest;
					const u32 *src = (u32*) value;
					int destSize = (stride + 12) & ~15;
					int destSkip = (destSize - stride) >> 2;
					int count2 = count;
					do {
						int stride2 = stride;
						do {
							*dest++ = *src++;
							stride2 -= 4;
						} while (stride2);
						dest += destSkip;
					} while (--count2);
				}
			}
			else
#endif
			{
#if EFFECT_CHECK_ENTRY_TYPES
				instanceData.Data()[index].Type = grcInstanceData::ET_FLOAT;
#endif
				const float *origDest = (float*) instanceData.Data()[index].Any;
				u32 *dest = (u32*) origDest;
				const u32 *src = (u32*) value;
				Assert(origDest != NULL);
				Assert(src != NULL);
				int destSize = (stride + 12) & ~15;
				Assert(destSize >= stride);
				Assert((stride & 3) == 0);
				int destSkip = (destSize - stride) >> 2;
				do {
					int stride2 = stride;
					do {
						*dest++ = *src++;
						stride2 -= 4;
					} while (stride2);
					dest += destSkip;
				} while (--count);
			}

			AssertMsg(!GRCDEVICE.CheckThreadOwnership() || !sm_CurrentPass,"I don't want to support setting variables while a pass is active any longer.");
		}
	}
}

void grcEffect::SetLocalCommonByRef(grcInstanceData &instanceData,grcEffectVar handle,const void *value,int stride,int count) const
{
	AssertMsg(((size_t)value & 15) == 0,"SetLocalCommonByRef - data must be 16-byte-aligned");
#if GRCORE_ON_SPU > 1
	// Ignore null handles
	if (handle && count>0) {
		int index = (int)handle-1;
		Assert(!instanceData.IsTexture(index));
		const grcParameter &parameter = m_Locals[index];
		if (GCM_CONTEXT) {
			SPU_COMMAND(grcEffect__SetVarFloatCommonByRef,index);
			cmd->dest = instanceData.Data()[index].Float;
			Assign(cmd->count,count);
			Assign(cmd->stride,stride);
			cmd->effect = this;
			cmd->src = const_cast<void*>(value);
			Assert(((u32)cmd->dest & 3) == 0);
		}
		else
			SetLocalCommon(instanceData, handle, value, stride, count);
	}
#else
	SetLocalCommon(instanceData, handle, value, stride, count);
#endif
}


#if (__D3D11 || RSG_ORBIS)
grcCBuffer *grcEffect::GetCBufferAndOffset(grcEffectVar handle, int &Offset)
{
	grcCBuffer *pRet = NULL;

	// Ignore null handles
	if(handle)
	{
		// Create an index from the handle.
		int index = (int)handle-1;
		// Obtain the index.
		Offset = m_Locals[index].GetCBufOffset();
		// Return the constant buffer.
		pRet = m_Locals[index].GetParentCBuf();
	}
	return pRet;
}
#endif //__D3D11

void grcEffect::SetVar(grcInstanceData &instanceData,grcEffectVar handle,const Color32 &value) const
{
	if (handle) {
#if GRCORE_ON_SPU > 1
		int index = (int)handle - 1;
		if (GCM_CONTEXT) {
			ASSERT_ONLY(instanceData.Flags |= grcInstanceData::FLAG_SET_FROM_SPU);		// remember that this was set from the SPU
			SPU_COMMAND(grcEffect__SetVar_Color32,1,sizeof(Color32));
			cmd->effect = this;
			cmd->dest = instanceData.Data()[index].Float;
			cmd->colors[0] = value.GetColor();	// Not device color, we'll do the work for free down on SPU
		}
		else
#endif
		{
			Vector4 temp(VEC4V_TO_VECTOR4(value.GetRGBA()));
			SetLocalCommon(instanceData,handle,&temp,16,1);
		}
	}
}

void grcEffect::SetVar(grcInstanceData &instanceData,grcEffectVar handle,const Color32 *values,int count) const
{
	if (handle && count) {
#if GRCORE_ON_SPU > 1
		int index = (int)handle - 1;
		if (GCM_CONTEXT) {
			ASSERT_ONLY(instanceData.Flags |= grcInstanceData::FLAG_SET_FROM_SPU);		// remember that this was set from the SPU
			SPU_COMMAND(grcEffect__SetVar_Color32,count,count*sizeof(Color32));
			cmd->effect = this;
			cmd->dest = instanceData.Data()[index].Float;
			for (int i=0; i<count; i++)
				cmd->colors[i] = values[i].GetColor();
		}
		else
#endif
		{
			Vector4 *temp = Alloca(Vector4, count);
			for (int i=0; i<count; i++)
				temp[i] = VEC4V_TO_VECTOR4(values[i].GetRGBA());
			SetLocalCommon(instanceData,handle,temp,16,count);
		}
	}
}


#if SUPPORT_UAV
void grcEffect::SetVar(grcInstanceData &instanceData,grcEffectVar handle,const grcBufferUAV *value)
{
	if (!handle)
	{
		return;
	}

	// TODO:- Make m_Locals[index].Data sub-render thread safe.

	const int index = static_cast<int>(handle) - 1;
	switch(m_Locals[index].Type)
	{
	case VT_STRUCTUREDBUFFER:
	case VT_BYTEADDRESSBUFFER:
#if EFFECT_CHECK_ENTRY_TYPES
		instanceData.Data()[index].Type = grcInstanceData::ET_BUFFER;
#endif
		/*m_Locals[index].RO_Buffer = */ instanceData.Data()[index].RO_Buffer = value;
		m_Locals[index].Data = NULL;
		break;
	default:
		Assertf( false, "Unsupported read-only buffer type: %d", m_Locals[index].Type );
	}
	
	AssertMsg(!GRCDEVICE.CheckThreadOwnership() || !sm_CurrentPass,"Changing a grcBufferUAV while an effect is active isn't supported yet.");
	
	if (GRCDEVICE.CheckThreadOwnership())
		sm_CurrentBind = NULL;
}

void grcEffect::SetVarUAV(grcInstanceData &instanceData, grcEffectVar handle, grcBufferUAV *value, int initCount)
{
	if (!handle)
	{
		return;
	}

	const int index = static_cast<int>(handle) - 1;
	switch(m_Locals[index].Type)
	{
	case VT_UAV_STRUCTURED:
	case VT_UAV_BYTEADDRESS:
#if EFFECT_CHECK_ENTRY_TYPES
		instanceData.Data()[index].Type = grcInstanceData::ET_BUFFER;
#endif
		/*m_Locals[index].RW_Buffer = */ instanceData.Data()[index].RW_Buffer = value;
		m_Locals[index].Data = reinterpret_cast<void*>( initCount );
		break;
	default:
		Assertf( false, "Unsupported UAV resource type: %d", m_Locals[index].Type );
	}
	
	AssertMsg(!GRCDEVICE.CheckThreadOwnership() || !sm_CurrentPass,"Changing a grcBufferUAV while an effect is active isn't supported yet.");

	if (GRCDEVICE.CheckThreadOwnership())
		sm_CurrentBind = NULL;
}

void grcEffect::SetVarUAV(grcInstanceData &instanceData, grcEffectVar handle, const grcTextureUAV *value)
{
	if (!handle)
	{
		return;
	}

	const int index = static_cast<int>(handle) - 1;
	Assert( m_Locals[index].Type == VT_UAV_TEXTURE );

#if EFFECT_CHECK_ENTRY_TYPES
	instanceData.Data()[index].Type = grcInstanceData::ET_TEXTURE;
#endif
	/*m_Locals[index].RW_Texture = */ instanceData.Data()[index].RW_Texture = value;
	m_Locals[index].Data = NULL;

	AssertMsg(!GRCDEVICE.CheckThreadOwnership() || !sm_CurrentPass,"Changing a grcTextureUAV while an effect is active isn't supported yet.");

	if (GRCDEVICE.CheckThreadOwnership())
		sm_CurrentBind = NULL;
}
#endif	//SUPPORT_UAV

void grcEffect::SetVar(grcInstanceData &instanceData,grcEffectVar handle,const grcTexture *value)
{
	if (handle) {
		
		const int index = (int)handle - 1;
		Assert( m_Locals[index].Type == VT_TEXTURE );

		//Note "value->GetName()" here to filter out textures created with CreateGivenDimensions
		if( value && value->GetResourceType() == grcTexture::RENDERTARGET && value->GetName() )
		{
			grcRenderTarget::LogTexture("SetVar",(grcRenderTarget*)value);
		}

#if HACK_GTA4 && __DEV
		// early detector for bugs similiar to Jimmy's BS#7081
		// allows to detect cases of free'd grcTexture memory (filled with 0xEE pattern in pgBase::FreeMemory())
		// passed here as a grcTexture to be processed by drawablespu and cause even more havoc later
		const grcTexture *pTexturePtr = value;
		if(pTexturePtr)
		{
			s32 refCount = pTexturePtr->GetRefCount() & 0xFFFF;
			if ((refCount < 1) || (refCount==0xEEEE) || (refCount==0xCDCD) || (refCount==0xDEDE))
			{
				Warningf("\ngrcEffect::SetVar: attempting to set free'd grcTexture (refCount=0x%X, texture=%p). For now I will try to (temporarily) fix this.", refCount, pTexturePtr);
				Assertf(false,"\ngrcEffect::SetVar: attempting to set free'd grcTexture (refCount=0x%X, texture=%p). For now I will try to (temporarily) fix this.", refCount, pTexturePtr);
				value = NULL;
			}
		}
#endif // HACK_GTA4...

#if GRCORE_ON_SPU > 1
		// If we're not the active thread, assume there's zero chance of conflict
		if (GCM_CONTEXT) {
			ASSERT_ONLY(instanceData.Flags |= grcInstanceData::FLAG_SET_FROM_SPU);		// remember that this was set from the SPU
			SPU_COMMAND(grcEffect__SetVar_grcTexture,index);
			cmd->instanceData = &instanceData;
#if USE_PACKED_GCMTEX
			cmd->textureHandle = grcTextureHandle::RegisterIndex(const_cast<grcTexture*>(value));
			Assert(!value || s_spuGcmState.PackedTextureArray[cmd->textureHandle].format);
#else
			// Constructor (and destructor) are not called for commands
			// allocated with SPU_COMMAND, so cmd->textureHandle.ptr will be
			// undefined.  This would cause problems when PGHANDLE_REF_COUNT is
			// enabled if we just used operator=, so instead we manually copy
			// the pointer and increment the ref count.
			cmd->textureHandle.ptr = (grcTexture**)pgHandleBase::Register(const_cast<grcTexture*>(value));
			cmd->textureHandle.IncRef();
#endif
			cmd->effect = this;
			cmd->data = (void*)(&instanceData.Data()[index].Texture);
#if TRACK_REFERENCE_BACKTRACE
			cmd->backtrace = sysStack::RegisterBacktrace();
#endif
		}
		else
#endif

#if MULTIPLE_RENDER_THREADS > 1
		if (!g_IsSubRenderThread && (instanceData.Flags & grcInstanceData::FLAG_EXTRA_DATA)) {
			for (int i=0; i<NUMBER_OF_RENDER_THREADS; i++)
			{
				instanceData.DataForThread(i)[index].Texture = const_cast<grcTexture*>(value);
# if EFFECT_CHECK_ENTRY_TYPES
				instanceData.DataForThread(i)[index].Type = grcInstanceData::ET_TEXTURE;
# endif
			}
		}
		else
#endif
		{
			if (instanceData.Data()[index].Texture != value) {
				PS3_ONLY(AssertMsg(!(instanceData.Flags & grcInstanceData::FLAG_SET_FROM_SPU),"Can't SetVar a texture from PPU once it's been set on SPU."));
				instanceData.Data()[index].Texture = const_cast<grcTexture*>(value);
			}
#if EFFECT_CHECK_ENTRY_TYPES
			instanceData.Data()[index].Type = grcInstanceData::ET_TEXTURE;
#endif
		}

#if __D3D11 && 0
		m_Locals[index].Tex = value;
#endif // __D3D11

		AssertMsg(!GRCDEVICE.CheckThreadOwnership() || !sm_CurrentPass,"Changing a texture while an effect is active isn't supported yet.");

		// This is somewhat dodgy because it can cause a SetVar in the update thread to affect the render thread.
		// Taking it out breaks immediate-mode rendering though.  It was first exposed as a problem when I introduced
		// a subtle problem in grcEffect::Bind's GRCORE_ON_SPU>1 path where sm_CurrentBind was written earlier in
		// the function and then accessed again later after it had possibly already been zeroed out here.
		// We now only clear the bind if we know we're in the render thread, which should improve things.
		if (GRCDEVICE.CheckThreadOwnership())
			sm_CurrentBind = NULL;
	}
}

void grcEffect::GetVar(const grcInstanceData &instanceData,grcEffectVar handle,grcTexture *&value) const
{
	if (handle)
	{
		const int index = (int)handle - 1;

		value = (instanceData.Data()[index].Texture?instanceData.Data()[index].Texture->GetReference():NULL);
	}
}

// grcEffectAnnotation grcEffect::LookupAnnotation(grcEffectVar var,const char *name,bool mustExist = true) const;

const grcParameter::Annotation *grcEffect::GetAnnotation(grcEffectVar var,const char *name) const
{
	return GetAnnotation(var, atStringHash(name));
}

const grcParameter::Annotation *grcEffect::GetAnnotation(grcEffectVar var,u32 nameHash) const
{
	if (var) {
		const grcParameter &parameter = m_Locals[(int)var-1];
		for (int i=0; i<parameter.AnnotationCount; i++)
			if (nameHash == parameter.Annotations[i].NameHash)
				return &parameter.Annotations[i];
	}
	return NULL;
}

bool grcEffect::HasAnnotation(grcEffectVar var,const char *name) const
{
	return GetAnnotation(var,name) != NULL;
}

bool grcEffect::HasAnnotation(grcEffectVar var,u32 hashcode) const
{
	return GetAnnotation(var,hashcode) != NULL;
}

const char *grcEffect::GetAnnotationData(grcEffectVar var,const char *name,const char *defaultValue) const
{
	const grcParameter::Annotation *anno = GetAnnotation(var, name);
	if (anno && anno->Type == grcParameter::Annotation::AT_STRING)
		return anno->String;
	else
		return defaultValue;
}

const char *grcEffect::GetAnnotationData(grcEffectVar var,u32 hashcode,const char *defaultValue) const
{
	const grcParameter::Annotation *anno = GetAnnotation(var, hashcode);
	if (anno && anno->Type == grcParameter::Annotation::AT_STRING)
		return anno->String;
	else
		return defaultValue;
}

float grcEffect::GetAnnotationData(grcEffectVar var,const char *name,float defaultValue) const
{
	const grcParameter::Annotation *anno = GetAnnotation(var, name);
	if (anno && anno->Type == grcParameter::Annotation::AT_FLOAT)
		return anno->Float;
	else if (anno && anno->Type == grcParameter::Annotation::AT_INT)
		return (float) anno->Int;
	else
		return defaultValue;
}

int grcEffect::GetAnnotationData(grcEffectVar var,const char *name,int defaultValue) const
{
	const grcParameter::Annotation *anno = GetAnnotation(var, name);
	if (anno && anno->Type == grcParameter::Annotation::AT_FLOAT)
		return (int) anno->Float;
	else if (anno && anno->Type == grcParameter::Annotation::AT_INT)
		return anno->Int;
	else
		return defaultValue;
}

#if EFFECT_PRESERVE_STRINGS
void grcEffect::GetGlobalVarDesc(int idx,const char *&name,VarType &type)
{
	name = sm_Globals[idx].Name;
	type = (VarType) sm_Globals[idx].Type;
}
#endif

const grcParameter::Annotation *grcEffect::GetGlobalAnnotation(int idx,const char *name)
{
	const grcParameter &parameter = sm_Globals[idx];
	u32 nameHash = atStringHash(name);
	for (int i=0; i<parameter.AnnotationCount; i++)
		if (nameHash == parameter.Annotations[i].NameHash)
			return &parameter.Annotations[i];
	return NULL;
}

const char *grcEffect::GetGlobalVarAnnotationData(int idx,const char *name,const char *defaultValue)
{
	const grcParameter::Annotation *anno = GetGlobalAnnotation(idx, name);
	if (anno && anno->Type == grcParameter::Annotation::AT_STRING)
		return anno->String;
	else
		return defaultValue;
}

float grcEffect::GetGlobalVarAnnotationData(int idx,const char *name,float defaultValue)
{
	const grcParameter::Annotation *anno = GetGlobalAnnotation(idx, name);
	if (anno && anno->Type == grcParameter::Annotation::AT_FLOAT)
		return anno->Float;
	else if (anno && anno->Type == grcParameter::Annotation::AT_INT)
		return (float) anno->Int;
	else
		return defaultValue;
}

int grcEffect::GetGlobalVarAnnotationData(int idx,const char *name,int defaultValue)
{
	const grcParameter::Annotation *anno = GetGlobalAnnotation(idx, name);
	if (anno && anno->Type == grcParameter::Annotation::AT_FLOAT)
		return (int) anno->Float;
	else if (anno && anno->Type == grcParameter::Annotation::AT_INT)
		return anno->Int;
	else
		return defaultValue;
}

int grcEffect::GetPropertyValue(const char *propName,int defaultValue) const
{
	u32 nameHash = atStringHash(propName);
	for (int i=0; i<m_Properties.GetCount(); i++)
		if (nameHash == m_Properties[i].NameHash && m_Properties[i].Type == grcParameter::Annotation::AT_INT)
			return m_Properties[i].Int;
	return defaultValue;
}

float grcEffect::GetPropertyValue(const char *propName,float defaultValue) const
{
	u32 nameHash = atStringHash(propName);
	for (int i=0; i<m_Properties.GetCount(); i++)
		if (nameHash == m_Properties[i].NameHash && m_Properties[i].Type == grcParameter::Annotation::AT_FLOAT)
			return m_Properties[i].Float;
	return defaultValue;
}

const char *grcEffect::GetPropertyValue(const char *propName,const char *defaultValue) const
{
	u32 nameHash = atStringHash(propName);
	for (int i=0; i<m_Properties.GetCount(); i++)
		if (nameHash == m_Properties[i].NameHash && m_Properties[i].Type == grcParameter::Annotation::AT_STRING)
			return m_Properties[i].String;
	return defaultValue;
}

void grcEffect::ResetAllGlobalsToDefaults()
{
	for (int i=0; i<sm_Globals.GetCount(); i++)
	{
		grcParameter &parameter = sm_Globals[i];
		if (parameter.Type != VT_TEXTURE && parameter.Data)
			SetGlobalFloatCommon(i+1, (/*const*/ float *) parameter.Data, parameter.Count, (VarType)parameter.Type);
	}
}

void grcEffect::ResetGlobalTextures()
{
	const grcTexture *const value = grcTexture::None;
	
	for (int i=0; i<sm_Globals.GetCount(); i++)
	{
		grcParameter &parameter = sm_Globals[i];
		if (parameter.Type == VT_TEXTURE)
		{
			SetGlobalVar(static_cast<grcEffectGlobalVar>(i+1), value);
		}
	}
}

char grcEffect::sm_DefaultPath[128] = "$/tune/shaders/lib";

void grcEffect::SetDefaultPath(const char *path) {
	safecpy(sm_DefaultPath,path,sizeof(sm_DefaultPath));
}

const char *grcEffect::GetTypeName(grcEffect::VarType vt) {
	static const char *names[] = { "NONE", "int", "float", "Vector2", "Vector3", "Vector4", "grcTexture", "bool", "Matrix34", "Matrix44", "string",
#if RSG_PC || RSG_DURANGO
		"int", "int2", "int3", "int4",
#endif
	};
	return names[vt];
}

grcEffect::VarType grcEffect::GetType(const char *type) {
	for (int vt=0; vt<VT_COUNT; vt++)
	{
		if (stricmp(GetTypeName((grcEffect::VarType)vt), type)==0)
		{
			return (grcEffect::VarType)vt;
		}
	}
	return VT_NONE;
}

u64 grcEffect::GetCurrentTimeStamp() const {
	return fiDevice::GetDevice(m_EffectPath)->GetFileTime(m_EffectPath);
}

void grcEffect::SetGlobalVar(grcEffectGlobalVar handle,const grcTexture *value)
{
	if (g_grcCommandBuffer)
		return;
	if (handle) {
#if GRCORE_ON_SPU > 1
		// TODO: Establish if it's even worth checking first?
		const grcParameter &global = sm_Globals[handle-1];
		if (global.Usage) {
			SPU_COMMAND(grcEffect__SetGlobalVar_grcTexture,0);
			cmd->Register = global.Register;
			cmd->Usage = global.Usage;
			cmd->SamplerStateSet = global.SamplerStateSet;		
#if USE_PACKED_GCMTEX
			cmd->textureHandle = grcTextureHandle::RegisterIndex(const_cast<grcTexture*>(value));
			Assert(!value || s_spuGcmState.PackedTextureArray[cmd->textureHandle].format);
#else
			cmd->texture = (void*) value;
#endif
		}

#else // GRCORE_ON_SPU
		u32 index = (u32) handle - 1;
		const grcParameter &global = sm_Globals[index];

# if __D3D11 || (RSG_ORBIS && ENABLE_LCUE) // PC DX11 + Durango.
		grcProgram::SetGlobalTexture(global.Register,(void *)value,global.SamplerStateSet);
# elif (RSG_PC && __D3D9) // PC DX9.
		if (global.Usage & USAGE_VERTEXPROGRAM)
			grcVertexProgram::SetTexture(global.Register,value,global.SamplerStateSet);
		if (global.Usage & USAGE_FRAGMENTPROGRAM)
			grcFragmentProgram::SetTexture(global.Register,value,global.SamplerStateSet);
# elif RSG_ORBIS && !ENABLE_LCUE
		if (global.Usage & USAGE_VERTEXPROGRAM)
		{
			grcVertexProgram::SetGlobalTexture(global.Register,value->GetCachedTexturePtr(),global.SamplerStateSet);
		}
		if (global.Usage & USAGE_FRAGMENTPROGRAM)
			grcFragmentProgram::SetTexture(global.Register,value->GetCachedTexturePtr(),global.SamplerStateSet);
# else // PS3, XENON
		if (global.Usage & USAGE_VERTEXPROGRAM)
			grcVertexProgram::SetTexture(global.Register,value->GetCachedTexturePtr(),global.SamplerStateSet);
		if (global.Usage & USAGE_FRAGMENTPROGRAM)
			grcFragmentProgram::SetTexture(global.Register,value->GetCachedTexturePtr(),global.SamplerStateSet);
#endif // PS3, XENON

#endif // GRCORE_ON_SPU
	}
}


template<class T>
void grcEffect::SetGlobalInternal(grcParameter &global, const float *value, int count)
{
#if RSG_PC || RSG_DURANGO || RSG_ORBIS
	if (global.Type == grcEffect::VT_BOOL_DONTUSE)
		T::SetFlag(global.Register, *value != 0, global.CBufferOffset, global.CBuf);
	else
		T::SetParameterW(global.Register, value, count, global.CBufferOffset, global.CBuf TRACK_CONSTANT_BUFFER_CHANGES_ONLY(, &global));
#else
		T::SetParameter(global.Register, value, count);
#endif //RSG_PC || RSG_DURANGO || RSG_ORBIS
}

void grcEffect::SetGlobalFloatCommon(int ci,/*const*/ float *value,int count,grcEffect::VarType WIN32_ONLY(DEV_ONLY(type)))
{
	if (g_grcCommandBuffer || !ci)
		return;
	grcParameter &global = sm_Globals[ci-1];

#if CHECK_GPU_DATA
	if (global.Type < VT_INT)
		checkFloatArray(value, count, global.Type,"unknown","unknown");
#endif

#if GRCORE_ON_SPU > 1
	if (global.Usage) {
		int qwCount = (count * g_Float4SizeByType[global.Type]);
		int byteCount = qwCount << 4;
		SPU_COMMAND(grcEffect__SetGlobalFloatCommon,0,byteCount);
		cmd->Register = global.Register;
		cmd->Usage = global.Usage;
		Assign(cmd->qwCount,qwCount);
		if ((u32)value & 15) {
			u32 *src = (u32*) value;
			u32 *dest = (u32*) cmd->alignedPayload;
			do {
				*dest++ = *src++;
				byteCount-=4;
			} while (byteCount>0);
		}
		else {
			u128 *src = (u128*) value;
			u128 *dest = (u128*) cmd->alignedPayload;
			do {
				*dest++ = *src++;
				byteCount-=16;
			} while (byteCount>0);
		}
	}
#else	// !(GRCORE_ON_SPU > 1)
#if RSG_PC || RSG_DURANGO || RSG_ORBIS
	if (count == 1) 
		count *= g_FloatSizeByType[global.Type];
	else
		count *= g_Float4SizeByType[global.Type] * 4;
#else
	count *= g_Float4SizeByType[global.Type];
#endif
	// By design, a null handle (grcegvNONE) will fail both tests below and will be silently ignored.
#if __D3D11 || RSG_ORBIS
	if (global.Usage & USAGE_ANYPROGRAM) {
		SetGlobalInternal<grcProgram>(global, value, count);
	}
#else
	if (global.Usage & USAGE_VERTEXPROGRAM) {
		SetGlobalInternal<grcVertexProgram>(global, value, count);
	}
	if (global.Usage & USAGE_FRAGMENTPROGRAM) {
		SetGlobalInternal<grcFragmentProgram>(global, value, count);
	}
#endif //__D3D11 || RSG_ORBIS

#if __DEV && __WIN32
	if ((global.Usage) && global.Type != type)
		grcErrorf("Global var %s (register c%d) shader had actual type %s, code expected type %s", global.Name.c_str(), global.Register, GetTypeName((grcEffect::VarType)global.Type),GetTypeName(type));
#endif
#endif	// !(GRCORE_ON_SPU > 1)
}


#if __PPU

inline void grcVertexProgram::RecordBind(const grcInstanceData &data) const
{ 
	// Vertex programs don't support vertex textures.
	int count = m_Constants.GetCount();
	for (int ii=0; ii<count; ii++) 
	{
		int localIndex = m_Constants[ii]; 
		grcInstanceData::Entry &e = data.Data()[localIndex];
		Assertf(e.Count,"grcVertexProgram::RecordBind - vertex textures not supported.");
		if (e.Count)
			RecordSetParameter(e.Register,e.Float,e.Count);
	}
	RecordBind();
}

inline void grcFragmentProgram::RecordBind(const grcInstanceData &data) const
{ 
	// Fragment programs don't support float parameters (since that requires patching)
	int count = m_Constants.GetCount();
	for (int ii=0; ii<count; ii++) 
	{
		int localIndex = m_Constants[ii]; 
		grcInstanceData::Entry &e = data.Data()[localIndex];
		Assertf(!e.Count,"grcFragmentProgram::RecordBind - float parameters not supported.");
		if (!e.Count)
			RecordSetTexture(e.Register,e.Texture?&e.Texture->GetGcmTexture():NULL,e.SamplerStateSet);
	}
	RecordBind();
}

void grcEffect::RecordBeginPass(int passIndex,const grcInstanceData &instanceData) const
{
	ASSERT_ONLY(sm_CurrentEffect = this);
	FastAssert(!sm_CurrentPass);
	sm_CurrentPass = &sm_CurrentTechnique->Passes[sm_CurrentPassIndex = passIndex];
	sm_CurrentPass->RecordBind(m_VertexPrograms[sm_CurrentPass->VertexProgramIndex],
		m_FragmentPrograms[sm_CurrentPass->FragmentProgramIndex], instanceData);
}

void grcEffect::RecordEndPass() const
{
	// no operation since we don't support state changes or fat textures
	sm_CurrentPass = NULL;
}

void grcEffect::Technique::Pass::RecordBind(const grcVertexProgram& vp,const grcFragmentProgram& fp, const grcInstanceData &instanceData) const
{
	vp.RecordBind(instanceData);
	fp.RecordBind(instanceData);
}
#endif // __PPU...

void grcEffect::RecordSetVar(grcInstanceData &instanceData,grcEffectVar handle,const grcTexture *value)
{
	if (handle) {
		int index = (int)handle - 1;
		if (instanceData.Data()[index].Texture != value) {
			PS3_ONLY(AssertMsg(!(instanceData.Flags & grcInstanceData::FLAG_SET_FROM_SPU),"Can't SetVar a texture from PPU once it's been set on SPU."));
			instanceData.Data()[index].Texture = const_cast<grcTexture*>(value);
		}
	}
}

void grcEffect::RecordSetVar(grcInstanceData &instanceData,grcEffectVar handle,float value) const
{
	if (handle) {
		int index = (int)handle-1;
		Assert(!instanceData.IsTexture(index));
		*(float*)(instanceData.Data()[index].Any) = value;
	}
}

void grcEffect::RecordSetVar(grcInstanceData &instanceData,grcEffectVar handle,const Vector2 &value) const
{
	if (handle) {
		int index = (int)handle-1;
		Assert(!instanceData.IsTexture(index));
		*(Vector2*)(instanceData.Data()[index].Any) = value;
	}
}

void grcEffect::RecordSetVar(grcInstanceData &instanceData,grcEffectVar handle,const Vector3 &value) const
{
	if (handle) {
		int index = (int)handle-1;
		Assert(!instanceData.IsTexture(index));
		*(Vector3*)(instanceData.Data()[index].Any) = value;
	}
}

void grcEffect::RecordSetVar(grcInstanceData &instanceData,grcEffectVar handle,const Vector4 &value) const
{
	if (handle) {
		int index = (int)handle-1;
		Assert(!instanceData.IsTexture(index));
		*(Vector4*)(instanceData.Data()[index].Any) = value;
	}
}


#if ENABLE_DEFRAGMENTATION
void grcTextureHandleUnionable::PointerFixup(datResource &NOTTOOL_ONLY(rsc))
{
#if !__TOOL
	if (ptr) {
		grcTexture *newPtr = (grcTexture*)((char*)ptr + rsc.GetFixup((char*)ptr));
		if (newPtr->GetResourceType() == grcTexture::REFERENCE) {
			grcTextureFactory::GetInstance().PlaceTexture(rsc, *newPtr);
			// No longer need to keep the reference around (and with the pgBase memory changes
			// we no longer have to worry about leaks).  Fix the refcount on the final texture.
			grcTexture *ref = newPtr;
			ptr = (grcTexture**)Register(ref->GetReference());
			ref->Release();
		}
		else
			ptr = (grcTexture**)Register(newPtr);

		if (!datResource_IsDefragmentation)
			IncRef();
	}
#endif
}
#else
void FixupTexture(grcTexture *&ptr,datResource &rsc)
{
	if (ptr) {
		rsc.PointerFixup(ptr);
		if (ptr->GetResourceType() == grcTexture::REFERENCE) {
			grcTextureFactory::GetInstance().PlaceTexture(rsc,*ptr);
			ptr = ptr->GetReference();
			if (ptr)
				ptr->Release();
		}
		// otherwise if it's not a reference, it's already been placed so don't do it again.
	}
}
#endif

#if USE_PACKED_GCMTEX
void grcTextureIndex32::PointerFixup(datResource &rsc)
{
	if (index) {
		grcTexture *newPtr = (grcTexture*)(index + rsc.GetFixup((char*)index));
		if (newPtr->GetResourceType() == grcTexture::REFERENCE) {
			grcTextureFactory::GetInstance().PlaceTexture(rsc, *newPtr);
			// No longer need to keep the reference around (and with the pgBase memory changes
			// we no longer have to worry about leaks).  Fix the refcount on the final texture.
			grcTexture *ref = newPtr;
			index = pgHandleBase::RegisterIndex(ref->GetReference());
			ref->Release();
		}
		else
			index = pgHandleBase::RegisterIndex(newPtr);
	}
}
#endif

grcSamplerStateHandle grcEffect::GetSamplerState(const grcInstanceData &data,grcEffectVar var) const
{
	AssertMsg(data.Data()[var-1].Count==0,"Variable is not a texture");
	return (grcSamplerStateHandle) data.Data()[var-1].SamplerStateSet;
}

void grcEffect::SetSamplerState(grcInstanceData &data,grcEffectVar var,grcSamplerStateHandle h)
{
	AssertMsg(data.Data()[var-1].Count==0,"Variable is not a texture");
#if __PPU
	if (GCM_CONTEXT) {
		SPU_COMMAND(grcEffect__SetSamplerState,var-1);
		cmd->instanceData = &data;
		cmd->handle = h;
		cmd->effect = this;
	}
	else
#endif
		Assign(data.Data()[var-1].SamplerStateSet,h);
}

void grcEffect::PushSamplerState(grcInstanceData &data,grcEffectVar var,grcSamplerStateHandle h)
{
	AssertMsg(data.Data()[var-1].Count==0,"Variable is not a texture");
#if __PPU
	AssertMsg(GCM_CONTEXT,"PushSamplerState must be called from render thread!");
	SPU_COMMAND(grcEffect__PushSamplerState,var-1);
	cmd->instanceData = &data;
	cmd->handle = h;
	cmd->effect = this;
#else
	AssertMsg(data.Data()[var-1].SavedSamplerStateSet == INVALID_STATEBLOCK,"Sampler state already pushed, cannot push again without popping");
	data.Data()[var-1].SavedSamplerStateSet = data.Data()[var-1].SamplerStateSet;
	Assign(data.Data()[var-1].SamplerStateSet,h);
#endif
}

void grcEffect::PopSamplerState(grcInstanceData &data,grcEffectVar var)
{
	AssertMsg(data.Data()[var-1].Count==0,"Variable is not a texture");
#if __PPU
	AssertMsg(GCM_CONTEXT,"PopSamplerState must be called from render thread!");
	ASSERT_ONLY(data.Flags |= grcInstanceData::FLAG_SET_FROM_SPU);		// remember that this was set from the SPU
	SPU_COMMAND(grcEffect__PopSamplerState,var-1);
	cmd->instanceData = &data;
	cmd->effect = this;
#else
	AssertMsg(data.Data()[var-1].SavedSamplerStateSet != INVALID_STATEBLOCK,"Sampler state not pushed, cannot pop");
	data.Data()[var-1].SamplerStateSet = data.Data()[var-1].SavedSamplerStateSet;
	ASSERT_ONLY(data.Data()[var-1].SavedSamplerStateSet = INVALID_STATEBLOCK);
#endif
}


// Note: The global ones don't need to be (and in fact cannot be) pipelined on PS3 because the sampler state set is only ever read by the PPU.
grcSamplerStateHandle grcEffect::GetGlobalSamplerState(grcEffectGlobalVar var) const
{
	AssertMsg(sm_Globals[var-1].Count==0,"Variable is not a texture");
	return (grcSamplerStateHandle) sm_Globals[var-1].SamplerStateSet;
}

void grcEffect::SetGlobalSamplerState(grcEffectGlobalVar var,grcSamplerStateHandle h)
{
	AssertMsg(sm_Globals[var-1].Count==0,"Variable is not a texture");
	Assign(sm_Globals[var-1].SamplerStateSet,h);
}

void grcEffect::PushGlobalSamplerState(grcEffectGlobalVar var,grcSamplerStateHandle h)
{
	AssertMsg(sm_Globals[var-1].Count==0,"Variable is not a texture");
	AssertMsg(sm_Globals[var-1].SavedSamplerStateSet == INVALID_STATEBLOCK,"Sampler state already pushed, cannot push again without popping");
	sm_Globals[var-1].SavedSamplerStateSet = sm_Globals[var-1].SamplerStateSet;
	Assign(sm_Globals[var-1].SamplerStateSet,h);
}

void grcEffect::PopGlobalSamplerState(grcEffectGlobalVar var)
{
	AssertMsg(sm_Globals[var-1].Count==0,"Variable is not a texture");
	AssertMsg(sm_Globals[var-1].SavedSamplerStateSet != INVALID_STATEBLOCK,"Sampler state not pushed, cannot pop");
	sm_Globals[var-1].SamplerStateSet = sm_Globals[var-1].SavedSamplerStateSet;
	ASSERT_ONLY(sm_Globals[var-1].SavedSamplerStateSet = INVALID_STATEBLOCK);
}

#if EFFECT_STENCIL_REF_MASK
static __THREAD u8 s_StencilRefMask = 0xFF;
u8 grcEffect::GetStencilRefMask() { return s_StencilRefMask; }
void grcEffect::SetStencilRefMask(u8 mask) { s_StencilRefMask = mask; }
#endif //EFFECT_STENCIL_REF_MASK

}	// namespace rage
