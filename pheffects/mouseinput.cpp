// 
// pheffects/mouseinput.cpp 
// 
// Copyright (C) 1999-2008 Rockstar Games.  All Rights Reserved. 
// 

#include "mouseinput.h"

#include "input/mouse.h"
#include "input/keyboard.h"
#include "input/keys.h"
//#include "fragment/instance.h"
#include "grcore/debugdraw.h"
#include "grcore/viewport.h"
#include "phbound/bound.h"
#include "phbound/boundbox.h"
#include "phbound/boundcapsule.h"
#include "phbound/bounddisc.h"
#include "phbound/boundcomposite.h"
#include "phbound/boundgrid.h"
#include "phbound/boundbvh.h"
#include "phbound/OptimizedBvh.h"
#include "phbullet/TriangleShape.h"
#include "pheffects/explosionmgr.h"
#include "physics/colliderdispatch.h"
#include "physics/archetype.h"
#include "physics/inst.h"
#include "physics/intersection.h"
#include "physics/simulator.h"
#include "physics/sleep.h"
#include "physics/constraintattachment.h"
#include "physics/debugPlayback.h"
#include "grprofile/drawmanager.h"
#include "system/timemgr.h"
#include "vector/colors.h"
#include "crskeleton/skeleton.h"

namespace rage {

phMouseInput*					phMouseInput::sm_ActiveInstance				= NULL;
bool							phMouseInput::sm_EnableLeftClick			= false;
bool							phMouseInput::sm_EnableRightClick			= false;
phMouseInput::LeftClickMode		phMouseInput::sm_LeftClickMode				= phMouseInput::SPRING;
bool							phMouseInput::sm_EnableLeftStickyClickMode	= false;
float							phMouseInput::sm_GrabSpringConstant			= 3.0f;
float							phMouseInput::sm_GrabSpringDamping			= 0.04f;
float							phMouseInput::sm_DefaultConstraintLength	= 0.0f;
float							phMouseInput::sm_ImpulseScale				= 8.0f;
bool							phMouseInput::sm_ImpulseScaleByMass			= true;
float							phMouseInput::sm_ImpulseBreakScale			= 1.0f;
float							phMouseInput::sm_ImpulseBulletSpeed			= 100.0f;
float							phMouseInput::sm_ImpulseBulletMass			= 10.0f;
float							phMouseInput::sm_BoxSpeed					= 80.0f;
float							phMouseInput::sm_BoxMass					= 10.0f;
phExplosionType					phMouseInput::sm_ExplosionType;
bool							phMouseInput::sm_DebugColliderUpdate		= false;
Vector3							phMouseInput::s_DampingConstants[phArchetypeDamp::NUM_DAMP_TYPES] = {};
phMouseInput::BoundFlagNameFunc	phMouseInput::sm_BoundFlagNameFunc = NULL;
bool							phMouseInput::sm_DrawSkeleton				= false;
bool							phMouseInput::sm_DrawAllCompParts			= false;
bool							phMouseInput::sm_DrawBvhLeaf				= false;
BvhRenderOpts					phMouseInput::sm_BvhRenderOpts;

static const int RIGHT_CLICK_STRING_LENGTH = 128;
static const int RIGHT_CLICK_STRING_LENGTH_EXTRA = 512;
static char s_InstPtrString[RIGHT_CLICK_STRING_LENGTH];
static char s_LevelIndexString[RIGHT_CLICK_STRING_LENGTH];
static char s_ArchetypeNameString[RIGHT_CLICK_STRING_LENGTH];
static char s_MatIdString[RIGHT_CLICK_STRING_LENGTH];
static char s_TypeFlagsString[RIGHT_CLICK_STRING_LENGTH];
static char s_TypeFlagNamesString[RIGHT_CLICK_STRING_LENGTH_EXTRA];
static char s_IncludeFlagsString[RIGHT_CLICK_STRING_LENGTH];
static char s_IncludeFlagNamesString[RIGHT_CLICK_STRING_LENGTH_EXTRA];
static char s_ComponentNumberString[RIGHT_CLICK_STRING_LENGTH];
static char s_PartNumberString[RIGHT_CLICK_STRING_LENGTH];
static char s_PartTypeString[RIGHT_CLICK_STRING_LENGTH];
static char s_PartNormalString[RIGHT_CLICK_STRING_LENGTH];
static char s_BoundPtrString[RIGHT_CLICK_STRING_LENGTH];
static char s_BoundTypeString[RIGHT_CLICK_STRING_LENGTH];
static char s_MaterialString[RIGHT_CLICK_STRING_LENGTH];
static char s_CompositeBoundCountString[RIGHT_CLICK_STRING_LENGTH];
static char s_PartCountString[RIGHT_CLICK_STRING_LENGTH];
static char s_PartBoundPtrString[RIGHT_CLICK_STRING_LENGTH];
static char s_PartBoundTypeString[RIGHT_CLICK_STRING_LENGTH];
static char s_CompositeTypeFlagsString[RIGHT_CLICK_STRING_LENGTH];
static char s_CompositeTypeFlagNamesString[RIGHT_CLICK_STRING_LENGTH_EXTRA];
static char s_CompositeIncludeFlagsString[RIGHT_CLICK_STRING_LENGTH];
static char s_CompositeIncludeFlagNamesString[RIGHT_CLICK_STRING_LENGTH_EXTRA];
static char s_ColliderPtrString[RIGHT_CLICK_STRING_LENGTH];
static char s_SleepModeString[RIGHT_CLICK_STRING_LENGTH];
static char s_AsleepTicksString[RIGHT_CLICK_STRING_LENGTH];
static char s_MotionlessTicksString[RIGHT_CLICK_STRING_LENGTH];
static char s_DistanceToCameraString[RIGHT_CLICK_STRING_LENGTH];
static char s_ReferenceFrameString[RIGHT_CLICK_STRING_LENGTH];
static char s_VelocityString[RIGHT_CLICK_STRING_LENGTH];
static char s_AngVelocityString[RIGHT_CLICK_STRING_LENGTH];
static char s_ApproximateRadiusString[RIGHT_CLICK_STRING_LENGTH];
static char s_LinkNumberString[RIGHT_CLICK_STRING_LENGTH];
static char s_ArchetypeMassString[RIGHT_CLICK_STRING_LENGTH];
static char s_ColliderMassString[RIGHT_CLICK_STRING_LENGTH];
static char s_ArticulatedColliderMassString[RIGHT_CLICK_STRING_LENGTH];
static char s_LinkMassString[RIGHT_CLICK_STRING_LENGTH];
static char s_ArchetypeAngularInertiaString[RIGHT_CLICK_STRING_LENGTH];
static char s_ColliderAngularInertiaString[RIGHT_CLICK_STRING_LENGTH];
static char s_LinkAngularInertiaString[RIGHT_CLICK_STRING_LENGTH];
static char s_SpeedString[RIGHT_CLICK_STRING_LENGTH];
static char s_PositionString[RIGHT_CLICK_STRING_LENGTH];
static char s_MatrixString1[RIGHT_CLICK_STRING_LENGTH];
static char s_MatrixString2[RIGHT_CLICK_STRING_LENGTH];
static char s_MatrixString3[RIGHT_CLICK_STRING_LENGTH];
//static char s_LastPositionString[RIGHT_CLICK_STRING_LENGTH];
//static char s_LastMatrixString1[RIGHT_CLICK_STRING_LENGTH];
//static char s_LastMatrixString2[RIGHT_CLICK_STRING_LENGTH];
//static char s_LastMatrixString3[RIGHT_CLICK_STRING_LENGTH];
static char s_PartMatrixString1[RIGHT_CLICK_STRING_LENGTH];
static char	s_PartMatrixString2[RIGHT_CLICK_STRING_LENGTH];
static char	s_PartMatrixString3[RIGHT_CLICK_STRING_LENGTH];
static char	s_PartPositionString[RIGHT_CLICK_STRING_LENGTH];
//static char	s_PartLastMatrixString1[RIGHT_CLICK_STRING_LENGTH];
//static char	s_PartLastMatrixString2[RIGHT_CLICK_STRING_LENGTH];
//static char	s_PartLastMatrixString3[RIGHT_CLICK_STRING_LENGTH];
//static char	s_PartLastPositionString[RIGHT_CLICK_STRING_LENGTH];

static char	s_InstPtrString_comp[RIGHT_CLICK_STRING_LENGTH];
static char	s_InstFlagMatches_LeftInclude_RightType[RIGHT_CLICK_STRING_LENGTH];
static char	s_InstFlagMatchesString_LeftInclude_RightType[RIGHT_CLICK_STRING_LENGTH_EXTRA];
static char	s_InstFlagMatches_LeftType_RightInclude[RIGHT_CLICK_STRING_LENGTH];
static char	s_InstFlagMatchesString_LeftType_RightInclude[RIGHT_CLICK_STRING_LENGTH_EXTRA];
static char	s_FlagMatches_LeftPartInclude_RightType[RIGHT_CLICK_STRING_LENGTH];
static char	s_FlagMatchesString_LeftPartInclude_RightType[RIGHT_CLICK_STRING_LENGTH_EXTRA];
static char	s_FlagMatches_LeftPartType_RightInclude[RIGHT_CLICK_STRING_LENGTH];
static char	s_FlagMatchesString_LeftPartType_RightInclude[RIGHT_CLICK_STRING_LENGTH_EXTRA];
static char	s_FlagMatches_LeftInclude_RightPartType[RIGHT_CLICK_STRING_LENGTH];
static char	s_FlagMatchesString_LeftInclude_RightPartType[RIGHT_CLICK_STRING_LENGTH_EXTRA];
static char	s_FlagMatches_LeftType_RightPartInclude[RIGHT_CLICK_STRING_LENGTH];
static char	s_FlagMatchesString_LeftType_RightPartInclude[RIGHT_CLICK_STRING_LENGTH_EXTRA];
static char	s_PartFlagMatches_LeftInclude_RightType[RIGHT_CLICK_STRING_LENGTH];
static char	s_PartFlagMatchesString_LeftInclude_RightType[RIGHT_CLICK_STRING_LENGTH_EXTRA];
static char	s_PartFlagMatches_LeftType_RightInclude[RIGHT_CLICK_STRING_LENGTH];
static char	s_PartFlagMatchesString_LeftType_RightInclude[RIGHT_CLICK_STRING_LENGTH_EXTRA];
static char	s_LevelIndexString_comp[RIGHT_CLICK_STRING_LENGTH];
static char	s_ArchetypeNameString_comp[RIGHT_CLICK_STRING_LENGTH];
static char	s_TypeFlagsString_comp[RIGHT_CLICK_STRING_LENGTH];
static char	s_TypeFlagNamesString_comp[RIGHT_CLICK_STRING_LENGTH_EXTRA];
static char	s_IncludeFlagsString_comp[RIGHT_CLICK_STRING_LENGTH];
static char	s_IncludeFlagNamesString_comp[RIGHT_CLICK_STRING_LENGTH_EXTRA];
static char	s_ComponentNumberString_comp[RIGHT_CLICK_STRING_LENGTH];
static char	s_PartNumberString_comp[RIGHT_CLICK_STRING_LENGTH];
static char	s_BoundPtrString_comp[RIGHT_CLICK_STRING_LENGTH];
static char	s_BoundTypeString_comp[RIGHT_CLICK_STRING_LENGTH];
static char	s_CompositeBoundCountString_comp[RIGHT_CLICK_STRING_LENGTH];
static char	s_PartBoundPtrString_comp[RIGHT_CLICK_STRING_LENGTH];
static char	s_PartBoundTypeString_comp[RIGHT_CLICK_STRING_LENGTH];
static char	s_CompositeTypeFlagsString_comp[RIGHT_CLICK_STRING_LENGTH];
static char	s_CompositeTypeFlagNamesString_comp[RIGHT_CLICK_STRING_LENGTH_EXTRA];
static char	s_CompositeIncludeFlagsString_comp[RIGHT_CLICK_STRING_LENGTH];
static char	s_CompositeIncludeFlagNamesString_comp[RIGHT_CLICK_STRING_LENGTH_EXTRA];

static char s_LeftClickModeString[RIGHT_CLICK_STRING_LENGTH];
static char s_ScaleString[RIGHT_CLICK_STRING_LENGTH];

static int s_FrameDelay = 10;

EXT_PFD_DECLARE_GROUP(TypeFlagFilter);
EXT_PFD_DECLARE_ITEM(TypeFlag0);
EXT_PFD_DECLARE_ITEM(TypeFlag1);
EXT_PFD_DECLARE_ITEM(TypeFlag2);
EXT_PFD_DECLARE_ITEM(TypeFlag3);
EXT_PFD_DECLARE_ITEM(TypeFlag4);
EXT_PFD_DECLARE_ITEM(TypeFlag5);
EXT_PFD_DECLARE_ITEM(TypeFlag6);
EXT_PFD_DECLARE_ITEM(TypeFlag7);
EXT_PFD_DECLARE_ITEM(TypeFlag8);
EXT_PFD_DECLARE_ITEM(TypeFlag9);
EXT_PFD_DECLARE_ITEM(TypeFlag10);
EXT_PFD_DECLARE_ITEM(TypeFlag11);
EXT_PFD_DECLARE_ITEM(TypeFlag12);
EXT_PFD_DECLARE_ITEM(TypeFlag13);
EXT_PFD_DECLARE_ITEM(TypeFlag14);
EXT_PFD_DECLARE_ITEM(TypeFlag15);
EXT_PFD_DECLARE_ITEM(TypeFlag16);
EXT_PFD_DECLARE_ITEM(TypeFlag17);
EXT_PFD_DECLARE_ITEM(TypeFlag18);
EXT_PFD_DECLARE_ITEM(TypeFlag19);
EXT_PFD_DECLARE_ITEM(TypeFlag20);
EXT_PFD_DECLARE_ITEM(TypeFlag21);
EXT_PFD_DECLARE_ITEM(TypeFlag22);
EXT_PFD_DECLARE_ITEM(TypeFlag23);
EXT_PFD_DECLARE_ITEM(TypeFlag24);
EXT_PFD_DECLARE_ITEM(TypeFlag25);
EXT_PFD_DECLARE_ITEM(TypeFlag26);
EXT_PFD_DECLARE_ITEM(TypeFlag27);
EXT_PFD_DECLARE_ITEM(TypeFlag28);
EXT_PFD_DECLARE_ITEM(TypeFlag29);
EXT_PFD_DECLARE_ITEM(TypeFlag30);
EXT_PFD_DECLARE_ITEM(TypeFlag31);

EXT_PFD_DECLARE_GROUP(IncludeFlagFilter);
EXT_PFD_DECLARE_ITEM(IncludeFlag0);
EXT_PFD_DECLARE_ITEM(IncludeFlag1);
EXT_PFD_DECLARE_ITEM(IncludeFlag2);
EXT_PFD_DECLARE_ITEM(IncludeFlag3);
EXT_PFD_DECLARE_ITEM(IncludeFlag4);
EXT_PFD_DECLARE_ITEM(IncludeFlag5);
EXT_PFD_DECLARE_ITEM(IncludeFlag6);
EXT_PFD_DECLARE_ITEM(IncludeFlag7);
EXT_PFD_DECLARE_ITEM(IncludeFlag8);
EXT_PFD_DECLARE_ITEM(IncludeFlag9);
EXT_PFD_DECLARE_ITEM(IncludeFlag10);
EXT_PFD_DECLARE_ITEM(IncludeFlag11);
EXT_PFD_DECLARE_ITEM(IncludeFlag12);
EXT_PFD_DECLARE_ITEM(IncludeFlag13);
EXT_PFD_DECLARE_ITEM(IncludeFlag14);
EXT_PFD_DECLARE_ITEM(IncludeFlag15);
EXT_PFD_DECLARE_ITEM(IncludeFlag16);
EXT_PFD_DECLARE_ITEM(IncludeFlag17);
EXT_PFD_DECLARE_ITEM(IncludeFlag18);
EXT_PFD_DECLARE_ITEM(IncludeFlag19);
EXT_PFD_DECLARE_ITEM(IncludeFlag20);
EXT_PFD_DECLARE_ITEM(IncludeFlag21);
EXT_PFD_DECLARE_ITEM(IncludeFlag22);
EXT_PFD_DECLARE_ITEM(IncludeFlag23);
EXT_PFD_DECLARE_ITEM(IncludeFlag24);
EXT_PFD_DECLARE_ITEM(IncludeFlag25);
EXT_PFD_DECLARE_ITEM(IncludeFlag26);
EXT_PFD_DECLARE_ITEM(IncludeFlag27);
EXT_PFD_DECLARE_ITEM(IncludeFlag28);
EXT_PFD_DECLARE_ITEM(IncludeFlag29);
EXT_PFD_DECLARE_ITEM(IncludeFlag30);
EXT_PFD_DECLARE_ITEM(IncludeFlag31);

const char* phMouseInput::GetLeftClickModeString()
{
	return s_LeftClickModeString;
}

const char* phMouseInput::GetScaleString()
{
	return s_ScaleString;
}

#if __BANK
static const char* s_LeftClickLJNames[] = 
{
	"Spring",
	"Constraint",
	"Impulse",
	"Bullet",
#if PHMOUSEINPUT_ENABLE_BOXES
	"Box",
#endif
#if PHMOUSEINPUT_ENABLE_EXPLOSIONS
	"Explosion",
#endif
	"Activate",
	"Deactivate",
	"Compare"
};
#endif

static const char* s_LeftClickRJNames[] = 
{
	"    Spring",
	"Constraint",
	"   Impulse",
	"    Bullet",
#if PHMOUSEINPUT_ENABLE_BOXES
	"       Box",
#endif
#if PHMOUSEINPUT_ENABLE_EXPLOSIONS
	" Explosion",
#endif
	"  Activate",
	"Deactivate",
	"   Compare"
};

phMouseInput::phMouseInput()
{
	m_Mapper.Reset();
	m_Mapper.Map(IOMS_MOUSE_BUTTON, ioMouse::MOUSE_LEFT, 	m_LeftClick);
	m_Mapper.Map(IOMS_MOUSE_BUTTON, ioMouse::MOUSE_MIDDLE, 	m_MiddleClick);
	m_Mapper.Map(IOMS_MOUSE_BUTTON, ioMouse::MOUSE_RIGHT, 	m_RightClick);
	m_Mapper.Map(IOMS_KEYBOARD,		KEY_A,					m_AngularImpetus);
	m_Mapper.Map(IOMS_KEYBOARD, 	KEY_LSHIFT, 			m_ReverseImpulse);
	m_Mapper.Map(IOMS_KEYBOARD, 	KEY_LSHIFT,				m_GrabDistanceWheel);
	m_Mapper.Map(IOMS_KEYBOARD,     KEY_R, 	                m_DisconnectSprings);
	m_Mapper.Map(IOMS_KEYBOARD, 	KEY_ALT,				m_CameraControl);

	m_ExplosionMgr = NULL;
	m_NumBoxBullets = 0;
	m_CurrentBoxBullet = 0;
	m_BoxBulletInsts = NULL;
	m_BoxBulletArchetype = NULL;
	m_BoxBulletBound = NULL;
	m_GrabSpring.instLevelIndex = phInst::INVALID_INDEX;
	m_NumPlacedSprings = 0;
	m_SelectedInst = BAD_INDEX;
	m_SelectedComponent = 0;
	m_SelectedPart = 0;
	m_SelectedInstArchDamp = NULL;
	m_CompareInst = BAD_INDEX;
	m_CompareComponent = 0;
	m_ComparePart = 0;

	m_ExplosionMgr = rage_new phExplosionMgr;
	m_ExplosionMgr->Init((u16)16);

	InitBoxGun(16);

	SetActiveInstance();
}

phMouseInput::~phMouseInput()
{
	ShutdownBoxGun();

	if (m_ExplosionMgr)
	{	
		m_ExplosionMgr->Shutdown();
	}
	delete m_ExplosionMgr;
	m_ExplosionMgr = NULL;
}

void phMouseInput::Reset()
{
	DisconnectSprings();
	ResetBoxGun();
	m_SelectedInst = BAD_INDEX;
	m_SelectedComponent = 0;
	m_SelectedPart = 0;
	m_SelectedInstArchDamp = NULL;
}

void phMouseInput::DampingChanged()
{
	if(sm_ActiveInstance->m_SelectedInstArchDamp != NULL)
	{
		for(int index=0;index < phArchetypeDamp::NUM_DAMP_TYPES;index++)
		{
			BANK_ONLY(sm_ActiveInstance->m_SelectedInstArchDamp->m_DampingConstant[index] = s_DampingConstants[index]);
		}
	}
}

void phMouseInput::DisconnectSprings()
{
	// Disconnect all the springs
	for (int i = m_NumPlacedSprings - 1; i >= 0; --i)
	{
		m_PlacedSprings[i].instLevelIndex = phInst::INVALID_INDEX;

		UpdateSpring(&m_PlacedSprings[i], false, ScalarV(V_ZERO).GetIntrin128ConstRef());
	}

	m_NumPlacedSprings = 0;
}

void phMouseInput::CreateSpring(const Spring& spring)
{
	m_PlacedSprings[m_NumPlacedSprings++] = spring;
}

void phMouseInput::FixGrabForBreak(phInst* brokenInst, const atFixedBitSet<phInstBreakable::MAX_NUM_BREAKABLE_COMPONENTS>& brokenParts, phInst* newInst)
{
	FixSpringForBreak(m_GrabSpring, brokenInst, brokenParts, newInst);

	for (int spring = 0; spring < m_NumPlacedSprings; ++spring)
	{
		FixSpringForBreak(m_PlacedSprings[spring], brokenInst, brokenParts, newInst);
	}
}

void phMouseInput::FixSpringForBreak(Spring& spring, phInst* brokenInst, const atFixedBitSet<phInstBreakable::MAX_NUM_BREAKABLE_COMPONENTS>& brokenParts, phInst* newInst)
{
	if (spring.instLevelIndex == brokenInst->GetLevelIndex() && brokenParts.IsSet(spring.component))
	{
		spring.instLevelIndex = newInst->GetLevelIndex();

		if (spring.constraintHandle.IsValid())
		{
			PHCONSTRAINT->Remove(spring.constraintHandle);
			spring.constraintHandle.Reset();

			phConstraintAttachment::Params  params;
											params.instanceA = newInst;
											params.componentA = spring.component;
											params.maxSeparation = spring.attachmentPoint.Dist(spring.worldPoint);
											params.constrainRotation = false;
											params.localPosA = RCC_VEC3V(spring.localPoint);
											params.localPosB = RCC_VEC3V(spring.attachmentPoint);

			phConstraintBase * constraintBase;
			spring.constraintHandle = PHCONSTRAINT->InsertAndReturnTemporaryPointer(params, constraintBase);
			PHSIM->ActivateObject(spring.instLevelIndex,NULL,NULL,constraintBase);
		}
	}
}


void phMouseInput::GetNamesFromFlags(char* name, int maxLen, u32 flags, const char* zeroFlagString)
{
	if (sm_BoundFlagNameFunc)
	{
		bool firstOne = true;
		int charsWritten = 0;
		for (int bit = 0; bit < 32; ++bit)
		{
			int mask = 1 << bit;
			
			if (flags & mask)
			{
				const char* flagName = sm_BoundFlagNameFunc(bit);
				int flagNameLen = (int)strlen(flagName);
				if (charsWritten + flagNameLen < maxLen - 2)
				{
					if (!firstOne)
					{
						name[charsWritten++] = '|';
					}
					firstOne = false;

					strcpy(name + charsWritten, flagName);
					charsWritten += flagNameLen;
				}
				else
				{
					char temp = name[10]; // Null from write below at [10]
					strcpy(name, "*Too Many*");
					// Let the old stuff stick around for viewing if we wrote past our new message already (It only wouldn't have if our max is this low too)
					if(charsWritten > 11)
					{
						name[10] = temp;
					}
					return;
				}
			}
		}
		if(charsWritten == 0)
		{
			strcpy(name, zeroFlagString);
		}
	}
	else
	{
		strcpy(name, "* set bound flag name func *");
	}
}


phExplosionMgr* phMouseInput::GetExplosionMgr ()
{
	return m_ExplosionMgr;
}


void phMouseInput::Update(bool paused, Vec::V3Param128 timeStep, const grcViewport* viewport)
{
	if (sm_EnableLeftClick == false && sm_EnableRightClick == false)
	{
		return;
	}

	if (viewport == NULL)
	{
		viewport = grcViewport::GetCurrent();
	}

	m_Mapper.Update();

	if (m_DisconnectSprings.IsPressed())
	{
		DisconnectSprings();
	}

	if (int mouseWheel = ioMouse::GetDZ())
	{
		const float DISTANCE_PER_CLICK = 0.1f;

		if (m_LeftClick.IsDown() && m_GrabSpring.instLevelIndex != phInst::INVALID_INDEX && m_GrabDistanceWheel.IsDown())
		{
			m_GrabDistance += DISTANCE_PER_CLICK * mouseWheel;
		}
		else if (m_MiddleClick.IsDown())
		{
			sm_LeftClickMode = LeftClickMode(sm_LeftClickMode + mouseWheel);

			while (sm_LeftClickMode < 0)
			{
				sm_LeftClickMode = LeftClickMode(sm_LeftClickMode + NUM_LEFT_CLICK_MODES);
			}

			while (sm_LeftClickMode >= NUM_LEFT_CLICK_MODES)
			{
				sm_LeftClickMode = LeftClickMode(sm_LeftClickMode - NUM_LEFT_CLICK_MODES);
			}
		}
		else if (m_GrabSpring.constraintHandle.IsValid())
		{
			phConstraintAttachment* constraintAttachment = static_cast<phConstraintAttachment*>( PHCONSTRAINT->GetTemporaryPointer(m_GrabSpring.constraintHandle) );
			if(constraintAttachment)
			{
				float length = constraintAttachment->GetMaxSeparation();
				length += DISTANCE_PER_CLICK * mouseWheel;
				length = Max(0.0f, length);
				constraintAttachment->SetMaxSeparation(length);
			}
			else
			{
				m_GrabSpring.constraintHandle.Reset();
			}
		}
		else
		{
			float* scaledFloat = NULL;

			switch (sm_LeftClickMode)
			{
			case SPRING:
				scaledFloat = &sm_GrabSpringConstant;
				break;

			case IMPULSE:
				scaledFloat = &sm_ImpulseScale;
				break;

			case BULLET:
				scaledFloat = &sm_ImpulseBulletSpeed;
				break;

#if PHMOUSEINPUT_ENABLE_BOXES
			case BOX:
				scaledFloat = &sm_BoxSpeed;
				break;
#endif

#if PHMOUSEINPUT_ENABLE_EXPLOSIONS
			case EXPLOSION:
				scaledFloat = &sm_ExplosionType.m_ForceFactor;
				break;
#endif

			default:
				break;
			}

			if (scaledFloat)
			{
				*scaledFloat *= powf(2.0f, static_cast<float>(mouseWheel));
				*scaledFloat = Min(*scaledFloat, 65536.0f);
			}
		}
	}

	formatf(s_LeftClickModeString, sizeof(s_LeftClickModeString), "mode:  %s", s_LeftClickRJNames[sm_LeftClickMode]);
	switch (sm_LeftClickMode)
	{
	case SPRING:
		formatf(s_ScaleString, sizeof(s_ScaleString), "constant:  %6.3f", sm_GrabSpringConstant);
		break;
		
	case CONSTRAINT:
		if (m_GrabSpring.constraintHandle.IsValid())
		{
			phConstraintAttachment* constraintAttachment = static_cast<phConstraintAttachment*>( PHCONSTRAINT->GetTemporaryPointer(m_GrabSpring.constraintHandle) );
			if(constraintAttachment)
			{
				formatf(s_ScaleString, sizeof(s_ScaleString), "length:    %6.3f", constraintAttachment->GetMaxSeparation());
			}
			else
			{
				m_GrabSpring.constraintHandle.Reset();
			}
		}
		else
		{
			s_ScaleString[0] = '\0';
		}

		break;

	case IMPULSE:
		formatf(s_ScaleString, sizeof(s_ScaleString), "impulse:   %6.3f", sm_ImpulseScale);
		break;

	case BULLET:
		formatf(s_ScaleString, sizeof(s_ScaleString), "speed:     %6.3f", sm_ImpulseBulletSpeed);
		break;

#if PHMOUSEINPUT_ENABLE_BOXES
	case BOX:
		formatf(s_ScaleString, sizeof(s_ScaleString), "speed:     %6.3f", sm_BoxSpeed);
		break;
#endif

#if PHMOUSEINPUT_ENABLE_EXPLOSIONS
	case EXPLOSION:
		formatf(s_ScaleString, sizeof(s_ScaleString), "force:     %6.3f", sm_ExplosionType.m_ForceFactor);
		break;
#endif

	case ACTIVATE:
		formatf(s_ScaleString, sizeof(s_ScaleString), "activate");
		break;

	case DEACTIVATE:
		formatf(s_ScaleString, sizeof(s_ScaleString), "deactivate");
		break;

	case COMPARE:
		formatf(s_ScaleString, sizeof(s_ScaleString), "compare");
		break;

	case NUM_LEFT_CLICK_MODES:
		s_ScaleString[0] = '\0';
		break;
	}

	m_BoxBulletArchetype->SetMass(sm_BoxMass);

	// mouseScreen and mouseFar as where are the world space points on the near plane and far plane respectively which project to the current mouse cursor location.
	Vector3 mouseScreen, mouseFar;

	// Get the raw mouse position.
	const float  mouseScreenX = static_cast<float>(ioMouse::GetX());
	const float  mouseScreenY = static_cast<float>(ioMouse::GetY());
	// Normalize screen space coordinates
	float mouseScreen0to1X = 0.0f;
	float mouseScreen0to1Y = 0.0f;
	if (grcViewport::GetDefaultScreen())
	{
		mouseScreen0to1X = mouseScreenX / grcViewport::GetDefaultScreen()->GetWidth();
		mouseScreen0to1Y = mouseScreenY / grcViewport::GetDefaultScreen()->GetHeight();
	}
	// Fix for reverse transform not working with scaled window
	const float viewportX = (mouseScreen0to1X) * viewport->GetWidth();
	const float viewportY = (mouseScreen0to1Y) * viewport->GetHeight();

	viewport->ReverseTransform(viewportX, viewportY, RC_VEC3V(mouseScreen), RC_VEC3V(mouseFar));

	// Establish a world space direction for the mouse click.
	Vector3 direction;
	direction.Subtract(mouseFar, mouseScreen);
	direction.Normalize();

	// Update spring attachment with screen movement
	m_GrabSpring.attachmentPoint.AddScaled(mouseScreen, direction, m_GrabDistance);

	if (m_CameraControl.IsUp() && (m_LeftClick.IsDown() || m_RightClick.IsDown()))
	{
		Vector3 segA, segB;
		segA = mouseScreen;
		segB.AddScaled(mouseScreen, direction, 100.0f);
		phSegment segment;
		segment.Set(segA, segB);

#define GET_TYPE_FLAG_IF_WIDGET_ENABLED(bit) (PFD_TypeFlag##bit.WillDraw() ? 1 << bit : 0)
#define GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(bit) (PFD_IncludeFlag##bit.WillDraw() ? 1 << bit : 0)

		unsigned typeFlags = TYPE_FLAGS_ALL;
		unsigned includeFlags = INCLUDE_FLAGS_ALL;

#if __PFDRAW
		if( PFDGROUP_TypeFlagFilter.WillDraw() )
		{
			typeFlags = 
				GET_TYPE_FLAG_IF_WIDGET_ENABLED(0) |
				GET_TYPE_FLAG_IF_WIDGET_ENABLED(1) |
				GET_TYPE_FLAG_IF_WIDGET_ENABLED(2) |
				GET_TYPE_FLAG_IF_WIDGET_ENABLED(3) |
				GET_TYPE_FLAG_IF_WIDGET_ENABLED(4) |
				GET_TYPE_FLAG_IF_WIDGET_ENABLED(5) |
				GET_TYPE_FLAG_IF_WIDGET_ENABLED(6) |
				GET_TYPE_FLAG_IF_WIDGET_ENABLED(7) |
				GET_TYPE_FLAG_IF_WIDGET_ENABLED(8) |
				GET_TYPE_FLAG_IF_WIDGET_ENABLED(9) |
				GET_TYPE_FLAG_IF_WIDGET_ENABLED(10) |
				GET_TYPE_FLAG_IF_WIDGET_ENABLED(11) |
				GET_TYPE_FLAG_IF_WIDGET_ENABLED(12) |
				GET_TYPE_FLAG_IF_WIDGET_ENABLED(13) |
				GET_TYPE_FLAG_IF_WIDGET_ENABLED(14) |
				GET_TYPE_FLAG_IF_WIDGET_ENABLED(15) |
				GET_TYPE_FLAG_IF_WIDGET_ENABLED(16) |
				GET_TYPE_FLAG_IF_WIDGET_ENABLED(17) |
				GET_TYPE_FLAG_IF_WIDGET_ENABLED(18) |
				GET_TYPE_FLAG_IF_WIDGET_ENABLED(19) |
				GET_TYPE_FLAG_IF_WIDGET_ENABLED(20) |
				GET_TYPE_FLAG_IF_WIDGET_ENABLED(21) |
				GET_TYPE_FLAG_IF_WIDGET_ENABLED(22) |
				GET_TYPE_FLAG_IF_WIDGET_ENABLED(23) |
				GET_TYPE_FLAG_IF_WIDGET_ENABLED(24) |
				GET_TYPE_FLAG_IF_WIDGET_ENABLED(25) |
				GET_TYPE_FLAG_IF_WIDGET_ENABLED(26) |
				GET_TYPE_FLAG_IF_WIDGET_ENABLED(27) |
				GET_TYPE_FLAG_IF_WIDGET_ENABLED(28) |
				GET_TYPE_FLAG_IF_WIDGET_ENABLED(29) |
				GET_TYPE_FLAG_IF_WIDGET_ENABLED(30) |
				GET_TYPE_FLAG_IF_WIDGET_ENABLED(31);
		}
		if( PFDGROUP_IncludeFlagFilter.WillDraw() )
		{
			includeFlags =
				GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(0) |
				GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(1) |
				GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(2) |
				GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(3) |
				GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(4) |
				GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(5) |
				GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(6) |
				GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(7) |
				GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(8) |
				GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(9) |
				GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(10) |
				GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(11) |
				GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(12) |
				GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(13) |
				GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(14) |
				GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(15) |
				GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(16) |
				GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(17) |
				GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(18) |
				GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(19) |
				GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(20) |
				GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(21) |
				GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(22) |
				GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(23) |
				GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(24) |
				GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(25) |
				GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(26) |
				GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(27) |
				GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(28) |
				GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(29) |
				GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(30) |
				GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(31);
		}
#endif // __PFDRAW

		if (sm_EnableLeftClick && m_LeftClick.IsPressed())
		{
#if PHMOUSEINPUT_ENABLE_BOXES
			if (sm_LeftClickMode == BOX)
			{
				ThrowBox(RCC_VEC3V(mouseScreen), RCC_VEC3V(direction));
			}
			else
#endif
			{
				u8 stateFlags =  phLevelBase::STATE_FLAG_ACTIVE | phLevelBase::STATE_FLAG_INACTIVE;
				if(sm_LeftClickMode == COMPARE)
				{
					stateFlags = phLevelBase::STATE_FLAGS_ALL;
				}

				phIntersection isect;
				if(sm_EnableLeftStickyClickMode && m_GrabSpring.instLevelIndex != phInst::INVALID_INDEX)
				{
					m_GrabSpring.instLevelIndex = phInst::INVALID_INDEX;
				}
				else if (PHLEVEL->TestProbe(segment, &isect, NULL, typeFlags, includeFlags, stateFlags))
				{
					switch (sm_LeftClickMode)
					{
					case SPRING:
					case CONSTRAINT:
						{							
							phInst* pInst = isect.GetInstance();

							// Create a grab spring
							m_GrabSpring.instLevelIndex = pInst->GetLevelIndex();
							m_GrabSpring.component = isect.GetComponent();
							(*(const Matrix34*)(&pInst->GetMatrix())).UnTransform(RCC_VECTOR3(isect.GetPosition()), m_GrabSpring.localPoint);
							
							PHCONSTRAINT->Remove(m_GrabSpring.constraintHandle);
							m_GrabSpring.constraintHandle.Reset();

							m_GrabSpring.worldPoint = RCC_VECTOR3(isect.GetPosition());

							if (pInst->GetArchetype()->GetBound()->GetType()==phBound::COMPOSITE)
							{
								// The instance that we hit has a composite bound so let's record some additional info.
								const phBoundComposite& compositeBound = *static_cast<phBoundComposite*>(pInst->GetArchetype()->GetBound());
								m_GrabSpring.instNumComponents = compositeBound.GetNumBounds();

								// Since a composite bound can have internal motion it's not enough just to record our hit position in the instance's local space - we need to record our hit position
								//   in the local space of the component that we hit.
								RCC_MATRIX34(compositeBound.GetCurrentMatrix(m_GrabSpring.component)).UnTransform(m_GrabSpring.localPoint);
							}
							else
							{
								m_GrabSpring.instNumComponents = 1;
							}

							// Record the distance from the camera position that our attachment point was.
							Vector3 grabLine;
							grabLine.Subtract(RCC_VECTOR3(isect.GetPosition()), mouseScreen);
							m_GrabDistance = grabLine.Dot(direction);
							m_GrabSpring.attachmentPoint.AddScaled(mouseScreen, direction, m_GrabDistance);
							Assert(square(m_GrabDistance) >= square(0.999f) * grabLine.Mag2());

							if (sm_LeftClickMode == CONSTRAINT)
							{
								phConstraintAttachment::Params  params;
																params.instanceA = pInst;
																params.componentA = m_GrabSpring.component;
																params.maxSeparation = sm_DefaultConstraintLength;
																params.constrainRotation = false;
																params.localPosA = RCC_VEC3V(m_GrabSpring.localPoint);
																params.localPosB = RCC_VEC3V(m_GrabSpring.attachmentPoint);

								phConstraintBase * constraintBase;
								m_GrabSpring.constraintHandle = PHCONSTRAINT->InsertAndReturnTemporaryPointer(params, constraintBase);
								PHSIM->ActivateObject(pInst,NULL,NULL,constraintBase);
							}

							break;
						}

					case IMPULSE:
					case BULLET:
						{
							// A direct line of sight probe at the mouse icon location hit something.
							Assert(isect.GetInstance() && isect.GetInstance()->GetLevelIndex()!=phInst::INVALID_INDEX);

							// Apply the impulse.
							if (sm_LeftClickMode == IMPULSE)
							{
								// Find the impulse magnitude.
								float impulseMag = 1.0f;
								if (sm_ImpulseScaleByMass)
								{
									impulseMag = isect.GetInstance()->GetArchetype()->GetMass();
								}

								impulseMag *= sm_ImpulseScale;

								if (m_ReverseImpulse.IsDown())
								{
									impulseMag *= -1.0f;
								}

#if !__FINAL // Should this whole file be !__FINAL?
								if (ioMapper::DebugKeyDown(KEY_B) && isect.GetInstance()->IsBreakable(NULL))
								{
									bool isActive = PHLEVEL->IsActive(isect.GetInstance()->GetLevelIndex());
									// The B key is down and the hit instance is breakable, so increase the impulse to make it
									// break the instance.
									const phInstBreakable* breakableInst = static_cast<const phInstBreakable*>(isect.GetInstance());
									impulseMag += breakableInst->GetImpulseLimit(isActive);
								}
#endif

								// Set the impulse.
								Vector3 impulse(direction);
								impulse.Scale(impulseMag);

								if (m_AngularImpetus.IsDown())
								{
									// Apply an angular impulse.
									const float angImpulseScale = 0.1f;
									impulse.Scale(angImpulseScale);
									PHSIM->ApplyAngImpulse(isect.GetInstance()->GetLevelIndex(),impulse);
								}
								else
								{
									PHSIM->ApplyImpulse(isect.GetInstance()->GetLevelIndex(), impulse, RCC_VECTOR3(isect.GetPosition()), isect.GetComponent(), isect.GetPartIndex(), sm_ImpulseBreakScale);
								}
							}
							else
							{
								ScalarV bulletMass = ScalarVFromF32(sm_ImpulseBulletMass);
								ScalarV bulletSpeed = ScalarVFromF32(sm_ImpulseBulletSpeed);
								ScalarV breakScale = ScalarVFromF32(sm_ImpulseBreakScale);
								PHSIM->ApplyBulletImpulse(isect.GetInstance()->GetLevelIndex(),bulletMass,bulletSpeed,RCC_VEC3V(direction),isect.GetPosition(),isect.GetComponent(),isect.GetPartIndex(),breakScale);
							}

							break;
						}

#if PHMOUSEINPUT_ENABLE_BOXES
					case BOX:
						{
							// Box case handled above since no probe is needed
							break;
						}
#endif

#if PHMOUSEINPUT_ENABLE_EXPLOSIONS
					case EXPLOSION:
						{
							m_ExplosionMgr->SpawnExplosion(RCC_VECTOR3(isect.GetPosition()), &sm_ExplosionType);
							break;
						}
#endif

					case ACTIVATE:
						{
							int levelIndex = isect.GetInstance()->GetLevelIndex();
							if (PHLEVEL->IsInactive(levelIndex))
							{
								PHSIM->ActivateObject(levelIndex);
							}
							break;
						}

					case DEACTIVATE:
						{
							int levelIndex = isect.GetInstance()->GetLevelIndex();
							if (PHLEVEL->IsActive(levelIndex))
							{
								PHSIM->DeactivateObject(levelIndex);
							}
							break;
						}

					case COMPARE:
						{
							m_CompareInst = isect.GetInstance()->GetLevelIndex();
							m_CompareComponent = isect.GetComponent();
							m_ComparePart = isect.GetPartIndex();
							break;
						}

					case NUM_LEFT_CLICK_MODES:
						{
							break;
						}
					}
				}
			}
		}

		if (sm_EnableLeftClick && (m_LeftClick.IsDown() || sm_EnableLeftStickyClickMode) && m_GrabSpring.instLevelIndex != phInst::INVALID_INDEX)
		{
			// The 'spring' button is still held down and the instance to which that spring is attached is still in the level.
			if (m_RightClick.IsPressed() && m_CameraControl.IsUp() && m_NumPlacedSprings < MAX_NUM_SPRINGS)
			{
				// They're want to place their spring.  Let's copy the spring they were using into the list of placed springs.
				m_PlacedSprings[m_NumPlacedSprings++] = m_GrabSpring;

				m_GrabSpring.instLevelIndex = phInst::INVALID_INDEX;
				// Ownership of the handle has been transferred
				m_GrabSpring.constraintHandle.Reset();
			}
		}
		else
		{
			if (sm_EnableRightClick && m_RightClick.IsPressed())
			{
				phIntersection isect;
				if (PHLEVEL->TestProbe(segment, &isect, NULL, typeFlags, includeFlags))
				{
					m_SelectedInst = isect.GetInstance()->GetLevelIndex();

					DR_ONLY( debugPlayback::SetCurrentSelectedEntity(isect.GetInstance(), false) );

					m_SelectedComponent = isect.GetComponent();
					m_SelectedPart = isect.GetPartIndex();
				}
				else
				{
					m_SelectedInst = BAD_INDEX;
					m_SelectedComponent = 0;
					m_SelectedPart = 0;
				}

				m_SelectedInstArchDamp = NULL;
			}

			m_GrabSpring.instLevelIndex = phInst::INVALID_INDEX;
		}
	}
	else
	{
		if (!sm_EnableLeftStickyClickMode)
		{
			m_GrabSpring.instLevelIndex = phInst::INVALID_INDEX;
		}
	}

	UpdateSpring(&m_GrabSpring, paused, timeStep);

	for (int spring = 0; spring < m_NumPlacedSprings; ++spring)
	{
		UpdateSpring(&m_PlacedSprings[spring], paused, timeStep);
	}

	if (sm_EnableRightClick == false)
	{
		m_SelectedInst = BAD_INDEX;
		m_SelectedInstArchDamp = NULL;
	}

	phBoundComposite* selectedComposite = NULL;
	if (m_SelectedInst != BAD_INDEX && PHLEVEL->IsInLevel(m_SelectedInst))
	{
		phInst* selectedInst = PHLEVEL->GetInstance(m_SelectedInst);
		formatf(s_InstPtrString, sizeof(s_InstPtrString), "0x%p", selectedInst);
		formatf(s_LevelIndexString, sizeof(s_LevelIndexString), "%d", selectedInst->GetLevelIndex());
		const char* filename = selectedInst->GetArchetype()->GetFilename();
		formatf(s_ArchetypeNameString, sizeof(s_ArchetypeNameString), "%s", filename ? filename : "(null)");
		const u32 typeFlags = PHLEVEL->GetInstanceTypeFlags(m_SelectedInst);
		const u32 includeFlags = PHLEVEL->GetInstanceIncludeFlags(m_SelectedInst);
		formatf(s_TypeFlagsString, sizeof(s_TypeFlagsString), "0x%08x", typeFlags);
		GetNamesFromFlags(s_TypeFlagNamesString, sizeof(s_TypeFlagNamesString), typeFlags, "* NONE *");
		formatf(s_IncludeFlagsString, sizeof(s_IncludeFlagsString), "0x%08x", includeFlags);
		GetNamesFromFlags(s_IncludeFlagNamesString, sizeof(s_IncludeFlagNamesString), includeFlags, "* NONE *");
		formatf(s_ComponentNumberString, sizeof(s_ComponentNumberString), "%d", m_SelectedComponent);
		formatf(s_PartNumberString, sizeof(s_PartNumberString), "%d", m_SelectedPart);

		if(m_SelectedInstArchDamp == NULL)
		{
			phArchetype* pArch = selectedInst->GetArchetype();
			m_SelectedInstArchDamp = dynamic_cast<phArchetypeDamp*>(pArch);
		}
		if(m_SelectedInstArchDamp != NULL)
		{
			for(int index=0;index < phArchetypeDamp::NUM_DAMP_TYPES;index++)
			{
				BANK_ONLY(s_DampingConstants[index] = m_SelectedInstArchDamp->m_DampingConstant[index]);
			}
		}
		else
		{
			for(int index=0;index < phArchetypeDamp::NUM_DAMP_TYPES;index++)
			{
				BANK_ONLY(s_DampingConstants[index] = Vector3(0.0f, 0.0f, 0.0f));
			}
		}

		Matrix34 partMtx = RCC_MATRIX34(selectedInst->GetMatrix());
		Matrix34 partLastMtx = RCC_MATRIX34(PHSIM->GetLastInstanceMatrix(selectedInst));

		phBound* selectedBound = selectedInst->GetArchetype()->GetBound();
		formatf(s_BoundPtrString, sizeof(s_BoundPtrString), "0x%p", selectedBound);
		strcpy(s_BoundTypeString, selectedBound->GetTypeString());
		
		phMaterialMgr::Id materialId = selectedBound->GetMaterialIdFromPartIndexAndComponent(m_SelectedPart, m_SelectedComponent);
		const phMaterial& material = MATERIALMGR.GetMaterial(materialId);
		strcpy(s_MaterialString, material.GetName());
#if PH_MATERIAL_ID_64BIT
		formatf(s_MatIdString, sizeof(s_MatIdString), "0x%016llx", materialId);
#else
		formatf(s_MatIdString, sizeof(s_MatIdString), "0x%08x", materialId);
#endif

		// Position and orient matrix of inst
		if(selectedInst)
		{
			Vector3 pos = RCC_VECTOR3(selectedInst->GetPosition());
			formatf(s_PositionString, sizeof(s_PositionString), "X = %5.6f, Y = %5.6f, Z = %5.6f", pos.GetX(), pos.GetY(), pos.GetZ());

			Matrix34 mat = RCC_MATRIX34(selectedInst->GetMatrix());
			formatf(s_MatrixString1, sizeof(s_MatrixString1), "X = %5.6f, Y = %5.6f, Z = %5.6f", mat.GetVector(0).GetX(), mat.GetVector(0).GetY(), mat.GetVector(0).GetZ());
			formatf(s_MatrixString2, sizeof(s_MatrixString2), "X = %5.6f, Y = %5.6f, Z = %5.6f", mat.GetVector(1).GetX(), mat.GetVector(1).GetY(), mat.GetVector(1).GetZ());
			formatf(s_MatrixString3, sizeof(s_MatrixString3), "X = %5.6f, Y = %5.6f, Z = %5.6f", mat.GetVector(2).GetX(), mat.GetVector(2).GetY(), mat.GetVector(2).GetZ());

			formatf(s_ArchetypeMassString, sizeof(s_ArchetypeMassString), "%f", selectedInst->GetArchetype()->GetMass());
			formatf(s_ArchetypeAngularInertiaString, sizeof(s_ArchetypeAngularInertiaString), "X = %5.6f, Y = %5.6f, Z = %5.6f", selectedInst->GetArchetype()->GetAngInertia().x, selectedInst->GetArchetype()->GetAngInertia().y, selectedInst->GetArchetype()->GetAngInertia().z);

		//	mat = RCC_MATRIX34(PHSIM->GetLastInstanceMatrix(selectedInst));
		//	formatf(s_LastMatrixString1, sizeof(s_LastMatrixString1), "X = %5.6f, Y = %5.6f, Z = %5.6f", mat.GetVector(0).GetX(), mat.GetVector(0).GetY(), mat.GetVector(0).GetZ());
		//	formatf(s_LastMatrixString2, sizeof(s_LastMatrixString2), "X = %5.6f, Y = %5.6f, Z = %5.6f", mat.GetVector(1).GetX(), mat.GetVector(1).GetY(), mat.GetVector(1).GetZ());
		//	formatf(s_LastMatrixString3, sizeof(s_LastMatrixString3), "X = %5.6f, Y = %5.6f, Z = %5.6f", mat.GetVector(2).GetX(), mat.GetVector(2).GetY(), mat.GetVector(2).GetZ());
		//	formatf(s_LastPositionString, sizeof(s_LastPositionString), "X = %5.6f, Y = %5.6f, Z = %5.6f", mat.GetVector(3).GetX(), mat.GetVector(3).GetY(), mat.GetVector(3).GetZ());
		}

		if (selectedBound->GetType() == phBound::COMPOSITE)
		{
			selectedComposite = static_cast<phBoundComposite*>(selectedBound);
			formatf(s_CompositeBoundCountString, sizeof(s_CompositeBoundCountString), "%d", selectedComposite->GetNumBounds());

			if (phBound* selectedPart = selectedComposite->GetBound(m_SelectedComponent))
			{
				partMtx.DotFromLeft(RCC_MATRIX34(selectedComposite->GetCurrentMatrix(m_SelectedComponent)));
			//	partLastMtx.DotFromLeft(RCC_MATRIX34(selectedComposite->GetLastMatrix(m_SelectedComponent)));
				formatf(s_PartMatrixString1, sizeof(s_PartMatrixString1), "X = %5.6f, Y = %5.6f, Z = %5.6f", partMtx.GetVector(0).GetX(), partMtx.GetVector(0).GetY(), partMtx.GetVector(0).GetZ());
				formatf(s_PartMatrixString2, sizeof(s_PartMatrixString2), "X = %5.6f, Y = %5.6f, Z = %5.6f", partMtx.GetVector(1).GetX(), partMtx.GetVector(1).GetY(), partMtx.GetVector(1).GetZ());
				formatf(s_PartMatrixString3, sizeof(s_PartMatrixString3), "X = %5.6f, Y = %5.6f, Z = %5.6f", partMtx.GetVector(2).GetX(), partMtx.GetVector(2).GetY(), partMtx.GetVector(2).GetZ());
				formatf(s_PartPositionString, sizeof(s_PartPositionString), "X = %5.6f, Y = %5.6f, Z = %5.6f", partMtx.GetVector(3).GetX(), partMtx.GetVector(3).GetY(), partMtx.GetVector(3).GetZ());
				// Last
			//	formatf(s_PartLastMatrixString1, sizeof(s_PartLastMatrixString1), "X = %5.6f, Y = %5.6f, Z = %5.6f", partLastMtx.GetVector(0).GetX(), partLastMtx.GetVector(0).GetY(), partLastMtx.GetVector(0).GetZ());
			//	formatf(s_PartLastMatrixString2, sizeof(s_PartLastMatrixString2), "X = %5.6f, Y = %5.6f, Z = %5.6f", partLastMtx.GetVector(1).GetX(), partLastMtx.GetVector(1).GetY(), partLastMtx.GetVector(1).GetZ());
			//	formatf(s_PartLastMatrixString3, sizeof(s_PartLastMatrixString3), "X = %5.6f, Y = %5.6f, Z = %5.6f", partLastMtx.GetVector(2).GetX(), partLastMtx.GetVector(2).GetY(), partLastMtx.GetVector(2).GetZ());
			//	formatf(s_PartLastPositionString, sizeof(s_PartLastPositionString), "X = %5.6f, Y = %5.6f, Z = %5.6f", partLastMtx.GetVector(3).GetX(), partLastMtx.GetVector(3).GetY(), partLastMtx.GetVector(3).GetZ());

				selectedBound = selectedPart;
				formatf(s_PartBoundPtrString, sizeof(s_PartBoundPtrString), "0x%p", selectedPart);
				strcpy(s_PartBoundTypeString, selectedPart->GetTypeString());
			}
			else
			{
				strcpy(s_PartBoundPtrString, "* null bound *");
				strcpy(s_PartBoundTypeString, "* null bound *");
			}

			if (selectedComposite->GetTypeAndIncludeFlags())
			{
				formatf(s_CompositeTypeFlagsString, sizeof(s_CompositeTypeFlagsString), "0x%08x", selectedComposite->GetTypeFlags(m_SelectedComponent));
				GetNamesFromFlags(s_CompositeTypeFlagNamesString, sizeof(s_CompositeTypeFlagsString), selectedComposite->GetTypeFlags(m_SelectedComponent), "* NONE *");
				formatf(s_CompositeIncludeFlagsString, sizeof(s_CompositeIncludeFlagsString), "0x%08x", selectedComposite->GetIncludeFlags(m_SelectedComponent));
				GetNamesFromFlags(s_CompositeIncludeFlagNamesString, sizeof(s_CompositeIncludeFlagsString), selectedComposite->GetIncludeFlags(m_SelectedComponent), "* NONE *");
			}
			else
			{
				strcpy(s_CompositeTypeFlagsString, "* no flags *");
				strcpy(s_CompositeTypeFlagNamesString, "* no flags *");
				strcpy(s_CompositeIncludeFlagsString, "* no flags *");
				strcpy(s_CompositeIncludeFlagNamesString, "* no flags *");
			}
		}
		else
		{
			strcpy(s_CompositeBoundCountString, "* not a composite *");
			strcpy(s_PartBoundPtrString, "* not a composite *");
			strcpy(s_PartBoundTypeString, "* not a composite *");
			strcpy(s_CompositeTypeFlagsString, "* not a composite *");
			strcpy(s_CompositeTypeFlagNamesString, "* not a composite *");
			strcpy(s_CompositeIncludeFlagsString, "* not a composite *");
			strcpy(s_CompositeIncludeFlagNamesString, "* not a composite *");
		}

		strcpy(s_PartCountString, "* No parts *");

		if (selectedBound->GetType() == phBound::GEOMETRY ||
			selectedBound->GetType() == phBound::BVH 
			USE_GRIDS_ONLY(|| selectedBound->GetType() == phBound::GRID))
		{
			phBoundPolyhedron* boundPoly = static_cast<phBoundPolyhedron*>(selectedBound);
			const phPrimitive* prim;
			if(selectedBound->GetType() == phBound::BVH)
			{
				prim = &static_cast<phBoundBVH*>(selectedBound)->GetPrimitive(m_SelectedPart);
				formatf(s_PartCountString, sizeof(s_PartCountString), "%d", static_cast<phBoundBVH*>(selectedBound)->GetNumPolygons());
			}
			else
			{
				prim = &boundPoly->GetPolygon(m_SelectedPart).GetPrimitive();
				formatf(s_PartCountString, sizeof(s_PartCountString), "%d", boundPoly->GetNumPolygons());
			}

			switch (prim->GetType())
			{
				case PRIM_TYPE_POLYGON:
					{
						strcpy(s_PartTypeString, "POLYGON");
						Vector3 normal = VEC3V_TO_VECTOR3(boundPoly->GetPolygonUnitNormal(m_SelectedPart));
						formatf(s_PartNormalString, "%f, %f, %f", normal.x, normal.y, normal.z);
					}
					break;
				case PRIM_TYPE_SPHERE:
					strcpy(s_PartTypeString, "SPHERE");
					strcpy(s_PartNormalString, "* not a poly *");
					break;
				case PRIM_TYPE_CAPSULE:
					strcpy(s_PartTypeString, "CAPSULE");
					strcpy(s_PartNormalString, "* not a poly *");
					break;
				case PRIM_TYPE_BOX:
					strcpy(s_PartTypeString, "BOX");
					strcpy(s_PartNormalString, "* not a poly *");
					break;
				default:
					strcpy(s_PartTypeString, "CYLINDER");
					strcpy(s_PartNormalString, "* not a poly *");
					break;
			}
		}


		if (phCollider* collider = PHSIM->GetCollider(m_SelectedInst))
		{
			if (sm_DebugColliderUpdate)
			{
#if __DEV
				phCollider::SetDebugColliderUpdate(collider);
#endif

				sm_DebugColliderUpdate = false;
			}

			formatf(s_ColliderPtrString, sizeof(s_ColliderPtrString), "0x%p", collider);
			formatf(s_ColliderMassString, sizeof(s_ColliderMassString), "%f", collider->GetMass());
			formatf(s_ColliderAngularInertiaString, sizeof(s_ColliderAngularInertiaString), "%f, %f, %f", VEC3V_ARGS(collider->GetAngInertia()));
			formatf(s_ApproximateRadiusString, sizeof(s_ApproximateRadiusString), "%f", collider->GetApproximateRadius());
			if(collider->IsArticulated())
			{
				phArticulatedCollider* articulatedCollider = static_cast<phArticulatedCollider*>(collider);
				int selectedLinkIndex = articulatedCollider->GetLinkFromComponent(m_SelectedComponent);
				phArticulatedBody* articulatedBody = articulatedCollider->GetBody();
				formatf(s_LinkNumberString, sizeof(s_LinkNumberString), "%i", selectedLinkIndex);
				formatf(s_LinkMassString, sizeof(s_LinkMassString), "%f", articulatedBody->GetMass(selectedLinkIndex).Getf());
				formatf(s_LinkAngularInertiaString, sizeof(s_LinkAngularInertiaString), "%f, %f, %f", VEC3V_ARGS(articulatedBody->GetAngInertia(selectedLinkIndex)));
			
				float totalLinkMass = 0;
				for(u8 linkIndex = 0; linkIndex < articulatedBody->GetNumBodyParts(); ++linkIndex)
				{
					totalLinkMass += articulatedBody->GetMass(linkIndex).Getf();
				}
				formatf(s_ArticulatedColliderMassString, sizeof(s_ArticulatedColliderMassString), "%f", totalLinkMass);
			}
			else
			{
				strcpy(s_ArticulatedColliderMassString, "* not articulated *");
				strcpy(s_LinkNumberString, "* not articulated *");
				strcpy(s_LinkMassString, "* not articulated *");
				strcpy(s_LinkAngularInertiaString, "* not articulated *");
			}

			if (phSleep* sleep = collider->GetSleep())
			{
				phSleep::SleepState sleepState = sleep->GetMode();
				strcpy(s_SleepModeString, sleepState == phSleep::AWAKE ? "AWAKE" : sleepState == phSleep::ASLEEP ? "ASLEEP" : "DONESLEEPING");
				formatf(s_AsleepTicksString, sizeof(s_AsleepTicksString), "%d", sleep->GetTicksAsleep());
				formatf(s_MotionlessTicksString, sizeof(s_MotionlessTicksString), "%d", sleep->GetTicksDoneSleeping());
			}
			else
			{
				strcpy(s_SleepModeString, "* no sleep *");
				strcpy(s_AsleepTicksString, "* no sleep *");
				strcpy(s_MotionlessTicksString, "* no sleep *");
			}
		}
		else
		{
			strcpy(s_ColliderPtrString, "* no collider *");
			strcpy(s_SleepModeString, "* no collider *");
			strcpy(s_AsleepTicksString, "* no collider *");
			strcpy(s_MotionlessTicksString, "* no collider *");
			strcpy(s_ColliderMassString, "* no collider *");
			strcpy(s_ArticulatedColliderMassString, "* no collider *");
			strcpy(s_ColliderAngularInertiaString, "* no collider *");
			strcpy(s_ApproximateRadiusString, "* no collider *");
			strcpy(s_LinkNumberString, "* no collider *");
			strcpy(s_LinkMassString, "* no collider *");
			strcpy(s_LinkAngularInertiaString, "* no collider *");
		}

		Vector3 vel(0.0f, 0.0f, 0.0f);
		Vector3 angVel(0.0f, 0.0f, 0.0f);
		Vector3 referenceFrame(0.0f, 0.0f, 0.0f);
		if(PHLEVEL->IsActive(selectedInst->GetLevelIndex()))
		{
			phCollider* collider = (phCollider*)(PHLEVEL->GetUserData(selectedInst->GetLevelIndex()));
			if(collider)
			{
				referenceFrame = RCC_VECTOR3(collider->GetReferenceFrameVelocity());
				vel = RCC_VECTOR3(collider->GetVelocity());
				angVel = RCC_VECTOR3(collider->GetAngVelocity());
			}
		}
		static int s_count = 0;
		if(s_count <= 0)
		{
			formatf(s_DistanceToCameraString, sizeof(s_DistanceToCameraString), "%5.3f", VEC3V_TO_VECTOR3(viewport->GetCameraPosition()).Dist(RCC_MATRIX34(selectedInst->GetMatrix()).d));
			formatf(s_ReferenceFrameString, sizeof(s_ReferenceFrameString), "X = %5.3f, Y = %5.3f, Z = %5.3f", referenceFrame.GetX(), referenceFrame.GetY(), referenceFrame.GetZ());
			formatf(s_VelocityString, sizeof(s_VelocityString), "X = %5.3f, Y = %5.3f, Z = %5.3f", vel.GetX(), vel.GetY(), vel.GetZ());
			formatf(s_AngVelocityString, sizeof(s_AngVelocityString), "X = %5.3f, Y = %5.3f, Z = %5.3f", angVel.GetX(), angVel.GetY(), angVel.GetZ());
			formatf(s_SpeedString, sizeof(s_SpeedString), "%5.3f", vel.Mag());
			s_count = s_FrameDelay;
		}
		else
		{
			s_count--;
		}
	}
	else
	{
		m_SelectedInst = BAD_INDEX;
		m_SelectedInstArchDamp = NULL;
		for(int index=0;index < phArchetypeDamp::NUM_DAMP_TYPES;index++)
		{
			BANK_ONLY(s_DampingConstants[index] = Vector3(0.0f, 0.0f, 0.0f));
		}

		strcpy(s_InstPtrString, "* right click to select *");
		strcpy(s_LevelIndexString, "* right click to select *");
		strcpy(s_ArchetypeNameString, "* right click to select *");
		strcpy(s_TypeFlagsString, "* right click to select *");
		strcpy(s_TypeFlagNamesString, "* right click to select *");
		strcpy(s_IncludeFlagsString, "* right click to select *");
		strcpy(s_IncludeFlagNamesString, "* right click to select *");
		strcpy(s_ComponentNumberString, "* right click to select *");
		strcpy(s_PartNumberString, "* right click to select *");
		strcpy(s_PartTypeString, "* right click to select *");
		strcpy(s_PartNormalString, "* right click to select *");
		strcpy(s_MaterialString, "* right click to select *");
		strcpy(s_BoundPtrString, "* right click to select *");
		strcpy(s_BoundTypeString, "* right click to select *");
		strcpy(s_CompositeBoundCountString, "* right click to select *");
		strcpy(s_PartCountString, "* right click to select *");
		strcpy(s_PartBoundPtrString, "* right click to select *");
		strcpy(s_PartBoundTypeString, "* right click to select *");
		strcpy(s_CompositeTypeFlagsString, "* right click to select *");
		strcpy(s_CompositeTypeFlagNamesString, "* right click to select *");
		strcpy(s_CompositeIncludeFlagsString, "* right click to select *");
		strcpy(s_CompositeIncludeFlagNamesString, "* right click to select *");
		strcpy(s_ColliderPtrString, "* right click to select *");
		strcpy(s_SleepModeString, "* right click to select *");
		strcpy(s_AsleepTicksString, "* right click to select *");
		strcpy(s_MotionlessTicksString, "* right click to select *");
		strcpy(s_LinkNumberString, "* right click to select *");
		strcpy(s_ArchetypeMassString, "* right click to select *");
		strcpy(s_ColliderMassString, "* right click to select *");
		strcpy(s_ArticulatedColliderMassString, "* right click to select *");
		strcpy(s_LinkMassString, "* right click to select *");
		strcpy(s_ArchetypeAngularInertiaString, "* right click to select *");
		strcpy(s_ColliderAngularInertiaString, "* right click to select *");
		strcpy(s_LinkAngularInertiaString, "* right click to select *");
		strcpy(s_ApproximateRadiusString, "* right click to select *");
	}

	if (sm_EnableLeftClick == false)
	{
		m_CompareInst = BAD_INDEX;
	}

	if (m_CompareInst != BAD_INDEX && PHLEVEL->IsInLevel(m_CompareInst))
	{
		phInst* compareInst = PHLEVEL->GetInstance(m_CompareInst);
		formatf(s_InstPtrString_comp, sizeof(s_InstPtrString_comp), "0x%p", compareInst);
		formatf(s_LevelIndexString_comp, sizeof(s_LevelIndexString_comp), "%d", compareInst->GetLevelIndex());
		const char* filename = compareInst->GetArchetype()->GetFilename();
		formatf(s_ArchetypeNameString_comp, sizeof(s_ArchetypeNameString_comp), "%s", filename ? filename : "(null)");
		const u32 typeFlags = PHLEVEL->GetInstanceTypeFlags(m_CompareInst);
		const u32 includeFlags = PHLEVEL->GetInstanceIncludeFlags(m_CompareInst);
		formatf(s_TypeFlagsString_comp, sizeof(s_TypeFlagsString_comp), "0x%08x", typeFlags);
		GetNamesFromFlags(s_TypeFlagNamesString_comp, sizeof(s_TypeFlagNamesString_comp), typeFlags, "* NONE *");
		formatf(s_IncludeFlagsString_comp, sizeof(s_IncludeFlagsString_comp), "0x%08x", includeFlags);
		GetNamesFromFlags(s_IncludeFlagNamesString_comp, sizeof(s_IncludeFlagNamesString_comp), includeFlags, "* NONE *");
		formatf(s_ComponentNumberString_comp, sizeof(s_ComponentNumberString_comp), "%d", m_CompareComponent);
		formatf(s_PartNumberString_comp, sizeof(s_PartNumberString_comp), "%d", m_SelectedPart);

		Matrix34 partMtx = RCC_MATRIX34(compareInst->GetMatrix());
		Matrix34 partLastMtx = RCC_MATRIX34(PHSIM->GetLastInstanceMatrix(compareInst));

		phBound* compareBound = compareInst->GetArchetype()->GetBound();
		formatf(s_BoundPtrString_comp, sizeof(s_BoundPtrString_comp), "0x%p", compareBound);
		strcpy(s_BoundTypeString_comp, compareBound->GetTypeString());

		phBoundComposite* compareComposite = NULL;
		if (compareBound->GetType() == phBound::COMPOSITE)
		{
			compareComposite = static_cast<phBoundComposite*>(compareBound);
			formatf(s_CompositeBoundCountString_comp, sizeof(s_CompositeBoundCountString_comp), "%d", compareComposite->GetNumBounds());

			if (phBound* selectedPart = compareComposite->GetBound(m_CompareComponent))
			{
				partMtx.DotFromLeft(RCC_MATRIX34(compareComposite->GetCurrentMatrix(m_CompareComponent)));

				compareBound = selectedPart;
				formatf(s_PartBoundPtrString_comp, sizeof(s_PartBoundPtrString_comp), "0x%p", selectedPart);
				strcpy(s_PartBoundTypeString_comp, selectedPart->GetTypeString());
			}
			else
			{
				strcpy(s_PartBoundPtrString_comp, "* null bound *");
				strcpy(s_PartBoundTypeString_comp, "* null bound *");
			}

			if (compareComposite->GetTypeAndIncludeFlags())
			{
				formatf(s_CompositeTypeFlagsString_comp, sizeof(s_CompositeTypeFlagsString_comp), "0x%08x", compareComposite->GetTypeFlags(m_CompareComponent));
				GetNamesFromFlags(s_CompositeTypeFlagNamesString_comp, sizeof(s_CompositeTypeFlagsString_comp), compareComposite->GetTypeFlags(m_CompareComponent), "* NONE *");
				formatf(s_CompositeIncludeFlagsString_comp, sizeof(s_CompositeIncludeFlagsString_comp), "0x%08x", compareComposite->GetIncludeFlags(m_CompareComponent));
				GetNamesFromFlags(s_CompositeIncludeFlagNamesString_comp, sizeof(s_CompositeIncludeFlagsString_comp), compareComposite->GetIncludeFlags(m_CompareComponent), "* NONE *");
			}
			else
			{
				strcpy(s_CompositeTypeFlagsString_comp, "* no flags *");
				strcpy(s_CompositeTypeFlagNamesString_comp, "* no flags *");
				strcpy(s_CompositeIncludeFlagsString_comp, "* no flags *");
				strcpy(s_CompositeIncludeFlagNamesString_comp, "* no flags *");
			}
		}

		// Reset
		strcpy(s_InstFlagMatches_LeftInclude_RightType, "* none *");
		strcpy(s_InstFlagMatchesString_LeftInclude_RightType, "* none *");
		strcpy(s_InstFlagMatches_LeftType_RightInclude, "* none *");
		strcpy(s_InstFlagMatchesString_LeftType_RightInclude, "* none *");
		strcpy(s_FlagMatches_LeftPartInclude_RightType, "* none *");
		strcpy(s_FlagMatchesString_LeftPartInclude_RightType, "* none *");
		strcpy(s_FlagMatches_LeftPartType_RightInclude, "* none *");
		strcpy(s_FlagMatchesString_LeftPartType_RightInclude, "* none *");
		strcpy(s_FlagMatches_LeftInclude_RightPartType, "* none *");
		strcpy(s_FlagMatchesString_LeftInclude_RightPartType, "* none *");
		strcpy(s_FlagMatches_LeftType_RightPartInclude, "* none *");
		strcpy(s_FlagMatchesString_LeftType_RightPartInclude, "* none *");
		strcpy(s_PartFlagMatches_LeftInclude_RightType, "* none *");
		strcpy(s_PartFlagMatchesString_LeftInclude_RightType, "* none *");
		strcpy(s_PartFlagMatches_LeftType_RightInclude, "* none *");
		strcpy(s_PartFlagMatchesString_LeftType_RightInclude, "* none *");

		//////////////////////////////////////////////////////////////////////////
		// Compare left and right click selections
		if(m_SelectedInst != BAD_INDEX && PHLEVEL->IsInLevel(m_SelectedInst))
		{
			const u32 typeFlags_Left = PHLEVEL->GetInstanceTypeFlags(m_CompareInst);
			const u32 includeFlags_Left = PHLEVEL->GetInstanceIncludeFlags(m_CompareInst);
			const u32 typeFlags_Right = PHLEVEL->GetInstanceTypeFlags(m_SelectedInst);
			const u32 includeFlags_Right = PHLEVEL->GetInstanceIncludeFlags(m_SelectedInst);

			formatf(s_InstFlagMatches_LeftInclude_RightType, sizeof(s_InstFlagMatches_LeftInclude_RightType), "0x%08x", includeFlags_Left & typeFlags_Right);
			GetNamesFromFlags(s_InstFlagMatchesString_LeftInclude_RightType, sizeof(s_InstFlagMatchesString_LeftInclude_RightType), includeFlags_Left & typeFlags_Right, "* NO MATCH *");
			formatf(s_InstFlagMatches_LeftType_RightInclude, sizeof(s_InstFlagMatches_LeftType_RightInclude), "0x%08x", typeFlags_Left & includeFlags_Right);
			GetNamesFromFlags(s_InstFlagMatchesString_LeftType_RightInclude, sizeof(s_InstFlagMatchesString_LeftType_RightInclude), typeFlags_Left & includeFlags_Right, "* NO MATCH *");

			if(compareComposite != NULL && compareComposite->GetTypeAndIncludeFlags())
			{
				const u32 typeFlags_Leftpart = compareComposite->GetTypeFlags(m_CompareComponent);
				const u32 includeFlags_Leftpart = compareComposite->GetIncludeFlags(m_CompareComponent);

				formatf(s_FlagMatches_LeftPartInclude_RightType, sizeof(s_FlagMatches_LeftPartInclude_RightType), "0x%08x", includeFlags_Leftpart & typeFlags_Right);
				GetNamesFromFlags(s_FlagMatchesString_LeftPartInclude_RightType, sizeof(s_FlagMatchesString_LeftPartInclude_RightType), includeFlags_Leftpart & typeFlags_Right, "* NO MATCH *");
				formatf(s_FlagMatches_LeftPartType_RightInclude, sizeof(s_FlagMatches_LeftPartType_RightInclude), "0x%08x", typeFlags_Leftpart & includeFlags_Right);
				GetNamesFromFlags(s_FlagMatchesString_LeftPartType_RightInclude, sizeof(s_FlagMatchesString_LeftPartType_RightInclude), typeFlags_Leftpart & includeFlags_Right, "* NO MATCH *");
			}

			if(selectedComposite != NULL && selectedComposite->GetTypeAndIncludeFlags())
			{
				const u32 typeFlags_Rightpart = selectedComposite->GetTypeFlags(m_SelectedComponent);
				const u32 includeFlags_Rightpart = selectedComposite->GetIncludeFlags(m_SelectedComponent);

				formatf(s_FlagMatches_LeftInclude_RightPartType, sizeof(s_FlagMatches_LeftInclude_RightPartType), "0x%08x", includeFlags_Left & typeFlags_Rightpart);
				GetNamesFromFlags(s_FlagMatchesString_LeftInclude_RightPartType, sizeof(s_FlagMatchesString_LeftInclude_RightPartType), includeFlags_Left & typeFlags_Rightpart, "* NO MATCH *");
				formatf(s_FlagMatches_LeftType_RightPartInclude, sizeof(s_FlagMatches_LeftType_RightPartInclude), "0x%08x", typeFlags_Left & includeFlags_Rightpart);
				GetNamesFromFlags(s_FlagMatchesString_LeftType_RightPartInclude, sizeof(s_FlagMatchesString_LeftType_RightPartInclude), typeFlags_Left & includeFlags_Rightpart, "* NO MATCH *");
			}

			if(compareComposite != NULL && compareComposite->GetTypeAndIncludeFlags() && selectedComposite != NULL && selectedComposite->GetTypeAndIncludeFlags())
			{
				const u32 typeFlags_Leftpart = compareComposite->GetTypeFlags(m_CompareComponent);
				const u32 includeFlags_Leftpart = compareComposite->GetIncludeFlags(m_CompareComponent);
				const u32 typeFlags_Rightpart = selectedComposite->GetTypeFlags(m_SelectedComponent);
				const u32 includeFlags_Rightpart = selectedComposite->GetIncludeFlags(m_SelectedComponent);

				formatf(s_PartFlagMatches_LeftInclude_RightType, sizeof(s_PartFlagMatches_LeftInclude_RightType), "0x%08x", includeFlags_Leftpart & typeFlags_Rightpart);
				GetNamesFromFlags(s_PartFlagMatchesString_LeftInclude_RightType, sizeof(s_PartFlagMatchesString_LeftInclude_RightType), includeFlags_Leftpart & typeFlags_Rightpart, "* NO MATCH *");
				formatf(s_PartFlagMatches_LeftType_RightInclude, sizeof(s_PartFlagMatches_LeftType_RightInclude), "0x%08x", typeFlags_Leftpart & includeFlags_Rightpart);
				GetNamesFromFlags(s_PartFlagMatchesString_LeftType_RightInclude, sizeof(s_PartFlagMatchesString_LeftType_RightInclude), typeFlags_Leftpart & includeFlags_Rightpart, "* NO MATCH *");
			}
		}
	}
	else
	{
		m_CompareInst = BAD_INDEX;

		strcpy(s_InstPtrString_comp, "* left click to select *");
		strcpy(s_LevelIndexString_comp, "* left click to select *");
		strcpy(s_ArchetypeNameString_comp, "* left click to select *");
		strcpy(s_TypeFlagsString_comp, "* left click to select *");
		strcpy(s_TypeFlagNamesString_comp, "* left click to select *");
		strcpy(s_IncludeFlagsString_comp, "* left click to select *");
		strcpy(s_IncludeFlagNamesString_comp, "* left click to select *");
		strcpy(s_ComponentNumberString_comp, "* left click to select *");
		strcpy(s_PartNumberString_comp, "* left click to select *");
		strcpy(s_BoundPtrString_comp, "* left click to select *");
		strcpy(s_BoundTypeString_comp, "* left click to select *");
		strcpy(s_CompositeBoundCountString_comp, "* left click to select *");
		strcpy(s_PartBoundPtrString_comp, "* left click to select *");
		strcpy(s_PartBoundTypeString_comp, "* left click to select *");
		strcpy(s_CompositeTypeFlagsString_comp, "* left click to select *");
		strcpy(s_CompositeTypeFlagNamesString_comp, "* left click to select *");
		strcpy(s_CompositeIncludeFlagsString_comp, "* left click to select *");
		strcpy(s_CompositeIncludeFlagNamesString_comp, "* left click to select *");

		strcpy(s_InstFlagMatches_LeftInclude_RightType, "* left click to select *");
		strcpy(s_InstFlagMatchesString_LeftInclude_RightType, "* left click to select *");
		strcpy(s_InstFlagMatches_LeftType_RightInclude, "* left click to select *");
		strcpy(s_InstFlagMatchesString_LeftType_RightInclude, "* left click to select *");

		strcpy(s_FlagMatches_LeftPartInclude_RightType, "* left click to select *");
		strcpy(s_FlagMatchesString_LeftPartInclude_RightType, "* left click to select *");
		strcpy(s_FlagMatches_LeftPartType_RightInclude, "* left click to select *");
		strcpy(s_FlagMatchesString_LeftPartType_RightInclude, "* left click to select *");

		strcpy(s_FlagMatches_LeftInclude_RightPartType, "* left click to select *");
		strcpy(s_FlagMatchesString_LeftInclude_RightPartType, "* left click to select *");
		strcpy(s_FlagMatches_LeftType_RightPartInclude, "* left click to select *");
		strcpy(s_FlagMatchesString_LeftType_RightPartInclude, "* left click to select *");

		strcpy(s_PartFlagMatches_LeftInclude_RightType, "* left click to select *");
		strcpy(s_PartFlagMatchesString_LeftInclude_RightType, "* left click to select *");
		strcpy(s_PartFlagMatches_LeftType_RightInclude, "* left click to select *");
		strcpy(s_PartFlagMatchesString_LeftType_RightInclude, "* left click to select *");
	}

	m_ExplosionMgr->Update();
}

void phMouseInput::UpdateSpring(Spring* spring, bool paused, Vec::V3Param128 timeStep)
{
	Assert(spring);
	
	phInst* attachedInst = (PHLEVEL->LegitLevelIndex(spring->instLevelIndex) && PHLEVEL->GetState(spring->instLevelIndex)!=phLevelBase::OBJECTSTATE_NONEXISTENT) ? PHLEVEL->GetInstance(spring->instLevelIndex) : NULL;

	if (attachedInst && attachedInst->IsInLevel())
	{
		// Our spring has got a good instance and we should update, so let's get to work.
		if (spring->constraintHandle.IsValid())
		{
			phConstraintAttachment* constraintAttachment = static_cast<phConstraintAttachment*>(PHCONSTRAINT->GetTemporaryPointer(spring->constraintHandle));
			if(constraintAttachment)
			{
				constraintAttachment->SetLocalPosB(RCC_VEC3V(spring->attachmentPoint));

				if( constraintAttachment->GetInstanceA() )
				{
					spring->worldPoint.Set(VEC3V_TO_VECTOR3(constraintAttachment->GetWorldPosA()));
				}
			}
			else
			{
				spring->constraintHandle.Reset();
			}
		}
		else
		{
			// First we need to update the world space position of the end of the spring that is attached to an object.
			Vector3 localGrabPoint;
			if (attachedInst->GetArchetype()->GetBound()->GetType()==phBound::COMPOSITE)
			{
				// The object we're attached to is a composite object.  This means that the localPoint member of the spring is a position in the local space of
				//   the individual component's bound.  First we must transform it by that bound's transform into order to get it into the local space of the instance.
				const phBoundComposite& compositeBound = *static_cast<phBoundComposite*>(attachedInst->GetArchetype()->GetBound());
				RCC_MATRIX34(compositeBound.GetCurrentMatrix(spring->component)).Transform(spring->localPoint,localGrabPoint);
			}
			else
			{
				localGrabPoint.Set(spring->localPoint);
			}
			// Transform from the instance's local space into world space.
			(*(const Matrix34*)(&attachedInst->GetMatrix())).Transform(localGrabPoint, spring->worldPoint); 

			if (!paused)
			{
				// Create a vector from the object end to the non-object end of the spring.
				Vector3 delta;
				delta.Subtract(spring->attachmentPoint, spring->worldPoint);

				// Calculate how much of a force our spring should apply to the object to which it is connected.
				// This spring uses the inertia of the object and scales its strength so that two objects of greatly
				// differing masses can both be manipulated effectively by a spring.
				Vector3 localVel(ORIGIN);
				float effectiveMass = 0.0f;
				phCollider* collider = PHSIM->GetCollider(attachedInst->GetLevelIndex());
				if (!collider)
				{
					collider = PHSIM->ActivateObject(attachedInst);
				}

				if (collider)
				{
					// Get the collider's velocity at the spring end.
					localVel = VEC3V_TO_VECTOR3(collider->GetLocalVelocity(VECTOR3_TO_INTRIN(spring->worldPoint), spring->component));

					// Adjust the spring's effective position to half way toward the collider point's next position.
					delta.SubtractScaled(localVel,0.5f*TIME.GetSeconds());

					// Get the spring direction from the mouse point to the object point.
					Vector3 deltaNorm(delta);
					deltaNorm.NormalizeSafe(YAXIS);

					Matrix33 imm;
					collider->GetInvMassMatrix(RC_MAT33V(imm),spring->worldPoint,NULL,spring->component,spring->component);
					Vector3 tranDeltaNorm;
					imm.Transform(deltaNorm,tranDeltaNorm);
					const float smallestMass = 0.001f;
					effectiveMass = 1.0f/Max(tranDeltaNorm.Dot(deltaNorm),smallestMass);
				}
				else
				{
					effectiveMass = attachedInst->GetArchetype()->GetMass();
				}

				delta.Scale(effectiveMass * sm_GrabSpringConstant);

				// Dampen our spring force based on the local velocity at the point of attachment.
				if (collider)
				{
					if (collider->IsArticulated())
					{
						delta.SubtractScaled(localVel,2.0f*sm_GrabSpringConstant*sm_GrabSpringDamping*effectiveMass);
					}
					else
					{
						delta.SubtractScaled(localVel,sm_GrabSpringConstant*sm_GrabSpringDamping*effectiveMass);
					}
				}

				// This applies a force instead of an impulse because it can repeat for multiple frames.
				// The contact manager reacts to forces before they affect the object's motion, while
				// impulses change the object's motion before the contact manager updates.
				delta.Scale(TIME.GetInvSeconds());
				PHSIM->ApplyForce(timeStep, attachedInst->GetLevelIndex(), delta, spring->worldPoint, spring->component);
			}
		}
	}
	else
	{
		spring->instLevelIndex = phInst::INVALID_INDEX;

		if (spring->constraintHandle.IsValid())
		{
			PHCONSTRAINT->Remove(spring->constraintHandle);
			spring->constraintHandle.Reset();
		}
	}
}

void DrawPrimitive(phBoundPolyhedron* boundPoly, int selectedPart, const Matrix34& partMtx, const bool drawBorder = false, const float normalOffset = 0.0f)
{
	const phPrimitive* prim;
	if(boundPoly->GetType() == phBound::BVH)
		prim = &static_cast<phBoundBVH*>(boundPoly)->GetPrimitive(selectedPart);
	else
		prim = &boundPoly->GetPolygon(selectedPart).GetPrimitive();

	switch (prim->GetType())
	{
	case PRIM_TYPE_POLYGON:
		{
			const phPolygon& polygon = prim->GetPolygon();
			grcWorldMtx(partMtx);

			int numVertices = POLY_MAX_VERTICES;

			const Vec3V normal = boundPoly->GetPolygonUnitNormal(selectedPart);
			const Vec3V offset = ScalarV(normalOffset) * normal;
			const Vec3V v0 = boundPoly->GetVertex(polygon.GetVertexIndex(0)) + offset;
			const Vec3V v1 = boundPoly->GetVertex(polygon.GetVertexIndex(1)) + offset;
			const Vec3V v2 = boundPoly->GetVertex(polygon.GetVertexIndex(2)) + offset;

			grcBindTexture(NULL);
			grcBegin(drawTris,numVertices);
			grcNormal3f(normal);
			grcVertex3f(v0);
			grcVertex3f(v1);
			grcVertex3f(v2);
			grcEnd();

			if (drawBorder)
			{
				grcBegin(drawLineStrip,4);
				const Color32 saveColor = grcGetCurrentColor();
				grcColor(Color_white);
				grcVertex3f(v0);
				grcVertex3f(v1);
				grcVertex3f(v2);
				grcVertex3f(v0);
				grcColor(saveColor);
				grcEnd();
			}
		}
		break;
	case PRIM_TYPE_SPHERE:
		{
			const phPrimSphere &primSphere = prim->GetSphere();
			rage::Vector3 sphereCenter;
			partMtx.Transform(VEC3V_TO_VECTOR3(boundPoly->GetVertex(primSphere.GetCenterIndex())), sphereCenter);
			grcDrawSphere(primSphere.GetRadius(), sphereCenter, 20, true, true);
		}
		break;
	case PRIM_TYPE_CAPSULE:
		{
			const phPrimCapsule & primCapsule = prim->GetCapsule();

			const Vector3 vCapsuleEnd0 = VEC3V_TO_VECTOR3(boundPoly->GetVertex(primCapsule.GetEndIndex0()));
			const Vector3 vCapsuleEnd1 = VEC3V_TO_VECTOR3(boundPoly->GetVertex(primCapsule.GetEndIndex1()));

			Matrix34 capsuleMatrix;
			const Vector3 vCapsuleShaft(vCapsuleEnd1 - vCapsuleEnd0);
			// Check if the shaft axis is parallel to the y-axis.
			if( vCapsuleShaft.x != 0.0f || vCapsuleShaft.z != 0.0f )
			{
				capsuleMatrix.a.Cross(vCapsuleShaft, YAXIS);
				capsuleMatrix.a.Normalize();
				capsuleMatrix.b = vCapsuleShaft;
				capsuleMatrix.b.Normalize();
				capsuleMatrix.c.Cross(capsuleMatrix.a, capsuleMatrix.b);
			}
			else
			{
				capsuleMatrix.Identity();
			}
			capsuleMatrix.d.Average(vCapsuleEnd0, vCapsuleEnd1);
			capsuleMatrix.Dot(partMtx);

			grcWorldIdentity();
			grcDrawCapsule(vCapsuleShaft.Mag(), primCapsule.GetRadius(), capsuleMatrix, 8, true);
		}
		break;
	case PRIM_TYPE_BOX:
		{
			const phPrimBox & primBox = prim->GetBox();

			const int iVertIndex0 = primBox.GetVertexIndex(0);
			const int iVertIndex1 = primBox.GetVertexIndex(1);
			const int iVertIndex2 = primBox.GetVertexIndex(2);
			const int iVertIndex3 = primBox.GetVertexIndex(3);

			const Vec3V vVert0 = boundPoly->GetVertex(iVertIndex0);
			const Vec3V vVert1 = boundPoly->GetVertex(iVertIndex1);
			const Vec3V vVert2 = boundPoly->GetVertex(iVertIndex2);
			const Vec3V vVert3 = boundPoly->GetVertex(iVertIndex3);

			Mat34V localBoxMatrix;
			Vec3V boxSize;
			ScalarV maxMargin;
			geomBoxes::ComputeBoxDataFromOppositeDiagonals(vVert0, vVert1, vVert2, vVert3, localBoxMatrix, boxSize, maxMargin);

			Matrix34 boxMatrix = RC_MATRIX34(localBoxMatrix);
			Vector3 vBoxSize = RC_VECTOR3(boxSize);
			grcWorldIdentity();
			grcDrawSolidBox(vBoxSize, boxMatrix, Color_yellow);
		}
		break;
	default:
		{
			const phPrimCylinder & primCylinder = prim->GetCylinder();

			const Vector3 vCylinderEnd0 = VEC3V_TO_VECTOR3(boundPoly->GetVertex(primCylinder.GetEndIndex0()));
			const Vector3 vCylinderEnd1 = VEC3V_TO_VECTOR3(boundPoly->GetVertex(primCylinder.GetEndIndex1()));

			Matrix34 cylinderMatrix;
			const Vector3 vCylinderShaft(vCylinderEnd1 - vCylinderEnd0);
			// Check if the shaft axis is parallel to the y-axis.
			if( vCylinderShaft.x != 0.0f || vCylinderShaft.z != 0.0f )
			{
				cylinderMatrix.a.Cross(vCylinderShaft, YAXIS);
				cylinderMatrix.a.Normalize();
				cylinderMatrix.b = vCylinderShaft;
				cylinderMatrix.b.Normalize();
				cylinderMatrix.c.Cross(cylinderMatrix.a, cylinderMatrix.b);
			}
			else
			{
				cylinderMatrix.Identity();
			}
			cylinderMatrix.d.Average(vCylinderEnd0, vCylinderEnd1);
			cylinderMatrix.Dot(partMtx);

			grcWorldIdentity();
			grcDrawCylinder(vCylinderShaft.Mag(), primCylinder.GetRadius(), cylinderMatrix, 8, true);
		}
		break;
	}
}

BvhRenderOpts::BvhRenderOpts()
{
	drawPrims = true;
	drawBorder = true;
	drawBoundingBox = true;
	drawDepth = true;
	boundingBoxSizeOffset = Vec3V(.1f,.1f,.1f);
	depthOffset = Vec3V(V_ZERO);
	normalOffset = 0.1f;
	normalOffsetInc = 0.0f;
	queryHalfSize = Vec3V(V_TEN);
	mode = 0;
}

void RenderBvhLeafNode(phBoundPolyhedron* boundPoly, const int node_i, Matrix34 & partMtx, const BvhRenderOpts & renderOpts, int offsetCount, int depth)
{
	const phBoundBVH* boundBvh = static_cast<phBoundBVH*>(boundPoly);
	const phOptimizedBvh* bvhStructure = boundBvh->GetBVH();
	const phOptimizedBvhNode* rootNode = bvhStructure->GetRootNode();
	const int numNodes = bvhStructure->GetNumNodes();
	(void)numNodes;
	Assert(node_i >= 0 && node_i < numNodes);
	const phOptimizedBvhNode& containingNode = rootNode[node_i];
	const int polyIndexStart = containingNode.GetPolygonStartIndex();
	const int polyCount = containingNode.GetPolygonCount();
	const int polyIndexEnd = polyIndexStart + polyCount;

	bool drawPrims = renderOpts.drawPrims;
	bool drawBorder = renderOpts.drawBorder;
	bool drawBoundingBox = renderOpts.drawBoundingBox;
	bool drawDepth = renderOpts.drawDepth;
	Vec3V boundingBoxSizeOffset = renderOpts.boundingBoxSizeOffset;
	Vec3V depthOffset = renderOpts.depthOffset;
	float normalOffset = renderOpts.normalOffset + offsetCount * renderOpts.normalOffsetInc;

	if (drawPrims)
	{
		for(int p = polyIndexStart; p < polyIndexEnd; p++)
			DrawPrimitive(boundPoly, p, partMtx, drawBorder, normalOffset);
	}
	if (drawBoundingBox || drawDepth)
	{
		const Color32 saveColor = grcGetCurrentColor();
		// This code for getting the bounding box was taken from InitBoundingBoxInfoFromBVHNode in collision.cpp
		Vec3V vBoundingBoxMin, vBoundingBoxMax;
		bvhStructure->UnQuantize(RC_VECTOR3(vBoundingBoxMin), containingNode.m_AABBMin);
		bvhStructure->UnQuantize(RC_VECTOR3(vBoundingBoxMax), containingNode.m_AABBMax);
		const Vec3V boxSize_ = vBoundingBoxMax - vBoundingBoxMin + boundingBoxSizeOffset;
		const Vector3 boxSize = RCC_VECTOR3(boxSize_); 
		const Vec3V boxCenter_ = Transform(RCC_MAT34V(partMtx),ScalarV(V_HALF) * (vBoundingBoxMin + vBoundingBoxMax));
		const Vector3 boxCenter = RCC_VECTOR3(boxCenter_);
		if (drawBoundingBox)
		{
			const Vector3 dSave = partMtx.d;
			partMtx.d = boxCenter;
			grcColor(Color_yellow);
			grcDrawBox(boxSize,partMtx,grcGetCurrentColor());
			partMtx.d = dSave;
		}
		if (drawDepth)
		{
			const Vec3V labelPos_ = Transform3x3(RCC_MAT34V(partMtx),depthOffset)+RCC_VEC3V(boxCenter);
			const Vector3 labelPos = RCC_VECTOR3(labelPos_);
			grcColor(Color_green);
			grcDrawLabelf(labelPos,"%d",depth);
		}
		grcColor(saveColor);
	}
}

void RenderBvhSubTree(phBoundPolyhedron* boundPoly, const int node_i, Matrix34 & partMtx, const BvhRenderOpts & renderOpts, int offsetCount, int depth)
{
	const phBoundBVH* boundBvh = static_cast<phBoundBVH*>(boundPoly);
	const phOptimizedBvh* bvhStructure = boundBvh->GetBVH();
	const phOptimizedBvhNode* rootNode = bvhStructure->GetRootNode();
	const int numNodes = bvhStructure->GetNumNodes();
	(void)numNodes;
	Assert(node_i >= 0 && node_i < numNodes);
	if (rootNode[node_i].IsLeafNode())
		RenderBvhLeafNode(boundPoly,node_i,partMtx,renderOpts,offsetCount,depth);
	else
	{
		const int left_i = node_i + 1;
		const int right_i = left_i + rootNode[left_i].GetEscapeIndex();
		RenderBvhSubTree(boundPoly,left_i,partMtx,renderOpts,offsetCount+1,depth+1);
		RenderBvhSubTree(boundPoly,right_i,partMtx,renderOpts,offsetCount+1,depth+1);
	}
}

void RenderBvhSubTree(phBoundPolyhedron* boundPoly, const int node_i, Matrix34 & partMtx, Vec3V_In aabbMin, Vec3V_In aabbMax, const BvhRenderOpts & renderOpts, int offsetCount, int depth, int * renderCount)
{
	const phBoundBVH* boundBvh = static_cast<phBoundBVH*>(boundPoly);
	const phOptimizedBvh* bvhStructure = boundBvh->GetBVH();
	const phOptimizedBvhNode* rootNode = bvhStructure->GetRootNode();
	const int numNodes = bvhStructure->GetNumNodes();
	(void)numNodes;
	Assert(node_i >= 0 && node_i < numNodes);
	static Color32 listColor[6] = {Color_red,Color_blue,Color_yellow,Color_green,Color_orange,Color_blue4};

	Vec3V vBoundingBoxMin, vBoundingBoxMax;
	bvhStructure->UnQuantize(RC_VECTOR3(vBoundingBoxMin), rootNode[node_i].m_AABBMin);
	bvhStructure->UnQuantize(RC_VECTOR3(vBoundingBoxMax), rootNode[node_i].m_AABBMax);

	if (IsLessThanAll(aabbMin,vBoundingBoxMax) && IsGreaterThanAll(aabbMax,vBoundingBoxMin))
	{
		if (rootNode[node_i].IsLeafNode())
		{
			Color32 saveColor = grcGetCurrentColor();
			Assert(*renderCount < 6);
			grcColor(listColor[*renderCount]);
			*renderCount = (*renderCount+1) % 6;
			RenderBvhLeafNode(boundPoly,node_i,partMtx,renderOpts,offsetCount,depth);
			grcColor(saveColor);
		}
		else
		{
			const int left_i = node_i + 1;
			const int right_i = left_i + rootNode[left_i].GetEscapeIndex();
			RenderBvhSubTree(boundPoly,left_i,partMtx,aabbMin,aabbMax,renderOpts,offsetCount+1,depth+1,renderCount);
			RenderBvhSubTree(boundPoly,right_i,partMtx,aabbMin,aabbMax,renderOpts,offsetCount+1,depth+1,renderCount);
		}
	}
}

void RenderBvhSubTreeStart(phBoundPolyhedron* boundPoly, Matrix34 & partMtx, Vec3V_In aabbMinLoc, Vec3V_In aabbMaxLoc, const BvhRenderOpts & renderOpts)
{
	const int node_i = 0;
	const int offsetCount = 0;
	const int depth = 0;
	int renderCount = 0;

	RenderBvhSubTree(boundPoly,node_i,partMtx,aabbMinLoc,aabbMaxLoc,renderOpts,offsetCount,depth,&renderCount);
}

void phMouseInput::Draw()
{
	if (sm_EnableLeftClick == false && sm_EnableRightClick == false)
	{
		return;
	}

	const Vector3 SIZE(0.025f, 0.025f, 0.025f);
	Mat34V ident(V_IDENTITY);
	Matrix34 boxMatrix = RCC_MATRIX34(ident);

	bool oldLighting = grcLighting(false);

	if (m_SelectedInst != BAD_INDEX)
	{
		if (!PHLEVEL->IsInLevel(m_SelectedInst))
		{
			m_SelectedInst = BAD_INDEX;
			m_SelectedComponent = 0;
		}
		else
		{
			phInst* inst = PHLEVEL->GetInstance(m_SelectedInst);

			phBound* bound = inst->GetArchetype()->GetBound();
			Assert(bound);

			Vector3 boxCenter = VEC3V_TO_VECTOR3(Average(bound->GetBoundingBoxMin(), bound->GetBoundingBoxMax()));
			(*(const Matrix34*)(&inst->GetMatrix())).Transform(boxCenter);
			Matrix34 boxMtx = (*(const Matrix34*)(&inst->GetMatrix()));
			boxMtx.d = boxCenter;

			Vector3 boxSize = VEC3V_TO_VECTOR3(bound->GetBoundingBoxSize());

			grcDrawBox(boxSize, boxMtx, Color_yellow);

			Matrix34 partMtx = RCC_MATRIX34(inst->GetMatrix());

			//////////////////////////////////////////////////////////////////////////
			int octreeIndex = PHLEVEL->GetOctreeNodeIndex(m_SelectedInst);
			phLevelNode* octreeNode = PHLEVEL->GetOctreeNode(octreeIndex);
			Assertf(PHLEVEL->IsObjectContained(octreeNode, m_SelectedInst), "Instance is not contained in octree node it thinks it is.");

			Vec3V octTreeMaxs = Vec3VFromF32(octreeNode->m_CenterXYZHalfWidthW.GetWf());
			Vec3V octTreeMins = Subtract(octreeNode->m_CenterXYZHalfWidthW.GetXYZ(), octTreeMaxs);
			octTreeMaxs = Add(octTreeMaxs, octreeNode->m_CenterXYZHalfWidthW.GetXYZ());

			grcDrawBox(VEC3V_TO_VECTOR3(octTreeMins), VEC3V_TO_VECTOR3(octTreeMaxs), Color_orange);
			//////////////////////////////////////////////////////////////////////////

			if (bound->GetType() == phBound::COMPOSITE)
			{
				const phBoundComposite* composite = static_cast<const phBoundComposite*>(bound);

				if (phBound* partBound = composite->GetBound(m_SelectedComponent))
				{
					Vector3 partMins = RCC_VECTOR3(composite->GetLocalBoxMins(m_SelectedComponent));
					Vector3 partMaxs = RCC_VECTOR3(composite->GetLocalBoxMaxs(m_SelectedComponent));
					Vector3 partCenter;
					partCenter.Average(partMins, partMaxs);
					Vector3 partSize;
					partSize.Subtract(partMaxs, partMins);

					partMtx.DotFromLeft(RCC_MATRIX34(composite->GetCurrentMatrix(m_SelectedComponent)));

					boxMtx.Set(partMtx);
					boxMtx.Transform(partCenter);
					boxMtx.d = partCenter;

					grcDrawBox(partSize, boxMtx, Color_yellow);

					bound = partBound;
				}
			}

			if (bound->GetType() == phBound::GEOMETRY)
			{
				phBoundPolyhedron* boundPoly = static_cast<phBoundPolyhedron*>(bound);
				if(!sm_DrawAllCompParts)
				{
					// Crash protection for when we missed an inst change (we don't check generation ids)
					m_SelectedPart = Min(m_SelectedPart, boundPoly->GetNumPolygons()-1);

					const phPolygon& polygon = boundPoly->GetPolygon(m_SelectedPart);
					int numVertices = POLY_MAX_VERTICES;

					grcWorldMtx(partMtx);

					grcBindTexture(NULL);
					grcBegin(drawTris,numVertices);
					grcNormal3f(boundPoly->GetPolygonUnitNormal(m_SelectedPart));
					grcVertex3fv(&boundPoly->GetVertex(polygon.GetVertexIndex(0))[0]);
					grcVertex3fv(&boundPoly->GetVertex(polygon.GetVertexIndex(1))[0]);
					grcVertex3fv(&boundPoly->GetVertex(polygon.GetVertexIndex(2))[0]);
					grcEnd();
				}
				else
				{
					grcWorldMtx(partMtx);
					grcBindTexture(NULL);

					int numPolys = boundPoly->GetNumPolygons();
					for(int i = 0; i < numPolys; i++)
					{
						const phPolygon& polygon = boundPoly->GetPolygon(i);
						int numVertices = POLY_MAX_VERTICES;

						grcBegin(drawTris,numVertices);
						grcNormal3f(boundPoly->GetPolygonUnitNormal(i));
						grcVertex3fv(&boundPoly->GetVertex(polygon.GetVertexIndex(0))[0]);
						grcVertex3fv(&boundPoly->GetVertex(polygon.GetVertexIndex(1))[0]);
						grcVertex3fv(&boundPoly->GetVertex(polygon.GetVertexIndex(2))[0]);
						grcEnd();
					}
				}
			}
			else if (bound->GetType() == phBound::BVH USE_GRIDS_ONLY(|| bound->GetType() == phBound::GRID))
			{
#if USE_GRIDS
				if (bound->GetType() == phBound::GRID)
				{
					bound = static_cast<phBoundGrid*>(bound)->GetOctree(m_SelectedComponent);
				}
#endif
				phBoundPolyhedron* boundPoly = static_cast<phBoundPolyhedron*>(bound);
				if(sm_DrawBvhLeaf && bound->GetType() == phBound::BVH)
				{
					const phBoundBVH* boundBvh = static_cast<phBoundBVH*>(bound);
					const phOptimizedBvh* bvhStructure = boundBvh->GetBVH();
					const phOptimizedBvhNode* rootNode = bvhStructure->GetRootNode();
					const int numNodes = bvhStructure->GetNumNodes();
					bool foundIt = false;
					int node_i = 0;
					for(; node_i < numNodes; node_i++)
					{
						//////////////////////////////////////////////////////////////////////////
						if(rootNode[node_i].IsLeafNode())
						{
							const int polyIndexStart = rootNode[node_i].GetPolygonStartIndex();
							const int polyCount = rootNode[node_i].GetPolygonCount();
							const int polyIndexEnd = polyIndexStart + polyCount;
							if (m_SelectedPart >= polyIndexStart && m_SelectedPart < polyIndexEnd)
								foundIt = true;
						}
						if(foundIt)
						{
							break;
						}
						//////////////////////////////////////////////////////////////////////////
					}
					if (foundIt)
					{
						if (sm_BvhRenderOpts.mode == 0)
						{
							// Find parent of this node.
							Assert(node_i != 0);	// node_i == 0 is the root of the bvh and has no parent.
							int parent_i = 0;
							int depth = 0;
							for(;;)
							{
								Assert(parent_i >= 0 && parent_i < numNodes);
								Assert(rootNode[parent_i].IsLeafNode() == false);
								const int left_i = parent_i + 1;
								const int right_i = left_i + rootNode[left_i].GetEscapeIndex();
								if (left_i == node_i || right_i == node_i)
									break;
								if (node_i > right_i)
									parent_i = right_i;
								else
									parent_i = left_i;
								depth++;
							}
							const Color32 saveColor = grcGetCurrentColor();
							const int left_i = parent_i + 1;
							const int right_i = left_i + rootNode[left_i].GetEscapeIndex();
							grcColor(Color_red);
							RenderBvhSubTree(boundPoly,left_i,partMtx,sm_BvhRenderOpts,0,depth);
							grcColor(Color_blue);
							RenderBvhSubTree(boundPoly,right_i,partMtx,sm_BvhRenderOpts,0,depth);
							grcColor(saveColor);
						}
						else if (sm_BvhRenderOpts.mode == 1)
						{
							// This code for getting the bounding box was taken from InitBoundingBoxInfoFromBVHNode in collision.cpp
							Vec3V vBoundingBoxMin, vBoundingBoxMax;
							bvhStructure->UnQuantize(RC_VECTOR3(vBoundingBoxMin), rootNode[node_i].m_AABBMin);
							bvhStructure->UnQuantize(RC_VECTOR3(vBoundingBoxMax), rootNode[node_i].m_AABBMax);
							vBoundingBoxMin -= sm_BvhRenderOpts.queryHalfSize;
							vBoundingBoxMax += sm_BvhRenderOpts.queryHalfSize;
							RenderBvhSubTreeStart(boundPoly,partMtx,vBoundingBoxMin,vBoundingBoxMax,sm_BvhRenderOpts);
						}
					}
				}
				else if(!sm_DrawAllCompParts)
				{
					DrawPrimitive(boundPoly, m_SelectedPart, partMtx);
				}
				else
				{
					int numPolys = boundPoly->GetNumPolygons();
					for(int i = 0; i < numPolys; i++)
					{
						DrawPrimitive(boundPoly, i, partMtx);
					}
				}
			}

			grcWorldIdentity();

#if __PFDRAW
			PHCONSTRAINT->ProfileDraw(inst);

			//
			grcWorldIdentity();
			crSkeleton* instSkeleton = inst->GetSkeleton();
			if(sm_DrawSkeleton && instSkeleton != NULL)
			{
				instSkeleton->ProfileDraw();
			}
#endif
		}
	}

	// Draw the line representing the 'grab spring' if it is currently in use.
	if (m_GrabSpring.instLevelIndex != phInst::INVALID_INDEX)
	{
		boxMatrix.d = m_GrabSpring.worldPoint;

		grcDrawSolidBox(SIZE, boxMatrix, PHCONSTRAINT->GetTemporaryPointer(m_GrabSpring.constraintHandle) ? Color_red : Color_white);

		grcWorldIdentity();

		grcBegin(drawLines,2);
		grcVertex3fv(&m_GrabSpring.worldPoint[0]);
		grcVertex3fv(&m_GrabSpring.attachmentPoint[0]);
		grcEnd();
	}

	// Draw the lines representing the placed springs that are currently in use.
	for (int spring = 0; spring < m_NumPlacedSprings; ++spring)
	{
		boxMatrix.d = m_PlacedSprings[spring].worldPoint;

		grcDrawSolidBox(SIZE, boxMatrix, PHCONSTRAINT->GetTemporaryPointer(m_PlacedSprings[spring].constraintHandle) ? Color_red : Color_white);

		grcWorldIdentity();

		grcBegin(drawLines,2);
		grcVertex3fv(&m_PlacedSprings[spring].worldPoint[0]);
		grcVertex3fv(&m_PlacedSprings[spring].attachmentPoint[0]);
		grcEnd();
	}

	grcLighting(oldLighting);
}

#if PHMOUSEINPUT_ENABLE_BOXES
void phMouseInput::SetBoxTypeIncludeFlags(u32 typeFlags, u32 includeFlags)
{
	m_BoxBulletArchetype->SetTypeFlags(typeFlags);
	m_BoxBulletArchetype->SetIncludeFlags(includeFlags);
}
#endif

void phMouseInput::InitBoxGun(int numBoxes)
{
	Assert(m_NumBoxBullets == 0 && m_BoxBulletInsts == NULL);
	m_NumBoxBullets = numBoxes;
	m_CurrentBoxBullet = 0;

	m_BoxBulletBound = rage_new phBoundBox(Vector3(0.5f, 0.5f, 0.5f));
	m_BoxBulletArchetype = rage_new phArchetypePhys;
	m_BoxBulletArchetype->AddRef();
	m_BoxBulletArchetype->SetBound(m_BoxBulletBound);
	m_BoxBulletArchetype->SetMass(sm_BoxMass);
	m_BoxBulletBound->Release();
	m_BoxBulletInsts = rage_new phInst[numBoxes];
	for (int boxIndex = 0; boxIndex < m_NumBoxBullets; ++boxIndex)
	{
		phInst* bullet = &m_BoxBulletInsts[boxIndex];
		bullet->SetArchetype(m_BoxBulletArchetype);
	}
}

void phMouseInput::ShutdownBoxGun()
{
	ResetBoxGun();

	delete [] m_BoxBulletInsts;
	m_BoxBulletInsts = NULL;

	Assert(m_BoxBulletArchetype->GetRefCount()==1);
	m_BoxBulletArchetype->Release();
	m_BoxBulletArchetype = NULL;
}

void phMouseInput::ResetBoxGun()
{
	for (int boxIndex = 0; boxIndex < m_NumBoxBullets; ++boxIndex)
	{
		phInst* bullet = &m_BoxBulletInsts[boxIndex];
		if (bullet->IsInLevel())
		{
			PHSIM->DeleteObject(bullet->GetLevelIndex());
		}
	}
}

void phMouseInput::ThrowBox(Vec3V_In position, Vec3V_In direction)
{
	phInst* bullet = &m_BoxBulletInsts[m_CurrentBoxBullet];

	++m_CurrentBoxBullet;

	if (m_CurrentBoxBullet >= m_NumBoxBullets)
	{
		m_CurrentBoxBullet = 0;
	}

	Vec3V action;
	action = Scale(direction, ScalarVFromF32(sm_BoxSpeed * bullet->GetArchetype()->GetMass()));

	Mat34V resetMatrix(V_IDENTITY);
	resetMatrix.SetCol3(position);

	if (bullet->IsInLevel())
	{
		PHSIM->TeleportObject(*bullet, RCC_MATRIX34(resetMatrix));
		if (phCollider* collider = PHSIM->GetCollider(bullet->GetLevelIndex()))
		{
			collider->Freeze();
		}
		else
		{
			PHSIM->ActivateObject(bullet);
		}

	}
	else
	{
		bullet->SetMatrix(resetMatrix);
		PHSIM->AddActiveObject(bullet);
	}

	PHSIM->ApplyImpulse(bullet->GetLevelIndex(), RCC_VECTOR3(action), RCC_VECTOR3(direction));
}

#if __BANK
void phMouseInput::AddClassWidgets(bkBank& bank)
{
	bank.AddButton("RESET", datCallback(MFA(phMouseInput::Reset), sm_ActiveInstance));

	bank.AddToggle("Left click interact", &sm_EnableLeftClick);

	bank.AddCombo("Click mode", (int*)&sm_LeftClickMode, int(NUM_LEFT_CLICK_MODES), s_LeftClickLJNames, 0);

	bank.AddToggle("Sticky Click", &sm_EnableLeftStickyClickMode);

	bank.AddSlider("Spring Constant", &sm_GrabSpringConstant, 0.0f, 20.0f, 0.1f);
	bank.AddSlider("Spring Damping", &sm_GrabSpringDamping, 0.0f, 1.0f, 0.01f);

	bank.AddSlider("Default Constraint Length", &sm_DefaultConstraintLength, 0.0f, 16.0f, 0.1f);

	bank.AddSlider("Impulse scale", &sm_ImpulseScale, 0.0f, 1000.0f, 1.0f);
	bank.AddToggle("Impulse scale times mass", &sm_ImpulseScaleByMass);
	bank.AddSlider("Break scale(Impulse & Bullet)", &sm_ImpulseBreakScale, 0.0f, 1000.0f, 1.0f);
	bank.AddSlider("Bullet speed", &sm_ImpulseBulletSpeed, 0.0f, 10000.0f, 1.0f);
	bank.AddSlider("Bullet mass", &sm_ImpulseBulletMass, 0.001f, 1000.0f, 0.1f);

#if PHMOUSEINPUT_ENABLE_BOXES
	bank.AddSlider("Box speed", &sm_BoxSpeed, 0.0f, 10000.0f, 0.25f);
	bank.AddSlider("Box mass", &sm_BoxMass, 0.001f, 10000.0f, 0.25f);
#endif

#if PHMOUSEINPUT_ENABLE_EXPLOSIONS
	bank.AddSlider("Explosion speed", &sm_ExplosionType.m_InitialRadiusSpeed, 0.0f, 10000.0f, 0.25f);
	bank.AddSlider("Explosion decay", &sm_ExplosionType.m_DecayFactor, -1000.0f, 0.0f, 0.25f);
	bank.AddSlider("Explosion force", &sm_ExplosionType.m_ForceFactor, 0.0f, 10000.0f, 0.25f);
#endif

	bkGroup* compareGroup = bank.AddGroup("Compare Selections", false);
	compareGroup->AddSlider("Force left click select to Level Index:", &(sm_ActiveInstance->m_CompareInst), -1, phInst::INVALID_INDEX, 1, datCallback(CFA(phMouseInput::LeftClickLevelIndexChangeCb)));
	compareGroup->AddText("Left Incl, Right Type", s_InstFlagMatches_LeftInclude_RightType, RIGHT_CLICK_STRING_LENGTH, true);
	compareGroup->AddText("Left Incl, Right Type", s_InstFlagMatchesString_LeftInclude_RightType, RIGHT_CLICK_STRING_LENGTH, true);
	compareGroup->AddText("Right Incl, Left Type", s_InstFlagMatches_LeftType_RightInclude, RIGHT_CLICK_STRING_LENGTH, true);
	compareGroup->AddText("Right Incl, Left Type", s_InstFlagMatchesString_LeftType_RightInclude, RIGHT_CLICK_STRING_LENGTH, true);
	compareGroup->AddText("LeftPart Incl, Right Type", s_FlagMatches_LeftPartInclude_RightType, RIGHT_CLICK_STRING_LENGTH, true);
	compareGroup->AddText("LeftPart Incl, Right Type", s_FlagMatchesString_LeftPartInclude_RightType, RIGHT_CLICK_STRING_LENGTH, true);
	compareGroup->AddText("Right Incl, LeftPart Type", s_FlagMatches_LeftPartType_RightInclude, RIGHT_CLICK_STRING_LENGTH, true);
	compareGroup->AddText("Right Incl, LeftPart Type", s_FlagMatchesString_LeftPartType_RightInclude, RIGHT_CLICK_STRING_LENGTH, true);
	compareGroup->AddText("Left Incl, RightPart Type", s_FlagMatches_LeftInclude_RightPartType, RIGHT_CLICK_STRING_LENGTH, true);
	compareGroup->AddText("Left Incl, RightPart Type", s_FlagMatchesString_LeftInclude_RightPartType, RIGHT_CLICK_STRING_LENGTH, true);
	compareGroup->AddText("RightPart Incl, Left Type", s_FlagMatches_LeftType_RightPartInclude, RIGHT_CLICK_STRING_LENGTH, true);
	compareGroup->AddText("RightPart Incl, Left Type", s_FlagMatchesString_LeftType_RightPartInclude, RIGHT_CLICK_STRING_LENGTH, true);
	compareGroup->AddText("LeftPart Incl, RightPart Type", s_PartFlagMatches_LeftInclude_RightType, RIGHT_CLICK_STRING_LENGTH, true);
	compareGroup->AddText("LeftPart Incl, RightPart Type", s_PartFlagMatchesString_LeftInclude_RightType, RIGHT_CLICK_STRING_LENGTH, true);
	compareGroup->AddText("RightPart Incl, LeftPart Type", s_PartFlagMatches_LeftType_RightInclude, RIGHT_CLICK_STRING_LENGTH, true);
	compareGroup->AddText("RightPart Incl, LeftPart Type", s_PartFlagMatchesString_LeftType_RightInclude, RIGHT_CLICK_STRING_LENGTH, true);

	bkGroup* leftSelectInfo = compareGroup->AddGroup("Left Selection Info", false);
	leftSelectInfo->AddText("Inst", s_InstPtrString_comp, RIGHT_CLICK_STRING_LENGTH, true);
	leftSelectInfo->AddText("Level Index", s_LevelIndexString_comp, RIGHT_CLICK_STRING_LENGTH, true);
	leftSelectInfo->AddText("Name", s_ArchetypeNameString_comp, RIGHT_CLICK_STRING_LENGTH, true);
	leftSelectInfo->AddText("Type Flags", s_TypeFlagsString_comp, RIGHT_CLICK_STRING_LENGTH, true);
	leftSelectInfo->AddText("Type Flag Names", s_TypeFlagNamesString_comp, RIGHT_CLICK_STRING_LENGTH, true);
	leftSelectInfo->AddText("Include Flags", s_IncludeFlagsString_comp, RIGHT_CLICK_STRING_LENGTH, true);
	leftSelectInfo->AddText("Include Flag Names", s_IncludeFlagNamesString_comp, RIGHT_CLICK_STRING_LENGTH, true);
	leftSelectInfo->AddText("Component", s_ComponentNumberString_comp, RIGHT_CLICK_STRING_LENGTH, true);
	leftSelectInfo->AddText("Part#", s_PartNumberString_comp, RIGHT_CLICK_STRING_LENGTH, true);
	leftSelectInfo->AddText("Bound", s_BoundPtrString_comp, RIGHT_CLICK_STRING_LENGTH, true);
	leftSelectInfo->AddText("Bound Type", s_BoundTypeString_comp, RIGHT_CLICK_STRING_LENGTH, true);
	leftSelectInfo->AddText("Composite Parts", s_CompositeBoundCountString_comp, RIGHT_CLICK_STRING_LENGTH, true);
	leftSelectInfo->AddText("Part Bound", s_PartBoundPtrString_comp, RIGHT_CLICK_STRING_LENGTH, true);
	leftSelectInfo->AddText("Part Bound Type", s_PartBoundTypeString_comp, RIGHT_CLICK_STRING_LENGTH, true);
	leftSelectInfo->AddText("Part Type Flags", s_CompositeTypeFlagsString_comp, RIGHT_CLICK_STRING_LENGTH, true);
	leftSelectInfo->AddText("Part Type Flag Names", s_CompositeTypeFlagNamesString_comp, RIGHT_CLICK_STRING_LENGTH, true);
	leftSelectInfo->AddText("Part Incl. Flags", s_CompositeIncludeFlagsString_comp, RIGHT_CLICK_STRING_LENGTH, true);
	leftSelectInfo->AddText("Part Incl. Flag Names", s_CompositeIncludeFlagNamesString_comp, RIGHT_CLICK_STRING_LENGTH, true);

#if __DEV
	bank.AddButton("Debug collider update", datCallback(CFA(phMouseInput::DebugColliderUpdateCb)));
	bank.AddButton("Dump selected inst manifolds", datCallback(MFA(phMouseInput::DumpManifoldsWithSelectedInst),sm_ActiveInstance));
	bank.AddButton("Dump selected inst invalid state", datCallback(MFA(phMouseInput::DumpInvalidStateWithSelectedInst),sm_ActiveInstance));
#endif

	bank.AddSlider("Force right click select to Level Index:", &(sm_ActiveInstance->m_SelectedInst), -1, phInst::INVALID_INDEX, 1, datCallback(CFA(phMouseInput::RightClickLevelIndexChangeCb)));
	bank.AddToggle("Right click select", &sm_EnableRightClick);
	bank.AddToggle("Draw Skeleton", &sm_DrawSkeleton);

	bank.AddButton("Next component", datCallback(CFA(phMouseInput::NextComponentCb)));
	bank.AddButton("Previous component", datCallback(CFA(phMouseInput::PreviousComponentCb)));
	bank.AddButton("Next part", datCallback(CFA(phMouseInput::NextPartCb)));
	bank.AddButton("Previous part", datCallback(CFA(phMouseInput::PreviousPartCb)));
	bank.AddToggle("Draw all component parts", &sm_DrawAllCompParts);
	bank.AddToggle("Draw BVH leaf", &sm_DrawBvhLeaf);

	bank.AddText("Inst", s_InstPtrString, RIGHT_CLICK_STRING_LENGTH, true);
	bank.AddText("Level Index", s_LevelIndexString, RIGHT_CLICK_STRING_LENGTH, true);
	bank.AddText("Name", s_ArchetypeNameString, RIGHT_CLICK_STRING_LENGTH, true);
	bank.AddText("Bound", s_BoundPtrString, RIGHT_CLICK_STRING_LENGTH, true);
	bank.AddText("Bound Type", s_BoundTypeString, RIGHT_CLICK_STRING_LENGTH, true);
	bank.AddText("Collider", s_ColliderPtrString, RIGHT_CLICK_STRING_LENGTH, true);
	bank.AddText("Type Flags", s_TypeFlagsString, RIGHT_CLICK_STRING_LENGTH, true);
	bank.AddText("Type Flag Names", s_TypeFlagNamesString, RIGHT_CLICK_STRING_LENGTH, true);
	bank.AddText("Include Flags", s_IncludeFlagsString, RIGHT_CLICK_STRING_LENGTH, true);
	bank.AddText("Include Flag Names", s_IncludeFlagNamesString, RIGHT_CLICK_STRING_LENGTH, true);
	bank.AddText("Num of Components", s_CompositeBoundCountString, RIGHT_CLICK_STRING_LENGTH, true);
	bank.AddText("Component#", s_ComponentNumberString, RIGHT_CLICK_STRING_LENGTH, true);
	bank.AddText("Num of Parts", s_PartCountString, RIGHT_CLICK_STRING_LENGTH, true);
	bank.AddText("Part#", s_PartNumberString, RIGHT_CLICK_STRING_LENGTH, true);
	bank.AddText("Part Type", s_PartTypeString, RIGHT_CLICK_STRING_LENGTH, true);
	bank.AddText("Part Normal", s_PartNormalString, RIGHT_CLICK_STRING_LENGTH, true);
	bank.AddText("MatId", s_MatIdString, RIGHT_CLICK_STRING_LENGTH, true);
	bank.AddText("Material", s_MaterialString, RIGHT_CLICK_STRING_LENGTH, true);
	bank.AddText("Part Bound", s_PartBoundPtrString, RIGHT_CLICK_STRING_LENGTH, true);
	bank.AddText("Part Bound Type", s_PartBoundTypeString, RIGHT_CLICK_STRING_LENGTH, true);
	bank.AddText("Part Type Flags", s_CompositeTypeFlagsString, RIGHT_CLICK_STRING_LENGTH, true);
	bank.AddText("Part Type Flag Names", s_CompositeTypeFlagNamesString, RIGHT_CLICK_STRING_LENGTH, true);
	bank.AddText("Part Incl. Flags", s_CompositeIncludeFlagsString, RIGHT_CLICK_STRING_LENGTH, true);
	bank.AddText("Part Incl. Flag Names", s_CompositeIncludeFlagNamesString, RIGHT_CLICK_STRING_LENGTH, true);
	bank.AddText("Sleep Mode", s_SleepModeString, RIGHT_CLICK_STRING_LENGTH, true);
	bank.AddText("Asleep Ticks", s_AsleepTicksString, RIGHT_CLICK_STRING_LENGTH, true);
	bank.AddText("Motionless Ticks", s_MotionlessTicksString, RIGHT_CLICK_STRING_LENGTH, true);
	bank.AddSlider("Polling Update Delay", &s_FrameDelay, 0, 30, 1);
	bank.AddText("Dist to camera", s_DistanceToCameraString, RIGHT_CLICK_STRING_LENGTH, true);
	bank.AddText("Reference Frame", s_ReferenceFrameString, RIGHT_CLICK_STRING_LENGTH, true);
	bank.AddText("Velocity", s_VelocityString, RIGHT_CLICK_STRING_LENGTH, true);
	bank.AddText("Angular Velocity", s_AngVelocityString, RIGHT_CLICK_STRING_LENGTH, true);
	bank.AddText("Approximate Radius", s_ApproximateRadiusString, RIGHT_CLICK_STRING_LENGTH, true);
	bank.AddText("Speed", s_SpeedString, RIGHT_CLICK_STRING_LENGTH, true);
	bank.AddText("Link#", s_LinkNumberString, RIGHT_CLICK_STRING_LENGTH, true);
	bank.AddText("Archetype Mass", s_ArchetypeMassString, RIGHT_CLICK_STRING_LENGTH, true);
	bank.AddText("Collider Mass", s_ColliderMassString, RIGHT_CLICK_STRING_LENGTH, true);
	bank.AddText("Articulated Collider Mass", s_ArticulatedColliderMassString, RIGHT_CLICK_STRING_LENGTH, true);
	bank.AddText("Link Mass", s_LinkMassString, RIGHT_CLICK_STRING_LENGTH, true);
	bank.AddText("Archetype Angular Inertia", s_ArchetypeAngularInertiaString, RIGHT_CLICK_STRING_LENGTH, true);
	bank.AddText("Collider Angular Inertia", s_ColliderAngularInertiaString, RIGHT_CLICK_STRING_LENGTH, true);
	bank.AddText("Link Angular Inertia", s_LinkAngularInertiaString, RIGHT_CLICK_STRING_LENGTH, true);
	bank.AddText("Position", s_PositionString, RIGHT_CLICK_STRING_LENGTH, true);
	bank.AddText("Matrix(Row 1)", s_MatrixString1, RIGHT_CLICK_STRING_LENGTH, true);
	bank.AddText("Matrix(Row 2)", s_MatrixString2, RIGHT_CLICK_STRING_LENGTH, true);
	bank.AddText("Matrix(Row 3)", s_MatrixString3, RIGHT_CLICK_STRING_LENGTH, true);
//	bank.AddText("LastPosition", s_LastPositionString, RIGHT_CLICK_STRING_LENGTH, true);
//	bank.AddText("LastMatrix(1)", s_LastMatrixString1, RIGHT_CLICK_STRING_LENGTH, true);
//	bank.AddText("LastMatrix(2)", s_LastMatrixString2, RIGHT_CLICK_STRING_LENGTH, true);
//	bank.AddText("LastMatrix(3)", s_LastMatrixString3, RIGHT_CLICK_STRING_LENGTH, true);
	bank.AddTitle("Component(Part) Matrices:");
	bank.AddText("Position", s_PartPositionString, RIGHT_CLICK_STRING_LENGTH, true);
	bank.AddText("Matrix(Row 1)", s_PartMatrixString1, RIGHT_CLICK_STRING_LENGTH, true);
	bank.AddText("Matrix(Row 2)", s_PartMatrixString2, RIGHT_CLICK_STRING_LENGTH, true);
	bank.AddText("Matrix(Row 3)", s_PartMatrixString3, RIGHT_CLICK_STRING_LENGTH, true);
//	bank.AddText("LastPosition", s_PartLastPositionString, RIGHT_CLICK_STRING_LENGTH, true);
//	bank.AddText("LastMatrix(1)", s_PartLastMatrixString1, RIGHT_CLICK_STRING_LENGTH, true);
//	bank.AddText("LastMatrix(2)", s_PartLastMatrixString2, RIGHT_CLICK_STRING_LENGTH, true);
//	bank.AddText("LastMatrix(3)", s_PartLastMatrixString3, RIGHT_CLICK_STRING_LENGTH, true);

	bank.PushGroup("Damping",false);
	const char* typeName[]={"Linear_Con","Linear_Vel","Linear_Vel2","Angular_Con","Angular_Vel","Angular_Vel2"};
	for(int index=0;index < phArchetypeDamp::NUM_DAMP_TYPES;index++)
	{
		bank.AddTitle(typeName[index]);
		bank.AddSlider("damping",&s_DampingConstants[index],0.0f,100.0f,0.1f,datCallback(CFA(phMouseInput::DampingChanged)));
	}
	bank.PopGroup();

	sm_ExplosionType.m_InitialRadiusSpeed = 300.0f;
	sm_ExplosionType.m_DecayFactor = -40.0f;
	sm_ExplosionType.m_ForceFactor = 10.0f;
}
#endif

void phMouseInput::DebugColliderUpdateCb()
{
	sm_DebugColliderUpdate = true;
}

#if __DEV
void phMouseInput::DumpManifoldsWithSelectedInst()
{
	if(m_SelectedInst != BAD_INDEX && PHLEVEL->IsInLevel(m_SelectedInst))
	{
		PHSIM->DumpManifoldsWithInstance(PHLEVEL->GetInstance(m_SelectedInst));
	}
}
void phMouseInput::DumpInvalidStateWithSelectedInst()
{
	if(m_SelectedInst != BAD_INDEX && PHLEVEL->IsInLevel(m_SelectedInst))
	{
		PHLEVEL->GetInstance(m_SelectedInst)->InvalidStateDump();
	}
}
#endif // __DEV

void phMouseInput::NextComponent()
{
	if (m_SelectedInst != BAD_INDEX && PHLEVEL->IsInLevel(m_SelectedInst))
	{
		phInst* selectedInst = PHLEVEL->GetInstance(m_SelectedInst);
		phBound* bound = selectedInst->GetArchetype()->GetBound();
		if (bound->GetType() == phBound::COMPOSITE)
		{
			++m_SelectedComponent;

			phBoundComposite* boundComposite = static_cast<phBoundComposite*>(bound);
			if (m_SelectedComponent >= boundComposite->GetMaxNumBounds())
			{
				m_SelectedComponent = 0;
			}

			m_SelectedPart = 0;
		}
	}
}

void phMouseInput::NextComponentCb()
{
	sm_ActiveInstance->NextComponent();
}

void phMouseInput::PreviousComponent()
{
	if (m_SelectedInst != BAD_INDEX && PHLEVEL->IsInLevel(m_SelectedInst))
	{
		phInst* selectedInst = PHLEVEL->GetInstance(m_SelectedInst);
		phBound* bound = selectedInst->GetArchetype()->GetBound();
		if (bound->GetType() == phBound::COMPOSITE)
		{
			--m_SelectedComponent;

			if (m_SelectedComponent < 0)
			{
				phBoundComposite* boundComposite = static_cast<phBoundComposite*>(bound);
				m_SelectedComponent = boundComposite->GetMaxNumBounds() - 1;
			}

			m_SelectedPart = 0;
		}
	}
}

void phMouseInput::PreviousComponentCb()
{
	sm_ActiveInstance->PreviousComponent();
}

void phMouseInput::NextPart()
{
	if (m_SelectedInst != BAD_INDEX && PHLEVEL->IsInLevel(m_SelectedInst))
	{
		phInst* selectedInst = PHLEVEL->GetInstance(m_SelectedInst);
		phBound* bound = selectedInst->GetArchetype()->GetBound();
		phBound* boundToCount = NULL;
		if (bound->GetType() == phBound::COMPOSITE)
		{
			phBoundComposite* boundComposite = static_cast<phBoundComposite*>(bound);
			phBound* compBound = boundComposite->GetBound(m_SelectedComponent);

			if (compBound->GetType() == phBound::GEOMETRY)
			{
				boundToCount = compBound;
			}
			else if (compBound->GetType() == phBound::BVH USE_GRIDS_ONLY(|| compBound->GetType() == phBound::GRID))
			{
#if USE_GRIDS
				if (compBound->GetType() == phBound::GRID)
				{
					compBound = static_cast<phBoundGrid*>(compBound)->GetOctree(m_SelectedComponent);
				}
#endif
				boundToCount = compBound;
			}
		}
		else if(bound->GetType() == phBound::BVH)
		{
			boundToCount = bound;
		}

		if(boundToCount != NULL)
		{
			phBoundPolyhedron* boundPoly = static_cast<phBoundPolyhedron*>(boundToCount);
			int maxCount = boundPoly->GetNumPolygons();

			++m_SelectedPart;
			if (m_SelectedPart >= maxCount)
			{
				m_SelectedPart = 0;
			}
		}
	}
}

void phMouseInput::NextPartCb()
{
	sm_ActiveInstance->NextPart();
}

void phMouseInput::PreviousPart()
{
	if (m_SelectedInst != BAD_INDEX && PHLEVEL->IsInLevel(m_SelectedInst))
	{
		phInst* selectedInst = PHLEVEL->GetInstance(m_SelectedInst);
		phBound* bound = selectedInst->GetArchetype()->GetBound();
		phBound* boundToCount = NULL;
		if (bound->GetType() == phBound::COMPOSITE)
		{
			phBoundComposite* boundComposite = static_cast<phBoundComposite*>(bound);
			phBound* compBound = boundComposite->GetBound(m_SelectedComponent);

			if (compBound->GetType() == phBound::GEOMETRY)
			{
				boundToCount = compBound;
			}
			else if (compBound->GetType() == phBound::BVH USE_GRIDS_ONLY(|| compBound->GetType() == phBound::GRID))
			{
#if USE_GRIDS
				if (compBound->GetType() == phBound::GRID)
				{
					compBound = static_cast<phBoundGrid*>(compBound)->GetOctree(m_SelectedComponent);
				}
#endif
				boundToCount = compBound;
			}
		}
		else if(bound->GetType() == phBound::BVH)
		{
			boundToCount = bound;
		}

		if(boundToCount != NULL)
		{
			phBoundPolyhedron* boundPoly = static_cast<phBoundPolyhedron*>(boundToCount);
			int maxCount = boundPoly->GetNumPolygons();

			--m_SelectedPart;
			if (m_SelectedPart < 0)
			{
				m_SelectedPart = maxCount - 1;
			}
		}
	}
}

void phMouseInput::PreviousPartCb()
{
	sm_ActiveInstance->PreviousPart();
}

void phMouseInput::LeftClickLevelIndexChangeCb()
{
	sm_ActiveInstance->m_CompareComponent = 0;
	sm_ActiveInstance->m_ComparePart = 0;
}

void phMouseInput::RightClickLevelIndexChangeCb()
{
	sm_ActiveInstance->m_SelectedComponent = 0;
	sm_ActiveInstance->m_SelectedPart = 0;
}

} // namespace rage
