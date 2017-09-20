#include "Cmp64Test.h"
#include "MemStream.h"

CCmp64Test::CCmp64Test(bool useConstant0, bool useConstant1, uint64 value0, uint64 value1)
: m_useConstant0(useConstant0)
, m_useConstant1(useConstant1)
, m_value0(value0)
, m_value1(value1)
{

}

void CCmp64Test::Run()
{
	memset(&m_context, 0, sizeof(m_context));

	m_context.value0 = m_value0;
	m_context.value1 = m_value1;

	m_function(&m_context);

	uint32 resultEq = (m_value0 == m_value1) ? 1 : 0;
	uint32 resultNe = (m_value0 != m_value1) ? 1 : 0;
	uint32 resultBl = static_cast<uint64>(m_value0) < static_cast<uint64>(m_value1) ? 1 : 0;
	uint32 resultLt = static_cast<int64>(m_value0) < static_cast<int64>(m_value1) ? 1 : 0;
	uint32 resultLe = static_cast<int64>(m_value0) <= static_cast<int64>(m_value1) ? 1 : 0;
	uint32 resultAb = static_cast<uint64>(m_value0) > static_cast<uint64>(m_value1) ? 1 : 0;
	uint32 resultGt = static_cast<int64>(m_value0) > static_cast<int64>(m_value1) ? 1 : 0;

	TEST_VERIFY(m_context.resultEq == resultEq);
	TEST_VERIFY(m_context.resultNe == resultNe);
	TEST_VERIFY(m_context.resultBl == resultBl);
	TEST_VERIFY(m_context.resultLt == resultLt);
	TEST_VERIFY(m_context.resultLe == resultLe);
	TEST_VERIFY(m_context.resultAb == resultAb);
	TEST_VERIFY(m_context.resultGt == resultGt);
}

void CCmp64Test::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	auto emitTest = 
		[&](Jitter::CONDITION condition, size_t output)
		{
			m_useConstant0 ? jitter.PushCst64(m_value0) : jitter.PushRel64(offsetof(CONTEXT, value0));
			m_useConstant1 ? jitter.PushCst64(m_value1) : jitter.PushRel64(offsetof(CONTEXT, value1));
			jitter.Cmp64(condition);
			jitter.PullRel(output);
		};

	jitter.Begin();
	{
		emitTest(Jitter::CONDITION_EQ, offsetof(CONTEXT, resultEq));
		emitTest(Jitter::CONDITION_NE, offsetof(CONTEXT, resultNe));
		emitTest(Jitter::CONDITION_BL, offsetof(CONTEXT, resultBl));
		emitTest(Jitter::CONDITION_LT, offsetof(CONTEXT, resultLt));
		emitTest(Jitter::CONDITION_LE, offsetof(CONTEXT, resultLe));
		emitTest(Jitter::CONDITION_AB, offsetof(CONTEXT, resultAb));
		emitTest(Jitter::CONDITION_GT, offsetof(CONTEXT, resultGt));
	}
	jitter.End();

	m_function = CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}
