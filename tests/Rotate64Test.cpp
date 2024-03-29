#include "Rotate64Test.h"
#include "MemStream.h"
#include "BitManip.h"

#define CONSTANT_1 (0x8000FFFF01234567ULL)
#define CONSTANT_2 (0x0123456789ABCDEFULL)

CRotate64Test::CRotate64Test(uint32 shiftAmount)
    : m_shiftAmount(shiftAmount)
{
}

void CRotate64Test::Run()
{
	memset(&m_context, 0, sizeof(m_context));

	m_context.value0 = CONSTANT_1;
	m_context.value1 = CONSTANT_2;
	m_context.shiftAmount = m_shiftAmount;

	m_function(&m_context);

	TEST_VERIFY(m_context.resultRol0 == __builtin_rotateleft64(CONSTANT_1, m_shiftAmount));
	TEST_VERIFY(m_context.resultRol1 == __builtin_rotateleft64(CONSTANT_2, m_shiftAmount));
}

void CRotate64Test::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		//------------------
		//ROL Constant
		jitter.PushRel64(offsetof(CONTEXT, value0));
		jitter.Rol64(m_shiftAmount);
		jitter.PullRel64(offsetof(CONTEXT, resultRol0));

		jitter.PushRel64(offsetof(CONTEXT, value1));
		jitter.Rol64(m_shiftAmount);
		jitter.PullRel64(offsetof(CONTEXT, resultRol1));
	}
	jitter.End();

	m_function = FunctionType(codeStream.GetBuffer(), codeStream.GetSize());
}
