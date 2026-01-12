#ifndef VECTORMATH_VEC3F_H
#define VECTORMATH_VEC3F_H

#include "data/struct.h"
#include "vectormath.h"
#include "vec2f.h"
#include "vector/vector3_config.h"

namespace rage
{
	class VECTOR_ALIGN Vec3f;

	//================================================
	// TYPES
	//================================================

	typedef Vec3f			Vec3f_Val;
	typedef Vec3f*			Vec3f_Ptr;
	typedef Vec3f&			Vec3f_Ref;
	typedef const Vec3f&		Vec3f_ConstRef;

#if __WIN32PC
	typedef Vec3f_ConstRef	Vec3f_In;
#else
	typedef Vec3f_Val		Vec3f_In;
#endif
	typedef const Vec3f_Val	Vec3f_Out;	// "const" to warn that this isn't a "return by ref" parameter
	typedef Vec3f_Ref		Vec3f_InOut;

	//================================================
	// VEC3
	//================================================

	class VECTOR_ALIGN Vec3f
	{
		friend class Vec4f;
		friend class Quatf;
		friend class Mat33;
		friend class Mat34;
		friend class Mat44;

	public:
		enum eZEROInitializer			{	ZERO,	};
		enum eIDENTITYInitializer		{	IDENTITY,	};
		enum eMASKXYZInitializer		{	MASKXYZ,	};
		enum eFLT_LARGE_8Initializer	{	FLT_LARGE_8,	};
		enum eFLT_EPSInitializer		{	FLT_EPS,	};
		enum eFLTMAXInitializer			{	FLTMAX,	};

	public:
		// vector constant generation syntax: Vec3f( Vec3f::ZERO )
		explicit Vec3f(eZEROInitializer);
		explicit Vec3f(eIDENTITYInitializer);
		explicit Vec3f(eMASKXYZInitializer);
		explicit Vec3f(eFLT_LARGE_8Initializer);
		explicit Vec3f(eFLT_EPSInitializer);
		explicit Vec3f(eFLTMAXInitializer);

		Vec3f();
		explicit Vec3f(float,float,float);
		explicit Vec3f(Vec::Vector_3_In);

		Vec3f(const Vec3f& v);
		Vec3f& operator=(const Vec3f& v);

		// PURPOSE: Resource constructor, for structures created from a resource
		// NOTES: This is here so that we can bypass the default ctor when using placement new, does not init vars.
		Vec3f(class datResource&) {}

		// PURPOSE: Used by the rorc resourcing system
		DECLARE_DUMMY_PLACE(Vec3f);

		// PURPOSE: Used by the rorc resourcing system
#if __DECLARESTRUCT
		void DeclareStruct(class datTypeStruct &s)
		{
			STRUCT_BEGIN(Vec3f);
			STRUCT_FIELD( v );
			STRUCT_END();
		}
#endif

		float GetX() const;
		float GetY() const;
		float GetZ() const;
		void SetX( float newX );
		void SetY( float newY );
		void SetZ( float newZ );

		Vec::Vector_3_Ref GetIntrinRef();
		Vec::Vector_3_Out GetIntrin() const;
		Vec::Vector_3_ConstRef GetIntrinConstRef() const;

		void SetIntrin( Vec::Vector_3_In );

		void ZeroComponents();

		//============================================================================
		// Output

#if !__NO_OUTPUT
		void Print(bool newline=true) const;
		void PrintHex(bool newline=true) const;
#endif

		//============================================================================
		// Operators

		// Arithmetic
		Vec3f_Out		operator*	(Vec3f_In b) const; // per-element multiply!
		Vec3f_Out		operator*	(float b) const;
		friend Vec3f_Out operator*	(float a, Vec3f_In b);
		Vec3f_Out		operator/	(Vec3f_In b) const; // per-element divide!
		Vec3f_Out		operator/	(float b) const;
		Vec3f_Out		operator+	(Vec3f_In b) const;
		Vec3f_Out		operator-	(Vec3f_In b) const;
		void			operator*=	(Vec3f_In b);
		void			operator*=	(float b);
		void			operator/=	(Vec3f_In b);
		void			operator/=	(float b);
		void			operator+=	(Vec3f_In b);
		void			operator-=	(Vec3f_In b);
		Vec3f_Out		operator-	() const;

		// Bitwise
		Vec3f_Out		operator|	(Vec3f_In b) const;
		Vec3f_Out		operator&	(Vec3f_In b) const;
		Vec3f_Out		operator^	(Vec3f_In b) const;
		void			operator|=	(Vec3f_In b);
		void			operator&=	(Vec3f_In b);
		void			operator^=	(Vec3f_In b);

		// Element access.
		const float&	operator[]	(u32 elem) const;
		float&			operator[]	(u32 elem);

	private:
		Vec::Vector_3 v;

		u32 pad;
	}
#if (__PS3) && VECTORIZED
	
#endif
		;
} // namespace rage


#include "vec3f.inl"


#endif // VECTORMATH_VEC3F_H
