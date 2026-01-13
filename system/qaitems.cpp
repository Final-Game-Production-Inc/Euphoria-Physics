//
// qa/rageqa.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//
#include "file/asset.h"
#include "system/main.h"
#include "system/param.h"
#include "system/timemgr.h"
#include "qa/item.h"
#include "qa/result.h"
#include "qa/resultmanager.h"

#if __QA

#ifdef QA_ITEMS_DEPEND_ON_SAMPLE_GRCORE
#include "sample_grcore/sample_grcore.h"
#endif

using namespace rage;

#ifdef QA_ITEMS_DEPEND_ON_SAMPLE_GRCORE
class qaSampleManager : public ragesamples::grcSampleManager
{
public:
	qaSampleManager();
	virtual ~qaSampleManager();

	void SetItem(rage::qaItem* item);

	virtual void DrawClient();

private:
	rage::qaItem* m_Item;
};

qaSampleManager::qaSampleManager()
{
	Init();
}

qaSampleManager::~qaSampleManager()
{
	Shutdown();
}

void qaSampleManager::SetItem(rage::qaItem* item)
{
	m_Item = item;
}

void qaSampleManager::DrawClient()
{
	m_Item->Draw();
}
#endif // QA_ITEMS_DEPEND_ON_SAMPLE_GRCORE

// We need to redefine all the stuff defined in qa/qa.h, so undefine it here
#ifdef QA_ITEM_FAMILY
#undef QA_ITEM_FAMILY
#endif // QA_ITEM_FAMILY

#ifdef QA_ITEM
#undef QA_ITEM
#endif // QA_ITEM

#ifdef QA_ITEM_DRAW
#undef QA_ITEM_DRAW
#endif // QA_ITEM_DRAW

#define QA_ITEM_FAMILY(NAME, PARAMS, PARAMNAMES) \
class NAME; \
rage::qaItem* Create##NAME PARAMS;

#ifdef QA_ITEMS_DEPEND_ON_SAMPLE_GRCORE 
#define QA_ITEM(NAME, PARAMS, REPORTTYPE) \
if (!testName || stricmp(testName, #NAME) == 0) \
{ \
    QALog( "\n---\n%s%s %s\n---\n", #NAME, #PARAMS, #REPORTTYPE );\
	currentQaResult.Reset(); \
	currentQaResult.SetReportType(REPORTTYPE); \
	currentQaResult.SetName(#NAME, #PARAMS); \
	{ \
		rage::sysTimer timer; \
		rage::qaItem* item = Create##NAME PARAMS; \
	\
		if (sampleManager) \
		{ \
			sampleManager->SetItem(item); \
		} \
		currentQaResult.SetInitTime(timer.GetTime()); \
	\
		while (currentQaResult.GetCondition() == rage::qaResult::INCOMPLETE) \
		{ \
			timer.Reset();\
			item->Update(currentQaResult); \
			currentQaResult.TestTime(timer.GetTime());\
	\
			if (sampleManager) \
			{ \
				sampleManager->Update(); \
				sampleManager->Draw(); \
			} \
		} \
	\
		timer.Reset(); \
		item->Shutdown(); \
		currentQaResult.SetShutdownTime(timer.GetTime()); \
	\
		currentQaResult.Display(); \
		currentQaResult.Report(resultManager); \
		delete item; \
	} \
	delete sampleManager; \
	sampleManager = NULL; \
}
#else // QA_ITEMS_DEPEND_ON_SAMPLE_GRCORE
#define QA_ITEM(NAME, PARAMS, REPORTTYPE) \
if (!testName || stricmp(testName, #NAME) == 0) \
{ \
    QALog( "\n---\n%s%s %s\n---\n", #NAME, #PARAMS, #REPORTTYPE );\
	currentQaResult.Reset(); \
	currentQaResult.SetReportType(REPORTTYPE); \
	currentQaResult.SetName(#NAME, #PARAMS); \
	{ \
		rage::sysTimer timer; \
		rage::qaItem* item = Create##NAME PARAMS; \
	\
		currentQaResult.SetInitTime(timer.GetTime()); \
	\
		while (currentQaResult.GetCondition() == rage::qaResult::INCOMPLETE) \
		{ \
			timer.Reset();\
			item->Update(currentQaResult); \
			currentQaResult.TestTime(timer.GetTime());\
		} \
	\
		timer.Reset(); \
		item->Shutdown(); \
		currentQaResult.SetShutdownTime(timer.GetTime()); \
	\
		currentQaResult.Display(); \
		currentQaResult.Report(resultManager); \
		delete item; \
	} \
}
#endif // QA_ITEMS_DEPEND_ON_SAMPLE_GRCORE

PARAM(qadraw, "[rageqa] Draw drawable qa items");

#ifdef QA_ITEMS_DEPEND_ON_SAMPLE_GRCORE
#define QA_ITEM_DRAW(NAME, PARAMS, REPORTTYPE) \
if (PARAM_qadraw.Get()) \
{ \
	sampleManager = rage_new qaSampleManager; \
} \
;QA_ITEM(NAME, PARAMS, REPORTTYPE) // Notice the semicolon at the beginning of the line, that keeps grep from finding this line
#else
#define QA_ITEM_DRAW(NAME, PARAMS, REPORTTYPE)
#endif

PARAM(runname, "[rageqa] The name to apply to the current run in the log file (defaults to the RAGE release number).");
PARAM(testname, "[rageqa] Only run the test specified in this parameter.");

int Main()
{
	rage::qaResult currentQaResult;
#ifdef QA_ITEMS_DEPEND_ON_SAMPLE_GRCORE
	qaSampleManager* sampleManager = NULL;
#endif
	rage::qaResultManager resultManager;
	const char* resultsFilename = "..\\..\\..\\qa\\qaresults";
	resultManager.Load(resultsFilename);
	resultManager.SetRunName(RAGE_RELEASE_STRING);

	const char* runName;

	if (PARAM_runname.Get(runName))
	{
		resultManager.SetRunName(runName);
	}

	const char* testName = NULL;
	PARAM_testname.Get(testName);
	
	#include "qaitems.h"
	ASSET.SetPath("");
	resultManager.Save(resultsFilename);
	return 0;
}

#else // __QA

int Main()
{
	return 0;
}

#endif // __QA
