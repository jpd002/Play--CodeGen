#include "MemAccessRefTest.h"
#include "MemStream.h"

#define LOAD_IDX (7)
#define NULLCHECK_IDX (1)

#define CONSTANT_1 (0x01234567)

void CMemAccessRefTest::Run()
{
	memset(&m_context, 0, sizeof(m_context));
	for(uint32 i = 0; i < MEMORY_SIZE; i++)
	{
		m_memory[i] = nullptr;
	}
	m_memory[LOAD_IDX] = &m_context.readValue;
	m_context.readValue = CONSTANT_1;

	m_context.memory = m_memory;

	m_function(&m_context);

	TEST_VERIFY(m_context.readValueResult == CONSTANT_1);
	TEST_VERIFY(m_context.nullCheck0 != 0);
	TEST_VERIFY(m_context.nullCheck1 == 0);
}

void CMemAccessRefTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		//Read ref test
		{
			jitter.PushRelRef(offsetof(CONTEXT, memory));
			jitter.PushCst(LOAD_IDX * sizeof(void*));
			jitter.AddRef();
			jitter.LoadRefFromRef();

			jitter.LoadFromRef();
			jitter.PullRel(offsetof(CONTEXT, readValueResult));
		}

		//Is ref null test (null ptr)
		{
			jitter.PushRelRef(offsetof(CONTEXT, memory));
			jitter.PushCst(NULLCHECK_IDX * sizeof(void*));
			jitter.AddRef();
			jitter.LoadRefFromRef();

			jitter.IsRefNull();
			jitter.PullRel(offsetof(CONTEXT, nullCheck0));
		}

		//Is ref null test (non-null ptr)
		{
			jitter.PushRelRef(offsetof(CONTEXT, memory));
			jitter.PushCst(LOAD_IDX * sizeof(void*));
			jitter.AddRef();
			jitter.LoadRefFromRef();

			jitter.IsRefNull();
			jitter.PullRel(offsetof(CONTEXT, nullCheck1));
		}
	}
	jitter.End();

	m_function = CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}
