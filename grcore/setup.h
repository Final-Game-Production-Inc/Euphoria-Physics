//
// grcore/setup.h
//
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved.
//

#ifndef GRCORE_SETUP_H
#define GRCORE_SETUP_H

#include "vector/color32.h"
#include "system/timer.h"
#include "grcore/effect_mrt_config.h"
#include "grcore/gputimer.h"
#include "data/base.h"

#define SUPPORT_RENDERTARGET_DUMP	(__BANK && !MULTIPLE_RENDER_THREADS)

#define MOUSE_RENDERING_SUPPORT (!__WIN32PC && !__FINAL)

namespace rage {

class bkBank;
class bkConsole;
class bkImageViewer;
class grcFont;
class XmlLog;

//
// PURPOSE
//	The grcSetup class provides convenient startup and shutdown routines.
//	You're free to take full control over startup if you want but this
//	framework takes care of a lot of the details.
//
//	The basic framework will be:
//	<CODE>
//	int Main() {
//		grcSetup setup;
//		setup.Init(RAGEROOT "assets/grcore", "Tester Name");
//		setup.BeginGfx();
//		setup.CreateDefaultFactories();
//		do {
//			setup.BeginUpdate();
//				// do update-related stuff
//			setup.EndUpdate();
//
//			setup.BeginDraw();
//				// do rendering here
//			setup.EndDraw();
//		} while (!setup.WantExit());
//
//		setup.DestroyFactories();
//		setup.EndGfx();
//		setup.Shutdown();
//	}
//	</CODE>
//
//	Init and BeginGfx are distinct because Init sets
//	up the filesystem and you may want to read some data before you
//	fire up the graphics window.  CreateDefaultFactories is separate
//	from BeginGfx because you may be subclassing one or more 
//	grcore factories yourself and don't want the default versions.
// <FLAG Component>
//
class grcSetup : public datBase {
public:
	grcSetup();
	virtual ~grcSetup();

	// PURPOSE:	Initialize the filesystem.
	// PARAMS:	assetPath - Root asset path.  Many testers use
	//				RAGEROOT here, which is defined in data/main.h.
	//			appName - Application name.  Used as the window title
	//				on pc builds.
	void Init(const char *assetPath,const char *appName);

	// PURPOSE:	Initialize the graphics system.
	// PARAMS: inWindow - Whether rage should run in a window (PC BUILDS ONLY)
	//		   topMost - Whether rage should create a window with a high z value  (PC BUILDS ONLY)
	void BeginGfx(bool inWindow, bool topMost=false);

	// PURPOSE:	Gets binary font data of internal system font.
	static u8* GetInternalFontBits();

	// PURPOSE:	Creates default texture, model, and shader factories.
	virtual void CreateDefaultFactories();

	// PURPOSE: Mark the beginning of scene update.  Also runs the Win32
	//			message pump, so make sure you call this once per frame.
	//			Note that there is no current grcViewport during update.
	virtual void BeginUpdate();

	// PURPOSE: Mark the ending of scene update.
	virtual void EndUpdate(float delta = 0.0f);

	// PURPOSE: Mark the beginning and ends of scene update waits (gpu and render).
	virtual void BeginUpdateWait();
	virtual void EndUpdateWait();

	// PURPOSE:	Gets the screen clear color used by BeginDraw
	// PARAMS:	none
	Color32 GetClearColor();

	// PURPOSE:	Sets the screen clear color used by BeginDraw
	// PARAMS:	color - Screen clear color
	void SetClearColor(Color32 color);

	// PURPOSE:	Marks beginning of scene render.  The first thing you
	//			should do after calling this is make an grcViewport current
	//			if you want to do any rendering.
	// PARAMS:	clearScreen - If true, clear the graphics screen.  Also
	//				clears depth and stencil; if you don't want that,
	//				pass in false and do it yourself.
	virtual bool BeginDraw(bool clearScreen = true);

	// PURPOSE: Ends the scene render.  Schedules a page flip.
	virtual void EndDraw();

	// PURPOSE:	Returns true if user wants to exit.  Currently this can
	//			only return true if the user clicked the close button on
	//			the window in a PC build, or hit Alt+F4.
	bool WantExit();

	// PURPOSE: Destroys all current texture, model, and shader factories.
	//			Even if you created the factories yourself, you can still
	//			use this function to clean everything up.
	virtual void DestroyFactories();

	// PURPOSE:	Terminates the graphics system (closes the window on PC).
	void EndGfx();

	// PURPOSE: Shuts down the setup API, cleans up filesystems.  Should
	//			be the last thing you do before exit.
	void Shutdown();

#if !__FINAL
	// RETURNS:	Last update task time (UpT), in milliseconds
	float GetUpdateTaskTime();

	// RETURNS:	Last draw task time (DrT), in milliseconds
	float GetDrawTaskTime();
#endif // !__FINAL

	// RETURNS:	Last update time
	float GetUpdateTime() const { return m_UpdateTime; }

	// RETURNS:	Last draw time, in milliseconds
	float GetDrawTime() const { return m_DrawTime; }

	// RETURNS: Last total time (time between BeginUpdate calls), in milliseconds
	float GetTotalTime() const { return m_TotalTime; }

	// RETURNS: Last GPU render time, in milliseconds
	float GetGpuTime() const { return m_GpuTime; }

	// RETURNS: Last fifo stall time, in milliseconds
#if __PPU
	float GetFifoStallTime() const { return m_FifoStallTime; }
#endif // __PPU

	// PURPOSE: Render the safe zone for the current platform
	void DrawSafeZone() const;

	// PURPOSE: Set or disable the frametime display
	void SetShowFrameTime(bool showFrameTime)	{ m_ShowFrameTime = showFrameTime; }

	// RETURNS: TRUE if the frametime display is currently enabled.
	bool GetShowFrameTime() const				{ return m_ShowFrameTime; }

	// PURPOSE: Set or disable the frametime display
	void SetShowFrameTicker(bool showFrameTicker)	{ m_ShowFrameTicker = showFrameTicker; }

	// RETURNS: TRUE if the frametime display is currently enabled.
	bool GetShowFrameTicker() const				{ return m_ShowFrameTicker; }

	enum EScreenshotNamingConvention
	{
		PROMPT_FOR_EACH_SCREENSHOT,
		NUMBERED_SCREENSHOTS,
		OVERWRITE_SCREENSHOT,
		NUM_OPTIONS
	};

#if !__FINAL
	// PURPOSE: Accessor to enable/disable the safe zone draw
	void EnableSafeZoneDraw(bool b=true, float size=0.85f);

	// PURPOSE: Specify bug information used by next AddBug call.
	// PARAMS:	x - X position of bug
	//			y - Y position of bug
	//			z - Z position of bug
	//			grid - Grid cell name containing bug
	//			owner - Owner of the bug
	void SetBugInfo(float x,float y,float z,const char *grid,const char *owner);

	// PURPOSE: Causes a bug screenshot to be added at the end of the next frame.
	// NOTES:	Use SetBugInfo to initialize other data.
	void AddBug();

	// PURPOSE: Causes a screenshot of .png format, or sequence of screenshots to be saved at the end of the next frame.
	void TakeScreenShotPNG();

	// PURPOSE: Causes a screenshot of .jpg format, or sequence of screenshots to be saved at the end of the next frame.
	void TakeScreenShotJPG();

	void TakeScreenShot( const char* name, bool isJpg = true );

#if SUPPORT_RENDERTARGET_DUMP
	void TakeRenderTargetShots( const char *name = NULL );
	void TakeRenderTargetShotsNow();
	void UntakeRenderTargetShots();
	bool ShouldCaptureRenderTarget( const char *name = NULL );
#endif //SUPPORT_RENDERTARGET_DUMP
	
	// PURPOSE: If a sequence of screenshots is in progress, stop them.
	void StopScreenShots();

	// PURPOSE: Check if we're taking a screenshot so we adjust rendering settings
	bool IsTakingScreenshot() { return (m_ScreenshotFrameDelay > 0); }

	// PURPOSE: Check if we're intending to take a screenshot
	bool ShouldTakeScreenshot() { return m_TakeScreenshot; }

	// PURPOSE: set the pointer/mouse cursor to not be drawn.  Useful for screen shots
	void SetMousePointerDisabled(bool disabled)	{ m_DisableMousePointerDraw = disabled; }

	// PURPOSE: has the pointer/mouse cursor been set not to draw.  Useful for screen shots
	bool IsMousePointerDisabled() const			{ return m_DisableMousePointerDraw; }

	// PURPOSE: Set if screen shots should be numbered.
	void NumberScreenShots(bool /*number*/) { AssertMsg( 0 , "deprecated.  Use SetScreenShotNamingConvention()." ); }

	// PURPOSE: Set how the screen shots should be named (numbered, overwrite, or prompt for name)
	void SetScreenShotNamingConvention( EScreenshotNamingConvention convention ) { m_ScreenshotNamingConvention = convention; }

	// PURPOSE: Set the screenshot name.
	void SetScreenShotName( const char* name ) { safecpy(m_ScreenshotName, name); }

	// PURPOSE: Stores to the xml log details about the current frame
	void TakeSnapShot( XmlLog& log ) const;
#endif

	void SetScreenShotGamma(float gamma) { m_ScreenshotGamma = gamma; }
	float GetScreenShotGamma() const { return m_ScreenshotGamma; }

#if __BANK
	bool ConnectToRag( const char *appName );

	// PURPOSE: Add user tunable widgets
	virtual void AddWidgets(bkBank &bk);
	void AddScreenShotViewerWidgets(bkBank &bk);
#endif

#if !__FINAL
	bool IsLocalConsoleActive() const { return m_InputEnabled; }
#else
	bool IsLocalConsoleActive() const { return false; }
#endif

	// PURPOSE: Returns true if the user has paused the game.
	static bool IsPaused();

	// PURPOSE: Set the paused state.
	static void SetPaused(bool pause);

	// PURPOSE: Get the built-in fixed width font.
	static grcFont *GetFixedWidthFont();

#if !__FINAL
	// PURPOSE: Get the built-in proportional font.
	static grcFont *GetProportionalFont();

	// PURPOSE: Get the built-in 6x8 fixed width font.
	static grcFont *GetMiniFixedWidthFont();

	// PURPOSE: resets the ticker after non consistent thread update, like flushes
	static void ResetTicker();

    // PURPOSE: register the number of buffer vblanks we've missed this frame
	static void SetFlipWaits(u32 waits);
#endif // !__FINAL

private:
	Color32 m_ClearColor;
	sysTimer m_UpdateTimer, m_WaitTimer, m_DrawTimer, m_TotalTimer;
	float m_UpdateTime, m_WaitTime, m_DrawTime, m_TotalTime, m_GpuTime, m_GpuIdleTime;
	bool m_AddBug;
	bool m_TakeScreenshotJPG;
	bool m_TakeScreenshot;
	u32  m_ScreenshotFrameDelay;
	int m_ScreenshotNamingConvention;
	bool m_UseScreenShotFileDialogue;
	bool m_DisableMousePointerDraw;
	bool m_DisableMousePointerDrawDuringScreenshot;
	bool m_RedDotSight;
	int m_ScreenshotsInARow;	
	int m_ScreenshotsLeft;
	sysTimer m_ScreenshotTimer;
	float m_ScreenshotIntervalTime;
	float m_ScreenshotLastTakenTime;
	float m_ScreenshotImageViewerLastUpdatedTime;
	float m_ScreenshotGamma;
#if __BANK
	bool m_needToDisplayScreenshotInRagApplicationWindow;
	bool m_DisplayScreenshotInRagApplicationWindow;

	u32 m_FrameToCapture;
	const char *m_DebugRenderTargetName;
#endif
	char m_ScreenshotName[256];
	bool m_ShowFrameTime, m_ShowFrameTicker;
	float m_BugX, m_BugY, m_BugZ;
	char m_BugGrid[64], m_BugOwner[64];
	u8 m_ClearDuringResolve;
#if !__FINAL
	bool m_DrawSafeZone;
	bool m_InputEnabled;
	char m_Input[128];
	int m_InputLength;
#endif
#if __BANK
	bkImageViewer* m_ImageViewer;
	float m_LastSendTime;
	void DrawColorBars();

	// PURPOSE: Sets appropriate Widget values to start this operation.
	void StartScreenshotsInRagApplicationWindow();
	
	enum EnumColorBars {COLORBARS_NONE,COLORBARS_75,COLORBARS_100,COLORBARS_GREY};
	EnumColorBars m_DrawColorBars;
#endif

	grcGpuTimer m_FrameTimer; 

#if MOUSE_RENDERING_SUPPORT
	u32 m_UnmovedMouseDrawCount;
	u32 m_MaxUnmovedMouseDrawCount;
	int m_PreviousMouseX;
	int m_PreviousMouseY;
#endif

#if __PPU
public:
	static void InitSysUtilCallback();

private:
	static void _SysutilCallback(u64 status, u64 param, void * userdata);

	u32 m_LastFifoStallTime, m_LastFragmentStallTime, m_LastJobTime, m_LastDmaTime;
	float m_FifoStallTime, m_FragmentStallTime, m_JobTime, m_DmaTime;
#endif

#if MOUSE_RENDERING_SUPPORT
public:
	static int m_DisableMousePointerRefCount; // let external systems disable the mouse pointer in a semi-elegant way
#if __BANK
	static void DisableMouseCursorBegin();
	static void DisableMouseCursorEnd();
#endif
#endif
};

extern grcSetup* grcSetupInstance;

}	// namespace rage

#endif
