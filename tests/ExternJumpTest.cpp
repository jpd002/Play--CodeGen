#include "ExternJumpTest.h"
#include "MemStream.h"
#include "offsetof_def.h"

#define TEST_VALUE 2

void CExternJumpTest::Compile(Jitter::CJitter& jitter)
{
	//Build target function
	{
		Framework::CMemStream codeStream;
		jitter.SetStream(&codeStream);

		jitter.Begin();
		{
			jitter.PushCst(TEST_VALUE);
			jitter.PullRel(offsetof(CONTEXT, value));
		}
		jitter.End();

		m_targetFunction = CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
	}

	//Build source function
	{
		Framework::CMemStream codeStream;
		jitter.SetStream(&codeStream);

		jitter.Begin();
		{
			jitter.JumpTo(m_targetFunction.GetCode());
		}
		jitter.End();

		m_sourceFunction = CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
	}
}

void CExternJumpTest::Run()
{
	CONTEXT context;
	m_sourceFunction(&context);
	TEST_VERIFY(context.value == TEST_VALUE);
}
