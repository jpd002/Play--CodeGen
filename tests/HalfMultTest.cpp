#include "HalfMultTest.h"
#include "MemStream.h"

CHalfMultTest::CHalfMultTest()
: m_function(NULL)
{

}

CHalfMultTest::~CHalfMultTest()
{

}

void CHalfMultTest::Run()
{
	memset(&m_context, 0, sizeof(m_context));

	m_context.relArg0 = 0xFFFF8000;
	m_context.relArg1 = 0x8000FFFF;

	(*m_function)(&m_context);

	TEST_VERIFY(m_context.multLoResult == 0x40000000);
	TEST_VERIFY(m_context.multHiResult == 0x00008000);
}

void CHalfMultTest::Compile(Jitter::CJitter& jitter)
{
	if(m_function != NULL) return;

	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		//Mult Halfword Lo
		jitter.PushRel(offsetof(CONTEXT, relArg0));
		jitter.PushRel(offsetof(CONTEXT, relArg0));
		jitter.MultSHL();
		jitter.PullRel(offsetof(CONTEXT, multLoResult));

		//Mult Halfword Hi
		jitter.PushRel(offsetof(CONTEXT, relArg0));
		jitter.PushRel(offsetof(CONTEXT, relArg1));
		jitter.MultSHH();
		jitter.PullRel(offsetof(CONTEXT, multHiResult));
	}
	jitter.End();

	m_function = new CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}
