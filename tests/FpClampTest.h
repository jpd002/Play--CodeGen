#pragma once

#include "Test.h"
#include "MemoryFunction.h"

class CFpClampTest : public CTest
{
public:
	virtual				~CFpClampTest() = default;

	void				Compile(Jitter::CJitter&);
	void				Run();

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
