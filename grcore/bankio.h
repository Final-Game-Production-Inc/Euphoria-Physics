// 
// grcore/bankio.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef GRCORE_BANKIO_H
#define GRCORE_BANKIO_H

#if __BANK

#include "bank/io.h"

namespace rage {

class grcSetup;

/*
	The grcBankIo class implements the bkIo interface to supply local widget
	input and output.  It is created and used internally by the setup code.
*/
class grcBankIo: public bkIo {
public:
	// PURPOSE:	Draw a string at specified coordinate
	// PARAMS:	x - Column to draw (character cell column, not pixel column)
	//			y - Row to draw (character cell row, not pixel row)
	//			string - String to draw
	virtual void Draw(int x,int y,const char* string) const;

	// PURPOSE:	Check for input
	// PARAMS:	outPressed - Which debug button is pressed, if any (ie just became
	//				down this frame and was up previously)
	//			outDown - Which debug button is down, if any
	//			outDial - Dial value to spin sliders faster
	virtual void Input(int &outPressed,int &outDown,float &outDial) const;

	// PURPOSE:	Set global widget draw scale, to make widgets larger or
	//			smaller depending on your tv set.
	// PARAMS:	scale - scale to apply to widgets
	static void SetDrawScale(float scale);

	static void AddWidgets( grcSetup *pSetup );
	static void RemoveWidgets();

protected:
	static void ConnectToRag();

private:
	static grcSetup* sm_pSetup;
	static float sm_BankDrawScale;
};

}	// namespace rage
#endif	// __BANK

#endif	// GRCORE_BANKIO_H
