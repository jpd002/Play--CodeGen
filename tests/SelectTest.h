#pragma once

#include "Test.h"

class CSelectTest : public CTest
{
public:
	CSelectTest(bool, bool);

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

		uint32 overwriteSrc2;
		uint32 overwriteSrc3;
	};
	
	bool m_valueTrueCst = false;
	bool m_valueFalseCst = false;

	CONTEXT m_context = {};
	FunctionType m_function;
};
