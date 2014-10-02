#include "SimpleMdTest.h"
#include "MemStream.h"

void CSimpleMdTest::Run()
{
	memset(&m_context, 0, sizeof(m_context));

	for(unsigned int i = 0; i < 4; i++)
	{
		m_context.op1[i] = i * 0x10000;
		m_context.op2[i] = i;
	}

	m_function(&m_context);

	for(unsigned int i = 0; i < 4; i++)
	{
		TEST_VERIFY(m_context.result[i] == m_context.op1[i] + m_context.op2[i]);
	}
}

void CSimpleMdTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		jitter.MD_PushRel(offsetof(CONTEXT, op1));
		jitter.MD_PushRel(offsetof(CONTEXT, op2));
		jitter.MD_AddW();
		jitter.MD_PullRel(offsetof(CONTEXT, result));
	}
	jitter.End();

	m_function = CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}
