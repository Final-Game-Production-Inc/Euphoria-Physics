
namespace rage
{
	__forceinline SoA_QuatV::SoA_QuatV(eZEROInitializer)
	{
		w = z = y = x = Vec::V4VConstant(V_ZERO);
	}

	__forceinline SoA_QuatV::SoA_QuatV(eIDENTITYInitializer)
	{
		z = y = x = Vec::V4VConstant(V_ZERO);
		w = Vec::V4VConstant(V_ONE);
	}

	__forceinline SoA_QuatV::SoA_QuatV(eFLT_LARGE_8Initializer)
	{
		w = z = y = x = Vec::V4VConstant(V_FLT_LARGE_8);
	}

	__forceinline SoA_QuatV::SoA_QuatV(eFLT_EPSInitializer)
	{
		w = z = y = x = Vec::V4VConstant(V_FLT_EPSILON);
	}

	__forceinline SoA_QuatV::SoA_QuatV()
	{
	}

	__forceinline SoA_QuatV::SoA_QuatV(SoA_QuatV_ConstRef q)
		:	x(q.x), y( q.y ), z( q.z ), w( q.w )
	{
	}

	__forceinline SoA_QuatV_Ref SoA_QuatV::operator= (SoA_QuatV_ConstRef q)
	{
		x = q.x;
		y = q.y;
		z = q.z;
		w = q.w;
		return *this;
	}

	__forceinline SoA_QuatV::SoA_QuatV(const float& _x, const float& _y, const float& _z, const float& _w)
	{
		Vec::V4Set( x, _x );
		Vec::V4Set( y, _y );
		Vec::V4Set( z, _z );
		Vec::V4Set( w, _w );
	}

	__forceinline SoA_QuatV::SoA_QuatV(Vec::Vector_4V_In scalar)
		: x( scalar ), y( scalar ), z(scalar), w(scalar)
	{
	}

	__forceinline SoA_QuatV::SoA_QuatV(Vec::Vector_4V_In _x, Vec::Vector_4V_In _y, Vec::Vector_4V_In _z, Vec::Vector_4V_In_After3Args _w)
		: x( _x ), y( _y ), z( _z ), w( _w )
	{
	}

	__forceinline void SoA_QuatV::SetXIntrin128(Vec::Vector_4V_In _v)
	{
		x = _v;
	}

	__forceinline void SoA_QuatV::SetYIntrin128(Vec::Vector_4V_In _v)
	{
		y = _v;
	}

	__forceinline void SoA_QuatV::SetZIntrin128(Vec::Vector_4V_In _v)
	{
		z = _v;
	}

	__forceinline void SoA_QuatV::SetWIntrin128(Vec::Vector_4V_In _v)
	{
		w = _v;
	}

	__forceinline Vec::Vector_4V_Out SoA_QuatV::GetXIntrin128() const
	{
		return x;
	}

	__forceinline Vec::Vector_4V_Out SoA_QuatV::GetYIntrin128() const
	{
		return y;
	}

	__forceinline Vec::Vector_4V_Out SoA_QuatV::GetZIntrin128() const
	{
		return z;
	}

	__forceinline Vec::Vector_4V_Out SoA_QuatV::GetWIntrin128() const
	{
		return w;
	}

	__forceinline Vec::Vector_4V_Ref SoA_QuatV::GetXIntrin128Ref()
	{
		return x;
	}

	__forceinline Vec::Vector_4V_Ref SoA_QuatV::GetYIntrin128Ref()
	{
		return x;
	}

	__forceinline Vec::Vector_4V_Ref SoA_QuatV::GetZIntrin128Ref()
	{
		return z;
	}

	__forceinline Vec::Vector_4V_Ref SoA_QuatV::GetWIntrin128Ref()
	{
		return w;
	}

	__forceinline SoA_ScalarV_Out SoA_QuatV::GetX() const
	{
		return SoA_ScalarV( x );
	}

	__forceinline SoA_ScalarV_Out SoA_QuatV::GetY() const
	{
		return SoA_ScalarV( y );
	}

	__forceinline SoA_ScalarV_Out SoA_QuatV::GetZ() const
	{
		return SoA_ScalarV( z );
	}

	__forceinline SoA_ScalarV_Out SoA_QuatV::GetW() const
	{
		return SoA_ScalarV( w );
	}

	__forceinline void SoA_QuatV::SetX( SoA_ScalarV_In newX )
	{
		x = newX.GetIntrin128();
	}

	__forceinline void SoA_QuatV::SetY( SoA_ScalarV_In newY )
	{
		y = newY.GetIntrin128();
	}

	__forceinline void SoA_QuatV::SetZ( SoA_ScalarV_In newZ )
	{
		z = newZ.GetIntrin128();
	}

	__forceinline void SoA_QuatV::SetW( SoA_ScalarV_In newW )
	{
		w = newW.GetIntrin128();
	}

	__forceinline void SoA_QuatV::ZeroComponents()
	{
		x = y = z = w = Vec::V4VConstant(V_ZERO);
	}

	//============================================================================
	// Operators

	__forceinline SoA_VecBool4V_Out SoA_QuatV::operator== (SoA_QuatV_In b) const
	{
		return SoA_VecBool4V(
						Vec::V4IsEqualV( x, b.x ),
						Vec::V4IsEqualV( y, b.y ),
						Vec::V4IsEqualV( z, b.z ),
						Vec::V4IsEqualV( w, b.w )
						);
	}

	__forceinline SoA_VecBool4V_Out SoA_QuatV::operator!= (SoA_QuatV_In b) const
	{
		return SoA_VecBool4V(
						Vec::V4InvertBits( Vec::V4IsEqualV( x, b.x ) ),
						Vec::V4InvertBits( Vec::V4IsEqualV( y, b.y ) ),
						Vec::V4InvertBits( Vec::V4IsEqualV( z, b.z ) ),
						Vec::V4InvertBits( Vec::V4IsEqualV( w, b.w ) )
						);
	}

	__forceinline Vec::Vector_4V_ConstRef SoA_QuatV::operator[] (u32 elem) const
	{
		VecAssertMsg( elem <= 3 , "Invalid element index." );
		return (&x)[elem];
	}

	__forceinline Vec::Vector_4V_Ref SoA_QuatV::operator[] (u32 elem)
	{
		VecAssertMsg( elem <= 3 , "Invalid element index." );
		return (&x)[elem];
	}
} // namespace rage
