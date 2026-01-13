// 
// atl/atfunctortemplate.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

//#ifdef guards purposely left out

// RAGE_QA: NO_INCLUDE_GUARD

//#ifndef ATL_ATFUNCTORTEMPLATE_H
//#define ATL_ATFUNCTORTEMPLATE_H

#ifdef AT_COMMA
#undef AT_COMMA
#endif

#if AT_PARAM_COUNT > 0
#define AT_COMMA   ,
#else
#define AT_COMMA
#endif

#define AT_FUNCTOR_NAME     AT_PASTE( atFunctor, AT_PARAM_COUNT )

#ifdef AT_FUNCTOR_ARGS
#undef AT_FUNCTOR_ARGS
#undef AT_FUNCTOR_PARAMS
#endif

#define AT_FUNCTOR_ARGS( A )        AT_ARGS( AT_PARAM_COUNT, A )
#define AT_FUNCTOR_PARAMS( P, p )   AT_PARAMS( AT_PARAM_COUNT, P, p )

namespace rage
{

template< typename R AT_COMMA AT_FUNCTOR_ARGS( typename P ) >
class AT_FUNCTOR_NAME : public atDelegate< R ( AT_FUNCTOR_ARGS( P ) ) >
{
    typedef atDelegate< R ( AT_FUNCTOR_ARGS( P ) ) > BASE;

public:

    //PURPOSE
    //  Returns true if the functor is callable.
    bool IsValid() const
    {
        return this->BASE::IsBound();
    }

    //PURPOSE
    //  Invalidates the functor.
    void Clear()
    {
        this->BASE::Reset();
    }

    //PURPOSE
    //  Equals operator.
    bool operator==( const AT_FUNCTOR_NAME< R AT_COMMA AT_FUNCTOR_ARGS( P ) >& rhs ) const
    {
        return this->BASE::operator==( rhs );
    }

    //PURPOSE
    //  Initializes a functor bound to an object of type T that calls
    //  method M.
    template< typename T, R ( T::*M )( AT_FUNCTOR_ARGS( P ) ) > 
    const AT_FUNCTOR_NAME& Reset( T* t )
    {
        this->Bind( t, M );
		return *this;
    }

    //PURPOSE
    //  Initializes a functor bound to an object of type T that calls
    //  const method M.
    //NOTES
    //  Unfortunately .NET 7.1.3088 can't handle an overloaded function
    //  where the template parameters differ (const method vs. non-const
    //  method).  When this is fixed ResetC() can be renamed Reset().
    template< typename T, R ( T::*M )( AT_FUNCTOR_ARGS( P ) ) const > 
    const AT_FUNCTOR_NAME& ResetC( const T* t )
    {
        this->Bind( t, M );
		return *this;
    }

    //PURPOSE
    //  Initializes a functor that calls free function F.
    template< R ( *F )( AT_FUNCTOR_ARGS( P ) ) > 
    const AT_FUNCTOR_NAME& Reset()
    {
        this->Bind( F );
		return *this;
    }

    //PURPOSE
    //  Added for compatibility with original Functor class.
    //NOTES
	//  In order to use this, your return value must have be default constructable.
	//SEE ALSO
	//  NullFunction
    static AT_FUNCTOR_NAME< R AT_COMMA AT_FUNCTOR_ARGS( P ) > NullFunctor()
	{
		AT_FUNCTOR_NAME<  R AT_COMMA AT_FUNCTOR_ARGS( P ) > f;

		f.Reset< &AT_FUNCTOR_NAME<  R AT_COMMA AT_FUNCTOR_ARGS( P ) >::NullFunction >();

		return f;
	}

private:

    //PURPOSE
    //  Added for compatibility with original Functor class.
	//NOTES
	//  In order to use this, your return value must have be default constructable.
	//SEE ALSO
	//  NullFunctor
    static R NullFunction( AT_FUNCTOR_ARGS( P ) ) { return R(); }
};

}   //namespace rage

//#ifdef guards purposely left out
//#endif  //ATL_FUNCTORTEMPLATE_H

