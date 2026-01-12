// 
// physics/debugevents.cpp
// 
// Copyright (C) 1999-20011 Rockstar Games.  All Rights Reserved. 
// 

#include "debugevents.h"

#if DR_ENABLED

#include "inst.h"
#include "levelnew.h"
#include "simulator.h"
#include "atl/hashstring.h"
#include "bank/bkmgr.h"
#include "system/stack.h"
#include "system/timemgr.h"
#include "system/param.h"
#include "grcore/im.h"
#include "input/keyboard.h"
#include "input/keys.h"
#include "physics/colliderdispatch.h"
#include "physics/debugcontext.h"
#include "grcore/viewport.h"
#include "grcore/viewport_inline.h"

PARAM(DR_SaveInstFile, "Set to bring up a dialog box option to save any time SaveRecordingForInst is called");
namespace rage
{
	namespace debugPlayback
	{
		//------------------------------------------------------------------//
		//								Statics
		//------------------------------------------------------------------//
		float SetMatrixEvent::sm_fAxisScale = 1.0f;
		PhysicsEvent::RecordInstFunc PhysicsEvent::sm_AppRecordInst = 0;

		size_t PhysicsFrame::ms_iCallstackID;

		const int kLineStep = 10;
		static u32 s_iLastFrame = 0;
		static int s_iLine = 0;

		SetTaggedFloatEvent::SetTaggedFloatEvent()
			:m_fValue(0.0f)
		{
		}

		void SetTaggedFloatEvent::LineUpdate()
		{
			if (s_iLastFrame == TIME.GetFrameCount())
			{
				s_iLine++;
			}
			else
			{
				s_iLine = 0;
				s_iLastFrame = TIME.GetFrameCount();
			}
		}

		int SetTaggedFloatEvent::Get3dLineOffset()
		{
			return s_iLine*kLineStep;
		}

		//------------------------------------------------------------------//
		//Our own timer to manage time as TIME.etc doesn't quite seem hooked
		//							up properly in V
		//------------------------------------------------------------------//
		sysTimer s_Timer;

		//------------------------------------------------------------------//
		//					The object we're going to track for now.
		//------------------------------------------------------------------//
		phInstId s_Selected;
		phInstId s_PerhapsSelected;	//Used if the user wasn't clicking on some menu item
		const int kMaxTrackEntities = 32;
		Vec3V svDistToRecordPos(V_ZERO);
		ScalarV svDistToRecordSqr(16.0f*16.0f);
		atFixedArray<phInstId, kMaxTrackEntities> s_Many;
		phInstId s_SavingFor;
		eTrackMode s_TrackMode = eTrackOne;

		eTrackMode GetTrackMode()
		{
			return s_TrackMode;
		}

		void SetTrackMode(eTrackMode mode)
		{
			s_TrackMode = mode;
		}

		void SetTrackDistToRecord(Vec3V_In pos, ScalarV_In range)
		{
			svDistToRecordPos = pos;
			svDistToRecordSqr = range * range;
		}

		void SetCurrentSelectedEntity(const phInst *pInst, bool bForce)
		{
			if (bForce)
			{
				s_Selected.Set(pInst);
				//s_TrackMode = eTrackOne;
				if (DebugRecorder::GetInstance())
				{
					DebugRecorder::GetInstance()->OnNewDataSelection();
				}
			}
			else
			{
				s_PerhapsSelected.Set(pInst);
			}
		}

		void UpdatePerhapsSelected(bool bApply)
		{
			if (bApply)
			{
				if (s_PerhapsSelected.IsValid())
				{
					SetCurrentSelectedEntity(s_PerhapsSelected.GetObject(), true);
					s_PerhapsSelected.Reset();
				}
			}
		}

		void AddSelectedEntity(const phInst &rInst)
		{
			if (s_TrackMode!=eTrackMany)
			{
				s_Many.Reset();
				if ((s_TrackMode == eTrackOne) && s_Selected.IsValid())
				{
					s_Many.Append() = s_Selected;
				}	
				s_TrackMode = eTrackMany;
			}
			//Check if we're already tracking this inst
			const int kCount = s_Many.GetCount();
			for (int i=0 ; i<kCount ; i++)
			{
				//Reuse slots if possible
				if (!s_Many[i].IsValid())
				{
					s_Many[i].Set(&rInst);
					return;
				}
				if (s_Many[i].Matches(rInst)) return; 
			}
			s_Many.Append().Set(&rInst);
		}

		void InstSwappedOut(phInstId rOld, const phInst *pNew)
		{
			//Entity is changing inst, make sure we track the new inst if we were tracking the old
			if (rOld.IsAssigned())
			{
				switch(s_TrackMode)
				{
				case eTrackOne:
					if (s_Selected == rOld)
					{
						s_Selected.Set(pNew);
					}
					break;
				case eTrackMany: {
						const int kCount = s_Many.GetCount();
						for (int i=0 ; i<kCount ; i++)
						{
							//Reuse slots if possible
							if (s_Many[i] == rOld)
							{
								s_Many[i].Set(pNew);
								return;
							}
						}
					}
					break;
				default:;
				}
			}
		}

		void ClearSelectedEntities()
		{
			s_TrackMode = eTrackOne;
			s_Selected.Reset();
			s_Many.Reset();
		}

		phInstId::Param32 GetCurrentSelectedEntity()
		{
			return s_Selected.ToU32();
		}

		int GetNumSelected()
		{
			switch(s_TrackMode)
			{	
				case eTrackOne:
					return s_Selected.IsValid() ? 1 : 0;
				case eTrackMany:
					return s_Many.GetCount();
				default:
					break;
			}
			return 0;
		}
		
		phInstId::Param32 GetSelected(int iIndex)
		{
			switch(s_TrackMode)
			{	
				case eTrackOne:
					if (iIndex == 0)
						return s_Selected.IsValid() ? 1 : 0;
				case eTrackMany:
					if (iIndex < s_Many.GetCount())
						return s_Many[iIndex].ToU32();
				default:
					break;
			}
			return 0;
		}

		bool OnlyEventsForThisInst(const EventBase *pEvent)
		{
			if (s_SavingFor.IsAssigned())
			{
				if (pEvent && pEvent->IsPhysicsEvent())
				{
					const debugPlayback::PhysicsEvent* pPhysicsEvent = static_cast<const debugPlayback::PhysicsEvent*>(pEvent);
					return pPhysicsEvent->IsInstInvolved(s_SavingFor);
				}
				return false;
			}
			return true; 
		}

		void SaveRecordingForInst(const phInst *pInst)
		{
			if (PARAM_DR_SaveInstFile.Get())
			{
				DebugRecorder *pInstance = DebugRecorder::GetInstance();
				if (pInstance)
				{
					char filename[256];
					bool fileSelected = BANKMGR.OpenFile(filename, 256, "*.des", true, "Debug event stream");

					//Make this file our new config file if selected
					if(fileSelected && filename[0])
					{
						s_SavingFor.Set(pInst);
						pInstance->Save(filename, OnlyEventsForThisInst);
						s_SavingFor.Reset();
					}
				}
			}
		}

		void SetTrackAllEntities(bool bTrackAll)
		{
			if (bTrackAll)
			{
				s_TrackMode = eTrackAll;
			}
			else
			{
				s_TrackMode = eTrackOne;
				s_Selected.Reset();
			}
		}

		//------------------------------------------------------------------//
		//Text output
		//------------------------------------------------------------------//
		bool TTYTextOutputVisitor::AddLine(const char *fmt, ...)
		{
			char buffer[512];
			for (int i=0 ; i<m_iIndent ; i++)
			{
				buffer[i] = '\t';
			}
			va_list args;
			va_start(args,fmt);
			vsprintf(buffer+m_iIndent,fmt,args);	//Not limited?
			va_end(args);

			Displayf("%s",buffer);
			return false;
		}

		//------------------------------------------------------------------//
		//							Helper draw functions
		//------------------------------------------------------------------//
		void PhysicsEvent::DrawVelocity(Vec3V_In vel, Vec3V_In pos, ScalarV_In scale, Color32 col, bool bWireframe)
		{
			col = DebugRecorder::ModifyAlpha(col);

			//Draw a sphere animated at the linear velocity
			Vec3V project = vel;
			ScalarV moveSpeed = Mag(vel);
			moveSpeed *= scale;
			float fSpeed = moveSpeed.Getf();
			float offset = s_Timer.GetTime();
			if (fSpeed > 0.0f)
			{
				offset -= floor(offset);
				project *= scale;
			}
			else
			{
				offset = 0.0f;
				project = Vec3V(V_ZERO);
			}

			if (offset < 0.9f)
			{
				Color32 col2(col);
				float kColorScaleForSphere = 0.75f;
				col2.Setf( col.GetGreenf() * kColorScaleForSphere, col.GetRedf() * kColorScaleForSphere, col.GetBluef() * kColorScaleForSphere );
				col2 = DebugRecorder::ModifyAlpha(col2);
				grcColor(col2);
				grcDrawSphere(bWireframe ? 0.1f : 0.025f,pos + project*ScalarV(offset),8,true,!bWireframe);
				grcWorldIdentity();
			}
			grcColor(col);
			grcDrawLine(pos, pos+project, col);
		}

		void PhysicsEvent::DrawRotation(Vec3V_In rot, Vec3V_In pos, ScalarV_In scale, Color32 col)
		{
			//Convert velocity to rotation and angle
			col = DebugRecorder::ModifyAlpha(col);
			grcColor(col);
			ScalarV angle = Mag(rot);
			BoolV hasRot = angle > ScalarV(SMALL_FLOAT);
			if (hasRot.Getb())
			{
				//Get an axis for rotation
				Vec3V unitRotation = Normalize(rot);

				//Remove silly large angles
				ScalarV fOffset = scale * angle * ScalarV(s_Timer.GetTime());
				fOffset -= ScalarV(2.0f * PI) * RoundToNearestIntNegInf(fOffset * ScalarV(1.0f / (2.0f * PI)));

				QuatV qRotation = QuatVFromAxisAngle(unitRotation, fOffset);

				//Build a matrix to rotate a sphere by, the sphere will clearly demonstrate rotation
				Mat34V mat(V_IDENTITY);
				Mat34VFromQuatV(mat, qRotation, pos);

				//Draw axis
				grcDrawLine(pos, pos+unitRotation, col);
				grcDrawSphere(0.2f,mat,8,true,false);
			}
			else
			{
				//Draw something in place
				grcDrawSphere(0.2f,pos,8,false,false);
			}
			grcWorldIdentity();		
		}

		Mat44V s_CameraCull;
		void SetCameraCullingInfo(Mat44V_In cullPlanes)
		{
			s_CameraCull = cullPlanes;
		}

		bool PhysicsEvent::IsSelected(const phInst& rInst)
		{
			switch (s_TrackMode)
			{
			case eNumTrackModes:
				s_TrackMode = eTrackAll;
				//Fallthru
			case eTrackAll:
				return true;
			case eTrackInFrustrum:{
				const phArchetype *pArch = rInst.GetArchetype();
				if (pArch)
				{
					const phBound *pBound = pArch->GetBound();
					if (pBound)
					{
						Vec3V vRange = pBound->GetBoundingBoxSize();
						Vec3V vMin(rInst.GetMatrix().GetCol3());
						Vec3V vMax(vMin);
						vMin = vMin - vRange;
						vMax = vMax + vRange;
						return grcViewport::IsAABBVisibleInline(vMin.GetIntrin128(), vMax.GetIntrin128(), s_CameraCull) != 0;
					}
				}
				break;
			}
			case eTrackOne:
				return s_Selected.Matches(rInst);
			case eTrackMany:{
				const int kCount = s_Many.GetCount();
				for (int i=0 ; i<kCount ; i++)
				{
					if (s_Many[i].Matches(rInst)) return true; 
				}
				break;
			}
			case eTrackInRange:
				return (0 != IsLessThanAll(DistSquared(rInst.GetMatrix().GetCol3(), svDistToRecordPos), svDistToRecordSqr));
			}
			return false;
		}

		//------------------------------------------------------------------//
		//						phInstId - should probably
		//						be a core object to the 
		//							physics system
		//------------------------------------------------------------------//
		phInstId::phInstId()
			:m_iLevelIndex(phInst::INVALID_INDEX)
			,m_iGeneration(0)
		{	}

		phInstId::phInstId(const phInstId& rOther):m_AsU32(rOther.m_AsU32){}
		phInstId::phInstId(const phInstId::Param32 rOther):m_AsU32(rOther){}

		phInstId::phInstId(const phInst &rInst)
		{
			Set(&rInst);
		}

		phInstId::phInstId(const phInst *pInst)
		{
			Set(pInst);
		}

		bool phInstId::IsAssigned() const
		{
			return m_iLevelIndex != phInst::INVALID_INDEX;
		}

		phInst* phInstId::GetObject() const
		{
			if (m_iLevelIndex != phInst::INVALID_INDEX)
			{
				if (PHLEVEL->LegitLevelIndex(m_iLevelIndex))
				{
					if (PHLEVEL->IsLevelIndexGenerationIDCurrent(m_iLevelIndex, m_iGeneration))
					{
						return PHLEVEL->GetInstance(m_iLevelIndex);
					}
				}
			}
			return 0;
		}

		bool phInstId::IsValid() const
		{
			if (m_iLevelIndex != phInst::INVALID_INDEX)
			{
				if (PHLEVEL->IsLevelIndexGenerationIDCurrent(m_iLevelIndex, m_iGeneration))
				{
					return true;
				}
			}
			return false;
		}

		bool phInstId::Matches(const phInst &rOther) const
		{
			if (rOther.IsInLevel() && (rOther.GetLevelIndex() == m_iLevelIndex))
			{
				//TMS: Could skip this if performance is an issue
				return PHLEVEL->GetGenerationID(m_iLevelIndex) == m_iGeneration;
			}
			return false;
		}

		void phInstId::Reset()
		{
			m_iLevelIndex = phInst::INVALID_INDEX;
		}

		void phInstId::Set(const phInst *pInst)
		{
			if (pInst)
			{
				m_iLevelIndex = pInst->GetLevelIndex();
				m_iGeneration = m_iLevelIndex == phInst::INVALID_INDEX ? 0 : PHLEVEL->GetGenerationID(m_iLevelIndex);
			}
			else
			{
				Reset();
			}
		}

		//------------------------------------------------------------------//
		//			Simple event to mark physics frame start / ends
		//------------------------------------------------------------------//
		u32 PhysicsFrame::sm_PhysicsFrame;
		bool PhysicsFrame::DrawListText(TextOutputVisitor &rText) const
		{
			return rText.AddLine("PhysicsFrame(%d)", m_Frame);
		}

		PhysicsFrame::PhysicsFrame()
			:m_Frame(0)
		{	}

		PhysicsFrame::PhysicsFrame(const CallstackHelper rCallstack)
			:EventBase(rCallstack)
			,m_Frame(sm_PhysicsFrame)
		{	}

		
		//------------------------------------------------------------------//
		//Store an event when a new object becomes involved in the stream
		//------------------------------------------------------------------//
		NewObjectData::NetObjectDescriptionCreator NewObjectData::smp_CreateNewObjectDescription = NULL;
		const char *NewObjectData::GetStoredDescription(phInstId::Param32 rInst, DebugRecorder &rRecorder, char *pBuffer, int iBufferSize)
		{
			DataBase* pDataBase = rRecorder.GetSharedData(rInst);
			if (pDataBase && pDataBase->IsNewObjectData())
			{
				NewObjectData *pEvent = static_cast<NewObjectData*>(pDataBase);
				return pEvent->GetObjectDescription(pBuffer, iBufferSize);
			}
			return "!!Invalid!!";
		}

		const char *NewObjectData::GetObjectDescription(char *pBuffer, int iBuffersize) const
		{
			//Base level implementation, can be overridden
			if (m_rInst.IsValid())
			{
				formatf(pBuffer, iBuffersize, "0x%p [%s]", m_rInst.GetObject(), m_rInst->GetArchetype() ? m_rInst->GetArchetype()->GetFilename() : "NULL_ARCH");
			}
			else
			{
				formatf(pBuffer, iBuffersize, "<<null>> [li:%d]", m_rInst.GetLevelIndex());
			}
			return pBuffer;
		}

		void NewObjectData::DebugText(TextOutputVisitor &rText) const
		{
			rText.AddLine("%s", m_Description.c_str());
			rText.PushIndent();
			rText.AddLine("Col0: %f, %f, %f", m_Matrix.GetCol0().GetXf(), m_Matrix.GetCol0().GetYf(), m_Matrix.GetCol0().GetZf());
			rText.AddLine("Col1: %f, %f, %f", m_Matrix.GetCol1().GetXf(), m_Matrix.GetCol1().GetYf(), m_Matrix.GetCol1().GetZf());
			rText.AddLine("Col2: %f, %f, %f", m_Matrix.GetCol2().GetXf(), m_Matrix.GetCol2().GetYf(), m_Matrix.GetCol2().GetZf());
			rText.AddLine("Col3: %f, %f, %f", m_Matrix.GetCol3().GetXf(), m_Matrix.GetCol3().GetYf(), m_Matrix.GetCol3().GetZf());
			rText.PopIndent();
		}

		NewObjectData *NewObjectData::CreateNewObjectData(phInst *pInst)
		{
			DR_MEMORY_HEAP();

			if (smp_CreateNewObjectDescription)
			{
				NewObjectData *pData = smp_CreateNewObjectDescription(pInst);
				if (pData)
				{
					pData->Init();
					return pData;
				}
				//Else let fill in with the default
			}
			NewObjectData *pNewData = rage_new NewObjectData(pInst);
			pNewData->Init();
			return pNewData;
		}

		void NewObjectData::Init()
		{
			char buf[256];
			GetObjectDescription(buf, sizeof(buf));
			m_Description = buf;
		}

		NewObjectData::NewObjectData() : m_Matrix(V_IDENTITY), m_bIsPlaybackEnabled(false) {}

		NewObjectData::NewObjectData(phInstId rInst)
			:m_rInst(rInst)
			,m_bIsPlaybackEnabled(false)
		{
			phInst *pInst = m_rInst.GetObject();
			if (pInst)
			{
				m_Matrix = pInst->GetMatrix();
			}
		}

		//------------------------------------------------------------------//
		//Base physics object
		//------------------------------------------------------------------//
		phInst* PhysicsEvent::GetSelectedInst()
		{
			return s_Selected.GetObject();
		}

		phCollider *PhysicsEvent::GetColliderForReplay(phInst *pInst) const
		{
			phInst *pInstToReplay = pInst ? pInst : m_rInst.GetObject();
			if (pInstToReplay)
			{
				int iLevelIndex = pInstToReplay->GetLevelIndex();
				if (PHLEVEL->IsActive( iLevelIndex ))
				{
					phCollider *pCollider = PHSIM->GetActiveCollider(iLevelIndex);
					if (pCollider)
					{
						return pCollider;
					}
				}

				return PHSIM->ActivateObject(iLevelIndex, pInstToReplay);
			}
			return 0;
		}
		
		void PhysicsEvent::RecordInst(phInstId rInst)
		{
			if (sm_AppRecordInst)
			{
				sm_AppRecordInst(rInst);
			}
			else
			{
				//TMS:	There are a set of stacks doing this, breaking apart, exploding things
				//		Setting positions and velocities on objects prior to adding to the level
				//		We might want to use a phInst pointer to track some data and possible link it
				//		to the level id when that is known.
				//DebugRecorder &rRecorder = *DebugRecorder::GetInstance();
				//if (!rRecorder.GetSharedData(rInst.ToU32()))
				//{
				//	DR_MEMORY_HEAP();
				//	NewObjectData *pOb = NewObjectData::CreateNewObjectData(rInst);
				//	rRecorder.AddSharedData(*pOb, rInst.ToU32());
				//}
			}
		}

		void PhysicsEvent::OnAddToList(DebugRecorder &rRecorder)
		{
			if (m_rInst.GetLevelIndex() != phInst::INVALID_INDEX)
			{
				RecordInst(m_rInst);
			}
			else
			{
				//static int s_Crapola;
				//static atMap<u16, bool> s_HAveMap;
				//u16 uStackId = sysStack::RegisterBacktrace(4);
				//if (!s_HAveMap.Access(uStackId))
				//{
				//	Displayf("New stack [#%d] for invalid inst", s_Crapola);
				//	sysStack::PrintStackTrace();
				//	s_Crapola++;
				//}
			}

			EventBase::OnAddToList(rRecorder);
		}

		bool PhysicsEvent::IsEventDataRelated(const EventBase& rOtherEvent) const
		{
			if(rOtherEvent.IsPhysicsEvent())
			{
				const PhysicsEvent* pOtherPhysEvent = static_cast<const PhysicsEvent*>(&rOtherEvent);
				if (pOtherPhysEvent)
				{
					//TMS:	Technically should be more complex than this
					//		but for the graphing purpose right now this is fast and works
					return (pOtherPhysEvent->m_rInst == m_rInst);
				}
			}
			return false;
		}

		bool PhysicsEvent::IsDataInvolved(const DataBase& rData) const
		{
			if (rData.IsNewObjectData())
			{
				const NewObjectData& rNOD = static_cast<const NewObjectData&>(rData);
				return IsInstInvolved(rNOD.m_rInst);
			}
			return false;
		}

		void PhysicsEvent::PrintInstName(const char *pPrefix, TextOutputVisitor &rText) const
		{
			DataBase* pDataBase = DebugRecorder::GetInstance()->GetSharedData(m_rInst);
			if (pDataBase && pDataBase->IsNewObjectData())
			{
				NewObjectData *pEvent = static_cast<NewObjectData*>(pDataBase);
				//rText.AddLine("%sphInst: %d (%s) : Playback Guid %d", pPrefix, m_rInst.GetLevelIndex(), pEvent->m_Description.c_str(), m_PlaybackGuid);
				rText.AddLine("%sphInst: %d (%s)", pPrefix, m_rInst.GetLevelIndex(), pEvent->m_Description.c_str());
			}
		}

		bool PhysicsEvent::DebugEventText(TextOutputVisitor &rText) const
		{
			PrintInstName("", rText);
			EventBase::DebugEventText(rText);	//Will write out the rest of the values
			return true;
		}

		void PhysicsEvent::Init(const CallstackHelper rCallstack, const phInst *pInst)
		{
			EventBase::Init(rCallstack);

			m_rInst.Set(pInst);

			if (pInst)
			{
				m_Pos = pInst->GetMatrix().GetCol3();
			}
			else
			{
				m_Pos.ZeroComponents();
			}
		}

		void PhysicsEvent::AddEventOptions(const Frame &frame, TextOutputVisitor &text, bool &bMouseDown) const
		{
			EventBase::AddEventOptions(frame, text, bMouseDown);
		}

		PhysicsEvent::PhysicsEvent()
			:m_Pos(V_ZERO)
		{	}

		PhysicsEvent::PhysicsEvent(const CallstackHelper rCallstack, const phInst *pInst)
			:EventBase(rCallstack)
		{
			//This will push the marker into the event stream if needed
			PhysicsFrame::OnNewPhysicsEvent();

			m_rInst.Set(pInst);

			if (pInst)
			{
				m_Pos = pInst->GetMatrix().GetCol3();
			}
			else
			{
				m_Pos.ZeroComponents();
			}
		}

		//------------------------------------------------------------------//
		//						CACHE INSTS IN THE CONTEXT
		//------------------------------------------------------------------//
		struct PhysicsDebugStack : public PhysicsEvent
		{
			void Init(CallstackHelper &rCallstack, u16 iLevelIndex, u16 iGeneration)
			{
				EventBase::Init(rCallstack);
				m_rInst.Set(iLevelIndex, iGeneration);				
			}

			DR_EVENT_DECL(PhysicsDebugStack)
			PAR_PARSABLE;
		};
		//------------------------------------------------------------------//

		//------------------------------------------------------------------//
		//						INST BASED SIMPLE DRAW FUNCTIONS
		//------------------------------------------------------------------//
		struct InstLineEvent : public PhysicsEvent
		{
			InstLineEvent()
				:m_Start(V_ZERO)
				,m_End(V_ZERO)
			{}

			void SetData(const char *pLabel, Vec3V_In vStart, Vec3V_In vEnd, u32 colorStart, u32 colorEnd)
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

			virtual void DebugDraw3d(eDrawFlags /*drawFlags*/) const
			{
				grcDrawLine(RCC_VECTOR3(m_Start), RCC_VECTOR3(m_End), DebugRecorder::ModifyAlpha(Color32(m_ColorStart)), DebugRecorder::ModifyAlpha(Color32(m_ColorEnd)));
			}

			virtual bool GetEventPos(Vec3V &rOut) const 
			{	
				rOut = (m_Start + m_End) * ScalarV(V_HALF);
				return true;
			}

			atHashString m_Label;
			Vec3V m_Start;
			Vec3V m_End;
			u32 m_ColorStart;
			u32 m_ColorEnd;

			DR_EVENT_DECL(InstLineEvent)
			PAR_PARSABLE;
		};

		struct InstSphereEvent : public PhysicsEvent
		{
			InstSphereEvent()
				:m_Radius(0.0f)
			{}

			void SetData(const char *pLabel, Vec3V_In vPos, float fRadius, u32 color)
			{
				m_Label = pLabel;
				m_Pos = vPos;		//Part of PhysicsEvent
				m_Radius = fRadius;
				m_Color = color;
			}

			virtual const char* GetEventSubType() const
			{
				return m_Label.GetCStr();
			}

			virtual void DebugDraw3d(eDrawFlags drawFlags) const
			{
				grcColor(DebugRecorder::ModifyAlpha(Color32(m_Color)));
				grcDrawSphere(m_Radius, m_Pos, drawFlags & eDrawHovered ? 8 : 4 , true, drawFlags & eDrawSelected ? true : false);
				grcWorldIdentity();
			}

			atHashString m_Label;
			float m_Radius;
			u32 m_Color;

			DR_EVENT_DECL(InstSphereEvent)
			PAR_PARSABLE;
		};
		//------------------------------------------------------------------//


		//------------------------------------------------------------------//
		//							EVENT TYPES
		//------------------------------------------------------------------//
		const float kfForceDisplayScale = 0.01f;
		struct ApplyForceEvent : public PhysicsEvent
		{
			virtual void DebugDraw3d(eDrawFlags drawFlags) const
			{
				Color32 col(255,0,0,255);
				col = DebugRecorder::ModifyAlpha(col);
				if (drawFlags & eDrawSelected)
					col.SetGreen(255);
				if (drawFlags & eDrawHovered)
					col.SetBlue(255);
				grcColor(col);

				DrawVelocity(m_Force, m_ForcePos, ScalarV(kfForceDisplayScale / m_fMass), col, true);
			}

			virtual bool GetEventPos(Vec3V &pos) const 
			{
				pos = m_ForcePos;
				return true;
			}

			void SetData(Vec3V_In force, Vec3V_In forcepos, int component)
			{
				Assign(m_iComponent, component);
				m_ForcePos = forcepos;
				m_Force = force;
				if (m_rInst.IsValid() && m_rInst->GetArchetype())
				{
					m_fMass = m_rInst->GetArchetype()->GetMass();
				}
				else
				{
					m_fMass = 1.f;
				}
			}
			
			ApplyForceEvent();

			s16 m_iComponent;
			Vec3V m_ForcePos;
			Vec3V m_Force;
			float m_fMass;

			PAR_PARSABLE;
			DR_EVENT_DECL(ApplyForceEvent)
		};

		ApplyForceEvent::ApplyForceEvent()
			:m_iComponent(0)
			,m_ForcePos(V_ZERO)
			,m_Force(V_ZERO)
			,m_fMass(0.0f)
		{	}

		struct ApplyForceCGEvent : public ApplyForceEvent
		{
			void SetData(Vec3V_In force)
			{
				ApplyForceEvent::SetData(force, Vec3V(V_ZERO), 0); 

				if (m_rInst)
				{
					if (PHLEVEL->IsActive( m_rInst->GetLevelIndex() ))
					{
						phCollider *pCollider = PHSIM->GetActiveCollider(m_rInst->GetLevelIndex());
						if (pCollider)
						{
							m_ForcePos = pCollider->GetMatrix().GetCol3();
						}
					}
				}
			}

			ApplyForceCGEvent();

			PAR_PARSABLE;
			DR_EVENT_DECL(ApplyForceCGEvent)
		};

		ApplyForceCGEvent::ApplyForceCGEvent()
		{	}

		struct ApplyTorqueEvent : public PhysicsEvent
		{
			virtual void DebugDraw3d(eDrawFlags drawFlags) const
			{
				Color32 col(100,50,250,255);
				col = DebugRecorder::ModifyAlpha(col);
				if (drawFlags & eDrawSelected)
				{
					col.SetRed(255);
				}
				if (drawFlags & eDrawHovered)
				{
					col.SetGreen(255);
				}
				DrawRotation(m_Torque, m_Pos, ScalarV(1.0f / m_fMass), col);
			}

			void SetData(Vec3V_In torque)
			{
				m_Torque = torque;
				if (m_rInst.IsValid() && m_rInst->GetArchetype())
				{
					m_fMass = m_rInst->GetArchetype()->GetMass();
				}
			}

			ApplyTorqueEvent();

			float m_fMass;
			Vec3V m_Torque;

			PAR_PARSABLE;
			DR_EVENT_DECL(ApplyTorqueEvent)
		};

		ApplyTorqueEvent::ApplyTorqueEvent()
			:m_fMass(0.0f)
			,m_Torque(V_ZERO)
		{	}

		struct ApplyImpulseEvent : public PhysicsEvent
		{
			virtual void DebugDraw3d(eDrawFlags drawFlags) const
			{
				Color32 col(255,128,128,255);
				col = DebugRecorder::ModifyAlpha(col);
				if (drawFlags & eDrawSelected)
				{
					col.SetGreen(255);
				}
				if (drawFlags & eDrawHovered)
				{
					col.SetBlue(255);
				}
				DrawVelocity(m_Impulse, m_ImpulsePos, ScalarV(0.1f), col);
			}

			void SetData(Vec3V_In impulse, Vec3V_In pos)
			{
				m_Impulse =- impulse;
				m_ImpulsePos = pos;

				if (m_rInst.IsValid())
				{
					//TMS: Is this transform really correct? I was told it was but seems doubtful and was incorrect for ApplyForce
					if (PHLEVEL->IsActive( m_rInst->GetLevelIndex() ))
					{
						phCollider *pCollider = PHSIM->GetActiveCollider(m_rInst->GetLevelIndex());
						if (pCollider)
						{
							m_ImpulsePos = Transform(pCollider->GetMatrix(), m_ImpulsePos);
							m_Impulse = Transform(pCollider->GetMatrix(), m_Impulse);
						}
					}
					else
					{
						m_ImpulsePos = Transform(m_rInst->GetMatrix(), m_ImpulsePos);
						m_Impulse = Transform(m_rInst->GetMatrix(), m_Impulse);
					}
				}			
			}

			ApplyImpulseEvent();

			Vec3V m_Impulse;
			Vec3V m_ImpulsePos;

			PAR_PARSABLE;
			DR_EVENT_DECL(ApplyImpulseEvent)
		};

		ApplyImpulseEvent::ApplyImpulseEvent()
			:m_Impulse(V_ZERO)
			,m_ImpulsePos(V_ZERO)
		{	}

		struct ApplyImpulseCGEvent : public ApplyImpulseEvent
		{
			void SetData(Vec3V_In impulse)
			{
				ApplyImpulseEvent::SetData(impulse, Vec3V(V_ZERO));

				if (m_rInst.IsValid())
				{
					if (PHLEVEL->IsActive( m_rInst->GetLevelIndex() ))
					{
						phCollider *pCollider = PHSIM->GetActiveCollider(m_rInst->GetLevelIndex());
						if (pCollider)
						{
							m_ImpulsePos = pCollider->GetMatrix().GetCol3();
						}
					}
				}
			}

			ApplyImpulseCGEvent();

			PAR_PARSABLE;
			DR_EVENT_DECL(ApplyImpulseCGEvent)
		};

		ApplyImpulseCGEvent::ApplyImpulseCGEvent()
		{	}

		struct SetVelocityEvent : public PhysicsEvent
		{
			virtual void DebugDraw3d(eDrawFlags drawFlags) const
			{
				Color32 col(255,255,255,255);
				col = DebugRecorder::ModifyAlpha(col);
				if (drawFlags & eDrawSelected)
				{
					col.Set(255,0,0,255);
				}
				else if (drawFlags & eDrawHovered)
				{
					col.Set(0,0,255,255);
				}
				DrawVelocity(m_Velocity, m_Pos, ScalarV(V_ONE), col);
			}

			virtual bool Replay(phInst *pInst) const 
			{
				if (m_rInst.IsValid() || pInst)
				{
					phCollider *pCollider = GetColliderForReplay(pInst);
					if (pCollider)
					{
						pCollider->SetVelocity(m_Velocity.GetIntrin128());
					}
					else
					{
						Errorf("Failed to %s due to lack of collider", GetEventType());
					}
				}
				return true;
			}

			void SetData(Vec3V_In vel)
			{
				m_Velocity = vel;
			}

			SetVelocityEvent();

			Vec3V m_Velocity;

			PAR_PARSABLE;
			DR_EVENT_DECL(SetVelocityEvent)
		};

		SetVelocityEvent::SetVelocityEvent()
			:m_Velocity(V_ZERO)
		{	}

		struct SetAngularVelocityEvent : public PhysicsEvent
		{
			virtual void DebugDraw3d(eDrawFlags drawFlags) const
			{
				Color32 col(0,0,255,255);
				col = DebugRecorder::ModifyAlpha(col);
				if (drawFlags & eDrawSelected)
				{
					col.SetRed(255);
				}
				if (drawFlags & eDrawHovered)
				{
					col.SetGreen(255);
				}
				DrawRotation(m_AngVelocity, m_Pos, ScalarV(V_ONE), col);
			}

			virtual bool Replay(phInst *pInst) const 
			{
				if (m_rInst.IsValid() || pInst)
				{
					phCollider *pCollider = GetColliderForReplay(pInst);
					if (pCollider)
					{
						pCollider->SetAngVelocity(m_AngVelocity.GetIntrin128());
					}
					else
					{
						Errorf("Failed to %s due to lack of collider", GetEventType());
					}				
				}
				return true;
			}

			void SetData(Vec3V_In vel)
			{
				m_AngVelocity =- vel;
			}

			SetAngularVelocityEvent();

			Vec3V m_AngVelocity;

			PAR_PARSABLE;
			DR_EVENT_DECL(SetAngularVelocityEvent)
		};

		SetAngularVelocityEvent::SetAngularVelocityEvent()
			:m_AngVelocity(V_ZERO)
		{	}

		void SetMatrixEvent::DebugDraw3d(eDrawFlags drawFlags) const
		{
			float fAxis = 1.0f;
			bool bDrawSphere = false;
			Color32 col(0,0,0,255);
			col = DebugRecorder::ModifyAlpha(col);
			if (drawFlags & eDrawSelected)
			{
				if ((TIME.GetFrameCount() & 0x7) > 3)
				{
					fAxis = 1.5f;
				}
				bDrawSphere = true;
				col.SetRed(255);
			}
			else if (drawFlags & eDrawHovered)
			{
				if ((TIME.GetFrameCount() & 0x7) > 3)
				{
					fAxis = 1.25f;
				}
				bDrawSphere = true;
				col.SetBlue(255);
			}

			fAxis *= sm_fAxisScale;
			DrawAxis(fAxis, m_Matrix);
			if(bDrawSphere)
			{
				DebugRecorder::ModifyAlpha(col);
				grcColor(col);
				grcDrawSphere(fAxis*0.1f, m_Matrix, 4, true, false);
				grcWorldIdentity();
			}
		}

		bool SetMatrixEvent::Replay(phInst *pInst) const 
		{
			if (m_rInst.IsValid() || pInst)
			{
				PHSIM->TeleportObject(pInst ? *pInst : *m_rInst.GetObject(), RCC_MATRIX34(m_Matrix), &RCC_MATRIX34(m_Matrix));
			}
			return true;
		}

		void SetMatrixEvent::AddEventOptions(const Frame &frame, TextOutputVisitor &rText, bool &bMouseDown) const
		{
			PhysicsEvent::AddEventOptions(frame, rText, bMouseDown);
			if (rText.AddLine("Axis scale [%f]", sm_fAxisScale) && bMouseDown)
			{
				sm_fAxisScale *= .5f;
				if(sm_fAxisScale<0.1f)
				{
					sm_fAxisScale = 1.0f;
				}
				bMouseDown = false;
			}
		}

		SetMatrixEvent::SetMatrixEvent()
			:m_Matrix(V_IDENTITY)
		{	}
		

		struct UpdateLocation : public PhysicsEvent
		{
			virtual void AddEventOptions(const Frame &rFrame, TextOutputVisitor &rText, bool &bMouseDown) const
			{
				PhysicsEvent::AddEventOptions(rFrame, rText, bMouseDown);
				if (rText.AddLine("Axis scale [%f]", sm_fAxisScale) && bMouseDown)
				{
					sm_fAxisScale *= .5f;
					if(sm_fAxisScale<0.1f)
					{
						sm_fAxisScale = 1.0f;
					}
					bMouseDown = false;
				}
			}

			virtual void DebugDraw3d(eDrawFlags drawFlags) const
			{
				float fAxis = 1.0f;
				bool bDrawSphere = false;
				Color32 col(0,0,0,255);
				col = DebugRecorder::ModifyAlpha(col);
				if (drawFlags & eDrawSelected)
				{
					if ((TIME.GetFrameCount() & 0x7) > 3)
					{
						fAxis = 1.5f;
					}
					bDrawSphere = true;
					col.SetRed(255);
				}
				else if (drawFlags & eDrawHovered)
				{
					if ((TIME.GetFrameCount() & 0x7) > 3)
					{
						fAxis = 1.25f;
					}
					bDrawSphere = true;
					col.SetBlue(255);
				}
				DebugRecorder::ModifyAlpha(col);

				fAxis *= sm_fAxisScale;
				DrawAxis(fAxis, m_NewMatrix);
				if(bDrawSphere)
				{
					grcColor(col);
					grcDrawSphere(fAxis*0.1f, m_NewMatrix, 4, true, false);
					//Return our matrix to something reasonable
					grcWorldIdentity();
				}

				
				if (m_bHasLastMatrix)
				{
					//Smaller secondary axis for matrix update
					fAxis *= 0.5f;
					DrawAxis(fAxis, m_LastMatrix);

					//And draw line connecting the two
					grcDrawLine(m_LastMatrix.GetCol3(), m_NewMatrix.GetCol3(), col);
				}
			}

			virtual bool Replay(phInst *pInst) const 
			{
				if (m_rInst.IsValid() || pInst)
				{
					PHSIM->TeleportObject(pInst ? *pInst : *m_rInst.GetObject(), RCC_MATRIX34(m_NewMatrix), m_bHasLastMatrix ? &RCC_MATRIX34(m_LastMatrix) : 0);
				}
				return true;
			}

			void SetData(const Mat34V *pLastMatrix)
			{
				m_bHasLastMatrix = pLastMatrix != 0;
				if (m_rInst.IsValid())
				{
					m_NewMatrix = m_rInst->GetMatrix();
				}
				if (m_bHasLastMatrix)
				{
					m_LastMatrix = *pLastMatrix;
				}
				else
				{
					m_LastMatrix = m_NewMatrix;
				}
			}

			UpdateLocation();

			bool m_bHasLastMatrix;
			Mat34V m_NewMatrix;
			Mat34V m_LastMatrix;
			static float sm_fAxisScale;

			PAR_PARSABLE;
			DR_EVENT_DECL(UpdateLocation)
		};

		UpdateLocation::UpdateLocation() : m_NewMatrix(V_IDENTITY), m_LastMatrix(V_IDENTITY), m_bHasLastMatrix(false) {}

		struct InstLabelEvent : public PhysicsEvent
		{
			InstLabelEvent();
			InstLabelEvent(const CallstackHelper rCallstack, const phInst *pInst);

			virtual ~InstLabelEvent(){}

			void Set(const char *pLabel, const char *pText)
			{
				m_Label = pLabel;
				m_Description = pText;
			}

			void DebugText(TextOutputVisitor &rText) const
			{
				EventBase::DebugText(rText);
				rText.PushIndent();
				PrintInstName(rText);
				rText.AddLine(m_Description.c_str());
				rText.PopIndent();
				rText.AddLine("---------------------");
			}

			virtual const char* GetEventType() const
			{
				//TMS: Share this? atStringHash?
				return m_Label.GetCStr();
			}

			atHashString m_Label;
			ConstString m_Description;

			PAR_PARSABLE;
			DR_EVENT_CUSTOM_TYPE_DECL(UpdateLocation)
		};
		
		InstLabelEvent::InstLabelEvent(){}
		InstLabelEvent::InstLabelEvent(const CallstackHelper rCallstack, const phInst *pInst)
			:PhysicsEvent(rCallstack, pInst)
		{
		}


		float UpdateLocation::sm_fAxisScale = 1.0f;

		void SetTaggedFloatEvent::DebugDraw3d(eDrawFlags drawFlags) const
		{
			if (drawFlags & eDrawFrameHovered)	//Looks messy, just say no
			{
				return;
			}

			Color32 col(128,128,128,255);
			col = DebugRecorder::ModifyAlpha(col);
			if (drawFlags & eDrawSelected)
			{
				col.SetRed(255);
			}
			else if (drawFlags & eDrawHovered)
			{
				col.SetBlue(255);
			}
			grcColor(col);
			grcDrawSphere(0.1f, m_Pos, 4, true, false);
			grcWorldIdentity();

			if (drawFlags & (eDrawSelected))
			{
				SetTaggedFloatEvent::LineUpdate();
				grcColor(DebugRecorder::ModifyAlpha(Color32(0,0,0,255)));
				grcDrawLabelf(RCC_VECTOR3(m_Pos), 1, 1 + SetTaggedFloatEvent::Get3dLineOffset()+1, "%s - %f", m_FloatName.GetCStr(), m_fValue);
				grcColor(col);
				grcDrawLabelf(RCC_VECTOR3(m_Pos), 0, 0 + SetTaggedFloatEvent::Get3dLineOffset(), "%s - %f", m_FloatName.GetCStr(), m_fValue);
			}
		}

		SetTaggedVectorEvent::SetTaggedVectorEvent()
			:m_Value(V_ZERO)
			,m_eVectorType(eVectorTypePosition)
		{
		}

		void SetTaggedVectorEvent::DebugDraw3d(eDrawFlags drawFlags) const
		{
			if (drawFlags & eDrawFrameHovered)	//Looks messy, just say no
			{
				return;
			}

			Color32 col(0,0,0,255);
			col = DebugRecorder::ModifyAlpha(col);
			if (drawFlags & eDrawSelected)
			{
				col.SetRed(255);
			}
			else if (drawFlags & eDrawHovered)
			{
				col.SetBlue(255);
			}

			switch(m_eVectorType)
			{
			case eVectorTypePosition:
				grcColor(col);
				grcDrawSphere(0.1f, m_Value, 4, true, false);
				grcWorldIdentity();
				break;
			case eVectorTypeVelocity:
				DrawVelocity(m_Value, m_Pos, ScalarV(V_ONE), col, true);
				break;
			}

			if (drawFlags & (eDrawSelected))
			{
				SetTaggedFloatEvent::LineUpdate();
				grcColor(DebugRecorder::ModifyAlpha(Color32(0,0,0,255)));
				grcDrawLabelf(RCC_VECTOR3(m_Pos), 11, 11 + SetTaggedFloatEvent::Get3dLineOffset()+1, "%s - <%f, %f, %f>", m_ValueName.GetCStr(), m_Value.GetXf(), m_Value.GetYf(), m_Value.GetZf());
				grcColor(col);
				grcDrawLabelf(RCC_VECTOR3(m_Pos), 10, 10 + SetTaggedFloatEvent::Get3dLineOffset(), "%s - <%f, %f, %f>", m_ValueName.GetCStr(), m_Value.GetXf(), m_Value.GetYf(), m_Value.GetZf());
			}
		}

		SetTaggedMatrixEvent::SetTaggedMatrixEvent()
			:m_Value(V_ZERO)
		{
		}

		void SetTaggedMatrixEvent::DebugDraw3d(eDrawFlags drawFlags) const
		{
			if (drawFlags & eDrawFrameHovered)	//Looks messy, just say no
			{
				return;
			}

			Color32 col(0,0,0,255);
			col = DebugRecorder::ModifyAlpha(col);
			float fSize = 1.0f;
			if (drawFlags & eDrawSelected)
			{
				col.SetRed(255);
				if ((TIME.GetFrameCount() & 0x7) > 3)
				{
					fSize = 1.25f;
				}
			}
			else if (drawFlags & eDrawHovered)
			{
				col.SetBlue(255);
				if ((TIME.GetFrameCount() & 0x7) > 3)
				{
					fSize = 1.5f;
				}
			}
			DrawAxis(fSize, m_Value);

			if (drawFlags & (eDrawSelected))
			{
				SetTaggedFloatEvent::LineUpdate();
				grcColor(DebugRecorder::ModifyAlpha(Color32(0,0,0,255)));
				grcDrawLabelf(RCC_VECTOR3(m_Pos), 10, 10 + SetTaggedFloatEvent::Get3dLineOffset(), "%s", m_ValueName.GetCStr());
			}
		}

		//------------------------------------------------------------------//
		//SetTaggedDataCollectionEvent
		//------------------------------------------------------------------//
		DataCollectionEvent::ChildData::ChildData() {}
		DataCollectionEvent::ChildData::ChildData(const char *pName)
			:m_Identifier(pName)
		{	}

		DataCollectionEvent::ChildData_String::ChildData_String() {}
		DataCollectionEvent::ChildData_String::ChildData_String(const char *pName, const char *pString)
			:ChildData(pName)
			,m_String(pString)
		{	}

		DataCollectionEvent::ChildData_Float::ChildData_Float():m_fValue(0.0f){}
		DataCollectionEvent::ChildData_Float::ChildData_Float(const char *pName, float value)
			:ChildData(pName)
			,m_fValue(value)
		{	}

		DataCollectionEvent::ChildData_Int::ChildData_Int():m_iValue(0){}
		DataCollectionEvent::ChildData_Int::ChildData_Int(const char *pName, int value)
			:ChildData(pName)
			,m_iValue(value)
		{	}

		DataCollectionEvent::ChildData_Bool::ChildData_Bool():m_bValue(0){}
		DataCollectionEvent::ChildData_Bool::ChildData_Bool(const char *pName, bool value)
			:ChildData(pName)
			,m_bValue(value)
		{	}

		DataCollectionEvent::ChildData_Vec3::ChildData_Vec3():m_fX(0.0f),m_fY(0.0f),m_fZ(0.0f),m_bIs3dLocation(false){}
		DataCollectionEvent::ChildData_Vec3::ChildData_Vec3(const char *pName, Vec3V_In value, bool bIs3dLocation)
			:ChildData(pName)
			,m_bIs3dLocation(bIs3dLocation)
		{
			m_fX = value.GetXf();
			m_fY = value.GetYf();
			m_fZ = value.GetZf();
		}

		const char * DataCollectionEvent::ChildData_Float::GetValue(char *pBufferMayNotBeUsed, int iBufferLen) const
		{
			formatf(pBufferMayNotBeUsed, iBufferLen, "%f", m_fValue);
			return pBufferMayNotBeUsed;
		}
		const char * DataCollectionEvent::ChildData_Int::GetValue(char *pBufferMayNotBeUsed, int iBufferLen) const
		{
			formatf(pBufferMayNotBeUsed, iBufferLen, "%d", m_iValue);
			return pBufferMayNotBeUsed;
		}
		const char * DataCollectionEvent::ChildData_Vec3::GetValue(char *pBufferMayNotBeUsed, int iBufferLen) const
		{
			formatf(pBufferMayNotBeUsed, iBufferLen, "<%f,%f,%f>", m_fX, m_fY, m_fZ);
			return pBufferMayNotBeUsed;
		}

		void DataCollectionEvent::AddChildData(ChildData* pChildData)
		{
			int iResize=2;
			if (m_Data.GetCount()>=4)
			{
				iResize = 4;
			}
		
			m_Data.Grow(iResize) = pChildData;
		}

		int DataCollectionEvent::GetNumChildDataItems(const char *pIdentifier)
		{
			int iReturn=0;
			if (pIdentifier)
			{
				for (int i=0; i<m_Data.GetCount() ; i++)
				{
					if (!strcmp(m_Data[i]->GetIdentifier(), pIdentifier))
					{
						iReturn++;
					}
				}
			}
			return iReturn;
		}

		void DataCollectionEvent::AddString(const char *pIdentifier, const char *pString)
		{
			DR_MEMORY_HEAP();
			AddChildData(rage_new ChildData_String(pIdentifier, pString));
		}

		void DataCollectionEvent::AddInt(const char *pIdentifier, int value)
		{
			DR_MEMORY_HEAP();
			AddChildData(rage_new ChildData_Int(pIdentifier, value));
		}

		void DataCollectionEvent::AddBool(const char *pIdentifier, bool value)
		{
			DR_MEMORY_HEAP();
			AddChildData(rage_new ChildData_Bool(pIdentifier, value));
		}

		void DataCollectionEvent::AddFloat(const char *pIdentifier, float value)
		{
			DR_MEMORY_HEAP();
			AddChildData(rage_new ChildData_Float(pIdentifier, value));
		}

		void DataCollectionEvent::AddVec3(const char *pIdentifier, Vec3V_In value, bool bIs3dLocation)
		{
			DR_MEMORY_HEAP();
			AddChildData(rage_new ChildData_Vec3(pIdentifier, value, bIs3dLocation));
		}

		void DataCollectionEvent::OutputData(TextOutputVisitor &rOutput) const
		{
			for (int i=0; i<m_Data.GetCount() ; i++)
			{
				char buffer[256];
				rOutput.AddLine("%s: %s", m_Data[i]->GetIdentifier(), m_Data[i]->GetValue(buffer, sizeof(buffer)));
			}
		}

		bool DataCollectionEvent::DebugEventText(TextOutputVisitor &rOutput) const
		{
			PrintInstName("", rOutput);
			OutputData(rOutput);
			return true;	
		}

		void DataCollectionEvent::ChildData_Vec3::DebugDraw3d(eDrawFlags drawFlags) const
		{
			if (m_bIs3dLocation)
			{
				Color32 col(128,128,128,255);
				col = DebugRecorder::ModifyAlpha(col);
				if (drawFlags & eDrawHovered)
				{
					col.SetRed(255);
				}
				else if (drawFlags & eDrawSelected)
				{
					col.SetBlue(255);
				}
				Vector3 vMin(m_fX-0.05f, m_fY-0.05f, m_fZ-0.05f);
				Vector3 vMax(m_fX+0.05f, m_fY+0.05f, m_fZ+0.05f);
				grcDrawBox(vMin, vMax, col);
			}
		}

		void DataCollectionEvent::DebugDraw3d(eDrawFlags drawFlags) const
		{
			for (int i=0 ; i<m_Data.GetCount() ; i++)
			{
				m_Data[i]->DebugDraw3d(drawFlags);
			}
		}

		bool SetTaggedDataCollectionEvent::DebugEventText(TextOutputVisitor &rOutput) const
		{
			PrintInstName("", rOutput);
			rOutput.AddLine("Name: %s", m_CollectionName.GetCStr());
			rOutput.PushIndent();
			DataCollectionEvent::DebugEventText(rOutput);
			rOutput.PopIndent();
			return true;	
		}

		void SetTaggedDataCollectionEvent::DebugDraw3d(eDrawFlags drawFlags) const
		{
			DataCollectionEvent::DebugDraw3d(drawFlags);

			if (drawFlags & eDrawFrameHovered)	//Looks messy, just say no
			{
				return;
			}

			Color32 col(200,0,200,255);
			col = DebugRecorder::ModifyAlpha(col);
			float fRadius = 0.2f;
			if (drawFlags & eDrawSelected)
			{
				fRadius = 0.21f;
				col.SetRed(0);
			}
			if (drawFlags & eDrawHovered)
			{
				fRadius = 0.21f;
				col.SetGreen(255);
			}

			grcColor(col);
			grcDrawSphere(fRadius,m_Pos,8,false,false);
			grcWorldIdentity();

			//Draw text for the tasks
			if (drawFlags & (eDrawSelected))
			{

				SetTaggedFloatEvent::LineUpdate();
				grcColor(DebugRecorder::ModifyAlpha(Color32(0,0,0,255)));
				grcDrawLabelf(RCC_VECTOR3(m_Pos), 1, 1+ SetTaggedFloatEvent::Get3dLineOffset()+1, "%s", GetEventSubType());
				grcColor(col);
				grcDrawLabelf(RCC_VECTOR3(m_Pos), 0, SetTaggedFloatEvent::Get3dLineOffset(), "%s", GetEventSubType());
				
				//for (int i=0 ; i<m_Data.GetCount() ; i++)
				//{
				//	char buffer[256];
				//	SetTaggedFloatEvent::LineUpdate();
				//	grcColor(Color32(0,0,0,255));
				//	grcDrawLabelf(RCC_VECTOR3(m_Pos), 1, SetTaggedFloatEvent::Get3dLineOffset()+1, "     %s : %s", m_Data[i]->GetIdentifier(), m_Data[i]->GetValue(buffer, sizeof(buffer)));
				//	grcColor(col);
				//	grcDrawLabelf(RCC_VECTOR3(m_Pos), 0, SetTaggedFloatEvent::Get3dLineOffset(), "     %s : %s", m_Data[i]->GetIdentifier(), m_Data[i]->GetValue(buffer, sizeof(buffer)));
				//}
			}
		}

		//------------------------------------------------------------------//
		//								STATICS
		//------------------------------------------------------------------//
		DR_EVENT_IMP(PhysicsFrame);
		DR_EVENT_IMP(PhysicsDebugStack);
		DR_EVENT_IMP(InstSphereEvent);
		DR_EVENT_IMP(InstLineEvent);
		DR_EVENT_IMP(ApplyForceEvent);
		DR_EVENT_IMP(ApplyImpulseEvent);
		DR_EVENT_IMP(ApplyImpulseCGEvent);
		DR_EVENT_IMP(ApplyForceCGEvent);
		DR_EVENT_IMP(ApplyTorqueEvent);
		DR_EVENT_IMP(SetVelocityEvent);
		DR_EVENT_IMP(SetAngularVelocityEvent);
		DR_EVENT_IMP(SetMatrixEvent);
		DR_EVENT_IMP(SetTaggedFloatEvent);
		DR_EVENT_IMP(SetTaggedVectorEvent);
		DR_EVENT_IMP(SetTaggedMatrixEvent);
		DR_EVENT_IMP(SetTaggedDataCollectionEvent);
		DR_EVENT_IMP(DataCollectionEvent);
		DR_EVENT_IMP(UpdateLocation);
		DR_EVENT_IMP(InstLabelEvent);

		//------------------------------------------------------------------//
		//						PUBLIC RECORDING INTERFACE
		//------------------------------------------------------------------//

		void RecordInstLabel(const phInst& rInst, const char *pListLabel, const char *pTextFmt, ...)
		{
			InstLabelEvent *pEvent = AddPhysicsEvent<InstLabelEvent>(&rInst);
			if (pEvent)
			{
				DR_MEMORY_HEAP();
				char buffer[512];
				va_list args;
				va_start(args,pTextFmt);
				vsprintf(buffer,pTextFmt,args);
				va_end(args);

				pEvent->Set(pListLabel, buffer);
			}
		}

		void RecordInstSphere(const phInst& rInst, const char *pLabel, Vec3V_In pos, float fRadius, Color32 color)
		{
			InstSphereEvent *pEvent = AddPhysicsEvent<InstSphereEvent>(&rInst);
			if (pEvent)
			{
				DR_MEMORY_HEAP();
				pEvent->SetData(pLabel, pos, fRadius, color.GetColor());
			}
		}

		void RecordInstLine(const phInst& rInst, const char *pLabel, Vec3V_In posA, Vec3V_In posB, Color32 colorA, Color32 colorB)
		{
			InstLineEvent *pEvent = AddPhysicsEvent<InstLineEvent>(&rInst);
			if (pEvent)
			{
				DR_MEMORY_HEAP();
				pEvent->SetData(pLabel, posA, posB, colorA.GetColor(), colorB.GetColor());
			}
		}

		void RecordApplyForce(const phInst& rInst, Vec::V3Param128 force, Vec::V3Param128 pos, int iComponent)
		{
			ApplyForceEvent *pEvent = AddPhysicsEvent<ApplyForceEvent>(&rInst);
			if (pEvent)
			{
				pEvent->SetData(Vec3V(force), Vec3V(pos), iComponent);
			}
		}

		void RecordApplyForceCG(const phInst& rInst, Vec::V3Param128 force)
		{
			ApplyForceCGEvent *pEvent = AddPhysicsEvent<ApplyForceCGEvent>(&rInst);
			if (pEvent)
			{
				pEvent->SetData(Vec3V(force));
			}
		}


		void RecordApplyImpulse(const phInst& rInst, Vec::V3Param128 impulse, Vec::V3Param128 pos)
		{
			ApplyImpulseEvent *pEvent = AddPhysicsEvent<ApplyImpulseEvent>(&rInst);
			if (pEvent)
			{
				pEvent->SetData(Vec3V(impulse), Vec3V(pos));
			}
		}

		void RecordApplyImpulseCG(const phInst& rInst, Vec::V3Param128 impulse)
		{
			ApplyImpulseCGEvent *pEvent = AddPhysicsEvent<ApplyImpulseCGEvent>(&rInst);
			if (pEvent)
			{
				pEvent->SetData(Vec3V(impulse));
			}
		}



		void RecordApplyTorque(const phInst& rInst, Vec::V3Param128 torque)
		{
			ApplyTorqueEvent *pEvent = AddPhysicsEvent<ApplyTorqueEvent>(&rInst);
			if (pEvent)
			{
				pEvent->SetData(Vec3V(torque));
			}
		}

		void RecordSetVelocity(const phInst& rInst, Vec::V3Param128 vel)
		{
			SetVelocityEvent *pEvent = AddPhysicsEvent<SetVelocityEvent>(&rInst);
			if (pEvent)
			{
				pEvent->SetData(Vec3V(vel));
			}
		}

		void RecordSetAngularVelocity(const phInst& rInst, Vec::V3Param128 angvel)
		{
			SetAngularVelocityEvent *pEvent = AddPhysicsEvent<SetAngularVelocityEvent>(&rInst);
			if (pEvent)
			{
				pEvent->SetData(Vec3V(angvel));
			}
		}

		void RecordSetMatrix(const phInst& rInst, Mat34V_In mat)
		{
			SetMatrixEvent *pEvent = AddPhysicsEvent<SetMatrixEvent>(&rInst);
			if (pEvent)
			{
				pEvent->SetData(mat);
			}
		}

		void RecordTaggedFloatValue(const phInst& rInst, float fValue, const char *pIdentifier)
		{
			SetTaggedFloatEvent *pEvent = AddPhysicsEvent<SetTaggedFloatEvent>(&rInst);
			if (pEvent)
			{
				DR_MEMORY_HEAP();
				pEvent->SetData(fValue, pIdentifier);
			}
		}

		SetTaggedDataCollectionEvent* RecordDataCollection(const phInst *pInst, const char *pNameFmt, ...)
		{
			SetTaggedDataCollectionEvent *pEvent = AddPhysicsEvent<SetTaggedDataCollectionEvent>(pInst);
			if (pEvent)
			{
				DR_MEMORY_HEAP();
				char buffer[256];
				va_list argptr;
				va_start(argptr, pNameFmt);
				vsprintf(buffer, pNameFmt, argptr);
				va_end(argptr);
				pEvent->SetName(buffer);
			}
			return pEvent;
		}

		void RecordTaggedVectorValue(const phInst& rInst, Vec3V_In value, const char *pIdentifier, eVectorType eType)
		{
			SetTaggedVectorEvent *pEvent = AddPhysicsEvent<SetTaggedVectorEvent>(&rInst);
			if (pEvent)
			{
				DR_MEMORY_HEAP();
				pEvent->SetData(value, pIdentifier, eType);
			}
		}

		void RecordTaggedMatrixValue(const phInst& rInst, Mat34V_In value, const char *pIdentifier)
		{
			SetTaggedMatrixEvent *pEvent = AddPhysicsEvent<SetTaggedMatrixEvent>(&rInst);
			if (pEvent)
			{
				DR_MEMORY_HEAP();
				pEvent->SetData(value, pIdentifier);
			}
		}

		void RecordUpdateLocation(const phInst& rInst, const Mat34V *pLastMatrix)
		{
			UpdateLocation *pEvent = AddPhysicsEvent<UpdateLocation>(&rInst);
			if (pEvent)
			{
				pEvent->SetData(pLastMatrix);
			}
		}

		void RecordUpdateLocation(int kLevelIndex, const Mat34V *pLastMatrix)
		{
			phInst *pInst = PHLEVEL->GetInstance(kLevelIndex);
			if(pInst)
			{
				RecordUpdateLocation(*pInst, pLastMatrix);
			}
		}


		void RecordNewPhysicsFrame()
		{
			//Using this to ID contacts - the rest of the event is disabled
			PhysicsFrame::sm_PhysicsFrame++;

			if (DR_EVENT_ENABLED(PhysicsFrame))
			{
				CallstackHelper cs;

				//We used to allocate the event right here but this led to empty frames
				//with just the frame marker being stored. Now instead we tell the system that
				//it happened but only add the event when another PhysicsEvent fires off. There is
				//the possibility that a non physics event will make it into the stream out of order
				//with the physics frame event.
				PhysicsFrame::StorePotentialPhysicsFrame(cs);
			}
		}

#if ENABLE_PH_DEBUG_CONTEXT
		void RecursRecordDebugContext_Internal(const phDebugContext &rContext, CallstackHelper &ch)
		{
			if (rContext.GetParentContext())
			{
				RecursRecordDebugContext_Internal(*rContext.GetParentContext(), ch);
			}

			PhysicsDebugStack *pEvent = rage_new PhysicsDebugStack;
			pEvent->Init(ch, rContext.GetLevelIndex(), rContext.GetGeneration());
			DebugRecorder::GetInstance()->AddEvent( *pEvent );
		}
#endif // ENABLE_PH_DEBUG_CONTEXT

		void RecordCurrentDebugContext()
		{
#if ENABLE_PH_DEBUG_CONTEXT
			if (DR_EVENT_ENABLED(PhysicsDebugStack))
			{
				DebugRecorder *pRecorder = DebugRecorder::GetInstance();
				if (pRecorder && pRecorder->IsRecording())
				{
					if (phDebugContext::GetCurrentContext())
					{
						DR_MEMORY_HEAP();
						CallstackHelper ch;
						RecursRecordDebugContext_Internal(*phDebugContext::GetCurrentContext(), ch);
					}
				}
			}
#endif // ENABLE_PH_DEBUG_CONTEXT
		}
	}
}

//Included last to get all the declarations within the CPP file
using namespace rage;
using namespace debugPlayback;
#include "debugevents_parser.h"

#endif //PDR_ENABLED
