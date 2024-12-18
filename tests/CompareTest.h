#pragma once

#include "Test.h"

class CCompareTest : public CTest
{
public:
	void Run() override;
	void Compile(Jitter::CJitter&) override;

private:
	struct CONTEXT
	{
		uint32 number1;
		uint32 number2;
		uint32 number3;
		uint32 number4;
		uint32 number5;
	};

	CONTEXT m_context;
	FunctionType m_function;
};
