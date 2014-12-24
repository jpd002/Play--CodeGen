#pragma once

#include "Test.h"
#include "Align16.h"
#include "MemoryFunction.h"

class CMdUnpackTest : public CTest
{
public:
	void				Compile(Jitter::CJitter&) override;
	void				Run() override;

private:
	struct CONTEXT
	{
		ALIGN16

		uint8			src0[16];
		uint8			src1[16];
		uint8			src2[16];
		uint8			src3[16];

		uint8			dstUnpackLowerBH[16];
		uint8			dstUnpackLowerHW[16];
		uint8			dstUnpackLowerWD[16];

		uint8			dstUnpackUpperBH[16];
		uint8			dstUnpackUpperHW[16];
		uint8			dstUnpackUpperWD[16];
	};

	CMemoryFunction		m_function;
};
