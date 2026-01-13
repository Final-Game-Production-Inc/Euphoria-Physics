//
// grcore/bankio.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 
#if __BANK

#include "bankio.h"
#include "device.h"
#include "font.h"
#include "im.h"
#include "quads.h"
#include "setup.h"

#include "bank/bank.h"
#include "bank/bkmgr.h"
#include "bank/packet.h"
#include "input/input.h"
#include "string/string.h"
#include "system/param.h"
#include "system/threadregistry.h"

using namespace rage;

XPARAM(rag);
XPARAM(ragviewer);
XPARAM(remotebank);
XPARAM(powerconsole);

float bankDialScale = 1.0f;
grcSetup* grcBankIo::sm_pSetup = NULL;
float grcBankIo::sm_BankDrawScale = 1.5f;
const Color32 bankColor = Color32(210,210,210);

void grcBankIo::SetDrawScale(float s) 
{
	sm_BankDrawScale = s;
}


void grcBankIo::Draw(int x,int y,const char *str) const {
	/* if (PIPE.IsScreenshotInProgress())
		return; */
	const grcFont &f = grcFont::GetCurrent();
	float fx = float(sm_BaseX + sm_BankDrawScale * f.GetWidth() * x);
	float fy = float(sm_BaseY + sm_BankDrawScale * f.GetHeight() * y);
#if 0	// Drop shadow.  Still hard to read against pure white.  Also 8x overdraw...
	const Color32 shadowColor = Color32(0,0,0);
	for (float dy=-1; dy<=1; dy++)
		for (float dx=-1; dx<=1; dx++)
			if (dx||dy)
				f.DrawScaled(fx+dx,fy+dy,0,shadowColor,sm_BankDrawScale,sm_BankDrawScale,str);
#else
	const Color32 shadowColor = Color32(0,0,0,100);
	grcBindTexture(NULL);
	const float margin = 4.0f;
	grcDrawSingleQuadf(fx-margin,fy,fx+margin+StringLength(str)*sm_BankDrawScale*f.GetWidth(),fy+sm_BankDrawScale*f.GetHeight(),0.0f,
		0,0,0,0,shadowColor);
#endif
	f.DrawScaled(fx,fy,0,bankColor,sm_BankDrawScale,sm_BankDrawScale,str);
}



void grcBankIo::Input(int &pressed,int &down,float &dial) const {
	ioPad &P = ioPad::GetPad(sm_PadIndex);

	pressed = down = 0;
	if (P.GetPressedDebugButtons() & ioPad::LUP)
		pressed = UP_TO_BANK;
	else if (P.GetPressedDebugButtons() & ioPad::L1)
		pressed = TRIGGER;

	else if (P.GetDebugButtons() & ioPad::START)
		dial = ioAddDeadZone(P.GetNormLeftX(),0.4f) * bankDialScale;
#if HAVE_KEYBOARD
	else if (KEYBOARD.KeyPressed(KEY_M) && KEYBOARD.KeyDown(KEY_CONTROL))
		pressed = TRIGGER;
#endif
	else if ((P.GetPressedButtons() & ioPad::LUP)
#if HAVE_KEYBOARD
		|| KEYBOARD.KeyPressed(KEY_NUMPAD8)
#endif
		) pressed = UP;
	else if ((P.GetButtons() & ioPad::LUP)
#if HAVE_KEYBOARD
		|| KEYBOARD.KeyDown(KEY_NUMPAD8)
#endif
		) down = UP;
	else if ((P.GetPressedButtons() & ioPad::LDOWN)
#if HAVE_KEYBOARD
		|| KEYBOARD.KeyPressed(KEY_NUMPAD2)
#endif
		) pressed = DOWN;
	else if ((P.GetButtons() & ioPad::LDOWN)
#if HAVE_KEYBOARD
		|| KEYBOARD.KeyDown(KEY_NUMPAD2)
#endif
		) down = DOWN;
	else if ((P.GetPressedButtons() & ioPad::LLEFT)
#if HAVE_KEYBOARD
		|| KEYBOARD.KeyPressed(KEY_NUMPAD4)
#endif
		) pressed = LEFT;
	else if ((P.GetButtons() & ioPad::LLEFT)
#if HAVE_KEYBOARD
		|| KEYBOARD.KeyDown(KEY_NUMPAD4)
#endif
		) down = LEFT;
	else if ((P.GetPressedButtons() & ioPad::LRIGHT)
#if HAVE_KEYBOARD
		|| KEYBOARD.KeyPressed(KEY_NUMPAD6)
#endif
		) pressed = RIGHT;
	else if ((P.GetButtons() & ioPad::LRIGHT)
#if HAVE_KEYBOARD
		|| KEYBOARD.KeyDown(KEY_NUMPAD6)
#endif
		) down = RIGHT;
}


void grcBankIo::AddWidgets( grcSetup *pSetup )
{
	sm_pSetup = pSetup;

	bkIo::AddWidgets();

	if ( !bkRemotePacket::IsConnectedToRag() )
	{
		if ( sm_pBank == NULL )
		{
			sm_pBank = BANKMGR.FindBank( "rage - Bank" );
			if ( sm_pBank == NULL )
			{
				sm_pBank = &BANKMGR.CreateBank( "rage - Bank" );
			}
		}
		
		sm_pBank->AddButton( "Start Rag", datCallback( CFA(grcBankIo::ConnectToRag) ) );
	}
}

void grcBankIo::RemoveWidgets()
{
	bkIo::RemoveWidgets();
}

void grcBankIo::ConnectToRag()
{
	if ( !PARAM_rag.Get() && !PARAM_ragviewer.Get() && !PARAM_remotebank.Get() && (sm_pSetup != NULL) )
	{		
		static const char* ragValue = "true";
		PARAM_rag.Set( ragValue );

		// Don't create widgets while we're in the process of connecting to RAG (which will create a new thread,
		// which in turn would create a widget).
#if THREAD_REGISTRY
		sysThreadRegistry::EnableWidgetCreation(false);
#endif // THREAD_REGISTRY

		if ( sm_pSetup->ConnectToRag( bkManager::GetAppName() ) )
		{
#if __WIN32PC
			// allows us to keep updating the render window when Rag has focus
			GRCDEVICE.SetBlockOnLostFocus( false );
#endif
			// re-initialize for Rag input capture
			if ( INPUT.HasBegun() )
			{
				INPUT.End();
				INPUT.Begin( INPUT.IsInWindow() );
			}

			// re-create the widgets in Rag
			BANKMGR.Rebuild();
		}

#if THREAD_REGISTRY
		sysThreadRegistry::EnableWidgetCreation(true);
		sysThreadRegistry::AddDelayedWidgets();
#endif // THREAD_REGISTRY
	}
}

#endif	// __BANK
