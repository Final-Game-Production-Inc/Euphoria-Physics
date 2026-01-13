//
// profile/chart.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "grprofile/chart.h"

#include "diag/tracker.h"
#include "grcore/im.h"
#include "grcore/light.h"
#include "grcore/stateblock.h"
#include "grcore/viewport.h"
#include "vector/geometry.h"

using namespace rage;

#if __STATS

//=============================================================================
// pfChart

pfChart::pfChart()
	: m_StoredViewport(NULL)
	, m_XMin(0.0f)
	, m_XMax(1.0f)
	, m_YMin(0.0f)
	, m_YMax(1.0f)
	, m_FieldLeft(0.0f)
	, m_FieldRight(100.0f)
	, m_FieldTop(0.0f)
	, m_FieldBottom(100.0f)
	, m_BorderOuterWidth(5.0f)
	, m_BorderInnerWidth(3.0f)
	, m_ChartViewport(NULL)
{
}

pfChart::~pfChart()
{
	delete m_ChartViewport;
}

bool pfChart::DrawOrthoBegin()
{
	// Create on first use, otherwise we'll crash in global ctor
	if (!m_ChartViewport)
	{
		RAGE_TRACK(ChartViewport);
		m_ChartViewport = rage_new grcViewport();
		UpdateViewport();
	}

	// Setup the viewport.
	Assert(!m_StoredViewport);
	m_StoredViewport = grcViewport::GetCurrent();
	if(!m_StoredViewport)
	{
		return false;
	}

	grcViewport::SetCurrent(m_ChartViewport);

	grcLightState::SetEnabled(false);
	grcStateBlock::SetDepthStencilState(grcStateBlock::DSS_IgnoreDepth);
	grcStateBlock::SetBlendState(grcStateBlock::BS_Normal);
	grcViewport::SetCurrentWorldIdentity();

	grcBindTexture(NULL);

	return true;
}


void pfChart::DrawOrthoEnd()
{
	// Restore the old viewport.
	Assert(m_StoredViewport!=NULL);
	grcViewport::SetCurrent(m_StoredViewport);
	m_StoredViewport = NULL;
}


void pfChart::SetBorderWidth(float outer, float inner)
{
	m_BorderOuterWidth = outer;
	m_BorderInnerWidth = inner;
	UpdateViewport();
}


void pfChart::SetDrawPosition(float top, float bottom, float left, float right)
{
	m_FieldTop = top;
	m_FieldBottom = bottom;
	m_FieldLeft = left;
	m_FieldRight = right;
	UpdateViewport();
}


void pfChart::SetXRange(float xMin, float xMax)
{
	Assert(xMax>xMin+SMALL_FLOAT);
	m_XMin = xMin;
	m_XMax = xMax;
	UpdateViewport();
}


void pfChart::SetYRange(float yMin, float yMax)
{
	Assert(yMax>yMin+SMALL_FLOAT);
	m_YMin = yMin;
	m_YMax = yMax;
	UpdateViewport();
}



void pfChart::UpdateViewport()
{
	float fullBorderWidth = m_BorderOuterWidth + m_BorderInnerWidth;

	float top = m_FieldTop - fullBorderWidth;
	float height = m_FieldBottom + fullBorderWidth - top;
	float left = m_FieldLeft - fullBorderWidth;
	float width = m_FieldRight + fullBorderWidth - left;

	Assert(height>0.0f && width>0.0f);

	m_ViewportXMin = m_XMin - fullBorderWidth * (m_XMax - m_XMin) / width;
	m_ViewportXMax = m_XMax + fullBorderWidth * (m_XMax - m_XMin) / width;
	m_ViewportYMin = m_YMin - fullBorderWidth * (m_YMax - m_YMin) / height;
	m_ViewportYMax = m_YMax + fullBorderWidth * (m_YMax - m_YMin) / height;

	m_ChartViewport->Ortho(m_ViewportXMin,m_ViewportXMax,m_ViewportYMin,m_ViewportYMax,0.0f,+1.0f);
	m_ChartViewport->SetWindow((int)left,(int)top,(int)width,(int)height);
}


void pfChart::SetDrawPosition(int top, int bottom, int left, int right)
{
	SetDrawPosition((float)top,(float)bottom,(float)left,(float)right);
}


float pfChart::ValToScreenX(float x) const
{
	return m_FieldLeft + ((x - m_XMin) / (m_XMax - m_XMin)) * (m_FieldRight - m_FieldLeft);
}


float pfChart::ValToScreenY(float y) const
{
	return m_FieldBottom - ((y - m_YMin) / (m_YMax - m_YMin)) * (m_FieldBottom - m_FieldTop);
}


float pfChart::ScreenToValX(float x) const
{
	return (x - m_FieldLeft) / (m_FieldRight - m_FieldLeft) * (m_XMax - m_XMin) + m_XMin;
}


float pfChart::ScreenToValY(float y) const
{
	return (y - m_FieldBottom) / (m_FieldTop - m_FieldBottom) * (m_YMax - m_YMin) + m_YMin;
}


void pfChart::DrawBackground()
{
	float innerBorderViewportLeft = m_XMin - m_BorderInnerWidth * (m_XMax - m_XMin) / m_ChartViewport->GetWidth();
	float innerBorderViewportRight = m_XMax + m_BorderInnerWidth * (m_XMax - m_XMin) / m_ChartViewport->GetWidth();
	float innerBorderViewportBottom = m_YMin - m_BorderInnerWidth * (m_YMax - m_YMin) / m_ChartViewport->GetHeight();
	float innerBorderViewportTop = m_YMax + m_BorderInnerWidth * (m_YMax - m_YMin) / m_ChartViewport->GetHeight();

	// Draw the outer border.
	grcColor4f(Vector4(0.2f,0.2f,0.2f,1.0f));

	grcBegin(drawTriStrip,4);
	grcVertex2f(m_ViewportXMax,innerBorderViewportTop);
	grcVertex2f(m_ViewportXMin,innerBorderViewportTop);
	grcVertex2f(m_ViewportXMax,m_ViewportYMax);
	grcVertex2f(m_ViewportXMin,m_ViewportYMax);
	grcEnd();
	grcBegin(drawTriStrip,4);
	grcVertex2f(m_ViewportXMax,innerBorderViewportBottom);
	grcVertex2f(m_ViewportXMin,innerBorderViewportBottom);
	grcVertex2f(m_ViewportXMax,m_ViewportYMin);
	grcVertex2f(m_ViewportXMin,m_ViewportYMin);
	grcEnd();
	grcBegin(drawTriStrip,4);
	grcVertex2f(m_ViewportXMax,m_ViewportYMax);
	grcVertex2f(innerBorderViewportRight,m_ViewportYMax);
	grcVertex2f(m_ViewportXMax,m_ViewportYMin);
	grcVertex2f(innerBorderViewportRight,m_ViewportYMin);
	grcEnd();
	grcBegin(drawTriStrip,4);
	grcVertex2f(m_ViewportXMin,m_ViewportYMax);
	grcVertex2f(innerBorderViewportLeft,m_ViewportYMax);
	grcVertex2f(m_ViewportXMin,m_ViewportYMin);
	grcVertex2f(innerBorderViewportLeft,m_ViewportYMin);
	grcEnd();

	// Draw the chart field.
	grcColor4f(Vector4(0.0f,0.0f,0.0f,0.3f));
	grcBegin(drawTriStrip,4);
	grcVertex2f(m_XMax,m_YMax);
	grcVertex2f(m_XMin,m_YMax);
	grcVertex2f(m_XMax,m_YMin);
	grcVertex2f(m_XMin,m_YMin);
	grcEnd();

	grcColor4f(Vector4(1.0f,1.0f,1.0f,0.5f));
	grcBegin(drawLineStrip,5);
	grcVertex2f(m_XMax,m_YMax);
	grcVertex2f(m_XMin,m_YMax);
	grcVertex2f(m_XMin,m_YMin);
	grcVertex2f(m_XMax,m_YMin);
	grcVertex2f(m_XMax,m_YMax);
	grcEnd();
}


void pfChart::DrawGrid(float xstep, float ystep)
{
	grcColor4f(Vector4(0.8f,0.8f,0.8f,0.4f));

	for(float x=ceilf(m_XMin/xstep)*xstep; x<=floorf(m_XMax/xstep)*xstep; x+=xstep)
	{
		grcBegin(drawLines,2);
		grcVertex2f(x,m_YMax);
		grcVertex2f(x,m_YMin);
		grcEnd();
	}

	for(float y=ceilf(m_YMin/ystep)*ystep; y<=floorf(m_YMax/ystep)*ystep; y+=ystep)
	{
		grcBegin(drawLines,2);
		grcVertex2f(m_XMin,y);
		grcVertex2f(m_XMax,y);
		grcEnd();
	}
}


void pfChart::DrawSegment(const Vector2 & start, const Vector2 & end)
{
	Vector2 clippedStart, clippedEnd;
	if(ClipSegmentToChart(clippedStart,clippedEnd,start,end))
	{
		Vector2 p0, p1;
		grcBegin(drawLines,2);
		grcVertex2f(clippedStart);
		grcVertex2f(clippedEnd);
		grcEnd();
	}
}


void pfChart::DrawSegmentColored(const Vector2 & start, const Vector4 & colorStart, const Vector2 & end, const Vector4 & colorEnd)
{
	Vector2 clippedStart, clippedEnd;
	if(ClipSegmentToChart(clippedStart,clippedEnd,start,end))
	{
		grcBegin(drawLines,2);
		grcColor4f(colorStart);
		grcVertex2f(clippedStart);
		grcColor4f(colorEnd);
		grcVertex2f(clippedEnd);
		grcEnd();
	}
}


bool pfChart::ClipSegmentToChart(Vector2 & screenStartOut, Vector2 & screenEndOut, const Vector2 & start, const Vector2 & end)
{
	float t0, t1;
	Vector3 p0, p01;
	p0.SetXZ(start);
	p01.SetXZ(end);
	p01.Subtract(p0);

	Vector3 clipMin(m_XMin,-FLT_MAX,m_YMin);
	Vector3 clipMax(m_XMax,FLT_MAX,m_YMax);

	Vector3 tmp0, tmp1;
	int idx0, idx1;
	int numIsects = geomBoxes::TestSegmentToBox(p0,p01,clipMin,clipMax,&t0,&tmp0,&idx0,&t1,&tmp1,&idx1);

	if(numIsects==0)
	{
		if(geomPoints::IsPointInBox(p0,clipMin,clipMax))
		{
			screenStartOut = start;
			screenEndOut = end;
		}
		else
		{
			screenStartOut.Zero();
			screenEndOut.Zero();
			return false;
		}
	}
	else if(numIsects==1)
	{
		if(geomPoints::IsPointInBox(p0,clipMin,clipMax))
		{
			screenStartOut = start;
			screenEndOut.Lerp(t0,start,end);
		}
		else
		{
			screenStartOut.Lerp(t0,start,end);
			screenEndOut = end;
		}
	}
	else if(numIsects==2)
	{
		screenStartOut.Lerp(t0,start,end);
		screenEndOut.Lerp(t1,start,end);
	}

	return true;
}

#endif // __STATS

// EOF profile/chart.c
