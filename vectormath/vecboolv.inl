namespace rage
{
	__forceinline VecBoolV::VecBoolV()
	{
	}

#if __WIN32PC
	__forceinline VecBoolV::VecBoolV(VecBoolV_ConstRef _v)
		: v(_v.v)
	{
	}
#endif

#if !__XENON
	__forceinline VecBoolV_ConstRef VecBoolV::operator=(VecBoolV_ConstRef _v)
	{
		v = _v.v;
		return *this;
	}
#endif

	__forceinline VecBoolV::VecBoolV(eZEROInitializer)
	{
		Vec::V4Set( v, Vec::V4VConstant(V_ZERO) );
	}

	__forceinline VecBoolV::VecBoolV(eMASKWInitializer)
	{
		Vec::V4Set( v, Vec::V4VConstant(V_MASKW) );
	}

	__forceinline VecBoolV::VecBoolV(eMASKZInitializer)
	{
		Vec::V4Set( v, Vec::V4VConstant(V_MASKZ) );
	}

	__forceinline VecBoolV::VecBoolV(eMASKZWInitializer)
	{
		Vec::V4Set( v, Vec::V4VConstant(V_MASKZW) );
	}

	__forceinline VecBoolV::VecBoolV(eMASKYInitializer)
	{
		Vec::V4Set( v, Vec::V4VConstant(V_MASKY) );
	}

	__forceinline VecBoolV::VecBoolV(eMASKYWInitializer)
	{
		Vec::V4Set( v, Vec::V4VConstant(V_MASKYW) );
	}

	__forceinline VecBoolV::VecBoolV(eMASKYZInitializer)
	{
		Vec::V4Set( v, Vec::V4VConstant(V_MASKYZ) );
	}

	__forceinline VecBoolV::VecBoolV(eMASKYZWInitializer)
	{
		Vec::V4Set( v, Vec::V4VConstant(V_MASKYZW) );
	}

	__forceinline VecBoolV::VecBoolV(eMASKXInitializer)
	{
		Vec::V4Set( v, Vec::V4VConstant(V_MASKX) );
	}

	__forceinline VecBoolV::VecBoolV(eMASKXWInitializer)
	{
		Vec::V4Set( v, Vec::V4VConstant(V_MASKXW) );
	}

	__forceinline VecBoolV::VecBoolV(eMASKXZInitializer)
	{
		Vec::V4Set( v, Vec::V4VConstant(V_MASKXZ) );
	}

	__forceinline VecBoolV::VecBoolV(eMASKXZWInitializer)
	{
		Vec::V4Set( v, Vec::V4VConstant(V_MASKXZW) );
	}

	__forceinline VecBoolV::VecBoolV(eMASKXYInitializer)
	{
		Vec::V4Set( v, Vec::V4VConstant(V_MASKXY) );
	}

	__forceinline VecBoolV::VecBoolV(eMASKXYWInitializer)
	{
		Vec::V4Set( v, Vec::V4VConstant(V_MASKXYW) );
	}

	__forceinline VecBoolV::VecBoolV(eMASKXYZInitializer)
	{
		Vec::V4Set( v, Vec::V4VConstant(V_MASKXYZ) );
	}

	__forceinline VecBoolV::VecBoolV(eMASKXYZWInitializer)
	{
		Vec::V4Set( v, Vec::V4VConstant(V_MASKXYZW) );
	}

	__forceinline VecBoolV::VecBoolV(Vec::Vector_4V_In a)
	{
		v = a;
	}

	__forceinline VecBoolV::VecBoolV(BoolV_In a)
	{
		v = a.v;
	}

	__forceinline BoolV_Out VecBoolV::GetX() const
	{
		return BoolV( Vec::V4SplatX(v) );
	}

	__forceinline BoolV_Out VecBoolV::GetY() const
	{
		return BoolV( Vec::V4SplatY( v ) );
	}

	__forceinline BoolV_Out VecBoolV::GetZ() const
	{
		return BoolV( Vec::V4SplatZ( v ) );
	}

	__forceinline BoolV_Out VecBoolV::GetW() const
	{
		return BoolV( Vec::V4SplatW( v ) );
	}

	__forceinline void VecBoolV::SetX( BoolV_In newX )
	{
		v = Vec::V4PermuteTwo<Vec::W1,Vec::Y2,Vec::Z2,Vec::W2>( newX.GetIntrin128(), v );
	}

	__forceinline void VecBoolV::SetY( BoolV_In newY )
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

	__forceinline void VecBoolV::SetZ( BoolV_In newZ )
	{
		v = Vec::V4PermuteTwo<Vec::X2,Vec::Y2,Vec::X1,Vec::W2>( newZ.GetIntrin128(), v );
	}

	__forceinline void VecBoolV::SetW( BoolV_In newW )
	{
		v = Vec::V4PermuteTwo<Vec::X2,Vec::Y2,Vec::Z2,Vec::X1>( newW.GetIntrin128(), v );
	}

	//============================================================================
	// Permute operations

	template <u32 permX, u32 permY>
	__forceinline VecBoolV_Out VecBoolV::Get() const
	{
		// TODO: For consistency, maybe these should be template specializations,
		// instead of counting on compile-time elimination of the branches.

		if( permX == Vec::X && permY == Vec::X )
		{
			return VecBoolV( Vec::V4SplatX( v ) );
		}
		else if( permX == Vec::X && permY == Vec::Y )
		{
			return VecBoolV( v );
		}
		else if( permX == Vec::Y && permY == Vec::Y )
		{
			return VecBoolV( Vec::V4SplatY( v ) );
		}
		else if( permX == Vec::Z && permY == Vec::Z )
		{
			return VecBoolV( Vec::V4SplatZ( v ) );
		}
		else if( permX == Vec::W && permY == Vec::W )
		{
			return VecBoolV( Vec::V4SplatW( v ) );
		}
		// These next few specializations happen to be faster on PS3.
		else if( permX == Vec::Y && permY == Vec::Z )
		{
			return VecBoolV( Vec::V4Permute<Vec::Y,Vec::Z,Vec::W,Vec::X>( v ) );
		}
		else if( permX == Vec::Z && permY == Vec::W )
		{
			return VecBoolV( Vec::V4Permute<Vec::Z,Vec::W,Vec::X,Vec::Y>( v ) );
		}
		else if( permX == Vec::W && permY == Vec::X )
		{
			return VecBoolV( Vec::V4Permute<Vec::W,Vec::X,Vec::Y,Vec::Z>( v ) );
		}
		// Default.
		else
		{
			return VecBoolV( Vec::V4Permute<permX, permY, Vec::W, Vec::W>( v ) );
		}
	}

	template <u32 permX, u32 permY, u32 permZ>
	__forceinline VecBoolV_Out VecBoolV::Get() const
	{
		if( permX == Vec::X && permY == Vec::X && permZ == Vec::X )
		{
			return VecBoolV( Vec::V4SplatX( v ) );
		}
		else if( permX == Vec::X && permY == Vec::X && permZ == Vec::Y )
		{
			return VecBoolV( Vec::V4MergeXY( v, v ) );
		}
		else if( permX == Vec::X && permY == Vec::Y && permZ == Vec::Z )
		{
			return VecBoolV( v );
		}
		else if( permX == Vec::Y && permY == Vec::Y && permZ == Vec::Y )
		{
			return VecBoolV( Vec::V4SplatY( v ) );
		}
		else if( permX == Vec::Z && permY == Vec::Z && permZ == Vec::Z )
		{
			return VecBoolV( Vec::V4SplatZ( v ) );
		}
		else if( permX == Vec::Z && permY == Vec::Z && permZ == Vec::W )
		{
			return VecBoolV( Vec::V4MergeZW( v, v ) );
		}
		else if( permX == Vec::W && permY == Vec::W && permZ == Vec::W )
		{
			return VecBoolV( Vec::V4SplatW( v ) );
		}
		// These next few specializations happen to be faster on PS3.
		else if( permX == Vec::Y && permY == Vec::Z && permZ == Vec::W )
		{
			return VecBoolV( Vec::V4Permute<Vec::Y,Vec::Z,Vec::W,Vec::X>( v ) );
		}
		else if( permX == Vec::Z && permY == Vec::W && permZ == Vec::X )
		{
			return VecBoolV( Vec::V4Permute<Vec::Z,Vec::W,Vec::X,Vec::Y>( v ) );
		}
		else if( permX == Vec::W && permY == Vec::X && permZ == Vec::Y )
		{
			return VecBoolV( Vec::V4Permute<Vec::W,Vec::X,Vec::Y,Vec::Z>( v ) );
		}
		// Default.
		else
		{
			return VecBoolV( Vec::V4Permute<permX, permY, permZ, Vec::W>( v ) );
		}
	}

	template <u32 permX, u32 permY, u32 permZ, u32 permW>
	__forceinline VecBoolV_Out VecBoolV::Get() const
	{
		CompileTimeAssert( !(permX == Vec::X && permY == Vec::Y && permZ == Vec::Z && permW == Vec::W) );
		// Useless permute!

		return VecBoolV( Vec::V4Permute<permX, permY, permZ, permW>( v ) );
	}

	template <u32 permX, u32 permY>
	__forceinline void VecBoolV::Set(VecBoolV_In b)
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
	__forceinline void VecBoolV::Set(VecBoolV_In b)
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
	__forceinline void VecBoolV::Set(VecBoolV_In b)
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

	__forceinline VecBoolV_Out VecBoolV::operator== (VecBoolV_In b) const
	{
		return VecBoolV( Vec::V4IsEqualIntV( v, b.v ) );
	}

	__forceinline VecBoolV_Out VecBoolV::operator!= (VecBoolV_In b) const
	{
		return VecBoolV( Vec::V4InvertBits( Vec::V4IsEqualV( v, b.v ) ) );
	}

	__forceinline VecBoolV_Out VecBoolV::operator! () const
	{
		return VecBoolV( Vec::V4InvertBits( v ) );
	}

	__forceinline VecBoolV_Out VecBoolV::operator| (VecBoolV_In b) const
	{
		return VecBoolV( Vec::V4Or( v, b.v ) );
	}

	__forceinline VecBoolV_Out VecBoolV::operator& (VecBoolV_In b) const
	{
		return VecBoolV( Vec::V4And( v, b.v ) );
	}

	__forceinline VecBoolV_Out VecBoolV::operator^ (VecBoolV_In b) const
	{
		return VecBoolV( Vec::V4Xor( v, b.v ) );
	}

	__forceinline void VecBoolV::operator|= (VecBoolV_In b)
	{
		v = Vec::V4Or( v, b.v );
	}

	__forceinline void VecBoolV::operator&= (VecBoolV_In b)
	{
		v = Vec::V4And( v, b.v );
	}

	__forceinline void VecBoolV::operator^= (VecBoolV_In b)
	{
		v = Vec::V4Xor( v, b.v );
	}

	__forceinline void VecBoolV::SetIntrin128(Vec::Vector_4V_In _v)
	{
		v = _v;
	}

	__forceinline Vec::Vector_4V VecBoolV::GetIntrin128() const
	{
		return v;
	}

	__forceinline Vec::Vector_4V_Ref VecBoolV::GetIntrin128Ref()
	{
		return v;
	}

	__forceinline Vec::Vector_4V_ConstRef VecBoolV::GetIntrin128ConstRef() const
	{
		return v;
	}

	namespace sysEndian
	{
		template<> inline void SwapMe(VecBoolV& v) { return SwapMe(v.GetIntrin128Ref()); }
	} // namespace sysEndian


} // namespace rage

