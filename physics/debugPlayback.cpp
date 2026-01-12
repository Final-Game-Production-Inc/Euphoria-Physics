// 
// physics/debugplayback.cpp 
// 
// Copyright (C) 1999-2015 Rockstar Games.  All Rights Reserved. 
// 

#include "debugplayback.h"

#if DR_ENABLED

#include "bank/bank.h"
#include "bank/bkmgr.h"
#include "grcore/debugdraw.h"
#include "grcore/font.h"
#include "grcore/im.h"
#include "grcore/viewport.h"
#include "input/keyboard.h"
#include "input/keys.h"
#include "input/mapper.h"
#include "input/mouse.h"
#include "math/float16.h"
#include "parser/manager.h"
#include "parser/psofile.h"
#include "parser/psoparserbuilder.h"
#include "parser/visitorxml.h"
#include "system/memmanager.h"
#include "system/stack.h"
#include "system/param.h"
#include "system/timemgr.h"
#include "system/tls.h"
#include "vector/geometry.h"

using namespace rage;
using namespace debugPlayback;

#define USE_BINARY 1	//Use PSO rather than XML data format

#include "debugplayback_parser.h"

namespace rage {

	namespace DR_MemMarkerVars
	{
		__THREAD int sm_iDepth;
		__THREAD size_t sm_iMarker;
		int sm_iAllocated;
	}

	sysCriticalSectionToken s_MemoryCriticalSectionToken;
	int DR_MemoryHeapHelper::GetMemoryAllocated()
	{
		sysCriticalSection critSection(s_MemoryCriticalSectionToken);
		return DR_MemMarkerVars::sm_iAllocated;
	}

	bool DR_MemoryHeapHelper::NeedToUseLessMemory()
	{
		return ((GetMemoryAllocated() + CurrentTrackedAllocation()) > (int)GetMemoryAllowance());
	}

	size_t DR_MemoryHeapHelper::GetMemoryAllowance()
	{
		sysCriticalSection critSection(s_MemoryCriticalSectionToken);
		return MEMMANAGER.GetRecorderAllocator()->GetHeapSize() >> 1;
	}

	int DR_MemoryHeapHelper::CurrentTrackedAllocation()
	{
		sysCriticalSection critSection(s_MemoryCriticalSectionToken);
		sysMemAllocator* pAllocator = MEMMANAGER.GetRecorderAllocator();
		return (int)pAllocator->GetMemoryUsed() - (int)DR_MemMarkerVars::sm_iMarker;
	}

	DR_MemoryHeapHelper::DR_MemoryHeapHelper()
	{
		s_MemoryCriticalSectionToken.Lock();
		//Assert(DR_MemMarkerVars::sm_iDepth >= 0);
		if (!DR_MemMarkerVars::sm_iDepth)
		{
			sysMemStartRecorder();
			sysMemAllocator* pAllocator = MEMMANAGER.GetRecorderAllocator();
			DR_MemMarkerVars::sm_iMarker = pAllocator->GetMemoryUsed();
		}
		++DR_MemMarkerVars::sm_iDepth;
	}

	DR_MemoryHeapHelper::~DR_MemoryHeapHelper()
	{
		--DR_MemMarkerVars::sm_iDepth;
		if (!DR_MemMarkerVars::sm_iDepth)
		{
			DR_MemMarkerVars::sm_iAllocated += CurrentTrackedAllocation();
			if (DR_MemMarkerVars::sm_iAllocated < 0)
			{
				Assertf(DR_MemMarkerVars::sm_iAllocated < 0, "TMS: How did we track negative memory? [%d]", DR_MemMarkerVars::sm_iAllocated);
				DR_MemMarkerVars::sm_iAllocated = 0;
			}
			sysMemAllocator* pAllocator = MEMMANAGER.GetRecorderAllocator();
			Assertf(DR_MemMarkerVars::sm_iAllocated<=(int)(pAllocator->GetMemoryAvailable()>>1), "TMS: Why are we using so much debug heap memory? [%d %" SIZETFMT "u]", DR_MemMarkerVars::sm_iAllocated, pAllocator->GetMemoryAvailable()>>1);
			Assert(&sysMemAllocator::GetCurrent() == pAllocator);
			DR_MemMarkerVars::sm_iMarker = pAllocator->GetMemoryUsed();

			sysMemEndRecorder();
		}
		s_MemoryCriticalSectionToken.Unlock();
	}


	namespace debugPlayback	{

		DR_EVENT_IMP(SimpleLabelEvent);
		DR_EVENT_IMP(SimpleLineEvent);
		DR_EVENT_IMP(SimpleMatrixEvent);
		DR_EVENT_IMP(SimpleSphereEvent);

		//------------------------------------------------------------------//
		//								IO
		//------------------------------------------------------------------//

		//Statics
		size_t CallstackHelper::sm_iTrapCallstack=0;
		bool CallstackHelper::sm_bTrapCallstack=false;
		DR_EventType* DR_EventType::smp_First;
		__THREAD bool g_bOverrideEventsOn=false;
		bool EventBase::smb_DrawIndex;
		u32 EventBase::sm_EventCount;
		u32 Frame::sm_FrameCount;
		u8 DebugRecorder::sm_iGlobalAlphaScale = 255;

#if USE_BINARY
//		static const char *s_FileExt = "des";
		static const char *s_FileExt_Search = "*.des";
#else
		static const char *s_FileExt = "xml";
		static const char *s_FileExt_Search = "*.xml";
#endif
		static float XMainDisplay = 250.0f;
		//static float XColumn1 = 1150.0f;
		static float XColumn2 = 1000.0f;
		static float XColumn3 = 880.0f;
		static float YStart = 50.0f;
		static float YMax = 600.0f;
		static float YLineOffset = 10.0f;
		static float FontScale = 1.0f;
		static float YChannel = 400.0f;

		//------------------------------------------------------------------//
		// Callstack helper manages the debug trap event and is then passed
		//				into events so the ID can be stored.
		//------------------------------------------------------------------//
		CallstackHelper::CallstackHelper()
		{
			DebugRecorder *pInstance = DebugRecorder::GetInstance();
			if (pInstance)
			{
				m_iCallstackID = pInstance->RegisterCallstack(3);
				if (sm_bTrapCallstack)
				{
					if (m_iCallstackID == sm_iTrapCallstack)
					{
						__debugbreak();
						sm_bTrapCallstack = false;	//Otherwise optimizations make it annoying to remove
					}
				}
			}
		}
		//------------------------------------------------------------------//

		ItemBase::ItemBase(){}

		DataBase::DataBase(){}
		

		//------------------------------------------------------------------//
		//			DrawOthersData - list of event subtypes to draw for		//
		//------------------------------------------------------------------//
		const int kMaxDrawOthers = 16;
		struct DrawOthersData
		{
			const char *mp_SubType;
			static DrawOthersData sm_DrawOthersList[];
			static int sm_iNumDrawOthers;

			static void SetDrawOthers(const char *pSubType, bool bOn)
			{
				for (int i=0; i<sm_iNumDrawOthers ; i++)
				{
					if (sm_DrawOthersList[i].mp_SubType == pSubType)
					{
						if (!bOn)
						{
							//Fast delete
							if (i < sm_iNumDrawOthers-1)
							{
								sm_DrawOthersList[i] = sm_DrawOthersList[sm_iNumDrawOthers-1];
							}
							--sm_iNumDrawOthers;
						}
						return;
					}
				}
				if (!bOn)
				{
					return;
				}
				if (sm_iNumDrawOthers < kMaxDrawOthers)
				{
					sm_DrawOthersList[sm_iNumDrawOthers].mp_SubType = pSubType;
					++sm_iNumDrawOthers;
				}
			}

			static bool GetDrawOthers(const char *pSubType) 
			{
				for (int i=0; i<sm_iNumDrawOthers ; i++)
				{
					if (sm_DrawOthersList[i].mp_SubType == pSubType)
					{
						return true;
					}
				}
				return false;
			}
		};
		DrawOthersData DrawOthersData::sm_DrawOthersList[kMaxDrawOthers];
		int DrawOthersData::sm_iNumDrawOthers=0;
		//------------------------------------------------------------------//


		//------------------------------------------------------------------//
		//							DebugRecorder
		//------------------------------------------------------------------//
		//For our one instance of this (?)
		DebugRecorder *DebugRecorder::smp_DebugRecorder = 0;
		DebugRecorder::DebugRecorder()
			:mp_FirstFrame(0)
			,mp_LastFrame(0)
			,mp_AnchorFrame(0)
			,mp_HoveredFrame(0)
			,mp_HoveredEvent(0)
			,mp_WasHoveredEvent(0)
			,mp_AnchorEvent(0)
			,mp_SelectedFrame(0)
			,mp_SelectedEvent(0)
			,mp_LastHoveredData(0)
			,m_iNumEventsRecorded(0)
			,m_iAnchorDisplayType(0)
			,m_iAnchorDataFilter(0)
			,m_iCachedEventIndex(0)
			,m_iFirstFrame(0)
			,m_iLastFrame(0)
			,m_iLastRecordedFrame(0)
			,m_iRecorderFrameMarker(0)
			,m_bRecording(false)
			,m_bDisplayOn(false)
			,m_bWasPaused(false)
			,m_bRestrictToDataFilter(false)
			,m_bDrawFramesList(true)
			,m_bDrawEventsList(true)
			,m_bDrawElementsList(false)
			,m_bFilterNeedsRefresh(true)
			,m_bPlaybackOn(false)
			,m_iFilterTypeIndex(0)
			,mp_HighlightData(0)
			,mp_SaveData(0)
		{
			Assert(!smp_DebugRecorder);
			smp_DebugRecorder = this;
		}

		DebugRecorder::~DebugRecorder()
		{
			ClearRecording();
			delete mp_SaveData;
			smp_DebugRecorder = 0;
		}

		size_t DebugRecorder::RegisterCallstack(int iIgnorelevels)
		{
			const int kCallstackDepth = 12;
			size_t temp[kCallstackDepth];
			for (int i=0 ; i<kCallstackDepth ; i++)
			{
				temp[i] = 0;
			}
			sysStack::CaptureStackTrace(temp,kCallstackDepth,iIgnorelevels);
			size_t hash = 0;
			for (int i=0; i<kCallstackDepth; i++)
				hash ^= temp[i];

			//New callstack?
			DR_MEMORY_HEAP();
			sysCriticalSection section(m_CallstackCSToken);
			if (!m_Callstacks.Access(hash))
			{
				Callstack *pCallstack = rage_new Callstack;
				m_Callstacks[hash] = pCallstack;
				pCallstack->Init(temp, kCallstackDepth);
				pCallstack->m_iSignature = hash;
				return hash;
			}

			return hash;
		}

		Callstack* DebugRecorder::GetCallstack(size_t key)
		{
			sysCriticalSection section(m_CallstackCSToken);
			Callstack** pStack = m_Callstacks.Access(key);
			return pStack ? *pStack : 0;
		}

		void DebugRecorder::AddEvent(EventBase &rEvent)
		{
			sysCriticalSection critSection(m_DataCSToken);
			DR_MEMORY_HEAP();

			//Reset systems that don't support data changing on them (pointers stored into data)
			if (m_Graph.HasData())
			{
				m_Graph.Reset();
			}

			m_EventTimeLine.Reset();

			//We'll need to update the display status filter after this
			m_bFilterNeedsRefresh = true;

			while (DR_MemoryHeapHelper::NeedToUseLessMemory() && mp_FirstFrame)
			{
				if (Verifyf(mp_FirstFrame->mp_FirstEvent, "Looks like we've been leaking events!"))
				{
					EventBase *pEventToDelete = mp_FirstFrame->mp_FirstEvent;
					mp_FirstFrame->mp_FirstEvent = mp_FirstFrame->mp_FirstEvent->mp_Next;
					if (mp_FirstFrame->mp_FirstEvent)
					{
						mp_FirstFrame->mp_FirstEvent->mp_Prev = 0;
					}

					if (mp_SelectedEvent == pEventToDelete)
					{
						mp_SelectedEvent = 0;
					}

					if (mp_HoveredEvent == pEventToDelete)
					{
						mp_HoveredEvent = 0;
					}

					if (mp_WasHoveredEvent == pEventToDelete)
					{
						mp_WasHoveredEvent = 0;
					}

					if (mp_AnchorEvent == pEventToDelete)
					{
						mp_AnchorEvent = 0;
					}

					//Delete it!
					delete pEventToDelete;
				}

				if (!mp_FirstFrame->mp_FirstEvent)
				{
					Frame *pFrameToDelete = mp_FirstFrame;
					mp_FirstFrame = mp_FirstFrame->mp_Next;
					if (mp_AnchorFrame)
					{
						mp_AnchorFrame = mp_FirstFrame;
					}
					if (Verifyf(mp_FirstFrame, "Not expected to delete all frames in continuous recording mode"))
					{
						mp_FirstFrame->mp_Prev = 0;
						m_iFirstFrame = mp_FirstFrame->m_FrameIndex;
					}
					if (mp_LastFrame == pFrameToDelete)
					{
						Assertf(0, "Not expected to delete all frames in continuous recording mode");
						mp_LastFrame = 0;
					}

					if (mp_SelectedFrame == pFrameToDelete)
					{
						mp_SelectedFrame = 0;
					}

					if (mp_HoveredFrame == pFrameToDelete)
					{
						mp_HoveredFrame = 0;
					}
					//Delete the frame!
					delete pFrameToDelete;
				}
			}

			//Set out index
			rEvent.m_iEventIndex = m_iNumEventsRecorded;
			++m_iNumEventsRecorded;

			//Add to frame (get new frame if needed)
			Frame *pFrame = GetOrAllocateCurrentFrame();
			Assert(pFrame);
			if (pFrame->mp_LastEvent)
			{
				rEvent.mp_Prev = pFrame->mp_LastEvent;
				pFrame->mp_LastEvent->mp_Next = &rEvent;
				pFrame->mp_LastEvent = &rEvent;
			}
			else
			{
				pFrame->mp_FirstEvent = pFrame->mp_LastEvent = &rEvent;
			}

			//Make sure we've cached the objects
			rEvent.OnAddToList(*this);
		}

		void DebugRecorder::ClearSelection()
		{
			mp_SelectedFrame = 0;
			mp_SelectedEvent = 0;
			mp_AnchorFrame = 0;
			mp_AnchorEvent = 0;
			mp_HoveredEvent = 0;
			mp_WasHoveredEvent = 0;
		}

		u32 DebugRecorder::GetRecorderFrameCount() const
		{
			return m_iRecorderFrameMarker;
		}

		void DebugRecorder::SetRecorderFrameCount(u32 iFrame)
		{
			m_iRecorderFrameMarker = iFrame;
		}

		void DebugRecorder::SetRecorderNetworkTime(unsigned iNetworkTime)
		{
			m_iNetworkTime = (u32)iNetworkTime;
		}

		void DebugRecorder::StartRecording()
		{
			sysCriticalSection critSection(m_DataCSToken);
			DR_MEMORY_HEAP();

			//Make sure we release any memory
			ClearRecording();

			m_iAnchorDisplayType = 0;
			m_iAnchorDataFilter = 0;
			m_iCachedEventIndex = 0;
	
			mp_FirstFrame = 0;
			m_iFirstFrame = TIME.GetFrameCount();
			m_iLastFrame = 0;
			m_iLastRecordedFrame = 0;
			m_bRecording = true;

			//New recordings will need different zooms etc, always want to start browseing the recording with clean data.
			m_ZoomData.Reset();

			//Allow derived types to respond to the starting of recording
			OnStartRecording();
		}

		void DebugRecorder::ClearRecording()
		{
			sysCriticalSection critSection(m_DataCSToken);

			DR_MEMORY_HEAP();

			OnClearRecording();

			//Delete all frames
			Frame *pFrame = mp_FirstFrame;
			while (pFrame)
			{
				Frame *pNext = pFrame->mp_Next;
				delete pFrame;
				pFrame = pNext;
			}

			//Clear the objects
			atMap<size_t, DataBase*>::Iterator itrRD = m_RecordedData.CreateIterator();
			while (!itrRD.AtEnd())
			{
				delete itrRD.GetData();
				++itrRD;
			}
			m_RecordedData.Kill();
			m_bRestrictToDataFilter = false;
			mp_HighlightData = 0;

			//And the callstacks
			sysCriticalSection section(m_CallstackCSToken);
			atMap<size_t, Callstack*>::Iterator itrCS = m_Callstacks.CreateIterator();
			while (!itrCS.AtEnd())
			{
				delete itrCS.GetData();
				++itrCS;
			}
			m_Callstacks.Kill();

			//Clear any selection
			ClearSelection();

			//Clear hooks into old data
			m_Graph.Reset();
			m_EventTimeLine.Reset();
			mp_FirstFrame = 0;
			mp_LastFrame = 0;

			//Clear info on old data
			m_iNumEventsRecorded = 0;
			m_iLastFrame = 0;
			m_iLastRecordedFrame = 0;
			m_iFirstFrame = TIME.GetFrameCount();

			//Clear cached info
			m_DisplayedNameList.Reset();
			m_DisplayedTypeMap.Reset();

			m_bFilterNeedsRefresh = true;
			m_bPlaybackOn = false;
		}

		void DebugRecorder::StopRecording()
		{
			sysCriticalSection critSection(m_DataCSToken);

			DR_MEMORY_HEAP();

			m_bRecording = false;
			
			//Default to the last frame - X frames on screen?
			if(!mp_AnchorFrame)
			{
				ClearSelection();

				mp_AnchorFrame = mp_LastFrame;

				if (mp_AnchorFrame)
				{
					int iRollback = 10;
					while (iRollback && mp_AnchorFrame->mp_Prev)
					{
						--iRollback;
						mp_AnchorFrame = mp_AnchorFrame->mp_Prev;
					}
				}
			}

			//Make sure we have a last frame
			m_iLastFrame = TIME.GetFrameCount();
		}

		Callstack::Callstack(const Callstack& rOther)
			:m_SymbolArray(rOther.m_SymbolArray)
			,m_AddressArray(rOther.m_AddressArray)
			,m_iSignature(rOther.m_iSignature)
			,m_bIsScript(rOther.m_bIsScript)
		{	}
		Callstack::Callstack()
			:m_iSignature(0)
			,m_bIsScript(false)
		{	}

		void Callstack::Init(size_t *pAddressArray, int iMaxArrayLimit)
		{
			m_AddressArray.Reset();

			if (pAddressArray)
			{
				for (int i=0 ; i<iMaxArrayLimit ; i++)
				{
					if (pAddressArray[i])
					{
						m_AddressArray.Grow(iMaxArrayLimit) = pAddressArray[i];
					}
				}
			}
		}

		void Callstack::Print(TextOutputVisitor &rText)
		{
			CacheStrings();

			for (int i=0 ; i<m_SymbolArray.GetCount() ; i++)
			{
				rText.AddLine("%s", m_SymbolArray[i].c_str());
			}
		}

		void Callstack::CacheStrings()
		{
			//Only cache strings for a call-stack once. Don't cache them if the callstack was loaded.
			if (!m_SymbolArray.GetCount() && m_AddressArray.GetCount())
			{
				sysStack::OpenSymbolFile();

				m_SymbolArray.Reserve(m_AddressArray.GetCount());
				for (int i=0; i<m_AddressArray.GetCount(); i++) 
				{
					char symname[256] = "unknown";
					u32 disp = sysStack::ParseMapFileSymbol(symname,sizeof(symname),m_AddressArray[i]);

					//------------------------------
					//A little work to reduce screen real estate used while presenting most of the useful information
					//Remove rage namespace
					bool bRageNameSpace = false;
					if (!strncmp(symname, "rage::", 6))
					{
						bRageNameSpace = true;
					}
					//Remove params
					strtok(symname, "(");
					//------------------------------

					char formatted[512];
					formatf(formatted, "%s+%x", bRageNameSpace ? &symname[6] : symname, disp);
					m_SymbolArray.Append() = formatted; 
				}

				sysStack::CloseSymbolFile();
			}
		}


		void DebugRecorder::Save(const char *pFileName, bool (*OnlyTheseEvents)(const EventBase *))
		{
			DR_MEMORY_HEAP();
			sysCriticalSection critSection(m_DataCSToken);

			if (!mp_SaveData)
			{
				mp_SaveData = CreateSaveData();
			}

			//Count the frames!
			Frame *pFrames = mp_FirstFrame;
			int iNumFrames=0;
			while (pFrames)
			{
				++iNumFrames;
				pFrames = pFrames->mp_Next;
			}

			if (iNumFrames > 0)
			{
				mp_SaveData->m_Frames.Resize(iNumFrames);
				pFrames = mp_FirstFrame;
				int iIndex=0;
				int iTotalEventCount = 0;
				while (pFrames)
				{
					if (pFrames == mp_SelectedFrame)
					{
						mp_SaveData->m_SelectedFrameIndex = iIndex;
					}
					if (pFrames == mp_AnchorFrame)
					{
						mp_SaveData->m_AnchorFrameIndex = iIndex;
					}
					
					mp_SaveData->m_Frames[iIndex].m_FrameIndex = pFrames->m_FrameIndex;

					//Count the events to pre-size the array
					EventBase *pEvents = pFrames->mp_FirstEvent;
					int iNumEvents = 0;
					while (pEvents)
					{
						if (!OnlyTheseEvents || OnlyTheseEvents(pEvents))
						{
						++iNumEvents;
						}
						pEvents = pEvents->mp_Next;
					}
					mp_SaveData->m_Frames[iIndex].m_pEvents.Resize(iNumEvents);

					//Set pointers to the actual events (not owned)
					pEvents = pFrames->mp_FirstEvent;
					int iEventIndex = 0;
					while (pEvents)
					{	
						if (!OnlyTheseEvents || OnlyTheseEvents(pEvents))
						{
						mp_SaveData->m_Frames[iIndex].m_pEvents[iEventIndex] = pEvents;
						if (pEvents == mp_SelectedEvent)
						{
							mp_SaveData->m_SelectedEventIndex = iTotalEventCount;
						}
						if (pEvents == mp_AnchorEvent)
						{
							mp_SaveData->m_AnchorEventIndex = iTotalEventCount;
						}

						//Increment
						++iEventIndex;
						++iTotalEventCount;
						}
						pEvents = pEvents->mp_Next;
					}

					++iIndex;
					pFrames = pFrames->mp_Next;
				}
			}

			//Save the 
			mp_SaveData->m_RecordedDataArray.Resize( m_RecordedData.GetNumUsed() );
			mp_SaveData->m_RecordedDataIDArray.Resize( m_RecordedData.GetNumUsed() );
			atMap<size_t, DataBase*>::Iterator itrRD = m_RecordedData.CreateIterator();
			int iCounter = 0;
			while (!itrRD.AtEnd())
			{
				mp_SaveData->m_RecordedDataArray[iCounter] = itrRD.GetData();
				mp_SaveData->m_RecordedDataIDArray[iCounter] = itrRD.GetKey();
				++iCounter;
				++itrRD;
			}

			//And store off the callstacks
			sysCriticalSection section(m_CallstackCSToken);
			mp_SaveData->m_SavedCallstacks.Resize(m_Callstacks.GetNumUsed());
			atMap<size_t, Callstack*>::Iterator itrCS = m_Callstacks.CreateIterator();
			iCounter = 0;
			while (!itrCS.AtEnd())
			{
				mp_SaveData->m_SavedCallstacks[iCounter] = itrCS.GetData();
				++iCounter;
				++itrCS;
			}

			//Give app level code a chance to save some data
			OnSave(*mp_SaveData);

#if USE_BINARY
			psoSaveObject(pFileName, this);
#else
			fiStream* stream = ASSET.Create(pFileName, s_FileExt);
			if (stream)
			{
				// Use the XML writer instead of PARSER.SaveObject because none of these objects have onSave callbacks that would require us to use SaveObject
				parXmlWriterVisitor visitor(stream);
				visitor.Visit(*this);
				stream->Close();
			}
#endif
			//Delete temp memory used for IO
			mp_SaveData->DeleteTempIOMemory();
		}

		void GraphFloats::GraphFloatLine::Add(const EventBase *pEvent, float fNewValue, s32 iFrame)
		{
			if (m_Values.GetCount())
			{
				m_fMax = Max(m_fMax, fNewValue);
				m_fMin = Min(m_fMin, fNewValue);
			}
			else
			{
				m_fMax = m_fMin = fNewValue;
			}

			m_Values.Grow(512) = fNewValue;

			GraphLineBase::Add(pEvent, fNewValue, iFrame);
		}

		void GraphFloats::GraphLineBase::Add(const EventBase *pEvent, float /*fNewValue*/, s32 iFrame)
		{
			m_Frames.Grow(512) = iFrame;
			m_Events.Grow(512) = pEvent;
		}

		bool GraphFloats::GraphTimeLine::Draw(
			int iIndex, Color32 graphLineColor,
			int iFrameMin, int iFrameMax, 
			float fMinY, float fMaxY,float fGraphLeft, float fGraphRight,
			float fMouseX, float fMouseY,
			const EventBase * &pSelected, float &fDistToMouse2,
			const EventBase *pCurrentSelected, const EventBase *pCurrentHighlighted,
			TimeLineZoom &rZoom
			) const
		{
			bool bHovered = false;
			float fAvY = (fMinY + fMaxY) * 0.5f;
			float fYRange = (fMaxY - fMinY) * 0.5f;
			grcColor(graphLineColor);
			int iLastFrame = 0;
			int iCountInFrame = 0;
			float fCountInFrameToX=0.0f;
			const float fGraphFrameToOffset = (fGraphRight - fGraphLeft) / float((iFrameMax + 1) - iFrameMin)*rZoom.m_fZoom;	//+1 allows for events within the last frame
			const float fGraphOffset = fGraphLeft + rZoom.m_fTimeLineOffset;
			grcColor( (iIndex&1) ? Color32(20,20,20,20) : Color32(50,50,50,20));
			grcBindTexture(NULL);

			grcStateBlock::SetBlendState(grcStateBlock::BS_CompositeAlpha);
			grcStateBlock::SetRasterizerState(grcStateBlock::RS_NoBackfaceCull);
			
			grcBegin(drawTriFan, 4);
			grcVertex2f(fGraphLeft, fMinY);
			grcVertex2f(fGraphRight, fMinY);
			grcVertex2f(fGraphRight, fMaxY);
			grcVertex2f(fGraphLeft, fMaxY);
			grcEnd();

			grcStateBlock::SetBlendState(grcStateBlock::BS_Normal);

			//Draw each element
			const int iCount = m_Events.GetCount();
			for (int i=0 ; i<iCount ; i++)
			{
				int iCurrentFrame = m_Frames[i];
				float fScreenX = ((float)(iCurrentFrame - iFrameMin))*fGraphFrameToOffset + fGraphOffset;

				//Update sub-frame scaling based on number of events in this current frame
				if(iLastFrame == iCurrentFrame)
				{
					iCountInFrame++;
					if (iCountInFrame == 1)
					{
						//Need to work out sub frame scaling
						int iNextFrame = i+1;
						while (iNextFrame < iCount)
						{
							if(m_Frames[iNextFrame] != iCurrentFrame)
							{
								break;
							}
							++iNextFrame;
						}

						fCountInFrameToX = fGraphFrameToOffset / float(iNextFrame - i + 1);
					}
					fScreenX += float(iCountInFrame) * fCountInFrameToX;
				}
				else
				{
					iCountInFrame = 0;
				}
				iLastFrame = iCurrentFrame;

				if (fScreenX >= fGraphLeft && fScreenX <= fGraphRight)
				{
					//Check mouse over
					Vector2 vPosDelta(fScreenX - fMouseX, fAvY - fMouseY);
					float dist2 = vPosDelta.Mag2();
					if (dist2 < fDistToMouse2)
					{
						fDistToMouse2 = dist2;
						pSelected = m_Events[i];
						bHovered = true;
					}

					//Draw box
					DrawSquare(fScreenX, fAvY, 0, fYRange, graphLineColor);

					//Draw highlights
					if (m_Events[ i ] == pCurrentSelected)
					{
						DrawSquare(fScreenX, fAvY+1, 1, fYRange+1.0f, Color32(1.0f,0.5f,0.5f));
						grcColor(graphLineColor);
					}
					else if (m_Events[ i ] == pCurrentHighlighted)
					{
						DrawSquare(fScreenX, fAvY+1, 1, fYRange+1.0f, Color32(0.8f,0.8f,0.8f));
						grcColor(graphLineColor);
					}
				}
			}

			return bHovered;
		}

		bool GraphFloats::GraphFloatLine::Draw(
					bool bSelected, Color32 graphLineColor,
					int iFrameMin, int iFrameMax, 
					float fGraphLeft, float fGraphTop, float fGraphRight, float fGraphBottom,
					float fMinY, float fMaxY,
					float fMouseX, float fMouseY,
					const EventBase * &pSelected, float &fDistToMouse2,
					float &fHoveredValue,
					const EventBase *pCurrentSelected, const EventBase *pCurrentHighlighted,
					TimeLineZoom &rZoom
				) const
		{
			bool bHovered = false;
			float fYScale = (fMaxY > fMinY) ? (fGraphBottom - fGraphTop) / (fMaxY - fMinY) : 0.0f;
			float fLastX = fGraphLeft;
			float fLastY = fGraphBottom;
			grcColor(graphLineColor);
			int iLastFrame = 0;
			int iCountInFrame = 0;
			float fCountInFrameToX=0.0f;
			const float fGraphFrameToOffset = (fGraphRight - fGraphLeft) / float((iFrameMax + 1) - iFrameMin) *rZoom.m_fZoom;	//+1 allows for events within the last frame
			const float fOffset = fGraphLeft + rZoom.m_fTimeLineOffset;
			const int iCount = m_Values.GetCount();
			for (int i=0 ; i<iCount ; i++)
			{
				int iCurrentFrame = m_Frames[i];
				float fScreenX = ((float)(iCurrentFrame - iFrameMin))*fGraphFrameToOffset + fOffset;
				float fScreenY = fGraphBottom - ((m_Values[i] - fMinY) * fYScale);

				//Update sub-frame scaling based on number of events in this current frame
				if(iLastFrame == iCurrentFrame)
				{
					iCountInFrame++;
					if (iCountInFrame == 1)
					{
						//Need to work out sub frame scaling
						int iNextFrame = i+1;
						while (iNextFrame < iCount)
						{
							if(m_Frames[iNextFrame] != iCurrentFrame)
							{
								break;
							}
							++iNextFrame;
						}

						fCountInFrameToX = fGraphFrameToOffset / float(iNextFrame - i + 1);
					}
					fScreenX += float(iCountInFrame) * fCountInFrameToX;
				}
				else
				{
					iCountInFrame = 0;
				}
				iLastFrame = iCurrentFrame;


				//Draw line
				if (	(fScreenX >= fGraphLeft) 
					&&	(fLastX >= fGraphLeft)
					&&	(fScreenX <= fGraphRight) 
					&&	(fLastX <= fGraphRight)
					)
				{
					if (i>0)
					{
						grcBegin(drawLineStrip, 2);
						grcVertex2f(fScreenX, fScreenY);
						grcVertex2f(fLastX, fLastY);
						grcEnd();

						if (bSelected)
						{
							//Draw thicker line
							grcBegin(drawLineStrip, 2);
							grcVertex2f(fScreenX, fScreenY+1);
							grcVertex2f(fLastX, fLastY+1);
							grcEnd();
							grcBegin(drawLineStrip, 2);
							grcVertex2f(fScreenX, fScreenY-1);
							grcVertex2f(fLastX, fLastY-1);
							grcEnd();
						}

						//Check mouse distance
						//float fTmpDist = Vector2(fScreenX, fScreenY).Dist(Vector2(fMouseX, fMouseY));
						Vector3 vMouse(fMouseX, fMouseY, 0.0f);
						Vector3 vLine0(fLastX, fLastY, 0.0f);
						Vector3 vLineDelta(fScreenX - fLastX, fScreenY - fLastY, 0.0f);
					
						if (	((fMouseY >= fGraphTop) && (fMouseY <= fGraphBottom))
							&&	((fMouseX >= fGraphLeft) && (fMouseX <= fGraphRight))
							)
						{
							float fNearestTValue = geomTValues::FindTValueSegToPoint(vLine0, vLineDelta, vMouse);
							Vector3 vDelta = vMouse - (vLine0 + (vLineDelta * fNearestTValue));
							float dist2 = vDelta.Mag2();
							if (dist2 < fDistToMouse2)
							{
								fDistToMouse2 = dist2;

								if (fNearestTValue > 0.5f)
								{
									pSelected = m_Events[i];
									fHoveredValue = m_Values[i];
								}
								else
								{
									pSelected = m_Events[i-1];
									fHoveredValue = m_Values[i-1];
								}

								bHovered = true;
							}
						}
					}

					//Draw highlights
					if (m_Events[ i ] == pCurrentSelected)
					{
						DrawSquare(fScreenX, fScreenY, 3, 3, Color32(1.0f,0.5f,0.5f));
						grcColor(graphLineColor);
					}
					else if (m_Events[ i ] == pCurrentHighlighted)
					{
						DrawSquare(fScreenX, fScreenY, 3, 3, Color32(0.8f,0.8f,0.8f));
						grcColor(graphLineColor);
					}
				}

				//If a point is under the mouse highlight it and the event that corresponds.							
				fLastX = fScreenX;
				fLastY = fScreenY;
			}

			return bHovered;
		}

		void GraphFloats::GraphFloatLine::ExtendValueRange(float &fMinMin, float &fMaxMax) const 
		{
			fMinMin = Min(fMinMin, m_fMin);
			fMaxMax = Max(fMaxMax, m_fMax);
		}

		void GraphFloats::GraphLineBase::DrawSquare(float fX, float fY, float dhX, float dhY, Color32 col) const
		{
			grcColor(col);
			grcBegin(drawLineStrip, 5);
			grcVertex2f(fX-dhX, fY-dhY);
			grcVertex2f(fX+dhX, fY-dhY);
			grcVertex2f(fX+dhX, fY+dhY);
			grcVertex2f(fX-dhX, fY+dhY);
			grcVertex2f(fX-dhX, fY-dhY);
			grcEnd();
		}

		bool GraphFloats::ShouldDrawLine(const GraphLineBase *pLine)
		{
			bool bDrawLine = false;
			switch (pLine->GetGraphLineType())
			{
			case GraphLineBase::eValueLine:
				bDrawLine = (m_eDrawGraph == eDrawMode_Values) || (m_eDrawGraph == eDrawMode_All);
				break;
			case GraphLineBase::eTimeLine:
				bDrawLine = (m_eDrawGraph == eDrawMode_TimeLine) || (m_eDrawGraph == eDrawMode_All);
			default:
				break;
			}
			return bDrawLine;
		}

		bool GraphFloats::DebugDraw(float mouseScreenX, float mouseScreenY, bool bMouseDown, const EventBase *pCurrentSelected, const EventBase *pCurrentHighlighted, TimeLineZoom &rZoomFactor)
		{
			DR_MEMORY_HEAP();

			bool bSelected=false;
			//Need tag selection to the left....
			OnScreenOutput TagSelection;
			TagSelection.m_fXPosition = m_fX;
			TagSelection.m_fYPosition = m_fY;
			TagSelection.m_fMaxY = m_fY + m_fHeight;
			TagSelection.m_fMouseX = mouseScreenX;
			TagSelection.m_fMouseY = mouseScreenY;
			TagSelection.m_fPerLineYOffset = YLineOffset;
			TagSelection.m_Color.Set(160,160,160,255);
			TagSelection.m_HighlightColor.Set(255,0,0,255);

			TagSelection.m_iMaxStringLength = 18;



			const char *pGraphLabel = "<-GRAPH->";
			switch(m_eDrawGraph)
			{
			case eDrawMode_Nothing:
				break;
			case eDrawMode_Values:
				pGraphLabel = "<-GRAPH-VALUE->";
				break;
			case eDrawMode_TimeLine:
				pGraphLabel = "<-GRAPH-TIMELINE->";
				break;
			case eDrawMode_All:
				pGraphLabel = "<-GRAPH-ALL->";
				break;
			}
			if (TagSelection.AddLine(pGraphLabel) && bMouseDown)
			{
				++m_eDrawGraph;
				if (m_eDrawGraph > eDrawMode_All)
				{
					m_eDrawGraph = eDrawMode_Nothing;
				}
				bMouseDown = false;
				bSelected = true;
			}

			if (	(m_eDrawGraph == eDrawMode_Nothing)
				||	(!DebugRecorder::GetInstance()->GetFirstFramePtr())
				)
			{
				return bSelected;
			}

			if (TagSelection.AddLine("Common range [%s]", m_bShareScale ? "X" : " ") && bMouseDown)
			{
				m_bShareScale = !m_bShareScale;
				bMouseDown = false;
				bSelected = true;
			}

			//Draw the outline of the graph
			const float fGraphLeft = m_fX + m_fGraphOffset;
			const float fGraphWidth = m_fWidth - m_fGraphOffset;
			const float fGraphRight = fGraphLeft + fGraphWidth;
			const float fGraphBottom = m_fY + m_fHeight;
			const float fGraphTop = m_fY;

			grcColor( Color32(20,20,20,40) );
			grcBindTexture(NULL);

			grcStateBlock::SetBlendState(grcStateBlock::BS_CompositeAlpha);
			grcStateBlock::SetRasterizerState(grcStateBlock::RS_NoBackfaceCull);

			grcBegin(drawTriFan, 4);
			grcVertex2f(fGraphLeft, fGraphTop);
			grcVertex2f(fGraphRight, fGraphTop);
			grcVertex2f(fGraphRight, fGraphBottom);
			grcVertex2f(fGraphLeft, fGraphBottom);
			grcEnd();

			grcStateBlock::SetBlendState(grcStateBlock::BS_Normal);

			//Make sure we keep something valid selected
			if (m_iSelectedTag >= m_GraphLines.GetCount())
			{
				m_iSelectedTag = -1;
			}

			//Draw tags, select
			// Color32 hoverColor = Color32(1.0f, 0.0f, 1.0f);
			float fMinMin=FLT_MAX;
			float fMaxMax=-FLT_MAX;
			atArray<float> textYPositions;
			for (int i=0 ; i<m_GraphLines.GetCount() ; i++)
			{
				if (ShouldDrawLine(m_GraphLines[i]))
				{
					TagSelection.m_bDrawBackground = m_iSelectedTag == i;
					Color32 lastColor = TagSelection.m_Color;

					TagSelection.m_bForceColor = true;
					int iColor = i+1;
					TagSelection.m_Color = Color32((iColor*769)&255, (iColor*991)&255, (iColor*383)&255);

					textYPositions.Grow(16) = TagSelection.m_fYPosition;
					if (TagSelection.AddLine("%s", m_GraphLines[i]->GetGraphLineName()) && bMouseDown)
					{
						bMouseDown = false;
						bSelected = true;
						m_iSelectedTag = i;
					}

					m_GraphLines[i]->ExtendValueRange(fMinMin, fMaxMax);

					TagSelection.m_bDrawBackground = false;
					TagSelection.m_bForceColor = false;
					TagSelection.m_Color = lastColor;
				}
			}


			if (mp_StoredGraph)
			{
				mp_StoredGraph->ExtendValueRange(fMinMin, fMaxMax);
			}

			//Move to bottom
			TagSelection.m_fYPosition = fGraphBottom;
			if (TagSelection.AddLine("[Clear selected]") && bMouseDown)
			{
				bMouseDown = false;
				bSelected = true;

				if (m_iSelectedTag>-1)
				{
					//Delete it
					delete m_GraphLines[m_iSelectedTag];

					//Copy down last pointer if needed
					m_GraphLines.DeleteFast(m_iSelectedTag);

					//Make sure we keep something selected
					--m_iSelectedTag;
				}
			}

			//grcViewport* prevViewport=grcViewport::GetCurrent();
			//grcViewport screenViewport;

			//grcViewport::SetCurrent(&screenViewport);
			//grcViewport::GetCurrent()->Screen();


			//Draw the frame
			grcColor(Color32(1.0f,1.0f,1.0f));
			grcBegin(drawLineStrip, 5);
			grcVertex2f(fGraphLeft, fGraphTop);
			grcVertex2f(fGraphRight, fGraphTop);
			grcVertex2f(fGraphRight, fGraphBottom);
			grcVertex2f(fGraphLeft, fGraphBottom);
			grcVertex2f(fGraphLeft, fGraphTop);
			grcEnd();

			bool bMouseInFrame =	(mouseScreenX >= fGraphLeft) 
								&&	(mouseScreenX <= fGraphRight)
								&&	(mouseScreenY >= fGraphTop)
								&&	(mouseScreenY <= fGraphBottom);
			
			//Then draw the graph for the selected tag
			const int iCount = m_GraphLines.GetCount();
			if ((iCount>0) || mp_StoredGraph)
			{
				//Get range of all graphs being drawn
				int iFrameMin = DebugRecorder::GetInstance()->GetFirstFramePtr()->m_FrameIndex;
				int iFrameMax = DebugRecorder::GetInstance()->GetLastFramePtr()->m_FrameIndex;

				if (m_fLineMarker >= 0)
				{
					grcColor(Color32(0.5f,0.5f,0.5f));
					grcBegin(drawLineStrip, 2);
					grcVertex2f(m_fLineMarker, fGraphTop);
					grcVertex2f(m_fLineMarker, fGraphBottom);
					grcEnd();
				}

				const EventBase* pClosestEvent=0;
				float fDistToMouse2 = FLT_MAX;	//Seed to restrict selection to items near mouse
				float fHoveredValue = 0.0f;
				m_iHovered = -1;
				int iDrawn=0;
				for (int i=0 ; i<iCount ; i++)
				{
					if (ShouldDrawLine(m_GraphLines[i]))
					{
						int iColor = i+1;
						Color32 lineColor((iColor*769)&255, (iColor*991)&255, (iColor*383)&255);

						switch(m_GraphLines[i]->GetGraphLineType())
						{
						case GraphLineBase::eValueLine: {
							float fMin = fMaxMax;
							float fMax = fMinMin;
							if (m_bShareScale)
							{
								fMin = fMinMin;
								fMax = fMaxMax;
							}
							else
							{
								m_GraphLines[i]->ExtendValueRange(fMin, fMax);
							}

							if (((GraphFloatLine*)m_GraphLines[i])->Draw(
									i == m_iSelectedTag, lineColor, iFrameMin, iFrameMax, 
									fGraphLeft+1, fGraphTop+1, fGraphRight-1, fGraphBottom-1, 
									fMin, fMax,
									mouseScreenX, mouseScreenY,
									pClosestEvent, fDistToMouse2, fHoveredValue,
									pCurrentSelected, pCurrentHighlighted, rZoomFactor
									)
								)
							{
								m_iHovered = i;
							}
							break;
						}
						case GraphLineBase::eTimeLine: {
							float fMin = textYPositions[iDrawn];
							float fMax = fMin + TagSelection.m_fPerLineYOffset;
							if (((GraphTimeLine*)m_GraphLines[i])->Draw(
									i, lineColor, iFrameMin, iFrameMax, 
									fMin, fMax, fGraphLeft+1, fGraphRight-1, 
									mouseScreenX, mouseScreenY,
									pClosestEvent, fDistToMouse2,
									pCurrentSelected, pCurrentHighlighted, rZoomFactor
									)
								)
							{
								m_iHovered = i;
							}
							break;
						}
						default:
							break;
						}

						++iDrawn;
					}
				}

				if (mp_StoredGraph)
				{
					Color32 lineColor(255, 255, 255);

					switch(mp_StoredGraph->GetGraphLineType())
					{
					case GraphLineBase::eValueLine: {
						float fMin = fMaxMax;
						float fMax = fMinMin;
						if (m_bShareScale)
						{
							fMin = fMinMin;
							fMax = fMaxMax;
						}
						else
						{
							mp_StoredGraph->ExtendValueRange(fMin, fMax);
						}
						((GraphFloatLine*)mp_StoredGraph)->Draw(
								false, lineColor, iFrameMin, iFrameMax, 
								fGraphLeft+1, fGraphTop+1, fGraphRight-1, fGraphBottom-1, 
								fMin, fMax,
								mouseScreenX, mouseScreenY,
								pClosestEvent, fDistToMouse2, fHoveredValue,
								pCurrentSelected, pCurrentHighlighted, rZoomFactor
							);
						break;
					}
					case GraphLineBase::eTimeLine:
						((GraphTimeLine*)mp_StoredGraph)->Draw(
							-1, lineColor, iFrameMin, iFrameMax, 
							fGraphTop+1, fGraphBottom-1, fGraphLeft+1, fGraphRight-1, 
							mouseScreenX, mouseScreenY,
							pClosestEvent, fDistToMouse2,
							pCurrentSelected, pCurrentHighlighted, rZoomFactor
							);
						break;
					default:
						break;
					}
				}
	
				if (pClosestEvent && bMouseInFrame)
				{
					//Make sure the main selection system knows we're hovering over this
					DebugRecorder::GetInstance()->SetHovered(pClosestEvent);

					//Draw the text value
					int iColor = m_iHovered + 1;
					grcColor(Color32((iColor*769)&255, (iColor*991)&255, (iColor*383)&255));
					grcDrawLabelf(Vector3(m_fX + m_fGraphOffset, fGraphTop + 10, 0.0f), "Value: %f", fHoveredValue);
					const Frame *pFrame = DebugRecorder::GetInstance()->FindFrame(pClosestEvent);
					if (pFrame)
					{
						pFrame->DebugDraw(eDrawNormal, mouseScreenX, mouseScreenY, bMouseDown, *DebugRecorder::GetInstance());
						grcDrawLabelf(Vector3(m_fX + m_fGraphOffset, fGraphTop + 20, 0.0f), "Frame: %d", pFrame->m_FrameIndex);
					}

					if (bMouseDown)
					{
						m_fLineMarker = mouseScreenX;
					}
				}
			}

			//grcViewport::SetCurrent(prevViewport);

			return bSelected;
		}

		void GraphFloats::Reset()			
		{
			for (int i=0 ; i<m_GraphLines.GetCount() ; i++)
			{
				m_GraphLines.Reset();
			}

			mp_CurrentEvent = 0;
			mp_LastSelected = 0;
			m_iSelectedTag = -1;
			m_fLineMarker = -1.0f;
			m_iHovered = -1;
			m_eDrawGraph = eDrawMode_Nothing;
		}

		class GraphFloatsNameArrayBuilder : public parInstanceVisitor
		{
		public:
			GraphFloatsNameArrayBuilder(atArray<ConstString> &rArray)
				:mp_NameArray(&rArray)
			{	}
		private:
			atArray<ConstString> *mp_NameArray;

			virtual void SimpleMember(parPtrToMember /*ptrToMember*/, parMemberSimple& metadata)
			{
				//Find graphable values....
				switch(metadata.GetType())
				{
				case parMemberType::TYPE_BOOL:
				case parMemberType::TYPE_CHAR:			// s8
				case parMemberType::TYPE_UCHAR:			// u8
				case parMemberType::TYPE_SHORT:			// s16
				case parMemberType::TYPE_USHORT:		// u16
				case parMemberType::TYPE_INT:
				case parMemberType::TYPE_UINT:
				case parMemberType::TYPE_SIZET:
				case parMemberType::TYPE_PTRDIFFT:
				case parMemberType::TYPE_FLOAT:
				case parMemberType::TYPE_FLOAT16:
					mp_NameArray->Grow(16) = metadata.GetName();
					break;
				default:
					break;
				}
			}

			virtual void VectorMember(parPtrToMember /*ptrToMember*/, parMemberVector& metadata)
			{
				//Find graphable values....
				switch(metadata.GetType())
				{
				case parMemberType::TYPE_VEC2V:
				case parMemberType::TYPE_VECTOR2:
					{
						char nameBuffer[64];
						formatf(nameBuffer, "%s.x", metadata.GetName());
						mp_NameArray->Grow(16) = nameBuffer;
						formatf(nameBuffer, "%s.y", metadata.GetName());
						mp_NameArray->Grow(16) = nameBuffer;
					}									
					break;
				case parMemberType::TYPE_VEC3V:
				case parMemberType::TYPE_VECTOR3:
					{
						char nameBuffer[64];
						formatf(nameBuffer, "%s.x", metadata.GetName());
						mp_NameArray->Grow(16) = nameBuffer;
						formatf(nameBuffer, "%s.y", metadata.GetName());
						mp_NameArray->Grow(16) = nameBuffer;
						formatf(nameBuffer, "%s.z", metadata.GetName());
						mp_NameArray->Grow(16) = nameBuffer;
					}									
					break;
				case parMemberType::TYPE_VEC4V:
				case parMemberType::TYPE_VECTOR4:
					{
						char nameBuffer[64];
						formatf(nameBuffer, "%s.x", metadata.GetName());
						mp_NameArray->Grow(16) = nameBuffer;
						formatf(nameBuffer, "%s.y", metadata.GetName());
						mp_NameArray->Grow(16) = nameBuffer;
						formatf(nameBuffer, "%s.z", metadata.GetName());
						mp_NameArray->Grow(16) = nameBuffer;
						formatf(nameBuffer, "%s.w", metadata.GetName());
						mp_NameArray->Grow(16) = nameBuffer;
					}									
					break;
				default:
					break;
				}
			}

			virtual void MatrixMember(parPtrToMember /*ptrToMember*/, parMemberMatrix& metadata)
			{
				//Find graphable values....
				switch(metadata.GetType())
				{
				case parMemberType::TYPE_MAT34V:
				case parMemberType::TYPE_MATRIX34:
					{
						char nameBuffer[64];
						formatf(nameBuffer, "%s.a.x", metadata.GetName());
						mp_NameArray->Grow(16) = nameBuffer;
						formatf(nameBuffer, "%s.a.y", metadata.GetName());
						mp_NameArray->Grow(16) = nameBuffer;
						formatf(nameBuffer, "%s.a.z", metadata.GetName());
						mp_NameArray->Grow(16) = nameBuffer;

						formatf(nameBuffer, "%s.b.x", metadata.GetName());
						mp_NameArray->Grow(16) = nameBuffer;
						formatf(nameBuffer, "%s.b.y", metadata.GetName());
						mp_NameArray->Grow(16) = nameBuffer;
						formatf(nameBuffer, "%s.b.z", metadata.GetName());
						mp_NameArray->Grow(16) = nameBuffer;

						formatf(nameBuffer, "%s.c.x", metadata.GetName());
						mp_NameArray->Grow(16) = nameBuffer;
						formatf(nameBuffer, "%s.c.y", metadata.GetName());
						mp_NameArray->Grow(16) = nameBuffer;
						formatf(nameBuffer, "%s.c.z", metadata.GetName());
						mp_NameArray->Grow(16) = nameBuffer;

						formatf(nameBuffer, "%s.d.x", metadata.GetName());
						mp_NameArray->Grow(16) = nameBuffer;
						formatf(nameBuffer, "%s.d.y", metadata.GetName());
						mp_NameArray->Grow(16) = nameBuffer;
						formatf(nameBuffer, "%s.d.z", metadata.GetName());
						mp_NameArray->Grow(16) = nameBuffer;
					}
					break;
				default:
					break;
				}
			}
		};

		void GraphFloats::BuildNameArray(const EventBase * pSelectedEvent)
		{
			GraphFloatsNameArrayBuilder visitor(m_NameArray);
			visitor.Visit(*const_cast<EventBase*>(pSelectedEvent));
		}

		class GraphFloatsGetValue : public parInstanceVisitor
		{
		public:
			GraphFloatsGetValue(int iIndex)
				:m_iValueIndex(iIndex)
				,m_iValuesRemaining(iIndex)
				,m_fValue(0.0f)
				,m_bHasResult(false)
			{	}
			void Reset()
			{
				m_iValuesRemaining = m_iValueIndex;
			}
			bool FoundValue() const
			{
				return m_bHasResult;		
			}
			float GetValue() const
			{
				return m_fValue;		
			}
		private:
			int m_iValueIndex;
			int m_iValuesRemaining;
			float m_fValue;
			bool m_bHasResult;

			virtual void SimpleMember(parPtrToMember ptrToMember, parMemberSimple& metadata)
			{
				//Find graphable values....
				switch(metadata.GetType())
				{
				case parMemberType::TYPE_BOOL:
					if (!m_iValuesRemaining)
					{
						m_bHasResult = true;
						m_fValue = *reinterpret_cast<bool*>(ptrToMember) ? 1.0f : 0.0f;
						break;
					}
					--m_iValuesRemaining;
					break;
				case parMemberType::TYPE_CHAR:			// s8
					if (!m_iValuesRemaining)
					{
						m_bHasResult = true;
						m_fValue = float(*reinterpret_cast<s8*>(ptrToMember));
						break;
					}
					--m_iValuesRemaining;
					break;
				case parMemberType::TYPE_UCHAR:			// u8
					if (!m_iValuesRemaining)
					{
						m_bHasResult = true;
						m_fValue = float(*reinterpret_cast<u8*>(ptrToMember));
						break;
					}
					--m_iValuesRemaining;
					break;
				case parMemberType::TYPE_SHORT:			// s16
					if (!m_iValuesRemaining)
					{
						m_bHasResult = true;
						m_fValue = float(*reinterpret_cast<s16*>(ptrToMember));
						break;
					}
					--m_iValuesRemaining;
					break;
				case parMemberType::TYPE_USHORT:		// u16
					if (!m_iValuesRemaining)
					{
						m_bHasResult = true;
						m_fValue = float(*reinterpret_cast<u16*>(ptrToMember));
						break;
					}
					--m_iValuesRemaining;
					break;
				case parMemberType::TYPE_INT:
					if (!m_iValuesRemaining)
					{
						m_bHasResult = true;
						m_fValue = float(*reinterpret_cast<s32*>(ptrToMember));
						break;
					}
					--m_iValuesRemaining;
					break;
				case parMemberType::TYPE_UINT:
					if (!m_iValuesRemaining)
					{
						m_bHasResult = true;
						m_fValue = float(*reinterpret_cast<u32*>(ptrToMember));
						break;
					}
					--m_iValuesRemaining;
					break;
				case parMemberType::TYPE_SIZET:
					if (!m_iValuesRemaining)
					{
						m_bHasResult = true;
						m_fValue = float(*reinterpret_cast<size_t*>(ptrToMember));
						break;
					}
					--m_iValuesRemaining;
					break;
				case parMemberType::TYPE_PTRDIFFT:
					if (!m_iValuesRemaining)
					{
						m_bHasResult = true;
						m_fValue = float(*reinterpret_cast<ptrdiff_t*>(ptrToMember));
						break;
					}
					--m_iValuesRemaining;
					break;
				case parMemberType::TYPE_FLOAT:
					if (!m_iValuesRemaining)
					{
						m_bHasResult = true;
						m_fValue = *reinterpret_cast<float*>(ptrToMember);
						break;
					}
					--m_iValuesRemaining;
					break;
				case parMemberType::TYPE_FLOAT16:
					if (!m_iValuesRemaining)
					{
						m_bHasResult = true;
						Float16 f16 = *reinterpret_cast<Float16*>(ptrToMember);
						m_fValue = f16.GetFloat32_FromFloat16();
						break;
					}
					--m_iValuesRemaining;
					break;
				default:
					break;
				}

				if (m_bHasResult)
				{
					EndTraversal();
				}
			}

			virtual void VectorMember(parPtrToMember ptrToMember, parMemberVector& metadata)
			{
				switch(metadata.GetType())
				{
				case parMemberType::TYPE_VEC2V:
				case parMemberType::TYPE_VECTOR2:
					if(m_iValuesRemaining < 2)
					{
						const Vector2 &rValue = *reinterpret_cast<Vector2*>(ptrToMember);
						switch(m_iValuesRemaining)
						{
						case 0: 
							m_bHasResult = true;
							m_fValue = rValue.x;
							break;
						case 1: 
							m_bHasResult = true;
							m_fValue = rValue.y;
							break;
						default:;
						}
					}
					m_iValuesRemaining -= 2;
					break;
				case parMemberType::TYPE_VEC3V:
				case parMemberType::TYPE_VECTOR3:
					if(m_iValuesRemaining < 3)
					{
						const Vector3 &rValue = *reinterpret_cast<Vector3*>(ptrToMember);
						switch(m_iValuesRemaining)
						{
						case 0: 
							m_bHasResult = true;
							m_fValue = rValue.x;
							break;
						case 1: 
							m_bHasResult = true;
							m_fValue = rValue.y;
							break;
						case 2: 
							m_bHasResult = true;
							m_fValue = rValue.z;
							break;
						default:;
							break;
						}
					}
					m_iValuesRemaining -= 3;
					break;
				case parMemberType::TYPE_VEC4V:
				case parMemberType::TYPE_VECTOR4:
					if(m_iValuesRemaining < 4)
					{
						const Vector4 &rValue = *reinterpret_cast<Vector4*>(ptrToMember);
						switch(m_iValuesRemaining)
						{
						case 0: 
							m_bHasResult = true;
							m_fValue = rValue.x;
							break;
						case 1: 
							m_bHasResult = true;
							m_fValue = rValue.y;
							break;
						case 2: 
							m_bHasResult = true;
							m_fValue = rValue.z;
							break;
						case 3: 
							m_bHasResult = true;
							m_fValue = rValue.w;
							break;
						default:;
							break;
						}
						m_iValuesRemaining -= 4;
					}
					break;
				default:
					break;
				}

				if (m_bHasResult)
				{
					EndTraversal();
				}
			}

			virtual void MatrixMember(parPtrToMember ptrToMember, parMemberMatrix& metadata)
			{
				switch(metadata.GetType())
				{
				case parMemberType::TYPE_MAT34V:
				case parMemberType::TYPE_MATRIX34:
					if (m_iValuesRemaining < 12)
					{
						const Matrix34 &rValue = *reinterpret_cast<Matrix34*>(ptrToMember);
						switch(m_iValuesRemaining)
						{
						case 0: 
							m_bHasResult = true;
							m_fValue = rValue.a.x;
							break;
						case 1: 
							m_bHasResult = true;
							m_fValue = rValue.a.y;
							break;
						case 2: 
							m_bHasResult = true;
							m_fValue = rValue.a.z;
							break;
						case 3: 
							m_bHasResult = true;
							m_fValue = rValue.b.x;
							break;
						case 4: 
							m_bHasResult = true;
							m_fValue = rValue.b.y;
							break;
						case 5: 
							m_bHasResult = true;
							m_fValue = rValue.b.x;
							break;
						case 6: 
							m_bHasResult = true;
							m_fValue = rValue.c.x;
							break;
						case 7: 
							m_bHasResult = true;
							m_fValue = rValue.c.y;
							break;
						case 8: 
							m_bHasResult = true;
							m_fValue = rValue.c.x;
							break;
						case 9: 
							m_bHasResult = true;
							m_fValue = rValue.d.x;
							break;
						case 10: 
							m_bHasResult = true;
							m_fValue =  rValue.d.y;
							break;
						case 11: 
							m_bHasResult = true;
							m_fValue =  rValue.d.z;
							break;
						default:;
							break;
						}
					}
					m_iValuesRemaining -= 12;
					break;
				default:
					break;
				}

				if (m_bHasResult)
				{
					EndTraversal();
				}
			}
		};

		float GraphFloats::GetFloatValue(const EventBase * pEvent, int iValue) const
		{
			GraphFloatsGetValue visitor(iValue);
			visitor.Visit(*const_cast<EventBase*>(pEvent));
			Assertf(visitor.FoundValue(), "Failed to find result %d", iValue);
			return visitor.GetValue();
		}

		int GraphFloats::GetGraphIndex(const char *pType, const char *pValueName) const
		{
			for (int i=0; i<m_GraphLines.GetCount() ; i++)
			{
				if (m_GraphLines[i]->Matches(pType, pValueName))
				{
					return i;
				}
			}
			return -1;
		}

		GraphFloats::GraphLineBase *GraphFloats::CreateGraphLine(const EventBase &rEventSelected, const char *pEventType, const char *pValueName, int iValue) const
		{
			DR_MEMORY_HEAP();
			GraphFloats::GraphLineBase *pGraphLine = 0;
			if (pValueName)
			{
				Assert(iValue >= 0);
				pGraphLine = rage_new GraphFloats::GraphFloatLine(pEventType, pValueName);
				const Frame *pFrame = DebugRecorder::GetInstance()->GetFirstFramePtr();
				while (pFrame)
				{
					s32 iFrame = pFrame->m_FrameIndex;
					const EventBase *pEvent = pFrame->mp_FirstEvent;
					while (pEvent)
					{
						if (pEvent->m_bIsDisplayEnabled && (pEvent->GetEventSubType() == pEventType))
						{
							if (rEventSelected.IsEventDataRelated(*pEvent))
							{
								float fNewValue = GetFloatValue(pEvent, iValue);
								pGraphLine->Add(pEvent, fNewValue, iFrame);
							}
						}
						pEvent = pEvent->mp_Next;
					}
					pFrame = pFrame->mp_Next;
				}
			}
			else
			{
				//Add timeline
				pGraphLine = rage_new GraphFloats::GraphTimeLine(pEventType);
				const Frame *pFrame = DebugRecorder::GetInstance()->GetFirstFramePtr();
				while (pFrame)
				{
					s32 iFrame = pFrame->m_FrameIndex;
					const EventBase *pEvent = pFrame->mp_FirstEvent;
					while (pEvent)
					{
						if (pEvent->m_bIsDisplayEnabled && (pEvent->GetEventSubType() == pEventType))
						{
							if (rEventSelected.IsEventDataRelated(*pEvent))
							{
								pGraphLine->Add(pEvent, 0.0f, iFrame);
							}
						}
						pEvent = pEvent->mp_Next;
					}
					pFrame = pFrame->mp_Next;
				}
				
			}
			return pGraphLine;
		}

		void GraphFloats::TogglePermanentValue(const EventBase &rEventSelected, const char *pEventType, const char *pValueName, int iValueIndex)
		{
			//Do we have a graph already?
			int iCurrentIndex = GetGraphIndex(pEventType, pValueName);
			if (iCurrentIndex<0)
			{
				//Create a new graph line
				GraphFloats::GraphLineBase *pNewLine = CreateGraphLine(rEventSelected, pEventType, pValueName, iValueIndex);

				//Add graphline to graph list
				m_GraphLines.Grow() = pNewLine;

				//Force drawing on when adding data
				switch(m_eDrawGraph)
				{
				case eDrawMode_Values:
					if (!pValueName)
					{
						m_eDrawGraph = eDrawMode_All;
					}
					break;
				case eDrawMode_TimeLine:
					if (pValueName)
					{
						m_eDrawGraph = eDrawMode_All;
					}
					break;
				default:
					m_eDrawGraph = pValueName ? eDrawMode_Values : eDrawMode_TimeLine;
				}
			}
			else
			{
				//Delete it
				delete m_GraphLines[iCurrentIndex];

				//Copy down last pointer if needed
				m_GraphLines.DeleteFast(iCurrentIndex);

				///Move selected tag
				if (m_iSelectedTag == iCurrentIndex)
				{
					--m_iSelectedTag;
				}
			}
		}

		void GraphFloats::SetTempValue(const EventBase &rSelectedEvent, const char *pEventTypeName, const char *pValueName, int iEventValue)
		{
			DR_MEMORY_HEAP();

			if (mp_StoredGraph)
			{
				delete mp_StoredGraph;
			}
			mp_StoredGraph = CreateGraphLine(rSelectedEvent, pEventTypeName, pValueName, iEventValue);
		}

		bool GraphFloats::DisplayEventDetails(OnScreenOutput &rTextWindow, const EventBase * pSelected, bool bMouseDown)
		{
			bool bSelected = false;
			//Make sure our list is up to date
			if (mp_LastSelected != pSelected)
			{
				m_NameArray.Reset();
				if (pSelected)
				{
					BuildNameArray(pSelected);
				}
				mp_LastSelected = pSelected;
			}

			if (!mp_LastSelected)
				return bSelected;

			const char *pEventTypeName = pSelected->GetEventSubType();

			//Add the timeline option
			int iGraphIndex = GetGraphIndex(pEventTypeName, 0);
			bool bTempSet = false;
			rTextWindow.m_bDrawBackground = IsInGraph(pEventTypeName, 0);
			if (rTextWindow.AddLine("[TimeLine[%s]]", iGraphIndex >= 0 ? "X" : " ") && bMouseDown)
			{
				if (bMouseDown)
				{
					//Toggle a event timeline graph
					TogglePermanentValue(*pSelected, pEventTypeName, 0, -1);
					bSelected = true;
					bMouseDown = false;
				}
				else
				{
					//Toggle a event timeline graph
					SetTempValue(*pSelected, pEventTypeName, 0, -1);
					bTempSet = true;
				}
			}
			rTextWindow.m_bDrawBackground = false;

			//Draw all the names
			int iEventMousedOver = -1;
			int iEventElementSelected = -1;
			for (int i=0 ; i<m_NameArray.GetCount() ; i++)
			{
				rTextWindow.m_bDrawBackground = IsInGraph(pEventTypeName, m_NameArray[i]);

				if (rTextWindow.AddLine(pSelected->GetValueName(m_NameArray[i].c_str())))
				{
					bSelected = true;
					if (bMouseDown)
					{
						bMouseDown = false;
						iEventElementSelected = i;
					}
					else
					{
						iEventMousedOver = i;
					}
				}
				rTextWindow.m_bDrawBackground = false;
			}

			if (bSelected)
			{
				if (iEventElementSelected > -1)
				{
					//Toggle entry in graph so it displays event when not highlighted
					TogglePermanentValue(*pSelected, pEventTypeName, pSelected->GetValueName(m_NameArray[iEventElementSelected]), iEventElementSelected);
				}
				else if (iEventMousedOver > -1)
				{
					SetTempValue(*pSelected, pEventTypeName, pSelected->GetValueName(m_NameArray[iEventMousedOver]), iEventMousedOver);
					bTempSet = true;
				}
			}

			if (!bTempSet)
			{
				ClearTempValue();
			}

			return bSelected;
		}

		void TotalEventTimeLine::Reset()
		{
			if (m_bInited)
			{
				m_iMaxEventCount = 0;
				m_Frames.Reset();
				m_FrameEventCounts.Reset();
				m_FrameSelectedDataCounts.Reset();
				m_bInited = false;
			}
		}
		
		void TotalEventTimeLine::SetSelectedDataCounts(const DataBase *pData)
		{
			for (int i=0 ; i<m_Frames.GetCount() ; i++)
			{
				m_FrameSelectedDataCounts[i] = 0;
				if (pData)
				{
					const EventBase *pEvent = m_Frames[i]->mp_FirstEvent;
					while (pEvent)
					{
						if (pEvent->IsDataInvolved(*pData))
						{
							m_FrameSelectedDataCounts[i]++;
						}
						pEvent = pEvent->mp_Next;
					}
				}
			}
		}

		void TotalEventTimeLine::Refresh(const Frame * const pFirstFrame)
		{
			int iFrameCount = 0;
			const Frame* pFrame = pFirstFrame;
			while (pFrame)
			{
				++iFrameCount;
				pFrame = pFrame->mp_Next;
			}
			m_Frames.Reset();
			m_FrameEventCounts.Reset();
			m_FrameSelectedDataCounts.Reset();
			m_Frames.Resize( iFrameCount );
			m_FrameSelectedDataCounts.Resize( iFrameCount );
			m_FrameEventCounts.Resize( iFrameCount );

			pFrame = pFirstFrame;
			m_iMaxEventCount = 0;
			int iCount = 0;
			while (pFrame)
			{
				int iEventCount = 0;
				const EventBase *pEvent = pFrame->mp_FirstEvent;
				while (pEvent)
				{
					++iEventCount;
					pEvent = pEvent->mp_Next;
				}

				m_Frames[iCount] = pFrame;
				m_FrameSelectedDataCounts[iCount] = 0;
				Assign(m_FrameEventCounts[iCount], iEventCount);
				m_iMaxEventCount = Max(m_iMaxEventCount, iEventCount);
				++iCount;
				pFrame = pFrame->mp_Next;
			}

			m_bInited = true;
		}

		void TimeLineZoom::UpdateZoom(float xMin, float xMax, float fMouse, float fMouseOffset)
		{
			if (ioMouse::HasWheel() && (ioMouse::GetDZ()!=0))
			{
				const float kMouseZoomFactor = 1.25f;
				float fOldZoom = m_fZoom;
				m_fZoom *= ioMouse::GetDZ() > 0 ? kMouseZoomFactor : 1.0f / kMouseZoomFactor;
				m_fZoom = Clamp(m_fZoom, 1.0f, 1000.0f);
				m_fTimeLineOffset += (xMax - xMin) * (fOldZoom - m_fZoom) * fMouseOffset;
			}
			else if (ioMouse::GetButtons() & ioMouse::MOUSE_LEFT)
			{
				if (m_LastMouseTrackFrame == (TIME.GetFrameCount() - 1))
				{
					float fDeltaX = fMouse - m_fLastMouseX;
					m_fTimeLineOffset += fDeltaX;
				}

				m_LastMouseTrackFrame = TIME.GetFrameCount();
				m_fLastMouseX = fMouse;
			}

			//Need to clamp this to the edges of the graph somehow.
			//float x = xMin + (xMax - xMin) * rZoom.m_fZoom * fGraphOffset + rZoom.m_fTimeLineOffset;
		}

		const Frame* TotalEventTimeLine::Display(float xMin, float yMin, float xMax, float yMax, float fMouseX, float fMouseY, Color32 col, TimeLineZoom &rZoom)
		{
			//Early out if mouse isn't in the area
			bool bOnARea = (fMouseX >= xMin) && (fMouseX <= xMax) && (fMouseY >= yMin) && (fMouseY <= yMax);
			if (!bOnARea && !m_bForceDrawGraph)
			{
				return 0;
			}

			//grcViewport* prevViewport=grcViewport::GetCurrent();
			//grcViewport screenViewport;

			//grcViewport::SetCurrent(&screenViewport);
			//grcViewport::GetCurrent()->Screen();

			grcColor(Color32(128,128,128,128));
			grcBegin(drawLineStrip, 5);
			grcVertex2f(xMin, yMin);
			grcVertex2f(xMax, yMin);
			grcVertex2f(xMax, yMax);
			grcVertex2f(xMin, yMax);
			grcVertex2f(xMin, yMin);
			grcEnd();

			//Shrink a little to avoid overlapping border
			xMin+=1;
			xMax-=1;
			yMin+=1;
			yMax-=1;

			const Frame* pHovered = 0;
			grcColor(col);
			const float kfMaxFrames = float(m_Frames.GetCount());
			const float kfInvMaxFrames = 1.0f / kfMaxFrames;
			const float kfInvMaxEventCount = 1.0f / Sqrtf(float(m_iMaxEventCount));

			//float fClosestTop=0.0f;
			//float fClosestX=0.0f;
			float fDistance=100.0f;
			const float kfBarSize = Max(1.0f, (xMax - xMin) * kfInvMaxFrames * 0.75f * rZoom.m_fZoom);
			float fMouseOffset = 0.0f;

			for (int i=0 ; i<m_Frames.GetCount() ; i++)
			{
				int iMaxFrameEvents=m_FrameEventCounts[i];
				const Frame *pFrame=m_Frames[i];
				float fGraphOffset = (float(i) * kfInvMaxFrames);
				float x = xMin + (xMax - xMin) * rZoom.m_fZoom * fGraphOffset + rZoom.m_fTimeLineOffset;
				if ((x >= xMin) && ((x+kfBarSize) <= xMax))
				{
					//Frames can get drawn on top of each other, the one with the highest frame count in this 
					//section is what we'll draw
					Color32 lineCol = col;
					u32 iMaxDisplayedFrameEvents = 0;
					u16 iMaxSelectedDataEvents = 0;
					bool bHovering = false;
					bool bSelected = false;
					while (i<m_Frames.GetCount())
					{
						iMaxDisplayedFrameEvents = Max(m_Frames[i]->m_DisplayCount, iMaxDisplayedFrameEvents);
						iMaxSelectedDataEvents = Max(m_FrameSelectedDataCounts[i], iMaxSelectedDataEvents);
						if (m_FrameEventCounts[i] > iMaxFrameEvents)
						{
							iMaxFrameEvents = m_FrameEventCounts[i];
							pFrame = m_Frames[i];	//Consider the largest frame the hovered frame
						}
						if (m_Frames[i] == DebugRecorder::GetInstance()->GetHoveredFrame())
						{
							bHovering = true;
						}
						else if (m_Frames[i] == DebugRecorder::GetInstance()->GetSelectedFrame())
						{
							bSelected = true;
						}
						if (floor((xMin + (xMax - xMin) * rZoom.m_fZoom * float(i+1) * kfInvMaxFrames) + rZoom.m_fTimeLineOffset) > floor(x))
						{
							break;
						}

						i++;
					}

					//if(i==m_Frames.GetCount())
					//{
					//	break;
					//}

					if (iMaxDisplayedFrameEvents > 0)
					{
						bool bOverrideColor = false;
						if (bHovering)
						{
							bOverrideColor = true;
							lineCol.Set(255,255,0,255);
						}
						else if (bSelected)
						{
							if (((TIME.GetFrameCount() & 0x7) > 0x3))
							{
								bOverrideColor = true;
								lineCol.Set(255,0,255,255);
							}
						}
						float top = yMax - (yMax - yMin) * Sqrtf(float(iMaxDisplayedFrameEvents)) * kfInvMaxEventCount;
						float topTotal = yMax - (yMax - yMin) * Sqrtf(float(iMaxFrameEvents)) * kfInvMaxEventCount;
						float topdisplayed = yMax - (yMax - yMin) * Sqrtf(float(iMaxSelectedDataEvents)) * kfInvMaxEventCount;
						if (bOnARea)
						{
							float fDist = Abs(x  - fMouseX);
							if (fDist < fDistance)
							{
								fDistance = fDist;
								//fClosestTop = top;
								//fClosestX = x;
								pHovered = pFrame;
								fMouseOffset = fGraphOffset;
							}
						}

						if (topTotal < top)
						{
							Vector3 points[4];
							points[0].Set(x, topTotal, 0.0f);
							points[1].Set(x+kfBarSize, topTotal, 0.0f);
							points[2].Set(x+kfBarSize, top, 0.0f);
							points[3].Set(x, top, 0.0f);
							grcDrawPolygon(points, 4, NULL, true, Color32(128,128,128,128));
						}

						if (top < yMax)
						{
							Vector3 points[4];
							points[0].Set(x, top, 0.0f);
							points[1].Set(x+kfBarSize, top, 0.0f);
							points[2].Set(x+kfBarSize, yMax, 0.0f);
							points[3].Set(x, yMax, 0.0f);
							grcDrawPolygon(points, 4, NULL, true, lineCol);
						}

						if (!bOverrideColor && iMaxSelectedDataEvents)
						{
							Vector3 points[4];
							points[0].Set(x, topdisplayed, 0.0f);
							points[1].Set(x+kfBarSize, topdisplayed, 0.0f);
							points[2].Set(x+kfBarSize, yMax, 0.0f);
							points[3].Set(x, yMax, 0.0f);
							grcDrawPolygon(points, 4, NULL, true, Color32(200,64,64,255));
						}
					}
				}
			}

			if (bOnARea)
			{
				rZoom.UpdateZoom(xMin, xMax, fMouseX, fMouseOffset);
			}

			Vector3 tpLeft(xMin, yMin, 0.0f);
			grcColor(Color32(128,128,128));
			grcDrawLabelf(tpLeft, "Zoom: %f, XOffset: %f", rZoom.m_fZoom, rZoom.m_fTimeLineOffset);

			//grcViewport::SetCurrent(prevViewport);

			//Allow higher level code to know we're hovering on a frame
			return pHovered;
		}

		void DebugRecorder::Load(const char *pFileName)
		{
			DR_MEMORY_HEAP();

			sysCriticalSection critSection(m_DataCSToken);

			//Clear everything
			ClearRecording();
			
			if (!mp_SaveData)
			{
				mp_SaveData = CreateSaveData();
			}

			bool bLoaded = false;
#if USE_BINARY
			psoFile * pPsoFile = psoLoadFile(pFileName);
			Assertf(pPsoFile, "Couldn't locate %s", pFileName);
			if(pPsoFile)
			{
				bLoaded = psoLoadObject(*pPsoFile, *this);
				delete pPsoFile;
			}
#else
			bLoaded = PARSER.LoadObject(pFileName, "xml", *this);
#endif
			if (bLoaded)
			{
				//Link the Frames
				EventBase::sm_EventCount = 0;	//TMS: Hacking, we're going to manually re-init because the PSO load is going to skip ctors it seems
				int iTotalEventCount=0;
				for (int iFrame=0 ; iFrame<mp_SaveData->m_Frames.GetCount() ; iFrame++)
				{
					const Frame::FrameData &rFrameData = mp_SaveData->m_Frames[iFrame];
					Frame *pFrame = rage_new Frame;
					pFrame->mp_Prev = mp_LastFrame;
					if (mp_LastFrame)
					{
						mp_LastFrame->mp_Next = pFrame;
						mp_LastFrame = pFrame;
					}
					else
					{
						mp_LastFrame = mp_FirstFrame = pFrame;
					}
					pFrame->m_FrameIndex = rFrameData.m_FrameIndex;

					if (iFrame == mp_SaveData->m_SelectedFrameIndex)
					{
						mp_SelectedFrame = pFrame;
					}
					if (iFrame == mp_SaveData->m_AnchorFrameIndex)
					{
						mp_AnchorFrame = pFrame;
					}

					const int iNumEventsInFrame = rFrameData.m_pEvents.GetCount();
					if (iNumEventsInFrame>0)
					{
						pFrame->mp_FirstEvent = rFrameData.m_pEvents[0];
						pFrame->mp_LastEvent = rFrameData.m_pEvents[iNumEventsInFrame-1];
						for (int iEvent = 0 ; iEvent<iNumEventsInFrame ; iEvent++)
						{
							EventBase *pEvent = rFrameData.m_pEvents[iEvent];
							++EventBase::sm_EventCount;

							pEvent->mp_Prev = iEvent ? rFrameData.m_pEvents[iEvent-1] : 0;
							pEvent->mp_Next = iEvent < (iNumEventsInFrame - 1) ? rFrameData.m_pEvents[iEvent+1] : 0;

							if (iTotalEventCount == mp_SaveData->m_SelectedEventIndex)
							{
								mp_SelectedEvent = pEvent;
								if (mp_SelectedEvent)
								{
									mp_SelectedEvent->OnSelection();
								}
							}

							if (iTotalEventCount == mp_SaveData->m_AnchorEventIndex)
							{
								mp_AnchorEvent = pEvent;
							}
							++iTotalEventCount;
						}
					}
					else
					{
						pFrame->mp_FirstEvent = pFrame->mp_LastEvent = 0;
					}
				}
			
				//Copy recorded data across
				for (int i=0 ; i<mp_SaveData->m_RecordedDataArray.GetCount() ; i++ )
				{
					m_RecordedData.Insert( mp_SaveData->m_RecordedDataIDArray[i], mp_SaveData->m_RecordedDataArray[i] );
				}

				//Copy callstacks across
				sysCriticalSection section(m_CallstackCSToken);
				for (int i=0 ; i<mp_SaveData->m_SavedCallstacks.GetCount() ; i++ )
				{
					m_Callstacks.Insert( mp_SaveData->m_SavedCallstacks[i]->m_iSignature, mp_SaveData->m_SavedCallstacks[i] );
				}

				OnLoad(*mp_SaveData);

				//Rebuild the display list
				ValidateAndBuildEventDisplayList();
			}

			//Delete temp storage
			mp_SaveData->DeleteTempIOMemory();

			//Force display on
			m_bDisplayOn = true;
		}

		Frame *DebugRecorder::GetOrAllocateCurrentFrame()
		{
			DR_MEMORY_HEAP();

			u32 iCurrentFrame = GetRecorderFrameCount();
			m_iLastRecordedFrame = iCurrentFrame;
			if (!mp_LastFrame)
			{
				//First frame to record
				Frame *pNewFrame = rage_new Frame;
				mp_FirstFrame = mp_LastFrame = pNewFrame;
				pNewFrame->m_FrameIndex = iCurrentFrame;
				pNewFrame->m_NetworkTime = m_iNetworkTime;
				return pNewFrame;
			}

			//Are we adding to the current frame?
			if (mp_LastFrame->m_FrameIndex == iCurrentFrame)
			{
				return mp_LastFrame;
			}

			//Need to append a new frame to the end of the list and modify the list pointers
			Frame *pNewFrame = rage_new Frame;
			pNewFrame->m_FrameIndex = iCurrentFrame;
			mp_LastFrame->mp_Next = pNewFrame;
			pNewFrame->mp_Prev = mp_LastFrame;
			pNewFrame->m_NetworkTime = m_iNetworkTime;
			mp_LastFrame = pNewFrame;
			return pNewFrame;
		}

		void DebugRecorder::SetHovered(const EventBase *pHoveredEvent)
		{
			mp_HoveredEvent = pHoveredEvent;
			if (mp_HoveredEvent)
			{
				mp_HoveredFrame = 0;
			}
		}

		void DebugRecorder::SetHovered(const Frame *pSelectedFrame)
		{
			if (mp_HoveredFrame)
			{
				mp_HoveredEvent = 0;
			}
			mp_HoveredFrame = pSelectedFrame;
		}

		const Frame* DebugRecorder::FindFrame(const EventBase *pFrameForThisEvent) const
		{
			const Frame *pFrame = mp_FirstFrame;
			while (pFrame)
			{
				//Use the frame event list to skip whole frames fast
				if (pFrame->mp_LastEvent && (pFrame->mp_LastEvent->m_iEventIndex >= pFrameForThisEvent->m_iEventIndex))
				{
					const EventBase *pEvent = pFrame->mp_FirstEvent;
					while (pEvent)
					{
						if (pEvent == pFrameForThisEvent)
						{
							return pFrame;
						}
						pEvent = pEvent->mp_Next;
					}
				}
				pFrame = pFrame->mp_Next;
			}
			return 0;
		}
		
		DebugRecorder::DebugRecorderData::DebugRecorderData()
			:m_SelectedFrameIndex(-1)
			,m_SelectedEventIndex(-1)
			,m_AnchorFrameIndex(-1)
			,m_AnchorEventIndex(-1)
		{	}

		void DebugRecorder::Select(const Frame *pSelectedFrame, const EventBase *pHoveredEvent, bool bAutoSelectNewEvent, bool bReAnchorFrameList)
		{
			if (!pSelectedFrame && pHoveredEvent)
			{
				//Clear hovered event if clicked on (toggle)
				if (pHoveredEvent == mp_SelectedEvent)
				{
					mp_SelectedEvent = 0;
					OnEventSelection(0);
					return;
				}

				//Find the frame
				pSelectedFrame = FindFrame(pHoveredEvent);

				if (!Verifyf(pSelectedFrame, "Failed to find frame for selected event!"))
				{
					return;
				}
			}
			else if (pSelectedFrame && (pSelectedFrame == mp_SelectedFrame))
			{
				//Clear selected frame if clicked on
				mp_SelectedFrame = 0;
				return;
			}

			if (mp_SelectedFrame != pSelectedFrame)
			{
				mp_AnchorEvent = pSelectedFrame->mp_FirstEvent;
			}

			const EventBase *pOldEvent = mp_SelectedEvent;
			const Frame *pOldFrame = mp_SelectedFrame;

			mp_SelectedFrame = pSelectedFrame;
			if (bReAnchorFrameList)
			{
				mp_AnchorFrame = mp_SelectedFrame;
			}
			mp_SelectedEvent = pHoveredEvent;

			//Auto select mode will select the first event with a matching callstack to the
			//previously selected event
			if (bAutoSelectNewEvent && mp_SelectedFrame && !mp_SelectedEvent)
			{
				if (pOldEvent && (pOldFrame != pSelectedFrame))
				{
					Displayf("Auto selecting event to match callstack %" SIZETFMT "x", pOldEvent->m_Callstack);
					const EventBase *pTest = mp_SelectedFrame->mp_FirstEvent;
					while (pTest)
					{
						if (pTest->m_Callstack == pOldEvent->m_Callstack)
						{
							mp_SelectedEvent = pTest;
							break;
						}
						pTest = pTest->mp_Next;
					}
				}
			}

			if (mp_SelectedEvent)
			{
				mp_SelectedEvent->OnSelection();
			}
			OnEventSelection(mp_SelectedEvent);
		}

		void DebugRecorder::ScrollFrameNext()
		{
			if (mp_AnchorFrame && mp_AnchorFrame->mp_Next)
			{
				mp_AnchorFrame = mp_AnchorFrame->mp_Next;
			}
		}

		void DebugRecorder::ScrollFramePrev()
		{
			if (mp_AnchorFrame && mp_AnchorFrame->mp_Prev)
			{
				mp_AnchorFrame = mp_AnchorFrame->mp_Prev;
			}
		}

		void DebugRecorder::ScrollEventNext()
		{
			if (mp_AnchorEvent && mp_AnchorEvent->mp_Next)
			{
				mp_AnchorEvent = mp_AnchorEvent->mp_Next;
			}
		}

		void DebugRecorder::ScrollEventPrev()
		{
			if (mp_AnchorEvent && mp_AnchorEvent->mp_Prev)
			{
				mp_AnchorEvent = mp_AnchorEvent->mp_Prev;
			}
		}

		void DebugRecorder::AddSharedData(DataBase &rRecordedData, size_t id)
		{
			if (!m_RecordedData.Access(id))
			{
				m_RecordedData[id] = &rRecordedData;
			}
		}
		
		DataBase *DebugRecorder::GetSharedData(size_t dataID)
		{
			DataBase **ppData = m_RecordedData.Access(dataID);
			return ppData ? *ppData : 0;
		}

		static parTreeNode *spNode=0;
		void DataBase::OnPreLoad(parTreeNode *pNode)
		{
			spNode = pNode;
		}

		void DataBase::OnPostLoad()
		{
			delete spNode;	//TMS: Try and save as much data as possible
			spNode = 0;
		}

		const DataBase *DebugRecorder::GetSharedData(size_t dataID) const
		{
			DataBase *const *ppData = m_RecordedData.Access(dataID);
			return ppData ? *ppData : 0;
		}

		EventBase::EventBase(const CallstackHelper rCallstack)
			:mp_Next(0)
			,mp_Prev(0)
			,m_bIsDisplayEnabled(true)
		{
			m_Callstack = rCallstack.m_iCallstackID;
			++sm_EventCount;
		}

		EventBase::EventBase()
			:mp_Next(0)
			,mp_Prev(0)
			,m_bIsDisplayEnabled(true)
		{
			m_Callstack = 0;
			++sm_EventCount;
		}

		bool EventBase::DrawListText(TextOutputVisitor &rText) const
		{
			if (smb_DrawIndex)
			{
				return rText.AddLine("%d - %s", m_iEventIndex, GetEventSubType());
			}
			return rText.AddLine(GetEventSubType());
		}

		void EventBase::DebugText(TextOutputVisitor &rText) const
		{
			rText.AddLine("---------------------");
			rText.AddLine("Event index [%d]", m_iEventIndex);
			rText.AddLine("Event type: %s", GetEventSubType());
			rText.AddLine("---------------------");
			rText.PushIndent();
			if (DebugEventText(rText))
			{
				rText.AddLine("---------------------");
			}
			rText.PopIndent();

			if (m_Callstack != 0)
			{
				rText.AddLine("Callstack:");
				rText.PushIndent();
				DebugRecorder *pInstance = DebugRecorder::GetInstance();
				if (pInstance)
				{
					Callstack *pCallstack = pInstance->GetCallstack(m_Callstack);
					if (pCallstack)
					{
						pCallstack->Print(rText);
					}
				}
				rText.PopIndent();
			}
			rText.AddLine("---------------------");
		}
		
		static const u32 ksiEventIndexHash = atLiteralStringHash("iEventIndex");
		static const u32 ksiEventCallstackHash = atLiteralStringHash("Callstack");
		static const u32 ksiEventAsU32Hash = atLiteralStringHash("AsU32");
		class EventDataVisitor : public parInstanceVisitor 
		{
			bool m_bPrinted;
			TextOutputVisitor *m_pOutput;
		public:
			EventDataVisitor(TextOutputVisitor &rOutput)
				:m_pOutput(&rOutput)
				,m_bPrinted(false)
			{

			}

			bool HasPrinted() const
			{
				return m_bPrinted;
			}

			virtual void MatrixMember(parPtrToMember ptrToMember, parMemberMatrix& metadata)
			{
				switch ( metadata.GetType() )
				{
				case parMemberType::TYPE_MAT34V:
				case parMemberType::TYPE_MATRIX34:
					{
						const Matrix34 &rValue = *reinterpret_cast<Matrix34*>(ptrToMember);
						m_pOutput->AddLine("%s.a: %f, %f, %f", metadata.GetName(), rValue.a.x, rValue.a.y, rValue.a.z);
						m_pOutput->AddLine("%s.b: %f, %f, %f", metadata.GetName(), rValue.b.x, rValue.b.y, rValue.b.z);
						m_pOutput->AddLine("%s.c: %f, %f, %f", metadata.GetName(), rValue.c.x, rValue.c.y, rValue.c.z);
						m_pOutput->AddLine("%s.d: %f, %f, %f", metadata.GetName(), rValue.d.x, rValue.d.y, rValue.d.z);
						m_bPrinted = true;
					}
					break;
				default:
					Assertf(0, "Unhandled matrix type %d for member %s", metadata.GetType(), metadata.GetName());
					break;
				}
			}

			virtual void VectorMember(parPtrToMember ptrToMember, parMemberVector& metadata)
			{
				switch ( metadata.GetType() )
				{
				case parMemberType::TYPE_VEC2V:
				case parMemberType::TYPE_VECTOR2:
					{
						const Vector2 &rValue = *reinterpret_cast<Vector2*>(ptrToMember);
						m_pOutput->AddLine("%s: %f, %f", metadata.GetName(), rValue.x, rValue.y);
						m_bPrinted = true;
					}
					break;
				case parMemberType::TYPE_VEC3V:
				case parMemberType::TYPE_VECTOR3:
					{
						const Vector3 &rValue = *reinterpret_cast<Vector3*>(ptrToMember);
						m_pOutput->AddLine("%s: %f, %f, %f", metadata.GetName(), rValue.x, rValue.y, rValue.z);
						m_bPrinted = true;
					}
					break;
				case parMemberType::TYPE_VEC4V:
				case parMemberType::TYPE_VECTOR4:
					{
						const Vector4 &rValue = *reinterpret_cast<Vector3*>(ptrToMember);
						m_pOutput->AddLine("%s: %f, %f, %f, %f", metadata.GetName(), rValue.x, rValue.y, rValue.z, rValue.w);
						m_bPrinted = true;
					}
					break;
				default:
					Assertf(0, "Unhandled vector type %d for member %s", metadata.GetType(), metadata.GetName());
					break;
				}
			}

			virtual void StringMember(parPtrToMember /*ptrToMember*/, const char* ptrToString, parMemberString& metadata)
			{
				m_pOutput->AddLine("%s: %s", metadata.GetName(), ptrToString);
			}

			virtual void SimpleMember(parPtrToMember ptrToMember, parMemberSimple& metadata)
			{
				//Filter out a couple of common elements the user doesn't need to see
				if (	(metadata.GetNameHash() == ksiEventIndexHash)
					||	(metadata.GetNameHash() == ksiEventCallstackHash)
					||	(metadata.GetNameHash() == ksiEventAsU32Hash)
					)
				{
					return;
				}

				switch(metadata.GetType())
				{
				case parMemberType::TYPE_BOOL:
					m_pOutput->AddLine("%s: %s", metadata.GetName(), *reinterpret_cast<bool*>(ptrToMember) ? "true" : "false");
					m_bPrinted = true;
					break;
				case parMemberType::TYPE_CHAR:
					m_pOutput->AddLine("%s: %d", metadata.GetName(), *reinterpret_cast<s8*>(ptrToMember));
					m_bPrinted = true;
					break;
				case parMemberType::TYPE_UCHAR:
					m_pOutput->AddLine("%s: %d", metadata.GetName(), *reinterpret_cast<u8*>(ptrToMember));
					m_bPrinted = true;
					break;
				case parMemberType::TYPE_UINT:
					m_pOutput->AddLine("%s: %u", metadata.GetName(),  *reinterpret_cast<u32*>(ptrToMember));
					m_bPrinted = true;
					break;
				case parMemberType::TYPE_SIZET:
					m_pOutput->AddLine("%s: %zu", metadata.GetName(),  *reinterpret_cast<size_t*>(ptrToMember));
					m_bPrinted = true;
					break;
				case parMemberType::TYPE_PTRDIFFT:
					m_pOutput->AddLine("%s: %td", metadata.GetName(),  *reinterpret_cast<ptrdiff_t*>(ptrToMember));
					m_bPrinted = true;
					break;
				case parMemberType::TYPE_INT:
					m_pOutput->AddLine("%s: %d", metadata.GetName(), *reinterpret_cast<s32*>(ptrToMember));
					m_bPrinted = true;
					break;
				case parMemberType::TYPE_FLOAT:
					m_pOutput->AddLine("%s: %f", metadata.GetName(), *reinterpret_cast<float*>(ptrToMember));
					m_bPrinted = true;
					break;
				case parMemberType::TYPE_FLOAT16:
					m_pOutput->AddLine("%s: %f", metadata.GetName(), (*reinterpret_cast<Float16*>(ptrToMember)).GetFloat32_FromFloat16());
					m_bPrinted = true;
					break;
				default:
					break;
				}
			}
		};

		bool EventBase::DebugEventText(TextOutputVisitor &rOutput) const
		{
			EventDataVisitor visitor(rOutput);
			visitor.Visit(*const_cast<EventBase*>(this));
			return visitor.HasPrinted();
		}

		SimpleLabelEvent::SimpleLabelEvent(){}

		SimpleLabelEvent::SimpleLabelEvent(const CallstackHelper rCallstack)
			:EventBase(rCallstack)
		{
		}

		void SimpleLabelEvent::Set(const char *pLabel, const char *pText)
		{
			m_Label = pLabel;
			m_Description = pText;
		}

		static bool sm_bDrawSphereLabels;
		void SimpleSphereEvent::AddEventOptions(const Frame &, TextOutputVisitor &rOutput, bool &bMouseDown) const 
		{
			if(rOutput.AddLine("SphereLabels[%s]", sm_bDrawSphereLabels ? "X" : " ") && bMouseDown)
			{
				bMouseDown = false;
				sm_bDrawSphereLabels = !sm_bDrawSphereLabels;
			}
		}

		void SimpleSphereEvent::DebugDraw3d(eDrawFlags drawFlags) const
		{
			Color32 col = DebugRecorder::ModifyAlpha(Color32(m_Color));
			if (drawFlags & eDrawHovered)
			{
				if ((TIME.GetFrameCount() & 0x7) > 0x3)
				{
					col.SetAlpha(col.GetAlpha()>>2);
				}
			}

			grcColor(col);
			grcDrawSphere(m_Radius, m_Pos, drawFlags & eDrawHovered ? 8 : 4 , true, drawFlags & eDrawSelected ? true : false);
			grcWorldIdentity();

			if (sm_bDrawSphereLabels)
			{
				grcColor(Color32(0,0,0,DebugRecorder::GetGlobalAlphaScale()));
				grcDrawLabelf(RCC_VECTOR3(m_Pos), 1, 1, m_Label.TryGetCStr());
				grcColor(col);
				grcDrawLabelf(RCC_VECTOR3(m_Pos), 0, 0, m_Label.TryGetCStr());
			}
		}

		void DrawAxis(float size, Mat34V_In mat)
		{
			//Function ripped from grcDrawAxis
			const Mat34V prevWorldMtx(grcWorldMtx(mat));
			const float fAlphaScale = ((float)DebugRecorder::GetGlobalAlphaScale()) / 255.0f;

			grcBegin(drawLines,6);
			grcColor4f(1,0,0, fAlphaScale);
			grcVertex3f(0,0,0);
			grcVertex3f(size,0,0);
			grcColor4f(0,1,0, fAlphaScale);
			grcVertex3f(0,0,0);
			grcVertex3f(0,size,0);
			grcColor4f(0,0,1, fAlphaScale);
			grcVertex3f(0,0,0);
			grcVertex3f(0,0,size);
			grcEnd();

			const float a0 = 0.8f * size;
			const float b0 = 0.07f * size;
			const float c0 = 1.0f * size;
			const float a1 = a0 + 0.01f * size;
			const float b1 = b0 - 0.01f * size;
			const float c1 = c0 - 0.01f * size;

			grcBegin(drawTris,9);
			grcColor4f(1.0f,0.0f,0.0f,0.5f*fAlphaScale);
			grcVertex3f(c1,0.0f,0.0f);
			grcVertex3f(a1,b1,b1);
			grcVertex3f(a1,-b1,-b1);
			grcColor4f(0.0f,1.0f,0.0f,0.5f*fAlphaScale);
			grcVertex3f(0.0f,c1,0.0f);
			grcVertex3f(b1,a1,b1);
			grcVertex3f(-b1,a1,-b1);
			grcColor4f(0.0f,0.0f,1.0f,0.5f*fAlphaScale);
			grcVertex3f(0.0f,0.0f,c1);
			grcVertex3f(b1,b1,a1);
			grcVertex3f(-b1,-b1,a1);
			grcEnd();

			grcBegin(drawLineStrip,4);
			grcColor4f(1,0,0,fAlphaScale);
			grcVertex3f(c0,0.0f,0.0f);
			grcVertex3f(a0,b0,b0);
			grcVertex3f(a0,-b0,-b0);
			grcVertex3f(c0,0.0f,0.0f);
			grcEnd();

			grcBegin(drawLineStrip,4);
			grcColor4f(0,1,0,fAlphaScale);
			grcVertex3f(0.0f,c0,0.0f);
			grcVertex3f(b0,a0,b0);
			grcVertex3f(-b0,a0,-b0);
			grcVertex3f(0.0f,c0,0.0f);
			grcEnd();

			grcBegin(drawLineStrip,4);
			grcColor4f(0,0,1,fAlphaScale);
			grcVertex3f(0.0f,0.0f,c0);
			grcVertex3f(b0,b0,a0);
			grcVertex3f(-b0,-b0,a0);
			grcVertex3f(0.0f,0.0f,c0);
			grcEnd();
			grcWorldMtx(prevWorldMtx);
		}

		void SimpleMatrixEvent::DebugDraw3d(eDrawFlags /*drawFlags*/) const
		{
			DrawAxis(m_fScale, m_Matrix);
		}

		void SimpleLineEvent::DebugDraw3d(eDrawFlags drawFlags) const
		{
			Color32 colStart = DebugRecorder::ModifyAlpha(Color32(m_ColorStart));
			Color32 colEnd = DebugRecorder::ModifyAlpha(Color32(m_ColorEnd));

			if (drawFlags & eDrawHovered)
			{
				if ((TIME.GetFrameCount() & 0x7) > 0x3)
				{
					colStart.SetAlpha(colStart.GetAlpha()>>1);
					colEnd.SetAlpha(colEnd.GetAlpha()>>1);
				}
			}

			grcDrawLine(RCC_VECTOR3(m_Start), RCC_VECTOR3(m_End), colStart, colEnd);
		}

		Frame::Frame()
			:mp_Next(0)
			,mp_Prev(0)
			,mp_FirstEvent(0)
			,mp_LastEvent(0)
			,m_FrameIndex(0)
			,m_NetworkTime(0)
			,m_DisplayCount(0)
		{
			++sm_FrameCount;
		}

		Frame::~Frame()
		{
			DR_MEMORY_HEAP();

			//Delete all events
			EventBase *pEvent = mp_FirstEvent;
			while (pEvent)
			{
				EventBase *pNext = pEvent->mp_Next;
				delete pEvent;
				pEvent = pNext;
			}
			--sm_FrameCount;
		}

		void Frame::DebugText(TextOutputVisitor &rText) const
		{
			if (m_NetworkTime>0)
			{
				rText.AddLine("Frame Index %d (Network: %d)", m_FrameIndex, m_NetworkTime);
			}
			else
			{
				rText.AddLine("Frame Index %d", m_FrameIndex);
			}
		}

		bool Frame::DrawListText(TextOutputVisitor &rText) const
		{
			if (m_NetworkTime>0)
			{
				return rText.AddLine("%d (Network: %d)", m_FrameIndex, m_NetworkTime);
			}
			return rText.AddLine("%d", m_FrameIndex);
		}

		bool Frame::DebugDraw(eDrawFlags drawFlags, float fMouseX, float fMouseY, bool /*bMouseDown*/, DebugRecorder &rRecorder) const
		{
			u8 iCached = DebugRecorder::GetGlobalAlphaScale();
			float fClosest2 = 50*50;	//Pixels

			//Draw events and look for a hovered event for selection
			const EventBase *pEvent = mp_FirstEvent;
			const DataBase* pHighlightedData = rRecorder.GetHighlightedData();
			const EventBase* pHighlightedEvent = rRecorder.GetWasHoveredEvent() ? rRecorder.GetWasHoveredEvent() : rRecorder.GetSelectedEvent();
			while (pEvent)
			{
				if(pEvent->m_bIsDisplayEnabled)
				{
					//Make events less associated with whatever we have selected fade out
					int iAlphaLevel = 0;	//Full strength
					if (pHighlightedData)
					{
						if (!pEvent->IsDataInvolved(*pHighlightedData))
						{
							++iAlphaLevel;
						}
					}
					
					if (pHighlightedEvent)
					{
						if (pEvent != pHighlightedEvent)
						{
							++iAlphaLevel;
							if (!pEvent->IsEventDataRelated(*pHighlightedEvent))
							{
								++iAlphaLevel;
							}
						}
					}

					switch(iAlphaLevel)
					{
					case 0:
						DebugRecorder::SetGlobalAlphaScale(255);
						break;
					case 1:
						DebugRecorder::SetGlobalAlphaScale(172);
						break;
					case 2:
						DebugRecorder::SetGlobalAlphaScale(128);
						break;
					default:
						DebugRecorder::SetGlobalAlphaScale(64);
						break;
					}
					pEvent->DebugDraw3d(drawFlags);
					//Allow selection of the event if the mouse is hovering over it
					Vec3V eventPos;
					if (pEvent->GetEventPos(eventPos))
					{
						Vec3V screenPos = grcViewport::GetCurrent()->Transform(eventPos);
						float dX = fMouseX - screenPos.GetXf();
						float dY = fMouseY - screenPos.GetYf();
						float fPixelDist2 = dX*dX + dY*dY;
						if (fPixelDist2 < fClosest2)
						{
							fClosest2 = fPixelDist2;
							rRecorder.SetHovered(pEvent);
						}
					}
				}
				pEvent = pEvent->mp_Next;
			}

			DebugRecorder::SetGlobalAlphaScale(iCached);
			return false;
		}

		//------------------------------------------------------------------//
		//							WIDGET INTERFACE
		//------------------------------------------------------------------//
		static void Widget_StartRecording()
		{
			DR_MEMORY_HEAP();

			DebugRecorder *pInstance = DebugRecorder::GetInstance();
			if(pInstance)
			{
				pInstance->StartRecording();
			}

			//Auto switch the display on when you start recording
		}

		static void Widget_StopRecording()
		{
			DebugRecorder *pInstance = DebugRecorder::GetInstance();
			if (!pInstance || !pInstance->IsRecording())
			{
				Errorf("Need to be recording to stop recording!");
				return;
			}

			pInstance->StopRecording();
		}

		static void Widget_Save()
		{
			DebugRecorder *pInstance = DebugRecorder::GetInstance();
			if (pInstance)
			{
				char filename[256]={0};
				bool fileSelected = BANKMGR.OpenFile(filename, 256, s_FileExt_Search, true, "Debug event stream");

				//Make this file our new config file if selected
				if(fileSelected && filename)
				{
					pInstance->Save(filename);
				}
			}
		}

		static void Widget_Load()
		{
			DR_MEMORY_HEAP();

			DebugRecorder *pInstance = DebugRecorder::GetInstance();
			if (!pInstance)
			{
				return;
			}

			pInstance->ClearRecording();

			char filename[256] = {0};
			bool fileSelected = BANKMGR.OpenFile(filename, 256, s_FileExt_Search, false, "Debug event stream");

			//Make this file our new config file if selected
			if(fileSelected && filename)
			{
				pInstance->Load(filename);
			}
		}

		void DebugRecorder::AddWidgets(bkBank &rBank)
		{
			rBank.PushGroup("Debug Recorder", false, "Record physics events for later visualization");
			rBank.AddToggle("Display On", &m_bDisplayOn);

			rBank.AddButton("Start Recording", datCallback(MFA(DebugRecorder::StartRecording), this));
			rBank.AddButton("Stop Recording", datCallback(MFA(DebugRecorder::StopRecording), this));
			rBank.AddButton("Load Recording", CFA(Widget_Load));
			rBank.AddButton("Save Recording", CFA(Widget_Save));

			rBank.PopGroup();

		}

		//------------------------------------------------------------------//
		//				Simple label is a generic text event
		//------------------------------------------------------------------//
		void AddSimpleLabel(const char *pListLabel, const char *pTextFmt, ...)
		{
			if (DR_EVENT_ENABLED(SimpleLabelEvent))
			{
				CallstackHelper cs;
				DebugRecorder *pInstance = DebugRecorder::GetInstance();
				if (pInstance && pInstance->IsRecording())
				{
					DR_MEMORY_HEAP();
					char buffer[512];
					va_list args;
					va_start(args,pTextFmt);
					vsprintf(buffer,pTextFmt,args);
					va_end(args);

					SimpleLabelEvent *pNewEvent = rage_new SimpleLabelEvent(cs);
					pNewEvent->Set(pListLabel, buffer);
					pInstance->AddEvent(*pNewEvent);
				}
			}
		}

		void AddSimpleSphere(const char *pLabel, Vec3V_In vPos, float fRadius, Color32 color)
		{
			if (DR_EVENT_ENABLED(SimpleSphereEvent))
			{
				CallstackHelper cs;
				DebugRecorder *pInstance = DebugRecorder::GetInstance();
				if (pInstance && pInstance->IsRecording())
				{
					DR_MEMORY_HEAP();
					SimpleSphereEvent *pNewEvent = rage_new SimpleSphereEvent(cs);
					pNewEvent->Set(pLabel, vPos, fRadius, color.GetColor());
					pInstance->AddEvent(*pNewEvent);
				}
			}
		}

		void AddSimpleLine(const char *pLabel, Vec3V_In vPosA, Vec3V_In vPosB, Color32 colorA, Color32 colorB)
		{
			if (DR_EVENT_ENABLED(SimpleLineEvent))
			{
				CallstackHelper cs;
				DebugRecorder *pInstance = DebugRecorder::GetInstance();
				if (pInstance && pInstance->IsRecording())
				{
					DR_MEMORY_HEAP();
					SimpleLineEvent *pNewEvent = rage_new SimpleLineEvent(cs);
					pNewEvent->Set(pLabel, vPosA, vPosB, colorA.GetColor(), colorB.GetColor());
					pInstance->AddEvent(*pNewEvent);
				}
			}
		}

		void AddSimpleMatrix(const char *pLabel, Mat34V_In mat, float fScale)
		{
			if (DR_EVENT_ENABLED(SimpleMatrixEvent))
			{
				CallstackHelper cs;
				DebugRecorder *pInstance = DebugRecorder::GetInstance();
				if (pInstance && pInstance->IsRecording())
				{
					DR_MEMORY_HEAP();
					SimpleMatrixEvent *pNewEvent = rage_new SimpleMatrixEvent(cs);
					pNewEvent->Set(pLabel, mat, fScale);
					pInstance->AddEvent(*pNewEvent);
				}
			}
		}



		//------------------------------------------------------------------//
		//						DEBUG DRAW INTERFACE
		//------------------------------------------------------------------//
		bool OnScreenOutput::AddLine(const char *fmt/*=0*/, ...)
		{
			//Check bounds first
			if (m_fYPosition > m_fMaxY)
			{
				return false;
			}

			bool bHoverHit = false;
			//print to current location + tab indentation
			if (fmt)
			{
				float fX = m_fXPosition + m_fPerTabOffset * (float)m_iIndent;
				char buffer[2048] = {0};
				char *pStartPoint = &buffer[0];
				va_list args;
				va_start(args,fmt);
				vformatf(buffer,fmt,args);
				va_end(args);

				bool bClamped=false;
				if (m_iMaxStringLength)
				{
					Assert(m_iMaxStringLength < sizeof(buffer));
					int iStrlen = StringLength(buffer);
					if (iStrlen > (int) m_iMaxStringLength)
					{
						buffer[m_iMaxStringLength] = 0;
						bClamped = true;
					}
				}

				float fStringWidth = float(grcFont::GetCurrent().GetStringWidth( buffer, StringLength(buffer) ));
				//float fStringHeight = float(grcFont::GetCurrent().GetHeight());

				if (m_bAllowMouseHover)
				{
					if (m_fMouseX >= fX && m_fMouseX<=(fX+fStringWidth))
					{
						if (m_fMouseY >= m_fYPosition && m_fMouseY<=(m_fYPosition+m_fPerLineYOffset-1.0f))
						{
							m_bHoverHit = bHoverHit = true;

							if (bClamped)
							{
								//Let's scroll hovered strings
								va_start(args,fmt);
								vformatf(buffer,fmt,args);
								va_end(args);

								int iStrlen = StringLength(buffer)+1;
								if (AssertVerify(iStrlen > (int) m_iMaxStringLength))
								{
									int iOffset = (sm_iFrameCounter>>2);
									iOffset %= (iStrlen - m_iMaxStringLength);
									pStartPoint += iOffset;
									iStrlen -= iOffset;
									if (iStrlen>(int) m_iMaxStringLength)
									{
										pStartPoint[m_iMaxStringLength] = 0;
									}
								}
							}
						}
					}
				}

				if (m_bDrawBackground)
				{
					float fLeft = fX-1;
					float fRight = fX + fStringWidth+1;
					float fTop = m_fYPosition-1.0f;
					float fBottom = m_fYPosition + m_fPerLineYOffset-1.0f;

					Vector3 points[4];
					points[0].Set(fLeft,fTop,0.0f);
					points[1].Set(fRight,fTop,0.0f);
					points[2].Set(fRight,fBottom,0.0f);
					points[3].Set(fLeft,fBottom,0.0f);

					grcDrawPolygon((Vector3 *) points, 4, NULL, true, m_BackGroundColor);

					//grcDraw2dQuad(
					//	Vector3(fLeft,fTop,0.0f),
					//	Vector3(fRight,fTop,0.0f),
					//	Vector3(fRight,fBottom,0.0f),
					//	Vector3(fLeft,fBottom,0.0f),
					//	m_BackGroundColor);
				}

				grcDebugDraw::Text(Vec2V(((fX+1)/grcDevice::GetWidth()), ((m_fYPosition+1)/grcDevice::GetHeight())), Color32(0,0,0,255),pStartPoint, false, m_fScale, m_fScale);
				Color32 textColor = bHoverHit ? m_HighlightColor : m_Color;
				if (m_bForceColor)
				{
					textColor = m_Color;
				}
				grcDebugDraw::Text(Vec2V(((fX)/grcDevice::GetWidth()), ((m_fYPosition)/grcDevice::GetHeight())), textColor,pStartPoint, false, m_fScale, m_fScale);
			}

			//Increment line / scale
			m_fYPosition += m_fPerLineYOffset * m_fScale;

			return bHoverHit;
		}

		void OnScreenOutput::PushIndent()
		{
			Assertf(m_iIndent < 32, "Very high indent level - push pop bug?");
			++m_iIndent;
		}

		void OnScreenOutput::PopIndent()
		{
			Assertf(m_iIndent > 0, "Indent level going negative- push pop bug?");
			--m_iIndent;
		}

        OnScreenOutput::~OnScreenOutput()
		{

		}
		int OnScreenOutput::sm_iFrameCounter;

		struct OnScreenEventList : public OnScreenOutput
		{
			static u32 ms_LastMouseTrackFrame;
			static float ms_fMouseScroll;
			static float ms_fLastMouseY;
			static bool ms_bDragging;

			OnScreenEventList(float fYLineSep)
				:OnScreenOutput(fYLineSep)
			{
			}

			bool Draw(const EventBase *pStartItem, const EventBase *pSelectedItem, const DataBase *pHighlightedData, bool bDrawContents, bool &bItemSelected, bool &bMouseDown, const char *pTitle)
			{
				const EventBase *pThrowAway=0;
				return Draw(pStartItem, pSelectedItem, pThrowAway, pHighlightedData, bDrawContents, bItemSelected, bMouseDown, pTitle);
			}

			bool Draw(const EventBase *pStartItem, const EventBase *pSelectedItem, const EventBase *& pHoverItem, const DataBase *pHighlightedData, bool bDrawContents, bool &bItemSelected, bool &bMouseDown, const char *pTitle)
			{
				if (!pStartItem)
				{
					return false;
				}

				int iCountDrawn=0;
				int iCountTotal=0;
				const EventBase *pItem = pStartItem;
				while (pItem)
				{
					if (pItem->m_bIsDisplayEnabled)
					{
						++iCountDrawn;
					}
					++iCountTotal;
					pItem = pItem->mp_Next;
				}
				pItem = pStartItem->mp_Prev;
				while (pItem)
				{
					if (pItem->m_bIsDisplayEnabled)
					{
						++iCountDrawn;
					}
					++iCountTotal;
					pItem = pItem->mp_Prev;
				}

				bool bReturn = false;
				if (pTitle)
				{
					bReturn = AddLine( "%s [%d/%d]", pTitle, iCountDrawn, iCountTotal);
				}

				if (!bDrawContents)
				{
					return bReturn;
				}

				bool bNewDragging = false;
				m_bAllowMouseHover = !ms_bDragging;
				float fStartingY = m_fYPosition;
				//Draw next items
				pItem = pStartItem;
				Color32 dataInvolvedCol(200,80,80,255);
				Color32 selectedCol(200,0,60,255);
				while (pItem)
				{
					if (m_fYPosition > (m_fMaxY - m_fPerLineYOffset))
					{
						break;
					}

					if (pItem->m_bIsDisplayEnabled)
					{
						m_Color.Set(220,220,220,255);
						m_bDrawBackground = false;
						if (pSelectedItem)
						{
							if (pItem->IsEventDataRelated(*pSelectedItem))
							{
								m_bDrawBackground = true;
								m_Color = dataInvolvedCol;
								m_BackGroundColor.Set(0,60,40,80);
							}
						}
						else if (pHighlightedData)
						{
							//TMS: Note IsEventDataRelated and IsDataInvolved are not directly equivilent, need to work that out for consistency
							if (pItem->IsDataInvolved(*pHighlightedData))
							{
								m_bDrawBackground = true;
								m_Color = dataInvolvedCol;
								m_BackGroundColor.Set(0,60,40,80);
							}
						}

						if (pSelectedItem)
						{
							if (pItem == pSelectedItem)
							{
								m_bForceColor = true;
								m_Color = selectedCol;
								m_BackGroundColor.Set(0,64,64,80);
							}
						}
						if (pItem->DrawListText( *this ))
						{
							if (!ms_bDragging)
							{
								bItemSelected = true;
								pHoverItem = pItem;
							}
						}
						m_bDrawBackground = false;
						m_bForceColor = false;
					}

					pItem = pItem->mp_Next;
				}

				//Draw a scroll bar if we're off one or the other end of the list
				if (pStartItem->mp_Prev || m_fYPosition > (m_fMaxY - m_fPerLineYOffset))
				{
					float fTotalCount = (float)iCountTotal;
					float fPreceedingCount = 0.0f;
					float fTrailingCount = 0.0f;
					const EventBase *pPreceeding = pStartItem->mp_Prev;
					while (pPreceeding)
					{
						if (pPreceeding->m_bIsDisplayEnabled)
						{
							fPreceedingCount+=1.0f;
						}
						pPreceeding = pPreceeding->mp_Prev;
					}

					while (pItem)
					{
						if (pItem->m_bIsDisplayEnabled)
						{
							fTrailingCount += 1.0f;
						}
						pItem = pItem->mp_Next;
					}

					float fDrawnCount = fTotalCount - (fPreceedingCount + fTrailingCount);

					float fBarRight = m_fXPosition-3.0f;
					float fBarLeft = fBarRight - 8.0f;
					float fBarTop = fStartingY;
					float fBarBottom = m_fMaxY;

					Vector3 vecBarBox[4];
					vecBarBox[0].Set(fBarLeft, fBarTop, 0.0f);
					vecBarBox[1].Set(fBarRight, fBarTop, 0.0f);
					vecBarBox[2].Set(fBarRight, fBarBottom, 0.0f);
					vecBarBox[3].Set(fBarLeft, fBarBottom, 0.0f);

					grcDrawPolygon(vecBarBox, 4, NULL, true, Color32(32,32,32,128));

					//Draw horizontal markers for any highlighted events on the scroll bar
					pItem = pStartItem;
					while (pItem && pItem->mp_Prev)
					{
						pItem = pItem->mp_Prev;
					}

					//Draw markers as scaled down line sizes
					float fLineSize = Max(1.5f, m_fPerLineYOffset * fDrawnCount / fTotalCount);
					float fCounter = 0.0f;
					while (pItem)
					{
						if (pItem->m_bIsDisplayEnabled)
						{
							float fMarkerTop = fBarTop + ((fBarBottom - fBarTop) * fCounter / fTotalCount);
							float fMarkerBottom = fMarkerTop + fLineSize;
							if (pItem == pSelectedItem)
							{	
								vecBarBox[0].Set(fBarLeft, fMarkerTop, 0.0f);
								vecBarBox[1].Set(fBarRight, fMarkerTop, 0.0f);
								vecBarBox[2].Set(fBarRight, fMarkerBottom, 0.0f);
								vecBarBox[3].Set(fBarLeft, fMarkerBottom, 0.0f);
								grcDrawPolygon(vecBarBox, 4, NULL, true, selectedCol);
							}
							else if (	(pSelectedItem && pItem->IsEventDataRelated(*pSelectedItem))
									||	(pHighlightedData && pItem->IsDataInvolved(*pHighlightedData))	)
							{
								vecBarBox[0].Set(fBarLeft, fMarkerTop, 0.0f);
								vecBarBox[1].Set(fBarRight, fMarkerTop, 0.0f);
								vecBarBox[2].Set(fBarRight, fMarkerBottom, 0.0f);
								vecBarBox[3].Set(fBarLeft, fMarkerBottom, 0.0f);
								grcDrawPolygon(vecBarBox, 4, NULL, true, dataInvolvedCol);
							}

							fCounter += 1.0f;
						}
						pItem = pItem->mp_Next;
					}

					bool bHighlightHighlightBar = false;
					float fHighlightTop = fBarTop + ms_fMouseScroll + ((fBarBottom - fBarTop) * fPreceedingCount / fTotalCount);
					float fHighlightBottom = fBarBottom + ms_fMouseScroll - ((fBarBottom - fBarTop) * fTrailingCount / fTotalCount);
					if (	ms_bDragging
						||	((m_fMouseX >= fBarLeft)
						&&	 (m_fMouseX <= fBarRight) 
						&& 	 (m_fMouseY >= fHighlightTop) 
						&& 	 (m_fMouseY <= fHighlightBottom) )
						)
					{
						bHighlightHighlightBar = true;
						bItemSelected = true;
						bMouseDown = false;

						//Is the mouse button down?
						if (ioMouse::GetButtons() & ioMouse::MOUSE_LEFT)
						{
							bNewDragging = true;
							if (ms_LastMouseTrackFrame == (TIME.GetFrameCount() - 1))
							{
								float fDeltaY = m_fMouseY - ms_fLastMouseY;
								ms_fMouseScroll += fDeltaY * fTotalCount / fDrawnCount;
								while (ms_fMouseScroll > m_fPerLineYOffset)
								{
									//Need to scroll to the next option
									DebugRecorder::GetInstance()->ScrollEventNext();
									ms_fMouseScroll -= m_fPerLineYOffset;
								}
								
								while (ms_fMouseScroll < -m_fPerLineYOffset)
								{
									//Need to scroll to the next option
									DebugRecorder::GetInstance()->ScrollEventPrev();
									ms_fMouseScroll += m_fPerLineYOffset;
								}
							}

							ms_LastMouseTrackFrame = TIME.GetFrameCount();
							ms_fLastMouseY = m_fMouseY;
						}
					}

					//Draw a highlight over the top of everything
					vecBarBox[0].Set(fBarLeft, fHighlightTop, 0.0f);
					vecBarBox[1].Set(fBarRight, fHighlightTop, 0.0f);
					vecBarBox[2].Set(fBarRight, fHighlightBottom, 0.0f);
					vecBarBox[3].Set(fBarLeft, fHighlightBottom, 0.0f);

					grcDrawPolygon(vecBarBox, 4, NULL, true, Color32(128,128,128,bHighlightHighlightBar ? 128 : 160));
				}

				ms_bDragging = bNewDragging;
				return bReturn;
			}
		};

		u32 OnScreenEventList::ms_LastMouseTrackFrame;
		float OnScreenEventList::ms_fMouseScroll;
		float OnScreenEventList::ms_fLastMouseY;
		bool OnScreenEventList::ms_bDragging;

		struct MenuItem
		{
			char m_TextBuffer[128];
			void (*m_ExecuteFunc)(void);
			MenuItem(const char *pText, void (*executeFunc)(void))
				:m_ExecuteFunc(executeFunc)
			{
				formatf(m_TextBuffer,pText);
			}

			bool DrawAndExecute(OnScreenOutput &rOutput, bool bMouseDown);
		};

		bool MenuItem::DrawAndExecute(OnScreenOutput &rOutput, bool bMouseDown)
		{
			if (rOutput.AddLine(m_TextBuffer))
			{
				if (bMouseDown && m_ExecuteFunc)
				{
					m_ExecuteFunc();
					return true;
				}
			}
			return false;
		}

		MenuItem menu_StartRecording("Start", Widget_StartRecording);
		MenuItem menu_StopRecording("Stop", Widget_StopRecording);

		void Init()
		{
		}

		void DebugRecorder::ValidateAndBuildEventDisplayList()
		{
			//Update the list if we've added events since the last time we did this
			if (mp_LastFrame && mp_LastFrame->mp_LastEvent && (mp_LastFrame->mp_LastEvent->m_iEventIndex > m_iCachedEventIndex))
			{
				const Frame* pFrame = mp_FirstFrame;
				while (pFrame)
				{
					//Quick skip
					if (m_iCachedEventIndex <= pFrame->mp_LastEvent->m_iEventIndex)
					{
						const EventBase *pEvent = pFrame->mp_FirstEvent;
						while (pEvent)
						{
							if (Verifyf(pEvent->GetEventSubType(), "Event type %s has NULL subtype", pEvent->GetEventType()))
							{
								size_t iAccess = (size_t)pEvent->GetEventSubType();
								SubEventType *pEventInf = m_DisplayedTypeMap.Access(iAccess);
								if(!pEventInf)
								{
									SubEventType *pNewType = &m_DisplayedTypeMap[iAccess];
									m_DisplayedNameList.Grow() = pNewType;
									pNewType->m_bEnabled = true;
									pNewType->mp_OriginalName = pEvent->GetEventSubType();
								}
							}
							pEvent = pEvent->mp_Next;
						}
					}
					pFrame = pFrame->mp_Next;
				}

				//Cache this point
				m_iCachedEventIndex = mp_LastFrame->mp_LastEvent->m_iEventIndex;

				//Sort the list.
				m_DisplayedNameList.QSort(0, -1, SubEventType::Compare);
			}
		}

		void DebugRecorder::ToggleSetFilterToJustThis(const char *pFilterName)
		{
			ValidateAndBuildEventDisplayList();
		
			bool bDisplayedOther = false;

			//Were we displaying anything else?
			for (int i=0 ; i<m_DisplayedNameList.GetCount() ; i++)
			{
				if (m_DisplayedNameList[i]->mp_OriginalName != pFilterName)
				{
					bDisplayedOther = m_DisplayedNameList[i]->m_bEnabled;
					if (bDisplayedOther)
						break;
				}
			}

			if (bDisplayedOther)
			{
				for (int i=0 ; i<m_DisplayedNameList.GetCount() ; i++)
				{
					m_DisplayedNameList[i]->m_bEnabled = m_DisplayedNameList[i]->mp_OriginalName == pFilterName;
				}
			}
			else
			{
				for (int i=0 ; i<m_DisplayedNameList.GetCount() ; i++)
				{
					m_DisplayedNameList[i]->m_bEnabled = true;
				}
			}
		}


		bool DebugRecorder::IsItemEnabled(const char *pIdentifier) const
		{
			//If we haven't got the item assume it's enabled
			const SubEventType *pItem = m_DisplayedTypeMap.Access((size_t)pIdentifier);
			return !pItem || pItem->m_bEnabled;
		}

		void DebugRecorder::EnableDataFilterForEvent(const EventBase &rEvent)
		{
			//Any data involved in the event should be enabled
			atMap<size_t, DataBase*>::Iterator itrRD = m_RecordedData.CreateIterator();
			while (!itrRD.AtEnd())
			{
				if (!itrRD.GetData()->IsFilterActive())
				{
					if (rEvent.IsDataInvolved(*itrRD.GetData()))
					{
						itrRD.GetData()->OnFilterToggle();
					}
				}
				++itrRD;
			}

			RefreshFilteredState();
		}

		void DebugRecorder::CallOnAllStoredData(void (*pSomeFunc)(DataBase&))
		{
			//Any data involved in the event should be enabled
			atMap<size_t, DataBase*>::Iterator itrRD = m_RecordedData.CreateIterator();
			while (!itrRD.AtEnd())
			{
				pSomeFunc(*itrRD.GetData());
				++itrRD;
			}
		}

		void DebugRecorder::RefreshFilteredState(const DataBase* pHovered)
		{
			m_bFilterNeedsRefresh = false;

			DataBase *pEnabledDatas[256];			
			int iNumEnabledDatas = 0;
	
			if (m_bRestrictToDataFilter)
			{
				//Check that we have one of data types indicating this event is enabled
				atMap<size_t, DataBase*>::Iterator itrRD = m_RecordedData.CreateIterator();
				while (!itrRD.AtEnd())
				{
					if (itrRD.GetData()->IsFilterActive())
					{
						if (iNumEnabledDatas < 256)
						{
							pEnabledDatas[iNumEnabledDatas] = itrRD.GetData();
							++iNumEnabledDatas;
						}
						else
						{
							Errorf("Too many data types enabled - slow iteration path enabled!");
							iNumEnabledDatas = -1;
							break;
						}
					}
					++itrRD;
				}
			}

			//Enable events with any filter saying they should be included
			Frame *pFrame = mp_FirstFrame;
			while (pFrame)
			{
				EventBase *pEvent = pFrame->mp_FirstEvent;
				pFrame->m_DisplayCount = 0;
				while (pEvent)
				{
					pEvent->m_bIsDisplayEnabled = false;
					if (IsItemEnabled(pEvent->GetEventSubType()))
					{
						if (m_bRestrictToDataFilter)
						{
							if (pHovered && pEvent->IsDataInvolved(*pHovered))
							{
								pEvent->m_bIsDisplayEnabled = true;
							}
							
							if (!pEvent->m_bIsDisplayEnabled)
							{
								//Check that we have one of data types indicating this event is enabled
								if (iNumEnabledDatas >= 0)
								{
									for (int i=0 ; i<iNumEnabledDatas ; i++)
									{
										if (pEvent->IsDataInvolved(*pEnabledDatas[i]))
										{
											pEvent->m_bIsDisplayEnabled = true;
											break;
										}
									}
								}
								else
								{
									atMap<size_t, DataBase*>::Iterator itrRD = m_RecordedData.CreateIterator();
									while (!itrRD.AtEnd())
									{
										if (itrRD.GetData()->IsFilterActive())
										{
											if (pEvent->IsDataInvolved(*itrRD.GetData()))
											{
												pEvent->m_bIsDisplayEnabled = true;
												break;
											}
										}
										++itrRD;
									}
								}
							}
						}
						else
						{
							//Display type only
							pEvent->m_bIsDisplayEnabled = true;
						}
					}

					if (pEvent->m_bIsDisplayEnabled)
					{
						pFrame->m_DisplayCount++;
					}
					pEvent = pEvent->mp_Next;
				}

				pFrame = pFrame->mp_Next;
			}
		}

		bool DebugRecorder::AddChannelMenu(OnScreenOutput &rOutput, OnScreenOutput &rInfoText, bool &bMouseDown, bool bPaused)
		{
			static DR_EventType *smp_AnchorEventType = DR_EventType::smp_First;
			bool bItemSelected = false;
			rOutput.m_fYPosition = YChannel;
			rOutput.m_iMaxStringLength = 19;
			rOutput.m_BackGroundColor.Set(256,256,256,32);

			//Used to allow hovering to modify what we display
			bool bFilterWasSet = false;
			bool bFilterChanged = false;

			//Allow user to cycle between different options
			switch(m_iFilterTypeIndex)
			{
			case 0:
				if (rOutput.AddLine("<-FILTERS->") && bMouseDown)
				{
					m_iFilterTypeIndex = 1;
					bMouseDown = false;
					bItemSelected = true;
					return true;
				}
				else
				{
					if (rOutput.AddLine("[SaveFilter]") && bMouseDown)
					{
						char filename[256]={0};
						bool fileSelected = BANKMGR.OpenFile(filename, 256, "*.meta", true, "Debug event stream");

						//Make this file our new config file if selected
						if(fileSelected && filename[0])
						{
							Displayf("Saving filter file to '%s'", filename);
							if (!SaveFilter(filename))
							{
								Errorf("Failed to save filter '%s'", filename);
							}
						}
						bMouseDown = false;
					}

					if (rOutput.AddLine("[LoadFilter]") && bMouseDown)
					{
						char filename[256] = {0};
						bool fileSelected = BANKMGR.OpenFile(filename, 256, "*.meta", false, "Debug event stream");

						//Make this file our new config file if selected
						if(fileSelected && filename[0])
						{
							Displayf("Clearing event filter");
							ClearFilter();

							Displayf("Loading event filter %s", filename);
							if (!LoadFilter(filename))
							{
								Errorf("Failed to load event filter %s", filename);
							}
						}
						bFilterChanged = true;
						bItemSelected = true;
						bMouseDown = false;
					}

					if (rOutput.AddLine("[OverlayFilter]") && bMouseDown)
					{
						char filename[256] = {0};
						bool fileSelected = BANKMGR.OpenFile(filename, 256, "*.meta", false, "Debug event stream");

						//Make this file our new config file if selected
						if(fileSelected && filename[0])
						{
							Warningf("NOT Clearing current event filter - overlaying new filter on old");

							Displayf("Loading event filter %s", filename);
							if (!LoadFilter(filename))
							{
								Errorf("Failed to load event filter %s", filename);
							}
						}
						bFilterChanged = true;
						bItemSelected = true;
						bMouseDown = false;
					}

					if (rOutput.AddLine("[ClearFilter]") && bMouseDown)
					{
						ClearFilter();
						bFilterChanged = true;
					}
				}
				return false;
			case 1:
				if (rOutput.AddLine("<-RECORD FILTER->") && bMouseDown)
				{
						bItemSelected = true;
						bMouseDown = false;
						m_iFilterTypeIndex = 2;
						//FALLTHRU
				}
				if (m_iFilterTypeIndex == 1)
				{
					DR_EventType *pTypeData = smp_AnchorEventType;
					if (pTypeData)
					{
						if (rOutput.AddLine("[Enable All]") && bMouseDown)
						{
							bItemSelected = true;
							bMouseDown = false;
							pTypeData = smp_AnchorEventType;
							while (pTypeData)
							{
								pTypeData->m_bEnabled = true;
								pTypeData = pTypeData->mp_Next;
							}
						}
						if (rOutput.AddLine("[Disable All]") && bMouseDown)
						{
							bItemSelected = true;
							bMouseDown = false;
							pTypeData = smp_AnchorEventType;
							while (pTypeData)
							{
								pTypeData->m_bEnabled = false;
								pTypeData = pTypeData->mp_Next;
							}
						}

						bool bHovered = false;
						bool bPastEnd = false;
						pTypeData = smp_AnchorEventType;
						while (pTypeData)
						{
							rOutput.m_bDrawBackground = pTypeData->m_bEnabled;
							rOutput.m_BackGroundColor.Set(256,256,256,64);
							if (rOutput.AddLine("[%s]%s", pTypeData->m_bEnabled ? "X" : " ", pTypeData->m_Name))
							{
								bHovered = true;
								if (bMouseDown)
								{
									bItemSelected = true;
									pTypeData->m_bEnabled = !pTypeData->m_bEnabled;
									bFilterChanged = true;
								}
							}
							if (rOutput.m_fYPosition > YMax)
							{
								bPastEnd = true;
								break;
							}
							pTypeData = pTypeData->mp_Next;
						}

						//Mouse wheel up and down list
						if (bHovered)
						{
							if (ioMouse::HasWheel() && (ioMouse::GetDZ()!=0))
							{
								if (ioMouse::GetDZ() < 0)
								{
									if (smp_AnchorEventType->mp_Next && bPastEnd)
									{
										smp_AnchorEventType = smp_AnchorEventType->mp_Next;
									}
								}
								else if (smp_AnchorEventType->mp_Prev)
								{
									smp_AnchorEventType = smp_AnchorEventType->mp_Prev;
								}
							}			
						}
					}
					break;
				}
			case 2:
				if (rOutput.AddLine("<-DISPLAY FILTER->") && bMouseDown)
				{
					bItemSelected = true;
					bMouseDown = false;
					m_iFilterTypeIndex = 3;
					//FALLTHRU
				}
				if (m_iFilterTypeIndex == 2)
				{
					//Make sure we have a list of currently displayed events by sub type name
					//Make sure we've got a quick lookup from name to info
					//Make sure we have an array that we can sort alphabetically
					if (m_bRecording && !bPaused)
					{
						rOutput.m_bDrawBackground = (TIME.GetFrameCount() & 0x7) > 3;
						rOutput.m_BackGroundColor.Set(256,256,256,64);
						rOutput.AddLine("Recording");
						break;
					}

					ValidateAndBuildEventDisplayList();

					if (rOutput.AddLine("[Enable All]") && bMouseDown)
					{
						bItemSelected = true;
						bMouseDown = false;
						for (int i=0 ; i<m_DisplayedNameList.GetCount() ; i++)
						{
							m_DisplayedNameList[i]->m_bEnabled = true;
						}
					}
					if (rOutput.AddLine("[Disable All]") && bMouseDown)
					{
						bItemSelected = true;
						bMouseDown = false;
						for (int i=0 ; i<m_DisplayedNameList.GetCount() ; i++)
						{
							m_DisplayedNameList[i]->m_bEnabled = false;
						}
					}

					//Now draw the full list, with scrolling if needed
					bool bHovered=false;
					bool bPastEnd=false;
					for (int i=m_iAnchorDisplayType ; i<m_DisplayedNameList.GetCount() ; i++)
					{
						rOutput.m_bDrawBackground = m_DisplayedNameList[i]->m_bEnabled;
						rOutput.m_BackGroundColor.Set(256,256,256,64);
						const char *pSubTypeName = m_DisplayedNameList[i]->mp_OriginalName;
						if (rOutput.AddLine("[%s]%s", m_DisplayedNameList[i]->m_bEnabled ? "X" : " ", pSubTypeName))
						{
							//If hovering over an event draw all of that type

							//Now draw X frames data
							const Frame *pFrame = mp_FirstFrame;
							const EventBase *pFirstEvent = 0;
							int iFramesToDraw = Frame::GetStaticFrameCount()>>5;
							int iFrameCount=0;
							static int siCycleState = 0;

							//Skip frames that we've already drawn
							while (pFrame && (iFrameCount<siCycleState))
							{
								iFrameCount++;
								pFrame = pFrame->mp_Next;
							}

							while (pFrame)
							{
								const EventBase *pEvent = pFrame->mp_FirstEvent;
								bool bHasData = false;
								while (pEvent)
								{
									if (pEvent->GetEventSubType() == pSubTypeName)
									{
										if (iFramesToDraw)
										{
											pEvent->DebugDraw3d(eDrawNormal);
										}
										bHasData = true;
										if (!pFirstEvent)
										{
											pFirstEvent = pEvent;
										}
									}
									pEvent = pEvent->mp_Next;
								}
								++siCycleState;
								if (bHasData)
								{
									if (iFramesToDraw)
									{
										--iFramesToDraw;
									}
								}
								if (!iFramesToDraw && pFirstEvent)
								{
									break;
								}
								pFrame = pFrame->mp_Next;
							}

							//Got to the end? if so start again!
							if (!pFrame)
							{
								siCycleState = 0;
							}

							rOutput.m_bDrawBackground = false;
							rOutput.m_BackGroundColor.Set(256,256,256,32);
							bHovered = true;
							if (bMouseDown)
							{
								if (pFirstEvent && ioMapper::DebugKeyDown(KEY_CONTROL))
								{
									//Quick selection of the first event
									Select(0, pFirstEvent);
									
									//Make sure we have the type enabled
									m_DisplayedNameList[i]->m_bEnabled = true;
									bFilterChanged = true;
									bItemSelected = true;
								}
								else
								{
									//Toggle 
									m_DisplayedNameList[i]->m_bEnabled = !m_DisplayedNameList[i]->m_bEnabled;
									bItemSelected = true;
									bFilterChanged = true;
								}
							}
							else if (ioMouse::GetButtons() & ioMouse::MOUSE_RIGHT)
							{
								if (pFirstEvent)
								{
									bItemSelected = true;

									//Quick selection of the first event
									Select(0, pFirstEvent);

									//And display others while we're at it
									DrawOthersData::SetDrawOthers(pFirstEvent->GetEventSubType(), true);
								}
							}

							if (rOutput.m_fYPosition > YMax)
							{
								bPastEnd = true;
								break;
							}
						}
					}

					//Mouse wheel up and down list
					if (bHovered)
					{
						if (ioMouse::HasWheel() && (ioMouse::GetDZ()!=0))
						{
							if (ioMouse::GetDZ() < 0)
							{
								if (m_iAnchorDisplayType < m_DisplayedNameList.GetCount() && !bPastEnd)
								{
									m_iAnchorDisplayType++;
								}
							}
							else if (m_iAnchorDisplayType)
							{
								--m_iAnchorDisplayType;
							}
						}			
					}	
					break;
				}
			case 3:
				if (rOutput.AddLine("<-DATA FILTER->") && bMouseDown)
				{
					bItemSelected = true;
					m_iFilterTypeIndex = 4;
					bMouseDown = false;
					//FALLTHRU
				}
				if (m_iFilterTypeIndex == 3)
				{
					if (rOutput.AddLine("[%s]Use data filter", m_bRestrictToDataFilter ? "X" : " ") && bMouseDown)
					{
						m_bRestrictToDataFilter = !m_bRestrictToDataFilter;
						bMouseDown = false;
						bItemSelected = true;
						bFilterWasSet = true;
						bFilterChanged = true;
					}

					atMap<size_t, DataBase*>::Iterator itrRD = m_RecordedData.CreateIterator();
					if(rOutput.AddLine("[Disable all]") && bMouseDown)
					{
						while (!itrRD.AtEnd())
						{
							if (itrRD.GetData()->IsFilterActive())
							{
								itrRD.GetData()->OnFilterToggle();
							}
							++itrRD;
						}
						itrRD.Start();

						bFilterWasSet = true;
						bFilterChanged = true;
						bMouseDown = false;
						bItemSelected = true;
					}
					if(rOutput.AddLine("[Enable all]") && bMouseDown)
					{
						while (!itrRD.AtEnd())
						{
							if (!itrRD.GetData()->IsFilterActive())
							{
								itrRD.GetData()->OnFilterToggle();
							}
							++itrRD;
						}
						itrRD.Start();

						bFilterChanged = true;
						bFilterWasSet = true;
						bMouseDown = false;
						bItemSelected = true;
					}
					int iCounter = 0;
					DataBase* pHovered = 0;
					bool bPastEnd = false;
					while (!itrRD.AtEnd())
					{
						if (iCounter >= m_iAnchorDataFilter)
						{
							if( itrRD.GetData()->HasFilterOption() )
							{
								rOutput.m_bDrawBackground = itrRD.GetData()->IsFilterActive();
								if (rOutput.AddLine("[%s]%s", itrRD.GetData()->IsFilterActive() ? "X" : " ", itrRD.GetData()->GetFilterText()))
								{
									pHovered = itrRD.GetData();							
								}

								if (rOutput.m_fYPosition > YMax)
								{
									bPastEnd = true;
									break;
								}
							}
						}
						++iCounter;
						++itrRD;
					}

					//Continue counting to allow scroll limiting
					while (!itrRD.AtEnd())
					{
						++iCounter;
						++itrRD;
					}

					if (pHovered)
					{
						//Toggle filter on click
						if (bMouseDown)
						{
							bFilterChanged = true;
							bItemSelected = true;
							bMouseDown = false;
							pHovered->OnFilterToggle();
						}

						//What about drawing all events related to this data type?	EXPENSIVE :)
						static int siCycleState = 0;
						int iFrameCount=0;
						u32 iFramesToDraw = Frame::GetStaticFrameCount()>>5;	//Cycle once per second
						iFramesToDraw = iFramesToDraw > 0 ? iFramesToDraw : 1;

						//Skip frames that we've already drawn
						const Frame *pFrame = mp_FirstFrame;
						while (pFrame && (iFrameCount<siCycleState))
						{
							iFrameCount++;
							pFrame = pFrame->mp_Next;
						}

						//Now draw X frame data
						while (pFrame)
						{
							bool bHasData = false;
							const EventBase *pEvent = pFrame->mp_FirstEvent;
							while (pEvent)
							{
								if (pEvent->IsDataInvolved(*pHovered))
								{
									bHasData = true;
									pEvent->DebugDraw3d(eDrawNormal);
								}
								pEvent = pEvent->mp_Next;
							}
							++siCycleState;
							if (bHasData)
							{
								--iFramesToDraw;
								if (!iFramesToDraw)
								{
									break;
								}
							}
							pFrame = pFrame->mp_Next;
						}

						//Loop around if needed
						if (!pFrame)
						{
							siCycleState = 0;
						}

						//Scroll filter list
						if (ioMouse::HasWheel() && (ioMouse::GetDZ()!=0))
						{
							if (ioMouse::GetDZ() < 0)
							{
								if (m_iAnchorDataFilter < iCounter && bPastEnd)
								{
									m_iAnchorDataFilter++;
								}
							}
							else if (m_iAnchorDataFilter)
							{
								--m_iAnchorDataFilter;
							}
						}	

						mp_LastHoveredData = pHovered;
						bFilterWasSet = true;
					}
				}
				break;
			default://4+
				return AddAppFilters(rOutput, rInfoText, m_iFilterTypeIndex, bMouseDown);
			}

			if (bFilterChanged || (!bFilterWasSet && mp_LastHoveredData))
			{
				RefreshFilteredState();
				mp_LastHoveredData = 0;
			}
			return bItemSelected;
		}

		bool DebugRecorder::DrawOthers3d(float fMouseX, float fMouseY)
		{
			u8 iCached = sm_iGlobalAlphaScale;
			sm_iGlobalAlphaScale = 128;				//Perhaps scale with mouse time over event type?
			bool bSelected = false;		
			if (DrawOthersData::sm_iNumDrawOthers)
			{
				//Loop through all events in the recording and show those matching this type, use RTTI until we implement a type system
				//Allow the user to click select on any events that report back their position using GetEventPos
				float fClosest2 = 50*50;	//Pixels
				//const Frame *pHoveredFrame = 0;

				//Draw all set for multiple frames, not everything as it goes too slow!
				static int siCycleState = 0;
				int iFrameCount=0;
				u32 iFramesToDraw = Frame::GetStaticFrameCount()>>5;	//Cycle once per second
				iFramesToDraw = iFramesToDraw > 0 ? iFramesToDraw : 1;

				//Skip frames that we've already drawn
				const Frame *pFrame = GetFirstFramePtr();
				while (pFrame && (iFrameCount<siCycleState))
				{
					iFrameCount++;
					pFrame = pFrame->mp_Next;
				}

				while (pFrame)
				{
					bool bHasData = false;
					const EventBase *pEvent = pFrame->mp_FirstEvent;
					while (pEvent)
					{
						//Check if we're on the draw others list
						const int kMax = DrawOthersData::sm_iNumDrawOthers;
						for (int i=0 ; i<kMax ; i++)
						{
							if ((pEvent->GetEventSubType() == DrawOthersData::sm_DrawOthersList[i].mp_SubType) && pEvent->m_bIsDisplayEnabled)
							{
								bHasData = true;
								pEvent->DebugDraw3d(eDrawNormal);

								//Allow selection of the event if the mouse is hovering over it
								Vec3V eventPos;
								if (pEvent->GetEventPos(eventPos))
								{
									Vec3V screenPos = grcViewport::GetCurrent()->Transform(eventPos);
									float dX = fMouseX - screenPos.GetXf();
									float dY = fMouseY - screenPos.GetYf();
									float fPixelDist2 = dX*dX + dY*dY;
									if (fPixelDist2 < fClosest2)
									{
										fClosest2 = fPixelDist2;
										DebugRecorder::GetInstance()->SetHovered(pEvent);
										//pHoveredFrame = pFrame;
									}
								}
							}
						}
						pEvent = pEvent->mp_Next;
					}

					++siCycleState;
					if (bHasData)
					{
						--iFramesToDraw;
						if (!iFramesToDraw)
						{
							break;
						}
					}

					pFrame = pFrame->mp_Next;
				}

				//Loop around if needed
				if (!pFrame)
				{
					siCycleState = 0;
				}
			}

			sm_iGlobalAlphaScale = iCached;
			return bSelected;
		}

		void DebugRecorder::AddEventOptions(const EventBase &rEvent, const Frame &rFrame, TextOutputVisitor &rText, bool &bMouseDown)
		{
			//Add options that are only valid when an event is selected
			DebugRecorder* pRecorder = DebugRecorder::GetInstance();
			if (!Verifyf(pRecorder, "WFT?"))
			{
				return;
			}

			bool bDrawOthers = DrawOthersData::GetDrawOthers(rEvent.GetEventSubType());
			if (rText.AddLine("Draw others[%s]", bDrawOthers ? "X" : " ") && bMouseDown)
			{
				bDrawOthers = !bDrawOthers;
				DrawOthersData::SetDrawOthers(rEvent.GetEventSubType(), bDrawOthers);
			}
				
			//Add any options specific to this event
			rEvent.AddEventOptions(rFrame, rText, bMouseDown);
		}

		void DebugRecorder::DebugDraw(bool bPaused)
		{
			if (!m_bDisplayOn)
			{
				return;
			}

			//Make sure any triggered allocations use the right heap
			DR_MEMORY_HEAP();

			if (bPaused && !m_bWasPaused)
			{
				//Select a decent anchor frame
				mp_AnchorFrame = mp_LastFrame;
				if (mp_AnchorFrame)
				{
					int iRollback = 20;
					while (iRollback && mp_AnchorFrame->mp_Prev)
					{
						--iRollback;
						mp_AnchorFrame = mp_AnchorFrame->mp_Prev;
					}
				}
			}
			m_bWasPaused = bPaused;

			//Skip our playback through multiple frames
			if (m_bPlaybackOn && bPaused)
			{
				const debugPlayback::Frame* pSelectedFrame = GetSelectedFrame();
				if (pSelectedFrame)
				{
					pSelectedFrame = pSelectedFrame->mp_Next;
				}
				if (!pSelectedFrame)
				{
					pSelectedFrame = GetFirstFramePtr();
				}

				Select(pSelectedFrame);
			}

			//Allow App debug callback
			OnStartDebugDraw();

			//Yucky. But functional for now...
			OnScreenOutput::sm_iFrameCounter++;

			//Make sure we're 
			grcLighting(false);
			grcColor(Color32(255,255,255,255));

			//Store mouse coords for hover functionality
			const float  mouseScreenX = static_cast<float>(ioMouse::GetX());
			const float  mouseScreenY = static_cast<float>(ioMouse::GetY());

			//Make sure we just hit the down button presses (change to up?)
			bool bMouseDown = (ioMouse::GetPressedButtons() & ioMouse::MOUSE_LEFT) != 0;
			bool bMouseRightDown = (ioMouse::GetPressedButtons() & ioMouse::MOUSE_RIGHT) != 0;

			//Reset the hovered event so we can highlight it again, dead mans switch
			mp_WasHoveredEvent = mp_HoveredEvent;
			mp_HoveredEvent = 0;

			const Frame *pNewHoveredFrame = 0;

			//Need to draw the 3d options first to get the text on top
			bool bDraw3d = true;
			if (IsRecording() && !bPaused)
			{
				bDraw3d = false;
			}

			if(!mp_AnchorFrame)
			{
				mp_AnchorFrame = mp_FirstFrame;
				if (!mp_AnchorFrame)
				{
					bDraw3d = false;
				}
			}

			sysCriticalSection critSection(m_DataCSToken);

			//Draw
			bool bMenuItemSelected = false;
			bool bDrewHoveredEvent3d = false;
			if (bDraw3d)
			{
				//Make sure we've cleared any matrices.
				grcWorldIdentity();

				//Draw 3d first so that text overlays it
//				bool bDrewSingleEvent3d = false;
				if (!mp_LastHoveredData)
				{
					//If hovering we we always draw that frames events
					if (	mp_HoveredFrame 
						&&	(mp_HoveredFrame != mp_SelectedFrame)
						)
					{
						bMenuItemSelected |= mp_HoveredFrame->DebugDraw(eDrawFrameHovered, mouseScreenX, mouseScreenY, bMouseDown, *this);
						OnDrawFrame(mp_HoveredFrame);
					}
					else if (mp_SelectedFrame)
					{
						bMenuItemSelected |= mp_SelectedFrame->DebugDraw(eDrawFrameHovered, mouseScreenX, mouseScreenY, bMouseDown, *this);
						OnDrawFrame(mp_SelectedFrame);
					}
				}

				//Might be drawing all of some event types
				bMenuItemSelected = DrawOthers3d(mouseScreenX, mouseScreenY);

				if (mp_SelectedEvent)
				{
					//Remember we already drew 3d
//					bDrewSingleEvent3d = true;

					//Also draw 3d info for our selected event, make sure we draw it post others to draw on top
					mp_SelectedEvent->DebugDraw3d(eDrawSelected);
				}

				if (mp_WasHoveredEvent)
				{
					//Do the hover drawing for the event
					mp_WasHoveredEvent->DebugDraw3d(eDrawHovered);

					if (bMouseDown)
					{
						Select(0, mp_WasHoveredEvent, false, true);
						bMenuItemSelected = true;
						bMouseDown = false;
					}

					//Remember we already drew 3d
//					bDrewSingleEvent3d = true;

					//Remember 
					bDrewHoveredEvent3d = true;
				}
				


				//if (!bDrewSingleEvent3d)
				//{
				//	//If a frame is selected we draw the 3d for it even if hovering
				//	if (mp_SelectedFrame)
				//	{
				//		bMenuItemSelected |= mp_SelectedFrame->DebugDraw(eDrawNormal, mouseScreenX, mouseScreenY, bMouseDown, *this);
				//	}

				//	//But otherwise we will draw the top frame in the list if nothing is selected
				//	else if (mp_AnchorFrame && !mp_SelectedFrame)
				//	{
				//		bMenuItemSelected |= mp_AnchorFrame->DebugDraw(eDrawNormal, mouseScreenX, mouseScreenY, bMouseDown, *this);
				//	}
				//}
			}

			//Give app level a chance to draw some 3d stuff
			OnDebugDraw3d(mp_WasHoveredEvent);

			//Switch to a 2d display mode
			grcViewport* prevViewport=grcViewport::GetCurrent();
			grcViewport screenViewport;

			grcViewport::SetCurrent(&screenViewport);
			grcViewport::GetCurrent()->Screen();

			grcStateBlock::SetBlendState(grcStateBlock::BS_CompositeAlpha);
			grcStateBlock::SetRasterizerState(grcStateBlock::RS_NoBackfaceCull);
			grcStateBlock::SetDepthStencilState(grcStateBlock::DSS_IgnoreDepth);
			//Store this -  later on we use this to decide if we selected an event based
			//				on a menu where we might want to toggle the selected event
			//const EventBase *p3dHoveredEvent = mp_HoveredEvent;

			OnScreenOutput MenuOutput;
			MenuOutput.m_fXPosition = 50.0f;
			MenuOutput.m_fYPosition = YStart;
			MenuOutput.m_fMaxY = YMax;
			MenuOutput.m_fMouseX = mouseScreenX;
			MenuOutput.m_fMouseY = mouseScreenY;
			MenuOutput.m_fPerLineYOffset = YLineOffset;
			MenuOutput.m_Color.Set(160,160,160,255);
			MenuOutput.m_BackGroundColor.Set(80,80,80,255);
			MenuOutput.m_HighlightColor.Set(255,0,0,255);
			MenuOutput.AddLine("--MENU--");

			OnScreenOutput LHSOutput;
			LHSOutput.m_iMaxStringLength = m_bDrawElementsList ? 60 : 90;
			LHSOutput.m_fXPosition = XMainDisplay;
			LHSOutput.m_fYPosition = YStart;
			LHSOutput.m_fMaxY = YChannel;
			LHSOutput.m_fMouseX = mouseScreenX;
			LHSOutput.m_fMouseY = mouseScreenY;
			LHSOutput.m_fScale = FontScale;
			LHSOutput.m_fPerLineYOffset = YLineOffset;
			LHSOutput.m_Color.Set(160,160,160,255);
			LHSOutput.m_BackGroundColor.Set(128,128,128,128);
			LHSOutput.m_HighlightColor.Set(255,255,255,255);

			//Add frame specific options if we have one
			bMenuItemSelected |= AppOptions(MenuOutput, LHSOutput, bMouseDown, bPaused);

			//Flash recording details while recording
			LHSOutput.m_bDrawBackground = IsRecording() ? (TIME.GetFrameCount() & 0x7) > 3 : false;
			LHSOutput.AddLine("Start frame: %d [last: %d]", m_iFirstFrame, m_iLastRecordedFrame);
			LHSOutput.m_bDrawBackground = false;
			if (IsRecording())
			{
				//Display recording status
				if (!bPaused)
				{
					//LHSOutput.PushIndent();
					float fNBRecorded = ((float)DR_MemoryHeapHelper::GetMemoryAllocated()) / (1024.0f*1024.0f);
					LHSOutput.AddLine("Recorded: %03f MB, %d events", fNBRecorded, EventBase::sm_EventCount);
					//LHSOutput.PopIndent();
				}

				bMenuItemSelected |= menu_StartRecording.DrawAndExecute(MenuOutput, bMouseDown);
				bMenuItemSelected |= menu_StopRecording.DrawAndExecute(MenuOutput, bMouseDown);

				static int s_iDrawRecent;
				const int kMaxRecentFramesToDraw = 0x7;
				if (MenuOutput.AddLine("DrawRecent[%d]", s_iDrawRecent) && bMouseDown)
				{
					bMenuItemSelected = true;
					s_iDrawRecent++;
					s_iDrawRecent %= kMaxRecentFramesToDraw;
				}

				if(s_iDrawRecent)
				{
					const Frame *pLastFrame = mp_LastFrame;
					int iCount = s_iDrawRecent;
					while (pLastFrame && iCount)
					{
						bMenuItemSelected |= pLastFrame->DebugDraw(eDrawNormal, mouseScreenX, mouseScreenY, bMouseDown, *this);
						pLastFrame = pLastFrame->mp_Prev;
						--iCount;
					}
				}

				if (!bPaused)
				{
					bMenuItemSelected |= AddChannelMenu(MenuOutput, LHSOutput, bMouseDown, bPaused);
					ClearSelection();

					grcViewport::SetCurrent(prevViewport);

					OnEndDebugDraw(bMenuItemSelected);
					return;
				}
				
				//LHSOutput.AddLine("GAME IS PAUSED SO ALLOWING BROWSING OF EVENTS!!!");
			}
			else
			{
				if(menu_StartRecording.DrawAndExecute(MenuOutput, bMouseDown))
				{
					grcViewport::SetCurrent(prevViewport);
					OnEndDebugDraw(bMenuItemSelected);
					return;	//Invalidates crap - avoid crash
				}
			}

			//Display recorded status
			//LHSOutput.AddLine("Current recording details:");
			//LHSOutput.PushIndent();
			//LHSOutput.AddLine("Start frame %d, Last recorded frame %d, Stopped frame %d", m_iFirstFrame, m_iLastRecordedFrame, m_iLastFrame);
			float fNBRecorded = ((float)DR_MemoryHeapHelper::GetMemoryAllocated()) / (1024.0f*1024.0f);
			LHSOutput.AddLine("Recorded: %03f MB, %d events", fNBRecorded, EventBase::sm_EventCount);
			if (mp_HoveredFrame)
			{
				mp_HoveredFrame->DebugText(LHSOutput);
			}
			else if (mp_SelectedFrame)
			{
				mp_SelectedFrame->DebugText(LHSOutput);
			}

			//if (mp_SelectedFrame)
			//{
			//	LHSOutput.AddLine("Selected frame: %d", mp_SelectedFrame->m_FrameIndex);
			//}
			//else	TMS: Too much real estate!!
			//{
			//	LHSOutput.AddLine("Tips:");
			//	LHSOutput.PushIndent();
			//	LHSOutput.AddLine("- Use the mouse to select a frame on the list of recorded frames at the left hand side");
			//	LHSOutput.AddLine("- Use the mouse wheel to scroll the view of frames");
			//	LHSOutput.AddLine("- Hover over a frame with no event selected to see all events in that frame");
			//	LHSOutput.PopIndent();
			//}
			//LHSOutput.PopIndent();

			if (!mp_AnchorFrame)
			{
				//Present the load option
				if (MenuOutput.AddLine("Load") && bMouseDown)
				{
					Widget_Load();
				}

				bMenuItemSelected |= AddChannelMenu(MenuOutput, LHSOutput, bMouseDown, bPaused);

				//Nothing more to display!
				grcViewport::SetCurrent(prevViewport);

				OnEndDebugDraw(bMenuItemSelected);
				return;
			}

			if (m_bPlaybackOn)
			{
				if ((TIME.GetFrameCount() & 0x7) > 0x3)
				{
					MenuOutput.m_bDrawBackground = true;
				}

				if (MenuOutput.AddLine("[Pause]") && bMouseDown)
				{
					bMenuItemSelected = true;
					m_bPlaybackOn = !m_bPlaybackOn;
					bMouseDown = false;
				}
				MenuOutput.m_bDrawBackground = false;
			}
			else
			{
				if (MenuOutput.AddLine("[Play]") && bMouseDown)
				{
					bMenuItemSelected = true;
					m_bPlaybackOn = !m_bPlaybackOn;
					bMouseDown = false;
				}
			}

			if (m_bFilterNeedsRefresh)
			{
				RefreshFilteredState();
			}

			//Draw the lists over the top
			//TOnScreenList<Frame> framesToScreen(YLineOffset);
			//framesToScreen.m_fScale = FontScale;
			//framesToScreen.m_fXPosition = XColumn1;
			//framesToScreen.m_fYPosition = YStart;
			//framesToScreen.m_fMaxY = m_Graph.IsEnabled() && m_Graph.HasData() ? YChannel : YMax;
			//framesToScreen.m_fMouseX = mouseScreenX;
			//framesToScreen.m_fMouseY = mouseScreenY;
			//framesToScreen.m_Color.Set(160,160,160,255);
			//framesToScreen.m_HighlightColor.Set(255,0,0,255);
			//if(framesToScreen.Draw(mp_AnchorFrame, mp_SelectedFrame, pNewHoveredFrame, m_bDrawFramesList, "FRAMES") && bMouseDown)
			//{
			//	m_bDrawFramesList = !m_bDrawFramesList;
			//}

			if (pNewHoveredFrame && !bDrewHoveredEvent3d)
			{
				bMenuItemSelected |= pNewHoveredFrame->DebugDraw(eDrawFrameHovered, mouseScreenX, mouseScreenY, bMouseDown, *this);
			}
			//Draw selected frame event list
			if (mp_SelectedEvent)
			{
				mp_SelectedEvent->WhileSelected();
			}
			//else if (mp_SelectedFrame) //Only show event tips when we're not showing frame tips.
			//{
			//	LHSOutput.AddLine("Tips:");
			//	LHSOutput.PushIndent();
			//	LHSOutput.AddLine("- Use the mouse wheel to scroll through all events in a frame ");
			//	LHSOutput.AddLine("- Hover over an event to see its details");
			//	LHSOutput.AddLine("- Use the left mouse button to select an event");
			//	LHSOutput.AddLine("- Use the 'break' option in the top left menu to trap code on a given event");
			//	LHSOutput.PopIndent();
			//}


			//Draw list
			if(mp_SelectedEvent)
			{
				OnScreenOutput eventsDetailsToScreen(YLineOffset);
				eventsDetailsToScreen.m_iMaxStringLength = 30;
				eventsDetailsToScreen.m_fXPosition = XColumn3;
				eventsDetailsToScreen.m_fYPosition = YStart;
				eventsDetailsToScreen.m_fMaxY = YMax;
				eventsDetailsToScreen.m_fScale = FontScale;
				eventsDetailsToScreen.m_fMouseX = mouseScreenX;
				eventsDetailsToScreen.m_fMouseY = mouseScreenY;
				eventsDetailsToScreen.m_Color.Set(160,160,160,255);
				eventsDetailsToScreen.m_HighlightColor.Set(255,0,0,255);

				if (eventsDetailsToScreen.AddLine("<-GRAPHABLE->") && bMouseDown)
				{
					m_bDrawElementsList = !m_bDrawElementsList;
					bMouseDown = false;
					bMenuItemSelected = true;
				}

				if (m_bDrawElementsList)
				{
					bMenuItemSelected |= m_Graph.DisplayEventDetails(eventsDetailsToScreen, mp_SelectedEvent, bMouseDown);
				}
			}

			if (!m_bPlaybackOn && (mp_AnchorEvent || mp_HoveredFrame))
			{
				//Draw list
				OnScreenEventList eventsToScreen(YLineOffset);
				eventsToScreen.m_iMaxStringLength = 30;
				eventsToScreen.m_fXPosition = XColumn2;
				eventsToScreen.m_fYPosition = YStart;
				eventsToScreen.m_fMaxY = m_Graph.IsEnabled() && m_Graph.HasData() ? YChannel : YMax;;
				eventsToScreen.m_fScale = FontScale;
				eventsToScreen.m_fMouseX = mouseScreenX;
				eventsToScreen.m_fMouseY = mouseScreenY;
				eventsToScreen.m_Color.Set(160,160,160,255);
				eventsToScreen.m_HighlightColor.Set(255,0,0,255);

				if (mp_HoveredFrame)
				{
					char eventListTitle[256];
					formatf(eventListTitle, "Hovered: %d", mp_HoveredFrame->m_FrameIndex);
					bMenuItemSelected |= eventsToScreen.Draw(mp_HoveredFrame->mp_FirstEvent, mp_SelectedEvent, mp_HoveredEvent, mp_HighlightData, true, bMenuItemSelected, bMouseDown, eventListTitle);
				}
				else if (mp_SelectedFrame)
				{
					char eventListTitle[256];
					formatf(eventListTitle, "<-Frame: %d->", mp_SelectedFrame ? (int)mp_SelectedFrame->m_FrameIndex : -1);
					if (eventsToScreen.Draw(mp_AnchorEvent, mp_SelectedEvent, mp_HoveredEvent, mp_HighlightData, m_bDrawEventsList, bMenuItemSelected, bMouseDown, eventListTitle))
					{
						if (bMouseDown)
						{
							if (!m_bDrawEventsList)
							{
								//Switch drawing on
								m_bDrawEventsList = true;
							}
							else
							{
								//Toggle on the index drawing
								if (!EventBase::smb_DrawIndex)
									EventBase::smb_DrawIndex = true;
								else
								{
									//Toggle off all drawing
									m_bDrawEventsList = false;
									EventBase::smb_DrawIndex = false;
								}
							}
							bMenuItemSelected = true;
							bMouseDown = false;
						}
					}
				}
			}

			if (mp_SelectedEvent && !m_bPlaybackOn)
			{
				//Add option to throw a breakpoint on the event
				if (MenuOutput.AddLine("Break on event [%s]", CallstackHelper::sm_bTrapCallstack ? "X" : " ") && bMouseDown)
				{
					bMenuItemSelected = true;
					bMouseDown = false;

					CallstackHelper::sm_bTrapCallstack =! CallstackHelper::sm_bTrapCallstack;
					if (CallstackHelper::sm_bTrapCallstack)
					{
						CallstackHelper::sm_iTrapCallstack = mp_SelectedEvent->m_Callstack;
					}
				}

				if (MenuOutput.AddLine("Print to TTY") && bMouseDown)
				{
					bMenuItemSelected = true;
					bMouseDown = false;

					TTYTextOutputVisitor rText;
					rText.AddLine("");
					rText.AddLine("--------------------------------------------");
					mp_SelectedEvent->DebugText( rText );
					rText.AddLine("--------------------------------------------");
					rText.AddLine("");
				}

				//Add any selected event specific options
				AddEventOptions(*mp_SelectedEvent, *mp_SelectedFrame, MenuOutput, bMouseDown);

				//Also draw details of selected event
				//	unless we are hovering over something in which case that takes precedence
				//	or playing back an event stream, in which case free up real estate
				if (!mp_WasHoveredEvent)
				{
					mp_SelectedEvent->DebugText( LHSOutput );
				}
			}

			//Also hovered event details
			if (mp_WasHoveredEvent && !m_bPlaybackOn)
			{
				mp_WasHoveredEvent->DebugText( LHSOutput );
			}

			//Draw the graphing widget
			if (m_Graph.HasData())
			{
				bMenuItemSelected |= m_Graph.DebugDraw(mouseScreenX, mouseScreenY, bMouseDown, mp_SelectedEvent, mp_WasHoveredEvent, m_ZoomData);
			}

			if (!m_EventTimeLine.IsInited())
			{
				m_EventTimeLine.Refresh(mp_FirstFrame);
				m_EventTimeLine.SetSelectedDataCounts(mp_HighlightData);
			}

			bool bReanchorFrameListOnSelection=false;
			const Frame *pTimeLineFrame = m_EventTimeLine.Display(350.0f, 630.0f, 1200.0f, 670.0f, mouseScreenX, mouseScreenY, Color32(64,128,64,128), m_ZoomData);
			pNewHoveredFrame = pTimeLineFrame ? pTimeLineFrame : pNewHoveredFrame;
			SetHovered(pNewHoveredFrame);
			if (pNewHoveredFrame)
			{
				//Hovering over a frame for easy selection
				bReanchorFrameListOnSelection = true;
			}

			if (MenuOutput.AddLine("Save") && bMouseDown)
			{
				bMouseDown = false;
				Widget_Save();
			}

			if (MenuOutput.AddLine("Load") && bMouseDown)
			{
				bMouseDown = false;
				Widget_Load();
			}

			//Removed as it was cluttering up the display - perhaps put in widget?
			//if (MenuOutput.AddLine("TimeLine[%s]", m_EventTimeLine.ShouldForceDraw() ? "X" : " ") && bMouseDown)
			//{
			//	m_EventTimeLine.SetForceDraw( !m_EventTimeLine.ShouldForceDraw() );
			//	bMenuItemSelected = true;
			//	bMouseDown = false;
			//}

			bMenuItemSelected |= AddChannelMenu(MenuOutput, LHSOutput, bMouseDown, bPaused);

			//Back to our previous draw mode
			grcViewport::SetCurrent(prevViewport);

			//Last chance for the app to draw something (this would go on top of all the text etc).
			OnEndDebugDraw(bMenuItemSelected);

			bMenuItemSelected |= MenuOutput.m_bHoverHit || LHSOutput.m_bHoverHit;

			//Some final mouse cleanup
			if (bMouseDown && !bMenuItemSelected)
			{
				//Mouse button is being pressed
				if (mp_HoveredFrame)
				{
					//if (mp_HoveredFrame != mp_SelectedFrame)	//Don't do this, allow Select to toggle
					{
						Select(mp_HoveredFrame, 0, false, bReanchorFrameListOnSelection);//, true);
					}
				}
				else if (mp_HoveredEvent) // && ())
				{
					//if (mp_HoveredEvent != p3dHoveredEvent)
					{
						//Coming from a menu rather than a 3d choice
						Select(0, mp_HoveredEvent);
					}
					//else
					//{

					//}
				}
				else
				{
					//Don't do this, allow select to toggle
					//Select(mp_SelectedFrame, 0);
				}
			}
			else if (bMouseRightDown && mp_HoveredEvent)
			{
				ToggleSetFilterToJustThis( mp_HoveredEvent->GetEventSubType() );

				Select(0, mp_HoveredEvent);
			}
			else if (ioMouse::HasWheel() && (ioMouse::GetDZ()!=0))
			{
				s32 dz = ioMouse::GetDZ();
				if(mp_HoveredFrame)
				{
					while (dz)
					{
						if (dz < 0)
						{
							ScrollFrameNext();
							++dz;
						}
						else
						{
							ScrollFramePrev();
							--dz;
						}
					}
				}
				else if (mp_HoveredEvent)
				{
					while (dz)
					{
						if (ioMouse::GetDZ() < 0)
						{
							ScrollEventNext();
							++dz;
						}
						else
						{
							ScrollEventPrev();
							--dz;
						}
					}
				}
			}

			//CTRL - Left or right 
			if (mp_SelectedEvent && ioMapper::DebugKeyDown(KEY_CONTROL))
			{
				const char *toLookFor = mp_SelectedEvent->GetEventSubType();
				const Frame *pFrame = FindFrame(mp_SelectedEvent);
				if (ioMapper::DebugKeyPressed(KEY_Z))
				{
					const EventBase *pEvent = mp_SelectedEvent->mp_Prev;
					bool bDone=false;
					while (pFrame && !bDone)
					{
						while (pEvent)
						{
							if (	(pEvent->GetEventSubType() == toLookFor)
								&&	(pEvent->IsEventDataRelated(*mp_SelectedEvent))	)

							{
								Select(pFrame, pEvent);
								bDone = true;
								break;
							}
							pEvent = pEvent->mp_Prev;
						}
						pFrame = pFrame->mp_Prev;
						if (pFrame)
						{
							pEvent = pFrame->mp_LastEvent;
						}
					}
				}
				else if (ioMapper::DebugKeyPressed(KEY_X))
				{
					const EventBase *pEvent = mp_SelectedEvent->mp_Next;
					bool bDone=false;
					while (pFrame && !bDone)
					{
						while (pEvent)
						{
							if (	(pEvent->GetEventSubType() == toLookFor)
								&&	(pEvent->IsEventDataRelated(*mp_SelectedEvent))	)
							{
								Select(pFrame, pEvent);
								bDone = true;
								break;
							}
							pEvent = pEvent->mp_Next;
						}
						pFrame = pFrame->mp_Next;
						if (pFrame)
						{
							pEvent = pFrame->mp_FirstEvent;
						}
					}
				}
			}
		}

		void DebugDraw(bool bPaused)
		{
			DebugRecorder *pInstance = DebugRecorder::GetInstance();
			if (pInstance)
			{
				pInstance->DebugDraw(bPaused);
			}
		}

		void AddWidgets(bkBank &rBank)
		{
			DebugRecorder *pInstance = DebugRecorder::GetInstance();
			if (pInstance)
			{
				pInstance->AddWidgets(rBank);
			}
		}
	}
}

#endif //DR_ENABLED
