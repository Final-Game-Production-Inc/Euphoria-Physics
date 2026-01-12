// 
// data/datGrowBuffer.cpp
// 
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved. 
// 

#include "growbuffer.h"

#include "file/device.h"
#include "system/memops.h"
#include "system/memory.h"
#include "system/new.h"

#include <string.h>

using namespace rage;

datGrowBuffer::datGrowBuffer()
: m_Buf(NULL)
, m_Capacity(0)
, m_Len(0)
, m_Flags(0)
, m_Allocator(NULL)
, m_FiHandle(fiHandleInvalid)
, m_growIncr(DEFAULT_GROW_INCR)
ASSERT_ONLY(, m_Initialized(false))
{
    this->Clear();
}

datGrowBuffer::~datGrowBuffer()
{
	this->Clear();
	ASSERT_ONLY(m_Initialized = false);
}

void
datGrowBuffer::Init(sysMemAllocator* allocator, const unsigned flags, const unsigned growIncr /*= DEFAULT_GROW_INCR*/)
{
	this->Clear();
	m_Flags = flags & ~FIXED_BUFFER;
	m_Allocator = allocator;
	m_growIncr = growIncr;
	Assertf(m_growIncr > 0 && (m_growIncr&(m_growIncr-1))==0, "%d is an invalid grow increment.  Must be power of 2", m_growIncr);
	ASSERT_ONLY(m_Initialized = true);
}

void
datGrowBuffer::Init(void* buffer,
				 const unsigned bufLen,
				 const unsigned flags)
{
	this->Clear();
	m_Buf = (u8*) buffer;
	m_Capacity = bufLen;
	m_Flags = flags | FIXED_BUFFER;
	m_Allocator = NULL;
	Assertf(m_growIncr > 0 && (m_growIncr&(m_growIncr-1))==0, "%d is an invalid grow increment.  Must be power of 2", m_growIncr);
	ASSERT_ONLY(m_Initialized = true);
}

void
datGrowBuffer::InitReadOnly(const void* buffer,
				            const unsigned bufLen)
{
	this->Clear();
	m_Buf = (u8*) buffer;
	m_Capacity = m_Len = bufLen;
	m_Flags = READ_ONLY | FIXED_BUFFER;
	m_Allocator = NULL;
	Assertf(m_growIncr > 0 && (m_growIncr&(m_growIncr-1))==0, "%d is an invalid grow increment.  Must be power of 2", m_growIncr);
	ASSERT_ONLY(m_Initialized = true);
}

void
datGrowBuffer::Clear()
{
	if(!(m_Flags & FIXED_BUFFER))
	{
		if(m_Buf)
		{
			this->Free(m_Buf);
		}

		m_Buf = NULL;
		m_Capacity = 0;
	}

	m_Len = 0;

    if(fiHandleInvalid != m_FiHandle)
    {
        this->GetFiDevice()->Close(m_FiHandle);
        m_FiHandle = fiHandleInvalid;
    }
}

void
datGrowBuffer::Reset()
{
	m_Len = 0;
}

fiHandle
datGrowBuffer::GetFiHandle()
{
    if(fiHandleInvalid == m_FiHandle)
    {
        char fname[64];
        fiDevice::MakeGrowBufferFileName(fname, sizeof(fname), this);
        m_FiHandle = this->GetFiDevice()->Create(fname);
    }

    return m_FiHandle;
}

const fiDevice*
datGrowBuffer::GetFiDevice()
{
    return &fiDeviceGrowBuffer::GetInstance();
}

unsigned
datGrowBuffer::GetFlags() const
{
	return m_Flags;
}

int
datGrowBuffer::Append(const void* buf, const unsigned bufLen)
{
	Assertf(m_Initialized, "datGrowBuffer::Append - not initialized");

	Assertf(!m_Buf || 
		(const u8*)buf >= (&m_Buf[0] + m_Len) || 
		((const u8*)buf + bufLen) <= &m_Buf[0], "Can't Append() a prior part of the grow buffer");

    int numChars = 0;

	if(bufLen)
	{
        const int nullTerm = (m_Flags & NULL_TERMINATE) ? 1 : 0;
		const int requiredCapacity = m_Len + bufLen + nullTerm;

		if(this->Preallocate(requiredCapacity))
		{
            numChars = (int)bufLen;
		}
        else if(m_Capacity > 0 && m_Len < (m_Capacity - nullTerm))
        {
            numChars = int(m_Capacity - m_Len - nullTerm);
        }

        Assert(numChars >= 0);
        Assert(m_Len + numChars + nullTerm <= m_Capacity);

		if(numChars > 0 && !(m_Flags & NULL_BUFFER))
		{
			sysMemCpy(&m_Buf[m_Len], buf, numChars);
    		m_Len += numChars;
			if(nullTerm)
			{
                Assert(m_Len < m_Capacity);
				m_Buf[m_Len] = '\0';
			}
		}

	}

	return numChars;
}

int
datGrowBuffer::Prepend(const void* buf, const unsigned bufLen)
{
	Assertf(m_Initialized, "datGrowBuffer::Prepend - not initialized");

	Assertf(!m_Buf || 
		(const u8*)buf >= (&m_Buf[0] + m_Len) || 
		((const u8*)buf + bufLen) <= &m_Buf[0], "Can't Prepend() a prior part of the grow buffer");

    int numChars = 0;

	if(bufLen)
	{
        const int nullTerm = (m_Flags & NULL_TERMINATE) ? 1 : 0;
		const int requiredCapacity = m_Len + bufLen + nullTerm;

		if(this->Preallocate(requiredCapacity))
		{
            numChars = (int)bufLen;
		}
        else if(m_Capacity > 0 && m_Len < (m_Capacity - nullTerm))
        {
            numChars = int(m_Capacity - m_Len - nullTerm);
        }

        Assert(numChars >= 0);
        Assert(m_Len + numChars + nullTerm <= m_Capacity);

		if(numChars > 0 && !(m_Flags & NULL_BUFFER))
		{
			if(m_Len)
			{
				//Shift the current buffer to make room for the
				//new chars.
				memmove(&m_Buf[numChars], m_Buf, m_Len);
			}

			sysMemCpy(m_Buf, buf, numChars);
    		m_Len += numChars;
			if(m_Flags & NULL_TERMINATE)
			{
                Assert(m_Len < m_Capacity);
				m_Buf[m_Len] = '\0';
			}
		}
	}

	return numChars;
}

bool
datGrowBuffer::AppendOrFail(const void* buf, const unsigned bufLen)
{
	Assertf(m_Initialized, "datGrowBuffer::AppendOrFail - not initialized");

    const int nullTerm = (m_Flags & NULL_TERMINATE) ? 1 : 0;
	const int requiredCapacity = m_Len + bufLen + nullTerm;
    return this->Preallocate(requiredCapacity)
            && AssertVerify(this->Append(buf, bufLen) == (int)bufLen);
}

bool
datGrowBuffer::PrependOrFail(const void* buf, const unsigned bufLen)
{
	Assertf(m_Initialized, "datGrowBuffer::PrependOrFail - not initialized");

    const int nullTerm = (m_Flags & NULL_TERMINATE) ? 1 : 0;
	const int requiredCapacity = m_Len + bufLen + nullTerm;
    return this->Preallocate(requiredCapacity)
            && AssertVerify(this->Prepend(buf, bufLen) == (int)bufLen);
}

void
datGrowBuffer::Truncate(const unsigned newLen)
{
	Assertf(m_Initialized, "datGrowBuffer::Truncate - not initialized");

	if(Verifyf(newLen <= this->Length(), "Can't truncate to %d, length is already %d", this->Length(), newLen))
	{
		m_Len = newLen;
		if(m_Flags & NULL_TERMINATE)
		{
            Assert(m_Len < m_Capacity);
			m_Buf[m_Len] = '\0';
		}
	}
}

void
datGrowBuffer::Remove(const int start, const unsigned len)
{
	Assertf(m_Initialized, "datGrowBuffer::Remove - not initialized");

    if(!AssertVerify(IsWritable()))
    {
        return;
    }

	datGrowBuffer::Remove(m_Buf, &m_Len, start, len);

	if(m_Flags & NULL_TERMINATE)
	{
        Assert(m_Len < m_Capacity);
		m_Buf[m_Len] = '\0';
	}
}

unsigned
datGrowBuffer::Consume(void* buf, const unsigned bufLen)
{
	Assertf(m_Initialized, "datGrowBuffer::Consume - not initialized");

    if(!AssertVerify(IsWritable()))
    {
        return 0;
    }

	unsigned count = 0;

	if(AssertVerify(m_Buf))
	{
		count = (m_Len > bufLen) ? bufLen : m_Len;
		sysMemCpy(buf, m_Buf, count);
		this->Remove(0, count);
	}

	return count;
}

//Make sure GROW_INCR is power of 2.
CompileTimeAssert(0 != datGrowBuffer::DEFAULT_GROW_INCR);
CompileTimeAssert((datGrowBuffer::DEFAULT_GROW_INCR&(datGrowBuffer::DEFAULT_GROW_INCR-1))==0);

#define DBUF_CAPACITY(len, incr) (((len) + (incr-1)) & ~(incr-1))

bool
datGrowBuffer::Preallocate(const unsigned newCapacity)
{
	Assertf(m_Initialized, "datGrowBuffer::Preallocate - not initialized");

    if(!AssertVerify(IsWritable()))
    {
        return false;
    }

	bool success = false;

	if(newCapacity > m_Capacity)
	{
		const unsigned nullterm = ((m_Flags & NULL_TERMINATE) && !(m_Flags & NULL_BUFFER)) ? 1 : 0;
		const unsigned newCap = DBUF_CAPACITY(newCapacity + nullterm, m_growIncr);

		if(!this->IsGrowable())
		{
			//Error
		}
		else if(m_Flags & NULL_BUFFER)
		{
			m_Capacity = newCap - nullterm;  //minus to account for the null terminator when present.
			success = true;
		}
		else
		{
			u8* newBuf = (u8*) this->Allocate(newCap);

			if(newBuf)
			{
				if(m_Buf)
				{
					sysMemCpy(newBuf, m_Buf, m_Len);
					this->Free(m_Buf);
				}
				m_Buf = newBuf;
				m_Capacity = newCap - nullterm;  //minus to account for the null terminator when present.

				success = true;
			}
		}
	}
	else
	{
		success = true;
	}

	return success;
}

void*
datGrowBuffer::GetBuffer()
{
	return m_Len ? m_Buf : NULL;
}

const void*
datGrowBuffer::GetBuffer() const
{
	return m_Len ? m_Buf : NULL;
}

unsigned
datGrowBuffer::Length() const
{
	return m_Len;
}

unsigned
datGrowBuffer::GetCapacity() const
{
	return m_Capacity;
}

bool
datGrowBuffer::IsGrowable() const
{
	return 0 == (m_Flags & FIXED_BUFFER);
}

bool
datGrowBuffer::IsReadOnly() const
{
	return 0 != (m_Flags & READ_ONLY);
}

bool
datGrowBuffer::IsWritable() const
{
	return 0 == (m_Flags & READ_ONLY);
}

void
datGrowBuffer::Remove(void* buffer, unsigned* bufLen, const unsigned start, const unsigned len)
{
	if(Verifyf(start <= *bufLen, "Can't remove byte %d, buffer only has %d bytes", start, *bufLen) && len > 0)
	{
		u8* p = (u8*) buffer;

		if(start + len >= *bufLen)
		{
			//Truncate
			*bufLen = start;
		}
		else
		{
			memmove(&p[start], &p[start+len], *bufLen - (start + len));
			*bufLen -= len;
		}
	}
}

//private:

void*
datGrowBuffer::Allocate(const unsigned size)
{
	Assertf(m_Initialized, "datGrowBuffer::Allocate - not initialized");

	Assert(this->IsGrowable());

	if(m_Allocator)
	{
		return m_Allocator->TryLoggedAllocate(size, sizeof(void*), 0, __FILE__, __LINE__);
	}
	else
	{
		return rage_new u8[size];
	}
}

void
datGrowBuffer::Free(void* data)
{
	Assertf(m_Initialized, "datGrowBuffer::Free - not initialized");

	Assert(this->IsGrowable());

	if(m_Allocator)
	{
		m_Allocator->Free(data);
	}
	else
	{
		delete[] (u8*) data;
	}
}

