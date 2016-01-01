#include "MemAccessTest.h"
#include "MemStream.h"

#define CONSTANT_1	(0xFFCC8844)
#define CONSTANT_2	(0xDEADBEEF)

#define MEMORY_IDX_0 (1)

#define ARRAY_IDX_0	(5)
#define ARRAY_IDX_1	(6)

void CMemAccessTest::Run()
{
	memset(&m_context, 0, sizeof(m_context));
	memset(&m_memory, 0x80, sizeof(m_memory));

	m_context.offset = MEMORY_IDX_0 * sizeof(uint32);
	m_context.memory = m_memory;
	m_context.array0[ARRAY_IDX_1] = CONSTANT_1;

	m_function(&m_context);

	TEST_VERIFY(m_memory[MEMORY_IDX_0] == CONSTANT_1);
	TEST_VERIFY(m_context.result0 == 0x80808080);
	TEST_VERIFY(m_context.result1 == CONSTANT_1);
	TEST_VERIFY(m_context.array0[ARRAY_IDX_0] == CONSTANT_2);
}

void CMemAccessTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		//Store test
		jitter.PushRelRef(offsetof(CONTEXT, memory));
		jitter.PushRel(offsetof(CONTEXT, offset));
		jitter.AddRef();

		jitter.PushCst(CONSTANT_1);
		jitter.StoreAtRef();

		//Read test
		jitter.PushRelRef(offsetof(CONTEXT, memory));
		jitter.PushCst(0x08);
		jitter.AddRef();

		jitter.LoadFromRef();
		jitter.PullRel(offsetof(CONTEXT, result0));

		//Write array test
		jitter.PushRelAddrRef(offsetof(CONTEXT, array0));
		jitter.PushCst(ARRAY_IDX_0 * 4);
		jitter.AddRef();

		jitter.PushCst(CONSTANT_2);
		jitter.StoreAtRef();

		//Read array test
		jitter.PushRelAddrRef(offsetof(CONTEXT, array0));
		jitter.PushCst(ARRAY_IDX_1 * 4);
		jitter.AddRef();

		jitter.LoadFromRef();
		jitter.PullRel(offsetof(CONTEXT, result1));
	}
	jitter.End();

	m_function = CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}
