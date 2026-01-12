// 
// fragmentnm/instance.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef FRAGMENTNM_INSTANCE_H
#define FRAGMENTNM_INSTANCE_H

#include "fragment/instance.h"
namespace ART {

class ARTFeedbackInterface;
class MessageParamsBase;
class NmRsCharacter;

}

namespace rage {

	struct phConstraintHandle;

#define HIGH_RAGDOLL_LOD_NUM_COMPONENTS 21
#define MED_RAGDOLL_LOD_NUM_COMPONENTS 16
#define LOW_RAGDOLL_LOD_NUM_COMPONENTS 12

class fragInstNM : public fragInst
{
public:
  fragInstNM(const fragType* type, const Matrix34& matrix, u32 guid = 0);
  fragInstNM(datResource& rsc);

  void PostARTMessage(const char* messageName, const ART::MessageParamsBase * params) const;
  void SetComponentTMsFromSkeleton(const crSkeleton& skeleton);
  void SetComponentTMsFromSkeleton(const crSkeleton& skeleton, int tmType, phArticulatedBody *body);//blank if !NM_ANIM_MATRICES
  void SetLastComponentTMsFromCurrent();
  void SetARTFeedbackInterface(ART::ARTFeedbackInterface* pInterface);
  void ConfigureCharacter(int characterID, bool zeroPose, int leftHandFree, int rightHandFree, float stanceBias, float COMBias);
  void ConfigureTheCharacter(int characterID, bool setZeroPose, bool setZeroPoseArms, bool configureBalancer, float stanceBias, float COMBias);
  void RegisterWeapon(int characterID, int hand, int levelIndex, rage::phConstraintHandle *constrHandle, rage::Matrix34 &gunToHand, rage::Vector3 &gunToMuzzleInGun, rage::Vector3 &gunToButtInGun);
  void SetWeaponMode(int characterID, int weaponMode);
  void SetWeaponBound(int characterID, rage::phBound* boundPtr, int hand, rage::Matrix34* boundTransform = NULL, float mass = 0.f);
  void SetWeaponBound(int characterID, rage::phBound* boundPtr, int hand, rage::Matrix34* boundTransform = NULL, float mass = 0.f, rage::Vector3* comOffset = 0);
  void setIncomingAnimationVelocityScale(int characterID, float scale);
  void CheckComponentToBoneMatrices();
#if __DEV
  //void CheckVelocities(int checkpoint);
#endif
  void ConfigureDontRegisterProbeVelocity(int characterID, float dontRegisterProbeVelocityMassBelow, float dontRegisterProbeVelocityVolBelow);
  void setProbeTypeIncludeFlags(int characterID, unsigned int flags);
  void setProbeTypeExcludeFlags(int characterID, unsigned int flags);  
  bool HasCollidedWithWorld(const char *twoCCMask) const;
  bool hasCollidedWithOtherCharacters(const char *twoCCMask);
  bool hasCollidedWithEnvironment(const char *twoCCMask);
  void ApplyInjuryMask(const char* twoCCMask, float injuryAmount);
  bool GetCOMVel(Vector3 &outCOMVel) const;
  bool GetCOMRotVel(Vector3 &outCOMVel) const;
  bool GetCOMTM(Matrix34 &outCOMTM) const;
  void initialiseCharacter();

  // 0 for normal, 1 for large
  void SetNMAssetSize(int newSize);

  // The component input is assumed to be a current ragdoll LOD component.
  // The returned component will be a mapping of the input component to high ragdoll LOD.
  int MapRagdollLODComponentCurrentToHigh(int currentLODComponent) const;
  static int MapRagdollLODComponentCurrentToHigh(int currentLODComponent, int currentLOD);

  // The component input is assumed to be a high ragdoll LOD component.
  // The returned component will be a mapping of the input component to the current ragdoll LOD.
  int MapRagdollLODComponentHighToCurrent(int highLODComponent) const;

  // Abort NM but remain a rage ragdoll
  void SwitchFromNMToRageRagdoll(bool bResetEffectors = true);

  void SetIsSimulatingAsNonNMRagdoll( bool set );
  bool IsSimulatingAsNonNMRagdoll() const { return m_SimulatingAsNonNMRagdoll; }

  float GetNMImpulseModifierUpdate();
  void ResetNMImpulseModifierUpdate();

  u16 GetRandomSeed() const { return m_randomSeed; }
  void SetRandomSeed(u16 randomSeed) { m_randomSeed = randomSeed; }

  void forceStayUprightSwitch(bool forceOn);

  void SetBlockNMActivation(bool set) { m_BlockNMActiavtion = set; }
  bool IsNMActivationBlocked() const { return m_BlockNMActiavtion; }

  bool IsZeroLastMatricesOnActivationNeeded() const { return m_ZeroLastMatricesOnActivation; }

  virtual int GetNMAgentID() const;
  virtual void PreComputeImpacts (phContactIterator impacts);
  virtual phInst* PrepareForActivation(phCollider** collider, phInst* otherInst, const phConstraintBase * constraint);
  virtual bool PrepareForDeactivation(bool colliderManagedBySim, bool forceDeactivate);
  virtual int GetARTAssetID() const;

  virtual void NotifyImpulse(const Vector3& UNUSED_PARAM(impulse), const Vector3& UNUSED_PARAM(position), int UNUSED_PARAM(component), int UNUSED_PARAM(element), float UNUSED_PARAM(breakScale)=1.0f) {}

  static float GetExtraAllowedRagdollPenetration() { return sm_ExtraAllowedRagdollPenetration; }
  static float sm_ExtraAllowedRagdollPenetration;

  int m_AgentId;

protected:
  void SetZeroLastMatricesOnActivation(bool set) { m_ZeroLastMatricesOnActivation = set; }

  void FixStuckInGeometry(phContactIterator iterator, int pedType);

private:
  bool m_BlockNMActiavtion;
  bool m_SimulatingAsNonNMRagdoll;
  bool m_ZeroLastMatricesOnActivation;

  // Maps between compoments of different ragdoll LODs
  static u8 ms_RagdollComponentHighToMedMap[HIGH_RAGDOLL_LOD_NUM_COMPONENTS];
  static u8 ms_RagdollComponentHighToLowMap[HIGH_RAGDOLL_LOD_NUM_COMPONENTS];
  static u8 ms_RagdollComponentMedToHighMap[MED_RAGDOLL_LOD_NUM_COMPONENTS];
  static u8 ms_RagdollComponentLowToHighMap[LOW_RAGDOLL_LOD_NUM_COMPONENTS];

  // Used to sync the random generator between an agent split over a network
  u16 m_randomSeed;
};

} // namespace rage

#endif // FRAGMENTNM_INSTANCE_H
