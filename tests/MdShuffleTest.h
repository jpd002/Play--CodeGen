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

		Vector shufResult1;
		Vector shufResult2;

		Vector permResult1;
		Vector permResult2;
		Vector permResult3;
	};

	CONTEXT m_context;
	FunctionType m_function;
};
