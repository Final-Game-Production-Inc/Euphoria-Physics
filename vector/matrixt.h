// 
// vector/matrixt.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef VECTOR_MATRIXT_H
#define VECTOR_MATRIXT_H

#include "file/serialize.h"
#include "vector/vectort.h"

namespace rage 
{

// PURPOSE:  A generalized matrix class.  Resizable at runtime.
template <class _Type> class MatrixT
{
public:
	
	// PURPOSE:  Default matrix constructor
	MatrixT();
	
	// PURPOSE:  Construct a matrix with the specified number of rows and columns.
	//           Elements initialized to 0.
	// PARAMS:  nRows - number of desired rows
	//          nCols - number of desired columns
	MatrixT(short nRows, short nCols);

	// PURPOSE:  Construct a copy of another matrix.
	// PARAMS:  rhs - matrix to copy
	MatrixT(const MatrixT<_Type>& rhs);

	// PURPOSE: Resource constructor
	MatrixT(datResource&);

	// PURPOSE:  Destructor
	virtual ~MatrixT();

	// PURPOSE: Placement
	void Place(datResource&);

	// PURPOSE: Offline resourcing
#if __DECLARESTRUCT
	void DeclareStruct(datTypeStruct&);
#endif // !__FINAL

	// PURPOSE:  Get the number of rows in this matrix
	// RETURNS:  number of rows in the matrix
	short GetRows() const { return( mRows ); }

	// PURPOSE:  Get the number of columns in this matrix
	// RETURNS:  number of columns in the matrix
	short GetCols() const { return( mCols ); }

	// PURPOSE:  Row access
	// PARAMS:  index - row index
	// RETURNS:  pointer to row data
	_Type* operator[] (short index)	{ return mData[index]; }
	const _Type* operator[] (short index) const	{ return mData[index]; }

	// PURPOSE: Copy matrix
	// PARAMS:  rhs - matrix to copy
	MatrixT<_Type>& operator=(const MatrixT<_Type>& rhs);

	// PURPOSE:  Initialize matrix with specified number of rows and columns.
	// PARAMS:  rows - desired number of rows
	//          cols - desired number of columns
	//          value - initial value for each element in matrix
	void Init(short rows, short cols, _Type value);

	// PURPOSE: Make this matrix a square diagonal matrix with the specified diagonal values.
	// PARAMS: diagonal - a vector that holds the values of the diagonal of the
	//                    matrix
	void MakeDiagonal(const VectorT<_Type>& diagonal);

	// PURPOSE: Make this matrix a square identity matrix.
	// PARAMS: size - the size of the identity matrix
	void MakeIdentity(short size);

	// PURPOSE:  Copies a possibly smaller matrix into this one.  This matrix must be large enough to hold the original 
	//           matrix starting at the startRow and startCol.
	// PARAMS:  orignal - the matrix to copy
	//          startRow - the row of this matrix to start copying 
	//                     into
	//          startCol - the column of this matrix to start 
	//                     copying into
	// RETURNS: true if this matrix starting at the specified row 
	//               and column is big enough to accomodate the original
	//          false otherwise.  This matrix is left unchanged in 
	//                this case.
	bool SetSubMatrix(const MatrixT<_Type>& original, short startRow, 
		short startCol);

	// PURPOSE:  Extract a submatrix from this one
	// PARAMS:  rowStart - the index of the row to begin extracting from
	//          numRows - the number of rows to extract
	//          colStart - the index of the column to begin extracting from 
	//          numCols - the number of columns to extract
	//          subMatrix -  a matrix to hold the extracted matrix
	// RETURNS: true if this matrix is large enough to extract the specified
	//               submatrix
	//          false otherwise.  submatrix is left unchanged.
	bool GetSubMatrix(short rowStart, 
		short numRows, short colStart, short numCols, MatrixT<_Type>& subMatrix) const;

	// Multiply matrix by its own transpose; M*Mt if mRows <= mCols,
	// Mt*M otherwise.
	void MultiplyByTranspose(MatrixT<_Type>& result) const;

	// PURPOSE:  Dot product with a col of a matrix.
	// PARAMS:  id0 - start index in vector.
	//          id1 - end index in vector.
	//          m - vector to dot with
	//          col - the col index of the matrix.
	// RETURNS:	the dot product
	// NOTES: (id1-id0+1) should be equal to this->mRows. And less than m.mSize
	_Type DotCol(VectorT<_Type>& m, int col) const;

	// PURPOSE:  Dot product with a col of a matrix.
	// PARAMS:  id0 - start index in vector.
	//          id1 - end index in vector.
	//          m - vector to dot with
	//          col - the col index of the matrix.
	// RETURNS:	the dot product
	// NOTES: (id1-id0+1) should be equal to this->mRows. And less than m.mSize
	_Type DotCol(short id0, short id1, VectorT<_Type>& m, int col) const;

	// PURPOSE:  Transforms the vector X by this matrix.  This Matrix must have 
	//           the same number of columns as the size of X.
	// PARAMS:  X - the vector that holds the value of X
	//          B - a vector to hold the result of applying this matrix to X
	// RETURNS: true if this matrix can be applied to X
	//          false otherwise
	bool Transform(const VectorT<_Type>& X, VectorT<_Type>& B) const;

	// PURPOSE:  Compute the transpose of this matrix.
	// PARAMS:  transposedResult - a matrix to hold the result
	void Transpose(MatrixT<_Type>& transposedResult) const;

	// PURPOSE:  Scales this matrix by a multiplier.
	// PARAMS:  multiplier - the scalar value
	void Scale(const _Type multiplier);

	// PURPOSE:  Scales the m rows of this matrix by corresponding values in the supplied vector.
	// PARAMS:  scalingVector - an m-vector containing the scaling terms for each row of this matrix
	// RETURNS:  true - if the supplied arguments have matching dimensions
	//           false - otherwise.  A will remain unchanged in this case.
	bool ScaleRows(const VectorT<_Type>& scalingVector);

	// PURPOSE:  Scales the n columns of this matrix by corresponding values in the supplied vector.
	// PARAMS:  scalingVector - an n-vector containing the scaling terms for each column of this matrix
	// RETURNS:  true - if the supplied arguments have matching dimensions
	//           false - otherwise.  A will remain unchanged in this case.
	bool ScaleCols(const VectorT<_Type>& scalingVector);

	// PURPOSE:  Adds this matrix and B together, component-wise.  The 
	//           matrices must have the same dimensionality.
	// PARAMS:  B - the matrix that holds the value of B
	//          result - a matrix to hold the result of this+B
	// RETURNS: true if this and B can be summed
	//          false otherwise. result remains unchanged
	bool Add(const MatrixT<_Type>& B, MatrixT<_Type>& result) const;

	// PURPOSE:  Sets this matrix to this+B.  The 
	//           matrices must have the same dimensionality.
	// PARAMS:  B - the matrix that holds the value of B
	// RETURNS: true if this and B can be summed
	//          false otherwise. This matrix remains unchanged
	bool Add(const MatrixT<_Type>& B);

	// PURPOSE:  Computes this-B, component-wise.  The
	//           matrices must have the same dimensionality
	// PARAMS:  B - the matrix that holds the value of B
	//          result - a matrix to hold the result
	// RETURNS: true if this and B can be differenced
	//          false otherwise. result remains unchanged
	bool Subtract(const MatrixT<_Type>& B, MatrixT<_Type>& result) const;

	// PURPOSE:  Sets this matrix to the result of this-B.  The
	//           matrices must have the same dimensionality
	// PARAMS:  B - the matrix that holds the value of B
	// RETURNS: true if this and B can be differenced
	//          false otherwise. This matrix remains unchanged
	bool Subtract(const MatrixT<_Type>& B);

	// PURPOSE:  Matrix multiplication.  Compute this*B.   This matrix must have
	//           the same number of columns as B has rows.
	// PARAMS:  B - the matrix that holds the value of B
	//          result - a matrix to hold the result of this*B
	// RETURNS: true if this matrix and B can be multiplied
	//          false otherwise. result remains unchanged.
	bool Multiply(const MatrixT<_Type>& B, MatrixT<_Type>& result) const;

	// PURPOSE:  Computes the squared norm of this matrix 
	//           (i.e. the sum of the squared elements of this matrix).
	// RETURNS: the squared norm of this matrix
	_Type NormSquared() const;

#if !__NO_OUTPUT
	// PURPOSE:  Prints the value of this matrix to standard out
	// PARAMS:  label - a labeling string
	void	Print(const char* label) const;
#endif

	// PURPOSE: Serialize the MatrixT to/from a file
	void Serialize(datSerialize&);

private:
	_Type** mData;
	short mRows;
	short mCols;
};

// PURPOSE:  Produce a new matrix that is one matrix stacked on top of 
//           another.  The two matrices to be stacked must have the same
//           number of columns
// PARAMS:  top - the matrix that is to be stacked on top
//          bottom - the matrix that is to be on the bottom of the stack
//          stacked - a matrix to hold the computed stacked matrix
// RETURNS: true if the matrices can be stacked
//          false otherwise.  stacked left unchanged.
// SEE ALSO:  Append
template<class _Type> bool Stack(const MatrixT<_Type>& top, const MatrixT<_Type>& bottom, 
						  MatrixT<_Type>& stacked);

// PURPOSE:  Produce a new matrix that is one matrix appended to another
//           The two matrices to be appended must have the same number of
//           rows.
// PARAMS:  left - the matrix that is to be on the left
//          right - the matrix that is to be on the right
//          result - a matrix to hold the computed appended matrix
// RETURNS: true if the matrices can be appended
//          false otherwise.  stacked left unchanged.
// SEE ALSO:  Stack
template<class _Type> bool Append(const MatrixT<_Type>& left, const MatrixT<_Type>& right, 
								  MatrixT<_Type>& result);

// PURPOSE:  Compute a block hankel matrix from the data stored in A.
//           If A is (a11 a12 ... a1m) = (A1)^T
//                   (a21 a22 ... a2m) = (A2)^T
//                   (      ...      ) ...
//                   (an1 an2 ... anm) = (An)^T
//          then the resulting block hankel matrix is
//              (A1      A2       ...  Aj       )
//              (A2      A3       ...  A(j+1)   )
//              ( ...     ...     ...   ...     )
//              (Ai      A(i+1)   ...  A(j+i-1) )
//              (A(i+1)  A(i+2)   ...  A(j+i)   )
//              ( ...     ...     ...   ...     )
//              (A(2i)   A(2i+1)  ...  A(j+2i-1))
// PARAMS:  A - a matrix that holds the original data where row t  
//              holds the m-dimensional observed data at time t.
//          numBlockRows - the number of desired block rows in the Hankel 
//                         Matrix (referred to as i above)
//          numColsHankel - the number of desired columns in the Hankel 
//                          Matrix (referred to as j above)
//          hankelA - a matrix to hold the computed block hankel matrix 
//                    from the data stored in A.
template<class _Type> void ComputeBlockHankel(const MatrixT<_Type>& A, short numBlockRows,
									 short numColsHankel, MatrixT<_Type>& hankelA);

// PURPOSE:  Computes the Kronecker Tensor Product of the matrices A and B.  If
//               (a11 a12 ... a1n) 
//               (a21 a22 ... a2n) 
//           A = (... ... ... ...)
//               (am1 am2 ... amn)
//           then the kronecker tensor product of A and B is
//               (a11*B a12*B ... a1n*B)
//               (a21*B a22*B ... a2n*B)
//               ( ...   ...  ...  ... )
//               (am1*B am2*B ... amn*B)
//           notice that since B is an ixj matrix, the kronecker product of A 
//           and B is an (m*i)x(n*j) matrix.
// PARAMS:  A - the matrix that holds the value of A
//          B - the matrix that holds the value of B
//          result - the matrix to hold the resulting kronecker tensor 
//                   product
template<class _Type> void ComputeKronProduct(const MatrixT<_Type>& A, 
										  const MatrixT<_Type>& B, MatrixT<_Type>& result);

// PURPOSE:  Create a matrix from sample data.
// PARAMS:  samples - the number of samples. samples = rows.
//          degrees - the degree of polynomial. degrees+1 = cols.
//          data - the sampled parameters.
//          A - The output matrix. It is sample * (degrees+1) dimension.
// NOTES:  The matrix is:
//	| 1.0 X1 X1^2 ........ X1^m |
//	| 1.0 X2 X2^2 .........X2^m |
//	| ......................... |
//	| 1.0 Xn Xn^2 .........Xn^m |
template<class _Type> void CreateMatrixFromSamples(short samples, short degrees, const _Type* data, MatrixT<_Type>& A);
template<class _Type> void CreateMatrixFromSamples(short id0, short id1, short degrees, const _Type* data, MatrixT<_Type>& A);




//////////////////Implementation


////////////////////////////////////////////////////////////////////////////////

template<class _Type> MatrixT<_Type>::MatrixT()
{
	mRows = 0;
	mCols = 0;
	mData = NULL;
}

////////////////////////////////////////////////////////////////////////////////

template<class _Type> MatrixT<_Type>::MatrixT(short rows, short cols)
{
	mRows = rows;
	mCols = cols;
	mData = rage_new _Type*[mRows];
	mData[0] = rage_new _Type[mRows*mCols];

	// Initialize with 0.0f.
	for( short i = 1; i < mRows; i++ )
		mData[i] = mData[i-1] + mCols;
	for( short i = 0; i < mRows; i++ )
		for( short j = 0; j < mCols; j++ )
			mData[i][j] = 0;
}

////////////////////////////////////////////////////////////////////////////////

template<class _Type> MatrixT<_Type>::MatrixT(const MatrixT<_Type>& rhs)
{
	short i,j;
	mRows = rhs.mRows;
	mCols = rhs.mCols;
	mData = rage_new _Type*[mRows];
	mData[0] = rage_new _Type[mRows*mCols];
	for( i = 1; i < mRows; i++ )
		mData[i] = mData[i-1] + mCols;
	for( i = 0; i < mRows; i++ )
		for( j = 0; j < mCols; j++ )
			mData[i][j] = rhs[i][j];
}

////////////////////////////////////////////////////////////////////////////////

template<class _Type> MatrixT<_Type>::MatrixT(datResource& rsc)
{
	rsc.PointerFixup(mData);
	for(int i=0; i<mRows; ++i)
	{
		rsc.PointerFixup(mData[i]);
	}
}

////////////////////////////////////////////////////////////////////////////////

template<class _Type> MatrixT<_Type>::~MatrixT()
{
	if( mData != NULL )
	{
		delete[] mData[0];
		delete[] mData;
	}
}

////////////////////////////////////////////////////////////////////////////////

template<class _Type> void MatrixT<_Type>::Place(datResource& rsc)
{
	::new (this) MatrixT<_Type>(rsc);
}

////////////////////////////////////////////////////////////////////////////////

#if __DECLARESTRUCT
template<class _Type> void MatrixT<_Type>::DeclareStruct(datTypeStruct& s)
{
	STRUCT_BEGIN(MatrixT<_Type>);
	if(mData)
		for(int i=0; i<mRows; ++i)
		{
			for(int j=0; j<mCols; ++j)
				datSwapper(mData[i][j]);
			datSwapper((void*&)(mData[i]));
		}
	STRUCT_FIELD_VP(mData);
	STRUCT_FIELD(mRows);
	STRUCT_FIELD(mCols);
	STRUCT_END();
}
#endif // !__FINAL

////////////////////////////////////////////////////////////////////////////////

template<class _Type> MatrixT<_Type>& MatrixT<_Type>::operator=(const MatrixT<_Type>& rhs)
{
	if( this != &rhs )
	{
		short i,j;
		if( mRows != rhs.mRows || mCols != rhs.mCols )
		{
			if( mData != 0 )
			{
				delete[] (mData[0]);
				delete[] (mData);
			}
			mRows = rhs.mRows;
			mCols = rhs.mCols;
			mData = rage_new _Type*[mRows];
			mData[0] = rage_new _Type[mRows*mCols];
		}

		for( i = 1; i < mRows; i++ )
			mData[i] = mData[i-1] + mCols;
		for( i = 0; i < mRows; i++ )
			for( j = 0; j < mCols; j++ )
				mData[i][j] = rhs[i][j];
	}

	return *this;
}

////////////////////////////////////////////////////////////////////////////////

template<class _Type> void MatrixT<_Type>::Init(short rows, short cols, _Type value)
{
	// Delete data, if there are some.
	if( mData != NULL )
	{
		delete[] mData[0];
		delete[] mData;
	}
	mRows = rows;
	mCols = cols;
	mData = rage_new _Type*[mRows];
	mData[0] = rage_new _Type[mRows*mCols];

	for( short i = 1; i < mRows; i++ )
		mData[i] = mData[i-1] + mCols;
	for( short i = 0; i < mRows; i++ )
		for( short j = 0; j < mCols; j++ )
			mData[i][j] = value;
}

////////////////////////////////////////////////////////////////////////////////

template<class _Type> void MatrixT<_Type>::MakeDiagonal(const VectorT<_Type>& diagonal)
{
	Init(diagonal.GetSize(), diagonal.GetSize(), 0);
	for(short i = 0; i < diagonal.GetSize(); i++)
	{
		mData[i][i] = diagonal[i];
	}
}

////////////////////////////////////////////////////////////////////////////////

template<class _Type> void MatrixT<_Type>::MakeIdentity(short size)
{
	Init(size, size, 0);
	for(short i = 0; i < size; i++)
	{
		mData[i][i] = 1;
	}
}

////////////////////////////////////////////////////////////////////////////////

template<class _Type> bool MatrixT<_Type>::SetSubMatrix(const MatrixT<_Type>& original, short startRow,
									   short startCol)
{
	if((this->GetRows() >= startRow+original.GetRows()) && 
	   (this->GetCols() >= startCol+original.GetCols()))
	{
		for(short i = 0; i < original.GetRows(); i++)
		{
			for(short j = 0; j < original.GetCols(); j++)
			{
				mData[startRow+i][startCol+j] = original[i][j];
			}
		}

		return true;
	}
	else
	{
		mthErrorf("MatrixT - Cannot set submatrix.  Matrix not large enough\n");
		return false;
	}
}

////////////////////////////////////////////////////////////////////////////////

template<class _Type> bool MatrixT<_Type>::GetSubMatrix(short rowStart, 
		short numRows, short colStart, short numCols, MatrixT<_Type>& subMatrix) const
{
	if((numRows+rowStart > this->GetRows()) || (numCols+colStart > this->GetCols()))
	{
		mthErrorf("MatrixT - Cannot get submatrix.  Matrix not large enough\n");
		return false;
	}

	subMatrix.Init(numRows, numCols, 0);

	for(short i = 0; i < numRows; i++)
	{
		for(short j = 0; j < numCols; j++)
		{
			subMatrix[i][j] = mData[rowStart+i][colStart+j];
		}
	}

	return true;
}
////////////////////////////////////////////////////////////////////////////////

template<class _Type> _Type MatrixT<_Type>::DotCol(VectorT<_Type>& m, int col) const
{
	return this->DotCol(0, m.GetSize()-1, m, col);
}

////////////////////////////////////////////////////////////////////////////////

template<class _Type> _Type MatrixT<_Type>::DotCol(short id0, short id1, VectorT<_Type>& m, int col) const
{
	if( (id1 - id0 + 1) != this->mRows )
		mthWarningf( "MatrixT::dot(id0, id1, m,c)\n" );

	_Type sum = 0, *vp = &m[id0], *mp = &mData[0][col];
	for( int i = id0; i <= id1; i++, mp += this->mCols )
		sum += *vp++ * *mp;
	return sum;
}

////////////////////////////////////////////////////////////////////////////////

template<class _Type> void MatrixT<_Type>::MultiplyByTranspose(MatrixT<_Type>& result) const
{
	if( mRows < mCols )
	{
		// M*Mt
		result.Init(mRows, mRows, 0.0f);

		for( short i = 0; i < mRows; i++ )
		{
			for( short j = 0; j <= i; j++ )
			{
				// dot row i with row j
				_Type sum = 0;
				for( short k = 0; k < mCols; k++ )
					sum += mData[i][k] * mData[j][k];

				// Assign to symmetric places in result
				result[i][j] = sum;
				result[j][i] = sum;
			}
		}
	}
	else
	{
		// Mt*M
		result.Init(mCols, mCols, 0.0f);

		for( short i = 0; i < mCols; i++ )
		{
			for( short j = 0; j <= i; j++ )
			{
				// Dot column i with column j
				_Type sum = 0;
				for( short k = 0; k < mRows; k++ )
					sum += mData[k][i] * mData[k][j];

				// assign to symmetric places in result
				result[i][j] = sum;
				result[j][i] = sum;
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

template<class _Type> void MatrixT<_Type>::Transpose(MatrixT<_Type>& transposedResult) const
{
	transposedResult.Init(this->GetCols(), this->GetRows(), 0);
	for(short i = 0; i < this->GetRows(); i++)
	{
		for(short j = 0; j < this->GetCols(); j++)
		{
			transposedResult[j][i] = mData[i][j];
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

template<class _Type> bool MatrixT<_Type>::Transform(const VectorT<_Type>& X, 
								  VectorT<_Type>& B) const
{
	// Check Dimensions
	if(this->GetCols() != X.GetSize())
	{
		mthErrorf("MatrixT - Cannot apply matrix w/o proper dimensionality\n");
		return false;
	}

	// Initialize the result
	B.Init(this->GetRows(), 0);

	// Multiply!
	for(short i = 0; i < this->GetRows(); i++)
	{
		for(short j = 0; j < X.GetSize(); j++)
		{
			B[i] += mData[i][j]*X[j];
		}
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////

template<class _Type> void MatrixT<_Type>::Scale(const _Type multiplier)
{
	for(short i = 0; i < this->GetRows(); i++)
	{
		for(short j = 0; j < this->GetCols(); j++)
		{
			mData[i][j] *= multiplier;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

template<class _Type> bool MatrixT<_Type>::ScaleRows(const VectorT<_Type>& scalingVector)
{
	if(this->GetRows() != scalingVector.GetSize())
	{
		mthErrorf("MatrixT - Cannot scale rows of matrix by mismatched vector\n");
		return false;
	}

	for(short i = 0; i < this->GetRows(); i++)
	{
		float multiplier = scalingVector[i];
		for(short j = 0; j < this->GetCols(); j++)
		{
			mData[i][j] *= multiplier;
		}
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////

template<class _Type> bool MatrixT<_Type>::ScaleCols(const VectorT<_Type>& scalingVector)
{
	if(this->GetCols() != scalingVector.GetSize())
	{
		mthErrorf("MatrixT - Cannot scale columns of matrix by mismatched vector\n");
		return false;
	}

	for(short j = 0; j < this->GetCols(); j++)
	{
		float multiplier = scalingVector[j];
		for(short i = 0; i < this->GetRows(); i++)
		{
			mData[i][j] *= multiplier;
		}
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////

template<class _Type> bool MatrixT<_Type>::Add(const MatrixT<_Type>& B)
{
	return this->Add(B, *this);
}

////////////////////////////////////////////////////////////////////////////////

template<class _Type> bool MatrixT<_Type>::Add(const MatrixT<_Type>& B, MatrixT<_Type>& result) const
{
	// Check Dimensions
	if((this->GetRows() != B.GetRows()) || (this->GetCols() != B.GetCols()))
	{
		mthErrorf("MatrixT - Cannot add matrices without matching dimensions\n");
		return false;
	}

	// Initialize the result
	if(this != &result) result.Init(this->GetRows(), this->GetCols(), 0);

	// Add!
	for(short i = 0; i < this->GetRows(); i++)
	{
		for(short j = 0; j < this->GetCols(); j++)
		{
			result[i][j] = mData[i][j]+B[i][j];
		}
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////

template<class _Type> bool MatrixT<_Type>::Subtract(const MatrixT<_Type>& B)
{
	return this->Subtract(B, *this);
}

////////////////////////////////////////////////////////////////////////////////

template<class _Type> bool MatrixT<_Type>::Subtract(const MatrixT<_Type>& B, MatrixT<_Type>& result) const
{
	// Check Dimensions
	if((this->GetRows() != B.GetRows()) || (this->GetCols() != B.GetCols()))
	{
		mthErrorf("MatrixT - Cannot subtract matrices w/o matching dimensions\n");
		return false;
	}

	// Initialize the result
	if(this != &result) result.Init(this->GetRows(), this->GetCols(), 0);

	// Add!
	for(short i = 0; i < this->GetRows(); i++)
	{
		for(short j = 0; j < this->GetCols(); j++)
		{
			result[i][j] = mData[i][j]-B[i][j];
		}
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////

template<class _Type> bool MatrixT<_Type>::Multiply(const MatrixT<_Type>& B, MatrixT<_Type>& result) const
{
	// Check Dimensions
	if(!(this->GetCols() == B.GetRows()))
	{
		mthErrorf("MatrixT - Cannot multiply matrices w/o matching dimensions\n");
		return false;
	}

	// Initialize the result
	result.Init(this->GetRows(), B.GetCols(), 0);

	// Multiply!
	for(short i = 0; i < this->GetRows(); i++)
	{
		for(short j = 0; j < B.GetCols(); j++)
		{
			for(short k = 0; k < this->GetCols(); k++)
			{
				result[i][j] += mData[i][k]*B[k][j];
			}
		}
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////

template<class _Type> _Type MatrixT<_Type>::NormSquared() const
{
	_Type sum = 0;
	for(short i = 0; i < this->GetRows(); i++)
	{
		for(short j = 0; j < this->GetCols(); j++)
		{
			sum += square(mData[i][j]);
		}
	}

	return sum;
}

////////////////////////////////////////////////////////////////////////////////

#if !__NO_OUTPUT
template<class _Type> void MatrixT<_Type>::Print(const char* label) const
{
	Printf("%s Matrix (%dx%d):\n", label,mRows,mCols);
	for(short r = 0; r < mRows; r++)
	{
		for( short c = 0; c < mCols; c++ )
			Printf (" %g", mData[r][c]);
		Printf("\n");
	}
	Printf("\n");
}
#endif

////////////////////////////////////////////////////////////////////////////////

template<class _Type> void MatrixT<_Type>::Serialize(datSerialize& s)
{
	const int latestVersion = 1;

	int version = latestVersion;
	s << datLabel("MatrixTVersion:") << version << datNewLine;

	if(version != latestVersion)
	{
		mthErrorf("MatrixT - attempting to load version '%d' (only '%d' supported)", version, latestVersion);
		return;
	}

	s << datLabel("Size:") << this->mRows << this->mCols << datNewLine;

	if(s.IsRead())
	{
		this->Init(mRows, mCols, 0);
	}

	for(short i = 0; i < mRows; i++)
	{
		for(short j = 0; j < mCols; j++)
		{
			s << this->mData[i][j];
		}
		s << datNewLine;
	}
}

////////////////////////////////////////////////////////////////////////////////

template<class _Type> bool Stack(const MatrixT<_Type>& top, 
									  const MatrixT<_Type>& bottom, 
									  MatrixT<_Type>& stacked)
{
	short numCols = top.GetCols();
	if(numCols != bottom.GetCols())
	{
		mthErrorf("Stack - Cannot stack matrices without matching number of columns\n");
		return false;
	}

	stacked.Init(top.GetRows() + bottom.GetRows(), numCols, 0);

	for(short i = 0; i < top.GetRows(); i++)
	{
		for(short j = 0; j < numCols; j++)
		{
			stacked[i][j] = top[i][j];
		}
	}

	short numRowsTop = top.GetRows();

	for(short i = 0; i < bottom.GetRows(); i++)
	{
		for(short j = 0; j < numCols; j++)
		{
			stacked[numRowsTop+i][j] = bottom[i][j];
		}
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////

template<class _Type> bool Append(const MatrixT<_Type>& left, 
									   const MatrixT<_Type>& right, MatrixT<_Type>& result)
{
	short numRows = left.GetRows();
	if(numRows != right.GetRows())
	{
		mthErrorf("Append - Cannot append matrices without matching number of rows\n");
		return false;
	}

	result.Init(numRows, left.GetCols() + right.GetCols(), 0);

	short numColsLeft = left.GetCols();

	for(short i = 0; i < numRows; i++)
	{
		for(short j = 0; j < result.GetCols(); j++)
		{
			if(j < numColsLeft)
			{
				result[i][j] = left[i][j];
			}
			else
			{
				result[i][j] = right[i][j-numColsLeft];
			}
		}
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////

template<class _Type> void ComputeBlockHankel(const MatrixT<_Type>& A, short numBlockRows,
									 short numColsHankel, MatrixT<_Type>& hankelA)
{
	short numDOFs = A.GetCols();

	short numRowsHankel = 2*numDOFs*numBlockRows;

	hankelA.Init(numRowsHankel, numColsHankel, 0);

	for(short i = 0; i < 2*numBlockRows; i++)
	{
		for(short j = 0; j < numColsHankel; j++)
		{
			short sampleNum = i+j;
			for(short k = 0; k < numDOFs; k++)
			{
				hankelA[i*numDOFs+k][j] = A[sampleNum][k];
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

template<class _Type> void ComputeKronProduct(const MatrixT<_Type>& A, 
										  const MatrixT<_Type>& B, MatrixT<_Type>& result)
{
	MatrixT<_Type> temp1;

	result.Init(A.GetRows()*B.GetRows(), A.GetCols()*B.GetCols(), 0);
	for(short i = 0; i < A.GetRows(); i++)
	{
		for(short j = 0; j < A.GetCols(); j++)
		{
			temp1 = B;
			_Type multiplier = A[i][j];
			temp1.Scale(multiplier);
			result.SetSubMatrix(temp1, i*B.GetRows(), j*B.GetCols());
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

template<class _Type> void CreateMatrixFromSamples(short samples, short degrees, const _Type* data, MatrixT<_Type>& A)
{
	CreateMatrixFromSamples(0, samples-1, degrees, data, A);
}

////////////////////////////////////////////////////////////////////////////////

template<class _Type> void CreateMatrixFromSamples(short id0, short id1, short degrees, const _Type* data, MatrixT<_Type>& A)
{
	A.Init((id1 - id0 + 1), (degrees+1), 0);
	_Type st = data[id0];
	for( short i = id0; i <= id1; i++ )
	{
		_Type t = data[i] - st;
		_Type a = 1.0f;
		A[i-id0][0] = a;
		for(short j = 1; j <= degrees; j++)
		{
			a *= t;
			A[i-id0][j] = a;
		}
	}
}

} // namespace rage

#endif
