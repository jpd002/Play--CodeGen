#pragma once

#include "Test.h"
#include "MemoryFunction.h"
#include "Align16.h"

class CHugeJumpTestLiteral : public CTest
{
public:
	void Compile(Jitter::CJitter&) override;
	void Run() override;

private:
	enum MAX_VARS
	{
		MAX_VARS = 32,
	};

	struct CONTEXT
	{
		uint32 condition;
		uint32 result;

		ALIGN16
		uint32 number[MAX_VARS];
	};

	CONTEXT m_context;
	CMemoryFunction m_function;
};
