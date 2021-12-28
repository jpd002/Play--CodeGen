#include "DoubleFusedMultiAddTest.h"
#include "MemStream.h"

CDoubleFusedMultiAddTest::CDoubleFusedMultiAddTest()
{
}

CDoubleFusedMultiAddTest::~CDoubleFusedMultiAddTest()
{
}

void CDoubleFusedMultiAddTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		jitter.MD_PushRel(offsetof(CONTEXT, number3));
		jitter.MD_PushRel(offsetof(CONTEXT, number2));
		jitter.MD_PushRel(offsetof(CONTEXT, number1));
		jitter.MD_MulAdd();
		jitter.MD_PullRel(offsetof(CONTEXT, res1));

		jitter.MD_PushRel(offsetof(CONTEXT, number4));
		jitter.MD_PushRel(offsetof(CONTEXT, number2));
		jitter.MD_PushRel(offsetof(CONTEXT, number2));
		jitter.MD_MulAdd();
		jitter.MD_PullRel(offsetof(CONTEXT, res2));

		jitter.MD_PushRel(offsetof(CONTEXT, number3));
		jitter.MD_PushRel(offsetof(CONTEXT, number4));
		jitter.MD_PushRel(offsetof(CONTEXT, number2));
		jitter.MD_MulAdd();
		jitter.MD_PullRel(offsetof(CONTEXT, res3));

		jitter.MD_PushRel(offsetof(CONTEXT, number3));
		jitter.MD_PushRel(offsetof(CONTEXT, number2));
		jitter.MD_PushRel(offsetof(CONTEXT, number1));
		jitter.MD_MulS();
		jitter.MD_AddS();
		jitter.MD_PullRel(offsetof(CONTEXT, res4));

		jitter.MD_PushRel(offsetof(CONTEXT, number4));
		jitter.MD_PushRel(offsetof(CONTEXT, number2));
		jitter.MD_PushRel(offsetof(CONTEXT, number2));
		jitter.MD_MulS();
		jitter.MD_AddS();
		jitter.MD_PullRel(offsetof(CONTEXT, res5));

		jitter.MD_PushRel(offsetof(CONTEXT, number3));
		jitter.MD_PushRel(offsetof(CONTEXT, number4));
		jitter.MD_PushRel(offsetof(CONTEXT, number2));
		jitter.MD_MulS();
		jitter.MD_AddS();
		jitter.MD_PullRel(offsetof(CONTEXT, res6));
	}
	jitter.End();

	m_function = CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}

void CDoubleFusedMultiAddTest::Run()
{
	memset(&m_context, 0, sizeof(CONTEXT));
	m_context.number1[1] = 1 ;
	m_context.number2[1] = 2 ;
	m_context.number3[1] = 4 ;
	m_context.number4[1] = 16;
    m_context.number1[2] = 1 ;
	m_context.number2[2] = 2 ;
	m_context.number3[2] = 4 ;
	m_context.number4[2] = 16;
	m_function(&m_context);
	for(int i = 0; i < 4; ++i)
	{
		TEST_VERIFY(m_context.res1[i] == m_context.res4[i]);
		TEST_VERIFY(m_context.res2[i] == m_context.res5[i]);
		TEST_VERIFY(m_context.res3[i] == m_context.res6[i]);
	}
}
