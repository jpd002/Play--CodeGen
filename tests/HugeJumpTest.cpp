#include "HugeJumpTest.h"
#include "MemStream.h"
#include "offsetof_def.h"

void CHugeJumpTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{		
		jitter.PushRel(offsetof(CONTEXT, condition));
		jitter.PushCst(0);

		jitter.BeginIf(Jitter::CONDITION_EQ);
		{
			for(unsigned int i = 2; i < MAX_VARS; i++)
			{
				jitter.PushRel(offsetof(CONTEXT, number[i - 2]));
				jitter.PushRel(offsetof(CONTEXT, number[i - 1]));
				jitter.Add();
				jitter.PullRel(offsetof(CONTEXT, number[i - 0]));
			}
		}
		jitter.Else();
		{
			for(unsigned int i = 2; i < MAX_VARS; i++)
			{
				jitter.PushRel(offsetof(CONTEXT, number[i - 2]));
				jitter.PushRel(offsetof(CONTEXT, number[i - 1]));
				jitter.Add();
				jitter.PullRel(offsetof(CONTEXT, number[i - 0]));
			}
		}
		jitter.EndIf();
	}
	jitter.End();

	m_function = CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}

void CHugeJumpTest::Run()
{
	memset(&m_context, 0, sizeof(CONTEXT));
	m_function(&m_context);
}
