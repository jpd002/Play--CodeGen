#include "FpDoubleTest.h"
#include "MemStream.h"

void CFpDoubleTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		jitter.FP_PushRel64(offsetof(CONTEXT, number1));
		jitter.FP_PushRel64(offsetof(CONTEXT, number2));
		jitter.FP_AddD();
		jitter.FP_PullRel64(offsetof(CONTEXT, resAdd));

		jitter.FP_PushRel64(offsetof(CONTEXT, number2));
		jitter.FP_PushRel64(offsetof(CONTEXT, number4));
		jitter.FP_SubD();
		jitter.FP_PullRel64(offsetof(CONTEXT, resSub));

		jitter.FP_PushRel64(offsetof(CONTEXT, number2));
		jitter.FP_PushRel64(offsetof(CONTEXT, number2));
		jitter.FP_MulD();
		jitter.FP_PullRel64(offsetof(CONTEXT, resMul));

		jitter.FP_PushRel64(offsetof(CONTEXT, number1));
		jitter.FP_PushRel64(offsetof(CONTEXT, number2));
		jitter.FP_DivD();
		jitter.FP_PullRel64(offsetof(CONTEXT, resDiv));

		jitter.FP_PushRel64(offsetof(CONTEXT, number3));
		jitter.FP_NegD();
		jitter.FP_PullRel64(offsetof(CONTEXT, resNeg));

		jitter.FP_PushRel64(offsetof(CONTEXT, number3));
		jitter.FP_AbsD();
		jitter.FP_PullRel64(offsetof(CONTEXT, resAbs));

		jitter.FP_PushCst64(4.0);
		jitter.FP_SqrtD();
		jitter.FP_PullRel64(offsetof(CONTEXT, resSqrtCst));

		jitter.FP_PushRel64(offsetof(CONTEXT, number1));
		jitter.FP_PushRel64(offsetof(CONTEXT, number3));
		jitter.FP_CmpD(Jitter::CONDITION_BL);
		jitter.PullRel(offsetof(CONTEXT, ltTest));

		jitter.FP_PushRel64(offsetof(CONTEXT, number3));
		jitter.FP_PushRel64(offsetof(CONTEXT, number1));
		jitter.FP_CmpD(Jitter::CONDITION_BE);
		jitter.PullRel(offsetof(CONTEXT, leTest));

		jitter.FP_PushRel64(offsetof(CONTEXT, number1));
		jitter.FP_PushRel64(offsetof(CONTEXT, number1));
		jitter.FP_CmpD(Jitter::CONDITION_EQ);
		jitter.PullRel(offsetof(CONTEXT, eqTest));
	}
	jitter.End();

	m_function = FunctionType(codeStream.GetBuffer(), codeStream.GetSize());
}

void CFpDoubleTest::Run()
{
	m_context = {};
	m_context.number1 = 1.0;
	m_context.number2 = 2.0;
	m_context.number3 = -4.0;
	m_context.number4 = 16.0;
	m_function(&m_context);
	TEST_VERIFY(m_context.resAdd == 3.0);
	TEST_VERIFY(m_context.resSub == -14.0);
	TEST_VERIFY(m_context.resMul == 4.0);
	TEST_VERIFY(m_context.resDiv == 0.5);
	TEST_VERIFY(m_context.resAbs == 4.0);
	TEST_VERIFY(m_context.resNeg == 4.0);
	TEST_VERIFY(m_context.resSqrtCst == 2.0);
	TEST_VERIFY(m_context.ltTest == 0);
	TEST_VERIFY(m_context.leTest != 0);
	TEST_VERIFY(m_context.eqTest != 0);
}
