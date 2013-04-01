#include "RandomAluTest3.h"
#include "MemStream.h"

#define TEST_NUMBER1 (0x00005555)
#define TEST_NUMBER2 (0xAAAAAAAA)

CRandomAluTest3::CRandomAluTest3(bool useConstant)
: m_useConstant(useConstant)
{

}

CRandomAluTest3::~CRandomAluTest3()
{

}

void CRandomAluTest3::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		//a = b ^ a
		if(m_useConstant)
		{
			jitter.PushCst(TEST_NUMBER1);
		}
		else
		{
			jitter.PushRel(offsetof(CONTEXT, number1));
		}

		if(m_useConstant)
		{
			jitter.PushCst(TEST_NUMBER2);
		}
		else
		{
			jitter.PushRel(offsetof(CONTEXT, number2));
		}

		jitter.Xor();

		//b = a

		jitter.PullRel(offsetof(CONTEXT, number1));

		jitter.PushRel(offsetof(CONTEXT, number1));
		jitter.PullRel(offsetof(CONTEXT, number2));
	}
	jitter.End();

	m_function = CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}

void CRandomAluTest3::Run()
{
	memset(&m_context, 0, sizeof(CONTEXT));
	m_context.number1 = TEST_NUMBER1;
	m_context.number2 = TEST_NUMBER2;
	m_function(&m_context);
	TEST_VERIFY(m_context.number1 == static_cast<uint32>(TEST_NUMBER1 ^ TEST_NUMBER2));
	TEST_VERIFY(m_context.number2 == m_context.number1);
}
