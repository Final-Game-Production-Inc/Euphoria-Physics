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

NM_RS_RO_PARAMETER_ACTUAL(NmRs1DofEffector, float, ActualAngle)
NM_RS_RO_PARAMETER_ACTUAL(NmRs1DofEffector, float, ActualAngleVel) //mmmmNote only used in crawlLearning at moment

NM_RS_PARAMETER(NmRs1DofEffector, float, DesiredAngle, -10.0f, 10.0f, 0.0f)
NM_RS_PARAMETER(NmRs1DofEffector, float, MuscleStiffness, 0.0f, 1000.0f, 24.0f)
NM_RS_PARAMETER(NmRs1DofEffector, float, MuscleStrength, 0.0f, 1000.0f, 144.0f)
NM_RS_PARAMETER(NmRs1DofEffector, float, MuscleDamping, 0.0f, 1000.0f, 24.0f)
NM_RS_PARAMETER(NmRs1DofEffector, float, OpposeGravity, 0.0f, 10.0f, 0.0f)

NM_RS_PARAMETER_DIRECT(NmRs1DofEffector, float, ForceCap, 0.0f, 1000.0f, 300.0f)
NM_RS_PARAMETER_DIRECT(NmRs3DofEffector, float, MuscleStiffnessScaling, 1.0f, 10.0f, 0.0f)
NM_RS_PARAMETER_DIRECT(NmRs3DofEffector, float, MuscleStrengthScaling, 1.0f, 10.0f, 0.0f)
NM_RS_PARAMETER_DIRECT(NmRs3DofEffector, float, MuscleDampingScaling, 1.0f, 10.0f, 0.0f)

NM_RS_RO_PARAMETER(NmRs1DofEffector, float, MinAngle)
NM_RS_RO_PARAMETER(NmRs1DofEffector, float, MaxAngle)
NM_RS_RO_PARAMETER(NmRs1DofEffector, float, MidAngle)
NM_RS_RO_PARAMETER(NmRs1DofEffector, float, Extent)
