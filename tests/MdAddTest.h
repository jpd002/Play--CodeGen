#pragma once

#include "Test.h"
#include "Align16.h"
#include "MemoryFunction.h"

class CMdAddTest : public CTest
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

		uint8			dstAddB[16];
		uint8			dstAddBUS[16];
		uint8			dstAddBSS[16];
		uint8			dstAddH[16];
		uint8			dstAddHUS[16];
		uint8			dstAddHSS[16];
		uint8			dstAddW[16];
		uint8			dstAddWUS[16];
		uint8			dstAddWSS[16];
	};

	uint32				ComputeWordUnsignedSaturation(uint32, uint32);
	uint32				ComputeWordSignedSaturation(uint32, uint32);

	CMemoryFunction		m_function;
};
