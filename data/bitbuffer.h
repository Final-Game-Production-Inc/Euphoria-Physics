// 
// data/bitbuffer.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

/*
    NOTICE!!!

    Before making changes to datBitBuffer and it's ilk please notice that
    the only functions that actually read/write from/to a buffer are the
    static functions datBitBuffer::ReadUnsigned(), datBitBuffer::WriteUnsigned,
    and datBitBuffer::CopyBits().

    All other member functions simply call those functions.

    This makes it easy to implement new functionality such as when we wanted
    to add a NULL implementation for export buffers that would simply count
    the number of bits written but not actually write them to a buffer.
*/

#ifndef DATA_BITBUFFER_H
#define DATA_BITBUFFER_H

namespace rage
{

namespace datbitbuffer
{

template<bool IS_INT = false>
struct MachineWordType
{
    typedef void UType;
    typedef void SType;
    typedef void MwType;
};

template<>
struct MachineWordType<true>
{
    typedef int UType;
    typedef int SType;
    typedef int MwType;
};

//PURPOSE
//  Integer traits.
template<typename T>
struct IntTraits
{
    struct Yes{char a[1];};
    struct No{char a[128];};

    static Yes IsConvertibleToInt(const int);
    static No IsConvertibleToInt(...);
    static T m_T;

    enum
    {
        IsInteger = sizeof(IsConvertibleToInt(m_T)) == sizeof(Yes),
        IsSigned  = IsInteger,
        NumBits   = sizeof(T) << 3,
    };

    //Define some types that we use when serializing.  We need to
    //know the signed variant of T, the unsigned variant of T, and
    //the native machine word variant of T.

    //Unsigned variant.
    typedef typename MachineWordType<IsInteger>::UType UType;
    //Signed variant.
    typedef typename MachineWordType<IsInteger>::SType SType;
    //Native machine word type.
    //For all unsigned types except u64 this is unsigned.
    //For all signed types except s64 this is int.
    typedef typename MachineWordType<IsInteger>::MwType MwType;
};

//Integer traits specializations.
template<> struct IntTraits<s8>{ enum{ IsInteger = true, IsSigned = true, NumBits = 8 }; typedef u8 UType; typedef s8 SType; typedef int MwType; };
template<> struct IntTraits<u8>{ enum{ IsInteger = true, IsSigned = false, NumBits = 8 }; typedef u8 UType; typedef s8 SType; typedef unsigned MwType; };
template<> struct IntTraits<s16>{ enum{ IsInteger = true, IsSigned = true, NumBits = 16 }; typedef u16 UType; typedef s16 SType; typedef int MwType; };
template<> struct IntTraits<u16>{ enum{ IsInteger = true, IsSigned = false, NumBits = 16 }; typedef u16 UType; typedef s16 SType; typedef unsigned MwType; };
template<> struct IntTraits<s32>{ enum{ IsInteger = true, IsSigned = true, NumBits = 32 }; typedef u32 UType; typedef s32 SType; typedef int MwType; };
template<> struct IntTraits<u32>{ enum{ IsInteger = true, IsSigned = false, NumBits = 32 }; typedef u32 UType; typedef s32 SType; typedef unsigned MwType; };
template<> struct IntTraits<s64>{ enum{ IsInteger = true, IsSigned = true, NumBits = 64 }; typedef u64 UType; typedef s64 SType; typedef s64 MwType; };
template<> struct IntTraits<u64>{ enum{ IsInteger = true, IsSigned = false, NumBits = 64 }; typedef u64 UType; typedef s64 SType; typedef u64 MwType; };

//PURPOSE
//  Used to optimize string serialization.
//NOTES
//  Serialized strings are stored zero-terminated and prepended with
//  the number of chars in string (including the zero terminator).
//  A char count < 128 requires only one byte in the bit buffer.
//  Char counts >= 128 require two bytes in the buffer and the MSB
//  in the first byte is a one.
enum
{
    SMALL_CHAR_COUNT_BITS           = 7,
    SMALL_CHAR_COUNT_MAX_CHARS      = 0x7F,
    SMALL_CHAR_COUNT_BYTES          = 1,
    BIG_CHAR_COUNT_BITS             = 15,
    BIG_CHAR_COUNT_MAX_CHARS        = 0x7FFF,
    BIG_CHAR_COUNT_BYTES            = 2,
};

}   //namespace datbitbuffer

//PURPOSE
//  Counts the number of bits required to store an unsigned integer with a
//  value of VAL.
//NOTES
//  This is used to determine the number of bits required to store
//  a value represented by a constant.
//
//  For example, the following determines the number of bits necessary to
//  store a value of Colors (assuming all Colors values are non-negative):
//
//  enum Colors{ RED, BLUE, GREEN, NUM_COLORS };
//  static const int COLOR_BITS_NEEDED = datBitsNeeded< NUM_COLORS >::COUNT;
//
//  For values that can be negative, add one to the bit count:
//
//  enum Colors{ INVALID = -1, RED, BLUE, GREEN, NUM_COLORS };
//  static const int COLOR_BITS_NEEDED = 1 + datBitsNeeded< NUM_COLORS >::COUNT;
//
template<unsigned VAL>
struct datBitsNeeded
{
    enum
    {
        COUNT   = VAL ? (datBitsNeeded< (VAL >> 1) >::COUNT + 1) : 0
    };
};

template<>
struct datBitsNeeded<0>
{
    enum { COUNT = 0 };
};

template<unsigned MAXSIZE>
struct datMaxBytesNeededForString
{
	enum
	{
		COUNT   = (MAXSIZE <= datbitbuffer::SMALL_CHAR_COUNT_MAX_CHARS) ?
				  (MAXSIZE + datbitbuffer::SMALL_CHAR_COUNT_BYTES) :
				  (MAXSIZE + datbitbuffer::BIG_CHAR_COUNT_BYTES)
	};
};

template<unsigned MAXSIZE>
struct datMaxBitsNeededForString
{
	enum
	{
		COUNT = (datMaxBytesNeededForString<MAXSIZE>::COUNT) << 3
	};
};

//PURPOSE
//  A bit buffer represents an array of bits that can be read and written.
//  The bits in a bit buffer are arranged from left to right, so bit zero
//  is the first bit in the array.
//
//  Bit buffers are used to serialize and de-serialize data in compact forms.
//  For example, an integer value can be written to a bit buffer using
//  an odd number of bits:
//
//  bitbuf.WriteInt(123, 9);
//
//  When reading and writing data bit buffers behave similarly to files.
//  Each bit buffer has an associated cursor position that represents the
//  next location in the buffer to which data will be written or from which
//  it will be read.
//
//  A bit buffer keeps track of the number of bits read/written independently
//  of its cursor position.  For example, the cursor can be moved to the
//  beginning of the buffer without affecting the number of bits the
//  buffer will report it contains.  If the cursor is moved backwards,
//  subsequent writes will over-write existing data.
//
//  The arrangement of bits in a bit buffer can be confusing when thinking
//  in terms of bytes because bit zero in a bit buffer is actually the MSB
//  of the first byte, not the LSB.  For example, the following code will
//  write a zero as the first bit of the buffer:
//
//  u8 b = 1;
//  bitbuf.WriteBits(&b, 1, 0);
//
//  The following code will write 1 as the first bit of the buffer:
//
//  u8 b = 0x80;
//  bitbuf.WriteBits(&b, 1, 0);
//
class datBitBuffer
{
public:

    //PURPOSE
    //  Reads an unsigned integer from a buffer.
    //PARAMS
    //  bits            - Source bits.
    //  u               - Reference to target unsigned int.
    //  numBits         - Number of bits to read.
    //  srcBitOffset    - Bit offset in source from which to read.
    static void ReadUnsigned(const void* bits,
                              unsigned& u,
                              const int numBits,
                              const int srcBitOffset);

    //PURPOSE
    //  Writes an unsigned integer to a buffer.
    //PARAMS
    //  bits            - Destination bits.
    //  u               - Source unsigned int.
    //  numBits         - Number of bits to read.
    //  dstBitOffset    - Bit offset in destination at which to write.
    static void WriteUnsigned(void* bits,
                               const unsigned u,
                               const int numBits,
                               const int dstBitOffset);

    //PURPOSE
    //  Copies bits.
    //PARAMS
    //  dst             - Destination bits.
    //  src             - Source bits.
    //  numBits         - Number of bits to copy.
    //  dstBitOffset    - Bit offset in dst at which to write.
    //  srcBitOffset    - Bit offset in src from which to read.
    static void CopyBits(void* dst,
                          const void* src,
                          const int numBits,
                          const int dstBitOffset,
                          const int srcBitOffset);

    //PURPOSE
    //  Returns the number of bits a string of the given length would
    //  require for serialization.
    //NOTES
    //  strLen should be the value returned by strlen(), i.e. it should
    //  not include the zero terminator.
    //  The returned value includes the one or two bytes required for the
    //  string length, and the zero terminator.
    static int StrBitLen(const int strLen);

    //PURPOSE
    //  Returns the number of bits the given string would require for
    //  serialization.
    //NOTES
    //  The returned value includes the one or two bytes required for the
    //  string length, and the zero terminator.
    static int StrBitLen(const char* str);

    datBitBuffer();

    //PURPOSE
    //  Resets the bit buffer to its initial state.  After calling this method
    //  reading/writing to the bit buffer will fail until SetReadOnlyBits() or
    //  SetReadWriteBits() is called.
    void Reset();

    //PURPOSE
    //  Effectively clears the buffer contents, setting the cursor position
    //  to zero.
    //  Equivalent to calling Set*Bits(Get*Bits(),GetMaxBits(),GetBaseBitOffset());
    void Clear();

    //PURPOSE
    //  Returns true if the bit buffer is read-only.
    bool IsReadOnly() const
	{
		return m_IsReadonly;
	}


    //PURPOSE
    //  Returns true if the bit buffer is a null-write buffer.
    bool IsNullWrite() const
	{
		return m_IsNullWrite;
	}


    //PURPOSE
    //  Returns true if the bit buffer is writable.
    bool IsWritable() const;

    //PURPOSE
    //  Initializes the bit buffer to a read-only buffer.
    //PARAMS
    //  bits        - Buffer from which bits will be read.
    //  maxBits     - Number of readable bits in the buffer.
    //  baseBitOffset   - Offset (in bits) to first accessible bit in the buffer.
    //NOTES
    //  All buffer operations will be relative to the base bit offset.
    void SetReadOnlyBits(const void* bits,
                          const int maxBits,
                          const int baseBitOffset);

    //PURPOSE
    //  Initializes the bit buffer to a read-only buffer.
    //PARAMS
    //  bytes       - Buffer from which bits will be read.
    //  maxBytes    - Number of readable bytes in the buffer.
    //NOTES
    //  The base bit offset will be zero.
    void SetReadOnlyBytes(const void* bytes,
                           const int maxBytes);

    //PURPOSE
    //  Initializes the bit buffer to a read/write buffer.
    //PARAMS
    //  bits        - Buffer from/to which bits will be read/written.
    //  maxBits     - Number of readable/writeable bits in the buffer.
    //  baseBitOffset   - Offset (in bits) to first accessible bit in the buffer.
    //NOTES
    //  All buffer operations will be relative to the base bit offset.
    void SetReadWriteBits(void* bits,
                           const int maxBits,
                           const int baseBitOffset);

    //PURPOSE
    //  Initializes the bit buffer to a read/write buffer.
    //PARAMS
    //  bytes       - Buffer from/to which bits will be read/written.
    //  maxBytes    - Number of readable/writeable bytes in the buffer.
    //NOTES
    //  The base bit offset will be zero.
    void SetReadWriteBytes(void* bytes,
                            const int maxBytes);

    //PURPOSE
    //  Initializes the bit buffer to a null-write buffer.
    //  Null-write buffers can be used to obtain the size of a series
    //  of calls to Write* functions without actually writing anything.
    //PARAMS
    //  maxBytes    - Number of writeable bytes in the buffer.
    //NOTES
    //  Null-write buffers cannot be read.
    void SetNullWriteBytes(const int maxBytes);

    //PURPOSE
    //  Returns a const pointer to the underlying memory buffer.
    const void* GetReadOnlyBits() const;

    //PURPOSE
    //  Returns a non-const pointer to the underlying memory buffer.
    //NOTES
    //  This will return NULL on a read-only buffer.
    void* GetReadWriteBits();

    //PURPOSE
    //  Returns a const pointer to the underlying memory buffer.
    //NOTES
    //  This will return NULL on a read-only buffer.
    const void* GetReadWriteBits() const;

    //PURPOSE
    //  Returns the offset (in bits) to the first accessible bit in the
    //  underlying memory buffer.
    int GetBaseBitOffset() const;

    //PURPOSE
    //  Returns the number of bits available for reading/writing.
    int GetMaxBits() const
	{
		return m_MaxBits;
	}


    //PURPOSE
    //  Returns number of actual data bits in the buffer.
    //  For read-only buffers this is the maxBits parameter passed to
    //  SetReaOnlyBits().
    //  For read-write buffers this is the number of bits written,
    //  regardless of the current cursor position.
    int GetBitLength() const
	{
		return this->IsReadOnly() ? this->GetMaxBits() : this->GetNumBitsWritten();
	}


    //PURPOSE
    //  Returns number of actual data bytes in the buffer.
    //  Equivalent to (GetBitLength() + 7) >> 3.
    int GetByteLength() const;

    //PURPOSE
    //  Returns the number of bits that have been written.
    //NOTES
    //  The value returned is not the same as that returned by GetCursorPos().
    //  The cursor position may change without changing the number of bits that
    //  have been read/written.
    //  A write to the buffer will change the value of "bits written"
    //  to the greater of the current "bits written" value or the cursor
    //  position after the write operation.
    int GetNumBitsWritten() const
	{
		return m_NumBitsWritten;
	}


    //PURPOSE
    //  Returns the number of bytes that have been written.
    int GetNumBytesWritten() const;

    //PURPOSE
    //  Artificially sets the number of bits written.
    //NOTES
    //  Also sets the cursor position to be equal to the
    //  number of bits written.
    bool SetNumBitsWritten(const int numBitsWritten);

    //PURPOSE
    //  Returns the number of bits that have been read.
    //NOTES
    //  The value returned is not the same as that returned by GetCursorPos().
    //  The cursor position may change without changing the number of bits that
    //  have been read/written.
    //  A read from the buffer will change the value of "bits read"
    //  to the greater of the current "bits read" value or the cursor
    //  position after the read operation.
    int GetNumBitsRead() const
	{
		return m_NumBitsRead;
	}

    //PURPOSE
    //  Returns the number of bytes that have been read.
    int GetNumBytesRead() const;

    //PURPOSE
    //  Artificially sets the number of bits read.
    //NOTES
    //  Also sets the cursor position to be equal to the
    //  number of bits read.
    bool SetNumBitsRead(const int numBitsRead) const;

    //PURPOSE
    //  Returns true if the given number of bits can be read.
    bool CanReadBits(const int numBits) const
	{
		return !this->IsNullWrite() && (m_CursorPos + numBits <= this->GetBitLength());
	}


    //PURPOSE
    //  Returns true if the given number of bytes can be read.
    bool CanReadBytes(const int numBytes) const;

    //PURPOSE
    //  Returns true if the given number of bits can be written.
    bool CanWriteBits(const int numBits) const
	{
		return !this->IsReadOnly() && (m_CursorPos + numBits) <= m_MaxBits;
	}


    //PURPOSE
    //  Returns true if the given number of bytes can be written.
    bool CanWriteBytes(const int numBytes) const;

    //PURPOSE
    //  Returns true if the given string can be written to the buffer.
    bool CanWriteStr(const char* str) const;

    //PURPOSE
    //  Sets the bit position of the cursor.  This is the location of the next
    //  read/write.
    //NOTES
    //  This does not change the number of bits read/written.  To change those
    //  values use SetNumBitsRead() and SetNumBitsWritten().
    bool SetCursorPos(const int pos) const;

    //PURPOSE
    //  Returns the current bit position of the cursor.
    int GetCursorPos() const;

    //PURPOSE
    //  Skips the given number of bits.
    //RETURNS
    //  True on success.
    //NOTES
    //  Pass a negative value to skip backwards.
    //  This function does not affect the values returned by
    //  GetNumBitsRead() or GetNumBitsWritten().
    bool SkipBits(const int numBits) const;

    //PURPOSE
    //  Skips the given number of bytess.
    //RETURNS
    //  True on success.
    //NOTES
    //  Pass a negative value to skip backwards.
    //  This function does not affect the values returned by
    //  GetNumBitsRead() or GetNumBitsWritten().
    bool SkipBytes(const int numBytes) const;

    //PURPOSE
    //  Reads bits (starting at the current cursor position) into the
    //  destination buffer.
    //PARAMS
    //  dst         - Destination buffer.
    //  numBits     - Number of bits to copy.
    //  dstBitOffset    - Offset (in bits) at which to copy bits into the
    //                    destination.
    bool ReadBits(void* dst,
                   const int numBits,
                   const int dstBitOffset) const;

    //PURPOSE
    //  Reads bits (starting at the current cursor position) into the
    //  destination buffer (starting at the current cursor position).
    //PARAMS
    //  dst         - Destination buffer.
    //  numBits     - Number of bits to copy.
    //NOTES
    //  Cursor positions in both buffers are incremented by the number of
    //  bits read.
    bool ReadBits(datBitBuffer& dst,
                    const int numBits) const;

    //PURPOSE
    //  Writes bits (starting at the current cursor position) from the
    //  source buffer.
    //PARAMS
    //  src         - Source buffer.
    //  numBits     - Number of bits to copy.
    //  srcBitOffset    - Offset (in bits) from which to copy bits from the
    //                    source.
    bool WriteBits(const void* src,
                    const int numBits,
                    const int srcBitOffset);

    //PURPOSE
    //  Writes bits (starting at the current cursor position) from the
    //  source buffer (starting at the current cursor position).
    //PARAMS
    //  src         - Source buffer.
    //  numBits     - Number of bits to copy.
    //NOTES
    //  Cursor positions in both buffers are incremented by the number of
    //  bits written.
    bool WriteBits(const datBitBuffer& src,
                    const int numBits);

    //PURPOSE
    //  Reads an unsigned integer from the buffer.
    //PARAMS
    //  u       - Reference to the target unsigned int.
    //  numBits - Number of bits to read.
    template<typename T>
    bool ReadUns(T& u,
                  const int numBits /*= datbitbuffer::IntTraits< T >::NumBits*/) const
    {
        CompileTimeAssert(sizeof(T) <= sizeof(unsigned)
                        && datbitbuffer::IntTraits<T>::NumBits > 0);
        FastAssert(numBits >= 0 && numBits <= datbitbuffer::IntTraits< T >::NumBits);

        bool success = false;

        if(/*AssertVerify*/(this->CanReadBits(numBits)))
        {
            unsigned tmp;
            datBitBuffer::ReadUnsigned(m_ReadBits,
                                        tmp,
                                        numBits,
                                        m_CursorPos + m_BaseBitOffset);

            u = static_cast<T>(tmp);
            this->BumpReadCursor(numBits);
            success = true;
        }
        else
        {
            u = static_cast<T>(0);
        }

        return success;
    }

    //PURPOSE
    //  Writes an unsigned integer to the buffer.
    //PARAMS
    //  u       - The unsigned int to write.
    //  numBits - Number of bits to write.
    template<typename T>
    bool WriteUns(const T u,
                   const int numBits /*= datbitbuffer::IntTraits< T >::NumBits*/)
    {
        CompileTimeAssert(sizeof(T) <= sizeof(unsigned)
                        && datbitbuffer::IntTraits<T>::NumBits > 0);
        FastAssert(numBits >= 0 && numBits <= datbitbuffer::IntTraits< unsigned >::NumBits);
        FastAssert(!m_IsReadonly);

        bool success = false;

        if(AssertVerify(this->CanWriteBits(numBits)))
        {
            if(!m_IsNullWrite)
            {
                datBitBuffer::WriteUnsigned(m_WriteBits,
                                             u,
                                             numBits,
                                             m_CursorPos + m_BaseBitOffset);
            }

            this->BumpReadWriteCursor(numBits);

            success = true;
        }

        return success;
    }

    //PURPOSE
    //  Reads a 64-bit unsigned int.
    bool ReadUns(u64& u, const int numBits) const;

    //PURPOSE
    //  Writes a 64-bit unsigned int.
    bool WriteUns(const u64 u, const int numBits);

    //PURPOSE
    //  Reads a signed integer from the buffer.
    //PARAMS
    //  i       - Reference to the target signed int.
    //  numBits - Number of bits to read.
    template<typename T>
    bool ReadInt(T& i,
                  const int numBits /*= datbitbuffer::IntTraits< T >::NumBits*/) const
    {
        CompileTimeAssert(datbitbuffer::IntTraits< T >::IsInteger
                        && datbitbuffer::IntTraits< T >::IsSigned
                        && datbitbuffer::IntTraits<T>::NumBits > 0);
        FastAssert(numBits >= 1 && numBits <= datbitbuffer::IntTraits< T >::NumBits);

        //For optimal code use the native machine word for all operations.
        typedef typename datbitbuffer::IntTraits< T >::MwType SMwType;
        typedef typename datbitbuffer::IntTraits< SMwType >::UType UMwType;

        //Use the unsigned variant of the machine word.
        UMwType u = 0;

        unsigned neg = 0;

        //The first bit of an integer determines if the value is postive or
        //negative (1 means negative).  The remaining bits are non-negative.

        const bool success = this->ReadUns(neg, 1)
                            && this->ReadUns(u, numBits - 1);

        //Negate if necessary.
        i = T((-SMwType(neg) ^ u) + neg);

        return success;
    }

    //PURPOSE
    //  Writes a signed integer to the buffer.
    //PARAMS
    //  i       - The signed int to write.
    //  numBits - Number of bits to write.
    template<typename T>
    bool WriteInt( const T i,
                    const int numBits /*= datbitbuffer::IntTraits< T >::NumBits*/)
    {
        CompileTimeAssert(datbitbuffer::IntTraits< T >::IsInteger
                        && datbitbuffer::IntTraits< T >::IsSigned
                        && datbitbuffer::IntTraits<T>::NumBits > 0);
        FastAssert(numBits >= 1 && numBits <= datbitbuffer::IntTraits< T >::NumBits);

        //Signed/unsigned machine word types.
        typedef typename datbitbuffer::IntTraits< T >::MwType SMwType;
        typedef typename datbitbuffer::IntTraits< SMwType >::UType UMwType;

        static const int MAX_BITS = datbitbuffer::IntTraits< SMwType >::NumBits;

        //The first bit written indicates whether the value is positive or
        //negative.  A 1 bit indicates negative.  The value is then converted to a
        //non-negative number and written to the buffer.

        //If i is negative, neg will be 1.
        const unsigned neg =
            unsigned(UMwType(SMwType(i)) >> (MAX_BITS - 1));

        return this->WriteUns(neg, 1)
            && this->WriteUns(UMwType((-SMwType(neg) ^ SMwType(i)) + neg),
                               numBits - 1);
    }

    //PURPOSE
    //  Reads a float from the buffer.
    bool ReadFloat(float& f) const;

    //PURPOSE
    //  Writes a float to the buffer.
    bool WriteFloat(const float f);

    //PURPOSE
    //  Reads a double from the buffer.
    bool ReadDouble(double& d) const;

    //PURPOSE
    //  Writes a double to the buffer.
    bool WriteDouble(const double d);

    //PURPOSE
    //  Reads a bool from the buffer.
    bool ReadBool(bool& b) const;

    //PURPOSE
    //  Writes a bool to the buffer.
    bool WriteBool(const bool b);

    //PURPOSE
    //  Reads bytes from the buffer.
    //PARAMS
    //  bytes       - Destination buffer.
    //  numBytes    - Number of bytes to read.
    //NOTES
    //  Bytes are read starting from the current cursor position.
    bool ReadBytes(void* bytes,
                    const int numBytes) const;

    //PURPOSE
    //  Writes bytes to the buffer.
    //PARAMS
    //  bytes       - Source buffer.
    //  numBytes    - Number of bytes to write.
    //NOTES
    //  Bytes are written starting at the current cursor position.
    bool WriteBytes(const void* bytes,
                     const int numBytes);

    //PURPOSE
    //  Reads a zero-terminated string from the buffer.
    //PARAMS
    //  str         - Destination buffer.
    //  maxChars    - Maximum number of characters to read (including
    //                the zero terminator).
    //NOTES
    //  Chars are read starting from the current cursor position.
    bool ReadStr(char* str,
                  const int maxChars) const;

    //PURPOSE
    //  Writes a zero-terminated string to the buffer.
    //PARAMS
    //  str         - Source buffer.
    //  maxChars    - Maximum number of characters to write (including
    //                the zero terminator).
    //NOTES
    //  Chars are written starting at the current cursor position.
    bool WriteStr(const char* str,
                   const int maxChars);

    //PURPOSE
    //  Writes an unsigned integer to the buffer using 7-bit encoding.
    template<typename T>
    bool Write7BitEncodedUns(const T val)
    {
        CompileTimeAssert(sizeof(T) <= sizeof(unsigned) && datbitbuffer::IntTraits<T>::NumBits > 0);
        FastAssert(!m_IsReadonly);

        u8 buf[sizeof(T) + ((sizeof(T) + 7)/8)];
        unsigned bufLen = 0;

        T temp = val;
        while(temp >= 0x80)
        {
            buf[bufLen++] = (u8)temp | 0x80;
            temp = temp >> 7;
        }
        buf[bufLen++] = (u8)temp;

        FastAssert(bufLen <= sizeof(buf));

        return datBitBuffer::WriteBytes(buf, bufLen);
    }

    //PURPOSE
    //  Writes a string (WITHOUT terminating null), prefaced by its length 
    //  written as a 7-bit encoded unsigned int.  This is compatible with
    //  .NET's BinaryWriter class.
    //NOTE
    //  If maxChars is 0, the entire string is always written.
    bool Write7BitEncodedStr(const char* str, const int maxChars = 0);

    //PURPOSE
    //  Copies bits from any location in the buffer without advancing the
    //  cursor.
    //PARAMS
    //  dst             - Destination buffer.
    //  numBits         - Number of bits to copy.
    //  dstBitOffset    - Bit offset in destination at which to copy.
    //  fromBit         - Bit offset in the bit buffer from which to copy.
    bool PeekBits(void* dst,
                   const int numBits,
                   const int dstBitOffset,
                   const int fromBit) const;

    //PURPOSE
    //  Copies bits to any location in the buffer without advancing the
    //  cursor.
    //PARAMS
    //  src             - Source buffer.
    //  numBits         - Number of bits to copy.
    //  srcBitOffset    - Bit offset in source from which to copy.
    //  toBit           - Bit offset in the bit buffer at which to copy.
    bool PokeBits(const void* src,
                   const int numBits,
                   const int srcBitOffset,
                   const int toBit);

    //PURPOSE
    //  Copies bytes from any location in the buffer without advancing the
    //  cursor.
    //PARAMS
    //  dst             - Destination buffer.
    //  numBytes        - Number of bytes to copy.
    //  fromBit         - Bit offset in the bit buffer from which to copy.
    bool PeekBytes(void* dst,
                    const int numBytes,
                    const int fromBit) const;

    //PURPOSE
    //  Copies bytes to any location in the buffer without advancing the
    //  cursor.
    //PARAMS
    //  src             - Source buffer.
    //  numBytes        - Number of bytes to copy.
    //  toBit           - Bit offset in the bit buffer at which to copy.
    bool PokeBytes(const void* src,
                    const int numBytes,
                    const int toBit);

    //PURPOSE
    //  Reads an unsigned int from any location in the buffer without
    //  advancing the cursor.
    //PARAMS
    //  u           - Target unsigned int.
    //  numBits     - Number of bits to read.
    //  fromBit     - Bit offset in the bit buffer from which to read.
    template<typename T>
    bool PeekUns(T& u,
                  const int numBits,
                  const int fromBit) const
    {
        bool success = false;
        if(AssertVerify(fromBit + numBits <= this->GetBitLength()))
        {
            ReadWriteState s;
            this->SaveState(&s);
            success = this->SetCursorPos(fromBit)
                    && this->ReadUns(u, numBits);
            this->RestoreState(&s);
        }
        return success;
    }

    //PURPOSE
    //  Writes an unsigned int to any location in the buffer without
    //  advancing the cursor.
    //PARAMS
    //  u           - Source unsigned int.
    //  numBits     - Number of bits to write.
    //  toBit       - Bit offset in the bit buffer at which to write.
    template<typename T>
    bool PokeUns(const T u,
                  const int numBits,
                  const int toBit)
    {
        bool success = false;
        if(AssertVerify(toBit + numBits <= this->GetBitLength()))
        {
            ReadWriteState s;
            this->SaveState(&s);
            success = this->SetCursorPos(toBit)
                    && this->WriteUns(u, numBits);
            this->RestoreState(&s);
        }
        return success;
    }

    //PURPOSE
    //  Reads a signed int from any location in the buffer without
    //  advancing the cursor.
    //PARAMS
    //  i           - Target signed int.
    //  numBits     - Number of bits to read.
    //  fromBit     - Bit offset in the bit buffer from which to read.
    template<typename T>
    bool PeekInt(T& i,
                  const int numBits,
                  const int fromBit) const
    {
        bool success = false;
        if(AssertVerify(fromBit + numBits <= this->GetBitLength()))
        {
            ReadWriteState s;
            this->SaveState(&s);
            success = this->SetCursorPos(fromBit)
                    && this->ReadInt(i, numBits);
            this->RestoreState(&s);
        }
        return success;
    }

    //PURPOSE
    //  Writes a signed int to any location in the buffer without
    //  advancing the cursor.
    //PARAMS
    //  i           - Source signed int.
    //  numBits     - Number of bits to write.
    //  toBit       - Bit offset in the bit buffer at which to write.
    template<typename T>
    bool PokeInt(const T i,
                  const int numBits,
                  const int toBit)
    {
        bool success = false;
        if(AssertVerify(toBit + numBits <= this->GetBitLength()))
        {
            ReadWriteState s;
            this->SaveState(&s);
            success = this->SetCursorPos(toBit)
                    && this->WriteInt(i, numBits);
            this->RestoreState(&s);
        }
        return success;
    }

    //PURPOSE
    //  Reads a zero-terminated string from any location in the buffer without
    //  advancing the cursor.
    //PARAMS
    //  str         - Destination buffer.
    //  maxChars    - Maximum number of characters to read (including the zero
    //                terminator).
    //  fromBit     - Bit offset in the bit buffer from which to read.
    bool PeekStr(char* str,
                  const int maxChars,
                  const int fromBit) const;

    //PURPOSE
    //  Writes a zero-terminated string to any location in the buffer without
    //  advancing the cursor.
    //PARAMS
    //  str         - Source buffer.
    //  maxChars    - Maximum number of characters to write (including the zero
    //                terminator).
    //  fromBit     - Bit offset in the bit buffer at which to write.
    bool PokeStr(const char* str,
                  const int maxChars,
                  const int toBit);

    //PURPOSE
    //  Reads a float from any location in the buffer without
    //  advancing the cursor.
    //PARAMS
    //  f           - Target float.
    //  fromBit     - Bit offset in the bit buffer from which to read.
    bool PeekFloat(float& f,
                    const int fromBit) const;

    //PURPOSE
    //  Writes a float to any location in the buffer without
    //  advancing the cursor.
    //PARAMS
    //  f           - Source float.
    //  fromBit     - Bit offset in the bit buffer at which to write.
    bool PokeFloat(const float f,
                    const int toBit);

    //PURPOSE
    //  Reads a double from any location in the buffer without
    //  advancing the cursor.
    //PARAMS
    //  d           - Target double.
    //  fromBit     - Bit offset in the bit buffer from which to read.
    bool PeekDouble(double& d,
                     const int fromBit) const;

    //PURPOSE
    //  Writes a double to any location in the buffer without
    //  advancing the cursor.
    //PARAMS
    //  d           - Source double.
    //  fromBit     - Bit offset in the bit buffer at which to write.
    bool PokeDouble(const double d,
                     const int toBit);

    //PURPOSE
    //  Reads a bool from any location in the buffer without
    //  advancing the cursor.
    //PARAMS
    //  b           - Target bool.
    //  fromBit     - Bit offset in the bit buffer from which to read.
    bool PeekBool(bool& b,
                   const int fromBit) const;

    //PURPOSE
    //  Writes a bool to any location in the buffer without
    //  advancing the cursor.
    //PARAMS
    //  b           - Source bool.
    //  fromBit     - Bit offset in the bit buffer at which to write.
    bool PokeBool(const bool b,
                   const int toBit);

protected:

    //PURPOSE
    //  Moves the read/write cursor.
    void BumpReadWriteCursor(const int delta);

    //PURPOSE
    //  Moves the read cursor.
    void BumpReadCursor(const int delta) const;

    //PURPOSE
    //  Moves the cursor.
    void BumpCursor(const int delta) const;

    struct ReadWriteState
    {
        int m_CursorPos;
        int m_NumBitsRead;
        int m_NumBitsWritten;
    };

    void SaveState(ReadWriteState* state) const;
    void RestoreState(const ReadWriteState* state);
    void RestoreState(const ReadWriteState* state) const;

    union
    {
        const u8* m_ReadBits;
        u8* m_WriteBits;
    };
    int m_BaseBitOffset;
    int m_MaxBits;
    mutable int m_CursorPos;
    int m_NumBitsWritten;
    mutable int m_NumBitsRead;
    bool m_IsReadonly   : 1;
    bool m_IsNullWrite  : 1;

private:

    datBitBuffer(const datBitBuffer&);
    datBitBuffer& operator=(const datBitBuffer&);
};

template<typename T, typename U>
inline
bool datImport(T& bb, U& a)
{
	u8 buf[sizeof(U)] = {0};
    unsigned impSize;
    unsigned size = (bb.GetBitLength() - bb.GetCursorPos()) >> 3;
    size = size > sizeof(buf) ? sizeof(buf) : size;
    return AssertVerify(bb.PeekBytes(buf, size, bb.GetCursorPos()))
            && a.Import(buf, size, &impSize)
            && AssertVerify(bb.SkipBytes(impSize))
            && AssertVerify(bb.SetNumBitsRead(bb.GetCursorPos()));
}

template<typename T, typename U>
inline
bool datExport(T& bb, U& a)
{
	// initialize the temp buffer to all zeros, because if the object's export
	// method doesn't clear the buffer, the bitbuffer will use the random data
	// that it contains
	u8 buf[sizeof(U)] = {0};
    unsigned expSize;
    return a.Export(buf, sizeof(buf), &expSize)
            && bb.WriteBytes(buf, expSize);
}

//PURPOSE
//  Used to export data to a bit buffer.
class datExportBuffer : public datBitBuffer
{
public:

    //PURPOSE
    //  Exports a user defined type object.
    //  Must be implemented by the application.
    template<typename T> bool SerUser(T& a)
    {
        return datExport(*this, a);
    }

    template<typename T>
    bool SerUns(T& a, const int numBits) {return this->WriteUns(a, numBits);}
    template<typename T>
    bool SerInt(T& a, const int numBits) {return this->WriteInt(a, numBits);}
    template<typename T>
    bool SerFloat(T& a) {return this->WriteFloat(a);}
    template<typename T>
    bool SerDouble(T& a) {return this->WriteDouble(a);}
    template<typename T>
    bool SerBool(T& a) {return this->WriteBool(a);}
    template<typename T>
    bool SerStr(T* a, const int maxChars) {return this->WriteStr(a, maxChars);}
    template<typename T>
    bool SerBytes(T* a, const int numBytes) {return this->WriteBytes(a, numBytes);}
    template<typename T>
    bool SerBits(T* a, const int numBits) {return this->WriteBits(a, numBits, 0);}
	bool SerBits(const datBitBuffer& a, const int numBits) {return this->WriteBits(a, numBits);}
    //PURPOSE
    //  Exports a serialize-able object.
    template<typename T>
    bool SerSer(T& a) {return a.Export(*this);}

    //PURPOSE
    //  Exports the number of used elements in an array.
    //PARAMS
    //  a    - The number of elements to write
    //  arr  - The array used to compute the bits needed for serialization and to verify the bounds
    //  name - Debug name of the array item
    template<typename T, typename ArrayItemType, unsigned SIZE>
    bool SerArraySize(T& a, const ArrayItemType(&/*arr*/)[SIZE] ASSERT_ONLY(, const char* name));

private:
    void SetReadOnlyBits(const void*, const int, const int);
    void SetReadOnlyBytes(const void*, const int);
    const void* GetReadOnlyBits() const;
    int GetNumBitsRead() const;
    int GetNumBytesRead() const;
    bool SetNumBitsRead(const int);
};

template<typename T, typename ArrayItemType, unsigned SIZE>
inline bool datExportBuffer::SerArraySize(T& a, const ArrayItemType(&/*arr*/)[SIZE] ASSERT_ONLY(, const char* name))
{
    const unsigned numBits = datBitsNeeded<SIZE>::COUNT;

    return Verifyf(static_cast<unsigned>(a) <= static_cast<unsigned>(SIZE),
        "%s overflow on export. value[%u] max[%u]", name, static_cast<unsigned>(a), static_cast<unsigned>(SIZE))
        && this->WriteUns(a, numBits);
}

//PURPOSE
//  Used to import data from a bit buffer.
class datImportBuffer : public datBitBuffer
{
public:

    //PURPOSE
    //  Imports a user defined type object.
    //  Must be implemented by the application.
    template<typename T> bool SerUser(T& a) const
    {
        return datImport(*this, a);
    }

    template<typename T>
    bool SerUns(T& a, const int numBits) const {return this->ReadUns(a, numBits);}
    template<typename T>
    bool SerInt(T& a, const int numBits) const {return this->ReadInt(a, numBits);}
    template<typename T>
    bool SerFloat(T& a) const {return this->ReadFloat(a);}
    template<typename T>
    bool SerDouble(T& a) const {return this->ReadDouble(a);}
    template<typename T>
    bool SerBool(T& a) const {return this->ReadBool(a);}
    template<typename T>
    bool SerStr(T* a, const int maxChars) const {return this->ReadStr(a, maxChars);}
    template<typename T>
    bool SerBytes(T* a, const int numBytes) const {return this->ReadBytes(a, numBytes);}
    template<typename T>
    bool SerBits(T* a, const int numBits) const {return this->ReadBits(a, numBits, 0);}
	bool SerBits(datBitBuffer& a, const int numBits) const {return this->ReadBits(a, numBits);}
    //PURPOSE
    //  Imports a serialize-able object.
    template<typename T>
    bool SerSer(T& a) const {return a.Import(*this);}

    //PURPOSE
    //  Imports the number of used elements in an array.
    //PARAMS
    //  a    - The returned number of elements
    //  arr  - The array used to compute the bits needed for serialization and to verify the bounds
    //  name - Debug name of the array item
    template<typename T, typename ArrayItemType, unsigned SIZE>
    bool SerArraySize(T& a, const ArrayItemType(&/*arr*/)[SIZE] ASSERT_ONLY(, const char* name)) const;

private:
    void SetReadWriteBits(void*, const int, const int);
    void SetReadWriteBytes(void*, const int);
    int GetNumBitsWritten() const;
    int GetNumBytesWritten() const;
    bool SetNumBitsWritten(const int);

    bool WriteBits(const void*, const int, const int) const;
    bool WriteBits(const datBitBuffer&, const int) const;
    template<typename T> bool WriteUns(const T, const int) const;
    bool WriteUns(const u64, const int) const;
    template<typename T> bool WriteInt(const T, const int) const;
    bool WriteFloat(const float) const;
    bool WriteDouble(const double) const;
    bool WriteBool(const bool) const;
    bool WriteBytes(const void*, const int) const;
    bool WriteStr(const char*, const int) const;
    bool PokeBits(const void*, const int, const int, const int) const;
    bool PokeBytes(const void*, const int, const int) const;
    template<typename T> bool PokeUns(const T, const int, const int) const;
    template<typename T> bool PokeInt(const T, const int, const int) const;
    bool PokeStr(const char*, const int, const int) const;
    bool PokeFloat(const float, const int) const;
    bool PokeDouble(const double, const int) const;
    bool PokeBool(const bool, const int) const;
};

template<typename T, typename ArrayItemType, unsigned SIZE>
inline bool datImportBuffer::SerArraySize(T& a, const ArrayItemType(&/*arr*/)[SIZE] ASSERT_ONLY(, const char* name)) const
{
    const unsigned numBits = datBitsNeeded<SIZE>::COUNT;

    if (!this->ReadUns(a, numBits))
    {
        return false;
    }
    
    if (!Verifyf(static_cast<unsigned>(a) <= static_cast<unsigned>(SIZE),
        "%s overflow on import. value[%u] max[%u]", name, static_cast<unsigned>(a), static_cast<unsigned>(SIZE)))
    {
        // We set it to SIZE just in case someone forgets to check the return value.
        // Later on read data in the buffer might be wrong but it should prevent writing outside the array bounds.
        a = static_cast<T>(SIZE);
        return false;
    }

    return true;
}

//PURPOSE
//  Used to determine the size of exported data.  Nothing is actually written
//  to the buffer, but the size of the buffer if it were written can be
//  determined.
class datNullExportBuffer : public datExportBuffer
{
public:
    datNullExportBuffer()
    {
        this->SetNullWriteBytes(0x7FFFFFFF >> 3);
    }

private:
    void SetReadWriteBits(void*, const int, const int);
    void SetReadWriteBytes(void*, const int);

    void SetReadOnlyBits(const void*, const int, const int);
    void SetReadOnlyBytes(const void*, const int);
    const void* GetReadOnlyBits() const;
    int GetNumBitsRead() const;
    int GetNumBytesRead() const;
    bool SetNumBitsRead(const int);

    bool ReadBits(const void*, const int, const int) const;
    bool ReadBits(const datBitBuffer&, const int) const;
    template<typename T> bool ReadUns(const T, const int) const;
    bool ReadUns(const u64, const int) const;
    template<typename T> bool ReadInt(const T, const int) const;
    bool ReadFloat(const float) const;
    bool ReadDouble(const double) const;
    bool ReadBool(const bool) const;
    bool ReadBytes(const void*, const int) const;
    bool ReadStr(const char*, const int) const;
    bool PeekBits(const void*, const int, const int, const int) const;
    bool PeekBytes(const void*, const int, const int) const;
    template<typename T> bool PeekUns(const T, const int, const int) const;
    template<typename T> bool PeekInt(const T, const int, const int) const;
    bool PeekStr(const char*, const int, const int) const;
    bool PeekFloat(const float, const int) const;
    bool PeekDouble(const double, const int) const;
    bool PeekBool(const bool, const int) const;
};

}   //namespace rage

#endif  //DATA_BITBUFFER_H
