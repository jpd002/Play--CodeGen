#include "FpDoubleTest.h"
#include "MemStream.h"

void CFpDoubleTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
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
	TEST_VERIFY(m_context.ltTest == 0);
	TEST_VERIFY(m_context.leTest != 0);
	TEST_VERIFY(m_context.eqTest != 0);
}
