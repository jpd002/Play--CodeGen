#include "CompareTest2.h"
#include "MemStream.h"

CCompareTest2::CCompareTest2(bool useConstant0, bool useConstant1, uint32 value0, uint32 value1)
    : m_useConstant0(useConstant0)
    , m_useConstant1(useConstant1)
    , m_value0(value0)
    , m_value1(value1)
{
}

void CCompareTest2::Run()
{
	m_context = {};
	m_context.value0 = m_value0;
	m_context.value1 = m_value1;

	m_function(&m_context);

	uint32 resultEq = (m_value0 == m_value1) ? 1 : 0;
	uint32 resultNe = (m_value0 != m_value1) ? 1 : 0;
	uint32 resultBl = static_cast<uint32>(m_value0) < static_cast<uint32>(m_value1) ? 1 : 0;
	uint32 resultBe = static_cast<uint32>(m_value0) <= static_cast<uint32>(m_value1) ? 1 : 0;
	uint32 resultAb = static_cast<uint32>(m_value0) > static_cast<uint32>(m_value1) ? 1 : 0;
	uint32 resultAe = static_cast<uint32>(m_value0) >= static_cast<uint32>(m_value1) ? 1 : 0;
	uint32 resultLt = static_cast<int32>(m_value0) < static_cast<int32>(m_value1) ? 1 : 0;
	uint32 resultLe = static_cast<int32>(m_value0) <= static_cast<int32>(m_value1) ? 1 : 0;
	uint32 resultGt = static_cast<int32>(m_value0) > static_cast<int32>(m_value1) ? 1 : 0;
	uint32 resultGe = static_cast<int32>(m_value0) >= static_cast<int32>(m_value1) ? 1 : 0;

	TEST_VERIFY(m_context.resultEq == resultEq);
	TEST_VERIFY(m_context.resultNe == resultNe);
	TEST_VERIFY(m_context.resultBl == resultBl);
	TEST_VERIFY(m_context.resultBe == resultBe);
	TEST_VERIFY(m_context.resultAb == resultAb);
	TEST_VERIFY(m_context.resultAe == resultAe);
	TEST_VERIFY(m_context.resultLt == resultLt);
	TEST_VERIFY(m_context.resultLe == resultLe);
	TEST_VERIFY(m_context.resultGt == resultGt);
	TEST_VERIFY(m_context.resultGe == resultGe);
}

void CCompareTest2::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	auto emitTest =
	    [&](Jitter::CONDITION condition, size_t output) {
		    m_useConstant0 ? jitter.PushCst(m_value0) : jitter.PushRel(offsetof(CONTEXT, value0));
		    m_useConstant1 ? jitter.PushCst(m_value1) : jitter.PushRel(offsetof(CONTEXT, value1));
		    jitter.Cmp(condition);
		    jitter.PullRel(output);
	    };

	jitter.Begin();
	{
		emitTest(Jitter::CONDITION_EQ, offsetof(CONTEXT, resultEq));
		emitTest(Jitter::CONDITION_NE, offsetof(CONTEXT, resultNe));
		emitTest(Jitter::CONDITION_BL, offsetof(CONTEXT, resultBl));
		emitTest(Jitter::CONDITION_BE, offsetof(CONTEXT, resultBe));
		emitTest(Jitter::CONDITION_AB, offsetof(CONTEXT, resultAb));
		emitTest(Jitter::CONDITION_AE, offsetof(CONTEXT, resultAe));
		emitTest(Jitter::CONDITION_LT, offsetof(CONTEXT, resultLt));
		emitTest(Jitter::CONDITION_LE, offsetof(CONTEXT, resultLe));
		emitTest(Jitter::CONDITION_GT, offsetof(CONTEXT, resultGt));
		emitTest(Jitter::CONDITION_GE, offsetof(CONTEXT, resultGe));
	}
	jitter.End();

	m_function = FunctionType(codeStream.GetBuffer(), codeStream.GetSize());
}
