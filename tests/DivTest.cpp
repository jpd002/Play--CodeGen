#include "DivTest.h"
#include "MemStream.h"

static constexpr uint32 CONSTANT_VALUE_1 = 0x80004040;
static constexpr uint32 CONSTANT_VALUE_2 = 0x40408000;

static constexpr uint32 RELATIVE_VALUE_1 = 0xFFFF8000;
static constexpr uint32 RELATIVE_VALUE_2 = 0x8000FFFF;

static constexpr int32 CSTCST_SIGNED_RESULT_LO = static_cast<int32>(CONSTANT_VALUE_1) / static_cast<int32>(CONSTANT_VALUE_2);
static constexpr int32 CSTCST_SIGNED_RESULT_HI = static_cast<int32>(CONSTANT_VALUE_1) % static_cast<int32>(CONSTANT_VALUE_2);

static constexpr uint32 CSTCST_UNSIGNED_RESULT_LO = CONSTANT_VALUE_1 / CONSTANT_VALUE_2;
static constexpr uint32 CSTCST_UNSIGNED_RESULT_HI = CONSTANT_VALUE_1 % CONSTANT_VALUE_2;

static constexpr int32 RELREL_SIGNED_RESULT_LO = static_cast<int32>(RELATIVE_VALUE_1) / static_cast<int32>(RELATIVE_VALUE_2);
static constexpr int32 RELREL_SIGNED_RESULT_HI = static_cast<int32>(RELATIVE_VALUE_1) % static_cast<int32>(RELATIVE_VALUE_2);

static constexpr uint32 RELREL_UNSIGNED_RESULT_LO = RELATIVE_VALUE_1 / RELATIVE_VALUE_2;
static constexpr uint32 RELREL_UNSIGNED_RESULT_HI = RELATIVE_VALUE_1 % RELATIVE_VALUE_2;

static constexpr int32 RELCST_SIGNED_RESULT_LO = static_cast<int32>(RELATIVE_VALUE_1) / static_cast<int32>(CONSTANT_VALUE_2);
static constexpr int32 RELCST_SIGNED_RESULT_HI = static_cast<int32>(RELATIVE_VALUE_1) % static_cast<int32>(CONSTANT_VALUE_2);

static constexpr uint32 RELCST_UNSIGNED_RESULT_LO = RELATIVE_VALUE_1 / CONSTANT_VALUE_2;
static constexpr uint32 RELCST_UNSIGNED_RESULT_HI = RELATIVE_VALUE_1 % CONSTANT_VALUE_2;

static constexpr int32 CSTREL_SIGNED_RESULT_LO = static_cast<int32>(CONSTANT_VALUE_1) / static_cast<int32>(RELATIVE_VALUE_2);
static constexpr int32 CSTREL_SIGNED_RESULT_HI = static_cast<int32>(CONSTANT_VALUE_1) % static_cast<int32>(RELATIVE_VALUE_2);

static constexpr uint32 CSTREL_UNSIGNED_RESULT_LO = CONSTANT_VALUE_1 / RELATIVE_VALUE_2;
static constexpr uint32 CSTREL_UNSIGNED_RESULT_HI = CONSTANT_VALUE_1 % RELATIVE_VALUE_2;

CDivTest::CDivTest(bool isSigned)
: m_isSigned(isSigned)
{

}

void CDivTest::Run()
{
	memset(&m_context, 0, sizeof(m_context));

	m_context.relArg0 = RELATIVE_VALUE_1;
	m_context.relArg1 = RELATIVE_VALUE_2;

	m_function(&m_context);

	if(m_isSigned)
	{
		TEST_VERIFY(m_context.cstCstResultLo == CSTCST_SIGNED_RESULT_LO);
		TEST_VERIFY(m_context.cstCstResultHi == CSTCST_SIGNED_RESULT_HI);

		TEST_VERIFY(m_context.relRelResultLo == RELREL_SIGNED_RESULT_LO);
		TEST_VERIFY(m_context.relRelResultHi == RELREL_SIGNED_RESULT_HI);

		TEST_VERIFY(m_context.relCstResultLo == RELCST_SIGNED_RESULT_LO);
		TEST_VERIFY(m_context.relCstResultHi == RELCST_SIGNED_RESULT_HI);

		TEST_VERIFY(m_context.cstRelResultLo == CSTREL_SIGNED_RESULT_LO);
		TEST_VERIFY(m_context.cstRelResultHi == CSTREL_SIGNED_RESULT_HI);
	}
	else
	{
		TEST_VERIFY(m_context.cstCstResultLo == CSTCST_UNSIGNED_RESULT_LO);
		TEST_VERIFY(m_context.cstCstResultHi == CSTCST_UNSIGNED_RESULT_HI);

		TEST_VERIFY(m_context.relRelResultLo == RELREL_UNSIGNED_RESULT_LO);
		TEST_VERIFY(m_context.relRelResultHi == RELREL_UNSIGNED_RESULT_HI);

		TEST_VERIFY(m_context.relCstResultLo == RELCST_UNSIGNED_RESULT_LO);
		TEST_VERIFY(m_context.relCstResultHi == RELCST_UNSIGNED_RESULT_HI);

		TEST_VERIFY(m_context.cstRelResultLo == CSTREL_UNSIGNED_RESULT_LO);
		TEST_VERIFY(m_context.cstRelResultHi == CSTREL_UNSIGNED_RESULT_HI);
	}
}

void CDivTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		//Cst / Cst
		{
			jitter.PushCst(CONSTANT_VALUE_1);
			jitter.PushCst(CONSTANT_VALUE_2);

			if(m_isSigned)
			{
				jitter.DivS();
			}
			else
			{
				jitter.Div();
			}

			jitter.PushTop();

			jitter.ExtLow64();
			jitter.PullRel(offsetof(CONTEXT, cstCstResultLo));

			jitter.ExtHigh64();
			jitter.PullRel(offsetof(CONTEXT, cstCstResultHi));
		}

		//Rel / Rel
		{
			jitter.PushRel(offsetof(CONTEXT, relArg0));
			jitter.PushRel(offsetof(CONTEXT, relArg1));

			if(m_isSigned)
			{
				jitter.DivS();
			}
			else
			{
				jitter.Div();
			}

			jitter.PushTop();

			jitter.ExtLow64();
			jitter.PullRel(offsetof(CONTEXT, relRelResultLo));

			jitter.ExtHigh64();
			jitter.PullRel(offsetof(CONTEXT, relRelResultHi));
		}

		//Rel / Cst
		{
			jitter.PushRel(offsetof(CONTEXT, relArg0));
			jitter.PushCst(CONSTANT_VALUE_2);

			if(m_isSigned)
			{
				jitter.DivS();
			}
			else
			{
				jitter.Div();
			}

			jitter.PushTop();

			jitter.ExtLow64();
			jitter.PullRel(offsetof(CONTEXT, relCstResultLo));

			jitter.ExtHigh64();
			jitter.PullRel(offsetof(CONTEXT, relCstResultHi));
		}

		//Cst / Rel
		{
			jitter.PushCst(CONSTANT_VALUE_1);
			jitter.PushRel(offsetof(CONTEXT, relArg1));

			if(m_isSigned)
			{
				jitter.DivS();
			}
			else
			{
				jitter.Div();
			}

			jitter.PushTop();

			jitter.ExtLow64();
			jitter.PullRel(offsetof(CONTEXT, cstRelResultLo));

			jitter.ExtHigh64();
			jitter.PullRel(offsetof(CONTEXT, cstRelResultHi));
		}
	}
	jitter.End();

	m_function = FunctionType(codeStream.GetBuffer(), codeStream.GetSize());
}
