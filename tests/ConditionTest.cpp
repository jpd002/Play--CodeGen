#include "ConditionTest.h"
#include "MemStream.h"

CConditionTest::CConditionTest(bool useConstant, uint32 value0, uint32 value1)
: m_useConstant(useConstant)
, m_value0(value0)
, m_value1(value1)
{

}

void CConditionTest::Run()
{
	memset(&m_context, 0, sizeof(m_context));

	m_context.value0 = m_value0;
	m_context.value1 = m_value1;

	//initalize result to nonzero to confirm that result is set to zero
	//if "else" is taken
	m_context.resultEq = m_context.resultNe = 0xdeadbeef;
	m_context.resultBl = m_context.resultBe = m_context.resultAe = m_context.resultLt = 0xdeadbeef;
	m_context.resultLe = m_context.resultAb = m_context.resultGt = m_context.resultGe = 0xdeadbeef;

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

void CConditionTest::MakeBeginIfCase(Jitter::CJitter& jitter, Jitter::CONDITION cond, size_t result)
{
	jitter.PushRel(offsetof(CONTEXT, value0));

	if(m_useConstant)
	{
		jitter.PushCst(m_value1);
	}
	else
	{
		jitter.PushRel(offsetof(CONTEXT, value1));
	}

	jitter.BeginIf(cond);
	{
		jitter.PushCst(1);
		jitter.PullRel(result);
	}
	jitter.Else();
	{
		jitter.PushCst(0);
		jitter.PullRel(result);
	}
	jitter.EndIf();
}

void CConditionTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		MakeBeginIfCase(jitter, Jitter::CONDITION_EQ, offsetof(CONTEXT, resultEq));
		MakeBeginIfCase(jitter, Jitter::CONDITION_NE, offsetof(CONTEXT, resultNe));

		MakeBeginIfCase(jitter, Jitter::CONDITION_BL, offsetof(CONTEXT, resultBl));
		MakeBeginIfCase(jitter, Jitter::CONDITION_BE, offsetof(CONTEXT, resultBe));
		MakeBeginIfCase(jitter, Jitter::CONDITION_AB, offsetof(CONTEXT, resultAb));
		MakeBeginIfCase(jitter, Jitter::CONDITION_AE, offsetof(CONTEXT, resultAe));

		MakeBeginIfCase(jitter, Jitter::CONDITION_LT, offsetof(CONTEXT, resultLt));
		MakeBeginIfCase(jitter, Jitter::CONDITION_LE, offsetof(CONTEXT, resultLe));
		MakeBeginIfCase(jitter, Jitter::CONDITION_GT, offsetof(CONTEXT, resultGt));
		MakeBeginIfCase(jitter, Jitter::CONDITION_GE, offsetof(CONTEXT, resultGe));
	}
	jitter.End();

	m_function = CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}
