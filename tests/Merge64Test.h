#pragma once

#include "Test.h"
#include "MemoryFunction.h"

class CMerge64Test : public CTest
{
public:
	void				Run() override;
	void				Compile(Jitter::CJitter&) override;

private:
	struct CONTEXT
	{
		uint32			op1;
		uint32			op2;

		uint64			resultMemMem;
		uint64			resultMemCst;
	};

	CONTEXT				m_context;
	CMemoryFunction		m_function;
};
