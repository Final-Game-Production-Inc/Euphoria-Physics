//
// profile/drawitem.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "drawitem.h"
#include "drawmanager.h"

#include "bank/bank.h"
#include "math/simplemath.h"


using namespace rage;

#if __PFDRAW

//==============================================================
// pfDrawItemBase

pfDrawItemBase::pfDrawItemBase
	(const char * name, Color32 baseColor, int priority, 
	pfDrawGroup * owningGroup, pfDrawManager * manager, bool startsEnabled)
	: m_Enabled(startsEnabled)
	, m_ParentsEnabled(false)
	, m_Priority(priority)
	, m_Manager(manager)
	, m_BaseColor(baseColor)
{
	strncpy(m_Name,name,kNameLenMax-1);
	m_Name[kNameLenMax-1] = '\0';
	if(owningGroup)
	{
		owningGroup->RegisterItem(this);
		SetParentsEnabled(owningGroup->GetEnabled() && owningGroup->GetParentsEnabled());
	}
	else
	{
		m_ParentsEnabled = true;
	}
}


bool pfDrawItemBase::BeginManager()
{
	bool result = m_Manager->Begin();
	grcColor(m_BaseColor);
	return result;
}


void pfDrawItemBase::End()
{
	if (m_DisabledLighting)
	{
		grcLighting(m_OldLighting);
	}

	Assert(m_Manager);
	m_Manager->End();
}



#if __BANK
void pfDrawItemBase::EnabledStateChanged()
{
	SetEnabled(m_Enabled);
}


void pfDrawItemBase::AddWidgets(bkBank & bank)
{
	bank.AddToggle(m_Name,&m_Enabled,datCallback(MFA(pfDrawItemBase::EnabledStateChanged),this));
}
#endif


//==============================================================
// pfDrawItem

void pfDrawItem::DrawTick(const Vector3& v0, float size, Color32 color)
{
	if (Begin())
	{
		Vector3 a, b;
		a = b = v0;
		a.x -= size;
		b.x += size;
		grcDrawLine(a, b, color);
		a = b = v0;
		a.y -= size;
		b.y += size;
		grcDrawLine(a, b, color);
		a = b = v0;
		a.z -= size;
		b.z += size;
		grcDrawLine(a, b, color);
		End();
	}
}

void pfDrawItem::DrawTick(const Matrix34& matrix, float size, Color32 color)
{
	if (Begin())
	{
		Vector3 a, b;

		a = b = ORIGIN;
		a.x -= size;
		b.x += size;
		matrix.Transform(a);
		matrix.Transform(b);
		grcDrawLine(a, b, color);

		a = b = ORIGIN;
		a.y -= size;
		b.y += size;
		matrix.Transform(a);
		matrix.Transform(b);
		grcDrawLine(a, b, color);

		a = b = ORIGIN;
		a.z -= size;
		b.z += size;
		matrix.Transform(a);
		matrix.Transform(b);
		grcDrawLine(a, b, color);

		End();
	}
}

void pfDrawItem::DrawBox(const Matrix34& mat, const Vector3& halfSize, Color32 color, bool solid)
{
	if (Begin())
	{
		Vector3 fullSize(halfSize);
		fullSize.Scale(2.0f);

		if(solid)
		{
			grcDrawSolidBox(fullSize, mat, color.MultiplyAlpha(50));
			grcDrawBox(fullSize, mat, color);
		}
		else
		{
			grcDrawBox(fullSize, mat, color);
		}

		End();
	}
}


void pfDrawItem::DrawCapsule(const Vector3& v0, const Vector3& v1, float radius, Color32 color)
{
	if (Begin())
	{
		bool oldLighting = grcLighting(false);
		grcColor(color);
		Vector3 axis;
		axis.Subtract(v1,v0);
		float length=axis.Mag();
		float inverseLength = InvertSafe(length,0.0f);
		axis.Scale(inverseLength);	// Normalize
		Matrix34 m1;
		m1.Identity3x3();
		m1.d.AddScaled(v0,axis,0.5f*length);
		if (length>0.0f)
		{
			m1.MakeRotateTo(m1.b,axis);
		}
		grcDrawCapsule(length, radius, m1, 6, false);
		grcLighting(oldLighting);

		End();
	}
}

void pfDrawItem::DrawCapsule(const Matrix34& mat, const float length, const float radius, const Color32 color)
{
	if (Begin())
	{
		bool oldLighting = grcLighting(false);
		grcColor(color);
		grcDrawCapsule(length, radius, mat, 6, false);
		grcLighting(oldLighting);

		End();
	}
}

void pfDrawItem::DrawTaperedCapsule(const Vector3& v0, const Vector3& v1, float radiusA, float radiusB, Color32 color)
{
	if (Begin())
	{
		bool oldLighting = grcLighting(false);
		grcColor(color);
		Vector3 axis;
		axis.Subtract(v1,v0);
		float length=axis.Mag();
		float inverseLength = InvertSafe(length,0.0f);
		axis.Scale(inverseLength);	// Normalize
		Matrix34 m1;
		m1.Identity3x3();
		m1.d.AddScaled(v0,axis,0.5f*length);
		m1.MakeRotateTo(m1.b,axis);
		grcDrawTaperedCapsule(length, radiusA, radiusB, m1, 6, false);
		grcLighting(oldLighting);

		End();
	}
}

void pfDrawItem::DrawTaperedCapsule(const Matrix34& mat, const float length, const float radiusA, const float radiusB, const Color32 color)
{
	if (Begin())
	{
		bool oldLighting = grcLighting(false);
		grcColor(color);
		grcDrawTaperedCapsule(length, radiusA, radiusB, mat, 6, false);
		grcLighting(oldLighting);

		End();
	}
}

void pfDrawItem::DrawCamera(const Matrix34& matrix, float fov, float* nearClip, float* aspectRatio, Color32 color)
{
	if (Begin())
	{
		DrawCameraIcon(matrix, fov, nearClip, aspectRatio, color);

		const float ratio = aspectRatio ? *aspectRatio : 1.333f;
		const float heightProportion = tanf(fov * 0.5f) * 2.0f;
		const float widthProportion = heightProportion * ratio;
		float len = 3.0f;
		float height = heightProportion * len;
		float width = widthProportion * len;

		grcDrawFrustum(matrix, width, height, len, color.MultiplyAlpha(100));

		End();
	}
}

void pfDrawItem::DrawCameraIcon(const Matrix34& matrix, float fov, float* nearClip, float* aspectRatio, Color32 color)
{
	const float d = 0.2f;
	const float ratio = aspectRatio ? *aspectRatio : 1.333f;
	const float heightProportion = tanf(fov * 0.5f) * 2.0f;
	const float widthProportion = heightProportion * ratio;
	
	
	if(nearClip)
	{
		float height = *nearClip * heightProportion;
		float width = *nearClip * widthProportion;

		grcDrawFrustum(matrix, width, height, *nearClip, Color32(255,0,0));
	}
	else
	{
		float height = 0.2f * heightProportion;
		float width = 0.2f * widthProportion;
		grcDrawFrustum(matrix, width, height, 0.2f,color);
	}


	Matrix34 m(matrix);
	m.d.AddScaled(m.c, 0.5f * d);
	grcDrawBox(Vector3(d,d,d), m, color);

	Matrix34 m1(m);
	m1.d.AddScaled(m1.b, 0.2f);
	m1.d.AddScaled(m1.c, 0.1f);
	m1.RotateLocalZ(PI * 0.5f);
	grcDrawCylinder(0.03f, 0.1f, m1, 6, false);

	Matrix34 m2(m);
	m2.d.AddScaled(m2.b, 0.2f);
	m2.d.AddScaled(m2.c, -0.1f);
	m2.RotateLocalZ(PI * 0.5f);
	grcDrawCylinder(0.03f, 0.1f, m2, 6, false);

}

void pfDrawItem::Draw2dText(float xPos, float yPos, const char* string)
{
	if (Begin())
	{
		grcDraw2dText(xPos, yPos, string);

		End();
	}
}


void pfDrawItem::Draw2dText(const Vector3& worldPosition, const char* string)
{
	if (Begin())
	{
		grcDrawLabelf(worldPosition,string);

		End();
	}
}

void pfDrawItem::Draw2dText(const Vector3& worldPosition, const char* string, Color32 color)
{
	if (Begin())
	{
		grcColor(color);
		grcDrawLabelf(worldPosition,string);

		End();
	}
}


float pfDrawItemSlider::GetValue()
{
	return m_Value;
}

float pfDrawItemSliderFull::GetValue()
{
	return m_Value;
}

int pfDrawItemSliderInt::GetValue()
{
	return m_Value;
}

int pfDrawItemSliderIntFull::GetValue()
{
	return m_Value;
}

void pfDrawItemSlider::SetValue(float value)
{
	m_Value = value;
}

void pfDrawItemSliderFull::SetValue(float value)
{
	m_Value = value;
}

void pfDrawItemSliderInt::SetValue(int value)
{
	m_Value = value;
}

void pfDrawItemSliderIntFull::SetValue(int value)
{
	m_Value = value;
}

Color32 pfDrawItemColor::GetValue()
{
	return m_Value;
}

#if __BANK
void pfDrawItemSlider::SliderValueChanged()
{
	m_Enabled = (m_Value != 0.0f);
	SetEnabled(m_Enabled);
}

void pfDrawItemSliderFull::SliderValueChanged()
{
	m_Enabled = true;//(m_Value != 0.0f);
	SetEnabled(m_Enabled);
}

void pfDrawItemSliderInt::SliderValueChanged()
{
	m_Enabled = (m_Value != 0);
	SetEnabled(m_Enabled);
}

void pfDrawItemSliderIntFull::SliderValueChanged()
{
	m_Enabled = true;//(m_Value != 0);
	SetEnabled(m_Enabled);
}

void pfDrawItemSlider::AddWidgets(bkBank & bank)
{
	bank.AddSlider(m_Name,&m_Value,0.0f,m_MaxValue,m_Increment,datCallback(MFA(pfDrawItemSlider::SliderValueChanged),this));
}

void pfDrawItemSliderInt::AddWidgets(bkBank & bank)
{
	bank.AddSlider(m_Name,&m_Value,0,m_MaxValue,m_Increment,datCallback(MFA(pfDrawItemSliderInt::SliderValueChanged),this));
}

void pfDrawItemSliderFull::AddWidgets(bkBank & bank)
{
	bank.AddSlider(m_Name,&m_Value,m_MinValue,m_MaxValue,m_Increment,datCallback(MFA(pfDrawItemSliderFull::SliderValueChanged),this));
}

void pfDrawItemSliderIntFull::AddWidgets(bkBank & bank)
{
	bank.AddSlider(m_Name,&m_Value,m_MinValue,m_MaxValue,m_Increment,datCallback(MFA(pfDrawItemSliderIntFull::SliderValueChanged),this));
}

void pfDrawItemColor::ColorValueChanged()
{
	m_Enabled = true;
	SetEnabled(m_Enabled);
}

void pfDrawItemColor::AddWidgets(bkBank & bank)
{
	bank.AddColor(m_Name, &m_Value, datCallback(MFA(pfDrawItemColor::ColorValueChanged),this));
}

void pfDrawItemButton::AddWidgets(bkBank & bank)
{
	bank.AddButton(m_Name, m_Callback, m_ToolTip);
}
#endif // __BANK

#endif // __PFDRAW

// <eof> profile/drawitem.cpp
