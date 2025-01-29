#pragma once

#include "Test.h"

class CSelectTest : public CTest
{
public:
	void Run() override;
	void Compile(Jitter::CJitter&) override;
	
private:
	struct CONTEXT
	{
		uint32 cmp0;
		uint32 cmp1;
		
		uint32 valueFalse;
		uint32 valueTrue;
		
		uint32 result;
	};
	
	CONTEXT m_context;
	FunctionType m_function;
};
