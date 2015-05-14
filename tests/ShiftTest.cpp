#include "ShiftTest.h"
#include "MemStream.h"

#define CONSTANT_1 (0x01234567ULL)
#define CONSTANT_2 (0xFEDCBA98ULL)

CShiftTest::CShiftTest(uint32 shiftAmount)
: m_shiftAmount(shiftAmount)
{

}

CShiftTest::~CShiftTest()
{

}

void CShiftTest::Run()
{
	memset(&m_context, 0, sizeof(m_context));

	m_context.value0 = CONSTANT_1;
	m_context.value1 = CONSTANT_2;
	m_context.shiftAmount = m_shiftAmount;

	m_function(&m_context);

	//Amounts are masked here because shift operations are expected to mask the shift amounts

	TEST_VERIFY(m_context.resultSra0 == static_cast<int32>(CONSTANT_1) >> static_cast<int32>(m_shiftAmount & 0x1F));
	TEST_VERIFY(m_context.resultSra1 == static_cast<int32>(CONSTANT_2) >> static_cast<int32>(m_shiftAmount & 0x1F));

	TEST_VERIFY(m_context.resultSraVar0 == static_cast<int32>(CONSTANT_1) >> static_cast<int32>(m_shiftAmount & 0x1F));
	TEST_VERIFY(m_context.resultSraVar1 == static_cast<int32>(CONSTANT_2) >> static_cast<int32>(m_shiftAmount & 0x1F));

	TEST_VERIFY(m_context.resultSrl0 == static_cast<uint32>(CONSTANT_1) >> static_cast<uint32>(m_shiftAmount & 0x1F));
	TEST_VERIFY(m_context.resultSrl1 == static_cast<uint32>(CONSTANT_2) >> static_cast<uint32>(m_shiftAmount & 0x1F));

	TEST_VERIFY(m_context.resultSrlVar0 == static_cast<uint32>(CONSTANT_1) >> static_cast<uint32>(m_shiftAmount & 0x1F));
	TEST_VERIFY(m_context.resultSrlVar1 == static_cast<uint32>(CONSTANT_2) >> static_cast<uint32>(m_shiftAmount & 0x1F));

	TEST_VERIFY(m_context.resultShl0 == static_cast<uint32>(CONSTANT_1) << static_cast<uint32>(m_shiftAmount & 0x1F));
	TEST_VERIFY(m_context.resultShl1 == static_cast<uint32>(CONSTANT_2) << static_cast<uint32>(m_shiftAmount & 0x1F));

	TEST_VERIFY(m_context.resultShlVar0 == static_cast<uint32>(CONSTANT_1) << static_cast<uint32>(m_shiftAmount & 0x1F));
	TEST_VERIFY(m_context.resultShlVar1 == static_cast<uint32>(CONSTANT_2) << static_cast<uint32>(m_shiftAmount & 0x1F));
}

void CShiftTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		//------------------
		//SRA Constant
		jitter.PushRel(offsetof(CONTEXT, value0));
		jitter.Sra(m_shiftAmount);
		jitter.PullRel(offsetof(CONTEXT, resultSra0));

		jitter.PushRel(offsetof(CONTEXT, value1));
		jitter.Sra(m_shiftAmount);
		jitter.PullRel(offsetof(CONTEXT, resultSra1));

		//------------------
		//SRA Variable
		jitter.PushRel(offsetof(CONTEXT, value0));
		jitter.PushRel(offsetof(CONTEXT, shiftAmount));
		jitter.Sra();
		jitter.PullRel(offsetof(CONTEXT, resultSraVar0));

		jitter.PushRel(offsetof(CONTEXT, value1));
		jitter.PushRel(offsetof(CONTEXT, shiftAmount));
		jitter.Sra();
		jitter.PullRel(offsetof(CONTEXT, resultSraVar1));

		//------------------
		//SRL Constant
		jitter.PushRel(offsetof(CONTEXT, value0));
		jitter.Srl(m_shiftAmount);
		jitter.PullRel(offsetof(CONTEXT, resultSrl0));

		jitter.PushRel(offsetof(CONTEXT, value1));
		jitter.Srl(m_shiftAmount);
		jitter.PullRel(offsetof(CONTEXT, resultSrl1));

		//------------------
		//SRL Variable
		jitter.PushRel(offsetof(CONTEXT, value0));
		jitter.PushRel(offsetof(CONTEXT, shiftAmount));
		jitter.Srl();
		jitter.PullRel(offsetof(CONTEXT, resultSrlVar0));

		jitter.PushRel(offsetof(CONTEXT, value1));
		jitter.PushRel(offsetof(CONTEXT, shiftAmount));
		jitter.Srl();
		jitter.PullRel(offsetof(CONTEXT, resultSrlVar1));

		//------------------
		//SHL Constant
		jitter.PushRel(offsetof(CONTEXT, value0));
		jitter.Shl(m_shiftAmount);
		jitter.PullRel(offsetof(CONTEXT, resultShl0));

		jitter.PushRel(offsetof(CONTEXT, value1));
		jitter.Shl(m_shiftAmount);
		jitter.PullRel(offsetof(CONTEXT, resultShl1));

		//------------------
		//SHL Variable
		jitter.PushRel(offsetof(CONTEXT, value0));
		jitter.PushRel(offsetof(CONTEXT, shiftAmount));
		jitter.Shl();
		jitter.PullRel(offsetof(CONTEXT, resultShlVar0));

		jitter.PushRel(offsetof(CONTEXT, value1));
		jitter.PushRel(offsetof(CONTEXT, shiftAmount));
		jitter.Shl();
		jitter.PullRel(offsetof(CONTEXT, resultShlVar1));
	}
	jitter.End();

	m_function = CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}
