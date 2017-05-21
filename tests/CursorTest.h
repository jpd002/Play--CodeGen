#pragma once

#include "Test.h"
#include "MemoryFunction.h"

class CCursorTest : public CTest
{
public:
	void Run() override;
	void Compile(Jitter::CJitter&) override;

private:
	struct CONTEXT
	{
		uint32 result1 = 0;
		uint32 result2 = 0;
	};

	CMemoryFunction m_function;
};
