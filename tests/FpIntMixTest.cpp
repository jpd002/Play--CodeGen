#include "FpIntMixTest.h"
#include "MemStream.h"

CFpIntMixTest::CFpIntMixTest()
{

}

CFpIntMixTest::~CFpIntMixTest()
{

}

void CFpIntMixTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		//number1 = 100.f
		jitter.PushCst(0x42C80000);
		jitter.PullRel(offsetof(CONTEXT, number1));

		//number1 = number1 * number2
		jitter.FP_PushSingle(offsetof(CONTEXT, number1));
		jitter.FP_PushSingle(offsetof(CONTEXT, number2));
		jitter.FP_Mul();
		jitter.FP_PullSingle(offsetof(CONTEXT, number1));

		//number1 = toInt(number1)
		jitter.FP_PushSingle(offsetof(CONTEXT, number1));
		jitter.FP_PullWordTruncate(offsetof(CONTEXT, number1));

		//result = number1
		jitter.PushRel(offsetof(CONTEXT, number1));
		jitter.PullRel(offsetof(CONTEXT, result));
	}
	jitter.End();

	m_function = CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}

void CFpIntMixTest::Run()
{
	memset(&m_context, 0, sizeof(CONTEXT));
	m_context.number2 = 2.0;
	m_function(&m_context);
	TEST_VERIFY(*reinterpret_cast<uint32*>(&m_context.number1)	== 200);
	TEST_VERIFY(m_context.number2								==   2);
	TEST_VERIFY(m_context.result								== 200);
}
