#pragma once

#include "Test.h"

class CMemAccess8Test : public CTest
{
public:
	typedef uint8 MemoryValueType;
	typedef uint32 VariableValueType;

	CMemAccess8Test(bool);

	void Run() override;
	void Compile(Jitter::CJitter&) override;

private:
	enum
	{
		MEMORY_SIZE = 0x20,
	};

	struct CONTEXT
	{
		void* memory;
		VariableValueType writeValue;
		VariableValueType readValue;
		VariableValueType readValueIdx;

		uint32 storeIdx0;
		uint32 storeIdx1;
		uint32 storeIdx2;
		uint32 storeIdx3;
		uint32 loadIdx0;
		uint32 loadIdx1;
	};

	CONTEXT m_context;
	MemoryValueType m_memory[MEMORY_SIZE];
	FunctionType m_function;
	bool m_useVariableIndices;
};
