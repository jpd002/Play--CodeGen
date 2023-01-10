#pragma once

#include "Test.h"

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

	FunctionType		m_function;
};
