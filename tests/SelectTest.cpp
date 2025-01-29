#include "SelectTest.h"
#include "MemStream.h"

void CSelectTest::Run()
{
	m_context = {};
	
	m_context.valueFalse = 0xFF00;
	m_context.valueTrue = 0x00FF;
	
	{
		m_context.cmp0 = 1;
		m_context.cmp1 = 2;
		
		m_function(&m_context);
		
		TEST_VERIFY(m_context.result == ((m_context.cmp0 <= m_context.cmp1) ? m_context.valueTrue : m_context.valueFalse));
	}
	
	{
		m_context.cmp0 = 2;
		m_context.cmp1 = 1;
		
		m_function(&m_context);
		
		TEST_VERIFY(m_context.result == ((m_context.cmp0 <= m_context.cmp1) ? m_context.valueTrue : m_context.valueFalse));
	}

}

void CSelectTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		jitter.PushRel(offsetof(CONTEXT, cmp0));
		jitter.PushRel(offsetof(CONTEXT, cmp1));
		jitter.Cmp(Jitter::CONDITION_LE);
		
		jitter.PushRel(offsetof(CONTEXT, valueTrue));
		jitter.PushRel(offsetof(CONTEXT, valueFalse));
		jitter.Select();
		
		jitter.PullRel(offsetof(CONTEXT, result));
	}
	jitter.End();
	
	m_function = FunctionType(codeStream.GetBuffer(), codeStream.GetSize());
}

