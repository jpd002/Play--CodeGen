#include "MdFpFlagTest.h"
#include "MemStream.h"

void CMdFpFlagTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		jitter.MD_PushRel(offsetof(CONTEXT, src0));
		jitter.MD_MakeSignZero();
		jitter.PullRel(offsetof(CONTEXT, dstSzStatus0));

		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_MakeSignZero();
		jitter.PullRel(offsetof(CONTEXT, dstSzStatus1));

		jitter.MD_PushRel(offsetof(CONTEXT, src2));
		jitter.MD_MakeSignZero();
		jitter.PullRel(offsetof(CONTEXT, dstSzStatus2));

		jitter.MD_PushRel(offsetof(CONTEXT, src3));
		jitter.MD_MakeSignZero();
		jitter.PullRel(offsetof(CONTEXT, dstSzStatus3));
	}
	jitter.End();

	m_function = FunctionType(codeStream.GetBuffer(), codeStream.GetSize());
}

void CMdFpFlagTest::Run()
{
	//Some documentation about the flags:
	//IsNegative -> Set when value's sign bit is set (regardless of value)
	//IsZero     -> Set when value is either +0 or -0

	CONTEXT ALIGN16 context;
	memset(&context, 0, sizeof(CONTEXT));

	context.src0[0] = 0.123412f;
	context.src0[1] = -0.2324f;
	context.src0[2] = 0.f;
	context.src0[3] = -0.f;

	context.src1[0] = 60.f;
	context.src1[1] = 600.f;
	context.src1[2] = -60.f;
	context.src1[3] = 6.f;

	context.src2[0] = -5.5f;
	context.src2[1] = 0;
	context.src2[2] = -7.5f;
	context.src2[3] = -8.5f;

	//Test some weird numbers
	//NOTE: On ARMv7 NEON, denormals are considered as 0, while they aren't on other archs.
	context.src3[0] = 0x7FFFFFFF; //NaN
	context.src3[1] = 0xFFFFFFFF; //NaN (negative)
	context.src3[2] = 0x7F800000; //INF
	context.src3[3] = 0xFF800000; //INF (negative)

	m_function(&context);

	TEST_VERIFY(context.dstSzStatus0 == 0x53);
	TEST_VERIFY(context.dstSzStatus1 == 0x20);
	TEST_VERIFY(context.dstSzStatus2 == 0xB4);
	TEST_VERIFY(context.dstSzStatus3 == 0x50);
}
