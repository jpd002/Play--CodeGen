#ifndef _FPUTEST_H_
#define _FPUTEST_H_

#include "Test.h"
#include "MemoryFunction.h"

class CFpuTest : public CTest
{
public:
						CFpuTest();
	virtual				~CFpuTest();

	void				Compile(Jitter::CJitter&);
	void				Run();

private:
	struct CONTEXT
	{
		float number1;
		float number2;
		float number3;
		float number4;
		
		float resAbs;
		float resRsqrt;

		float resMax;
		float resMin;
	};

	CONTEXT				m_context;
	CMemoryFunction*	m_function;
};

#endif
