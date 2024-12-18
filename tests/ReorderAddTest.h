#pragma once

#include "Test.h"

class CReorderAddTest : public CTest
{
public:
	void Compile(Jitter::CJitter&);
	void Run();

private:
	struct CONTEXT
	{
		uint32 address;
		uint32 result;
		uint32 result2;
	};

	CONTEXT m_context;
	FunctionType m_function;
};
