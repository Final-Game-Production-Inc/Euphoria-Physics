#ifndef NM_ART_ROCKSTAR_H
#define NM_ART_ROCKSTAR_H

#include "art/ARTBaseDefs.h"
#include "physics/leveldefs.h"

namespace rage
{
  class phSimulator;
  class Vector3;
}

namespace ART 
{
  extern void handleCollision(ART::AgentID characterID, rage::phContactIterator impacts);
  extern bool setIncomingAnimationVelocityScale(float scale);
  extern bool setIncomingAnimationVelocityScale(ART::AgentID characterID, float scale);
  extern rage::phBound *getBoundFromAgent(ART::AgentID characterID);
  extern int getBoundIndexFromComponentIndex(ART::AgentID characterID, int componentIndex);
  extern rage::Matrix34 *getComponentToBoneTransform(ART::AgentID characterID, int componentIndex);
  extern void applyAgentImpacts(ART::AgentID characterID, rage::phContactIterator impacts);
  extern void setPhInstForNextAgent(rage::phInst *phInst);
  extern void setPhInstForAgent(ART::AgentID characterID, rage::phInst *phInst);
  extern rage::phInst * getPhInstForAgent(ART::AgentID characterID);
  extern bool setRockstarEnvironment(rage::phSimulator *, rage::phLevelNew *);
  extern void configureCharacter(ART::AgentID characterID, bool useZeroPose, bool leftHandFree, bool rightHandFree, float stanceBias, float COMBias);
  extern void configureTheCharacter(ART::AgentID characterID, bool setZeroPose, bool setZeroPoseArms, bool configureBalancer, float stanceBias, float COMBias);
  extern void registerWeapon(ART::AgentID characterID, int hand, int levelIndex, rage::phConstraintHandle *constrHandle, rage::Matrix34 &gunToHand, rage::Vector3 &gunToMuzzleInGun, rage::Vector3 &gunToButtInGun);
  extern void setWeaponMode(ART::AgentID characterID, int weaponMode);
  extern void setWeaponBound(ART::AgentID agentID, rage::phBound* bBound, int hand, float mass = 0, rage::Vector3* comOffset = 0);
  extern void configureDontRegisterProbeVelocity(ART::AgentID characterID, float dontRegisterProbeVelocityMassBelow, float dontRegisterProbeVelocityVolBelow);
  extern void setPhInstReferenceForThisFrame(ART::AgentID characterID, int slotIndex, rage::phInst *pInst);
  extern void setProbeIncludeFlags(ART::AgentID characterID, unsigned int flags);
  extern void setProbeTypeIncludeFlags(ART::AgentID characterID, unsigned int flags);
  extern void setProbeTypeExcludeFlags(ART::AgentID characterID, unsigned int flags);   
  extern void setTaskScheduler(int schedulerIndex);
  extern void setDistributedTasksEnabled(bool enabled);
  extern bool areDistributedTasksEnabled();
  extern void setbSpyObject(int levelIndex);
  extern bool setFromAnimationMaxSpeed(float speed);
  extern bool setFromAnimationMaxAngSpeed(float angSpeed);
  extern bool setFromAnimationAverageVel(const rage::Vector3 &vel, bool reset);
  extern bool hasCollidedWithWorld(int characterID, const char *twoCCMask); //WithNotOwnCharacter
  extern bool hasCollidedWithOtherCharacters(int characterID, const char *twoCCMask);
  extern bool hasCollidedWithEnvironment(int characterID, const char *twoCCMask);
  extern void getAttachedWeaponIndices(int characterID, int& leftHandWeaponIndex, int& rightHandWeaponIndex);
  extern void applyInjuryMask(int characterID, const char* twoCCMask, float injuryAmount);
  extern bool getCOMVel(int characterID, rage::Vector3 &outVel);
  extern bool getCOMRotVel(int characterID, rage::Vector3 &outVel);
  extern void hardConstraintOnCharacter(int characterID, int *partIndex, rage::Vector3 *offset=NULL);
  extern void balancerForceFail(int characterID);
  extern void balancerSetIgnoreFailure(int characterID, bool bValue);
  extern bool balancerHasFailed(int characterID);

} // namespace ART

#endif //NM_ART_ROCKSTAR_H
