// 
// system/noncopyable.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef SYSTEM_NONCOPYABLE_H
#define SYSTEM_NONCOPYABLE_H

namespace rage
{

// PURPOSE: This a macro for declaring a class as noncopyable.
// This is useful for subclass or instances where you don't want to derive
// from sysNonCopyable, or if you can't compile a class that complains
// that the copy constructor can't be defined due to the class layout.
#define NON_COPYABLE(__ClassName)						\
private:												\
	__ClassName( const __ClassName& );					\
	const __ClassName& operator=( const __ClassName& );

namespace sysNonCopyable_  // protection from unintended ADL
{


// PURPOSE:
// This Class makes copy constructor and assignment operator private. When inherited
// privately this makes it impossible to copy the derived class.
class sysNonCopyable
{
protected:
	sysNonCopyable() {}
	~sysNonCopyable() {}
private:  // emphasize the following members are private
	NON_COPYABLE(sysNonCopyable);
};

} // namespace noncopyable_

typedef sysNonCopyable_::sysNonCopyable sysNonCopyable;


} // namespace rage

#endif  // SYSTEM_NONCOPYABLE_H
