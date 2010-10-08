#ifndef _FPINTMIXTEST_H_
#define _FPINTMIXTEST_H_

#include "Test.h"
#include "MemoryFunction.h"

class CFpIntMixTest : public CTest
{
public:
						CFpIntMixTest();
	virtual				~CFpIntMixTest();

	void				Compile(Jitter::CJitter&);
	void				Run();

private:
	struct CONTEXT
	{
		float number1;
		float number2;
		uint32 result;
	};

	CONTEXT				m_context;
	CMemoryFunction*	m_function;
};

#endif
