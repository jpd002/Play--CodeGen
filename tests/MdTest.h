#pragma once

#include "Test.h"
#include "Align16.h"
#include "MemoryFunction.h"

class CMdTest : public CTest
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

		uint8			dstMov[16];

		uint8			dstSrl256_1[16];
		uint8			dstSrl256_2[16];

		uint8			dstPackHB[16];
		uint8			dstPackWH[16];
		
		uint8			dstPackHBAlias[16];
		uint8			dstPackWHAlias[16];

		uint32			shiftAmount;
	};

	CMemoryFunction		m_function;
};
