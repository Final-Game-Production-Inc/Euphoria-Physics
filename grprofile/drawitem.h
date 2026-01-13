//
// grprofile/drawitem.h
//
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved.
//

#ifndef GRPROFILE_DRAWITEM_H
#define GRPROFILE_DRAWITEM_H

//==============================================================
//
// The classes in these files form a system for queuing graphical
// annotations at any point in a game loop for later drawing.
// 

#include "drawcore.h"
#include "data/callback.h"

#if __PFDRAW

#include "atl/slist.h"
#include "data/base.h"
#include "system/codecheck.h"

namespace rage {

class bkBank;
class pfDrawGroup;
class pfDrawManager;
//class Vector3;


//=============================================================================
// PURPOSE
//   The base class for all components of the profile drawing system that can
//   be turned on/off and placed in groups.  These are pfDrawItems, pfDrawGroups,
//   and the pfDrawManager.
//
// <FLAG Component>
//
class pfDrawItemBase : public datBase
{
public:
	pfDrawItemBase
		(const char * name, Color32 baseColor, int priority, 
		pfDrawGroup * owningGroup, pfDrawManager * manager, bool startsEnabled=false);

	//=========================================
	// Accessors.

	const char * GetName() const
	{	return m_Name;	}

	int GetPriority() const
	{	return m_Priority;	}

	Color32 GetBaseColor()
	{	return m_BaseColor;	}

	bool GetEnabled() const
	{	return m_Enabled;	}

	bool *GetEnabledPtr()
	{	return &m_Enabled;	}

	bool GetParentsEnabled() const
	{	return m_ParentsEnabled;	}

	void SetName(const char *name)
	{	formatf(m_Name, kNameLenMax, name);	}

	virtual void SetEnabled(bool v)
	{	m_Enabled = v;	}

	bool WillDraw() const
	{	return m_Enabled && m_ParentsEnabled;	}

	//=========================================
	// Drawing interface.

	RETURNCHECK(bool) Begin(bool disableLighting = true, bool pushIdentity = true);

	void End();

	//=========================================
	// Widgets.

#if __BANK
	virtual void AddWidgets(bkBank & bank);

	void EnabledStateChanged();
#endif

	enum { kNameLenMax = 28 };

#if !HACK_RDR2
protected:
#endif
	friend class pfDrawGroup;
	virtual void SetParentsEnabled(bool v)
	{	m_ParentsEnabled = v;	}
protected:

	bool BeginManager();

	//=========================================
	// Data

	char m_Name[kNameLenMax];

	bool m_Enabled;
	bool m_ParentsEnabled;
	bool m_OldLighting;
	bool m_DisabledLighting;
	int m_Priority;
	pfDrawManager * m_Manager;
	Color32 m_BaseColor;

	pfDrawItemBase* m_Next;
};


//=============================================================================
// PURPOSE
//   The unit of drawing in the profile draw system.  An item can be enabled
//   or disabled.  If enabled, then a call to Begin will start buffering of the
//   graphical data and return true.  Then a call to End will stop buffering.
//   Additionally, the pfDrawItem class provides convenience functions to
//   turn drawing on/off and draw something all at once.
//
// <FLAG Component>
//
class pfDrawItem : public pfDrawItemBase
{
public:
	pfDrawItem
		(const char * name, Color32 baseColor, int priority, 
		pfDrawGroup * owningGroup, pfDrawManager * manager, bool startsEnabled=false)
		:	pfDrawItemBase(name, baseColor, priority, owningGroup, manager,startsEnabled)
	{	}

	void SetWorldMtx(const Matrix34& world);

	void DrawLine(const Vector3& start, const Vector3& end);
	void DrawLine(const Vector3& start, const Vector3& end, Color32 color);

	void DrawArrow(const Vector3& start, const Vector3& end);
	void DrawArrow(const Vector3& start, const Vector3& end, Color32 color);

	void DrawTick(const Vector3& v0, float size);
	void DrawTick(const Vector3& v0, float size, Color32 color);

	void DrawTick(const Matrix34& matrix, float size);
	void DrawTick(const Matrix34& matrix, float size, Color32 color);

	void DrawBox(const Matrix34& mat, const Vector3& halfSize, bool solid=false);
	void DrawBox(const Matrix34& mat, const Vector3& halfSize, Color32 color, bool solid=false);

	void DrawSphere(float radius, const Vector3& center);
	void DrawSphere(float radius, const Vector3& center, Color32 color, int steps=8);

	void DrawSolidSphere(float radius, const Vector3& center, Color32 solidColor, Color32 wireColor,int steps=8);

	void DrawCamera(const Matrix34& matrix, float fov, float* nearClip, float* aspectRatio);
	void DrawCamera(const Matrix34& matrix, float fov, float* nearClip, float* aspectRatio, Color32 color);

	static void DrawCameraIcon(const Matrix34& matrix, float fov, float* nearClip, float* aspectRatio, Color32 color);

	void DrawCapsule(const Vector3& v0, const Vector3& v1, float radius);
	void DrawCapsule(const Vector3& v0, const Vector3& v1, float radius, Color32 color);
	void DrawCapsule(const Matrix34& mat, const float length, const float radius);
	void DrawCapsule(const Matrix34& mat, const float length, const float radius, const Color32 color);

	void DrawTaperedCapsule(const Vector3& v0, const Vector3& v1, float radiusA, float radiusB);
	void DrawTaperedCapsule(const Vector3& v0, const Vector3& v1, float radiusA, float radiusB, Color32 color);
	void DrawTaperedCapsule(const Matrix34& mat, const float length, const float radiusA, const float radiusB);
	void DrawTaperedCapsule(const Matrix34& mat, const float length, const float radiusA, const float radiusB, const Color32 color);

	void Draw2dText(float xPos, float yPos, const char* string);
	void Draw2dText(const Vector3& worldPosition, const char* string);
	void Draw2dText(const Vector3& worldPosition, const char* string, Color32 color);
};

//=============================================================================
// PURPOSE
//   A subclass of pfDrawItem, that displays a slider widget. That widget can
//   be controlled by the user to customize the display of this draw item.
//
// NOTES
//   A slider value of zero indicates that this item is disabled.
//
// <FLAG Component>
//
class pfDrawItemSlider : public pfDrawItem
{
public:
	pfDrawItemSlider(const char * name,
					 Color32 baseColor,
					 int priority, 
					 pfDrawGroup * owningGroup,
					 pfDrawManager * manager,
					 float startingValue,
					 float maxValue,
					 float increment)
		: pfDrawItem(name, baseColor, priority, owningGroup, manager, startingValue != 0.0f)
		, m_Value(startingValue)
		, m_MaxValue(maxValue)
		, m_Increment(increment)
	{	}

	float GetValue();
	void SetValue(float value);

#if __BANK
	virtual void AddWidgets(bkBank & bank);

	void SliderValueChanged();
#endif

private:
	float m_Value;
	float m_MaxValue;
	float m_Increment;
};

//=============================================================================
// PURPOSE
//   A subclass of pfDrawItem, that displays a slider widget. That widget can
//   be controlled by the user to customize the display of this draw item.
//
// NOTES
//   Unlike the basic slider, this one allows negative values. It therefore
//   requires an explicit minimum value.
//
// <FLAG Component>
//
class pfDrawItemSliderFull : public pfDrawItem
{
public:
	pfDrawItemSliderFull(const char * name,
		Color32 baseColor,
		int priority, 
		pfDrawGroup * owningGroup,
		pfDrawManager * manager,
		float startingValue,
		float minValue,
		float maxValue,
		float increment)
		: pfDrawItem(name, baseColor, priority, owningGroup, manager, startingValue != 0.0f)
		, m_Value(startingValue)
		, m_MinValue(minValue)
		, m_MaxValue(maxValue)
		, m_Increment(increment)
	{	}

	float GetValue();
	void SetValue(float value);

#if __BANK
	virtual void AddWidgets(bkBank & bank);

	void SliderValueChanged();
#endif

private:
	float m_Value;
	float m_MinValue;
	float m_MaxValue;
	float m_Increment;
};


//=============================================================================
// PURPOSE
//   A subclass of pfDrawItem, that displays a slider widget. That widget can
//   be controlled by the user to customize the display of this draw item.
//
// NOTES
//   A slider value of zero indicates that this item is disabled.
//
// <FLAG Component>
//
class pfDrawItemSliderInt : public pfDrawItem
{
public:
	pfDrawItemSliderInt(const char * name,
					    Color32 baseColor,
					    int priority, 
					    pfDrawGroup * owningGroup,
					    pfDrawManager * manager,
					    int startingValue,
					    int maxValue,
					    int increment)
		: pfDrawItem(name, baseColor, priority, owningGroup, manager, startingValue != 0.0f)
		, m_Value(startingValue)
		, m_MaxValue(maxValue)
		, m_Increment(increment)
	{	}

	int GetValue();
	void SetValue(int value);

#if __BANK
	virtual void AddWidgets(bkBank & bank);

	void SliderValueChanged();
#endif

private:
	int m_Value;
	int m_MaxValue;
	int m_Increment;
};

//=============================================================================
// PURPOSE
//   A subclass of pfDrawItem, that displays a slider widget. That widget can
//   be controlled by the user to customize the display of this draw item.
//
// NOTES
//   A slider value of zero indicates that this item is disabled.
//
// <FLAG Component>
//
class pfDrawItemSliderIntFull : public pfDrawItem
{
public:
	pfDrawItemSliderIntFull(const char * name,
		Color32 baseColor,
		int priority, 
		pfDrawGroup * owningGroup,
		pfDrawManager * manager,
		int startingValue,
		int minValue,
		int maxValue,
		int increment)
		: pfDrawItem(name, baseColor, priority, owningGroup, manager, startingValue != 0.0f)
		, m_Value(startingValue)
		, m_MinValue(minValue)
		, m_MaxValue(maxValue)
		, m_Increment(increment)
	{	}

	int GetValue();
	void SetValue(int value);

#if __BANK
	virtual void AddWidgets(bkBank & bank);

	void SliderValueChanged();
#endif

private:
	int m_Value;
	int m_MinValue;
	int m_MaxValue;
	int m_Increment;
};

//=============================================================================
// PURPOSE
//   A subclass of pfDrawItem, that displays a color widget. That widget can
//   be controlled by the user to customize the display of this draw item.
//
// <FLAG Component>
//
class pfDrawItemColor : public pfDrawItem
{
public:
	pfDrawItemColor(const char * name,
					 Color32 baseColor,
					 int priority, 
					 pfDrawGroup * owningGroup,
					 pfDrawManager * manager,
					 Color32 startingColor = Color32(128,128,128))
		: pfDrawItem(name, baseColor, priority, owningGroup, manager)
		, m_Value(startingColor)
	{	}

	Color32 GetValue();

#if __BANK
	virtual void AddWidgets(bkBank & bank);

	void ColorValueChanged();
#endif

private:
	Color32 m_Value;
};

//=============================================================================
// PURPOSE
//   A subclass of pfDrawItem that adds a button with an arbitrary callback.
//
// <FLAG Component>
//
class pfDrawItemButton : public pfDrawItem
{
public:
	pfDrawItemButton(const char * name,
		Color32 baseColor,
		int priority, 
		pfDrawGroup * owningGroup,
		pfDrawManager * manager,
		datCallback callback=NullCB,
		const char *tooltop=0
		)
		: pfDrawItem(name, baseColor, priority, owningGroup, manager, true)
		, m_Callback(callback)
		, m_ToolTip(tooltop)
	{	}

#if __BANK
	virtual void AddWidgets(bkBank & bank);
#endif

private:
	datCallback m_Callback;
	const char * m_ToolTip;
};

//=============================================================================
// Implementations.

inline RETURNCHECK(bool) pfDrawItemBase::Begin(bool disableLighting, bool pushIdentity)
{
	FastAssert(m_Manager);
	if(m_Enabled && m_ParentsEnabled)
	{
		bool managerResult = BeginManager();

		if (disableLighting)
		{
			m_OldLighting = grcLighting(false);
			m_DisabledLighting = true;
		}
		else
		{
			m_DisabledLighting = false;
		}

		if (pushIdentity)
		{
			grcWorldIdentity();
		}

		return managerResult;
	}
	return false;
}


inline void pfDrawItem::SetWorldMtx(const Matrix34& world)
{
	if (Begin(false))
	{
		grcWorldMtx(RCC_MAT34V(world));
		End();
	}
}

inline void pfDrawItem::DrawLine(const Vector3& start, const Vector3& end)
{	
	DrawLine(start, end, GetBaseColor());
}

inline void pfDrawItem::DrawLine(const Vector3& start, const Vector3& end, Color32 color)
{
	if (Begin(false))
	{
		grcDrawLine(start, end, color);
		End();
	}
}

inline void pfDrawItem::DrawArrow(const Vector3& start, const Vector3& end)
{	
	DrawArrow(start, end, GetBaseColor());
}

inline void pfDrawItem::DrawArrow(const Vector3& start, const Vector3& end, Color32 color)
{
	if (Begin(false))
	{
		grcColor(color);
		pfDrawArrow(start, end);
		End();
	}
}

inline void pfDrawItem::DrawSphere(float radius, const Vector3& center)
{
	DrawSphere(radius, center, GetBaseColor());
}

inline void pfDrawItem::DrawSphere(float radius, const Vector3& center, Color32 color, int steps)
{
	if (Begin(false))
	{
		grcColor(color);
		grcDrawSphere(radius,center,steps,true);
		End();
	}
}
inline void pfDrawItem::DrawSolidSphere(float radius, const Vector3& center, Color32 solidColor, Color32 wireColor,int steps)
{
	if (Begin(false))
	{
		grcColor(solidColor);
		grcDrawSphere(radius, center, steps, false, true);
		if (solidColor != wireColor) {
			grcColor(wireColor);
			grcDrawSphere(radius, center, steps, true, false);
		}
		End();
	}
}

inline void pfDrawItem::DrawTick(const Vector3& v0, float size) {
	DrawTick(v0, size, GetBaseColor());
}
inline void pfDrawItem::DrawTick(const Matrix34& matrix, float size) {
	DrawTick(matrix, size, GetBaseColor());
}
inline void pfDrawItem::DrawBox(const Matrix34& mat, const Vector3& halfSize, bool solid)
{
	DrawBox(mat, halfSize, GetBaseColor(), solid);
}
inline void pfDrawItem::DrawCamera(const Matrix34& matrix, float fov, float* nearClip, float* aspectRatio)
{
	DrawCamera(matrix, fov, nearClip, aspectRatio, GetBaseColor());
}

inline void pfDrawItem::DrawCapsule(const Vector3& v0, const Vector3& v1, float radius)
{
	DrawCapsule(v0, v1, radius, GetBaseColor());
}

inline void pfDrawItem::DrawCapsule(const Matrix34& mat, const float length, const float radius)
{
	DrawCapsule(mat, length, radius, GetBaseColor());
}

inline void pfDrawItem::DrawTaperedCapsule(const Vector3& v0, const Vector3& v1, float radiusA, float radiusB)
{
	DrawTaperedCapsule(v0, v1, radiusA, radiusB, GetBaseColor());
}

inline void pfDrawItem::DrawTaperedCapsule(const Matrix34& mat, const float length, const float radiusA, const float radiusB)
{
	DrawTaperedCapsule(mat, length, radiusA, radiusB, GetBaseColor());
}

}	// namespace rage

#endif // __PFDRAW
#endif // GRPROFILE_DRAWITEM_H

// <eof> grprofile/drawitem.h
