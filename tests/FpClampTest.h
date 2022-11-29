#pragma once

#include "Test.h"
#include "MemoryFunction.h"

class CFpClampTest : public CTest
{
public:
	void				Compile(Jitter::CJitter&) override;
	void				Run() override;

private:
	struct CONTEXT
	{
		float zero;
		float positiveNan;
		float negativeNan;

		float result1;
		float result2;
	};

	CMemoryFunction		m_function;
};
