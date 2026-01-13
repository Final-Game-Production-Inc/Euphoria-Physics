//
// grcore/monitor.h
//
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved.
//

#ifndef GRCORE_MONITOR_H
#define GRCORE_MONITOR_H

#define SUPPORT_MULTI_MONITOR	(RSG_PC)
#if SUPPORT_MULTI_MONITOR
#	define MULTI_MONITOR_ONLY(...)	__VA_ARGS__
#else
#	define MULTI_MONITOR_ONLY(...)
#endif

#include "../../../framework/src/fwmaths/Rect.h"

struct ADLMode;


namespace rage	{
	class Vector2;

	struct GridMonitor	{
		u32 uGridPosX, uGridPosY;
		bool bLandscape;
		float fPhysicalAspect;
		float fActualPhysicalAspect;
		u32 uLeft, uRight, uTop, uBottom;

		GridMonitor(): uGridPosX(0), uGridPosY(0),
			bLandscape(true), fPhysicalAspect(1.f),
			uLeft(0), uRight(1), uTop(0), uBottom(1)
		{}

		bool hasArea() const	{ return uRight>uLeft && uBottom>uTop; }
		u32 getWidth() const	{ return uRight - uLeft; }
		u32 getHeight() const	{ return uBottom - uTop; }
		fwRect getArea() const;

		float getLogicalAspect() const	{ return getWidth() / static_cast<float>(getHeight()); }
		float getPixelAspect() const	{ return fPhysicalAspect / getLogicalAspect(); }
	};

	
	enum MonitorComposition	{
		MC_FLAT,	// all monitors are parallel (preferred)
		MC_ROUND,	// distance to all monitors is the same
		MC_INVALID
	};

	class MonitorConfiguration	{
	public:
		struct Mode	{
			unsigned width, height;
			Mode(): width(0),height(0)	{}
			Mode(unsigned w, unsigned h): width(w),height(h)	{}
			Mode(const ADLMode& amode);
			bool operator==(const Mode& m) const	{ return width==m.width && height==m.height; }
		};
	private:
		float	m_fForcedGlobalAspect;
		bool	m_bDirty;
#		if SUPPORT_MULTI_MONITOR
		GridMonitor *m_Monitors;
		GridMonitor const *m_Preferred;
		bool m_bEmulated, m_bBezelCompensated;
		MonitorComposition m_Composition;
	
		Mode *m_Modes;
		int m_TotalNativeModes, m_TotalBezelModes;

		int findModeIdHelper(int startId, int total, const Mode &mode) const;
		int findNativeModeId(const Mode &mode) const	{ return findModeIdHelper(0,m_TotalNativeModes,mode); }
		int findBezelModeId	(const Mode &mode) const	{ return findModeIdHelper(m_TotalNativeModes,m_TotalBezelModes,mode); }

		const GridMonitor* findPreferredMonitor() const;
		bool queryConfigAMD(int iTargetAdapter);
		bool queryConfigNvidia(int iTargetAdapter);
#		endif	//SUPPORT_MULTI_MONITOR

	public:
		GridMonitor m_WholeScreen;
#		if SUPPORT_MULTI_MONITOR
		bool m_bMerged;
		u32 m_uGridSizeX, m_uGridSizeY;
		u32 m_ChangeTimestamp;
#		endif	//SUPPORT_MULTI_MONITOR

		MonitorConfiguration();
		~MonitorConfiguration();

		static float readAspectParameter();
		// force a global aspect ratio. Call with 0 to unset.
		void setGlobalAspect(float fAspect);

		void markDirty()	{ m_bDirty = true; }
		const MonitorConfiguration& update();

#		if SUPPORT_MULTI_MONITOR
		void allocateMonitors();
		void releaseMonitors();
		void emulateGrid(unsigned gridX, unsigned gridY);
		void setHUDLayout(unsigned int uX, unsigned int uY);

		unsigned getMonitorCount() const	{ return m_uGridSizeX * m_uGridSizeY; }
		bool isMultihead() const;
		bool isNativeResolution() const;

		bool isModeEyefinity		(const Mode &mode) const	{ return findNativeModeId(mode)>=0;	}
		bool isModeBezelCompensated	(const Mode &mode) const	{ return findBezelModeId(mode)>=0;	}

		// returns one of the monitors by index. -1 stands for the whole screen.
		const GridMonitor* getMonitorPtr(int id) const;
		// returns a reference to either a central monitor or the whole screen.
		const GridMonitor& getCentralMonitor() const;
		// returns a reference to either a landscape central monitor or the whole screen.
		const GridMonitor& getLandscapeMonitor() const;

		bool queryConfiguration();
		// derive vertical from horisontal FOV
		float computeVerticalFOV(float fovX) const;
		// transform to multi-head FOV
		float transformFOV(float fovY) const;
#		endif	//SUPPORT_MULTI_MONITOR

#		if __BANK
		void addWidgets(bkBank &bank);
#		endif
	};

}	//rage
#endif	//GRCORE_MONITOR_H