#include "Logic64Test.h"
#include "MemStream.h"

#define CONSTANT_1 (0xEEEEEEEE55555555ULL)
#define CONSTANT_2 (0x22222222CCCCCCCCULL)

void CLogic64Test::Run()
{
	memset(&m_context, 0, sizeof(m_context));

	m_context.op1 = CONSTANT_1;
	m_context.op2 = CONSTANT_2;

	m_function(&m_context);

	TEST_VERIFY(m_context.resultAnd == (CONSTANT_1 & CONSTANT_2));
	TEST_VERIFY(m_context.resultAndZero1 == 0);
	TEST_VERIFY(m_context.resultAndZero2 == 0);
}

void CLogic64Test::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		jitter.PushRel64(offsetof(CONTEXT, op1));
		jitter.PushRel64(offsetof(CONTEXT, op2));
		jitter.And64();
		jitter.PullRel64(offsetof(CONTEXT, resultAnd));

		jitter.PushRel64(offsetof(CONTEXT, op1));
		jitter.PushCst64(0);
		jitter.And64();
		jitter.PullRel64(offsetof(CONTEXT, resultAndZero1));

		jitter.PushCst64(0);
		jitter.PushRel64(offsetof(CONTEXT, op2));
		jitter.And64();
		jitter.PullRel64(offsetof(CONTEXT, resultAndZero2));
	}
	jitter.End();

	m_function = CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}
