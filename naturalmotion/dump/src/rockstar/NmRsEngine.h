
/*
* Copyright (c) 2005-2012 NaturalMotion Ltd. All rights reserved. 
*
* Not to be copied, adapted, modified, used, distributed, sold,
* licensed or commercially exploited in any manner without the
* written consent of NaturalMotion. 
*
* All non public elements of this software are the confidential
* information of NaturalMotion and may not be disclosed to any
* person nor used for any purpose not expressly approved by
* NaturalMotion in writing.
*
*/

#ifndef NM_RS_ENGINE_H
#define NM_RS_ENGINE_H

#include "NmRsCommon.h"
#include "NmRsInclude.h"
#include "atl/queue.h"
#include "atl/vector.h"


#if __PFDRAW
EXT_PFD_DECLARE_GROUP(NaturalMotion);
EXT_PFD_DECLARE_ITEM(GrabBehaviour);
#endif // __PFDRAW

namespace ART
{
  class NmRsCBUTaskManager;
  class NmRsLimbManager;
  class NmRsGenericPart;
  class NmRsCharacter;
  class NmRsDebugDraw;
  struct NmRsCharacterTypeData;
#if ART_ENABLE_BSPY
  class NmRsSpy;
#endif // ART_ENABLE_BSPY

  struct SelfCollisionSet
  {
	  rage::atArray<rage::u8,16> m_PartCanCollideA;
	  rage::atArray<rage::u8,16> m_PartCanCollideB;
  };

  /**
  * The NmRsEngine class acts as a character management system, responsible for
  * primary communication between the ART API and our active simulated NmRsCharacter instances
  */
  class NmRsEngine 
  {
  public:
    NmRsEngine(ART::MemoryManager* services);
    virtual ~NmRsEngine();

    bool initEngine();
    bool termEngine();

    NmRsCharacter*  createCharacter(AssetID assetID, rage::phArticulatedCollider *collider = NULL, unsigned randomSeed = 0);
    bool            destroyCharacter(NmRsCharacter* hCharacter);

    bool insertAgent(ART::AgentID characterID);
    bool removeAgent(ART::AgentID characterID);


#if ART_ENABLE_BSPY
    void setbSpyObject(int levelIndex); 
    void bSpyProcessInstanceOnContact(rage::phInst* inst);
    void bSpyProcessDynamicBoundOnContact(rage::phInst* inst, rage::phBound* bound, const rage::Matrix34& tm);
    //mmmmBSPY3INTEGRATION
    int getBSpyIDFromCharacterID(AgentID characterId);
#endif//ART_ENABLE_BSPY
#if (!ART_ENABLE_BSPY) && NM_EMPTY_BSPYSERVER
    //mmmmBSPY3INTEGRATION
    int getBSpyIDFromCharacterID(AgentID characterId);
#endif//#if (!ART_ENABLE_BSPY) && NM_EMPTY_BSPYSERVER

    void ResetProfileTimers();
    void stepPhase1(float deltaTime);
    void stepPhase2(float deltaTime);

    ART::MemoryManager * GetArtMemoryManager() { return m_artMemoryManager; }

    // ART::TransformSource
    bool getComponentTMs(NmRsCharacter* rsCharacter, rage::Matrix34* dest, size_t destLength) const;
    int getComponentTMs(AgentID characterID, rage::Matrix34* dest) const;
    bool setInitialTM(ART::AgentID characterID, int componentIdx, const rage::Matrix34 &tm, const rage::Matrix34 *previousFrameTm);
    bool setComponentTMs(NmRsCharacter* rsCharacter, const rage::Matrix34* src, size_t srcLength, IncomingTransformStatus itmStatusFlags, IncomingTransformSource source);

    rage::Matrix34* GetWorldLastMatrices(ART::AgentID characterID);
    rage::Matrix34* GetWorldCurrentMatrices(ART::AgentID characterID);
#if NM_ANIM_MATRICES
    rage::Matrix34* GetBlendOutAnimationMatrices(ART::AgentID characterID);
#endif
    bool setAgentFeedbackInterface(AgentID characterID, ARTFeedbackInterface* iface);

    // support for direct systems invocation
    bool directInvoke(NmRsCharacter* rsCharacter, InvokeUID iUID, const MessageParamsBase* const params);

    float getImpulseModifierUpdate(NmRsCharacter* rsCharacter);
    void resetImpulseModifierUpdate(NmRsCharacter* rsCharacter);

    int GetActiveNonNMRagdolls() const { return m_activeNonNMRagdolls; }
    void IncrementNonNMRagdoll() { m_activeNonNMRagdolls++; }
    void DecrementNonNMRagdoll() { m_activeNonNMRagdolls--; }

    // functions to bind us to the running in-game environment
    void setRockstarEnvironment(rage::phSimulator* sim, rage::phLevelNew* level) { m_simulator = sim; m_level = level; }
    void setIncomingAnimationVelocityScale(float scale) { m_incomingAnimationVelocityScale = scale; }
    void setIncomingAnimationVelocityScale(ART::AgentID agentID, float scale);
    void setFromAnimationMaxSpeed(float speed) { m_animationMaxSpeed = speed; }
    void setFromAnimationMaxAngSpeed(float angSpeed) { m_animationMaxAngSpeed = angSpeed; }
    void setFromAnimationAverageVel(const rage::Vector3 &vel, bool reset);
    void setPhInstForAgent(ART::AgentID agentID, rage::phInst *pInst);
    rage::phInst * getPhInstForAgent(ART::AgentID agentID);
    void setProbeIncludeFlags(unsigned int flags);
    void setProbeTypeIncludeFlags(unsigned int flags);
    void setProbeTypeExcludeFlags(unsigned int flags);
    bool hasCollidedWithWorld(int agentID, const char *twoCCMask);
    void applyAgentImpacts(ART::AgentID agentID, rage::phContactIterator impacts);

	static void AssignSelfCollisionSet(rage::phArticulatedCollider *artCollider, SelfCollisionSet *selfCollisionSet);

    void configureCharacter(
      ART::AgentID agentID, 
      bool useZeroPose, 
      bool leftHandFree, 
        bool rightHandFree, 
		float stanceBias, 
		float COMBias);//mmmmUnused leftHandFree, rightHandFree

    void configureTheCharacter(
      ART::AgentID  characterID, 
      bool setZeroPose, 
      bool setZeroPoseArms, 
      bool configureBalancer, 
      float stanceBias, 
      float COMBias);

    void registerWeapon(
      ART::AgentID characterID, 
      int hand, 
      int levelIndex, 
		rage::phConstraintHandle *constrHandle,
		rage::Matrix34 &gunToHand, 
		rage::Vector3 &gunToMuzzleInGun, 
		rage::Vector3 &gunToButtInGun);

    void setWeaponMode(
      ART::AgentID agentID, 
      int weaponMode);

    void setWeaponBound(
      ART::AgentID agentID,
      rage::phBound* boundPtr,
      int hand, 
      float mass = 0.f,
      rage::Vector3* comOffset = 0);

    void configureDontRegisterProbeVelocity(
      ART::AgentID agentID,
      float dontRegisterProbeVelocityMassBelow,
      float dontRegisterProbeVelocityVolBelow);

    // access to RAGE-side internal objects
    rage::phBound *getBoundFromAgent(ART::AgentID agentID) const;
    int getBoundIndexFromComponentIndex(ART::AgentID agentID, int componentIndex) const;
    rage::Matrix34 *getComponentToBoneTransform(ART::AgentID agentID, int componentIndex) const;


    // access to level/simulator
    inline rage::phSimulator* getSimulator() const { return m_simulator; }
    inline rage::phLevelNew* getLevel() const { return m_level; }

    // access to NM-side internal objects / characters
    NmRsGenericPart *getGenericPartForComponentInActor(ART::AgentID agentID, int componentIdx) const;
    inline NmRsCBUTaskManager* getTaskManager() const { return m_taskManager; }
    inline NmRsLimbManager* getLimbManager() const { return m_limbManager; }

    // time-step data
    inline float getIncomingAnimationVelocityScale() const { return m_incomingAnimationVelocityScale; }
    inline float getAnimationMaxSpeed() const { return m_animationMaxSpeed; }
    inline float getAnimationMaxAngSpeed() const { return m_animationMaxAngSpeed; }
    inline rage::Vector3 getAnimationAverageVel() const { return m_animationAverageVel; }
    inline float getLastKnownUpdateStep() const { return m_lastKnownUpdateStep; }
    inline float getLastKnownUpdateStepClamped() const { return m_lastKnownUpdateStepClamped; }
    inline rage::Vector3 getUpVector() const { return m_gUp; }

    inline void setSchedulerIndex(int sched) { m_schedulerIndex = sched; }
    inline int getSchedulerIndex() const { return m_schedulerIndex; }

	void SetAllowLegInterpenetration(bool allow) { m_AllowLegInterpenetration = allow; }
	bool GetAllowLegInterpenetration() const { return ART::gRockstarARTInstance->m_AllowLegInterpenetration; }

	bool ShouldAllowExtraPenetration(ART::AgentID agentID);

#if ART_ENABLE_BSPY
#if NM_ANIM_MATRICES

    inline bool leadInAnimationSentIn(ART::AssetID assetID) 
    { 
      Assert(assetID < NUM_ASSETS);
      return m_leadInAnimationSent[assetID];
    }

    inline void setLeadInAnimationSentIn(ART::AssetID assetID, bool sent) 
    { 
      Assert(assetID < NUM_ASSETS);
      m_leadInAnimationSent[assetID] = sent;
    }

    void setLeadInAnimationTMs(rage::Matrix34* tms, ART::AssetID assetID);//TEST ONLY

    rage::Matrix34* GetLeadInAnimationMatrices(ART::AssetID assetID);
#endif
#endif


    inline int GetNumInsertedCharacters() { return (int)m_characterVector.size(); }

    bool areDistributedTasksEnabled() const;
    void setDistributedTasksEnabled(bool onoff);

#if ART_ENABLE_BSPY
    NmRsSpy *getNmRsSpy() const { return m_spy; }
    void getAgentStateBlock(AgentState& as) const;
#endif // ART_ENABLE_BSPY

    /**
    * perlin noise function for use by behaviours
    * there is only one instance of the perlin grid data, and it is deterministic
    * based on the 3 inputs, so it can be shared here, by the Engine class
    */
    float perlin3(float x, float y, float z) const;


    inline NmRsCharacter* getCharacterFromAgentID(AgentID characterID) const;
    ART::AgentID getAgentIDFromCharacter(NmRsCharacter* character) const;


    unsigned int  getMaxAssets() const { return m_maxAssets; }

    NmRsCharacterTypeData      *m_characterTypeData;

	static SelfCollisionSet			m_DefaultSelfCollisionSet;
	static SelfCollisionSet			m_PointGunSelfCollisionSet;
	static SelfCollisionSet			m_FlinchSelfCollisionSet;

    int                         m_framesSinceDyingAgentRemoved;
    unsigned int                m_activeNonNMRagdolls;

  private:

    bool initialiseAgent(AssetID asset, AgentID characterID, rage::phArticulatedCollider *collider = NULL, unsigned randomSeed = 0);

    enum { MaxNumCharacters = 32 };

    typedef rage::atFixedArray<ART::AgentID, MaxNumCharacters>      NmRsCharacterIDStack;
    typedef rage::atFixedVector<NmRsCharacter*, MaxNumCharacters>   NmRsCharacterVector32;
    typedef rage::atFixedVector<ART::AssetID, 8>                    AssetIDVector8;

    // up vector, -ve gravity direction
    rage::Vector3               m_gUp;

    // stack of available IDs - pulled from when a new character is created, pushed back to when character is deleted
    NmRsCharacterIDStack        m_characterIDs;

    // list of active characters, plus array of pointers for fast lookup by index 
    NmRsCharacterVector32       m_characterVector;
    NmRsCharacter              *m_characterByIndex[MaxNumCharacters];


    // task manager for all characters
    NmRsCBUTaskManager         *m_taskManager;

    // Limb manager manages limb request memory for all characters.
    NmRsLimbManager*            m_limbManager;

    // instance of simulator & scene, either built when plugged into euphoria:studio or
    // set by the game client through exposed interface functions
    rage::phSimulator          *m_simulator;
    rage::phLevelNew           *m_level;

#if ART_ENABLE_BSPY
    // instance of remote debugger, if applicable/present
    NmRsSpy                    *m_spy;

    // tracks asset IDs that we have encountered while building characters; any one not seen before needs serializing
    // and transmitting back to bSpy
    AssetIDVector8              m_knownAssetIDs; 

    // level indices forced to transmit state back to bSpy
    atQueue<int, 30>            m_bSpyObjectsQueue;

#if NM_ANIM_MATRICES
    rage::Matrix34             *m_LeadInAnimationMatrices[NUM_ASSETS];
    bool                        m_leadInAnimationSent[NUM_ASSETS];
#endif//NM_ANIM_MATRICES
#endif // ART_ENABLE_BSPY

    // this is part of a temporary fix for GTA (and RAGE in general)
    // where the physics is double-stepped but the applied animation
    // is not; meaning that calculated velocities end up twice as large
    // as they should be. it's 1.0 by default.
    float                       m_incomingAnimationVelocityScale,
                                m_animationMaxSpeed,
                                m_animationMaxAngSpeed;
    rage::Vector3               m_animationAverageVel;
    bool m_animationAverageVelSet;

    // last physics update step recorded, initialized to 1/60
    float                       m_lastKnownUpdateStep;
    float                       m_lastKnownUpdateStepClamped; // max(last known update, 1/60)

    // used when creating RAGE tasks
    int                         m_schedulerIndex;

    unsigned int                m_maxAssets;

    ART::MemoryManager         *m_artMemoryManager;

    // toggle this to enable use of the RAGE task manager in pushing the computation
    // of CBU tasks out into multiple threads / SPUs. turn it off to assist debugging / per-task profiling.
    bool                        m_distributedTasksEnabled;

	bool						m_AllowLegInterpenetration;

  public:
    // Timers
    static float ms_StepPhase1TimeUS;
    static float ms_ApplyTransformsTimeUS;
    static float ms_PreStepTimeUS;
    static float ms_StepPhase2TimeUS;
    static float ms_CalcCOMValsTimeUS;
    static float ms_BehaviorsTimeUS;
    static float ms_PostStepTimeUS;
  };

  inline NmRsCharacter* NmRsEngine::getCharacterFromAgentID(ART::AgentID id) const
  {
    if (id == INVALID_AGENTID)
      return NULL;

    Assert(id < MaxNumCharacters);
    return m_characterByIndex[id];
  }
}

#endif // NM_RS_ENGINE_H
