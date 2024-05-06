#pragma once

#include "Test.h"
#include "Align16.h"
#include <array>

class CMdShuffleTest : public CTest
{
public:
	void Run() override;
	void Compile(Jitter::CJitter&) override;

private:
	using Vector = std::array<uint8, 16>;

	struct CONTEXT
	{
		ALIGN16

		Vector op1;
		Vector op2;

		Vector permVec1;
		Vector permVec2;
		Vector permVec3;

		Vector result1;
		Vector result2;
		Vector result3;
	};

	CONTEXT m_context;
	FunctionType m_function;
};
