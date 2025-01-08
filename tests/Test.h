#pragma once

#include <cstdio>
#include <cstring>
#include <cmath>

#include "AlignedAlloc.h"
#include "Jitter.h"
#include "offsetof_def.h"
#include "MemoryFunction.h"

#ifdef __ANDROID__
#include <android/log.h>
#endif

#ifdef __ANDROID__
#define PRINT_ERROR(...) __android_log_print(ANDROID_LOG_ERROR, "CodeGenTestSuite", __VA_ARGS__)
#else
#define PRINT_ERROR(...) printf(__VA_ARGS__)
#endif

#define TEST_VERIFY(a)                                             \
	if(!(a))                                                       \
	{                                                              \
		PRINT_ERROR("Verification failed: '%s'. Aborting.\n", #a); \
		std::abort();                                              \
	}

class CTest
{
public:
	typedef CMemoryFunction FunctionType;

	virtual ~CTest() = default;

	virtual void Run() = 0;
	virtual void Compile(Jitter::CJitter&) = 0;

	void* operator new(size_t allocSize)
	{
		return framework_aligned_alloc(allocSize, 0x10);
	}

	void operator delete(void* ptr)
	{
		framework_aligned_free(ptr);
	}
};
