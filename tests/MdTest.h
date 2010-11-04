#ifndef _MDTEST_H_
#define _MDTEST_H_

#include "Test.h"
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
#ifdef WIN32
__declspec(align(16))
#else
__attribute__((aligned(16)))
#endif
		uint8			src0[16];
		uint8			src1[16];
		uint8			src2[16];

		uint8			dstAddWSS[16];
		uint8			dstUnpackLowerHW[16];
		uint8			dstUnpackLowerWD[16];
		uint8			dstUnpackUpperWD[16];
	};

	uint32				ComputeWordSignedSaturation(uint32, uint32);

	CMemoryFunction*	m_function;
};

#endif
