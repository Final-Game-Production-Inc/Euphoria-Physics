//
// grcore/light.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef GRCORE_LIGHT_H
#define GRCORE_LIGHT_H

#include "atl/array.h"
#include "atl/bitset.h"
#include "vectormath/vec3v.h"
#include "vectormath/vec4v.h"
#include "vector/color32.h"

namespace rage {

/*
	A grcLightGroup is a collection of lighting information in a platform-independent format.

	This object is sent down to RAGE to be passed into the GPU.  Higher level code is responsible
	for converting its own set of light objects into an "active set" to be used when rendering
	geometry.
*/
class grcLightGroup {
public:
	// RAGE defaults to supporting 3 light sources
	grcLightGroup(int maxAllowed = DEFAULT_LIGHT_COUNT);

	// Types of lights allowed
	enum grcLightType { LTTYPE_DIR, LTTYPE_POINT, LTTYPE_SPOT, LTTYPE_COUNT };
	// Default count of lights supported by built-int RAGE rendering
	static const int DEFAULT_LIGHT_COUNT = 4;
	// Maximum # of lights allowed by grcore -- higher level code is responsible for
	//	rendering with any lights above DEFAULT_LIGHT_COUNT
	static const int MAX_LIGHTS = 4;

	// PURPOSE: Retrieve the active light count
	inline s32 GetActiveCount() const;
	// PURPOSE: Set the active count (first count are always active)
	inline void SetActiveCount(s32 count);

    // PURPOSE: Retrieve the max number of lights allowed
    inline s32 GetMaxAllowed() const;

	// PURPOSE: Retrieve the direction vector
	inline Vec3V_Out GetDirection(int id) const;
	// PURPOSE: Set the direction vector
	inline void SetDirection(int id, Vec3V_In dir);

	// PURPOSE: Retrieve the position of the light source (not valid for directional lights)
	inline Vec3V_Out GetPosition(int id) const;
	// PURPOSE: Set the position of the light source
	inline void SetPosition(int id, Vec3V_In pos);

	// PURPOSE: Retrieve the light source's color
	inline Vec3V_Out GetColor(int id) const;
	// PURPSE: Set the light source's color
	inline void SetColor(int id, Vec3V_In c);
	// PURPOSE: Set the light source's color
	inline void SetColor(int id, const Color32 &c);
	// PURPOSE: Set the light source's color
	inline void SetColor(int id, float r, float g, float b);

	// PURPOSE: Get the intensity of the light source
	inline float GetIntensity(int id) const;
	// PURPOSE: Set the intensity of the light source
	inline void SetIntensity(int id, float intensity);

	// PURPOSE: Get the falloff radius of the light source (dist at which light is "black")
	inline float GetFalloff(int id) const;
	// PURPOSE: Set the falloff radius of the light source
	inline void SetFalloff(int id, float falloff);

	// PURPOSE: Get the cone angle of the light source (beyond this angle is "black")
	inline float GetConeAngle(int id) const;
	// PURPOSE: Set the cone angle of the light source
	inline void SetConeAngle(int id, float angle);

	// PURPOSE: Retrieve the type of the specified light source
	inline grcLightType GetLightType(int id) const;
	// PURPOSE: Sets the type of the specified light source
	inline void SetLightType(int id, grcLightType type);

	// PURPOSE: Retrieve the ambient light color
	inline Vec4V_Out GetAmbient() const;
	// PURPOSE: Set the ambient light color
	inline void SetAmbient(Vec4V_In ambient);
	// PURPOSE: Set the ambient light color
	inline void SetAmbient(const Color32 &ambient);
	// PURPOSE: Set the ambient light color
	inline void SetAmbient(float r, float g, float b,float a = 1.0f);

	// PURPOSE: Reset the light structure
	// PARAMS:  Id - Resets the specified light ID, -1 to reset all light sources
	void Reset(int id = -1);
	
	// PURPOSE: Modify direction vector of all light sources to point toward origin from current position
	void InitDirectionalFromPoint();

	// PURPOSE: return if the light is enabled or not
	inline bool IsLightEnabled(int id) const;
	// PURPOSE: Sets the light enabled property
	inline void SetLightEnabled(int id, bool enabled);

	// PURPOSE: say if we want to use ambient light
	inline bool DontUseAmbient() const;
	// PURPOSE: sat we want to use ambient light or not
	inline void SetUseAmbient(bool BANK_ONLY(use));

	// PURPOSE: Verify that light sources have appropriate data configured
#if __ASSERT
	bool Validate() const;
#else
	bool Validate() const { return true; }
#endif

#if __BANK
	// PURPOSE: Add tuning widgets for light sources
	void AddWidgets(class bkBank &B);
#endif

	struct grcLightSource {
		Vec3V m_Dir;				// Light direction
		Vec3V m_Pos;				// Light position
		Vec3V m_Color;				// Light color
		float	m_Intensity;		// Light intensity
		float	m_Falloff;			// light radius (meters)
		float   m_ConeAngle;		// light angle (radians)
		u8		m_Type;				// Type of light source
		u8		m_Enabled;			// Enabled light source		
	};

protected:
	atRangeArray<grcLightSource, MAX_LIGHTS> m_LightSources;	// Light sources used
	Vec4V m_Ambient;											// Ambient light
	s16	m_ActiveCount;											// Number of active lights
	s16 m_MaxAllowed;											// Maximum number of lights allowed for this instance
#if __BANK
	u8 m_DontUseAmbient;										// allow disable of ambient through bank
#endif
};

class grcLightState {
public:
	// These switch the default technique for every shader when not using a forced technique between draw[skinned] and unlit_draw[skinned]:
	static bool IsEnabled() { return sm_Lighting; }
	static bool SetEnabled(bool flag); 

	// These functions assume you're using RAGE lighting
	static void SetLightingGroup(const grcLightGroup &data);
	static grcLightGroup *GetLightingGroup() {return &sm_LightGroup;}

#if __DEV || __BANK
	static bool GetToolMode() { return sm_ToolMode; }
	static void SetToolMode(bool flag) { sm_ToolMode = flag; }
#endif

#if __BANK
	// PURPOSE: Add tuning widgets
	static void AddWidgets(class bkBank &B);
#endif

private:
	static DECLARE_MTR_THREAD bool sm_Lighting;
#if __DEV || __BANK
	static bool sm_ToolMode;
#endif
	static grcLightGroup sm_LightGroup;
};



inline s32 grcLightGroup::GetActiveCount() const {
	return m_ActiveCount;
}

inline s32 grcLightGroup::GetMaxAllowed() const {
    return m_MaxAllowed;
}

inline void grcLightGroup::SetActiveCount(s32 count) {
	AssertMsg(count <= (s32) m_MaxAllowed,"Active count out of range for this instance");
	m_ActiveCount = static_cast<s16>(count);
}

inline Vec3V_Out grcLightGroup::GetDirection(int id) const {
	return m_LightSources[id].m_Dir;
}

inline void grcLightGroup::SetDirection(int id, Vec3V_In dir) {
	m_LightSources[id].m_Dir = (dir);
}

inline Vec3V_Out grcLightGroup::GetPosition(int id) const {
	return m_LightSources[id].m_Pos;
}

inline void grcLightGroup::SetPosition(int id, Vec3V_In pos) {
	m_LightSources[id].m_Pos = (pos);
}

inline Vec3V_Out grcLightGroup::GetColor(int id ) const {
	return m_LightSources[id].m_Color;
}

inline void grcLightGroup::SetColor(int id, Vec3V_In c) {
	m_LightSources[id].m_Color = (c);
}

inline void grcLightGroup::SetColor(int id, const Color32 &c) {
	m_LightSources[id].m_Color = c.GetRGBA().GetXYZ();
}

inline void grcLightGroup::SetColor(int id, float r, float g, float b) {
	m_LightSources[id].m_Color = Vec3V(r, g, b);
}

inline float grcLightGroup::GetIntensity(int id) const {
	return m_LightSources[id].m_Intensity;
}

inline void grcLightGroup::SetIntensity(int id, float intensity) {
	m_LightSources[id].m_Intensity = intensity; 
}

inline float grcLightGroup::GetFalloff(int id) const {
	return m_LightSources[id].m_Falloff;
}

inline void grcLightGroup::SetFalloff(int id, float falloff) {
	m_LightSources[id].m_Falloff = falloff;
}

inline float grcLightGroup::GetConeAngle(int id) const {
	return m_LightSources[id].m_ConeAngle;
}

inline void grcLightGroup::SetConeAngle(int id, float angle)
{
	m_LightSources[id].m_ConeAngle = angle;
}

inline grcLightGroup::grcLightType grcLightGroup::GetLightType(int id) const {
	return static_cast<grcLightType>(m_LightSources[id].m_Type);
}

inline void grcLightGroup::SetLightType(int id, grcLightType type) {
	m_LightSources[id].m_Type = static_cast<u8>(type);
	AssertMsg( (grcLightType) m_LightSources[id].m_Type == type, "Error downcasting type to u8" );
}

inline Vec4V_Out grcLightGroup::GetAmbient() const {
	return m_Ambient;
}

inline void grcLightGroup::SetAmbient(Vec4V_In c) {
	m_Ambient = (c);
}

inline void grcLightGroup::SetAmbient(const Color32 &c) {
	m_Ambient = Vec4V(c.GetRedf(), c.GetGreenf(), c.GetBluef(), c.GetAlphaf());
}

inline void grcLightGroup::SetAmbient(float r, float g, float b, float a) {
	m_Ambient = Vec4V(r, g, b, a);
}

inline bool grcLightGroup::IsLightEnabled(int id) const
{
	return m_LightSources[id].m_Enabled != 0;
}

inline void grcLightGroup::SetLightEnabled(int id, bool enabled)
{
	m_LightSources[id].m_Enabled = enabled ? 1 : 0;
}

inline bool grcLightGroup::DontUseAmbient() const
{
#if __BANK
	return m_DontUseAmbient != 0;
#else
	return false;
#endif
}

inline void grcLightGroup::SetUseAmbient(bool BANK_ONLY(use))
{
#if __BANK
	m_DontUseAmbient = (use)? 0x0 : 0x1;
#endif
}

}	// namespace rage
#endif
