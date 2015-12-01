#include "Alu64Test.h"
#include "MemStream.h"

#define CONSTANT_1 (0xFEDCBA9876543210ULL)
#define CONSTANT_2 (0x76543210FEDCBA98ULL)
#define CONSTANT_3 (0xFEDCBA9800000000ULL)
#define CONSTANT_4 (0x0000000076543210ULL)
#define CONSTANT_5 (0xFFFFFFFFFFFFFFFFULL)

void CAlu64Test::Run()
{
	memset(&m_context, 0, sizeof(m_context));

	m_context.value0 = CONSTANT_1;
	m_context.value1 = CONSTANT_2;
	m_context.value2 = CONSTANT_3;
	m_context.value3 = CONSTANT_4;

	m_function(&m_context);

	TEST_VERIFY(m_context.resultAdd0 == (CONSTANT_1 + CONSTANT_2));
	TEST_VERIFY(m_context.resultAdd1 == (CONSTANT_3 + CONSTANT_4));
	TEST_VERIFY(m_context.resultAddCst == (CONSTANT_1 + CONSTANT_5));

	TEST_VERIFY(m_context.resultSub0 == (CONSTANT_1 - CONSTANT_2));
	TEST_VERIFY(m_context.resultSub1 == (CONSTANT_3 - CONSTANT_4));
	TEST_VERIFY(m_context.resultSubCst0 == (CONSTANT_5 - CONSTANT_1));
	TEST_VERIFY(m_context.resultSubCst1 == (CONSTANT_2 - CONSTANT_3));
}

void CAlu64Test::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		jitter.PushRel64(offsetof(CONTEXT, value0));
		jitter.PushRel64(offsetof(CONTEXT, value1));
		jitter.Add64();
		jitter.PullRel64(offsetof(CONTEXT, resultAdd0));

		jitter.PushRel64(offsetof(CONTEXT, value2));
		jitter.PushRel64(offsetof(CONTEXT, value3));
		jitter.Add64();
		jitter.PullRel64(offsetof(CONTEXT, resultAdd1));

		jitter.PushRel64(offsetof(CONTEXT, value0));
		jitter.PushCst64(CONSTANT_5);
		jitter.Add64();
		jitter.PullRel64(offsetof(CONTEXT, resultAddCst));

		jitter.PushRel64(offsetof(CONTEXT, value0));
		jitter.PushRel64(offsetof(CONTEXT, value1));
		jitter.Sub64();
		jitter.PullRel64(offsetof(CONTEXT, resultSub0));

		jitter.PushRel64(offsetof(CONTEXT, value2));
		jitter.PushRel64(offsetof(CONTEXT, value3));
		jitter.Sub64();
		jitter.PullRel64(offsetof(CONTEXT, resultSub1));

		jitter.PushCst64(CONSTANT_5);
		jitter.PushRel64(offsetof(CONTEXT, value0));
		jitter.Sub64();
		jitter.PullRel64(offsetof(CONTEXT, resultSubCst0));
		
		jitter.PushRel64(offsetof(CONTEXT, value1));
		jitter.PushCst64(CONSTANT_3);
		jitter.Sub64();
		jitter.PullRel64(offsetof(CONTEXT, resultSubCst1));
	}
	jitter.End();

	m_function = CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}
