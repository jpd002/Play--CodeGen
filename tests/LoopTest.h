#pragma once

#include "Test.h"
#include "MemoryFunction.h"

class CLoopTest : public CTest
{
public:
	void				Run() override;
	void				Compile(Jitter::CJitter&) override;

private:
	struct CONTEXT
	{
		uint32 counter = 0;
		uint32 total = 0;
	};

	CONTEXT				m_context;
	CMemoryFunction		m_function;
};
