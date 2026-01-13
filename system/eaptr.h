#ifndef _INC_EAPTR_H_
#define _INC_EAPTR_H_

namespace rage {

#if RSG_ORBIS
#ifndef _SIZE_T_DECLARED
	typedef	unsigned long	size_t;
#define	_SIZE_T_DECLARED
#endif

//#include <sys/types.h>
//#include <stdint.h> 
#endif

/*
PURPOSE:

This class is just a simple wrapper around a pointer to main memory. It has very limited use in code running on
traditional CPUs (PPU, Xenon, PC...) but on SPU it's useful to *clearly* mark a pointer as being relative
to main memory. I.e. it's better to use

struct Object { ... };
sysEaPtr< Object >	myObj;

instead of:

u32			myObjEa;	// lacks any type information whatsoever
Object*		myObjEa;	// bad stuff happens if you accidentally deference it
*/

template <typename Type>
class sysEaPtr
{
private:

	Type*		m_ptr;

	void __CompileTimeAsserts() {
		CompileTimeAssert( sizeof( sysEaPtr<Type> ) == 4 );
	}

public:

	sysEaPtr()											{ }
	sysEaPtr(Type* ptr) : m_ptr( ptr )					{ }

	void Set(Type* ptr)									{ m_ptr = ptr; }

	size_t ToUint() const								{ return reinterpret_cast<size_t>( m_ptr ); }
	Type* ToPtr() const									{ return m_ptr; }
	
	sysEaPtr<Type> operator ++(s32)						{ Type* ret = m_ptr + 1; m_ptr++; return ret; }
	sysEaPtr<Type> operator +(const s32 op) const		{ return m_ptr + op; }
	sysEaPtr<Type>& operator +=(const s32 op)			{ m_ptr += op; return *this; }
	bool operator ==(const sysEaPtr<Type>& op) const	{ return m_ptr == op.m_ptr; }
	bool operator !=(const sysEaPtr<Type>& op) const	{ return m_ptr != op.m_ptr; }

	Type* operator ->() const							{ return m_ptr; }

#if !__SPU
	Type& operator *() const							{ return *m_ptr; }
#endif
};

} // namespace rage

#endif // !defined _INC_EAPTR_H_
