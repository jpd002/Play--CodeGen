#pragma once

#include "Test.h"

class CCompareTest2 : public CTest
{
public:
	CCompareTest2(bool, bool, uint32, uint32);

	void Run() override;
	void Compile(Jitter::CJitter&) override;

private:
	struct CONTEXT
	{
		uint32 value0;
		uint32 value1;

		uint32 resultEq;
		uint32 resultNe;
		uint32 resultBl;
		uint32 resultBe;
		uint32 resultAb;
		uint32 resultAe;
		uint32 resultLt;
		uint32 resultLe;
		uint32 resultGt;
		uint32 resultGe;
	};

	bool m_useConstant0 = false;
	bool m_useConstant1 = false;
	uint32 m_value0 = 0;
	uint32 m_value1 = 0;
	CONTEXT m_context = {};
	FunctionType m_function;
};
