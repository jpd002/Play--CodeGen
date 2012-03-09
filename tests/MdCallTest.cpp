#include "MdCallTest.h"
#include "MemStream.h"

#define FUNCTION_MASK	(0x0A)

CMdCallTest::CMdCallTest()
{

}

CMdCallTest::~CMdCallTest()
{
	delete m_function;
}

uint32 CMdCallTest::MdInputFunction(const uint128& value, uint32 mask)
{
	float result = 0;
	for(unsigned int i = 0; i < 4; i++)
	{
		if(mask & (1 << i))
		{
			result += value.v[i];
		}
	}
	return *reinterpret_cast<uint32*>(&result);
}

CMdCallTest::uint128 CMdCallTest::MdOutputFunction(uint32 mask)
{
	uint128 result;
	memset(&result, 0, sizeof(result));

	for(unsigned int i = 0; i < 4; i++)
	{
		if(mask & (1 << i))
		{
			result.v[i] = static_cast<float>(1 << i);
		}
	}

	return result;
}

void CMdCallTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		//Result 0
		{
			jitter.MD_PushRel(offsetof(CONTEXT, value0));
			jitter.PushCst(FUNCTION_MASK);

			jitter.Call(reinterpret_cast<void*>(&CMdCallTest::MdInputFunction), 2, Jitter::CJitter::RETURN_VALUE_32);
			jitter.PullRel(offsetof(CONTEXT, result0));
		}

		//Result 1
		{
			jitter.MD_PushRel(offsetof(CONTEXT, value0));
			jitter.MD_PushRel(offsetof(CONTEXT, value1));
			jitter.MD_AddS();

			jitter.PushCst(FUNCTION_MASK);

			jitter.Call(reinterpret_cast<void*>(&CMdCallTest::MdInputFunction), 2, Jitter::CJitter::RETURN_VALUE_32);
			jitter.PullRel(offsetof(CONTEXT, result1));
		}

		//Result 2
		{
			jitter.PushCst(FUNCTION_MASK);
			jitter.Call(reinterpret_cast<void*>(&CMdCallTest::MdOutputFunction), 1, Jitter::CJitter::RETURN_VALUE_128);
			jitter.MD_PullRel(offsetof(CONTEXT, result2));
		}
	}
	jitter.End();

	m_function = new CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}

void CMdCallTest::Run()
{
	CONTEXT ALIGN16 context;
	memset(&context, 0, sizeof(CONTEXT));
	
	context.value0.v[0] = 8.0f;
	context.value0.v[1] = 4.0f;
	context.value0.v[2] = 2.0f;
	context.value0.v[3] = 1.0f;

	context.value1.v[0] = 16.0f;
	context.value1.v[1] = 8.0f;
	context.value1.v[2] = 4.0f;
	context.value1.v[3] = 2.0f;

	(*m_function)(&context);

	//Result 0
	{
		float result = 0;
		for(unsigned int i = 0; i < 4; i++)
		{
			if(FUNCTION_MASK & (1 << i))
			{
				result += context.value0.v[i];
			}
		}

		TEST_VERIFY(result == context.result0);
	}

	//Result 1
	{
		float result = 0;
		for(unsigned int i = 0; i < 4; i++)
		{
			if(FUNCTION_MASK & (1 << i))
			{
				result += context.value0.v[i] + context.value1.v[i];
			}
		}

		TEST_VERIFY(result == context.result1);
	}

	//Result 2
	{
		for(unsigned int i = 0; i < 4; i++)
		{
			if(FUNCTION_MASK & (1 << i))
			{
				TEST_VERIFY(context.result2.v[i] == static_cast<float>(1 << i));
			}
			else
			{
				TEST_VERIFY(context.result2.v[i] == 0);
			}
		}
	}
}
