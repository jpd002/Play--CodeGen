#pragma once

#include "Test.h"
#include "Align16.h"
#include "MemoryFunction.h"

class CMdSubTest : public CTest
{
public:
						CMdSubTest();
	virtual				~CMdSubTest();

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

		uint8			dstSubBUS[16];
		uint8			dstSubH[16];
		uint8			dstSubHSS[16];
		uint8			dstSubHUS[16];
	};

	CMemoryFunction		m_function;
};
