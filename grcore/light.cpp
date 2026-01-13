//
// grcore/light.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "light.h"
#include "effect.h"
#include "effect_config.h"
#include "channel.h"

#include "bank/bank.h"
#include "string/string.h"

using namespace rage;

DECLARE_MTR_THREAD bool grcLightState::sm_Lighting;
#if __DEV || __BANK
bool grcLightState::sm_ToolMode;
#endif

bool grcLightState::SetEnabled(bool value) {
	bool prev = sm_Lighting;
	sm_Lighting = value;
#if GRCORE_ON_SPU
	SPU_SIMPLE_COMMAND(grcState__SetLightingMode,value);
#endif
	return prev;
}

grcLightGroup grcLightState::sm_LightGroup;


struct ShaderLightGroup {
	Vec4V lightType;
	Vec4V lightAmbient;
	Vec4V lightDir[grcLightGroup::MAX_LIGHTS];
	Vec4V lightPos[grcLightGroup::MAX_LIGHTS];
	Vec4V lightColor[grcLightGroup::MAX_LIGHTS];
};

#define LIGHTS_IN_CONSTANT_BUFFER RSG_SM_50

#if !LIGHTS_IN_CONSTANT_BUFFER
static grcEffectGlobalVar
	s_gvLightColor,
	s_gvLightPos,
	s_gvLightDir,
	s_gvLightType,
	s_gvLightAmbient;
#endif

void grcLightState::SetLightingGroup(const grcLightGroup &grp) {
	grp.Validate();
	sm_LightGroup = grp;

#if LIGHTS_IN_CONSTANT_BUFFER
	static grcCBuffer *s_RageLighting;
	if (!s_RageLighting)
	{
		s_RageLighting = grcEffect::LookupGlobalConstantBufferByHash(ATSTRINGHASH("rage_lighting",0xda399846));
		Assertf(s_RageLighting != NULL, "Failed to find rage_lighting global constant buffer");
	}
	static ShaderLightGroup oDummy;
	ShaderLightGroup &p = (s_RageLighting != NULL) ? *s_RageLighting->BeginTypedUpdate<ShaderLightGroup>() : oDummy;
#else
	ShaderLightGroup p;
#endif

	Vec3V vect;
	Vec3V direction;
	int type;
	float falloff;
	int i;
	for (i = 0; i < grp.GetActiveCount(); ++i) {
		vect = Vec3V(V_ZERO);
		direction = Vec3V(V_ZERO);
		type = grcLightGroup::LTTYPE_DIR;
		falloff = grp.GetFalloff(i);
		if (grp.GetLightType(i) == grcLightGroup::LTTYPE_POINT) {
			vect = grp.GetPosition(i);
			type = grcLightGroup::LTTYPE_POINT;
			p.lightDir[i] = Vec4V(V_ZERO);
		}
		else if (grp.GetLightType(i) == grcLightGroup::LTTYPE_SPOT) {
			vect = grp.GetPosition(i);
			type = grcLightGroup::LTTYPE_SPOT;
			p.lightDir[i] = Vec4V(-grp.GetDirection(i),ScalarVFromF32(grp.GetConeAngle(i)));
		}
		else {
			vect = grp.GetDirection(i);
			p.lightDir[i] = Vec4V(V_ZERO);
		}
		p.lightPos[i] = Vec4V(vect,ScalarVFromF32(falloff));
		p.lightColor[i] = Vec4V(grp.GetColor(i),ScalarVFromF32(grp.GetIntensity(i)));
		p.lightType[i] = (float) type;
	}
	int count = grp.GetActiveCount();

#if !__XENON
	// Some PC cards (ahem nVidia at least) can't properly handle for loops with constant registers
	for (; i < grcLightGroup::DEFAULT_LIGHT_COUNT; ++i) {
		p.lightColor[i] = Vec4V(V_ZERO);
		p.lightPos[i] = Vec4V(V_ZERO_WONE);
		p.lightType[i] = grcLightGroup::LTTYPE_DIR;
		++count;
	}
#endif // !__XENON

#if !LIGHTS_IN_CONSTANT_BUFFER
	if (!s_gvLightColor) {
		s_gvLightColor		= grcEffect::LookupGlobalVar("Diffuse",false);
		s_gvLightPos		= grcEffect::LookupGlobalVar("Position",false);
		s_gvLightDir		= grcEffect::LookupGlobalVar("Direction",false);
		s_gvLightType		= grcEffect::LookupGlobalVar("LightType",false);
		s_gvLightAmbient	= grcEffect::LookupGlobalVar("Ambient",false);
	}

	grcEffect::SetGlobalVar(s_gvLightColor,p.lightColor,count);
	grcEffect::SetGlobalVar(s_gvLightPos,p.lightPos,count);
	grcEffect::SetGlobalVar(s_gvLightDir,p.lightDir,count);
	grcEffect::SetGlobalVar(s_gvLightType,p.lightType);
	grcEffect::SetGlobalVar(s_gvLightAmbient,grp.GetAmbient());
#else
	p.lightAmbient = grp.GetAmbient();
	if (s_RageLighting)
		s_RageLighting->EndUpdate();
#endif
}



grcLightGroup::grcLightGroup(int maxAllowed) : m_ActiveCount((s16) maxAllowed), m_MaxAllowed((s16) maxAllowed) {
	AssertMsg((s16) maxAllowed <= MAX_LIGHTS,"That many light sources not currently supported");

	// Init to some default
    for (int i = 0; i < MAX_LIGHTS; ++i)
    {
        Reset( i );
    }
}

void grcLightGroup::Reset(int id) {
	const int start = id < 0 ? 0 : id;
	const int end = id < 0 ? m_ActiveCount : id+1;
	Vec3V zero(0.f, 0.f, 0.f);
	Vec3V down(0.f, -1.f, 0.f);
	for (int i = start; i < end; ++i) 
    {
        SetLightType(i, grcLightGroup::LTTYPE_POINT);
		SetDirection(i, down);
		SetPosition(i, zero);
		SetColor(i, zero);
		SetIntensity(i, 1.0f);
		SetFalloff(i, 100.f);
		SetConeAngle(i, 0.7f);
		SetLightEnabled(i, true);
	}
	m_Ambient = Vec4V(0.25f,0.25f,0.25f,1.0f);
#if __BANK
	m_DontUseAmbient = 0x0;
#endif
}

void grcLightGroup::InitDirectionalFromPoint() {
	for (int i = 0; i < m_ActiveCount; ++i) {
		m_LightSources[i].m_Dir = -Normalize(m_LightSources[i].m_Pos);
	}
}

#if __ASSERT
bool grcLightGroup::Validate() const {
	bool result = true;

	for (int i=0; i<m_ActiveCount; i++) {
		if (IsTrue(Abs(MagSquared(GetDirection(i)) - ScalarV(V_ONE)) > ScalarV(V_FLT_SMALL_2))) {
			grcErrorf("Light %d direction (%f,%f,%f) not normalized",i,VEC3V_ARGS(GetDirection(i)));
			result = false;
		}
	}

	return result;
}
#endif

#if __BANK
void grcLightGroup::AddWidgets(bkBank &B) {
	char groupName[64];
	static const char *typelist[] = {"Dir", "Point", "Spot"};
	for (int i = 0; i < m_MaxAllowed; ++i) {
		formatf(groupName, 64, "Light %d", i);
		B.PushGroup(groupName,false);
			B.AddToggle("Enabled", &m_LightSources[i].m_Enabled, 0x01);
			B.AddCombo("Type", &m_LightSources[i].m_Type, LTTYPE_COUNT, typelist);
			B.AddVector("Direction",&m_LightSources[i].m_Dir,-1,+1,0.01f);
			B.AddVector("Position",&m_LightSources[i].m_Pos,-1000,+1000,0.1f);
			B.AddColor("Color",&m_LightSources[i].m_Color);
			B.AddSlider("Intensity",&m_LightSources[i].m_Intensity,0,1000,0.1f);
			B.AddSlider("Falloff",&m_LightSources[i].m_Falloff,0,100000,0.1f);
            B.AddAngle( "Cone Angle", &m_LightSources[i].m_ConeAngle, bkAngleType::RADIANS, 0.0f , PI );
		B.PopGroup();
	}
	B.PushGroup("Ambient",false);
		B.AddToggle("Don't use ambient", &m_DontUseAmbient, 0x01);
		B.AddColor("Color",&m_Ambient);
	B.PopGroup();
	B.AddSlider("Active Count", &m_ActiveCount, 1, m_MaxAllowed, 1);
}

void grcLightState::AddWidgets(bkBank &B) {
	B.AddToggle("Enable Tool Techniques",&sm_ToolMode);
}
#endif
