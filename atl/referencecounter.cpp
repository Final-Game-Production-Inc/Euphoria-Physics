//
// atl/referencecounter.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "referencecounter.h"

using namespace rage;



#if __ATL_SMARTPOINTER_LOGS
    int atReferenceCounter::numberOfInstances=0;                           //holds the total number of all reference counted allocated objects of this type
#endif // __ATL_SMARTPOINTER_LOGS




atReferenceCounter::atReferenceCounter() : references(0)
{
#if __ATL_SMARTPOINTER_LOGS
    numberOfInstances++;
#endif // __ATL_SMARTPOINTER_LOGS
};

atReferenceCounter::atReferenceCounter(datResource& /*rsc*/)
{
#if __ATL_SMARTPOINTER_LOGS
    numberOfInstances++;
#endif // __ATL_SMARTPOINTER_LOGS
}

atReferenceCounter::~atReferenceCounter()
{
#if __ATL_SMARTPOINTER_LOGS
    numberOfInstances--;
#endif // __ATL_SMARTPOINTER_LOGS
};





#if __ATL_SMARTPOINTER_LOGS


/*
RETURN
    Returns the total number of all reference counted allocated objects of
    all types.

NOTES
    For debug builds only.
*/
int atReferenceCounter::GetNumberOfInstances()
{
    return numberOfInstances;
};


/*
PURPOSE
    Adds a callbackhook that is called whenever the reference counter of
    the object has changed.

    Use RemoveCallbackHook() to remove the hook. All hooks are removed
    auomatically when the object is deleted.

PARAMS
    CallbackPtr     - pointer to a function of type
                    ui2ReferenceCounterCallbackPtr
    UserArg         - is passed through to the callbackhook

NOTES
    For internal use and debug builds only.

SEE ALSO
    RemoveCallbackHook, ReferenceCounterCallbackPtr
*/
void atReferenceCounter::AddCallbackHook(ReferenceCounterCallbackPtr callbackPtr, void *userArg)
{
    CallbackHelper              CbHtmp;

    CbHtmp.callbackPtr= callbackPtr;
    CbHtmp.userArg= userArg;

    callbackList.push_front(CbHtmp);
};


/*
PURPOSE
    Removes the first callbackhook in the internal list matching
    callbackPtr.

PARAMS
    callbackPtr     - specifies the hook to remove.

NOTES
    For internal use and debug builds only.

SEE ALSO
    AddCallbackHook, ReferenceCounterCallbackPtr
*/
void atReferenceCounter::RemoveCallbackHook(ReferenceCounterCallbackPtr callbackPtr)
{
    std::list<CallbackHelper>::iterator             Iterator;

    Iterator= callbackList.begin();

    while(Iterator != callbackList.end())
    {
        if((*Iterator).callbackPtr == callbackPtr)
        {
            callbackList.erase(Iterator);

            break;
        }

        Iterator++;
    }
};


/*
PURPOSE
    Calls all registered callbackhooks.

NOTES
    For internal use and debug builds only.
*/
void atReferenceCounter::CallCallbackhooks() const
{
    std::list<CallbackHelper>::const_iterator             Iterator;

    Iterator= callbackList.begin();

    while(Iterator != callbackList.end())
    {
        (*Iterator).callbackPtr(references, (*Iterator).userArg);

        Iterator++;
    }
}

#endif // __ATL_SMARTPOINTER_LOGS


#if __DECLARESTRUCT
void atReferenceCounter::DeclareStruct(datTypeStruct& s)
{
	STRUCT_BEGIN(atReferenceCounter);
	STRUCT_FIELD(references);
	STRUCT_END();
}
#endif // !__FINAL
