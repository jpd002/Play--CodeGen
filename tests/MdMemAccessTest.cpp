#include "MdMemAccessTest.h"
#include "MemStream.h"

static constexpr int LOADFROM_INDEX_0 = 0; //Unused
static constexpr int LOADFROM_INDEX_1 = 5;
static constexpr int LOADFROMMASKED_BASE = 0x10;
static constexpr int STOREAT_INDEX_0 = 4;
static constexpr int STOREAT_INDEX_1 = 8;
static constexpr int STOREATMASKED_BASE = 0x10;

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

	memset(m_context.loadMaskedResult, 0xFF, sizeof(m_context.loadMaskedResult));

	for(int i = 0; i < sizeof(m_memory); i++)
	{
		if(i < 0x100)
		{
			reinterpret_cast<uint8*>(m_memory)[i] = i;
		}
		else
		{
			reinterpret_cast<uint8*>(m_memory)[i] = ((i / 4) & 3) | 0xF0;
		}
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

	for(int i = 0; i < MASK_COUNT; i++)
	{
		if((i == 0) || (i == 0xF)) continue;
		const auto& result = m_context.loadMaskedResult[i];
		TEST_VERIFY(result.nV[0] == (((i & 1) == 0) ? (~0U) : (0xF0F0F0F0)));
		TEST_VERIFY(result.nV[1] == (((i & 2) == 0) ? (~0U) : (0xF1F1F1F1)));
		TEST_VERIFY(result.nV[2] == (((i & 4) == 0) ? (~0U) : (0xF2F2F2F2)));
		TEST_VERIFY(result.nV[3] == (((i & 8) == 0) ? (~0U) : (0xF3F3F3F3)));
	}

	TEST_VERIFY(m_memory[STOREAT_INDEX_0].nV[0] == 0x00000);
	TEST_VERIFY(m_memory[STOREAT_INDEX_0].nV[1] == 0x10000);
	TEST_VERIFY(m_memory[STOREAT_INDEX_0].nV[2] == 0x20000);
	TEST_VERIFY(m_memory[STOREAT_INDEX_0].nV[3] == 0x30000);

	TEST_VERIFY(m_memory[STOREAT_INDEX_1].nV[0] == 0x00000);
	TEST_VERIFY(m_memory[STOREAT_INDEX_1].nV[1] == 0x10000);
	TEST_VERIFY(m_memory[STOREAT_INDEX_1].nV[2] == 0x20000);
	TEST_VERIFY(m_memory[STOREAT_INDEX_1].nV[3] == 0x30000);

	for(int i = 0; i < MASK_COUNT; i++)
	{
		if((i == 0) || (i == 0xF)) continue;
		const auto& result = m_memory[STOREATMASKED_BASE + i];
		TEST_VERIFY(result.nV[0] == (((i & 1) == 0) ? (0xF0F0F0F0) : (0x00000)));
		TEST_VERIFY(result.nV[1] == (((i & 2) == 0) ? (0xF1F1F1F1) : (0x10000)));
		TEST_VERIFY(result.nV[2] == (((i & 4) == 0) ? (0xF2F2F2F2) : (0x20000)));
		TEST_VERIFY(result.nV[3] == (((i & 8) == 0) ? (0xF3F3F3F3) : (0x30000)));
	}
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

		for(unsigned int i = 0; i < MASK_COUNT; i++)
		{
			if((i == 0) || (i == 0xF)) continue;
			jitter.PushRelRef(offsetof(CONTEXT, array));
			jitter.PushCst((i + LOADFROMMASKED_BASE) * sizeof(uint128));
			jitter.MD_PushRel(offsetof(CONTEXT, loadMaskedResult[i]));
			jitter.MD_LoadFromRefIdxMasked((i & 1) != 0, (i & 2) != 0, (i & 4) != 0, (i & 8) != 0);
			jitter.MD_PullRel(offsetof(CONTEXT, loadMaskedResult[i]));
		}

		jitter.PushRelRef(offsetof(CONTEXT, array));
		jitter.PushCst(STOREAT_INDEX_0 * sizeof(uint128));
		jitter.AddRef();
		jitter.MD_PushRel(offsetof(CONTEXT, op));
		jitter.MD_StoreAtRef();

		jitter.PushRelRef(offsetof(CONTEXT, array));
		jitter.PushRel(offsetof(CONTEXT, storeAtIdx1));
		jitter.MD_PushRel(offsetof(CONTEXT, op));
		jitter.MD_StoreAtRefIdx(1);

		for(unsigned int i = 0; i < MASK_COUNT; i++)
		{
			if((i == 0) || (i == 0xF)) continue;
			jitter.PushRelRef(offsetof(CONTEXT, array));
			jitter.PushCst((i + STOREATMASKED_BASE) * sizeof(uint128));
			jitter.MD_PushRel(offsetof(CONTEXT, op));
			jitter.MD_StoreAtRefIdxMasked((i & 1) != 0, (i & 2) != 0, (i & 4) != 0, (i & 8) != 0);
		}
	}
	jitter.End();

	m_function = FunctionType(codeStream.GetBuffer(), codeStream.GetSize());
}
