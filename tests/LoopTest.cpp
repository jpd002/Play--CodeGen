#include "LoopTest.h"
#include "MemStream.h"

#define COUNTER_INIT (5)
#define ADD_AMOUNT (0x10)

void CLoopTest::Run()
{
	m_context = {};
	m_context.counter = COUNTER_INIT;
	
	m_function(&m_context);

	TEST_VERIFY(m_context.counter == 0);
	TEST_VERIFY(m_context.total == (COUNTER_INIT * ADD_AMOUNT));
	TEST_VERIFY(m_context.exit == 1);
	TEST_VERIFY(m_context.prevFinal == (COUNTER_INIT - 1));
	TEST_VERIFY(m_context.notDone == (COUNTER_INIT - 1));
}

void CLoopTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		auto loopLabel = jitter.CreateLabel();
		auto finalLabel = jitter.CreateLabel();

		jitter.MarkLabel(loopLabel);
		
		jitter.PushRel(offsetof(CONTEXT, total));
		jitter.PushCst(ADD_AMOUNT);
		jitter.Add();
		jitter.PullRel(offsetof(CONTEXT, total));

		jitter.PushRel(offsetof(CONTEXT, counter));
		jitter.PushCst(1);
		jitter.Sub();
		jitter.PullRel(offsetof(CONTEXT, counter));
		
		jitter.PushRel(offsetof(CONTEXT, counter));
		jitter.PushCst(0);
		jitter.BeginIf(Jitter::CONDITION_NE);
		{
			jitter.PushRel(offsetof(CONTEXT, notDone));
			jitter.PushCst(1);
			jitter.Add();
			jitter.PullRel(offsetof(CONTEXT, notDone));
		}
		jitter.Else();
		{
			jitter.PushCst(1);
			jitter.PullRel(offsetof(CONTEXT, exit));
			jitter.Goto(finalLabel);
		}
		jitter.EndIf();

		jitter.PushRel(offsetof(CONTEXT, prevFinal));
		jitter.PushCst(1);
		jitter.Add();
		jitter.PullRel(offsetof(CONTEXT, prevFinal));

		jitter.MarkLabel(finalLabel);

		jitter.PushRel(offsetof(CONTEXT, exit));
		jitter.PushCst(0);
		jitter.BeginIf(Jitter::CONDITION_EQ);
		{
			jitter.Goto(loopLabel);
		}
		jitter.EndIf();
	}
	jitter.End();

	m_function = CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}
