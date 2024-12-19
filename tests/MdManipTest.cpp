#include "MdManipTest.h"
#include "MemStream.h"

void CMdManipTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		//Masked Mov
		jitter.MD_PushRel(offsetof(CONTEXT, src0));
		jitter.MD_PullRel(offsetof(CONTEXT, dstMasked0), true, false, false, true);

		jitter.MD_PushRel(offsetof(CONTEXT, src0));
		jitter.MD_PullRel(offsetof(CONTEXT, dstMasked1), true, true, true, false);

		//Push Rel Expand
		jitter.MD_PushRelExpand(offsetof(CONTEXT, src1[1]));
		jitter.MD_PullRel(offsetof(CONTEXT, dstExpandRel));

		//Push Rel Element Expand
		jitter.MD_PushRelElementExpand(offsetof(CONTEXT, src0), 1);
		jitter.MD_PullRel(offsetof(CONTEXT, dstExpandRelElem));

		//Push Cst Expand
		jitter.MD_PushCstExpand(31415.f);
		jitter.MD_PullRel(offsetof(CONTEXT, dstExpandCst));

		//Push Cst Expand Zero
		jitter.MD_PushCstExpand(0U);
		jitter.MD_PullRel(offsetof(CONTEXT, dstExpandCstZero));
	}
	jitter.End();

	m_function = FunctionType(codeStream.GetBuffer(), codeStream.GetSize());
}

void CMdManipTest::Run()
{
	CONTEXT ALIGN16 context;
	memset(&context, 0xFF, sizeof(CONTEXT));

	context.dstMasked0[0] = 1.0f;
	context.dstMasked0[1] = 1.0f;
	context.dstMasked0[2] = 1.0f;
	context.dstMasked0[3] = 1.0f;

	context.dstMasked1[0] = 4.0f;
	context.dstMasked1[1] = 4.0f;
	context.dstMasked1[2] = 4.0f;
	context.dstMasked1[3] = 4.0f;

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

	m_function(&context);

	TEST_VERIFY(context.dstMasked0[0] == 5.0f);
	TEST_VERIFY(context.dstMasked0[1] == 1.0f);
	TEST_VERIFY(context.dstMasked0[2] == 1.0f);
	TEST_VERIFY(context.dstMasked0[3] == 5000.0f);

	TEST_VERIFY(context.dstMasked1[0] == 5.0f);
	TEST_VERIFY(context.dstMasked1[1] == 50.0f);
	TEST_VERIFY(context.dstMasked1[2] == 500.0f);
	TEST_VERIFY(context.dstMasked1[3] == 4.0f);

	TEST_VERIFY(context.dstExpandRel[0] == 600.0f);
	TEST_VERIFY(context.dstExpandRel[1] == 600.0f);
	TEST_VERIFY(context.dstExpandRel[2] == 600.0f);
	TEST_VERIFY(context.dstExpandRel[3] == 600.0f);

	TEST_VERIFY(context.dstExpandRelElem[0] == 50.0f);
	TEST_VERIFY(context.dstExpandRelElem[1] == 50.0f);
	TEST_VERIFY(context.dstExpandRelElem[2] == 50.0f);
	TEST_VERIFY(context.dstExpandRelElem[3] == 50.0f);

	TEST_VERIFY(context.dstExpandCst[0] == 31415.0f);
	TEST_VERIFY(context.dstExpandCst[1] == 31415.0f);
	TEST_VERIFY(context.dstExpandCst[2] == 31415.0f);
	TEST_VERIFY(context.dstExpandCst[3] == 31415.0f);

	TEST_VERIFY(context.dstExpandCstZero[0] == 0);
	TEST_VERIFY(context.dstExpandCstZero[1] == 0);
	TEST_VERIFY(context.dstExpandCstZero[2] == 0);
	TEST_VERIFY(context.dstExpandCstZero[3] == 0);
}
