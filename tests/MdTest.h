#ifndef _MDTEST_H_
#define _MDTEST_H_

#include "Test.h"
#include "Align16.h"
#include "MemoryFunction.h"

class CMdTest : public CTest
{
public:
						CMdTest();
	virtual				~CMdTest();

	void				Compile(Jitter::CJitter&);
	void				Run();

private:
	struct CONTEXT
	{
		ALIGN16

		uint8			src0[16];
		uint8			src1[16];
		uint8			src2[16];
		uint8			src3[16];

		uint8			dstMov[16];
		uint8			dstAddB[16];
		uint8			dstAddBUS[16];
		uint8			dstAddH[16];
		uint8			dstAddHSS[16];
		uint8			dstAddW[16];
		uint8			dstAddWUS[16];
		uint8			dstAddWSS[16];

		uint8			dstSubHSS[16];

		uint8			dstCmpEqW[16];
		uint8			dstCmpGtH[16];

		uint8			dstMinH[16];
		uint8			dstMaxH[16];
		uint8			dstMaxW[16];

		uint8			dstMinW[16];

		uint8			dstSrlH[16];
		uint8			dstSllH[16];
		uint8			dstSraH[16];
		uint8			dstSrlW[16];
		uint8			dstSraW[16];
		uint8			dstSllW[16];
		uint8			dstSrl256_1[16];
		uint8			dstSrl256_2[16];

		uint8			dstPackHB[16];
		uint8			dstPackWH[16];

		uint8			dstUnpackLowerBH[16];
		uint8			dstUnpackLowerHW[16];
		uint8			dstUnpackLowerWD[16];

		uint8			dstUnpackUpperBH[16];
		uint8			dstUnpackUpperWD[16];

		uint32			shiftAmount;
	};

	uint32				ComputeWordUnsignedSaturation(uint32, uint32);
	uint32				ComputeWordSignedSaturation(uint32, uint32);

	CMemoryFunction		m_function;
};

#endif
