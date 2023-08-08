#pragma once

#include "Test.h"

class CMemAccess64Test : public CTest
{
public:
	CMemAccess64Test(bool);

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
		uint64    readValueIdx;

		uint32    storeIdx0;
		uint32    storeIdx1;
		uint32    storeIdx2;
		uint32    storeIdx3;
		uint32    loadIdx0;
		uint32    loadIdx1;
	};

	CONTEXT            m_context;
	uint64             m_memory[MEMORY_SIZE];
	FunctionType       m_function;
	bool               m_useVariableIndices;
};
