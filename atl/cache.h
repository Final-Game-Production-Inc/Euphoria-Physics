//
// atl/cache.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef ATL_CACHE_H
#define ATL_CACHE_H

#include <memory.h>
#include <math.h>
#include <float.h>

#include "math/constants.h"

namespace rage
{

/*
PURPOSE:
	A generic trait to make the is-equal check of the key and values a pluggable
	element for the cache.

REMARKS:
	See the two float and double based traits for examples of implementing
	traits for your own types below.

PARAMETERS:
	_T: The type that the equals trait check for.
 */
template<class _T>
struct atCacheEqualsTrait
{
	static bool equals(const _T& t1, const _T& t2)
	{
		return t1 == t2;
	}
};


/*
PURPOSE:
	A simple cache that stores a number of keys/value pairs, and
	reuses the oldest entry when it runs out of space.

REMARKS:
	The current size of the cache defaults to 16 which should suffice in
	most cases. The maximum size is 127.

	The only requirements on the _Key and _Value are the assignment and
	equals operator. The equals comparison can be overridden by writing your own
	atlEqualsTraits template specialisation.

	The basic use of the cache is as a static memory of a function call:

	<CODE>
	CNode* ExpensiveCalculation(int input)
	{
		static atCache<int, CNode*> cache;
		CNode* output;
		if (cache.Read(input, output) == true)
		{
			return output;
		}

		... do expensive calculation  ...

		cache.Store(input, output);
		return output;
	}
	</CODE>

	If the data in the cache needs to be invalidated, the cache supports complete
	invalidation and invalidation by key or value. At that stage the cache is best
	integrated as a member of a class design, so it can be accessed for those
	situations. Otherwise, the cache can work fine as a static variable inside 
	function scope.

	A possible variant use of the cache above could be a close-enough-cache.
	This can be implemented by plugging in a trait that implements a very rough 
	equals function for a key. A cache such as that could be used to speed up
	a closest cover finder, as the cache returns cover points around a rough position 
	once they have been calculated. 

PARAMETERS:
	_Key - The type of the key that the cache uses to find a previous value.
	_Value - The type of the value that is associated with the key.
	_Size - The size of the cache, beyond which the oldest items are reused (default 16).
	_KeyTraits - The trait to check for equal keys (default atCacheEqualsTrait<_Key>).
	_ValueTraits - The trait to check for equal values (default atCacheEqualsTrait<_Value>).
 */
template<class _Key, class _Value, int _Size = 16, class _KeyTraits = atCacheEqualsTrait<_Key>,
		class _ValueTraits = atCacheEqualsTrait<_Value> >
class atCache
{
public:
	// Constructor.
	atCache()
	{
		for (_Index i = 0; i < _Size; ++i)
		{
			m_indices[i] = i | UNUSED;
		}
	}

	// Check a particular key and get the value back if it is in the cache.
	// Returns true if the key was found, false otherwise. Value is only changed when true.
	bool Read(const _Key& key, _Value& value)
	{
		for (_Index i = 0; i < _Size; ++i)
		{
			if (m_indices[i] & UNUSED)
			{
				break;
			}

			_Index index = m_indices[i] & ~UNUSED;

			if (_KeyTraits::equals(m_keys[index], key))
			{
				value = m_values[index];

				// move the item to the front
				m_indices[i] = m_indices[0];
				m_indices[0] = index;
				return true;
			}
		}

		return false;
	}

	// Stores a value into the cache.
	// Replaces oldest entry if the number of items is _Size.
	void Store(const _Key& key, const _Value& value)
	{
		_Index i;
		for (i = 0; i < _Size - 1; ++i) // stop one before end
		{
			if (m_indices[i] & UNUSED || _KeyTraits::equals(m_keys[m_indices[i]], key))
			{
				break;
			}
		}

		m_indices[i] &= ~UNUSED;
		_Index index = m_indices[i];

		// move index to the front
		m_indices[i] = m_indices[0];
		m_indices[0] = index;

		// store values
		m_values[index] = value;
		m_keys[index] = key;
	}

	// Invalidates all the keys.
	void InvalidateAll()
	{
		for (_Index i = 0; i < _Size; ++i)
		{
			m_indices[i] |= UNUSED;
		}
	}

	// Invalidate a specific key.
	void InvalidateKey(const _Key& key)
	{
		// find the key
		for (_Index i = 0; i < _Size; ++i)
		{
			_Index index = m_indices[i];

			if (index & UNUSED)
			{
				break;
			}

			if (_KeyTraits::equals(m_keys[index], key))
			{
				// invalidate and move to the end
				memcpy(&m_indices[i], &m_indices[i+1], (_Size - i - 1) * sizeof(_Index));
				m_indices[_Size - 1] = index & UNUSED;
				break; // quit after key found
			}
		}
	}

	// Invalidate a particular value.
	// All the keys with this value will be invalidated.
	void InvalidateValue(const _Value& value)
	{
		// find the value
		for (_Index i = 0; i < _Size; ++i)
		{
			_Index index = m_indices[i];

			if (index & UNUSED)
			{
				break;
			}

			if (_ValueTraits::equals(m_values[index], value))
			{
				// invalidate and move to the end
				memcpy(&m_indices[i], &m_indices[i+1], (_Size - i - 1) * sizeof(_Index));
				m_indices[_Size - 1] = index & UNUSED;
				// continue - there may be another key with the same value
			}
		}
	}

private:
	typedef unsigned char	_Index;					// type of the index

	static const _Index		UNUSED = (1 << (sizeof(_Index) * 8 - 1));
													// UNUSED flag - any indices that aren't used are masked with this value.
	_Index					m_indices[_Size];		// a set of indices sorted by age, with UNUSED indices moved to the end.
	_Key					m_keys[_Size];			// array of keys
	_Value					m_values[_Size];		// array of values
}; 


// Trait specific to float comparisons.
template<>
struct atCacheEqualsTrait<float>
{
	static bool equals(const float& t1, const float& t2)
	{
		return fabs(t1 - t2) < SMALL_FLOAT;
	}
};


// Trait specific to double comparisons.
template<>
struct atCacheEqualsTrait<double>
{
	static bool equals(const double& t1, const double& t2)
	{
		return fabs(t1 - t2) < VERY_SMALL_FLOAT;
	}
};

} // namespace rage

#endif // ATL_CACHE_H
