#include "MdMemAccessTest.h"
#include "MemStream.h"

static constexpr int LOADFROM_INDEX_0 = 0; //Unused
static constexpr int LOADFROM_INDEX_1 = 5;
static constexpr int STOREAT_INDEX_0 = 4;
static constexpr int STOREAT_INDEX_1 = 8;

void CMdMemAccessTest::Run()
{
	m_context = {};
	m_context.array = m_memory;
	m_context.loadFromIdx1 = LOADFROM_INDEX_1 * sizeof(uint128);
	m_context.storeAtIdx1 = STOREAT_INDEX_1 * sizeof(uint128);

	for(unsigned int i = 0; i < 4; i++)
	{
		m_context.op.nV[i] = i * 0x10000;
	}

	for(int i = 0; i < sizeof(m_memory); i++)
	{
		reinterpret_cast<uint8*>(m_memory)[i] = i;
	}

	m_function(&m_context);

	TEST_VERIFY(m_context.loadResult0.nV[0] == 0x03020100);
	TEST_VERIFY(m_context.loadResult0.nV[1] == 0x07060504);
	TEST_VERIFY(m_context.loadResult0.nV[2] == 0x0B0A0908);
	TEST_VERIFY(m_context.loadResult0.nV[3] == 0x0F0E0D0C);

	TEST_VERIFY(m_context.loadResult1.nV[0] == 0x53525150);
	TEST_VERIFY(m_context.loadResult1.nV[1] == 0x57565554);
	TEST_VERIFY(m_context.loadResult1.nV[2] == 0x5B5A5958);
	TEST_VERIFY(m_context.loadResult1.nV[3] == 0x5F5E5D5C);

	TEST_VERIFY(m_memory[STOREAT_INDEX_0].nV[0] == 0x00000);
	TEST_VERIFY(m_memory[STOREAT_INDEX_0].nV[1] == 0x10000);
	TEST_VERIFY(m_memory[STOREAT_INDEX_0].nV[2] == 0x20000);
	TEST_VERIFY(m_memory[STOREAT_INDEX_0].nV[3] == 0x30000);

	TEST_VERIFY(m_memory[STOREAT_INDEX_1].nV[0] == 0x00000);
	TEST_VERIFY(m_memory[STOREAT_INDEX_1].nV[1] == 0x10000);
	TEST_VERIFY(m_memory[STOREAT_INDEX_1].nV[2] == 0x20000);
	TEST_VERIFY(m_memory[STOREAT_INDEX_1].nV[3] == 0x30000);
}

void CMdMemAccessTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		jitter.PushRelRef(offsetof(CONTEXT, array));
		jitter.MD_LoadFromRef();
		jitter.MD_PullRel(offsetof(CONTEXT, loadResult0));

		jitter.PushRelRef(offsetof(CONTEXT, array));
		jitter.PushRel(offsetof(CONTEXT, loadFromIdx1));
		jitter.MD_LoadFromRefIdx(1);
		jitter.MD_PullRel(offsetof(CONTEXT, loadResult1));

		jitter.PushRelRef(offsetof(CONTEXT, array));
		jitter.PushCst(STOREAT_INDEX_0 * sizeof(uint128));
		jitter.AddRef();
		jitter.MD_PushRel(offsetof(CONTEXT, op));
		jitter.MD_StoreAtRef();

		jitter.PushRelRef(offsetof(CONTEXT, array));
		jitter.PushRel(offsetof(CONTEXT, storeAtIdx1));
		jitter.MD_PushRel(offsetof(CONTEXT, op));
		jitter.MD_StoreAtRefIdx(1);
	}
	jitter.End();

	m_function = FunctionType(codeStream.GetBuffer(), codeStream.GetSize());
}
