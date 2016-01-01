#pragma once

#include "Test.h"
#include "MemoryFunction.h"

class CMemAccess64Test : public CTest
{
public:
	void    Run() override;
	void    Compile(Jitter::CJitter&) override;

private:
	enum
	{
		MEMORY_SIZE = 0x20,
	};

	struct CONTEXT
	{
		void*     memory;
		uint64    writeValue;
		uint64    readValue;
	};

	CONTEXT            m_context;
	uint64             m_memory[MEMORY_SIZE];
	CMemoryFunction    m_function;
};
