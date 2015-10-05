#include "LogicTest.h"
#include "MemStream.h"

CLogicTest::CLogicTest(uint32 value1, bool constant1, uint32 value2, bool constant2)
: m_value1(value1)
, m_constant1(constant1)
, m_value2(value2)
, m_constant2(constant2)
{

}

void CLogicTest::Run()
{
	memset(&m_context, 0, sizeof(m_context));

	m_context.op1 = m_value1;
	m_context.op2 = m_value2;

	m_function(&m_context);

	TEST_VERIFY(m_context.resultAnd == (m_value1 & m_value2));
	TEST_VERIFY(m_context.resultOr  == (m_value1 | m_value2));
	TEST_VERIFY(m_context.resultXor == (m_value1 ^ m_value2));
}

void CLogicTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		m_constant1 ? jitter.PushCst(m_value1) : jitter.PushRel(offsetof(CONTEXT, op1));
		m_constant2 ? jitter.PushCst(m_value2) : jitter.PushRel(offsetof(CONTEXT, op2));
		jitter.And();
		jitter.PullRel(offsetof(CONTEXT, resultAnd));
		
		m_constant1 ? jitter.PushCst(m_value1) : jitter.PushRel(offsetof(CONTEXT, op1));
		m_constant2 ? jitter.PushCst(m_value2) : jitter.PushRel(offsetof(CONTEXT, op2));
		jitter.Or();
		jitter.PullRel(offsetof(CONTEXT, resultOr));
		
		m_constant1 ? jitter.PushCst(m_value1) : jitter.PushRel(offsetof(CONTEXT, op1));
		m_constant2 ? jitter.PushCst(m_value2) : jitter.PushRel(offsetof(CONTEXT, op2));
		jitter.Xor();
		jitter.PullRel(offsetof(CONTEXT, resultXor));
	}
	jitter.End();

	m_function = CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}
