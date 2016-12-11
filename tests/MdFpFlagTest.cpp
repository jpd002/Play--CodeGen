#include "MdFpFlagTest.h"
#include "MemStream.h"

CMdFpFlagTest::CMdFpFlagTest()
{

}

CMdFpFlagTest::~CMdFpFlagTest()
{

}

void CMdFpFlagTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		//0
		jitter.MD_PushRel(offsetof(CONTEXT, src0));
		jitter.MD_IsNegative();
		jitter.PullRel(offsetof(CONTEXT, dstIsNegative0));

		jitter.MD_PushRel(offsetof(CONTEXT, src0));
		jitter.MD_IsZero();
		jitter.PullRel(offsetof(CONTEXT, dstIsZero0));

		//1
		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_IsNegative();
		jitter.PullRel(offsetof(CONTEXT, dstIsNegative1));

		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_IsZero();
		jitter.PullRel(offsetof(CONTEXT, dstIsZero1));

		//2
		jitter.MD_PushRel(offsetof(CONTEXT, src2));
		jitter.MD_IsNegative();
		jitter.PullRel(offsetof(CONTEXT, dstIsNegative2));

		jitter.MD_PushRel(offsetof(CONTEXT, src2));
		jitter.MD_IsZero();
		jitter.PullRel(offsetof(CONTEXT, dstIsZero2));
	}
	jitter.End();

	m_function = CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}

void CMdFpFlagTest::Run()
{
	//Some documentation about the flags:
	//IsNegative -> Set when value is not (+/-)0 and less than 0
	//IsZero     -> Set when value is either +0 or -0

	//Notes:
	//- Currently not consistent throughout platforms (x86 returns good result, AArch32/AArch64 fails)
	//- IsNegative is more expensive to implement, might be interesting to have it only check sign bit
	//  and XOR with result from IsZero to achieve the same effect if needed

	CONTEXT ALIGN16 context;
	memset(&context, 0, sizeof(CONTEXT));
	
	context.src0[0] =  0.123412f;
	context.src0[1] = -0.2324f;
	context.src0[2] =  0.f;
	context.src0[3] = -0.f;

	context.src1[0] = 60.f;
	context.src1[1] = 600.f;
	context.src1[2] = -60.f;
	context.src1[3] = 6.f;

	context.src2[0] = -5.5f;
	context.src2[1] = 0;
	context.src2[2] = -7.5f;
	context.src2[3] = -8.5f;

	m_function(&context);

	TEST_VERIFY(context.dstIsNegative0	== 0x4);
	TEST_VERIFY(context.dstIsZero0		== 0x3);

	TEST_VERIFY(context.dstIsNegative1	== 0x2);
	TEST_VERIFY(context.dstIsZero1		== 0x0);

	TEST_VERIFY(context.dstIsNegative2	== 0xB);
	TEST_VERIFY(context.dstIsZero2		== 0x4);
}
