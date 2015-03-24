#include "Cmp64Test.h"
#include "MemStream.h"

CCmp64Test::CCmp64Test(bool useConstant, uint64 value0, uint64 value1)
: m_useConstant(useConstant)
, m_value0(value0)
, m_value1(value1)
{

}

void CCmp64Test::Run()
{
	memset(&m_context, 0, sizeof(m_context));

	m_context.value0 = m_value0;
	m_context.value1 = m_value1;

	m_function(&m_context);

	uint32 resultEq = (m_value0 == m_value1) ? 1 : 0;
	uint32 resultNe = (m_value0 != m_value1) ? 1 : 0;
	uint32 resultBl = static_cast<uint64>(m_value0) < static_cast<uint64>(m_value1) ? 1 : 0;
	uint32 resultLt = static_cast<int64>(m_value0) < static_cast<int64>(m_value1) ? 1 : 0;
	uint32 resultLe = static_cast<int64>(m_value0) <= static_cast<int64>(m_value1) ? 1 : 0;
	uint32 resultAb = static_cast<uint64>(m_value0) > static_cast<uint64>(m_value1) ? 1 : 0;
	uint32 resultGt = static_cast<int64>(m_value0) > static_cast<int64>(m_value1) ? 1 : 0;

	TEST_VERIFY(m_context.resultEq == resultEq);
	TEST_VERIFY(m_context.resultNe == resultNe);
	TEST_VERIFY(m_context.resultBl == resultBl);
	TEST_VERIFY(m_context.resultLt == resultLt);
	TEST_VERIFY(m_context.resultLe == resultLe);
	TEST_VERIFY(m_context.resultAb == resultAb);
	TEST_VERIFY(m_context.resultGt == resultGt);
}

void CCmp64Test::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		//Equal
		//---------------------------------
		jitter.PushRel64(offsetof(CONTEXT, value0));
		if(m_useConstant)
		{
			jitter.PushCst64(m_value1);
		}
		else
		{
			jitter.PushRel64(offsetof(CONTEXT, value1));
		}
		jitter.Cmp64(Jitter::CONDITION_EQ);
		jitter.PullRel(offsetof(CONTEXT, resultEq));

		//Not Equal
		//---------------------------------
		jitter.PushRel64(offsetof(CONTEXT, value0));
		if(m_useConstant)
		{
			jitter.PushCst64(m_value1);
		}
		else
		{
			jitter.PushRel64(offsetof(CONTEXT, value1));
		}
		jitter.Cmp64(Jitter::CONDITION_NE);
		jitter.PullRel(offsetof(CONTEXT, resultNe));

		//Less Than (unsigned)
		//---------------------------------
		jitter.PushRel64(offsetof(CONTEXT, value0));
		if(m_useConstant)
		{
			jitter.PushCst64(m_value1);
		}
		else
		{
			jitter.PushRel64(offsetof(CONTEXT, value1));
		}
		jitter.Cmp64(Jitter::CONDITION_BL);
		jitter.PullRel(offsetof(CONTEXT, resultBl));

		//Less Than (signed)
		//---------------------------------
		jitter.PushRel64(offsetof(CONTEXT, value0));
		if(m_useConstant)
		{
			jitter.PushCst64(m_value1);
		}
		else
		{
			jitter.PushRel64(offsetof(CONTEXT, value1));
		}
		jitter.Cmp64(Jitter::CONDITION_LT);
		jitter.PullRel(offsetof(CONTEXT, resultLt));

		//Less Equal (signed)
		//---------------------------------
		jitter.PushRel64(offsetof(CONTEXT, value0));
		if(m_useConstant)
		{
			jitter.PushCst64(m_value1);
		}
		else
		{
			jitter.PushRel64(offsetof(CONTEXT, value1));
		}
		jitter.Cmp64(Jitter::CONDITION_LE);
		jitter.PullRel(offsetof(CONTEXT, resultLe));

		//Greater Than (unsigned)
		//---------------------------------
		jitter.PushRel64(offsetof(CONTEXT, value0));
		if(m_useConstant)
		{
			jitter.PushCst64(m_value1);
		}
		else
		{
			jitter.PushRel64(offsetof(CONTEXT, value1));
		}
		jitter.Cmp64(Jitter::CONDITION_AB);
		jitter.PullRel(offsetof(CONTEXT, resultAb));

		//Greater Than (signed)
		//---------------------------------
		jitter.PushRel64(offsetof(CONTEXT, value0));
		if(m_useConstant)
		{
			jitter.PushCst64(m_value1);
		}
		else
		{
			jitter.PushRel64(offsetof(CONTEXT, value1));
		}
		jitter.Cmp64(Jitter::CONDITION_GT);
		jitter.PullRel(offsetof(CONTEXT, resultGt));
	}
	jitter.End();

	m_function = CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}
