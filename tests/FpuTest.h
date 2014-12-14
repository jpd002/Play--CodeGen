#pragma once

#include "Test.h"
#include "MemoryFunction.h"

class CFpuTest : public CTest
{
public:
						CFpuTest();
	virtual				~CFpuTest();

	void				Compile(Jitter::CJitter&) override;
	void				Run() override;

private:
	struct CONTEXT
	{
		float number1;
		float number2;
		float number3;
		float number4;
		
		float resSub;
		float resAbs;
		float resRsqrt;
		float resSqrtCst;

		float resMax;
		float resMin;

		uint32 ltTest;
		uint32 leTest;
		uint32 eqTest;
	};

	CONTEXT				m_context;
	CMemoryFunction		m_function;
};
