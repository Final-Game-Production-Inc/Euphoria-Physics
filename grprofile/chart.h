//
// grprofile/chart.h
//
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved.
//

#ifndef GRPROFILE_CHART_H
#define GRPROFILE_CHART_H

//=============================================================================
// external defines

#include "diag/stats.h"
#include "math/constants.h"
#include "vector/vector2.h"

#if __STATS

namespace rage {

class Vector4;
class grcViewport;

//=============================================================================
// pfChart
// PURPOSE
//   This class provides some functionality common to drawing simple 2-d
//   charts onscreen.  This forms the basic charting used by the pfEKGMgr.
//   Additionally, it is used by some runtime tools to display 2-d data.
//
// <FLAG Component>
class pfChart
{
public:
	pfChart();
	~pfChart();

	//=========================================================================
	// Operations

	bool DrawOrthoBegin();

	void DrawOrthoEnd();

	void DrawBackground();

	void DrawGrid(float xStep, float yStep);

	float ValToScreenX(float x) const;

	float ValToScreenY(float y) const;

	float ScreenToValX(float x) const;

	float ScreenToValY(float y) const;

	void DrawSegment(const Vector2 & start, const Vector2 & end);

	void DrawSegmentColored(const Vector2 & start, const Vector4 & colorStart, const Vector2 & end, const Vector4 & colorEnd);

	//=========================================================================
	// Accessors

	float GetBorderWidth() const;

	const grcViewport & GetViewport() const;

	//=========================================================================
	// Manipulators

	void SetXRange(float xMin, float xMax);

	void SetYRange(float yMin, float yMax);

	void SetDrawPosition(float top, float bottom, float left, float right);

	void SetDrawPosition(int top, int bottom, int left, int right);

	void SetBorderWidth(float outer, float inner);

protected:
	//=========================================================================
	// Internal operations

	void UpdateViewport();

	bool ClipSegmentToChart(Vector2 & screenStart, Vector2 & screenEnd, const Vector2 & start, const Vector2 & end);

	//=========================================================================
	// Data

	grcViewport * m_StoredViewport;

	grcViewport * m_ChartViewport;

	float m_XMin;

	float m_XMax;

	float m_YMin;

	float m_YMax;

	float m_ViewportXMin;

	float m_ViewportXMax;

	float m_ViewportYMin;

	float m_ViewportYMax;

	// PURPOSE: The left side of the chart field in screen space.
	// NOTE: The chart "field" does not include the border area.
	float m_FieldLeft;

	// PURPOSE: The right side of the chart field in screen space.
	// NOTE: The chart "field" does not include the border area.
	float m_FieldRight;

	// PURPOSE: The top of the chart field in screen space.
	// NOTE: The chart "field" does not include the border area.
	float m_FieldTop;

	// PURPOSE: The bottom of the chart field in screen space.
	// NOTE: The chart "field" does not include the border area.
	float m_FieldBottom;

	// PURPOSE: The width in pixels of the chart's outer border.
	float m_BorderOuterWidth;

	// PURPOSE: The width in pixels of the chart's inner border.
	float m_BorderInnerWidth;
};


//=============================================================================
// Implementations

inline float pfChart::GetBorderWidth() const
{
	return m_BorderInnerWidth + m_BorderOuterWidth;
}

inline const grcViewport & pfChart::GetViewport() const
{
	FastAssert(m_ChartViewport);
	return *m_ChartViewport;
}


}	// namespace rage

#endif // __STATS

#endif // GRPROFILE_CHART_H

// EOF grprofile/chart.h
