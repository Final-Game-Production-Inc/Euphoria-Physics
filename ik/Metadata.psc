<?xml version="1.0"?>
<ParserSchema xmlns="http://www.rockstargames.com/RageParserSchema">

  <structdef type="CLegIkSolver::Tunables::InterpolationSettings">
    <float name="m_Rate" init="0.5f" min="0.0f" max="100.0f" description="Linear: The rate that the value will be lerped toward the target. Acceleration based: The maximum rate that the value will be lerped toward the target" />
    <bool name="m_AccelerationBased" init="false" description="If true, use acceleration based interpolation"/>
    <bool name="m_ScaleAccelWithDelta" init="false" description="If true, use acceleration based interpolation"/>
    <bool name="m_ZeroRateOnDirectionChange" init="false" description="If true, use acceleration based interpolation"/>
    <float name="m_AccelRate" init="0.5f" min="0.0f" max="100.0f" description="Acceleration based only: the rate of accelration towards the maximum rate" />
  </structdef>
  
  <structdef type="CLegIkSolver::Tunables" autoregister="true" base="CTuning">
    <struct name="m_PelvisInterp" type="CLegIkSolver::Tunables::InterpolationSettings" description="The interpolation settings for the pelvis height"/>
    <struct name="m_PelvisInterpMoving" type="CLegIkSolver::Tunables::InterpolationSettings" description="The interpolation settings for the pelvis height"/>
    <struct name="m_PelvisInterpOnDynamic" type="CLegIkSolver::Tunables::InterpolationSettings" description="The interpolation settings for the pelvis height"/>
    <struct name="m_FootInterp" type="CLegIkSolver::Tunables::InterpolationSettings" description="The interpolation settings for the foot height"/>
    <struct name="m_FootInterpIntersecting" type="CLegIkSolver::Tunables::InterpolationSettings" description="The interpolation settings for the foot height when intersecting"/>
    <struct name="m_FootInterpMoving" type="CLegIkSolver::Tunables::InterpolationSettings" description="The interpolation settings for the foot height"/>
    <struct name="m_FootInterpIntersectingMoving" type="CLegIkSolver::Tunables::InterpolationSettings" description="The interpolation settings for the foot height when intersecting"/>
    <struct name="m_FootInterpOnDynamic" type="CLegIkSolver::Tunables::InterpolationSettings" description="The interpolation settings for the foot height"/>

    <struct name="m_StairsPelvisInterp" type="CLegIkSolver::Tunables::InterpolationSettings" description="The interpolation settings for the pelvis height"/>
    <struct name="m_StairsPelvisInterpMoving" type="CLegIkSolver::Tunables::InterpolationSettings" description="The interpolation settings for the pelvis height"/>
    <struct name="m_StairsPelvisInterpCoverAim" type="CLegIkSolver::Tunables::InterpolationSettings" description="The interpolation settings for the pelvis height"/>
    <struct name="m_StairsFootInterp" type="CLegIkSolver::Tunables::InterpolationSettings" description="The interpolation settings for the foot height"/>
    <struct name="m_StairsFootInterpIntersecting" type="CLegIkSolver::Tunables::InterpolationSettings" description="The interpolation settings for the foot height when intersecting"/>
    <struct name="m_StairsFootInterpCoverAim" type="CLegIkSolver::Tunables::InterpolationSettings" description="The interpolation settings for the foot height"/>

    <float name="m_UpStairsPelvisMaxDeltaZMoving" init="0.0" min="0.0f" max="10.0f" description="The maximum distance upward the pelvis can be adjusted by the ik whilst the ped is on stairs" />
    <float name="m_UpStairsPelvisMaxNegativeDeltaZMoving" init="-0.37" min="-10.0f" max="0.0f" description="The minimum distance downward the pelvis can be adjusted by the ik whilst the ped is on stairs" />
    <float name="m_DownStairsPelvisMaxDeltaZMoving" init="0.0" min="0.0f" max="10.0f" description="The maximum distance upward the pelvis can be adjusted by the ik whilst the ped is on stairs" />
    <float name="m_DownStairsPelvisMaxNegativeDeltaZMoving" init="-0.37" min="-10.0f" max="0.0f" description="The minimum distance downward the pelvis can be adjusted by the ik whilst the ped is on stairs" />

    <float name="m_StairsPelvisMaxNegativeDeltaZCoverAim" init="-0.20" min="-30.0f" max="0.0f" description="The minimum distance downward the pelvis can be adjusted by the ik whilst the ped is on stairs and in cover aim" />

    <float name="m_VelMagStairsSpringMin" init="0.0f" min="0.0f" max="100.0f" description="Multiplies the strength of the peds capsule spring whilst on stairs." />
    <float name="m_VelMagStairsSpringMax" init="4.0f" min="0.0f" max="100.0f" description="Multiplies the strength of the peds capsule spring whilst on stairs." />
    <float name="m_StairsSpringMultiplierMin" init="1.0f" min="0.0f" max="10.0f" description="Multiplies the strength of the peds capsule spring whilst on stairs." />
    <float name="m_StairsSpringMultiplierMax" init="1.0f" min="0.0f" max="10.0f" description="Multiplies the strength of the peds capsule spring whilst on stairs." />
  </structdef>

  <!-- Enable below if IK_QUAD_LEG_SOLVER_ENABLE -->
  <!--
  <structdef type="CQuadLegIkSolver::Tunables" autoregister="true" base="CTuning">
    <float name="m_RootBlendInRate" init="0.2f" min="0.0f" max="5.0f" description="" />
    <float name="m_RootBlendOutRate" init="0.2f" min="0.0f" max="5.0f" description="" />

    <float name="m_RootInterpolationRate" init="0.4f" min="0.0f" max="100.0f" description="" />
    <float name="m_RootGroundNormalInterpolationRate" init="5.0f" min="0.0f" max="100.0f" description="" />
    <float name="m_RootMinLimitScale" init="-0.075f" min="-2.0f" max="2.0f" description="" />
    <float name="m_RootMaxLimitScale" init="0.075f" min="-2.0f" max="2.0f" description="" />

    <float name="m_LegBlendInRate" init="0.2f" min="0.0f" max="5.0f" description="" />
    <float name="m_LegBlendOutRate" init="0.2f" min="0.0f" max="5.0f" description="" />
    <float name="m_LegMinLengthScale" init="0.4f" min="0.0f" max="1.0f" description="" />
    <float name="m_LegMaxLengthScale" init="0.96f" min="0.0f" max="1.0f" description="" />

    <float name="m_EffectorInterpolationRate" init="1.3f" min="0.0f" max="100.0f" description="" />
    <float name="m_EffectorGroundNormalInterpolationRate" init="5.0f" min="0.0f" max="100.0f" description="" />

    <float name="m_ShapeTestRadius" init="0.07f" min="0.0f" max="1.0f" description="" />
    <float name="m_ShapeTestLengthScale" init="0.35f" min="0.0f" max="1.0f" description="" />

    <float name="m_MaxIntersectionHeightScale" init="0.40f" min="0.0f" max="1.0f" description="" />
    <float name="m_ClampNormalLimit" init="0.7835f" min="-3.14f" max="3.14f" description="" />
    <float name="m_AnimHeightBlend" init="0.25f" min="0.0f" max="1.0f" description="" />
  </structdef>
  -->

</ParserSchema>
