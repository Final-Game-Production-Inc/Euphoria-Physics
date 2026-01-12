// 
// sample_physics/mouseinput.h 
// 
// Copyright (C) 1999-2008 Rockstar Games.  All Rights Reserved. 
// 
#ifndef INC_PHEFFETCS_MOUSEINPUT
#define INC_PHEFFETCS_MOUSEINPUT

#include "atl/bitset.h"
#include "input/mapper.h"
#include "pheffects/explosiontype.h"
#include "physics/instbreakable.h"
#include "physics/constrainthandle.h"
#include "vector/vector3.h"

// If your application has special requirements for UserData it may not be able to spawn boxes properly
#define PHMOUSEINPUT_ENABLE_BOXES 1
// If your application does not use rage physics explosions then this may need to be disabled
#define PHMOUSEINPUT_ENABLE_EXPLOSIONS (!HACK_GTA4)

namespace rage {

class phDemoObject;
class phExplosionMgr;
class phInst;

struct BvhRenderOpts
{
	bool drawPrims;
	bool drawBorder;
	bool drawBoundingBox;
	bool drawDepth;
	Vec3V boundingBoxSizeOffset;
	Vec3V depthOffset;
	float normalOffset;
	float normalOffsetInc;
	int mode;
	Vec3V queryHalfSize;
	BvhRenderOpts();
};

class phMouseInput : public datBase
{
public:
	struct Spring
	{
		u32 instLevelIndex;				// The instance to which this spring is attached. This isn't const because it goes into AttachObjectToWorld.
		phConstraintHandle constraintHandle;	// The (optional) constraint that satisfies this interaction device
		u16 component;				// The index of the component in the bound of the above instance.
		int instNumComponents;		// The number of components in the bound of above instance.

		Vector3 localPoint;			// This is where the spring is attached to the instance relative to the "most local" bound in the instance.
		// This means relative to the instance for non-composite bounds, and relative to the specific bound component
		//   that was hit for composite bounds.
		// Once a spring is attached to an object, this doesn't change.
		Vector3 worldPoint;			// This is the world space version of localPoint.  It is updated every frame in UpdateSpring().
		Vector3 attachmentPoint;	// This is the world space location of the end of the spring that is not attached to an object.
	};

	phMouseInput();
	~phMouseInput();

	void SetActiveInstance() { sm_ActiveInstance = this; }

	static void EnableLeftClick(bool enabled = true) { sm_EnableLeftClick = enabled; }
	static void EnableRightClick(bool enabled = true) { sm_EnableRightClick = enabled; }

	static bool IsEnabled() { return sm_EnableLeftClick || sm_EnableRightClick; }

	void Reset();

	static void DampingChanged();

	void Update(bool paused,Vec::V3Param128 timeStep, const grcViewport* viewport = NULL);
	void UpdateSpring(Spring* spring, bool paused, Vec::V3Param128 timeStep);

	void Draw();

	const Spring& GetGrabSpring()								{ return m_GrabSpring; }

	phExplosionMgr* GetExplosionMgr ();

	void FixGrabForBreak(phInst* brokenInst, const atFixedBitSet<phInstBreakable::MAX_NUM_BREAKABLE_COMPONENTS>& brokenParts, phInst* newInst);

	typedef const char* (*BoundFlagNameFunc)(const u32 bit);

#if PHMOUSEINPUT_ENABLE_BOXES
	void SetBoxTypeIncludeFlags(u32 typeFlags, u32 includeFlags);
#endif

	static void SetBoundFlagNameFunc(BoundFlagNameFunc bfnf)	{ sm_BoundFlagNameFunc = bfnf; }

	static void SetImpulseScale(const float scale)				{ sm_ImpulseScale=scale; }
	static float GetImpulseScale()								{ return sm_ImpulseScale; }

	static const char* GetLeftClickModeString();
	static const char* GetScaleString();

	BANK_ONLY(static void AddClassWidgets(bkBank& bank);)

	enum LeftClickMode {
		SPRING,
		CONSTRAINT,
		IMPULSE,
		BULLET,
#if PHMOUSEINPUT_ENABLE_BOXES
		BOX,
#endif
#if PHMOUSEINPUT_ENABLE_EXPLOSIONS
		EXPLOSION,
#endif
		ACTIVATE,
		DEACTIVATE,

		COMPARE,

		NUM_LEFT_CLICK_MODES
	};

	static void SetLeftClickMode(LeftClickMode mode)			{ sm_LeftClickMode = mode; }

private:
	void DisconnectSprings();
	void CreateSpring(const Spring& spring);

	void InitBoxGun(int numBoxes = 16);
	void ShutdownBoxGun();
	void ResetBoxGun();
	void ThrowBox(Vec3V_In position, Vec3V_In direction);

	void FixSpringForBreak(Spring& spring, phInst* brokenInst, const atFixedBitSet<phInstBreakable::MAX_NUM_BREAKABLE_COMPONENTS>& brokenParts, phInst* newInst);

	void GetNamesFromFlags(char* name, int maxLen, u32 flags, const char* zeroFlagString);

	static void DebugColliderUpdateCb();
#if __DEV
	void DumpManifoldsWithSelectedInst();
	void DumpInvalidStateWithSelectedInst();
#endif // __DEV
	void NextComponent();
	static void NextComponentCb();
	void PreviousComponent();
	static void PreviousComponentCb();
	void NextPart();
	static void NextPartCb();
	void PreviousPart();
	static void PreviousPartCb();

	static void LeftClickLevelIndexChangeCb();
	static void RightClickLevelIndexChangeCb();

	ioMapper m_Mapper;
	ioValue	m_LeftClick;
	ioValue m_MiddleClick;
	ioValue m_RightClick;
	ioValue m_AngularImpetus;
	ioValue m_ReverseImpulse;
	ioValue m_GrabDistanceWheel;
	ioValue	m_DisconnectSprings;
	ioValue m_CameraControl;

	int m_NumBoxBullets;										// The number of box gun bullets (boxes) that can exist
	int m_CurrentBoxBullet;										// The index of the next box to be thrown
	phInst* m_BoxBulletInsts;
	phArchetypePhys* m_BoxBulletArchetype;
	phBound* m_BoxBulletBound;

	Spring m_GrabSpring;										// This is the interactive spring that the user can play with.
	float m_GrabDistance;										// In order to allow the user to control a 3d point (the spring attachment point) with a 2d input device (the mouse), we
	//   restrict the attachment point to maintain a specified distance from the camera.

	static const int MAX_NUM_SPRINGS = 32;
	Spring m_PlacedSprings[MAX_NUM_SPRINGS];
	int m_NumPlacedSprings;

	int m_SelectedInst;
	int m_SelectedComponent;
	int m_SelectedPart;
	phArchetypeDamp* m_SelectedInstArchDamp;

	int m_CompareInst;
	int m_CompareComponent;
	int m_ComparePart;

	phExplosionMgr * m_ExplosionMgr;

	static BoundFlagNameFunc sm_BoundFlagNameFunc;

	static phMouseInput* sm_ActiveInstance;
	static bool sm_EnableLeftClick;
	static bool sm_EnableRightClick;
	static LeftClickMode sm_LeftClickMode;
	static bool sm_EnableLeftStickyClickMode;
	static float sm_GrabSpringConstant;
	static float sm_GrabSpringDamping;
	static float sm_DefaultConstraintLength;
	static float sm_ImpulseScale;								// mouse parameter
	static bool sm_ImpulseScaleByMass;							// scale mouse impulse by mass
	static float sm_ImpulseBreakScale;							// break scale
	static float sm_ImpulseBulletSpeed;							// mouse impulse bullet speed
	static float sm_ImpulseBulletMass;							// mouse impulse bullet mass
	static float sm_BoxSpeed;
	static float sm_BoxMass;
	static phExplosionType sm_ExplosionType;
	static bool sm_DebugColliderUpdate;
	static bool sm_ForceGrabbedObjectsActive;
	static Vector3 s_DampingConstants[phArchetypeDamp::NUM_DAMP_TYPES];
	static bool sm_DrawSkeleton;
	static bool sm_DrawAllCompParts;
	static bool sm_DrawBvhLeaf;
	static BvhRenderOpts sm_BvhRenderOpts;
};

} // namespace rage

#endif // INC_PHEFFETCS_MOUSEINPUT
