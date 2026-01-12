
namespace rage
{
	__forceinline TransformV::TransformV()
	{
	}

	__forceinline TransformV::TransformV(eIDENTITYInitializer)
	{
		m_rot = Vec::V4VConstant(V_ZERO_WONE);
		m_pos = Vec::V4VConstant(V_ZERO);
	}

	__forceinline TransformV::TransformV(QuatV_In rot, Vec3V_In pos)
		:	m_rot( rot.GetIntrin128() ),
			m_pos( pos.GetIntrin128() )
	{
	}

	__forceinline TransformV::TransformV(TransformV_ConstRef a)
		:	m_rot( a.m_rot ),
			m_pos( a.m_pos )
	{
	}

	__forceinline TransformV_ConstRef TransformV::operator= (TransformV_ConstRef a)
	{
		m_rot = a.m_rot;
		m_pos = a.m_pos;
		return *this;
	}

#if __DECLARESTRUCT
	inline void TransformV::DeclareStruct(class datTypeStruct &s)
	{
		STRUCT_BEGIN(TransformV);
		STRUCT_FIELD( m_rot );
		STRUCT_FIELD( m_pos );
		STRUCT_END();
	}
#endif

	__forceinline QuatV_Out TransformV::GetRotation() const
	{
		return QuatV(m_rot);
	}

	__forceinline Vec3V_Out TransformV::GetPosition() const
	{
		return Vec3V(m_pos);
	}

	__forceinline QuatV_Ref TransformV::GetRotationRef()
	{
		return reinterpret_cast<QuatV_Ref>(m_rot);
	}

	__forceinline Vec3V_Ref TransformV::GetPositionRef()
	{
		return reinterpret_cast<Vec3V_Ref>(m_pos);
	}

	__forceinline QuatV_ConstRef TransformV::GetRotationConstRef() const
	{
		return reinterpret_cast<QuatV_ConstRef>(m_rot);
	}

	__forceinline Vec3V_ConstRef TransformV::GetPositionConstRef() const
	{
		return reinterpret_cast<Vec3V_ConstRef>(m_pos);
	}

	__forceinline void TransformV::SetRotation(QuatV_In q)
	{
		m_rot = q.GetIntrin128();
	}

	__forceinline void TransformV::SetPosition(Vec3V_In p)
	{
		m_pos = p.GetIntrin128();
	}

} // namespace rage
