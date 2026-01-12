// 
// fragmentnm/manager.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef FRAGMENTNM_MANAGER_H
#define FRAGMENTNM_MANAGER_H

#include "rockstar/NmRsCommon.h"
#include "rockstar/NmRsEngine.h"
#include "atl/array.h"

namespace ART 
{
  class NmRsEngine;
  class MemoryManager;

  extern NmRsEngine*   gRockstarARTInstance;

  typedef unsigned int AgentID;
  typedef unsigned int AssetID;

} // namespace ART

namespace NMutils 
{
  struct MemoryConfiguration;
} // namespace NMutils

namespace rage 
{
	class fragPhysicsLOD;
} // namespace rage

namespace rage 
{

  struct nmAsset;

  class fragNMAssetManager
  {
  public:
    static void Create();
    static void Destroy();

    static fragNMAssetManager* GetInstance();

    void Load(const char* artPath);//mmmmmRename to loadAssetFile
    void InitSelfCollisionsGroups(rage::fragPhysicsLOD *fragLOD);
    void AssignSelfCollisionSet(rage::phArticulatedCollider *artCollider, ART::SelfCollisionSet *selfCollisionSet = &ART::NmRsEngine::m_DefaultSelfCollisionSet);
    void CreateCharacter(ART::AssetID assetID, int numToCreate);//Create numToCreate characters of type assetID
    void DestroyCharacter(ART::AssetID assetID);

    bool SuccessfullyLoaded(int assetId) const;

    void SetIncomingAnimVelocityScale(float fScale);

    ART::AgentID GetAgent(ART::AssetID assetI, phArticulatedCollider *collider = NULL, unsigned randomSeed = 0);
    int GetAgentCount(ART::AssetID assetId);
    int GetAgentCapacity(ART::AssetID assetId);
    void RecycleAgent(ART::AssetID assetId, ART::AgentID agentId);
    void SetDyingAgentRemoved();
    bool GetDyingAgentRemovedRecently() const;

	void SetAllowLegInterpenetration(bool allow) { ART::gRockstarARTInstance->SetAllowLegInterpenetration(allow); }
	bool GetAllowLegInterpenetration() { return ART::gRockstarARTInstance->GetAllowLegInterpenetration(); }

    int GetNumActiveNonNMRagdolls();
    int GetNonNMRagdollCapacity();
    void IncrementNonNMRagdollCount();
    void DecrementNonNMRagdollCount();

    // Profile timer funcitons
    void ResetProfileTimers();
    float GetTotalTimeUS() const { return GetStepPhase1TimeUS() + GetStepPhase2TimeUS(); }
    float GetStepPhase1TimeUS() const { return ART::gRockstarARTInstance->ms_StepPhase1TimeUS; }
    float GetApplyTransformsTimeUS() const { return ART::gRockstarARTInstance->ms_ApplyTransformsTimeUS; }
    float GetPreStepTimeUS() const { return ART::gRockstarARTInstance->ms_PreStepTimeUS; }
    float GetStepPhase2TimeUS() const { return ART::gRockstarARTInstance->ms_StepPhase2TimeUS; }
    float GetCalcCOMValsTimeUS() const { return ART::gRockstarARTInstance->ms_CalcCOMValsTimeUS; }
    float GetBehaviorsTimeUS() const { return ART::gRockstarARTInstance->ms_BehaviorsTimeUS; }
    float GetPostStepTimeUS() const { return ART::gRockstarARTInstance->ms_PostStepTimeUS; }

    // Accessors for character buoyancy data
    float GetPartBuoyancyMultiplier(int assetID, int part);
    float GetBodyBuoyancyMultiplier(int assetID);
    float GetDragMultiplier(int assetID);
    float GetWeightBeltMultiplier(int assetID);
    void SetPartBuoyancyMultiplier(int assetID, int part, float set);
    void SetBodyBuoyancyMultiplier(int assetID, float set);
    void SetDragMultiplier(int assetID, float set);
    void SetWeightBeltMultiplier(int assetID, float set);

    Matrix34* GetWorldLastMatrices(ART::AgentID agent);
    Matrix34* GetWorldCurrentMatrices(ART::AgentID agent);
#if NM_ANIM_MATRICES
    Matrix34* GetBlendOutAnimationMatrices(ART::AgentID agent);
#endif
    bool InsertAgent(int agentId);
    bool RemoveAgent(int agentId);

	bool ShouldAllowExtraPenetration(ART::AgentID agent);

    void StepPhase1(float timeStep);
    void StepPhase2(float timeStep);
    void EnableAPILogging(bool enable = true);//mmmmnoART cannot remove called by PedDebugVisualiser.obj

    bool IsOnGround(int agentId);

    void SetDistributedTasksEnabled(bool enable = true);
    bool AreDistributedTasksEnabled() const;

    void setbSpyObject(int levelIndex);
    bool SetFromAnimationMaxSpeed(float speed) const;
    bool SetFromAnimationMaxAngSpeed(float angSpeed) const;
    bool SetFromAnimationAverageVel(const rage::Vector3 &vel, bool reset = false) const;

#if __PFDRAW
	void FragNMManagerColorChoice(const phInst* inst) const;
#endif

  private:
    fragNMAssetManager();
    ~fragNMAssetManager();

    NMutils::MemoryConfiguration*     m_MemConfig;
    atArray< atArray<ART::AgentID> >  m_AgentPools;
    bool m_SuccessfullyLoaded[ART::NUM_ASSETS];//mmmmtodo mmmmhere (bug: used to mean last asset loaded succesfully.) Now means all NM assets loaded successfully. Code calls SuccessfullyLoaded as if character loaded successfully   
    static fragNMAssetManager*        sm_Instance;
    ART::MemoryManager*               m_artMemoryManager;
    ART::NmRsEngine*                  m_NmRsEngineInstance;
    int                               sm_nAssetsLoaded;
  };

#define FRAGNMASSETMGR fragNMAssetManager::GetInstance()

  inline fragNMAssetManager* fragNMAssetManager::GetInstance()
  {
    return sm_Instance; 
  }

  inline bool fragNMAssetManager::SuccessfullyLoaded(int assetId) const
  {
    return m_SuccessfullyLoaded[assetId];
  }

  //mmmmMP3 2 functions below moved to cpp in mp3
  inline Matrix34* fragNMAssetManager::GetWorldLastMatrices(ART::AgentID agent)
  {
    return ART::gRockstarARTInstance->GetWorldLastMatrices(agent);
  }

  inline Matrix34* fragNMAssetManager::GetWorldCurrentMatrices(ART::AgentID agent)
  {
    return ART::gRockstarARTInstance->GetWorldCurrentMatrices(agent);
  }

#if NM_ANIM_MATRICES
  inline Matrix34* fragNMAssetManager::GetBlendOutAnimationMatrices(ART::AgentID agent)
  {
    return ART::gRockstarARTInstance->GetBlendOutAnimationMatrices(agent);
  }
#endif

  inline bool fragNMAssetManager::ShouldAllowExtraPenetration(ART::AgentID agent)
  {
	  return ART::gRockstarARTInstance->ShouldAllowExtraPenetration(agent);
  }

} // namespace rage

#endif // FRAGMENTNM_MANAGER_H
