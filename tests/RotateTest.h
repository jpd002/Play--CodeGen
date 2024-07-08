#pragma once

#include "Test.h"

class CRotateTest : public CTest
{
public:
	CRotateTest(uint32);

	void Run() override;
	void Compile(Jitter::CJitter&) override;

private:
	struct CONTEXT
	{
		uint32 value0;
		uint32 value1;

		uint32 shiftAmount;

		uint32 resultRol0;
		uint32 resultRol1;
		uint32 resultRolVar0;
		uint32 resultRolVar1;
	};

	CONTEXT m_context;
	FunctionType m_function;
	uint32 m_shiftAmount = 0;
};
