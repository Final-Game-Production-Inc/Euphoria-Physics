#ifndef PHCORE_AVL_TREE_H
#define PHCORE_AVL_TREE_H

/*
// Example avl_tree accessor. This may seem like a lot to implement but it is done this way to maximize flexibility.
struct avl_tree_accessor
{
	typedef NODE_ID NID;		// Node ID type. Could be and index, pointer, etc.
	typedef KEY_TYPE KT;		// Key type.
	typedef BALANCE_TYPE BT;	// Balance type. Needs to be large enough to hold an integer in [-2,+2]

	static __forceinline NID null() { return NULL; }
	
	static __forceinline NID & get_left(NID & nid) { return nid->left; }
	static __forceinline NID & get_right(NID & nid) { return nid->right; }
	
	static __forceinline BT & get_bal(NID & nid) { return nid->balance; }
	
	static __forceinline const KT & get_key(const NID & nid) { return nid->key; }
	static __forceinline const void set_key(NID & nid, const KT & key) { nid->key = key; }
	
	static __forceinline int cmp(const KT & k1, const KT & k2);
	static __forceinline int equ(const KT & k1, const KT & k2);

	static __forceinline void prefetch_find(NID & nid) {}
	static __forceinline void prefetch_insert(NID & nid) {}
	static __forceinline void prefetch_remove(NID & nid) {}
};
*/

#define AVL_TREE_DEBUG 0
#define AVL_ASSERT(b,msg) FastAssert(b)

template <class A> class avl_tree
{
	template <class T> __forceinline T MAX(T a, T b) { return (a >= b) ? a : b; }

	enum { MAX_STACK_SIZE = 16 };

	struct stack_info
	{
		typename A::NID * m_nid_ptr;	// pointer to the left or right child of this node's parent. 
		typename A::BT m_child;			// branch this node went down.
	};

	typename A::NID m_root;

	__forceinline typename A::NID rotate_right(typename A::NID root)
	{
		typename A::NID left = A::get_left(root);
		A::get_left(root) = A::get_right(left);
		A::get_right(left) = root;
		A::get_bal(root) += 1 + MAX<typename A::BT>(-A::get_bal(left),0);
		A::get_bal(left) += 1 + MAX<typename A::BT>(A::get_bal(root),0);
		return left;
	}

	__forceinline typename A::NID rotate_left(typename A::NID root)
	{
		typename A::NID right = A::get_right(root);
		A::get_right(root) = A::get_left(right);
		A::get_left(right) = root;
		A::get_bal(root) -= 1 + MAX<typename A::BT>(A::get_bal(right),0);
		A::get_bal(right) -= 1 + MAX<typename A::BT>(-A::get_bal(root),0);
		return right;
	}

public:

	__forceinline avl_tree() : m_root(A::null()) 
	{
	}

	__forceinline avl_tree(typename A::NID root) : m_root(root) 
	{
	}

	__forceinline void set_root(typename A::NID root) 
	{ 
		m_root = root; 
	}
	
	__forceinline typename A::NID get_root() 
	{ 
		return m_root; 
	}

	__forceinline typename A::NID find(const typename A::KT & key)
	{
		A::prefetch_find(m_root);
		typename A::NID cur = m_root;
		while ((cur != A::null()) && (A::equ(key,A::get_key(cur)) == 0))
		{
			if (A::cmp(key,A::get_key(cur)))
				cur = A::get_left(cur);
			else
			{
				AVL_ASSERT(A::cmp(A::get_key(cur),key),"invalid key comparison logic.");
				cur = A::get_right(cur);
			}
			A::prefetch_find(cur);
		}
		return cur;
	}

	void insert(typename A::NID insert_nid, const typename A::KT & key)
	{
		A::prefetch_insert(m_root);
		stack_info stack[MAX_STACK_SIZE];
		stack_info * cur_si = stack;
		cur_si->m_nid_ptr = &m_root;
		
		// Find leaf node position for insertion.
		while (*cur_si->m_nid_ptr != A::null())
		{
			typename A::NID root = *cur_si->m_nid_ptr;
			stack_info * next_si = cur_si + 1;
			AVL_ASSERT((next_si - stack < MAX_STACK_SIZE),"avl_tree stack overflow");
			if (A::cmp(key,A::get_key(root)))
			{
				cur_si->m_child = -1;
				next_si->m_nid_ptr = &A::get_left(root);
			}
			else
			{
				AVL_ASSERT(A::cmp(A::get_key(root),key),"Node with this key already in the avl_tree or invalid key comparison logic.");
				cur_si->m_child = +1;
				next_si->m_nid_ptr = &A::get_right(root);
			}
			cur_si = next_si;
			A::prefetch_insert(*cur_si->m_nid_ptr);
		}

		// Init node.
		A::get_left(insert_nid) = A::null();
		A::get_right(insert_nid) = A::null();
		A::get_bal(insert_nid) = 0;
		A::set_key(insert_nid,key);

		// Link node into parent's child.
		*cur_si->m_nid_ptr = insert_nid;

		// Rebalance the tree.
		while (cur_si > stack)
		{
			cur_si--;
			typename A::NID & root = *cur_si->m_nid_ptr;
			A::get_bal(root) += cur_si->m_child;
			if (A::get_bal(root) == -2)
			{
				typename A::NID & left = A::get_left(root);
				AVL_ASSERT((A::get_bal(left) == -1 || A::get_bal(left) == +1),"avl_tree internal error.");
				if (A::get_bal(left) == 1)
					left = rotate_left(left);
				root = rotate_right(root);
			}
			else if (A::get_bal(root) == +2)
			{
				typename A::NID & right = A::get_right(root);
				AVL_ASSERT((A::get_bal(right) == -1 || A::get_bal(right) == +1),"avl_tree internal error.");
				if (A::get_bal(right) == -1)
					right = rotate_right(right);
				root = rotate_left(root);
			}
			if (A::get_bal(root) == 0)
				break;
		}
	}

	typename A::NID remove(const typename A::KT & key)
	{
		A::prefetch_remove(m_root);
		stack_info stack[MAX_STACK_SIZE];
		stack_info * cur_si = stack;
		cur_si->m_nid_ptr = &m_root;
		
		// Find the remove node.
		for (;;)
		{
			typename A::NID root = *cur_si->m_nid_ptr;
			if (root == A::null())
				return A::null();
			stack_info * next_si = cur_si + 1;
			AVL_ASSERT((next_si - stack < MAX_STACK_SIZE),"avl_tree stack overflow");
			if (A::cmp(key,A::get_key(root)))
			{
				cur_si->m_child = -1;
				next_si->m_nid_ptr = &A::get_left(root);
			}
			else if (A::cmp(A::get_key(root),key))
			{
				cur_si->m_child = +1;
				next_si->m_nid_ptr = &A::get_right(root);
			}
			else
			{
				AVL_ASSERT(A::equ(key,A::get_key(root)),"Invalid key comparison logic.");
				break;
			}
			cur_si = next_si;
			A::prefetch_remove(*cur_si->m_nid_ptr);
		}

		// Find the replace node.
		typename A::NID * remove_nid = cur_si->m_nid_ptr;
		if (A::get_right(*remove_nid) != A::null())
		{
			cur_si->m_child = +1;
			cur_si++;
			cur_si->m_nid_ptr = &A::get_right(*remove_nid);
			stack_info * right_si = cur_si;
			while (A::get_left(*cur_si->m_nid_ptr) != A::null())
			{
				stack_info * next_si = cur_si + 1;
				AVL_ASSERT((next_si - stack < MAX_STACK_SIZE),"avl_tree stack overflow.");
				cur_si->m_child = -1;
				next_si->m_nid_ptr = &A::get_left(*cur_si->m_nid_ptr);
				cur_si = next_si;
			}
			typename A::NID replace_nid = *cur_si->m_nid_ptr;
			*cur_si->m_nid_ptr = A::get_right(replace_nid);
			A::get_left(replace_nid) = A::get_left(*remove_nid);
			A::get_right(replace_nid) = A::get_right(*remove_nid);
			A::get_bal(replace_nid) = A::get_bal(*remove_nid);
			right_si->m_nid_ptr = &A::get_right(replace_nid);
			*remove_nid = replace_nid;
		}
		else
			*remove_nid = A::get_left(*remove_nid);

		// Rebalance the tree.
		while (cur_si > stack)
		{
			cur_si--;
			typename A::NID & root = *cur_si->m_nid_ptr;
			A::get_bal(root) -= cur_si->m_child;
			if (A::get_bal(root) == -2)
			{
				typename A::NID & left = A::get_left(root);
				if (A::get_bal(left) == 1)
					left = rotate_left(left);
				root = rotate_right(root);
				AVL_ASSERT((A::get_bal(root) == 0 || A::get_bal(root) == +1),"avl_tree internal error.");
			}
			else if (A::get_bal(root) == +2)
			{
				typename A::NID & right = A::get_right(root);
				if (A::get_bal(right) == -1)
					right = rotate_right(right);
				root = rotate_left(root);
				AVL_ASSERT((A::get_bal(root) == 0 || A::get_bal(root) == -1),"avl_tree internal error.");
			}
			if (A::get_bal(root) != 0)
				break;
		}
		return *remove_nid;
	}

	__forceinline void remove_all() 
	{ 
		m_root = A::null(); 
	} 

#if AVL_TREE_DEBUG
	int verify_tree_recurse(typename A::NID root)
	{
		const typename A::BT REPEAT_CHECK = -4;
		if (root != A::null())
		{
			AVL_ASSERT((A::get_left(root) != A::get_right(root) || A::get_left(root) == A::null()),"invalid avl_tree.");
			AVL_ASSERT((A::get_bal(root) != REPEAT_CHECK),"Circular link in avl_tree.");
			AVL_ASSERT((A::get_bal(root) >= -1 && A::get_bal(root) <= +1),"Invalid avl_tree balance.");
			typename A::BT save_bal = A::get_bal(root);
			int left_height = verify_tree_recurse(A::get_left(root));
			int right_height = verify_tree_recurse(A::get_right(root));
			A::get_bal(root) = save_bal;
			AVL_ASSERT((right_height - left_height == A::get_bal(root)),"avl_tree balance doesn't match height.");
			return MAX<int>(left_height,right_height) + 1;
		}
		else
			return 0;
	}

	void verify_tree()
	{
		verify_tree_recurse(m_root);
	}
#endif // AVL_TREE_DEBUG
};

#endif // PHCORE_AVL_TREE_H
