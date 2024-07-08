#include "RotateTest.h"
#include "MemStream.h"
#include "BitManip.h"

#define CONSTANT_1 (0x01234567U)
#define CONSTANT_2 (0xFEDCBA98U)

CRotateTest::CRotateTest(uint32 shiftAmount)
    : m_shiftAmount(shiftAmount)
{
}

void CRotateTest::Run()
{
	memset(&m_context, 0, sizeof(m_context));

	m_context.value0 = CONSTANT_1;
	m_context.value1 = CONSTANT_2;
	m_context.shiftAmount = m_shiftAmount;

	m_function(&m_context);

	TEST_VERIFY(m_context.resultRol0 == __builtin_rotateleft32(CONSTANT_1, m_shiftAmount));
	TEST_VERIFY(m_context.resultRol1 == __builtin_rotateleft32(CONSTANT_2, m_shiftAmount));
}

void CRotateTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		//------------------
		//ROL Constant
		jitter.PushRel(offsetof(CONTEXT, value0));
		jitter.Rol(m_shiftAmount);
		jitter.PullRel(offsetof(CONTEXT, resultRol0));

		jitter.PushRel(offsetof(CONTEXT, value1));
		jitter.Rol(m_shiftAmount);
		jitter.PullRel(offsetof(CONTEXT, resultRol1));

		//------------------
		//ROL Variable
		jitter.PushRel(offsetof(CONTEXT, value0));
		jitter.PushRel(offsetof(CONTEXT, shiftAmount));
		jitter.Rol();
		jitter.PullRel(offsetof(CONTEXT, resultRolVar0));

		jitter.PushRel(offsetof(CONTEXT, value1));
		jitter.PushRel(offsetof(CONTEXT, shiftAmount));
		jitter.Rol();
		jitter.PullRel(offsetof(CONTEXT, resultRolVar1));
	}
	jitter.End();

	m_function = FunctionType(codeStream.GetBuffer(), codeStream.GetSize());
}
