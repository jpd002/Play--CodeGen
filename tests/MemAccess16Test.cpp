#include "MemAccess16Test.h"
#include "MemStream.h"

#define CONSTANT_1	(0xCCCC)
#define CONSTANT_2	(0xBEEF)
#define CONSTANT_3	(0x5555)

#define MEMORY_IDX_0 (1)
#define MEMORY_IDX_1 (8)

#define ARRAY_IDX_0	(5)
#define ARRAY_IDX_1	(6)
#define ARRAY_IDX_2	(7)

void CMemAccess16Test::Run()
{
	memset(&m_context, 0, sizeof(m_context));
	memset(&m_memory, 0x80, sizeof(m_memory));

	m_context.offset = MEMORY_IDX_0 * sizeof(UnitType);
	m_context.memory = m_memory;
	m_context.value = CONSTANT_3;
	m_context.array0[ARRAY_IDX_1] = CONSTANT_1;

	m_function(&m_context);

	TEST_VERIFY(m_memory[MEMORY_IDX_0] == CONSTANT_1);
	TEST_VERIFY(m_context.result0 == 0x8080);
	TEST_VERIFY(m_context.result1 == CONSTANT_1);
	TEST_VERIFY(m_context.array0[ARRAY_IDX_0] == CONSTANT_2);
	TEST_VERIFY(m_context.array0[ARRAY_IDX_2] == CONSTANT_3);
}

void CMemAccess16Test::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		//Store test
		{
			jitter.PushRelRef(offsetof(CONTEXT, memory));
			jitter.PushRel(offsetof(CONTEXT, offset));
			jitter.AddRef();

			jitter.PushCst(CONSTANT_1);
			jitter.Store16AtRef();
		}

		//Read test
		{
			jitter.PushRelRef(offsetof(CONTEXT, memory));
			jitter.PushCst(MEMORY_IDX_1);
			jitter.AddRef();

			jitter.Load16FromRef();
			jitter.PullRel(offsetof(CONTEXT, result0));
		}

		//Write array test (cst)
		{
			jitter.PushRelAddrRef(offsetof(CONTEXT, array0));
			jitter.PushCst(ARRAY_IDX_0 * sizeof(UnitType));
			jitter.AddRef();

			jitter.PushCst(CONSTANT_2);
			jitter.Store16AtRef();
		}

		//Write array test (variable)
		{
			jitter.PushRelAddrRef(offsetof(CONTEXT, array0));
			jitter.PushCst(ARRAY_IDX_2 * sizeof(UnitType));
			jitter.AddRef();

			jitter.PushRel(offsetof(CONTEXT, value));
			jitter.Store16AtRef();
		}

		//Read array test
		{
			jitter.PushRelAddrRef(offsetof(CONTEXT, array0));
			jitter.PushCst(ARRAY_IDX_1 * sizeof(UnitType));
			jitter.AddRef();

			jitter.Load16FromRef();
			jitter.PullRel(offsetof(CONTEXT, result1));
		}
	}
	jitter.End();

	m_function = CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}
