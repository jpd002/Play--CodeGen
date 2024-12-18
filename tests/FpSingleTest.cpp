#include "FpSingleTest.h"
#include "MemStream.h"

void CFpSingleTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		jitter.FP_PushRel32(offsetof(CONTEXT, number1));
		jitter.FP_PushRel32(offsetof(CONTEXT, number2));
		jitter.FP_AddS();
		jitter.FP_PullRel32(offsetof(CONTEXT, resAdd));

		jitter.FP_PushRel32(offsetof(CONTEXT, number2));
		jitter.FP_PushRel32(offsetof(CONTEXT, number4));
		jitter.FP_SubS();
		jitter.FP_PullRel32(offsetof(CONTEXT, resSub));

		jitter.FP_PushRel32(offsetof(CONTEXT, number2));
		jitter.FP_PushRel32(offsetof(CONTEXT, number2));
		jitter.FP_MulS();
		jitter.FP_PullRel32(offsetof(CONTEXT, resMul));

		jitter.FP_PushRel32(offsetof(CONTEXT, number1));
		jitter.FP_PushRel32(offsetof(CONTEXT, number2));
		jitter.FP_DivS();
		jitter.FP_PullRel32(offsetof(CONTEXT, resDiv));

		jitter.FP_PushRel32(offsetof(CONTEXT, number3));
		jitter.FP_NegS();
		jitter.FP_PullRel32(offsetof(CONTEXT, resNeg));

		jitter.FP_PushRel32(offsetof(CONTEXT, number3));
		jitter.FP_AbsS();
		jitter.FP_PullRel32(offsetof(CONTEXT, resAbs));

		jitter.FP_PushRel32(offsetof(CONTEXT, number4));
		jitter.FP_SqrtS();
		jitter.FP_PullRel32(offsetof(CONTEXT, resSqrt));

		jitter.FP_PushCst32(4);
		jitter.FP_SqrtS();
		jitter.FP_PullRel32(offsetof(CONTEXT, resSqrtCst));

		jitter.FP_PushRel32(offsetof(CONTEXT, number3));
		jitter.FP_RcplS();
		jitter.FP_PullRel32(offsetof(CONTEXT, resRcpl));

		jitter.FP_PushRel32(offsetof(CONTEXT, number4));
		jitter.FP_RsqrtS();
		jitter.FP_PullRel32(offsetof(CONTEXT, resRsqrt));

		jitter.FP_PushRel32(offsetof(CONTEXT, number1));
		jitter.FP_PushRel32(offsetof(CONTEXT, number3));
		jitter.FP_MaxS();
		jitter.FP_PullRel32(offsetof(CONTEXT, resMax));

		jitter.FP_PushRel32(offsetof(CONTEXT, number1));
		jitter.FP_PushRel32(offsetof(CONTEXT, number3));
		jitter.FP_MinS();
		jitter.FP_PullRel32(offsetof(CONTEXT, resMin));

		jitter.FP_PushRel32(offsetof(CONTEXT, number1));
		jitter.FP_PushRel32(offsetof(CONTEXT, number3));
		jitter.FP_CmpS(Jitter::CONDITION_BL);
		jitter.PullRel(offsetof(CONTEXT, ltTest));

		jitter.FP_PushRel32(offsetof(CONTEXT, number3));
		jitter.FP_PushRel32(offsetof(CONTEXT, number1));
		jitter.FP_CmpS(Jitter::CONDITION_BE);
		jitter.PullRel(offsetof(CONTEXT, leTest));

		jitter.FP_PushRel32(offsetof(CONTEXT, number1));
		jitter.FP_PushRel32(offsetof(CONTEXT, number1));
		jitter.FP_CmpS(Jitter::CONDITION_EQ);
		jitter.PullRel(offsetof(CONTEXT, eqTest));
	}
	jitter.End();

	m_function = FunctionType(codeStream.GetBuffer(), codeStream.GetSize());
}

void CFpSingleTest::Run()
{
	memset(&m_context, 0, sizeof(CONTEXT));
	m_context.number1 = 1.0;
	m_context.number2 = 2.0;
	m_context.number3 = -4.0f;
	m_context.number4 = 16.0f;
	m_function(&m_context);
	TEST_VERIFY(m_context.resAdd == 3.0f);
	TEST_VERIFY(m_context.resSub == -14.0f);
	TEST_VERIFY(m_context.resMul == 4.0f);
	TEST_VERIFY(m_context.resDiv == 0.5f);
	TEST_VERIFY(m_context.resAbs == 4.0f);
	TEST_VERIFY(m_context.resNeg == 4.0f);
	TEST_VERIFY(m_context.resSqrt == 4.0f)
	TEST_VERIFY(m_context.resSqrtCst == 2.0f);
	TEST_VERIFY(fabs(-0.25f - m_context.resRcpl) < 0.0001f); //Result not exact
	TEST_VERIFY(fabs(0.25f - m_context.resRsqrt) < 0.0001f); //Result not exact
	TEST_VERIFY(m_context.resMax == 1.0f);
	TEST_VERIFY(m_context.resMin == -4.0f);
	TEST_VERIFY(m_context.ltTest == 0);
	TEST_VERIFY(m_context.leTest != 0);
	TEST_VERIFY(m_context.eqTest != 0);
}
