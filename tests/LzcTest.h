#pragma once

#include "Test.h"
#include "MemoryFunction.h"

class CLzcTest : public CTest
{
public:
	void				Run();
	void				Compile(Jitter::CJitter&);

private:
	struct CONTEXT
	{
		uint32			input0;
		uint32			input1;
		uint32			input2;
		uint32			input3;

		uint32			result0;
		uint32			result1;
		uint32			result2;
		uint32			result3;
		uint32			result4;
	};

	CONTEXT				m_context;
	CMemoryFunction		m_function;
};
