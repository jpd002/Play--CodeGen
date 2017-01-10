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
	TEST_VERIFY(m_context.nullCheck2 != 0);
	TEST_VERIFY(m_context.nullCheck3 == 0);
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

		EmitNullTest(jitter, NULLCHECK_IDX, offsetof(CONTEXT, nullCheck0));
		EmitNullTest(jitter, LOAD_IDX, offsetof(CONTEXT, nullCheck1));
		EmitNullComparison(jitter, NULLCHECK_IDX, offsetof(CONTEXT, nullCheck2));
		EmitNullComparison(jitter, LOAD_IDX, offsetof(CONTEXT, nullCheck3));
	}
	jitter.End();

	m_function = CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}

void CMemAccessRefTest::EmitNullTest(Jitter::CJitter& jitter, uint32 index, size_t resultOffset)
{
	jitter.PushRelRef(offsetof(CONTEXT, memory));
	jitter.PushCst(index * sizeof(void*));
	jitter.AddRef();
	jitter.LoadRefFromRef();

	jitter.IsRefNull();
	jitter.PullRel(resultOffset);
}

void CMemAccessRefTest::EmitNullComparison(Jitter::CJitter& jitter, uint32 index, size_t resultOffset)
{
	jitter.PushRelRef(offsetof(CONTEXT, memory));
	jitter.PushCst(index * sizeof(void*));
	jitter.AddRef();
	jitter.LoadRefFromRef();

	jitter.PushCst(0);
	jitter.BeginIf(Jitter::CONDITION_EQ);
	{
		jitter.PushCst(1);
		jitter.PullRel(resultOffset);
	}
	jitter.EndIf();
}
