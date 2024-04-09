#pragma once

#include "Test.h"

class CFpConvTest : public CTest
{
public:
	void Compile(Jitter::CJitter&) override;
	void Run() override;

private:
	struct CONTEXT
	{
		double inDouble;
		float inFloat;
		float outDtoS;
		double outStoD;
	};

	FunctionType m_function;
};
