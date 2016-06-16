#include "AliasTest.h"
#include "MemStream.h"
#include "offsetof_def.h"

#define CONSTANT_1	(0x5400)
#define CONSTANT_2	(0xFFFF)
#define CONSTANT_3	(0x34002010)

void CAliasTest::Run()
{
	memset(&m_context, 0, sizeof(m_context));
	m_context.value1[0] = CONSTANT_1;
	m_context.value1[1] = CONSTANT_2;

	for(unsigned int i = 0; i < 4; i++)
	{
		m_context.value2[i] = CONSTANT_3;
		m_context.value3[i] = CONSTANT_3;
	}

	m_context.value4[0] = 0x01234567;
	m_context.value4[1] = 0x01234567;
	m_context.value4[2] = 0x01234567;
	m_context.value4[3] = 0x01234567;

	m_function(&m_context);

	TEST_VERIFY(m_context.result != 0);
	TEST_VERIFY(m_context.value4[0] == (CONSTANT_3 * 2));
	TEST_VERIFY(m_context.value4[1] == 0);
	TEST_VERIFY(m_context.value4[2] == 0);
	TEST_VERIFY(m_context.value4[3] == (CONSTANT_3 * 2));
}

void CAliasTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		jitter.PushCst64(0);
		jitter.PullRel64(offsetof(CONTEXT, value0));

		jitter.PushCst(CONSTANT_1);
		jitter.PullRel(offsetof(CONTEXT, value0[0]));

		jitter.PushCst(CONSTANT_2);
		jitter.PullRel(offsetof(CONTEXT, value0[1]));

		jitter.PushRel64(offsetof(CONTEXT, value0));
		jitter.PushRel64(offsetof(CONTEXT, value1));
		jitter.Cmp64(Jitter::CONDITION_EQ);
		jitter.PullRel(offsetof(CONTEXT, result));

		for(unsigned int i = 0; i < 4; i++)
		{
			jitter.PushCst(0);
			jitter.PullRel(offsetof(CONTEXT, value4[i]));
		}

		jitter.MD_PushRel(offsetof(CONTEXT, value2));
		jitter.MD_PushRel(offsetof(CONTEXT, value3));
		jitter.MD_AddWSS();
		jitter.MD_PullRel(offsetof(CONTEXT, value4), true, false, false, true);
	}
	jitter.End();

	m_function = CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}
