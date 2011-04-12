#include "FpuTest.h"
#include "MemStream.h"

CFpuTest::CFpuTest()
{

}

CFpuTest::~CFpuTest()
{
	delete m_function;
}

void CFpuTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		jitter.FP_PushSingle(offsetof(CONTEXT, number1));
		jitter.FP_PushSingle(offsetof(CONTEXT, number2));
		jitter.FP_Add();
		jitter.FP_PullSingle(offsetof(CONTEXT, number1));

		jitter.FP_PushSingle(offsetof(CONTEXT, number2));
		jitter.FP_PushSingle(offsetof(CONTEXT, number1));
		jitter.FP_Div();
		jitter.FP_PullSingle(offsetof(CONTEXT, number1));

		jitter.FP_PushSingle(offsetof(CONTEXT, number1));
		jitter.FP_Rcpl();
		jitter.FP_PullSingle(offsetof(CONTEXT, number1));

		jitter.FP_PushSingle(offsetof(CONTEXT, number1));
		jitter.FP_Neg();
		jitter.FP_PullSingle(offsetof(CONTEXT, number2));

		jitter.FP_PushSingle(offsetof(CONTEXT, number2));
		jitter.FP_Abs();
		jitter.FP_PullSingle(offsetof(CONTEXT, resAbs));

		jitter.FP_PushSingle(offsetof(CONTEXT, number2));
		jitter.FP_PushSingle(offsetof(CONTEXT, number2));
		jitter.FP_Mul();
		jitter.FP_PullSingle(offsetof(CONTEXT, number3));

		jitter.FP_PushSingle(offsetof(CONTEXT, number3));
		jitter.FP_Sqrt();
		jitter.FP_PullSingle(offsetof(CONTEXT, number3));

		jitter.FP_PushSingle(offsetof(CONTEXT, number4));
		jitter.FP_Rsqrt();
		jitter.FP_PullSingle(offsetof(CONTEXT, resRsqrt));

		jitter.FP_PushSingle(offsetof(CONTEXT, number1));
		jitter.FP_PushSingle(offsetof(CONTEXT, number2));
		jitter.FP_Max();
		jitter.FP_PullSingle(offsetof(CONTEXT, resMax));

		jitter.FP_PushSingle(offsetof(CONTEXT, number1));
		jitter.FP_PushSingle(offsetof(CONTEXT, number2));
		jitter.FP_Min();
		jitter.FP_PullSingle(offsetof(CONTEXT, resMin));
	}
	jitter.End();

	m_function = new CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}

void CFpuTest::Run()
{
	memset(&m_context, 0, sizeof(CONTEXT));
	m_context.number1 = 1.0;
	m_context.number2 = 2.0;
	m_context.number4 = 16.0f;
	(*m_function)(&m_context);
	TEST_VERIFY(m_context.number1 ==  1.5f);
	TEST_VERIFY(m_context.number2 == -1.5f);
	TEST_VERIFY(m_context.number1 == m_context.number3);
	TEST_VERIFY(m_context.resAbs == 1.5f);
	//Result is not exact
	TEST_VERIFY(fabs(0.25f - m_context.resRsqrt) < 0.001f);
	TEST_VERIFY(m_context.resMax == 1.5f);
	TEST_VERIFY(m_context.resMin == -1.5f);
}
