#pragma once

#include "Test.h"
#include "MemoryFunction.h"

class CMemAccessTest : public CTest
{
public:
	void				Run() override;
	void				Compile(Jitter::CJitter&) override;

private:
	struct CONTEXT
	{
		void*			memory;
		uint32			offset;
		uint32			result0;
		uint32			result1;
		uint32			array0[0x10];
	};

	CONTEXT				m_context;
	uint32				m_memory[0x20];
	CMemoryFunction		m_function;
};
