#pragma once

#include "Jitter.h"

#define TEST_VERIFY(a) if(!(a)) { int* p = 0; (*p) = 0; }

class CTest
{
public:
	virtual			~CTest() {}

	virtual void	Run()						= 0;
	virtual void	Compile(Jitter::CJitter&)	= 0;
};
