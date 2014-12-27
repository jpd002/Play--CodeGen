#include "Cmp64Test.h"
#include "MemStream.h"

void CCmp64Test::Run()
{
	memset(&m_context, 0, sizeof(m_context));

	m_context.value0 = 0xFEDCBA9876543210ULL;
	m_context.value1 = 0x012389AB4567CDEFULL;

	m_function(&m_context);

	TEST_VERIFY(m_context.resultBl == 0);
	TEST_VERIFY(m_context.resultLt != 0);
}

void CCmp64Test::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		jitter.PushRel64(offsetof(CONTEXT, value0));
		jitter.PushRel64(offsetof(CONTEXT, value1));
		jitter.Cmp64(Jitter::CONDITION_BL);
		jitter.PullRel(offsetof(CONTEXT, resultBl));

		jitter.PushRel64(offsetof(CONTEXT, value0));
		jitter.PushRel64(offsetof(CONTEXT, value1));
		jitter.Cmp64(Jitter::CONDITION_LT);
		jitter.PullRel(offsetof(CONTEXT, resultLt));
	}
	jitter.End();

	m_function = CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}
