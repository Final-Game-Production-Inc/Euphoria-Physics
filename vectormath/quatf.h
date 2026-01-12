#ifndef VECTORMATH_QUATF_H
#define VECTORMATH_QUATF_H

#include "data/struct.h"
#include "vectormath.h"
#include "vec4f.h"
#include "vec3f.h"
#include "vec2f.h"

namespace rage
{
	class Quatf;

	//================================================
	// TYPES
	//================================================

	typedef Quatf			Quatf_Val;
	typedef Quatf*			Quatf_Ptr;
	typedef Quatf&			Quatf_Ref;
	typedef const Quatf&		Quatf_ConstRef;

#if __WIN32PC
	typedef Quatf_ConstRef	Quatf_In;
#else
	typedef Quatf_Val		Quatf_In;
#endif
	typedef const Quatf_Val	Quatf_Out;	// "const" to warn that this isn't a "return by ref" parameter	
	typedef Quatf_Ref		Quatf_InOut;


	//================================================
	// QUAT
	//================================================

	class Quatf
	{
	public:
		enum eZEROInitializer			{	ZERO,	};
		enum eFLT_LARGE_8Initializer	{	FLT_LARGE_8,	};
		enum eFLT_EPSInitializer		{	FLT_EPS,	};

	public:
		// quat constant generation syntax: Quatf( Quatf::ZERO )
		explicit Quatf(eZEROInitializer);
		explicit Quatf(eFLT_LARGE_8Initializer);
		explicit Quatf(eFLT_EPSInitializer);

		Quatf();
		explicit Quatf(float,float,float,float);
		explicit Quatf(Vec::Vector_4_In);

		// PURPOSE: Resource constructor, for structures created from a resource
		// NOTES: This is here so that we can bypass the default ctor when using placement new, does not init vars.
		Quatf(class datResource&) {}

		// PURPOSE: Used by the rorc resourcing system
		DECLARE_DUMMY_PLACE(Quatf);

		// PURPOSE: Used by the rorc resourcing system
#if __DECLARESTRUCT
		void DeclareStruct(class datTypeStruct &s)
		{
			STRUCT_BEGIN(Quatf);
			STRUCT_FIELD( v );
			STRUCT_END();
		}
#endif

		float GetX() const;
		float GetY() const;
		float GetZ() const;
		float GetW() const;
		Vec3f_Out GetXYZ() const;

		Vec::Vector_4_Ref GetIntrinRef();
		Vec::Vector_4_Out GetIntrin() const;
		Vec::Vector_4_ConstRef GetIntrinConstRef() const;
		
		void SetIntrin( Vec::Vector_4_In );

		void SetX( float newX );
		void SetY( float newY );
		void SetZ( float newZ );
		void SetW( float newW );
		void SetXYZ( Vec3f_In newXYZ );

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

		// Element access.
		const float&	operator[]	(u32 elem) const;
		float&			operator[]	(u32 elem);

	private:
		Vec::Vector_4 v;
	};
} // namespace rage

#include "quatf.inl"


#endif // VECTORMATH_QUATF_H
