#pragma once

#include "Test.h"

class CFpIntMixTest : public CTest
{
public:
	void				Compile(Jitter::CJitter&) override;
	void				Run() override;

private:
	struct CONTEXT
	{
		uint32 multiplier;
		uint64 addend;
		float number1;
		float number2;
		float number3;
		double number4;
		double number5;
		uint32 result1;
		uint32 result2;
		uint32 result3;
		uint64 result4;
	};

	CONTEXT				m_context;
	FunctionType		m_function;
};
