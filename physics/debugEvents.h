// 
// physics/debugevents.h 
// 
// Copyright (C) 1999-20011 Rockstar Games.  All Rights Reserved. 
// 
#ifndef __PHYSICS_DEBUG_EVENTS_H__
#define __PHYSICS_DEBUG_EVENTS_H__
#include "debugPlayback.h"

#define PDR_ENABLED (DR_ENABLED && (!__SPU) && 0)
#if PDR_ENABLED
	#define PDR_ONLY(x) x
#else
	#define PDR_ONLY(x)
#endif

#if DR_ENABLED	//NOT PDR_ENABLED so other systems can log PDR_ type events.

#include "parser/macros.h"

#if __WIN32
#pragma warning(push)
#pragma warning(disable:4201)	// nonstandard extension used : nameless struct/union
#endif

namespace rage {
	class phInst;
	class bkBank;
	class phCollider;
	namespace debugPlayback	{
		struct phInstId
		{
			typedef u32 Param32;
			union {
				struct {
					u16 m_iLevelIndex;
					u16 m_iGeneration;
				};
				Param32 m_AsU32;
			};
			phInstId();
			phInstId(const phInstId& rOther);
			phInstId(const phInstId::Param32 rOther);
			phInstId(const phInst *pInst);
			phInstId(const phInst &rInst);
			void operator=(Param32 param){ m_AsU32 = param; }
			void operator=(const phInstId &rOther){ m_AsU32 = rOther.m_AsU32; }
			bool operator==(const phInstId &rOther) const { return m_AsU32 == rOther.m_AsU32; }
			bool operator!=(const phInstId &rOther) const { return m_AsU32 != rOther.m_AsU32; }
			operator phInst* () const { return GetObject(); }
			phInst *operator -> () const { return GetObject(); }
			operator Param32() const { return m_AsU32; }
			bool Matches(const phInst &rOther) const;
			bool Matches(Param32 param) const { return param == m_AsU32;}
			u32 ToU32() const {return m_AsU32;}
			bool IsValid() const;
			phInst* GetObject() const;
			void Set(const phInst *pInst);
			void Set(u16 iLevelIndex, u16 iGeneration)
			{
				m_iLevelIndex = iLevelIndex;
				m_iGeneration = iGeneration;
			}
			void Reset();
			bool IsAssigned() const;
			u16 GetLevelIndex() const {return m_iLevelIndex;}

			PAR_SIMPLE_PARSABLE;
		};

		struct NewObjectData : public DataBase
		{
			//Interface to allow applications to overload the descriptions
			typedef NewObjectData* (*NetObjectDescriptionCreator)(phInst *pInst);			
			static NetObjectDescriptionCreator smp_CreateNewObjectDescription;

			phInstId m_rInst;
			Mat34V m_Matrix;
			ConstString m_Description;
			bool m_bIsPlaybackEnabled;
			virtual void DebugText(TextOutputVisitor &) const ;
			virtual const char *GetObjectDescription(char *pBuffer, int iBuffersize) const;
			virtual bool HasFilterOption() const {return true;}
			virtual const char* GetFilterText() const {return m_Description.c_str();}
			virtual void OnFilterToggle(){m_bIsPlaybackEnabled = !m_bIsPlaybackEnabled;}
			virtual bool IsFilterActive(){return m_bIsPlaybackEnabled;}
			virtual bool IsNewObjectData() const {return true;}
			void Init();	//Called right after the object is created, calls GetObjectDescription and stores in m_Description

			static NewObjectData *CreateNewObjectData(phInst *pInst);
			static const char *GetStoredDescription(phInstId::Param32 rInst, DebugRecorder &rRecorder, char *pBuffer, int iBufferSize);


			NewObjectData(phInstId rInst);
			NewObjectData();

			PAR_PARSABLE;
		};

		struct PhysicsFrame : public EventBase
		{
			virtual bool DrawListText(TextOutputVisitor &rText) const;
			PhysicsFrame(const CallstackHelper rCallstack);
			PhysicsFrame();

			static size_t ms_iCallstackID;
			
			static void StorePotentialPhysicsFrame(CallstackHelper &cs)
			{
				ms_iCallstackID = cs.m_iCallstackID;
			}
			
			static void OnNewPhysicsEvent()
			{
				if (ms_iCallstackID)
				{
					CallstackHelper cs(ms_iCallstackID);

					//Create a physics frame event with our fake callstack
					DebugRecorder *pRecorder = DebugRecorder::GetRecorderForEvent();
					if (pRecorder)
					{
						pRecorder->AddEvent(*rage_new PhysicsFrame(cs));
					}

					//Clear it
					ms_iCallstackID = 0;
				}
			}

			u32 m_Frame;
			static u32 sm_PhysicsFrame;

			PAR_PARSABLE;
			DR_EVENT_DECL(PhysicsFrame)
		};

		struct PhysicsEvent : public EventBase
		{
			static phInst* GetSelectedInst();
			typedef void (*RecordInstFunc)(phInstId);
			
			virtual bool IsPhysicsEvent() const {return true;}
			static RecordInstFunc sm_AppRecordInst;
			virtual bool DebugEventText(TextOutputVisitor &) const ;
			void PrintInstName(TextOutputVisitor &rText) const { PrintInstName("", rText); }
			void PrintInstName(const char *pPrefix, TextOutputVisitor &rText) const;
			virtual bool IsDataInvolved(const DataBase& rData) const;
			virtual bool IsEventDataRelated(const EventBase& ) const;
			virtual void OnAddToList(DebugRecorder &rRecorder);
			virtual bool GetEventPos(Vec3V &pos) const {pos = m_Pos; return true;}
			phCollider *GetColliderForReplay(phInst *pInst) const;
			void Init(const CallstackHelper rCallstack, const phInst *pInst=0);
			static void DrawVelocity(Vec3V_In vel, Vec3V_In pos, ScalarV_In scale, Color32 col, bool bWireframe=false);
			static void DrawRotation(Vec3V_In rot, Vec3V_In pos, ScalarV_In scale, Color32 col);
			static bool IsSelected(const phInst& rInst);
			void RecordInst(phInstId rInst);
			virtual void AddEventOptions(const Frame &, TextOutputVisitor &, bool &bMouseDown) const;
			virtual bool IsInstInvolved(phInstId rInst) const
			{
				return m_rInst == rInst;
			}
			PhysicsEvent(const CallstackHelper rCallstack, const phInst *pInst);
			PhysicsEvent();

			Vec3V m_Pos;		//Could be worked out, currently storing for ease of development
			phInstId m_rInst;

			PAR_PARSABLE;
		};

		struct SetMatrixEvent : public PhysicsEvent
		{
			virtual void DebugDraw3d(eDrawFlags drawFlags) const;
			virtual bool Replay(phInst *pInst) const;
			virtual void AddEventOptions(const Frame &, TextOutputVisitor &, bool &bMouseDown) const;
			void SetData(Mat34V_In mat)
			{
				m_Matrix = mat;
			}
			SetMatrixEvent();
			static float sm_fAxisScale;
			Mat34V m_Matrix;
			PAR_PARSABLE;
			DR_EVENT_DECL(SetMatrixEvent)
		};

		//------------------------------------------------------------------//

		//------------------------------------------------------------------//
		//	Some general purpose classes for storing float, vector and matrix
		//				data associated with an isntance.
		//------------------------------------------------------------------//
		struct DataCollectionEvent : public PhysicsEvent
		{
			struct ChildData
			{
				atHashString m_Identifier;
				ChildData();
				ChildData(const char *pName);
				const char *GetIdentifier() const { return m_Identifier.GetCStr(); }
				virtual const char *GetValue(char *pBufferMayNotBeUsed, int iBufferLen) const = 0;
				virtual void DebugDraw3d(eDrawFlags /*drawFlags*/) const {};
				virtual ~ChildData(){};
				PAR_PARSABLE;
			};
			struct ChildData_String : public ChildData
			{
				atHashString m_String;
				ChildData_String();
				ChildData_String(const char *pName, const char *pString);
				virtual const char *GetValue(char *, int ) const { return m_String.GetCStr(); }
				PAR_PARSABLE;
			};
			struct ChildData_Float : public ChildData
			{
				float m_fValue;
				ChildData_Float();
				ChildData_Float(const char *pName, float value);				
				virtual const char * GetValue(char *pBufferMayNotBeUsed, int iBufferLen) const;
				PAR_PARSABLE;
			};
			struct ChildData_Int : public ChildData
			{
				int m_iValue;
				ChildData_Int();
				ChildData_Int(const char *pName, int value);				
				virtual const char * GetValue(char *pBufferMayNotBeUsed, int iBufferLen) const;
				PAR_PARSABLE;
			};
			struct ChildData_Bool : public ChildData
			{
				bool m_bValue;
				ChildData_Bool();
				ChildData_Bool(const char *pName, bool value);
				virtual const char * GetValue(char *, int ) const { return m_bValue ? "true" : "false"; }
				PAR_PARSABLE;
			};
			struct ChildData_Vec3 : public ChildData
			{
				float m_fX, m_fY, m_fZ;	//Better packing
				bool m_bIs3dLocation;
				ChildData_Vec3();
				ChildData_Vec3(const char *pName, Vec3V_In value, bool bIs3dLocation);
				virtual const char * GetValue(char *pBufferMayNotBeUsed, int iBufferLen) const;
				virtual void DebugDraw3d(eDrawFlags /*drawFlags*/) const;
				PAR_PARSABLE;
			};

			atArray<ChildData*> m_Data;

			void AddString(const char *pIdentifier, const char *pString);
			void AddInt(const char *pIdentifier, int value);
			void AddBool(const char *pIdentifier, bool value);
			void AddFloat(const char *pIdentifier, float value);
			void AddVec3(const char *pIdentifier, Vec3V_In value, bool bIs3dLocation=false);
			void AddChildData(ChildData* pChildData);

			void OutputData(TextOutputVisitor &rOutput) const;

			virtual bool DebugEventText(TextOutputVisitor &rOutput) const;
			virtual void DebugDraw3d(eDrawFlags drawFlags) const;
			int GetNumChildDataItems() const { return m_Data.GetCount(); }
			int GetNumChildDataItems(const char *pIdentifier);
			DataCollectionEvent(){}
			DataCollectionEvent(const CallstackHelper rCallstack, const phInst *pInst)
				:PhysicsEvent(rCallstack, pInst)
			{	}

			~DataCollectionEvent()
			{
				DR_MEMORY_HEAP();
				for (int i=0 ; i<m_Data.GetCount() ; i++)
				{
					delete m_Data[i];
				}
			}

			PAR_PARSABLE;
			DR_EVENT_DECL(DataCollectionEvent)
		};

		struct SetTaggedDataCollectionEvent : public DataCollectionEvent
		{
			atHashString m_CollectionName;
			virtual const char* GetEventSubType() const { return m_CollectionName.GetCStr(); }
			virtual bool DebugEventText(TextOutputVisitor &rOutput) const;
			virtual void DebugDraw3d(eDrawFlags drawFlags) const;
			void SetName(const char *pName)
			{
				m_CollectionName.SetFromString(pName);
			}
			SetTaggedDataCollectionEvent() {}
			~SetTaggedDataCollectionEvent(){}
			PAR_PARSABLE;
			DR_EVENT_DECL(SetTaggedDataCollectionEvent)
		};

		struct SetTaggedFloatEvent : public PhysicsEvent
		{
			atHashString m_FloatName;
			float m_fValue;

			static void LineUpdate();
			static int Get3dLineOffset();

			virtual const char* GetValueName(const char *pValueId) const { return stricmp(pValueId, "fValue") ? pValueId : GetEventSubType(); }
			virtual void DebugDraw3d(eDrawFlags drawFlags) const;
			virtual const char* GetEventSubType() const { return m_FloatName.GetCStr(); }

			void SetData(float value, const char *pValueName)
			{
				m_fValue = value;
				m_FloatName.SetFromString(pValueName);
			}
			
			SetTaggedFloatEvent();
			PAR_PARSABLE;
			DR_EVENT_DECL(SetTaggedFloatEvent)
		};

		enum eVectorType {eVectorTypeVelocity, eVectorTypePosition};

		struct SetTaggedVectorEvent : public PhysicsEvent
		{
			atHashString m_ValueName;
			eVectorType m_eVectorType;
			Vec3V m_Value;

			virtual const char* GetValueName(const char *pValueId) const { return stricmp(pValueId, "Value") ? pValueId : GetEventSubType(); }
			virtual const char* GetEventSubType() const { return m_ValueName.GetCStr(); }
			virtual void DebugDraw3d(eDrawFlags drawFlags) const;
			void SetData(Vec3V_In value, const char *pValueName, eVectorType eType)
			{
				m_Value = value;
				m_ValueName.SetFromString(pValueName);
				m_eVectorType = eType;
			}

			SetTaggedVectorEvent();
			PAR_PARSABLE;
			DR_EVENT_DECL(SetTaggedVectorEvent)
		};

		struct SetTaggedMatrixEvent : public PhysicsEvent
		{
			atHashString m_ValueName;
			Mat34V m_Value;

			virtual const char* GetValueName(const char *pValueId) const { return stricmp(pValueId, "Value") ? pValueId : GetEventSubType(); }
			virtual const char* GetEventSubType() const { return m_ValueName.GetCStr(); }
			virtual void DebugDraw3d(eDrawFlags drawFlags) const;

			void SetData(Mat34V_In value, const char *pValueName)
			{
				m_Value = value;
				m_ValueName.SetFromString(pValueName);
			}

			SetTaggedMatrixEvent();
			PAR_PARSABLE;
			DR_EVENT_DECL(SetTaggedMatrixEvent)
		};

		void SetCurrentSelectedEntity(const phInst *pInst, bool bForce);
		void UpdatePerhapsSelected(bool bApply);
		void AddSelectedEntity(const phInst &rInst);
		void InstSwappedOut(phInstId rOld, const phInst *pNew);
		void ClearSelectedEntities();
		phInstId::Param32 GetCurrentSelectedEntity();
		int GetNumSelected();
		phInstId::Param32 GetSelected(int iIndex=0);
		void SetTrackAllEntities(bool bTrackAll);
		void SetTrackEntities(bool bTrackAll);
		enum eTrackMode
		{
			eTrackAll,
			eTrackInFrustrum,
			eTrackMany,
			eTrackOne,
			eTrackInRange,
			eNumTrackModes
		};
		eTrackMode GetTrackMode();
		void SetTrackMode(eTrackMode mode);
		void SetTrackDistToRecord(Vec3V_In pos, ScalarV_In range);
		void SaveRecordingForInst(const phInst *pInst);
		void RecordInstLine(const phInst& rInst, const char *pLabel, Vec3V_In posA, Vec3V_In posB, Color32 colorA, Color32 colorB);
		void RecordInstSphere(const phInst& rInst, const char *pLabel, Vec3V_In pos, float fRadius, Color32 color);
		void RecordInstLabel(const phInst& rInst, const char *pListLabel, const char *pFmt, ...);
		void RecordApplyForce(const phInst& rInst, Vec::V3Param128 force, Vec::V3Param128 pos, int iComponent);
		void RecordApplyForceCG(const phInst& rInst, Vec::V3Param128 force);
		void RecordApplyImpulse(const phInst& rInst, Vec::V3Param128 impulse, Vec::V3Param128 pos);
		void RecordApplyImpulseCG(const phInst& rInst, Vec::V3Param128 impulse);
		void RecordApplyTorque(const phInst& rInst, Vec::V3Param128 torque);
		void RecordSetMomentum(const phInst& rInst, Vec::V3Param128 momentum);
		void RecordSetVelocity(const phInst& rInst, Vec::V3Param128 vel);
		void RecordSetAngularVelocity(const phInst& rInst, Vec::V3Param128 angvel);
		void RecordSetAngularMomentum(const phInst& rInst, Vec::V3Param128 angvel);
		void RecordSetMatrix(const phInst& rInst, Mat34V_In mat);
		void RecordTaggedFloatValue(const phInst& rInst, float fValue, const char *pIdentifier);
		void RecordTaggedVectorValue(const phInst& rInst, Vec3V_In value, const char *pIdentifier, eVectorType eType);
		void RecordTaggedMatrixValue(const phInst& rInst, Mat34V_In value, const char *pIdentifier);
		SetTaggedDataCollectionEvent* RecordDataCollection(const phInst *pInst, const char *pNameFmt, ...);
		void RecordUpdateLocation(int kLevelIndex, const Mat34V *pLastMatrix);
		void RecordNewPhysicsFrame();
		void SetCameraCullingInfo(Mat44V_In cullPlanes);
		//PURPOSE:
		//	Record instances that are currently on the debug context stack
		//	Likely called during a crash or assertion just prior to saving the log
		void RecordCurrentDebugContext();
	}
}

#if __WIN32
#pragma warning(pop)  	// nonstandard extension used : nameless struct/union
#endif

#endif
#endif
