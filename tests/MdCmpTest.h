#pragma once

#include "Test.h"
#include "Align16.h"
#include "MemoryFunction.h"

class CMdCmpTest : public CTest
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
		uint8			src4[16];

		uint8			dstCmpEqB[16];
		uint8			dstCmpEqH[16];
		uint8			dstCmpEqW[16];

		uint8			dstCmpGtB[16];
		uint8			dstCmpGtH[16];
		uint8			dstCmpGtW[16];
	};

	CMemoryFunction		m_function;
};
