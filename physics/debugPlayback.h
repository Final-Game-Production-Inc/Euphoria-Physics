
// 
// physics/debugplayback.h 
// 
// Copyright (C) 1999-2015 Rockstar Games.  All Rights Reserved. 
// 

#ifndef PHYSICS_DEBUGPLAYBACK_H 
#define PHYSICS_DEBUGPLAYBACK_H 

#include "system\memory.h"

//DR_ shorthand for (D)ebug (R)ecorder
#define DR_ENABLED (1 && (!__SPU) && __DEV && !__XENON)
#if DR_ENABLED
#if __DEV
	#define DR_DEV_ONLY(x) x
#else
	#define DR_DEV_ONLY(x)
#endif
#define DR_ONLY(x) x

#include "atl/array.h"
#include "atl/binmap.h"
#include "atl/map.h"
#include "data/base.h"
#include "parser/macros.h"
#include "string/string.h"
#include "system/criticalsection.h"
#include "vector/color32.h"
#include "vectormath/vec3v.h"
#include "vectormath/mat34v.h"

#define DR_MEMORY_HEAP() DR_MemoryHeapHelper drMemoryHeap;

#define DR_EVENT_CUSTOM_TYPE_DECL(x)\
	public:\
	virtual bool IsTypeDisplayEnabled() const { return sm_EventTypeForList.m_bEnabled; }\
	virtual debugPlayback::DR_EventType& GetEventTypeData() const { return sm_EventTypeForList; }\
	static debugPlayback::DR_EventType sm_EventTypeForList;

#define DR_EVENT_DECL(x)\
	DR_EVENT_CUSTOM_TYPE_DECL(x)\
	static __forceinline const char* GetStaticEventType() {return #x;}\
	virtual const char* GetEventType() const {return GetStaticEventType();}


#define DR_EVENT_IMP(x)	DR_EventType x::sm_EventTypeForList(#x);

#define DR_EVENT_ENABLED(x) (Unlikely(x::sm_EventTypeForList.m_bEnabled || debugPlayback::g_bOverrideEventsOn))

namespace rage {
	class phInst;
	class bkBank;
	class parTreeNode;

	class DR_MemoryHeapHelper
	{
	public:
		static int GetMemoryAllocated();
		static bool NeedToUseLessMemory();
		static size_t GetMemoryAllowance();
		static int CurrentTrackedAllocation();

		DR_MemoryHeapHelper();
		~DR_MemoryHeapHelper();
	};

	namespace debugPlayback	{
		
		extern __THREAD bool g_bOverrideEventsOn;
		class DebugRecorder;
		struct DataBase;
		struct Frame;
		enum eDrawFlags
		{
			eDrawNormal = BIT(0),
			eDrawHovered = BIT(1),
			eDrawSelected = BIT(2),
			eDrawFrameHovered = BIT(3)
		};

		struct DR_EventType
		{
			//Static initializer puts in list
			DR_EventType(const char *pTypeName, bool bEnabled=false)
				:m_Name(pTypeName)
				,m_bEnabled(bEnabled)
			{
				mp_Next = smp_First;
				if (smp_First)
				{
					smp_First->mp_Prev = this;
				}
				smp_First = this;
			}
			static DR_EventType *smp_First;

			static void DisableAll()
			{
				DR_EventType *pElem = smp_First;
				while (pElem)
				{
					pElem->m_bEnabled = false;
					pElem = pElem->mp_Next;
				}
			}

			static void EnableAll()
			{
				DR_EventType *pElem = smp_First;
				while (pElem)
				{
					pElem->m_bEnabled = true;
					pElem = pElem->mp_Next;
				}
			}

			static void Enable(const char *pName)
			{
				DR_EventType *pElem = smp_First;
				while (pElem)
				{
					if (!strcmp(pElem->m_Name, pName))
					{
						pElem->m_bEnabled = true;
						return;
					}
					pElem = pElem->mp_Next;
				}
			}

			static void AddEnabled(atArray<ConstString> &rOutput)
			{
				DR_EventType *pElem = smp_First;
				int iCount = 0;
				while (pElem)
				{
					if (pElem->m_bEnabled)
					{
						++iCount;
					}
					pElem = pElem->mp_Next;
				}
				rOutput.Reset();
				rOutput.Resize(iCount);

				pElem = smp_First;
				iCount = 0;
				while (pElem)
				{
					if (pElem->m_bEnabled)
					{
						rOutput[iCount] = pElem->m_Name;
						++iCount;
					}
					pElem = pElem->mp_Next;
				}
			}

			DR_EventType *mp_Next;
			DR_EventType *mp_Prev;
			const char *m_Name;
			bool m_bEnabled;
		};

		template<typename _TEventType>
		struct FilterToEvent
		{
			bool m_bIsSet;
			FilterToEvent()
			{	
				m_bIsSet = DR_EVENT_ENABLED(_TEventType);
				if (m_bIsSet)
				{
					debugPlayback::g_bOverrideEventsOn = true;
				}
			}

			bool IsSet() const
			{
				return m_bIsSet;
			}

			void Reset()
			{
				if (m_bIsSet)
				{
					m_bIsSet = false;
					debugPlayback::g_bOverrideEventsOn = false;
				}
			}

			~FilterToEvent()
			{	
				Reset();
			}
		};

		struct TextOutputVisitor
		{
			virtual bool AddLine(const char *fmt, ...) = 0;	//Return true if hovered over
			virtual void PushIndent() {}
			virtual void PopIndent() {}
			virtual ~TextOutputVisitor(){}
		};

		struct TTYTextOutputVisitor : public TextOutputVisitor
		{
			int m_iIndent;
			virtual bool AddLine(const char *fmt, ...);
			virtual void PushIndent() {++m_iIndent;}
			virtual void PopIndent() {--m_iIndent;}
			TTYTextOutputVisitor()
				:m_iIndent(0)
			{	}
			virtual ~TTYTextOutputVisitor(){}
		};

        struct OnScreenOutput : public TextOutputVisitor
        {
            static int sm_iFrameCounter;
            u32 m_iIndent;
            u32 m_iMaxStringLength;
            Color32 m_Color;
            Color32 m_HighlightColor;
			Color32 m_BackGroundColor;
            float m_fXPosition;
            float m_fYPosition;
			float m_fMaxY;
            float m_fPerLineYOffset;
            float m_fPerTabOffset;
            float m_fScale;
            float m_fMouseX;
            float m_fMouseY;
            bool m_bDrawBackground;
            bool m_bForceColor;
			bool m_bAllowMouseHover;
			bool m_bHoverHit;

            OnScreenOutput(float fPerLineOffset=20.0f, float fPerTabOffset=15.0f)
                :m_iIndent(0)
                ,m_iMaxStringLength(0)
                ,m_fXPosition(0.0f)
                ,m_fYPosition(0.0f)
				,m_fMaxY(1024.0f)
                ,m_fPerLineYOffset(fPerLineOffset)
                ,m_fPerTabOffset(fPerTabOffset)
                ,m_fScale(1.0f)
                ,m_fMouseX(0.0f)
                ,m_fMouseY(0.0f)
                ,m_bDrawBackground(false)
                ,m_bForceColor(false)
				,m_bAllowMouseHover(true)
				,m_bHoverHit(false)
            {
                m_Color.Set(255,255,255,255);
				m_BackGroundColor.Set(32,32,32,32);
                m_HighlightColor = m_Color;
            }

            virtual bool AddLine(const char *fmt=0, ...);
            virtual void PushIndent();
            virtual void PopIndent();
            virtual ~OnScreenOutput();
        };

		struct CallstackHelper
		{
			static size_t sm_iTrapCallstack;
			static bool sm_bTrapCallstack;
			size_t m_iCallstackID;
			CallstackHelper(size_t id)
				:m_iCallstackID(id)
			{

			}
			CallstackHelper();
		};

		struct ItemBase
		{
			virtual void DebugText(TextOutputVisitor &) const { }
			virtual bool DrawListText(TextOutputVisitor &) const { return false; }
			virtual void DebugDraw3d(eDrawFlags /*drawFlags*/) const {}
			ItemBase();
			virtual ~ItemBase(){}

			PAR_PARSABLE;
		};

		//Base level for data stored with the recording
		struct DataBase : public ItemBase
		{
			DataBase();
			virtual ~DataBase(){}
			void OnPreLoad(parTreeNode *pNode);
			void OnPostLoad();
			virtual bool HasFilterOption() const {return false;}
			virtual const char* GetFilterText() const {return "!!INVALID!!";}
			virtual void OnFilterToggle(){}
			virtual bool IsFilterActive(){return false;}
			virtual bool IsNewObjectData() const {return false;}

			PAR_PARSABLE;
		};


		struct EventBase : public ItemBase
		{
			EventBase(const CallstackHelper rCallstack);
			EventBase();
			virtual ~EventBase()
			{
				--sm_EventCount;
			}

			void Init(const CallstackHelper rCallstack)
			{			
				m_Callstack = rCallstack.m_iCallstackID;
			}

			virtual bool Replay(phInst * /*pInst*/) const { return false; }
			virtual void DebugText(TextOutputVisitor &rText) const;
			virtual bool DebugEventText(TextOutputVisitor &) const ;
			//Return true when the text consumes a mouse click
			virtual bool DrawListText(TextOutputVisitor &rText) const;
			virtual void OnAddToList(DebugRecorder &){}
			virtual void WhileSelected() const {}
			virtual void OnSelection() const {}
			//Return true when consuming mouse click
			virtual void AddEventOptions(const Frame &, TextOutputVisitor &, bool &/*bMouseDown*/) const {}
			//Return true when filling out a valid position
			virtual bool GetEventPos(Vec3V &) const {return false;}
			virtual const char* GetEventType() const = 0;
			virtual const char* GetEventSubType() const { return GetEventType(); }
			virtual const char* GetValueName(const char *pElemId) const { return pElemId; }
			virtual bool IsDataInvolved(const DataBase& ) const { return false; }
			virtual bool IsEventDataRelated(const EventBase& ) const { return false; }
			virtual bool IsTypeDisplayEnabled() const = 0;
			virtual debugPlayback::DR_EventType& GetEventTypeData() const = 0;
			virtual bool IsPhysicsEvent() const {return false;}
			EventBase *mp_Prev;
			EventBase *mp_Next;
			u32 m_iEventIndex;
			size_t m_Callstack;
			bool m_bIsDisplayEnabled;
			static bool smb_DrawIndex;
			static u32 sm_EventCount;
			PAR_PARSABLE;
		};

		struct SimpleLabelEvent : public EventBase
		{
			SimpleLabelEvent(const CallstackHelper rCallstack);
			SimpleLabelEvent();

			void Set(const char *pLabel, const char *pText);

			virtual const char* GetEventSubType() const
			{
				return m_Label.GetCStr();
			}

			atHashString m_Label;
			ConstString m_Description;

			DR_EVENT_DECL(SimpleLabelEvent)
			PAR_PARSABLE;
		};

		struct SimpleLineEvent : public EventBase
		{
			SimpleLineEvent(const CallstackHelper rCallstack)
				:EventBase(rCallstack)
				,m_Start(V_ZERO)
				,m_End(V_ZERO)
				,m_ColorStart(0)
				,m_ColorEnd(0)
			{}
			SimpleLineEvent()
				:m_Start(V_ZERO)
				,m_End(V_ZERO)
				,m_ColorStart(0)
				,m_ColorEnd(0)
			{}

			void Set(const char *pLabel, Vec3V_In vStart, Vec3V_In vEnd, u32 colorStart, u32 colorEnd)
			{
				m_Label = pLabel;
				m_Start = vStart;
				m_End = vEnd;
				m_ColorStart = colorStart;
				m_ColorEnd = colorEnd;
			}

			virtual const char* GetEventSubType() const
			{
				return m_Label.GetCStr();
			}

			virtual void DebugDraw3d(eDrawFlags /*drawFlags*/) const;
			virtual bool GetEventPos(Vec3V &rOut) const {rOut = (m_Start + m_End) * ScalarV(V_HALF); return true;}

			atHashString m_Label;
			Vec3V m_Start;
			Vec3V m_End;
			u32 m_ColorStart;
			u32 m_ColorEnd;

			DR_EVENT_DECL(SimpleLineEvent)
			PAR_PARSABLE;
		};

		struct SimpleSphereEvent : public EventBase
		{
			SimpleSphereEvent(const CallstackHelper rCallstack)
				:EventBase(rCallstack)
				,m_Pos(V_ZERO)
				,m_Radius(0.0f)
				,m_Color(0)
			{}
			SimpleSphereEvent()
				:m_Pos(V_ZERO)
				,m_Radius(0.0f)
				,m_Color(0)
			{}

			void Set(const char *pLabel, Vec3V_In vPos, float fRadius, u32 color)
			{
				m_Label = pLabel;
				m_Pos = vPos;
				m_Radius = fRadius;
				m_Color = color;
			}

			virtual const char* GetEventSubType() const
			{
				return m_Label.GetCStr();
			}

			virtual void AddEventOptions(const Frame &, TextOutputVisitor &, bool &/*bMouseDown*/) const;
			virtual void DebugDraw3d(eDrawFlags /*drawFlags*/) const;
			virtual bool GetEventPos(Vec3V &rOut) const {rOut = m_Pos; return true;}
			atHashString m_Label;
			Vec3V m_Pos;
			float m_Radius;
			u32 m_Color;

			DR_EVENT_DECL(SimpleSphereEvent)
			PAR_PARSABLE;
		};

		struct SimpleMatrixEvent : public EventBase
		{
			atHashString m_Label;
			float m_fScale;
			Mat34V m_Matrix;
			SimpleMatrixEvent()
				:m_fScale(0.0f)
			{

			}
			SimpleMatrixEvent(const CallstackHelper rCallstack)
				:EventBase(rCallstack)
				,m_fScale(0.0f)
			{}
			virtual const char* GetEventSubType() const
			{
				return m_Label.GetCStr();
			}
			virtual void DebugDraw3d(eDrawFlags /*drawFlags*/) const;
			virtual bool GetEventPos(Vec3V &rOut) const {rOut = m_Matrix.GetCol3(); return true;}
			void Set(const char *pLabel, Mat34V_In mat, float fScale)
			{
				m_Label = pLabel;
				m_Matrix = mat;
				m_fScale = fScale;
			}
			DR_EVENT_DECL(SimpleMatrixEvent)
			PAR_PARSABLE;
		};

		struct Frame
		{
			struct FrameData
			{
				atArray<EventBase*> m_pEvents;
				u32 m_FrameIndex;

				PAR_SIMPLE_PARSABLE;
			};
			void DebugText(TextOutputVisitor &) const ;
			bool DrawListText(TextOutputVisitor &) const ;
			bool DebugDraw(eDrawFlags drawFlags, float mouseX, float mouseY, bool bMouseDown, DebugRecorder &rRecorder) const ;
			Frame *mp_Next;
			Frame *mp_Prev;
			EventBase *mp_FirstEvent;
			EventBase *mp_LastEvent;
			u32 m_FrameIndex;
			u32 m_NetworkTime;
			u32 m_DisplayCount;
			static u32 sm_FrameCount;
			static u32 GetStaticFrameCount()
			{
				return sm_FrameCount;
			}
			Frame();
			~Frame();
		};
	
		struct Callstack
		{
			void Print(TextOutputVisitor &rText);
			void CacheStrings();
			void Init(size_t *pAddressArray, int iMaxArrayLimit);
			Callstack(const Callstack& rOther);
			Callstack();
			atArray<ConstString> m_SymbolArray;
			atArray<size_t> m_AddressArray;
			size_t m_iSignature;
			bool m_bIsScript;
			PAR_SIMPLE_PARSABLE;
		};

		struct TimeLineZoom
		{
			TimeLineZoom()
			{
				Reset(); 
			}

			void UpdateZoom(float xMinX, float xMaxX, float fMouseX, float fMouseOffset);
			void Reset()
			{
				m_LastMouseTrackFrame = 0;
				m_fLastMouseX = 0.0f;
				m_fZoom = 1.0f;
				m_fTimeLineOffset = 0.0f;
			}
			u32 m_LastMouseTrackFrame;
			float m_fLastMouseX;
			float m_fZoom;
			float m_fTimeLineOffset;
		};

		class GraphFloats
		{

			enum { eDrawMode_Nothing, eDrawMode_Values, eDrawMode_TimeLine, eDrawMode_All };
			//The graph maintains a set of tags associated with the current element.
			//Each data value is associated with an event
			//Due to this relationship graphs will be unstable when recording, so need to
			//be invalidated anytime the event stream is changed.

			struct GraphLineBase
			{
				ConstString m_EventTypeName;
				atArray<const EventBase *> m_Events;
				atArray<s32> m_Frames;
				
				enum eGraphLineType
				{
					eUnknown,
					eValueLine,
					eTimeLine
				};

				virtual eGraphLineType GetGraphLineType() const {return eUnknown; }
				virtual void Add(const EventBase *pEvent, float fNewValue, s32 iFrame);
				virtual void ExtendValueRange(float &/*fMinMin*/, float &/*fMaxMax*/) const {	}
				void DrawSquare(float fX, float fY, float dhX, float dhY, Color32 col) const;
				virtual const char *GetGraphLineName() const { return m_EventTypeName.c_str(); }

				virtual bool Matches(const char *pEventTypeName, const char * pValueName = 0) const
				{
					if (pValueName)
						return 0;

					return m_EventTypeName == pEventTypeName;
				}

				GraphLineBase(const char *pEventTypeName)
					:m_EventTypeName(pEventTypeName)
				{

				}

				GraphLineBase(const GraphLineBase& rOther)
					:m_EventTypeName(rOther.m_EventTypeName)
					,m_Frames(rOther.m_Frames)
					,m_Events(rOther.m_Events)
				{

				}

				virtual ~GraphLineBase(){}
			};

			struct GraphTimeLine : public GraphLineBase
			{
				GraphTimeLine(const char *pEventType)
					:GraphLineBase( pEventType )
				{

				}

				virtual eGraphLineType GetGraphLineType() const {return eTimeLine; }

				bool Draw(
					int iIndex, Color32 lineColor,
					int iFrameMin, int iFrameMax, 
					float fMinY, float fMaxY,float fGraphLeft, float fGraphRight,
					float fMouseX, float fMouseY,
					const EventBase * &pSelected, float &fDistToMouse2,
					const EventBase *pCurrentSelected, const EventBase *pCurrentHighlighted,
					TimeLineZoom &rZoom
					) const;
			};

			struct GraphFloatLine : public GraphLineBase
			{
				ConstString m_EventValueName;
				atArray<float> m_Values;
				float m_fMin;
				float m_fMax;
				virtual const char *GetGraphLineName() const { return m_EventValueName.c_str(); }
				GraphFloatLine(const char *pEventTypeName, const char *pEventValueName)
					:GraphLineBase(pEventTypeName)
					,m_EventValueName(pEventValueName)
					,m_fMax(-FLT_MAX)
					,m_fMin(FLT_MAX)
				{

				}

				GraphFloatLine(const GraphFloatLine &rOther)
					:GraphLineBase(rOther)
					,m_EventValueName(rOther.m_EventValueName)
					,m_Values(rOther.m_Values)
					,m_fMin(rOther.m_fMin)
					,m_fMax(rOther.m_fMax)
				{

				}

				virtual eGraphLineType GetGraphLineType() const {return eValueLine; }
				virtual void ExtendValueRange(float &fMinMin, float &fMaxMax) const;

				virtual bool Matches(const char *pEventTypeName, const char *pEventValueName) const
				{
					return GraphLineBase::Matches(pEventTypeName) && (m_EventValueName == pEventValueName);
				}

				void Add(const EventBase *pEvent, float fNewValue, s32 iFrame);

				bool Draw(
					bool bSelected, Color32 lineColor,
					int iFrameMin, int iFrameMax, 
					float fGraphLeft, float fGraphTop, float fGraphRight, float fGraphBottom,
					float fMinY, float fMaxY,
					float fMouseX, float fMouseY,
					const EventBase * &pSelected, float &fDistToMouse2,
					float &fHoveredValue,
					const EventBase *pCurrentSelected, const EventBase *pCurrentHighlighted,
					TimeLineZoom &rZoom
					) const;
			};

			atArray<ConstString> m_NameArray;
			atArray<GraphLineBase*> m_GraphLines;
			const EventBase *mp_LastSelected;

			EventBase *mp_CurrentEvent;
			GraphLineBase *mp_StoredGraph;
			s32 m_iSelectedTag;
			float m_fLineMarker;
			s32 m_iHovered;
			float m_fX;
			float m_fY;
			float m_fGraphOffset;
			float m_fWidth;
			float m_fHeight;
			int m_eDrawGraph;
			bool m_bShareScale;

			float GetFloatValue(const EventBase * pEvent, int iValue) const;
			void BuildNameArray(const EventBase * pSelectedEvent);
			bool IsInGraph(const char *pType, const char *pValueName) const
			{
				return GetGraphIndex(pType, pValueName) > -1;
			}
			GraphLineBase *CreateGraphLine(const EventBase &rEventSelected, const char *pEventType, const char *pValueName, int iValue) const;
			int GetGraphIndex(const char *pType, const char *pValueName) const;
			void TogglePermanentValue(const EventBase &rEventSelected, const char *pEventType, const char *pValueName, int iEventValue);
			void SetTempValue(const EventBase &rSelectedEvent, const char *pEventTypeName, const char *pValueName, int iEventValue);
			bool ShouldDrawLine(const GraphLineBase *pLine);

			void ClearTempValue()
			{
				delete mp_StoredGraph;
				mp_StoredGraph = 0;
			}
		public:
			GraphFloats()
				:mp_LastSelected(0)
				,mp_CurrentEvent(0)
				,mp_StoredGraph(0)
				,m_iSelectedTag(-1)
				,m_fLineMarker(-1.0f)
				,m_iHovered(-1)
				,m_fX(200.0f)
				,m_fGraphOffset(150)
				,m_fY(400.0f)
				,m_fWidth(1000.0f)
				,m_fHeight(220.0f)
				,m_bShareScale(false)
				,m_eDrawGraph(eDrawMode_Nothing)
			{	
			}

			~GraphFloats()
			{
				Reset();
			}

			//void Enable()
			//{
			//	m_eDrawGraph = eDrawMode_All;
			//}
			bool IsEnabled() const
			{
				return m_eDrawGraph != eDrawMode_Nothing;
			}
			bool HasData() const
			{
				return mp_StoredGraph || m_GraphLines.GetCount();
			}
			void Reset();
			bool DebugDraw(float mouseScreenX, float mouseScreenY, bool bMouseDown, const EventBase *pSelected, const EventBase *pHighlighted, TimeLineZoom &rZoomFactor);
			bool DisplayEventDetails(OnScreenOutput &rTextWindow, const EventBase *pSelected, bool bMouseDown);
		};


		class TotalEventTimeLine
		{
			atArray<u16> m_FrameEventCounts;
			atArray<u16> m_FrameSelectedDataCounts;
			atArray<const Frame*> m_Frames;
			int m_iMaxEventCount;
			bool m_bInited;
			bool m_bForceDrawGraph;
		public:
			TotalEventTimeLine()
				:m_iMaxEventCount(0)
				,m_bInited(false)
			{

			}
			bool IsInited() const { return m_bInited; }
			bool ShouldForceDraw() const { return m_bForceDrawGraph; }
			void SetForceDraw(bool bForce) { m_bForceDrawGraph = bForce; }
			void Reset();
			void SetSelectedDataCounts(const DataBase *pData);
			void Refresh(const Frame * const pFirstFrame);
			const Frame* Display(float xMin, float yMin, float xMax, float yMax, float fMouseX, float fMouseY, Color32 col, TimeLineZoom &rZoom);
		};

		class DebugRecorder : public datBase
		{
		public:
			DebugRecorder();
			virtual ~DebugRecorder();
			virtual void Update(){}
			virtual void PreUpdate(){}
			static DebugRecorder* GetInstance()
			{
				return smp_DebugRecorder;
			}
			static DebugRecorder *GetRecorderForEvent()
			{
				DebugRecorder *pInstance = GetInstance();
				if ((pInstance && pInstance->IsRecording()) || CallstackHelper::sm_bTrapCallstack)
				{
					return pInstance;
				}
				return 0;
			}

			void AddWidgets(bkBank &rBank);
			void DebugDraw(bool bPaused);

			void AddEvent(EventBase &rEvent);
			void AddSharedData(DataBase &rRecordedData, size_t id);
			DataBase *GetSharedData(size_t dataID);
			const DataBase *GetSharedData(size_t dataID) const;
			virtual size_t RegisterCallstack(int iIgnorelevels);
			Callstack* GetCallstack(size_t key);
			void StartRecording();
			bool IsRecording() const
			{
				return m_bRecording;
			}
			void ToggleDisplay()
			{
				m_bDisplayOn = !m_bDisplayOn;
			}
			bool IsDisplayOn() const
			{
				return m_bDisplayOn;
			}
			void ClearRecording();
			void ClearSelection();
			virtual void OnNewDataSelection(){}
			virtual void OnDrawFrame(const Frame *){}
			bool IsItemEnabled(const char *pIdentifier) const;
			void StopRecording();
			void Save(const char *pFileName, bool (*OnlyTheseEvents)(const EventBase *)=0);
			void Load(const char *pFileName);
			void ScrollFramePrev();
			void ScrollFrameNext();
			void SetHovered(const EventBase *pHoveredEvent);
			void SetHovered(const Frame *pSelectedFrame);
			void Select(const Frame *pSelectedFrame, const EventBase *pSelectedEvent=0, bool bAutoSelectNewEvent=false, bool bReAnchorFrameList=false);
			void ScrollEventNext();
			void ScrollEventPrev();
			void ValidateAndBuildEventDisplayList();
			void ToggleSetFilterToJustThis(const char *pFilterName);
			u32 GetRecorderFrameCount() const;
			void SetRecorderFrameCount(u32 iFrame);
			void SetRecorderNetworkTime(unsigned iNetworkTime);
			void RefreshFilteredState(const DataBase* pHovered=0);

			float GetZoomDataZoom() const
			{
				return m_ZoomData.m_fZoom;
			}
			float GetZoomDataOffset() const
			{
				return m_ZoomData.m_fTimeLineOffset;
			}
			void SetZoomDataZoom(float fZoom)
			{
				m_ZoomData.m_fZoom = fZoom;
			}
			void SetZoomDataOffset(float fTimeLineOffset)
			{
				m_ZoomData.m_fTimeLineOffset = fTimeLineOffset;
			}

			static u8 GetGlobalAlphaScale()
			{
				return sm_iGlobalAlphaScale;
			}

			static void SetGlobalAlphaScale(u8 iScale)
			{
				sm_iGlobalAlphaScale = iScale;
			}

			static Color32 ModifyAlpha(Color32 col)
			{
				col.SetAlpha((col.GetAlpha() * sm_iGlobalAlphaScale) >> 8);
				return col;
			}

			const EventBase *GetHoveredEvent() const
			{
				return mp_HoveredEvent;		
			}
			const EventBase *GetWasHoveredEvent() const
			{
				return mp_WasHoveredEvent;
			}
			const EventBase *GetSelectedEvent() const
			{
				return mp_SelectedEvent;
			}
			const Frame *GetSelectedFrame() const
			{
				return mp_SelectedFrame;
			}
			const Frame *GetHoveredFrame() const
			{
				return mp_HoveredFrame;
			}
			const Frame* FindFrame(const EventBase *pFrameForThisEvent) const;
			const Frame *GetFirstFramePtr() const
			{
				return mp_FirstFrame;
			}
			const Frame *GetLastFramePtr() const
			{
				return mp_LastFrame;
			}
			
			int GetFilterIndex() const
			{
				return m_iFilterTypeIndex;
			}

			void SetFilterIndex(int iIndex)
			{
				m_iFilterTypeIndex = iIndex;
			}

			void EnableDataFilterForEvent(const EventBase &rEvent);
			void SetDataFilterOn(bool bOn)
			{
				m_bRestrictToDataFilter = bOn;
				RefreshFilteredState();
			}
			void CallOnAllStoredData(void (*pSomeFunc)(DataBase&));

			void SetDataToHighlight(const DataBase *pData)
			{
				mp_HighlightData = pData;
				m_EventTimeLine.SetSelectedDataCounts(pData);
			}

			const DataBase* GetHighlightedData() const
			{
				return mp_HighlightData;
			}

			struct DebugRecorderData
			{
				atArray<Frame::FrameData> m_Frames;
				atArray<DataBase*> m_RecordedDataArray;
				atArray<size_t> m_RecordedDataIDArray;
				atArray<Callstack*> m_SavedCallstacks;
				int m_SelectedFrameIndex;
				int m_SelectedEventIndex;
				int m_AnchorFrameIndex;
				int m_AnchorEventIndex;
				virtual void DeleteTempIOMemory()
				{
					m_Frames.Reset();
					m_RecordedDataArray.Reset();
					m_RecordedDataIDArray.Reset();	//Only delete the array not the data!
					m_SavedCallstacks.Reset();		//Only delete the array not the data!
					m_SelectedFrameIndex = -1;
					m_SelectedEventIndex = -1;
					m_AnchorFrameIndex = -1;
					m_AnchorEventIndex = -1;
				}
				DebugRecorderData();
				virtual ~DebugRecorderData()
				{
					DeleteTempIOMemory();	//Just in case
				}
				PAR_PARSABLE;
			};
		protected:
			atMap<size_t, Callstack*> m_Callstacks;
			sysCriticalSectionToken m_CallstackCSToken;
			sysCriticalSectionToken m_DataCSToken;
		private:
			struct SubEventType 
			{	const char *mp_OriginalName;
				bool m_bEnabled;
				SubEventType()
					:mp_OriginalName(0)
					,m_bEnabled(false)	{	}
				static int Compare(SubEventType * const * p1, SubEventType * const * p2)
				{	return strcmp((*p1)->mp_OriginalName, (*p2)->mp_OriginalName);	}
			};
			
			virtual DebugRecorder::DebugRecorderData *CreateSaveData() const
			{
				return rage_new DebugRecorder::DebugRecorderData;
			}

			virtual void OnSave(DebugRecorder::DebugRecorderData & /*rData*/)
			{
			}

			virtual void OnLoad(DebugRecorder::DebugRecorderData & /*rData*/)
			{
			}
			Frame *GetOrAllocateCurrentFrame();
			void AddEventOptions(const EventBase &rEvent, const Frame &rFrame, TextOutputVisitor &rText, bool &bMouseDown);
			virtual void OnEventSelection(const EventBase *){}
			bool DrawOthers3d(float fMouseX, float fMouseY);
			bool AddChannelMenu(OnScreenOutput &rOutput, OnScreenOutput &rInfoText, bool &bMouseDown, bool bPaused);
			virtual bool AppOptions(TextOutputVisitor &/*rText*/, TextOutputVisitor &/*rInfoText*/, bool /*bMouseDown*/, bool /*bPaused*/){return false;}
			virtual void OnStartRecording(){}
			virtual void OnClearRecording(){}
			virtual void OnStartDebugDraw(){}
			virtual void OnDebugDraw3d(const EventBase * /*pHovered*/){}
			virtual void OnEndDebugDraw(bool /*bMenuItemSelected*/){}
			virtual bool AddAppFilters(OnScreenOutput &/*rOutput*/, OnScreenOutput &/*rInfoText*/, int &iFilterIndex, bool /*bMouseDown*/) { iFilterIndex = 0; return false; }
			virtual bool LoadFilter(const char * /*pFilerFileName*/){return false;}
			virtual bool SaveFilter(const char * /*pFilerFileName*/){return false;}
			virtual void ClearFilter(){}
			GraphFloats m_Graph;
			TimeLineZoom m_ZoomData;
			TotalEventTimeLine m_EventTimeLine;
			Frame *mp_FirstFrame;
			Frame *mp_LastFrame;
			const Frame *mp_AnchorFrame;
			const Frame *mp_HoveredFrame;
			const EventBase *mp_AnchorEvent;
			const EventBase *mp_HoveredEvent;
			const EventBase *mp_WasHoveredEvent;
			const Frame *mp_SelectedFrame;
			const EventBase *mp_SelectedEvent;
			const DataBase *mp_LastHoveredData;
			u32 m_iNumEventsRecorded;
			u32 m_iFirstFrame;
			u32 m_iLastFrame;
			u32 m_iLastRecordedFrame;
			u32 m_iRecorderFrameMarker;
			u32 m_iNetworkTime;
			s32 m_iAnchorDisplayType;
			s32 m_iAnchorDataFilter;
			u32 m_iCachedEventIndex;
			bool m_bHoveredFromFrameList;
			bool m_bRecording;
			bool m_bDisplayOn;
			bool m_bWasPaused;
			bool m_bRestrictToDataFilter;
			bool m_bDrawFramesList;
			bool m_bDrawEventsList;
			bool m_bDrawElementsList;
			bool m_bFilterNeedsRefresh;
			bool m_bPlaybackOn;

			atMap<size_t, DataBase*> m_RecordedData;
			const DataBase *mp_HighlightData;
			atMap<size_t, SubEventType> m_DisplayedTypeMap;
			atArray<SubEventType *> m_DisplayedNameList;
			DebugRecorder::DebugRecorderData *mp_SaveData;
			static DebugRecorder* smp_DebugRecorder;
			int m_iFilterTypeIndex;

			static u8 sm_iGlobalAlphaScale;

			PAR_PARSABLE;
		};


		void Init();
		void AddWidgets(bkBank &rBank);
		void DebugDraw(bool bPaused);
		void AddSimpleLabel(const char *pListLabel, const char *pTextFmt, ...);
		void AddSimpleSphere(const char *pLabel, Vec3V_In vPos, float fRadius, Color32 color);
		void AddSimpleLine(const char *pLabel, Vec3V_In vPosA, Vec3V_In vPosB, Color32 colorA, Color32 colorB);
		void AddSimpleMatrix(const char *pLabel, Mat34V_In mat, float fScale);
		void DrawAxis(float size, Mat34V_In mat);

		//------------------------------------------------------------------//
		//	  Helper function to make adding events to the system easier	//
		//------------------------------------------------------------------//
		template<typename _TEvent>
		_TEvent* AddEvent()
		{
			if (DR_EVENT_ENABLED(_TEvent))
			{
				DebugRecorder *pRecorder = DebugRecorder::GetInstance();
				if ((pRecorder && pRecorder->IsRecording()) || CallstackHelper::sm_bTrapCallstack)
				{
					CallstackHelper ch;
					if (pRecorder->IsRecording())
					{
						DR_MEMORY_HEAP();
						_TEvent *pEvent = rage_new _TEvent;
						pEvent->Init(ch);
						pRecorder->AddEvent( *pEvent );
						return pEvent;
					}
				}
			}
			return 0;
		}
		//------------------------------------------------------------------//

	}
}

#else //DR_ENABLED

#define DR_ONLY(x)
#define DR_DEV_ONLY(x)
#endif //DR_ENABLED

#endif //PHYSICS_DEBUGPLAYBACK_H
