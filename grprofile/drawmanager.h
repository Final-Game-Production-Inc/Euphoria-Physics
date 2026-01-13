//
// grprofile/drawmanager.h
//
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved.
//

#ifndef GRPROFILE_DRAWMANAGER_H
#define GRPROFILE_DRAWMANAGER_H

//==============================================================
//
// The classes in these files form a system for queuing graphical
// annotations at any point in a game loop for later drawing.
// 

#include "drawcore.h"

#if __PFDRAW

#include "grprofile/drawgroup.h"
#include "system/criticalsection.h"

namespace rage {

class sysNamedPipe;

//=============================================================================
// The RAGE default static pfDrawManager.

pfDrawManager & GetRageProfileDraw();


//=============================================================================
// Macro interface for easy creation of items and groups.

#define PFD_DECLARE_GROUP(name)				::rage::pfDrawGroup PFDGROUP_##name(#name,&::rage::GetRageProfileDraw(),&::rage::GetRageProfileDraw())
#define PFD_DECLARE_GROUP_ON(name)			::rage::pfDrawGroup PFDGROUP_##name(#name,&::rage::GetRageProfileDraw(),&::rage::GetRageProfileDraw(),true)
#define PFD_DECLARE_SUBGROUP(name,group)	::rage::pfDrawGroup PFDGROUP_##name(#name,&PFDGROUP_##group,&::rage::GetRageProfileDraw())
#define PFD_DECLARE_SUBGROUP_ON(name,group)	::rage::pfDrawGroup PFDGROUP_##name(#name,&PFDGROUP_##group,&::rage::GetRageProfileDraw(),true)

#define PFD_DECLARE_ITEM(name,color,group)						::rage::pfDrawItem PFD_##name(#name,color,1,&PFDGROUP_##group,&::rage::GetRageProfileDraw())
#define PFD_DECLARE_ITEM_ON(name,color,group)					::rage::pfDrawItem PFD_##name(#name,color,1,&PFDGROUP_##group,&::rage::GetRageProfileDraw(),true)
#define PFD_DECLARE_ITEM_TOGGLE(name,color,group,initialValue)	::rage::pfDrawItem PFD_##name(#name,color,1,&PFDGROUP_##group,&::rage::GetRageProfileDraw(),initialValue)

#define PFD_DECLARE_ITEM_SLIDER(name,color,group,initialValue,maxValue,increment) \
	::rage::pfDrawItemSlider PFD_##name(#name,color,1,&PFDGROUP_##group,&::rage::GetRageProfileDraw(),initialValue,maxValue,increment)
#define PFD_DECLARE_ITEM_SLIDER_FULL(name,color,group,initialValue,minValue,maxValue,increment) \
	::rage::pfDrawItemSliderFull PFD_##name(#name,color,1,&PFDGROUP_##group,&::rage::GetRageProfileDraw(),initialValue,minValue,maxValue,increment)
#define PFD_DECLARE_ITEM_SLIDER_INT(name,color,group,initialValue,maxValue,increment) \
	::rage::pfDrawItemSliderInt PFD_##name(#name,color,1,&PFDGROUP_##group,&::rage::GetRageProfileDraw(),initialValue,maxValue,increment)
#define PFD_DECLARE_ITEM_SLIDER_INT_FULL(name,color,group,initialValue,minValue,maxValue,increment) \
	::rage::pfDrawItemSliderIntFull PFD_##name(#name,color,1,&PFDGROUP_##group,&::rage::GetRageProfileDraw(),initialValue,minValue,maxValue,increment)
#define PFD_DECLARE_ITEM_COLOR(name,group,initialColor) \
	::rage::pfDrawItemColor PFD_##name(#name,Color32(0,0,0),1,&PFDGROUP_##group,&::rage::GetRageProfileDraw(),initialColor)
#define PFD_DECLARE_ITEM_BUTTON(name,group,callback,ptr,tooltip) \
	::rage::pfDrawItemButton PFD_##name(#name,Color32(0,0,0),1,&PFDGROUP_##group,&::rage::GetRageProfileDraw(),datCallback(MFA(callback),ptr),tooltip)
#define PFD_DECLARE_ITEM_BUTTON_STATIC(name,group,callback,tooltip) \
	::rage::pfDrawItemButton PFD_##name(#name,Color32(0,0,0),1,&PFDGROUP_##group,&::rage::GetRageProfileDraw(),datCallback(CFA(callback)),tooltip)

#define EXT_PFD_DECLARE_GROUP(name)					extern ::rage::pfDrawGroup				PFDGROUP_##name
#define EXT_PFD_DECLARE_ITEM(name)					extern ::rage::pfDrawItem				PFD_##name
#define EXT_PFD_DECLARE_ITEM_SLIDER(name)			extern ::rage::pfDrawItemSlider			PFD_##name
#define EXT_PFD_DECLARE_ITEM_SLIDER_FULL(name)		extern ::rage::pfDrawItemSliderFull		PFD_##name
#define EXT_PFD_DECLARE_ITEM_SLIDER_INT(name)		extern ::rage::pfDrawItemSliderInt		PFD_##name
#define EXT_PFD_DECLARE_ITEM_SLIDER_INT_FULL(name)	extern ::rage::pfDrawItemSliderIntFull	PFD_##name
#define EXT_PFD_DECLARE_ITEM_COLOR(name)			extern ::rage::pfDrawItemColor			PFD_##name
#define EXT_PFD_DECLARE_ITEM_BUTTON(name)			extern ::rage::pfDrawItemButton			PFD_##name


//=============================================================================
// Macro interface enabling items and groups.

#define PFD_GROUP_ENABLE(name,enable)				PFDGROUP_##name.SetEnabled(enable)
#define PFD_ITEM_ENABLE(name,enable)				PFD_##name.SetEnabled(enable)
#define PFD_ITEM_SLIDER_SET_VALUE(name,value)		PFD_##name.SetValue(value)


//=============================================================================
// PURPOSE
//   This class is the root of the tree of groups and items in the draw system.
//   It is responsible for allocating buffer space to capture drawing commands.
//   Additionally this manager performs the drawing of buffered data.
//
// <FLAG Component>
//
class pfDrawManager : public pfDrawGroup
{
public:
	pfDrawManager(const char * name);

	~pfDrawManager();

	void Init(int bufferSize, bool createWidgets=true, grcBatcher::BufferFullMode fullMode = grcBatcher::BUF_FULL_ONSCREEN_WARNING);

	void Shutdown();

	bool Begin();

	void End();

	void Render(bool flush = true);

	void Flush();

	static void SetDrawImmediately(bool drawImmediately)
	{
		sm_DrawImmediately = drawImmediately;
	}

	//============================================================================
	// Draw client / server functions.


	void SendDrawCameraForServer(const Matrix34 & matrix);

#if __BANK
	virtual void AddWidgets(bkBank & bank);
#endif

	const Matrix34& GetDrawCamera() {return m_LastDrawCamera;}

protected:
	bool m_Initialized;

	grcBatcher m_Batcher;

	int m_BeginDepth;

	grcBatcher * m_PushedBatcher;

    sysNamedPipe * m_DrawPipe;

	mutable sysCriticalSectionToken m_CriticalSection;

	Matrix34 m_LastDrawCamera;

	static __THREAD bool sm_DrawImmediately;

#if __BANK
	bkBank * m_Bank;
#endif
};


inline bool pfDrawManager::Begin()
{
	if(m_Initialized && !sm_DrawImmediately)
	{
		m_CriticalSection.Lock();

		if (m_BeginDepth == 0)
		{
			m_PushedBatcher = grcBatcher::GetCurrent();
		}

		++m_BeginDepth;
		grcBatcher::SetCurrent(&m_Batcher);
	}
	return m_Initialized;
}


inline void pfDrawManager::End()
{	
	if (!sm_DrawImmediately)
	{
		FastAssert(m_BeginDepth > 0);
		--m_BeginDepth;

		if (m_BeginDepth == 0)
		{
			grcBatcher::SetCurrent(m_PushedBatcher);	
			m_PushedBatcher = NULL;
		}

		m_CriticalSection.Unlock();
	}
}

} // namespace rage

#else // !__PFDRAW

namespace rage {

//=============================================================================
// Empty macro implementations if !__PFDRAW

#define PFD_DECLARE_GROUP(name)
#define PFD_DECLARE_GROUP_ON(name)
#define PFD_DECLARE_SUBGROUP(name,group)
#define PFD_DECLARE_SUBGROUP_ON(name,group)
#define PFD_DECLARE_ITEM(name,color,group)
#define PFD_DECLARE_ITEM_ON(name,color,group)
#define PFD_DECLARE_ITEM_TOGGLE(name,color,group,initialValue)
#define PFD_DECLARE_ITEM_SLIDER(name,color,group,initialValue,maxValue,increment)
#define PFD_DECLARE_ITEM_SLIDER_FULL(name,color,group,initialValue,minValue,maxValue,increment)
#define PFD_DECLARE_ITEM_SLIDER_INT(name,color,group,initialValue,maxValue,increment)
#define PFD_DECLARE_ITEM_SLIDER_INT_FULL(name,color,group,initialValue,minValue,maxValue,increment)
#define PFD_DECLARE_ITEM_COLOR(name,group,initialColor)
#define PFD_DECLARE_ITEM_BUTTON(name,group,callback,ptr,tooltip)
#define PFD_DECLARE_ITEM_BUTTON_STATIC(name,group,callback,tooltip)
#define EXT_PFD_DECLARE_ITEM(name)
#define EXT_PFD_DECLARE_GROUP(name)
#define EXT_PFD_DECLARE_ITEM_SLIDER(name)
#define EXT_PFD_DECLARE_ITEM_SLIDER_INT(name)
#define EXT_PFD_DECLARE_ITEM_SLIDER_FULL(name)
#define EXT_PFD_DECLARE_ITEM_SLIDER_INT_FULL(name)
#define EXT_PFD_DECLARE_ITEM_COLOR(name)
#define EXT_PFD_DECLARE_ITEM_BUTTON(name)

#define PFD_GROUP_ENABLE(name,enable)
#define PFD_ITEM_ENABLE(name,enable)
#define PFD_ITEM_SLIDER_SET_VALUE(name,value)

}	// namespace rage

#endif // __PFDRAW

#endif // GRPROFILE_DRAWMANAGER_H

// <eof> grprofile/drawmanager.h
