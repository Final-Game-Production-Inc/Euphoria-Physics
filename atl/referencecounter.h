//
// atl/referencecounter.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef ATL_REFERENCE_COUNTER_H
#define ATL_REFERENCE_COUNTER_H

#if __DEV
#define __ATL_SMARTPOINTER_LOGS     0
#else
#define __ATL_SMARTPOINTER_LOGS     0
#endif

#if __ATL_SMARTPOINTER_LOGS
    #include "stl/list.h"                                                   //AngelStudios internal wrapper ...
#endif // __ATL_SMARTPOINTER_LOGS


#include "data/base.h"
#include "data/struct.h"

namespace rage {

/*
PURPOSE
    Simple baseclass implementing reference counting for use with smart
    pointers.

NOTES
    * You must not delete a reference counted object.

    * Can be used when the class itself is declared const.

    * You must not allocate a reference counted object on the stack (at least
      not if its referencecounting functionality is used). So either use a
      class derived from this class exclusively with smart pointers or use it
      like any other class and make neither usage of smart pointers nor
      reference counting. 
      Not all derived classes may work properly without reference counting
      (a class could for example depend on AddRef and Release calls by
      overwriting them).
<FLAG Component>
*/
class atReferenceCounter : public datBase
{
public:
    atReferenceCounter();
    
    atReferenceCounter(datResource& rsc);

protected:
    virtual ~atReferenceCounter();                                          //protected: make sure noone is manually deleting a reference counted object.

public:
    /*
    PURPOSE
        A method of this type can optionally be used as callbackhook which is
        called whenever the objects reference counter has changed.

    INPUT
        references          - the current count of references to this object
        userArg             - passed through as user argument.

    NOTES
        For internal use and debug builds only.

    SEE ALSO
        AddCallbackHook, RemoveCallbackHook
    */
#if __ATL_SMARTPOINTER_LOGS
    typedef void (*ReferenceCounterCallbackPtr)(long references, void *userArg);
#endif // __ATL_SMARTPOINTER_LOGS


    /*
    PURPOSE
        Increases the reference counter by 1

    NOTES
        Override this method to control the reference counting.

    SEE ALSO
        Release
    */
    inline virtual void AddRef() const
    {
        references++;

#if __ATL_SMARTPOINTER_LOGS
        CallCallbackhooks();
#endif // __ATL_SMARTPOINTER_LOGS
    };

    /*
    PURPOSE
        Decreases the reference counter by 1 and deletes the object if it´s 0.

    NOTES
        Override this method to control the reference counting. In this case
        call this method as your last statement because afterwards the object
        may not exist anymore.

    SEE ALSO
        AddRef
    */
    inline virtual int Release() const
    {
        references--;

#if __ATL_SMARTPOINTER_LOGS
        CallCallbackhooks();
#endif // __ATL_SMARTPOINTER_LOGS

        if(references == 0)
        {
            delete this;
			return 0;
        }
		else
			return references;
    };


#if __ATL_SMARTPOINTER_LOGS
    void AddCallbackHook(ReferenceCounterCallbackPtr callbackPtr, void *userArg);
    void RemoveCallbackHook(ReferenceCounterCallbackPtr callbackPtr);
    static int GetNumberOfInstances();
#endif // __ATL_SMARTPOINTER_LOGS

#if __DECLARESTRUCT
	virtual void DeclareStruct(datTypeStruct&);
#endif // !__FINAL

protected:
    mutable int        references;                                         //Mutable allows referencecounting even if the class itself is declared const.


#if __ATL_SMARTPOINTER_LOGS

private:
    struct CallbackHelper
    {
        ReferenceCounterCallbackPtr     callbackPtr;
        void                            *userArg;
    };

    std::list<CallbackHelper>           callbackList;                       //Stores the callback hooks ...

    static int numberOfInstances;                                           //Holds the total number of all reference counted allocated objects of this type.

    void CallCallbackhooks() const;

#endif // __ATL_SMARTPOINTER_LOGS
};

}	// namespace rage


#endif //ATL_REFERENCE_COUNTER_H
