// 
// atl/atpreprocessor.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef ATL_ATPREPROCESSOR_H
#define ATL_ATPREPROCESSOR_H

#define AT_PASTE_I( a, b )  a##b
#define AT_PASTE( a, b )    AT_PASTE_I( a, b )

#define AT_STRINGIZE( x )  #x

//PURPOSE
//  Creates an enumeration of symbols.
//  Example:
//
//  AT_ENUM( 3, i )     //creates i0, i1, i2
//
#define AT_ENUM( count, P )     AT_ENUM_I( count, P )
#define AT_ENUM_I( count, P )   AT_ENUM_##count( P )
#define AT_ENUM_16( P )         AT_ENUM_15( P ),    P##15
#define AT_ENUM_15( P )         AT_ENUM_14( P ),    P##14
#define AT_ENUM_14( P )         AT_ENUM_13( P ),    P##13
#define AT_ENUM_13( P )         AT_ENUM_12( P ),    P##12
#define AT_ENUM_12( P )         AT_ENUM_11( P ),    P##11
#define AT_ENUM_11( P )         AT_ENUM_10( P ),    P##10
#define AT_ENUM_10( P )         AT_ENUM_9( P ),     P##9
#define AT_ENUM_9( P )          AT_ENUM_8( P ),     P##8
#define AT_ENUM_8( P )          AT_ENUM_7( P ),     P##7
#define AT_ENUM_7( P )          AT_ENUM_6( P ),     P##6
#define AT_ENUM_6( P )          AT_ENUM_5( P ),     P##5
#define AT_ENUM_5( P )          AT_ENUM_4( P ),     P##4
#define AT_ENUM_4( P )          AT_ENUM_3( P ),     P##3
#define AT_ENUM_3( P )          AT_ENUM_2( P ),     P##2
#define AT_ENUM_2( P )          AT_ENUM_1( P ),     P##1
#define AT_ENUM_1( P )          P##0
#define AT_ENUM_0( P )

//PURPOSE
//  Creates a parameter list.
//  Example:
//
//  AT_PARAMS( 3, T, t )     //creates T0 t0, T1 t1, T2 t2
//
#define AT_PARAMS( count, T, a )    AT_PARAMS_I( count, T, a )
#define AT_PARAMS_I( count, T, a )  AT_PARAMS_##count( T, a )
#define AT_PARAMS_16( T, a )        AT_PARAMS_15( T, a ),   T##15   a##15
#define AT_PARAMS_15( T, a )        AT_PARAMS_14( T, a ),   T##14   a##14
#define AT_PARAMS_14( T, a )        AT_PARAMS_13( T, a ),   T##13   a##13
#define AT_PARAMS_13( T, a )        AT_PARAMS_12( T, a ),   T##12   a##12
#define AT_PARAMS_12( T, a )        AT_PARAMS_11( T, a ),   T##11   a##11
#define AT_PARAMS_11( T, a )        AT_PARAMS_10( T, a ),   T##10   a##10
#define AT_PARAMS_10( T, a )        AT_PARAMS_9( T, a ),    T##9    a##9
#define AT_PARAMS_9( T, a )         AT_PARAMS_8( T, a ),    T##8    a##8
#define AT_PARAMS_8( T, a )         AT_PARAMS_7( T, a ),    T##7    a##7
#define AT_PARAMS_7( T, a )         AT_PARAMS_6( T, a ),    T##6    a##6
#define AT_PARAMS_6( T, a )         AT_PARAMS_5( T, a ),    T##5    a##5
#define AT_PARAMS_5( T, a )         AT_PARAMS_4( T, a ),    T##4    a##4
#define AT_PARAMS_4( T, a )         AT_PARAMS_3( T, a ),    T##3    a##3
#define AT_PARAMS_3( T, a )         AT_PARAMS_2( T, a ),    T##2    a##2
#define AT_PARAMS_2( T, a )         AT_PARAMS_1( T, a ),    T##1    a##1
#define AT_PARAMS_1( T, a )         T##0 a##0
#define AT_PARAMS_0( T, a )

//PURPOSE
//  Creates a declaration list.
//  Example:
//
//  AT_DECLARE( 3, T, t )     //creates T0 t0; T1 t1; T2 t2;
//
#define AT_DECLARE( count, T, a )   AT_DECLARE_I( count, T, a )
#define AT_DECLARE_I( count, T, a ) AT_DECLARE_##count( T, a )
#define AT_DECLARE_16( T, a )       AT_DECLARE_15( T, a );  T##15   a##15
#define AT_DECLARE_15( T, a )       AT_DECLARE_14( T, a );  T##14   a##14
#define AT_DECLARE_14( T, a )       AT_DECLARE_13( T, a );  T##13   a##13
#define AT_DECLARE_13( T, a )       AT_DECLARE_12( T, a );  T##12   a##12
#define AT_DECLARE_12( T, a )       AT_DECLARE_11( T, a );  T##11   a##11
#define AT_DECLARE_11( T, a )       AT_DECLARE_10( T, a );  T##10   a##10
#define AT_DECLARE_10( T, a )       AT_DECLARE_9( T, a );   T##9    a##9 
#define AT_DECLARE_9( T, a )        AT_DECLARE_8( T, a );   T##8    a##8 
#define AT_DECLARE_8( T, a )        AT_DECLARE_7( T, a );   T##7    a##7 
#define AT_DECLARE_7( T, a )        AT_DECLARE_6( T, a );   T##6    a##6 
#define AT_DECLARE_6( T, a )        AT_DECLARE_5( T, a );   T##5    a##5 
#define AT_DECLARE_5( T, a )        AT_DECLARE_4( T, a );   T##4    a##4 
#define AT_DECLARE_4( T, a )        AT_DECLARE_3( T, a );   T##3    a##3 
#define AT_DECLARE_3( T, a )        AT_DECLARE_2( T, a );   T##2    a##2 
#define AT_DECLARE_2( T, a )        AT_DECLARE_1( T, a );   T##1    a##1 
#define AT_DECLARE_1( T, a )        T##0 a##0
#define AT_DECLARE_0( T, a )

//PURPOSE
//  Creates a operation list.
//  Example:
//
//  AT_OP( 3, m_t, =, t )     //creates m_t0 = t0; m_t1 = t1; m_t2 = t2;
//
#define AT_OP( count, a, op, b )	AT_OP_I( count, a, op, b )
#define AT_OP_I( count, a, op, b )	AT_OP_##count( a, op, b )
#define AT_OP_16( a, op, b )		AT_OP_15( a, op, b );  a##15 op b##15
#define AT_OP_15( a, op, b )		AT_OP_14( a, op, b );  a##14 op b##14
#define AT_OP_14( a, op, b )		AT_OP_13( a, op, b );  a##13 op b##13
#define AT_OP_13( a, op, b )		AT_OP_12( a, op, b );  a##12 op b##12
#define AT_OP_12( a, op, b )		AT_OP_11( a, op, b );  a##11 op b##11
#define AT_OP_11( a, op, b )		AT_OP_10( a, op, b );  a##10 op b##10
#define AT_OP_10( a, op, b )		AT_OP_9( a, op, b );   a##9  op b##9 
#define AT_OP_9( a, op, b )			AT_OP_8( a, op, b );   a##8  op b##8
#define AT_OP_8( a, op, b )			AT_OP_7( a, op, b );   a##7  op b##7 
#define AT_OP_7( a, op, b )			AT_OP_6( a, op, b );   a##6  op b##6
#define AT_OP_6( a, op, b )			AT_OP_5( a, op, b );   a##5  op b##5 
#define AT_OP_5( a, op, b )			AT_OP_4( a, op, b );   a##4  op b##4
#define AT_OP_4( a, op, b )			AT_OP_3( a, op, b );   a##3  op b##3
#define AT_OP_3( a, op, b )			AT_OP_2( a, op, b );   a##2  op b##2
#define AT_OP_2( a, op, b )			AT_OP_1( a, op, b );   a##1  op b##1
#define AT_OP_1( a, op, b )			a##0 op b##0
#define AT_OP_0( a, op, b )

//PURPOSE
//  Creates a operation sequence separated.
//  Example:
//
//  AT_OP_SEQ( 3, m_t, ==, t, && )     //creates m_t0 == t0 && m_t1 == t1 && m_t2 == t2
//
#define AT_OP_SEQ( count, a, op, b, sep )	AT_OP_SEQ_I( count, a, op, b, sep )
#define AT_OP_SEQ_I( count, a, op, b, sep )	AT_OP_SEQ_##count( a, op, b, sep )
#define AT_OP_SEQ_16( a, op, b, sep )		AT_OP_SEQ_15( a, op, b, sep ) sep a##15 op b##15
#define AT_OP_SEQ_15( a, op, b, sep )		AT_OP_SEQ_14( a, op, b, sep ) sep a##14 op b##14
#define AT_OP_SEQ_14( a, op, b, sep )		AT_OP_SEQ_13( a, op, b, sep ) sep a##13 op b##13
#define AT_OP_SEQ_13( a, op, b, sep )		AT_OP_SEQ_12( a, op, b, sep ) sep a##12 op b##12
#define AT_OP_SEQ_12( a, op, b, sep )		AT_OP_SEQ_11( a, op, b, sep ) sep a##11 op b##11
#define AT_OP_SEQ_11( a, op, b, sep )		AT_OP_SEQ_10( a, op, b, sep ) sep a##10 op b##10
#define AT_OP_SEQ_10( a, op, b, sep )		AT_OP_SEQ_9( a, op, b, sep )  sep a##9  op b##9 
#define AT_OP_SEQ_9( a, op, b, sep )		AT_OP_SEQ_8( a, op, b, sep )  sep a##8  op b##8
#define AT_OP_SEQ_8( a, op, b, sep )		AT_OP_SEQ_7( a, op, b, sep )  sep a##7  op b##7 
#define AT_OP_SEQ_7( a, op, b, sep )		AT_OP_SEQ_6( a, op, b, sep )  sep a##6  op b##6
#define AT_OP_SEQ_6( a, op, b, sep )		AT_OP_SEQ_5( a, op, b, sep )  sep a##5  op b##5 
#define AT_OP_SEQ_5( a, op, b, sep )		AT_OP_SEQ_4( a, op, b, sep )  sep a##4  op b##4
#define AT_OP_SEQ_4( a, op, b, sep )		AT_OP_SEQ_3( a, op, b, sep )  sep a##3  op b##3
#define AT_OP_SEQ_3( a, op, b, sep )		AT_OP_SEQ_2( a, op, b, sep )  sep a##2  op b##2
#define AT_OP_SEQ_2( a, op, b, sep )		AT_OP_SEQ_1( a, op, b, sep )  sep a##1  op b##1
#define AT_OP_SEQ_1( a, op, b, sep )		a##0 op b##0
#define AT_OP_SEQ_0( a, op, b, sep )

//PURPOSE
//  Convenience macro to create an argument list.
//  Example:
//
//  AT_ARGS( 3, a )     //creates a0, a1, a2
//
#define AT_ARGS( count, a )     AT_ENUM( count, a )

#endif  //ATL_PREPROCESSOR_H
