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
		uint32 value = 0;
	};

	CMemoryFunction m_sourceFunction;
	CMemoryFunction m_targetFunction;
};
