#pragma once

#include "Test.h"

class CShiftTest : public CTest
{
public:
	CShiftTest(uint32);

	void Run() override;
	void Compile(Jitter::CJitter&) override;

private:
	struct CONTEXT
	{
		uint32 value0;
		uint32 value1;

		uint32 shiftAmount;

		uint32 resultSra0;
		uint32 resultSra1;

		uint32 resultSrl0;
		uint32 resultSrl1;

		uint32 resultShl0;
		uint32 resultShl1;

		uint32 resultSraVar0;
		uint32 resultSraVar1;

		uint32 resultSrlVar0;
		uint32 resultSrlVar1;

		uint32 resultShlVar0;
		uint32 resultShlVar1;
	};

	CONTEXT m_context;
	FunctionType m_function;
	uint32 m_shiftAmount = 0;
};
