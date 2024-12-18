#pragma once

#include "Test.h"

class CNestedIfTest : public CTest
{
public:
	void Run() override;
	void Compile(Jitter::CJitter&) override;

private:
	struct CONTEXT
	{
		uint32 input;
		uint32 result;
	};

	CONTEXT m_context;
	FunctionType m_function;
};
