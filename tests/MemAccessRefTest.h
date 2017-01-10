#pragma once

#include "Test.h"
#include "MemoryFunction.h"

class CMemAccessRefTest : public CTest
{
public:
	void    Run() override;
	void    Compile(Jitter::CJitter&) override;

private:
	void    EmitNullTest(Jitter::CJitter&, uint32, size_t);
	void    EmitNullComparison(Jitter::CJitter&, uint32, size_t);

	enum
	{
		MEMORY_SIZE = 0x20,
	};

	struct CONTEXT
	{
		void*     memory;
		uint32    readValue;
		uint32    readValueResult;
		uint32    nullCheck0;
		uint32    nullCheck1;
		uint32    nullCheck2;
		uint32    nullCheck3;
	};

	CONTEXT            m_context;
	void*              m_memory[MEMORY_SIZE];
	CMemoryFunction    m_function;
};
