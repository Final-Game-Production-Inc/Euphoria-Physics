//
// atl/slistsimple.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef ATL_SLISTSIMPLE_H
#define ATL_SLISTSIMPLE_H

#include "data/struct.h"

#if __WIN32
#pragma warning(push)
#pragma warning(disable: 4201)
#pragma warning(disable: 4324)
#endif

// You can use this as an easy way to initialize the link list template...
#define SLIST_SIMPLE_INIT(CLASS_NAME, LINK_NAME) CLASS_NAME, &CLASS_NAME::LINK_NAME

//PURPOSE
//  Enable __DEBUG_SLLIST to validate item insertion and removal.
#ifndef __DEBUG_SLLIST
#define __DEBUG_SLLIST  0
#endif  //__DEBUG_SLLIST

#if 0
#define SLIST_SIMPLE_ASSERT(X) FastAssert(X)
#else
#define SLIST_SIMPLE_ASSERT(X)
#endif

namespace rage {

/*
PURPOSE: 
	The class that holds the the forwards and backwards link for a class.
*/
template <class _Type> struct SLinkSimple
{
public:
	datRef<_Type>	pNext;
	
	//this is really largely for debugging, but sometimes darn useful...
	SLinkSimple()
    {
        this->pNext = 0;
#if __DEBUG_SLLIST
        pOwner = 0;
#endif  //__DEBUG_SLLIST
    }

	SLinkSimple(datResource&)	{}
	
	IMPLEMENT_PLACE_INLINE(SLinkSimple);

#if __DECLARESTRUCT
	void DeclareStruct(datTypeStruct& s)
	{
		STRUCT_BEGIN(SLinkSimple);
		STRUCT_FIELD(pNext);
		STRUCT_END();
	}
#endif

#if __DEBUG_SLLIST
    //The list in which we're contained - used for debugging
    void* pOwner;
#endif  //__DEBUG_SLLIST
};

/*
PURPOSE: 
	Yet another single linked list template.  This one saves some space by only having 
	the next pointer embedded in the link part.  The tradeoff is that the list class 
	needs to know the offset for each link.  As such, only homogenous types may be 
	linked up (hence the name 'simple').  Otherwise, this class sould be fairly simple 
	to use - you embed as many SLinkSimple classes as you wish in a class, and you 
	initialize a list with the offset to the particular link structure you want to use, 
	using the SLIST_SIMPLE_INIT, if you want.
*/
template <class _Type, SLinkSimple<_Type> _Type::* _Member> class SLListSimple
{
private:
	_Type		*pHeadLink;
	_Type		*pTailLink;

    int m_Count;
	
private:
	inline void Remove(_Type* pObj, _Type* pPrev)
	{
		if (pPrev)
		{
			//not first one...
			this->GetLinkFromObj(pPrev)->pNext = this->GetLinkFromObj(pObj)->pNext;
			if (this->GetLinkFromObj(pObj)->pNext == 0)
			{
				//last one...
				this->pTailLink = pPrev;
			}
		}
		else
		{
			//first one...
			this->pHeadLink = this->GetLinkFromObj(pObj)->pNext;
			if (this->pTailLink == pObj)
			{
				//only one...
				this->pTailLink = 0;				
			}
		}

		this->GetLinkFromObj(pObj)->pNext = 0; //mostly for debug, but useful!

		--m_Count;
		SLIST_SIMPLE_ASSERT( m_Count >= 0 );

#if __DEBUG_SLLIST
		this->GetLinkFromObj(pObj)->pOwner = 0;
#endif  //__DEBUG_SLLIST
	}

public:

    class Iterator
    {
    public:

        Iterator()
            : m_Item( 0 )
        {
        }

        explicit Iterator( _Type* item )
            : m_Item( item )
        {
        }

		void SetTo( _Type* item)
		{
			m_Item = item;
		}

        bool IsValid() const
        {
            return ( 0 != m_Item );
        }

        void Next()
        {
            m_Item ? m_Item = ( m_Item->*_Member ).pNext : 0;
        }

        _Type* Item()
        {
            return m_Item;
        }

        const _Type* Item() const
        {
            return m_Item;
        }

    private:

        _Type* m_Item;
    };

    class ConstIterator
    {
    public:

        ConstIterator()
            : m_Item( 0 )
        {
        }

        explicit ConstIterator( const _Type* item )
            : m_Item( item )
        {
        }

        bool IsValid() const
        {
            return ( 0 != m_Item );
        }

        void Next()
        {
            if ( this->IsValid() )
            {
                m_Item = ( m_Item->*_Member ).pNext;
            }
        }

        const _Type* Item() const
        {
            return m_Item;
        }

    private:

        const _Type* m_Item;
    };

	SLListSimple(){this->pHeadLink = this->pTailLink = 0;m_Count = 0;};
	
	inline _Type* GetFirst(){return this->pHeadLink;};
	inline _Type* GetLast(){return this->pTailLink;};
	
	inline const _Type* GetFirst() const {return this->pHeadLink;};
	inline const _Type* GetLast() const {return this->pTailLink;};
	
	inline _Type* GetNext(_Type* pObj){SLIST_SIMPLE_ASSERT(pObj);return this->GetLinkFromObj(pObj)->pNext;};
	
	inline const _Type* GetNext(const _Type* pObj) const {SLIST_SIMPLE_ASSERT(pObj);return this->GetLinkFromObj(pObj)->pNext;};
	
	inline SLinkSimple<_Type> *GetLinkFromObj(_Type* pObj){SLIST_SIMPLE_ASSERT(pObj);return &(pObj->*_Member);}
	// inline _Type* GetObjFromLink(SLinkSimple<_Type> *pLink){SLIST_SIMPLE_ASSERT(pLink);return (_Type* )((u32)pLink - uLinkOffset);}
	
	inline const SLinkSimple<_Type> *GetLinkFromObj(const _Type* pObj) const {SLIST_SIMPLE_ASSERT(pObj);return &(pObj->*_Member);}
	// inline const _Type* GetObjFromLink(const SLinkSimple<_Type> *pLink) const {SLIST_SIMPLE_ASSERT(pLink);return (const _Type* )((u32)pLink - uLinkOffset);}

	// The HasXXX functions may falsely return true for links that linked into OTHER lists
	/* inline bool HasLink(const SLinkSimple<_Type> *pLink) const
	{
		SLIST_SIMPLE_ASSERT(pLink);
		const _Type* pObj = this->GetObjFromLink(pLink);
		return pLink->pNext || pObj == this->pHeadLink;
	} */
	
	inline bool HasObj(const _Type* pObj) const
	{
		SLIST_SIMPLE_ASSERT(pObj);
		const SLinkSimple<_Type> *pLink = this->GetLinkFromObj(pObj);
		return pLink->pNext || pObj == this->pHeadLink;
	}

    //PURPOSE
    //  Returns a pointer to the item if the item is indeed contained
    //  in this list.
    inline _Type* Find( const _Type* pObj )
    {
        _Type* t = this->GetFirst();

        for( ; t; t = this->GetNext( t ) )
        {
            if ( t == pObj )
            {
                break;
            }
        }

        return t;
    }
    inline const _Type* Find( const _Type* pObj ) const
    {
        return const_cast< SLListSimple< _Type, _Member >* >( this )->Find( pObj );
    }

    //PURPOSE
    //  Returns true if the list contains the given item.
    bool Contains( const _Type* item ) const
    {
        return ( 0 != this->Find( item ) );
    }
	
	inline void Add(_Type* pObj, _Type* pPrev) {AddAfter(pObj, pPrev);}

	inline void AddAfter(_Type* pObj, _Type* pPrev)
	{
		_Type	*pTemp;
		
		SLIST_SIMPLE_ASSERT(pObj);
		SLIST_SIMPLE_ASSERT(pPrev);
		SLIST_SIMPLE_ASSERT(this->pHeadLink);
		SLIST_SIMPLE_ASSERT(!this->GetLinkFromObj(pObj)->pNext);
#if __DEBUG_SLLIST
        SLIST_SIMPLE_ASSERT(!this->GetLinkFromObj(pObj)->pOwner);
#endif  //__DEBUG_SLLIST
		
		pTemp = this->GetLinkFromObj(pPrev)->pNext;
		this->GetLinkFromObj(pObj)->pNext = pTemp;
		if (pTemp == NULL)
		{
			//last one...
			this->pTailLink = pObj;
		}
		
		this->GetLinkFromObj(pPrev)->pNext = pObj;
#if __DEBUG_SLLIST
        this->GetLinkFromObj(pObj)->pOwner = this;
#endif  //__DEBUG_SLLIST

        ++m_Count;
	}
	
	inline void Push(_Type* pObj) {AddToHead(pObj);}

	inline void AddToHead(_Type* pObj)
	{
		SLIST_SIMPLE_ASSERT(pObj);
		SLIST_SIMPLE_ASSERT(!this->GetLinkFromObj(pObj)->pNext);
#if __DEBUG_SLLIST
        SLIST_SIMPLE_ASSERT(!this->GetLinkFromObj(pObj)->pOwner);
#endif  //__DEBUG_SLLIST

		if (this->pHeadLink)
		{
			//add before the head link...
			this->GetLinkFromObj(pObj)->pNext = this->pHeadLink;
			this->pHeadLink = pObj;
		}
		else
		{
			//first one...
			this->pTailLink = this->pHeadLink = pObj;
			this->GetLinkFromObj(pObj)->pNext = 0;			
		}

		++m_Count;

#if __DEBUG_SLLIST
        this->GetLinkFromObj(pObj)->pOwner = this;
#endif  //__DEBUG_SLLIST
	}
	
	inline void AddToTail(_Type* pObj)
	{
		SLIST_SIMPLE_ASSERT(pObj);
		SLIST_SIMPLE_ASSERT(!this->GetLinkFromObj(pObj)->pNext);
#if __DEBUG_SLLIST
        SLIST_SIMPLE_ASSERT(!this->GetLinkFromObj(pObj)->pOwner);
#endif  //__DEBUG_SLLIST

        if (this->pHeadLink)
		{
			//add after the tail link...
			this->AddAfter(pObj, this->pTailLink);
		}
		else
		{
			//first one...
			this->pTailLink = this->pHeadLink = pObj;
			this->GetLinkFromObj(pObj)->pNext = 0;
            ++m_Count;
		}

#if __DEBUG_SLLIST
        this->GetLinkFromObj(pObj)->pOwner = this;
#endif  //__DEBUG_SLLIST
	}
	
	inline _Type* Pop()
	{
		_Type* pObj = this->pHeadLink;
		
		if (pObj)
		{
			this->pHeadLink = this->GetLinkFromObj(pObj)->pNext;
			this->GetLinkFromObj(pObj)->pNext = 0;

			if (this->pTailLink == pObj)
			{
				//only one...
				this->pTailLink = 0;				
			}

			--m_Count;
			SLIST_SIMPLE_ASSERT( m_Count >= 0 );
		}

		return pObj;
	}

	inline _Type* Remove(_Type* pObj)
	{
		SLIST_SIMPLE_ASSERT(pObj);
		SLIST_SIMPLE_ASSERT((pObj == this->pHeadLink) || (this->GetLinkFromObj(pObj)->pNext));
#if __DEBUG_SLLIST
		SLIST_SIMPLE_ASSERT(this == this->GetLinkFromObj(pObj)->pOwner);
#endif  //__DEBUG_SLLIST

		_Type* pNext = this->pHeadLink;
		_Type* pPrev = 0;

		while (pNext)
		{
			if (pNext == pObj)
			{
				pNext = this->GetLinkFromObj(pNext)->pNext;
				Remove(pObj, pPrev);				
				break;
			}

			pPrev = pNext;
			pNext = this->GetLinkFromObj(pNext)->pNext;
		}

		return pNext;
	}

    bool IsEmpty() const
    {
        return ( 0 == pHeadLink );
    }

    int GetCount() const
    {
        return m_Count;
    }

    void Reset()
    {
        _Type* p = this->GetFirst();

        while( p )
        {
            p = this->Remove( p );
        }
    }
	
	inline void SetAndInitPool(_Type* paLinks, u32 uNumLinks)
	{
		u32		uCount;
		
		SLIST_SIMPLE_ASSERT(paLinks);
		for(uCount = 0;uCount < uNumLinks;uCount++)
		{
			this->AddToHead(&paLinks[uCount]);
		}
	}
};

} // namespace rage

#if __WIN32
#pragma warning(pop)
#endif

#endif
