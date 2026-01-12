// 
// physics/forcesolverartdma.h 
// 
// Copyright (C) 1999-2008 Rockstar Games.  All Rights Reserved. 
// 

#ifndef PHARTICULATED_SPUDMA_H 
#define PHARTICULATED_SPUDMA_H 

namespace rage {

	class phArticulatedBody;
	class phArticulatedBodyType;
	class phJoint;
	class phJointType;
	class phJoint3Dof;
	class phJoint3DofType;
	class phArticulatedBodyPart;
	class phArticulatedBodyPartType;
	class phArticulatedCollider;
	class Mat34V;
	class Vec4V;
	class phPhaseSpaceVector;

	template<typename JointBaseT, typename JointT, typename LinkT, typename JointBaseTypeT, typename JointTypeT>

	class ArticulatedDma
	{
	public:

		void	Init(u8* scratchPad, u32 scratchPadSize);

		bool	FetchToLs(phArticulatedCollider* colliderLs, phArticulatedCollider* colliderCopy = NULL);
		void	WriteFromLs();

		template <typename _T> _T* Allocate(int count = 1);

	private:

		u8*							m_Scratchpad;
		u32							m_ScratchTop;
		u32							m_ScratchBottom;
		u32							m_ScratchSize;  // diagnostic only
		u32							m_numLinks;     // diagnostic only

		phArticulatedCollider*      m_Collider;
		phArticulatedBody*			m_BodyMm;
		phArticulatedBodyType*		m_BodyTypeMm;

		int*						m_LimitsJointIndexMm;
		int*						m_LimitsDofIndexMm;
		float*						m_LimitsExcessHardMm;
		float*						m_LimitsResponseMm;
		float*						m_AccumJointImpulseMm;
		Vec3V*						m_SavedLinearVelocitiesMm;
		Vec3V*						m_SavedAngularVelocitiesMm;
		phPhaseSpaceVector*			m_PartVelocitiesMm;
		phPhaseSpaceVector*			m_VelocitiesToPropUpMm;
		Vec4V*						m_AngInertiaXYZmassWMm;
		int*						m_ComponentToLinkIndexMm;
		JointBaseT**				m_JointArrayMm;
		LinkT**						m_BodyPartArrayMm;
		JointBaseTypeT**			m_JointTypeArrayMm;
	};


	typedef ArticulatedDma<phJoint,     phJoint3Dof,     phArticulatedBodyPart,
		phJointType, phJoint3DofType>				ArticulatedDmaFull;


} // namespace rage

#endif // PHARTICULATED_SPUDMA_H 