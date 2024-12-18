#pragma once

#include "Test.h"
#include "Align16.h"

class CMdFpFlagTest : public CTest
{
public:
	void Compile(Jitter::CJitter&) override;
	void Run() override;

private:
	struct CONTEXT
	{
		uint32 dstSzStatus0;
		uint32 dstSzStatus1;
		uint32 dstSzStatus2;
		uint32 dstSzStatus3;

		ALIGN16

		float src0[4];
		float src1[4];
		float src2[4];
		uint32 src3[4];
	};

	FunctionType m_function;
};
