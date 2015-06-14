#pragma once

#include "AlignedAlloc.h"
#include "Jitter.h"

#define TEST_VERIFY(a) if(!(a)) { int* p = 0; (*p) = 0; }

class CTest
{
public:
	virtual			~CTest() {}

	virtual void	Run()						= 0;
	virtual void	Compile(Jitter::CJitter&)	= 0;

	void* operator new(size_t allocSize)
	{
		return framework_aligned_alloc(allocSize, 0x10);
	}

	void operator delete(void* ptr)
	{
		framework_aligned_free(ptr);
	}
};
