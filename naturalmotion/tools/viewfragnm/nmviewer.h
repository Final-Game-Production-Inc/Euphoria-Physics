// 
// nmviewer/nmviewer.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef NMVIEWER_NMVIEWER_H
#define NMVIEWER_NMVIEWER_H

#include "atl/bitset.h"
#include "grcore/texturereference.h"
#include "sample_simpleworld/sample_simpleworld.h"

#define EXTRA_INSTS 1

namespace rage
{
    class bkBank;
    class crAnimation;
    class fragInst;
    class fragInstNM;
    class grcTexture;
    class NMBehaviorInst;
}

namespace ragesamples
{

class NMViewer : public rmcSampleSimpleWorld
{
public:
    NMViewer();
    ~NMViewer();

    void InitClient();
    void ShutdownClient();
    void InitCamera();

#if __BANK
    void AddWidgetsClient();
#endif

    void Update();
    void DrawHelpClient();

    void PreSimUpdate(float deltaTime);
    void PostSimUpdate(float deltaTime);

protected:
    virtual const char* GetSampleName() const
    {
        return "viewfragnm";
    }

private:
#if __BANK
    void BankSelectAnimation();
    void BankPlayAnimation();
    void LoadAnimation();
    void TriggerBehavior();
    void AddCurrentBehaviorWidgets();

    void BankSaveBehaviorInstance();
    void BankLoadBehaviorInstance();

    void BankSaveBehaviorInstances();
    void BankLoadBehaviorInstances();

    void AddBehaviorListComboWidget();
#endif // __BANK

    static void PartsBrokeOff(fragInst* oldInst,
        atFixedBitSet<phInstBreakable::MAX_NUM_BREAKABLE_COMPONENTS>& brokenParts,
        fragInst* newInst);

	bool m_UseRageActivePose;
    bool m_UseNMActivePose;
    fragType*					m_Type;
    pgDictionary<grcTexture>*	m_TextureDictionary;
    fragInstNM*					m_Inst;
	fragInstNM*					m_ActivePoseInst;
#if EXTRA_INSTS > 0
    fragInstNM*					m_Insts[EXTRA_INSTS];
#endif // EXTRA_INSTS > 0

    // Animation related
    static const int			MAX_NUM_ANIMS = 256;
    crAnimation*				m_LoadedAnimation;
    crAnimation*				m_Animation;
    crFrame*				m_AnimFrame;
    float						m_AnimPhase;
    float						m_FrameRate;
	float						m_ActivePoseStiffness;
	phConstraintHandle			m_Pin;

#if __BANK
    bkBank*						m_AnimBank;
#endif

    const char*					m_AnimNames[MAX_NUM_ANIMS];
    int							m_AnimIndex;
    bool						m_LoopWidget;

#if __BANK
    bkBank*						m_BehaviorBank;
    bkCombo*                    m_BehaviorListComboBox;
    int                         m_currentNMBehavior;
    int                         m_previousNMBehavior;
    atArray<NMBehaviorInst *>   m_NMBehaviorInstances;
#endif
};

} // namespace ragesamples

#endif // NMVIEWER_NMVIEWER_H
