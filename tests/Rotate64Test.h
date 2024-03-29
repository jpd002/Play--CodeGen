#pragma once

#include "Test.h"

class CRotate64Test : public CTest
{
public:
	CRotate64Test(uint32);

	void Run() override;
	void Compile(Jitter::CJitter&) override;

private:
	struct CONTEXT
	{
		uint64 value0;
		uint64 value1;

		uint32 shiftAmount;

		uint64 resultRol0;
		uint64 resultRol1;
	};

	CONTEXT m_context;
	FunctionType m_function;
	uint32 m_shiftAmount = 0;
};
