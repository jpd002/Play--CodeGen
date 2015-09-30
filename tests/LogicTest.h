#pragma once

#include "Test.h"
#include "MemoryFunction.h"

class CLogicTest : public CTest
{
public:
						CLogicTest(uint32, bool, uint32, bool);
	
	void				Run() override;
	void				Compile(Jitter::CJitter&) override;

private:
	struct CONTEXT
	{
		uint32			op1;
		uint32			op2;

		uint32			resultAnd;
		uint32			resultOr;
		uint32			resultXor;
	};

	CONTEXT				m_context;
	CMemoryFunction		m_function;
	
	uint32				m_value1 = 0;
	bool				m_constant1 = false;
	uint32				m_value2 = 0;
	bool				m_constant2 = false;
};
