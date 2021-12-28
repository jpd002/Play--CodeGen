#pragma once

#include "Test.h"
#include "MemoryFunction.h"
#include "Align16.h"

class CFloatFusedMultiAddTest : public CTest
{
public:
						CFloatFusedMultiAddTest();
	virtual				~CFloatFusedMultiAddTest();

	void				Compile(Jitter::CJitter&) override;
	void				Run() override;

private:
	struct CONTEXT
	{
		ALIGN16

		float number1;
		float number2;
		float number3;
		float number4;


		float res1;
		float res2;
		float res3;
		float res4;
		float res5;
		float res6;

		
		float res1_org;
		float res2_org;
		float res3_org;
		float res4_org;
		float res5_org;
		float res6_org;
	};

	CONTEXT				m_context;
	CMemoryFunction		m_function;
};
