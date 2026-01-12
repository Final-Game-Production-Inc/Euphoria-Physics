//
// data/callback.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef DATA_CALLBACK_H
#define DATA_CALLBACK_H

#if __WIN32
#pragma warning(disable: 4514)
#endif

#include "data/base.h"

namespace rage {

typedef void *CallbackData;

typedef void (*Static0)();
typedef void (*Static1)(CallbackData);
typedef void (*Static2)(CallbackData,CallbackData);
typedef void (datBase::*Member0)();
typedef void (datBase::*Member1)(CallbackData);
typedef void (datBase::*Member2)(CallbackData,CallbackData);

/*
PURPOSE
This union defines where is the callback function from, a static function or
a member function of a class.
*/
union CallbackAddress {
	Member0 member0;
	Member1 member1;
	Member2 member2;
	Static0 static0;
	Static1 static1;
	Static2 static2;
};

#define CFA(x)  ((::rage::Static0)(x))
#define CFA1(x) ((::rage::Static1)(x))
#define CFA2(x) ((::rage::Static2)(x))

// I think the & is superfluous but ee-gcc demands it and MSVC++ doesn't care, so there it is...
#define MFA(x)  ((::rage::Member0)(&x))
#define MFA1(x) ((::rage::Member1)(&x))
#define MFA2(x) ((::rage::Member2)(&x))

/* If you are using these macros for widget callbacks, here's how they will get called:
datCallback(CFA(fn))                     ->       fn()
datCallback(CFA1(fn), 0, false)          ->       fn(bkWidget*)
datCallback(CFA1(fn), data, true)        ->       fn(data)
datCallback(CFA2(fn), data, true)        ->       fn(data, bkWidget*)

datCallback(MFA(fn), obj)                ->       obj->fn()
datCallback(MFA1(fn), obj, 0, false)     ->       obj->fn(bkWidget*)
datCallback(MFA1(fn), obj, data, true)   ->       obj->fn(data)
datCallback(MFA2(fn), obj, data, true)   ->       obj->fn(data, bkWidget*)
*/

/*
PURPOSE
This class a callback. A callback is defined by user and wrapped up with
datCallback. Most usage is in UI. When create a button, need a callback to
response the click-button event. The member callbacks will only work if the 
target's class was derived from Base.
<FLAG Component>
*/
class pgBase;

class datCallback {
	friend class datCallbackList;
	datBase *thisptr;
	CallbackAddress address;
#if __WIN32
	char m_Pad[4]; // padding cause we need datCallback to be the same size on all platforms (for inclusion in resources)
#endif
	CallbackData client;
	unsigned int type;
public:
	enum {NULL_CALLBACK=0,NO_PARAMETERS=1,ONE_PARAMETER=2,TWO_PARAMETERS=3,USE_CALL_PARAM=4};
	/* Constructors */
	datCallback();
	datCallback(Member0,datBase*);
	datCallback(Member1,datBase*,CallbackData=0,bool useCallParam=false);
	datCallback(Member2,datBase*,CallbackData=0);
//#if !__BANK
	datCallback(Member0,pgBase*);
	datCallback(Member1,pgBase*,CallbackData=0,bool useCallParam=false);
	datCallback(Member2,pgBase*,CallbackData=0);
//#endif
	datCallback(Static0);
	datCallback(Static1,CallbackData=0,bool useCallParam=false);
	datCallback(Static2,CallbackData=0);

	int GetType() const				{return type;}
	bool SetBase(datBase& base);
	void SetClient(CallbackData cb)	{client=cb;}
	void Call(CallbackData = 0) const;
	void *GetClient() const			{return client;}

	bool operator==(const datCallback& other) const
	{
		return (thisptr == other.thisptr) && (address.member0 == other.address.member0) && (client == other.client) && (type == other.type);
	}

	bool operator!=(const datCallback& other) const
	{
		return (thisptr != other.thisptr) || (address.member0 != other.address.member0) || (client != other.client) || (type != other.type);
	}
};

#if __CONSOLE || __RESOURCECOMPILER
CompileTimeAssertSize(datCallback, 20, 40);
#endif

extern datCallback NullCallback;	// for default arg's
#define NullCB NullCallback

}	// namespace rage

#endif
