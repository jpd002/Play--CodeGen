#pragma once

#include "Test.h"

class CMultTest : public CTest
{
public:
	CMultTest(bool);

	void Run() override;
	void Compile(Jitter::CJitter&) override;

private:
	struct CONTEXT
	{
		uint32 relArg0;
		uint32 relArg1;

		uint32 cstCstResultLo;
		uint32 cstCstResultHi;

		uint32 relRelResultLo;
		uint32 relRelResultHi;

		uint32 relCstResultLo;
		uint32 relCstResultHi;
	};

	bool m_isSigned;
	CONTEXT m_context;
	FunctionType m_function;
};
