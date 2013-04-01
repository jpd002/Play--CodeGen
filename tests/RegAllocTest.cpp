#include "RegAllocTest.h"
#include "MemStream.h"
#include "offsetof_def.h"

#define TEST_NUMBER1 (0xDEADDEAD)
#define TEST_NUMBER2 (0xCAFECAFE)

CRegAllocTest::CRegAllocTest()
{

}

CRegAllocTest::~CRegAllocTest()
{

}

void CRegAllocTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{		
		for(unsigned int i = 2; i < MAX_VARS; i++)
		{
			jitter.PushRel(offsetof(CONTEXT, number[i - 2]));
			jitter.PushRel(offsetof(CONTEXT, number[i - 1]));
			jitter.Xor();
			jitter.PullRel(offsetof(CONTEXT, number[i - 0]));
		}
	}
	jitter.End();

	m_function = CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}

void CRegAllocTest::Run()
{
	memset(&m_context, 0, sizeof(CONTEXT));
	m_context.number[0] = TEST_NUMBER1;
	m_context.number[1] = TEST_NUMBER2;
	m_function(&m_context);
	TEST_VERIFY(m_context.number[0] == TEST_NUMBER1);
	TEST_VERIFY(m_context.number[1] == TEST_NUMBER2);
	for(unsigned int i = 2; i < MAX_VARS; i++)
	{
		TEST_VERIFY(m_context.number[i] == (m_context.number[i - 1] ^ m_context.number[i - 2]));
	}
}
