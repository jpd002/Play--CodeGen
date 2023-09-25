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

CMemAccess64Test::CMemAccess64Test(bool useVariableIndices)
	: m_useVariableIndices(useVariableIndices)
{

}

void CMemAccess64Test::Run()
{
	m_context = {};
	m_context.storeIdx0 = STORE_IDX_0 * sizeof(MemoryValueType);
	m_context.storeIdx1 = STORE_IDX_1 * sizeof(MemoryValueType);
	m_context.storeIdx2 = STORE_IDX_2 * sizeof(MemoryValueType);
	m_context.storeIdx3 = STORE_IDX_3 * sizeof(MemoryValueType);
	m_context.loadIdx0 = LOAD_IDX_0 * sizeof(MemoryValueType);
	m_context.loadIdx1 = LOAD_IDX_1 * sizeof(MemoryValueType);

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
#define PUSH_LOAD_IDX(idx) \
	if(m_useVariableIndices) \
	{ \
		jitter.PushRel(offsetof(CONTEXT, loadIdx##idx)); \
	} \
	else \
	{ \
		jitter.PushCst(LOAD_IDX_##idx * sizeof(MemoryValueType)); \
	}

#define PUSH_STORE_IDX(idx) \
	if(m_useVariableIndices) \
	{ \
		jitter.PushRel(offsetof(CONTEXT, storeIdx##idx)); \
	} \
	else \
	{ \
		jitter.PushCst(STORE_IDX_##idx * sizeof(MemoryValueType)); \
	}
	
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		//Store test
		{
			jitter.PushRelRef(offsetof(CONTEXT, memory));
			PUSH_STORE_IDX(0);
			jitter.AddRef();

			jitter.PushRel64(offsetof(CONTEXT, writeValue));
			jitter.Store64AtRef();
		}

		//Store test (cst)
		{
			jitter.PushRelRef(offsetof(CONTEXT, memory));
			PUSH_STORE_IDX(1);
			jitter.AddRef();

			jitter.PushCst64(CONSTANT_2);
			jitter.Store64AtRef();
		}

		//Store test (indexed)
		{
			jitter.PushRelRef(offsetof(CONTEXT, memory));
			PUSH_STORE_IDX(2);

			jitter.PushRel64(offsetof(CONTEXT, writeValue));
			jitter.Store64AtRefIdx(1);
		}

		//Store test (indexed, constant)
		{
			jitter.PushRelRef(offsetof(CONTEXT, memory));
			PUSH_STORE_IDX(3);

			jitter.PushCst64(CONSTANT_4);
			jitter.Store64AtRefIdx(1);
		}

		//Read test
		{
			jitter.PushRelRef(offsetof(CONTEXT, memory));
			PUSH_LOAD_IDX(0);
			jitter.AddRef();

			jitter.Load64FromRef();
			jitter.PullRel64(offsetof(CONTEXT, readValue));
		}

		//Read test (indexed)
		{
			jitter.PushRelRef(offsetof(CONTEXT, memory));
			PUSH_LOAD_IDX(1);

			jitter.Load64FromRefIdx(1);
			jitter.PullRel64(offsetof(CONTEXT, readValueIdx));
		}
	}
	jitter.End();

	m_function = FunctionType(codeStream.GetBuffer(), codeStream.GetSize());
}
