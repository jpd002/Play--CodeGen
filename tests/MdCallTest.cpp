#include "MdCallTest.h"
#include "MemStream.h"

#define FUNCTION_MASK	(0x0A)

CMdCallTest::CMdCallTest()
{

}

CMdCallTest::~CMdCallTest()
{

}

uint32 CMdCallTest::MdInputFunction(const uint128& value, uint32 mask)
{
	assert(mask == FUNCTION_MASK);
	
	uint32 result = 0;
	for(unsigned int i = 0; i < 4; i++)
	{
		if(mask & (1 << i))
		{
			result += value.nV[i];
		}
	}
	return result;
}

CMdCallTest::uint128 CMdCallTest::MdOutputFunction(uint32 mask)
{
	assert(mask == FUNCTION_MASK);
	
	uint128 result;
	memset(&result, 0, sizeof(result));

	for(unsigned int i = 0; i < 4; i++)
	{
		if(mask & (1 << i))
		{
			result.nV[i] = (1 << i);
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
			jitter.MD_AddW();

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

	m_function = CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}

void CMdCallTest::Run()
{
	CONTEXT ALIGN16 context;
	memset(&context, 0, sizeof(CONTEXT));
	
	context.value0.nV[0] = 8;
	context.value0.nV[1] = 4;
	context.value0.nV[2] = 2;
	context.value0.nV[3] = 1;

	context.value1.nV[0] = 16;
	context.value1.nV[1] = 8;
	context.value1.nV[2] = 4;
	context.value1.nV[3] = 2;

	m_function(&context);

	//Result 0
	{
		uint32 result = 0;
		for(unsigned int i = 0; i < 4; i++)
		{
			if(FUNCTION_MASK & (1 << i))
			{
				result += context.value0.nV[i];
			}
		}

		TEST_VERIFY(result == context.result0);
	}

	//Result 1
	{
		uint32 result = 0;
		for(unsigned int i = 0; i < 4; i++)
		{
			if(FUNCTION_MASK & (1 << i))
			{
				result += context.value0.nV[i] + context.value1.nV[i];
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
				TEST_VERIFY(context.result2.nV[i] == (1 << i));
			}
			else
			{
				TEST_VERIFY(context.result2.nV[i] == 0);
			}
		}
	}
}
