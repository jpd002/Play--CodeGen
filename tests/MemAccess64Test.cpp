#include "MemAccess64Test.h"
#include "MemStream.h"

#define CONSTANT_1	(0x0123456789ABCDEF)
#define CONSTANT_2	(0xFEDCBA9876543210)

#define STORE_IDX_0 (1)
#define STORE_IDX_1 (6)
#define LOAD_IDX (7)

void CMemAccess64Test::Run()
{
	memset(&m_context, 0, sizeof(m_context));
	memset(&m_memory, 0x80, sizeof(m_memory));

	m_context.writeValue = CONSTANT_1;
	m_context.memory = m_memory;

	m_function(&m_context);

	TEST_VERIFY(m_memory[STORE_IDX_0] == CONSTANT_1);
	TEST_VERIFY(m_memory[STORE_IDX_1] == CONSTANT_2);
	TEST_VERIFY(m_context.readValue == 0x8080808080808080);
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
			jitter.StoreAtRef();
		}

		//Store test 64 (cst)
		{
			jitter.PushRelRef(offsetof(CONTEXT, memory));
			jitter.PushCst(STORE_IDX_1 * sizeof(uint64));
			jitter.AddRef();

			jitter.PushCst64(CONSTANT_2);
			jitter.StoreAtRef();
		}

		//Read test 64
		{
			jitter.PushRelRef(offsetof(CONTEXT, memory));
			jitter.PushCst(LOAD_IDX * sizeof(uint64));
			jitter.AddRef();

			jitter.Load64FromRef();
			jitter.PullRel64(offsetof(CONTEXT, readValue));
		}
	}
	jitter.End();

	m_function = CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}
