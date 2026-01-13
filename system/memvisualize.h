
#ifndef SYSTEM_MEMVISUALIZE_H
#define SYSTEM_MEMVISUALIZE_H

#include "diag/tracker.h"

#if RAGE_TRACKING

namespace rage {

#define MEMVISUALIZE sysMemVisualize::GetInstance()

// Class to manage memvisualize runtime parameters
/*
	a - audio
	d - debug
	f - frag cache
	g - game heaps (e.g. virt, phys)
	h - header virtual
	m - misc	
	n - network heaps (e.g. rline, cxmgr, network, live mgr, etc.)
	p - platform specific (e.g. xex: pools, ps3: flex, MoVE, residual, etc.)
	r - res heaps (e.g. virt, phys, streaming)	
	s - streaming
	u - UI (e.g. scaleform)	
	v - Video Transcoding
	x - xtl (xenon only)
	y - replay
*/
class sysMemVisualize
{
public:
	enum {NONE = -1, AUDIO, DEBUG, FRAG, GAME, HEADER, MISC, NETWORK, PLATFORM, RESOURCE, STREAMING, UI, VTC, XTL, REPLAY, MAX};

private:
	size_t m_bits;

private:
	sysMemVisualize();
	bool Get(size_t pos) const;

public:	
	static sysMemVisualize& GetInstance();

	void Set(char ch);
	void Set(const char* psz);	
	void SetAll();

	bool HasAudio() const {return Get(AUDIO);}
	bool HasDebug() const {return Get(DEBUG);}
	bool HasFrag() const {return Get(FRAG);}
	bool HasGame() const {return Get(GAME);}
	bool HasHeader() const {return Get(HEADER);}
	bool HasMisc() const {return Get(MISC);}
	bool HasNetwork() const {return Get(NETWORK);}
	bool HasPlatform() const {return Get(PLATFORM);}
	bool HasResource() const {return Get(RESOURCE);}
	bool HasStreaming() const {return Get(STREAMING);}
	bool HasUI() const {return Get(UI);}

#if __XENON || RSG_DURANGO || RSG_PC
	bool HasVideoTranscoding() const {return Get(VTC);}
	bool HasXTL() const {return Get(XTL);}
#else
	bool HasVideoTranscoding() const {return false;}
	bool HasXTL() const {return false;}
#endif

#if RSG_DURANGO || RSG_ORBIS || RSG_PC
	bool HasReplay() const {return Get(REPLAY);}
#else
	bool HasReplay() const {return false;}
#endif
};
} // namespace rage

#endif // RAGE_TRACKING

#endif // SYSTEM_MEMDEBUG_H
