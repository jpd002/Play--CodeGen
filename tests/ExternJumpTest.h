#pragma once

#include "Test.h"
#include "MemoryFunction.h"

class CExternJumpTest : public CTest
{
public:
	virtual ~CExternJumpTest() = default;

	void Compile(Jitter::CJitter&) override;
	void Run() override;

private:
	struct CONTEXT
	{
		uint32 cst1 = 0;
		uint32 cst2 = 0;
		uint32 result1 = 0;
		uint32 result2 = 0;
	};

	CMemoryFunction m_sourceFunction;
	CMemoryFunction m_targetFunction;
};
