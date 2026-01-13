// 
// profile/trail.cpp 
// 
// Copyright (C) 1999-2009 Rockstar Games.  All Rights Reserved. 
// 

#include "trail.h"

#include "bank/bank.h"
#include "diag/tracker.h"
#include "grprofile/drawitem.h"
#include "grcore/im.h"
#include "grcore/stateblock.h"
#include "grcore/viewport.h"
#include "system/timemgr.h"
#include "vectormath/legacyconvert.h"

namespace rage {

	pfTrail::pfTrail()
#if __STATS
		: m_KeyframeQueue(NULL)
		, m_Active(false)
		, m_WasActive(false)
		, m_OverPushes(0)
		, m_Color(0xFFFFFFFF)
		, m_Label(NULL)
#endif
	{
	}

	pfTrail::~pfTrail()
	{
#if __STATS
		delete m_KeyframeQueue;
#endif
	}

	void pfTrail::ResetKeyframe(KeyframeEntry& STATS_ONLY(kf))
	{
#if __STATS
		kf.m_PosAndFloat1.ZeroComponents();
		kf.m_DirAndFloat2.ZeroComponents();
		kf.m_Color = m_Color;
		kf.m_Label = m_Label;
		kf.m_Time = TIME.GetElapsedTime();
		kf.m_ExtraInt = 0;
#endif
	}

#if __STATS
	void pfTrail::Reset()
	{
		if (m_KeyframeQueue)
		{
			m_KeyframeQueue->Reset();
		}
		m_Color.Set(0xFFFFFFFF);
		m_Label = NULL;
	}

	void pfTrail::Update()
	{
		if(!ShouldRecord())
		{
			return;
		}

		if(!m_Active)
		{
			if(m_KeyframeQueue)
			{
				m_KeyframeQueue->Reset();
			}
			m_WasActive = false;
			return;
		}

		// Important - make sure we don't do anything with the mutex
		// if m_Active is false, so we don't affect the frame timing. /FF
		sysCriticalSection critSec(m_Mutex);

		m_WasActive = true;

		if(!m_KeyframeQueue)
		{
			RAGE_TRACK(pfTrail);
			m_KeyframeQueue = rage_new KeyframeQueue;
		}

		KeyframeEntry kf;
		ResetKeyframe(kf);
		SampleKeyframe(kf);
		bool shouldAdd = false;
		if(!m_KeyframeQueue->IsEmpty())
		{
			shouldAdd = ShouldAddKeyframe(m_KeyframeQueue->End(), kf);
		}
		else
		{
			shouldAdd = true;
		}

		if(shouldAdd)
		{
			if(m_KeyframeQueue->IsFull())
			{
				m_KeyframeQueue->Drop();
				m_OverPushes++;
			}
			m_KeyframeQueue->Push(kf);
		}
	}
#endif

#if __BANK
	void pfTrail::AddWidgets(bkBank &STATS_ONLY(bank))
	{
#if __STATS
		bank.AddToggle("Active", &m_Active);
		bank.AddColor("Current Color", &m_Color);
#endif
	}
#endif

	bool pfTrail::ShouldAddKeyframe(const KeyframeEntry &UNUSED_PARAM(oldEntry), const KeyframeEntry &UNUSED_PARAM(newEntry)) const
	{
		return true;
	}

	bool pfTrail::ShouldRecord() const
	{
		return true;
	}

	pfTrailVisualizer::pfTrailVisualizer()
#if __STATS
		: m_MinTime(-FLT_MAX)
		, m_MaxTime(FLT_MAX)
		, m_SampleSpacing(3.0f)
		, m_SampleUnits(FRAMES_PER_SAMPLE)
#endif
	{
	}

	void pfTrailVisualizer::SetFrameSpacing(int STATS_ONLY(frames))
	{
#if __STATS
		m_SampleUnits = FRAMES_PER_SAMPLE;
		m_SampleSpacing = float(frames);
#endif
	}

	void pfTrailVisualizer::SetTimeSpacing(float STATS_ONLY(time))
	{
#if __STATS
		m_SampleUnits = SECONDS_PER_SAMPLE;
		m_SampleSpacing = time;
#endif
	}

#if __STATS
	void pfTrailVisualizer::Draw(const pfTrail& trail) const
	{
		if (!trail.m_Active)
		{
			return;
		}

		// Important - make sure we don't do anything with the mutex
		// if m_Active is false, so we don't affect the frame timing.
		// Note that using a mutex like this isn't really good as we may
		// either include the previous frame's update or not, depending
		// on the timing between threads. But in this case, the data is
		// simple enough that that shouldn't matter. /FF
		sysCriticalSection critSec(trail.m_Mutex);

		if (!trail.m_KeyframeQueue)
		{
			return;
		}

		pfTrail::KeyframeQueue& queue = *trail.m_KeyframeQueue;

		if (queue.IsEmpty())
		{
			return;
		}

		grcStateBlock::Default();
		grcViewport::SetCurrentWorldIdentity();

		// get the real min and max range for the trail
		float startTime = queue[0].m_Time;
		float endTime = queue.Top().m_Time;

		startTime = Max(startTime, m_MinTime);
		endTime = Max(endTime, m_MaxTime);

		int startIndex = 0;
		int endIndex = queue.GetCount();

		// Find out which keys are in the right range
		for(int i = 0; i < queue.GetCount(); i++)
		{
			if (queue[i].m_Time < startTime)
			{
				startIndex++;
			}
			else
			{
				break;
			}
		}
		for(int i = queue.GetCount() - 1; i >= 0; i--)
		{
			if (queue[i].m_Time > endTime)
			{
				endIndex--;
			}
			else
			{
				break;
			}
		}

		if (startIndex < endIndex)
		{
			DrawOverTime(queue, startIndex, endIndex);
		}

		if (m_SampleUnits == FRAMES_PER_SAMPLE)
		{
			// draw every Nth keyframe
			int step = int(m_SampleSpacing);
			step = Max(step, 1);

			// offset the start index a bit so that we always appear to draw on a multiple of N frames
			startIndex += step - trail.m_OverPushes % step;

			for(int i = startIndex; i < endIndex; i += step)
			{
				DrawSample(queue[i]);
			}
		}
		else if (m_SampleUnits == SECONDS_PER_SAMPLE)
		{
			// draw every Nth second
			int prevIndex = 0;
			int prevStep = 0;
			float prevTime = queue[startIndex].m_Time;

			const float invSampleSpacing = 1.0f/m_SampleSpacing;
			const float offs = floorf(prevTime*invSampleSpacing)*m_SampleSpacing;

			for(int current = startIndex+1; current < endIndex; current++)
			{
				const float currentTime = queue[current].m_Time;
				const int currentStep = (int)floorf((currentTime - offs)*invSampleSpacing);

				if(currentStep != prevStep)
				{
					const float t = currentStep*m_SampleSpacing + offs;
					const float delta = t - prevTime;
					const float step = currentTime - prevTime;
					if(step <= VERY_SMALL_FLOAT)
					{
						continue;
					}
					const float p = delta/step;

					pfTrail::KeyframeEntry lerpedEntry;
					LerpSamples(p, queue[prevIndex], queue[current], lerpedEntry);
					DrawSample(lerpedEntry);

					prevStep = currentStep;
					prevIndex = current;
					prevTime = currentTime;
				}
			}
		}
		else if (m_SampleUnits == SINGLE_SAMPLE)
		{
			int sampleNum = Clamp((endIndex-1) - (int)m_SampleSpacing, startIndex, endIndex-1);
			DrawSample(queue[sampleNum]);
		}

	}
#endif

	void pfTrailVisualizer::LerpSamples(float t, const pfTrail::KeyframeEntry& a, const pfTrail::KeyframeEntry& b, pfTrail::KeyframeEntry& out) const
	{
		ScalarV tV;
		tV.Setf(t);
		out.m_PosAndFloat1 = Lerp(tV, a.m_PosAndFloat1, b.m_PosAndFloat1);
		out.m_DirAndFloat2 = Lerp(tV, a.m_DirAndFloat2, b.m_DirAndFloat2);
		out.m_Color = Lerp(t, a.m_Color, b.m_Color);
		out.m_Label = a.m_Label;
		out.m_Time = Lerp(t, a.m_Time, b.m_Time);
		out.m_ExtraInt = Lerp(t, a.m_ExtraInt, b.m_ExtraInt);
	}


	void pfTrailVisualizerSimple::DrawOverTime(const pfTrail::KeyframeQueue& STATS_ONLY(queue), int STATS_ONLY(first), int STATS_ONLY(last)) const
	{
#if __STATS
		const int numKeyframes = last - first;
		int current = 0;
		grcViewport::SetCurrentWorldIdentity();
		while(current < numKeyframes - 1)
		{
			const int numRemainingLines = numKeyframes - current - 1;
			const int numThisBatch = Min(numRemainingLines + 1, grcBeginMax);
			grcBegin(drawLineStrip, numThisBatch);
			for(int i = 0; i < numThisBatch; i++)
			{
				const pfTrail::KeyframeEntry &kf = queue[current + first];
				grcColor(kf.m_Color);
				grcVertex3f(VEC3V_TO_VECTOR3(kf.GetPos()));

				current++;
			}
			grcEnd();
			current--;
		}

		// now draw any text labels
		const char* lastLabel = NULL;
		for(int i = first; i != last; i++)
		{
			if (queue[i].m_Label != lastLabel)
			{
				lastLabel = queue[i].m_Label;
				grcDrawLabel(VEC3V_TO_VECTOR3(queue[i].GetPos()), lastLabel);
			}
		}
#endif
	}

	void pfTrailVisualizerSimple::DrawSample(const pfTrail::KeyframeEntry& STATS_ONLY(sample)) const
	{
#if __STATS
		Matrix34 sphereMtx;
		sphereMtx.Identity();
		sphereMtx.Translate(VEC3V_TO_VECTOR3(sample.GetPos()));
		grcColor(sample.m_Color);
		grcDrawSphere(0.05f, sphereMtx, 6, true);
#endif
	}

	void pfTrailVisualizerPosAndDir::DrawSample(const pfTrail::KeyframeEntry& STATS_ONLY(sample)) const
	{
#if __STATS
		Matrix34 sphereMtx;
		sphereMtx.Identity();
		sphereMtx.Translate(VEC3V_TO_VECTOR3(sample.GetPos()));
		grcColor(sample.m_Color);
		grcDrawSphere(0.025f, sphereMtx, 4, true);
		Vector3 dirStart(VEC3V_TO_VECTOR3(sample.GetPos()));
		Vector3 dirEnd(dirStart);
		dirEnd.AddScaled(VEC3V_TO_VECTOR3(sample.GetDir()),-0.4f);
		grcViewport::SetCurrentWorldIdentity();
		grcDrawLine(dirStart,dirEnd,Color32(1.0f,1.0f,1.0f));
#endif
	}

	pfTrailVisualizerCamera::pfTrailVisualizerCamera()
		: pfTrailVisualizerSimple()
		, m_DrawCameraIcon(false)
		, m_TimeTickSpacing(0)
		, m_Viewport(NULL)
	{
	}

#if __BANK
	const char* g_TrailSampleUnits[] = 
	{
		"Frames per sample",
		"Seconds per sample",
		"Single sample",
		NULL
	};

	void pfTrailVisualizerCamera::AddWidgets(bkBank &STATS_ONLY(bank))
	{
#if __STATS
		bank.AddToggle("Draw Camera Icon", &m_DrawCameraIcon);
		bank.AddSlider("Time Indicator Spacing Frames", &m_TimeTickSpacing, 0, 1000, 1);
		bank.AddSlider("Detail Sample Spacing", &m_SampleSpacing, 0.0f, 1000.0f, 0.1f);
		bank.AddCombo("Detail Sample Units", &m_SampleUnits, NUM_SAMPLE_UNITS, g_TrailSampleUnits);
#endif
	}
#endif

	void pfTrailVisualizerCamera::DrawOverTime(const pfTrail::KeyframeQueue& STATS_ONLY(queue), int STATS_ONLY(first), int STATS_ONLY(last)) const
	{
#if __STATS
		pfTrailVisualizerSimple::DrawOverTime(queue, first, last);

		if (m_TimeTickSpacing > 0)
		{
			Color32 solidCol(0xff, 0xc0, 0x80, 0xff);

			grcColor(solidCol);
			grcWorldIdentity();

			for(int current = last-1; current >= first; current -= m_TimeTickSpacing)
			{
				const pfTrail::KeyframeEntry& kf = queue[current];

				Vector3 pos0 = VEC3V_TO_VECTOR3(kf.GetPos());

				Vector3 curveDir;
				curveDir.Zero();
				if(current < last-1)
				{
					const pfTrail::KeyframeEntry &nextKf = queue[current + 1];
					Vector3 nextPos = VEC3V_TO_VECTOR3(nextKf.GetPos());
					curveDir.Add(nextPos);
					curveDir.Subtract(pos0);
				}
				if(current > first)
				{
					const pfTrail::KeyframeEntry &prevKf = queue[current - 1];
					Vector3 prevPos = VEC3V_TO_VECTOR3(prevKf.GetPos());
					curveDir.Add(pos0);
					curveDir.Subtract(prevPos);
				}


				Vector3 orthDir;
				orthDir.Zero();

				if (m_Viewport)
				{
					const Vector3& camPos = VEC3V_TO_VECTOR3(m_Viewport->GetCameraPosition());
					Vector3 camToPos;
					camToPos.Subtract(pos0, camPos);
					// orthDir.Cross(camDir, curveDir);
					orthDir.Cross(camToPos, curveDir);
				}
				else
				{
					orthDir.Cross(XAXIS, curveDir);
				}

				if(orthDir.NormalizeSafeRet())
				{
					static const float kTickHalfLen = 0.1f;

					Vector3 pos1;
					Vector3 pos2;
					pos1.AddScaled(pos0, orthDir, kTickHalfLen);
					pos2.SubtractScaled(pos0, orthDir, kTickHalfLen);
					grcBegin(drawLines, 2);
					grcVertex3f(pos1);
					grcVertex3f(pos2);
					grcEnd();
				}
			}
		}
#endif // __STATS
	}

	void pfTrailVisualizerCamera::DrawSample(const pfTrail::KeyframeEntry& STATS_ONLY(sample)) const
	{
#if __STATS
		Color32 solidCol(0xff, 0xc0, 0x80, 0xff);
		Color32 frustumCol(0xff, 0xc0, 0x80, 0x80);
		Color32 frustumLineCol(0x00, 0x00, 0x00, 0x80);
		Color32 transparentCol(0xff, 0xc0, 0x80, 0x00);
		Color32 transparentLineCol(0x00, 0x00, 0x00, 0x00);

		Vector3 dir = VEC3V_TO_VECTOR3(sample.GetDir());
		Vector3 pos = VEC3V_TO_VECTOR3(sample.GetPos());

		if(dir.NormalizeSafeRet())
		{

			grcStateBlock::SetDepthStencilState(grcStateBlock::DSS_IgnoreDepth);
			grcStateBlock::SetRasterizerState(grcStateBlock::RS_NoBackfaceCull);

			Matrix34 mtrx;
			mtrx.LookDown(dir);
			mtrx.d = pos;

#if __PFDRAW
			if (m_DrawCameraIcon)
			{
				pfDrawItem::DrawCameraIcon(mtrx, sample.m_PosAndFloat1.GetWf(), NULL, NULL, sample.m_Color);
				grcWorldMtx(mtrx);
				grcBegin(drawLines, 2);
				grcVertex3f(0.0f, 0.0f, 0.0f);
				grcVertex3f(0.0f, 0.0f, -5.0f);
				grcEnd();
			}
#endif

			grcWorldMtx(mtrx);

			// MAGIC! This doesn't really match the true field of view of the camera. /FF
			const float sx = 1.0f;
			const float sy = 9.0f/16.0f;	// 16/9 ratio
			const float sz = 0.8f;

			const float a=0.5f*sx, b=0.5f*sy, c=sz;
			Vector3 base[4];
			Vector3 apex(0.0f, 0.0f, 0.0f);

			base[0].Set(a, b, -c);
			base[1].Set(a, -b, -c);
			base[2].Set(-a, b, -c);
			base[3].Set(-a, -b, -c);

			grcBegin(drawTris, 12);

			grcColor(transparentCol);
			grcVertex3f(base[0]);
			grcVertex3f(base[1]);
			grcColor(frustumCol);
			grcVertex3f(apex);

			grcColor(transparentCol);
			grcVertex3f(base[1]);
			grcVertex3f(base[3]);
			grcColor(frustumCol);
			grcVertex3f(apex);

			grcColor(transparentCol);
			grcVertex3f(base[3]);
			grcVertex3f(base[2]);
			grcColor(frustumCol);
			grcVertex3f(apex);

			grcColor(transparentCol);
			grcVertex3f(base[2]);
			grcVertex3f(base[0]);
			grcColor(frustumCol);
			grcVertex3f(apex);

			grcEnd();

			// draw to apex
			grcBegin(drawLines, 8);
			for(int i=0; i<4; i++)
			{
				grcColor(frustumLineCol);
				grcVertex3f(apex);

				grcColor(transparentLineCol);
				grcVertex3f(base[i]);
			}
			grcEnd();
		}
#endif // __STATS
	}

} // namespace rage

