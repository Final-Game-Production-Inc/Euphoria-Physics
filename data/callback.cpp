//
// data/callback.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "callback.h"
#include "paging/base.h"

#include <string.h>

namespace rage {

datCallback NullCallback;


/*
PURPOSE
	Constructor.
PARAMS
	none.
RETURNS
	none.
NOTES
*/
datCallback::datCallback() {
	memset(this,0,sizeof(*this));
}


/*
PURPOSE
	Create a callback with the datBase member function.
PARAMS
	m0 - the callback function without parameter.
	th - user defined callback object.
RETURNS
	none.
NOTES
*/
datCallback::datCallback(Member0 m0,datBase* th) {
	address.member0 = m0;
	type = NO_PARAMETERS;
	thisptr = th;
	client = 0;
}
//#if !__BANK
datCallback::datCallback(Member0 m0,pgBase* th) {
	address.member0 = m0;
	type = NO_PARAMETERS;
	thisptr = (datBase*)th;
	client = 0;
}
//#endif

/*
PURPOSE
	Create a callback with the datBase member function.
PARAMS
	m1 - the callback function with one parameter.
	th - user defined callback object.
	useCallParam - to indicate the type of callback function.
RETURNS
	none.
NOTES
*/
datCallback::datCallback(Member1 m1,datBase* th,CallbackData cd,bool useCallParam) {
	address.member1 = m1;
	type = useCallParam ? USE_CALL_PARAM : ONE_PARAMETER;
	thisptr = th;
	client = cd;
}
//#if !__BANK
datCallback::datCallback(Member1 m1,pgBase* th,CallbackData cd,bool useCallParam) {
	address.member1 = m1;
	type = useCallParam ? USE_CALL_PARAM : ONE_PARAMETER;
	thisptr = (datBase*)th;
	client = cd;
}
//#endif


/*
PURPOSE
	Create a callback with the datBase member function.
PARAMS
	m2 - the callback function with two parameter.
	th - user defined callback object.
	cd - the callback data.
RETURNS
	none.
NOTES
*/
datCallback::datCallback(Member2 m2,datBase* th,CallbackData cd) {
	address.member2 = m2;
	type = TWO_PARAMETERS;
	thisptr = th;
	client = cd;
}
//#if !__BANK
datCallback::datCallback(Member2 m2,pgBase* th,CallbackData cd) {
	address.member2 = m2;
	type = TWO_PARAMETERS;
	thisptr = (datBase*)th;
	client = cd;
}
//#endif

/*
PURPOSE
	Create a callback with the static function.
PARAMS
	s0 - the callback function with no parameter.
RETURNS
	none.
NOTES
*/
datCallback::datCallback(Static0 s0) {
	address.static0 = s0;
	type = NO_PARAMETERS;
	thisptr = NULL;
	client = 0;
}


/*
PURPOSE
	Create a callback with the static function.
PARAMS
	s1 - the callback function with one parameter.
	cd - the callback data.
	useCallParam - use the call parameter if it is true.
RETURNS
	none.
NOTES
*/
datCallback::datCallback(Static1 s1,CallbackData cd,bool useCallParam) {
	address.static1 = s1;
	type = useCallParam ? USE_CALL_PARAM : ONE_PARAMETER;
	thisptr = NULL;
	client = cd;
}


/*
PURPOSE
	Create a callback with the static function.
PARAMS
	s2 - the callback function with two parameter.
	cd - the callback data.
RETURNS
	none.
NOTES
*/
datCallback::datCallback(Static2 s2,CallbackData cd) {
	address.static2 = s2;
	type = TWO_PARAMETERS;
	thisptr = NULL;
	client = cd;
}


/*
PURPOSE
	Set the datBase data member.
PARAMS
	base - used to set the data member.
RETURNS
	true if the base data member exists.
NOTES
*/
bool datCallback::SetBase(datBase &base) {
	if (thisptr) {
		thisptr = &base;
		return true;
	} 
	else 
		return false;
}


/*
PURPOSE
	Call a callback function based on the type.
PARAMS
	cd - callback data. It is used in the callback function if the callback 
	type is USE_CALL_PARAM.
RETURNS
	none.
NOTES
*/
void datCallback::Call(CallbackData cd) const {
	if (type != NULL_CALLBACK) {
		if (thisptr) {
			if (type == NO_PARAMETERS)
				(thisptr->*address.member0)();
			else if (type == ONE_PARAMETER)
				(thisptr->*address.member1)(client);
			else if (type == USE_CALL_PARAM)
				(thisptr->*address.member1)(cd);
			else if (type == TWO_PARAMETERS)
				(thisptr->*address.member2)(client,cd);
		}
		else {
			if (type == NO_PARAMETERS)
				address.static0();
			else if (type == ONE_PARAMETER)
				address.static1(client);
			else if (type == USE_CALL_PARAM)
				address.static1(cd);
			else if (type == TWO_PARAMETERS)
				address.static2(client,cd);
		}
	}
}


}	// namespace rage
