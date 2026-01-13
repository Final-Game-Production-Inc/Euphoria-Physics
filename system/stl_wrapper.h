#ifndef SYSTEM_STL_WRAPPER
#define SYSTEM_STL_WRAPPER

#include <map>
#include <vector>
#include <set>
#include <string>
#include <list>
#include <sstream>
#include <iostream>
#include <unordered_set>
#include <unordered_map>
#include "system/new.h"		// for rage_heap_new

namespace rage {

	template<typename T> struct stlAllocator : public std::allocator<T> {
		stlAllocator() {}
		template<class Other> stlAllocator(const stlAllocator<Other>& /*_Right*/) {}

		inline typename std::allocator<T>::pointer allocate(typename std::allocator<T>::size_type n, typename std::allocator<void>::const_pointer = 0) {
			return reinterpret_cast<typename std::allocator<T>::pointer>(::operator rage_heap_new(n * sizeof(T)));
		}

		inline void deallocate(typename std::allocator<T>::pointer p, typename std::allocator<T>::size_type) {
			::operator delete(p);
		}

		template<typename U> struct rebind {
			typedef stlAllocator<U> other;
		};
	};

	template <typename key, typename value> class stlMap : public std::map< key, value, std::less<key>, stlAllocator< std::pair<key, value> > > {
	};

	template <typename key> class stlSet : public std::set< key, std::less<key>, stlAllocator< key > > {
	};

	template <typename key> class stlMultiSet : public std::multiset< key, std::less<key>, stlAllocator< key > > {
	};

	template <typename type> class stlVector : public std::vector< type, stlAllocator< type > > {
	};

	template <typename type> class stlList : public std::list< type, stlAllocator< type > > {
	};

	template <typename key> class stlUnorderedSet : public std::unordered_set< key, std::hash<key>, std::equal_to<key>, stlAllocator<key> > {
	};

	template <typename key, typename value> class stlUnorderedMap : public std::unordered_map< key, value, std::hash<key>, std::equal_to<key>, stlAllocator< std::pair<const key, value> > > {
	};

	template <typename key, typename value> class stlUnorderedMultiMap : public std::unordered_multimap< key, value, std::hash<key>, std::equal_to<key>, stlAllocator< std::pair<const key, value> > > {
	};

	typedef std::basic_string<char, std::char_traits<char>, stlAllocator<char> >	stlString;
	typedef std::basic_string<wchar_t, std::char_traits<wchar_t>, stlAllocator<wchar_t> >	stlWstring;

}

#endif	// SYSTEM_STL_WRAPPER