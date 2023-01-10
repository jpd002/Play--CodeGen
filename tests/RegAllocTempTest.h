#pragma once

#include "Test.h"

class CRegAllocTempTest : public CTest
{
public:
	static void PrepareExternalFunctions();

	void Compile(Jitter::CJitter&) override;
	void Run() override;

private:
	struct CONTEXT
	{
		uint32 inValue = 0;
		uint32 outValue = 0;
	};

	CONTEXT m_context;
	FunctionType m_function;
};
