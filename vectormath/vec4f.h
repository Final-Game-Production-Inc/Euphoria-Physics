#ifndef VECTORMATH_VEC4F_H
#define VECTORMATH_VEC4F_H

#include "data/struct.h"
#include "vectormath.h"
#include "vec3f.h"
#include "vec2f.h"

namespace rage
{

	class Vec4f;

	//================================================
	// TYPES
	//================================================

	typedef Vec4f			Vec4f_Val;
	typedef Vec4f*			Vec4f_Ptr;
	typedef Vec4f&			Vec4f_Ref;
	typedef const Vec4f&		Vec4f_ConstRef;

#if __WIN32PC
	typedef Vec4f_ConstRef	Vec4f_In;
#else
	typedef Vec4f_Val		Vec4f_In;
#endif
	typedef const Vec4f_Val	Vec4f_Out;	// "const" to warn that this isn't a "return by ref" parameter
	typedef Vec4f_Ref		Vec4f_InOut;

	//================================================
	// VEC4
	//================================================

	class Vec4f
	{

		friend class Quatf;
		friend class Mat33;
		friend class Mat34;
		friend class Mat44;

	public:
		enum eZEROInitializer			{	ZERO = 0,	};
		enum eONEInitializer			{	ONE = 0,	};
		enum eMASKXYZWInitializer		{	MASKXYZW = 0,	};
		enum eFLT_LARGE_8Initializer	{	FLT_LARGE_8 = 0,	};
		enum eFLT_EPSInitializer		{	FLT_EPS = 0,	};
		enum eFLTMAXInitializer			{	FLTMAX = 0,	};
		enum eZERO_WONEInitializer		{	ZERO_WONE = 0,	};

	public:
		// vector constant generation syntax: Vec4f( Vec4f::ZERO )
		explicit Vec4f(eZEROInitializer);
		explicit Vec4f(eONEInitializer);
		explicit Vec4f(eMASKXYZWInitializer);
		explicit Vec4f(eFLT_LARGE_8Initializer);
		explicit Vec4f(eFLT_EPSInitializer);
		explicit Vec4f(eFLTMAXInitializer);
		explicit Vec4f(eZERO_WONEInitializer);

		Vec4f();
		explicit Vec4f(float,float,float,float);
		explicit Vec4f(Vec::Vector_4_In);

		// PURPOSE: Resource constructor, for structures created from a resource
		// NOTES: This is here so that we can bypass the default ctor when using placement new, does not init vars.
		Vec4f(class datResource&) {}

		// PURPOSE: Used by the rorc resourcing system
		DECLARE_DUMMY_PLACE(Vec4f);

		// PURPOSE: Used by the rorc resourcing system
#if __DECLARESTRUCT
		void DeclareStruct(class datTypeStruct &s)
		{
			STRUCT_BEGIN(Vec4f);
			STRUCT_FIELD( v );
			STRUCT_END();
		}
#endif

		//============================================================================
		// Getters / setters

		float GetX() const;
		float GetY() const;
		float GetZ() const;
		float GetW() const;
		int GetXi() const;
		int GetYi() const;
		int GetZi() const;
		int GetWi() const;
		Vec3f_Out GetXYZ() const;
		void SetX( float newX );
		void SetY( float newY );
		void SetZ( float newZ );
		void SetW( float newW );
		void SetXi( int newX );
		void SetYi( int newY );
		void SetZi( int newZ );
		void SetWi( int newW );
		void SetXYZ( Vec3f_In newXYZ );

		Vec::Vector_4_ConstRef GetIntrinConstRef() const;
		Vec::Vector_4_Ref GetIntrinRef();
		Vec::Vector_4_Out GetIntrin() const;

		void SetIntrin( Vec::Vector_4_In );

		float GetXf() const { return GetX(); }
		float GetYf() const { return GetY(); }
		float GetZf() const { return GetZ(); }
		float GetWf() const { return GetW(); }
		void SetXf( float floatVal ) { SetX(floatVal); }
		void SetYf( float floatVal ) { SetY(floatVal); }
		void SetZf( float floatVal ) { SetZ(floatVal); }
		void SetWf( float floatVal ) { SetW(floatVal); }
		void ZeroComponents();
		void SetWZero();

		//============================================================================
		// Output

#if !__NO_OUTPUT
		void Print(bool newline=true) const;
		void PrintHex(bool newline=true) const;
#endif

		//============================================================================
		// Operators

		// Arithmetic
		Vec4f_Out		operator*	(Vec4f_In b) const; // per-element multiply!
		Vec4f_Out		operator*	(float b) const;
		friend Vec4f_Out operator*	(float a, Vec4f_In b);
		Vec4f_Out		operator/	(Vec4f_In b) const; // per-element divide!
		Vec4f_Out		operator/	(float b) const;
		Vec4f_Out		operator+	(Vec4f_In b) const;
		Vec4f_Out		operator-	(Vec4f_In b) const;
		void			operator*=	(Vec4f_In b);
		void			operator*=	(float b);
		void			operator/=	(Vec4f_In b);
		void			operator/=	(float b);
		void			operator+=	(Vec4f_In b);
		void			operator-=	(Vec4f_In b);
		Vec4f_Out		operator-	() const;

		// Bitwise
		Vec4f_Out		operator|	(Vec4f_In b) const;
		Vec4f_Out		operator&	(Vec4f_In b) const;
		Vec4f_Out		operator^	(Vec4f_In b) const;
		void			operator|=	(Vec4f_In b);
		void			operator&=	(Vec4f_In b);
		void			operator^=	(Vec4f_In b);

		// Element access.
		const float&	operator[]	(u32 elem) const;
		float&			operator[]	(u32 elem);	

	private:
		Vec::Vector_4 v;
	};
} // namespace rage

#include "vec4f.inl"


#endif // VECTORMATH_VEC4F_H
