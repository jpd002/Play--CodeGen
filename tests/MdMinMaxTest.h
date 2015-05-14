#pragma once

#include "Test.h"
#include "Align16.h"
#include "MemoryFunction.h"

class CMdMinMaxTest : public CTest
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

		uint8			dstMinH[16];
		uint8			dstMinW[16];

		uint8			dstMaxH[16];
		uint8			dstMaxW[16];
	};

	CMemoryFunction		m_function;
};
