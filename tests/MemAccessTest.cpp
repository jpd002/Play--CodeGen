#include "MemAccessTest.h"
#include "MemStream.h"

#define CONSTANT_1	(0xFFCC8844)

CMemAccessTest::CMemAccessTest()
: m_function(NULL)
{

}

CMemAccessTest::~CMemAccessTest()
{

}

void CMemAccessTest::Run()
{
	memset(&m_context, 0, sizeof(m_context));
	memset(&m_memory, 0x80, sizeof(m_memory));

	m_context.offset = 0x4;
	m_context.memory = m_memory;

	(*m_function)(&m_context);

	TEST_VERIFY(m_memory[1] == CONSTANT_1);
	TEST_VERIFY(m_context.result == 0x80808080);
}

void CMemAccessTest::Compile(Jitter::CJitter& jitter)
{
	if(m_function != NULL) return;

	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		//Store test
		jitter.PushRelRef(offsetof(CONTEXT, memory));
		jitter.PushRel(offsetof(CONTEXT, offset));
		jitter.AddRef();

		jitter.PushCst(CONSTANT_1);
		jitter.StoreAtRef();

		//Read test
		jitter.PushRelRef(offsetof(CONTEXT, memory));
		jitter.PushCst(0x08);
		jitter.AddRef();

		jitter.LoadFromRef();
		jitter.PullRel(offsetof(CONTEXT, result));
	}
	jitter.End();

	m_function = new CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}
