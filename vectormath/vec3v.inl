
// Note: Everything here must be __forceinline'd, since *this will not be
// passed in vector registers! Force-inlining is okay here, since these functions
// are simple calls to free functions (which are only inline'd, not forced, but
// always pass via vector registers when not inlined).

#if RSG_CPU_INTEL && !defined(__ORBIS__)
#pragma warning(disable: 4714)
#endif // RSG_CPU_INTEL

namespace rage
{
#define DEF_Vec3VConstantInitializer(constant) \
	__forceinline Vec3V::Vec3V(e##constant##Initializer) \
	{ \
		Vec::V4Set( v, Vec::V4VConstant(V_##constant) ); \
	}

	// PLEASE KEEP THESE DEFINED IN THE SAME ORDER AS THE ENUMS IN vectortypes.h !

	DEF_Vec3VConstantInitializer(ZERO                 )
	DEF_Vec3VConstantInitializer(ONE                  )
	DEF_Vec3VConstantInitializer(TWO                  )
	DEF_Vec3VConstantInitializer(THREE                )
	DEF_Vec3VConstantInitializer(FOUR                 )
	DEF_Vec3VConstantInitializer(FIVE                 )
	DEF_Vec3VConstantInitializer(SIX                  )
	DEF_Vec3VConstantInitializer(SEVEN                )
	DEF_Vec3VConstantInitializer(EIGHT                )
	DEF_Vec3VConstantInitializer(NINE                 )
	DEF_Vec3VConstantInitializer(TEN                  )
	DEF_Vec3VConstantInitializer(ELEVEN               )
	DEF_Vec3VConstantInitializer(TWELVE               )
	DEF_Vec3VConstantInitializer(THIRTEEN             )
	DEF_Vec3VConstantInitializer(FOURTEEN             )
	DEF_Vec3VConstantInitializer(FIFTEEN              )
	DEF_Vec3VConstantInitializer(NEGONE               )
	DEF_Vec3VConstantInitializer(NEGTWO               )
	DEF_Vec3VConstantInitializer(NEGTHREE             )
	DEF_Vec3VConstantInitializer(NEGFOUR              )
	DEF_Vec3VConstantInitializer(NEGFIVE              )
	DEF_Vec3VConstantInitializer(NEGSIX               )
	DEF_Vec3VConstantInitializer(NEGSEVEN             )
	DEF_Vec3VConstantInitializer(NEGEIGHT             )
	DEF_Vec3VConstantInitializer(NEGNINE              )
	DEF_Vec3VConstantInitializer(NEGTEN               )
	DEF_Vec3VConstantInitializer(NEGELEVEN            )
	DEF_Vec3VConstantInitializer(NEGTWELVE            )
	DEF_Vec3VConstantInitializer(NEGTHIRTEEN          )
	DEF_Vec3VConstantInitializer(NEGFOURTEEN          )
	DEF_Vec3VConstantInitializer(NEGFIFTEEN           )
	DEF_Vec3VConstantInitializer(NEGSIXTEEN           )

	DEF_Vec3VConstantInitializer(NEG_FLT_MAX          )
	DEF_Vec3VConstantInitializer(FLT_MAX              )
	DEF_Vec3VConstantInitializer(FLT_MIN              )
	DEF_Vec3VConstantInitializer(FLT_LARGE_2          )
	DEF_Vec3VConstantInitializer(FLT_LARGE_4          )
	DEF_Vec3VConstantInitializer(FLT_LARGE_6          )
	DEF_Vec3VConstantInitializer(FLT_LARGE_8          )
	DEF_Vec3VConstantInitializer(FLT_EPSILON          )
	DEF_Vec3VConstantInitializer(FLT_SMALL_6          )
	DEF_Vec3VConstantInitializer(FLT_SMALL_5          )
	DEF_Vec3VConstantInitializer(FLT_SMALL_4          )
	DEF_Vec3VConstantInitializer(FLT_SMALL_3          )
	DEF_Vec3VConstantInitializer(FLT_SMALL_2          )
	DEF_Vec3VConstantInitializer(FLT_SMALL_1          )
	DEF_Vec3VConstantInitializer(FLT_SMALL_12         )
	DEF_Vec3VConstantInitializer(ONE_PLUS_EPSILON     )
	DEF_Vec3VConstantInitializer(ONE_MINUS_FLT_SMALL_3)

	DEF_Vec3VConstantInitializer(ZERO_WONE            )
	DEF_Vec3VConstantInitializer(ONE_WZERO            )

	DEF_Vec3VConstantInitializer(X_AXIS_WONE          )
	DEF_Vec3VConstantInitializer(Y_AXIS_WONE          )
	DEF_Vec3VConstantInitializer(Z_AXIS_WONE          )

	DEF_Vec3VConstantInitializer(X_AXIS_WZERO         )
	DEF_Vec3VConstantInitializer(Y_AXIS_WZERO         )
	DEF_Vec3VConstantInitializer(Z_AXIS_WZERO         )

	DEF_Vec3VConstantInitializer(MASKX                )
	DEF_Vec3VConstantInitializer(MASKY                )
	DEF_Vec3VConstantInitializer(MASKZ                )
	DEF_Vec3VConstantInitializer(MASKW                )
	DEF_Vec3VConstantInitializer(MASKXY               )
	DEF_Vec3VConstantInitializer(MASKXZ               )
	DEF_Vec3VConstantInitializer(MASKXW               )
	DEF_Vec3VConstantInitializer(MASKYZ               )
	DEF_Vec3VConstantInitializer(MASKYW               )
	DEF_Vec3VConstantInitializer(MASKZW               )
	DEF_Vec3VConstantInitializer(MASKYZW              )
	DEF_Vec3VConstantInitializer(MASKXZW              )
	DEF_Vec3VConstantInitializer(MASKXYW              )
	DEF_Vec3VConstantInitializer(MASKXYZ              )
	DEF_Vec3VConstantInitializer(MASKXYZW             )

	DEF_Vec3VConstantInitializer(QUARTER              )
	DEF_Vec3VConstantInitializer(THIRD                )
	DEF_Vec3VConstantInitializer(HALF                 )
	DEF_Vec3VConstantInitializer(NEGHALF              )
	DEF_Vec3VConstantInitializer(INF                  )
	DEF_Vec3VConstantInitializer(NEGINF               )
	DEF_Vec3VConstantInitializer(NAN                  )
	DEF_Vec3VConstantInitializer(LOG2_TO_LOG10        )

	DEF_Vec3VConstantInitializer(ONE_OVER_1024        )
	DEF_Vec3VConstantInitializer(ONE_OVER_PI          )
	DEF_Vec3VConstantInitializer(TWO_OVER_PI          )
	DEF_Vec3VConstantInitializer(PI                   )
	DEF_Vec3VConstantInitializer(TWO_PI               )
	DEF_Vec3VConstantInitializer(PI_OVER_TWO          )
	DEF_Vec3VConstantInitializer(NEG_PI               )
	DEF_Vec3VConstantInitializer(NEG_PI_OVER_TWO      )
	DEF_Vec3VConstantInitializer(TO_DEGREES           )
	DEF_Vec3VConstantInitializer(TO_RADIANS           )
	DEF_Vec3VConstantInitializer(SQRT_TWO		      )
	DEF_Vec3VConstantInitializer(ONE_OVER_SQRT_TWO	  )
	DEF_Vec3VConstantInitializer(SQRT_THREE		      )
	DEF_Vec3VConstantInitializer(E					  )

	DEF_Vec3VConstantInitializer(INT_1                )
	DEF_Vec3VConstantInitializer(INT_2                )
	DEF_Vec3VConstantInitializer(INT_3                )
	DEF_Vec3VConstantInitializer(INT_4                )
	DEF_Vec3VConstantInitializer(INT_5                )
	DEF_Vec3VConstantInitializer(INT_6                )
	DEF_Vec3VConstantInitializer(INT_7                )
	DEF_Vec3VConstantInitializer(INT_8                )
	DEF_Vec3VConstantInitializer(INT_9                )
	DEF_Vec3VConstantInitializer(INT_10               )
	DEF_Vec3VConstantInitializer(INT_11               )
	DEF_Vec3VConstantInitializer(INT_12               )
	DEF_Vec3VConstantInitializer(INT_13               )
	DEF_Vec3VConstantInitializer(INT_14               )
	DEF_Vec3VConstantInitializer(INT_15               )

	DEF_Vec3VConstantInitializer(7FFFFFFF             )
	DEF_Vec3VConstantInitializer(80000000             )

#undef DEF_Vec3VConstantInitializer

	__forceinline Vec3V::Vec3V()
	{
	}

#if __WIN32PC
	__forceinline Vec3V::Vec3V(Vec3V_ConstRef _v)
		: v(_v.v)
	{
	}
#endif

#if !__XENON
	__forceinline Vec3V_ConstRef Vec3V::operator=(Vec3V_ConstRef _v)
	{
		v = _v.v;
		return *this;
	}
#endif

	__forceinline Vec3V::Vec3V(const float& s1, const float& s2, const float& s3)
	{
		Vec::V3Set( v, s1, s2, s3 );
	}

	__forceinline Vec3V::Vec3V(ScalarV_In a, ScalarV_In b, ScalarV_In c)
		: v( Vec::V4MergeXY( Vec::V4MergeXY( a.v, c.v ), b.v ) )
	{
	}

	__forceinline Vec3V::Vec3V(Vec2V_In a, ScalarV_In b)
		: v( Vec::V4PermuteTwo<Vec::X1,Vec::Y1,Vec::X2,Vec::Y2>( a.v, b.v ) )
	{
	}

	__forceinline Vec3V::Vec3V(ScalarV_In a, Vec2V_In b)
		: v( Vec::V4PermuteTwo<Vec::X1,Vec::X2,Vec::Y2,Vec::Z2>( a.v, b.v ) )
	{
	}

#if !UNIQUE_VECTORIZED_TYPE
	__forceinline Vec3V::Vec3V(const Vec::Vector_4V_Persistent& vec)
	{
		v = *(Vec::Vector_4*)(&vec);
	}
#endif // !UNIQUE_VECTORIZED_TYPE

	__forceinline Vec3V::Vec3V(Vec::Vector_4V_In a)
		: v( a )
	{
	}

	__forceinline Vec3V::Vec3V(ScalarV_In a)
		: v( a.v )
	{
	}

	__forceinline Vec3V::Vec3V(VecBoolV_In a)
		: v( a.v )
	{
	}

	__forceinline Vec3V::Vec3V(BoolV_In a)
		: v( a.v )
	{
	}

	__forceinline void Vec3V::SetIntrin128(Vec::Vector_4V_In _v)
	{
		v = _v;
	}

	__forceinline Vec::Vector_4V_Val Vec3V::GetIntrin128() const
	{
		return v;
	}

	__forceinline Vec::Vector_4V_Ref Vec3V::GetIntrin128Ref()
	{
		return v;
	}

	__forceinline Vec::Vector_4V_ConstRef Vec3V::GetIntrin128ConstRef() const
	{
		return v;
	}

	__forceinline VecBoolV_Out Vec3V::AsVecBoolV() const
	{
		return VecBoolV( v );
	}

	__forceinline ScalarV_Out Vec3V::AsScalarV() const
	{
		return ScalarV( v );
	}

	__forceinline ScalarV_Out Vec3V::GetX() const
	{
		return ScalarV( Vec::V4SplatX(v) );
	}

	__forceinline ScalarV_Out Vec3V::GetY() const
	{
		return ScalarV( Vec::V4SplatY(v) );
	}

	__forceinline ScalarV_Out Vec3V::GetZ() const
	{
		return ScalarV( Vec::V4SplatZ(v) );
	}

	__forceinline Vec2V_Out Vec3V::GetXY() const
	{
		return Vec2V( v );
	}

	__forceinline void Vec3V::SetX( ScalarV_In newX )
	{
		v = Vec::V4PermuteTwo<Vec::W1,Vec::Y2,Vec::Z2,Vec::W2>( newX.v, v );		
	}

	__forceinline void Vec3V::SetY( ScalarV_In newY )
	{
#if __PS3
		v = Vec::V4PermuteTwo<Vec::X2,Vec::X1,Vec::Z2,Vec::Y1>( newY.v, v );
#elif __XENON
		v = Vec::V4PermuteTwo<Vec::X2,Vec::Z1,Vec::Z2,Vec::W1>( newY.v, v );
#else
		v = Vec::V4PermuteTwo<Vec::X2,Vec::Y1,Vec::Z2,Vec::W1>( newY.v, v );
#endif
	}

	__forceinline void Vec3V::SetZ( ScalarV_In newZ )
	{
		v = Vec::V4PermuteTwo<Vec::X2,Vec::Y2,Vec::X1,Vec::Y1>( newZ.v, v );
	}

	__forceinline void Vec3V::SetXInMemory( ScalarV_In newX )
	{
		Vec::SetXInMemory( v, newX.GetIntrin128() );
	}

	__forceinline void Vec3V::SetYInMemory( ScalarV_In newY )
	{
		Vec::SetYInMemory( v, newY.GetIntrin128() );
	}

	__forceinline void Vec3V::SetZInMemory( ScalarV_In newZ )
	{
		Vec::SetZInMemory( v, newZ.GetIntrin128() );
	}

	__forceinline void Vec3V::SetWInMemory( ScalarV_In newW )
	{
		Vec::SetWInMemory( v, newW.GetIntrin128() );
	}

	__forceinline void Vec3V::SetX( const float& floatVal )
	{
		Vec::SetX( v, floatVal );
	}

	__forceinline void Vec3V::SetY( const float& floatVal )
	{
		Vec::SetY( v, floatVal );
	}

	__forceinline void Vec3V::SetZ( const float& floatVal )
	{
		Vec::SetZ( v, floatVal );
	}

	__forceinline float Vec3V::GetXf() const
	{
		return Vec::GetX( v );
	}

	__forceinline float Vec3V::GetYf() const
	{
		return Vec::GetY( v );
	}

	__forceinline float Vec3V::GetZf() const
	{
		return Vec::GetZ( v );
	}

	__forceinline int Vec3V::GetXi() const
	{
		return Vec::GetXi( v );
	}

	__forceinline int Vec3V::GetYi() const
	{
		return Vec::GetYi( v );
	}

	__forceinline int Vec3V::GetZi() const
	{
		return Vec::GetZi( v );
	}

	__forceinline void Vec3V::SetElemf( unsigned elem, float fVal )
	{
		VecAssertMsg( elem <= 2 , "Invalid elem!" );
		(*this)[elem] = fVal;
	}

	__forceinline float Vec3V::GetElemf( unsigned elem ) const
	{
		VecAssertMsg( elem <= 2 , "Invalid elem!" );
		return (*this)[elem];
	}

	__forceinline void Vec3V::SetElemi( unsigned elem, int intVal )
	{
		VecAssertMsg( elem <= 2 , "Invalid elem!" );
		((int*)(this))[elem] = intVal;
	}

	__forceinline int Vec3V::GetElemi( unsigned elem ) const
	{
		VecAssertMsg( elem <= 2 , "Invalid elem!" );
		return ((const int*)(this))[elem];
	}

	__forceinline void Vec3V::SetXf( float floatVal )
	{
		Vec::SetXInMemory( v, floatVal );
	}

	__forceinline void Vec3V::SetYf( float floatVal )
	{
		Vec::SetYInMemory( v, floatVal );
	}

	__forceinline void Vec3V::SetZf( float floatVal )
	{
		Vec::SetZInMemory( v, floatVal );
	}

	__forceinline void Vec3V::SetXi( int intVal )
	{
		Vec::SetXInMemory( v, intVal );
	}

	__forceinline void Vec3V::SetYi( int intVal )
	{
		Vec::SetYInMemory( v, intVal );
	}

	__forceinline void Vec3V::SetZi( int intVal )
	{
		Vec::SetZInMemory( v, intVal );
	}

	__forceinline void Vec3V::ZeroComponents()
	{
		Vec::V4ZeroComponents( v );
	}

	__forceinline ScalarV_Out Vec3V::GetW() const
	{
		return ScalarV( Vec::V4SplatW( v ) );
	}

	__forceinline void Vec3V::SetW( ScalarV_In newW )
	{
		// 1 instruction on Xenon.
		// 2 dependent instructions on PS3 (if v4permtwo2instructions_ps3.inl is #included in v4vector4vcore_ps3.inl).
		v = Vec::V4PermuteTwo<Vec::X2,Vec::Y2,Vec::Z2,Vec::X1>( newW.v, v );
	}

	__forceinline float Vec3V::GetWf() const
	{
		return Vec::GetW( v );
	}

	__forceinline int Vec3V::GetWi() const
	{
		return Vec::GetWi( v );
	}

	__forceinline void Vec3V::SetWf( float floatVal )
	{
		Vec::SetWInMemory( v, floatVal );
	}

	__forceinline void Vec3V::SetWi( int intVal )
	{
		Vec::SetWInMemory( v, intVal );
	}

	__forceinline void Vec3V::SetW( const float& floatVal )
	{
		Vec::SetW( v, floatVal );
	}

	__forceinline void Vec3V::SetWZero()
	{
		Vec::V4SetWZero( v );
	}

	//============================================================================
	// Operators

	__forceinline VecBoolV_Out Vec3V::operator== (Vec3V_In b) const
	{
		return VecBoolV( Vec::V4IsEqualV( v, b.v ) );
	}

	__forceinline VecBoolV_Out Vec3V::operator!= (Vec3V_In b) const
	{
		return VecBoolV( Vec::V4InvertBits( Vec::V4IsEqualV( v, b.v ) ) );
	}

	__forceinline VecBoolV_Out	Vec3V::operator< (Vec3V_In bigVector) const
	{
		return VecBoolV( Vec::V4IsLessThanV( v, bigVector.v ) );
	}

	__forceinline VecBoolV_Out	Vec3V::operator<= (Vec3V_In bigVector) const
	{
		return VecBoolV( Vec::V4IsLessThanOrEqualV( v, bigVector.v ) );
	}

	__forceinline VecBoolV_Out	Vec3V::operator> (Vec3V_In smallVector) const
	{
		return VecBoolV( Vec::V4IsGreaterThanV( v, smallVector.v ) );
	}

	__forceinline VecBoolV_Out	Vec3V::operator>= (Vec3V_In smallVector) const
	{
		return VecBoolV( Vec::V4IsGreaterThanOrEqualV( v, smallVector.v ) );
	}

	__forceinline Vec3V_Out Vec3V::operator* (Vec3V_In b) const
	{
		return Vec3V( Vec::V4Scale( v, b.v ) );
	}

	__forceinline Vec3V_Out Vec3V::operator* (ScalarV_In b) const
	{
		return Vec3V( Vec::V4Scale( v, b.v ) );
	}

	__forceinline Vec3V_Out operator* (ScalarV_In a, Vec3V_In b)
	{
		return Vec3V( Vec::V4Scale( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline Vec3V_Out Vec3V::operator/ (Vec3V_In b) const
	{
		return Vec3V( Vec::V4InvScale( v, b.v ) );
	}

	__forceinline Vec3V_Out Vec3V::operator/ (ScalarV_In b) const
	{
		return Vec3V( Vec::V4InvScale( v, b.v ) );
	}

	__forceinline Vec3V_Out Vec3V::operator+ (Vec3V_In b) const
	{
		return Vec3V( Vec::V4Add( v, b.v ) );
	}

	__forceinline Vec3V_Out Vec3V::operator- (Vec3V_In b) const
	{
		return Vec3V( Vec::V4Subtract( v, b.v ) );
	}

	__forceinline void Vec3V::operator*= (Vec3V_In b)
	{
		v = Vec::V4Scale( v, b.v );
	}

	__forceinline void Vec3V::operator*= (ScalarV_In b)
	{
		v = Vec::V4Scale( v, b.v );
	}

	__forceinline void Vec3V::operator/= (Vec3V_In b)
	{
		v = Vec::V4InvScale( v, b.v );
	}

	__forceinline void Vec3V::operator/= (ScalarV_In b)
	{
		v = Vec::V4InvScale( v, b.v );
	}

	__forceinline void Vec3V::operator+= (Vec3V_In b)
	{
		v = Vec::V4Add( v, b.v );
	}

	__forceinline void Vec3V::operator-= (Vec3V_In b)
	{
		v = Vec::V4Subtract( v, b.v );
	}

	__forceinline Vec3V_Out Vec3V::operator+ () const
	{
		return Vec3V( v );
	}

	__forceinline Vec3V_Out Vec3V::operator- () const
	{
		return Vec3V( Vec::V4Negate( v ) );
	}

	__forceinline Vec3V_Out Vec3V::operator| (Vec3V_In b) const
	{
		return Vec3V( Vec::V4Or( v, b.v ) );
	}

	__forceinline Vec3V_Out Vec3V::operator& (Vec3V_In b) const
	{
		return Vec3V( Vec::V4And( v, b.v ) );
	}

	__forceinline Vec3V_Out Vec3V::operator^ (Vec3V_In b) const
	{
		return Vec3V( Vec::V4Xor( v, b.v ) );
	}

	__forceinline void Vec3V::operator|= (Vec3V_In b)
	{
		v = Vec::V4Or( v, b.v );
	}

	__forceinline void Vec3V::operator&= (Vec3V_In b)
	{
		v = Vec::V4And( v, b.v );
	}

	__forceinline void Vec3V::operator^= (Vec3V_In b)
	{
		v = Vec::V4Xor( v, b.v );
	}

	__forceinline Vec3V_Out Vec3V::operator~ () const
	{
		return Vec3V( Vec::V4InvertBits( v ) );
	}

	__forceinline const float& Vec3V::operator[] (unsigned elem) const
	{
		VecAssertMsg( elem <= 2 , "Invalid element index." );
		return Vec::GetElemRef( &v, elem );
	}

	__forceinline float& Vec3V::operator[] (unsigned elem)
	{
		VecAssertMsg( elem <= 2 , "Invalid element index." );
		return Vec::GetElemRef( &v, elem );
	}

	template <u32 permX, u32 permY>
	__forceinline Vec2V_Out Vec3V::Get() const
	{
		if( permX == Vec::X && permY == Vec::X )
		{
			return Vec2V( Vec::V4SplatX( v ) );
		}
		else if( permX == Vec::X && permY == Vec::Y )
		{
			return Vec2V( v );
		}
		else if( permX == Vec::Y && permY == Vec::Y )
		{
			return Vec2V( Vec::V4SplatY( v ) );
		}
		else if( permX == Vec::Z && permY == Vec::Z )
		{
			return Vec2V( Vec::V4SplatZ( v ) );
		}
		// This next specialization happens to be faster on PS3.
		else if( permX == Vec::Y && permY == Vec::Z )
		{
			return Vec2V( Vec::V4Permute<Vec::Y,Vec::Z,Vec::W,Vec::X>( v ) );
		}
		// Default.
		else
		{
			return Vec2V( Vec::V4Permute<permX, permY, Vec::W, Vec::W>( v ) );
		}
	}

	template <u32 permX, u32 permY, u32 permZ>
	__forceinline Vec3V_Out Vec3V::Get() const
	{
		CompileTimeAssert( !(permX == Vec::X && permY == Vec::Y && permZ == Vec::Z) );
		// Useless permute!

		if( permX == Vec::X && permY == Vec::X && permZ == Vec::X )
		{
			return Vec3V( Vec::V4SplatX( v ) );
		}
		else if( permX == Vec::X && permY == Vec::X && permZ == Vec::Y )
		{
			return Vec3V( Vec::V4MergeXY( v, v ) );
		}
		else if( permX == Vec::Y && permY == Vec::Y && permZ == Vec::Y )
		{
			return Vec3V( Vec::V4SplatY( v ) );
		}
		else if( permX == Vec::Z && permY == Vec::Z && permZ == Vec::Z )
		{
			return Vec3V( Vec::V4SplatZ( v ) );
		}
		// These next few specializations happen to be faster on PS3.
		else if( permX == Vec::Y && permY == Vec::Z && permZ == Vec::W )
		{
			return Vec3V( Vec::V4Permute<Vec::Y,Vec::Z,Vec::W,Vec::X>( v ) );
		}
		else if( permX == Vec::Z && permY == Vec::W && permZ == Vec::X )
		{
			return Vec3V( Vec::V4Permute<Vec::Z,Vec::W,Vec::X,Vec::Y>( v ) );
		}
		else if( permX == Vec::W && permY == Vec::X && permZ == Vec::Y )
		{
			return Vec3V( Vec::V4Permute<Vec::W,Vec::X,Vec::Y,Vec::Z>( v ) );
		}
		// Default.
		else
		{
			return Vec3V( Vec::V4Permute<permX, permY, permZ, Vec::W>( v ) );
		}
	}

	template <u32 permX, u32 permY>
	__forceinline void Vec3V::Set(Vec2V_In b)
	{
		CompileTimeAssert( permX == Vec::X || permX == Vec::Y || permX == Vec::Z );
		CompileTimeAssert( permY == Vec::X || permY == Vec::Y || permY == Vec::Z );
		// Invalid permute!

		CompileTimeAssert( permX != permY );
		// Can't use two of the same components in the swizzle!

		if ( permX == Vec::X && permY == Vec::Y )
		{
			v = Vec::V4PermuteTwo<Vec::X1, Vec::Y1, Vec::Z2, Vec::W2>( b.GetIntrin128(), v );
		}
		else if ( permX == Vec::X && permY == Vec::Z )
		{
			v = Vec::V4PermuteTwo<Vec::X1, Vec::Y2, Vec::Y1, Vec::W2>( b.GetIntrin128(), v );
		}
		else if ( permX == Vec::Y && permY == Vec::X )
		{
			v = Vec::V4PermuteTwo<Vec::Y1, Vec::X1, Vec::Z2, Vec::W2>( b.GetIntrin128(), v );
		}
		else if ( permX == Vec::Y && permY == Vec::Z )
		{
			v = Vec::V4PermuteTwo<Vec::X2, Vec::X1, Vec::Y1, Vec::W2>( b.GetIntrin128(), v );
		}
		else if ( permX == Vec::Z && permY == Vec::X )
		{
			v = Vec::V4PermuteTwo<Vec::Y1, Vec::Y2, Vec::X1, Vec::W2>( b.GetIntrin128(), v );
		}
		else if ( permX == Vec::Z && permY == Vec::Y )
		{
			v = Vec::V4PermuteTwo<Vec::X2, Vec::Y1, Vec::X1, Vec::W2>( b.GetIntrin128(), v );
		}
		else
		{
			VecAssertMsg(0, "invalid permute");
		}
	}

	template <u32 permX, u32 permY, u32 permZ>
	__forceinline void Vec3V::Set(Vec3V_In b)
	{
		CompileTimeAssert( permX == Vec::X || permX == Vec::Y || permX == Vec::Z );
		CompileTimeAssert( permY == Vec::X || permY == Vec::Y || permY == Vec::Z );
		CompileTimeAssert( permZ == Vec::X || permZ == Vec::Y || permZ == Vec::Z );
		// Invalid permute!

		CompileTimeAssert( !(permX == Vec::X && permY == Vec::Y && permZ == Vec::Z) );
		// Useless permute!

		CompileTimeAssert( permX != permY && permY != permZ && permZ != permX );
		// Can't use two of the same components in the swizzle!

		v = Vec::V4Permute<permX, permY, permZ, Vec::W>( b.GetIntrin128() );
	}

	namespace sysEndian
	{
		template<> inline void SwapMe(Vec3V& v) { return SwapMe(v.GetIntrin128Ref()); }
	} // namespace sysEndian


} // namespace rage
