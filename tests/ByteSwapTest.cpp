#include "ByteSwapTest.h"
#include "MemStream.h"
#include "EndianUtils.h"

static constexpr uint32 CONSTANT_0 = 0xDEADBEEF;

void CByteSwapTest::Run()
{
	m_context = {};
	m_context.value0 = CONSTANT_0;

	m_function(&m_context);

	TEST_VERIFY(m_context.value0 == CONSTANT_0);
	TEST_VERIFY(m_context.result0 == __builtin_bswap32(CONSTANT_0));
}

void CByteSwapTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		jitter.PushRel(offsetof(CONTEXT, value0));
		jitter.ByteSwap();
		jitter.PullRel(offsetof(CONTEXT, result0));
	}
	jitter.End();

	m_function = FunctionType(codeStream.GetBuffer(), codeStream.GetSize());
}
