#include "FloatFusedMultiAddTest.h"
#include "MemStream.h"

CFloatFusedMultiAddTest::CFloatFusedMultiAddTest()
{
}

CFloatFusedMultiAddTest::~CFloatFusedMultiAddTest()
{
}

void CFloatFusedMultiAddTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		jitter.FP_PushSingle(offsetof(CONTEXT, number3));
		jitter.FP_PushSingle(offsetof(CONTEXT, number2));
		jitter.FP_PushSingle(offsetof(CONTEXT, number1));
		jitter.FP_MulAdd();
		jitter.FP_PullSingle(offsetof(CONTEXT, res1));

		jitter.FP_PushSingle(offsetof(CONTEXT, number4));
		jitter.FP_PushSingle(offsetof(CONTEXT, number2));
		jitter.FP_PushSingle(offsetof(CONTEXT, number2));
		jitter.FP_MulAdd();
		jitter.FP_PullSingle(offsetof(CONTEXT, res2));

		jitter.FP_PushSingle(offsetof(CONTEXT, number3));
		jitter.FP_PushSingle(offsetof(CONTEXT, number4));
		jitter.FP_PushSingle(offsetof(CONTEXT, number2));
		jitter.FP_MulAdd();
		jitter.FP_PullSingle(offsetof(CONTEXT, res3));

		jitter.FP_PushSingle(offsetof(CONTEXT, number3));
		jitter.FP_PushSingle(offsetof(CONTEXT, number2));
		jitter.FP_PushSingle(offsetof(CONTEXT, number1));
		jitter.FP_MulSub();
		jitter.FP_PullSingle(offsetof(CONTEXT, res4));

		jitter.FP_PushSingle(offsetof(CONTEXT, number4));
		jitter.FP_PushSingle(offsetof(CONTEXT, number2));
		jitter.FP_PushSingle(offsetof(CONTEXT, number2));
		jitter.FP_MulSub();
		jitter.FP_PullSingle(offsetof(CONTEXT, res5));

		jitter.FP_PushSingle(offsetof(CONTEXT, number3));
		jitter.FP_PushSingle(offsetof(CONTEXT, number4));
		jitter.FP_PushSingle(offsetof(CONTEXT, number2));
		jitter.FP_MulSub();
		jitter.FP_PullSingle(offsetof(CONTEXT, res6));

		jitter.FP_PushSingle(offsetof(CONTEXT, number3));
		jitter.FP_PushSingle(offsetof(CONTEXT, number2));
		jitter.FP_PushSingle(offsetof(CONTEXT, number1));
		jitter.FP_Mul();
		jitter.FP_Add();
		jitter.FP_PullSingle(offsetof(CONTEXT, res1_org));

		jitter.FP_PushSingle(offsetof(CONTEXT, number4));
		jitter.FP_PushSingle(offsetof(CONTEXT, number2));
		jitter.FP_PushSingle(offsetof(CONTEXT, number2));
		jitter.FP_Mul();
		jitter.FP_Add();
		jitter.FP_PullSingle(offsetof(CONTEXT, res2_org));

		jitter.FP_PushSingle(offsetof(CONTEXT, number3));
		jitter.FP_PushSingle(offsetof(CONTEXT, number4));
		jitter.FP_PushSingle(offsetof(CONTEXT, number2));
		jitter.FP_Mul();
		jitter.FP_Add();
		jitter.FP_PullSingle(offsetof(CONTEXT, res3_org));

		jitter.FP_PushSingle(offsetof(CONTEXT, number3));
		jitter.FP_PushSingle(offsetof(CONTEXT, number2));
		jitter.FP_PushSingle(offsetof(CONTEXT, number1));
		jitter.FP_Mul();
		jitter.FP_Sub();
		jitter.FP_PullSingle(offsetof(CONTEXT, res4_org));

		jitter.FP_PushSingle(offsetof(CONTEXT, number4));
		jitter.FP_PushSingle(offsetof(CONTEXT, number2));
		jitter.FP_PushSingle(offsetof(CONTEXT, number2));
		jitter.FP_Mul();
		jitter.FP_Sub();
		jitter.FP_PullSingle(offsetof(CONTEXT, res5_org));

		jitter.FP_PushSingle(offsetof(CONTEXT, number3));
		jitter.FP_PushSingle(offsetof(CONTEXT, number4));
		jitter.FP_PushSingle(offsetof(CONTEXT, number2));
		jitter.FP_Mul();
		jitter.FP_Sub();
		jitter.FP_PullSingle(offsetof(CONTEXT, res6_org));
	}
	jitter.End();

	m_function = CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}

void CFloatFusedMultiAddTest::Run()
{
	memset(&m_context, 0, sizeof(CONTEXT));
	m_context.number1 = 1.0f;
	m_context.number2 = 2.0f;
	m_context.number3 = -4.0f;
	m_context.number4 = 16.0f;
	m_function(&m_context);
	TEST_VERIFY(m_context.res1 == m_context.res1_org);
	TEST_VERIFY(m_context.res2 == m_context.res2_org);
	TEST_VERIFY(m_context.res3 == m_context.res3_org);

	TEST_VERIFY(m_context.res4 == m_context.res4_org);
	TEST_VERIFY(m_context.res5 == m_context.res5_org);
	TEST_VERIFY(m_context.res6 == m_context.res6_org);
}
