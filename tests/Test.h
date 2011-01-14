#ifndef _TEST_H_
#define _TEST_H_

#include "Jitter.h"

#ifdef __APPLE__
#include "TargetConditionals.h"
#endif

#ifndef TARGET_CPU_ARM
#define HAS_ADVANCED_OPS
#endif

#define TEST_VERIFY(a) if(!(a)) { int* p = 0; (*p) = 0; }

class CTest
{
public:
	virtual			~CTest() {}

	virtual void	Run()						= 0;
	virtual void	Compile(Jitter::CJitter&)	= 0;
};

#endif
