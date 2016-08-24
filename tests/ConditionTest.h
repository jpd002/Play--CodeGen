#pragma once

#include "Test.h"
#include "MemoryFunction.h"

class CConditionTest : public CTest
{
public:
						CConditionTest(bool, uint32, uint32);

	void				Run() override;
	void				Compile(Jitter::CJitter&) override;

private:

	void MakeBeginIfCase(Jitter::CJitter& jitter, Jitter::CONDITION, size_t result);

	struct CONTEXT
	{
		uint32			value0;
		uint32			value1;

		uint32			resultEq;
		uint32			resultNe;
		uint32			resultBl;
		uint32			resultBe;
		uint32			resultAe;
		uint32			resultLt;
		uint32			resultLe;
		uint32			resultAb;
		uint32			resultGt;
		uint32			resultGe;
	};

	bool				m_useConstant = false;
	uint32				m_value0 = 0;
	uint32				m_value1 = 0;
	CONTEXT				m_context;
	CMemoryFunction		m_function;
};
