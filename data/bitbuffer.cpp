// 
// data/bitbuffer.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#include "bitbuffer.h"

#include <string.h>
#include "system/memops.h"

namespace rage
{

using namespace datbitbuffer;

void
datBitBuffer::ReadUnsigned(const void* bits,
                            unsigned& u,
                            const int numBits,
                            const int srcBitOffset)
{
    Assert(numBits >= 0 && numBits <= IntTraits< unsigned >::NumBits);
    Assert(srcBitOffset >= 0);

	const int firstByte = srcBitOffset >> 3;
	
    const u8* src = &((const u8*) bits)[firstByte];
    const int bit = srcBitOffset & 0x07;

	int curByte = 0;
	const int lastByte = ((srcBitOffset + ((numBits > 0) ? numBits - 1 : 0)) >> 3) - firstByte;

    unsigned val;

    //Get the first few bits
    val = u8(src[curByte++] << bit);

    for(int i = 8; (i < numBits) && (curByte <= lastByte); i += 8)
    {
        //Shift current bits left and
        //put the next 8 bits in our LSB
        val = (val << 8) | (src[curByte++] << bit);
    }

    //Get the last few bits

	// make sure we don't read past the end of the buffer,
	// which causes an exception when using the sparse allocator.
	if(curByte <= lastByte)
	{
		val |= (src[curByte] >> (8 - bit));
	}

    //The bits are currently shifted all the way left.
    //Shift them down so the LSB is where it should be.
    val >>= ((numBits + 7) & ~0x07) - numBits;

    u = val;
}

void
datBitBuffer::WriteUnsigned(void* bits,
                             const unsigned u,
                             const int numBits,
                             const int dstBitOffset)
{
    Assert(numBits >= 0 && numBits <= IntTraits< unsigned >::NumBits);
    Assert(dstBitOffset >= 0);

    //For optimal code use the native machine word for all operations.

    static const int MAX_BITS   = IntTraits< unsigned >::NumBits;

	Assertf(u < (1u << numBits) || numBits == MAX_BITS,"... u[%d] < (1u << numBits[%d])[%d] || numBits[%d] == MAX_BITS[%d]",u,numBits,(1u << numBits),numBits,MAX_BITS);

    static const unsigned TOP8_BITS_SHIFT = MAX_BITS - 8;

    u8* dst = &((u8*) bits)[dstBitOffset >> 3];
    const int bit = dstBitOffset & 0x07;

    //Shift the bits all the way to the left.
    unsigned val = u << (MAX_BITS - numBits);

    //Mask with one bits corresponding to bits in the destination that 
    //that will be written.  The complement of this mask will be used
    //when writing to the destination to clear/preserve appropriate bits.
    unsigned dstMask = (~0u << (MAX_BITS - numBits));

    //Write the first few bits.
    *dst = u8((*dst & ~((dstMask >> TOP8_BITS_SHIFT) >> bit)) |
               ((val >> TOP8_BITS_SHIFT) >> bit));
    ++dst;

    //Shift away the bits we've written.
    val <<= (8 - bit);
    dstMask <<= (8 - bit);

    for(int i = 8 - bit; i < numBits; i += 8)
    {
        //Write the next 8 bits
        *dst = u8((*dst & ~(dstMask >> TOP8_BITS_SHIFT)) | (val >> TOP8_BITS_SHIFT));
        ++dst;

        //Shift away the bits we've written.
        val <<= 8;
        dstMask <<= 8;
    }
}

void
datBitBuffer::CopyBits(void* dst,
                        const void* src,
                        const int numBits,
                        const int dstBitOffset,
                        const int srcBitOffset)
{
    const u8* s = &((const u8*) src)[srcBitOffset >> 3];
    int curDstBit = dstBitOffset;
    const int srcBit = srcBitOffset & 0x07;
    int bitCount = numBits;

    if(srcBit)
    {
        int numBitsToWrite = 8 - srcBit;

        if(numBitsToWrite > numBits) numBitsToWrite = numBits;

        datBitBuffer::WriteUnsigned(dst, 
                                unsigned((0xFF & (*s << srcBit)) >> (8 - numBitsToWrite)),
                                numBitsToWrite,
                                curDstBit);

        ++s;

        bitCount -= numBitsToWrite;
        curDstBit += numBitsToWrite;
    }

    if(bitCount > 0)
    {
        const int numBytes = bitCount >> 3;

        if(numBytes > 0)
        {
            u8* d = &((u8*) dst)[curDstBit >> 3];
            const int bit = curDstBit & 0x07;

            if(bit)
            {
                const int upshift = 8 - bit;
                const int downshift = bit;
                const unsigned upmask = 0xFF << upshift;
                const unsigned downmask = 0xFF >> downshift;

                for(int i = 0; i < numBytes; ++i, ++s)
                {
                    *d = u8((*d & upmask) | (*s >> downshift));
                    ++d;
                    *d = u8((*d & downmask) | (*s << upshift));
                }
            }
            else
            {
                sysMemCpy(d, s, numBytes);
                s += numBytes;
            }

            curDstBit += numBytes << 3;

            const int leftOver = bitCount & 0x07;

            if(leftOver)
            {
                datBitBuffer::WriteUnsigned(dst,
                                        unsigned(*s >> (8 - leftOver)),
                                        leftOver,
                                        curDstBit);
            }
        }
        else
        {
            datBitBuffer::WriteUnsigned(dst,
                                    unsigned(*s >> (8 - bitCount)),
                                    bitCount,
                                    curDstBit);
        }
    }
}

int
datBitBuffer::StrBitLen(const int strLen)
{
    const int numChars = strLen + 1;
    int numBits = numChars << 3;

    if(numChars > SMALL_CHAR_COUNT_MAX_CHARS)
    {
        numBits += BIG_CHAR_COUNT_BITS + 1;
    }
    else
    {
        numBits += SMALL_CHAR_COUNT_BITS + 1;
    }

    return numBits;
}

int
datBitBuffer::StrBitLen(const char* str)
{
    return datBitBuffer::StrBitLen((int)::strlen(str));
}

datBitBuffer::datBitBuffer()
{
    this->Reset();
}

void
datBitBuffer::Reset()
{
    m_ReadBits = 0;
    m_BaseBitOffset = 0;
    m_MaxBits = 0;
    m_CursorPos = 0;
    m_NumBitsWritten = 0;
    m_NumBitsRead = 0;
    m_IsReadonly = false;
    m_IsNullWrite = false;
}

void
datBitBuffer::Clear()
{
    if(this->IsReadOnly())
    {
        this->SetReadOnlyBits(this->GetReadOnlyBits(),
                               this->GetMaxBits(),
                               this->GetBaseBitOffset());
    }
    else
    {
        this->SetReadWriteBits(this->GetReadWriteBits(),
                                this->GetMaxBits(),
                                this->GetBaseBitOffset());
    }
}

bool
datBitBuffer::IsWritable() const
{
    return !m_IsReadonly && m_MaxBits > 0;
}

void
datBitBuffer::SetReadOnlyBits(const void* bits,
                               const int maxBits,
                               const int baseBitOffset)
{
    Assert(bits || 0 == maxBits);
    Assert(maxBits >= 0);
    Assert(baseBitOffset >= 0);

    this->Reset();
    m_ReadBits = (const u8*) bits;
    m_MaxBits = maxBits;
    m_BaseBitOffset = baseBitOffset;
    m_IsReadonly = true;
}

void
datBitBuffer::SetReadOnlyBytes(const void* bytes, const int numBytes)
{
    this->SetReadOnlyBits(bytes, numBytes << 3, 0);
}

void
datBitBuffer::SetReadWriteBits(void* bits,
                             const int maxBits,
                             const int baseBitOffset)
{
    Assert(bits || 0 == maxBits);
    Assert(maxBits >= 0);
    Assert(baseBitOffset >= 0);

    this->Reset();
    m_WriteBits = (u8*) bits;
    m_MaxBits = maxBits;
    m_BaseBitOffset = baseBitOffset;
}

void
datBitBuffer::SetReadWriteBytes(void* bytes, const int numBytes)
{
    this->SetReadWriteBits(bytes, numBytes << 3, 0);
}

void
datBitBuffer::SetNullWriteBytes(const int maxBytes)
{
    Assert(maxBytes >= 0);

    this->Reset();
    m_MaxBits = maxBytes << 3;
    Assert(m_MaxBits > maxBytes);
    m_IsNullWrite = true;
}

const void*
datBitBuffer::GetReadOnlyBits() const
{
    return m_ReadBits;
}

void*
datBitBuffer::GetReadWriteBits()
{
    return (this->IsReadOnly() || this->IsNullWrite()) ? NULL : m_WriteBits;
}

const void*
datBitBuffer::GetReadWriteBits() const
{
    return (this->IsReadOnly() || this->IsNullWrite()) ? NULL : m_WriteBits;
}

int
datBitBuffer::GetBaseBitOffset() const
{
    return m_BaseBitOffset;
}

int
datBitBuffer::GetByteLength() const
{
    return (this->GetBitLength() + 7) >> 3;
}

int
datBitBuffer::GetNumBytesWritten() const
{
    return (this->GetNumBitsWritten() + 7) >> 3;
}

bool
datBitBuffer::SetNumBitsWritten(const int numBitsWritten)
{
    if(AssertVerify(!this->IsReadOnly()
                      && numBitsWritten >= 0
                      && numBitsWritten <= m_MaxBits))
    {
        m_NumBitsWritten = numBitsWritten;
        this->SetCursorPos(numBitsWritten);
        return true;
    }

    return false;
}

int
datBitBuffer::GetNumBytesRead() const
{
    return (this->GetNumBitsRead() + 7) >> 3;
}

bool
datBitBuffer::SetNumBitsRead(const int numBitsRead) const
{
    if(AssertVerify(!this->IsNullWrite())
        && AssertVerify(numBitsRead >= 0 && numBitsRead <= m_MaxBits))
    {
        m_NumBitsRead = numBitsRead;
        this->SetCursorPos(numBitsRead);
        return true;
    }

    return false;
}

bool
datBitBuffer::CanReadBytes(const int numBytes) const
{
    return this->CanReadBits(numBytes << 3);
}

bool
datBitBuffer::CanWriteBytes(const int numBytes) const
{
    return this->CanWriteBits(numBytes << 3);
}

bool
datBitBuffer::CanWriteStr(const char* str) const
{
    return this->CanWriteBits(StrBitLen(str));
}

bool
datBitBuffer::SetCursorPos(const int pos) const
{
    if(AssertVerify(pos >= 0 && pos <= this->GetBitLength()))
    {
        m_CursorPos = pos;
        return true;
    }

    return false;
}

int
datBitBuffer::GetCursorPos() const
{
    return m_CursorPos;
}

bool
datBitBuffer::SkipBits(const int numBits) const
{
    const int newPos = this->GetCursorPos() + numBits;
    return this->SetCursorPos(newPos);
}

bool
datBitBuffer::SkipBytes(const int numBytes) const
{
    return this->SkipBits(numBytes << 3);
}

bool
datBitBuffer::ReadBits(void* dst,
                       const int numBits,
                       const int dstBitOffset) const
{
    Assert(numBits > 0);

    if(numBits && /*AssertVerify*/(this->CanReadBits(numBits)))
    {
        datBitBuffer::CopyBits(dst,
                           &m_ReadBits[m_BaseBitOffset >> 3],
                           numBits,
                           dstBitOffset,
                           m_CursorPos + (m_BaseBitOffset & 0x07)); 

        this->BumpReadCursor(numBits);

        return true;
    }

    return false;
}

bool
datBitBuffer::ReadBits(datBitBuffer& dst, const int numBits) const
{
    const int offset = dst.GetBaseBitOffset() + dst.GetCursorPos();
    return
        /*AssertVerify*/(dst.CanWriteBits(numBits))
        && (!numBits || /*AssertVerify*/(this->ReadBits(dst.GetReadWriteBits(),
                                                    numBits,
                                                    offset)))
        && (dst.BumpReadWriteCursor(numBits), true);
}

bool
datBitBuffer::WriteBits(const void* src,
                         const int numBits,
                         const int srcBitOffset)
{
    Assert(numBits > 0);
    Assert(!m_IsReadonly);

    if(AssertVerify(this->CanWriteBits(numBits)))
    {
        if(!m_IsNullWrite)
        {
            datBitBuffer::CopyBits(&m_WriteBits[m_BaseBitOffset >> 3],
                               src,
                               numBits,
                               m_CursorPos + (m_BaseBitOffset & 0x07),
                               srcBitOffset); 
        }

        this->BumpReadWriteCursor(numBits);

        return true;
    }

    return false;
}

bool
datBitBuffer::WriteBits(const datBitBuffer& src, const int numBits)
{
    return src.ReadBits(*this, numBits);
}

bool
datBitBuffer::ReadUns(u64& u, const int numBits) const
{
    bool success;

    static const int NUM_UNS_BITS = datbitbuffer::IntTraits< unsigned >::NumBits;

    if(numBits > NUM_UNS_BITS)
    {
        unsigned hi = 0, lo = 0;

        success = this->ReadUns(lo, NUM_UNS_BITS)
                  && this->ReadUns(hi, numBits - NUM_UNS_BITS);

        u = (u64(hi) << NUM_UNS_BITS) | lo;
    }
    else
    {
        unsigned lo = 0;

        success = this->ReadUns(lo, numBits);

        u = lo;
    }

    return success;
}

bool
datBitBuffer::WriteUns(const u64 u, const int numBits)
{
    bool success;

    static const int NUM_UNS_BITS =
        datbitbuffer::IntTraits< unsigned >::NumBits;

    if(numBits > NUM_UNS_BITS )
    {
        success = this->WriteUns(unsigned(u), NUM_UNS_BITS)
                  && this->WriteUns(unsigned(u >> NUM_UNS_BITS), numBits - NUM_UNS_BITS);
    }
    else
    {
        success = this->WriteUns((unsigned) u, numBits);
    }

    return success;
}

bool
datBitBuffer::ReadFloat(float& f) const
{
    CompileTimeAssert(sizeof(float) == sizeof(unsigned));
    union { float f; unsigned u; } tmp;
    const bool success = this->ReadUns(tmp.u, sizeof(tmp.u) << 3);
    f = tmp.f;
    return success;
}

bool
datBitBuffer::WriteFloat(const float f)
{
    CompileTimeAssert(sizeof(float) == sizeof(unsigned));
    union { float f; unsigned u; } tmp;
    tmp.f = f;
    return this->WriteUns(tmp.u, sizeof(tmp.u) << 3);
}

bool
datBitBuffer::ReadDouble(double& d) const
{
    CompileTimeAssert(sizeof(double) == sizeof(u64));
    union { double d; u64 u; } tmp;
    const bool success = this->ReadUns(tmp.u, sizeof(tmp.u) << 3);
    d = tmp.d;
    return success;
}

bool
datBitBuffer::WriteDouble(const double d)
{
    CompileTimeAssert(sizeof(double) == sizeof(u64));
    union { double d; u64 u; } tmp;
    tmp.d = d;
    return this->WriteUns(tmp.u, sizeof(tmp.u) << 3);
}

bool
datBitBuffer::ReadBool(bool& b) const
{
    unsigned u = 0;
    const bool success = this->ReadUns(u, 1);
    b = success && (0 != u);
    return success;
}

bool
datBitBuffer::WriteBool(const bool b)
{
	return b ? this->WriteUns(1, 1) : this->WriteUns(0, 1);
}

bool
datBitBuffer::ReadBytes(void* bytes, const int numBytes) const
{
    return this->ReadBits(bytes, numBytes << 3, 0);
}

bool
datBitBuffer::WriteBytes(const void* bytes, const int numBytes)
{
    return this->WriteBits(bytes, numBytes << 3, 0);
}

bool
datBitBuffer::ReadStr(char* str, const int maxChars) const
{
    CompileTimeAssert(sizeof(char) == sizeof(u8));

    bool success = false;

    Assert(maxChars <= BIG_CHAR_COUNT_MAX_CHARS);

    str[0] = '\0';

    unsigned flag;

    if(/*AssertVerify*/(this->ReadUns(flag, 1)))
    {
        unsigned numChars = 0;
        int charCountBits;//, charCountBytes;

        if(flag)
        {
            charCountBits = BIG_CHAR_COUNT_BITS;
            // charCountBytes = BIG_CHAR_COUNT_BYTES;
        }
        else
        {
            charCountBits = SMALL_CHAR_COUNT_BITS;
            // charCountBytes = SMALL_CHAR_COUNT_BYTES;
        }

        success = /*AssertVerify*/(this->ReadUns(numChars, charCountBits))
            && /*AssertVerify*/(numChars <= (unsigned) maxChars);

        if(success && numChars)
        {
            success =
                /*AssertVerify*/(this->ReadBytes(str, numChars))
                && /*AssertVerify*/('\0' == str[numChars - 1]);
        }
    }

    return success;
}

bool
datBitBuffer::WriteStr(const char* str, const int maxChars)
{
    CompileTimeAssert(sizeof(char) == sizeof(u8));

    bool success = false;

    Assert(maxChars <= BIG_CHAR_COUNT_MAX_CHARS);

    int numChars = (int)::strlen(str) + 1;

    if(numChars > maxChars)
    {
        numChars = maxChars;
    }

    int charCountBits; // , charCountBytes;
    unsigned isBig;

    if(numChars > SMALL_CHAR_COUNT_MAX_CHARS)
    {
        charCountBits = BIG_CHAR_COUNT_BITS;
        // charCountBytes = BIG_CHAR_COUNT_BYTES;
        isBig = 1;
    }
    else
    {
        charCountBits = SMALL_CHAR_COUNT_BITS;
        // charCountBytes = SMALL_CHAR_COUNT_BYTES;
        isBig = 0;
    }

    Assert((numChars << 3) + charCountBits + 1 == datBitBuffer::StrBitLen(str));

    if(this->CanWriteBits(datBitBuffer::StrBitLen(str)))
    {
        success = AssertVerify(this->WriteUns(isBig, 1))
                  && AssertVerify(this->WriteUns(numChars, charCountBits))
                  && numChars ? AssertVerify(this->WriteBytes(str, numChars)) : true;
    }

    return success;
}

bool 
datBitBuffer::Write7BitEncodedStr(const char* str, const int maxChars)
{
    FastAssert(!m_IsReadonly);

    int len = (int)strlen(str);
    if(maxChars && (len > maxChars)) len = maxChars;

    return Write7BitEncodedUns(len) && WriteBytes(str, len);
}

bool
datBitBuffer::PeekBits(void* dst,
                        const int numBits,
                        const int dstBitOffset,
                        const int fromBit) const
{
    bool success = false;
    if(/*AssertVerify*/(fromBit + numBits <= this->GetBitLength()))
    {
        ReadWriteState s;
        this->SaveState(&s);
        success = this->SetCursorPos(fromBit)
                && this->ReadBits(dst, numBits, dstBitOffset);
        this->RestoreState(&s);
    }
    return success;
}

bool
datBitBuffer::PokeBits(const void* src,
                        const int numBits,
                        const int srcBitOffset,
                        const int toBit)
{
    bool success = false;
    if(AssertVerify(toBit + numBits <= this->GetBitLength()))
    {
        ReadWriteState s;
        this->SaveState(&s);
        success = this->SetCursorPos(toBit)
                && this->WriteBits(src, numBits, srcBitOffset);
        this->RestoreState(&s);
    }
    return success;
}

bool
datBitBuffer::PeekBytes(void* dst,
                         const int numBytes,
                         const int fromBit) const
{
    return this->PeekBits(dst, numBytes << 3, 0, fromBit);
}

bool
datBitBuffer::PokeBytes(const void* src,
                         const int numBytes,
                         const int toBit)
{
    return this->PokeBits(src, numBytes << 3, 0, toBit);
}

bool
datBitBuffer::PeekStr(char* str,
                      const int maxChars,
                      const int fromBit) const
{
    bool success = false;
    if(/*AssertVerify*/(fromBit <= this->GetBitLength()))
    {
        ReadWriteState s;
        this->SaveState(&s);
        success = this->SetCursorPos(fromBit)
                && this->ReadStr(str, maxChars);
        this->RestoreState(&s);
    }
    return success;
}

bool
datBitBuffer::PokeStr(const char* str,
                      const int maxChars,
                      const int toBit)
{
    bool success = false;
    if(AssertVerify(toBit + StrBitLen(str) <= this->GetBitLength()))
    {
        ReadWriteState s;
        this->SaveState(&s);
        success = this->SetCursorPos(toBit)
                && this->WriteStr(str, maxChars);
        this->RestoreState(&s);
    }
    return success;
}

bool
datBitBuffer::PeekFloat(float& f, const int fromBit) const
{
    CompileTimeAssert(sizeof(float) == sizeof(unsigned));
    union { float f; unsigned u; } tmp;
    const bool success = this->PeekUns(tmp.u, sizeof(tmp.u) << 3, fromBit);
    f = tmp.f;
    return success;
}

bool
datBitBuffer::PokeFloat(const float f, const int toBit)
{
    CompileTimeAssert(sizeof(float) == sizeof(unsigned));
    union { float f; unsigned u; } tmp;
    tmp.f = f;
    return this->PokeUns(tmp.u, sizeof(tmp.u) << 3, toBit);
}

bool
datBitBuffer::PeekDouble(double &d, const int fromBit) const
{
    CompileTimeAssert(sizeof(double) == sizeof(u64));
    union { double d; u64 u; } tmp;
    const bool success = this->PeekUns(tmp.u, sizeof(tmp.u) << 3, fromBit);
    d = tmp.d;
    return success;
}

bool
datBitBuffer::PokeDouble(const double d, const int toBit)
{
    CompileTimeAssert(sizeof(double) == sizeof(u64));
    union { double d; u64 u; } tmp;
    tmp.d = d;
    return this->PokeUns(tmp.u, sizeof(tmp.u) << 3, toBit);
}

bool
datBitBuffer::PeekBool(bool &b, const int fromBit) const
{
    unsigned u = 0;
    const bool success = this->PeekUns(u, 1, fromBit);
    b = success && (0 != u);
    return success;
}

bool
datBitBuffer::PokeBool(const bool b, const int toBit)
{
    return this->PokeUns((unsigned) b, 1, toBit);
}

//protected:

void
datBitBuffer::BumpReadWriteCursor(const int delta)
{
    this->BumpCursor(delta);

    if(m_CursorPos > m_NumBitsWritten)
    {
        m_NumBitsWritten = m_CursorPos;
    }
}

void
datBitBuffer::BumpReadCursor(const int delta) const
{
    this->BumpCursor(delta);

    if(m_CursorPos > m_NumBitsRead)
    {
        m_NumBitsRead = m_CursorPos;
    }
}

void
datBitBuffer::BumpCursor(const int delta) const
{
    Assert(delta >= 0);
    Assert(this->GetCursorPos() + delta <= this->GetMaxBits());

    m_CursorPos += delta;
}

void
datBitBuffer::SaveState(ReadWriteState* state) const
{
    state->m_CursorPos = m_CursorPos;
    state->m_NumBitsRead = m_NumBitsRead;
    state->m_NumBitsWritten = m_NumBitsWritten;
}

void
datBitBuffer::RestoreState(const ReadWriteState* state)
{
    Assert(!state->m_NumBitsWritten || !m_IsReadonly);

    m_CursorPos = state->m_CursorPos;
    m_NumBitsRead = state->m_NumBitsRead;
    m_NumBitsWritten = state->m_NumBitsWritten;
}

void
datBitBuffer::RestoreState(const ReadWriteState* state) const
{
    Assert(m_NumBitsWritten == state->m_NumBitsWritten);

    m_CursorPos = state->m_CursorPos;
    m_NumBitsRead = state->m_NumBitsRead;
}

}   //namespace rage

///////////////////////////////////////////////////////////////////////////////
// Unit tests
///////////////////////////////////////////////////////////////////////////////

#include "qa/qa.h"

#if __QA

#include "system/nelem.h"
#include "qaseritem.h"

//#define QA_MAKE_RAND_SEED   20656058
#define QA_MAKE_RAND_SEED   int(sysTimer::GetSystemMsTime())

using namespace rage;

static const int NUM_SER_ITEMS    = 256;

mthRandom qaSerItem::sm_Random;

class qa_datBitBufferA : public qaItem
{
public:

    void Init(const int bufSize, u8* buffer, const int bitOffset)
    {
        static bool s_IsInitialized = false;
        if(!s_IsInitialized)
        {
            const int seed = QA_MAKE_RAND_SEED;

            QALog("qa_datBitBufferA: Using random seed:%u", seed);

            qaSerItem::ResetRandomSeed(seed);
            s_IsInitialized = true;
        }

        m_BufSize = bufSize;
        m_Buffer = buffer;
        m_BitOffset = bitOffset;
    }

    void Shutdown()
    {
    }

	void Update(qaResult& result);

    int m_BufSize;
    u8* m_Buffer;
    int m_BitOffset;
};

class qa_datBitBufferOwnBufA : public qa_datBitBufferA
{
public:

	void Init(const int bitOffset)
    {
        this->qa_datBitBufferA::Init(sizeof(m_Buffer), m_Buffer, bitOffset);
    };

    u8 m_Buffer[256];
};

class qa_datBitBufferCheckTruncateA : public qaItem
{
public:
    qa_datBitBufferCheckTruncateA()
    {
    }

	void Init()
    {
    };

    void Shutdown()
    {
    }

	void Update(qaResult& result);
};

void
qa_datBitBufferA::Update(qaResult& result)
{
    qaSerItem inItems[NUM_SER_ITEMS];

    datBitBuffer bb;

    Assert(m_Buffer);

    bb.SetReadWriteBits(m_Buffer, (m_BufSize << 3) - m_BitOffset, m_BitOffset);

    int numItemsWritten = 0;
    int numBitsWritten = 0;

    //Write items to the buffer

    qaSerItem::Write(&bb,
                       inItems,
                       NUM_SER_ITEMS,
                       &numItemsWritten,
                       &numBitsWritten);

    QA_CHECK(bb.GetNumBitsWritten() == numBitsWritten);

    Displayf("%d items", numItemsWritten);

    qaSerItem tmp;

    //Check Peek*() methods

    int atBit = 0;

    for(int i = 0; i < numItemsWritten; ++i)
    {
        const qaSerItem* s = &inItems[i];

        switch(s->m_Type)
        {
            case qaSerItem::TYPE_INT:
                QA_CHECK(bb.PeekInt(tmp.iValue, s->m_NumBits, atBit));
                QA_CHECK(s->iValue == tmp.iValue);
                break;

            case qaSerItem::TYPE_UNSIGNED:
                QA_CHECK(bb.PeekUns(tmp.uValue, s->m_NumBits, atBit));
                QA_CHECK(s->uValue == tmp.uValue);
                break;

            case qaSerItem::TYPE_S64:
                QA_CHECK(bb.PeekInt(tmp.s64Value, s->m_NumBits, atBit));
                QA_CHECK(s->s64Value == tmp.s64Value);
                break;

            case qaSerItem::TYPE_U64:
                QA_CHECK(bb.PeekUns(tmp.u64Value, s->m_NumBits, atBit));
                QA_CHECK(s->u64Value == tmp.u64Value);
                break;
            
            case qaSerItem::TYPE_FLOAT:
                QA_CHECK(bb.PeekFloat(tmp.fValue, atBit));
                QA_CHECK(s->fValue == tmp.fValue);
                break;

            case qaSerItem::TYPE_DOUBLE:
                QA_CHECK(bb.PeekDouble(tmp.dValue, atBit));
                QA_CHECK(s->dValue == tmp.dValue);
                break;

            case qaSerItem::TYPE_BOOL:
                QA_CHECK(bb.PeekBool(tmp.bValue, atBit));
                QA_CHECK(s->bValue == tmp.bValue);
                break;

            case qaSerItem::TYPE_STRING:
                QA_CHECK(bb.PeekStr(tmp.str, NELEM(tmp.str), atBit));
                QA_CHECK(0 == ::strncmp(tmp.str, s->str, NELEM(tmp.str)));
                break;

            case qaSerItem::TYPE_BITBUF:
                QA_CHECK(bb.PeekBits(tmp.bitbuf, s->m_NumBits, 0, atBit));
                QA_CHECK(qaSerItem::CheckBitBuf(s, &tmp));
                break;

            case qaSerItem::TYPE_BYTEBUF:
                QA_CHECK(bb.PeekBytes(tmp.bytebuf, s->m_NumBytes, atBit));
                QA_CHECK(qaSerItem::CheckByteBuf(s, &tmp));
                break;

            default:
                Assert(false);
                break;
        }

        atBit += s->m_NumBits;
    }

    //Check Read*() methods

    bb.SetCursorPos(0);

    for(int i = 0; i < numItemsWritten; ++i)
    {
        const qaSerItem* s = &inItems[i];

        if(qaSerItem::TYPE_INT == s->m_Type)
        {
            QA_CHECK(bb.ReadInt(tmp.iValue, s->m_NumBits));
            QA_CHECK(tmp.iValue == s->iValue);
        }
        else if(qaSerItem::TYPE_UNSIGNED == s->m_Type)
        {
            QA_CHECK(bb.ReadUns(tmp.uValue, s->m_NumBits));
            QA_CHECK(tmp.uValue == s->uValue);
        }
        else if(qaSerItem::TYPE_S64 == s->m_Type)
        {
            QA_CHECK(bb.ReadInt(tmp.s64Value, s->m_NumBits));
            QA_CHECK(tmp.s64Value == s->s64Value);
        }
        else if(qaSerItem::TYPE_U64 == s->m_Type)
        {
            QA_CHECK(bb.ReadUns(tmp.u64Value, s->m_NumBits));
            QA_CHECK(tmp.u64Value == s->u64Value);
        }
        else if(qaSerItem::TYPE_FLOAT == s->m_Type)
        {
            QA_CHECK(bb.ReadFloat(tmp.fValue));
            QA_CHECK(tmp.fValue == s->fValue);
        }
        else if(qaSerItem::TYPE_DOUBLE == s->m_Type)
        {
            QA_CHECK(bb.ReadDouble(tmp.dValue));
            QA_CHECK(tmp.dValue == s->dValue);
        }
        else if(qaSerItem::TYPE_BOOL == s->m_Type)
        {
            QA_CHECK(bb.ReadBool(tmp.bValue));
            QA_CHECK(tmp.bValue == s->bValue);
        }
        else if(qaSerItem::TYPE_STRING == s->m_Type)
        {
            QA_CHECK(bb.ReadStr(tmp.str, sizeof(tmp.str)));
            QA_CHECK(0 == ::strcmp(tmp.str, s->str));
        }
        else if(qaSerItem::TYPE_BITBUF == s->m_Type)
        {
            QA_CHECK(bb.ReadBits(tmp.bitbuf, s->m_NumBits, 0));
            QA_CHECK(qaSerItem::CheckBitBuf(s, &tmp));
        }
        else if(qaSerItem::TYPE_BYTEBUF == s->m_Type)
        {
            QA_CHECK(bb.ReadBytes(tmp.bytebuf, s->m_NumBytes));
            QA_CHECK(qaSerItem::CheckByteBuf(s, &tmp));
        }
    }

    //Check Poke*() methods

    atBit = 0;

    for(int i = 0; i < numItemsWritten; ++i)
    {
        const qaSerItem* s = &inItems[i];

        if(qaSerItem::TYPE_INT == s->m_Type)
        {
            QA_CHECK(bb.PokeInt(s->iValue, s->m_NumBits, atBit));
        }
        else if(qaSerItem::TYPE_UNSIGNED == s->m_Type)
        {
            QA_CHECK(bb.PokeUns(s->uValue, s->m_NumBits, atBit));
        }
        else if(qaSerItem::TYPE_S64 == s->m_Type)
        {
            QA_CHECK(bb.PokeInt(s->s64Value, s->m_NumBits, atBit));
        }
        else if(qaSerItem::TYPE_U64 == s->m_Type)
        {
            QA_CHECK(bb.PokeUns(s->u64Value, s->m_NumBits, atBit));
        }
        else if(qaSerItem::TYPE_FLOAT == s->m_Type)
        {
            QA_CHECK(bb.PokeFloat(s->fValue, atBit));
        }
        else if(qaSerItem::TYPE_DOUBLE == s->m_Type)
        {
            QA_CHECK(bb.PokeDouble(s->dValue, atBit));
        }
        else if(qaSerItem::TYPE_BOOL == s->m_Type)
        {
            QA_CHECK(bb.PokeBool(s->bValue, atBit));
        }
        else if(qaSerItem::TYPE_STRING == s->m_Type)
        {
            QA_CHECK(bb.PokeStr(s->str, sizeof(s->str), atBit));
        }
        else if(qaSerItem::TYPE_BITBUF == s->m_Type)
        {
            bb.PokeBits(s->bitbuf, s->m_NumBits, s->m_Offset, atBit);
        }
        else if(qaSerItem::TYPE_BYTEBUF == s->m_Type)
        {
            bb.PokeBytes(s->bytebuf, s->m_NumBytes, atBit);
        }

        atBit += s->m_NumBits;
    }

    bb.SetCursorPos(0);

    for(int i = 0; i < numItemsWritten; ++i)
    {
        const qaSerItem* s = &inItems[i];

        if(qaSerItem::TYPE_INT == s->m_Type)
        {
            QA_CHECK(bb.ReadInt(tmp.iValue, s->m_NumBits));
            QA_CHECK(tmp.iValue == s->iValue);
        }
        else if(qaSerItem::TYPE_UNSIGNED == s->m_Type)
        {
            QA_CHECK(bb.ReadUns(tmp.uValue, s->m_NumBits));
            QA_CHECK(tmp.uValue == s->uValue);
        }
        else if(qaSerItem::TYPE_S64 == s->m_Type)
        {
            QA_CHECK(bb.ReadInt(tmp.s64Value, s->m_NumBits));
            QA_CHECK(tmp.s64Value == s->s64Value);
        }
        else if(qaSerItem::TYPE_U64 == s->m_Type)
        {
            QA_CHECK(bb.ReadUns(tmp.u64Value, s->m_NumBits));
            QA_CHECK(tmp.u64Value == s->u64Value);
        }
        else if(qaSerItem::TYPE_FLOAT == s->m_Type)
        {
            QA_CHECK(bb.ReadFloat(tmp.fValue));
            QA_CHECK(tmp.fValue == s->fValue);
        }
        else if(qaSerItem::TYPE_DOUBLE == s->m_Type)
        {
            QA_CHECK(bb.ReadDouble(tmp.dValue));
            QA_CHECK(tmp.dValue == s->dValue);
        }
        else if(qaSerItem::TYPE_BOOL == s->m_Type)
        {
            QA_CHECK(bb.ReadBool(tmp.bValue));
            QA_CHECK(tmp.bValue == s->bValue);
        }
        else if(qaSerItem::TYPE_STRING == s->m_Type)
        {
            QA_CHECK(bb.ReadStr(tmp.str, sizeof(tmp.str)));
            QA_CHECK(0 == ::strcmp(tmp.str, s->str));
        }
        else if(qaSerItem::TYPE_BITBUF == s->m_Type)
        {
            QA_CHECK(bb.ReadBits(tmp.bitbuf, s->m_NumBits, 0));
            QA_CHECK(qaSerItem::CheckBitBuf(s, &tmp));
        }
        else if(qaSerItem::TYPE_BYTEBUF == s->m_Type)
        {
            QA_CHECK(bb.ReadBytes(tmp.bytebuf, s->m_NumBytes));
            QA_CHECK(qaSerItem::CheckByteBuf(s, &tmp));
        }
    }

    //Check overwrite
    //Before each value is written, check that the expected
    //value is already in the buffer.  This ensures that previous
    //writes affect only their intended bits.

    bb.SetCursorPos(0);

    for(int i = 0; i < numItemsWritten; ++i)
    {
        const qaSerItem* s = &inItems[i];

        if(qaSerItem::TYPE_INT == s->m_Type)
        {
            QA_CHECK(bb.PeekInt(tmp.iValue, s->m_NumBits, bb.GetCursorPos()));
            QA_CHECK(tmp.iValue == s->iValue);
            QA_CHECK(bb.WriteInt(s->iValue, s->m_NumBits));
        }
        else if(qaSerItem::TYPE_UNSIGNED == s->m_Type)
        {
            QA_CHECK(bb.PeekUns(tmp.uValue, s->m_NumBits, bb.GetCursorPos()));
            QA_CHECK(tmp.uValue == s->uValue);
            QA_CHECK(bb.WriteUns(s->uValue, s->m_NumBits));
        }
        else if(qaSerItem::TYPE_S64 == s->m_Type)
        {
            QA_CHECK(bb.PeekInt(tmp.s64Value, s->m_NumBits, bb.GetCursorPos()));
            QA_CHECK(tmp.s64Value == s->s64Value);
            QA_CHECK(bb.WriteInt(s->s64Value, s->m_NumBits));
        }
        else if(qaSerItem::TYPE_U64 == s->m_Type)
        {
            QA_CHECK(bb.PeekUns(tmp.u64Value, s->m_NumBits, bb.GetCursorPos()));
            QA_CHECK(tmp.u64Value == s->u64Value);
            QA_CHECK(bb.WriteUns(s->u64Value, s->m_NumBits));
        }
        else if(qaSerItem::TYPE_FLOAT == s->m_Type)
        {
            QA_CHECK(bb.PeekFloat(tmp.fValue, bb.GetCursorPos()));
            QA_CHECK(tmp.fValue == s->fValue);
            QA_CHECK(bb.WriteFloat(s->fValue));
        }
        else if(qaSerItem::TYPE_DOUBLE == s->m_Type)
        {
            QA_CHECK(bb.PeekDouble(tmp.dValue, bb.GetCursorPos()));
            QA_CHECK(tmp.dValue == s->dValue);
            QA_CHECK(bb.WriteDouble(s->dValue));
        }
        else if(qaSerItem::TYPE_BOOL == s->m_Type)
        {
            QA_CHECK(bb.PeekBool(tmp.bValue, bb.GetCursorPos()));
            QA_CHECK(tmp.bValue == s->bValue);
            QA_CHECK(bb.WriteBool(s->bValue));
        }
        else if(qaSerItem::TYPE_STRING == s->m_Type)
        {
            QA_CHECK(bb.PeekStr(tmp.str, sizeof(tmp.str), bb.GetCursorPos()));
            QA_CHECK(0 == ::strcmp(tmp.str, s->str));
            QA_CHECK(bb.WriteStr(s->str, sizeof(s->str)));
        }
        else if(qaSerItem::TYPE_BITBUF == s->m_Type)
        {
            QA_CHECK(bb.PeekBits(tmp.bitbuf, s->m_NumBits, 0, bb.GetCursorPos()));
            QA_CHECK(qaSerItem::CheckBitBuf(s, &tmp));
            QA_CHECK(bb.WriteBits(tmp.bitbuf, s->m_NumBits, s->m_Offset));
        }
        else if(qaSerItem::TYPE_BYTEBUF == s->m_Type)
        {
            QA_CHECK(bb.PeekBytes(tmp.bytebuf, s->m_NumBytes, bb.GetCursorPos()));
            QA_CHECK(qaSerItem::CheckByteBuf(s, &tmp));
            QA_CHECK(bb.WriteBytes(tmp.bytebuf, s->m_NumBytes));
        }
    }

    QA_CHECK(bb.GetNumBitsWritten() == numBitsWritten);
    QA_CHECK(bb.GetNumBitsWritten() <= (m_BufSize << 3) - m_BitOffset);

    TST_PASS;
}

/*void
qa_datBitBufferCheckTruncateA::Update(qaResult& result)
{
    qaSerItem items[NUM_SER_ITEMS];

    static const int NUM_BYTES      = 256;
    static const int NUM_BITS       = NUM_BYTES << 3;
    static const int TRUNC_BYTES    = 128;
    static const int TRUNC_BITS     = TRUNC_BYTES << 3;

    u8 buf0[NUM_BYTES], buf1[TRUNC_BYTES];

    datBitBuffer bb0, bb_test;
    bb0.SetReadWriteBits(buf0, NUM_BITS, 0);
    bb_test.SetReadWriteBits(buf1, TRUNC_BITS, 0);

    int numItemsWritten, numItemsWritten_Test;
    int numBitsWritten, numBitsWritten_Test;

    //Write the same items to bb0, bb1, bb_ro
    qaSerItem::Write(&bb0,
                        items,
                        NUM_SER_ITEMS,
                        &numItemsWritten,
                        &numBitsWritten);

    //Check bb0
    QA_CHECK(qaSerItem::Check(&bb0, items, numItemsWritten));

    //Write fewer items to bb_test
    qaSerItem::Write(&bb_test,
                        items,
                        NUM_SER_ITEMS,
                        &numItemsWritten_Test,
                        &numBitsWritten_Test);

    //Check bb_test
    QA_CHECK(qaSerItem::Check(&bb_test, items, numItemsWritten_Test));

    //Truncate bb0 and check that it has the same items as bb_test
    QA_CHECK(bb0.SetBitSize(bb_test.GetBitSize()));
    QA_CHECK(qaSerItem::Check(&bb0, items, numItemsWritten_Test));

    TST_PASS;
}*/

//QA_ITEM_FAMILY(qa_datBitBufferA, (const int bufSize, u8* buffer, const int bitOffset), (bufSize, buffer, bitOffset));
QA_ITEM_FAMILY(qa_datBitBufferOwnBufA, (const int bitOffset), (bitOffset));
//QA_ITEM_FAMILY(qa_datBitBufferCheckTruncateA, (), ());

QA_ITEM(qa_datBitBufferOwnBufA, (0), qaResult::PASS_OR_FAIL);
QA_ITEM(qa_datBitBufferOwnBufA, (3), qaResult::PASS_OR_FAIL);
QA_ITEM(qa_datBitBufferOwnBufA, (40), qaResult::PASS_OR_FAIL);
QA_ITEM(qa_datBitBufferOwnBufA, (51), qaResult::PASS_OR_FAIL);
//QA_ITEM(qa_datBitBufferCheckTruncateA, (), qaResult::PASS_OR_FAIL);

#endif  //__QA
