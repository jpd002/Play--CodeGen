#pragma once

#include "Test.h"
#include "Align16.h"
#include "uint128.h"

class CMdCallTest : public CTest
{
public:
	void				Compile(Jitter::CJitter&) override;
	void				Run() override;

private:
	struct CONTEXT
	{
		ALIGN16

		uint128			value0;
		uint128			value1;

		uint32			result0;
		uint32			result1;

		ALIGN16

		uint128			result2;
	};

	static uint32		MdInputFunction(const uint128&, uint32);
	static uint128		MdOutputFunction(uint32);

	FunctionType		m_function;
};
