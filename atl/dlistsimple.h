//
// atl/dlistsimple.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef ATL_DLISTSIMPLE_H
#define ATL_DLISTSIMPLE_H

#include "data/struct.h"

#if __WIN32
#pragma warning(push)
#pragma warning(disable: 4201)
#pragma warning(disable: 4324)
#endif

// You can use this as an easy way to initialize the link list template...
#define DLIST_SIMPLE_INIT(CLASS_NAME, LINK_NAME) CLASS_NAME, &CLASS_NAME::LINK_NAME

//PURPOSE
//  Enable __DEBUG_DLLIST to validate item insertion and removal.
#ifndef __DEBUG_DLLIST
#define __DEBUG_DLLIST  0
#endif  //__DEBUG_DLLIST

#if 0
#define DLIST_SIMPLE_ASSERT(X) FastAssert(X)
#else
#define DLIST_SIMPLE_ASSERT(X)
#endif

namespace rage {

/*
PURPOSE: 
	The class that holds the the forwards and backwards link for a class.
*/
template <class _Type> struct DLinkSimple
{
public:
	datRef<_Type>	pPrev;
	datRef<_Type>	pNext;
	
	//this is really largely for debugging, but sometimes darn useful...
	DLinkSimple()
    {
        this->pPrev = this->pNext = 0;
#if __DEBUG_DLLIST
        pOwner = 0;
#endif  //__DEBUG_DLLIST
    }

	DLinkSimple(datResource&)	{}
	
	IMPLEMENT_PLACE_INLINE(DLinkSimple);

#if __DECLARESTRUCT
	void DeclareStruct(datTypeStruct& s)
	{
		STRUCT_BEGIN(DLinkSimple);
		STRUCT_FIELD(pPrev);
		STRUCT_FIELD(pNext);
		STRUCT_END();
	}
#endif

#if __DEBUG_DLLIST
    //The list in which we're contained - used for debugging
    void* pOwner;
#endif  //__DEBUG_DLLIST
};

/*
PURPOSE: 
	Yet another doubly linked list template.  This one saves some space by only having 
	the previous and next pointers embedded in the link part.  The tradeoff is that the 
	list class needs to know the offset for each link.  As such, only homogenous types 
	may be linked up(hence the name 'simple').  Otherwise, this class sould be fairly 
	simple to use - you embed as many DLinkSimple classes as you wish in a class, and 
	you initialize a list with the offset to the particular link structure you want to use, 
	using the DLIST_SIMPLE_INIT, if you want.
*/
template <class _Type, DLinkSimple<_Type> _Type::* _Member> class DLListSimple
{
private:
	_Type		*pHeadLink;
	_Type		*pTailLink;

    int m_Count;
	
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

        void Prev()
        {
            m_Item ? m_Item = ( m_Item->*_Member ).pPrev : 0;
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
            if( this->IsValid() )
            {
                m_Item = ( m_Item->*_Member ).pNext;
            }
        }

        void Prev()
        {
            if( this->IsValid() )
            {
                m_Item = ( m_Item->*_Member ).pPrev;
            }
        }

        const _Type* Item() const
        {
            return m_Item;
        }

    private:

        const _Type* m_Item;
    };

	DLListSimple(){this->pHeadLink = this->pTailLink = 0;m_Count = 0;};
	
	inline _Type *GetFirst(){return this->pHeadLink;};
	inline _Type *GetLast(){return this->pTailLink;};
	
	inline const _Type *GetFirst() const {return this->pHeadLink;};
	inline const _Type *GetLast() const {return this->pTailLink;};
	
	inline _Type *GetPrev(_Type *pObj){DLIST_SIMPLE_ASSERT(pObj);return this->GetLinkFromObj(pObj)->pPrev;};
	inline _Type *GetNext(_Type *pObj){DLIST_SIMPLE_ASSERT(pObj);return this->GetLinkFromObj(pObj)->pNext;};
	
	inline const _Type *GetPrev(const _Type *pObj) const {DLIST_SIMPLE_ASSERT(pObj);return this->GetLinkFromObj(pObj)->pPrev;};
	inline const _Type *GetNext(const _Type *pObj) const {DLIST_SIMPLE_ASSERT(pObj);return this->GetLinkFromObj(pObj)->pNext;};
	
	inline DLinkSimple<_Type> *GetLinkFromObj(_Type *pObj){DLIST_SIMPLE_ASSERT(pObj);return &(pObj->*_Member);}
	// inline _Type *GetObjFromLink(DLinkSimple<_Type> *pLink){DLIST_SIMPLE_ASSERT(pLink);return (_Type *)((u32)pLink - uLinkOffset);}
	
	inline const DLinkSimple<_Type> *GetLinkFromObj(const _Type *pObj) const {DLIST_SIMPLE_ASSERT(pObj);return &(pObj->*_Member);}
	// inline const _Type *GetObjFromLink(const DLinkSimple<_Type> *pLink) const {DLIST_SIMPLE_ASSERT(pLink);return (const _Type *)((u32)pLink - uLinkOffset);}

	// The HasXXX functions may falsely return true for links that linked into OTHER lists
	/* inline bool HasLink(const DLinkSimple<_Type> *pLink) const
	{
		DLIST_SIMPLE_ASSERT(pLink);
		const _Type *pObj = this->GetObjFromLink(pLink);
		return pLink->pNext || pLink->pPrev || pObj == this->pHeadLink;
	} */
	
	inline bool HasObj(const _Type *pObj) const
	{
		DLIST_SIMPLE_ASSERT(pObj);
		const DLinkSimple<_Type> *pLink = this->GetLinkFromObj(pObj);
		return pLink->pNext || pLink->pPrev || pObj == this->pHeadLink;
	}

    //PURPOSE
    //  Returns a pointer to the item if the item is indeed contained
    //  in this list.
    inline _Type* Find( const _Type* pObj )
    {
        _Type* t = this->GetFirst();

        for( ; t; t = this->GetNext( t ) )
        {
            if( t == pObj )
            {
                break;
            }
        }

        return t;
    }
    inline const _Type* Find( const _Type* pObj ) const
    {
        return const_cast< DLListSimple< _Type, _Member >* >( this )->Find( pObj );
    }

    //PURPOSE
    //  Returns true if the list contains the given item.
    bool Contains( const _Type* item ) const
    {
        return ( 0 != this->Find( item ) );
    }

	inline void AddBefore(_Type *pObj, _Type *pNext)
	{
		_Type	*pTemp;
		
		DLIST_SIMPLE_ASSERT(pObj);
		DLIST_SIMPLE_ASSERT(pNext);
		DLIST_SIMPLE_ASSERT(this->pHeadLink);
		DLIST_SIMPLE_ASSERT(!this->GetLinkFromObj(pObj)->pPrev);
		DLIST_SIMPLE_ASSERT(!this->GetLinkFromObj(pObj)->pNext);
#if __DEBUG_DLLIST
        DLIST_SIMPLE_ASSERT(!this->GetLinkFromObj(pObj)->pOwner);
#endif  //__DEBUG_DLLIST
		
		pTemp = this->GetLinkFromObj(pNext)->pPrev;
		this->GetLinkFromObj(pObj)->pPrev = pTemp;
		if(pTemp)
		{
			//not first one...
			this->GetLinkFromObj(this->GetLinkFromObj(pObj)->pPrev)->pNext = pObj;
		}
		else
		{
			//first one...
			this->pHeadLink = pObj;
		}
		
		this->GetLinkFromObj(pNext)->pPrev = pObj;
		this->GetLinkFromObj(pObj)->pNext = pNext;
#if __DEBUG_DLLIST
        this->GetLinkFromObj(pObj)->pOwner = this;
#endif  //__DEBUG_DLLIST

        ++m_Count;
	}
	
	inline void AddAfter(_Type *pObj, _Type *pPrev)
	{
		_Type	*pTemp;
		
		DLIST_SIMPLE_ASSERT(pObj);
		DLIST_SIMPLE_ASSERT(pPrev);
		DLIST_SIMPLE_ASSERT(this->pHeadLink);
		DLIST_SIMPLE_ASSERT(!this->GetLinkFromObj(pObj)->pPrev);
		DLIST_SIMPLE_ASSERT(!this->GetLinkFromObj(pObj)->pNext);
#if __DEBUG_DLLIST
        DLIST_SIMPLE_ASSERT(!this->GetLinkFromObj(pObj)->pOwner);
#endif  //__DEBUG_DLLIST
		
		pTemp = this->GetLinkFromObj(pPrev)->pNext;
		this->GetLinkFromObj(pObj)->pNext = pTemp;
		if(pTemp)
		{
			//not first one...
			this->GetLinkFromObj(this->GetLinkFromObj(pObj)->pNext)->pPrev = pObj;
		}
		else
		{
			//last one...
			this->pTailLink = pObj;
		}
		
		this->GetLinkFromObj(pPrev)->pNext = pObj;
		this->GetLinkFromObj(pObj)->pPrev = pPrev;
#if __DEBUG_DLLIST
        this->GetLinkFromObj(pObj)->pOwner = this;
#endif  //__DEBUG_DLLIST

        ++m_Count;
	}
	
	inline void AddToHead(_Type *pObj)
	{
		DLIST_SIMPLE_ASSERT(pObj);
		DLIST_SIMPLE_ASSERT(!this->GetLinkFromObj(pObj)->pPrev);
		DLIST_SIMPLE_ASSERT(!this->GetLinkFromObj(pObj)->pNext);
#if __DEBUG_DLLIST
        DLIST_SIMPLE_ASSERT(!this->GetLinkFromObj(pObj)->pOwner);
#endif  //__DEBUG_DLLIST

		if(this->pHeadLink)
		{
			//add before the head link...
			this->AddBefore(pObj, this->pHeadLink);
		}
		else
		{
			//first one...
			this->pTailLink = this->pHeadLink = pObj;
			this->GetLinkFromObj(pObj)->pPrev = this->GetLinkFromObj(pObj)->pNext = 0;

            ++m_Count;
		}

#if __DEBUG_DLLIST
        this->GetLinkFromObj(pObj)->pOwner = this;
#endif  //__DEBUG_DLLIST
	}
	
	inline void AddToTail(_Type *pObj)
	{
		DLIST_SIMPLE_ASSERT(pObj);
		DLIST_SIMPLE_ASSERT(!this->GetLinkFromObj(pObj)->pPrev);
		DLIST_SIMPLE_ASSERT(!this->GetLinkFromObj(pObj)->pNext);
#if __DEBUG_DLLIST
        DLIST_SIMPLE_ASSERT(!this->GetLinkFromObj(pObj)->pOwner);
#endif  //__DEBUG_DLLIST

        if(this->pHeadLink)
		{
			//add after the tail link...
			this->AddAfter(pObj, this->pTailLink);
		}
		else
		{
			//first one...
			this->pTailLink = this->pHeadLink = pObj;
			this->GetLinkFromObj(pObj)->pPrev = this->GetLinkFromObj(pObj)->pNext = 0;
            ++m_Count;
		}

#if __DEBUG_DLLIST
        this->GetLinkFromObj(pObj)->pOwner = this;
#endif  //__DEBUG_DLLIST
	}
	
	inline _Type* Remove(_Type *pObj)
	{
		DLIST_SIMPLE_ASSERT(pObj);
		DLIST_SIMPLE_ASSERT((pObj == this->pHeadLink) || (this->GetLinkFromObj(pObj)->pPrev || this->GetLinkFromObj(pObj)->pNext));
#if __DEBUG_DLLIST
        DLIST_SIMPLE_ASSERT(this == this->GetLinkFromObj(pObj)->pOwner);
#endif  //__DEBUG_DLLIST

        _Type* next = ( pObj->*_Member ).pNext;

		if(this->GetLinkFromObj(pObj)->pPrev == 0)
		{
			//first one...
			DLIST_SIMPLE_ASSERT(this->pHeadLink == pObj);
			this->pHeadLink = this->GetLinkFromObj(pObj)->pNext;
			if(this->GetLinkFromObj(pObj)->pNext)
			{
				//not the only one...
				this->GetLinkFromObj(this->GetLinkFromObj(pObj)->pNext)->pPrev = 0;
			}
			else
			{
				//only one...
				this->pTailLink = 0;
			}
		}
		else
		{
			//not first one...
			this->GetLinkFromObj(this->GetLinkFromObj(pObj)->pPrev)->pNext = this->GetLinkFromObj(pObj)->pNext;
			if(this->GetLinkFromObj(pObj)->pNext)
			{
				//not the last...
				this->GetLinkFromObj(this->GetLinkFromObj(pObj)->pNext)->pPrev = this->GetLinkFromObj(pObj)->pPrev;
			}
			else
			{
				//last one...
				this->pTailLink = this->GetLinkFromObj(pObj)->pPrev;
			}
		}
		this->GetLinkFromObj(pObj)->pPrev = this->GetLinkFromObj(pObj)->pNext = 0; //mostly for debug, but useful!

        --m_Count;
        DLIST_SIMPLE_ASSERT( m_Count >= 0 );

#if __DEBUG_DLLIST
        this->GetLinkFromObj(pObj)->pOwner = 0;
#endif  //__DEBUG_DLLIST

        return next;
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
	
	inline void SetAndInitPool(_Type *paLinks, u32 uNumLinks)
	{
		u32		uCount;
		
		DLIST_SIMPLE_ASSERT(paLinks);
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
