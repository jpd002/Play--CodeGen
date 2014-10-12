#pragma once

#include "Test.h"
#include "Align16.h"
#include "MemoryFunction.h"

class CMdMemAccessTest : public CTest
{
public:
	void				Run() override;
	void				Compile(Jitter::CJitter&) override;

private:
	struct CONTEXT
	{
		ALIGN16

		uint32			op[4];
		uint32			result[4];

		uint32*			array;
	};

	CONTEXT				m_context;
	uint32				m_memory[0x20];
	CMemoryFunction		m_function;
};
