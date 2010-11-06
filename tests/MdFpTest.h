#ifndef _MDFPTEST_H_
#define _MDFPTEST_H_

#include "Test.h"
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
#ifdef WIN32
__declspec(align(16))
#else
__attribute__((aligned(16)))
#endif
		float			src0[4];
		float			src1[4];
		float			src2[4];

		float			dstAdd[4];
		float			dstSub[4];
		float			dstMul[4];
		float			dstMax[4];
		float			dstMin[4];

		float			dstMasked[4];

		uint32			dstIsNegative[4];
		uint32			dstIsZero[4];

		float			dstExpandRel[4];
		float			dstExpandCst[4];

		uint32			dstCvtWord[4];
	};

	CMemoryFunction*	m_function;
};

#endif
