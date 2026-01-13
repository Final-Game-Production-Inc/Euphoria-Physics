#ifndef _INC_USERPARAMS_H_
#define _INC_USERPARAMS_H_

namespace rage {

/*
PURPOSE:

This class represents a chunk of memory containing arbitrary, typed, user data. Typical usage of this class
is in payloads, task and job params, etc.
*/

template <u32 Size>
class sysUserParams
{
private:

	u8	m_data[ Size ];

	template <typename Type>
	void SetValue(const Type value, u32 index)
	{
		Assertf( ( index + 1 ) * sizeof(Type) <= Size, "Cannot access the user param at given index" );

		Type*			typedData = reinterpret_cast< Type* >( m_data );
		typedData[ index ] = value;
	}

	template <typename Type>
	Type GetValue(u32 index) const
	{
		Assertf( ( index + 1 ) * sizeof(Type) <= Size, "Cannot access the user param at given index" );

		const Type*		typedData = reinterpret_cast< const Type* >( m_data );
		return typedData[ index ];
	}

public:

	void Reset()												{ memset( this, 0, Size ); }
	u32 GetSize() const											{ return Size; }

	void SetUint8(const u8 value, const u32 index = 0)			{ SetValue<u8>( value, index ); }
	void SetInt8(const s8 value, const u32 index = 0)			{ SetValue<s8>( value, index ); }
	void SetUint16(const u16 value, const u32 index = 0)		{ SetValue<u16>( value, index ); }
	void SetInt16(const s16 value, const u32 index = 0)			{ SetValue<s16>( value, index ); }
	void SetUint32(const u32 value, const u32 index = 0)		{ SetValue<u32>( value, index ); }
	void SetInt32(const s32 value, const u32 index = 0)			{ SetValue<s32>( value, index ); }
	void SetUint64(const u64 value, const u32 index = 0)		{ SetValue<u64>( value, index ); }
	void SetInt64(const s64 value, const u32 index = 0)			{ SetValue<s64>( value, index ); }
	void SetFloat(const float value, const u32 index = 0)		{ SetValue<float>( value, index ); }
	void SetDouble(const double value, const u32 index = 0)		{ SetValue<double>( value, index ); }
	void SetBool(const bool value, const u32 index = 0)			{ SetValue<bool>( value, index ); }
	void SetVoidPointer(const void* value, const u32 index = 0)	{ SetValue<const void*>( value, index ); }

	template <typename Type>
	void SetPointer(const Type* value, const u32 index = 0)		{ SetValue<const Type*>( value, index ); }

	u8 GetUint8(const u32 index = 0) const						{ return GetValue<u8>( index ); }
	s8 GetInt8(const u32 index = 0) const						{ return GetValue<s8>( index ); }
	u16 GetUint16(const u32 index = 0) const					{ return GetValue<u16>( index ); }
	s16 GetInt16(const u32 index = 0) const						{ return GetValue<s16>( index ); }
	u32 GetUint32(const u32 index = 0) const					{ return GetValue<u32>( index ); }
	s32 GetInt32(const u32 index = 0) const						{ return GetValue<s32>( index ); }
	u64 GetUint64(const u32 index = 0) const					{ return GetValue<u64>( index ); }
	s64 GetInt64(const u32 index = 0) const						{ return GetValue<s64>( index ); }
	float GetFloat(const u32 index = 0) const					{ return GetValue<float>( index ); }
	double GetDouble(const u32 index = 0) const					{ return GetValue<double>( index ); }
	bool GetBool(const u32 index = 0) const						{ return GetValue<bool>( index ); }
	void* GetVoidPointer(const u32 index = 0) const				{ return GetValue<void*>( index ); }

	template <typename Type>
	Type* GetPointer(const u32 index = 0) const					{ return GetValue<Type*>( index ); }
};

} // namespace rage

#endif // !defined _INC_USERPARAMS_H_
