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

#ifndef NM_RS_EFFECTORS_H
#define NM_RS_EFFECTORS_H

#include "NmRsEffectorDefs.h"
#include "NmRsShadows.h"
#include "NmRsCharacter.h"

#if ART_ENABLE_BSPY
#include "NmRsCBU_Shared.h"
#endif

#define NM_BSPY_JOINT_DEBUG_PLUS_MASK 0

#if NM_BSPY_JOINT_DEBUG_PLUS_MASK
#define NM_BSPY_JOINT_DEBUG 1
#define NM_BSPY_JOINT_DEBUG_INDEX (gtaJtShoulder_Right) // Output info only about this joint to bSpy - to stop overcrowding
#define NM_BSPY_JOINT_DEBUG_MASK (0xFFFFFFFF);
#else
#define NM_BSPY_JOINT_DEBUG 0
#define NM_BSPY_JOINT_DEBUG_INDEX (gtaJtShoulder_Right) // Output info only about this joint to bSpy - to stop overcrowding
#define NM_BSPY_JOINT_DEBUG_MASK (1 << gtaUpper_Arm_Right) // (bvmask_ThighRight)
#endif

//#if defined(NM_RS_VALIDATE_VITAL_VALUES) || ART_ENABLE_BSPY
#if ART_ENABLE_BSPY
#define NM_RS_SETBY_PARAMS_DEFAULTS , const char * setBy = 0
#define NM_RS_SETBY_PARAMS , const char * setBy
#define NM_RS_SETBY_PARAMS_UNUSED , const char *
#define NM_RS_SETBY_PARAMS_ARG(_setBy) , setBy
#else
#define NM_RS_SETBY_PARAMS_DEFAULTS
#define NM_RS_SETBY_PARAMS
#define NM_RS_SETBY_PARAMS_UNUSED
#define NM_RS_SETBY_PARAMS_ARG(_setBy)
#endif

namespace NM
{
  class Vector3;
  class Matrix34;
}

namespace ART
{
     class NmRsCharacter;
#if ART_ENABLE_BSPY
    class NmRsSpy;
#endif

# define nmrsSetAngles(eff,l1,l2,tw) {(eff)->setDesiredLean1(l1);(eff)->setDesiredLean2(l2);(eff)->setDesiredTwist(tw);}
# define nmrsSetLean1(eff,l1) ((eff)->setDesiredLean1(l1))
# define nmrsSetLean2(eff,l2) ((eff)->setDesiredLean2(l2))
# define nmrsSetTwist(eff,tw) ((eff)->setDesiredTwist(tw))
# define nmrsSetAngle(eff,angle) ((eff)->setDesiredAngle(angle))

#define nmrsGetDesiredLean1(eff) ((eff)->getDesiredLean1())
#define nmrsGetDesiredLean2(eff) ((eff)->getDesiredLean2())
#define nmrsGetDesiredTwist(eff) ((eff)->getDesiredTwist())
#define nmrsGetDesiredAngle(eff) ((eff)->getDesiredAngle())

#define nmrsGetActualLean1(eff) ((eff)->getActualLean1())
#define nmrsGetActualLean2(eff) ((eff)->getActualLean2())
#define nmrsGetActualTwist(eff) ((eff)->getActualTwist())
#define nmrsGetActualAngle(eff) ((eff)->getActualAngle())

    /**
     * base class for all effectors
     */
    class NmRsEffectorBase
    {
    public:

      NmRsEffectorBase(bool is3Dof, ART::MemoryManager *services, int jointIndex, int jointTypeIndex) : 
        m_artMemoryManager(services),
        m_character(0),
        m_jointIndex(jointIndex),
        m_jointTypeIndex(jointTypeIndex)
      { 
        state.m_is3Dof = is3Dof; 
        state.m_zeroPoseStored = false; 
        state.m_masked = false; 
        state.m_partOfGroundedChain = false; 
        state.m_limitsSet = false;
        state.m_limitsSetThisFrame = false;
      }
      virtual ~NmRsEffectorBase(){}

      virtual void init(NmRsCharacter *character)=0;
      virtual void term()=0;
      virtual void preStep(float dtClamped, float minMuscleDamping)=0;
      virtual void postStep()=0;

      inline bool is3DofEffector() const { return state.m_is3Dof; }
      virtual rage::phJoint* getJoint() const=0;


      /**
      * masking control
      */
      inline void maskEffector() { state.m_masked = true; }
      inline void clearMask() { state.m_masked = false; }
      inline bool isMasked() const { return state.m_masked; }
#if ART_ENABLE_BSPY
      inline BehaviourID currentBehaviour() const { if (m_character) return m_character->m_currentBehaviour; else return bvid_Invalid; }
      inline const char* currentSubBehaviour() const { if (m_character) return m_character->m_currentSubBehaviour; else return 0; }
      inline int currentFrame() const { if (m_character) return m_character->m_currentFrame; else return -1; }
#endif

      /**
      * Injury control
      */
      inline bool isInjured() const {return m_injuryAmount>0.f;}
      inline float getInjuryAmount() const {return m_injuryAmount;}

      virtual void setInjured(float injuryAmount NM_RS_SETBY_PARAMS_DEFAULTS) = 0;
      virtual void applyInjuryToEffector(float &effeciveMStrength,float &effeciveMDamping)=0;

      inline bool isPartOfGroundedChain() const {return state.m_partOfGroundedChain;}
      inline void setIsPartOfGroundedChain(bool isPart) {state.m_partOfGroundedChain = isPart;}

      rage::Vector3 getJointPosition() const;

      void getMatrix1(rage::Matrix34 &mat) const;
      void getMatrix2(rage::Matrix34 &mat) const;

      void getMatrixIncomingTransform1(rage::Matrix34 &mat, rage::Matrix34 &mat1, rage::Vector3 *rotVel = NULL) const;
      void getMatrixIncomingTransform2(rage::Matrix34 &mat, rage::Matrix34 &mat2) const;

      rage::phArticulatedBodyPart *getParentPart() const;
      rage::phArticulatedBodyPart *getChildPart() const;

      virtual int getParentIndex() const=0;
      virtual int getChildIndex() const=0;

      bool getJointQuaternionFromIncomingTransform(NMutils::NMVector4Ptr q, IncomingTransformSource transformSource = kITSourceCurrent)const ;
      bool getJointQuaternionFromIncomingTransform_uncached(NMutils::NMVector4Ptr q, IncomingTransformSource transformSource = kITSourceCurrent)const ;

      bool getJointQuatPlusVelFromIncomingTransform(rage::Quaternion &quat, rage::Vector3 &rotVel) const;

      void setDriveState(rage::phJoint::driveState state) { getJoint()->SetDriveState(state); }

      virtual void getQuaternionFromDesiredAngles(rage::Quaternion &q) const=0;
      virtual void getQuaternionFromDesiredRawAngles(rage::Quaternion &q) const=0;

#if ART_ENABLE_BSPY && NM_BSPY_JOINT_DEBUG
      virtual void renderDebugDraw()=0;
#endif

      // apply incoming transforms as desired angle(s)
      virtual void activePose(int transformSource = (int)kITSourceCurrent)=0;

      // hold the current pose, set actual => desired
      virtual void holdPose()=0;

      // store incoming transform frame as zero pose angles
      virtual bool storeZeroPose()=0;

      // returns true if storeZeroPose() was successfully called on this effector
      virtual bool hasStoredZeroPose() const=0;

      // clamp desired angles within limits
      virtual void clamp(float amount /* = 1 */)=0;

      // blend to the stored zero pose angles, interpolating by t;
      // 0 being current desired angles, 1 being the zero pose angles.
      // calling this with a value of 1.0 is effectively resetting the effector to the zero pose.
      virtual void blendToZeroPose(float t NM_RS_SETBY_PARAMS_DEFAULTS)=0;

      // multiply the strength and damping to relax the joint
      virtual void setRelaxation(float mult NM_RS_SETBY_PARAMS_DEFAULTS)=0;
      virtual void setRelaxation_DampingOnly(float mult NM_RS_SETBY_PARAMS_DEFAULTS)=0;

      // reset effector muscle data to default calibration
      virtual void resetEffectorCalibrations()=0;
      virtual void resetEffectorMuscleStiffness()=0;

      // reset desired angles
      virtual void resetAngles()=0;
      virtual void setStiffness(float stiffness, float dampingScale, float *muscleStiffness = NULL)=0;

      // generic base-class access to some members
      virtual void setMuscleStrength(float MuscleStrength NM_RS_SETBY_PARAMS_DEFAULTS)=0;
      virtual void setMuscleDamping(float MuscleDamping NM_RS_SETBY_PARAMS_DEFAULTS)=0;
      virtual void setMuscleStiffness(float MuscleStiffness NM_RS_SETBY_PARAMS_DEFAULTS)=0;

      virtual void setOpposeGravity(float oppose NM_RS_SETBY_PARAMS_DEFAULTS)=0;

      virtual float getMuscleStrength() const=0;
      virtual float getMuscleDamping() const=0;
      virtual float getMuscleStiffness() const=0;
      virtual float getOpposeGravity() const=0;

      int getJointIndex() const { return m_jointIndex; }

#if NM_RUNTIME_LIMITS
      // runtime limit modification.
      virtual void cacheCurrentLimits()=0;
      virtual void disableLimits()=0;
      virtual void restoreLimits(float step = 0.f)=0;
      virtual void setLimitsToPose(bool useActual = false, float margin = 0.f)=0;
#endif


#if ART_ENABLE_BSPY
      void setNameToken(bSpy::bSpyStringToken tkn) { m_nameToken = tkn; }
      bSpy::bSpyStringToken getNameToken() const { return m_nameToken; }

      virtual void sendDescriptor() = 0;
      virtual void sendUpdate() = 0;

#endif // ART_ENABLE_BSPY

    protected:

      /**
      * set all transient data to default values, performed during insertAgent
      */
      virtual void initialiseData();

      /**
      * calculate and cache the joint matrices first time they are required in a frame
      */
      void calculateJointMatrixCache() const;

      /**
      * calculate and cache the joint quat from incoming tms first time they are required in a frame
      */
      void calculateJointQuatFromITMCache(IncomingTransformSource transformSource = kITSourceCurrent) const;


      rage::Matrix34              m_matrix1Cache, 
        m_matrix2Cache;

      NMutils::NMVector4          m_jointQuatCache;

      ART::MemoryManager         *m_artMemoryManager;
      NmRsCharacter              *m_character;

      float                       m_injuryAmount; 

      int                         m_jointIndex,       // index in the master list of effectors
                                  m_jointTypeIndex;   // index in the type-specific list (eg. #3 of 4 1DOF effectors)

#if ART_ENABLE_BSPY
      bSpy::bSpyStringToken       m_nameToken;
#endif // ART_ENABLE_BSPY

      struct EffectorBitField
      {
        bool                      m_is3Dof:1;           // otherwise, 1 dof

        bool                      m_zeroPoseStored:1,
                                  m_masked:1;           // if true, disallow any 'set' commands

        bool                      m_jointMatrixCacheValid:1;
        bool                      m_jointQuatFromITMValid:1,
                                  m_jointQuatFromITMSuccess:1;
        bool                      m_partOfGroundedChain:1;

        bool                      m_limitsSetThisFrame:1;
        bool                      m_limitsSet:1;

        bool                      m_actualAnglesValid:1;
      }                           state;
    };


    /**
    * Rockstar 1-Dof Effector
    * Joint effector providing one degree of freedom, like a hinge
    */
    class NmRs1DofEffector : public NmRsEffectorBase
    {
    public:

      NmRs1DofEffector(ART::MemoryManager* services, rage::phJoint1Dof* joint, int jointIndex, int jointTypeIndex, NmRs1DofEffectorParams &info );
      virtual ~NmRs1DofEffector() {}

      void init(NmRsCharacter *character);
      void term();
      void preStep(float dtClamped, float minMuscleDamping);
      void postStep();

      /**
      * save effector settings into the passed shadow state
      */
      void saveToShadow(Shadow1Dof& state) const;

      /**
      * load desired angles from the passed state
      */
      void loadFromShadow(Shadow1Dof& state);

      void setInjured(float injuryAmount NM_RS_SETBY_PARAMS_DEFAULTS);
      void applyInjuryToEffector(float &effeciveMStrength,float &effeciveMDamping);


      inline rage::phJoint1Dof* get1DofJoint() const { return m_1DofJoint; }
      virtual rage::phJoint* getJoint() const { return m_1DofJoint; }

      virtual int getParentIndex() const { return m_info.parentIndex; }
      virtual int getChildIndex() const { return m_info.childIndex; }

      virtual void getQuaternionFromDesiredAngles(rage::Quaternion &q) const;
      virtual void getQuaternionFromDesiredRawAngles(rage::Quaternion &q) const;

#if ART_ENABLE_BSPY && NM_BSPY_JOINT_DEBUG
      virtual void renderDebugDraw();
#endif

      void activeAnimInfo(float timeStep, float *angle, float *angleVel) const;
      virtual void activePose(int transformSource = (int)kITSourceCurrent);
      virtual void holdPose();
      virtual bool storeZeroPose();
      virtual void blendToZeroPose(float t NM_RS_SETBY_PARAMS_DEFAULTS);
      virtual bool hasStoredZeroPose() const { return state.m_zeroPoseStored; }
      virtual void setRelaxation(float mult NM_RS_SETBY_PARAMS_DEFAULTS);
      virtual void setRelaxation_DampingOnly(float mult NM_RS_SETBY_PARAMS_DEFAULTS);
      virtual void resetEffectorCalibrations();
      virtual void resetEffectorMuscleStiffness();
      virtual void resetAngles();
      virtual void setStiffness(float stiffness, float damping, float *muscleStiffness = NULL);
      virtual void clamp(float /*amount = 1*/) {};

      void setDesiredAngleRelative(float angle);
      float getDesiredAngleFromRelative(float angle) const;

      void ApplyTorque(float torque);
      void ApplyAngImpulse(float impulse);

      void setDesiredAngleZeroRelative(float angle);

      const NmRs1DofEffectorParams& getInfo() const { return m_info; }

      // blend to supplied angle, interpolating by t;
      // 0 being current desired angles, 1 being the supplied angle.
      void blendToPose(float angle, float t);

#if NM_RUNTIME_LIMITS
      // runtime limit modification.
      void setLimits(float min, float max);
      void disableLimits();
      void restoreLimits(float step = 0.f);
      void cacheCurrentLimits();
      void setLimitsToPose(bool useActual = false, float margin = 0.f);
#endif

#if ART_ENABLE_BSPY
      void sendDescriptor();
      void sendUpdate();
#endif // ART_ENABLE_BSPY

      float getZeroPoseAngle() const { return m_zeroPoseAngle; }

    protected:

      /**
      * set all transient data to default values, performed during insertAgent
      */
      virtual void initialiseData();

      // update the current angle values
      void updateCurrentAngles() const;
      NmRs1DofEffectorParams    &m_info;
      rage::phJoint1Dof*        m_1DofJoint;
      float                     m_zeroPoseAngle;

      float                     m_minLimitCache;
      float                     m_maxLimitCache;

      // declare parameters and accessors
#define NM_RS_EFFECTOR_CLASS_BODY
# include "NmRsEffectorAutoreg.inl"
#include "common\NmRs1DofEffector.inl"
#undef NM_RS_EFFECTOR_CLASS_BODY
    };


    /**
    * Rockstar 3-Dof Effector
    * Joint effector providing three degrees of freedom
    */
    class NmRs3DofEffector : public NmRsEffectorBase
    {
    public:

      NmRs3DofEffector(ART::MemoryManager* services, rage::phJoint3Dof* joint, int jointIndex, int jointTypeIndex, NmRs3DofEffectorParams &info );
      virtual ~NmRs3DofEffector() {}

      void init(NmRsCharacter *character);
      void term();
      void preStep(float dtClamped, float minMuscleDamping);
      void postStep();

      /**
      * save effector settings into the passed shadow state
      */
      void saveToShadow(Shadow3Dof& state) const;

      /**
      * load desired angles from the passed state
      */
      void loadFromShadow(Shadow3Dof& state);

      void setInjured(float injuryAmount NM_RS_SETBY_PARAMS_DEFAULTS);
      void applyInjuryToEffector(float &effeciveMStrength,float &effeciveMDamping);

      inline rage::phJoint3Dof* get3DofJoint() const { return m_3DofJoint; }
      virtual rage::phJoint* getJoint() const { return m_3DofJoint; }

      virtual int getParentIndex() const { return m_info.parentIndex; }
      virtual int getChildIndex() const { return m_info.childIndex; }

      virtual void getQuaternionFromDesiredAngles(rage::Quaternion &q) const;
      virtual void getQuaternionFromDesiredRawAngles(rage::Quaternion &q) const;

      // Clamp joint rotation to limits
      //  * q/leanTwist are *raw* joint angles
      //  * amount scales limit extents
      float clampRawLeanTwist(rage::Quaternion& q, float amount = 1) const;
      float clampRawLeanTwist(rage::Vector3& leanTwist, float amount = 1) const;

#if ART_ENABLE_BSPY && NM_BSPY_JOINT_DEBUG
      virtual void renderDebugDraw();
      void addArrow(float l1, float l2, const rage::Matrix34 &mat, float scale, const rage::Vector3 &col);
#endif

      void activeAnimInfo(float timeStep, float *lean1, float *lean2, float *twist, float *lean1Vel, float *lean2Vel, float *twistVel) const;
      virtual void activePose(int transformSource = (int)kITSourceCurrent);
      virtual void holdPose();
      virtual bool storeZeroPose();
      virtual void blendToZeroPose(float t NM_RS_SETBY_PARAMS_DEFAULTS);
      virtual bool hasStoredZeroPose() const { return state.m_zeroPoseStored; }
      virtual void setRelaxation(float mult NM_RS_SETBY_PARAMS_DEFAULTS);
      virtual void setRelaxation_DampingOnly(float mult NM_RS_SETBY_PARAMS_DEFAULTS);
      virtual void resetEffectorCalibrations();
      virtual void resetEffectorMuscleStiffness();
      virtual void resetAngles();
      virtual void setStiffness(float stiffness, float dampingScale, float *muscleStiffness = NULL);
      virtual void clamp(float amount = 1);

      void ApplyTorque(const rage::Vector3 &torque);
      void ApplyAngImpulse(const rage::Vector3 &angImpulse);

      void clamp(rage::Vector3& leanTwist, float amount /* = 1 */) const;

      void getTwistAndSwingFromRawTwistAndSwing(rage::Vector3 &ts, const rage::Vector3 &tsRaw) const;
      void getRawTwistAndSwingFromTwistAndSwing(rage::Vector3 &tsRaw, const rage::Vector3 &ts) const;

      void setDesiredLean1Relative(float angle);
      void setDesiredLean2Relative(float angle);
      void setDesiredTwistRelative(float angle);

      float getDesiredLean1FromRelative(float angle) const;
      float getDesiredLean2FromRelative(float angle) const;
      float getDesiredTwistFromRelative(float angle) const;

      void setDesiredLean1ZeroRelative(float angle);
      void setDesiredLean2ZeroRelative(float angle);
      void setDesiredTwistZeroRelative(float angle);

      const NmRs3DofEffectorParams& getInfo() const { return m_info; }

      float extentToLimit() const;

      // blend to supplied pose angles, interpolating by t;
      // 0 being current desired angles, 1 being the supplied pose angles.
      void blendToPose(float twist, float lean1, float lean2, float t);

#if NM_RUNTIME_LIMITS
      void setLimits(float lean1, float lean2, float twist);
      void setTwistLimit(float twist);
      void setLean1Limit(float lean1);
      void setLean2Limit(float lean2);
      void disableLimits();
      void restoreLimits(float step = 0.f);
      void cacheCurrentLimits();
      void setLimitsToPose(bool useActual = false, float margin = 0.f);
#endif

#if ART_ENABLE_BSPY
      void sendDescriptor();
      void sendUpdate();
#endif // ART_ENABLE_BSPY

      rage::Vector3 getZeroPoseAngles() const { return m_zeroPoseAngles; }

    protected:

      /**
      * set all transient data to default values, performed during insertAgent
      */
      virtual void initialiseData();

      // update the current angle values
      void updateCurrentAngles() const;

      rage::Vector3             m_zeroPoseAngles;
      rage::phJoint3Dof*        m_3DofJoint;
      NmRs3DofEffectorParams    &m_info;

      float                     m_lean1LimitCache;//Lean1Extent from setup model.  i.e before any runtime limits applied
      float                     m_lean2LimitCache;//Lean2Extent from setup model.  i.e before any runtime limits applied
      float                     m_twistLimitCache;//TwistExtent from setup model.  i.e before any runtime limits applied

      // declare parameters and accessors
#define NM_RS_EFFECTOR_CLASS_BODY
# include "NmRsEffectorAutoreg.inl"
#include "common\NmRs3DofEffector.inl"
#undef NM_RS_EFFECTOR_CLASS_BODY
    };

    /**
    * member function typedef for calling arbitrary effector functions either with or
    * without a float argument
    */
    typedef void (NmRs3DofEffector::*Effector3DFunctionNoArgs)();
    typedef void (NmRs3DofEffector::*Effector3DFunctionFloatArg)(float arg NM_RS_SETBY_PARAMS);
    typedef void (NmRs3DofEffector::*Effector3DFunctionIntArg)(int arg);
    typedef void (NmRs1DofEffector::*Effector1DFunctionNoArgs)();
    typedef void (NmRs1DofEffector::*Effector1DFunctionFloatArg)(float arg NM_RS_SETBY_PARAMS);
    typedef void (NmRs1DofEffector::*Effector1DFunctionIntArg)(int arg);

    /*
    *  replacement functions for new mask system
    */
    void callMaskedEffectorFunctionFloatArg(
      NmRsCharacter* character, 
      BehaviourMask mask,
      float floatValue,
      Effector1DFunctionFloatArg oneDofFn,
      Effector3DFunctionFloatArg threeDofFn);

#if 0
    void callMaskedEffectorFunctionIntArg(
      NmRsCharacter* character, 
      BehaviourMask mask,
      int intValue,
      Effector1DFunctionIntArg oneDofFn,
      Effector3DFunctionIntArg threeDofFn);
#endif

    void callMaskedEffectorFunctionNoArgs(
      NmRsCharacter* character, 
      BehaviourMask mask,
      Effector1DFunctionNoArgs oneDofFn,
      Effector3DFunctionNoArgs threeDofFn);
}

#endif // NM_RS_EFFECTORS_H

