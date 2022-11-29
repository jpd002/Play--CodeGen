#pragma once

#include "Test.h"
#include "MemoryFunction.h"

class CGotoTest : public CTest
{
public:
	void Compile(Jitter::CJitter&) override;
	void Run() override;

private:
	enum RESULT
	{
		RESULT_NONE = 0,
		RESULT_IF = 1,
		RESULT_ELSE = 2,
		RESULT_OUTSIDE = 3,
	};

	struct CONTEXT
	{
		uint32 condition;
		uint32 result;
	};

	CONTEXT m_context;
	CMemoryFunction m_function;
};
