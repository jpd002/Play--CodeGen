#include "AliasTest2.h"
#include "MemStream.h"
#include "offsetof_def.h"

#define CONSTANT_1 0x3F000000	//0.5
#define CONSTANT_2 0x3F800000	//1.0
#define CONSTANT_3 0x40000000	//2.0
#define CONSTANT_4 0x40800000	//4.0

void CAliasTest2::Run()
{
	memset(&m_context, 0, sizeof(m_context));

	for(int i = 0; i < 4; i++)
	{
		m_context.value2[i] = 1.0f;
	}

	m_function(&m_context);

	TEST_VERIFY(m_context.value1[0] == 3.0f);
	TEST_VERIFY(m_context.value1[1] == 3.0f);
	TEST_VERIFY(m_context.value1[2] == 3.0f);
	TEST_VERIFY(m_context.value1[3] == 4.0f);

	TEST_VERIFY(m_context.value3[0] == 4.0f);
	TEST_VERIFY(m_context.value3[1] == 4.0f);
	TEST_VERIFY(m_context.value3[2] == 4.0f);
	TEST_VERIFY(m_context.value3[3] == 4.0f);
}

void CAliasTest2::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		jitter.PushCst(CONSTANT_1);
		jitter.PullRel(offsetof(CONTEXT, value1[0]));

		jitter.PushCst(CONSTANT_2);
		jitter.PullRel(offsetof(CONTEXT, value1[1]));

		jitter.PushCst(CONSTANT_3);
		jitter.PullRel(offsetof(CONTEXT, value1[2]));

		jitter.PushCst(CONSTANT_4);
		jitter.PullRel(offsetof(CONTEXT, value1[3]));

		jitter.MD_PushRel(offsetof(CONTEXT, value2));
		jitter.MD_PushRelExpand(offsetof(CONTEXT, value1[2]));
		jitter.MD_AddS();
		jitter.MD_PullRel(offsetof(CONTEXT, value1), true, true, true, false);
		//Here value1 should be (3, 3, 3, 4)

		jitter.MD_PushRel(offsetof(CONTEXT, value2));
		jitter.MD_PushRelExpand(offsetof(CONTEXT, value1[2]));
		jitter.MD_AddS();
		jitter.MD_PullRel(offsetof(CONTEXT, value3));
		//Here value3 should be (4, 4, 4, 4)
	}
	jitter.End();

	m_function = CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}
