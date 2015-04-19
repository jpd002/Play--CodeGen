#include "MdManipTest.h"
#include "MemStream.h"

CMdManipTest::CMdManipTest()
{

}

CMdManipTest::~CMdManipTest()
{

}

void CMdManipTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		//Masked Mov
		jitter.MD_PushRel(offsetof(CONTEXT, src0));
		jitter.MD_PullRel(offsetof(CONTEXT, dstMasked), true, false, false, true);

		//Push Rel Expand
		jitter.MD_PushRelExpand(offsetof(CONTEXT, src1[1]));
		jitter.MD_PullRel(offsetof(CONTEXT, dstExpandRel));

		//Push Cst Expand
		jitter.MD_PushCstExpand(31415.f);
		jitter.MD_PullRel(offsetof(CONTEXT, dstExpandCst));
	}
	jitter.End();

	m_function = CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}

void CMdManipTest::Run()
{
	CONTEXT ALIGN16 context;
	memset(&context, 0, sizeof(CONTEXT));
	
	context.dstMasked[0] = 1.0f;
	context.dstMasked[1] = 1.0f;
	context.dstMasked[2] = 1.0f;
	context.dstMasked[3] = 1.0f;

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

	TEST_VERIFY(context.dstMasked[0] ==    5.0f);
	TEST_VERIFY(context.dstMasked[1] ==    1.0f);
	TEST_VERIFY(context.dstMasked[2] ==    1.0f);
	TEST_VERIFY(context.dstMasked[3] == 5000.0f);

	TEST_VERIFY(context.dstExpandRel[0] == 600.0f);
	TEST_VERIFY(context.dstExpandRel[1] == 600.0f);
	TEST_VERIFY(context.dstExpandRel[2] == 600.0f);
	TEST_VERIFY(context.dstExpandRel[3] == 600.0f);

	TEST_VERIFY(context.dstExpandCst[0] == 31415.0f);
	TEST_VERIFY(context.dstExpandCst[1] == 31415.0f);
	TEST_VERIFY(context.dstExpandCst[2] == 31415.0f);
	TEST_VERIFY(context.dstExpandCst[3] == 31415.0f);
}
