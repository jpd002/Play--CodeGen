#include "Merge64Test.h"
#include "MemStream.h"

#define CONSTANT_1		(0xEEEEFFFFULL)
#define CONSTANT_2		(0x22221111ULL)
#define MERGED_CONSTANT	((CONSTANT_2 << 32) | CONSTANT_1)

void CMerge64Test::Run()
{
	memset(&m_context, 0, sizeof(m_context));

	m_context.op1 = CONSTANT_1;
	m_context.op2 = CONSTANT_2;

	m_function(&m_context);

	TEST_VERIFY(m_context.resultMemMem == MERGED_CONSTANT);
	TEST_VERIFY(m_context.resultMemCst == MERGED_CONSTANT);
}

void CMerge64Test::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		jitter.PushRel(offsetof(CONTEXT, op1));
		jitter.PushRel(offsetof(CONTEXT, op2));
		jitter.MergeTo64();
		jitter.PullRel64(offsetof(CONTEXT, resultMemMem));

		jitter.PushCst(CONSTANT_1);
		jitter.PushRel(offsetof(CONTEXT, op2));
		jitter.MergeTo64();
		jitter.PullRel64(offsetof(CONTEXT, resultMemCst));
	}
	jitter.End();

	m_function = CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}
