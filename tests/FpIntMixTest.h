#pragma once

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
		uint32 multiplier;
		float number1;
		float number2;
		float number3;
		uint32 result1;
		uint32 result2;
	};

	CONTEXT				m_context;
	CMemoryFunction		m_function;
};
