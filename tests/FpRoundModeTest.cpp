#include "FpRoundModeTest.h"
#include "MemStream.h"

void CFpRoundModeTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		jitter.FP_SetRoundingMode(Jitter::CJitter::ROUND_TRUNCATE);
		jitter.FP_SetRoundingMode(Jitter::CJitter::ROUND_NEAREST);
	}
	jitter.End();

	m_function = FunctionType(codeStream.GetBuffer(), codeStream.GetSize());
}

void CFpRoundModeTest::Run()
{
	//There's not much to test here, we just want to have an easy way to run
	//the FP_SetRoundingMode code on the platforms that support it.
	//We could use fegetround() to obtain the rounding mode value, but again
	//since changing rounding mode is not supported everywhere, it would just
	//be a bit strange.

	CONTEXT context = {};
	m_function(&context);
}
