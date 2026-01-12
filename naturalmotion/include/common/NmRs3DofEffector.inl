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

NM_RS_RO_PARAMETER_ACTUAL(NmRs3DofEffector, float, RawLean1)
NM_RS_RO_PARAMETER_ACTUAL(NmRs3DofEffector, float, ActualLean1)
NM_RS_RO_PARAMETER_ACTUAL(NmRs3DofEffector, float, ActualLean2)
NM_RS_RO_PARAMETER_ACTUAL(NmRs3DofEffector, float, ActualTwist)
NM_RS_RO_PARAMETER_ACTUAL(NmRs3DofEffector, float, ActualLean1Vel)//mmmmNote only used in crawlLearning at moment
NM_RS_RO_PARAMETER_ACTUAL(NmRs3DofEffector, float, ActualLean2Vel)//mmmmNote only used in crawlLearning at moment
NM_RS_RO_PARAMETER_ACTUAL(NmRs3DofEffector, float, ActualTwistVel)//mmmmNote only used in crawlLearning at moment

NM_RS_PARAMETER(NmRs3DofEffector, float, DesiredLean1, -10.0f, 10.0f, 0.0f)
NM_RS_PARAMETER(NmRs3DofEffector, float, DesiredLean2, -10.0f, 10.0f, 0.0f)
NM_RS_PARAMETER(NmRs3DofEffector, float, DesiredTwist, -10.0f, 10.0f, 0.0f)
NM_RS_PARAMETER(NmRs3DofEffector, float, OpposeGravity, 0.0f, 10.0f, 0.0f)
NM_RS_PARAMETER(NmRs3DofEffector, float, MuscleStiffness, 0.0f, 1000.0f, 24.0f)
NM_RS_PARAMETER(NmRs3DofEffector, float, MuscleStrength, 0.0f, 1000.0f, 144.0f)
NM_RS_PARAMETER(NmRs3DofEffector, float, MuscleDamping, 0.0f, 1000.0f, 24.0f)

NM_RS_PARAMETER_DIRECT(NmRs3DofEffector, float, LeanForceCap, 0.0f, 1000.0f, 300.0f)
NM_RS_PARAMETER_DIRECT(NmRs3DofEffector, float, TwistForceCap, 0.0f, 1000.0f, 300.0f)
NM_RS_PARAMETER_DIRECT(NmRs3DofEffector, float, MuscleStiffnessScaling, 1.0f, 10.0f, 0.0f)
NM_RS_PARAMETER_DIRECT(NmRs3DofEffector, float, MuscleStrengthScaling, 1.0f, 10.0f, 0.0f)
NM_RS_PARAMETER_DIRECT(NmRs3DofEffector, float, MuscleDampingScaling, 1.0f, 10.0f, 0.0f)

NM_RS_RO_PARAMETER(NmRs3DofEffector, float, MinLean1)
NM_RS_RO_PARAMETER(NmRs3DofEffector, float, MaxLean1)
NM_RS_RO_PARAMETER(NmRs3DofEffector, float, MinLean2)
NM_RS_RO_PARAMETER(NmRs3DofEffector, float, MaxLean2)
NM_RS_RO_PARAMETER(NmRs3DofEffector, float, MinTwist)
NM_RS_RO_PARAMETER(NmRs3DofEffector, float, MaxTwist)

NM_RS_RO_PARAMETER(NmRs3DofEffector, float, MidLean1)
NM_RS_RO_PARAMETER(NmRs3DofEffector, float, Lean1Extent)
NM_RS_RO_PARAMETER(NmRs3DofEffector, float, MidLean2)
NM_RS_RO_PARAMETER(NmRs3DofEffector, float, Lean2Extent)
NM_RS_RO_PARAMETER(NmRs3DofEffector, float, MidTwist)
NM_RS_RO_PARAMETER(NmRs3DofEffector, float, TwistExtent)
