#pragma once

#include "Test.h"

class CMemAccessIdxTest : public CTest
{
public:
	void Run() override;
	void Compile(Jitter::CJitter&) override;

private:
	typedef uint32 UnitType;

	struct CONTEXT
	{
		void*  memory;
		uint32 index0;
		uint32 index2;
		uint32 index0Value;
		uint32 index1Value;
		uint32 index4Value;
	};

	CONTEXT         m_context;
	UnitType        m_memory[0x20];
	FunctionType    m_function;
};
