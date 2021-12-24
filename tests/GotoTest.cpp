#include "GotoTest.h"
#include "MemStream.h"
#include "offsetof_def.h"

void CGotoTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		auto endOfBlockLabel = jitter.CreateLabel();

		jitter.PushRel(offsetof(CONTEXT, condition));
		jitter.PushCst(0);

		jitter.BeginIf(Jitter::CONDITION_NE);
		{
			jitter.PushCst(RESULT_IF);
			jitter.PullRel(offsetof(CONTEXT, result));
		}
		jitter.Else();
		{
			jitter.PushCst(RESULT_ELSE);
			jitter.PullRel(offsetof(CONTEXT, result));
			jitter.Goto(endOfBlockLabel);
		}
		jitter.EndIf();

		jitter.PushCst(RESULT_OUTSIDE);
		jitter.PullRel(offsetof(CONTEXT, result));

		jitter.MarkLabel(endOfBlockLabel);
	}
	jitter.End();

	m_function = CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}

void CGotoTest::Run()
{
	memset(&m_context, 0, sizeof(CONTEXT));
	m_function(&m_context);
	TEST_VERIFY(m_context.result == RESULT_ELSE);
}
