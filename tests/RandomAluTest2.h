#pragma once

#include "Test.h"

class CRandomAluTest2 : public CTest
{
public:
						CRandomAluTest2(bool);

	void				Compile(Jitter::CJitter&) override;
	void				Run() override;

private:
	struct CONTEXT
	{
		uint32 number1;
		uint32 number2;
		uint32 result;
	};

	bool				m_useConstant;
	CONTEXT				m_context;
	FunctionType		m_function;
};
