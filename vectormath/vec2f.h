#ifndef VECTORMATH_VEC2F_H
#define VECTORMATH_VEC2F_H

#include "vectormath.h"
#include "data/struct.h"

namespace rage
{
	class Vec2f;

	//================================================
	// TYPES
	//================================================

	typedef Vec2f			Vec2f_Val;
	typedef Vec2f*			Vec2f_Ptr;
	typedef Vec2f&			Vec2f_Ref;
	typedef const Vec2f&		Vec2f_ConstRef;

#if __WIN32PC
	typedef Vec2f_ConstRef	Vec2f_In;
#else
	typedef Vec2f_Val		Vec2f_In;
#endif
	typedef const Vec2f_Val	Vec2f_Out;	// "const" to warn that this isn't a "return by ref" parameter
	typedef Vec2f_Ref		Vec2f_InOut;

	//================================================
	// VEC2
	//================================================

	class Vec2f
	{
		friend class Vec3f;
		friend class Vec4f;

	public:
		enum eZEROInitializer			{	ZERO,	};
		enum eFLT_LARGE_8Initializer	{	FLT_LARGE_8,	};
		enum eFLTMAXInitializer			{	FLTMAX,	};

	public:
		// vector constant generation syntax: Vec2f( Vec2f::ZERO )
		explicit Vec2f(eZEROInitializer);
		explicit Vec2f(eFLT_LARGE_8Initializer);
		explicit Vec2f(eFLTMAXInitializer);

		Vec2f();
		explicit Vec2f(float,float);
		explicit Vec2f(Vec::Vector_2_In);

		// PURPOSE: Resource constructor, for structures created from a resource
		// NOTES: This is here so that we can bypass the default ctor when using placement new, does not init vars.
		Vec2f(class datResource&) {}

		// PURPOSE: Used by the rorc resourcing system
		DECLARE_DUMMY_PLACE(Vec2f);

		// PURPOSE: Used by the rorc resourcing system
#if __DECLARESTRUCT
		void DeclareStruct(class datTypeStruct &s)
		{
			STRUCT_BEGIN(Vec2f);
			STRUCT_FIELD( v );
			STRUCT_END();
		}
#endif

		float GetX() const;
		float GetY() const;
		void SetX( float newX );
		void SetY( float newY );

		Vec::Vector_2_Out GetIntrin() const;
		Vec::Vector_2_Ref GetIntrinRef();
		Vec::Vector_2_ConstRef GetIntrinConstRef() const;

		void SetIntrin( Vec::Vector_2_In );

		void ZeroComponents();

		//============================================================================
		// Output

		void Print(bool newline=true) const;
		void PrintHex(bool newline=true) const;

		//============================================================================
		// Operators

		// Arithmetic
		Vec2f_Out		operator*	(Vec2f_In b) const; // per-element multiply!
		Vec2f_Out		operator*	(float b) const;
		friend Vec2f_Out operator*	(float a, Vec2f_In b);
		Vec2f_Out		operator/	(Vec2f_In b) const; // per-element divide!
		Vec2f_Out		operator/	(float b) const;
		Vec2f_Out		operator+	(Vec2f_In b) const;
		Vec2f_Out		operator-	(Vec2f_In b) const;
		void			operator*=	(Vec2f_In b);
		void			operator*=	(float b);
		void			operator/=	(Vec2f_In b);
		void			operator/=	(float b);
		void			operator+=	(Vec2f_In b);
		void			operator-=	(Vec2f_In b);
		Vec2f_Out		operator-	() const;

		// Bitwise
		Vec2f_Out		operator|	(Vec2f_In b) const;
		Vec2f_Out		operator&	(Vec2f_In b) const;
		Vec2f_Out		operator^	(Vec2f_In b) const;
		void			operator|=	(Vec2f_In b);
		void			operator&=	(Vec2f_In b);
		void			operator^=	(Vec2f_In b);

		// Element access.
		const float&	operator[]	(u32 elem) const;
		float&			operator[]	(u32 elem);

	private:
		Vec::Vector_2 v;
	};
} // namespace rage

#include "vec2f.inl"


#endif // VECTORMATH_VEC2F_H
