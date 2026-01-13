#include <algorithm>

#if __WIN32

// This exists purely to resolve issues with 
// VS2008 swapping variables when calling lower_bound
//in a debug build

namespace std{
#if _MSC_VER == 1500 //this is the code for VS2008 apparently
	template<class _FwdIt,
	class _Ty,
	class _Diff,
	class _Pr> inline
		_FwdIt _Lower_boundRSG(_FwdIt _First, _FwdIt _Last,
		const _Ty& _Val, _Pr _Pred, _Diff *)
	{	// find first element not before _Val, using _Pred		
		//_DEBUG_POINTER(_Pred);
		//_DEBUG_ORDER_SINGLE_PRED(_First, _Last, _Pred, true);
		_Diff _Count = 0;
		_Distance(_First, _Last, _Count);
		for (; 0 < _Count; )
		{	// divide and conquer, find half that contains answer
			_Diff _Count2 = _Count / 2;
			_FwdIt _Mid = _First;
			std::advance(_Mid, _Count2);
			//_DEBUG_ORDER_SINGLE_PRED(_Mid, _Last, _Pred, false);
			if (_Pred(*_Mid, _Val))
				_First = ++_Mid, _Count -= _Count2 + 1;
			else
				_Count = _Count2;
		}
		return (_First);
	}

	template<class _FwdIt,
	class _Ty,
	class _Pr> inline
		_FwdIt lower_boundRSG(_FwdIt _First, _FwdIt _Last,
		const _Ty& _Val, _Pr _Pred)
	{	// find first element not before _Val, using _Pred
		_ASSIGN_FROM_BASE(_First,
			_Lower_boundRSG(_CHECKED_BASE(_First), _CHECKED_BASE(_Last), _Val, _Pred, _Dist_type(_First)));
		return _First;
	}
#else
#define lower_boundRSG lower_bound
#endif //VS VERSION
}
#else
#define lower_boundRSG lower_bound
#endif //__WIN32