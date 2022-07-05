#pragma once

#include "Test.h"
#include "MemoryFunction.h"

class CRegAllocTempTest : public CTest
{
public:
	virtual ~CRegAllocTempTest() = default;

	void Compile(Jitter::CJitter&);
	void Run();

private:
	static void DummyFunction(uint32, uint32, uint32);

	struct CONTEXT
	{
		uint32 inValue = 0;
		uint32 outValue = 0;
	};

	CONTEXT m_context;
	CMemoryFunction m_function;
};
