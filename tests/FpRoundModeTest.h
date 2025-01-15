#pragma once

#include "Test.h"
#include "Align16.h"

class CFpRoundModeTest : public CTest
{
public:
	void Compile(Jitter::CJitter&) override;
	void Run() override;

private:
	struct CONTEXT
	{
		float zero;
	};

	FunctionType m_function;
};
