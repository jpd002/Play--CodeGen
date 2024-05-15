#include "FpClampTest.h"
#include "MemStream.h"

void CFpClampTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		//0 - +NaN = 0x7F7FFFFF -- Scalar
		{
			jitter.FP_PushRel32(offsetof(CONTEXT, zero));
			jitter.FP_ClampS();

			jitter.FP_PushRel32(offsetof(CONTEXT, positiveNan));
			jitter.FP_ClampS();

			jitter.FP_AddS();

			jitter.FP_PullRel32(offsetof(CONTEXT, result1));
		}

		//0 + -NaN = 0xFF7FFFFF -- Scalar
		{
			jitter.FP_PushRel32(offsetof(CONTEXT, zero));
			jitter.FP_ClampS();

			jitter.FP_PushRel32(offsetof(CONTEXT, negativeNan));
			jitter.FP_ClampS();

			jitter.FP_AddS();

			jitter.FP_PullRel32(offsetof(CONTEXT, result2));
		}

		//0 - +NaN = 0x7F7FFFFF -- Vector
		{
			jitter.MD_PushRel(offsetof(CONTEXT, zero));
			jitter.MD_ClampS();

			jitter.MD_PushRel(offsetof(CONTEXT, mdPositiveNan));
			jitter.MD_ClampS();

			jitter.MD_AddS();

			jitter.MD_PullRel(offsetof(CONTEXT, mdResult1));
		}

		//0 + -NaN = 0xFF7FFFFF -- Vector
		{
			jitter.MD_PushRel(offsetof(CONTEXT, mdZero));
			jitter.MD_ClampS();

			jitter.MD_PushRel(offsetof(CONTEXT, mdNegativeNan));
			jitter.MD_ClampS();

			jitter.MD_AddS();

			jitter.MD_PullRel(offsetof(CONTEXT, mdResult2));
		}
	}
	jitter.End();

	m_function = FunctionType(codeStream.GetBuffer(), codeStream.GetSize());
}

void CFpClampTest::Run()
{
	CONTEXT context = {};

	context.positiveNan = std::numeric_limits<float>::quiet_NaN();
	context.negativeNan = -std::numeric_limits<float>::quiet_NaN();

	for(int i = 0; i < 4; i++)
	{
		context.mdPositiveNan[i] = std::numeric_limits<float>::quiet_NaN();
		context.mdNegativeNan[i] = -std::numeric_limits<float>::quiet_NaN();
	}

	m_function(&context);

	TEST_VERIFY(!std::isunordered(context.result1, 0));
	TEST_VERIFY(!std::isunordered(context.result2, 0));
	TEST_VERIFY(context.result1 > context.result2);

	for(int i = 0; i < 4; i++)
	{
		TEST_VERIFY(!std::isunordered(context.mdResult1[i], 0));
		TEST_VERIFY(!std::isunordered(context.mdResult2[i], 0));
		TEST_VERIFY(context.mdResult1[i] > context.mdResult2[i]);
	}
}
