#ifndef VECTORMATH_TRANFORMV_H
#define VECTORMATH_TRANFORMV_H

#include "data/struct.h"
#include "vectormath.h"
#include "vec3v.h"
#include "quatv.h"

namespace rage
{
	class TransformV;

	////================================================
	//// TYPES
	////================================================

	typedef TransformV			TransformV_Val;
	typedef TransformV*			TransformV_Ptr;
	typedef TransformV&			TransformV_Ref;
	typedef const TransformV&	TransformV_ConstRef;

	typedef TransformV_ConstRef		TransformV_In;
	typedef const TransformV_Val	TransformV_Out;	// "const" to warn that this isn't a "return by ref" parameter
	typedef TransformV_Ref			TransformV_InOut;

	//================================================
	// TransformV : rigid transform containing rotation and position
	//================================================

	class TransformV
	{

	public:
		TransformV();
		TransformV(TransformV_ConstRef);
		TransformV(class datResource&) {}
		explicit TransformV(eIDENTITYInitializer);
		explicit TransformV(QuatV_In rot, Vec3V_In pos);
		TransformV_ConstRef operator= (TransformV_ConstRef);
		DECLARE_DUMMY_PLACE(TransformV);

#if __DECLARESTRUCT
		void DeclareStruct(class datTypeStruct &s);
#endif

		//============================================================================
		// Getters / setters

		QuatV_Out GetRotation() const;
		Vec3V_Out GetPosition() const;
		QuatV_Ref GetRotationRef();
		Vec3V_Ref GetPositionRef();
		QuatV_ConstRef GetRotationConstRef() const;
		Vec3V_ConstRef GetPositionConstRef() const;

		void SetRotation(QuatV_In q);
		void SetPosition(Vec3V_In p);

	private:
		Vec::Vector_4V m_rot;
		Vec::Vector_4V m_pos;
	};

} // namespace rage

#include "transformv.inl"

#endif // VECTORMATH_TRANFORMV_H
