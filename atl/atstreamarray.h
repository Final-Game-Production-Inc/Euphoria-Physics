//
// 
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef ARRAY_STREAM_H
#define ARRAY_STREAM_H

#include "system/memory.h"
#include "system/cache.h"

namespace rage {

#ifdef __WIN32
	typedef Vector3 sixteenbytealign;
#else
	typedef __vector4 sixteenbytealign;
#endif

class atArrayStreamHeader
{
public:

	void Reset()
	{
		m_type = 0;
		m_count = 0;
		m_size = 0;
	}

	inline int GetCount() const
	{
		return m_count;
	}

	inline int GetType() const
	{
		return m_type;
	}

	inline unsigned int GetSize() const
	{
		return m_size;
	}

	inline void IncrementCount()
	{
		m_count++;
	}

	inline void DecrementCount()
	{
		m_count--;
	}

	int m_type;
	unsigned int m_size;
	int m_count;
	int m_pad;  // pad this out to 16 bytes since we allocated with 16-byte alignment
};

CompileTimeAssert( ((sizeof(atArrayStreamHeader) + 15) & ~15) == sizeof( atArrayStreamHeader ) );

class atArrayStream16
{
public:

	atArrayStream16(){ m_dataSize = 0; }

	~atArrayStream16()
	{ 
		ReleaseData(); 
	}

	bool Init( int dataSize )
	{
		m_dataSize = (dataSize + 15) & ~15;

		if( m_datahead )
		{
			m_current = m_datahead;
			m_currentHeader.Reset();
			m_currentHeaderPointer = NULL;
			m_nBlock = 0;
			return true;
		}
		else
		{
			return false;
		}
//		Displayf( "%d", m_dataSize );
	}

	void ReleaseData()
	{
		delete m_datahead;
		m_datahead = NULL;
		Reset();
	}

	inline bool IsEnd() const
	{
		FastAssert( m_iBlock <= m_nBlock ); 
//		return (( m_iBlock == m_nBlock ) && ( m_currentHeader.GetCount() == 0 ));
		return ( ((m_nBlock - m_iBlock) + m_currentHeader.m_count) == 0 );
	}

	// this headers reference count value will decrement as new elements are read
	inline const atArrayStreamHeader &ReadHeader()
	{
		FastAssert( m_currentHeader.GetCount() == 0 );

		m_currentHeader = *((atArrayStreamHeader*)(m_current));
		m_current++;
		m_iBlock++;

		PrefetchDC(m_current);

		return m_currentHeader;
	}

	inline void *ReadBlockElement()
	{
		FastAssert( m_currentHeader.GetCount() > 0 );
		m_currentHeader.m_count--;
		void *el = (void *)(m_current);

		int sze = m_currentHeader.GetSize();
		sze = (sze + 15) & ~15;
		const int incr = (sze >> 4);
		m_current += incr;

		PrefetchDC(m_current);
		PrefetchDC(m_current+incr);

		return el;
	}

	inline void ReadBlockElement( void *to )
	{
		FastAssert( m_currentHeader.GetCount() > 0 );
		m_currentHeader.DecrementCount();

		int sze = m_currentHeader.GetSize();

		sysMemCpy( to, m_current, sze );

		sze = (sze + 15) & ~15;

		m_current += (sze >> 4);
	}

	inline void *ReadElement()
	{
		if( !IsEnd() )
		{
			if( m_currentHeader.GetCount() == 0 )
			{
				ReadHeader();
				AssertMsg( ( m_currentHeader.GetCount() != 0 ) , "zero elements in block" );
			}

			return ReadBlockElement();
		}
		else
		{
			return NULL;
		}
	}

	inline int GetElementsInCurrentBlockCount() const
	{
		return m_currentHeader.GetCount();
	}


	void Reset()
	{
		m_current = m_datahead;

		m_currentHeader.Reset();
		m_iBlock = 0;
	}

	// creates a new header
	void StartBlock( int typeID, int typeSize )
	{
		FastAssert( (((int)(m_current)) - ((int)(m_datahead)) + (sizeof(atArrayStreamHeader) + 15) & ~15) <= m_dataSize );

		m_nBlock++;

		m_currentHeader.m_count = 0;
		m_currentHeader.m_size = typeSize;
		m_currentHeader.m_type = typeID;

		m_currentHeaderPointer = (atArrayStreamHeader *)(m_current);

		*m_currentHeaderPointer = m_currentHeader;

		m_current++;

	}

	inline int GetCurrentType() const
	{
		return m_currentHeader.GetType();
	}

	inline void Prefetch() const
	{
		PrefetchDC( m_current );
	}

	inline void Prefetch2() const
	{
		PrefetchDC( m_current );
		PrefetchDC( m_current + (128/16) );
	}

	inline void Prefetch3() const
	{
		PrefetchDC( m_current );
		PrefetchDC( m_current + (128/16) );
		PrefetchDC( m_current + (256/16) );
	}

	inline void Prefetch4() const
	{
		PrefetchDC( m_current );
		PrefetchDC( m_current + (128/16) );
		PrefetchDC( m_current + (256/16) );
		PrefetchDC( m_current + (384/16) );
	}


	void AddElement( void *element, int typeID, int typeSize )
	{
		if( GetCurrentType() != typeID )
		{
			StartBlock(typeID, typeSize);
		}

		unsigned int sze = m_currentHeader.GetSize();

		FastAssert( (((int)(m_current)) - ((int)(m_datahead)) + (sze + 15) & ~15) <= m_dataSize );

		m_currentHeaderPointer->IncrementCount();
		// let's keep these in sync
		m_currentHeader.IncrementCount();  
		
		sysMemCpy( m_current, element, sze );

		sze = (sze + 15) & ~15;

		m_current += (sze >> 4);

	}

	int m_nBlock;
	int m_iBlock;
	unsigned int m_dataSize;

	atArrayStreamHeader *m_currentHeaderPointer;
	// the count member will be decremented as we read through the stream
	atArrayStreamHeader m_currentHeader;

	sixteenbytealign *m_current;
	sixteenbytealign *m_datahead;

};

}

#endif // ARRAY_STREAM_H
