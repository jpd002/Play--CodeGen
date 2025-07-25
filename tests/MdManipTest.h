#pragma once

#include "Test.h"
#include "Align16.h"

class CMdManipTest : public CTest
{
public:
	void Compile(Jitter::CJitter&) override;
	void Run() override;

private:
	struct CONTEXT
	{
		ALIGN16

		float src0[4];
		float src1[4];
		float src2[4];

		float dstMasked0[4];
		float dstMasked1[4];

		float dstExpandWRel[4];
		float dstExpandRelElem0[4];
		float dstExpandRelElem1[4];
		float dstExpandRelElem2[4];
		float dstExpandRelElem3[4];
		float dstExpandCst[4];
		float dstExpandCstZero[4];
		float dstExpandCstOne[4];
	};

	FunctionType m_function;
};
