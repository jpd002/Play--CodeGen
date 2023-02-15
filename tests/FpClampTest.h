#pragma once

#include "Test.h"
#include "Align16.h"

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

		ALIGN16

		float mdZero[4];
		float mdPositiveNan[4];
		float mdNegativeNan[4];

		float mdResult1[4];
		float mdResult2[4];
	};

	FunctionType		m_function;
};
