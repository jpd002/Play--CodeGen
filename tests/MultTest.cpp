#include "MultTest.h"
#include "MemStream.h"

static constexpr uint32 CONSTANT_VALUE_1 = 0x80004040;
static constexpr uint32 CONSTANT_VALUE_2 = 0x40408000;

static constexpr int64 CONSTANT_SIGNED_RESULT = 
	static_cast<int64>(static_cast<int32>(CONSTANT_VALUE_1)) *
	static_cast<int64>(static_cast<int32>(CONSTANT_VALUE_2));

static constexpr uint64 CONSTANT_UNSIGNED_RESULT = 
	static_cast<uint64>(CONSTANT_VALUE_1) *
	static_cast<uint64>(CONSTANT_VALUE_2);

static constexpr uint32 RELATIVE_VALUE_1 = 0xFFFF8000;
static constexpr uint32 RELATIVE_VALUE_2 = 0x8000FFFF;

static constexpr int64 RELATIVE_SIGNED_RESULT =
    static_cast<int64>(static_cast<int32>(RELATIVE_VALUE_1)) *
    static_cast<int64>(static_cast<int32>(RELATIVE_VALUE_2));

static constexpr uint64 RELATIVE_UNSIGNED_RESULT =
    static_cast<uint64>(RELATIVE_VALUE_1) *
    static_cast<uint64>(RELATIVE_VALUE_2);

CMultTest::CMultTest(bool isSigned)
: m_isSigned(isSigned)
{

}

void CMultTest::Run()
{
	memset(&m_context, 0, sizeof(m_context));

	m_context.relArg0 = RELATIVE_VALUE_1;
	m_context.relArg1 = RELATIVE_VALUE_2;

	m_function(&m_context);

	if(!m_isSigned)
	{
		TEST_VERIFY(m_context.cstResultLo == static_cast<uint32>(CONSTANT_UNSIGNED_RESULT));
		TEST_VERIFY(m_context.cstResultHi == static_cast<uint32>(CONSTANT_UNSIGNED_RESULT >> 32));

		TEST_VERIFY(m_context.relResultLo == static_cast<uint32>(RELATIVE_UNSIGNED_RESULT));
		TEST_VERIFY(m_context.relResultHi == static_cast<uint32>(RELATIVE_UNSIGNED_RESULT >> 32));
	}
	else
	{
		TEST_VERIFY(m_context.cstResultLo == static_cast<uint32>(CONSTANT_SIGNED_RESULT));
		TEST_VERIFY(m_context.cstResultHi == static_cast<uint32>(CONSTANT_SIGNED_RESULT >> 32));
		
		TEST_VERIFY(m_context.relResultLo == static_cast<uint32>(RELATIVE_SIGNED_RESULT));
		TEST_VERIFY(m_context.relResultHi == static_cast<uint32>(RELATIVE_SIGNED_RESULT >> 32));
	}
}

void CMultTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		//Cst x Cst
		jitter.PushCst(CONSTANT_VALUE_1);
		jitter.PushCst(CONSTANT_VALUE_2);

		if(m_isSigned)
		{
			jitter.MultS();
		}
		else
		{
			jitter.Mult();
		}

		jitter.PushTop();

		jitter.ExtLow64();
		jitter.PullRel(offsetof(CONTEXT, cstResultLo));

		jitter.ExtHigh64();
		jitter.PullRel(offsetof(CONTEXT, cstResultHi));

		//Rel x Rel
		jitter.PushRel(offsetof(CONTEXT, relArg0));
		jitter.PushRel(offsetof(CONTEXT, relArg1));

		if(m_isSigned)
		{
			jitter.MultS();
		}
		else
		{
			jitter.Mult();
		}

		jitter.PushTop();

		jitter.ExtLow64();
		jitter.PullRel(offsetof(CONTEXT, relResultLo));

		jitter.ExtHigh64();
		jitter.PullRel(offsetof(CONTEXT, relResultHi));
	}
	jitter.End();

	m_function = FunctionType(codeStream.GetBuffer(), codeStream.GetSize());
}
