
// Note: Everything here must be __forceinline'd, since *this will not be
// passed in vector registers! Force-inlining is okay here, since these functions
// are simple calls to free functions (which are only inline'd, not forced, but
// always pass via vector registers when not inlined).

namespace rage
{
#define DEF_Vec4VConstantInitializer(constant) \
	__forceinline Vec4V::Vec4V(e##constant##Initializer) \
	{ \
		Vec::V4Set( v, Vec::V4VConstant(V_##constant) ); \
	}

	// PLEASE KEEP THESE DEFINED IN THE SAME ORDER AS THE ENUMS IN vectortypes.h !

	DEF_Vec4VConstantInitializer(ZERO                 )
	DEF_Vec4VConstantInitializer(ONE                  )
	DEF_Vec4VConstantInitializer(TWO                  )
	DEF_Vec4VConstantInitializer(THREE                )
	DEF_Vec4VConstantInitializer(FOUR                 )
	DEF_Vec4VConstantInitializer(FIVE                 )
	DEF_Vec4VConstantInitializer(SIX                  )
	DEF_Vec4VConstantInitializer(SEVEN                )
	DEF_Vec4VConstantInitializer(EIGHT                )
	DEF_Vec4VConstantInitializer(NINE                 )
	DEF_Vec4VConstantInitializer(TEN                  )
	DEF_Vec4VConstantInitializer(ELEVEN               )
	DEF_Vec4VConstantInitializer(TWELVE               )
	DEF_Vec4VConstantInitializer(THIRTEEN             )
	DEF_Vec4VConstantInitializer(FOURTEEN             )
	DEF_Vec4VConstantInitializer(FIFTEEN              )
	DEF_Vec4VConstantInitializer(NEGONE               )
	DEF_Vec4VConstantInitializer(NEGTWO               )
	DEF_Vec4VConstantInitializer(NEGTHREE             )
	DEF_Vec4VConstantInitializer(NEGFOUR              )
	DEF_Vec4VConstantInitializer(NEGFIVE              )
	DEF_Vec4VConstantInitializer(NEGSIX               )
	DEF_Vec4VConstantInitializer(NEGSEVEN             )
	DEF_Vec4VConstantInitializer(NEGEIGHT             )
	DEF_Vec4VConstantInitializer(NEGNINE              )
	DEF_Vec4VConstantInitializer(NEGTEN               )
	DEF_Vec4VConstantInitializer(NEGELEVEN            )
	DEF_Vec4VConstantInitializer(NEGTWELVE            )
	DEF_Vec4VConstantInitializer(NEGTHIRTEEN          )
	DEF_Vec4VConstantInitializer(NEGFOURTEEN          )
	DEF_Vec4VConstantInitializer(NEGFIFTEEN           )
	DEF_Vec4VConstantInitializer(NEGSIXTEEN           )

	DEF_Vec4VConstantInitializer(NEG_FLT_MAX          )
	DEF_Vec4VConstantInitializer(FLT_MAX              )
	DEF_Vec4VConstantInitializer(FLT_MIN              )
	DEF_Vec4VConstantInitializer(FLT_LARGE_2          )
	DEF_Vec4VConstantInitializer(FLT_LARGE_4          )
	DEF_Vec4VConstantInitializer(FLT_LARGE_6          )
	DEF_Vec4VConstantInitializer(FLT_LARGE_8          )
	DEF_Vec4VConstantInitializer(FLT_EPSILON          )
	DEF_Vec4VConstantInitializer(FLT_SMALL_6          )
	DEF_Vec4VConstantInitializer(FLT_SMALL_5          )
	DEF_Vec4VConstantInitializer(FLT_SMALL_4          )
	DEF_Vec4VConstantInitializer(FLT_SMALL_3          )
	DEF_Vec4VConstantInitializer(FLT_SMALL_2          )
	DEF_Vec4VConstantInitializer(FLT_SMALL_1          )
	DEF_Vec4VConstantInitializer(FLT_SMALL_12         )
	DEF_Vec4VConstantInitializer(ONE_PLUS_EPSILON     )
	DEF_Vec4VConstantInitializer(ONE_MINUS_FLT_SMALL_3)

	DEF_Vec4VConstantInitializer(ZERO_WONE            )
	DEF_Vec4VConstantInitializer(ONE_WZERO            )

	DEF_Vec4VConstantInitializer(X_AXIS_WONE          )
	DEF_Vec4VConstantInitializer(Y_AXIS_WONE          )
	DEF_Vec4VConstantInitializer(Z_AXIS_WONE          )

	DEF_Vec4VConstantInitializer(X_AXIS_WZERO         )
	DEF_Vec4VConstantInitializer(Y_AXIS_WZERO         )
	DEF_Vec4VConstantInitializer(Z_AXIS_WZERO         )

	DEF_Vec4VConstantInitializer(MASKX                )
	DEF_Vec4VConstantInitializer(MASKY                )
	DEF_Vec4VConstantInitializer(MASKZ                )
	DEF_Vec4VConstantInitializer(MASKW                )
	DEF_Vec4VConstantInitializer(MASKXY               )
	DEF_Vec4VConstantInitializer(MASKXZ               )
	DEF_Vec4VConstantInitializer(MASKXW               )
	DEF_Vec4VConstantInitializer(MASKYZ               )
	DEF_Vec4VConstantInitializer(MASKYW               )
	DEF_Vec4VConstantInitializer(MASKZW               )
	DEF_Vec4VConstantInitializer(MASKYZW              )
	DEF_Vec4VConstantInitializer(MASKXZW              )
	DEF_Vec4VConstantInitializer(MASKXYW              )
	DEF_Vec4VConstantInitializer(MASKXYZ              )
	DEF_Vec4VConstantInitializer(MASKXYZW             )

	DEF_Vec4VConstantInitializer(QUARTER              )
	DEF_Vec4VConstantInitializer(THIRD                )
	DEF_Vec4VConstantInitializer(HALF                 )
	DEF_Vec4VConstantInitializer(NEGHALF              )
	DEF_Vec4VConstantInitializer(INF                  )
	DEF_Vec4VConstantInitializer(NEGINF               )
	DEF_Vec4VConstantInitializer(NAN                  )
	DEF_Vec4VConstantInitializer(LOG2_TO_LOG10        )

	DEF_Vec4VConstantInitializer(ONE_OVER_1024        )
	DEF_Vec4VConstantInitializer(ONE_OVER_PI          )
	DEF_Vec4VConstantInitializer(TWO_OVER_PI          )
	DEF_Vec4VConstantInitializer(PI                   )
	DEF_Vec4VConstantInitializer(TWO_PI               )
	DEF_Vec4VConstantInitializer(PI_OVER_TWO          )
	DEF_Vec4VConstantInitializer(NEG_PI               )
	DEF_Vec4VConstantInitializer(NEG_PI_OVER_TWO      )
	DEF_Vec4VConstantInitializer(TO_DEGREES           )
	DEF_Vec4VConstantInitializer(TO_RADIANS           )
	DEF_Vec4VConstantInitializer(SQRT_TWO		      )
	DEF_Vec4VConstantInitializer(ONE_OVER_SQRT_TWO	  )
	DEF_Vec4VConstantInitializer(SQRT_THREE		      )
	DEF_Vec4VConstantInitializer(E					  )

	DEF_Vec4VConstantInitializer(INT_1                )
	DEF_Vec4VConstantInitializer(INT_2                )
	DEF_Vec4VConstantInitializer(INT_3                )
	DEF_Vec4VConstantInitializer(INT_4                )
	DEF_Vec4VConstantInitializer(INT_5                )
	DEF_Vec4VConstantInitializer(INT_6                )
	DEF_Vec4VConstantInitializer(INT_7                )
	DEF_Vec4VConstantInitializer(INT_8                )
	DEF_Vec4VConstantInitializer(INT_9                )
	DEF_Vec4VConstantInitializer(INT_10               )
	DEF_Vec4VConstantInitializer(INT_11               )
	DEF_Vec4VConstantInitializer(INT_12               )
	DEF_Vec4VConstantInitializer(INT_13               )
	DEF_Vec4VConstantInitializer(INT_14               )
	DEF_Vec4VConstantInitializer(INT_15               )

	DEF_Vec4VConstantInitializer(7FFFFFFF             )
	DEF_Vec4VConstantInitializer(80000000             )

#undef DEF_Vec4VConstantInitializer

	__forceinline Vec4V::Vec4V()
	{
	}

#if __WIN32PC
	__forceinline Vec4V::Vec4V(Vec4V_ConstRef _v)
		: v(_v.v)
	{
	}
#endif

#if !__XENON
	__forceinline Vec4V_ConstRef Vec4V::operator=(Vec4V_ConstRef _v)
	{
		v = _v.v;
		return *this;
	}
#endif

	__forceinline Vec4V::Vec4V(const float& s1, const float& s2, const float& s3, const float& s4)
	{
		Vec::V4Set( v, s1, s2, s3, s4 );
	}

#if !UNIQUE_VECTORIZED_TYPE
	__forceinline Vec4V::Vec4V(const Vec::Vector_4V_Persistent& vec)
	{
		v = *(Vec::Vector_4*)(&vec);
	}
#endif // !UNIQUE_VECTORIZED_TYPE

	__forceinline Vec4V::Vec4V(Vec::Vector_4V_In a)
		: v( a )
	{
	}

	__forceinline Vec4V::Vec4V(VecBoolV_In a)
		: v( a.GetIntrin128() )
	{
	}

	__forceinline Vec4V::Vec4V(BoolV_In a)
		: v( a.GetIntrin128() )
	{
	}

	__forceinline Vec4V::Vec4V(ScalarV_In a)
		: v( a.GetIntrin128() )
	{
	}

	__forceinline Vec4V::Vec4V(ScalarV_In a, ScalarV_In b, ScalarV_In c, ScalarV_In d)
		: v(Vec::V4MergeXY( Vec::V4MergeXY( a.GetIntrin128(), c.GetIntrin128() ), Vec::V4MergeXY( b.GetIntrin128(), d.GetIntrin128() )))
	{
	}

	__forceinline Vec4V::Vec4V(ScalarV_In a, ScalarV_In b, Vec2V_In c)
		: v( Vec::V4PermuteTwo<Vec::Z1,Vec::W1,Vec::X2,Vec::Y2>( Vec::V4PermuteTwo<Vec::Y1,Vec::Z1,Vec::W1,Vec::X2>( a.GetIntrin128(), b.GetIntrin128() ), c.GetIntrin128() ) )
	{
	}

	__forceinline Vec4V::Vec4V(ScalarV_In a, Vec2V_In b, ScalarV_In c)
		: v( Vec::V4PermuteTwo<Vec::Y1,Vec::Z1,Vec::W1,Vec::X2>( Vec::V4PermuteTwo<Vec::Z1,Vec::W1,Vec::X2,Vec::Y2>( a.GetIntrin128(), b.GetIntrin128() ), c.GetIntrin128() ) )
	{
	}

	__forceinline Vec4V::Vec4V(Vec2V_In a, ScalarV_In b, ScalarV_In c)
		: v( Vec::V4PermuteTwo<Vec::Z1,Vec::W1,Vec::X2,Vec::Y2>( Vec::V4PermuteTwo<Vec::Z1,Vec::W1,Vec::X2,Vec::Y2>( a.GetIntrin128(), a.GetIntrin128() ), Vec::V4MergeXY( b.GetIntrin128(), c.GetIntrin128() ) ) )
	{
	}

	__forceinline Vec4V::Vec4V(ScalarV_In a, Vec3V_In b)
		: v( Vec::V4PermuteTwo<Vec::W1,Vec::X2,Vec::Y2,Vec::Z2>( a.GetIntrin128(), b.GetIntrin128() ) )
	{
	}

	__forceinline Vec4V::Vec4V(Vec2V_In a, Vec2V_In b)
		: v( Vec::V4PermuteTwo<Vec::X1, Vec::Y1, Vec::X2, Vec::Y2>( a.GetIntrin128(), b.GetIntrin128() ) )
	{
	}

	__forceinline Vec4V::Vec4V(Vec3V_In a, ScalarV_In b)
		: v( Vec::V4PermuteTwo<Vec::X1, Vec::Y1, Vec::Z1, Vec::X2>( a.GetIntrin128(), b.GetIntrin128() ) )
	{
	}

	__forceinline Vec4V::Vec4V(Vec3V_In a)
		: v( a.GetIntrin128() )
	{
	}

	__forceinline Vec4V::Vec4V(Vec2V_In a)
		: v( a.GetIntrin128() )
	{
	}

	__forceinline Vec4V::Vec4V(QuatV_In q)
		: v( q.GetIntrin128() )
	{
	}

//#if !UNIQUE_VECTORIZED_TYPE
//	__forceinline Vec4V::Vec4V(const Vector4& _v)
//	{
//		v.f.x = _v.x;
//		v.f.y = _v.y;
//		v.f.z = _v.z;
//		v.f.w = _v.w;
//	}
//	__forceinline Vec4V::Vec4V(Vec::Vector_4V_Legacy _v)
//	{
//		union
//		{
//			Vec::Vector_4V_Legacy	_vec;
//			float					_flt[4];
//		} Temp;
//		Temp._vec = _v;
//		v.f.x = Temp._flt[0];
//		v.f.y = Temp._flt[1];
//		v.f.z = Temp._flt[2];
//		v.f.w = Temp._flt[3];
//	}
//#endif // !UNIQUE_VECTORIZED_TYPE

	__forceinline void Vec4V::SetIntrin128(Vec::Vector_4V_In _v)
	{
		v = _v;
	}

	__forceinline Vec::Vector_4V_Val Vec4V::GetIntrin128() const
	{
		return v;
	}

	__forceinline Vec::Vector_4V_Ref Vec4V::GetIntrin128Ref()
	{
		return v;
	}

	__forceinline Vec::Vector_4V_ConstRef Vec4V::GetIntrin128ConstRef() const
	{
		return v;
	}

	__forceinline VecBoolV_Out Vec4V::AsVecBoolV() const
	{
		return VecBoolV( v );
	}

	__forceinline ScalarV_Out Vec4V::AsScalarV() const
	{
		return ScalarV( v );
	}

	__forceinline ScalarV_Out Vec4V::GetX() const
	{
		return ScalarV( Vec::V4SplatX(v) );
	}

	__forceinline ScalarV_Out Vec4V::GetY() const
	{
		return ScalarV( Vec::V4SplatY( v ) );
	}

	__forceinline ScalarV_Out Vec4V::GetZ() const
	{
		return ScalarV( Vec::V4SplatZ( v ) );
	}

	__forceinline ScalarV_Out Vec4V::GetW() const
	{
		return ScalarV( Vec::V4SplatW( v ) );
	}

	__forceinline Vec2V_Out Vec4V::GetXY() const
	{
		return Vec2V( v );
	}

	__forceinline Vec2V_Out Vec4V::GetZW() const
	{
		return Get<Vec::Z,Vec::W>();
	}

	__forceinline Vec3V_Out Vec4V::GetXYZ() const
	{
		return Vec3V( v );
	}

	__forceinline void Vec4V::SetX( ScalarV_In newX )
	{
		v = Vec::V4PermuteTwo<Vec::W1,Vec::Y2,Vec::Z2,Vec::W2>( newX.GetIntrin128(), v );
	}

	__forceinline void Vec4V::SetY( ScalarV_In newY )
	{
		// Equivalent if specializations aren't used, optimal for each platform if specializations are used.
#if __PS3
		v = Vec::V4PermuteTwo<Vec::X2,Vec::X1,Vec::Z2,Vec::W2>( newY.GetIntrin128(), v ); // Specialization.
#elif __XENON
		v = Vec::V4PermuteTwo<Vec::X2,Vec::Z1,Vec::Z2,Vec::W2>( newY.GetIntrin128(), v ); // Specialization.
#else
		v = Vec::V4PermuteTwo<Vec::X2,Vec::Y1,Vec::Z2,Vec::W2>( newY.GetIntrin128(), v ); // Whatever.
#endif
	}

	__forceinline void Vec4V::SetZ( ScalarV_In newZ )
	{
		v = Vec::V4PermuteTwo<Vec::X2,Vec::Y2,Vec::X1,Vec::W2>( newZ.GetIntrin128(), v );
	}

	__forceinline void Vec4V::SetW( ScalarV_In newW )
	{
		v = Vec::V4PermuteTwo<Vec::X2,Vec::Y2,Vec::Z2,Vec::X1>( newW.GetIntrin128(), v );
	}

	__forceinline void Vec4V::SetXYZ( Vec3V_In newXYZ )
	{
		v = Vec::V4PermuteTwo<Vec::X1, Vec::Y1, Vec::Z1, Vec::W2>( newXYZ.GetIntrin128(), v );
	}

	__forceinline void Vec4V::SetXInMemory( ScalarV_In newX )
	{
		Vec::SetXInMemory( v, newX.GetIntrin128() );
	}

	__forceinline void Vec4V::SetYInMemory( ScalarV_In newY )
	{
		Vec::SetYInMemory( v, newY.GetIntrin128() );
	}

	__forceinline void Vec4V::SetZInMemory( ScalarV_In newZ )
	{
		Vec::SetZInMemory( v, newZ.GetIntrin128() );
	}

	__forceinline void Vec4V::SetWInMemory( ScalarV_In newW )
	{
		Vec::SetWInMemory( v, newW.GetIntrin128() );
	}

	__forceinline void Vec4V::SetElemf( unsigned elem, float fVal )
	{
		VecAssertMsg( elem <= 3 , "Invalid elem!" );
		(*this)[elem] = fVal;
	}

	__forceinline float Vec4V::GetElemf( unsigned elem ) const
	{
		VecAssertMsg( elem <= 3 , "Invalid elem!" );
		return (*this)[elem];
	}

	__forceinline void Vec4V::SetElemi( unsigned elem, int intVal )
	{
		VecAssertMsg( elem <= 3 , "Invalid elem!" );
		((int*)(this))[elem] = intVal;
	}

	__forceinline int Vec4V::GetElemi( unsigned elem ) const
	{
		VecAssertMsg( elem <= 3 , "Invalid elem!" );
		return ((const int*)(this))[elem];
	}

	__forceinline float Vec4V::GetXf() const
	{
		return Vec::GetX( v );
	}

	__forceinline float Vec4V::GetYf() const
	{
		return Vec::GetY( v );
	}

	__forceinline float Vec4V::GetZf() const
	{
		return Vec::GetZ( v );
	}

	__forceinline float Vec4V::GetWf() const
	{
		return Vec::GetW( v );
	}

	__forceinline int Vec4V::GetXi() const
	{
		return Vec::GetXi( v );
	}

	__forceinline int Vec4V::GetYi() const
	{
		return Vec::GetYi( v );
	}

	__forceinline int Vec4V::GetZi() const
	{
		return Vec::GetZi( v );
	}

	__forceinline int Vec4V::GetWi() const
	{
		return Vec::GetWi( v );
	}

	__forceinline void Vec4V::SetX( const float& floatVal )
	{
		Vec::SetX( v, floatVal );
	}

	__forceinline void Vec4V::SetY( const float& floatVal )
	{
		Vec::SetY( v, floatVal );
	}

	__forceinline void Vec4V::SetZ( const float& floatVal )
	{
		Vec::SetZ( v, floatVal );
	}

	__forceinline void Vec4V::SetW( const float& floatVal )
	{
		Vec::SetW( v, floatVal );
	}

	__forceinline void Vec4V::SetXf( float floatVal )
	{
		Vec::SetXInMemory( v, floatVal );
	}

	__forceinline void Vec4V::SetYf( float floatVal )
	{
		Vec::SetYInMemory( v, floatVal );
	}

	__forceinline void Vec4V::SetZf( float floatVal )
	{
		Vec::SetZInMemory( v, floatVal );
	}

	__forceinline void Vec4V::SetWf( float floatVal )
	{
		Vec::SetWInMemory( v, floatVal );
	}

	__forceinline void Vec4V::SetXi( int intVal )
	{
		Vec::SetXInMemory( v, intVal );
	}

	__forceinline void Vec4V::SetYi( int intVal )
	{
		Vec::SetYInMemory( v, intVal );
	}

	__forceinline void Vec4V::SetZi( int intVal )
	{
		Vec::SetZInMemory( v, intVal );
	}

	__forceinline void Vec4V::SetWi( int intVal )
	{
		Vec::SetWInMemory( v, intVal );
	}

	__forceinline void Vec4V::ZeroComponents()
	{
		Vec::V4ZeroComponents( v );
	}

	__forceinline void Vec4V::SetWZero()
	{
		Vec::V4SetWZero( v );
	}

	//============================================================================
	// Permute operations

	template <u32 permX, u32 permY>
	__forceinline Vec2V_Out Vec4V::Get() const
	{
		// TODO: For consistency, maybe these should be template specializations,
		// instead of counting on compile-time elimination of the branches.

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
		else if( permX == Vec::W && permY == Vec::W )
		{
			return Vec2V( Vec::V4SplatW( v ) );
		}
		// These next few specializations happen to be faster on PS3.
		else if( permX == Vec::Y && permY == Vec::Z )
		{
			return Vec2V( Vec::V4Permute<Vec::Y,Vec::Z,Vec::W,Vec::X>( v ) );
		}
		else if( permX == Vec::Z && permY == Vec::W )
		{
			return Vec2V( Vec::V4Permute<Vec::Z,Vec::W,Vec::X,Vec::Y>( v ) );
		}
		else if( permX == Vec::W && permY == Vec::X )
		{
			return Vec2V( Vec::V4Permute<Vec::W,Vec::X,Vec::Y,Vec::Z>( v ) );
		}
		// Default.
		else
		{
			return Vec2V( Vec::V4Permute<permX, permY, Vec::W, Vec::W>( v ) );
		}
	}

	template <u32 permX, u32 permY, u32 permZ>
	__forceinline Vec3V_Out Vec4V::Get() const
	{
		if( permX == Vec::X && permY == Vec::X && permZ == Vec::X )
		{
			return Vec3V( Vec::V4SplatX( v ) );
		}
		else if( permX == Vec::X && permY == Vec::X && permZ == Vec::Y )
		{
			return Vec3V( Vec::V4MergeXY( v, v ) );
		}
		else if( permX == Vec::X && permY == Vec::Y && permZ == Vec::Z )
		{
			return Vec3V( v );
		}
		else if( permX == Vec::Y && permY == Vec::Y && permZ == Vec::Y )
		{
			return Vec3V( Vec::V4SplatY( v ) );
		}
		else if( permX == Vec::Z && permY == Vec::Z && permZ == Vec::Z )
		{
			return Vec3V( Vec::V4SplatZ( v ) );
		}
		else if( permX == Vec::Z && permY == Vec::Z && permZ == Vec::W )
		{
			return Vec3V( Vec::V4MergeZW( v, v ) );
		}
		else if( permX == Vec::W && permY == Vec::W && permZ == Vec::W )
		{
			return Vec3V( Vec::V4SplatW( v ) );
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

	template <u32 permX, u32 permY, u32 permZ, u32 permW>
	__forceinline Vec4V_Out Vec4V::Get() const
	{
		CompileTimeAssert( !(permX == Vec::X && permY == Vec::Y && permZ == Vec::Z && permW == Vec::W) );
		// Useless permute!

		return Vec4V( Vec::V4Permute<permX, permY, permZ, permW>( v ) );
	}

	template <u32 permX, u32 permY>
	__forceinline void Vec4V::Set(Vec2V_In b)
	{
		CompileTimeAssert( permX != permY );
		CompileTimeAssert( permX == Vec::X || permX == Vec::Y || permX == Vec::Z || permX == Vec::W );
		CompileTimeAssert( permY == Vec::X || permY == Vec::Y || permY == Vec::Z || permY == Vec::W );
		// Useless permute!

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
		else if ( permX == Vec::X && permY == Vec::W )
		{
			v = Vec::V4PermuteTwo<Vec::X1, Vec::Y2, Vec::Z2, Vec::Y1>( b.GetIntrin128(), v );
		}
		else if ( permX == Vec::Y && permY == Vec::X )
		{
			v = Vec::V4PermuteTwo<Vec::Y1, Vec::X1, Vec::Z2, Vec::W2>( b.GetIntrin128(), v );
		}
		else if ( permX == Vec::Y && permY == Vec::Z )
		{
			v = Vec::V4PermuteTwo<Vec::X2, Vec::X1, Vec::Y1, Vec::W2>( b.GetIntrin128(), v );
		}
		else if ( permX == Vec::Y && permY == Vec::W )
		{
			v = Vec::V4PermuteTwo<Vec::X2, Vec::X1, Vec::Z2, Vec::Y1>( b.GetIntrin128(), v );
		}
		else if ( permX == Vec::Z && permY == Vec::X )
		{
			v = Vec::V4PermuteTwo<Vec::Y1, Vec::Y2, Vec::X1, Vec::W2>( b.GetIntrin128(), v );
		}
		else if ( permX == Vec::Z && permY == Vec::Y )
		{
			v = Vec::V4PermuteTwo<Vec::X2, Vec::Y1, Vec::X1, Vec::W2>( b.GetIntrin128(), v );
		}
		else if ( permX == Vec::Z && permY == Vec::W )
		{
			v = Vec::V4PermuteTwo<Vec::X2, Vec::Y2, Vec::X1, Vec::Y1>( b.GetIntrin128(), v );
		}
		else if ( permX == Vec::W && permY == Vec::X )
		{
			v = Vec::V4PermuteTwo<Vec::Y1, Vec::Y2, Vec::Z2, Vec::X1>( b.GetIntrin128(), v );
		}
		else if ( permX == Vec::W && permY == Vec::Y )
		{
			v = Vec::V4PermuteTwo<Vec::X2, Vec::Y1, Vec::Z2, Vec::X1>( b.GetIntrin128(), v );
		}
		else if ( permX == Vec::W && permY == Vec::Z )
		{
			v = Vec::V4PermuteTwo<Vec::X2, Vec::Y2, Vec::X1, Vec::Y1>( b.GetIntrin128(), v );
		}
		else
		{
			VecAssertMsg(0, "invalid permute");
		}
	}

	template <u32 permX, u32 permY, u32 permZ>
	__forceinline void Vec4V::Set(Vec3V_In b)
	{
		CompileTimeAssert( !(permX == Vec::X && permY == Vec::Y && permZ == Vec::Z) );
		// Useless permute!

		CompileTimeAssert( permX == Vec::X || permX == Vec::Y || permX == Vec::Z || permX == Vec::W );
		CompileTimeAssert( permY == Vec::X || permY == Vec::Y || permY == Vec::Z || permY == Vec::W );
		CompileTimeAssert( permZ == Vec::X || permZ == Vec::Y || permZ == Vec::Z || permZ == Vec::W );
		// Invalid permute!

		CompileTimeAssert( permX != permY && permY != permZ && permZ != permX );
		// Can't use two of the same components in the swizzle!

		if ( permX == Vec::X && permY == Vec::Y && permZ == Vec::W )
		{
			v = Vec::V4PermuteTwo<Vec::X1, Vec::Y1, Vec::Z2, Vec::Z1>( b.GetIntrin128(), v );
		}
		else if ( permX == Vec::X && permY == Vec::Z && permZ == Vec::Y )
		{
			v = Vec::V4PermuteTwo<Vec::X1, Vec::Z1, Vec::Y1, Vec::W2>( b.GetIntrin128(), v );
		}
		else if ( permX == Vec::X && permY == Vec::Z && permZ == Vec::W )
		{
			v = Vec::V4PermuteTwo<Vec::X1, Vec::Y2, Vec::Y1, Vec::Z1>( b.GetIntrin128(), v );
		}
		else if ( permX == Vec::X && permY == Vec::W && permZ == Vec::Y )
		{
			v = Vec::V4PermuteTwo<Vec::X1, Vec::Z1, Vec::Z2, Vec::Y1>( b.GetIntrin128(), v );
		}
		else if ( permX == Vec::X && permY == Vec::W && permZ == Vec::Z )
		{
			v = Vec::V4PermuteTwo<Vec::X1, Vec::Y2, Vec::Z1, Vec::Y1>( b.GetIntrin128(), v );
		}
		else if ( permX == Vec::Y && permY == Vec::X && permZ == Vec::Z )
		{
			v = Vec::V4PermuteTwo<Vec::Y1, Vec::X1, Vec::Z1, Vec::W2>( b.GetIntrin128(), v );
		}
		else if ( permX == Vec::Y && permY == Vec::X && permZ == Vec::W )
		{
			v = Vec::V4PermuteTwo<Vec::Y1, Vec::X1, Vec::Z2, Vec::Z1>( b.GetIntrin128(), v );
		}
		else if ( permX == Vec::Y && permY == Vec::Z && permZ == Vec::X )
		{
			v = Vec::V4PermuteTwo<Vec::Z1, Vec::X1, Vec::Y1, Vec::W2>( b.GetIntrin128(), v );
		}
		else if ( permX == Vec::Y && permY == Vec::Z && permZ == Vec::W )
		{
			v = Vec::V4PermuteTwo<Vec::X2, Vec::X1, Vec::Y1, Vec::Z1>( b.GetIntrin128(), v );
		}
		else if ( permX == Vec::Y && permY == Vec::W && permZ == Vec::X )
		{
			v = Vec::V4PermuteTwo<Vec::Z1, Vec::X1, Vec::Z2, Vec::Y1>( b.GetIntrin128(), v );
		}
		else if ( permX == Vec::Y && permY == Vec::W && permZ == Vec::Z )
		{
			v = Vec::V4PermuteTwo<Vec::X2, Vec::X1, Vec::Z1, Vec::Y1>( b.GetIntrin128(), v );
		}
		else if ( permX == Vec::Z && permY == Vec::X && permZ == Vec::Y )
		{
			v = Vec::V4PermuteTwo<Vec::Y1, Vec::Z1, Vec::X1, Vec::W2>( b.GetIntrin128(), v );
		}
		else if ( permX == Vec::Z && permY == Vec::X && permZ == Vec::W )
		{
			v = Vec::V4PermuteTwo<Vec::Y1, Vec::Y2, Vec::X1, Vec::Z1>( b.GetIntrin128(), v );
		}
		else if ( permX == Vec::Z && permY == Vec::Y && permZ == Vec::X )
		{
			v = Vec::V4PermuteTwo<Vec::Z1, Vec::Y1, Vec::X1, Vec::W2>( b.GetIntrin128(), v );
		}
		else if ( permX == Vec::Z && permY == Vec::Y && permZ == Vec::W )
		{
			v = Vec::V4PermuteTwo<Vec::X2, Vec::Y1, Vec::X1, Vec::Z1>( b.GetIntrin128(), v );
		}
		else if ( permX == Vec::Z && permY == Vec::W && permZ == Vec::X )
		{
			v = Vec::V4PermuteTwo<Vec::Z1, Vec::Y2, Vec::X1, Vec::Y1>( b.GetIntrin128(), v );
		}
		else if ( permX == Vec::Z && permY == Vec::W && permZ == Vec::Y )
		{
			v = Vec::V4PermuteTwo<Vec::X2, Vec::Z1, Vec::X1, Vec::Y1>( b.GetIntrin128(), v );
		}
		else if ( permX == Vec::W && permY == Vec::X && permZ == Vec::Y )
		{
			v = Vec::V4PermuteTwo<Vec::Y1, Vec::Z1, Vec::Z2, Vec::X1>( b.GetIntrin128(), v );
		}
		else if ( permX == Vec::W && permY == Vec::X && permZ == Vec::Z )
		{
			v = Vec::V4PermuteTwo<Vec::Y1, Vec::Y2, Vec::Z1, Vec::X1>( b.GetIntrin128(), v );
		}
		else if ( permX == Vec::W && permY == Vec::Y && permZ == Vec::X )
		{
			v = Vec::V4PermuteTwo<Vec::Z1, Vec::Y1, Vec::Z2, Vec::X1>( b.GetIntrin128(), v );
		}
		else if ( permX == Vec::W && permY == Vec::Y && permZ == Vec::Z )
		{
			v = Vec::V4PermuteTwo<Vec::X2, Vec::Y1, Vec::Z1, Vec::X1>( b.GetIntrin128(), v );
		}
		else if ( permX == Vec::W && permY == Vec::Z && permZ == Vec::X )
		{
			v = Vec::V4PermuteTwo<Vec::Z1, Vec::Y2, Vec::Y1, Vec::X1>( b.GetIntrin128(), v );
		}
		else if ( permX == Vec::W && permY == Vec::Z && permZ == Vec::Y )
		{
			v = Vec::V4PermuteTwo<Vec::X2, Vec::Z1, Vec::Y1, Vec::X1>( b.GetIntrin128(), v );
		}
		else
		{
			VecAssertMsg(0, "invalid permute");
		}
	}

	template <u32 permX, u32 permY, u32 permZ, u32 permW>
	__forceinline void Vec4V::Set(Vec4V_In b)
	{
		CompileTimeAssert( !(permX == Vec::X && permY == Vec::Y && permZ == Vec::Z && permW == Vec::W) );
		// Useless permute!

		CompileTimeAssert( permX == Vec::X || permX == Vec::Y || permX == Vec::Z || permX == Vec::W );
		CompileTimeAssert( permY == Vec::X || permY == Vec::Y || permY == Vec::Z || permY == Vec::W );
		CompileTimeAssert( permZ == Vec::X || permZ == Vec::Y || permZ == Vec::Z || permZ == Vec::W );
		CompileTimeAssert( permW == Vec::X || permW == Vec::Y || permW == Vec::Z || permW == Vec::W );
		// Invalid permute!

		CompileTimeAssert( permX != permY && permY != permZ && permZ != permW && permW != permX );
		// Can't use two of the same components in the swizzle!

		v = Vec::V4Permute<permX, permY, permZ, permW>( b );
	}

#if __XENON || __PS3
	template <u8 byte0,u8 byte1,u8 byte2,u8 byte3,u8 byte4,u8 byte5,u8 byte6,u8 byte7,u8 byte8,u8 byte9,u8 byte10,u8 byte11,u8 byte12,u8 byte13,u8 byte14,u8 byte15>
	__forceinline Vec4V_Out Vec4V::ByteGet() const
	{
		return Vec4V( Vec::V4BytePermute<byte0,byte1,byte2,byte3,byte4,byte5,byte6,byte7,byte8,byte9,byte10,byte11,byte12,byte13,byte14,byte15>(v) );
	}

	__forceinline Vec4V_Out Vec4V::Get( Vec4V_In controlVec ) const
	{
		return Vec4V( Vec::V4Permute( v, controlVec.GetIntrin128() ) );
	}
#endif // __XENON || __PS3


	//============================================================================
	// Operators

	__forceinline VecBoolV_Out Vec4V::operator== (Vec4V_In b) const
	{
		return VecBoolV( Vec::V4IsEqualV( v, b.v ) );
	}

	__forceinline VecBoolV_Out Vec4V::operator!= (Vec4V_In b) const
	{
		return VecBoolV( Vec::V4InvertBits( Vec::V4IsEqualV( v, b.v ) ) );
	}

	__forceinline VecBoolV_Out Vec4V::operator< (Vec4V_In bigVector) const
	{
		return VecBoolV( Vec::V4IsLessThanV( v, bigVector.v ) );
	}

	__forceinline VecBoolV_Out Vec4V::operator<= (Vec4V_In bigVector) const
	{
		return VecBoolV( Vec::V4IsLessThanOrEqualV( v, bigVector.v ) );
	}

	__forceinline VecBoolV_Out Vec4V::operator> (Vec4V_In smallVector) const
	{
		return VecBoolV( Vec::V4IsGreaterThanV( v, smallVector.v ) );
	}

	__forceinline VecBoolV_Out Vec4V::operator>= (Vec4V_In smallVector) const
	{
		return VecBoolV( Vec::V4IsGreaterThanOrEqualV( v, smallVector.v ) );
	}

	__forceinline Vec4V_Out Vec4V::operator* (Vec4V_In b) const
	{
		return Vec4V( Vec::V4Scale( v, b.v ) );
	}

	__forceinline Vec4V_Out Vec4V::operator* (ScalarV_In b) const
	{
		return Vec4V( Vec::V4Scale( v, b.v ) );
	}

	__forceinline Vec4V_Out operator* (ScalarV_In a, Vec4V_In b)
	{
		return Vec4V( Vec::V4Scale( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline Vec4V_Out Vec4V::operator/ (Vec4V_In b) const
	{
		return Vec4V( Vec::V4InvScale( v, b.v ) );
	}

	__forceinline Vec4V_Out Vec4V::operator/ (ScalarV_In b) const
	{
		return Vec4V( Vec::V4InvScale( v, b.v ) );
	}

	__forceinline Vec4V_Out Vec4V::operator+ (Vec4V_In b) const
	{
		return Vec4V( Vec::V4Add( v, b.v ) );
	}

	__forceinline Vec4V_Out Vec4V::operator- (Vec4V_In b) const
	{
		return Vec4V( Vec::V4Subtract( v, b.v ) );
	}

	__forceinline void Vec4V::operator*= (Vec4V_In b)
	{
		v = Vec::V4Scale( v, b.v );
	}

	__forceinline void Vec4V::operator*= (ScalarV_In b)
	{
		v = Vec::V4Scale( v, b.v );
	}

	__forceinline void Vec4V::operator/= (Vec4V_In b)
	{
		v = Vec::V4InvScale( v, b.v );
	}

	__forceinline void Vec4V::operator/= (ScalarV_In b)
	{
		v = Vec::V4InvScale( v, b.v );
	}

	__forceinline void Vec4V::operator+= (Vec4V_In b)
	{
		v = Vec::V4Add( v, b.v );
	}

	__forceinline void Vec4V::operator-= (Vec4V_In b)
	{
		v = Vec::V4Subtract( v, b.v );
	}

	__forceinline Vec4V_Out Vec4V::operator+ () const
	{
		return Vec4V( v );
	}

	__forceinline Vec4V_Out Vec4V::operator- () const
	{
		return Vec4V( Vec::V4Negate( v ) );
	}

	__forceinline Vec4V_Out Vec4V::operator| (Vec4V_In b) const
	{
		return Vec4V( Vec::V4Or( v, b.v ) );
	}

	__forceinline Vec4V_Out Vec4V::operator& (Vec4V_In b) const
	{
		return Vec4V( Vec::V4And( v, b.v ) );
	}

	__forceinline Vec4V_Out Vec4V::operator^ (Vec4V_In b) const
	{
		return Vec4V( Vec::V4Xor( v, b.v ) );
	}

	__forceinline void Vec4V::operator|= (Vec4V_In b)
	{
		v = Vec::V4Or( v, b.v );
	}

	__forceinline void Vec4V::operator&= (Vec4V_In b)
	{
		v = Vec::V4And( v, b.v );
	}

	__forceinline void Vec4V::operator^= (Vec4V_In b)
	{
		v = Vec::V4Xor( v, b.v );
	}

	__forceinline Vec4V_Out Vec4V::operator~ () const
	{
		return Vec4V( Vec::V4InvertBits( v ) );
	}

	__forceinline const float& Vec4V::operator[] (unsigned elem) const
	{
		return Vec::GetElemRef( &v, elem );
	}

	__forceinline float& Vec4V::operator[] (unsigned elem)
	{
		return Vec::GetElemRef( &v, elem );
	}

	namespace sysEndian
	{
		template<> inline void SwapMe(Vec4V& v) { return SwapMe(v.GetIntrin128Ref()); }
	} // namespace sysEndian

} // namespace rage
