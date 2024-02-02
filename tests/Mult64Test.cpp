#include "Mult64Test.h"
#include "MemStream.h"

static constexpr uint64 VALUE_1 = 0xFFCCFF558632100F;
static constexpr uint64 VALUE_2 = 0x70022000FFCCEEFF;

static constexpr int64 LO_RESULT = VALUE_1 * VALUE_2;

void CMult64Test::Run()
{
	memset(&m_context, 0, sizeof(m_context));

	m_context.relValue1 = VALUE_1;
	m_context.relValue2 = VALUE_2;

	m_function(&m_context);

	TEST_VERIFY(m_context.relLoResult == LO_RESULT)
	TEST_VERIFY(m_context.cstLoResult == LO_RESULT)
}

void CMult64Test::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	//For lower 64-bit of result, signedness doesn't matter

	jitter.Begin();
	{
		//Rel x Rel
		jitter.PushRel64(offsetof(CONTEXT, relValue1));
		jitter.PushRel64(offsetof(CONTEXT, relValue2));
		jitter.Mult();
		jitter.PullRel64(offsetof(CONTEXT, relLoResult));

		//Rel x Cst
		jitter.PushRel64(offsetof(CONTEXT, relValue1));
		jitter.PushCst64(VALUE_2);
		jitter.Mult();
		jitter.PullRel64(offsetof(CONTEXT, cstLoResult));
	}
	jitter.End();

	m_function = FunctionType(codeStream.GetBuffer(), codeStream.GetSize());
}
