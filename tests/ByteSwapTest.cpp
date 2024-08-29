#include "ByteSwapTest.h"
#include "MemStream.h"
#include "EndianUtils.h"

static constexpr uint32 CONSTANT_0 = 0xDEADBEEF;
static constexpr uint64 CONSTANT_1 = 0xCAFECAFEBEEFFEED;

void CByteSwapTest::Run()
{
	m_context = {};
	m_context.value0 = CONSTANT_0;
	m_context.value1 = CONSTANT_1;

	m_function(&m_context);

	TEST_VERIFY(m_context.value0 == CONSTANT_0);
	TEST_VERIFY(m_context.value1 == CONSTANT_1);
	TEST_VERIFY(m_context.result0 == __builtin_bswap32(CONSTANT_0));
	TEST_VERIFY(m_context.result1 == __builtin_bswap64(CONSTANT_1));
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

		jitter.PushRel64(offsetof(CONTEXT, value1));
		jitter.ByteSwap64();
		jitter.PullRel64(offsetof(CONTEXT, result1));
	}
	jitter.End();

	m_function = FunctionType(codeStream.GetBuffer(), codeStream.GetSize());
}
