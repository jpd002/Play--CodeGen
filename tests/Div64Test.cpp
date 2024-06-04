#include "Div64Test.h"
#include "MemStream.h"

static constexpr uint64 VALUE_1 = 0xFFCCFF558632100F;
static constexpr uint64 VALUE_2 = 0x0000000000000123;

static constexpr int64 SIGNED_RESULT = static_cast<int64>(VALUE_1) / static_cast<int64>(VALUE_2);
static constexpr uint64 UNSIGNED_RESULT = VALUE_1 / VALUE_2;

CDiv64Test::CDiv64Test(bool isSigned)
    : m_isSigned(isSigned)
{
}

void CDiv64Test::Run()
{
	memset(&m_context, 0, sizeof(m_context));

	m_context.relArg0 = VALUE_1;
	m_context.relArg1 = VALUE_2;

	m_function(&m_context);

	if(m_isSigned)
	{
		TEST_VERIFY(m_context.relRelResult == SIGNED_RESULT);
	}
	else
	{
		TEST_VERIFY(m_context.relRelResult == UNSIGNED_RESULT);
	}
}

void CDiv64Test::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		//Rel / Rel
		{
			jitter.PushRel64(offsetof(CONTEXT, relArg0));
			jitter.PushRel64(offsetof(CONTEXT, relArg1));

			if(m_isSigned)
			{
				jitter.DivS();
			}
			else
			{
				jitter.Div();
			}

			jitter.PullRel64(offsetof(CONTEXT, relRelResult));
		}
	}
	jitter.End();

	m_function = FunctionType(codeStream.GetBuffer(), codeStream.GetSize());
}
