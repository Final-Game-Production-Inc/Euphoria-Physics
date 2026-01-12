
// 
// physics/debugconetacts.cpp
// 
// Copyright (C) 1999-20011 Rockstar Games.  All Rights Reserved. 
// 

#include "debugcontacts.h"

#if PDR_ENABLED

#include "inst.h"
#include "levelnew.h"
#include "simulator.h"
#include "debugEvents.h"
#include "system/timemgr.h"
#include "grcore/im.h"
#include "input/keyboard.h"
#include "phbound/boundgeom.h"
#include "physics/manifold.h"

namespace rage
{
	namespace debugPlayback
	{

		//------------------------------------------------------------------//
		//					Polygon stored with each contact
		//------------------------------------------------------------------//
		struct PolyData : public DataBase
		{
			atArray<Vec3V> m_VertArray;
			void Init(const phPolygon &rPoly, const phBoundPolyhedron &rBoundGeom)
			{
				m_VertArray.Resize( POLY_MAX_VERTICES );
				for (int i=0 ; i<m_VertArray.GetCount() ; i++)
				{
					const phPolygon::Index vertIndex = rPoly.GetVertexIndex(i);
					m_VertArray[i] = rBoundGeom.GetVertex(vertIndex);
				}
			}

			void DebugDraw3d(eDrawFlags drawFlags) const 
			{
				Color32 col(255,200,64,128);
				if (drawFlags & eDrawSelected)
				{
					col.Set(255,64,64,128);
				}
				else if (drawFlags & eDrawHovered)
				{
					col.Set(64,255,255);
				}
				col = DebugRecorder::ModifyAlpha(col);
				for (int i=0 ; i<m_VertArray.GetCount()-1 ; i++)
				{
					grcDrawLine(m_VertArray[i], m_VertArray[i+1], col);
				}
				grcDrawLine(m_VertArray[0], m_VertArray[m_VertArray.GetCount()-1], col);
			}

			virtual ~PolyData()
			{
			}

			PAR_PARSABLE;
		};
		//------------------------------------------------------------------//

		//------------------------------------------------------------------//
		//								Event class
		//------------------------------------------------------------------//
		class RecordedContact : public DataCollectionEvent
		{
		public:
			void OnAddToList(DebugRecorder &rRecorder)
			{
				if (m_rOther.GetLevelIndex() != phInst::INVALID_INDEX)
				{
					RecordInst(m_rOther);

					if (!rRecorder.GetSharedData(m_rOther.ToU32()))
					{
						DR_MEMORY_HEAP();
						NewObjectData *pOb = rage_new NewObjectData(m_rOther);
						rRecorder.AddSharedData(*pOb, m_rOther.ToU32());
					}
				}

				PhysicsEvent::OnAddToList(rRecorder);
			}


			void AddEventOptions(const Frame &rFrame, TextOutputVisitor &rText, bool &bMouseDown) const
			{
				PhysicsEvent::AddEventOptions(rFrame, rText, bMouseDown);

				rText.AddLine("--CONTACTS--");
				rText.PushIndent();
				if(rText.AddLine("Polys[%s]", sm_bDrawContactPoly ? "X" : " ") && bMouseDown)
				{
					sm_bDrawContactPoly = !sm_bDrawContactPoly;
					bMouseDown = false;
				}
				if(rText.AddLine("My contact[%s]", sm_bMyContacts? "X" : " ") && bMouseDown)
				{
					sm_bMyContacts = !sm_bMyContacts;
					bMouseDown = false;
				}
				if(rText.AddLine("Other contact[%s]", sm_bOtherContacts? "X" : " ") && bMouseDown)
				{
					sm_bOtherContacts = !sm_bOtherContacts;
					bMouseDown = false;
				}
				if(rText.AddLine("Constraints[%s]", sm_bDrawConstraints? "X" : " ") && bMouseDown)
				{
					sm_bDrawConstraints = !sm_bDrawConstraints;
					bMouseDown = false;
				}
				
				rText.PopIndent();
			}

			virtual bool DebugEventText(TextOutputVisitor &rText) const
			{
				PrintInstName(rText);

				NewObjectData *pOtherData = dynamic_cast<NewObjectData*>(DebugRecorder::GetInstance()->GetSharedData(m_rOther));
				if (pOtherData)
				{				
					rText.AddLine("Other Inst: %d (%s)", m_rOther.GetLevelIndex(), pOtherData->m_Description.c_str());
				}

				rText.PushIndent();
					if (GetNumChildDataItems() > 0)
					{
						rText.AddLine("Modifications:");
						rText.PushIndent();
						DataCollectionEvent::OutputData(rText);
						rText.PopIndent();
					}
					rText.AddLine("MyNormal: <%f, %f, %f>", m_MyContactNormal.GetXf(), m_MyContactNormal.GetYf(), m_MyContactNormal.GetZf() );
					rText.AddLine("MyPos: <%f, %f, %f>", m_MyContactPos.GetXf(), m_MyContactPos.GetYf(), m_MyContactPos.GetZf() );
					rText.AddLine("OtherNormal: <%f, %f, %f>", m_OtherContactNormal.GetXf(), m_OtherContactNormal.GetYf(), m_OtherContactNormal.GetZf() );
					rText.AddLine("OtherPos: <%f, %f, %f>", m_OtherContactPos.GetXf(), m_OtherContactPos.GetYf(), m_OtherContactPos.GetZf() );
					rText.AddLine("Element: %d", m_iElement);
					rText.AddLine("OtherElement: %d", m_iOtherElement);
					rText.AddLine("Component: %d", m_iComponent);
					rText.AddLine("OtherComponent: %d", m_iOtherComponent);
					rText.AddLine("Disabled: %s", m_bDisabled ? "true" : "false");
					rText.AddLine("IsConstraint: %s", m_bIsConstraint ? "true" : "false");
					rText.AddLine("Depth: %f", m_fDepth);
				rText.PopIndent();
				return true;
			}

			virtual void DebugDraw3d(eDrawFlags drawFlags) const
			{
				const bool flickerFrame = ((TIME.GetFrameCount() & 0x7) > 0x3);

				//Grey = disabled
				//Green = original
				//Red = adjusted
				Color32 col = m_bDisabled ? Color32(140,140,140,255) : Color32(0,255,0,255);
				if (m_bIsConstraint)
				{
					col.SetBlue(238);
					if (!sm_bDrawConstraints)
					{
						return;
					}
				}

				//Draw contacts
				float fRadius = 0.05f;
				if (sm_bMyContacts)
				{
					float fScale = 1.0f;

					if (drawFlags & eDrawHovered)
					{
						fScale = 1.2f;
						if (flickerFrame)
						{
							col = Color32(0,0,0,255);
							fRadius = 0.075f;
						}
					}
					else if (drawFlags & eDrawSelected)
					{
						fScale = 1.1f;

						if (flickerFrame)
						{
							col = Color32(255,255,255,255);
							fRadius = 0.1f;
						}
					}

					col = DebugRecorder::ModifyAlpha(col);
					Vec3V vLine = m_MyContactNormal * ScalarV(fScale*0.5f);
					grcWorldIdentity();
					grcDrawLine(m_MyContactPos, m_MyContactPos+vLine, col);

					grcDrawSphere(fRadius*0.25f, m_MyContactPos, 6, true, true);
					grcWorldIdentity();
				}

				//Make other contact darker
				fRadius = 0.05f;
				if (sm_bOtherContacts)
				{
					Color32 col2(col);
					col2.SetRed( (int(col.GetRed())) >> 2 );
					col2.SetGreen( (int(col.GetGreen())) >> 2);
					col2.SetBlue( (int(col.GetBlue())) >> 2);
					float fScale = 1.0f;

					if (drawFlags & eDrawHovered)
					{
						fScale = 1.2f;
						if (flickerFrame)
						{
							col2 = Color32(0,0,0,DebugRecorder::GetGlobalAlphaScale());
							fRadius = 0.075f;
						}
					}
					else if (drawFlags & eDrawSelected)
					{
						fScale = 1.1f;

						if (flickerFrame)
						{
							col2 = Color32(255,255,255,DebugRecorder::GetGlobalAlphaScale());
							fRadius = 0.1f;
						}
					}

					Vec3V vLine = m_OtherContactNormal * ScalarV(fScale*0.5f);
					grcWorldIdentity();
					grcDrawLine(m_OtherContactPos, m_OtherContactPos+vLine, col2);

					grcDrawSphere(fRadius*0.25f, m_OtherContactPos, 6, true, true);
					grcWorldIdentity();
				}

				if (sm_bDrawContactPoly)
				{
					if (m_PolyID)
					{
						DebugRecorder *pRecorder = DebugRecorder::GetInstance();
						if (pRecorder)
						{
							PolyData* pPoly = dynamic_cast<PolyData*>(pRecorder->GetSharedData(m_PolyID));
							if(pPoly)
							{
								pPoly->DebugDraw3d(drawFlags);
							}
						}
					}
				}

				//Draw any modifications / tagged info
				DataCollectionEvent::DebugDraw3d(drawFlags);
			}

			virtual bool GetEventPos(Vec3V &pos) const 
			{
				pos = m_OtherContactPos; //Otherpos? Mypos? halfway? where?
				return true;
			}

			static void ClearCurrent()
			{
				smp_LastContact = 0;
			}

			void RecordPoly()
			{
				int polyIndex = m_iOtherElement;
				if (polyIndex>=0)
				{
					const phInst *pOtherInst = m_rOther;
					if (pOtherInst)
					{
						const phArchetype *pOtherArch = pOtherInst->GetArchetype();
						if (pOtherArch)
						{
							const phBound* pOtherBound = pOtherArch->GetBound();
							if (pOtherBound)
							{
								int boundType = pOtherBound->GetType();
								if (boundType == phBound::BVH ||
									boundType == phBound::GEOMETRY 
									USE_GEOMETRY_CURVED_ONLY(|| boundType == phBound::GEOMETRY_CURVED))
								{
									const phBoundPolyhedron* pBoundGeom = static_cast<const phBoundPolyhedron*>(pOtherBound);
									if (polyIndex < pBoundGeom->GetNumPolygons())
									{
										const phPolygon &rPoly = pBoundGeom->GetPolygon(polyIndex);
										if(rPoly.GetPrimitive().GetType() == rage::PRIM_TYPE_POLYGON)
										{
											//Create an ID for the polygon
											m_PolyID = size_t(&rPoly) ^ size_t(pOtherBound) ^ size_t(pOtherInst) ^ polyIndex;

											DebugRecorder *pRecorder = DebugRecorder::GetInstance();
											PolyData *pSharedPolyData = dynamic_cast<PolyData*>(pRecorder->GetSharedData(m_PolyID));
											if(!pSharedPolyData)
											{
												DR_MEMORY_HEAP();
												pSharedPolyData = rage_new PolyData;
												pSharedPolyData->Init(rPoly, *pBoundGeom);
												pRecorder->AddSharedData(*pSharedPolyData, m_PolyID);
											}
										}
									}
								}
							}
						}
					}
				}
			}

			RecordedContact(const CallstackHelper rCallstack, const phContactIterator &impacts)
				:DataCollectionEvent(rCallstack, impacts.GetMyInstance())
				,m_rOther(impacts.GetOtherInstance())
				,m_iElement(impacts.GetMyElement())
				,m_iOtherElement(impacts.GetOtherElement())
				,m_iComponent(u8(impacts.GetMyComponent()))
				,m_iOtherComponent(u8(impacts.GetOtherComponent()))
				,m_bDisabled(impacts.IsDisabled())
				,m_PolyID(0)
				,m_bIsConstraint(impacts.IsConstraint())
				,m_fDepth(impacts.GetDepth())
			{

				//ID by contact pointer, not 100% safe?
				m_ID = (size_t)&impacts.GetContact();

				smp_LastContact = this;
				sm_iLastContactFrame = TIME.GetFrameCount();

				//Displayf("Contact %d (0x%x) - id 0x%x", PhysicsFrame::sm_PhysicsFrame, u32(&impacts.GetContact()), m_ID);
				m_OtherContactPos = impacts.GetOtherPosition();
				impacts.GetOtherNormal(m_OtherContactNormal);
				m_MyContactPos = impacts.GetMyPosition();
				impacts.GetMyNormal(m_MyContactNormal);

				RecordPoly();
			}

			RecordedContact(const CallstackHelper rCallstack, const phCachedContactIterator &impacts)
				:DataCollectionEvent(rCallstack, impacts.GetMyInstance())
				,m_rOther(impacts.GetOtherInstance())
				,m_iElement(impacts.GetMyElement())
				,m_iOtherElement(impacts.GetOtherElement())
				,m_iComponent(u8(impacts.GetMyComponent()))
				,m_iOtherComponent(u8(impacts.GetOtherComponent()))
				,m_bDisabled(impacts.IsDisabled())
				,m_PolyID(0)
				,m_bIsConstraint(impacts.IsConstraint())
				,m_fDepth(impacts.GetDepth())
			{

				//ID by contact pointer, not 100% safe?
				m_ID = (size_t)&impacts.GetContact();

				smp_LastContact = this;
				sm_iLastContactFrame = TIME.GetFrameCount();

				//Displayf("Contact %d (0x%x) - id 0x%x", PhysicsFrame::sm_PhysicsFrame, u32(&impacts.GetContact()), m_ID);
				m_OtherContactPos = impacts.GetOtherPosition();
				impacts.GetOtherNormal(m_OtherContactNormal);
				m_MyContactPos = impacts.GetMyPosition();
				impacts.GetMyNormal(m_MyContactNormal);

				RecordPoly();
			}

			RecordedContact(const CallstackHelper rCallstack, const phManifold &manifold, int iContact, bool bIsInstanceAOwner)
				:DataCollectionEvent(rCallstack, bIsInstanceAOwner ? manifold.GetInstanceA() : manifold.GetInstanceB())
				,m_rOther(bIsInstanceAOwner ? manifold.GetInstanceB() : manifold.GetInstanceA())
				,m_iComponent(u8(bIsInstanceAOwner ? manifold.GetComponentA() : manifold.GetComponentB()))
				,m_iOtherComponent(u8(bIsInstanceAOwner ? manifold.GetComponentB() : manifold.GetComponentA()))
				,m_PolyID(0)
				,m_bIsConstraint(manifold.IsConstraint())
			{
				const phContact& contact = manifold.GetContactPoint(iContact);
				m_iElement = bIsInstanceAOwner ? contact.GetElementA() : contact.GetElementB();
				m_iOtherElement = bIsInstanceAOwner ? contact.GetElementB() : contact.GetElementA();
				m_bDisabled = contact.IsContactActive();
				m_fDepth = contact.GetDepth();

				//ID by contact pointer, not 100% safe?
				m_ID = (size_t)&contact;

				smp_LastContact = this;
				sm_iLastContactFrame = TIME.GetFrameCount();

				//Displayf("Contact %d (0x%x) - id 0x%x", PhysicsFrame::sm_PhysicsFrame, u32(&impacts.GetContact()), m_ID);
				m_OtherContactPos = bIsInstanceAOwner ? contact.GetWorldPosB() : contact.GetWorldPosA();
				m_OtherContactNormal = bIsInstanceAOwner ? Negate(contact.GetWorldNormal()) : contact.GetWorldNormal();
				m_MyContactPos = bIsInstanceAOwner ? contact.GetWorldPosA() : contact.GetWorldPosB();
				m_MyContactNormal = bIsInstanceAOwner ? contact.GetWorldNormal() : Negate(contact.GetWorldNormal());

				RecordPoly();
			}

			RecordedContact()
				:m_OtherContactPos(V_ZERO)
				,m_OtherContactNormal(V_ZERO)
				,m_MyContactPos(V_ZERO)
				,m_MyContactNormal(V_ZERO)
				,m_ID(0)
				,m_PolyID(0)
				,m_fDepth(0.0f)
				,m_iElement(0)
				,m_iOtherElement(0)
				,m_iComponent(0)
				,m_iOtherComponent(0)
				,m_bDisabled(false)
				,m_bIsConstraint(false)
			{

			}

			virtual bool IsInstInvolved(phInstId rInst) const
			{
				return (m_rOther == rInst) ||  debugPlayback::PhysicsEvent::IsInstInvolved(rInst);
			}

			Vec3V m_MyContactNormal;
			Vec3V m_MyContactPos;
			Vec3V m_OtherContactNormal;
			Vec3V m_OtherContactPos;
			phInstId m_rOther;
			size_t m_ID;
			size_t m_PolyID;
			float m_fDepth;
			u32 m_iElement;
			u32 m_iOtherElement;
			u8 m_iComponent;
			u8 m_iOtherComponent;
			bool m_bDisabled;	
			bool m_bIsConstraint;
			static bool sm_bDrawContactPoly;
			static bool sm_bMyContacts;
			static bool sm_bOtherContacts;
			static bool sm_bDrawConstraints;
			static RecordedContact *smp_LastContact;
			static u32 sm_iLastContactFrame;			//Small sanity check for stale data

			static RecordedContact *GetCurrentContact(size_t iIDContact)
			{
				if (TIME.GetFrameCount() == sm_iLastContactFrame)
				{
					if (smp_LastContact->m_ID == iIDContact)
					{
						return smp_LastContact;
					}
					EventBase *pPrior = smp_LastContact->mp_Prev;
					while (pPrior)
					{
						RecordedContact *pContact = dynamic_cast<RecordedContact*>(pPrior);
						if (pContact && pContact->m_ID == iIDContact)
						{
							return pContact;
						}
						pPrior = pPrior->mp_Prev;
					}
					return 0;
				}
				return 0;
			}

			PAR_PARSABLE;
			DR_EVENT_DECL(RecordedContact);
		};

		class RecordedContactWithMatrices : public RecordedContact
		{
		public:
			RecordedContactWithMatrices()
				:m_MyMatrix(V_ZERO)
				,m_OtherMatrix(V_ZERO)
			{

			}

			RecordedContactWithMatrices(const CallstackHelper rCallstack, const phContactIterator &impacts)
				:RecordedContact(rCallstack, impacts)
			{
				if (m_rInst.IsValid())
				{
					m_MyMatrix = m_rInst->GetMatrix();
				}

				if (m_rOther.IsValid())
				{
					m_OtherMatrix = m_rOther->GetMatrix();
				}
			}

			RecordedContactWithMatrices(const CallstackHelper rCallstack, const phCachedContactIterator &impacts)
				:RecordedContact(rCallstack, impacts)
			{
				if (m_rInst.IsValid())
				{
					m_MyMatrix = m_rInst->GetMatrix();
				}

				if (m_rOther.IsValid())
				{
					m_OtherMatrix = m_rOther->GetMatrix();
				}
			}

			RecordedContactWithMatrices(const CallstackHelper rCallstack, const phManifold &manifold, int iContact, bool bIsInstanceAOwner)
				:RecordedContact(rCallstack, manifold, iContact, bIsInstanceAOwner)
			{
				if (m_rInst.IsValid())
				{
					m_MyMatrix = m_rInst->GetMatrix();
				}

				if (m_rOther.IsValid())
				{
					m_OtherMatrix = m_rOther->GetMatrix();
				}
			}

			virtual void WhileSelected() const
			{
				RecordedContact::WhileSelected();

#if __DEV
				if (sm_bDrawInstances)
				{
					if(m_rInst.IsValid())
					{
						grcColor3f(0.0f, 1.0f, 0.0f);
#if __PFDRAW
						m_rInst->GetArchetype()->GetBound()->Draw(m_MyMatrix);
#endif
					}

					if(m_rOther.IsValid())
					{
						grcColor3f(1.0f, 0.0f, 0.0f);
#if __PFDRAW
						m_rOther->GetArchetype()->GetBound()->Draw(m_OtherMatrix);
#endif
					}
				}
#endif
			}

			void AddEventOptions(const Frame &rFrame, TextOutputVisitor &rText, bool &bMouseDown) const
			{
				PhysicsEvent::AddEventOptions(rFrame, rText, bMouseDown);
				RecordedContact::AddEventOptions(rFrame, rText, bMouseDown);
				if(rText.AddLine("Instances[%s]", sm_bDrawInstances ? "X" : " ") && bMouseDown)
				{
					sm_bDrawInstances = !sm_bDrawInstances;
					bMouseDown = false;
				}
			}

			Mat34V m_MyMatrix;
			Mat34V m_OtherMatrix;

			static bool sm_bDrawInstances;

			PAR_PARSABLE;
			DR_EVENT_DECL(RecordedContactWithMatrices);
		};

		struct NMImpacts : public PhysicsEvent
		{
			NMImpacts()
				:m_bResult(false)
			{	}
			void SetData(phInstId rOther, bool bResult, const char *pIdentifier)
			{	
				DR_MEMORY_HEAP();
				m_OtherId = rOther;
				m_bResult = bResult;
				m_StringId = pIdentifier;
			}
			virtual bool IsInstInvolved(phInstId rInst) const
			{
				return (m_OtherId == rInst) ||  debugPlayback::PhysicsEvent::IsInstInvolved(rInst);
			}
			phInstId m_OtherId;
			atHashString m_StringId;
			bool m_bResult;

			PAR_PARSABLE;
			DR_EVENT_DECL(NMImpacts);
		};

		//------------------------------------------------------------------//
		//								Channels / Statics
		//------------------------------------------------------------------//
		DR_EVENT_IMP(RecordedContact);
		DR_EVENT_IMP(NMImpacts);
		DR_EVENT_IMP(RecordedContactWithMatrices);

		bool RecordedContact::sm_bDrawContactPoly = false;
		bool RecordedContact::sm_bMyContacts = true;
		bool RecordedContact::sm_bOtherContacts = true;
		bool RecordedContact::sm_bDrawConstraints = true;
		RecordedContact *RecordedContact::smp_LastContact = 0;
		u32 RecordedContact::sm_iLastContactFrame = 0;
		bool RecordedContactWithMatrices::sm_bDrawInstances = false;

		//------------------------------------------------------------------//
		//						PUBLIC RECORDING INTERFACE
		//------------------------------------------------------------------//
		extern DebugRecorder *GetRecorder();
		extern phInstId s_Selected;

		void ClearCurrentContact()
		{
			RecordedContact::ClearCurrent();
		}
		
		void RecordContact(const phContactIterator &impact)
		{
			if (!impact.GetMyInstance() || !PhysicsEvent::IsSelected( *impact.GetMyInstance() ) )
			{
				return;
			}

			if (!(DR_EVENT_ENABLED(RecordedContactWithMatrices) || DR_EVENT_ENABLED(RecordedContact)))
			{
				return;
			}

			DebugRecorder *pRecorder = DebugRecorder::GetRecorderForEvent();
			if (pRecorder)
			{
				CallstackHelper cs;

				if (pRecorder->IsRecording())
				{
					DR_MEMORY_HEAP();
					RecordedContact *pNewContact;
					if (DR_EVENT_ENABLED(RecordedContactWithMatrices))
						pNewContact = rage_new RecordedContactWithMatrices(cs, impact);
					else
						pNewContact = rage_new RecordedContact(cs, impact);

					pRecorder->AddEvent(*pNewContact);
				}
			}
		}

		void RecordContact(	const phCachedContactIterator &impact)
		{
			if (!impact.GetMyInstance() || !PhysicsEvent::IsSelected( *impact.GetMyInstance() ) )
			{
				return;
			}

			if (!(DR_EVENT_ENABLED(RecordedContactWithMatrices) || DR_EVENT_ENABLED(RecordedContact)))
			{
				return;
			}

			DebugRecorder *pRecorder = DebugRecorder::GetRecorderForEvent();
			if (pRecorder)
			{
				CallstackHelper cs;

				if (pRecorder->IsRecording())
				{
					DR_MEMORY_HEAP();
					RecordedContact *pNewContact;
					if (DR_EVENT_ENABLED(RecordedContactWithMatrices))
						pNewContact = rage_new RecordedContactWithMatrices(cs, impact);
					else
						pNewContact = rage_new RecordedContact(cs, impact);

					pRecorder->AddEvent(*pNewContact);
				}
			}
		}

		void RecordContact(const phManifold &manifold, int iContact, bool bIsInstanceAOwner)
		{
			if (!manifold.GetContactPoint(iContact).IsContactActive())
			{
				return;
			}

			if(bIsInstanceAOwner && !(manifold.GetInstanceA() && PhysicsEvent::IsSelected(*manifold.GetInstanceA())))
			{
				return;
			}

			if(!bIsInstanceAOwner && !(manifold.GetInstanceB() && PhysicsEvent::IsSelected(*manifold.GetInstanceB())))
			{
				return;
			}

			if (!(DR_EVENT_ENABLED(RecordedContactWithMatrices) || DR_EVENT_ENABLED(RecordedContact)))
			{
				return;
			}

			DebugRecorder *pRecorder = DebugRecorder::GetRecorderForEvent();
			if (pRecorder)
			{
				CallstackHelper cs;

				if (pRecorder->IsRecording())
				{
					DR_MEMORY_HEAP();
					RecordedContact *pNewContact;
					if (DR_EVENT_ENABLED(RecordedContactWithMatrices))
						pNewContact = rage_new RecordedContactWithMatrices(cs, manifold, iContact, bIsInstanceAOwner);
					else
						pNewContact = rage_new RecordedContact(cs, manifold, iContact, bIsInstanceAOwner);

					pRecorder->AddEvent(*pNewContact);
				}
			}
		}


		template<typename TIterator>
		static size_t GetContactID(const TIterator &impact)
		{
			if (!impact.GetCachedManifold().GetNumContacts())
			{
				return 0;
			}

//			if (impact.IsConstraint())
//			{
//				return (u32)&impact.GetCachedManifold();
//				//return 1;//(u32)impact.GetInstanceA() ^ (u32)impact.GetInstanceB() ^ 0x7654321;
//			}
//			else if (impact.IsForce())
//			{
//				return (u32)&impact.GetCachedManifold();
////				return 2;//(u32)impact.GetInstanceA() ^ (u32)impact.GetInstanceB() ^ 0x1234567;
//			}
//			Assert(!impact.AtEnd());
			return (size_t)&impact.GetContact();
		}


		void RecordModificationToContact( const phContactConstIterator &impact, const char *pModification, const char *pValue )
		{
			if (!(DR_EVENT_ENABLED(RecordedContactWithMatrices) || DR_EVENT_ENABLED(RecordedContact)))
			{
				return;
			}

			size_t iContactId = GetContactID<phContactConstIterator>(impact);
			if (iContactId)
			{
				RecordedContact *pCurrentContact = RecordedContact::GetCurrentContact(iContactId);
				if (pCurrentContact)
				{
					Assertf(!pCurrentContact->GetNumChildDataItems(pModification), "Already have a modification tagged: '%s'", pModification);
					pCurrentContact->AddString(pModification, pValue);
				}
			}
		}

		void RecordModificationToContact( const phCachedContactIterator &impact, const char *pModification, const char *pValue )
		{
			if (!(DR_EVENT_ENABLED(RecordedContactWithMatrices) || DR_EVENT_ENABLED(RecordedContact)))
			{
				return;
			}

			size_t iContactId = GetContactID<phCachedContactIterator>(impact);
			if (iContactId)
			{
				RecordedContact *pCurrentContact = RecordedContact::GetCurrentContact(iContactId);
				if (pCurrentContact)
				{
					Assertf(!pCurrentContact->GetNumChildDataItems(pModification), "Already have a modification tagged: '%s'", pModification);
					pCurrentContact->AddString(pModification, pValue);
				}
			}
		}
		
		void RecordModificationToContact( const phContactConstIterator &impact, const char *pModification, float fValue )
		{
			if (!(DR_EVENT_ENABLED(RecordedContactWithMatrices) || DR_EVENT_ENABLED(RecordedContact)))
			{
				return;
			}
		
			size_t iContactId = GetContactID<phContactConstIterator>(impact);
			if (iContactId)
			{
				RecordedContact *pCurrentContact = RecordedContact::GetCurrentContact(iContactId);
				if (pCurrentContact)
				{
					Assert(!pCurrentContact->GetNumChildDataItems(pModification));
					pCurrentContact->AddFloat(pModification, fValue);
				}
			}
		}

		void RecordModificationToContact( const phCachedContactIterator &impact, const char *pModification, float fValue )
		{
			if (!(DR_EVENT_ENABLED(RecordedContactWithMatrices) || DR_EVENT_ENABLED(RecordedContact)))
			{
				return;
			}

			size_t iContactId = GetContactID<phCachedContactIterator>(impact);
			if (iContactId)
			{
				RecordedContact *pCurrentContact = RecordedContact::GetCurrentContact(iContactId);
				if (pCurrentContact)
				{
					Assert(!pCurrentContact->GetNumChildDataItems(pModification));
					pCurrentContact->AddFloat(pModification, fValue);
				}
			}
		}

		void RecordModificationToContact( const phContactConstIterator &impact, const char *pModification, Vec3V_In vValue, bool bIs3dPosition )
		{
			if (!(DR_EVENT_ENABLED(RecordedContactWithMatrices) || DR_EVENT_ENABLED(RecordedContact)))
			{
				return;
			}

			size_t iContactId = GetContactID<phContactConstIterator>(impact);
			if (iContactId)
			{
				RecordedContact *pCurrentContact = RecordedContact::GetCurrentContact(iContactId);
				if (pCurrentContact)
				{
					Assert(!pCurrentContact->GetNumChildDataItems(pModification));
					pCurrentContact->AddVec3(pModification, vValue, bIs3dPosition);
				}
			}
		}

		void RecordNMFindImpacts(const phInst* pThisInst, const phInst* pOtherInst, bool bReturned, const char *pIdentifier)
		{
			NMImpacts *pFindImpacts = AddPhysicsEvent<NMImpacts>(pThisInst);
			if (pFindImpacts)
			{
				pFindImpacts->SetData(pOtherInst, bReturned, pIdentifier);
			}
		}
	}
}

//Included last to get the class defs in the CPP file
using namespace rage;
using namespace debugPlayback;
#include "debugcontacts_parser.h"

#endif //PDR_ENABLED
