// 
// atl/slistsorted.h 
// 
// Copyright (C) 1999-2008 Rockstar Games.  All Rights Reserved. 
// 
#ifndef ATL_SLISTSORTED_H
#define ATL_SLISTSORTED_H

#include "atl/slist.h"
#include "atl/delegate.h"

namespace rage {

// PURPOSE: 
// Subclass of atSList that sort the items in the list (using a delegate)

template <class _Type> 
class atSListSorted : public atSList<_Type>
{
public:
	typedef atDelegate<bool (const _Type&, const _Type&)> LessThanFunc;
	typedef atSList<_Type> Base;

	atSListSorted(const LessThanFunc& lessThanFunc) : 
		Base(),
		m_LessThanFunc(lessThanFunc)
	{
	}

	void Sort()
	{
		if(m_LessThanFunc.IsBound())
		{
			atSListSorted<_Type> list(m_LessThanFunc);
			atSNode<_Type>* node = Base::PopHead();
			while(node)
			{
				list.SortedInsert(*node);
				node = Base::PopHead();
			}

			SwapList(list);
		}
	}

	void SortedInsert(atSNode<_Type>& node)
	{
		if(m_LessThanFunc.IsBound())
		{
			atSNode<_Type>* temp = Base::GetHead();
			if(!temp)
			{
				Base::Append(node);
			}
			else
			{
				atSNode<_Type>* prev = NULL;
				bool inserted = false;
				while(temp)
				{
					if(m_LessThanFunc(node.Data, temp->Data))
					{
						if(prev == NULL)
							Base::Prepend(node);
						else
							Base::InsertAfter(*prev, node);
						inserted = true;
						break;
					}
					else
					{
						prev = temp;
						temp = temp->GetNext();
					}
				}
				if(!inserted)
				{
					Base::InsertAfter(*prev, node);
				}
			}
		}
		else
		{
			Assert(0 && "Sorted insert failed.  Please provide a compare function");
		}
	}

protected:
	LessThanFunc	m_LessThanFunc;

};

}

#endif//ATL_SLISTSORTED_H

