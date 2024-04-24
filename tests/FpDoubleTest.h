#pragma once

#include "Test.h"

class CFpDoubleTest : public CTest
{
public:
	void				Compile(Jitter::CJitter&) override;
	void				Run() override;

private:
	struct CONTEXT
	{
		double number1;
		double number2;
		double number3;
		double number4;
		
		double resMul;
		double resDiv;

		uint32 ltTest;
		uint32 leTest;
		uint32 eqTest;
	};

	CONTEXT				m_context;
	FunctionType		m_function;
};
