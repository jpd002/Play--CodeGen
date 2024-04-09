#include "FpIntMixTest.h"
#include "MemStream.h"

void CFpIntMixTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		//number1 = 100.f
		jitter.PushCst(0x42C80000);
		jitter.PullRel(offsetof(CONTEXT, number1));

		//number2 = toFloat(multiplier)
		jitter.FP_PushRel32(offsetof(CONTEXT, multiplier));
		jitter.FP_ToSingleI32();
		jitter.FP_PullRel32(offsetof(CONTEXT, number2));

		//number1 = number1 * number2
		jitter.FP_PushRel32(offsetof(CONTEXT, number1));
		jitter.FP_PushRel32(offsetof(CONTEXT, number2));
		jitter.FP_MulS();
		jitter.FP_PullRel32(offsetof(CONTEXT, number1));

		//number1 = toInt(number1)
		jitter.FP_PushRel32(offsetof(CONTEXT, number1));
		jitter.FP_ToInt32TruncateS();
		jitter.FP_PullRel32(offsetof(CONTEXT, number1));

		//result = number1
		jitter.PushRel(offsetof(CONTEXT, number1));
		jitter.PullRel(offsetof(CONTEXT, result1));

		//result2 = toInt(number3)
		jitter.FP_PushRel32(offsetof(CONTEXT, number3));
		jitter.FP_ToInt32TruncateS();
		jitter.FP_PullRel32(offsetof(CONTEXT, result2));

		//number4 = toDouble(addend)
		jitter.FP_PushRel64(offsetof(CONTEXT, addend));
		jitter.FP_ToDoubleI64();
		jitter.FP_PullRel64(offsetof(CONTEXT, number4));

		//result3 = toInt32(number3)
		jitter.FP_PushRel64(offsetof(CONTEXT, number5));
		jitter.FP_ToInt32TruncateD();
		jitter.FP_PullRel32(offsetof(CONTEXT, result3));
	}
	jitter.End();

	m_function = FunctionType(codeStream.GetBuffer(), codeStream.GetSize());
}

void CFpIntMixTest::Run()
{
	static constexpr uint64 addend = 1ULL << 42;
	m_context = {};
	m_context.multiplier = 2;
	m_context.addend = addend;
	m_context.number3 = -1.75f;
	m_context.number5 = 1024.73142;
	m_function(&m_context);
	TEST_VERIFY(m_context.multiplier							==      2);
	TEST_VERIFY(m_context.addend								== addend);
	TEST_VERIFY(*reinterpret_cast<uint32*>(&m_context.number1)	==    200);
	TEST_VERIFY(m_context.number2								==   2.0f);
	TEST_VERIFY(m_context.result1								==    200);
	TEST_VERIFY(m_context.result2								==     -1);
	TEST_VERIFY(m_context.number4								== static_cast<double>(addend));
	TEST_VERIFY(m_context.result3								==   1024);
}
