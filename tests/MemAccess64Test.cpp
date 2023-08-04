#include "MemAccess64Test.h"
#include "MemStream.h"

#define CONSTANT_1	(0x0123456789ABCDEF)
#define CONSTANT_2	(0xFFFFFFFF87654321) //Sign-extendable constant value, to test a specific x86 addressing mode
#define CONSTANT_3	(0x5588558855885588)
#define CONSTANT_4	(0x4422332244009922)
#define CONSTANT_5	(0x8786738190980123)

#define STORE_IDX_0 (1)
#define STORE_IDX_1 (2)
#define STORE_IDX_2 (3)
#define STORE_IDX_3 (4)
#define LOAD_IDX_0 (5)
#define LOAD_IDX_1 (6)

void CMemAccess64Test::Run()
{
	memset(&m_context, 0, sizeof(m_context));
	memset(&m_memory, 0x80, sizeof(m_memory));

	m_memory[LOAD_IDX_0] = CONSTANT_3;
	m_memory[LOAD_IDX_1] = CONSTANT_5;

	m_context.writeValue = CONSTANT_1;
	m_context.memory = m_memory;

	m_function(&m_context);

	TEST_VERIFY(m_memory[STORE_IDX_0] == CONSTANT_1);
	TEST_VERIFY(m_memory[STORE_IDX_1] == CONSTANT_2);
	TEST_VERIFY(m_memory[STORE_IDX_2] == CONSTANT_1);
	TEST_VERIFY(m_memory[STORE_IDX_3] == CONSTANT_4);
	TEST_VERIFY(m_context.readValue == CONSTANT_3);
	TEST_VERIFY(m_context.readValueIdx == CONSTANT_5);
}

void CMemAccess64Test::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		//Store test 64
		{
			jitter.PushRelRef(offsetof(CONTEXT, memory));
			jitter.PushCst(STORE_IDX_0 * sizeof(uint64));
			jitter.AddRef();

			jitter.PushRel64(offsetof(CONTEXT, writeValue));
			jitter.Store64AtRef();
		}

		//Store test 64 (cst)
		{
			jitter.PushRelRef(offsetof(CONTEXT, memory));
			jitter.PushCst(STORE_IDX_1 * sizeof(uint64));
			jitter.AddRef();

			jitter.PushCst64(CONSTANT_2);
			jitter.Store64AtRef();
		}

		//Store test 64 (indexed)
		{
			jitter.PushRelRef(offsetof(CONTEXT, memory));
			jitter.PushCst(STORE_IDX_2 * sizeof(uint64));

			jitter.PushRel64(offsetof(CONTEXT, writeValue));
			jitter.Store64AtRefIdx(1);
		}

		//Store test 64 (indexed, constant)
		{
			jitter.PushRelRef(offsetof(CONTEXT, memory));
			jitter.PushCst(STORE_IDX_3 * sizeof(uint64));
			jitter.PushCst64(CONSTANT_4);
			jitter.Store64AtRefIdx(1);
		}

		//Read test 64
		{
			jitter.PushRelRef(offsetof(CONTEXT, memory));
			jitter.PushCst(LOAD_IDX_0 * sizeof(uint64));
			jitter.AddRef();

			jitter.Load64FromRef();
			jitter.PullRel64(offsetof(CONTEXT, readValue));
		}

		//Read test 64 (indexed)
		{
			jitter.PushRelRef(offsetof(CONTEXT, memory));
			jitter.PushCst(LOAD_IDX_1 * sizeof(uint64));

			jitter.Load64FromRefIdx(1);
			jitter.PullRel64(offsetof(CONTEXT, readValueIdx));
		}
	}
	jitter.End();

	m_function = FunctionType(codeStream.GetBuffer(), codeStream.GetSize());
}
