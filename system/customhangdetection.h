//
// system/customhangdetection.h
//
// Copyright (C) 2014-2014 Rockstar Games.  All Rights Reserved.
//

#ifndef SYSTEM_CUSTOMHANGDETECTION_H
#define SYSTEM_CUSTOMHANGDETECTION_H

#include "hangdetect.h"
#include "timer.h"

namespace rage {

#define CUSTOM_HANG_DETECTION_DEFAULT_MIN_POLL_INTERVAL_SEC      (0.01f)

class CustomHangDetection {
public:

	inline CustomHangDetection(const char *name, float timeoutSec, float minPollIntervalSec=CUSTOM_HANG_DETECTION_DEFAULT_MIN_POLL_INTERVAL_SEC) {
#		if HANG_DETECT_THREAD
			HANG_DETECT_SAVEZONE_ENTER();
			m_prevTimer             = 0.f;
			m_name                  = name;
			m_timeoutSec            = timeoutSec;
			m_minPollIntervalSec    = minPollIntervalSec;
#		else
			(void)name;
			(void)timeoutSec;
			(void)minPollIntervalSec;
#		endif
	}

	inline ~CustomHangDetection() {
#		if HANG_DETECT_THREAD
			HANG_DETECT_SAVEZONE_EXIT(m_name);
#		endif
	}

	inline bool pollHasTimedOut() {
#		if HANG_DETECT_THREAD
			// If we have not been polled frequently enough, assume we must have
			// been paused in a debugger, so reset timer to prevent it from
			// timing out.
			const float t = m_timer.GetTime();
			if (t - m_prevTimer > m_minPollIntervalSec) {
				m_timer.Reset();
				m_prevTimer = 0.f;
			}
			else if (t > m_timeoutSec) {
				m_timer.Reset();
				return true;
			}
			else
				m_prevTimer = t;
#		endif
		return false;
	}

private:

#	if HANG_DETECT_THREAD
		sysTimer    m_timer;
		float       m_prevTimer;
		const char *m_name;
		float       m_timeoutSec;
		float       m_minPollIntervalSec;
#	endif
};

} // namespace rage

#endif // SYSTEM_CUSTOMHANGDETECTION_H
