//
// pheffects/explosiontype.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHEFFECTS_EXPLOSIONTYPE_H
#define PHEFFECTS_EXPLOSIONTYPE_H

namespace rage {

class phExplosionType
{
public:
	phExplosionType() :
	  // I'm not exactly sure what a good value for this is yet.
	  // (moved this comment from instbehaviorexplosion.h. /FF)
			m_DeactivationRadiusSpeed(10.0f),
			m_InitialRadiusSpeed(270.0f),
			m_DecayFactor(-3.5f),
			m_ForceFactor(2.0f)
	  {
		  ;
	  }

	phExplosionType(const phExplosionType& phExplType) :
		m_DeactivationRadiusSpeed(phExplType.m_DeactivationRadiusSpeed),
		m_InitialRadiusSpeed(phExplType.m_InitialRadiusSpeed),
		m_DecayFactor(phExplType.m_DecayFactor),
		m_ForceFactor(phExplType.m_ForceFactor)
	{

	}

	const phExplosionType& operator= (const phExplosionType& phExplType)
	{
		m_DeactivationRadiusSpeed	= phExplType.m_DeactivationRadiusSpeed;
		m_InitialRadiusSpeed		= phExplType.m_InitialRadiusSpeed;
		m_DecayFactor				= phExplType.m_DecayFactor;
		m_ForceFactor				= phExplType.m_ForceFactor;
		return phExplType;
	}

	bool operator== (const phExplosionType& phExplType) const
	{
		return	((m_DeactivationRadiusSpeed == phExplType.m_DeactivationRadiusSpeed) &&
				(m_InitialRadiusSpeed == phExplType.m_InitialRadiusSpeed) &&
				(m_DecayFactor == phExplType.m_DecayFactor) &&
				(m_ForceFactor == phExplType.m_ForceFactor));
	}

	bool operator!= (const phExplosionType& phExplType) const
	{
		return !(*this == phExplType);
	}

	// PURPOSE:	The threshold value for determining if the radius speed
	//			has dropped down to a value low enough to stop the explosion.
	// NOTES:	This used to be a hardcoded value in
	//			phInstBehaviorExplosion::IsActive(), but I moved it 
	//			here since it was needed elsewhere and I didn't want to
	//			duplicate it. /FF
	float m_DeactivationRadiusSpeed;

	float m_InitialRadiusSpeed, m_DecayFactor;
	float m_ForceFactor;
};

} // namespace rage

#endif
