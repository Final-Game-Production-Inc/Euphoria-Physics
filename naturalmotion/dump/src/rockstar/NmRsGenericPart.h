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

#ifndef NM_RS_GENERIC_PART_H
#define NM_RS_GENERIC_PART_H

#include "NmRsShadows.h"
#include "NmRsUtils.h"

namespace rage
{
  class phArticulatedBodyPart;
}

namespace ART
{
  class NmRsCharacter;
  class NmRsArticulatedWrapper;

  /**
   * Encapsulation of any type of object that can be added into
   * the character for addition into a Rockstar simulator.
   */
  class NmRsGenericPart
  {
    friend class NmRsCharacter;
    friend class NmRsFrictionHelper;
  
  public:

    NmRsGenericPart(ART::MemoryManager* services, void* ptr, int partIndex, NmRsCharacter *character);
    ~NmRsGenericPart();

    /**
     * save part data into the passed shadow state
     * NOTE: there is no load, as we don't support any changes to parts inside SPU-able behaviour modules atm
     */
    void saveToShadow(ShadowGPart& state) const;

    /**
     * set all transient data to default values, performed during insertAgent
     */
    void initialiseData();

    /**
     * add the contents of this part into the given simulator.
     */
    void initialisePart();

    /**
     * 
     */
    void updateMatrices();

    /**
     * calculate and apply the linear and angular velocities required to move the part from one
     * transformation to another
     */
    void applyVelocitiesToPart(const rage::Matrix34& fromTm, const rage::Matrix34& toTm, float invDeltaTime);

    /**
     * rage::Matrix44 version of getMatrix
     */
    void getMatrix(rage::Matrix34 &mtm) const;

    /**
     * 
     */
    void setMatrix(const rage::Matrix34 &rageMatrix, bool initial = false);

    /**
     * this is like setMatrix but the old matrix is updated too.
     */
    void teleportMatrix(const rage::Matrix34 &matrix, bool initial = false);

    /**
     * Return position of this part as a rage::Vector3
     */
    rage::Vector3 getPosition() const;

    /**
     * Return linear velocity for this part as a rage::Vector3
     */
    rage::Vector3 getLinearVelocity(const rage::Vector3 *point = NULL) const;

    /**
     * Return linear velocity for this point (local to the root pos) as a rage::Vector3
     */
    rage::Vector3 getVelocityAtLocalPoint(const rage::Vector3 &point) const;

    /**
     * Return angular velocity for this part as a rage::Vector3
     */
    rage::Vector3 getAngularVelocity() const;

    /**
     * If this part is not immovable, set the linear velocity
     */
    void setLinearVelocity(float x, float y, float z);

    /**
     * If this part is not immovable, set the angular velocity
     */
    void setAngularVelocity(float x, float y, float z);

    /**
     * If this part is not immovable, set the linear velocity via a rage::Vector3
     */
    void setLinearVelocity(rage::Vector3 &linVel);

    /**
     * If this part is not immovable, set the angular velocity via a rage::Vector3
     */
    void setAngularVelocity(rage::Vector3 &angVel);

    /**
     * Sets both linear and angular simultaneously
     */
    void setVelocities(rage::Vector3 &linear, rage::Vector3 &angular);

    /**
     * Add an impulse to this part in world space
     */
    void applyImpulse(const rage::Vector3 &impulse, const rage::Vector3 &position);

    /**
     * Add a force to this part
     */
    void applyForce(const rage::Vector3 &force, rage::Vector3 *position = NULL);

    /**
     * Add torque to this part
     */
    void applyTorque(const rage::Vector3 &torque);

    /**
     * Add torque impulse to this part
     */
    void applyTorqueImpulse(const rage::Vector3 &torqueImpulse);


    /**
     * Associate a matrix.
     */
    inline void associateMatrixPointer(rage::Matrix34 *current, rage::Matrix34 *last)
    { 
      m_currentMtx = current;
      m_lastMtx = last;
    }

    inline void setInitialMatrix(const rage::Matrix34 &init, bool setInitialOnly = false)
    {     
      m_initialMtx.Set(init);

      if (!setInitialOnly)
      {
        rage::phArticulatedBodyPart* bodyPart = (rage::phArticulatedBodyPart*)m_ptr;
        rage::Matrix34 partMatrix(init);
        bodyPart->SetMatrix(partMatrix); 
      }
    }

    inline const rage::Matrix34 &getInitialMatrix() const
    {     
      return m_initialMtx;
    }

    void getBoundMatrix(rage::Matrix34 *mat) const;


    void setPartToBoneMatrix(const rage::Matrix34& mtx) { m_toBoneMatrix.Set(mtx); }
    void getPartToBoneMatrix(rage::Matrix34 &outMtx) const { outMtx.Set(m_toBoneMatrix); }
    rage::Matrix34*  getToBoneMatrix() { return &m_toBoneMatrix; }

    // accessors to check the collision toggles
    inline bool collided()                              const { return state.m_collided; }
    inline bool collidedWithOtherCharacter()            const { return state.m_collidedOtherCharacters; }
    inline bool collidedWithOwnCharacter()              const { return state.m_collidedOwnCharacter; }
    inline bool collidedWithNotOwnCharacter()           const { return state.m_collided && (state.m_collidedOtherCharacters || state.m_collidedEnvironment); }
    inline bool collidedWithEnvironment()               const { return state.m_collidedEnvironment;}
    inline bool previousCollided()                      const { return state.m_previousCollided; }
    inline bool previousCollidedWithOtherCharacter()    const { return state.m_previousCollidedOtherCharacters; }
    inline bool previousCollidedWithOwnCharacter()      const { return state.m_previousCollidedOwnCharacter; }
    inline bool previousCollidedWithNotOwnCharacter()   const { return state.m_previousCollided && (state.m_previousCollidedOtherCharacters || state.m_previousCollidedEnvironment); }
    inline bool previousCollidedWithEnvironment()       const { return state.m_previousCollidedEnvironment;}

    inline bool isCollisionEnabled()                    const { return state.m_collisionEnabled; }
    inline void setCollisionEnabled(bool enabled = true)      { state.m_collisionEnabled = enabled; }
#if NM_UNUSED_CODE
    void getCollisionZMPWithOwnCharacter(rage::Vector3 &zmpPos, rage::Vector3 &zmpNormal, float *zmpDepth = 0);
    void getCollisionZMPWithOtherCharacters(rage::Vector3 &zmpPos, rage::Vector3 &zmpNormal, float *zmpDepth = 0, rage::phInst **zmpInst = 0, int *zmpInstGenID = 0);
#endif
    void getCollisionZMPWithEnvironment(rage::Vector3 &zmpPos, rage::Vector3 &zmpNormal, float *zmpDepth = 0, rage::phInst **zmpInst = 0, int *zmpInstGenID = 0);
    void getCollisionZMPWithNotOwnCharacter(rage::Vector3 &zmpPos, rage::Vector3 &zmpNormal, float *zmpDepth = 0, rage::phInst **zmpInst = 0, int *zmpInstGenID = 0);

protected:
    inline void setFrictionMultiplier(float frictionM){m_fictionMultiplier = frictionM;};
public:
    inline float getFrictionMultiplier()  const { return m_fictionMultiplier;};

    inline void setElasticityMultiplier(float elasticityM){m_elasticityMultiplier = elasticityM;};
    inline float getElasticityMultiplier()  const { return m_elasticityMultiplier;};

    const rage::phBound* getBound() const;
    rage::phInst *getInstance() const;

    inline NmRsCharacter* getCharacter()  const { return m_character; }
    inline int getPartIndex()             const { return m_partIndex; }
    inline void* getDataPtr()             const { return m_ptr; }


#if ART_ENABLE_BSPY
    void sendDescriptor();
    void sendUpdate();

    void setNameToken(bSpy::bSpyStringToken tkn) { m_nameToken = tkn; }
    bSpy::bSpyStringToken getNameToken() const { return m_nameToken; }
#endif // ART_ENABLE_BSPY


  protected:

    inline bool isRootPart() const { return m_partIndex == 0; }

    void getArticulatedMatrix(rage::Matrix34& mtm) const;

    /**
     * called each step to allow the class to reset its collision flags
     */
    void resetCollided();

    /**
     * called by the character in response to this part being involved in a collision.
     * Note that <tt>collidee</tt> may be null if we hit something outside of our
     * virtual world knowledge (eg. some random R* stuff, not a Character-controlled object)
     */
    void handleCollision(const rage::Vector3 &pos, const rage::Vector3 &normal, float depth, NmRsGenericPart* collidee, rage::phInst *collideeInst);

    // averaging of collision results into a ZMP
    enum
    {
      zmpCalculation,
      zmpResults,
      zmpBufferCount
    };


    rage::Matrix34              m_initialMtx;
    rage::Matrix34              m_toBoneMatrix;

    rage::Vector3               m_subtreeCOM, 
                                m_subtreeCOMvel;

    rage::Vector3               m_zmpPositionOwnCharacter[zmpBufferCount];
    rage::Vector3               m_zmpNormalOwnCharacter[zmpBufferCount];
    float                       m_zmpDepthOwnCharacter[zmpBufferCount];
    float                       m_depthTotalOwnCharacter;
    rage::phInst               *m_collidedInstOwnCharacter;

    rage::Vector3               m_zmpPositionOtherCharacters[zmpBufferCount];
    rage::Vector3               m_zmpNormalOtherCharacters[zmpBufferCount];
    float                       m_zmpDepthOtherCharacters[zmpBufferCount];
    float                       m_depthTotalOtherCharacters;
    rage::phInst               *m_collidedInstOtherCharacters;
    int                         m_collidedInstGenIDOtherCharacters;

    rage::Vector3               m_zmpPositionEnvironment[zmpBufferCount];
    rage::Vector3               m_zmpNormalEnvironment[zmpBufferCount];
    float                       m_zmpDepthEnvironment[zmpBufferCount];
    float                       m_depthTotalEnvironment;
    rage::phInst               *m_collidedInstEnvironment;
    int                         m_collidedInstGenIDEnvironment;

    rage::Vector3               m_zmpPositionNotOwnCharacter;
    rage::Vector3               m_zmpNormalNotOwnCharacter;
    float                       m_zmpDepthNotOwnCharacter;

    int                         m_partIndex,
                                m_indexInLevel;
    float                       m_subtreeMass;

    NmRsCharacter              *m_character;    // owning character
    void                       *m_ptr;          // generic pointer representing the various objects the part can be
    NmRsArticulatedWrapper     *m_wrapper;      

    rage::Matrix34             *m_currentMtx, 
                               *m_lastMtx;     
    ART::MemoryManager         *m_artMemoryManager;

    // Allow us to modify the friction of this part collision 
    float                       m_fictionMultiplier;
    float                       m_elasticityMultiplier;

#if ART_ENABLE_BSPY
    bSpy::bSpyStringToken       m_nameToken;
#endif // ART_ENABLE_BSPY

    struct GenericPartBitField
    {
      bool                      m_collisionEnabled:1;
      bool                      m_collided:1,
                                m_collidedOtherCharacters:1,
                                m_collidedOwnCharacter:1,
                                m_collidedEnvironment:1,
                                m_previousCollided:1,
                                m_previousCollidedOtherCharacters:1,
                                m_previousCollidedOwnCharacter:1,
                                m_previousCollidedEnvironment:1;
      bool                      m_zmpRecalcOwnCharacter:1,    // toggled when a collision occurs
                                m_zmpRecalcOtherCharacters:1,
                                m_zmpRecalcEnvironment:1,
                                m_zmpRecalcNotOwnCharacter:1;
    } state;

  };
}

#endif // NM_RS_GENERIC_PART_H
