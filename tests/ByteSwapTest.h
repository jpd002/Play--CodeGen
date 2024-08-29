#pragma once

#include "Test.h"

class CByteSwapTest : public CTest
{
public:
	void Run() override;
	void Compile(Jitter::CJitter&) override;

private:
	struct CONTEXT
	{
		uint32 value0;
		uint64 value1;

		uint32 result0;
		uint64 result1;
	};

	CONTEXT m_context;
	FunctionType m_function;
};
