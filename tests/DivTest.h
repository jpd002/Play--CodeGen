#pragma once

#include "Test.h"

class CDivTest : public CTest
{
public:
						CDivTest(bool);

	void				Run() override;
	void				Compile(Jitter::CJitter&) override;

private:
	struct CONTEXT
	{
		uint32			cstResultLo;
		uint32			cstResultHi;

		uint32			relArg0;
		uint32			relArg1;

		uint32			relResultLo;
		uint32			relResultHi;
	};

	bool				m_isSigned;
	CONTEXT				m_context;
	FunctionType		m_function;
};
