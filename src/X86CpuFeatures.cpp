#include "X86CpuFeatures.h"
#include <array>
#include "Types.h"

//Check if CPUID is available
#ifdef _WIN32
#define HAS_CPUID
#define HAS_CPUID_MSVC
#include <intrin.h>
#endif

#if defined(__linux__)
#if defined(__i386__) || defined(__x86_64__)
#define HAS_CPUID
#define HAS_CPUID_GCC
#include <cpuid.h>
#endif
#endif

#if defined(__APPLE__)
#include <TargetConditionals.h>
#if TARGET_CPU_X86 || TARGET_CPU_X86_64
#define HAS_CPUID
#define HAS_CPUID_GCC
#include <cpuid.h>
#endif
#endif

CX86CpuFeatures CX86CpuFeatures::AutoDetect()
{
	CX86CpuFeatures features;

#ifdef HAS_CPUID
	static const uint32 CPUID_FLAG_SSSE3 = 0x000200;
	static const uint32 CPUID_FLAG_SSE41 = 0x080000;
	static const uint32 CPUID_FLAG_AVX = 0x10000000;
	static const uint32 CPUID_FLAG_AVX2 = 0x20;

#ifdef HAS_CPUID_MSVC
	std::array<int, 4> cpuInfo1;
	std::array<int, 4> cpuInfo7;
	__cpuid(cpuInfo1.data(), 1);
	__cpuid(cpuInfo7.data(), 7);
#endif //HAS_CPUID_MSVC

#ifdef HAS_CPUID_GCC
	std::array<unsigned int, 4> cpuInfo1;
	std::array<unsigned int, 4> cpuInfo7;
	__get_cpuid(1, &cpuInfo1[0], &cpuInfo1[1], &cpuInfo1[2], &cpuInfo1[3]);
	__get_cpuid(7, &cpuInfo7[0], &cpuInfo7[1], &cpuInfo7[2], &cpuInfo7[3]);
#endif //HAS_CPUID_GCC

	features.hasSsse3 = (cpuInfo1[2] & CPUID_FLAG_SSSE3) != 0;
	features.hasSse41 = (cpuInfo1[2] & CPUID_FLAG_SSE41) != 0;
	features.hasAvx = (cpuInfo1[2] & CPUID_FLAG_AVX) != 0;
	features.hasAvx2 = (cpuInfo7[1] & CPUID_FLAG_AVX2) != 0;

#endif //HAS_CPUID

	return features;
}
