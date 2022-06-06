#include "FpClampTest.h"
#include "MemStream.h"

void CFpClampTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		//0 - +NaN = 0x7F7FFFFF
		{
			jitter.FP_PushSingle(offsetof(CONTEXT, zero));
			jitter.FP_Clamp();

			jitter.FP_PushSingle(offsetof(CONTEXT, positiveNan));
			jitter.FP_Clamp();

			jitter.FP_Add();

			jitter.FP_PullSingle(offsetof(CONTEXT, result1));
		}

		//0 + -NaN = 0xFF7FFFFF
		{
			jitter.FP_PushSingle(offsetof(CONTEXT, zero));
			jitter.FP_Clamp();

			jitter.FP_PushSingle(offsetof(CONTEXT, negativeNan));
			jitter.FP_Clamp();

			jitter.FP_Add();

			jitter.FP_PullSingle(offsetof(CONTEXT, result2));
		}
	}
	jitter.End();

	m_function = CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}

void CFpClampTest::Run()
{
	CONTEXT context;
	memset(&context, 0, sizeof(CONTEXT));

	context.positiveNan = std::numeric_limits<float>::quiet_NaN();
	context.negativeNan = -std::numeric_limits<float>::quiet_NaN();

	m_function(&context);

	TEST_VERIFY(!std::isunordered(context.result1, 0));
	TEST_VERIFY(!std::isunordered(context.result2, 0));
}
