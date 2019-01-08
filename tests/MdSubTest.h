#pragma once

#include "Test.h"
#include "Align16.h"
#include "MemoryFunction.h"

class CMdSubTest : public CTest
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
		uint8			src3[16];

		uint8			srcSat0[16];
		uint8			srcSat1[16];

		uint8			dstSubB[16];
		uint8			dstSubBUS[16];
		uint8			dstSubH[16];
		uint8			dstSubHSS[16];
		uint8			dstSubHUS[16];
		uint8			dstSubW[16];
		uint8			dstSubWSS[16];
		uint8			dstSubWUS[16];
		uint8			dstSubWSSSat[16];
	};

	CMemoryFunction		m_function;
};
