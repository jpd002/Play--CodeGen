#ifndef _MDFPTEST_H_
#define _MDFPTEST_H_

#include "Test.h"
#include "Align16.h"
#include "MemoryFunction.h"

class CMdFpTest : public CTest
{
public:
						CMdFpTest();
	virtual				~CMdFpTest();

	void				Compile(Jitter::CJitter&);
	void				Run();

private:
	struct CONTEXT
	{
		uint32			dstIsNegative;
		uint32			dstIsZero;

		ALIGN16

		float			src0[4];
		float			src1[4];
		float			src2[4];

		float			dstAdd[4];
		float			dstSub[4];
		float			dstMul[4];
		float			dstDiv[4];
		float			dstAbs[4];
		float			dstMax[4];
		float			dstMin[4];

		float			dstMasked[4];

		float			dstExpandRel[4];
		float			dstExpandCst[4];

		uint32			dstCvtWord[4];
		float			dstCvtSingle[4];
	};

	CMemoryFunction*	m_function;
};

#endif
