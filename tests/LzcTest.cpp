#include "LzcTest.h"
#include "MemStream.h"

void CLzcTest::Run()
{
	memset(&m_context, 0, sizeof(m_context));

	m_context.input0 = 0x00001FFF;	//19 leading zeroes
	m_context.input1 = 0x00000000;	//32 leading zeroes
	m_context.input2 = 0xFF0000FF;	//8 leading ones

	m_function(&m_context);

	//LZC returns the number of leading bits that have the same value minus one
	TEST_VERIFY(m_context.result0 == 18);
	TEST_VERIFY(m_context.result1 == 31);
	TEST_VERIFY(m_context.result2 == 7);
	TEST_VERIFY(m_context.result3 == 31);		//Constant ~0
	TEST_VERIFY(m_context.result4 == 30);		//Constant 1
}

void CLzcTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		jitter.PushRel(offsetof(CONTEXT, input0));
		jitter.Lzc();
		jitter.PullRel(offsetof(CONTEXT, result0));

		jitter.PushRel(offsetof(CONTEXT, input1));
		jitter.Lzc();
		jitter.PullRel(offsetof(CONTEXT, result1));

		jitter.PushRel(offsetof(CONTEXT, input2));
		jitter.Lzc();
		jitter.PullRel(offsetof(CONTEXT, result2));

		jitter.PushCst(~0);
		jitter.Lzc();
		jitter.PullRel(offsetof(CONTEXT, result3));

		jitter.PushCst(1);
		jitter.Lzc();
		jitter.PullRel(offsetof(CONTEXT, result4));
	}
	jitter.End();

	m_function = CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}
