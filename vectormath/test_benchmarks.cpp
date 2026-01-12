#if 0 //!__DEV // RELEASE ONLY

// DOM-IGNORE-BEGIN

//#include "grprofile/pix.h"
//#define BOOL int
//#define DWORD long
//#include "Tracerecording.h"
#include "system/new.h"
#if !__SPU
#include "system/ipc.h"
#include "test_benchmarks.h"
#endif

#if __XENON
#include <fstream> // for printing trig graphs
#endif

// Whether to benchmark.
#define RUN_BENCH_TESTS 0
// Whether to output the trig graphs in csv format (XENON only).
#define DRAW_TRIG_GRAPHS 0

// Whether to benchmark the new library or the old library, or the others.
#define NEW_RAGE_VEC_LIB 1
#define OLD_RAGE_VEC_LIB 0

// The # of tests to run per-operation. (This is repeated 200 times, and an average is taken.)
#define NUM_TESTS 100000

// So that vector/matrix ops saved into stack variables don't get optimized away,
// we need to access one value of theirs (at an index the compiler doesn't know about).
rage::u32 g_alwaysZero = 0;


// NOTE 1: Since the XENON/WIN32 benchmarks have been finished, each of NEW RAGE, OLD RAGE, and SONY benchmark methods have been
// modified slightly (storing result to heap after timing is finished). If XENON/WIN32 benchmarks are repeated, and results are
// different, check the disasm (or e-mail wpfeil@rockstarsandiego.com).

// NOTE 2: The benchmarks are kept in tact and up to date, however notice that the OLD RAGE trig benchmarks do not have SIMD versions,
// so the NEW RAGE's scalar float trig funcs are measured in those test slots. It is only for reasons of convenience that this was temporarily done.




// The four configurations:

//#if __XENON && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)	// XMVECTOR
//#elif __PS3 && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)	// SONY'S
//#elif NEW_RAGE_VEC_LIB && (!OLD_RAGE_VEC_LIB)				// NEW RAGE
//#elif OLD_RAGE_VEC_LIB && (!NEW_RAGE_VEC_LIB)				// OLD RAGE
//#else
//	error;
//#endif

// Include the vector libraries.
#if __XENON && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
#include "vectormath/xboxmath_rage.h"
#elif __PPU && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
#include "C:/usr/local/cell/target/ppu/include/vectormath/cpp/vectormath_aos.h"
#include "C:/usr/local/cell/target/ppu/include/bits/sce_math.h"
#elif __SPU && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
#include "C:/usr/local/cell/target/spu/include/vectormath/cpp/vectormath_aos.h"
#include "C:/usr/local/cell/target/spu/include/bits/sce_math.h"
#include "C:/usr/local/cell/target/spu/include/spu_intrinsics_gcc.h"
	#define VEC4_TYPE vec_float4
#elif NEW_RAGE_VEC_LIB && (!OLD_RAGE_VEC_LIB) 
#include "classes.h"
	#if __SPU
	#define VEC4_TYPE rage::Vec::Vector_4V
	#endif
#elif OLD_RAGE_VEC_LIB && (!NEW_RAGE_VEC_LIB)
	#if __SPU
	#include "vector/vector3_consts_spu.cpp"
	#endif
#include "vector/vector3.h"
#include "vector/vector4.h"
#include "vector/matrix33.h"
#include "vector/matrix34.h"
#include "vector/matrix44.h"

// New lib, for scalar trig benchmarking. Doing this under "OLD_RAGE_VEC_LIB" just b/c it's convenient.
#include "vectormath/mathops.h"
	#if __SPU
	#define VEC4_TYPE __vector4
	#endif
#else
	error "Undefined combination... WIN32 w/ no new/old rage vec lib, doesn't have a vec lib to fall back on."
#endif

	// Useful for not allowing a Vector_4V result to optimize away (SPU).
#if __SPU
	void foo( float ) __attribute__((noinline));
	void foo( float ) {}
	void foo( VEC4_TYPE ) __attribute__((noinline));
	void foo( VEC4_TYPE ) {}
	void foo( VEC4_TYPE, VEC4_TYPE ) __attribute__((noinline));
	void foo( VEC4_TYPE, VEC4_TYPE ) {}
	void foo( VEC4_TYPE, VEC4_TYPE, VEC4_TYPE ) __attribute__((noinline));
	void foo( VEC4_TYPE, VEC4_TYPE, VEC4_TYPE ) {}
	void foo( VEC4_TYPE, VEC4_TYPE, VEC4_TYPE, VEC4_TYPE ) __attribute__((noinline));
	void foo( VEC4_TYPE, VEC4_TYPE, VEC4_TYPE, VEC4_TYPE ) {}
#endif

// Utility.
#include "system/timer.h"

namespace rage
{
namespace vecBenchmarks
{
#if !__SPU
	typedef float (*my_test_fnptr)();
	my_test_fnptr funcs[38] =
	{
		Benchmark_DetM33,
		Benchmark_DetM44,
		Benchmark_MulM44M44,
		Benchmark_MulM44V4,
		Benchmark_MulV4M44,
		Benchmark_MulM33M33,
		Benchmark_MulM33V3,
		Benchmark_MulV3M33,
		Benchmark_TransM34M34,
		Benchmark_TransV3M34V3,
		Benchmark_TransP3M34V3,

		Benchmark_InvM44,
		Benchmark_InvM33,
		Benchmark_InvTransM34,
		Benchmark_InvTransOrthoM34,
		Benchmark_UnTransOrthoM44M44,
		Benchmark_UnTransOrthoM44V4,
		Benchmark_UnTransOrthoM33M33,
		Benchmark_UnTransOrthoM33V3,
		Benchmark_UnTransOrthoM34M34,
		Benchmark_UnTransV3OrthoM34V3,
		Benchmark_UnTransP3OrthoM34V3,

		Benchmark_SinAndCos,
		Benchmark_Sin,
		Benchmark_Cos,
		Benchmark_Tan,
		Benchmark_Asin,
		Benchmark_Acos,
		Benchmark_Atan,
		Benchmark_Atan2,

		Benchmark_FASTSinAndCos,
		Benchmark_FASTSin,
		Benchmark_FASTCos,
		Benchmark_FASTTan,
		Benchmark_FASTAsin,
		Benchmark_FASTAcos,
		Benchmark_FASTAtan,
		Benchmark_FASTAtan2
	};
#endif

	void Run()
	{

		float fResults[38];
		for( int i = 0; i < 38; i++ )
			fResults[i] = 0.0f;

		// Sleep before starting the tests.
		// (Only seems to be necessary on PS3 PPU, for stability.)
#if __PPU
		sysIpcSleep( 10000 );
#endif

#if RUN_BENCH_TESTS
#if !__SPU
		// Take averages.
		for( int funcnum = 0; funcnum <= 37; funcnum++ )
		{
			for( int i = 0; i < 200; i++ )
				fResults[funcnum] += funcs[funcnum]();
			fResults[funcnum] /= 200.0f;
		}
#else
		// Manually unrolled for SPU... it doesn't like static initialization of 
		// function pointers in PIC.
		for( int i = 0; i < 200; i++ )
			fResults[0] += Benchmark_DetM33();
		fResults[0] /= 200.0f;
		for( int i = 0; i < 200; i++ )
			fResults[1] += Benchmark_DetM44();
		fResults[1] /= 200.0f;
		for( int i = 0; i < 200; i++ )
			fResults[2] += Benchmark_MulM44M44();
		fResults[2] /= 200.0f;
		for( int i = 0; i < 200; i++ )
			fResults[3] += Benchmark_MulM44V4();
		fResults[3] /= 200.0f;
		for( int i = 0; i < 200; i++ )
			fResults[4] += Benchmark_MulV4M44();
		fResults[4] /= 200.0f;
		for( int i = 0; i < 200; i++ )
			fResults[5] += Benchmark_MulM33M33();
		fResults[5] /= 200.0f;
		for( int i = 0; i < 200; i++ )
			fResults[6] += Benchmark_MulM33V3();
		fResults[6] /= 200.0f;
		for( int i = 0; i < 200; i++ )
			fResults[7] += Benchmark_MulV3M33();
		fResults[7] /= 200.0f;
		for( int i = 0; i < 200; i++ )
			fResults[8] += Benchmark_TransM34M34();
		fResults[8] /= 200.0f;
		for( int i = 0; i < 200; i++ )
			fResults[9] += Benchmark_TransV3M34V3();
		fResults[9] /= 200.0f;
		for( int i = 0; i < 200; i++ )
			fResults[10] += Benchmark_TransP3M34V3();
		fResults[10] /= 200.0f;
		for( int i = 0; i < 200; i++ )
			fResults[11] += Benchmark_InvM44();
		fResults[11] /= 200.0f;
		for( int i = 0; i < 200; i++ )
			fResults[12] += Benchmark_InvM33();
		fResults[12] /= 200.0f;
		for( int i = 0; i < 200; i++ )
			fResults[13] += Benchmark_InvTransM34();
		fResults[13] /= 200.0f;
		for( int i = 0; i < 200; i++ )
			fResults[14] += Benchmark_InvTransOrthoM34();
		fResults[14] /= 200.0f;
		for( int i = 0; i < 200; i++ )
			fResults[15] += Benchmark_UnTransOrthoM44M44();
		fResults[15] /= 200.0f;
		for( int i = 0; i < 200; i++ )
			fResults[16] += Benchmark_UnTransOrthoM44V4();
		fResults[16] /= 200.0f;
		for( int i = 0; i < 200; i++ )
			fResults[17] += Benchmark_UnTransOrthoM33M33();
		fResults[17] /= 200.0f;
		for( int i = 0; i < 200; i++ )
			fResults[18] += Benchmark_UnTransOrthoM33V3();
		fResults[18] /= 200.0f;
		for( int i = 0; i < 200; i++ )
			fResults[19] += Benchmark_UnTransOrthoM34M34();
		fResults[19] /= 200.0f;
		for( int i = 0; i < 200; i++ )
			fResults[20] += Benchmark_UnTransV3OrthoM34V3();
		fResults[20] /= 200.0f;
		for( int i = 0; i < 200; i++ )
			fResults[21] += Benchmark_UnTransP3OrthoM34V3();
		fResults[21] /= 200.0f;
#endif
#endif // RUN_BENCH_TESTS

		for( int i = 0; i <= 37; i++ )
		{
			mthErrorf( "Result %i: %f us\n", i, fResults[i] );
		}

	
		//================================================
		// Draw trig graphs.
		//================================================
#if __XENON && DRAW_TRIG_GRAPHS

		std::ofstream outFile0;
		std::ofstream outFile1;
		std::ofstream outFile2;
		std::ofstream outFile3;
		std::ofstream outFile4;
		std::ofstream outFile5;
		std::ofstream outFile6;
		std::ofstream outFile7;
		std::ofstream outFile8;
		std::ofstream outFile0_Fast;
		std::ofstream outFile1_Fast;
		std::ofstream outFile2_Fast;
		std::ofstream outFile3_Fast;
		std::ofstream outFile4_Fast;
		std::ofstream outFile5_Fast;
		std::ofstream outFile6_Fast;
		std::ofstream outFile7_Fast;
		std::ofstream outFile8_Fast;
		outFile0.open( "devkit:\\newrage_sinandcos_sin.csv" );
		outFile1.open( "devkit:\\newrage_sinandcos_cos.csv" );
		outFile2.open( "devkit:\\newrage_sin.csv" );
		outFile3.open( "devkit:\\newrage_cos.csv" );
		outFile4.open( "devkit:\\newrage_tan.csv" );
		outFile5.open( "devkit:\\newrage_asin.csv" );
		outFile6.open( "devkit:\\newrage_acos.csv" );
		outFile7.open( "devkit:\\newrage_atan.csv" );
		outFile8.open( "devkit:\\newrage_atan2.csv" );
		outFile0_Fast.open( "devkit:\\fast_newrage_sinandcos_sin.csv" );
		outFile1_Fast.open( "devkit:\\fast_newrage_sinandcos_cos.csv" );
		outFile2_Fast.open( "devkit:\\fast_newrage_sin.csv" );
		outFile3_Fast.open( "devkit:\\fast_newrage_cos.csv" );
		outFile4_Fast.open( "devkit:\\fast_newrage_tan.csv" );
		outFile5_Fast.open( "devkit:\\fast_newrage_asin.csv" );
		outFile6_Fast.open( "devkit:\\fast_newrage_acos.csv" );
		outFile7_Fast.open( "devkit:\\fast_newrage_atan.csv" );
		outFile8_Fast.open( "devkit:\\fast_newrage_atan2.csv" );

		const int numSamples = 3000;
		const float bottomOfRange = -5.0f;
		const float topOfRange = 5.0f;
	
		Vec::Vector_4V* bigVecArray1;
		Vec::Vector_4V* bigVecArray2;
		Vec::Vector_4V* outVecArray1;
		Vec::Vector_4V* outVecArray2;
		bigVecArray1 = rage_new Vec::Vector_4V[numSamples];
		bigVecArray2 = rage_new Vec::Vector_4V[numSamples];
		outVecArray1 = rage_new Vec::Vector_4V[numSamples];
		outVecArray2 = rage_new Vec::Vector_4V[numSamples];
		for(int i = 0; i < numSamples; i++)
		{
			Vec::V4Set( bigVecArray1[i], (bottomOfRange+(topOfRange-bottomOfRange)*i/(numSamples-1)) );
			Vec::V4Set( bigVecArray2[i], (bottomOfRange+(topOfRange-bottomOfRange)*i/(numSamples-1)) );
		}

		for( int trigFunc = 0; trigFunc < 18; trigFunc++ )
		{
			switch (trigFunc)
			{
			case (0):
				for(int i = 0; i < numSamples; i++)
				{
					Vec::V4SinAndCos( outVecArray1[i], outVecArray2[i], bigVecArray1[i] );
				}
				for(int i = 0; i < numSamples; i++)
				{
					float val = Vec::GetX( outVecArray1[i] );
					outFile0 << (bottomOfRange+(topOfRange-bottomOfRange)*i/(numSamples-1)) << "; " << val << std::endl;
				}
				break;
			case (1):
				for(int i = 0; i < numSamples; i++)
				{
					Vec::V4SinAndCos( outVecArray1[i], outVecArray2[i], bigVecArray1[i] );
				}
				for(int i = 0; i < numSamples; i++)
				{
					float val = Vec::GetX( outVecArray2[i] );
					outFile1 << (bottomOfRange+(topOfRange-bottomOfRange)*i/(numSamples-1)) << "; " << val << std::endl;
				}
				break;
			case (2):
				for(int i = 0; i < numSamples; i++)
				{
					outVecArray1[i] = Vec::V4Sin( bigVecArray1[i] );
				}
				for(int i = 0; i < numSamples; i++)
				{
					float val = Vec::GetX( outVecArray1[i] );
					outFile2 << (bottomOfRange+(topOfRange-bottomOfRange)*i/(numSamples-1)) << "; " << val << std::endl;
				}
				break;
			case (3):
				for(int i = 0; i < numSamples; i++)
				{
					outVecArray1[i] = Vec::V4Cos( bigVecArray1[i] );
				}
				for(int i = 0; i < numSamples; i++)
				{
					float val = Vec::GetX( outVecArray1[i] );
					outFile3 << (bottomOfRange+(topOfRange-bottomOfRange)*i/(numSamples-1)) << "; " << val << std::endl;
				}
				break;
			case (4):
				for(int i = 0; i < numSamples; i++)
				{
					outVecArray1[i] = Vec::V4Tan( bigVecArray1[i] );
				}
				for(int i = 0; i < numSamples; i++)
				{
					float val = Vec::GetX( outVecArray1[i] );
					outFile4 << (bottomOfRange+(topOfRange-bottomOfRange)*i/(numSamples-1)) << "; " << val << std::endl;
				}
				break;
			case (5):
				for(int i = 0; i < numSamples; i++)
				{
					outVecArray1[i] = Vec::V4Arcsin( bigVecArray1[i] );
				}
				for(int i = 0; i < numSamples; i++)
				{
					float val = Vec::GetX( outVecArray1[i] );
					outFile5 << (bottomOfRange+(topOfRange-bottomOfRange)*i/(numSamples-1)) << "; " << val << std::endl;
				}
				break;
			case (6):
				for(int i = 0; i < numSamples; i++)
				{
					outVecArray1[i] = Vec::V4Arccos( bigVecArray1[i] );
				}
				for(int i = 0; i < numSamples; i++)
				{
					float val = Vec::GetX( outVecArray1[i] );
					outFile6 << (bottomOfRange+(topOfRange-bottomOfRange)*i/(numSamples-1)) << "; " << val << std::endl;
				}
				break;
			case (7):
				for(int i = 0; i < numSamples; i++)
				{
					outVecArray1[i] = Vec::V4Arctan( bigVecArray1[i] );
				}
				for(int i = 0; i < numSamples; i++)
				{
					float val = Vec::GetX( outVecArray1[i] );
					outFile7 << (bottomOfRange+(topOfRange-bottomOfRange)*i/(numSamples-1)) << "; " << val << std::endl;
				}
				break;
			case (8):
				// TODO: Need 4 test cases here (one for each quadrant)
				for(int i = 0; i < numSamples; i++)
				{
					outVecArray1[i] = Vec::V4Arctan2( bigVecArray1[i], bigVecArray2[i] );
				}
				for(int i = 0; i < numSamples; i++)
				{
					float val = Vec::GetX( outVecArray1[i] );
					outFile8 << (bottomOfRange+(topOfRange-bottomOfRange)*i/(numSamples-1)) << "; " << val << std::endl;
				}
				break;
			case (9):
				for(int i = 0; i < numSamples; i++)
				{
					Vec::V4SinAndCosFast( outVecArray1[i], outVecArray2[i], bigVecArray1[i] );
				}
				for(int i = 0; i < numSamples; i++)
				{
					float val = Vec::GetX( outVecArray1[i] );
					outFile0_Fast << (bottomOfRange+(topOfRange-bottomOfRange)*i/(numSamples-1)) << "; " << val << std::endl;
				}
				break;
			case (10):
				for(int i = 0; i < numSamples; i++)
				{
					Vec::V4SinAndCosFast( outVecArray1[i], outVecArray2[i], bigVecArray1[i] );
				}
				for(int i = 0; i < numSamples; i++)
				{
					float val = Vec::GetX( outVecArray2[i] );
					outFile1_Fast << (bottomOfRange+(topOfRange-bottomOfRange)*i/(numSamples-1)) << "; " << val << std::endl;
				}
				break;
			case (11):
				for(int i = 0; i < numSamples; i++)
				{
					outVecArray1[i] = Vec::V4SinFast( bigVecArray1[i] );
				}
				for(int i = 0; i < numSamples; i++)
				{
					float val = Vec::GetX( outVecArray1[i] );
					outFile2_Fast << (bottomOfRange+(topOfRange-bottomOfRange)*i/(numSamples-1)) << "; " << val << std::endl;
				}
				break;
			case (12):
				for(int i = 0; i < numSamples; i++)
				{
					outVecArray1[i] = Vec::V4CosFast( bigVecArray1[i] );
				}
				for(int i = 0; i < numSamples; i++)
				{
					float val = Vec::GetX( outVecArray1[i] );
					outFile3_Fast << (bottomOfRange+(topOfRange-bottomOfRange)*i/(numSamples-1)) << "; " << val << std::endl;
				}
				break;
			case (13):
				for(int i = 0; i < numSamples; i++)
				{
					outVecArray1[i] = Vec::V4TanFast( bigVecArray1[i] );
				}
				for(int i = 0; i < numSamples; i++)
				{
					float val = Vec::GetX( outVecArray1[i] );
					outFile4_Fast << (bottomOfRange+(topOfRange-bottomOfRange)*i/(numSamples-1)) << "; " << val << std::endl;
				}
				break;
			case (14):
				for(int i = 0; i < numSamples; i++)
				{
					outVecArray1[i] = Vec::V4ArcsinFast( bigVecArray1[i] );
				}
				for(int i = 0; i < numSamples; i++)
				{
					float val = Vec::GetX( outVecArray1[i] );
					outFile5_Fast << (bottomOfRange+(topOfRange-bottomOfRange)*i/(numSamples-1)) << "; " << val << std::endl;
				}
				break;
			case (15):
				for(int i = 0; i < numSamples; i++)
				{
					outVecArray1[i] = Vec::V4ArccosFast( bigVecArray1[i] );
				}
				for(int i = 0; i < numSamples; i++)
				{
					float val = Vec::GetX( outVecArray1[i] );
					outFile6_Fast << (bottomOfRange+(topOfRange-bottomOfRange)*i/(numSamples-1)) << "; " << val << std::endl;
				}
				break;
			case (16):
				for(int i = 0; i < numSamples; i++)
				{
					outVecArray1[i] = Vec::V4ArctanFast( bigVecArray1[i] );
				}
				for(int i = 0; i < numSamples; i++)
				{
					float val = Vec::GetX( outVecArray1[i] );
					outFile7_Fast << (bottomOfRange+(topOfRange-bottomOfRange)*i/(numSamples-1)) << "; " << val << std::endl;
				}
				break;
			case (17):
				// TODO: Need 4 test cases here (one for each quadrant)
				for(int i = 0; i < numSamples; i++)
				{
					outVecArray1[i] = Vec::V4Arctan2Fast( bigVecArray1[i], bigVecArray2[i] );
				}
				for(int i = 0; i < numSamples; i++)
				{
					float val = Vec::GetX( outVecArray1[i] );
					outFile8_Fast << (bottomOfRange+(topOfRange-bottomOfRange)*i/(numSamples-1)) << "; " << val << std::endl;
				}
				break;
			};
		}
		
		delete [] bigVecArray1;
		delete [] bigVecArray2;
		delete [] outVecArray1;
		delete [] outVecArray2;

		outFile0.close();
		outFile1.close();
		outFile2.close();
		outFile3.close();
		outFile4.close();
		outFile5.close();
		outFile6.close();
		outFile7.close();
		outFile8.close();
		outFile0_Fast.close();
		outFile1_Fast.close();
		outFile2_Fast.close();
		outFile3_Fast.close();
		outFile4_Fast.close();
		outFile5_Fast.close();
		outFile6_Fast.close();
		outFile7_Fast.close();
		outFile8_Fast.close();
#endif // DRAW_TRIG_GRAPHS

	}


	//================================================
	//
	//================================================
	float Benchmark_DetM33()
	{
		float fTime = -1.0f;
		sysTimer tm;

#if __XENON && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace xb;
		// N/A
#elif __PS3 && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace Vectormath;
		using namespace Aos;
	#if !__SPU
		floatInVec* pV = rage_new floatInVec;
	#endif

		Matrix3 matTemp = Matrix3::identity();
	#if __SPU
		float vecTemp;
		vec_float4 vec4Temp;
	#else
		floatInVec vecTemp;
	#endif
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
	#if __SPU
			vecTemp = determinant( matTemp );
			vec4Temp = spu_splats( vecTemp );
			matTemp.setCol0( Vector3(vec4Temp) );
			matTemp.setCol1( Vector3(vec4Temp) );
			matTemp.setCol2( Vector3(vec4Temp) );
	#else
			vecTemp = determinant( matTemp );
			matTemp.setCol0( Vector3(vecTemp) );
			matTemp.setCol1( Vector3(vecTemp) );
			matTemp.setCol2( Vector3(vecTemp) );
	#endif
		}
		fTime = tm.GetUsTime();
	#if !__SPU
		(*pV) = vecTemp;
		delete pV;
	#else
		foo( vecTemp );
	#endif
#elif NEW_RAGE_VEC_LIB && (!OLD_RAGE_VEC_LIB) 
	#if !__SPU
		ScalarV* pV = rage_new ScalarV;
	#endif

		Mat33V matTemp(V_ZERO);
		ScalarV vecTemp(V_ZERO);
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp = Determinant( matTemp );
			matTemp.SetCols( Vec3V(vecTemp) );
		}
		fTime = tm.GetUsTime();
	#if !__SPU
		(*pV) = vecTemp;
		delete pV;
	#else
		foo( vecTemp.GetIntrin128() );
	#endif
#elif OLD_RAGE_VEC_LIB && (!NEW_RAGE_VEC_LIB)
	#if !__SPU
		float* pV = rage_new float;
	#endif

		Matrix33 matTemp;
		matTemp.Identity();
		float vecTemp = 0.0f;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp = matTemp.Determinant();
			Vector3 temp(vecTemp,vecTemp,vecTemp);
			matTemp.a = matTemp.b = matTemp.c = temp;
		}
		fTime = tm.GetUsTime();

	#if !__SPU
		(*pV) = vecTemp;
		delete pV;
	#else
		foo( vecTemp );
	#endif
#else
		error;
#endif

		return fTime;
	}


	//================================================
	//
	//================================================
	float Benchmark_DetM44()
	{
		float fTime = -1.0f;
		sysTimer tm;

#if __XENON && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace xb;

		XMMATRIX matTemp = XMMatrixIdentity();
		XMVECTOR vecTemp;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp = XMMatrixDeterminant( matTemp );
			matTemp.r[0] = matTemp.r[1] = matTemp.r[2] = matTemp.r[3] = vecTemp;
		}
		fTime = tm.GetUsTime();
#elif __PS3 && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace Vectormath;
		using namespace Aos;
	#if !__SPU
		floatInVec* pV = rage_new floatInVec;
	#endif

		Matrix4 matTemp = Matrix4::identity();
	#if __SPU
		float vecTemp;
		vec_float4 vec4Temp;
	#else
		floatInVec vecTemp;
	#endif
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
	#if __SPU
			vecTemp = determinant( matTemp );
			vec4Temp = spu_splats( vecTemp );
			matTemp.setCol0( Vector4(vec4Temp) );
			matTemp.setCol1( Vector4(vec4Temp) );
			matTemp.setCol2( Vector4(vec4Temp) );
			matTemp.setCol3( Vector4(vec4Temp) );
	#else
			vecTemp = determinant( matTemp );
			matTemp.setCol0( Vector4(vecTemp) );
			matTemp.setCol1( Vector4(vecTemp) );
			matTemp.setCol2( Vector4(vecTemp) );
			matTemp.setCol3( Vector4(vecTemp) );
	#endif
		}
		fTime = tm.GetUsTime();
	#if !__SPU		
		(*pV) = vecTemp;
		delete pV;
	#else
		foo( vecTemp );
	#endif		
#elif NEW_RAGE_VEC_LIB && (!OLD_RAGE_VEC_LIB)
	#if !__SPU
		ScalarV* pV = rage_new ScalarV;
	#endif

		Mat44V matTemp(V_IDENTITY);
		ScalarV vecTemp(V_ZERO);
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp = Determinant( matTemp );
			matTemp.SetCols( Vec4V(vecTemp) );
		}
		fTime = tm.GetUsTime();
	#if !__SPU
		(*pV) = vecTemp;
		delete pV;
	#else
		foo( vecTemp.GetIntrin128() );
	#endif
#elif OLD_RAGE_VEC_LIB && (!NEW_RAGE_VEC_LIB)
	#if !__SPU
		float* pV = rage_new float;

		Matrix44 matTemp;
		matTemp.Identity();
		float vecTemp = 0.0f;
		Vector4 vecTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp = matTemp.Determinant();
			vecTemp2.Set( vecTemp );
			matTemp.a = matTemp.b = matTemp.c = matTemp.d = vecTemp2;
		}
		fTime = tm.GetUsTime();
		(*pV) = vecTemp;
		delete pV;
	#endif
#else
		error;
#endif

		return fTime;
	}

	//================================================
	//
	//================================================
	float Benchmark_MulM44M44()
	{
		float fTime = -1.0f;
		sysTimer tm;

#if __XENON && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace xb;

		XMMATRIX matTemp = XMMatrixIdentity();
		XMMATRIX matTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			matTemp2 = XMMatrixMultiply( matTemp, matTemp );
			matTemp = XMMatrixMultiply( matTemp2, matTemp2 );
		}
		fTime = tm.GetUsTime();
#elif __PS3 && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace Vectormath;
		using namespace Aos;
	#if !__SPU
		Matrix4* pV = rage_new Matrix4;
	#endif

		Matrix4 matTemp1 = Matrix4::identity();
		Matrix4 matTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			matTemp2 = matTemp1 * matTemp1;
			matTemp1 = matTemp2 * matTemp2;
		}
		fTime = tm.GetUsTime();
	#if !__SPU
		(*pV) = matTemp1;
		delete pV;
	#else
		foo( matTemp1.getCol0().get128(),matTemp1.getCol1().get128(),matTemp1.getCol2().get128(),matTemp1.getCol3().get128() );
	#endif
#elif NEW_RAGE_VEC_LIB && (!OLD_RAGE_VEC_LIB)
	#if !__SPU
		Mat44V* pV = rage_new Mat44V;
	#endif
	
		Mat44V matTemp(V_IDENTITY);
		Mat44V matTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			Multiply( matTemp2, matTemp, matTemp );
			Multiply( matTemp, matTemp2, matTemp2 );
		}
		fTime = tm.GetUsTime();
	#if !__SPU
		(*pV) = matTemp;
		delete pV;
	#else
		foo( matTemp.GetCol0Intrin128(),matTemp.GetCol1Intrin128(),matTemp.GetCol2Intrin128(),matTemp.GetCol3Intrin128() );
	#endif
#elif OLD_RAGE_VEC_LIB && (!NEW_RAGE_VEC_LIB)
	#if !__SPU
		Matrix44* pV = rage_new Matrix44;
	#endif

		Matrix44 matTemp;
		matTemp.Identity();
		Matrix44 matTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			matTemp2.Dot( matTemp, matTemp );
			matTemp.Dot( matTemp2, matTemp2 );
		}
		fTime = tm.GetUsTime();
	#if !__SPU
		(*pV) = matTemp;
		delete pV;
	#else
		foo( matTemp.a.xyzw,matTemp.b.xyzw,matTemp.c.xyzw,matTemp.d.xyzw );
	#endif
#else
		error;
#endif

		return fTime;
	}

	//================================================
	//
	//================================================
	float Benchmark_MulM44V4()
	{
		float fTime = -1.0f;
		sysTimer tm;

#if __XENON && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace xb;
		// N/A
#elif __PS3 && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace Vectormath;
		using namespace Aos;
	#if !__SPU
		Vector4* pV = rage_new Vector4;
	#endif

		Matrix4 matTemp = Matrix4::identity();
	#if __SPU
		Vector4 vecTemp( (vec_float4)spu_splats(0.0f) );
	#else
		Vector4 vecTemp( (vec_float4)vec_splat_s32(0) );
	#endif
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp = matTemp * vecTemp;
			matTemp.setCol0( vecTemp );
			matTemp.setCol1( vecTemp );
			matTemp.setCol2( vecTemp );
			matTemp.setCol3( vecTemp );
		}
		fTime = tm.GetUsTime();

		Vector4 temp = vecTemp + vecTemp;
	#if !__SPU
		(*pV) = temp;
		delete pV;
	#else
		foo( temp.get128() );
	#endif
#elif NEW_RAGE_VEC_LIB && (!OLD_RAGE_VEC_LIB) 
	#if !__SPU
		Vec4V* pV = rage_new Vec4V;
	#endif
	
		Mat44V matTemp(V_IDENTITY);
		Vec4V vecTemp(V_ZERO);
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp = Multiply( matTemp, vecTemp );

			// Need this to keep the transpose from being optimized away... that wouldn't be fair.
			matTemp.SetCols( vecTemp );
		}
		fTime = tm.GetUsTime();

		// Dummy instructions.
		// Without the first one, a "stvx" happens inside the loop.
		// Without the second one, the loop will never even happen, since the optimizer gets rid of ret-by-val values if unused.
		Vec4V temp = Add( vecTemp, vecTemp );
	#if !__SPU
		(*pV) = temp;
		delete pV;
	#else
		foo( temp.GetIntrin128() );
	#endif
#elif OLD_RAGE_VEC_LIB && (!NEW_RAGE_VEC_LIB)
		// N/A
#else
		error;
#endif

		return fTime;
	}

	//================================================
	//
	//================================================
	float Benchmark_MulV4M44()
	{
		float fTime = -1.0f;
		sysTimer tm;

#if __XENON && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace xb;
		XMVECTOR* pV = rage_new XMVECTOR;
		
		// Warm-up, to get into vector registers (as would be the normal case).

		XMMATRIX matTemp = XMMatrixIdentity();
		XMVECTOR vecTemp;
		XMDUMMY_INITIALIZE_VECTOR(vecTemp);
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp = XMVector4Transform( vecTemp, matTemp );

			// We need the code to re-calculate the matrix transpose again, for it to be a fair comparison...
			matTemp.r[0] = matTemp.r[1] = matTemp.r[2] = matTemp.r[3] = vecTemp;
		}
		fTime = tm.GetUsTime();

		// Dummy instructions.
		// Without the first one, a "stvx" happens inside the loop.
		// Without the second one, the loop will never even happen, since the optimizer gets rid of ret-by-val values if unused.
		XMVECTOR temp = XMVectorAdd( vecTemp, vecTemp );
		(*pV) = temp;
		delete pV;
#elif __PS3 && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace Vectormath;
		using namespace Aos;
		// N/A
#elif NEW_RAGE_VEC_LIB && (!OLD_RAGE_VEC_LIB) 
	#if !__SPU
		Vec4V* pV = rage_new Vec4V;
	#endif

		Mat44V matTemp = Mat44V(V_IDENTITY);
		Vec4V vecTemp = Vec4V(V_ZERO);
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp = Multiply( vecTemp, matTemp );

			// We need the code to re-calculate the matrix transpose again, for it to be a fair comparison...
			matTemp.SetCols( vecTemp );
		}
		fTime = tm.GetUsTime();

		// Dummy instructions.
		// Without the first one, a "stvx" happens inside the loop.
		// Without the second one, the loop will never even happen, since the optimizer gets rid of ret-by-val values if unused.
		Vec4V temp = Add( vecTemp, vecTemp );
	#if !__SPU
		(*pV) = temp;
		delete pV;
	#else
		foo( temp.GetIntrin128() );
	#endif
#elif OLD_RAGE_VEC_LIB && (!NEW_RAGE_VEC_LIB)
	#if !__SPU
		Vector4* pV = rage_new Vector4;
	#endif

		Matrix44 matTemp;
		matTemp.Identity();
		Vector4 vecTemp;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			matTemp.Transform( vecTemp, vecTemp );
			matTemp.a = matTemp.b = matTemp.c = matTemp.d = vecTemp;
		}
		fTime = tm.GetUsTime();

		// Dummy instructions.
		// Without the first one, a "stvx" happens inside the loop.
		// Without the second one, the loop will never even happen, since the optimizer gets rid of ret-by-val values if unused.
		Vector4 temp;
		temp.Add( vecTemp, vecTemp );
	#if !__SPU
		(*pV) = temp;
		delete pV;
	#else
		foo( temp.xyzw );
	#endif
#else
		error;
#endif
		return fTime;
	}

	//================================================
	//
	//================================================
	float Benchmark_MulM33M33()
	{
		float fTime = -1.0f;
		sysTimer tm;

#if __XENON && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace xb;
		// N/A
#elif __PS3 && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace Vectormath;
		using namespace Aos;
	#if !__SPU
		Matrix3* pV = rage_new Matrix3;
	#endif

		Matrix3 matTemp = Matrix3::identity();
		Matrix3 matTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			matTemp2 = matTemp * matTemp;
			matTemp = matTemp2 * matTemp2;
		}
		fTime = tm.GetUsTime();

	#if !__SPU
		(*pV) = matTemp;
		delete pV;
	#else
		foo( matTemp.getCol0().get128(),matTemp.getCol1().get128(),matTemp.getCol2().get128() );
	#endif
#elif NEW_RAGE_VEC_LIB && (!OLD_RAGE_VEC_LIB) 
	#if !__SPU
		Mat33V* pV = rage_new Mat33V;
	#endif

		Mat33V matTemp(V_IDENTITY);
		Mat33V matTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			Multiply( matTemp2, matTemp, matTemp );
			Multiply( matTemp, matTemp2, matTemp2 );
		}
		fTime = tm.GetUsTime();

	#if !__SPU
		(*pV) = matTemp;
		delete pV;
	#else
		foo( matTemp.GetCol0Intrin128(),matTemp.GetCol1Intrin128(),matTemp.GetCol2Intrin128() );
	#endif
#elif OLD_RAGE_VEC_LIB && (!NEW_RAGE_VEC_LIB)
	#if !__SPU
		Matrix33* pV = rage_new Matrix33;
	#endif

		Matrix33 matTemp;
		matTemp.Identity();
		Matrix33 matTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			matTemp2.Dot( matTemp, matTemp );
			matTemp.Dot( matTemp2, matTemp2 );
		}
		fTime = tm.GetUsTime();
	#if !__SPU
		(*pV) = matTemp;
		delete pV;
	#else
		foo( matTemp.a.xyzw,matTemp.b.xyzw,matTemp.c.xyzw );
	#endif
#else
		error;
#endif
		return fTime;
	}

	//================================================
	//
	//================================================
	float Benchmark_MulM33V3()
	{
		float fTime = -1.0f;
		sysTimer tm;

#if __XENON && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace xb;
		// N/A
#elif __PS3 && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace Vectormath;
		using namespace Aos;
	#if !__SPU
		Aos::Vector3* pV = rage_new Aos::Vector3;
	#endif

		Matrix3 matTemp = Matrix3::identity();
	#if __SPU
		Aos::Vector3 vecTemp = Aos::Vector3( (vec_float4)spu_splats(0.0f) );
	#else
		Aos::Vector3 vecTemp = Aos::Vector3( (vec_float4)vec_splat_s32(1) );
	#endif
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp = matTemp * vecTemp;

			matTemp.setCol0( vecTemp );
			matTemp.setCol1( vecTemp );
			matTemp.setCol2( vecTemp );
		}
		fTime = tm.GetUsTime();

		// Dummy instructions.
		// Without the first one, a "stvx" happens inside the loop.
		// Without the second one, the loop will never even happen, since the optimizer gets rid of ret-by-val values if unused.
		Aos::Vector3 temp = vecTemp + vecTemp;
	#if !__SPU
		(*pV) = temp;
		delete pV;
	#else
		foo( temp.get128() );
	#endif
#elif NEW_RAGE_VEC_LIB && (!OLD_RAGE_VEC_LIB)
	#if !__SPU
		Vec3V* pV = rage_new Vec3V;
	#endif
	
		Mat33V matTemp = Mat33V(V_IDENTITY);
		Vec3V vecTemp = Vec3V( Vec::V4VConstant(V_INT_1) );
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp = Multiply( matTemp, vecTemp );

			// We need the code to re-calculate the matrix transpose again, for it to be a fair comparison...
			matTemp.SetCols( vecTemp );
		}
		fTime = tm.GetUsTime();

		// Dummy instructions.
		// Without the first one, a "stvx" happens inside the loop.
		// Without the second one, the loop will never even happen, since the optimizer gets rid of ret-by-val values if unused.
		Vec3V temp = Add( vecTemp, vecTemp );
	#if !__SPU
		(*pV) = temp;
		delete pV;
	#else
		foo( temp.GetIntrin128() );
	#endif

#elif OLD_RAGE_VEC_LIB && (!NEW_RAGE_VEC_LIB)
		// N/A
#else
		error;
#endif

		return fTime;
	}

	//================================================
	//
	//================================================
	float Benchmark_MulV3M33()
	{
		float fTime = -1.0f;
		sysTimer tm;

#if __XENON && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace xb;
		// N/A
#elif __PS3 && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace Vectormath;
		using namespace Aos;
		// N/A
#elif NEW_RAGE_VEC_LIB && (!OLD_RAGE_VEC_LIB) 
	#if !__SPU
		Vec3V* pV = rage_new Vec3V;
	#endif

		Mat33V matTemp = Mat33V(V_IDENTITY);
		Vec3V vecTemp = Vec3V( Vec::V4VConstant(V_INT_1) );
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp = Multiply( vecTemp, matTemp );

			// We need the code to re-calculate the matrix transpose again, for it to be a fair comparison...
			matTemp.SetCols( vecTemp );
		}
		fTime = tm.GetUsTime();

	#if !__SPU
		(*pV) = vecTemp;
		delete pV;
	#else
		foo( vecTemp.GetIntrin128() );
	#endif
#elif OLD_RAGE_VEC_LIB && (!NEW_RAGE_VEC_LIB)
	#if !__SPU
		Vector3* pV = rage_new Vector3;
	#endif

		Matrix33 matTemp;
		matTemp.Identity();
		Vector3 vecTemp;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			matTemp.Transform( vecTemp, vecTemp );

			// We need the code to re-calculate the matrix transpose again (where applicable), for it to be a fair comparison...
			matTemp.a = matTemp.b = matTemp.c = vecTemp;
		}
		fTime = tm.GetUsTime();

	#if !__SPU
		(*pV) = vecTemp;
		delete pV;
	#else
		foo( vecTemp.xyzw );
	#endif
#else
		error;
#endif
		return fTime;
	}

	//================================================
	//
	//================================================
	float Benchmark_TransM34M34()
	{
		float fTime = -1.0f;
		sysTimer tm;

#if __XENON && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace xb;
		// N/A
#elif __PS3 && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace Vectormath;
		using namespace Aos;
	#if !__SPU
		Transform3* pV = rage_new Transform3;
	#endif

		Transform3 matTemp1 = Transform3::identity();
		Transform3 matTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			matTemp2 = matTemp1 * matTemp1;
			matTemp1 = matTemp2 * matTemp2;
		}
		fTime = tm.GetUsTime();

	#if !__SPU
		(*pV) = matTemp1;
		delete pV;
	#else
		foo( matTemp1.getCol0().get128(),matTemp1.getCol1().get128(),matTemp1.getCol2().get128(),matTemp1.getCol3().get128() );
	#endif
#elif NEW_RAGE_VEC_LIB && (!OLD_RAGE_VEC_LIB) 
	#if !__SPU
		Mat34V* pV = rage_new Mat34V;
	#endif

		Mat34V matTemp1 = Mat34V(V_IDENTITY);
		Mat34V matTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			Transform( matTemp2, matTemp1, matTemp1 );
			Transform( matTemp1, matTemp2, matTemp2 );
		}
		fTime = tm.GetUsTime();

	#if !__SPU
		(*pV) = matTemp1;
		delete pV;
	#else
		foo( matTemp1.GetCol0Intrin128(),matTemp1.GetCol1Intrin128(),matTemp1.GetCol2Intrin128(),matTemp1.GetCol3Intrin128() );
	#endif
#elif OLD_RAGE_VEC_LIB && (!NEW_RAGE_VEC_LIB)
	#if !__SPU
		Matrix34* pV = rage_new Matrix34;
	#endif

		Matrix34 matTemp1;
		matTemp1.Identity();
		Matrix34 matTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			matTemp2.Dot( matTemp1, matTemp1 );
			matTemp1.Dot( matTemp2, matTemp2 );
		}
		fTime = tm.GetUsTime();

	#if !__SPU
		(*pV) = matTemp1;
		delete pV;
	#else
		foo( matTemp1.a.xyzw,matTemp1.b.xyzw,matTemp1.c.xyzw,matTemp1.d.xyzw );
	#endif
#else
		error;
#endif

		return fTime;
	}

	//================================================
	//
	//================================================
	float Benchmark_TransV3M34V3()
	{
		float fTime = -1.0f;
		sysTimer tm;

#if __XENON && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace xb;
		// N/A
#elif __PS3 && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace Vectormath;
		using namespace Aos;
	#if !__SPU
		Aos::Vector3* pV = rage_new Aos::Vector3;
	#endif

		Transform3 matTemp = Transform3::identity();
	#if __SPU
		Aos::Vector3 vecTemp = Aos::Vector3((vec_float4)spu_splats(0.0f));
	#else
		Aos::Vector3 vecTemp = Aos::Vector3((vec_float4)vec_splat_s32(0));
	#endif
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp = matTemp * vecTemp;

			// Do this so that the Transpose is not optimized away.
			matTemp.setCol0( vecTemp );
			matTemp.setCol1( vecTemp );
			matTemp.setCol2( vecTemp );
			matTemp.setCol3( vecTemp );
		}
		fTime = tm.GetUsTime();

		// Dummy instructions.
		// Without the first one, a "stvx" happens inside the loop.
		// Without the second one, the loop will never even happen, since the optimizer gets rid of ret-by-val values if unused.
		Aos::Vector3 temp = vecTemp + vecTemp;
	#if !__SPU
		(*pV) = temp;
		delete pV;
	#else
		foo( temp.get128() );
	#endif
#elif NEW_RAGE_VEC_LIB && (!OLD_RAGE_VEC_LIB) 
	#if !__SPU
		Vec3V* pV = rage_new Vec3V;
	#endif

		Mat34V matTemp(V_ZERO);
		Vec3V vecTemp(V_ZERO);
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp = Transform3x3( matTemp, vecTemp );

			// Do this so that the Transpose is not optimized away.
			matTemp.SetCols( vecTemp );
		}
		fTime = tm.GetUsTime();

		// Dummy instructions.
		// Without the first one, a "stvx" happens inside the loop.
		// Without the second one, the loop will never even happen, since the optimizer gets rid of ret-by-val values if unused.
		Vec3V temp = Add( vecTemp, vecTemp );
	#if !__SPU
		(*pV) = temp;
		delete pV;
	#else
		foo( temp.GetIntrin128() );
	#endif
#elif OLD_RAGE_VEC_LIB && (!NEW_RAGE_VEC_LIB)
		// N/A
#else
		error;
#endif

		return fTime;
	}

	//================================================
	//
	//================================================
	float Benchmark_TransP3M34V3()
	{
		float fTime = -1.0f;
		sysTimer tm;

#if __XENON && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace xb;
		XMVECTOR* pV = rage_new XMVECTOR;

		XMMATRIX matTemp = XMMatrixIdentity();
		XMVECTOR vecTemp;
		XMDUMMY_INITIALIZE_VECTOR(vecTemp);
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp = XMVector3Transform( vecTemp, matTemp );
			matTemp.r[0] = matTemp.r[1] = matTemp.r[2] = matTemp.r[3] = vecTemp;
		}
		fTime = tm.GetUsTime();

		// Dummy instructions.
		// Without the first one, a "stvx" happens inside the loop.
		// Without the second one, the loop will never even happen, since the optimizer gets rid of ret-by-val values if unused.
		XMVECTOR temp = XMVectorAdd( vecTemp, vecTemp );
		(*pV) = temp;
		delete pV;
#elif __PS3 && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace Vectormath;
		using namespace Aos;
	#if !__SPU
		Point3* pV = rage_new Point3;
	#endif

		Transform3 matTemp = Transform3::identity();
	#if __SPU
		Point3 vecTemp = Point3((vec_float4)spu_splats(0.0f));
	#else
		Point3 vecTemp = Point3((vec_float4)vec_splat_s32(0));
	#endif
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp = matTemp * vecTemp;

			// Do this so that the Transpose is not optimized away.
			matTemp.setCol0( Aos::Vector3(vecTemp) );
			matTemp.setCol1( Aos::Vector3(vecTemp) );
			matTemp.setCol2( Aos::Vector3(vecTemp) );
			matTemp.setCol3( Aos::Vector3(vecTemp) );
		}
		fTime = tm.GetUsTime();

		// Dummy instructions.
		// Without the first one, a "stvx" happens inside the loop.
		// Without the second one, the loop will never even happen, since the optimizer gets rid of ret-by-val values if unused.
		Point3 temp = vecTemp + Aos::Vector3(vecTemp);
	#if !__SPU
		(*pV) = temp;
		delete pV;
	#else
		foo( temp.get128() );
	#endif
#elif NEW_RAGE_VEC_LIB && (!OLD_RAGE_VEC_LIB)
	#if !__SPU
		Vec3V* pV = rage_new Vec3V;
	#endif
		
		Mat34V matTemp(V_IDENTITY);
		Vec3V vecTemp(V_ZERO);
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp = Transform( matTemp, vecTemp );

			// Do this so that the Transpose is not optimized away.
			matTemp.SetCols( vecTemp );
		}
		fTime = tm.GetUsTime();

		// Dummy instructions.
		// Without the first one, a "stvx" happens inside the loop.
		// Without the second one, the loop will never even happen, since the optimizer gets rid of ret-by-val values if unused.
		Vec3V temp = Add( vecTemp, vecTemp );
	#if !__SPU
		(*pV) = temp;
		delete pV;
	#else
		foo( temp.GetIntrin128() );
	#endif
#elif OLD_RAGE_VEC_LIB && (!NEW_RAGE_VEC_LIB)
	#if !__SPU
		Vector3* pV = rage_new Vector3;
	#endif

		Matrix34 matTemp;
		matTemp.Identity();
		Vector3 vecTemp;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			matTemp.Transform( vecTemp, vecTemp );

			// Do this so that the Transpose is not optimized away.
			matTemp.a = matTemp.b = matTemp.c = matTemp.d = vecTemp;
		}
		fTime = tm.GetUsTime();

		// Dummy instructions.
		// Without the first one, a "stvx" happens inside the loop.
		// Without the second one, the loop will never even happen, since the optimizer gets rid of ret-by-val values if unused.
		Vector3 temp;
		temp.Add( vecTemp, vecTemp );
	#if !__SPU
		(*pV) = temp;
		delete pV;
	#else
		foo( temp.xyzw );
	#endif
#else
		error;
#endif

		return fTime;
	}

	//================================================
	//
	//================================================
	float Benchmark_InvM44()
	{
		float fTime = -1.0f;
		sysTimer tm;

#if __XENON && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace xb;

		XMMATRIX matTemp1 = XMMatrixIdentity();
		XMVECTOR vecTemp1;
		XMMATRIX matTemp2;
		XMVECTOR vecTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp1 = XMMatrixDeterminant( matTemp1 );
			matTemp2 = XMMatrixInverse( &vecTemp1, matTemp1 );
			vecTemp2 = XMMatrixDeterminant( matTemp2 );
			matTemp1 = XMMatrixInverse( &vecTemp2, matTemp2 );
		}
		fTime = tm.GetUsTime();
#elif __PS3 && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace Vectormath;
		using namespace Aos;
	#if !__SPU
		Matrix4* pV = rage_new Matrix4;
	#endif

		Matrix4 matTemp1 = Matrix4::identity();
		Matrix4 matTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			matTemp2 = inverse( matTemp1 );
			matTemp1 = inverse( matTemp2 );
		}
		fTime = tm.GetUsTime();
	#if !__SPU
		(*pV) = matTemp1;
		delete pV;
	#else
		foo( matTemp1.getCol0().get128(),matTemp1.getCol1().get128(),matTemp1.getCol2().get128(),matTemp1.getCol3().get128() );
	#endif
#elif NEW_RAGE_VEC_LIB && (!OLD_RAGE_VEC_LIB) 
	#if !__SPU
		Mat44V* pV = rage_new Mat44V;
	#endif

		Mat44V matTemp1(V_IDENTITY);
		Mat44V matTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			Invert( matTemp2, matTemp1 );
			Invert( matTemp1, matTemp2 );
		}
		fTime = tm.GetUsTime();
	#if !__SPU
		(*pV) = matTemp1;
		delete pV;
	#else
		foo( matTemp1.GetCol0Intrin128(),matTemp1.GetCol1Intrin128(),matTemp1.GetCol2Intrin128(),matTemp1.GetCol3Intrin128() );
	#endif
#elif OLD_RAGE_VEC_LIB && (!NEW_RAGE_VEC_LIB)
	#if !__SPU
		Matrix44* pV = rage_new Matrix44;

		Matrix44 matTemp1;
		matTemp1.Identity();
		Matrix44 matTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			matTemp2.Inverse( matTemp1 );
			matTemp1.Inverse( matTemp2 );
		}
		fTime = tm.GetUsTime();
		(*pV) = matTemp1;
		delete pV;
	#endif
#else
		error;
#endif

		return fTime;
	}

	//================================================
	//
	//================================================
	float Benchmark_InvM33()
	{
		float fTime = -1.0f;
		sysTimer tm;

#if __XENON && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace xb;
		// N/A
#elif __PS3 && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace Vectormath;
		using namespace Aos;
	#if !__SPU
		Matrix3* pV = rage_new Matrix3;
	#endif

		Matrix3 matTemp1 = Matrix3::identity();
		Matrix3 matTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			matTemp2 = inverse( matTemp1 );
			matTemp1 = inverse( matTemp2 );
		}
		fTime = tm.GetUsTime();
	#if !__SPU
		(*pV) = matTemp1;
		delete pV;
	#else
		foo( matTemp1.getCol0().get128(),matTemp1.getCol1().get128(),matTemp1.getCol2().get128() );
	#endif
#elif NEW_RAGE_VEC_LIB && (!OLD_RAGE_VEC_LIB) 
	#if !__SPU
		Mat33V* pV = rage_new Mat33V;
	#endif
		
		Mat33V matTemp1(V_IDENTITY);
		Mat33V matTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			Invert( matTemp2, matTemp1 );
			Invert( matTemp1, matTemp2 );
		}
		fTime = tm.GetUsTime();
	#if !__SPU
		(*pV) = matTemp1;
		delete pV;
	#else
		foo( matTemp1.GetCol0Intrin128(),matTemp1.GetCol1Intrin128(),matTemp1.GetCol2Intrin128() );
	#endif
#elif OLD_RAGE_VEC_LIB && (!NEW_RAGE_VEC_LIB)
	#if !__SPU
		Matrix33* pV = rage_new Matrix33;
	#endif

		Matrix33 matTemp1;
		matTemp1.Identity();
		Matrix33 matTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			matTemp2.Inverse( matTemp1 );
			matTemp1.Inverse( matTemp2 );
		}
		fTime = tm.GetUsTime();
	#if !__SPU
		(*pV) = matTemp1;
		delete pV;
	#else
		foo( matTemp1.a.xyzw,matTemp1.b.xyzw,matTemp1.c.xyzw );
	#endif
#else
		error;
#endif

		return fTime;
	}

	//================================================
	//
	//================================================
	float Benchmark_InvTransM34()
	{
		float fTime = -1.0f;
		sysTimer tm;

#if __XENON && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace xb;
		// N/A
#elif __PS3 && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace Vectormath;
		using namespace Aos;
	#if !__SPU
		Transform3* pV = rage_new Transform3;
	#endif

		Transform3 matTemp1 = Transform3::identity();
		Transform3 matTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			matTemp2 = inverse( matTemp1 );
			matTemp1 = inverse( matTemp2 );
		}
		fTime = tm.GetUsTime();

	#if !__SPU
		(*pV) = matTemp1;
		delete pV;
	#else
		foo( matTemp1.getCol0().get128(),matTemp1.getCol1().get128(),matTemp1.getCol2().get128(),matTemp1.getCol3().get128() );
	#endif
#elif NEW_RAGE_VEC_LIB && (!OLD_RAGE_VEC_LIB) 
	#if !__SPU
		Mat34V* pV = rage_new Mat34V;
	#endif

		Mat34V matTemp1(V_IDENTITY);
		Mat34V matTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			InvertTransform( matTemp2, matTemp1 );
			InvertTransform( matTemp1, matTemp2 );
		}
		fTime = tm.GetUsTime();
	#if !__SPU
		(*pV) = matTemp1;
		delete pV;
	#else
		foo( matTemp1.GetCol0Intrin128(),matTemp1.GetCol1Intrin128(),matTemp1.GetCol2Intrin128(),matTemp1.GetCol3Intrin128() );
	#endif
#elif OLD_RAGE_VEC_LIB && (!NEW_RAGE_VEC_LIB)
	#if !__SPU
		Matrix34* pV = rage_new Matrix34;
	#endif

		Matrix34 matTemp1;
		matTemp1.Identity();
		Matrix34 matTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			matTemp2.Inverse( matTemp1 );
			matTemp1.Inverse( matTemp2 );
		}
		fTime = tm.GetUsTime();
	#if !__SPU
		(*pV) = matTemp1;
		delete pV;
	#else
		foo( matTemp1.a.xyzw,matTemp1.b.xyzw,matTemp1.c.xyzw,matTemp1.d.xyzw );
	#endif
#else
		error;
#endif

		return fTime;
	}

	//================================================
	//
	//================================================
	float Benchmark_InvTransOrthoM34()
	{
		float fTime = -1.0f;
		sysTimer tm;

#if __XENON && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace xb;
		// N/A
#elif __PS3 && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace Vectormath;
		using namespace Aos;
	#if !__SPU
		Transform3* pV = rage_new Transform3;
	#endif

		Transform3 matTemp1 = Transform3::identity();
		Transform3 matTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			matTemp2 = orthoInverse( matTemp1 );
			matTemp1 = orthoInverse( matTemp2 );
		}
		fTime = tm.GetUsTime();
	#if !__SPU
		(*pV) = matTemp1;
		delete pV;
	#else
		foo( matTemp1.getCol0().get128(),matTemp1.getCol1().get128(),matTemp1.getCol2().get128(),matTemp1.getCol3().get128() );
	#endif
#elif NEW_RAGE_VEC_LIB && (!OLD_RAGE_VEC_LIB)
	#if !__SPU
		Mat34V* pV = rage_new Mat34V;
	#endif

		Mat34V matTemp1(V_IDENTITY);
		Mat34V matTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			InvertTransformOrtho( matTemp2, matTemp1 );
			InvertTransformOrtho( matTemp1, matTemp2 );
		}
		fTime = tm.GetUsTime();
	#if !__SPU
		(*pV) = matTemp1;
		delete pV;
	#else
		foo( matTemp1.GetCol0Intrin128(),matTemp1.GetCol1Intrin128(),matTemp1.GetCol2Intrin128(),matTemp1.GetCol3Intrin128() );
	#endif
#elif OLD_RAGE_VEC_LIB && (!NEW_RAGE_VEC_LIB)
	#if !__SPU
		Matrix34* pV = rage_new Matrix34;
	#endif

		Matrix34 matTemp1;
		matTemp1.Identity();
		Matrix34 matTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			matTemp2.FastInverse( matTemp1 );
			matTemp1.FastInverse( matTemp2 );
		}
		fTime = tm.GetUsTime();
	#if !__SPU
		(*pV) = matTemp1;
		delete pV;
	#else
		foo( matTemp1.a.xyzw,matTemp1.b.xyzw,matTemp1.c.xyzw,matTemp1.d.xyzw );
	#endif
#else
		error;
#endif

		return fTime;
	}

	//================================================
	//
	//================================================
	float Benchmark_UnTransOrthoM44M44()
	{
		float fTime = -1.0f;
		sysTimer tm;

#if __XENON && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace xb;
		// N/A
#elif __PS3 && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace Vectormath;
		using namespace Aos;
	#if !__SPU
		Matrix4* pV = rage_new Matrix4;
	#endif

		Matrix4 matTemp1 = Matrix4::identity();
		Matrix4 matTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			matTemp1 = orthoInverse( matTemp1 );
			matTemp2 = matTemp1 * matTemp1;

			matTemp2 = orthoInverse( matTemp2 );
			matTemp1 = matTemp2 * matTemp2;
		}
		fTime = tm.GetUsTime();
	#if !__SPU
		(*pV) = matTemp1;
		delete pV;
	#else
		foo( matTemp1.getCol0().get128(),matTemp1.getCol1().get128(),matTemp1.getCol2().get128(),matTemp1.getCol3().get128() );
	#endif
#elif NEW_RAGE_VEC_LIB && (!OLD_RAGE_VEC_LIB)
	#if !__SPU
		Mat44V* pV = rage_new Mat44V;
	#endif
		
		Mat44V matTemp1(V_IDENTITY);
		Mat44V matTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			UnTransformOrtho( matTemp2, matTemp1, matTemp1 );
			UnTransformOrtho( matTemp1, matTemp2, matTemp2 );
		}
		fTime = tm.GetUsTime();
	#if !__SPU
		(*pV) = matTemp1;
		delete pV;
	#else
		foo( matTemp1.GetCol0Intrin128(),matTemp1.GetCol1Intrin128(),matTemp1.GetCol2Intrin128(),matTemp1.GetCol3Intrin128() );
	#endif
#elif OLD_RAGE_VEC_LIB && (!NEW_RAGE_VEC_LIB)
	#if !__SPU
		Matrix44* pV = rage_new Matrix44;

		Matrix44 matTemp1;
		matTemp1.Identity();
		Matrix44 matTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			matTemp1.FastInverse( matTemp1 );
			matTemp2.Dot( matTemp1, matTemp1 );

			matTemp2.FastInverse( matTemp2 );
			matTemp1.Dot( matTemp2, matTemp2 );
		}
		fTime = tm.GetUsTime();
		(*pV) = matTemp1;
		delete pV;
	#endif
#else
		error;
#endif

		return fTime;
	}

	//================================================
	//
	//================================================
	float Benchmark_UnTransOrthoM44V4()
	{
		float fTime = -1.0f;
		sysTimer tm;

#if __XENON && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace xb;
		// N/A
#elif __PS3 && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace Vectormath;
		using namespace Aos;
	#if !__SPU
		Vector4* pV = rage_new Vector4;
	#endif
		
		Matrix4 matTemp = Matrix4::identity();
	#if __SPU
		Vector4 vecTemp = Vector4((vec_float4)spu_splats(0.0f));
	#else
		Vector4 vecTemp = Vector4((vec_float4)vec_splat_s32(0));
	#endif
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			matTemp = orthoInverse( matTemp );
			vecTemp = matTemp * vecTemp;

			// Recycle result so that nothing is optimized away.
			matTemp.setCol0( vecTemp );
			matTemp.setCol1( vecTemp );
			matTemp.setCol2( vecTemp );
			matTemp.setCol3( vecTemp );
		}
		fTime = tm.GetUsTime();

		// Dummy instructions.
		// Without the first one, a "stvx" happens inside the loop.
		// Without the second one, the loop will never even happen, since the optimizer gets rid of ret-by-val values if unused.
		Vector4 temp = vecTemp + vecTemp;
	#if !__SPU
		(*pV) = temp;
		delete pV;
	#else
		foo( temp.get128() );
	#endif
#elif NEW_RAGE_VEC_LIB && (!OLD_RAGE_VEC_LIB) 
	#if !__SPU
		Vec4V* pV = rage_new Vec4V;
	#endif
		
		Mat44V matTemp(V_IDENTITY);
		Vec4V vecTemp(V_ZERO);
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp = UnTransformOrtho( matTemp, vecTemp );

			// Recycle result so that nothing is not optimized away.
			matTemp.SetCols( vecTemp );
		}
		fTime = tm.GetUsTime();

		// Dummy instructions.
		// Without the first one, a "stvx" happens inside the loop.
		// Without the second one, the loop will never even happen, since the optimizer gets rid of ret-by-val values if unused.
		Vec4V temp = Add( vecTemp, vecTemp );
	#if !__SPU
		(*pV) = temp;
		delete pV;
	#else
		foo( temp.GetIntrin128() );
	#endif
#elif OLD_RAGE_VEC_LIB && (!NEW_RAGE_VEC_LIB)
	#if !__SPU
		Vector4* pV = rage_new Vector4;

		Matrix44 matTemp;
		matTemp.Identity();
		Vector4 vecTemp;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			matTemp.FastInverse( matTemp );
			matTemp.Transform( vecTemp, vecTemp );

			// Recycle result so that nothing is not optimized away.
			matTemp.a = matTemp.b = matTemp.c = matTemp.d = vecTemp;
		}
		fTime = tm.GetUsTime();

		// Dummy instructions.
		// Without the first one, a "stvx" happens inside the loop.
		// Without the second one, the loop will never even happen, since the optimizer gets rid of ret-by-val values if unused.
		Vector4 temp;
		temp.Add( vecTemp, vecTemp );
		(*pV) = temp;
		delete pV;
	#endif
#else
		error;
#endif

		return fTime;
	}

	//================================================
	//
	//================================================
	float Benchmark_UnTransOrthoM33M33()
	{
		float fTime = -1.0f;
		sysTimer tm;

#if __XENON && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace xb;
		// N/A
#elif __PS3 && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace Vectormath;
		using namespace Aos;
	#if !__SPU
		Matrix3* pV = rage_new Matrix3;
	#endif

		Matrix3 matTemp1 = Matrix3::identity();
		Matrix3 matTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			matTemp1 = transpose( matTemp1 );
			matTemp2 = matTemp1 * matTemp1;

			matTemp2 = transpose( matTemp2 );
			matTemp1 = matTemp2 * matTemp2;
		}
		fTime = tm.GetUsTime();
	#if !__SPU
		(*pV) = matTemp1;
		delete pV;
	#else
		foo( matTemp1.getCol0().get128(),matTemp1.getCol1().get128(),matTemp1.getCol2().get128() );
	#endif
#elif NEW_RAGE_VEC_LIB && (!OLD_RAGE_VEC_LIB)
	#if !__SPU
		Mat33V* pV = rage_new Mat33V;
	#endif

		Mat33V matTemp1(V_IDENTITY);
		Mat33V matTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			UnTransformOrtho( matTemp2, matTemp1, matTemp1 );
			UnTransformOrtho( matTemp1, matTemp2, matTemp2 );
		}
		fTime = tm.GetUsTime();
	#if !__SPU
		(*pV) = matTemp1;
		delete pV;
	#else
		foo( matTemp1.GetCol0Intrin128(),matTemp1.GetCol1Intrin128(),matTemp1.GetCol2Intrin128() );
	#endif
#elif OLD_RAGE_VEC_LIB && (!NEW_RAGE_VEC_LIB)
	#if !__SPU
		Matrix33* pV = rage_new Matrix33;
	#endif

		Matrix33 matTemp1;
		matTemp1.Identity();
		Matrix33 matTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			matTemp1.Transpose( matTemp1 );
			matTemp2.Dot( matTemp1, matTemp1 );

			matTemp2.Transpose( matTemp2 );
			matTemp1.Dot( matTemp2, matTemp2 );
		}
		fTime = tm.GetUsTime();
	#if !__SPU
		(*pV) = matTemp1;
		delete pV;
	#else
		foo( matTemp1.a.xyzw,matTemp1.b.xyzw,matTemp1.c.xyzw );
	#endif
#else
		error;
#endif

		return fTime;
	}

	//================================================
	//
	//================================================
	float Benchmark_UnTransOrthoM33V3()
	{
		float fTime = -1.0f;
		sysTimer tm;

#if __XENON && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace xb;
		// N/A
#elif __PS3 && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace Vectormath;
		using namespace Aos;
	#if !__SPU
		Aos::Vector3* pV = rage_new Aos::Vector3;
	#endif

		Matrix3 matTemp = Matrix3::identity();
	#if __SPU
		Aos::Vector3 vecTemp = Aos::Vector3((vec_float4)spu_splats(0.0f));
	#else
		Aos::Vector3 vecTemp = Aos::Vector3((vec_float4)vec_splat_s32(0));
	#endif
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			matTemp = transpose( matTemp );
			vecTemp = matTemp * vecTemp;

			// Recycle result so that nothing is optimized away.
			matTemp.setCol0( vecTemp );
			matTemp.setCol1( vecTemp );
			matTemp.setCol2( vecTemp );
		}
		fTime = tm.GetUsTime();

		// Dummy instructions.
		// Without the first one, a "stvx" happens inside the loop.
		// Without the second one, the loop will never even happen, since the optimizer gets rid of ret-by-val values if unused.
		Aos::Vector3 temp = vecTemp + vecTemp;
	#if !__SPU
		(*pV) = temp;
		delete pV;
	#else
		foo( temp.get128() );
	#endif
#elif NEW_RAGE_VEC_LIB && (!OLD_RAGE_VEC_LIB) 
	#if !__SPU
		Vec3V* pV = rage_new Vec3V;
	#endif
		
		Mat33V matTemp(V_IDENTITY);
		Vec3V vecTemp(V_ZERO);
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp = UnTransformOrtho( matTemp, vecTemp );

			// Recycle result so that nothing is optimized away.
			matTemp.SetCols( vecTemp );
		}
		fTime = tm.GetUsTime();

		// Dummy instructions.
		// Without the first one, a "stvx" happens inside the loop.
		// Without the second one, the loop will never even happen, since the optimizer gets rid of ret-by-val values if unused.
		Vec3V temp = Add( vecTemp, vecTemp );
	#if !__SPU
		(*pV) = temp;
		delete pV;
	#else
		foo( temp.GetIntrin128() );
	#endif
#elif OLD_RAGE_VEC_LIB && (!NEW_RAGE_VEC_LIB)
	#if !__SPU
		Vector3* pV = rage_new Vector3;
	#endif

		Matrix33 matTemp;
		matTemp.Identity();
		Vector3 vecTemp;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			matTemp.UnTransform( vecTemp, vecTemp );

			// Recycle result so that nothing is optimized away.
			matTemp.a = matTemp.b = matTemp.c = vecTemp;
		}
		fTime = tm.GetUsTime();

		// Dummy instructions.
		// Without the first one, a "stvx" happens inside the loop.
		// Without the second one, the loop will never even happen, since the optimizer gets rid of ret-by-val values if unused.
		Vector3 temp;
		temp.Add( vecTemp, vecTemp );
	#if !__SPU
		(*pV) = temp;
		delete pV;
	#else
		foo( temp.xyzw );
	#endif
#else
		error;
#endif

		return fTime;
	}

	//================================================
	//
	//================================================
	float Benchmark_UnTransOrthoM34M34()
	{
		float fTime = -1.0f;
		sysTimer tm;

#if __XENON && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace xb;
		// N/A
#elif __PS3 && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace Vectormath;
		using namespace Aos;
	#if !__SPU
		Transform3* pV = rage_new Transform3;
	#endif

		Transform3 matTemp1 = Transform3::identity();
		Transform3 matTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			matTemp1 = orthoInverse( matTemp1 );
			matTemp2 = matTemp1 * matTemp1;

			matTemp2 = orthoInverse( matTemp2 );
			matTemp1 = matTemp2 * matTemp2;
		}
		fTime = tm.GetUsTime();
	#if !__SPU
		(*pV) = matTemp1;
		delete pV;
	#else
		foo( matTemp1.getCol0().get128(),matTemp1.getCol1().get128(),matTemp1.getCol2().get128(),matTemp1.getCol3().get128() );
	#endif
#elif NEW_RAGE_VEC_LIB && (!OLD_RAGE_VEC_LIB) 
	#if !__SPU
		Mat34V* pV = rage_new Mat34V;
	#endif
		
		Mat34V matTemp1(V_IDENTITY);
		Mat34V matTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			UnTransformOrtho( matTemp2, matTemp1, matTemp1 );
			UnTransformOrtho( matTemp1, matTemp2, matTemp2 );
		}
		fTime = tm.GetUsTime();
	#if !__SPU
		(*pV) = matTemp1;
		delete pV;
	#else
		foo( matTemp1.GetCol0Intrin128(),matTemp1.GetCol1Intrin128(),matTemp1.GetCol2Intrin128(),matTemp1.GetCol3Intrin128() );
	#endif
#elif OLD_RAGE_VEC_LIB && (!NEW_RAGE_VEC_LIB)
	#if !__SPU
		Matrix34* pV = rage_new Matrix34;
	#endif

		Matrix34 matTemp1;
		matTemp1.Identity();
		Matrix34 matTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			matTemp1.FastInverse( matTemp1 );
			matTemp2.Dot( matTemp1, matTemp1 );

			matTemp2.FastInverse( matTemp2 );
			matTemp1.Dot( matTemp2, matTemp2 );
		}
		fTime = tm.GetUsTime();
	#if !__SPU
		(*pV) = matTemp1;
		delete pV;
	#else
		foo( matTemp1.a.xyzw,matTemp1.b.xyzw,matTemp1.c.xyzw,matTemp1.d.xyzw );;
	#endif
#else
		error;
#endif

		return fTime;
	}

	//================================================
	//
	//================================================
	float Benchmark_UnTransV3OrthoM34V3()
	{
		float fTime = -1.0f;
		sysTimer tm;

#if __XENON && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace xb;
		// N/A
#elif __PS3 && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace Vectormath;
		using namespace Aos;
	#if !__SPU
		Aos::Vector3* pV = rage_new Aos::Vector3;
	#endif

		Transform3 matTemp = Transform3::identity();
	#if __SPU
		Aos::Vector3 vecTemp = Aos::Vector3((vec_float4)spu_splats(0.0f));
	#else
		Aos::Vector3 vecTemp = Aos::Vector3((vec_float4)vec_splat_s32(0));
	#endif
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			matTemp = orthoInverse( matTemp );
			vecTemp = matTemp * vecTemp;

			// Recycle result so that nothing is optimized away.
			matTemp.setCol0( vecTemp );
			matTemp.setCol1( vecTemp );
			matTemp.setCol2( vecTemp );
			matTemp.setCol3( vecTemp );
		}
		fTime = tm.GetUsTime();

		// Dummy instructions.
		// Without the first one, a "stvx" happens inside the loop.
		// Without the second one, the loop will never even happen, since the optimizer gets rid of ret-by-val values if unused.
		Aos::Vector3 temp = vecTemp + vecTemp;
	#if !__SPU
		(*pV) = temp;
		delete pV;
	#else
		foo( temp.get128() );
	#endif
#elif NEW_RAGE_VEC_LIB && (!OLD_RAGE_VEC_LIB)
	#if !__SPU
		Vec3V* pV = rage_new Vec3V;
	#endif
		
		Mat34V matTemp(V_IDENTITY);
		Vec3V vecTemp(V_ZERO);
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp = UnTransform3x3Ortho( matTemp, vecTemp );

			// Recycle result so that nothing is optimized away.
			matTemp.SetCols( vecTemp );
		}
		fTime = tm.GetUsTime();

		// Dummy instructions.
		// Without the first one, a "stvx" happens inside the loop.
		// Without the second one, the loop will never even happen, since the optimizer gets rid of ret-by-val values if unused.
		Vec3V temp = Add( vecTemp, vecTemp );
	#if !__SPU
		(*pV) = temp;
		delete pV;
	#else
		foo( temp.GetIntrin128() );
	#endif
#elif OLD_RAGE_VEC_LIB && (!NEW_RAGE_VEC_LIB)
		// N/A
#else
		error;
#endif

		return fTime;
	}

	//================================================
	//
	//================================================
	float Benchmark_UnTransP3OrthoM34V3()
	{
		float fTime = -1.0f;
		sysTimer tm;

#if __XENON && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace xb;
		// N/A
#elif __PS3 && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace Vectormath;
		using namespace Aos;
	#if !__SPU
		Point3* pV = rage_new Point3;
	#endif
		
		Transform3 matTemp = Transform3::identity();
	#if __SPU
		Point3 vecTemp = Point3((vec_float4)spu_splats(0.0f));
	#else
		Point3 vecTemp = Point3((vec_float4)vec_splat_s32(0));
	#endif
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			matTemp = orthoInverse( matTemp );
			vecTemp = matTemp * vecTemp;

			// Recycle result so that nothing is optimized away.
			matTemp.setCol0( Aos::Vector3(vecTemp) );
			matTemp.setCol1( Aos::Vector3(vecTemp) );
			matTemp.setCol2( Aos::Vector3(vecTemp) );
			matTemp.setCol3( Aos::Vector3(vecTemp) );
		}
		fTime = tm.GetUsTime();

		// Dummy instructions.
		// Without the first one, a "stvx" happens inside the loop.
		// Without the second one, the loop will never even happen, since the optimizer gets rid of ret-by-val values if unused.
		Point3 temp = vecTemp + Aos::Vector3(vecTemp);
	#if !__SPU
		(*pV) = temp;
		delete pV;
	#else
		foo( temp.get128() );
	#endif
#elif NEW_RAGE_VEC_LIB && (!OLD_RAGE_VEC_LIB) 
	#if !__SPU
		Vec3V* pV = rage_new Vec3V;
	#endif
		
		Mat34V matTemp(V_IDENTITY);
		Vec3V vecTemp(V_ZERO);
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp = UnTransformOrtho( matTemp, vecTemp );
			matTemp.SetCols( vecTemp );
		}
		fTime = tm.GetUsTime();

		// Dummy instructions.
		// Without the first one, a "stvx" happens inside the loop.
		// Without the second one, the loop will never even happen, since the optimizer gets rid of ret-by-val values if unused.
		Vec3V temp = Add( vecTemp, vecTemp );
	#if !__SPU
		(*pV) = temp;
		delete pV;
	#else
		foo( temp.GetIntrin128() );
	#endif
#elif OLD_RAGE_VEC_LIB && (!NEW_RAGE_VEC_LIB)
	#if !__SPU
		Vector3* pV = rage_new Vector3;
	#endif

		Matrix34 matTemp;
		matTemp.Identity();
		Vector3 vecTemp;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			matTemp.UnTransform( vecTemp, vecTemp );

			// Recycle result so that nothing is optimized away.
			matTemp.a = matTemp.b = matTemp.c = matTemp.d = vecTemp;
		}
		fTime = tm.GetUsTime();

		// Dummy instructions.
		// Without the first one, a "stvx" happens inside the loop.
		// Without the second one, the loop will never even happen, since the optimizer gets rid of ret-by-val values if unused.
		Vector3 temp;
		temp.Add( vecTemp, vecTemp );
	#if !__SPU
		(*pV) = temp;
		delete pV;
	#else
		foo( temp.xyzw );
	#endif
#else
		error;
#endif

		return fTime;
	}


























	////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////
	// BELOW THIS LINE = NO SPU (TESTING) SUPPORT!
	////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////

	//================================================
	//
	//================================================
	float Benchmark_SinAndCos()
	{
		float fTime = -1.0f;
		sysTimer tm;

#if __XENON && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace xb;
		XMVECTOR* pV = rage_new XMVECTOR;

		XMVECTOR vecTemp;
		XMDUMMY_INITIALIZE_VECTOR(vecTemp);
		XMVECTOR vecTemp2, vecTemp3;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			XMVectorSinCos( &vecTemp2, &vecTemp3, vecTemp );
			vecTemp = XMVectorAdd( vecTemp2, vecTemp3 );
		}
		fTime = tm.GetUsTime();
		(*pV) = vecTemp;
		delete pV;
#elif __PPU && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace Vectormath;
		using namespace Aos;
		vec_float4* pV = rage_new vec_float4;

		vec_float4 vecTemp = (vec_float4)(vec_splat_s32(0));
		vec_float4 vecTemp2, vecTemp3;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			sincosf4( vecTemp, &vecTemp2, &vecTemp3 );
			vecTemp = vecTemp2 + vecTemp3;
		}
		fTime = tm.GetUsTime();
		(*pV) = vecTemp;
		delete pV;
#elif NEW_RAGE_VEC_LIB && (!OLD_RAGE_VEC_LIB)
		Vec4V* pV = rage_new Vec4V;

		Vec4V vecTemp = Vec4V(V_ZERO);
		Vec4V vecTemp2, vecTemp3;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			SinAndCos( vecTemp2, vecTemp3, vecTemp );
			vecTemp = Add( vecTemp2, vecTemp3 );
		}
		fTime = tm.GetUsTime();
		(*pV) = vecTemp;
		delete pV;
#elif OLD_RAGE_VEC_LIB && (!NEW_RAGE_VEC_LIB)
		// N/A
#else
		fTime = 0.0f;
#endif

		return fTime;
	}

	//================================================
	//
	//================================================
	float Benchmark_Sin()
	{
		float fTime = -1.0f;
		sysTimer tm;

#if __XENON && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace xb;
		XMVECTOR* pV = rage_new XMVECTOR;

		XMVECTOR vecTemp;
		XMDUMMY_INITIALIZE_VECTOR(vecTemp);
		XMVECTOR vecTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp2 = XMVectorSin( vecTemp );
			vecTemp = vecTemp2;
		}
		fTime = tm.GetUsTime();
		(*pV) = vecTemp;
		delete pV;
#elif __PPU && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace Vectormath;
		using namespace Aos;
		vec_float4* pV = rage_new vec_float4;

		vec_float4 vecTemp = (vec_float4)(vec_splat_s32(0));
		vec_float4 vecTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp2 = sinf4( vecTemp );
			vecTemp = vecTemp2;
		}
		fTime = tm.GetUsTime();
		(*pV) = vecTemp;
		delete pV;
#elif NEW_RAGE_VEC_LIB && (!OLD_RAGE_VEC_LIB) 
		Vec4V* pV = rage_new Vec4V;

		Vec4V vecTemp = Vec4V(V_ZERO);
		Vec4V vecTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp2 = Sin( vecTemp );
			vecTemp = vecTemp2;
		}
		fTime = tm.GetUsTime();
		(*pV) = vecTemp;
		delete pV;
#elif OLD_RAGE_VEC_LIB && (!NEW_RAGE_VEC_LIB)
		// N/A
		// (PUTTING SCALAR FLOAT TRIG BENCHMARKS HERE FOR NOW!)
		float* pV = rage_new float;

		float temp = 0.0f;
		float temp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			temp2 = FPSin( temp );
			temp = temp2;
		}
		fTime = tm.GetUsTime();
		(*pV) = temp;
		delete pV;
#else
		fTime = 0.0f;
#endif

		return fTime;
	}

	//================================================
	//
	//================================================
	float Benchmark_Cos()
	{
		float fTime = -1.0f;
		sysTimer tm;

#if __XENON && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace xb;
		XMVECTOR* pV = rage_new XMVECTOR;

		XMVECTOR vecTemp;
		XMDUMMY_INITIALIZE_VECTOR(vecTemp);
		XMVECTOR vecTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp2 = XMVectorCos( vecTemp );
			vecTemp = vecTemp2;
		}
		fTime = tm.GetUsTime();
		(*pV) = vecTemp;
		delete pV;
#elif __PPU && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace Vectormath;
		using namespace Aos;
		vec_float4* pV = rage_new vec_float4;

		vec_float4 vecTemp = (vec_float4)(vec_splat_s32(0));
		vec_float4 vecTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp2 = cosf4( vecTemp );
			vecTemp = vecTemp2;
		}
		fTime = tm.GetUsTime();
		(*pV) = vecTemp;
		delete pV;
#elif NEW_RAGE_VEC_LIB && (!OLD_RAGE_VEC_LIB) 
		Vec4V* pV = rage_new Vec4V;

		Vec4V vecTemp = Vec4V(V_ZERO);
		Vec4V vecTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp2 = Cos( vecTemp );
			vecTemp = vecTemp2;
		}
		fTime = tm.GetUsTime();
		(*pV) = vecTemp;
		delete pV;
#elif OLD_RAGE_VEC_LIB && (!NEW_RAGE_VEC_LIB)
		// N/A
		// (PUTTING SCALAR FLOAT TRIG BENCHMARKS HERE FOR NOW!)
		float* pV = rage_new float;

		float temp = 0.0f;
		float temp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			temp2 = FPCos( temp );
			temp = temp2;
		}
		fTime = tm.GetUsTime();
		(*pV) = temp;
		delete pV;
#else
		fTime = 0.0f;
#endif

		return fTime;
	}

	//================================================
	//
	//================================================
	float Benchmark_Tan()
	{
		float fTime = -1.0f;
		sysTimer tm;

#if __XENON && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace xb;
		XMVECTOR* pV = rage_new XMVECTOR;

		XMVECTOR vecTemp;
		XMDUMMY_INITIALIZE_VECTOR(vecTemp);
		XMVECTOR vecTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp2 = XMVectorTan( vecTemp );
			vecTemp = vecTemp2;
		}
		fTime = tm.GetUsTime();
		(*pV) = vecTemp;
		delete pV;
#elif __PPU && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace Vectormath;
		using namespace Aos;
		vec_float4* pV = rage_new vec_float4;

		vec_float4 vecTemp = (vec_float4)(vec_splat_s32(0));
		vec_float4 vecTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp2 = tanf4( vecTemp );
			vecTemp = vecTemp2;
		}
		fTime = tm.GetUsTime();
		(*pV) = vecTemp;
		delete pV;
#elif NEW_RAGE_VEC_LIB && (!OLD_RAGE_VEC_LIB) 
		Vec4V* pV = rage_new Vec4V;

		Vec4V vecTemp = Vec4V(V_ZERO);
		Vec4V vecTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp2 = Tan( vecTemp );
			vecTemp = vecTemp2;
		}
		fTime = tm.GetUsTime();
		(*pV) = vecTemp;
		delete pV;
#elif OLD_RAGE_VEC_LIB && (!NEW_RAGE_VEC_LIB)
		// N/A
		// (PUTTING SCALAR FLOAT TRIG BENCHMARKS HERE FOR NOW!)
		float* pV = rage_new float;

		float temp = 0.0f;
		float temp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			temp2 = FPTan( temp );
			temp = temp2;
		}
		fTime = tm.GetUsTime();
		(*pV) = temp;
		delete pV;
#else
		fTime = 0.0f;
#endif

		return fTime;
	}

	//================================================
	//
	//================================================
	float Benchmark_Asin()
	{
		float fTime = -1.0f;
		sysTimer tm;

#if __XENON && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace xb;
		XMVECTOR* pV = rage_new XMVECTOR;

		XMVECTOR vecTemp;
		XMDUMMY_INITIALIZE_VECTOR(vecTemp);
		XMVECTOR vecTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp2 = XMVectorASin( vecTemp );
			vecTemp = vecTemp2;
		}
		fTime = tm.GetUsTime();
		(*pV) = vecTemp;
		delete pV;
#elif __PPU && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace Vectormath;
		using namespace Aos;
		vec_float4* pV = rage_new vec_float4;

		vec_float4 vecTemp = (vec_float4)(vec_splat_s32(0));
		vec_float4 vecTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp2 = asinf4( vecTemp );
			vecTemp = vecTemp2;
		}
		fTime = tm.GetUsTime();
		(*pV) = vecTemp;
		delete pV;
#elif NEW_RAGE_VEC_LIB && (!OLD_RAGE_VEC_LIB) 
		Vec4V* pV = rage_new Vec4V;

		Vec4V vecTemp = Vec4V(V_ZERO);
		Vec4V vecTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp2 = Arcsin( vecTemp );
			vecTemp = vecTemp2;
		}
		fTime = tm.GetUsTime();
		(*pV) = vecTemp;
		delete pV;
#elif OLD_RAGE_VEC_LIB && (!NEW_RAGE_VEC_LIB)
		// N/A
		// (PUTTING SCALAR FLOAT TRIG BENCHMARKS HERE FOR NOW!)
		float* pV = rage_new float;

		float temp = 0.0f;
		float temp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			temp2 = FPASin( temp );
			temp = temp2;
		}
		fTime = tm.GetUsTime();
		(*pV) = temp;
		delete pV;
#else
		fTime = 0.0f;
#endif

		return fTime;
	}

	//================================================
	//
	//================================================
	float Benchmark_Acos()
	{
		float fTime = -1.0f;
		sysTimer tm;

#if __XENON && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace xb;
		XMVECTOR* pV = rage_new XMVECTOR;

		XMVECTOR vecTemp;
		XMDUMMY_INITIALIZE_VECTOR(vecTemp);
		XMVECTOR vecTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp2 = XMVectorACos( vecTemp );
			vecTemp = vecTemp2;
		}
		fTime = tm.GetUsTime();
		(*pV) = vecTemp;
		delete pV;
#elif __PPU && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace Vectormath;
		using namespace Aos;
		vec_float4* pV = rage_new vec_float4;

		vec_float4 vecTemp = (vec_float4)(vec_splat_s32(0));
		vec_float4 vecTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp2 = acosf4( vecTemp );
			vecTemp = vecTemp2;
		}
		fTime = tm.GetUsTime();
		(*pV) = vecTemp;
		delete pV;
#elif NEW_RAGE_VEC_LIB && (!OLD_RAGE_VEC_LIB) 
		Vec4V* pV = rage_new Vec4V;

		Vec4V vecTemp = Vec4V(V_ZERO);
		Vec4V vecTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp2 = Arccos( vecTemp );
			vecTemp = vecTemp2;
		}
		fTime = tm.GetUsTime();
		(*pV) = vecTemp;
		delete pV;
#elif OLD_RAGE_VEC_LIB && (!NEW_RAGE_VEC_LIB)
		// N/A
		// (PUTTING SCALAR FLOAT TRIG BENCHMARKS HERE FOR NOW!)
		float* pV = rage_new float;

		float temp = 0.0f;
		float temp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			temp2 = FPACos( temp );
			temp = temp2;
		}
		fTime = tm.GetUsTime();
		(*pV) = temp;
		delete pV;
#else
		fTime = 0.0f;
#endif

		return fTime;
	}

	//================================================
	//
	//================================================
	float Benchmark_Atan()
	{
		float fTime = -1.0f;
		sysTimer tm;

#if __XENON && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace xb;
		XMVECTOR* pV = rage_new XMVECTOR;

		XMVECTOR vecTemp;
		XMDUMMY_INITIALIZE_VECTOR(vecTemp);
		XMVECTOR vecTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp2 = XMVectorATan( vecTemp );
			vecTemp = vecTemp2;
		}
		fTime = tm.GetUsTime();
		(*pV) = vecTemp;
		delete pV;
#elif __PPU && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace Vectormath;
		using namespace Aos;
		vec_float4* pV = rage_new vec_float4;

		vec_float4 vecTemp = (vec_float4)(vec_splat_s32(0));
		vec_float4 vecTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp2 = atanf4( vecTemp );
			vecTemp = vecTemp2;
		}
		fTime = tm.GetUsTime();
		(*pV) = vecTemp;
		delete pV;
#elif NEW_RAGE_VEC_LIB && (!OLD_RAGE_VEC_LIB) 
		Vec4V* pV = rage_new Vec4V;

		Vec4V vecTemp = Vec4V(V_ZERO);
		Vec4V vecTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp2 = Arctan( vecTemp );
			vecTemp = vecTemp2;
		}
		fTime = tm.GetUsTime();
		(*pV) = vecTemp;
		delete pV;
#elif OLD_RAGE_VEC_LIB && (!NEW_RAGE_VEC_LIB)
		// N/A
		// (PUTTING SCALAR FLOAT TRIG BENCHMARKS HERE FOR NOW!)
		float* pV = rage_new float;

		float temp = 0.0f;
		float temp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			temp2 = FPATan( temp );
			temp = temp2;
		}
		fTime = tm.GetUsTime();
		(*pV) = temp;
		delete pV;
#else
		fTime = 0.0f;
#endif

		return fTime;
	}

	//================================================
	//
	//================================================
	float Benchmark_Atan2()
	{
		float fTime = -1.0f;
		sysTimer tm;

#if __XENON && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace xb;
		XMVECTOR* pV = rage_new XMVECTOR;

		XMVECTOR vecTemp;
		XMDUMMY_INITIALIZE_VECTOR(vecTemp);
		XMVECTOR vecTemp2;
		XMDUMMY_INITIALIZE_VECTOR(vecTemp2);
		XMVECTOR vecTemp3;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp3 = XMVectorATan2( vecTemp, vecTemp2 );
			vecTemp = vecTemp2 = vecTemp3;
		}
		fTime = tm.GetUsTime();
		(*pV) = vecTemp;
		delete pV;
#elif __PPU && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace Vectormath;
		using namespace Aos;
		vec_float4* pV = rage_new vec_float4;

		vec_float4 vecTemp = (vec_float4)(vec_splat_s32(0));
		vec_float4 vecTemp2 = (vec_float4)(vec_splat_s32(0));
		vec_float4 vecTemp3;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp3 = atan2f4( vecTemp, vecTemp2 );
			vecTemp = vecTemp2 = vecTemp3;
		}
		fTime = tm.GetUsTime();
		(*pV) = vecTemp;
		delete pV;
#elif NEW_RAGE_VEC_LIB && (!OLD_RAGE_VEC_LIB) 
		Vec4V* pV = rage_new Vec4V;

		Vec4V vecTemp = Vec4V(V_ZERO);
		Vec4V vecTemp2 = Vec4V(V_ZERO);
		Vec4V vecTemp3;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp3 = Arctan2( vecTemp, vecTemp2 );
			vecTemp = vecTemp2 = vecTemp3;
		}
		fTime = tm.GetUsTime();
		(*pV) = vecTemp;
		delete pV;
#elif OLD_RAGE_VEC_LIB && (!NEW_RAGE_VEC_LIB)
		// N/A
		// (PUTTING SCALAR FLOAT TRIG BENCHMARKS HERE FOR NOW!)
		float* pV = rage_new float;

		float temp = 0.0f;
		float temp2 = 0.0f;
		float temp3;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			temp3 = FPATan2( temp, temp2 );
			temp = temp2 = temp3;
		}
		fTime = tm.GetUsTime();
		(*pV) = temp;
		delete pV;
#else
		fTime = 0.0f;
#endif

		return fTime;
	}


	//================================================
	//
	//================================================
	float Benchmark_FASTSinAndCos()
	{
		float fTime = -1.0f;
		sysTimer tm;

#if __XENON && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace xb;
		XMVECTOR* pV = rage_new XMVECTOR;

		XMVECTOR vecTemp;
		XMDUMMY_INITIALIZE_VECTOR(vecTemp);
		XMVECTOR vecTemp2, vecTemp3;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			XMVectorSinCosEst( &vecTemp2, &vecTemp3, vecTemp );
			vecTemp = XMVectorAdd( vecTemp2, vecTemp3 );
		}
		fTime = tm.GetUsTime();
		(*pV) = vecTemp;
		delete pV;
#elif __PPU && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace Vectormath;
		using namespace Aos;
		vec_float4* pV = rage_new vec_float4;

		vec_float4 vecTemp = (vec_float4)(vec_splat_s32(0));
		vec_float4 vecTemp2, vecTemp3;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			sincosf4fast( vecTemp, &vecTemp2, &vecTemp3 );
			vecTemp = vecTemp2 + vecTemp3;
		}
		fTime = tm.GetUsTime();
		(*pV) = vecTemp;
		delete pV;
#elif NEW_RAGE_VEC_LIB && (!OLD_RAGE_VEC_LIB) 
		Vec4V* pV = rage_new Vec4V;

		Vec4V vecTemp = Vec4V(V_ZERO);
		Vec4V vecTemp2, vecTemp3;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			SinAndCosFast( vecTemp2, vecTemp3, vecTemp );
			vecTemp = Add( vecTemp2, vecTemp3 );
		}
		fTime = tm.GetUsTime();
		(*pV) = vecTemp;
		delete pV;
#elif OLD_RAGE_VEC_LIB && (!NEW_RAGE_VEC_LIB)
		// N/A
#else
		fTime = 0.0f;
#endif

		return fTime;
	}

	//================================================
	//
	//================================================
	float Benchmark_FASTSin()
	{
		float fTime = -1.0f;
		sysTimer tm;

#if __XENON && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace xb;
		XMVECTOR* pV = rage_new XMVECTOR;

		XMVECTOR vecTemp;
		XMDUMMY_INITIALIZE_VECTOR(vecTemp);
		XMVECTOR vecTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp2 = XMVectorSinEst( vecTemp );
			vecTemp = vecTemp2;
		}
		fTime = tm.GetUsTime();
		(*pV) = vecTemp;
		delete pV;
#elif __PPU && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace Vectormath;
		using namespace Aos;
		vec_float4* pV = rage_new vec_float4;

		vec_float4 vecTemp = (vec_float4)(vec_splat_s32(0));
		vec_float4 vecTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp2 = sinf4fast( vecTemp );
			vecTemp = vecTemp2;
		}
		fTime = tm.GetUsTime();
		(*pV) = vecTemp;
		delete pV;
#elif NEW_RAGE_VEC_LIB && (!OLD_RAGE_VEC_LIB) 
		Vec4V* pV = rage_new Vec4V;

		Vec4V vecTemp = Vec4V(V_ZERO);
		Vec4V vecTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp2 = SinFast( vecTemp );
			vecTemp = vecTemp2;
		}
		fTime = tm.GetUsTime();
		(*pV) = vecTemp;
		delete pV;
#elif OLD_RAGE_VEC_LIB && (!NEW_RAGE_VEC_LIB)
		// N/A
#else
		fTime = 0.0f;
#endif

		return fTime;
	}

	//================================================
	//
	//================================================
	float Benchmark_FASTCos()
	{
		float fTime = -1.0f;
		sysTimer tm;

#if __XENON && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace xb;
		XMVECTOR* pV = rage_new XMVECTOR;

		XMVECTOR vecTemp;
		XMDUMMY_INITIALIZE_VECTOR(vecTemp);
		XMVECTOR vecTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp2 = XMVectorCosEst( vecTemp );
			vecTemp = vecTemp2;
		}
		fTime = tm.GetUsTime();
		(*pV) = vecTemp;
		delete pV;
#elif __PPU && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace Vectormath;
		using namespace Aos;
		vec_float4* pV = rage_new vec_float4;

		vec_float4 vecTemp = (vec_float4)(vec_splat_s32(0));
		vec_float4 vecTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp2 = cosf4fast( vecTemp );
			vecTemp = vecTemp2;
		}
		fTime = tm.GetUsTime();
		(*pV) = vecTemp;
		delete pV;
#elif NEW_RAGE_VEC_LIB && (!OLD_RAGE_VEC_LIB) 
		Vec4V* pV = rage_new Vec4V;

		Vec4V vecTemp = Vec4V(V_ZERO);
		Vec4V vecTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp2 = CosFast( vecTemp );
			vecTemp = vecTemp2;
		}
		fTime = tm.GetUsTime();
		(*pV) = vecTemp;
		delete pV;
#elif OLD_RAGE_VEC_LIB && (!NEW_RAGE_VEC_LIB)
		// N/A
#else
		fTime = 0.0f;
#endif

		return fTime;
	}

	//================================================
	//
	//================================================
	float Benchmark_FASTTan()
	{
		float fTime = -1.0f;
		sysTimer tm;

#if __XENON && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace xb;
		XMVECTOR* pV = rage_new XMVECTOR;

		XMVECTOR vecTemp;
		XMDUMMY_INITIALIZE_VECTOR(vecTemp);
		XMVECTOR vecTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp2 = XMVectorTanEst( vecTemp );
			vecTemp = vecTemp2;
		}
		fTime = tm.GetUsTime();
		(*pV) = vecTemp;
		delete pV;
#elif __PPU && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace Vectormath;
		using namespace Aos;
		vec_float4* pV = rage_new vec_float4;

		vec_float4 vecTemp = (vec_float4)(vec_splat_s32(0));
		vec_float4 vecTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp2 = tanf4fast( vecTemp );
			vecTemp = vecTemp2;
		}
		fTime = tm.GetUsTime();
		(*pV) = vecTemp;
		delete pV;
#elif NEW_RAGE_VEC_LIB && (!OLD_RAGE_VEC_LIB) 
		Vec4V* pV = rage_new Vec4V;

		Vec4V vecTemp = Vec4V(V_ZERO);
		Vec4V vecTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp2 = TanFast( vecTemp );
			vecTemp = vecTemp2;
		}
		fTime = tm.GetUsTime();
		(*pV) = vecTemp;
		delete pV;
#elif OLD_RAGE_VEC_LIB && (!NEW_RAGE_VEC_LIB)
		// N/A
#else
		fTime = 0.0f;
#endif

		return fTime;
	}

	//================================================
	//
	//================================================
	float Benchmark_FASTAsin()
	{
		float fTime = -1.0f;
		sysTimer tm;

#if __XENON && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace xb;
		XMVECTOR* pV = rage_new XMVECTOR;

		XMVECTOR vecTemp;
		XMDUMMY_INITIALIZE_VECTOR(vecTemp);
		XMVECTOR vecTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp2 = XMVectorASinEst( vecTemp );
			vecTemp = vecTemp2;
		}
		fTime = tm.GetUsTime();
		(*pV) = vecTemp;
		delete pV;
#elif __PPU && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace Vectormath;
		using namespace Aos;
		vec_float4* pV = rage_new vec_float4;

		vec_float4 vecTemp = (vec_float4)(vec_splat_s32(0));
		vec_float4 vecTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp2 = asinf4fast( vecTemp );
			vecTemp = vecTemp2;
		}
		fTime = tm.GetUsTime();
		(*pV) = vecTemp;
		delete pV;
#elif NEW_RAGE_VEC_LIB && (!OLD_RAGE_VEC_LIB) 
		Vec4V* pV = rage_new Vec4V;

		Vec4V vecTemp = Vec4V(V_ZERO);
		Vec4V vecTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp2 = ArcsinFast( vecTemp );
			vecTemp = vecTemp2;
		}
		fTime = tm.GetUsTime();
		(*pV) = vecTemp;
		delete pV;
#elif OLD_RAGE_VEC_LIB && (!NEW_RAGE_VEC_LIB)
		// N/A
#else
		fTime = 0.0f;
#endif

		return fTime;
	}

	//================================================
	//
	//================================================
	float Benchmark_FASTAcos()
	{
		float fTime = -1.0f;
		sysTimer tm;

#if __XENON && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace xb;
		XMVECTOR* pV = rage_new XMVECTOR;

		XMVECTOR vecTemp;
		XMDUMMY_INITIALIZE_VECTOR(vecTemp);
		XMVECTOR vecTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp2 = XMVectorACosEst( vecTemp );
			vecTemp = vecTemp2;
		}
		fTime = tm.GetUsTime();
		(*pV) = vecTemp;
		delete pV;
#elif __PPU && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace Vectormath;
		using namespace Aos;
		vec_float4* pV = rage_new vec_float4;

		vec_float4 vecTemp = (vec_float4)(vec_splat_s32(0));
		vec_float4 vecTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp2 = acosf4fast( vecTemp );
			vecTemp = vecTemp2;
		}
		fTime = tm.GetUsTime();
		(*pV) = vecTemp;
		delete pV;
#elif NEW_RAGE_VEC_LIB && (!OLD_RAGE_VEC_LIB) 
		Vec4V* pV = rage_new Vec4V;

		Vec4V vecTemp = Vec4V(V_ZERO);
		Vec4V vecTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp2 = ArccosFast( vecTemp );
			vecTemp = vecTemp2;
		}
		fTime = tm.GetUsTime();
		(*pV) = vecTemp;
		delete pV;
#elif OLD_RAGE_VEC_LIB && (!NEW_RAGE_VEC_LIB)
		// N/A
#else
		fTime = 0.0f;
#endif

		return fTime;
	}

	//================================================
	//
	//================================================
	float Benchmark_FASTAtan()
	{
		float fTime = -1.0f;
		sysTimer tm;

#if __XENON && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace xb;
		XMVECTOR* pV = rage_new XMVECTOR;

		XMVECTOR vecTemp;
		XMDUMMY_INITIALIZE_VECTOR(vecTemp);
		XMVECTOR vecTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp2 = XMVectorATanEst( vecTemp );
			vecTemp = vecTemp2;
		}
		fTime = tm.GetUsTime();
		(*pV) = vecTemp;
		delete pV;
#elif __PPU && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace Vectormath;
		using namespace Aos;
		vec_float4* pV = rage_new vec_float4;

		vec_float4 vecTemp = (vec_float4)(vec_splat_s32(0));
		vec_float4 vecTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp2 = atanf4fast( vecTemp );
			vecTemp = vecTemp2;
		}
		fTime = tm.GetUsTime();
		(*pV) = vecTemp;
		delete pV;
#elif NEW_RAGE_VEC_LIB && (!OLD_RAGE_VEC_LIB)
		Vec4V* pV = rage_new Vec4V;

		Vec4V vecTemp = Vec4V(V_ZERO);
		Vec4V vecTemp2;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp2 = ArctanFast( vecTemp );
			vecTemp = vecTemp2;
		}
		fTime = tm.GetUsTime();
		(*pV) = vecTemp;
		delete pV;
#elif OLD_RAGE_VEC_LIB && (!NEW_RAGE_VEC_LIB)
		// N/A
#else
		fTime = 0.0f;
#endif

		return fTime;
	}

	//================================================
	//
	//================================================
	float Benchmark_FASTAtan2()
	{
		float fTime = -1.0f;
		sysTimer tm;

#if __XENON && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace xb;
		XMVECTOR* pV = rage_new XMVECTOR;

		XMVECTOR vecTemp;
		XMDUMMY_INITIALIZE_VECTOR(vecTemp);
		XMVECTOR vecTemp2;
		XMDUMMY_INITIALIZE_VECTOR(vecTemp2);
		XMVECTOR vecTemp3;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp3 = XMVectorATan2Est( vecTemp, vecTemp2 );
			vecTemp = vecTemp2 = vecTemp3;
		}
		fTime = tm.GetUsTime();
		(*pV) = vecTemp;
		delete pV;
#elif __PPU && (!NEW_RAGE_VEC_LIB) && (!OLD_RAGE_VEC_LIB)
		using namespace Vectormath;
		using namespace Aos;
		vec_float4* pV = rage_new vec_float4;

		vec_float4 vecTemp = (vec_float4)(vec_splat_s32(0));
		vec_float4 vecTemp2 = (vec_float4)(vec_splat_s32(0));
		vec_float4 vecTemp3;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp3 = atan2f4fast( vecTemp, vecTemp2 );
			vecTemp = vecTemp2 = vecTemp3;
		}
		fTime = tm.GetUsTime();
		(*pV) = vecTemp;
		delete pV;
#elif NEW_RAGE_VEC_LIB && (!OLD_RAGE_VEC_LIB) 
		Vec4V* pV = rage_new Vec4V;

		Vec4V vecTemp = Vec4V(V_ZERO);
		Vec4V vecTemp2 = Vec4V(V_ZERO);
		Vec4V vecTemp3;
		tm.Reset();
		for(int i = 0; i < NUM_TESTS; i++)
		{
			vecTemp3 = Arctan2Fast( vecTemp, vecTemp2 );
			vecTemp = vecTemp2 = vecTemp3;
		}
		fTime = tm.GetUsTime();
		(*pV) = vecTemp;
		delete pV;
#elif OLD_RAGE_VEC_LIB && (!NEW_RAGE_VEC_LIB)
#else
		fTime = 0.0f;
#endif

		return fTime;
	}





} // namespace vecBenchmarks
} // namespace rage

// DOM-IGNORE-END

#endif // #if !__DEV
