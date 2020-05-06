#include "MemAccessIdxTest.h"
#include "MemStream.h"

#define MEMORY_IDX_0 (1)
#define MEMORY_IDX_1 (8)
#define MEMORY_IDX_2 (9)
#define MEMORY_IDX_3 (13)
#define MEMORY_IDX_4 (15)

#define MEMORY_IDX_0_VALUE (0xFFCC8844)
#define MEMORY_IDX_1_VALUE (0x80808080)
#define MEMORY_IDX_2_VALUE (0xC5C5C5C5)
#define MEMORY_IDX_3_VALUE (0x01234567)
#define MEMORY_IDX_4_VALUE (0x89ABCDEF)

void CMemAccessIdxTest::Run()
{
	memset(&m_context, 0, sizeof(m_context));
	memset(&m_memory, 0xFF, sizeof(m_memory));

	m_memory[MEMORY_IDX_0] = MEMORY_IDX_0_VALUE;
	m_memory[MEMORY_IDX_1] = MEMORY_IDX_1_VALUE;

	m_context.memory = m_memory;
	m_context.index0 = MEMORY_IDX_0 * sizeof(UnitType);
	m_context.index2 = MEMORY_IDX_2;
	m_context.index4Value = MEMORY_IDX_4_VALUE;

	m_function(&m_context);

	TEST_VERIFY(m_memory[MEMORY_IDX_1] == MEMORY_IDX_1_VALUE);
	TEST_VERIFY(m_memory[MEMORY_IDX_2] == MEMORY_IDX_2_VALUE);
	TEST_VERIFY(m_memory[MEMORY_IDX_3] == MEMORY_IDX_3_VALUE);
	TEST_VERIFY(m_memory[MEMORY_IDX_4] == MEMORY_IDX_4_VALUE);
	TEST_VERIFY(m_context.index0Value == MEMORY_IDX_0_VALUE);
	TEST_VERIFY(m_context.index1Value == MEMORY_IDX_1_VALUE);
}

void CMemAccessIdxTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		//Store test (indexed * 4)
		{
			jitter.PushRelRef(offsetof(CONTEXT, memory));
			jitter.PushRel(offsetof(CONTEXT, index2));
			jitter.PushCst(MEMORY_IDX_2_VALUE);
			jitter.StoreAtRefIdx4();
		}

		//Store test (constant indexed)
		{
			jitter.PushRelRef(offsetof(CONTEXT, memory));
			jitter.PushCst(MEMORY_IDX_3 * sizeof(UnitType));
			jitter.PushCst(MEMORY_IDX_3_VALUE);
			jitter.StoreAtRefIdx();
		}

		//Store test (constant indexed)
		{
			jitter.PushRelRef(offsetof(CONTEXT, memory));
			jitter.PushCst(MEMORY_IDX_4 * sizeof(UnitType));
			jitter.PushRel(offsetof(CONTEXT, index4Value));
			jitter.StoreAtRefIdx();
		}

		//Read test (indexed)
		{
			jitter.PushRelRef(offsetof(CONTEXT, memory));
			jitter.PushRel(offsetof(CONTEXT, index0));
			jitter.LoadFromRefIdx();
			jitter.PullRel(offsetof(CONTEXT, index0Value));
		}

		//Read test (constant indexed)
		{
			jitter.PushRelRef(offsetof(CONTEXT, memory));
			jitter.PushCst(MEMORY_IDX_1 * sizeof(UnitType));
			jitter.LoadFromRefIdx();
			jitter.PullRel(offsetof(CONTEXT, index1Value));
		}
	}
	jitter.End();

	m_function = CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}
