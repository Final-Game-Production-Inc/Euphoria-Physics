// 
// atl/any.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 
// See http://www.boost.org/libs/any for Documentation.

#ifndef ATL_ANY_H
#define ATL_ANY_H

// what:  variant type boost::any
// who:   contributed by Kevlin Henney,
//        with features contributed and bugs found by
//        Ed Brey, Mark Rodgers, Peter Dimov, and James Curran
// when:  July 2001
// where: tested with BCC 5.5, MSVC 6.0, and g++ 2.95

#include "data/struct.h"
#include "system/typeinfo.h"

#define ATL_ANY_USE_MARKERS 0

#if ATL_ANY_USE_MARKERS
#define DATA_MARKER_INTENTIONAL_HEADER_INCLUDE 1
#include "data/marker.h"
#define ATL_ANY_FUNC() RAGE_FUNC()
#else
#define ATL_ANY_FUNC()
#endif

#if __SPU
namespace rage 
{
	typedef ::std::type_info type_info;
};
#endif

#if __WIN32
#pragma warning(push)
#pragma warning(disable: 4324)
#endif

namespace rage {

class datResource;

class atAny
{
public: // structors

    atAny()
        : m_Content(0)
#if __TOOL
		, m_AlignTo(0)
#endif
    {
    }

    template<typename _T>
    atAny(const _T & value)
	{
#if !ATL_ANY_USE_MARKERS
		ATL_ANY_FUNC();
#endif

#if __TOOL
		m_AlignTo = __alignof(_T);
		if( m_AlignTo > 8 ) // POD natural alignment
		{
			// Allocate.
			m_Content = (Holder<_T>*)_aligned_malloc( sizeof(Holder<_T>), m_AlignTo );

			// Placement new.
			::new (m_Content) Holder<_T>(value);
		}
		else
#endif
		{
			m_Content = rage_new Holder<_T>(value);
		}
	}

    atAny(const atAny & other)
        : m_Content(other.m_Content ? other.m_Content->clone() : 0)
    {
    }

	atAny(datResource&/*rsc*/)
//		: m_Content(rsc)
	{
		Assert(m_Content == NULL);	// atAny objects cannot be resourced with content.
		m_Content = NULL;
	}

#if __DECLARESTRUCT
	void DeclareStruct(datTypeStruct &s)
	{
		STRUCT_BEGIN(atAny);

		STRUCT_IGNORE(m_Content);	// Has to be NULL - we can't fix those up properly.

		STRUCT_END();
	}
#endif // __DECLARESTRUCT

	IMPLEMENT_PLACE_INLINE(atAny);

    ~atAny()
    {
#if __TOOL
		if( m_AlignTo > 8 ) // POD natural alignment
		{
			if( m_Content )
			{
				// Destruct.
				m_Content->~PlaceHolder();

				// Delete.
				_aligned_free( m_Content );
			}
		}
		else
#endif
		{
			delete m_Content;
		}
    }

public: // modifiers

    atAny & swap(atAny & rhs)
    {
		datRef<PlaceHolder> tmp = m_Content;
        m_Content = rhs.m_Content;
		rhs.m_Content = tmp;
        return *this;
    }

    template<typename _T>
    atAny & operator=(const _T & rhs)
    {
		if (m_Content)
		{
#if !__SPU
			Assertf(m_Content->GetType() == typeid(_T),
				       "Changing type by assignment from %s to %s not allowed, instead use myAny = atAny(value) to change GetType",
						m_Content->GetType().name(), typeid(_T).name());
#endif

			static_cast<atAny::Holder<_T> *>((PlaceHolder*)m_Content)->m_Held = rhs;
		}
		else
		{
			atAny(rhs).swap(*this);
		}
        return *this;
    }

    atAny & operator=(const atAny & rhs)
    {
        atAny(rhs).swap(*this);
        return *this;
    }

public: // queries

    bool IsEmpty() const
    {
        return !m_Content;
    }

    const type_info & GetType() const
    {
#if __SPU
		return m_Content->GetType();
#else
        return m_Content ? m_Content->GetType() : typeid(void);
#endif
    }

private: // types

    class PlaceHolder
    {
    public: // structors

        virtual ~PlaceHolder()
        {
        }

    public: // queries

        virtual const type_info & GetType() const = 0;

        virtual PlaceHolder * clone() const = 0;

    };

    template<typename _T>
    class Holder : public PlaceHolder
    {
    public: // structors

        Holder(const _T & value)
            : m_Held(value)
        {
        }

    public: // queries

        virtual const type_info & GetType() const
        {
#if __SPU
			return NULL;
#else
            return typeid(_T);
#endif
        }

        virtual PlaceHolder * clone() const
        {

#if __TOOL
			s32 AlignTo = __alignof(_T);
			if( AlignTo > 8 ) // POD natural alignment
			{
				Holder<_T>* pTemp;

				// Allocate.
				pTemp = (Holder<_T>*)_aligned_malloc( sizeof(Holder<_T>), AlignTo );

				// Placement new.
				::new (pTemp) Holder<_T>(m_Held);

				return pTemp;
			}
			else
#endif
			{
				return rage_new Holder(m_Held);
			}
        }

    public: // representation

        _T m_Held;
    };

private: // representation

    template<typename _T>
    friend _T * atAnyCast(atAny *);

    datRef<PlaceHolder> m_Content;

#if __TOOL
	u32 m_AlignTo; // Hack for this class so that alignment will work on __TOOL builds.
#endif
};

template<typename _T>
_T * atAnyCast(atAny * operand)
{
#if !__SPU
# ifdef _CPPRTTI
	if (operand)
	{
		const type_info& operandType = operand->GetType();
		const type_info& castType = typeid(_T);
		//const char* opName = operandType.name();
		//const char* castName = castType.name();
		//Displayf("%s : %s", opName, castName);
		if (operandType == castType)
		{
			return &static_cast<atAny::Holder<_T> *>((atAny::PlaceHolder*)operand->m_Content)->m_Held;
		}
	}
# else
	return &static_cast<atAny::Holder<_T> *>((atAny::PlaceHolder*)operand->m_Content)->m_Held;
	/*NOTREACHED*/
# endif
#endif

	return NULL;
}

template<typename _T>
const _T * atAnyCast(const atAny * operand)
{
    return atAnyCast<_T>(const_cast<atAny *>(operand));
}

template<typename _T>
const _T atAnyCast(const atAny & operand)
{
	const _T * result = atAnyCast<_T>(&operand);
#if !__SPU
	Assertf(result, "Bad atAnyCast on reference, from %s to %s", operand.GetType().name(), typeid(_T).name());
#endif
	return *result;
}

template<typename _T>
_T atAnyCast(atAny & operand)
{
	_T * result = atAnyCast<_T>(&operand);
#if !__SPU
	Assertf(result, "Bad atAnyCast on reference, from %s to %s", operand.GetType().name(), typeid(_T).name());
#endif
	return *result;
}

} // namespace rage

// Based on boost::any, which came with the following license information:

// Copyright Kevlin Henney, 2000, 2001, 2002. All rights reserved.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if __WIN32
#pragma warning(pop)
#endif

#endif

