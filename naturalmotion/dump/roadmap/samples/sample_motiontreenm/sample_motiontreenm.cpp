// 
// /sample_motiontreenm.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#include "sample_motiontree/sample_motiontree.h"

#include "cranimation/animation.h"
#include "crmotiontree/motiontree.h"
#include "crmotiontree/observer.h"
#include "crmotiontree/requestragdoll.h"
#include "crmotiontree/requestanimation.h"
#include "crmotiontree/requestblend.h"
#include "fragmentnm/instance.h"
#include "fragmentnm/manager.h"
#include "system/main.h"
#include "system/param.h"

#define DEFAULT_FILE "$/fragments/cowboynm/entity.type"
XPARAM(file);

#define DEFAULT_ANIM "Gent_nor_hnd_for_wlk"
PARAM(anim,"[sample_motiontreenm] Animation file to load (default is \"" DEFAULT_ANIM "\")");

#define DEFAULT_ART_FILE RAGE_ASSET_ROOT "/naturalmotion/Cowboy"
PARAM(artfile,"[sample_motiontreenm] NaturalMotion ART file(default is \"" DEFAULT_ART_FILE "\")");

namespace ragesamples 
{

using namespace rage;

class crMotionTreeSampleNm : public ragesamples::crMotionTreeSampleManager
{
public:
	crMotionTreeSampleNm() : ragesamples::crMotionTreeSampleManager()
	{
	}

protected:
	void InitClient()
	{
		crMotionTreeSampleManager::InitClient();

		// load animation
		const char* animName = DEFAULT_ANIM;
		PARAM_anim.Get(animName);
		crAnimation* anim = crAnimation::AllocateAndLoad(animName);
		if(!anim)
		{
			Quitf("Unable to load animation '%s%s", ASSET.GetPath(), animName);
		}

		// create an animation node
		crmtRequestAnimation reqAnim0(anim, 0.f, 0.1f);
		reqAnim0.SetLooping(true,true);
		GetMotionTree().Request(reqAnim0);

		anim->Release();

		crmtRequestRagdoll reqRagdoll(GetFragInst());
		GetMotionTree().Request(reqRagdoll);
	}

	virtual void InitFragment()
	{
		// load nm art file
		const char* artFile = DEFAULT_ART_FILE;
		PARAM_artfile.Get(artFile);
		fragNMAssetManager::Create();
		FRAGNMASSETMGR->Load(artFile, 3);

		crMotionTreeSampleManager::InitFragment();
	}

	virtual fragInst* CreateFragInst()
	{
		return new fragInstNM(&GetFragType(), M34_IDENTITY);
	}

	virtual fragType* CreateFragType()
	{
		const char* file = DEFAULT_FILE;
		PARAM_file.Get(file);

		return fragType::Load(file);
	}

private:
	crmtObserver m_Observer;
};

} // namespace ragesamples

// main application
int Main()
{
	ragesamples::crMotionTreeSampleNm sampleMotionTreeNm;
	sampleMotionTreeNm.Init("sample_cranimation/newskel");

	sampleMotionTreeNm.UpdateLoop();

	sampleMotionTreeNm.Shutdown();

	return 0;
}
