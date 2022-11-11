#include "ReorderAddTest.h"
#include "MemStream.h"

#define TEST_ADDRESS (0x320000)
#define TEST_DISPLACEMENT (-0x4000)

void CReorderAddTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		jitter.PushRel(offsetof(CONTEXT, address));
		jitter.Srl(12);
		jitter.PushCst(TEST_DISPLACEMENT >> 12);
		jitter.Add();
		jitter.Shl(3);
		jitter.PullRel(offsetof(CONTEXT, result));

		jitter.PushRel(offsetof(CONTEXT, address));
		jitter.PushCst(TEST_DISPLACEMENT);
		jitter.Add();
		jitter.Shl(4);
		jitter.PushCst(TEST_DISPLACEMENT);
		jitter.Add();
		jitter.PullRel(offsetof(CONTEXT, result2));
	}
	jitter.End();

	m_function = CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}

void CReorderAddTest::Run()
{
	m_context = {};
	m_context.address = TEST_ADDRESS;
	m_function(&m_context);
	constexpr uint32 testResult = ((TEST_ADDRESS + TEST_DISPLACEMENT) >> 12) << 3;
	constexpr uint32 testResult2 = ((TEST_ADDRESS + TEST_DISPLACEMENT) << 4) + TEST_DISPLACEMENT;
	TEST_VERIFY(m_context.result == testResult);
	TEST_VERIFY(m_context.result2 == testResult2);
}
