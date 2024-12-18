#pragma once

#include "Test.h"

class CCall64Test : public CTest
{
public:
	static void PrepareExternalFunctions();

	void Compile(Jitter::CJitter&) override;
	void Run() override;

private:
	struct CONTEXT
	{
		uint64 value0;
		uint64 value1;

		uint64 result0;
		uint64 result1;
		uint64 result2;
		uint64 result3;
	};

	static uint64 Add64(uint64, uint64);
	static uint64 Sub64(uint64, uint64);
	static uint64 AddMul64(uint32, uint64, uint64);
	static uint64 AddMul64_2(uint32, uint64, uint32);

	FunctionType m_function;
};
