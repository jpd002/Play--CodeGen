#pragma once

#include "Test.h"
#include "MemoryFunction.h"

class CMemAccess16Test : public CTest
{
public:
	void Run() override;
	void Compile(Jitter::CJitter&) override;

private:
	typedef uint16 UnitType;

	struct CONTEXT
	{
		void*    memory;
		uint32   offset;
		uint32   value;
		uint32   result0;
		uint32   result1;
		UnitType array0[0x10];
	};

	CONTEXT         m_context;
	UnitType        m_memory[0x20];
	CMemoryFunction m_function;
};
