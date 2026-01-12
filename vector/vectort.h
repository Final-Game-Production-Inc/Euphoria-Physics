// 
// vector/vectort.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef VECTOR_VECTORT_H
#define VECTOR_VECTORT_H

#include "data/resource.h"
#include "data/struct.h"
#include "math/channel.h"
#include "system/new.h"

namespace rage 
{
	
// PURPOSE:  A generalized vector class.  Resizable at runtime.
template <class _Type> class VectorT
{
public:

	// PURPOSE:  Default matrix constructor
	VectorT();

	// PURPOSE: Resource constructor
	VectorT(datResource&);

	// PURPOSE:  Construct a vector with the specified size.
	// PARAMS:  size - desired vector size
	//          value - initial value for each element of the vector
	VectorT(short size, _Type value);

	// PURPOSE:  Construct a copy of another vector.
	// PARAMS:  rhs - vector to copy
	VectorT(const VectorT<_Type>& rhs);

	// PURPOSE:  Destructor
	virtual ~VectorT();

	// PURPOSE: Placement
	DECLARE_PLACE(VectorT<_Type>);

	// PURPOSE: Offline resourcing
#if __DECLARESTRUCT
	void DeclareStruct(datTypeStruct&);
#endif // !__FINAL

	// PURPOSE:  Get the size in this vector
	// RETURNS:  the size of the vector
	short GetSize() const { return mSize; }

	// PURPOSE:  Get the data of this vector as an array
	const _Type* GetData() const { return mData; }

	// PURPOSE:  Data access
	// PARAMS:  index - element index
	_Type& operator[](short index) { return mData[index]; }
	const _Type& operator[](short index) const	{ return mData[index]; }

	// PURPOSE: Copy vector
	// PARAMS:  rhs - vector to copy
	VectorT& operator=(const VectorT<_Type>& rhs);

	// PURPOSE:  Initialize vector with specified size.
	// PARAMS:  size - desired vector size
	//          value - initial value for each element in vector
	void Init(short size, _Type value);

	// PURPOSE:  Calculate the dot product of this vector with another vector.
	// PARAMS:  rhs - other vector
	// RETURNS: the dot product
	_Type Dot(const VectorT<_Type>& rhs) const;

	// PURPOSE:  Calculate the dot product of this vector with an array.
	// PARAMS:  v2 - the array
	// RETURNS: the dot product
	_Type Dot(const _Type* v2) const;

	// PURPOSE:  Scales this n-vector by corresponding scaling terms found in the scaling vector.
	// PARAMS:  scalingVector - an n-vector containing the scaling terms for each element of this matrix
	// RETURNS:  true - if the supplied arguments have matching dimensions
	//           false - otherwise.  A will remain unchanged in this case.
	bool Scale(const VectorT<_Type>& scalingVector);

	// PURPOSE:  Sets this vector to the result of this+B.  The vectors
	//           must be of the same size.
	// PARAMS:  B - the vector that holds the value of B
	// RETURNS: true if this vector and B can be summed
	//          false otherwise.  This vector remains unchanged.
	bool Add(const VectorT<_Type>& B);

	// PURPOSE:  Adds this vector to B, component-wise.  The vectors
	//           must be of the same size.
	// PARAMS:  B - the vector that holds the value of B
	//          result - a vector to hold the result
	// RETURNS: true if this vector and B can be summed
	//          false otherwise.  result vector remains unchanged.
	bool Add(const VectorT<_Type>& B, VectorT<_Type>& result) const;

	// PURPOSE:  Sets this vector to the result of this-B.  The 
	//           vectors must be of the same size.
	// PARAMS:  B - the vector that holds the value of B
	// RETURNS: true if this and B can be differenced
	//          false otherwise.  This is left unchanged.
	bool Subtract(const VectorT<_Type>& B);

	// PURPOSE:  Computes this-B.  The 
	//           vectors must be of the same size.
	// PARAMS:  B - the vector that holds the value of B
	//          result - a vector to hold the result of this-B
	// RETURNS: true if this and B can be differenced
	//          false otherwise.  result is left unchanged.
	bool Subtract(const VectorT<_Type>& B, 
		VectorT<_Type>& result) const;

#if !__NO_OUTPUT
	// PURPOSE:  Prints the value of this vector to standard out
	// PARAMS:  label - a labeling string
	void Print(const char* label) const;
#endif

private:
	_Type* mData;
	short mSize;
};

////////////////////////////////////////////////////////////////////////////////

template<class _Type> VectorT<_Type>::VectorT()
{
	mSize = 0;
	mData = NULL;
}

////////////////////////////////////////////////////////////////////////////////

template<class _Type> VectorT<_Type>::VectorT(datResource& rsc)
{
	rsc.PointerFixup(mData);
}

////////////////////////////////////////////////////////////////////////////////

template<class _Type> VectorT<_Type>::VectorT(short size, _Type value)
{
	mSize = size;
	mData = rage_new _Type[mSize];
	for( short i =0; i < mSize; i++ )
		mData[i] = value;
}

////////////////////////////////////////////////////////////////////////////////

template<class _Type> VectorT<_Type>::VectorT(const VectorT<_Type>& rhs)
{
	mSize = rhs.mSize;
	mData = rage_new _Type[mSize];
	for( short i = 0; i < mSize; i++ )
		mData[i] = rhs[i];
}

////////////////////////////////////////////////////////////////////////////////

template<class _Type> VectorT<_Type>::~VectorT()
{
	if( mData != NULL )
		delete[] mData;
}


////////////////////////////////////////////////////////////////////////////////

template<class _Type> IMPLEMENT_PLACE(VectorT<_Type>)

////////////////////////////////////////////////////////////////////////////////

#if __DECLARESTRUCT
template<class _Type> void VectorT<_Type>::DeclareStruct(datTypeStruct& s)
{
	STRUCT_BEGIN(VectorT<_Type>);
	STRUCT_DYNAMIC_ARRAY(mData,mSize);
	STRUCT_FIELD(mSize);
	STRUCT_END();
}
#endif // __DECLARESTRUCT

////////////////////////////////////////////////////////////////////////////////

template<class _Type> VectorT<_Type>& VectorT<_Type>::operator=(const VectorT<_Type>& rhs)
{
	if( this != &rhs )
	{
		if( mSize != rhs.mSize )
		{
			if( mData != NULL )
				delete[] mData;

			mSize = rhs.mSize;
			mData = rage_new _Type[mSize];
		}

		for( short i = 0; i < mSize; i++ )
			mData[i] = rhs[i];
	}

	return *this;
}

////////////////////////////////////////////////////////////////////////////////

template<class _Type> void VectorT<_Type>::Init(short size, _Type value)
{
	if( mData != NULL )
		delete [] mData;

	mSize = size;
	mData = rage_new _Type[mSize];
	for( short i =0; i < mSize; i++ )
		mData[i] = value;
}

////////////////////////////////////////////////////////////////////////////////

template<class _Type> _Type VectorT<_Type>::Dot(const VectorT<_Type>& rhs) const
{
	if( rhs.mSize != mSize )
		mthWarningf( "VectorNF::dot(v)\n" );
	return Dot( &rhs[0] );
}

////////////////////////////////////////////////////////////////////////////////

template<class _Type> _Type VectorT<_Type>::Dot(const _Type* v2) const
{
	double sum = 0;
	const _Type* vp = &mData[0];
	const _Type* v2p = &v2[0];
	for( int i = 0; i < mSize; i++ )
		sum += *vp++ * *v2p++;
	return sum;
}

////////////////////////////////////////////////////////////////////////////////

template<class _Type> bool VectorT<_Type>::Scale(const VectorT<_Type>& scalingVector)
{
	if(this->GetSize() != scalingVector.GetSize())
	{
		mthErrorf("VectorT - Cannot scale vector by mismatched vector\n");
		return false;
	}

	for(short i = 0; i < this->GetSize(); i++)
	{
		mData[i] *= scalingVector[i];
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////

template<class _Type> bool VectorT<_Type>::Subtract(const VectorT<_Type>& B)
{
	return this->Subtract(B, *this);
}

////////////////////////////////////////////////////////////////////////////////

template<class _Type> bool VectorT<_Type>::Subtract(const VectorT<_Type>& B, 
								 VectorT<_Type>& result) const
{
	// Check Dimensions
	if(this->GetSize() != B.GetSize())
	{
		mthErrorf("VectorT - Cannot subtract vectors w/o identical dimensionality\n");
		return false;
	}

	// Initialize the result
	if(this != &result) result.Init(B.GetSize(), 0);

	// Add!
	for(short i = 0; i < this->GetSize(); i++)
	{
		result[i] = mData[i]-B[i];
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////

template<class _Type> bool VectorT<_Type>::Add(const VectorT<_Type>& B)
{
	return this->Add(B, *this);
}

////////////////////////////////////////////////////////////////////////////////

template<class _Type> bool VectorT<_Type>::Add(const VectorT<_Type>& B, 
								 VectorT<_Type>& result) const
{
	// Check Dimensions
	if(this->GetSize() != B.GetSize())
	{
		mthErrorf("VectorT - Cannot add vectors without identical dimensionality\n");
		return false;
	}

	// Initialize the result
	if(this != &result) result.Init(B.GetSize(), 0);

	// Add!
	for(short i = 0; i < this->GetSize(); i++)
	{
		result[i] = mData[i]+B[i];
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////

#if !__NO_OUTPUT
template<class _Type> void VectorT<_Type>::Print(const char* label) const
{
	Printf("%s Vector (%d):", label,mSize);
	for(short i = 0; i < mSize; i++)
	{
		Printf( " %g", mData[i]);
	}
	Printf("\n");
}
#endif

} // namespace rage

#endif
