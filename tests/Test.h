#pragma once

#include "Jitter.h"

#define TEST_VERIFY(a) if(!(a)) { int* p = 0; (*p) = 0; }

class CTest
{
public:
	virtual			~CTest() {}

	virtual void	Run()						= 0;
	virtual void	Compile(Jitter::CJitter&)	= 0;

#ifdef _MSC_VER

	//Overload operator new and delete to make sure we get proper 16-byte aligned structures
	//C++11 has support for aligned struct/classes, but VC++2013 doesn't support them

	void* operator new(size_t allocSize)
	{
		return _aligned_malloc(allocSize, 16);
	}

	void operator delete(void* ptr)
	{
		_aligned_free(ptr);
	}

#endif
};
