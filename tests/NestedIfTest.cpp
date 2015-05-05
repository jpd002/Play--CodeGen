#include "NestedIfTest.h"
#include "MemStream.h"
#include "offsetof_def.h"

void CNestedIfTest::Run()
{
	{
		memset(&m_context, 0, sizeof(m_context));
		m_context.input = 0;

		m_function(&m_context);

		TEST_VERIFY(m_context.input == 0);
		TEST_VERIFY(m_context.result == 0);
	}

	{
		memset(&m_context, 0, sizeof(m_context));
		m_context.input = 1;

		m_function(&m_context);

		TEST_VERIFY(m_context.input == 1);
		TEST_VERIFY(m_context.result == 1);
	}

	{
		memset(&m_context, 0, sizeof(m_context));
		m_context.input = 2;

		m_function(&m_context);

		TEST_VERIFY(m_context.input == 2);
		TEST_VERIFY(m_context.result == 2);
	}
}

void CNestedIfTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		jitter.PushCst(0);
		jitter.PullRel(offsetof(CONTEXT, result));

		jitter.PushRel(offsetof(CONTEXT, input));
		jitter.PushCst(0);

		jitter.BeginIf(Jitter::CONDITION_NE);
		{
			jitter.PushCst(1);
			jitter.PullRel(offsetof(CONTEXT, result));

			jitter.PushRel(offsetof(CONTEXT, input));
			jitter.PushCst(1);

			jitter.BeginIf(Jitter::CONDITION_NE);
			{
				jitter.PushCst(2);
				jitter.PullRel(offsetof(CONTEXT, result));
			}
			jitter.EndIf();
		}
		jitter.EndIf();
	}
	jitter.End();

	m_function = CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}
