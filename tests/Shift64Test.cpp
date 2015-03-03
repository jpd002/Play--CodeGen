#include "Shift64Test.h"
#include "MemStream.h"

CShift64Test::CShift64Test()
{

}

CShift64Test::~CShift64Test()
{

}

void CShift64Test::Run()
{
	memset(&m_context, 0, sizeof(m_context));

	m_context.value0 = 0x8000FFFF01234567ULL;
	m_context.value1 = 0x0123456789ABCDEFULL;
	m_context.shiftAmount0 = 12;
	m_context.shiftAmount1 = 48;

	m_function(&m_context);

	TEST_VERIFY(m_context.resultSra0 == 0xFFFFFFFFFFFF8000ULL);
	TEST_VERIFY(m_context.resultSra1 == 0xFFFF8000FFFF0123ULL);

	TEST_VERIFY(m_context.resultSraVar0 == 0xFFF8000FFFF01234ULL);
	TEST_VERIFY(m_context.resultSraVar1 == 0xFFFFFFFFFFFF8000ULL);

	TEST_VERIFY(m_context.resultSrl0 == 0x0000000000000123ULL);
	TEST_VERIFY(m_context.resultSrl1 == 0x00000123456789ABULL);

	TEST_VERIFY(m_context.resultSrlVar0 == 0x0000123456789ABCULL);
	TEST_VERIFY(m_context.resultSrlVar1 == 0x0000000000000123ULL);

	TEST_VERIFY(m_context.resultShl0 == 0xCDEF000000000000ULL);
	TEST_VERIFY(m_context.resultShl1 == 0x456789ABCDEF0000ULL);

	TEST_VERIFY(m_context.resultShlVar0 == 0x0FFFF01234567000ULL);
	TEST_VERIFY(m_context.resultShlVar1 == 0x4567000000000000ULL);
}

void CShift64Test::Compile(Jitter::CJitter& jitter)
{
	//TODO: Check result if shift amount is greater than 0x40 for variable shifts

	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		//------------------
		//SRA Constant
		jitter.PushRel64(offsetof(CONTEXT, value0));
		jitter.Sra64(48);
		jitter.PullRel64(offsetof(CONTEXT, resultSra0));

		jitter.PushRel64(offsetof(CONTEXT, value0));
		jitter.Sra64(16);
		jitter.PullRel64(offsetof(CONTEXT, resultSra1));

		//------------------
		//SRA Variable
		jitter.PushRel64(offsetof(CONTEXT, value0));
		jitter.PushRel(offsetof(CONTEXT, shiftAmount0));
		jitter.Sra64();
		jitter.PullRel64(offsetof(CONTEXT, resultSraVar0));

		jitter.PushRel64(offsetof(CONTEXT, value0));
		jitter.PushRel(offsetof(CONTEXT, shiftAmount1));
		jitter.Sra64();
		jitter.PullRel64(offsetof(CONTEXT, resultSraVar1));

		//------------------
		//SRL Constant
		jitter.PushRel64(offsetof(CONTEXT, value1));
		jitter.Srl64(48);
		jitter.PullRel64(offsetof(CONTEXT, resultSrl0));

		jitter.PushRel64(offsetof(CONTEXT, value1));
		jitter.Srl64(16);
		jitter.PullRel64(offsetof(CONTEXT, resultSrl1));

		//------------------
		//SRL Variable
		jitter.PushRel64(offsetof(CONTEXT, value1));
		jitter.PushRel(offsetof(CONTEXT, shiftAmount0));
		jitter.Srl64();
		jitter.PullRel64(offsetof(CONTEXT, resultSrlVar0));

		jitter.PushRel64(offsetof(CONTEXT, value1));
		jitter.PushRel(offsetof(CONTEXT, shiftAmount1));
		jitter.Srl64();
		jitter.PullRel64(offsetof(CONTEXT, resultSrlVar1));

		//------------------
		//SHL Constant
		jitter.PushRel64(offsetof(CONTEXT, value1));
		jitter.Shl64(48);
		jitter.PullRel64(offsetof(CONTEXT, resultShl0));

		jitter.PushRel64(offsetof(CONTEXT, value1));
		jitter.Shl64(16);
		jitter.PullRel64(offsetof(CONTEXT, resultShl1));

		//------------------
		//SHL Variable
		jitter.PushRel64(offsetof(CONTEXT, value0));
		jitter.PushRel(offsetof(CONTEXT, shiftAmount0));
		jitter.Shl64();
		jitter.PullRel64(offsetof(CONTEXT, resultShlVar0));

		jitter.PushRel64(offsetof(CONTEXT, value0));
		jitter.PushRel(offsetof(CONTEXT, shiftAmount1));
		jitter.Shl64();
		jitter.PullRel64(offsetof(CONTEXT, resultShlVar1));
	}
	jitter.End();

	m_function = CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}
