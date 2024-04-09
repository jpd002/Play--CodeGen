#include "FpConvTest.h"
#include "MemStream.h"

void CFpConvTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		jitter.FP_PushRel64(offsetof(CONTEXT, inDouble));
		jitter.FP_ToSingleD();
		jitter.FP_PullRel32(offsetof(CONTEXT, outDtoS));

		jitter.FP_PushRel32(offsetof(CONTEXT, inFloat));
		jitter.FP_ToDoubleS();
		jitter.FP_PullRel64(offsetof(CONTEXT, outStoD));
	}
	jitter.End();

	m_function = FunctionType(codeStream.GetBuffer(), codeStream.GetSize());
}

void CFpConvTest::Run()
{
	CONTEXT context = {};
	context.inDouble = 123;
	context.inFloat = 345;

	m_function(&context);

	TEST_VERIFY(context.outDtoS == 123.f);
	TEST_VERIFY(context.outStoD == 345.f);
}
