// 
// atl/delegatetemplate.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifdef DLGT_COMMA
#undef DLGT_COMMA
#undef DLGT_AND
#endif

#if DLGT_PARAM_COUNT > 0
#define DLGT_COMMA	,
#define DLGT_AND	&&
#else
#define DLGT_COMMA
#define DLGT_AND
#endif

#define DLGT_NAME       AT_PASTE(atDelegate, DLGT_PARAM_COUNT)
#define DLGTOR_NAME     AT_PASTE(atDelegator, DLGT_PARAM_COUNT)
#define CLSR_NAME       AT_PASTE(atClosure, DLGT_PARAM_COUNT)

#ifdef DLGT_ARGS
#undef DLGT_ARGS
#undef DLGT_PARAMS
#undef DLGT_MEMBERS
#undef DLGT_ASSIGN
#undef DLGT_EQUAL_AND
#endif

#define DLGT_ARGS(A)        AT_ARGS(DLGT_PARAM_COUNT, A)
#define DLGT_PARAMS(P, p)   AT_PARAMS(DLGT_PARAM_COUNT, P, p)
#define DLGT_MEMBERS(P, p)  AT_DECLARE(DLGT_PARAM_COUNT, P, p)
#define DLGT_ASSIGN(p, q)	AT_OP(DLGT_PARAM_COUNT, p, =, q)
#define DLGT_EQUAL_AND(p,q)	AT_OP_SEQ(DLGT_PARAM_COUNT, p, ==, q, &&)

template< typename R DLGT_COMMA DLGT_ARGS(typename P) >
class DLGT_NAME
{
    //PURPOSE
    //  Given a member function extract the type of the object
    //  in which the member function was declared.
    template< typename TT >
    struct TypeFromMember;

    template< typename TT, typename RR DLGT_COMMA DLGT_ARGS(typename PP) >
    struct TypeFromMember< RR (TT::*)(DLGT_ARGS(PP)) >
    {
        typedef TT Type;
    };

    template< typename TT, typename RR DLGT_COMMA DLGT_ARGS(typename PP) >
    struct TypeFromMember< RR (TT::*)(DLGT_ARGS(PP)) const >
    {
        typedef TT Type;
    };

    //PURPOSE
    //  Given a member function determine if its signature
    //  matches the signature of the delegate.
    template<typename TT>
    struct SigMatch
    {
        struct Yes{ char a[1]; };
        struct No{ char a[128]; };

        static No IsMatch(...);
        static Yes IsMatch(R (TT::*)(DLGT_ARGS(P)));
        static Yes IsMatch(R (TT::*)(DLGT_ARGS(P)) const);
    };

public:

    DLGT_NAME()
    {
        this->Reset();
    }

    //PURPOSE
    //  Constructor that binds a member function.
    template< typename X, typename Y >
    DLGT_NAME(X* target, Y memFunc)
    {
        this->Bind(target, memFunc);
    }

    //PURPOSE
    //  Constructor that binds a free function.
    explicit DLGT_NAME(R (*freeFunc)(DLGT_ARGS(P)))
    {
        this->Bind(freeFunc);
    }

    //PURPOSE
    //  Reset the delegate to its default state.
    void Reset()
    {
        m_Target = 0;
        m_Func.m_MemFunc = 0;
        m_Func.m_FreeFunc = 0;
    }

    //PURPOSE
    //  Binds a member function.  Calling operator() will dispatch the
    //  function call on the target.
    template< typename X, typename Y >
    void Bind(X* target, Y memFunc)
    {
        //Make sure Y has the signature specified by the template args
        CompileTimeAssert(sizeof(SigMatch<typename TypeFromMember<Y>::Type>::IsMatch(memFunc)) ==
                        sizeof(typename SigMatch<typename TypeFromMember<Y>::Type>::Yes));
        this->Reset();
        m_Target = delegate_detail::BindHelper(target,
                                                (typename TypeFromMember< Y >::Type*) 0,
                                                memFunc,
                                                m_Func.m_MemFunc);
    }

    //PURPOSE
    //  Binds a free function.  Calling operator() will call the function.
    void Bind(R (*freeFunc)(DLGT_ARGS(P)))
    {
        this->Reset();
        m_Func.m_FreeFunc = freeFunc;
    }

    //PURPOSE
    //  Synonym for Reset()
    void Unbind()
    {
        this->Reset();
    }

    //PURPOSE
    //  Synonym for Reset()
    void Clear()
    {
        this->Reset();
    }

    //PURPOSE
    //  Calls the bound function.
    R Invoke(DLGT_PARAMS(P, p)) const
    {
        FastAssert(this->IsBound());

        return m_Target ? (m_Target->*m_Func.m_MemFunc)(DLGT_ARGS(p)) : (*m_Func.m_FreeFunc)(DLGT_ARGS(p));
    }

    //PURPOSE
    //  Calls the bound function.
    R operator()(DLGT_PARAMS(P, p)) const
    {
        return this->Invoke(DLGT_ARGS(p));
    }

    //PURPOSE
    //  Returns true if the delegate is bound to a function.
    bool IsBound() const
    {
        return 0 != m_Func.m_MemFunc || 0 != m_Func.m_FreeFunc;
    }

    //PURPOSE
    //  Changes the object on which this function is invoked.
    //NOTES
    //	No checks are done to ensure that the function is still callable on the
    //  new object. Use with caution!
    template< typename X >
    void Retarget(X* target)
    {
        m_Target = (delegate_detail::GenericClass*) target;
    }

    bool operator==(const DLGT_NAME< R DLGT_COMMA DLGT_ARGS(P) >& that) const
    {
        return (m_Target == that.m_Target
                 && m_Func.m_MemFunc == that.m_Func.m_MemFunc
                 && m_Func.m_FreeFunc == that.m_Func.m_FreeFunc);
    }

    bool operator!=(const DLGT_NAME< R DLGT_COMMA DLGT_ARGS(P) >& that) const
    {
        return (m_Target != that.m_Target
                 || m_Func.m_MemFunc != that.m_Func.m_MemFunc
                 || m_Func.m_FreeFunc != that.m_Func.m_FreeFunc);
    }

private:

    delegate_detail::GenericClass* m_Target;
    union
    {
        R (delegate_detail::GenericClass::*m_MemFunc)(DLGT_ARGS(P));
        R (*m_FreeFunc)(DLGT_ARGS(P));
    } m_Func;
};

template< typename R DLGT_COMMA DLGT_ARGS(typename P) >
class DLGTOR_NAME
{
    typedef DLGTOR_NAME< R DLGT_COMMA DLGT_ARGS(P) > DelegatorType;

public:

    //PURPOSE
    //  Delegate that can be registered with a delegator.
    class Delegate : public DLGT_NAME< R DLGT_COMMA DLGT_ARGS(P) >
    {
        friend class DLGTOR_NAME< R DLGT_COMMA DLGT_ARGS(P) >;

        typedef DLGT_NAME< R DLGT_COMMA DLGT_ARGS(P) > Super;

    public:

        Delegate()
            : m_Owner(0)
        {
        }

        Delegate(const Delegate& that)
        {
            *this = that;
        }

        //PURPOSE
        //  Constructor that binds a member function.
        template< typename X, typename Y >
        Delegate(X* target, R (Y::*memFunc)(DLGT_ARGS(P)))
            : m_Owner(0)
        {
            this->Bind(target, memFunc);
        }

        //PURPOSE
        //  Constructor that binds a const member function.
        template< typename X, typename Y >
        Delegate(X* target, R (Y::*constMemFunc)(DLGT_ARGS(P)) const)
            : m_Owner(0)
        {
            this->Bind(target, constMemFunc);
        }

        //PURPOSE
        //  Constructor that binds a free function.
        explicit Delegate(R (*freeFunc)(DLGT_ARGS(P)))
            : m_Owner(0)
        {
            this->Bind(freeFunc);
        }

        ~Delegate()
        {
            AssertMsg(!m_Owner , "Delegate is still registered to a delegator");
        }

        //PURPOSE
        //  Returns a pointer to the delegator with which the delegate is
        //  registered.
        DelegatorType* GetOwner()
        {
            return m_Owner;
        }

        //PURPOSE
        //  Returns a pointer to the delegator with which the delegate is
        //  registered.
        const DelegatorType* GetOwner() const
        {
            return m_Owner;
        }

        //PURPOSE
        //  Returns true if the delegate is registered with a delegator.
        bool IsRegistered() const
        {
            return (NULL != m_Owner);
        }

        //PURPOSE
        //  Removes the delegate from the delegator with which it's registered.
        void Cancel()
        {
            //Copy to local storage to avoid contention with other threads.
            DelegatorType* owner = m_Owner;

            if(owner)
            {
                owner->RemoveDelegate(this);
            }
        }

        Delegate& operator=(const Delegate& that)
        {
            if(this != &that)
            {
                (Super&) *this = (const Super&) that;

                //We want to leave our selves registered to our current
                //owner so don't copy the link or the owner.
            }

            return *this;
        }

        inlist_node< Delegate > m_ListLink;

    private:

        DelegatorType* m_Owner;
    };

    DLGTOR_NAME()
        : m_ItCur(0)
    {
    }

    ~DLGTOR_NAME()
    {
        this->Clear();
    }

    //PURPOSE
    //  Adds a delegate to the delegator.
    void AddDelegate(Delegate* dlgt)
    {
		if(dlgt)
		{
			const void* prevOwner = sysInterlockedCompareExchangePointer((void**) &dlgt->m_Owner, this, 0);
			Assert(0 == prevOwner);
			if(0 == prevOwner)
			{
				m_Delegates.push_back(dlgt);
			}
		}
    }

    //PURPOSE
    //  Removes a delegate from the delegator.
    void RemoveDelegate(Delegate* dlgt)
    {
        if(dlgt)
        {
            const void* prevOwner = sysInterlockedCompareExchangePointer((void**) &dlgt->m_Owner, 0, this);
			Assert(!prevOwner || this == prevOwner);
            if(prevOwner && this == prevOwner)
            {
                if(m_ItCur)
                {
                    m_ItCur->Skip(dlgt);
                }

                m_Delegates.erase(dlgt);
            }
        }
    }

    //PURPOSE
    //  Removes all delegates from the delegator.
    void Clear()
    {
        while(!m_Delegates.empty())
        {
            this->RemoveDelegate(*m_Delegates.begin());
        }
    }

    //PURPOSE
    //  Returns true if the given delegate is registered with the delegator.
    bool HasDelegate(const Delegate* delegate) const
    {
        return (this == delegate->m_Owner);
    }

    //PURPOSE
    //  Returns true if any delegates are registered.
    bool HasDelegates() const
    {
        return m_Delegates.size() > 0;
    }

    //PURPOSE
    //  Calls the function bound to each delegate registered with the
    //  delegator.
    void Dispatch(DLGT_PARAMS(P, p))
    {
        Iterator< Delegates > it(m_Delegates.begin(), m_Delegates.end());

        it.m_Next = m_ItCur;
        m_ItCur = &it;

        while(it.IsValid())
        {
            Delegate* dlgt = *it.m_It;
            FastAssert(this == dlgt->m_Owner);

            (*dlgt)(DLGT_ARGS(p));

            it.Advance();
        }

        m_ItCur = m_ItCur->m_Next;
    }

    //PURPOSE
    //  Calls the function bound to each delegate registered with the
    //  delegator.
    void operator()(DLGT_PARAMS(P, p))
    {
        this->Dispatch(DLGT_ARGS(p));
    }

private:

    typedef inlist< Delegate, &Delegate::m_ListLink > Delegates;

    Delegates m_Delegates;

    template< typename T >
    struct Iterator
    {
        Iterator()
            : m_Next(0)
            , m_Advance(true)
        {
        }

        Iterator(const typename T::iterator& it,
                    const typename T::const_iterator& stop)
            : m_Next(0)
            , m_It(it)
            , m_Stop(stop)
            , m_Advance(true)
        {
        }

        void Skip(const typename T::value_type& val)
        {
            for(Iterator< T >* cur = this; cur; cur = cur->m_Next)
            {
                if(cur->IsValid() && val == *cur->m_It)
                {
                    ++cur->m_It;
                    cur->m_Advance = false;
                }
            }
        }

        void Advance()
        {
            if(m_Advance)
            {
                ++m_It;
            }
            else
            {
                m_Advance = true;
            }
        }

        bool IsValid() const
        {
            return m_Stop != m_It;
        }

        Iterator* m_Next;
        typename T::iterator m_It;
        typename T::const_iterator m_Stop;
        bool m_Advance  : 1;
    };

    Iterator< Delegates >* m_ItCur;

    //Prevent copying
    DLGTOR_NAME< R DLGT_COMMA DLGT_ARGS(P) >(const DLGTOR_NAME< R DLGT_COMMA DLGT_ARGS(P) >&);
    DLGTOR_NAME< R DLGT_COMMA DLGT_ARGS(P) >& operator=(const DLGTOR_NAME< R DLGT_COMMA DLGT_ARGS(P) >&);
};

template< typename R DLGT_COMMA DLGT_ARGS(typename P) >
class atDelegate< R (DLGT_ARGS(P)) > : public DLGT_NAME< R DLGT_COMMA DLGT_ARGS(P) >
{
public:

    atDelegate()
    {
    }

    //PURPOSE
    //  Constructor that binds a member function.
    template< typename X, typename Y >
    atDelegate(X* target, Y memFunc)
    {
        this->Bind(target, memFunc);
    }

    //PURPOSE
    //  Constructor that binds a free function.
    explicit atDelegate(R (*freeFunc)(DLGT_ARGS(P)))
    {
        this->Bind(freeFunc);
    }
};

template< typename R DLGT_COMMA DLGT_ARGS(typename P) >
class atDelegator< R (DLGT_ARGS(P)) > : public DLGTOR_NAME< R DLGT_COMMA DLGT_ARGS(P) >
{
};

template< typename R DLGT_COMMA DLGT_ARGS(typename P) >
class CLSR_NAME
{
	//PURPOSE
	//  Given a member function extract the type of the object
	//  in which the member function was declared.
	template< typename TT >
	struct TypeFromMember;

	template< typename TT, typename RR DLGT_COMMA DLGT_ARGS(typename PP) >
	struct TypeFromMember< RR (TT::*)(DLGT_ARGS(PP)) >
	{
		typedef TT Type;
	};

	template< typename TT, typename RR DLGT_COMMA DLGT_ARGS(typename PP) >
	struct TypeFromMember< RR (TT::*)(DLGT_ARGS(PP)) const >
	{
		typedef TT Type;
	};

	//PURPOSE
	//  Given a member function determine if its signature
	//  matches the signature of the delegate.
	template<typename TT>
	struct SigMatch
	{
		struct Yes{ char a[1]; };
		struct No{ char a[128]; };

		static No IsMatch(...);
		static Yes IsMatch(R (TT::*)(DLGT_ARGS(P)));
		static Yes IsMatch(R (TT::*)(DLGT_ARGS(P)) const);
	};

public:

	CLSR_NAME()
	{
		this->Reset();
	}

	//PURPOSE
	//  Constructor that binds a member function.
	template< typename X, typename Y >
	CLSR_NAME(X* target, Y memFunc )
	{
		this->Bind(target, memFunc);
	}

	//PURPOSE
	//  Constructor that binds a free function.
	explicit CLSR_NAME(R (*freeFunc)(DLGT_ARGS(P)))
	{
		this->Bind(freeFunc);
	}

	//PURPOSE
	//  Reset the delegate to its default state.
	void Reset()
	{
		m_Target = 0;
		m_Func.m_MemFunc = 0;
		m_Func.m_FreeFunc = 0;
	}

	//PURPOSE
	//  Binds a member function.  Calling operator() will dispatch the
	//  function call on the target.
	template< typename X, typename Y >
	void Bind(X* target, Y memFunc DLGT_COMMA DLGT_PARAMS(P, p))
	{
		//Make sure Y has the signature specified by the template args
		CompileTimeAssert(sizeof(SigMatch<typename TypeFromMember<Y>::Type>::IsMatch(memFunc)) ==
			sizeof(typename SigMatch<typename TypeFromMember<Y>::Type>::Yes));
		this->Reset();
		m_Target = delegate_detail::BindHelper(target,
			(typename TypeFromMember< Y >::Type*) 0,
			memFunc,
			m_Func.m_MemFunc);

		DLGT_ASSIGN(m_p, p);
	}

	//PURPOSE
	//  Binds a free function.  Calling operator() will call the function.
	void Bind(R (*freeFunc)(DLGT_ARGS(P)) DLGT_COMMA DLGT_PARAMS(P, p))
	{
		this->Reset();
		m_Func.m_FreeFunc = freeFunc;

		DLGT_ASSIGN(m_p, p);
	}

	//PURPOSE
	//  Synonym for Reset()
	void Unbind()
	{
		this->Reset();
	}

	//PURPOSE
	//  Calls the bound function.
	R Invoke() const
	{
		FastAssert(this->IsBound());

		return m_Target ? (m_Target->*m_Func.m_MemFunc)(DLGT_ARGS(m_p)) : (*m_Func.m_FreeFunc)(DLGT_ARGS(m_p));
	}

	//PURPOSE
	//  Calls the bound function.
	R operator()() const
	{
		return this->Invoke();
	}

	//PURPOSE
	//  Returns true if the delegate is bound to a function.
	bool IsBound() const
	{
		return 0 != m_Func.m_MemFunc || 0 != m_Func.m_FreeFunc;
	}

	//PURPOSE
	//  Changes the object on which this function is invoked.
	//NOTES
	//	No checks are done to ensure that the function is still callable on the
	//  new object. Use with caution!
	template< typename X >
	void Retarget(X* target)
	{
		m_Target = (delegate_detail::GenericClass*) target;
	}

	bool operator==(const DLGT_NAME< R DLGT_COMMA DLGT_ARGS(P) >& that) const
	{
		return (m_Target == that.m_Target
			&& m_Func.m_MemFunc == that.m_Func.m_MemFunc
			&& m_Func.m_FreeFunc == that.m_Func.m_FreeFunc DLGT_AND DLGT_EQUAL_AND(m_p, that.m_p));
	}

	bool operator!=(const DLGT_NAME< R DLGT_COMMA DLGT_ARGS(P) >& that) const
	{
		return !(this == that);
	}

private:

	delegate_detail::GenericClass* m_Target;
	union
	{
		R (delegate_detail::GenericClass::*m_MemFunc)(DLGT_ARGS(P));
		R (*m_FreeFunc)(DLGT_ARGS(P));
	} m_Func;

	DLGT_MEMBERS(P, m_p);
};

template< typename R DLGT_COMMA DLGT_ARGS(typename P) >
class atClosure< R (DLGT_ARGS(P)) > : public CLSR_NAME< R DLGT_COMMA DLGT_ARGS(P) >
{
public:

	atClosure()
	{
	}

	//PURPOSE
	//  Constructor that binds a member function.
	template< typename X, typename Y >
	atClosure(X* target, Y memFunc DLGT_COMMA DLGT_PARAMS(P, p))
	{
		this->Bind(target, memFunc DLGT_COMMA DLGT_ARGS(p) );
	}

	//PURPOSE
	//  Constructor that binds a free function.
	explicit atClosure(R (*freeFunc)(DLGT_ARGS(P)) DLGT_COMMA DLGT_PARAMS(P, p))
	{
		this->Bind(freeFunc DLGT_COMMA DLGT_ARGS(p));
	}
};

#undef DLGT_COMMA
#undef DLGT_AND
#undef DLGT_NAME
#undef DLGTOR_NAME
#undef DLGT_ARGS
#undef DLGT_PARAMS
#undef DLGT_MEMBERS
#undef DLGT_ASSIGN
#undef DLGT_EQUAL_AND