#ifndef VECTORMATH_TEST_BENCHMARKS_H
#define VECTORMATH_TEST_BENCHMARKS_H

// DOM-IGNORE-BEGIN

namespace rage
{
namespace vecBenchmarks
{
	// Run all the functions below.
	void Run();

	// The benchmarks.
	float Benchmark_DetM33();
	float Benchmark_DetM44();

	float Benchmark_MulM44M44();
	float Benchmark_MulM44V4();
	float Benchmark_MulV4M44();
	float Benchmark_MulM33M33();
	float Benchmark_MulM33V3();
	float Benchmark_MulV3M33();
	float Benchmark_TransM34M34();
	float Benchmark_TransV3M34V3();
	float Benchmark_TransP3M34V3();

	float Benchmark_InvM44();
	float Benchmark_InvM33();
	float Benchmark_InvTransM34();
	float Benchmark_InvTransOrthoM34();
	float Benchmark_UnTransOrthoM44M44();
	float Benchmark_UnTransOrthoM44V4();
	float Benchmark_UnTransOrthoM33M33();
	float Benchmark_UnTransOrthoM33V3();
	float Benchmark_UnTransOrthoM34M34();
	float Benchmark_UnTransV3OrthoM34V3();
	float Benchmark_UnTransP3OrthoM34V3();

	float Benchmark_SinAndCos();
	float Benchmark_Sin();
	float Benchmark_Cos();
	float Benchmark_Tan();
	float Benchmark_Asin();
	float Benchmark_Acos();
	float Benchmark_Atan();
	float Benchmark_Atan2();

	float Benchmark_FASTSinAndCos();
	float Benchmark_FASTSin();
	float Benchmark_FASTCos();
	float Benchmark_FASTTan();
	float Benchmark_FASTAsin();
	float Benchmark_FASTAcos();
	float Benchmark_FASTAtan();
	float Benchmark_FASTAtan2();
}
}

// DOM-IGNORE-END

#if __SPU
#include "test_benchmarks.cpp"
#endif

#endif // VECTORMATH_TEST_BENCHMARKS_H


