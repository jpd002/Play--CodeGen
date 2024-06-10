#pragma once

#include "Test.h"

class CLzcTest : public CTest
{
public:
	void				Run() override;
	void				Compile(Jitter::CJitter&) override;

private:
	struct CONTEXT
	{
		uint32			input0;
		uint32			input1;
		uint32			input2;
		uint32			input3;

		uint64			input64_0;
		uint64			input64_1;

		uint32			resultLzc0;
		uint32			resultLzc1;
		uint32			resultLzc2;
		uint32			resultLzc3;
		uint32			resultLzc4;

		uint32			resultClz0;
		uint32			resultClz1;

		uint64			resultClz64_0;
		uint64			resultClz64_1;
	};

	CONTEXT				m_context;
	FunctionType		m_function;
};
