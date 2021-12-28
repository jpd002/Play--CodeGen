#pragma once

#include "Test.h"
#include "MemoryFunction.h"
#include "Align16.h"

class CDoubleFusedMultiAddTest : public CTest
{
public:
						CDoubleFusedMultiAddTest();
	virtual				~CDoubleFusedMultiAddTest();

	void				Compile(Jitter::CJitter&) override;
	void				Run() override;

private:
	struct CONTEXT
	{
		ALIGN16

		uint8 number1[16];
		uint8 number2[16];
		uint8 number3[16];
		uint8 number4[16];


		uint8 res1[16];
		uint8 res2[16];
		uint8 res3[16];
		uint8 res4[16];
		uint8 res5[16];
		uint8 res6[16];
	};

	CONTEXT				m_context;
	CMemoryFunction		m_function;
};
