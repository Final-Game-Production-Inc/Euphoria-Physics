// 
// grprofile/trail.h 
// 
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved. 
// 

#ifndef GRPROFILE_TRAIL_H 
#define GRPROFILE_TRAIL_H 

#include "atl/queue.h"
#include "diag/stats.h"
#include "system/criticalsection.h"
#include "vectormath/classes.h"
#include "vector/color32.h"

namespace rage {

	class pfTrailVisualizer;
	class bkBank;
	class grcViewport;


	// PURPOSE: pfTrail holds a trail of sample points in a circular queue. 
	// It is an abstract class, clients that want to draw a trail of something need to 
	// derive from it and define the SampleKeyframe function. That function should write any 
	// relevant data to the KeyframeEntry object it receives.
	class pfTrail
	{
	public:
		// Creates a new pfTrail. No memory is allocated for the queue until the first Update call.
		pfTrail();

		virtual ~pfTrail();

#if __STATS
		// PURPOSE: Updates the trail. Call this every frame you are doing trail recording.
		// NOTES: If the trail is active this function may call the virtual functions
		// ShouldRecord (to determine whether or not to record anything), 
		// SampleKeyframe (to gather keyframe info), and 
		// ShouldAddKeyframe (to determine whether or not this keyframe is 'interesting' enough to go in the queue)
		void Update();

		// RETURNS: True if this trail is active - an inactive trail will not record or draw.
		bool IsActive() const
		{	return m_Active;	}

		// PURPOSE: Sets the active state for this trail. 
		void SetActive(bool b)
		{	m_Active = b;	}

		// PURPOSE: Resets the trail, erasing all the recorded keyframes
		void Reset();
#else
		inline void Update() {}
		bool IsActive() {return false;}
		void SetActive(bool) {}
		inline void Reset() {}
#endif

		// PURPOSE: Sets the default color for each trail point. Colors can be overridden
		// in the SampleKeyframe function.
		inline void SetColor(Color32 col);

		// PURPOSE: Sets the default label for each trail point. Labels may be overridden in
		// the SampleKeyframe function.
		// NOTES: No memory is copied here, so the label should either be a static string
		// or have some persistent space allocated for it.
		inline void SetLabel(const char* label);

#if __BANK
		void AddWidgets(bkBank &bank);
#endif

		struct KeyframeEntry
		{
			void SetPos(Vec3V_In pos) {m_PosAndFloat1.SetXYZ(pos);}
			Vec3V_Out GetPos() const {return m_PosAndFloat1.GetXYZ();}

			void SetDir(Vec3V_In dir) {m_DirAndFloat2.SetXYZ(dir);}
			Vec3V_Out GetDir() const {return m_DirAndFloat2.GetXYZ();}

			Vec4V	m_PosAndFloat1;
			Vec4V	m_DirAndFloat2;
			float				m_Time;
			Color32				m_Color;
			const char*			m_Label;
			int					m_ExtraInt;
		};

		static const int kMaxEntries = 1024;
		typedef atQueue<KeyframeEntry, kMaxEntries> KeyframeQueue;

	protected:
		friend class pfTrailVisualizer;

		// PURPOSE: Record a single keyframe. Subclasses can fill in any of the values
		// of KeyframeEntry that make sense. m_Time, m_Color, and m_Label are already
		// initialized but can be modified. m_Time should continually increase from sample
		// to sample.
		virtual void SampleKeyframe(KeyframeEntry &entryOut) const = 0;

		// PURPOSE: Determine whether or not this keyframe is "interesting" enough to record.
		// NOTES: Subclasses may use this to omit some keyframes based on the previous values, for
		// example if the new value hadn't moved far enough from the old one.
		virtual bool ShouldAddKeyframe(const KeyframeEntry &oldEntry, const KeyframeEntry &newEntry) const;

		// PURPOSE: Determines whether or not recording should be done. 
		// NOTES: Subclasses may want to check the game state, or a 'recording' flag to determine
		// whether or not to record info.
		virtual bool ShouldRecord() const;

		void ResetKeyframe(KeyframeEntry& kf);

#if __STATS

		mutable sysCriticalSectionToken	m_Mutex;

		pfTrail*						m_Parent;

		KeyframeQueue*					m_KeyframeQueue;
		int								m_OverPushes; // how many times has a push removed an element from the queue
		Color32							m_Color;
		const char*						m_Label;

		bool							m_Active;
		bool							m_WasActive;
#endif

	};

	// PURPOSE: Visualizes a pfTrail. You can set the interval of the trail you're interested in
	// and the interval that you want to see samples drawn in
	// NOTES: Subclasses should override DrawOverTime to draw a trail connecting each sample, and
	// DrawSample to draw an individual sample point in higher detail
	class pfTrailVisualizer
	{
	public:
		pfTrailVisualizer();

		virtual ~pfTrailVisualizer(){}

		// PURPOSE: Draws the trail
#if __STATS
		void Draw(const pfTrail& trail) const;
#else
		inline void Draw(const pfTrail&) const {};
#endif

		// PURPOSE: Sets the sample drawing interval to draw once every N frames
		void SetFrameSpacing(int frames);

		// PURPOSE: Sets the sample drawing interval to draw once every N seconds
		void SetTimeSpacing(float time);
	protected:

		// PURPOSE: Subclasses should override this to draw a continuous path between queue[first] and queue[last]
		virtual void DrawOverTime(const pfTrail::KeyframeQueue& queue, int first, int last) const = 0;

		// PURPOSE: Subclasses should override this to draw a detailed representation of the keyframe 
		virtual void DrawSample(const pfTrail::KeyframeEntry& sample) const = 0;

		// PURPOSE: Lerps two samples, generally subclasses won't need to override this but might have to if
		// they are interpreting the keyframe variables in a special way
		virtual void LerpSamples(float t, const pfTrail::KeyframeEntry& a, const pfTrail::KeyframeEntry& b, pfTrail::KeyframeEntry& out) const;

#if __STATS
		float m_MinTime;
		float m_MaxTime;

		float m_SampleSpacing;
		enum SampleUnits
		{
			FRAMES_PER_SAMPLE,
			SECONDS_PER_SAMPLE,
			SINGLE_SAMPLE,
			NUM_SAMPLE_UNITS,
		};
		u8 m_SampleUnits;
#endif
	};

	// PURPOSE: This visualizer draws the trail as a connected line and draws the sample points
	// as small spheres.
	class pfTrailVisualizerSimple : public pfTrailVisualizer
	{
	protected:
		virtual void DrawOverTime(const pfTrail::KeyframeQueue& queue, int first, int last) const;
		virtual void DrawSample(const pfTrail::KeyframeEntry& sample) const;
	};

	// PURPOSE: This visualizer draws the trail as a connected line and draws the sample points
	// as very small spheres and directional vectors
	class pfTrailVisualizerPosAndDir : public pfTrailVisualizerSimple
	{
	protected:
		virtual void DrawSample(const pfTrail::KeyframeEntry& sample) const;
	};

	// PURPOSE: This visualizer draws the trail as a connected line and draws the sample points
	// as camera frustums
	class pfTrailVisualizerCamera : public pfTrailVisualizerSimple
	{
	public:
		pfTrailVisualizerCamera();

#if __BANK
		void AddWidgets(bkBank &bank);
#endif

		void SetDebugDrawViewport(const grcViewport* vp) { m_Viewport = vp; }

		bool m_DrawCameraIcon;
		int  m_TimeTickSpacing;

	protected:
		const grcViewport* m_Viewport;

		virtual void DrawOverTime(const pfTrail::KeyframeQueue& queue, int first, int last) const;
		virtual void DrawSample(const pfTrail::KeyframeEntry& sample) const;
	};


	void pfTrail::SetColor( Color32 STATS_ONLY(col) )
	{
#if __STATS
		m_Color = col;
#endif
	}

	void pfTrail::SetLabel(const char* STATS_ONLY(label))
	{
#if __STATS
		m_Label = label;
#endif
	}

} // namespace rage


#endif // GRPROFILE_TRAIL_H 
