#pragma once

#include "Test.h"

class CRegAllocTest : public CTest
{
public:
	void Compile(Jitter::CJitter&) override;
	void Run() override;

private:
	enum MAX_VARS
	{
		MAX_VARS = 16,
	};

	struct CONTEXT
	{
		uint32 number[MAX_VARS];
	};

	CONTEXT m_context;
	FunctionType m_function;
};
