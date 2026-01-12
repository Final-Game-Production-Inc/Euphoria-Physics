/*
* Copyright (c) 2005-2010 NaturalMotion Ltd. All rights reserved.
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


#include "NmRsInclude.h"
#include "NmRsEngine.h"
#include "NmRsCharacter.h"
#include "NmRsUtils.h"
#include "NmRsGenericPart.h"
#include "NmRsEffectors.h"
#include "NmRsCBU_TaskManager.h"
#include "NmRsIK.h"
#include "NmRsEffectors.h"

#include "NmRsCBU_ArmsWindmillAdaptive.h"
#include "NmRsCBU_Shared.h"
#include "NmRsCBU_BodyFoetal.h"
#include "NmRsCBU_BodyWrithe.h"
#include "NmRsCBU_DynamicBalancer.h"
#include "NmRsCBU_BodyBalance.h"
#include "NmRsCBU_Teeter.h"
#include "NmRsCBU_Electrocute.h"
#include "NmRsCBU_Yanked.h"
#include "NmRsCBU_StaggerFall.h"
#include "NmRsCBU_BalancerCollisionsReaction.h"
#include "NmRsCBU_Dragged.h"
#include "NmRsCBU_BraceForImpact.h"
#include "NmRsCBU_AnimPose.h"
#include "NmRsCBU_HeadLook.h"
#include "NmRsCBU_Pedal.h"
#include "NmRsCBU_RollUp.h"
#include "NmRsCBU_Flinch.h"
#include "NmRsCBU_SpineTwist.h"
#include "NmRsCBU_RollDownStairs.h"
#include "NmRsCBU_InjuredOnGround.h"
#include "NmRsCBU_Carried.h"
#include "NmRsCBU_Dangle.h"
#include "NmRsCBU_CatchFall.h"
#include "NmRsCBU_TaskManager.h"
#include "NmRsCBU_Grab.h"
#include "NmRsCBU_HighFall.h"
#if ALLOW_TRAINING_BEHAVIOURS
#include "NmRsCBU_Landing.h"
#endif
#include "NmRsCBU_Shot.h"
#include "NmRsCBU_PointArm.h"
#include "NmRsCBU_LearnedCrawl.h"
#include "NmRsCBU_FallOverWall.h"
#include "NmRsCBU_PointGun.h"
#include "NmRsCBU_Stumble.h"
#include "NmRsCBU_ArmsWindmill.h"
#include "NmRsCBU_Buoyancy.h"
#include "NmRsBullet.h"
#if ALLOW_DEBUG_BEHAVIOURS
//Debug only behaviours
#include "NmRsCBU_DebugRig.h"
#endif //ALLOW_DEBUG_BEHAVIOURS

#include "nmutils/TypeUtils.h"
#include "art/MessageParams.h"

static class mBehaviour* mCurrentBehaviour = NULL;
class mBehaviour
{
public:
  mBehaviour(const char *str){ m_name = str; m_hash = NMutils::hashString(str); mCurrentBehaviour = this; }
  unsigned int m_hash;
  const char *m_name;
};
class mParameter
{
public:
  mParameter(const char *str, float defaultValue, float minValue, float maxValue)
  {
    m_name = str;
    m_hash = NMutils::hashString(str);
    m_type = ART::MessageParams::kFloat;
    m_default.f = defaultValue;
    m_min.f = minValue;
    m_max.f = maxValue;
    m_behaviour = mCurrentBehaviour;
  }
  mParameter(const char *str, int defaultValue, int minValue, int maxValue)
  {
    m_name = str;
    m_hash = NMutils::hashString(str);
    m_type = ART::MessageParams::kInt;
    m_default.i = defaultValue;
    m_min.i = minValue;
    m_max.i = maxValue;
    m_behaviour = mCurrentBehaviour;
  }
  mParameter(const char *str, bool defaultValue, bool minValue, bool maxValue)
  {
    m_name = str;
    m_hash = NMutils::hashString(str);
    m_type = ART::MessageParams::kBool;
    m_default.b = defaultValue;
    m_min.b = minValue;
    m_max.b = maxValue;
    m_behaviour = mCurrentBehaviour;
  }
  /* mParameter(const char *str, const rage::Vector3 &defaultValue, float minLength, float maxLength)
  {
    m_name = str;
    m_hash = NMutils::hashString(str);
    m_type = ART::MessageParams::kVector3;
    m_default.vec[0] = defaultValue.x; m_default.vec[1] = defaultValue.y; m_default.vec[2] = defaultValue.z;
    m_min.vec[0] = minLength;
    m_max.vec[0] = maxLength;
    m_behaviour = mCurrentBehaviour;
  } */
  mParameter(const char *str, float x, float y, float z, float minLength, float maxLength)
  {
	  m_name = str;
	  m_hash = NMutils::hashString(str);
	  m_type = ART::MessageParams::kVector3;
	  m_default.vec[0] = x; m_default.vec[1] = y; m_default.vec[2] = z;
	  m_min.vec[0] = minLength;
	  m_max.vec[0] = maxLength;
	  m_behaviour = mCurrentBehaviour;
  }
  mParameter(const char *str, float maxLength)
  {
	  m_name = str;
	  m_hash = NMutils::hashString(str);
	  m_type = ART::MessageParams::kVector3;
	  m_default.vec[0] = 0; m_default.vec[1] = 0; m_default.vec[2] = 0;
	  m_min.vec[0] = 0;
	  m_max.vec[0] = maxLength;
	  m_behaviour = mCurrentBehaviour;
  }
  mParameter(const char *str, const char *defaultValue, const char *UNUSED_PARAM(minValue), const char *UNUSED_PARAM(maxValue))
  {
    m_name = str;
    m_hash = NMutils::hashString(str);
    m_type = ART::MessageParams::kString;
    strncpy(m_default.s, defaultValue, 1 + rage::Min(strlen(defaultValue), (size_t)ART_PARAMETER_STRING_LENGTH_IN_MESSAGEPARAMS));
    m_behaviour = mCurrentBehaviour;
  }
  mParameter(const char *str, const void *defaultValue, const void *UNUSED_PARAM(minValue), const void *UNUSED_PARAM(maxValue))
  {
    m_name = str;
    m_hash = NMutils::hashString(str);
    m_type = ART::MessageParams::kReference;
    m_default.r = defaultValue;
    m_behaviour = mCurrentBehaviour;
  }
  void setDefault(bool& val){ val = m_default.b; }
  void setDefault(int& val){ val = m_default.i; }
  void setDefault(float& val){ val = m_default.f; }
  void setDefault(rage::Vector3& val){ val.Set(m_default.vec[0], m_default.vec[1], m_default.vec[2]); }
  void setDefault(char* val){ strncpy(val, m_default.s, 1 + rage::Min(strlen(m_default.s), (size_t)ART_PARAMETER_STRING_LENGTH_IN_MESSAGEPARAMS)); }
  void setDefault(const void*& val){ val = m_default.r; }

  void setValue(bool& val, const ART::MessageParams::Parameter& param){ Assert(param.m_type==ART::MessageParams::kBool); val = check(param.v.b); }
  void setValue(int&  val, const ART::MessageParams::Parameter& param){ Assert(param.m_type==ART::MessageParams::kInt); val = check(param.v.i); }
  void setValue(float& val, const ART::MessageParams::Parameter& param){Assert(param.m_type==ART::MessageParams::kFloat); val = check(param.v.f); }
  void setValue(rage::Vector3& val, const ART::MessageParams::Parameter& param){ Assert(param.m_type==ART::MessageParams::kVector3); val = check(param.v.vec); }
  void setValue(char* val, const ART::MessageParams::Parameter& param){ Assert(param.m_type==ART::MessageParams::kString); const char *string = check(param.v.s); strncpy(val, string, 1 + rage::Min(strlen(string), (size_t)ART_PARAMETER_STRING_LENGTH_IN_MESSAGEPARAMS)); }
  void setValue(const void*& val, const ART::MessageParams::Parameter& param){ Assert(param.m_type==ART::MessageParams::kReference); val = check(param.v.r); }

  bool check(bool val){ Assert(m_type==ART::MessageParams::kBool); Assertf(val >= m_min.b && val <= m_max.b, "%s:%s bool value is out of range. Value: %d, min: %d, max: %d", m_behaviour->m_name, m_name, val, m_min.b, m_max.b); return val; }
  int check(int val){ Assert(m_type==ART::MessageParams::kInt); Assertf(val >= m_min.i && val <= m_max.i, "%s:%s int value is out of range. Value: %d, min: %d, max: %d", m_behaviour->m_name, m_name, val, m_min.i, m_max.i); return val; }
  float check(float val){ Assert(m_type==ART::MessageParams::kFloat); Assertf(val >= m_min.f && val <= m_max.f, "%s:%s float value is out of range. Value: %.3f, min: %.3f, max: %.3f", m_behaviour->m_name, m_name, val, m_min.f, m_max.f); return val; }
  rage::Vector3 check(const NMutils::NMVector3 vec)
  { 
    rage::Vector3 val(vec[0], vec[1], vec[2]);
    Assert(m_type==ART::MessageParams::kVector3);
    Assertf((val.Mag2()+0.025f >= m_min.vec[0]*m_min.vec[0]) && (val.Mag2()-0.025f <= m_max.vec[0]*m_max.vec[0]), "%s:%s vector length is out of range. Length: %.3f, min: %.3f, max: %.3f", m_behaviour->m_name, m_name, val.Mag(), m_min.vec[0], m_max.vec[0]); // wrong length vector. Some give allowed
    return val;
  }
  const char *check(const char* val){ Assert(m_type==ART::MessageParams::kString); return val; }
  const void *check(const void* val){ Assert(m_type==ART::MessageParams::kReference); return val; }

  const ART::MessageParams::Parameter::Value* findParam(unsigned int *hashes, int numHashes, const ART::MessageParamsBase* const params, bool bUseDefault = true)
  {
    for (int i = numHashes-1; i>=0; i--)
      if (hashes[i] == m_hash)
        return &params->getParam(i).v;
    return bUseDefault ? &m_default : NULL;
  }
  // Note that if messageParams stored the hash with the name, then this class is getting close to the messageParams::Parameters class!
  ART::MessageParams::ParamType m_type;
  unsigned int m_hash;
  const char *m_name;
  mBehaviour *m_behaviour;
  ART::MessageParams::Parameter::Value m_default, m_min, m_max;
};

//If you change the name of the macros or add to them in the block below then Assets\Rockstar_DocoMessages\docomessages.py will have to be updated
#define BEHAVIOUR(_name_) static mBehaviour m##_name_(#_name_); namespace ns##_name_ 
#define PARAMETER(_name_,_default_,_type_,_min_,_max_) static mParameter mp##_name_(#_name_,(_type_)_default_,_min_,_max_)
#define PARAMETERV(_name_,_x_,_y_,_z_,_min_,_max_) static mParameter mp##_name_(#_name_,_x_,_y_,_z_,_min_,_max_)
#define PARAMETERV0(_name_,_max_) static mParameter mp##_name_(#_name_,_max_)
#define FEEDBACK(_type_, _feedbackname_)  // doesn't actually do anything
#define FEEDBACKPARAM(_feedbackname_, _argNo_, _argType_)  // doesn't actually do anything
#define FEEDBACKDESCR(_feedbackname_, _argNo_, _argValue_, _argName_)  // doesn't actually do anything
#define rage_Vector3(x,y,z)	x,y,z

#include "NmRsMessageDefinitions.h"

#undef rage_Vector3

#define APPLY_PARAMETER(_from_,_to_) \
  if (start==1 || !params)\
  mp##_from_.setDefault(_to_);\
  for (int i = 0; i<numHashes; i++)\
{\
  if (hashes[i] == mp##_from_.m_hash)   \
{\
  mp##_from_.setValue(_to_, params->getParam(i)); \
  Assert(_to_ == _to_); /* invalid value */ \
}\
}
// does mask string to bit mask conversion
#define APPLY_MASK_PARAMETER(_from_,_to_)\
{\
  char buffer[ART_PARAMETER_STRING_LENGTH_IN_MESSAGEPARAMS];\
  buffer[0] = 0;\
  APPLY_PARAMETER(_from_,buffer);\
  if(buffer[0] != 0)\
  _to_ = m_character->expressionToMask(buffer);\
}

// TDL looks messy, all it is trying to do is give you the parameter value if it is set, or the default otherwise
#define ARG_FLOAT(_from_)   (mp##_from_.check(mp##_from_.findParam(hashes, numHashes, params)->f))
#define ARG_INT(_from_)     (mp##_from_.check(mp##_from_.findParam(hashes, numHashes, params)->i))
#define ARG_BOOL(_from_)    (mp##_from_.check(mp##_from_.findParam(hashes, numHashes, params)->b))
#define ARG_VECTOR3(_from_) (mp##_from_.check(mp##_from_.findParam(hashes, numHashes, params)->vec))
#define ARG_STRING(_from_)  (mp##_from_.check(mp##_from_.findParam(hashes, numHashes, params)->s))
#define ARG_POINTER(_from_) (mp##_from_.check(mp##_from_.findParam(hashes, numHashes, params)->r))
#define ARG_SET(_from_)     (mp##_from_.findParam(hashes, numHashes, params, false)!=NULL)

#define TRY_UPDATE_BEHAVIOUR_MESSAGE(_class_,_name_) \
  if (iUID == m##_name_.m_hash)\
{ \
  _class_ *cbuBehaviour = (_class_ *)m_cbuTaskManager->getTaskByID(m_agentID, bvid_##_name_);\
  /* Assert(cbuBehaviour); can't find this behaviour */ \
  if (cbuBehaviour) \
  { \
    cbuBehaviour->updateBehaviourMessage(params);\
    BSPY_DINV(#_name_, params); \
    return true; \
  } \
}
#define TRY_UPDATE_MESSAGE(_name_) \
  if (iUID == m##_name_.m_hash)\
{ \
  BSPY_DINV(#_name_, params); \
  _name_##Update(params, this);\
  return true; \
}

int getStart(ART::NmRsCharacter* character, unsigned int *hashes, int numHashes, int& priority, ART::BehaviourMask& mask, float& blend, const ART::MessageParamsBase* const params)
{
  int start = -1; // neither activate or deactivate
  for (int i = 0; i<numHashes; i++)
    hashes[i] = NMutils::hashString(params->getParam(i).m_name);

  unsigned int startHash = NMutils::hashString("start"); // should make this fixed eg -1022375
  unsigned int priorityHash = NMutils::hashString("priority"); // should make this fixed eg -1022375
  unsigned int maskHash = NMutils::hashString("mask"); // should make this fixed eg -1022375
  unsigned int blendHash    = NMutils::hashString("blend");    // should make this fixed eg -1022375

  for (int i = 0; i<numHashes; i++)
  {
    if(hashes[i] == startHash)
      start = (int)params->getParam(i).v.b;
    if(hashes[i] == priorityHash)
      priority = (int)params->getParam(i).v.i;
    if(hashes[i] == maskHash)
       mask = character->expressionToMask(params->getParam(i).v.s);
	if(hashes[i] == blendHash)
      blend = params->getParam(i).v.f;
  }
  return start;
}

#define GET_ONESHOT_START(_name_) \
  using namespace ns##_name_; \
  int numHashes = params ? params->getUsedParamCount() : 0; \
  unsigned int *hashes = (unsigned int *) alloca(numHashes * sizeof(unsigned int)); \
  int priority = -1;\
  BehaviourMask mask = bvmask_Full;\
  float blend = 1.0f;\
  int start; start = getStart(character, hashes, numHashes, priority, mask, blend, params); start = start; mask = mask;

#define GET_BEHAVIOUR_START(_name_) \
  using namespace ns##_name_; \
  int numHashes = params ? params->getUsedParamCount() : 0; \
  unsigned int *hashes = (unsigned int *) alloca(numHashes * sizeof(unsigned int)); \
  m_priority = getBvID();\
  int start; start = getStart(m_character, hashes, numHashes, m_priority, m_mask, m_blend, params);\

namespace ART
{
 
#if ART_ENABLE_BSPY
# define BSPY_DINV(name, params) sendDirectInvoke(name, params);
#else
# define BSPY_DINV(name, params)
#endif
#if ART_ENABLE_BSPY

    void NmRsCharacter::sendDirectInvoke(const char* msg, const ART::MessageParamsBase* const params)
    {
      if (getBSpyID() == INVALID_AGENTID)
        return;
      
      NmRsSpy& spy = *m_rsEngine->getNmRsSpy();
      if (!spy.isClientConnected())
        return;

      bSpy::DirectInvokePacket dip((bs_uint16)getBSpyID(), (bs_uint8)params->getUsedParamCount());
      dip.m_msgNameToken = spy.getTokenForString(msg);

      // translate all the parameters... 
      for (int i=0; i<params->getUsedParamCount(); i++)
      {
        const MessageParams::Parameter& p = params->getParam(i);

        dip.m_params[i].m_name = spy.getTokenForString(p.m_name);
        switch (p.m_type)
        {
        case MessageParams::kInt:
          dip.m_params[i].set(p.v.i);
          break;
        case MessageParams::kFloat:
          dip.m_params[i].set(p.v.f);
          break;
        case MessageParams::kBool:
          dip.m_params[i].set(p.v.b);
          break;
        case MessageParams::kString:
          dip.m_params[i].set(spy.getTokenForString(p.v.s));
          break;
        case MessageParams::kVector3:
          dip.m_params[i].set(p.v.vec[0], p.v.vec[1], p.v.vec[2]);
          break;
        case MessageParams::kReference:
          {
            // have to send something...
            int voidRefCast = (int)(p.v.r);
            dip.m_params[i].set(voidRefCast);
          }
          break;

        case MessageParams::kInvalid:
        case MessageParams::kUnknown:
          Assert(0);
          break;
        }    
      }

      bspySendPacket(dip);
    }

#endif // ART_ENABLE_BSPY

    void incomingTransformsUpdate(const MessageParamsBase* const params, NmRsCharacter *character)
    {
      GET_ONESHOT_START(incomingTransforms);
      if (start == 1)
        character->setIncomingTransformApplyMode(kEnabling);
      else
        character->setIncomingTransformApplyMode(kDisabling);
    }
    void leanInDirectionUpdate(const MessageParamsBase* const params, NmRsCharacter *character)
    {
      GET_ONESHOT_START(leanInDirection);
      NmRsCBUDynamicBalancer* cbuDyn = (NmRsCBUDynamicBalancer*)character->getTask(bvid_dynamicBalancer);
      Assert(cbuDyn);
      if (start)
      cbuDyn->autoLeanInDirection(ARG_VECTOR3(dir), ARG_FLOAT(leanAmount));
      else
        cbuDyn->autoLeanCancel();
    }
    void leanRandomUpdate(const MessageParamsBase* const params, NmRsCharacter *character)
    {
      GET_ONESHOT_START(leanRandom);
      NmRsCBUDynamicBalancer* cbuDyn = (NmRsCBUDynamicBalancer*)character->getTask(bvid_dynamicBalancer);
      Assert(cbuDyn);
      if (start)
      cbuDyn->autoLeanRandom(ARG_FLOAT(leanAmountMin), ARG_FLOAT(leanAmountMax),ARG_FLOAT(changeTimeMin), ARG_FLOAT(changeTimeMax));
      else
        cbuDyn->autoLeanCancel();
    }
    void leanToPositionUpdate(const MessageParamsBase* const params, NmRsCharacter *character)
    {
      GET_ONESHOT_START(leanToPosition);
      NmRsCBUDynamicBalancer* cbuDyn = (NmRsCBUDynamicBalancer*)character->getTask(bvid_dynamicBalancer);
      Assert(cbuDyn);
      if (start)
      cbuDyn->autoLeanToPosition(ARG_VECTOR3(pos), ARG_FLOAT(leanAmount));
      else
        cbuDyn->autoLeanCancel();
    }
    void leanTowardsObjectUpdate(const MessageParamsBase* const params, NmRsCharacter *character)
    {
      GET_ONESHOT_START(leanTowardsObject);
      NmRsCBUDynamicBalancer* cbuDyn = (NmRsCBUDynamicBalancer*)character->getTask(bvid_dynamicBalancer);
      Assert(cbuDyn);
      if (start)
      cbuDyn->autoLeanToObject(ARG_INT(instanceIndex), ARG_INT(boundIndex), ARG_VECTOR3(offset), ARG_FLOAT(leanAmount));
      else
        cbuDyn->autoLeanCancel();

    }
    void hipsLeanInDirectionUpdate(const MessageParamsBase* const params, NmRsCharacter *character)
    {
      GET_ONESHOT_START(hipsLeanInDirection);
      NmRsCBUDynamicBalancer* cbuDyn = (NmRsCBUDynamicBalancer*)character->getTask(bvid_dynamicBalancer);
      Assert(cbuDyn);
      if (start)
      cbuDyn->autoLeanHipsInDirection(ARG_VECTOR3(dir), ARG_FLOAT(leanAmount));
      else
        cbuDyn->autoLeanHipsCancel();
    }
    void hipsLeanRandomUpdate(const MessageParamsBase* const params, NmRsCharacter *character)
    {
      GET_ONESHOT_START(hipsLeanRandom);
      NmRsCBUDynamicBalancer* cbuDyn = (NmRsCBUDynamicBalancer*)character->getTask(bvid_dynamicBalancer);
      Assert(cbuDyn);
      if (start)
      cbuDyn->autoLeanHipsRandom(ARG_FLOAT(leanAmountMin), ARG_FLOAT(leanAmountMax),ARG_FLOAT(changeTimeMin),ARG_FLOAT(changeTimeMax));
      else
        cbuDyn->autoLeanHipsCancel();
    }
    void hipsLeanToPositionUpdate(const MessageParamsBase* const params, NmRsCharacter *character)
    {
      GET_ONESHOT_START(hipsLeanToPosition);
      NmRsCBUDynamicBalancer* cbuDyn = (NmRsCBUDynamicBalancer*)character->getTask(bvid_dynamicBalancer);
      Assert(cbuDyn);
      if (start)
      cbuDyn->autoLeanHipsToPosition(ARG_VECTOR3(pos), ARG_FLOAT(leanAmount));
      else
        cbuDyn->autoLeanHipsCancel();
    }
    void hipsLeanTowardsObjectUpdate(const MessageParamsBase* const params, NmRsCharacter *character)
    {
      GET_ONESHOT_START(hipsLeanTowardsObject);
      NmRsCBUDynamicBalancer* cbuDyn = (NmRsCBUDynamicBalancer*)character->getTask(bvid_dynamicBalancer);
      Assert(cbuDyn);
      if (start)
      cbuDyn->autoLeanHipsToObject(ARG_INT(instanceIndex), ARG_INT(boundIndex), ARG_VECTOR3(offset), ARG_FLOAT(leanAmount));
      else
        cbuDyn->autoLeanHipsCancel();

    }
    void forceLeanInDirectionUpdate(const MessageParamsBase* const params, NmRsCharacter *character)
    {
      GET_ONESHOT_START(forceLeanInDirection);
      NmRsCBUDynamicBalancer* cbuDyn = (NmRsCBUDynamicBalancer*)character->getTask(bvid_dynamicBalancer);
      Assert(cbuDyn);
      if (start)
        cbuDyn->autoLeanForceInDirection(ARG_VECTOR3(dir), ARG_FLOAT(leanAmount), ARG_INT(bodyPart));
      else
        cbuDyn->autoLeanForceCancel();
    }
    void forceLeanRandomUpdate(const MessageParamsBase* const params, NmRsCharacter *character)
    {
      GET_ONESHOT_START(forceLeanRandom);
      NmRsCBUDynamicBalancer* cbuDyn = (NmRsCBUDynamicBalancer*)character->getTask(bvid_dynamicBalancer);
      Assert(cbuDyn);
      if (start)
        cbuDyn->autoLeanForceRandom(ARG_FLOAT(leanAmountMin), ARG_FLOAT(leanAmountMax),ARG_FLOAT(changeTimeMin),ARG_FLOAT(changeTimeMax), ARG_INT(bodyPart));
      else
        cbuDyn->autoLeanForceCancel();
    }
    void forceLeanToPositionUpdate(const MessageParamsBase* const params, NmRsCharacter *character)
    {
      GET_ONESHOT_START(forceLeanToPosition);
      NmRsCBUDynamicBalancer* cbuDyn = (NmRsCBUDynamicBalancer*)character->getTask(bvid_dynamicBalancer);
      Assert(cbuDyn);
      if (start)
        cbuDyn->autoLeanForceToPosition(ARG_VECTOR3(pos), ARG_FLOAT(leanAmount), ARG_INT(bodyPart));
      else
        cbuDyn->autoLeanForceCancel();
    }
    void forceLeanTowardsObjectUpdate(const MessageParamsBase* const params, NmRsCharacter *character)
    {
      GET_ONESHOT_START(forceLeanTowardsObject);
      NmRsCBUDynamicBalancer* cbuDyn = (NmRsCBUDynamicBalancer*)character->getTask(bvid_dynamicBalancer);
      Assert(cbuDyn);
      if (start)
        cbuDyn->autoLeanForceToObject(ARG_INT(instanceIndex), ARG_INT(boundIndex), ARG_VECTOR3(offset), ARG_FLOAT(leanAmount), ARG_INT(bodyPart));
      else
        cbuDyn->autoLeanForceCancel();

    }
    void stayUprightUpdate(const MessageParamsBase* const params, NmRsCharacter *character)
    {
      GET_ONESHOT_START(stayUpright);
      character->m_uprightConstraint.forceActive = ARG_BOOL(useForces) && start;//sent as normal message and want stayUpright to stop when start=false
      character->m_uprightConstraint.torqueActive = ARG_BOOL(useTorques) && start;//sent as normal message and want stayUpright to stop when start=false
      character->m_uprightConstraint.lastStandMode = ARG_BOOL(lastStandMode) && start;//sent as normal message and want stayUpright to stop when start=false
      character->m_uprightConstraint.lastStandSinkRate = ARG_FLOAT(lastStandSinkRate);
      character->m_uprightConstraint.lastStandHorizDamping = ARG_FLOAT(lastStandHorizDamping);
      character->m_uprightConstraint.lastStandMaxTime = ARG_FLOAT(lastStandMaxTime);
      character->m_uprightConstraint.turnTowardsBullets = ARG_BOOL(turnTowardsBullets);
      character->m_uprightConstraint.velocityBased = ARG_BOOL(velocityBased);
      character->m_uprightConstraint.torqueOnlyInAir = ARG_BOOL(torqueOnlyInAir);
      character->m_uprightConstraint.forceStrength = ARG_FLOAT(forceStrength);
      character->m_uprightConstraint.forceDamping = ARG_FLOAT(forceDamping);
      character->m_uprightConstraint.forceFeetMult = ARG_FLOAT(forceFeetMult);  
      character->m_uprightConstraint.forceSpine3Share = ARG_FLOAT(forceSpine3Share); 
      character->m_uprightConstraint.forceLeanReduction = ARG_FLOAT(forceLeanReduction);        
      character->m_uprightConstraint.forceInAirShare = ARG_FLOAT(forceInAirShare);
      character->m_uprightConstraint.forceMin = ARG_FLOAT(forceMin);
      character->m_uprightConstraint.forceMax = ARG_FLOAT(forceMax);
      character->m_uprightConstraint.forceSaturationVel = ARG_FLOAT(forceSaturationVel);
      character->m_uprightConstraint.forceThresholdVel = ARG_FLOAT(forceThresholdVel);
      character->m_uprightConstraint.torqueStrength = ARG_FLOAT(torqueStrength);
      character->m_uprightConstraint.torqueDamping = ARG_FLOAT(torqueDamping);   
      character->m_uprightConstraint.torqueSaturationVel = ARG_FLOAT(torqueSaturationVel);
      character->m_uprightConstraint.torqueThresholdVel = ARG_FLOAT(torqueThresholdVel);
      character->m_uprightConstraint.supportPosition = ARG_FLOAT(supportPosition);   
      character->m_uprightConstraint.noSupportForceMult = ARG_FLOAT(noSupportForceMult);   
      character->m_uprightConstraint.stepUpHelp = ARG_FLOAT(stepUpHelp);     
      character->m_uprightConstraint.stayUpAcc = ARG_FLOAT(stayUpAcc);
      character->m_uprightConstraint.stayUpAccMax = ARG_FLOAT(stayUpAccMax);
    }
    void activePoseUpdate(const MessageParamsBase* const params, NmRsCharacter *character)
    {
      GET_ONESHOT_START(activePose);

#if ART_ENABLE_BSPY
      character->setCurrentSubBehaviour("-MActivePose");
#endif
	  NmRsHumanBody *body = character->getBody();
      Assert(body);
      body->setup(bvid_DirectInvoke, priority, 0, blend, mask DEBUG_LIMBS_PARAMETER("activePose"));

      BehaviourMask bitMask = character->expressionToMask(ARG_STRING(mask));
      int animSource = ARG_INT(animSource);
      body->activePose(animSource, bitMask);

      if (ARG_BOOL(useGravityCompensation))
        body->setOpposeGravity(1.0f, bitMask);

      body->postLimbInputs();

#if ART_ENABLE_BSPY
      character->setCurrentSubBehaviour("");
#endif
    }
    void setCharacterStrengthUpdate(const MessageParamsBase* const params, NmRsCharacter *character)
    {
      GET_ONESHOT_START(setCharacterStrength);
      character->m_strength = ARG_FLOAT(characterStrength);
    }
    void setFallingReactionUpdate(const MessageParamsBase* const params, NmRsCharacter *character)
    {
      GET_ONESHOT_START(setFallingReaction);
      NmRsCBUCatchFall* catchFallTask = (NmRsCBUCatchFall*)character->getTask(bvid_catchFall);
      Assert(catchFallTask);

      catchFallTask->m_handsAndKnees = ARG_BOOL(handsAndKnees);
      catchFallTask->m_callRDS = ARG_BOOL(callRDS);
      catchFallTask->m_comVelRDSThresh = ARG_FLOAT(comVelRDSThresh);
      catchFallTask->m_resistRolling = ARG_BOOL(resistRolling);
      catchFallTask->m_armReduceSpeed = ARG_FLOAT(armReduceSpeed);
#if 0 // avoiding integrating new catch fall stuff for the moment.
      catchFallTask->applyReachLengthMultiplier(ARG_FLOAT(reachLengthMultiplier)); // Sets reachLengthMultiplier and update reach length and the probe length.
#endif
      catchFallTask->m_inhibitRollingTime = ARG_FLOAT(inhibitRollingTime);
      catchFallTask->m_changeFrictionTime = ARG_FLOAT(changeFrictionTime);   
      catchFallTask->m_groundFriction = ARG_FLOAT(groundFriction);
      character->m_minImpactFriction = ARG_FLOAT(frictionMin);
      character->m_maxImpactFriction = ARG_FLOAT(frictionMax);
    }
    void setCharacterUnderwaterUpdate(const MessageParamsBase* const params, NmRsCharacter *character)
    {
      GET_ONESHOT_START(setCharacterUnderwater);
      character->setUnderwater(ARG_BOOL(underwater));
      character->setViscosity(ARG_FLOAT(viscosity));
      character->setGravityFactor(ARG_FLOAT(gravityFactor));
      character->setStroke(ARG_FLOAT(stroke));
      character->setLinearStroke(ARG_BOOL(linearStroke));
    }
#if NM_EA
    void addPatchUpdate(const MessageParamsBase* const params, NmRsCharacter *character)
    {
      GET_ONESHOT_START(addPatch);
      //MMMMtodo don't like this initialization done here (XBox build complain otherwise)
      int geomType = 1;
      int instanceIndex = -1;
      int boundIndex = 0;
      int action = 0;
      rage::Vector3 corner,faceNormal0, faceNormal1, faceNormal2;
      rage::Vector3 edgeLengths;
      float edgeRadius = 0.f;
      bool localVectors = true;

      APPLY_PARAMETER(geomType, geomType); 
      APPLY_PARAMETER(instanceIndex, instanceIndex); 
      APPLY_PARAMETER(boundIndex, boundIndex); 
      APPLY_PARAMETER(action, action); 
      APPLY_PARAMETER(edgeLengths, edgeLengths); 
      APPLY_PARAMETER(edgeRadius, edgeRadius); 
      APPLY_PARAMETER(localVectors, localVectors); 
      APPLY_PARAMETER(corner, corner); 
      APPLY_PARAMETER(faceNormal0, faceNormal0); 
      APPLY_PARAMETER(faceNormal1, faceNormal1); 
      APPLY_PARAMETER(faceNormal2, faceNormal2); 

      character->Patch_Add(geomType, action, instanceIndex, boundIndex, corner, faceNormal0, faceNormal1, faceNormal2, edgeLengths, edgeRadius, localVectors);
    }
#endif//#if NM_EA
    void applyBulletImpulseUpdate(const MessageParamsBase* const params, NmRsCharacter *character)
    {
      GET_ONESHOT_START(applyBulletImpulse);
      int partIndex = ARG_INT(partIndex);
      Assert(partIndex < character->getNumberOfParts());
      if (partIndex > character->getNumberOfParts() - 1)
        return;//Don't apply the force if the bodyPart is out of range

      NmRsGenericPart* part = character->getGenericPartByIndex(partIndex);//Returns part 0 if not in correct range
      Assert(part);
      if(!part)
        return;


      float equalize = ARG_FLOAT(equalizeAmount);
      rage::Vector3 loc = ARG_VECTOR3(hitPoint);
      if (!ARG_SET(hitPoint))
        loc = part->getPosition();

      bool useLocalHitInfo = ARG_BOOL(localHitPointInfo);
      if (useLocalHitInfo)
      {
        rage::Vector3 localPos = loc;
        rage::Matrix34 mat;
        part->getMatrix(mat);
        mat.Transform(localPos, loc);
      }

      character->m_currentBulletApplier ++;
      if (character->m_currentBulletApplier >= NUM_OF_BULLETS)
        character->m_currentBulletApplier = 0;
      character->m_bulletApplier[character->m_currentBulletApplier].newHit(partIndex, ARG_VECTOR3(impulse), loc, equalize);
    }
    void applyImpulseUpdate(const MessageParamsBase* const params, NmRsCharacter *character)
    {
      GET_ONESHOT_START(applyImpulse);

      int partIndex = ARG_INT(partIndex);
      Assert(partIndex < character->getNumberOfParts());
      if (partIndex > character->getNumberOfParts() - 1)
        return;//Don't apply the force if the bodyPart is out of range

      NmRsGenericPart* part = character->getGenericPartByIndex(partIndex);//Returns part 0 if not in correct range
      Assert(part);
      if(!part)
        return;

      float equalize = ARG_FLOAT(equalizeAmount);
      float averageMass = character->getTotalMass() / character->getNumberOfEffectors();
      float massProp = getPartMass(character->getArticulatedBody()->GetLink(partIndex)) / averageMass;
      float scale = massProp * equalize + 1 * (1.0f - equalize);

      rage::Vector3 loc = ARG_VECTOR3(hitPoint);
      rage::Vector3 imp = ARG_VECTOR3(impulse);

      bool useLocalHitInfo = ARG_BOOL(localHitPointInfo);
      bool useImpulseInfo = ARG_BOOL(localImpulseInfo);
      if (useLocalHitInfo || useImpulseInfo)
      {
        rage::Matrix34 mat;
        part->getMatrix(mat);
        if (useLocalHitInfo)
        {
          rage::Vector3 localPos = loc;
          mat.Transform(localPos, loc);
        }
        if (useImpulseInfo)
        {
          rage::Vector3 localImpulse = imp;
          mat.Transform3x3(localImpulse, imp);
        }
      }

      if (!ARG_SET(hitPoint))
        loc = part->getPosition();

      bool treatAsAngularImpulse = ARG_BOOL(angularImpulse);

      if (treatAsAngularImpulse)
      {
        part->applyTorqueImpulse(imp * scale);
      }
      else
      part->applyImpulse(imp * scale, loc);
    }

    void bodyRelaxUpdate(const MessageParamsBase* const params, NmRsCharacter *character)
    {
      GET_ONESHOT_START(bodyRelax);
#if ART_ENABLE_BSPY
      character->setCurrentSubBehaviour("-MRelax");
#endif

      NmRsHumanBody *body = character->getBody();
      Assert(body);
      body->setup(bvid_DirectInvoke, priority, 0, blend, mask DEBUG_LIMBS_PARAMETER("bodyRelax"));

      bool doHoldPose = ARG_BOOL(holdPose);
      float mult = (100.0f - ARG_FLOAT(relaxation)) / 100.0f;

      float multDamping = ARG_FLOAT(damping);
      bool useDamping = ARG_SET(damping);
      BehaviourMask bitMask = character->expressionToMask(ARG_STRING(mask));
      bool bUseMask = ARG_SET(mask);

      if(!bUseMask)
      {
        bitMask = bvmask_Full;
      }

      if (doHoldPose)
      {
        body->holdPose(bitMask);
      }
      
      if(useDamping)
      {
        body->setRelaxation(mult, bitMask, &multDamping);
      }
      else
      {
        body->setRelaxation(mult, bitMask);
      }
      
	  body->postLimbInputs();

#if ART_ENABLE_BSPY
      character->setCurrentSubBehaviour(""); 
#endif
    }

    void configureBalanceUpdate(const MessageParamsBase* const params, NmRsCharacter *character)
    {
      GET_ONESHOT_START(configureBalance);
#if ART_ENABLE_BSPY
      character->setCurrentSubBehaviour("-MCBalance"); 
#endif
      NmRsCBUDynamicBalancer* cbuDyn = (NmRsCBUDynamicBalancer*)character->getTask(bvid_dynamicBalancer);
      Assert(cbuDyn);

      if (ARG_SET(stepHeight))
        cbuDyn->setStepHeight(ARG_FLOAT(stepHeight));
#if NM_STEP_UP
      if (ARG_SET(stepHeightInc4Step))
        cbuDyn->setStepHeightInc4Step(ARG_FLOAT(stepHeightInc4Step)); 
#endif //#if NM_STEP_UP
      if (ARG_SET(legsApartRestep))
        cbuDyn->setLegsApartRestep(ARG_FLOAT(legsApartRestep));
      if (ARG_SET(legsTogetherRestep))
        cbuDyn->setLegsTogetherRestep(ARG_FLOAT(legsTogetherRestep));
      if (ARG_SET(legsApartMax))
        cbuDyn->setLegsApartMax(ARG_FLOAT(legsApartMax));
      if (ARG_SET(taperKneeStrength))
        cbuDyn->taperKneeStrength(ARG_BOOL(taperKneeStrength));
      if (ARG_SET(legStiffness))
      {
        cbuDyn->setLeftLegStiffness(ARG_FLOAT(legStiffness));
        cbuDyn->setRightLegStiffness(ARG_FLOAT(legStiffness));
        if(cbuDyn->isActive())
        {
          NmRsHumanBody *body = character->getBody();
          Assert(body);
          body->setup(bvid_DirectInvoke, priority, 0, blend, mask DEBUG_LIMBS_PARAMETER("configureBalance"));

		  cbuDyn->calibrateLowerBodyEffectors(body); // so stiffness can change each frame
          body->postLimbInputs();
        }
      }
      if (ARG_SET(leftLegSwingDamping))
        cbuDyn->setLeftLegSwingDamping(ARG_FLOAT(leftLegSwingDamping));
      if (ARG_SET(rightLegSwingDamping))
        cbuDyn->setRightLegSwingDamping(ARG_FLOAT(rightLegSwingDamping));
      if (ARG_SET(opposeGravityLegs))
        cbuDyn->setOpposeGravityLegs(ARG_FLOAT(opposeGravityLegs));
      if (ARG_SET(opposeGravityAnkles))
        cbuDyn->setOpposeGravityAnkles(ARG_FLOAT(opposeGravityAnkles));

      if (ARG_SET(leanAcc))
        cbuDyn->setLeanAcc(ARG_FLOAT(leanAcc));
      if (ARG_SET(hipLeanAcc))
        cbuDyn->setHipLeanAcc(ARG_FLOAT(hipLeanAcc));
      if (ARG_SET(leanAccMax))
        cbuDyn->setLeanAccMax(ARG_FLOAT(leanAccMax));
      if (ARG_SET(resistAcc))
        cbuDyn->setResistAcc(ARG_FLOAT(resistAcc));
      if (ARG_SET(resistAccMax))
        cbuDyn->setResistAccMax(ARG_FLOAT(resistAccMax));
      if (ARG_SET(footSlipCompOnMovingFloor))
        cbuDyn->setFootSlipCompOnMovingFloor(ARG_BOOL(footSlipCompOnMovingFloor));

      if (ARG_SET(ankleEquilibrium))
        cbuDyn->setAnkleEquilibrium(ARG_FLOAT(ankleEquilibrium));
      if (ARG_SET(extraFeetApart))
        cbuDyn->setExtraFeetApart(ARG_FLOAT(extraFeetApart));

      if (ARG_SET(balanceAbortThreshold))
        cbuDyn->setGiveUpThreshold(ARG_FLOAT(balanceAbortThreshold));
      if (ARG_SET(giveUpHeight))
        cbuDyn->setGiveUpHeight(ARG_FLOAT(giveUpHeight));
      if (ARG_SET(stepClampScale))
        cbuDyn->setStepClampScale(ARG_FLOAT(stepClampScale));
      if (ARG_SET(stepClampScaleVariance))
        cbuDyn->setStepClampScaleVariance(ARG_FLOAT(stepClampScaleVariance));
      if (ARG_SET(predictionTime))
        cbuDyn->setBalanceTime(ARG_FLOAT(predictionTime));
      if (ARG_SET(predictionTimeHip))
        cbuDyn->setBalanceTimeHip(ARG_FLOAT(predictionTimeHip));
      if (ARG_SET(predictionTimeVariance))
        cbuDyn->setBalanceTimeVariance(ARG_FLOAT(predictionTimeVariance));
      if (ARG_SET(maxSteps))
        cbuDyn->setMaxSteps(ARG_INT(maxSteps));
      if (ARG_SET(extraSteps))
        cbuDyn->decrementSteps(ARG_INT(extraSteps));
      if (ARG_SET(extraTime))
        cbuDyn->decrementTime(ARG_FLOAT(extraTime));
      if (ARG_SET(fallType))
        cbuDyn->setFallType(ARG_INT(fallType));
      if (ARG_SET(fallMult))
        cbuDyn->setFallMult(ARG_FLOAT(fallMult));
      if (ARG_SET(maxBalanceTime))
        cbuDyn->setMaximumBalanceTime(ARG_FLOAT(maxBalanceTime));
      if (ARG_SET(failMustCollide))
        cbuDyn->setFailMustCollide(ARG_BOOL(failMustCollide));
      if (ARG_SET(ignoreFailure))
        cbuDyn->setIgnoreFailure(ARG_BOOL(ignoreFailure));
      if (ARG_SET(changeStepTime))
        cbuDyn->setChangeStepTime(ARG_FLOAT(changeStepTime));			
      if (ARG_SET(balanceIndefinitely ))
        cbuDyn->setBalanceIndefinitely(ARG_BOOL(balanceIndefinitely));
      if (ARG_SET(rampHipPitchOnFail ))
        cbuDyn->setRampHipPitchOnFail(ARG_BOOL(rampHipPitchOnFail));
      if (ARG_SET(stableLinSpeedThresh))
        cbuDyn->setStableSuccessMinimumLinSpeed(ARG_FLOAT(stableLinSpeedThresh));
      if (ARG_SET(stableRotSpeedThresh))
        cbuDyn->setStableSuccessMinimumRotSpeed(ARG_FLOAT(stableRotSpeedThresh));
      if (ARG_SET(movingFloor))
      {
        cbuDyn->setMovingFloor(ARG_BOOL(movingFloor));
        character->setMovingFloor(ARG_BOOL(movingFloor));
      }
      if (ARG_SET(airborneStep))
        cbuDyn->setAirborneStep(ARG_BOOL(airborneStep));  
      if (ARG_SET(useComDirTurnVelThresh))
        cbuDyn->setUseComDirTurnVelThresh(ARG_FLOAT(useComDirTurnVelThresh));
      if (ARG_SET(minKneeAngle))
        cbuDyn->setMinKneeAngle(ARG_FLOAT(minKneeAngle));
      if (ARG_SET(flatterSwingFeet))
        cbuDyn->setFlatterSwingFeet(ARG_BOOL(flatterSwingFeet));
      if (ARG_SET(flatterStaticFeet))
        cbuDyn->setFlatterStaticFeet(ARG_BOOL(flatterStaticFeet));
      if (ARG_SET(leanAgainstVelocity))
        cbuDyn->setLeanAgainstVelocity(ARG_FLOAT(leanAgainstVelocity));
      if (ARG_SET(stepDecisionThreshold))
        cbuDyn->setStepDecisionThreshold(ARG_FLOAT(stepDecisionThreshold));
      if (ARG_SET(stepIfInSupport))
        cbuDyn->setStepIfInSupport(ARG_BOOL(stepIfInSupport));
      if (ARG_SET(alwaysStepWithFarthest))
        cbuDyn->setAlwaysStepWithFarthest(ARG_BOOL(alwaysStepWithFarthest));
      if (ARG_SET(standUp))
        cbuDyn->setStandUp(ARG_BOOL(standUp));
      if (ARG_SET(depthFudge))
        character->m_depthFudge = ARG_FLOAT(depthFudge);
      if (ARG_SET(depthFudgeStagger))
        character->m_depthFudgeStagger = ARG_FLOAT(depthFudgeStagger);
      if (ARG_SET(footFriction))
        character->m_footFriction = ARG_FLOAT(footFriction);
      if (ARG_SET(footFrictionStagger))
        character->m_footFrictionStagger = ARG_FLOAT(footFrictionStagger);


#if DYNBAL_GIVEUP_RAMP
      if (ARG_SET(giveUpHeightEnd))
        cbuDyn->setGiveUpHeightEnd(ARG_FLOAT(giveUpHeightEnd));
      if (ARG_SET(balanceAbortThresholdEnd))
        cbuDyn->setGiveUpThresholdEnd(ARG_FLOAT(balanceAbortThresholdEnd));
      if (ARG_SET(giveUpRampDuration))
        cbuDyn->setGiveUpRampDuration(ARG_FLOAT(giveUpRampDuration));
      if (ARG_SET(leanToAbort))
        cbuDyn->setLeanToAbort(ARG_FLOAT(leanToAbort));
#endif
#if ART_ENABLE_BSPY
      character->setCurrentSubBehaviour(""); 
#endif
    }
    void configureBalanceResetUpdate(const MessageParamsBase* const params, NmRsCharacter *character)
    {
      GET_ONESHOT_START(configureBalanceReset);
#if ART_ENABLE_BSPY
      character->setCurrentSubBehaviour("-MResetBalance"); 
#endif
      NmRsCBUDynamicBalancer* cbuDyn = (NmRsCBUDynamicBalancer*)character->getTask(bvid_dynamicBalancer);
      Assert(cbuDyn);

      cbuDyn->setStepHeight(0.1f);
#if NM_STEP_UP
      cbuDyn->setStepHeightInc4Step(0.1f);
#endif //#if NM_STEP_UP
      cbuDyn->setLegsApartRestep(0.2f);
      cbuDyn->setLegsTogetherRestep(1.f);
      cbuDyn->setLegsApartMax(2.0f);
      cbuDyn->taperKneeStrength(true);
      cbuDyn->setLeftLegStiffness(12.0f);
      cbuDyn->setRightLegStiffness(12.0f);
      cbuDyn->setLeftLegSwingDamping(1.f);
      cbuDyn->setRightLegSwingDamping(1.f);
      cbuDyn->setOpposeGravityLegs(1.f);
      cbuDyn->setOpposeGravityAnkles(1.f);
      cbuDyn->setLeanAcc(0.0f);
      cbuDyn->setHipLeanAcc(0.5f);
      cbuDyn->setLeanAccMax(5.0f);
      cbuDyn->setResistAcc(0.5f);
      cbuDyn->setResistAccMax(3.0f);
      cbuDyn->setFootSlipCompOnMovingFloor(true);
      cbuDyn->setAnkleEquilibrium(0.f);
      cbuDyn->setExtraFeetApart(0.f);
      cbuDyn->setGiveUpThreshold(0.6f);
      if(cbuDyn->isActive())
      {
        NmRsHumanBody *body = character->getBody();
        Assert(body);
        body->setup(bvid_DirectInvoke, priority, 0, blend, mask DEBUG_LIMBS_PARAMETER("configureBalanceReset"));

        cbuDyn->calibrateLowerBodyEffectors(body); // so stiffness can change each frame
        body->postLimbInputs();
      }
      cbuDyn->setStepClampScale(1.f);
      cbuDyn->setStepClampScaleVariance(0.f);
      cbuDyn->setBalanceTime(0.2f);
      cbuDyn->setBalanceTimeHip(0.3f);
      cbuDyn->setBalanceTimeVariance(0.0f);
      cbuDyn->setMaxSteps(100);
      cbuDyn->setFallType(0);
      cbuDyn->setFallMult(1.f);
      cbuDyn->setMaximumBalanceTime(50.f);
      cbuDyn->setFailMustCollide(false);
      cbuDyn->setIgnoreFailure(false);
      cbuDyn->setChangeStepTime(-1.f);			
      cbuDyn->setBalanceIndefinitely(false);
      cbuDyn->setRampHipPitchOnFail(false);
      cbuDyn->setStableSuccessMinimumLinSpeed(0.25f);
      cbuDyn->setStableSuccessMinimumRotSpeed(0.25f);
      cbuDyn->setMovingFloor(false);
      character->setMovingFloor(false);
      cbuDyn->setAirborneStep(true);  
      cbuDyn->setUseComDirTurnVelThresh(0.f);
      cbuDyn->setMinKneeAngle(-0.5f);
      cbuDyn->setFlatterSwingFeet(false);
      cbuDyn->setFlatterStaticFeet(false);
      cbuDyn->setLeanAgainstVelocity(0.f);
      cbuDyn->setStepDecisionThreshold(0.f);
      cbuDyn->setStepIfInSupport(true);
      cbuDyn->setAlwaysStepWithFarthest(false);
      cbuDyn->setStandUp(true);
      character->m_depthFudge = 0.01f;
      character->m_depthFudgeStagger = 0.01f;
      character->m_footFriction = 1.0f;
      character->m_footFrictionStagger = 1.0f;
      //cbuDyn->setStepWithBoth(false);

#if DYNBAL_GIVEUP_RAMP
      cbuDyn->setGiveUpHeight(0.5f);
      cbuDyn->setGiveUpHeightEnd(0.5f);
      cbuDyn->setGiveUpThresholdEnd(0.6f);
      cbuDyn->setGiveUpRampDuration(-1.0f);
      cbuDyn->setLeanToAbort(0.6f);
#endif
#if ART_ENABLE_BSPY
      character->setCurrentSubBehaviour(""); 
#endif
    }
    void configureBulletsUpdate(const MessageParamsBase* const params, NmRsCharacter *character)
    {
      GET_ONESHOT_START(configureBullets);
      Assert(character);
      BulletApplier::s_nextSetup.impulsePeriod = ARG_FLOAT(impulsePeriod);
      BulletApplier::s_nextSetup.impulseTorqueScale = ARG_FLOAT(impulseTorqueScale);
      BulletApplier::s_nextSetup.impulseDelay = ARG_FLOAT(impulseDelay);
      BulletApplier::s_nextSetup.impulseSpreadOverParts = ARG_BOOL(impulseSpreadOverParts);  

      BulletApplier::s_nextSetup.torqueDelay = ARG_FLOAT(torqueDelay);
      BulletApplier::s_nextSetup.torqueGain = ARG_FLOAT(torqueGain);
      BulletApplier::s_nextSetup.torqueCutoff = ARG_FLOAT(torqueCutoff);
      BulletApplier::s_nextSetup.torquePeriod = ARG_FLOAT(torquePeriod);
      BulletApplier::s_nextSetup.torqueReductionPerTick = ARG_FLOAT(torqueReductionPerTick);
      BulletApplier::s_nextSetup.torqueMode = ARG_INT(torqueMode);
      BulletApplier::s_nextSetup.torqueSpinMode = ARG_INT(torqueSpinMode);
      BulletApplier::s_nextSetup.torqueFilterMode = ARG_INT(torqueFilterMode);
      BulletApplier::s_nextSetup.torqueAlwaysSpine3 = ARG_BOOL(torqueAlwaysSpine3);

      BulletApplier::s_nextSetup.liftGain = ARG_FLOAT(liftGain);

      BulletApplier::s_nextSetup.counterImpulseMag = ARG_FLOAT(counterImpulseMag);
      BulletApplier::s_nextSetup.counterImpulseDelay = ARG_FLOAT(counterImpulseDelay);
      BulletApplier::s_nextSetup.counterAfterMagReached = ARG_BOOL(counterAfterMagReached);
      BulletApplier::s_nextSetup.doCounterImpulse = ARG_BOOL(doCounterImpulse);
      BulletApplier::s_nextSetup.counterImpulse2Hips = ARG_FLOAT(counterImpulse2Hips);

      BulletApplier::s_nextSetup.impulseAirMult = ARG_FLOAT(impulseAirMult);
      BulletApplier::s_nextSetup.impulseAirMultStart = ARG_FLOAT(impulseAirMultStart);
      BulletApplier::s_nextSetup.impulseAirMax = ARG_FLOAT(impulseAirMax);
      BulletApplier::s_nextSetup.impulseAirApplyAbove = ARG_FLOAT(impulseAirApplyAbove);
      BulletApplier::s_nextSetup.impulseAirOn = ARG_BOOL(impulseAirOn);

#if NM_ONE_LEG_BULLET
      BulletApplier::s_nextSetup.impulseOneLegMult = ARG_FLOAT(impulseOneLegMult);
      BulletApplier::s_nextSetup.impulseOneLegMultStart = ARG_FLOAT(impulseOneLegMultStart);
      BulletApplier::s_nextSetup.impulseOneLegMax = ARG_FLOAT(impulseOneLegMax);
      BulletApplier::s_nextSetup.impulseOneLegApplyAbove = ARG_FLOAT(impulseOneLegApplyAbove);
      BulletApplier::s_nextSetup.impulseOneLegOn = ARG_BOOL(impulseOneLegOn);
#endif//#if NM_ONE_LEG_BULLET
      BulletApplier::s_nextSetup.loosenessFix = ARG_BOOL(loosenessFix);
#if NM_RIGID_BODY_BULLET
      //BulletApplier::s_nextSetup.rbForce = ARG_FLOAT(rbForce);
      BulletApplier::s_nextSetup.rbRatio = ARG_FLOAT(rbRatio);
      BulletApplier::s_nextSetup.rbLowerShare = ARG_FLOAT(rbLowerShare);     
      BulletApplier::s_nextSetup.rbMoment = ARG_FLOAT(rbMoment);
      BulletApplier::s_nextSetup.rbMaxTwistMomentArm = ARG_FLOAT(rbMaxTwistMomentArm);
      BulletApplier::s_nextSetup.rbMaxBroomMomentArm = ARG_FLOAT(rbMaxBroomMomentArm);
      BulletApplier::s_nextSetup.rbRatioAirborne = ARG_FLOAT(rbRatioAirborne);
      BulletApplier::s_nextSetup.rbMomentAirborne = ARG_FLOAT(rbMomentAirborne);
      BulletApplier::s_nextSetup.rbMaxTwistMomentArmAirborne = ARG_FLOAT(rbMaxTwistMomentArmAirborne);
      BulletApplier::s_nextSetup.rbMaxBroomMomentArmAirborne = ARG_FLOAT(rbMaxBroomMomentArmAirborne);
      BulletApplier::s_nextSetup.rbRatioOneLeg = ARG_FLOAT(rbRatioOneLeg);
      BulletApplier::s_nextSetup.rbMomentOneLeg = ARG_FLOAT(rbMomentOneLeg);
      BulletApplier::s_nextSetup.rbMaxTwistMomentArmOneLeg = ARG_FLOAT(rbMaxTwistMomentArmOneLeg);
      BulletApplier::s_nextSetup.rbMaxBroomMomentArmOneLeg = ARG_FLOAT(rbMaxBroomMomentArmOneLeg);
      BulletApplier::s_nextSetup.rbTwistAxis = ARG_INT(rbTwistAxis);
      BulletApplier::s_nextSetup.rbPivot = ARG_BOOL(rbPivot);
#endif
      character->m_impulseReductionPerShot = ARG_FLOAT(impulseReductionPerShot);
      character->m_impulseRecovery = ARG_FLOAT(impulseRecovery);
      character->m_impulseLeakageStrengthScaled = ARG_BOOL(impulseLeakageStrengthScaled);
    }

#if NM_USE_IK_SELF_AVOIDANCE
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    // BBDD Self avoidance tech.
    void configureSelfAvoidanceUpdate(const MessageParamsBase* const params, NmRsCharacter *character)
    {
      GET_ONESHOT_START(configureSelfAvoidance);
      Assert(character);
      character->m_selfAvoidance.useSelfAvoidance = ARG_BOOL(useSelfAvoidance);
      character->m_selfAvoidance.overwriteDragReduction = ARG_BOOL(overwriteDragReduction);
      character->m_selfAvoidance.m_selfAvoidanceParams.torsoSwingFraction = ARG_FLOAT(torsoSwingFraction);
      character->m_selfAvoidance.m_selfAvoidanceParams.maxTorsoSwingAngleRad = ARG_FLOAT(maxTorsoSwingAngleRad);
      character->m_selfAvoidance.m_selfAvoidanceParams.selfAvoidIfInSpineBoundsOnly = ARG_BOOL(selfAvoidIfInSpineBoundsOnly);
      character->m_selfAvoidance.m_selfAvoidanceParams.selfAvoidAmount = ARG_FLOAT(selfAvoidAmount);
      character->m_selfAvoidance.m_selfAvoidanceParams.overwriteTwist = ARG_BOOL(overwriteTwist);
      character->m_selfAvoidance.usePolarPathAlgorithm = ARG_BOOL(usePolarPathAlgorithm);
      character->m_selfAvoidance.m_polarSelfAvoidanceParams.radius = ARG_FLOAT(radius);
    }
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
#endif //NM_USE_IK_SELF_AVOIDANCE

  void configureShotInjuredArmUpdate(const MessageParamsBase* const params, NmRsCharacter *character)
  {
    GET_ONESHOT_START(configureShotInjuredArm);
    NmRsCBUShot* shotTask = (NmRsCBUShot*)character->getTask(bvid_shot);
    Assert(shotTask);

    shotTask->m_injuredArm.injuredArmTime = ARG_FLOAT(injuredArmTime);
    shotTask->m_injuredArm.hipYaw = ARG_FLOAT(hipYaw);
    shotTask->m_injuredArm.hipRoll = ARG_FLOAT(hipRoll);
    shotTask->m_injuredArm.forceStepExtraHeight = ARG_FLOAT(forceStepExtraHeight);
    shotTask->m_injuredArm.shrugTime = ARG_FLOAT(shrugTime);  
    shotTask->m_injuredArm.forceStep = ARG_BOOL(forceStep);
    shotTask->m_injuredArm.stepTurn = ARG_BOOL(stepTurn);

    shotTask->m_injuredArm.velMultiplierStart = ARG_FLOAT(velMultiplierStart);
    shotTask->m_injuredArm.velMultiplierEnd = ARG_FLOAT(velMultiplierEnd);
    shotTask->m_injuredArm.velForceStep = ARG_FLOAT(velForceStep);
    shotTask->m_injuredArm.velStepTurn = ARG_FLOAT(velStepTurn);
    shotTask->m_injuredArm.velScales = ARG_BOOL(velScales);
  }
  void configureShotInjuredLegUpdate(const MessageParamsBase* const params, NmRsCharacter *character)
  {
    GET_ONESHOT_START(configureShotInjuredLeg);
    NmRsCBUShot* shotTask = (NmRsCBUShot*)character->getTask(bvid_shot);
    Assert(shotTask);
    APPLY_PARAMETER(timeBeforeCollapseWoundLeg, shotTask->m_parameters.timeBeforeCollapseWoundLeg);
    APPLY_PARAMETER(legInjuryTime, shotTask->m_parameters.legInjuryTime);
    APPLY_PARAMETER(legForceStep, shotTask->m_parameters.legForceStep);
    APPLY_PARAMETER(legLimpBend, shotTask->m_parameters.legLimpBend);
    APPLY_PARAMETER(legLiftTime, shotTask->m_parameters.legLiftTime);
    APPLY_PARAMETER(legInjury, shotTask->m_parameters.legInjury);       
    APPLY_PARAMETER(legInjuryLiftHipPitch, shotTask->m_parameters.legInjuryLiftHipPitch);       
    APPLY_PARAMETER(legInjuryHipPitch, shotTask->m_parameters.legInjuryHipPitch);       
    APPLY_PARAMETER(legInjuryLiftSpineBend, shotTask->m_parameters.legInjuryLiftSpineBend);       
    APPLY_PARAMETER(legInjurySpineBend, shotTask->m_parameters.legInjurySpineBend);       
    }

#if NM_RUNTIME_LIMITS
    void configureLimitsUpdate(const MessageParamsBase* const params, NmRsCharacter* character)
    {
      GET_ONESHOT_START(configureLimits);
      Assert(character);

      int index = ARG_INT(index);
      if(index > -1) // effector has been specified, operate on this effector only
      {
        NmRsEffectorBase* effector = character->getEffectorDirect(index);
        Assert(effector);
        if(!ARG_BOOL(enable))
          effector->disableLimits();
        else if(ARG_BOOL(restore))
          effector->restoreLimits();
        else if(ARG_BOOL(toDesired))
          effector->setLimitsToPose();
        else if(ARG_BOOL(toCurAnimation))
          effector->setLimitsToPose(true,ARG_FLOAT(margin));
        else
          if(effector->is3DofEffector())
            ((NmRs3DofEffector*)effector)->setLimits(ARG_FLOAT(lean1), ARG_FLOAT(lean2),ARG_FLOAT(twist));
          else
            ((NmRs1DofEffector*)effector)->setLimits(ARG_FLOAT(twist), ARG_FLOAT(lean1)); // todo: think of sensible scheme for parameter naming
      }
      else // no effector specified, process mask instead
      {
        // we don't have multiple arg masked effector support
        // so this will be done manually
        int i;
        NmRsEffectorBase* effector = NULL;
        BehaviourMask bitMask = character->expressionToMask(ARG_STRING(mask));
        for(i = 0; i < character->getNumberOfEffectors(); ++i)
        {
          character->isEffectorInMask(bitMask, i);
          effector = character->getEffectorDirect(i);
          if(!ARG_BOOL(enable))
            effector->disableLimits();
          else if(ARG_BOOL(restore))
            effector->restoreLimits();
          else if(ARG_BOOL(toDesired))
            effector->setLimitsToPose();
          else if(ARG_BOOL(toCurAnimation))
            effector->setLimitsToPose(true,ARG_FLOAT(margin));
          else
            if(effector->is3DofEffector())
              ((NmRs3DofEffector*)effector)->setLimits(ARG_FLOAT(lean1), ARG_FLOAT(lean2), ARG_FLOAT(twist));
            else
              ((NmRs1DofEffector*)effector)->setLimits(ARG_FLOAT(twist), ARG_FLOAT(lean1));
        }
      }
#else
    void configureLimitsUpdate(const MessageParamsBase* const /* params */, NmRsCharacter* /* character */)
    {
      Assert(false);
#endif
    }
    void defineAttachedObjectUpdate(const MessageParamsBase* const params, NmRsCharacter *character)
    {
      GET_ONESHOT_START(defineAttachedObject);
      character->m_attachedObject.partIndex = ARG_INT(partIndex);
      character->m_attachedObject.worldCOMPos = ARG_VECTOR3(worldPos);
      character->m_attachedObject.mass = ARG_FLOAT(objectMass);
	  //mmmmtodo add a levelIndex param so that this autoUpdates?
	  //also the worldCOMPos will be set to zero in preStep() and mass set ot 0 because of
	  // updateAttachedObject and the levelIndex being -1 (this message will have to come in after the preStep() but before the behaviours tick to have an effect)
    }
    void forceToBodyPartUpdate(const MessageParamsBase* const params, NmRsCharacter *character)
    {
      GET_ONESHOT_START(forceToBodyPart);

      int partIndex = ARG_INT(partIndex);
      Assert(partIndex < character->getNumberOfParts());
      if (partIndex > character->getNumberOfParts() - 1)
        return;//Don't apply the force if the bodyPart is out of range

      NmRsGenericPart* part = character->getGenericPartByIndex(partIndex);//Returns part 0 if not in correct range
      Assert(part);
      if(!part)
        return;

      if (ARG_BOOL(forceDefinedInPartSpace))
      {
        rage::Vector3 forceV = ARG_VECTOR3(force);
        rage::Matrix34 boundMat;
        part->getBoundMatrix(&boundMat);
        forceV.Dot(forceV,boundMat);
        part->applyForce(forceV);

        // [jrp] cache impulse in shot behaviour for debug purposes
        NmRsCBUShot *shotTask = (NmRsCBUShot *)character->getTask(bvid_shot);
        if(shotTask)
          shotTask->m_lastForce.Set(forceV);
      }
      else
      {
        part->applyForce(ARG_VECTOR3(force));

        // [jrp] cache impulse in shot behaviour for debug purposes
        NmRsCBUShot *shotTask = (NmRsCBUShot *)character->getTask(bvid_shot);
        if(shotTask)
          shotTask->m_lastForce.Set(ARG_VECTOR3(force));
      }
    }

    void setStiffnessUpdate(const MessageParamsBase* const params, NmRsCharacter *character)
    {
      GET_ONESHOT_START(setStiffness);
#if ART_ENABLE_BSPY
      character->setCurrentSubBehaviour("-MsetStiff");
#endif

      NmRsHumanBody *body = character->getBody();
      Assert(body);
      body->setup(bvid_DirectInvoke, priority, 0, blend, mask DEBUG_LIMBS_PARAMETER("setStiffness"));

      BehaviourMask bitMask = character->expressionToMask(ARG_STRING(mask));

      character->m_rememberSetStiffness = true;
      character->m_rememberStiff = ARG_FLOAT(bodyStiffness);
      character->m_rememberDamp = ARG_FLOAT(damping);
      character->m_rememberStiffnessMask = bitMask;
      character->m_rememberStiffnessPriority = priority;
      character->m_rememberStiffnessBlend = blend;

      body->setStiffness(ARG_FLOAT(bodyStiffness), ARG_FLOAT(damping), bitMask, NULL, true);

      body->postLimbInputs();
    }

    void setMuscleStiffnessUpdate(const MessageParamsBase* const params, NmRsCharacter *character)
    {
      GET_ONESHOT_START(setMuscleStiffness);
#if ART_ENABLE_BSPY
      character->setCurrentSubBehaviour("-MsetMStiff");
#endif

      NmRsHumanBody *body = character->getBody();
      Assert(body);
      body->setup(bvid_DirectInvoke, priority, 0, blend, mask DEBUG_LIMBS_PARAMETER("setMuscleStiffness"));

      float muscleStiffness = ARG_FLOAT(muscleStiffness);
      BehaviourMask bitMask = character->expressionToMask(ARG_STRING(mask));

      character->m_rememberSetMuscleStiffness = true;
      character->m_rememberMuscleStiff = muscleStiffness;
      character->m_rememberMuscleStiffnessMask = bitMask;
      character->m_rememberMuscleStiffnessPriority = priority;
	  character->m_rememberMuscleStiffnessBlend = blend;

      body->callMaskedEffectorDataFunctionFloatArg(
        bitMask,
        muscleStiffness,
        &NmRs1DofEffectorInputWrapper::setMuscleStiffness,
        &NmRs3DofEffectorInputWrapper::setMuscleStiffness);

      body->postLimbInputs();

#if ART_ENABLE_BSPY
      character->setCurrentSubBehaviour(""); 
#endif
    }
    void setWeaponModeUpdate(const MessageParamsBase* const params, NmRsCharacter *character)
    {
      GET_ONESHOT_START(setWeaponMode);
      character->setWeaponMode(ARG_INT(weaponMode));
    }
    void registerWeaponUpdate(const MessageParamsBase* const params, NmRsCharacter *character)
    {
      GET_ONESHOT_START(registerWeapon);
      //if new message
      if (ARG_SET(gunToHandA) && ARG_SET(gunToHandB) && ARG_SET(gunToHandC) && ARG_SET(gunToHandD) && ARG_SET(gunToMuzzleInGun) && ARG_SET(gunToButtInGun) )
      {
        rage::Matrix34 gun2Hand;
        gun2Hand.a = ARG_VECTOR3(gunToHandA);
        gun2Hand.b = ARG_VECTOR3(gunToHandB);
        gun2Hand.c = ARG_VECTOR3(gunToHandC);
        gun2Hand.d = ARG_VECTOR3(gunToHandD);
        rage::phConstraintHandle constrHandle;// = (rage::phConstraintHandle*) ARG_INT(constraintHandle);
        rage::Vector3 vecGunToMuzzleInGun = ARG_VECTOR3(gunToMuzzleInGun);
        rage::Vector3 vecGunToButtInGun = ARG_VECTOR3(gunToButtInGun);
        character->registerWeapon(ARG_INT(hand), ARG_INT(levelIndex), &constrHandle, gun2Hand, vecGunToMuzzleInGun, vecGunToButtInGun);
        //if (ARG_SET(gunToHandConstraint))
        //  gunToHandConstraint = (rage::phConstraintHandle *) ARG_POINTER(gunToHandConstraint);
      }
      else//old message
        character->registerWeapon(ARG_INT(hand), ARG_INT(levelIndex), 0.f, 0.f);//mmmm1todo get rid of this when North not using
    }
    void shotRelaxUpdate(const MessageParamsBase* const params, NmRsCharacter *character)
    {
      GET_ONESHOT_START(shotRelax);
      NmRsCBUShot* cbuShot = (NmRsCBUShot*)character->getTask(bvid_shot);
      Assert(cbuShot);
      cbuShot->setRelaxPeriodUpper(ARG_FLOAT(relaxPeriodUpper));
      cbuShot->setRelaxPeriodLower(ARG_FLOAT(relaxPeriodLower));
    }
    void fireWeaponUpdate(const MessageParamsBase* const params, NmRsCharacter *character)
    {
      GET_ONESHOT_START(fireWeapon);
      rage::Vector3 vDirection = ARG_VECTOR3(direction);
      if(vDirection.Mag2() != 1.f)
        vDirection.Normalize();

      // notify point gun that a weapon has fired
      NmRsCBUPointGun* cbuPointGun = (NmRsCBUPointGun*)character->getTask(bvid_pointGun);
      Assert(cbuPointGun);
      if(cbuPointGun->isActive())
      {
        NmRsHumanBody *body = character->getBody();
        Assert(body);
        body->setup(bvid_DirectInvoke, priority, 0, blend, mask DEBUG_LIMBS_PARAMETER("fireWeapon"));

        cbuPointGun->fireWeapon(*body, ARG_INT(gunHandEnum), ARG_FLOAT(firedWeaponStrength), ARG_BOOL(applyFireGunForceAtClavicle), ARG_FLOAT(inhibitTime), vDirection, ARG_FLOAT(split));
        body->postLimbInputs();
      }
      else
      {
        // if we are not pointing, character must handle the recoil.
        //character->fireGun(ARG_INT(gunHandEnum), ARG_FLOAT(firedWeaponStrength), ARG_BOOL(applyFireGunForceAtClavicle), ARG_FLOAT(inhibitTime), vDirection, ARG_FLOAT(split));
      }

    }

    void configureConstraintsUpdate(const MessageParamsBase* const params, NmRsCharacter *character)
    {
      GET_ONESHOT_START(configureConstraints);
      character->handCuffs = ARG_BOOL(handCuffs);
      character->handCuffsBehindBack = ARG_BOOL(handCuffsBehindBack);
      character->legCuffs = ARG_BOOL(legCuffs);
      character->rightDominant = ARG_BOOL(rightDominant);
      character->passiveMode = ARG_INT(passiveMode);
      character->bespokeBehaviour = ARG_BOOL(bespokeBehaviour);
      character->blend2ZeroPose = ARG_FLOAT(blend2ZeroPose);
      
    }

    void stopAllBehavioursUpdate(const MessageParamsBase* const params, NmRsCharacter * character)
    {
      GET_ONESHOT_START(stopAllBehaviours);

	  NmRsHumanBody *body = character->getBody();
      Assert(body);
      body->setup(bvid_DirectInvoke, priority, 0, blend, mask DEBUG_LIMBS_PARAMETER("stopAll"));

      // stop everything!
      character->getTaskManager()->deactivateAllTasks(character->getID());
      // and you!
      body->resetEffectors(kResetCalibrations | kResetAngles);
      // and you and you!
      body->holdPose();
      body->setStiffness(5.0f, 0.5f);

      body->postLimbInputs();
    }

#if ALLOW_DEBUG_BEHAVIOURS && ART_ENABLE_BSPY
    void debugSkeletonUpdate(const MessageParamsBase* const params, NmRsCharacter *character)
    {
      GET_ONESHOT_START(debugSkeleton);
      character->setSkeletonVizMode((NmRsCharacter::SkelVizMode)ARG_INT(mode));
      character->setSkeletonVizRoot(ARG_INT(root));
      character->setSkeletonVizMask(character->expressionToMask(ARG_STRING(mask)));
    }
#endif//ALLOW_DEBUG_BEHAVIOURS && ART_ENABLE_BSPY

    /**************************************** RANGED MESSAGES ************************************************/

    void NmRsCBUAnimPose::updateBehaviourMessage(const MessageParamsBase* const params)
    {
      GET_BEHAVIOUR_START(animPose);
      if (start == 0)
        deactivate();
      else
      {
        APPLY_PARAMETER(stiffness, m_parameters.stiffness);
        APPLY_PARAMETER(damping, m_parameters.damping);
        APPLY_PARAMETER(muscleStiffness, m_parameters.muscleStiffness);
        APPLY_PARAMETER(overideHeadlook, m_parameters.overideHeadlook);
        APPLY_PARAMETER(overidePointArm, m_parameters.overidePointArm);
        APPLY_PARAMETER(overidePointGun, m_parameters.overidePointGun);
        APPLY_PARAMETER(gravityCompensation, m_parameters.gravityCompensation);
        APPLY_PARAMETER(useZMPGravityCompensation, m_parameters.useZMPGravityCompensation);
        APPLY_MASK_PARAMETER(effectorMask, m_parameters.effectorMask);
        APPLY_PARAMETER(muscleStiffnessLeftArm, m_parameters.muscleStiffnessLeftArm);
        APPLY_PARAMETER(muscleStiffnessRightArm, m_parameters.muscleStiffnessRightArm);
        APPLY_PARAMETER(muscleStiffnessSpine, m_parameters.muscleStiffnessSpine);
        APPLY_PARAMETER(muscleStiffnessLeftLeg, m_parameters.muscleStiffnessLeftLeg);
        APPLY_PARAMETER(muscleStiffnessRightLeg, m_parameters.muscleStiffnessRightLeg);
        APPLY_PARAMETER(stiffnessLeftArm, m_parameters.stiffnessLeftArm);
        APPLY_PARAMETER(stiffnessRightArm, m_parameters.stiffnessRightArm);
        APPLY_PARAMETER(stiffnessSpine, m_parameters.stiffnessSpine);
        APPLY_PARAMETER(stiffnessLeftLeg, m_parameters.stiffnessLeftLeg);
        APPLY_PARAMETER(stiffnessRightLeg, m_parameters.stiffnessRightLeg);
        APPLY_PARAMETER(dampingLeftArm, m_parameters.dampingLeftArm);
        APPLY_PARAMETER(dampingRightArm, m_parameters.dampingRightArm);
        APPLY_PARAMETER(dampingSpine, m_parameters.dampingSpine);
        APPLY_PARAMETER(dampingLeftLeg, m_parameters.dampingLeftLeg);
        APPLY_PARAMETER(dampingRightLeg, m_parameters.dampingRightLeg);
        APPLY_PARAMETER(gravCompLeftArm, m_parameters.gravCompLeftArm);
        APPLY_PARAMETER(gravCompRightArm, m_parameters.gravCompRightArm);
        APPLY_PARAMETER(gravCompSpine, m_parameters.gravCompSpine);
        APPLY_PARAMETER(gravCompLeftLeg, m_parameters.gravCompLeftLeg);
        APPLY_PARAMETER(gravCompRightLeg, m_parameters.gravCompRightLeg);
        APPLY_PARAMETER(connectedLeftHand, m_parameters.connectedLeftHand);
        APPLY_PARAMETER(connectedRightHand, m_parameters.connectedRightHand);
        APPLY_PARAMETER(connectedLeftFoot, m_parameters.connectedLeftFoot);
        APPLY_PARAMETER(connectedRightFoot, m_parameters.connectedRightFoot);
        APPLY_PARAMETER(animSource, m_parameters.animSource);

        if (start == 1)
          activate();
      }
    }
    void NmRsCBUArmsWindmill::updateBehaviourMessage(const MessageParamsBase* const params)
    {
      GET_BEHAVIOUR_START(armsWindmill);
      if (start == 0)
        deactivate();
      else
      {
        APPLY_PARAMETER(leftPartID, m_parameters.m_leftCircleDesc.partID); 
        APPLY_PARAMETER(leftRadius1, m_parameters.m_leftCircleDesc.radius1);
        APPLY_PARAMETER(leftRadius2, m_parameters.m_leftCircleDesc.radius2);
        APPLY_PARAMETER(leftSpeed, m_parameters.m_leftCircleDesc.speed);
        APPLY_PARAMETER(leftNormal, m_parameters.m_leftCircleDesc.normal);
        APPLY_PARAMETER(leftCentre, m_parameters.m_leftCircleDesc.centre);
        APPLY_PARAMETER(rightPartID, m_parameters.m_rightCircleDesc.partID); 
        APPLY_PARAMETER(rightRadius1, m_parameters.m_rightCircleDesc.radius1);
        APPLY_PARAMETER(rightRadius2, m_parameters.m_rightCircleDesc.radius2);
        APPLY_PARAMETER(rightSpeed, m_parameters.m_rightCircleDesc.speed);
        APPLY_PARAMETER(rightNormal, m_parameters.m_rightCircleDesc.normal);
        APPLY_PARAMETER(rightCentre, m_parameters.m_rightCircleDesc.centre);
        APPLY_PARAMETER(shoulderStiffness, m_parameters.m_shoulderStiffness);
        APPLY_PARAMETER(shoulderDamping, m_parameters.m_shoulderDamping);
        APPLY_PARAMETER(elbowStiffness, m_parameters.m_elbowStiffness);
        APPLY_PARAMETER(elbowDamping, m_parameters.m_elbowDamping);
        APPLY_PARAMETER(leftElbowMin, m_parameters.m_leftElbowMin);
        APPLY_PARAMETER(rightElbowMin, m_parameters.m_rightElbowMin);        
        APPLY_PARAMETER(phaseOffset, m_parameters.m_phaseOffset);
        APPLY_PARAMETER(dragReduction, m_parameters.m_dragReduction);
        APPLY_PARAMETER(IKtwist, m_parameters.m_IKtwist);
        APPLY_PARAMETER(angVelThreshold, m_parameters.m_angVelThreshold);
        APPLY_PARAMETER(angVelGain, m_parameters.m_angVelGain);
        APPLY_PARAMETER(mirrorMode, m_parameters.m_mirrorMode);
        APPLY_PARAMETER(adaptiveMode, m_parameters.m_adaptiveMode);
        APPLY_PARAMETER(forceSync, m_parameters.m_forceSync);
        APPLY_PARAMETER(useLeft, m_parameters.m_useLeft);
        APPLY_PARAMETER(useRight, m_parameters.m_useRight);
        APPLY_PARAMETER(disableOnImpact, m_parameters.m_disableOnImpact);
        if (start == 1)
          activate();
      }
    }
    void NmRsCBUArmsWindmillAdaptive::updateBehaviourMessage(const MessageParamsBase* const params)
    {
      GET_BEHAVIOUR_START(armsWindmillAdaptive);
      if (start == 0)
        deactivate();
      else
      {
        APPLY_PARAMETER(angSpeed, m_parameters.angSpeed);
        APPLY_PARAMETER(bodyStiffness, m_parameters.bodyStiffness);
        APPLY_PARAMETER(amplitude, m_parameters.amplitude);
        APPLY_PARAMETER(phase, m_parameters.phase);
        APPLY_PARAMETER(armStiffness, m_parameters.armStiffness);
        APPLY_PARAMETER(leftElbowAngle, m_parameters.leftElbowAngle);
        APPLY_PARAMETER(rightElbowAngle, m_parameters.rightElbowAngle);
        APPLY_PARAMETER(lean1mult, m_parameters.lean1mult);		
        APPLY_PARAMETER(lean1offset, m_parameters.lean1offset);		
        APPLY_PARAMETER(elbowRate, m_parameters.elbowRate);		
        APPLY_PARAMETER(armDirection, m_parameters.armDirection);   
        APPLY_PARAMETER(disableOnImpact, m_parameters.disableOnImpact);
        APPLY_PARAMETER(setBackAngles, m_parameters.setBackAngles);
        APPLY_PARAMETER(useAngMom, m_parameters.useAngMom);
        APPLY_PARAMETER(bendLeftElbow, m_parameters.bendLeftElbow);
        APPLY_PARAMETER(bendRightElbow, m_parameters.bendRightElbow);
        APPLY_MASK_PARAMETER(mask, m_parameters.effectorMask);

        if (start == 1)
          activate();
      }
    }
    void NmRsCBUBodyFoetal::updateBehaviourMessage(const MessageParamsBase* const params)
    {
      GET_BEHAVIOUR_START(bodyFoetal);
      if (start == 0)
        deactivate();
      else
      {
        APPLY_PARAMETER(stiffness, m_parameters.m_stiffness);
        APPLY_PARAMETER(dampingFactor, m_parameters.m_damping);
        APPLY_PARAMETER(asymmetry, m_parameters.m_asymmetrical);
        APPLY_PARAMETER(randomSeed, m_parameters.m_randomSeed);
        APPLY_PARAMETER(backTwist, m_parameters.m_backTwist);
        APPLY_MASK_PARAMETER(mask, m_parameters.m_effectorMask);


        if (start == 1)
          activate();
      }
    }
    void NmRsCBUStumble::updateBehaviourMessage(const MessageParamsBase* const params)
    {
      GET_BEHAVIOUR_START(stumble);
      if (start == 0)
        deactivate();
      else
      {
        APPLY_PARAMETER(torsoStiffness, m_parameters.m_torsoStiffness);
        APPLY_PARAMETER(legsStiffness, m_parameters.m_legsStiffness);
        APPLY_PARAMETER(armsStiffness, m_parameters.m_armsStiffness);
        APPLY_PARAMETER(armReduceSpeed, m_parameters.armReduceSpeed);		  
        APPLY_PARAMETER(wristMS, m_parameters.wristMS);
        APPLY_PARAMETER(staggerTime, m_parameters.staggerTime);
        APPLY_PARAMETER(dropVal, m_parameters.dropVal);
        APPLY_PARAMETER(injuryRate, m_parameters.injuryRate);	  
        APPLY_PARAMETER(armTwist, m_parameters.armTwist);	  
        APPLY_PARAMETER(backwardsMinArmOffset, m_parameters.m_backwardsMinArmOffset);
        APPLY_PARAMETER(forwardMaxArmOffset, m_parameters.m_forwardMaxArmOffset);
        APPLY_PARAMETER(zAxisSpinReduction, m_parameters.m_zAxisSpinReduction);
        APPLY_PARAMETER(dampPelvis, m_parameters.dampPelvis);       
        APPLY_PARAMETER(pitchInContact, m_parameters.pitchInContact);       
        APPLY_PARAMETER(different, m_parameters.different);       
        APPLY_PARAMETER(twistSpine, m_parameters.twistSpine);       
        APPLY_PARAMETER(useHeadLook, m_parameters.m_useHeadLook);
        APPLY_PARAMETER(leanRate, m_parameters.leanRate);
        APPLY_PARAMETER(maxLeanBack, m_parameters.maxLeanBack);
        APPLY_PARAMETER(maxLeanForward, m_parameters.maxLeanForward);
        APPLY_PARAMETER(feetMS, m_parameters.feetMS);	  
        APPLY_PARAMETER(grabRadius2, m_parameters.grabRadius2);	  
        APPLY_PARAMETER(leanTowards, m_parameters.leanTowards);	    
        APPLY_MASK_PARAMETER(fallMask, m_parameters.fallMask);
        if (start == 1)
          activate();
      }
    }

    void NmRsCBURollUp::updateBehaviourMessage(const MessageParamsBase* const params)
    {
      GET_BEHAVIOUR_START(bodyRollUp);
      if (start == 0)
        deactivate();
      else
      {
        APPLY_PARAMETER(stiffness, m_parameters.m_stiffness);
        APPLY_PARAMETER(useArmToSlowDown, m_parameters.m_useArmToSlowDown);
        APPLY_PARAMETER(armReachAmount, m_parameters.m_armReachAmount);
        APPLY_PARAMETER(legPush, m_parameters.m_legPush);
        APPLY_PARAMETER(asymmetricalLegs, m_parameters.m_asymmetricalLegs);
        APPLY_PARAMETER(noRollTimeBeforeSuccess, m_parameters.m_noRollTimeBeforeSuccess);
        APPLY_PARAMETER(rollVelForSuccess, m_parameters.m_rollVelForSuccess);
        APPLY_PARAMETER(rollVelLinearContribution, m_parameters.m_rollVelLinearContribution);
        APPLY_MASK_PARAMETER(mask, m_parameters.m_effectorMask);
        if (start == 1)
          activate();
      }
    }
    void NmRsCBUBodyWrithe::updateBehaviourMessage(const MessageParamsBase* const params)
    {
      GET_BEHAVIOUR_START(bodyWrithe);
      if (start == 0)
        deactivate();
      else
      {
        APPLY_PARAMETER(armStiffness, m_parameters.m_armStiffness);
        APPLY_PARAMETER(backStiffness, m_parameters.m_backStiffness);
        APPLY_PARAMETER(legStiffness, m_parameters.m_legStiffness);
        APPLY_PARAMETER(armDamping, m_parameters.m_armDamping);
        APPLY_PARAMETER(backDamping, m_parameters.m_backDamping);
        APPLY_PARAMETER(legDamping, m_parameters.m_legDamping);
        APPLY_PARAMETER(armPeriod, m_parameters.m_armPeriod);
        APPLY_PARAMETER(backPeriod, m_parameters.m_backPeriod);
        APPLY_PARAMETER(legPeriod, m_parameters.m_legPeriod);
        APPLY_MASK_PARAMETER(mask, m_parameters.m_effectorMask);
        APPLY_PARAMETER(armAmplitude, m_parameters.m_armAmplitude);
        APPLY_PARAMETER(backAmplitude, m_parameters.m_backAmplitude);
        APPLY_PARAMETER(legAmplitude, m_parameters.m_legAmplitude);
        APPLY_PARAMETER(elbowAmplitude, m_parameters.m_elbowAmplitude);
        APPLY_PARAMETER(kneeAmplitude, m_parameters.m_kneeAmplitude);
        APPLY_PARAMETER(rollOverFlag, m_parameters.m_rollOverFlag);
        APPLY_PARAMETER(blendArms, m_parameters.m_blendArms);
        APPLY_PARAMETER(blendBack, m_parameters.m_blendBack);
        APPLY_PARAMETER(blendLegs, m_parameters.m_blendLegs);
        APPLY_PARAMETER(applyStiffness, m_parameters.m_applyStiffness);
        if (start == 1)
          activate();
      }
    }
#if ALLOW_DEBUG_BEHAVIOURS
    void NmRsCBUDebugRig::updateBehaviourMessage(const MessageParamsBase* const params)//Debug only behaviour
    {
      GET_BEHAVIOUR_START(debugRig);
      if (start == 0)
        deactivate();
      else
      {
		APPLY_PARAMETER(muscleStiffness, m_parameters.muscleStiffness);
        APPLY_PARAMETER(stiffness, m_parameters.m_stiffness);
        APPLY_PARAMETER(damping, m_parameters.m_damping);
		APPLY_PARAMETER(speed, m_parameters.speed);
		APPLY_PARAMETER(joint, m_parameters.joint);

        if (start == 1)
          activate();
      }
    }
#endif //ALLOW_DEBUG_BEHAVIOURS
    void NmRsCBUFallOverWall::updateBehaviourMessage(const MessageParamsBase* const params)
    {
      GET_BEHAVIOUR_START(fallOverWall);
      if (start == 0)
        deactivate();
      else
      {
        APPLY_PARAMETER(bodyStiffness, m_parameters.bodyStiffness);
        APPLY_PARAMETER(damping, m_parameters.bodyDamping);
        APPLY_PARAMETER(magOfForce, m_parameters.magOfForce);
        APPLY_PARAMETER(maxDistanceFromPelToHitPoint, m_parameters.maxDistanceFromPelToHitPoint);
        APPLY_PARAMETER(maxForceDist, m_parameters.maxForceDist);
        APPLY_PARAMETER(stepExclusionZone, m_parameters.stepExclusionZone);
        APPLY_PARAMETER(minLegHeight, m_parameters.minLegHeight);
        APPLY_PARAMETER(fallOverWallEndA, m_parameters.fallOverWallEndA);
        APPLY_PARAMETER(fallOverWallEndB, m_parameters.fallOverWallEndB);
        APPLY_PARAMETER(forceAngleAbort, m_parameters.forceAngleAbort);
        APPLY_PARAMETER(forceTimeOut, m_parameters.forceTimeOut);
        if (ARG_SET(bodyTwist) || start == 1)
        {
          float torque = 0.f;
          APPLY_PARAMETER(bodyTwist, torque);
          m_parameters.minTwistTorqueScale = 45.0f * torque;
          m_parameters.maxTwistTorqueScale = m_parameters.minTwistTorqueScale + (40.0f * torque);
        }
        APPLY_PARAMETER(maxTwist, m_parameters.maxTwist);
        APPLY_PARAMETER(moveArms, m_parameters.moveArms);
        APPLY_PARAMETER(moveLegs, m_parameters.moveLegs);
        APPLY_PARAMETER(bendSpine, m_parameters.bendSpine);
        APPLY_PARAMETER(angleDirWithWallNormal,m_parameters.angleDirWithWallNormal);
        APPLY_PARAMETER(leaningAngleThreshold, m_parameters.leaningAngleThreshold);
        APPLY_PARAMETER(maxAngVel, m_parameters.maxAngVel);
        APPLY_PARAMETER(adaptForcesToLowWall, m_parameters.adaptForcesToLowWall);
        APPLY_PARAMETER(maxWallHeight, m_parameters.maxWallHeight); 
        APPLY_PARAMETER(distanceToSendSuccessMessage, m_parameters.distanceToSendSuccessMessage);
        APPLY_PARAMETER(rollingBackThr, m_parameters.rollingBackThr);
        APPLY_PARAMETER(rollingPotential, m_parameters.rollingPotential);

#if useNewFallOverWall
        APPLY_PARAMETER(useArmIK, m_parameters.useArmIK);
        APPLY_PARAMETER(reachDistanceFromHitPoint, m_parameters.reachDistanceFromHitPoint);
        APPLY_PARAMETER(minReachDistanceFromHitPoint, m_parameters.minReachDistanceFromHitPoint);
        APPLY_PARAMETER(angleTotallyBack, m_parameters.angleTotallyBack);
#endif // useNewFallOverWall
        if (start == 1)
          activate();
      }
    }
    void NmRsCBUHeadLook::updateBehaviourMessage(const MessageParamsBase* const params)
    {
      GET_BEHAVIOUR_START(headLook);
      if (start == 0)
        deactivate();
      else
      {
        APPLY_PARAMETER(damping, m_parameters.m_damping);
        APPLY_PARAMETER(stiffness, m_parameters.m_stiffness);
        APPLY_PARAMETER(instanceIndex, m_parameters.m_instanceIndex);
        APPLY_PARAMETER(vel, m_parameters.m_vel);
        APPLY_PARAMETER(pos, m_parameters.m_pos);
        APPLY_PARAMETER(alwaysLook, m_parameters.m_alwaysLook);
        APPLY_PARAMETER(eyesHorizontal, m_parameters.m_eyesHorizontal);
        APPLY_PARAMETER(alwaysEyesHorizontal, m_parameters.m_alwaysEyesHorizontal);
        APPLY_PARAMETER(keepHeadAwayFromGround, m_parameters.m_keepHeadAwayFromGround);
        APPLY_PARAMETER(twistSpine, m_parameters.twistSpine);

        if (start == 1)
          activate();
      }
    }
    void NmRsCBUHighFall::updateBehaviourMessage(const MessageParamsBase* const params)
    {
      GET_BEHAVIOUR_START(highFall);
      if (start == 0)
        deactivate();
      else
      {
        APPLY_PARAMETER(bodyStiffness, m_parameters.m_bodyStiffness);
        APPLY_PARAMETER(bodydamping, m_parameters.m_bodydamping);
        APPLY_PARAMETER(catchfalltime, m_parameters.m_catchfalltime);
        APPLY_PARAMETER(crashOrLandCutOff, m_parameters.m_crashOrLandCutOff);
        APPLY_PARAMETER(pdStrength, m_parameters.m_pdStrength);
        APPLY_PARAMETER(pdDamping, m_parameters.m_pdDamping);
        APPLY_PARAMETER(armAngSpeed, m_parameters.m_armAngSpeed);
        APPLY_PARAMETER(armAmplitude, m_parameters.m_armAmplitude);
        APPLY_PARAMETER(armPhase, m_parameters.m_armPhase);
        APPLY_PARAMETER(armBendElbows, m_parameters.m_armBendElbows);
        APPLY_PARAMETER(legRadius, m_parameters.m_legRadius);
        APPLY_PARAMETER(legAngSpeed, m_parameters.m_legAngSpeed);
        APPLY_PARAMETER(legAsymmetry, m_parameters.m_legAsymmetry);
        APPLY_PARAMETER(arms2LegsPhase, m_parameters.m_arms2LegsPhase);
        APPLY_PARAMETER(arms2LegsSync, m_parameters.m_arms2LegsSync);
        APPLY_PARAMETER(armsUp, m_parameters.m_armsUp);
        APPLY_PARAMETER(orientateBodyToFallDirection, m_parameters.m_orientateBodyToFallDirection);
        APPLY_PARAMETER(orientateTwist, m_parameters.m_orientateTwist);
        APPLY_PARAMETER(orientateMax, m_parameters.m_orientateMax);
        APPLY_PARAMETER(alanRickman, m_parameters.m_alanRickman);
        APPLY_PARAMETER(fowardRoll, m_parameters.m_forwardRoll);
        APPLY_PARAMETER(useZeroPose_withFowardRoll, m_parameters.m_useZeroPose_withForwardRoll);
        APPLY_PARAMETER(aimAngleBase, m_parameters.m_aimAngleBase);
        APPLY_PARAMETER(fowardVelRotation, m_parameters.m_forwardVelRotation);
        APPLY_PARAMETER(footVelCompScale, m_parameters.m_footVelCompScale);
        APPLY_PARAMETER(sideD, m_parameters.m_sideD);
        APPLY_PARAMETER(fowardOffsetOfLegIK, m_parameters.m_forwardOffsetOfLegIK);
        APPLY_PARAMETER(legL, m_parameters.m_legL);
        APPLY_PARAMETER(catchFallCutOff, m_parameters.m_catchFallCutOff);
        APPLY_PARAMETER(legStrength, m_parameters.m_legStrength);
        APPLY_PARAMETER(balance, m_parameters.m_balance);
        APPLY_PARAMETER(ignorWorldCollisions, m_parameters.m_ignorWorldCollisions);
        APPLY_PARAMETER(adaptiveCircling, m_parameters.m_adaptiveCircling);
        APPLY_PARAMETER(hula, m_parameters.m_hula);
        APPLY_PARAMETER(minSpeedForBrace, m_parameters.m_minSpeedForBrace);
        APPLY_PARAMETER(maxSpeedForRecoverableFall, m_parameters.m_maxSpeedForRecoverableFall);
        APPLY_PARAMETER(landingNormal, m_parameters.m_landingNormal);
        if (start == 1)
          activate();
      }
    }
#if ALLOW_TRAINING_BEHAVIOURS
    void NmRsCBULanding::updateBehaviourMessage(const MessageParamsBase* const params)
    {
      GET_BEHAVIOUR_START(landing);
      if (start == 0)
        deactivate();
      else
      {
        APPLY_PARAMETER(bodyStiffness, m_parameters.m_bodyStiffness);
        APPLY_PARAMETER(bodydamping, m_parameters.m_bodydamping);
        APPLY_PARAMETER(catchfalltime, m_parameters.m_catchfalltime);
        APPLY_PARAMETER(crashOrLandCutOff, m_parameters.m_crashOrLandCutOff);
        APPLY_PARAMETER(angleToCatchFallCutOff, m_parameters.m_angleToCatchFallCutOff);
        APPLY_PARAMETER(pdStrength, m_parameters.m_pdStrength);
        APPLY_PARAMETER(legRadius, m_parameters.m_legRadius);
        APPLY_PARAMETER(legAngSpeed, m_parameters.m_legAngSpeed);
        APPLY_PARAMETER(armsUp, m_parameters.m_armsUp);
        APPLY_PARAMETER(armsFrontward, m_parameters.m_armsFrontward);
        APPLY_PARAMETER(orientateBodyToFallDirection, m_parameters.m_orientateBodyToFallDirection);
        APPLY_PARAMETER(predictedTimeToOrientateBodytoFallDirection, m_parameters.m_predictedTimeToOrientateBodytoFallDirection);
        APPLY_PARAMETER(factorToReduceInitialAngularVelocity, m_parameters.m_factorToReduceInitialAngularVelocity);
        APPLY_PARAMETER(limitNormalFall, m_parameters.m_limitNormalFall);
        APPLY_PARAMETER(aimAngleBase, m_parameters.m_aimAngleBase);
        APPLY_PARAMETER(fowardVelRotation, m_parameters.m_forwardVelRotation);
        APPLY_PARAMETER(sideD, m_parameters.m_sideD);
        APPLY_PARAMETER(legL, m_parameters.m_legL);

        APPLY_PARAMETER(legStrength, m_parameters.m_legStrength);
        APPLY_PARAMETER(ignorWorldCollisions, m_parameters.m_ignorWorldCollisions);

        APPLY_PARAMETER(forwardRoll, m_parameters.m_forwardRoll);
        APPLY_PARAMETER(feetBehindCOM, m_parameters.m_feetBehindCOM);
        APPLY_PARAMETER(feetBehindCOMVel, m_parameters.m_feetBehindCOMVel);
        APPLY_PARAMETER(cheatingTorqueToForwardRoll, m_parameters.m_cheatingTorqueToForwardRoll);
        APPLY_PARAMETER(maxAngVelForForwardRoll, m_parameters.m_maxAngVelForForwardRoll);
        APPLY_PARAMETER(stopFWCOMRoT, m_parameters.m_stopFWCOMRoT);
        APPLY_PARAMETER(stopEndFWCOMRoT, m_parameters.m_stopEndFWCOMRoT);
        APPLY_PARAMETER(standUpCOMBehindFeet, m_parameters.m_standUpCOMBehindFeet);
        APPLY_PARAMETER(standUpRotVel, m_parameters.m_standUpRotVel);
        APPLY_PARAMETER(strengthKneeToStandUp, m_parameters.m_strengthKneeToStandUp);
        APPLY_PARAMETER(sideRoll, m_parameters.m_sideRoll);
        APPLY_PARAMETER(maxVelForSideRoll, m_parameters.m_maxVelForSideRoll);
        if (start == 1)
          activate();
      }
    }
#endif // ALLOW_TRAINING_BEHAVIOURS
    void NmRsCBUPedal::updateBehaviourMessage(const MessageParamsBase* const params)
    {
      GET_BEHAVIOUR_START(pedalLegs);
      if (start == 0)
        deactivate();
      else
      {
        APPLY_PARAMETER(pedalLeftLeg, m_parameters.pedalLeftLeg);
        APPLY_PARAMETER(pedalRightLeg, m_parameters.pedalRightLeg);
        APPLY_PARAMETER(backPedal, m_parameters.backPedal);
        APPLY_PARAMETER(radius, m_parameters.radius);
        APPLY_PARAMETER(angularSpeed, m_parameters.angularSpeed);
        APPLY_PARAMETER(legStiffness, m_parameters.legStiffness);
        APPLY_PARAMETER(pedalOffset, m_parameters.pedalOffset);
        APPLY_PARAMETER(randomSeed, m_parameters.randomSeed);
        APPLY_PARAMETER(speedAsymmetry, m_parameters.speedAsymmetry);
        APPLY_PARAMETER(adaptivePedal4Dragging, m_parameters.adaptivePedal4Dragging);
        APPLY_PARAMETER(angSpeedMultiplier4Dragging, m_parameters.angSpeedMultiplier4Dragging);
        APPLY_PARAMETER(radiusVariance, m_parameters.radiusVariance);
        APPLY_PARAMETER(legAngleVariance, m_parameters.legAngleVariance);
        APPLY_PARAMETER(centreForwards, m_parameters.centreForwards);
        APPLY_PARAMETER(centreSideways, m_parameters.centreSideways);
        APPLY_PARAMETER(centreUp, m_parameters.centreUp);
        APPLY_PARAMETER(ellipse, m_parameters.ellipse);
        APPLY_PARAMETER(dragReduction, m_parameters.dragReduction);
        APPLY_PARAMETER(hula, m_parameters.hula);
        if (start == 1)
          activate();
      }
    }
    void NmRsCBUGrab::updateBehaviourMessage(const MessageParamsBase* const params)
    {
      GET_BEHAVIOUR_START(grab);
      if (start == 0)
        deactivate();
      else
      {
        APPLY_PARAMETER(pos1, m_parameters.pos);
        APPLY_PARAMETER(pos2, m_parameters.pos1);
        APPLY_PARAMETER(pos3, m_parameters.pos2);
        APPLY_PARAMETER(pos4, m_parameters.pos3);
        APPLY_PARAMETER(normalR, m_parameters.normalR);
        APPLY_PARAMETER(normalL, m_parameters.normalL);
				APPLY_PARAMETER(normalR2, m_parameters.normalR2);
				APPLY_PARAMETER(normalL2, m_parameters.normalL2);
        APPLY_PARAMETER(useLineGrab, m_parameters.useLineGrab);
				APPLY_PARAMETER(surfaceGrab, m_parameters.surfaceGrab);
				APPLY_PARAMETER(pointsX4grab, m_parameters.pointsX4grab);
#if NM_EA
				APPLY_PARAMETER(fromEA, m_parameters.fromEA);
#endif
				APPLY_PARAMETER(handsCollide, m_parameters.handsCollide);        
        APPLY_PARAMETER(justBrace, m_parameters.justBrace);
        APPLY_PARAMETER(useLeft, m_parameters.useLeft);
        APPLY_PARAMETER(useRight, m_parameters.useRight);
        APPLY_PARAMETER(dropWeaponIfNecessary, m_parameters.dropWeaponIfNecessary);
        APPLY_PARAMETER(dropWeaponDistance, m_parameters.dropWeaponDistance);
        APPLY_PARAMETER(instanceIndex, m_parameters.instanceIndex);
        APPLY_PARAMETER(dontLetGo, m_parameters.dontLetGo);
        APPLY_PARAMETER(grabStrength, m_parameters.grabStrength);
        APPLY_PARAMETER(bodyStiffness, m_parameters.bodyStiffness);
        APPLY_PARAMETER(reachAngle, m_parameters.reachAngle);
        APPLY_PARAMETER(oneSideReachAngle, m_parameters.oneSideReachAngle);
        APPLY_PARAMETER(grabDistance, m_parameters.grabDistance);
				APPLY_PARAMETER(move2Radius, m_parameters.move2Radius);
        APPLY_PARAMETER(pullUpStrengthRight, m_parameters.pullUpStrengthRight);
        APPLY_PARAMETER(pullUpStrengthLeft, m_parameters.pullUpStrengthLeft);
        APPLY_PARAMETER(pullUpTime, m_parameters.pullUpTime);
        APPLY_PARAMETER(armStiffness, m_parameters.armStiffness);
        APPLY_PARAMETER(grabHoldMaxTimer, m_parameters.grabHoldMaxTimer);
        APPLY_PARAMETER(maxReachDistance, m_parameters.maxReachDistance);
        APPLY_PARAMETER(orientationConstraintScale, m_parameters.orientationConstraintScale);
        APPLY_PARAMETER(instancePartIndex, m_parameters.boundIndex);
        APPLY_PARAMETER(maxWristAngle, m_parameters.maxWristAngle);
        APPLY_PARAMETER(useHeadLookToTarget, m_parameters.useHeadLookToTarget);
        APPLY_PARAMETER(lookAtGrab, m_parameters.lookAtGrab);
        APPLY_PARAMETER(targetForHeadLook, m_parameters.targetForHeadLook);
        APPLY_PARAMETER(stickyHands, m_parameters.stickyHands);
        APPLY_PARAMETER(turnToTarget, m_parameters.turnToTarget);

        if (start == 1)
          activate();
      }
    }
    void NmRsCBUFlinch::updateBehaviourMessage(const MessageParamsBase* const params)
    {
      GET_BEHAVIOUR_START(upperBodyFlinch);
      if (start == 0)
        deactivate();
      else
      {
        APPLY_PARAMETER(rightHanded, m_parameters.m_righthanded);
        APPLY_PARAMETER(leftHanded, m_parameters.m_lefthanded);
        APPLY_PARAMETER(handDistanceLeftRight, m_parameters.m_hand_dist_lr);
        APPLY_PARAMETER(handDistanceFrontBack, m_parameters.m_hand_dist_fb);
        APPLY_PARAMETER(handDistanceVertical, m_parameters.m_hand_dist_vert);
        APPLY_PARAMETER(bodyStiffness, m_parameters.m_bodyStiffness);
        APPLY_PARAMETER(bodyDamping, m_parameters.m_bodyDamping);
        APPLY_PARAMETER(backBendAmount, m_parameters.m_backBendAmount);
        APPLY_PARAMETER(useRightArm, m_parameters.m_useRight);
        APPLY_PARAMETER(useLeftArm, m_parameters.m_useLeft);
        APPLY_PARAMETER(noiseScale, m_parameters.m_noiseScale);
        APPLY_PARAMETER(newHit, m_parameters.m_newHit);
        APPLY_PARAMETER(turnTowards, m_parameters.m_turnTowards);
        APPLY_PARAMETER(protectHeadToggle, m_parameters.m_protectHeadToggle);
        APPLY_PARAMETER(dontBraceHead, m_parameters.m_dontBraceHead);
        APPLY_PARAMETER(headLookAwayFromTarget, m_parameters.m_headLookAwayFromTarget);
        APPLY_PARAMETER(useHeadLook, m_parameters.m_useHeadLook);       
        APPLY_PARAMETER(applyStiffness, m_parameters.m_applyStiffness);       
        APPLY_PARAMETER(pos, m_parameters.m_pos);
        if (start == 1)
          activate();
      }
    }
    void NmRsCBUCatchFall::updateBehaviourMessage(const MessageParamsBase* const params)
    {
      GET_BEHAVIOUR_START(catchFall);
      if (start == 0)
        deactivate();
      else
      {
        APPLY_PARAMETER(torsoStiffness, m_parameters.m_torsoStiffness);
        APPLY_PARAMETER(legsStiffness, m_parameters.m_legsStiffness);
        APPLY_PARAMETER(armsStiffness, m_parameters.m_armsStiffness);
        APPLY_PARAMETER(backwardsMinArmOffset, m_parameters.m_backwardsMinArmOffset);
        APPLY_PARAMETER(forwardMaxArmOffset, m_parameters.m_forwardMaxArmOffset);
        APPLY_PARAMETER(zAxisSpinReduction, m_parameters.m_zAxisSpinReduction);
        APPLY_PARAMETER(useHeadLook, m_parameters.m_useHeadLook);
        APPLY_MASK_PARAMETER(mask, m_parameters.m_effectorMask);
        if (start == 1)
          activate();
      }
    }
    void NmRsCBURollDownStairs::updateBehaviourMessage(const MessageParamsBase* const params)
    {
      GET_BEHAVIOUR_START(rollDownStairs);
      if (start == 0)
        deactivate();
      else
      {
        APPLY_PARAMETER(stiffness, m_parameters.m_Stiffness);
        APPLY_PARAMETER(damping, m_parameters.m_Damping);
        APPLY_PARAMETER(forcemag, m_parameters.m_ForceMag);
        APPLY_PARAMETER(asymmetry, m_parameters.m_AsymmetricalForces);//unused
        APPLY_PARAMETER(m_useArmToSlowDown, m_parameters.m_UseArmsToSlowDown);
        APPLY_PARAMETER(useZeroPose, m_parameters.m_UseZeroPose);
        APPLY_PARAMETER(spinWhenInAir, m_parameters.m_SpinWhenInAir);
        APPLY_PARAMETER(m_armReachAmount, m_parameters.m_ArmReachAmount);
        APPLY_PARAMETER(m_legPush, m_parameters.m_LegPush);
        APPLY_PARAMETER(tryToAvoidHeadButtingGround, m_parameters.m_TryToAvoidHeadButtingGround);
        APPLY_PARAMETER(armReachLength, m_parameters.m_ArmL);
        APPLY_PARAMETER(customRollDir, m_parameters.m_CustomRollDir);
        APPLY_PARAMETER(useCustomRollDir, m_parameters.m_UseCustomRollDir);
        APPLY_PARAMETER(stiffnessDecayTarget, m_parameters.m_StiffnessDecayTarget);
        APPLY_PARAMETER(stiffnessDecayTime, m_parameters.m_StiffnessDecayTime);
        APPLY_PARAMETER(asymmetricalLegs, m_parameters.m_AsymmetricalLegs);
        APPLY_PARAMETER(zAxisSpinReduction, m_parameters.m_zAxisSpinReduction);
        APPLY_PARAMETER(targetLinearVelocityDecayTime, m_parameters.m_targetLinearVelocityDecayTime);
        APPLY_PARAMETER(targetLinearVelocity, m_parameters.m_targetLinearVelocity);
        APPLY_PARAMETER(onlyApplyHelperForces, m_parameters.m_onlyApplyHelperForces);
        APPLY_PARAMETER(useVelocityOfObjectBelow, m_parameters.m_useVelocityOfObjectBelow);
        APPLY_PARAMETER(useRelativeVelocity, m_parameters.m_useRelativeVelocity);

        APPLY_PARAMETER(applyFoetalToLegs, m_parameters.m_applyFoetalToLegs);
        APPLY_PARAMETER(movementLegsInFoetalPosition, m_parameters.m_movementLegsInFoetalPosition);

        APPLY_PARAMETER(maxAngVelAroundFrontwardAxis, m_parameters.m_maxAngVelAroundFrontwardAxis);
        APPLY_PARAMETER(applyNewRollingCheatingTorques, m_parameters.m_applyNewRollingCheatingTorques);
        APPLY_PARAMETER(magOfTorqueToRoll, m_parameters.m_magOfTorqueToRoll);
        APPLY_PARAMETER(maxAngVel, m_parameters.m_maxAngVel);
        APPLY_PARAMETER(minAngVel, m_parameters.m_minAngVel);

        APPLY_PARAMETER(applyHelPerTorqueToAlign, m_parameters.m_applyHelPerTorqueToAlign);
        APPLY_PARAMETER(delayToAlignBody, m_parameters.m_delayToAlignBody);
        APPLY_PARAMETER(magOfTorqueToAlign, m_parameters.m_magOfTorqueToAlign);
        APPLY_PARAMETER(airborneReduction, m_parameters.m_airborneReduction);
        if (start == 1)
          activate();
      }
    }

  void NmRsCBUInjuredOnGround::updateBehaviourMessage(const MessageParamsBase* const params)
  {
    GET_BEHAVIOUR_START(injuredOnGround);
    if (start == 0)
      deactivate();
    else
    {
		APPLY_PARAMETER(numInjuries, m_parameters.m_numInjuries);
		APPLY_PARAMETER(injury1Component, m_parameters.m_injury1Component);
		APPLY_PARAMETER(injury2Component, m_parameters.m_injury2Component);
		APPLY_PARAMETER(injury1LocalPosition, m_parameters.m_injury1LocalPosition);
		APPLY_PARAMETER(injury2LocalPosition, m_parameters.m_injury2LocalPosition);
		APPLY_PARAMETER(injury1LocalNormal, m_parameters.m_injury1LocalNormal);
		APPLY_PARAMETER(injury2LocalNormal, m_parameters.m_injury2LocalNormal);
		APPLY_PARAMETER(dontReachWithLeft, m_parameters.m_dontReachWithLeft);
		APPLY_PARAMETER(dontReachWithRight, m_parameters.m_dontReachWithRight);
		APPLY_PARAMETER(attackerPos, m_parameters.m_attackerPos);
		APPLY_PARAMETER(strongRollForce, m_parameters.m_strongRollForce);
      if (start == 1)
        activate();
    }
  }

  void NmRsCBUCarried::updateBehaviourMessage(const MessageParamsBase* const params)
  {
	  GET_BEHAVIOUR_START(carried);
	  if (start == 0)
		  deactivate();
	  else
	  {
		  if (start == 1)
			  activate();
	  }
  }

  void NmRsCBUDangle::updateBehaviourMessage(const MessageParamsBase* const params)
  {
    GET_BEHAVIOUR_START(dangle);
    if (start == 0)
      deactivate();
    else
    {
      APPLY_PARAMETER(doGrab, m_parameters.m_doGrab);       
      APPLY_PARAMETER(grabFrequency, m_parameters.m_grabFrequency);
      if (start == 1)
        activate();
    }
  }

    /*
    *  shot specific configure messages
    */
    void shotNewBulletUpdate(const MessageParamsBase* const params, NmRsCharacter *character)
    {
      GET_ONESHOT_START(shotNewBullet);
      NmRsCBUShot* shotTask = (NmRsCBUShot*)character->getTask(bvid_shot);

      if(shotTask)
      {
        shotTask->m_parameters.bodyPart = ARG_INT(bodyPart);
        shotTask->m_parameters.localHitPointInfo = ARG_BOOL(localHitPointInfo);
        shotTask->m_parameters.normal = ARG_VECTOR3(normal);
        shotTask->m_parameters.hitPoint = ARG_VECTOR3(hitPoint);
        shotTask->m_parameters.bulletVel = ARG_VECTOR3(bulletVel);
        if(ARG_SET(bodyPart))
        {
          NmRsHumanBody *body = character->getBody();
          Assert(body);
          body->setup(bvid_DirectInvoke, priority, 0, blend, mask DEBUG_LIMBS_PARAMETER("newHit"));
#if ART_ENABLE_BSPY
          character->setCurrentSubBehaviour("-NewHit"); 
#endif
          if (shotTask->isActive())
            shotTask->newHit(*body);
#if ART_ENABLE_BSPY
          character->setCurrentSubBehaviour(""); 
#endif
          body->postLimbInputs();
        }
      }
    }
    void shotSnapUpdate(const MessageParamsBase* const params, NmRsCharacter *character)
    {
      GET_ONESHOT_START(shotSnap);
      NmRsCBUShot* shotTask = (NmRsCBUShot*)character->getTask(bvid_shot);
      if(shotTask)
      {
        APPLY_PARAMETER(snap, shotTask->m_parameters.snap);
        APPLY_PARAMETER(snapMag, shotTask->m_parameters.snapMag);
        APPLY_PARAMETER(snapMovingMult, shotTask->m_parameters.snapMovingMult);
        APPLY_PARAMETER(snapBalancingMult, shotTask->m_parameters.snapBalancingMult);
        APPLY_PARAMETER(snapAirborneMult, shotTask->m_parameters.snapAirborneMult);
        APPLY_PARAMETER(snapMovingThresh, shotTask->m_parameters.snapMovingThresh);
        APPLY_PARAMETER(snapDirectionRandomness, shotTask->m_parameters.snapDirectionRandomness);
        APPLY_PARAMETER(snapLeftLeg, shotTask->m_parameters.snapLeftLeg);
        APPLY_PARAMETER(snapRightLeg, shotTask->m_parameters.snapRightLeg);
        APPLY_PARAMETER(snapLeftArm, shotTask->m_parameters.snapLeftArm);
        APPLY_PARAMETER(snapRightArm, shotTask->m_parameters.snapRightArm);
        APPLY_PARAMETER(snapSpine, shotTask->m_parameters.snapSpine);
        APPLY_PARAMETER(snapNeck, shotTask->m_parameters.snapNeck);
        APPLY_PARAMETER(snapPhasedLegs, shotTask->m_parameters.snapPhasedLegs);
        APPLY_PARAMETER(snapHipType, shotTask->m_parameters.snapHipType);
        APPLY_PARAMETER(snapUseBulletDir, shotTask->m_parameters.snapUseBulletDir);
        APPLY_PARAMETER(snapHitPart, shotTask->m_parameters.snapHitPart);		
        APPLY_PARAMETER(unSnapInterval, shotTask->m_parameters.unSnapInterval);
        APPLY_PARAMETER(unSnapRatio, shotTask->m_parameters.unSnapRatio);
        APPLY_PARAMETER(snapUseTorques, shotTask->m_parameters.snapUseTorques);       
      }
    }
    void shotShockSpinUpdate(const MessageParamsBase* const params, NmRsCharacter *character)
    {
      GET_ONESHOT_START(shotShockSpin);
      NmRsCBUShot* shotTask = (NmRsCBUShot*)character->getTask(bvid_shot);
      if(shotTask)
      {
        shotTask->m_parameters.addShockSpin = ARG_BOOL(addShockSpin);
        shotTask->m_parameters.randomizeShockSpinDirection = ARG_BOOL(randomizeShockSpinDirection);
        shotTask->m_parameters.alwaysAddShockSpin = ARG_BOOL(alwaysAddShockSpin);
        shotTask->m_parameters.shockSpinMin = ARG_FLOAT(shockSpinMin);
        shotTask->m_parameters.shockSpinMax = ARG_FLOAT(shockSpinMax);
        shotTask->m_parameters.shockSpinLiftForceMult = ARG_FLOAT(shockSpinLiftForceMult);
        shotTask->m_parameters.shockSpinDecayMult = ARG_FLOAT(shockSpinDecayMult);
        shotTask->m_parameters.shockSpinScalePerComponent = ARG_FLOAT(shockSpinScalePerComponent);
        shotTask->m_parameters.shockSpinMaxTwistVel = ARG_FLOAT(shockSpinMaxTwistVel);
        shotTask->m_parameters.shockSpin1FootMult = ARG_FLOAT(shockSpin1FootMult);
        shotTask->m_parameters.shockSpinAirMult = ARG_FLOAT(shockSpinAirMult);
        shotTask->m_parameters.shockSpinFootGripMult = ARG_FLOAT(shockSpinFootGripMult);
        shotTask->m_parameters.shockSpinScaleByLeverArm = ARG_BOOL(shockSpinScaleByLeverArm);
        shotTask->m_parameters.bracedSideSpinMult = ARG_FLOAT(bracedSideSpinMult);
      }
    }
    void shotFallToKneesUpdate(const MessageParamsBase* const params, NmRsCharacter *character)
    {
      GET_ONESHOT_START(shotFallToKnees);
      NmRsCBUShot* shotTask = (NmRsCBUShot*)character->getTask(bvid_shot);
      if(shotTask)
      {
        shotTask->m_parameters.fallToKnees = ARG_BOOL(fallToKnees);
        shotTask->m_parameters.ftkBalanceTime = ARG_FLOAT(ftkBalanceTime);
        shotTask->m_parameters.ftkHelperForce = ARG_FLOAT(ftkHelperForce);
        shotTask->m_parameters.ftkLeanHelp = ARG_FLOAT(ftkLeanHelp);
        shotTask->m_parameters.ftkSpineBend = ARG_FLOAT(ftkSpineBend);        
        shotTask->m_parameters.ftkImpactLoosenessTime = ARG_FLOAT(ftkImpactLoosenessTime);
        shotTask->m_parameters.ftkImpactLooseness = ARG_FLOAT(ftkImpactLooseness);
        shotTask->m_parameters.ftkBendRate = ARG_FLOAT(ftkBendRate);
        shotTask->m_parameters.ftkHipBlend = ARG_FLOAT(ftkHipBlend);
        shotTask->m_parameters.ftkFricMult = ARG_FLOAT(ftkFricMult);
        shotTask->m_parameters.ftkHipAngleFall = ARG_FLOAT(ftkHipAngleFall);
        shotTask->m_parameters.ftkPitchForwards = ARG_FLOAT(ftkPitchForwards);
        shotTask->m_parameters.ftkPitchBackwards = ARG_FLOAT(ftkPitchBackwards);
        shotTask->m_parameters.ftkFallBelowStab = ARG_FLOAT(ftkFallBelowStab);
        shotTask->m_parameters.ftkBalanceAbortThreshold = ARG_FLOAT(ftkBalanceAbortThreshold);
        shotTask->m_parameters.ftkFailMustCollide = ARG_BOOL(ftkFailMustCollide);
        shotTask->m_parameters.ftkOnKneesArmType = ARG_INT(ftkOnKneesArmType);
        shotTask->m_parameters.ftkReleaseReachForWound = ARG_BOOL(ftkReleaseReachForWound);
        shotTask->m_parameters.ftkReleasePointGun = ARG_BOOL(ftkReleasePointGun);     
        shotTask->m_parameters.ftkHelperForceOnSpine = ARG_BOOL(ftkHelperForceOnSpine);
        shotTask->m_parameters.ftkAlwaysChangeFall = ARG_BOOL(ftkAlwaysChangeFall);      
        shotTask->m_parameters.ftkStiffSpine = ARG_BOOL(ftkStiffSpine);       
      }
    }
    void shotFromBehindUpdate(const MessageParamsBase* const params, NmRsCharacter *character)
    {
      GET_ONESHOT_START(shotFromBehind);
      NmRsCBUShot* shotTask = (NmRsCBUShot*)character->getTask(bvid_shot);
      if(shotTask)
      {
        shotTask->m_parameters.shotFromBehind = ARG_BOOL(shotFromBehind);
        shotTask->m_parameters.sfbSpineAmount = ARG_FLOAT(sfbSpineAmount);
        shotTask->m_parameters.sfbNeckAmount = ARG_FLOAT(sfbNeckAmount);
        shotTask->m_parameters.sfbHipAmount = ARG_FLOAT(sfbHipAmount);
        shotTask->m_parameters.sfbKneeAmount = ARG_FLOAT(sfbKneeAmount);
        shotTask->m_parameters.sfbPeriod = ARG_FLOAT(sfbPeriod);
        shotTask->m_parameters.sfbForceBalancePeriod = ARG_FLOAT(sfbForceBalancePeriod);
        shotTask->m_parameters.sfbArmsOnset = ARG_FLOAT(sfbArmsOnset);
        shotTask->m_parameters.sfbKneesOnset = ARG_FLOAT(sfbKneesOnset);
        shotTask->m_parameters.sfbNoiseGain = ARG_FLOAT(sfbNoiseGain);
      }
    }
    void shotInGutsUpdate(const MessageParamsBase* const params, NmRsCharacter *character)
    {
      GET_ONESHOT_START(shotInGuts);
      NmRsCBUShot* shotTask = (NmRsCBUShot*)character->getTask(bvid_shot);
      if(shotTask)
      {
        shotTask->m_parameters.shotInGuts = ARG_BOOL(shotInGuts);
        shotTask->m_parameters.sigSpineAmount = ARG_FLOAT(sigSpineAmount);
        shotTask->m_parameters.sigNeckAmount = ARG_FLOAT(sigNeckAmount);
        shotTask->m_parameters.sigHipAmount = ARG_FLOAT(sigHipAmount);
        shotTask->m_parameters.sigKneeAmount = ARG_FLOAT(sigKneeAmount);
        shotTask->m_parameters.sigPeriod = ARG_FLOAT(sigPeriod);
        shotTask->m_parameters.sigForceBalancePeriod = ARG_FLOAT(sigForceBalancePeriod);
        shotTask->m_parameters.sigKneesOnset = ARG_FLOAT(sigKneesOnset);
      }  
    }
    void shotHeadLookUpdate(const MessageParamsBase* const params, NmRsCharacter *character)
    {
      GET_ONESHOT_START(shotHeadLook);
      NmRsCBUShot* shotTask = (NmRsCBUShot*)character->getTask(bvid_shot);
      if(shotTask)
      {
        bool wasUsingHeadLook = shotTask->m_parameters.useHeadLook;
        shotTask->m_parameters.useHeadLook = ARG_BOOL(useHeadLook);
        shotTask->m_parameters.headLookPos = ARG_VECTOR3(headLook);
        shotTask->m_parameters.headLookAtWoundMinTimer = ARG_FLOAT(headLookAtWoundMinTimer);
        shotTask->m_parameters.headLookAtWoundMaxTimer = ARG_FLOAT(headLookAtWoundMaxTimer);
        shotTask->m_parameters.headLookAtHeadPosMinTimer = ARG_FLOAT(headLookAtHeadPosMinTimer);
        shotTask->m_parameters.headLookAtHeadPosMaxTimer = ARG_FLOAT(headLookAtHeadPosMaxTimer);
        
        // initialize toggleTimer that switches between looking at wound and target
        if(!wasUsingHeadLook && shotTask->m_parameters.useHeadLook)
          shotTask->setHeadLookToggleTimer(character->getRandom().GetRanged(shotTask->m_parameters.headLookAtWoundMinTimer, shotTask->m_parameters.headLookAtWoundMaxTimer));
      }
    }
    void shotConfigureArmsUpdate(const MessageParamsBase* const params, NmRsCharacter *character)
    {
      GET_ONESHOT_START(shotConfigureArms);
      NmRsCBUShot* shotTask = (NmRsCBUShot*)character->getTask(bvid_shot);
      if(shotTask)
      {
        shotTask->m_parameters.brace = ARG_BOOL(brace);
        shotTask->m_parameters.pointGun = ARG_BOOL(pointGun);
        shotTask->m_parameters.useArmsWindmill = ARG_BOOL(useArmsWindmill);
        shotTask->m_parameters.alwaysReachTime = ARG_FLOAT(alwaysReachTime);
        shotTask->m_parameters.AWSpeedMult = ARG_FLOAT(AWSpeedMult);
        shotTask->m_parameters.AWRadiusMult = ARG_FLOAT(AWRadiusMult);
        shotTask->m_parameters.AWStiffnessAdd = ARG_FLOAT(AWStiffnessAdd);
        shotTask->m_parameters.releaseWound = ARG_INT(releaseWound);
        shotTask->m_parameters.reachWithOneHand = ARG_INT(reachWithOneHand);     
        shotTask->m_parameters.allowLeftPistolRFW = ARG_BOOL(allowLeftPistolRFW);
        shotTask->m_parameters.allowRightPistolRFW = ARG_BOOL(allowRightPistolRFW);
        shotTask->m_parameters.rfwWithPistol = ARG_BOOL(rfwWithPistol);
      }
    }
    void NmRsCBUShot::updateBehaviourMessage(const MessageParamsBase* const params)
    {
      GET_BEHAVIOUR_START(shot);
      if (start == 0)
        deactivate();
      else
      {
        APPLY_PARAMETER(bodyStiffness, m_parameters.bodyStiffness);
        APPLY_PARAMETER(armStiffness, m_parameters.armStiffness);      
        APPLY_PARAMETER(spineDamping, m_parameters.spineDamping);
        APPLY_PARAMETER(initialNeckStiffness, m_parameters.initialNeckStiffness);
        APPLY_PARAMETER(initialNeckDamping, m_parameters.initialNeckDamping);
        APPLY_PARAMETER(neckStiffness, m_parameters.neckStiffness);
        APPLY_PARAMETER(neckDamping, m_parameters.neckDamping);
        APPLY_PARAMETER(angVelScale, m_parameters.angVelScale);
        APPLY_MASK_PARAMETER(angVelScaleMask, m_parameters.angVelScaleMask);    
        APPLY_PARAMETER(kMultOnLoose, m_parameters.kMultOnLoose);
        APPLY_PARAMETER(kMult4Legs, m_parameters.kMult4Legs);      
        APPLY_PARAMETER(loosenessAmount, m_parameters.loosenessAmount);
        APPLY_PARAMETER(looseness4Fall, m_parameters.looseness4Fall);
        APPLY_PARAMETER(looseness4Stagger, m_parameters.looseness4Stagger);
        APPLY_PARAMETER(grabHoldTime, m_parameters.grabHoldTime);
        APPLY_PARAMETER(bulletProofVest, m_parameters.bulletProofVest);      
        APPLY_PARAMETER(alwaysResetLooseness, m_parameters.alwaysResetLooseness);      
        APPLY_PARAMETER(alwaysResetNeckLooseness, m_parameters.alwaysResetNeckLooseness);
        APPLY_PARAMETER(spineBlendExagCPain, m_parameters.spineBlendExagCPain);        
        APPLY_PARAMETER(minArmsLooseness, m_parameters.minArmsLooseness);      
        APPLY_PARAMETER(minLegsLooseness, m_parameters.minLegsLooseness);      
        APPLY_PARAMETER(spineBlendZero, m_parameters.spineBlendZero);       
        APPLY_PARAMETER(crouching, m_parameters.crouching);    
        APPLY_PARAMETER(chickenArms, m_parameters.chickenArms);
        APPLY_PARAMETER(fling, m_parameters.fling);
        APPLY_PARAMETER(flingWidth, m_parameters.flingWidth);
        APPLY_PARAMETER(allowInjuredLeg, m_parameters.allowInjuredLeg);
        APPLY_PARAMETER(allowInjuredLowerLegReach, m_parameters.allowInjuredLowerLegReach);
        APPLY_PARAMETER(allowInjuredThighReach, m_parameters.allowInjuredThighReach);    
        APPLY_PARAMETER(allowInjuredArm, m_parameters.allowInjuredArm);
        APPLY_PARAMETER(stableHandsAndNeck, m_parameters.stableHandsAndNeck);
        APPLY_PARAMETER(melee, m_parameters.melee);

        APPLY_PARAMETER(reachForWound, m_parameters.reachForWound);
        APPLY_PARAMETER(timeBeforeReachForWound, m_parameters.timeBeforeReachForWound);

        //BulletExaggeration/reflex - just affects spine at the mo
        APPLY_PARAMETER(exagDuration, m_parameters.exagDuration);
        APPLY_PARAMETER(exagMag, m_parameters.exagMag);
        APPLY_PARAMETER(exagTwistMag, m_parameters.exagTwistMag);
        APPLY_PARAMETER(exagSmooth2Zero, m_parameters.exagSmooth2Zero);
        APPLY_PARAMETER(exagZeroTime, m_parameters.exagZeroTime);
        //Conscious pain - just affects spine at the mo
        APPLY_PARAMETER(cpainSmooth2Time, m_parameters.cpainSmooth2Time);
        APPLY_PARAMETER(cpainDuration, m_parameters.cpainDuration);
        APPLY_PARAMETER(cpainMag, m_parameters.cpainMag);
        APPLY_PARAMETER(cpainTwistMag, m_parameters.cpainTwistMag);
        APPLY_PARAMETER(cpainSmooth2Zero, m_parameters.cpainSmooth2Zero);

        APPLY_PARAMETER(useCatchFallOnFall, m_parameters.useCatchFallOnFall);
        APPLY_PARAMETER(useExtendedCatchFall, m_parameters.useExtendedCatchFall);

        APPLY_PARAMETER(initialWeaknessRampDuration, m_parameters.initialWeaknessRampDuration);
        APPLY_PARAMETER(initialWeaknessZeroDuration, m_parameters.initialWeaknessZeroDuration);
        APPLY_PARAMETER(initialNeckRampDuration, m_parameters.initialNeckRampDuration);
        APPLY_PARAMETER(initialNeckDuration, m_parameters.initialNeckDuration);

        APPLY_PARAMETER(useCStrModulation, m_parameters.useCStrModulation);
        APPLY_PARAMETER(cStrUpperMin, m_parameters.cStrUpperMin);
        APPLY_PARAMETER(cStrUpperMax, m_parameters.cStrUpperMax);
        APPLY_PARAMETER(cStrLowerMin, m_parameters.cStrLowerMin);
        APPLY_PARAMETER(cStrLowerMax, m_parameters.cStrLowerMax);

        APPLY_PARAMETER(deathTime, m_parameters.deathTime);       

        if (start == 1)
        {
          shotHeadLookUpdate(params, m_character);
          shotInGutsUpdate(params, m_character);
          shotFromBehindUpdate(params, m_character);
          shotFallToKneesUpdate(params, m_character);
          shotSnapUpdate(params, m_character);
          shotShockSpinUpdate(params, m_character);
          configureShotInjuredLegUpdate(params,m_character);//mmmmmCHECK THIS
          activate();
          shotNewBulletUpdate(params, m_character);//sets newHit to true if bodyPart set
          //mmmtodo here because ArmsUpdate are not parameters
          shotConfigureArmsUpdate(params, m_character);
        }
      }
    }

    void NmRsCBUBodyBalance::updateBehaviourMessage(const MessageParamsBase* const params)
    {
      GET_BEHAVIOUR_START(bodyBalance);
      if (start == 0)
        deactivate();
      else
      {
        APPLY_PARAMETER(armStiffness, m_parameters.m_armStiffness);
        APPLY_PARAMETER(elbow, m_parameters.m_elbow);
        APPLY_PARAMETER(shoulder, m_parameters.m_shoulder);
        APPLY_PARAMETER(armDamping, m_parameters.m_armDamping);
        APPLY_PARAMETER(useHeadLook, m_parameters.m_useHeadLook);
        APPLY_PARAMETER(headLookPos, m_parameters.m_headLookPos);
        APPLY_PARAMETER(headLookInstanceIndex, m_parameters.m_headLookInstanceIndex);
        APPLY_PARAMETER(spineStiffness, m_parameters.m_spineStiffness);
        APPLY_PARAMETER(somersaultAngle, m_parameters.m_somersaultAngle);
        APPLY_PARAMETER(somersaultAngleThreshold, m_parameters.m_somersaultAngleThreshold);
        APPLY_PARAMETER(sideSomersaultAngle, m_parameters.m_sideSomersaultAngle);
        APPLY_PARAMETER(sideSomersaultAngleThreshold, m_parameters.m_sideSomersaultAngleThreshold);
        APPLY_PARAMETER(armsOutOnPush, m_parameters.m_armsOutOnPush);
        APPLY_PARAMETER(backwardsAutoTurn, m_parameters.m_backwardsAutoTurn);
        APPLY_PARAMETER(backwardsArms, m_parameters.m_backwardsArms);
        APPLY_PARAMETER(blendToZeroPose, m_parameters.m_blendToZeroPose);		
        APPLY_PARAMETER(armsOutOnPushMultiplier, m_parameters.m_armsOutOnPushMultiplier);
        APPLY_PARAMETER(armsOutOnPushTimeout, m_parameters.m_armsOutOnPushTimeout);
        APPLY_PARAMETER(returningToBalanceArmsOut, m_parameters.m_returningToBalanceArmsOut);
        APPLY_PARAMETER(armsOutStraightenElbows, m_parameters.m_armsOutStraightenElbows);
        APPLY_PARAMETER(armsOutMinLean2, m_parameters.m_armsOutMinLean2 );
        APPLY_PARAMETER(spineDamping, m_parameters.m_spineDamping);
        APPLY_PARAMETER(useBodyTurn, m_parameters.m_useBodyTurn);
        APPLY_PARAMETER(elbowAngleOnContact, m_parameters.m_elbowAngleOnContact);
        APPLY_PARAMETER(bendElbowsTime, m_parameters.m_bendElbowsTime);
        APPLY_PARAMETER(bendElbowsGait, m_parameters.m_bendElbowsGait);
        APPLY_PARAMETER(hipL2ArmL2, m_parameters.m_hipL2ArmL2);
        APPLY_PARAMETER(shoulderL2, m_parameters.m_shoulderL2);
        APPLY_PARAMETER(shoulderL1, m_parameters.m_shoulderL1);
        APPLY_PARAMETER(shoulderTwist, m_parameters.m_shoulderTwist);
        APPLY_PARAMETER(headLookAtVelProb, m_parameters.m_headLookAtVelProb);
        APPLY_PARAMETER(turnOffProb, m_parameters.m_turnOffProb);
        APPLY_PARAMETER(turn2VelProb, m_parameters.m_turn2VelProb);
        APPLY_PARAMETER(turnAwayProb, m_parameters.m_turnAwayProb);
        APPLY_PARAMETER(turnLeftProb, m_parameters.m_turnLeftProb);
        APPLY_PARAMETER(turnRightProb, m_parameters.m_turnRightProb);
        APPLY_PARAMETER(turn2TargetProb, m_parameters.m_turn2TargetProb);
        rage::Vector3 angVelMult;
        rage::Vector3 angVelThresh;
        APPLY_PARAMETER(angVelMultiplier, angVelMult);
        APPLY_PARAMETER(angVelThreshold, angVelThresh);
        if (ARG_SET(angVelMultiplier) || start == 1)
        {
          m_parameters.m_somersaultAngVel = angVelMult.x;
          m_parameters.m_twistAngVel = angVelMult.y;
          m_parameters.m_sideSomersaultAngVel = angVelMult.z;
        }
        if (ARG_SET(angVelThreshold) || start == 1)
        {
          m_parameters.m_somersaultAngVelThreshold = angVelThresh.x;
          m_parameters.m_twistAngVelThreshold = angVelThresh.y;
          m_parameters.m_sideSomersaultAngVelThreshold = angVelThresh.z;
        }
        if (start == 1)
          activate();
      }
    }
    void NmRsCBUBraceForImpact::updateBehaviourMessage(const MessageParamsBase* const params)
    {
      GET_BEHAVIOUR_START(braceForImpact);
      if (start == 0)
        deactivate();
      else
      {
        APPLY_PARAMETER(braceDistance, m_parameters.braceDistance);
        APPLY_PARAMETER(targetPredictionTime, m_parameters.targetPredictionTime);
        APPLY_PARAMETER(reachAbsorbtionTime, m_parameters.reachAbsorbtionTime);
        APPLY_PARAMETER(instanceIndex, m_parameters.instanceIndex);
        if (ARG_SET(instanceIndex))
        {
          setCarInstGenID();
#if ART_ENABLE_BSPY
          //Add levelIndex of object to brace against to pool of bSpy objects
          // Braced against object will continue to be sent to bSpy when brace no longer running 
          m_character->getEngineNotConst()->setbSpyObject(m_parameters.instanceIndex);
#endif
        }
        APPLY_PARAMETER(bodyStiffness, m_parameters.bodyStiffness);
        APPLY_PARAMETER(grabDontLetGo, m_parameters.grabDontLetGo);
        APPLY_PARAMETER(grabStrength, m_parameters.grabStrength);
        APPLY_PARAMETER(grabDistance, m_parameters.grabDistance);
        APPLY_PARAMETER(grabReachAngle, m_parameters.grabReachAngle);
        APPLY_PARAMETER(grabHoldTimer, m_parameters.grabHoldTimer);
        APPLY_PARAMETER(maxGrabCarVelocity, m_parameters.maxGrabCarVelocity);
        APPLY_PARAMETER(legStiffness, m_parameters.legStiffness);
        APPLY_PARAMETER(timeToBackwardsBrace, m_parameters.timeToBackwardsBrace);
        APPLY_PARAMETER(minBraceTime, m_parameters.minBraceTime);
        APPLY_PARAMETER(handsDelayMin, m_parameters.handsDelayMin);
        APPLY_PARAMETER(handsDelayMax, m_parameters.handsDelayMax);
       
        APPLY_PARAMETER(moveAway, m_parameters.moveAway);
        APPLY_PARAMETER(moveAwayAmount, m_parameters.moveAwayAmount);
        APPLY_PARAMETER(moveAwayLean, m_parameters.moveAwayLean);
        APPLY_PARAMETER(moveSideways, m_parameters.moveSideways);

        APPLY_PARAMETER(look, m_parameters.look);
        APPLY_PARAMETER(pos, m_parameters.pos);
        APPLY_PARAMETER(moveAway, m_parameters.moveAway);
        APPLY_PARAMETER(moveAwayAmount, m_parameters.moveAwayAmount);
        APPLY_PARAMETER(moveAwayLean, m_parameters.moveAwayLean);
        APPLY_PARAMETER(moveSideways, m_parameters.moveSideways);     
        APPLY_PARAMETER(bbArms, m_parameters.bbArms);
        APPLY_PARAMETER(roll2Velocity, m_parameters.roll2Velocity); 
        APPLY_PARAMETER(newBrace, m_parameters.newBrace); 
        APPLY_PARAMETER(braceOnImpact, m_parameters.braceOnImpact);
        APPLY_PARAMETER(rollType, m_parameters.rollType);
        if (start == 1)
          activate();
      }
    }
    void NmRsCBUPointArm::updateBehaviourMessage(const MessageParamsBase* const params)
    {
      GET_BEHAVIOUR_START(pointArm);
      if (start == 0)
        deactivate();
      else
      {
        APPLY_PARAMETER(targetLeft, m_parameters_Left.target);
        APPLY_PARAMETER(twistLeft, m_parameters_Left.twist);
        APPLY_PARAMETER(armStraightnessLeft, m_parameters_Left.armStraightness);
        APPLY_PARAMETER(useLeftArm, m_parameters_Left.useLeftArm);
        APPLY_PARAMETER(armStiffnessLeft, m_parameters_Left.armStiffness);
        APPLY_PARAMETER(armDampingLeft, m_parameters_Left.armDamping);
        APPLY_PARAMETER(instanceIndexLeft, m_parameters_Left.instanceIndex);
        APPLY_PARAMETER(pointSwingLimitLeft, m_parameters_Left.pointSwingLimit);
        APPLY_PARAMETER(useZeroPoseWhenNotPointingLeft, m_parameters_Left.useZeroPoseWhenNotPointing);

        APPLY_PARAMETER(targetRight, m_parameters_Right.target);
        APPLY_PARAMETER(twistRight, m_parameters_Right.twist);
        APPLY_PARAMETER(armStraightnessRight, m_parameters_Right.armStraightness);
        APPLY_PARAMETER(useRightArm, m_parameters_Right.useRightArm);
        APPLY_PARAMETER(armStiffnessRight, m_parameters_Right.armStiffness);
        APPLY_PARAMETER(armDampingRight, m_parameters_Right.armDamping);
        APPLY_PARAMETER(instanceIndexRight, m_parameters_Right.instanceIndex);
        APPLY_PARAMETER(pointSwingLimitRight, m_parameters_Right.pointSwingLimit);
        APPLY_PARAMETER(useZeroPoseWhenNotPointingRight, m_parameters_Right.useZeroPoseWhenNotPointing);
        if (start == 1)
          activate();
      }
    }
    void NmRsCBUYanked::updateBehaviourMessage(const MessageParamsBase* const params)
    {
      GET_BEHAVIOUR_START(yanked);
      if (start == 0)
        deactivate();
      else
      {
        APPLY_PARAMETER(armStiffness, m_parameters.m_armStiffness);
        APPLY_PARAMETER(armDamping, m_parameters.m_armDamping);
        APPLY_PARAMETER(spineDamping, m_parameters.m_spineDamping);
        APPLY_PARAMETER(spineStiffness, m_parameters.m_spineStiffness);
        APPLY_PARAMETER(armStiffnessStart, m_parameters.m_armStiffnessStart);
        APPLY_PARAMETER(armDampingStart, m_parameters.m_armDampingStart);
        APPLY_PARAMETER(spineDampingStart, m_parameters.m_spineDampingStart);
        APPLY_PARAMETER(spineStiffnessStart, m_parameters.m_spineStiffnessStart);
        APPLY_PARAMETER(timeAtStartValues, m_parameters.m_timeAtStartValues);
        APPLY_PARAMETER(rampTimeFromStartValues, m_parameters.m_rampTimeFromStartValues);
        APPLY_PARAMETER(stepsTillStartEnd, m_parameters.m_stepsTillStartEnd);
        APPLY_PARAMETER(timeStartEnd, m_parameters.m_timeStartEnd);
        APPLY_PARAMETER(rampTimeToEndValues, m_parameters.m_rampTimeToEndValues);
        APPLY_PARAMETER(lowerBodyStiffness, m_parameters.m_lowerBodyStiffness);
        APPLY_PARAMETER(lowerBodyStiffnessEnd, m_parameters.m_lowerBodyStiffnessEnd);
        APPLY_PARAMETER(perStepReduction, m_parameters.m_perStepReduction);
        APPLY_PARAMETER(hipPitchForward, m_parameters.m_hipPitchForward);
        APPLY_PARAMETER(hipPitchBack, m_parameters.m_hipPitchBack);
        APPLY_PARAMETER(spineBend, m_parameters.m_spineBend);
        APPLY_PARAMETER(footFriction, m_parameters.m_footFriction);        
        APPLY_PARAMETER(comVelRDSThresh, m_parameters.m_comVelRDSThresh); 
        APPLY_PARAMETER(useHeadLook, m_parameters.m_useHeadLook);
        APPLY_PARAMETER(headLookPos, m_parameters.m_headLookPos);
        APPLY_PARAMETER(headLookInstanceIndex, m_parameters.m_headLookInstanceIndex);
        APPLY_PARAMETER(headLookAtVelProb, m_parameters.m_headLookAtVelProb);
        APPLY_PARAMETER(turnThresholdMin, m_parameters.m_turnThresholdMin);
        APPLY_PARAMETER(turnThresholdMax, m_parameters.m_turnThresholdMax);
        APPLY_PARAMETER(hulaPeriod, m_parameters.m_hulaPeriod); 
        APPLY_PARAMETER(hipAmplitude, m_parameters.m_hipAmplitude); 
        APPLY_PARAMETER(spineAmplitude, m_parameters.m_hipAmplitude); 
        APPLY_PARAMETER(minRelaxPeriod, m_parameters.m_minRelaxPeriod); 
        APPLY_PARAMETER(maxRelaxPeriod, m_parameters.m_maxRelaxPeriod); 
        APPLY_PARAMETER(rollHelp, m_parameters.m_rollHelp);  
        APPLY_PARAMETER(groundArmStiffness, m_parameters.m_groundArmStiffness);  
        APPLY_PARAMETER(groundLegStiffness, m_parameters.m_groundLegStiffness);  
        APPLY_PARAMETER(groundSpineStiffness, m_parameters.m_groundSpineStiffness);  
        APPLY_PARAMETER(groundArmDamping, m_parameters.m_groundArmDamping);  
        APPLY_PARAMETER(groundLegDamping, m_parameters.m_groundLegDamping);  
        APPLY_PARAMETER(groundSpineDamping, m_parameters.m_groundSpineDamping);  
        APPLY_PARAMETER(groundFriction, m_parameters.m_groundFriction);  
        
        if (start == 1)
          activate();
      }
    }
    void NmRsCBUBalancerCollisionsReaction::updateBehaviourMessage(const MessageParamsBase* const params)
    {
      GET_BEHAVIOUR_START(balancerCollisionsReaction);
      if (start == 0)
        deactivate();
      else
      {
        APPLY_PARAMETER(numStepsTillSlump, m_parameters.numStepsTillSlump);
        APPLY_PARAMETER(stable2SlumpTime, m_parameters.stable2SlumpTime);
        APPLY_PARAMETER(exclusionZone, m_parameters.exclusionZone);   
        APPLY_PARAMETER(footFrictionMultStart, m_parameters.footFrictionMultStart);
        APPLY_PARAMETER(footFrictionMultRate, m_parameters.footFrictionMultRate);
        APPLY_PARAMETER(backFrictionMultStart, m_parameters.backFrictionMultStart);
        APPLY_PARAMETER(backFrictionMultRate, m_parameters.backFrictionMultRate);
        APPLY_PARAMETER(impactLegStiffReduction, m_parameters.impactLegStiffReduction);
        APPLY_PARAMETER(slumpLegStiffReduction, m_parameters.slumpLegStiffReduction);
        APPLY_PARAMETER(slumpLegStiffRate, m_parameters.slumpLegStiffRate);
        APPLY_PARAMETER(reactTime, m_parameters.reactTime);

        APPLY_PARAMETER(glanceSpinTime, m_parameters.glanceSpinTime);
        APPLY_PARAMETER(glanceSpinMag, m_parameters.glanceSpinMag);
        APPLY_PARAMETER(glanceSpinDecayMult, m_parameters.glanceSpinDecayMult);

        APPLY_PARAMETER(ignoreColWithIndex, m_parameters.ignoreColWithIndex);
        APPLY_PARAMETER(slumpMode, m_parameters.slumpMode);
        APPLY_PARAMETER(reboundMode, m_parameters.reboundMode);      
        APPLY_PARAMETER(ignoreColMassBelow, m_parameters.ignoreColMassBelow);
        APPLY_PARAMETER(ignoreColVolumeBelow, m_parameters.ignoreColVolumeBelow);
        APPLY_PARAMETER(fallOverWallDrape, m_parameters.fallOverWallDrape);
        APPLY_PARAMETER(fallOverHighWalls, m_parameters.fallOverHighWalls);   

        APPLY_PARAMETER(snap, m_parameters.snap);
        APPLY_PARAMETER(snapMag, m_parameters.snapMag);
        APPLY_PARAMETER(snapDirectionRandomness, m_parameters.snapDirectionRandomness);
        APPLY_PARAMETER(snapLeftLeg, m_parameters.snapLeftLeg);
        APPLY_PARAMETER(snapRightLeg, m_parameters.snapRightLeg);
        APPLY_PARAMETER(snapLeftArm, m_parameters.snapLeftArm);
        APPLY_PARAMETER(snapRightArm, m_parameters.snapRightArm);
        APPLY_PARAMETER(snapSpine, m_parameters.snapSpine);
        APPLY_PARAMETER(snapNeck, m_parameters.snapNeck);
        APPLY_PARAMETER(snapPhasedLegs, m_parameters.snapPhasedLegs);
        APPLY_PARAMETER(snapHipType, m_parameters.snapHipType);
        APPLY_PARAMETER(unSnapInterval, m_parameters.unSnapInterval);
        APPLY_PARAMETER(unSnapRatio, m_parameters.unSnapRatio);
        APPLY_PARAMETER(snapUseTorques, m_parameters.snapUseTorques);       

        APPLY_PARAMETER(impactWeaknessRampDuration, m_parameters.impactWeaknessRampDuration);
        APPLY_PARAMETER(impactWeaknessZeroDuration, m_parameters.impactWeaknessZeroDuration);
        APPLY_PARAMETER(impactLoosenessAmount, m_parameters.impactLoosenessAmount);
        
        APPLY_PARAMETER(objectBehindVictim, m_parameters.objectBehindVictim);
        APPLY_PARAMETER(objectBehindVictimPos, m_parameters.objectBehindVictimPos);
        APPLY_PARAMETER(objectBehindVictimNormal, m_parameters.objectBehindVictimNormal);

        if (start == 1)
          activate();
      }
    }
    
    void NmRsCBUTeeter::updateBehaviourMessage(const MessageParamsBase* const params)
    {
      GET_BEHAVIOUR_START(teeter);
      if (start == 0)
        deactivate();
      else
      {
        APPLY_PARAMETER(edgeLeft, m_parameters.edgeLeft);
        APPLY_PARAMETER(edgeRight, m_parameters.edgeRight);
        APPLY_PARAMETER(useExclusionZone, m_parameters.useExclusionZone);        
        APPLY_PARAMETER(callHighFall, m_parameters.callHighFall);        
        APPLY_PARAMETER(leanAway, m_parameters.leanAway);        
        if (start == 1)
          activate();
      }
    }

    void NmRsCBUElectrocute::updateBehaviourMessage(const MessageParamsBase* const params)
    {
      GET_BEHAVIOUR_START(electrocute);
      if (start == 0)
        deactivate();
      else
      {
        APPLY_PARAMETER(tazeMag, m_parameters.tazeMag);
        APPLY_PARAMETER(initialMult, m_parameters.initialMult);
        APPLY_PARAMETER(largeMult, m_parameters.largeMult);
        APPLY_PARAMETER(largeMinTime, m_parameters.largeMinTime);
        APPLY_PARAMETER(largeMaxTime, m_parameters.largeMaxTime);
        APPLY_PARAMETER(movingMult, m_parameters.movingMult);
        APPLY_PARAMETER(balancingMult, m_parameters.balancingMult);
        APPLY_PARAMETER(airborneMult, m_parameters.airborneMult);
        APPLY_PARAMETER(movingThresh, m_parameters.movingThresh);
        APPLY_PARAMETER(tazeInterval, m_parameters.tazeInterval);
        APPLY_PARAMETER(directionRandomness, m_parameters.directionRandomness);
        APPLY_PARAMETER(leftLeg, m_parameters.leftLeg);
        APPLY_PARAMETER(rightLeg, m_parameters.rightLeg);
        APPLY_PARAMETER(leftArm, m_parameters.leftArm);
        APPLY_PARAMETER(rightArm, m_parameters.rightArm);
        APPLY_PARAMETER(spine, m_parameters.spine);
        APPLY_PARAMETER(neck, m_parameters.neck);
        APPLY_PARAMETER(applyStiffness, m_parameters.applyStiffness);
        APPLY_PARAMETER(useTorques, m_parameters.useTorques);       
        APPLY_PARAMETER(phasedLegs, m_parameters.phasedLegs);
        APPLY_PARAMETER(hipType, m_parameters.hipType);
        if (start == 1)
          activate();
      }
    }

    void NmRsCBUStaggerFall::updateBehaviourMessage(const MessageParamsBase* const params)
    {
      GET_BEHAVIOUR_START(staggerFall);
      if (start == 0)
        deactivate();
      else
      {
        APPLY_PARAMETER(armStiffness, m_parameters.m_armStiffness);
        APPLY_PARAMETER(armDamping, m_parameters.m_armDamping);
        APPLY_PARAMETER(spineDamping, m_parameters.m_spineDamping);
        APPLY_PARAMETER(spineStiffness, m_parameters.m_spineStiffness);
        APPLY_PARAMETER(armStiffnessStart, m_parameters.m_armStiffnessStart);
        APPLY_PARAMETER(armDampingStart, m_parameters.m_armDampingStart);
        APPLY_PARAMETER(spineDampingStart, m_parameters.m_spineDampingStart);
        APPLY_PARAMETER(spineStiffnessStart, m_parameters.m_spineStiffnessStart);
        APPLY_PARAMETER(timeAtStartValues, m_parameters.m_timeAtStartValues);
        APPLY_PARAMETER(rampTimeFromStartValues, m_parameters.m_rampTimeFromStartValues);   
        APPLY_PARAMETER(staggerStepProb, m_parameters.m_staggerStepProb);
        APPLY_PARAMETER(stepsTillStartEnd, m_parameters.m_stepsTillStartEnd);
        APPLY_PARAMETER(timeStartEnd, m_parameters.m_timeStartEnd);
        APPLY_PARAMETER(rampTimeToEndValues, m_parameters.m_rampTimeToEndValues);
        APPLY_PARAMETER(lowerBodyStiffness, m_parameters.m_lowerBodyStiffness);
        APPLY_PARAMETER(lowerBodyStiffnessEnd, m_parameters.m_lowerBodyStiffnessEnd);
        APPLY_PARAMETER(predictionTime, m_parameters.m_predictionTime);
        APPLY_PARAMETER(perStepReduction1, m_parameters.m_perStepReduction1);
        APPLY_PARAMETER(leanInDirRate, m_parameters.m_leanInDirRate);
        APPLY_PARAMETER(leanInDirMaxF, m_parameters.m_leanInDirMaxF);
        APPLY_PARAMETER(leanInDirMaxB, m_parameters.m_leanInDirMaxB);
        APPLY_PARAMETER(leanHipsMaxF, m_parameters.m_leanHipsMaxF);
        APPLY_PARAMETER(leanHipsMaxB, m_parameters.m_leanHipsMaxB);
        APPLY_PARAMETER(lean2multF, m_parameters.m_lean2multF);
        APPLY_PARAMETER(lean2multB, m_parameters.m_lean2multB);
        APPLY_PARAMETER(pushOffDist, m_parameters.pushOffDist);
        APPLY_PARAMETER(maxPushoffVel, m_parameters.maxPushoffVel);
        APPLY_PARAMETER(hipBendMult, m_parameters.m_hipBendMult);
        APPLY_PARAMETER(alwaysBendForwards, m_parameters.m_alwaysBendForwards);        
        APPLY_PARAMETER(spineBendMult, m_parameters.m_spineBendMult);
        APPLY_PARAMETER(useHeadLook, m_parameters.m_useHeadLook);
        APPLY_PARAMETER(headLookPos, m_parameters.m_headLookPos);
        APPLY_PARAMETER(headLookInstanceIndex, m_parameters.m_headLookInstanceIndex);
        APPLY_PARAMETER(headLookAtVelProb, m_parameters.m_headLookAtVelProb);
        APPLY_PARAMETER(turnOffProb, m_parameters.m_turnOffProb);
        APPLY_PARAMETER(turn2TargetProb, m_parameters.m_turn2TargetProb);
        APPLY_PARAMETER(turn2VelProb, m_parameters.m_turn2VelProb);
        APPLY_PARAMETER(turnAwayProb, m_parameters.m_turnAwayProb);
        APPLY_PARAMETER(turnLeftProb, m_parameters.m_turnLeftProb);
        APPLY_PARAMETER(turnRightProb, m_parameters.m_turnRightProb);
        APPLY_PARAMETER(useBodyTurn, m_parameters.m_useBodyTurn);
        APPLY_PARAMETER(upperBodyReaction, m_parameters.m_upperBodyReaction);
        if (start == 1)
          activate();
      }
    }
    void NmRsCBUDragged::updateBehaviourMessage(const MessageParamsBase* const params)
    {
      GET_BEHAVIOUR_START(dragged);
      if (start == 0)
        deactivate();
      else
      {
        APPLY_PARAMETER(armStiffness, m_parameters.m_armStiffness);
        APPLY_PARAMETER(armDamping, m_parameters.m_armDamping);
        APPLY_PARAMETER(armMuscleStiffness, m_parameters.m_armMuscleStiffness); 
        APPLY_PARAMETER(radiusTolerance, m_parameters.m_radiusTolerance);    
        APPLY_PARAMETER(ropeAttachedToInstance, m_parameters.m_ropeAttachedToInstance);
        APPLY_PARAMETER(ropePos, m_parameters.m_ropePos);
        APPLY_PARAMETER(ropedBodyPart, m_parameters.m_ropedBodyPart);
        APPLY_PARAMETER(ropeTaut, m_parameters.m_ropeTaut);
        APPLY_PARAMETER(playerControl, m_parameters.m_playerControl);
        APPLY_PARAMETER(grabLeft, m_parameters.m_grabLeft);
        APPLY_PARAMETER(grabRight, m_parameters.m_grabRight);

        APPLY_PARAMETER(lengthTolerance, m_parameters.m_lengthTolerance);
        APPLY_PARAMETER(armTwist, m_parameters.m_armTwist);
        APPLY_PARAMETER(reach, m_parameters.m_reach);
        if (start == 1)
          activate();
      }
    }

    void NmRsCBULearnedCrawl::updateBehaviourMessage(const MessageParamsBase* const params)
    {
      GET_BEHAVIOUR_START(learnedCrawl);
      if (start == 0)
        deactivate();
      else
      {
        APPLY_PARAMETER(stiffness, m_parameters.stiffness);
        APPLY_PARAMETER(damping, m_parameters.damping);
        APPLY_PARAMETER(learn, m_parameters.bLearn);
        APPLY_PARAMETER(numFrames2Learn, m_parameters.numFrames2Learn);
        APPLY_PARAMETER(inputSequence, m_parameters.inputSequence);
        APPLY_PARAMETER(inputSequenceSize, m_parameters.inputSequenceSize);
        APPLY_PARAMETER(yawOffset, m_parameters.yawOffset);
        APPLY_PARAMETER(targetPosition, m_parameters.targetPosition);
        APPLY_PARAMETER(speed, m_parameters.speed);
        APPLY_PARAMETER(animIndex, m_parameters.animIndex);
        APPLY_PARAMETER(learnFromAnimPlayback, m_parameters.learnFromAnimPlayback);
        APPLY_PARAMETER(useSpine3Thing, m_parameters.useSpine3Thing);
        APPLY_PARAMETER(useRollBoneCompensation, m_parameters.useRollBoneCompensation);
        APPLY_PARAMETER(useTwister, m_parameters.useTwister);
        if (start == 1)
          activate();
      }
    }
    void NmRsCBUPointGun::updateBehaviourMessage(const MessageParamsBase* const params)
    {
      GET_BEHAVIOUR_START(pointGun);
      if (start == 0)
        deactivate();
      else
      {
        APPLY_PARAMETER(leftHandTarget, m_parameters.leftHandTarget);  
        APPLY_PARAMETER(leftHandTargetIndex, m_parameters.leftHandTargetIndex);  
        APPLY_PARAMETER(leftHandParentEffector, m_parameters.leftHandParentEffector);
        APPLY_PARAMETER(leftHandParentOffset, m_parameters.leftHandParentOffset);
        APPLY_PARAMETER(rightHandTarget, m_parameters.rightHandTarget); 
        APPLY_PARAMETER(rightHandTargetIndex, m_parameters.rightHandTargetIndex);  
        APPLY_PARAMETER(rightHandParentEffector, m_parameters.rightHandParentEffector);
        APPLY_PARAMETER(rightHandParentOffset, m_parameters.rightHandParentOffset);
        APPLY_PARAMETER(leadTarget, m_parameters.leadTarget);
        APPLY_PARAMETER(primaryHandWeaponDistance, m_parameters.primaryHandWeaponDistance);
        APPLY_PARAMETER(armStiffness, m_parameters.armStiffness);
        APPLY_PARAMETER(armStiffnessDetSupport, m_parameters.armStiffnessDetSupport);
        APPLY_PARAMETER(armDamping, m_parameters.armDamping);
        APPLY_PARAMETER(clavicleBlend,m_parameters.clavicleBlend);
        APPLY_PARAMETER(elbowAttitude,m_parameters.elbowAttitude);
        APPLY_PARAMETER(supportConstraint,m_parameters.supportConstraint);
        APPLY_PARAMETER(oneHandedPointing,m_parameters.oneHandedPointing);
        APPLY_PARAMETER(constraintMinDistance,m_parameters.constraintMinDistance);
        APPLY_PARAMETER(makeConstraintDistance,m_parameters.makeConstraintDistance);
        APPLY_PARAMETER(reduceConstraintLengthVel,m_parameters.reduceConstraintLengthVel);	
        APPLY_PARAMETER(breakingStrength,m_parameters.breakingStrength);
        APPLY_PARAMETER(constraintStrength,m_parameters.constraintStrength);
        APPLY_PARAMETER(constraintThresh,m_parameters.constraintThresh);
        APPLY_PARAMETER(brokenSupportTime,m_parameters.brokenSupportTime);
        APPLY_PARAMETER(brokenToSideProb,m_parameters.brokenToSideProb);
        APPLY_PARAMETER(connectAfter,m_parameters.connectAfter);
        APPLY_PARAMETER(connectFor,m_parameters.connectFor);
        APPLY_PARAMETER(useIncomingTransforms,m_parameters.useIncomingTransforms);
        APPLY_PARAMETER(measureParentOffset,m_parameters.measureParentOffset);       
        APPLY_PARAMETER(weaponMask,m_parameters.weaponMask);
        APPLY_PARAMETER(stabilizeRifleStock,m_parameters.stabilizeRifleStock);
        APPLY_PARAMETER(oriStiff,m_parameters.oriStiff);
        APPLY_PARAMETER(oriDamp,m_parameters.oriDamp);
        APPLY_PARAMETER(posStiff,m_parameters.posStiff);
        APPLY_PARAMETER(posDamp,m_parameters.posDamp);
        APPLY_PARAMETER(extraTilt,m_parameters.extraTilt);
        APPLY_PARAMETER(fireWeaponRelaxTime,m_parameters.fireWeaponRelaxTime);
        APPLY_PARAMETER(fireWeaponRelaxAmount,m_parameters.fireWeaponRelaxAmount);
        APPLY_PARAMETER(fireWeaponRelaxDistance,m_parameters.fireWeaponRelaxDistance);
        APPLY_PARAMETER(gravityOpposition,m_parameters.gravityOpposition);
        APPLY_PARAMETER(gravOppDetachedSupport,m_parameters.gravOppDetachedSupport);
        APPLY_PARAMETER(massMultDetachedSupport,m_parameters.massMultDetachedSupport);
        APPLY_PARAMETER(constrainRifle,m_parameters.constrainRifle);
        APPLY_PARAMETER(rifleConstraintMinDistance,m_parameters.rifleConstraintMinDistance);
        APPLY_PARAMETER(enableRight,m_parameters.enableRight);
        APPLY_PARAMETER(enableLeft,m_parameters.enableLeft);
        APPLY_PARAMETER(disableArmCollisions,m_parameters.disableArmCollisions);
        APPLY_PARAMETER(disableRifleCollisions,m_parameters.disableRifleCollisions);
        APPLY_PARAMETER(poseUnusedGunArm,m_parameters.poseUnusedGunArm);
        APPLY_PARAMETER(poseUnusedSupportArm,m_parameters.poseUnusedSupportArm);
        APPLY_PARAMETER(poseUnusedOtherArm,m_parameters.poseUnusedOtherArm);

        APPLY_PARAMETER(timeWarpActive, m_parameters.timeWarpActive);
        APPLY_PARAMETER(timeWarpStrengthScale, m_parameters.timeWarpStrengthScale);
        APPLY_PARAMETER(maxAngleAcross, m_parameters.maxAngleAcross);
        APPLY_PARAMETER(maxAngleAway, m_parameters.maxAngleAway);
        APPLY_PARAMETER(pistolNeutralType, m_parameters.pistolNeutralType);
        APPLY_PARAMETER(neutralPoint4Pistols, m_parameters.neutralPoint4Pistols);
        APPLY_PARAMETER(neutralPoint4Rifle, m_parameters.neutralPoint4Rifle);
        APPLY_PARAMETER(checkNeutralPoint, m_parameters.checkNeutralPoint);
        APPLY_PARAMETER(point2Side, m_parameters.point2Side);
        APPLY_PARAMETER(add2WeaponDistSide, m_parameters.add2WeaponDistSide);
        APPLY_PARAMETER(point2Connect, m_parameters.point2Connect);
        APPLY_PARAMETER(add2WeaponDistConnect, m_parameters.add2WeaponDistConnect);

        APPLY_PARAMETER(errorThreshold, m_parameters.errorThreshold);
        APPLY_PARAMETER(usePistolIK, m_parameters.usePistolIK);
        APPLY_PARAMETER(useSpineTwist, m_parameters.useSpineTwist);
        APPLY_PARAMETER(useTurnToTarget, m_parameters.useTurnToTarget);
        APPLY_PARAMETER(useHeadLook, m_parameters.useHeadLook);
        APPLY_PARAMETER(alwaysSupport, m_parameters.alwaysSupport);
        APPLY_PARAMETER(allowShotLooseness, m_parameters.allowShotLooseness);
        m_parameters.targetValid = true;
        if (start == 1)
          activate();
      }
    }
    void NmRsCBUBuoyancy::updateBehaviourMessage(const MessageParamsBase* const params)
    {
      GET_BEHAVIOUR_START(buoyancy);
      if (start == 0)
        deactivate();
      else
      {
        APPLY_PARAMETER(surfacePoint, m_parameters.surfacePoint);
        APPLY_PARAMETER(surfaceNormal, m_parameters.surfaceNormal);
        APPLY_PARAMETER(buoyancy, m_parameters.buoyancy);
        APPLY_PARAMETER(chestBuoyancy, m_parameters.chestBuoyancy);
        APPLY_PARAMETER(damping, m_parameters.damping);
        APPLY_PARAMETER(righting, m_parameters.righting);
        APPLY_PARAMETER(rightingStrength, m_parameters.rightingStrength);
        APPLY_PARAMETER(rightingTime, m_parameters.rightingTime);
        if (start == 1)
          activate();
      }
    }

    /******************************** MESSAGE HANDLER *********************************************/

    bool NmRsCharacter::handleDirectInvoke(InvokeUID iUID, const MessageParamsBase* const params)
    {
      // non-behaviours
      if( isBiped())
      {
        // human only
        TRY_UPDATE_MESSAGE(incomingTransforms);
        TRY_UPDATE_MESSAGE(leanInDirection);
        TRY_UPDATE_MESSAGE(leanRandom);
        TRY_UPDATE_MESSAGE(leanToPosition);
        TRY_UPDATE_MESSAGE(leanTowardsObject);
        TRY_UPDATE_MESSAGE(hipsLeanInDirection);
        TRY_UPDATE_MESSAGE(hipsLeanRandom);
        TRY_UPDATE_MESSAGE(hipsLeanToPosition);
        TRY_UPDATE_MESSAGE(hipsLeanTowardsObject);
        TRY_UPDATE_MESSAGE(forceLeanInDirection);
        TRY_UPDATE_MESSAGE(forceLeanRandom);
        TRY_UPDATE_MESSAGE(forceLeanToPosition);
        TRY_UPDATE_MESSAGE(forceLeanTowardsObject);
        TRY_UPDATE_MESSAGE(stayUpright);
        TRY_UPDATE_MESSAGE(bodyRelax);
        TRY_UPDATE_MESSAGE(configureBalance);
        TRY_UPDATE_MESSAGE(configureBalanceReset);
        TRY_UPDATE_MESSAGE(configureShotInjuredArm);
        TRY_UPDATE_MESSAGE(configureShotInjuredLeg);
#if NM_USE_IK_SELF_AVOIDANCE
        TRY_UPDATE_MESSAGE(configureSelfAvoidance);
#endif // NM_USE_IK_SELF_AVOIDANCE
#if NM_EA
        TRY_UPDATE_MESSAGE(addPatch);
#endif//#if NM_EA

        // split-up shot messages
        TRY_UPDATE_MESSAGE(shotNewBullet);
        TRY_UPDATE_MESSAGE(shotSnap);
        TRY_UPDATE_MESSAGE(shotShockSpin);
        TRY_UPDATE_MESSAGE(shotFallToKnees);
        TRY_UPDATE_MESSAGE(shotFromBehind);
        TRY_UPDATE_MESSAGE(shotInGuts);
        TRY_UPDATE_MESSAGE(shotHeadLook);
        TRY_UPDATE_MESSAGE(shotConfigureArms);

        TRY_UPDATE_MESSAGE(defineAttachedObject);
        TRY_UPDATE_MESSAGE(shotRelax);
        TRY_UPDATE_MESSAGE(fireWeapon);
        TRY_UPDATE_MESSAGE(configureConstraints);  
      }
      else if(m_bodyIdent == rdrHorse ||
        m_bodyIdent == mp3Dog)
      {
        // quadruped only
      }
      // common to all rigs
      TRY_UPDATE_MESSAGE(applyImpulse);
      TRY_UPDATE_MESSAGE(applyBulletImpulse);
      TRY_UPDATE_MESSAGE(activePose);
      TRY_UPDATE_MESSAGE(incomingTransforms);
      TRY_UPDATE_MESSAGE(configureBullets);
      TRY_UPDATE_MESSAGE(forceToBodyPart);
      TRY_UPDATE_MESSAGE(setStiffness);
      TRY_UPDATE_MESSAGE(setMuscleStiffness);
      TRY_UPDATE_MESSAGE(setWeaponMode);
      TRY_UPDATE_MESSAGE(registerWeapon);
      TRY_UPDATE_MESSAGE(stopAllBehaviours);
      TRY_UPDATE_MESSAGE(setCharacterStrength);
      TRY_UPDATE_MESSAGE(setFallingReaction);
      TRY_UPDATE_MESSAGE(setCharacterUnderwater);
      TRY_UPDATE_MESSAGE(configureLimits);


      // compiled behaviour units
      // human only
      if( isBiped())
      {
        TRY_UPDATE_BEHAVIOUR_MESSAGE(NmRsCBUAnimPose, animPose);
        TRY_UPDATE_BEHAVIOUR_MESSAGE(NmRsCBUArmsWindmill, armsWindmill);
        TRY_UPDATE_BEHAVIOUR_MESSAGE(NmRsCBUArmsWindmillAdaptive, armsWindmillAdaptive);
        TRY_UPDATE_BEHAVIOUR_MESSAGE(NmRsCBUBodyFoetal, bodyFoetal);
        TRY_UPDATE_BEHAVIOUR_MESSAGE(NmRsCBUTeeter, teeter);
        TRY_UPDATE_BEHAVIOUR_MESSAGE(NmRsCBUElectrocute, electrocute);
        TRY_UPDATE_BEHAVIOUR_MESSAGE(NmRsCBURollUp, bodyRollUp);
        TRY_UPDATE_BEHAVIOUR_MESSAGE(NmRsCBUBodyWrithe, bodyWrithe);
        TRY_UPDATE_BEHAVIOUR_MESSAGE(NmRsCBUFallOverWall, fallOverWall);
        TRY_UPDATE_BEHAVIOUR_MESSAGE(NmRsCBUInjuredOnGround, injuredOnGround);
        TRY_UPDATE_BEHAVIOUR_MESSAGE(NmRsCBUCarried, carried);
        TRY_UPDATE_BEHAVIOUR_MESSAGE(NmRsCBUDangle, dangle);
        TRY_UPDATE_BEHAVIOUR_MESSAGE(NmRsCBUHeadLook, headLook);
        TRY_UPDATE_BEHAVIOUR_MESSAGE(NmRsCBUHighFall, highFall);
#if ALLOW_TRAINING_BEHAVIOURS
        TRY_UPDATE_BEHAVIOUR_MESSAGE(NmRsCBULanding, landing);
#endif
        TRY_UPDATE_BEHAVIOUR_MESSAGE(NmRsCBUBuoyancy, buoyancy);
        TRY_UPDATE_BEHAVIOUR_MESSAGE(NmRsCBUPedal, pedalLegs);
        TRY_UPDATE_BEHAVIOUR_MESSAGE(NmRsCBUGrab, grab);
        TRY_UPDATE_BEHAVIOUR_MESSAGE(NmRsCBUFlinch, upperBodyFlinch);
        TRY_UPDATE_BEHAVIOUR_MESSAGE(NmRsCBUCatchFall, catchFall);
        TRY_UPDATE_BEHAVIOUR_MESSAGE(NmRsCBURollDownStairs, rollDownStairs);
        TRY_UPDATE_BEHAVIOUR_MESSAGE(NmRsCBUShot, shot);
        TRY_UPDATE_BEHAVIOUR_MESSAGE(NmRsCBUBraceForImpact, braceForImpact);
        TRY_UPDATE_BEHAVIOUR_MESSAGE(NmRsCBUPointArm, pointArm);
        TRY_UPDATE_BEHAVIOUR_MESSAGE(NmRsCBUYanked, yanked);
        TRY_UPDATE_BEHAVIOUR_MESSAGE(NmRsCBUStaggerFall, staggerFall);
        TRY_UPDATE_BEHAVIOUR_MESSAGE(NmRsCBUDragged, dragged);
        TRY_UPDATE_BEHAVIOUR_MESSAGE(NmRsCBUBalancerCollisionsReaction, balancerCollisionsReaction);
        TRY_UPDATE_BEHAVIOUR_MESSAGE(NmRsCBULearnedCrawl, learnedCrawl);
        TRY_UPDATE_BEHAVIOUR_MESSAGE(NmRsCBUBodyBalance, bodyBalance);
        TRY_UPDATE_BEHAVIOUR_MESSAGE(NmRsCBUPointGun, pointGun);
        TRY_UPDATE_BEHAVIOUR_MESSAGE(NmRsCBUStumble, stumble);

#if ALLOW_DEBUG_BEHAVIOURS
        //Debug only behaviours
        TRY_UPDATE_BEHAVIOUR_MESSAGE(NmRsCBUDebugRig, debugRig);
#if ART_ENABLE_BSPY
        TRY_UPDATE_MESSAGE(debugSkeleton);
#endif//ART_ENABLE_BSPY
#endif //ALLOW_DEBUG_BEHAVIOURS
      }
      else
        Assert(false); // removed non-biped support for the moment.

      // common to all rigs

      Assertf(0, "NaturalMotion - attempt to start unknown behaviour (hash %i)", iUID);

      return false;
    }

}

