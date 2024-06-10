#include "LzcTest.h"
#include "MemStream.h"

void CLzcTest::Run()
{
	memset(&m_context, 0, sizeof(m_context));

	m_context.input0 = 0x00001FFF;	//19 leading zeroes
	m_context.input1 = 0x00000000;	//32 leading zeroes
	m_context.input2 = 0xFF0000FF;	//8 leading ones

	m_context.input64_0 = ~0ULL;
	m_context.input64_1 = 0x00000000000000FF;

	m_function(&m_context);

	//LZC returns the number of leading bits that have the same value minus one
	TEST_VERIFY(m_context.resultLzc0 == 18);
	TEST_VERIFY(m_context.resultLzc1 == 31);
	TEST_VERIFY(m_context.resultLzc2 == 7);
	TEST_VERIFY(m_context.resultLzc3 == 31); //Constant ~0
	TEST_VERIFY(m_context.resultLzc4 == 30); //Constant 1
	TEST_VERIFY(m_context.resultClz0 == 19);
	TEST_VERIFY(m_context.resultClz1 == 0);
	TEST_VERIFY(m_context.resultClz64_0 == 0);
	TEST_VERIFY(m_context.resultClz64_1 == 56);
}

void CLzcTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		jitter.PushRel(offsetof(CONTEXT, input0));
		jitter.Lzc();
		jitter.PullRel(offsetof(CONTEXT, resultLzc0));

		jitter.PushRel(offsetof(CONTEXT, input1));
		jitter.Lzc();
		jitter.PullRel(offsetof(CONTEXT, resultLzc1));

		jitter.PushRel(offsetof(CONTEXT, input2));
		jitter.Lzc();
		jitter.PullRel(offsetof(CONTEXT, resultLzc2));

		jitter.PushCst(~0);
		jitter.Lzc();
		jitter.PullRel(offsetof(CONTEXT, resultLzc3));

		jitter.PushCst(1);
		jitter.Lzc();
		jitter.PullRel(offsetof(CONTEXT, resultLzc4));

		//--------------------------------
		//CLZ

		jitter.PushRel(offsetof(CONTEXT, input0));
		jitter.Clz();
		jitter.PullRel(offsetof(CONTEXT, resultClz0));

		jitter.PushRel(offsetof(CONTEXT, input2));
		jitter.Clz();
		jitter.PullRel(offsetof(CONTEXT, resultClz1));

		//--------------------------------
		//CLZ64

		jitter.PushRel64(offsetof(CONTEXT, input64_0));
		jitter.Clz64();
		jitter.PullRel64(offsetof(CONTEXT, resultClz64_0));

		jitter.PushRel64(offsetof(CONTEXT, input64_1));
		jitter.Clz64();
		jitter.PullRel64(offsetof(CONTEXT, resultClz64_1));
	}
	jitter.End();

	m_function = FunctionType(codeStream.GetBuffer(), codeStream.GetSize());
}
