#include "MdShiftTest.h"
#include "MemStream.h"

CMdShiftTest::CMdShiftTest(uint32 shiftAmount)
: m_shiftAmount(shiftAmount)
{

}

CMdShiftTest::~CMdShiftTest()
{

}

void CMdShiftTest::Run()
{
	memset(&m_context, 0, sizeof(m_context));

	for(unsigned int i = 0; i < 16; i++)
	{
		m_context.value[i] = (i << 4);
	}

	m_function(&m_context);

	for(unsigned int i = 0; i < 8; i++)
	{
		uint16 value = *reinterpret_cast<const uint16*>(m_context.value + i * 2);
		TEST_VERIFY(m_context.resultSllH[i] == static_cast<uint16>(static_cast<uint16>(value) << static_cast<uint16>(m_shiftAmount & 0xF)));
		TEST_VERIFY(m_context.resultSrlH[i] == static_cast<uint16>(static_cast<uint16>(value) >> static_cast<uint16>(m_shiftAmount & 0xF)));
		TEST_VERIFY(m_context.resultSraH[i] == static_cast<uint16>(static_cast<int16>(value) >> static_cast<int16>(m_shiftAmount & 0xF)));
	}

	for(unsigned int i = 0; i < 4; i++)
	{
		uint32 value = *reinterpret_cast<const uint32*>(m_context.value + i * 4);
		TEST_VERIFY(m_context.resultSllW[i] == static_cast<uint32>(static_cast<uint32>(value) << static_cast<uint32>(m_shiftAmount & 0x1F)));
		TEST_VERIFY(m_context.resultSrlW[i] == static_cast<uint32>(static_cast<uint32>(value) >> static_cast<uint32>(m_shiftAmount & 0x1F)));
		TEST_VERIFY(m_context.resultSraW[i] == static_cast<uint32>(static_cast<int32>(value) >> static_cast<int32>(m_shiftAmount & 0x1F)));
	}
}

void CMdShiftTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		jitter.MD_PushRel(offsetof(CONTEXT, value));
		jitter.MD_SllH(m_shiftAmount);
		jitter.MD_PullRel(offsetof(CONTEXT, resultSllH));

		jitter.MD_PushRel(offsetof(CONTEXT, value));
		jitter.MD_SrlH(m_shiftAmount);
		jitter.MD_PullRel(offsetof(CONTEXT, resultSrlH));

		jitter.MD_PushRel(offsetof(CONTEXT, value));
		jitter.MD_SraH(m_shiftAmount);
		jitter.MD_PullRel(offsetof(CONTEXT, resultSraH));

		jitter.MD_PushRel(offsetof(CONTEXT, value));
		jitter.MD_SllW(m_shiftAmount);
		jitter.MD_PullRel(offsetof(CONTEXT, resultSllW));

		jitter.MD_PushRel(offsetof(CONTEXT, value));
		jitter.MD_SrlW(m_shiftAmount);
		jitter.MD_PullRel(offsetof(CONTEXT, resultSrlW));

		jitter.MD_PushRel(offsetof(CONTEXT, value));
		jitter.MD_SraW(m_shiftAmount);
		jitter.MD_PullRel(offsetof(CONTEXT, resultSraW));
	}
	jitter.End();

	m_function = CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}
