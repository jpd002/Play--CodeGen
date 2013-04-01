#include "CompareTest.h"
#include "MemStream.h"

CCompareTest::CCompareTest()
{

}

CCompareTest::~CCompareTest()
{

}

void CCompareTest::Run()
{
	memset(&m_context, 0, sizeof(m_context));
	m_context.number1 = 0x80000000;
	m_context.number2 = 0x10;
	m_context.number3 = 0x10000;

	m_function(&m_context);

	TEST_VERIFY(m_context.number1 == 0x10000);
	TEST_VERIFY(m_context.number2 == 0x10);
	TEST_VERIFY(m_context.number3 == 0);
	TEST_VERIFY(m_context.number4 == 0xFFFF8000);
	TEST_VERIFY(m_context.number5 == 1);
}

void CCompareTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		//number1 = ((number1 >> number2) < -1) << number2

		jitter.PushRel(offsetof(CONTEXT, number1));
		jitter.PushRel(offsetof(CONTEXT, number2));
		jitter.Sra();

		jitter.PushTop();
		jitter.PullRel(offsetof(CONTEXT, number4));

		jitter.PushCst(0xFFFFFFFF);
		jitter.Cmp(Jitter::CONDITION_LT);

		jitter.PushTop();
		jitter.PullRel(offsetof(CONTEXT, number5));

		jitter.PushRel(offsetof(CONTEXT, number2));
		jitter.Shl();
		jitter.PullRel(offsetof(CONTEXT, number1));

		//number3 = number1 != number3

		jitter.PushRel(offsetof(CONTEXT, number1));
		jitter.PushRel(offsetof(CONTEXT, number3));
		jitter.Cmp(Jitter::CONDITION_NE);

		jitter.PullRel(offsetof(CONTEXT, number3));
	}
	jitter.End();

	m_function = CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}
