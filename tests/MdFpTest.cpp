#include "MdFpTest.h"
#include "MemStream.h"

CMdFpTest::CMdFpTest()
{

}

CMdFpTest::~CMdFpTest()
{
	delete m_function;
}

void CMdFpTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		jitter.MD_PushRel(offsetof(CONTEXT, src0));
		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_AddS();
		jitter.MD_PullRel(offsetof(CONTEXT, dstAdd));

		jitter.MD_PushRel(offsetof(CONTEXT, src0));
		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_SubS();
		jitter.MD_PullRel(offsetof(CONTEXT, dstSub));

		jitter.MD_PushRel(offsetof(CONTEXT, src0));
		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_MulS();
		jitter.MD_PullRel(offsetof(CONTEXT, dstMul));

		jitter.MD_PushRel(offsetof(CONTEXT, dstSub));
		jitter.MD_PullRel(offsetof(CONTEXT, dstMasked), false, true, true, false);

		jitter.MD_PushRel(offsetof(CONTEXT, dstMasked));
		jitter.MD_IsNegative();
		jitter.MD_PullRel(offsetof(CONTEXT, dstIsNegative));

		jitter.MD_PushRel(offsetof(CONTEXT, dstMasked));
		jitter.MD_IsZero();
		jitter.MD_PullRel(offsetof(CONTEXT, dstIsZero));

		jitter.MD_PushRelExpand(offsetof(CONTEXT, src0[1]));
		jitter.MD_PullRel(offsetof(CONTEXT, dstExpandRel));

		jitter.MD_PushCstExpand(31415);
		jitter.MD_PullRel(offsetof(CONTEXT, dstExpandCst));

		jitter.MD_PushRel(offsetof(CONTEXT, src2));
		jitter.MD_ToWordTruncate();
		jitter.MD_PullRel(offsetof(CONTEXT, dstCvtWord));
	}
	jitter.End();

	m_function = new CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}

void CMdFpTest::Run()
{
	CONTEXT __declspec(align(16)) context;
	memset(&context, 0, sizeof(CONTEXT));
	
	context.src0[0] = 5.f;
	context.src0[1] = 50.f;
	context.src0[2] = 500.f;
	context.src0[3] = 5000.f;

	context.src1[0] = 6000.f;
	context.src1[1] = 600.f;
	context.src1[2] = 60.f;
	context.src1[3] = 6.f;

	context.src2[0] = 5.5f;
	context.src2[1] = 6.5f;
	context.src2[2] = 7.5f;
	context.src2[3] = 8.5f;

	(*m_function)(&context);

	TEST_VERIFY(context.dstAdd[0] ==  6005.f);
	TEST_VERIFY(context.dstAdd[1] ==   650.f);
	TEST_VERIFY(context.dstAdd[2] ==   560.f);
	TEST_VERIFY(context.dstAdd[3] ==  5006.f);

	TEST_VERIFY(context.dstSub[0] == -5995.f);
	TEST_VERIFY(context.dstSub[1] ==  -550.f);
	TEST_VERIFY(context.dstSub[2] ==   440.f);
	TEST_VERIFY(context.dstSub[3] ==  4994.f);

	TEST_VERIFY(context.dstMul[0] == 30000.f);
	TEST_VERIFY(context.dstMul[1] == 30000.f);
	TEST_VERIFY(context.dstMul[2] == 30000.f);
	TEST_VERIFY(context.dstMul[3] == 30000.f);

	TEST_VERIFY(context.dstMasked[0] ==    0.f);
	TEST_VERIFY(context.dstMasked[1] == -550.f);
	TEST_VERIFY(context.dstMasked[2] ==  440.f);
	TEST_VERIFY(context.dstMasked[3] ==    0.f);

	TEST_VERIFY(!context.dstIsNegative[0]);
	TEST_VERIFY(context.dstIsNegative[1]);
	TEST_VERIFY(!context.dstIsNegative[2]);
	TEST_VERIFY(!context.dstIsNegative[3]);

	TEST_VERIFY(context.dstIsZero[0]);
	TEST_VERIFY(!context.dstIsZero[1]);
	TEST_VERIFY(!context.dstIsZero[2]);
	TEST_VERIFY(context.dstIsZero[3]);

	TEST_VERIFY(context.dstExpandRel[0] == 50.0f);
	TEST_VERIFY(context.dstExpandRel[1] == 50.0f);
	TEST_VERIFY(context.dstExpandRel[2] == 50.0f);
	TEST_VERIFY(context.dstExpandRel[3] == 50.0f);

	TEST_VERIFY(context.dstExpandCst[0] == 31415.0f);
	TEST_VERIFY(context.dstExpandCst[1] == 31415.0f);
	TEST_VERIFY(context.dstExpandCst[2] == 31415.0f);
	TEST_VERIFY(context.dstExpandCst[3] == 31415.0f);

	TEST_VERIFY(context.dstCvtWord[0] == 5);
	TEST_VERIFY(context.dstCvtWord[1] == 6);
	TEST_VERIFY(context.dstCvtWord[2] == 7);
	TEST_VERIFY(context.dstCvtWord[3] == 8);
}
