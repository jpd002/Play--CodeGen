#include "SelectTest.h"
#include "MemStream.h"

constexpr uint32 VALUE_TRUE = 0x00FF;
constexpr uint32 VALUE_FALSE = 0xFF00;

CSelectTest::CSelectTest(bool valueTrueCst, bool valueFalseCst)
    : m_valueTrueCst(valueTrueCst)
    , m_valueFalseCst(valueFalseCst)
{
}

void CSelectTest::Run()
{
	m_context = {};

	m_context.valueFalse = VALUE_FALSE;
	m_context.valueTrue = VALUE_TRUE;

	{
		m_context.cmp0 = 1;
		m_context.cmp1 = 2;
		m_context.overwriteSrc2 = ~0U;
		m_context.overwriteSrc3 = ~0U;
		m_context.externCmp = (m_context.cmp0 <= m_context.cmp1) ? 1 : 0;

		m_function(&m_context);

		TEST_VERIFY(m_context.result == ((m_context.cmp0 <= m_context.cmp1) ? VALUE_TRUE : VALUE_FALSE));
		TEST_VERIFY(m_context.resultExtern == ((m_context.cmp0 <= m_context.cmp1) ? VALUE_TRUE : VALUE_FALSE));
		TEST_VERIFY(m_context.overwriteSrc2 == ((m_context.cmp0 <= m_context.cmp1) ? ~0U : VALUE_FALSE))
		TEST_VERIFY(m_context.overwriteSrc3 == ((m_context.cmp0 <= m_context.cmp1) ? VALUE_TRUE : ~0U));
	}

	{
		m_context.cmp0 = 2;
		m_context.cmp1 = 1;
		m_context.overwriteSrc2 = ~0U;
		m_context.overwriteSrc3 = ~0U;
		m_context.externCmp = (m_context.cmp0 <= m_context.cmp1) ? 1 : 0;

		m_function(&m_context);

		TEST_VERIFY(m_context.result == ((m_context.cmp0 <= m_context.cmp1) ? VALUE_TRUE : VALUE_FALSE));
		TEST_VERIFY(m_context.resultExtern == ((m_context.cmp0 <= m_context.cmp1) ? VALUE_TRUE : VALUE_FALSE));
		TEST_VERIFY(m_context.overwriteSrc2 == ((m_context.cmp0 <= m_context.cmp1) ? ~0U : VALUE_FALSE))
		TEST_VERIFY(m_context.overwriteSrc3 == ((m_context.cmp0 <= m_context.cmp1) ? VALUE_TRUE : ~0U));
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

		if(m_valueTrueCst)
		{
			jitter.PushCst(VALUE_TRUE);
		}
		else
		{
			jitter.PushRel(offsetof(CONTEXT, valueTrue));
		}
		if(m_valueFalseCst)
		{
			jitter.PushCst(VALUE_FALSE);
		}
		else
		{
			jitter.PushRel(offsetof(CONTEXT, valueFalse));
		}
		jitter.Select();

		jitter.PullRel(offsetof(CONTEXT, result));

		{
			jitter.PushRel(offsetof(CONTEXT, cmp0));
			jitter.PushRel(offsetof(CONTEXT, cmp1));
			jitter.Cmp(Jitter::CONDITION_LE);

			jitter.PushRel(offsetof(CONTEXT, overwriteSrc2));
			jitter.PushRel(offsetof(CONTEXT, valueFalse));
			jitter.Select();
			jitter.PullRel(offsetof(CONTEXT, overwriteSrc2));
		}

		{
			jitter.PushRel(offsetof(CONTEXT, cmp0));
			jitter.PushRel(offsetof(CONTEXT, cmp1));
			jitter.Cmp(Jitter::CONDITION_LE);

			jitter.PushRel(offsetof(CONTEXT, valueTrue));
			jitter.PushRel(offsetof(CONTEXT, overwriteSrc3));
			jitter.Select();
			jitter.PullRel(offsetof(CONTEXT, overwriteSrc3));
		}

		{
			jitter.PushRel(offsetof(CONTEXT, externCmp));
			jitter.PushRel(offsetof(CONTEXT, valueTrue));
			jitter.PushRel(offsetof(CONTEXT, valueFalse));
			jitter.Select();
			jitter.PullRel(offsetof(CONTEXT, resultExtern));
		}
	}
	jitter.End();

	m_function = FunctionType(codeStream.GetBuffer(), codeStream.GetSize());
}
