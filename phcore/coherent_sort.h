#ifndef PHCORE_COHERENT_SORT_H
#define PHCORE_COHERENT_SORT_H

/*
// Implement this accessor to use the coherent sorted list functions. It is recommended that
// the cmp function use a strict inequality in order to prevent unnecessary swaps.
struct coherent_sort_accessor
{
	typedef NODE_ID NID;	// Node ID type. Could be and index, pointer, etc.
	typedef KEY_TYPE KT;	// Key type.

	static __forceinline NID null() { return NULL; }
	static __forceinline int cmp(const KT & k1, const KT & k2) { return 'compare k1 to k2'; }
	static __forceinline int equ(const KT & k1, const KT & k2) { return (k1 == k2); }
	static __forceinline const KT & get_key(const NID & nid) { return nid.key; }
};
*/

template <class A> class coherent_sort
{
public:
	// For these functions, a and b are the start and end of a list. b is included in the list.

	// frame_coherent_merge_sort is an in-place merge sort function. It breaks the list into
	// two equal halves and then recursively sorts them. It then merges the two
	// halves from the center, merging the right list into the left list with
	// an insertion sort. This works well for lists that are close to being sorted.
	void frame_coherent_merge_sort(typename A::NID * a, typename A::NID * b)
	{
		// break the list into two equal halves and recursively sort them.
		A::NID * c = a + (b - a) / 2;
		if (a < c) 
			frame_coherent_merge_sort<A>(a,c); 
		A::NID * prev_c = c; 
		c++; 
		if (c < b) 
			frame_coherent_merge_sort<A>(c,b);

		// merge the right list into the left list using an insertion sort.
		while ((c <= b) && A::cmp(A::key(*c),A::key(*prev_c))) 
		{ 
			A::NID * const save_c = c;
			A::NID const save_c_val = *c; 
			do 
			{ 
				*c = *prev_c; 
				c = prev_c; 
				prev_c--; 
			} while ((prev_c >= a) && A::cmp(A::key(save_c_val),A::key(*prev_c))); 
			*c = save_c_val; 
			prev_c = save_c; 
			c = prev_c + 1; 
		}
	}

	void verify_sorted_list(typename A::NID * a, typename A::NID * b)
	{ 
		for (A::NID * c = a, * next_c = c + 1 ; c < b ; c = next_c, next_c++) 
			Assert(A::cmp(A::key(*c),A::key(*next_c)));
	}

	typename A::NID binary_search_sorted_list(const typename A::KT & key, typename A::NID * a, typename A::NID * b)
	{ 
		while (a <= b)
		{ 
			A::NID * c = a + (b - a) / 2;
			if (A::equ(key,A::key(*c)))
				return *c;
			else if (A::cmp(key,A::key(*c))) 
				b = c - 1; 
			else
			{
				Assert(A::cmp(A::key(*c),key));
				a = c + 1;
			}
		} 
		return A::null(); 
	}

	typename A::NID search_unsorted_list(const typename A::KT & key, typename A::NID * a, typename A::NID * b)
	{ 
		for (A::NID * c = a ; c <= b ; c++)
		{
			if (A::equ(key,A::key(*c)))
				return *c;
		}
		return A::null();
	}
};

#endif // PHCORE_COHERENT_SORT_H
