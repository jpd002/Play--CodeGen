#include "MdMemAccessTest.h"
#include "MemStream.h"

void CMdMemAccessTest::Run()
{
	memset(&m_context, 0, sizeof(m_context));

	memset(m_memory, 0x20, sizeof(m_memory));
	m_context.array = m_memory;

	for(unsigned int i = 0; i < 4; i++)
	{
		m_context.op[i] = i * 0x10000;
	}

	m_function(&m_context);

	TEST_VERIFY(m_context.result[0] == 0x20202020);
	TEST_VERIFY(m_context.result[1] == 0x20202020);
	TEST_VERIFY(m_context.result[2] == 0x20202020);
	TEST_VERIFY(m_context.result[3] == 0x20202020);

	TEST_VERIFY(m_memory[4] == 0x00000);
	TEST_VERIFY(m_memory[5] == 0x10000);
	TEST_VERIFY(m_memory[6] == 0x20000);
	TEST_VERIFY(m_memory[7] == 0x30000);
}

void CMdMemAccessTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		jitter.PushRelRef(offsetof(CONTEXT, array));
		jitter.MD_LoadFromRef();
		jitter.MD_PullRel(offsetof(CONTEXT, result));

		jitter.PushRelRef(offsetof(CONTEXT, array));
		jitter.PushCst(0x10);
		jitter.AddRef();
		jitter.MD_PushRel(offsetof(CONTEXT, op));
		jitter.MD_StoreAtRef();
	}
	jitter.End();

	m_function = CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}
