#include "ExternJumpTest.h"
#include "MemStream.h"
#include "offsetof_def.h"

#define TEST_CST_1 0x12
#define TEST_CST_2 0xFF
#define TEST_RESULT_1 (TEST_CST_1 + TEST_CST_2)
#define TEST_RESULT_2 2

static void DumbFunctionCall(uint32 param)
{
	assert(param == TEST_RESULT_1);
}

void CExternJumpTest::Compile(Jitter::CJitter& jitter)
{
	//Build target function
	{
		Framework::CMemStream codeStream;
		jitter.SetStream(&codeStream);

		jitter.Begin();
		{
			jitter.PushCst(TEST_RESULT_2);
			jitter.PullRel(offsetof(CONTEXT, result2));
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
			//Add simple add to make sure variables allocated to registers
			//are properly spilled before executing the jump.
			jitter.PushRel(offsetof(CONTEXT, cst1));
			jitter.PushRel(offsetof(CONTEXT, cst2));
			jitter.Add();
			jitter.PullRel(offsetof(CONTEXT, result1));

			//Add call function to make sure we're still in a good state when jumping
			jitter.PushRel(offsetof(CONTEXT, result1));
			jitter.Call(reinterpret_cast<void*>(&DumbFunctionCall), 1, Jitter::CJitter::RETURN_VALUE_NONE);

			jitter.JumpTo(m_targetFunction.GetCode());
		}
		jitter.End();

		m_sourceFunction = CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
	}
}

void CExternJumpTest::Run()
{
	CONTEXT context;
	context.cst1 = TEST_CST_1;
	context.cst2 = TEST_CST_2;
	m_sourceFunction(&context);
	TEST_VERIFY(context.result1 == TEST_RESULT_1);
	TEST_VERIFY(context.result2 == TEST_RESULT_2);
}
