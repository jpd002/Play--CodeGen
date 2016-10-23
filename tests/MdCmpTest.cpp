#include "MdCmpTest.h"
#include "MemStream.h"

void CMdCmpTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		jitter.MD_PushRel(offsetof(CONTEXT, src3));
		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_CmpEqB();
		jitter.MD_PullRel(offsetof(CONTEXT, dstCmpEqB));

		jitter.MD_PushRel(offsetof(CONTEXT, src0));
		jitter.MD_PushRel(offsetof(CONTEXT, src4));
		jitter.MD_CmpEqH();
		jitter.MD_PullRel(offsetof(CONTEXT, dstCmpEqH));

		jitter.MD_PushRel(offsetof(CONTEXT, src0));
		jitter.MD_PushRel(offsetof(CONTEXT, src2));
		jitter.MD_CmpEqW();
		jitter.MD_PullRel(offsetof(CONTEXT, dstCmpEqW));

		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_PushRel(offsetof(CONTEXT, src3));
		jitter.MD_CmpGtB();
		jitter.MD_PullRel(offsetof(CONTEXT, dstCmpGtB));

		jitter.MD_PushRel(offsetof(CONTEXT, src3));
		jitter.MD_PushRel(offsetof(CONTEXT, src4));
		jitter.MD_CmpGtH();
		jitter.MD_PullRel(offsetof(CONTEXT, dstCmpGtH));

		jitter.MD_PushRel(offsetof(CONTEXT, src0));
		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_CmpGtW();
		jitter.MD_PullRel(offsetof(CONTEXT, dstCmpGtW));
	}
	jitter.End();

	m_function = CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}

void CMdCmpTest::Run()
{
	CONTEXT ALIGN16 context;
	memset(&context, 0, sizeof(CONTEXT));
	
	for(unsigned int i = 0; i < 16; i++)
	{
		context.src0[i] = i;
		context.src1[i] = (i << 4);
		if((i % 8) < 4)
		{
			context.src2[i] = context.src0[i];
		}
		else
		{
			context.src2[i] = context.src1[i];
		}
		context.src3[i] = 0xC0;
		if((i % 4) < 2)
		{
			context.src4[i] = context.src0[i];
		}
		else
		{
			context.src4[i] = context.src1[i];
		}
	}

	m_function(&context);

	static const uint8 dstCmpEqBRes[16] =
	{
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0xFF, 0x00, 0x00, 0x00,
	};

	static const uint8 dstCmpEqHRes[16] =
	{
		0xFF, 0xFF, 0x00, 0x00,
		0xFF, 0xFF, 0x00, 0x00,
		0xFF, 0xFF, 0x00, 0x00,
		0xFF, 0xFF, 0x00, 0x00,
	};

	static const uint8 dstCmpEqWRes[16] =
	{
		0xFF, 0xFF, 0xFF, 0xFF,
		0x00, 0x00, 0x00, 0x00,
		0xFF, 0xFF, 0xFF, 0xFF,
		0x00, 0x00, 0x00, 0x00,
	};

	static const uint8 dstCmpGtBRes[16] =
	{
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0xFF, 0xFF, 0xFF,
	};

	static const uint8 dstCmpGtHRes[16] =
	{
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0xFF, 0xFF,
		0x00, 0x00, 0x00, 0x00,
	};

	static const uint8 dstCmpGtWRes[16] =
	{
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,
	};

	for(unsigned int i = 0; i < 16; i++)
	{
		TEST_VERIFY(dstCmpEqBRes[i]			== context.dstCmpEqB[i]);
		TEST_VERIFY(dstCmpEqHRes[i]			== context.dstCmpEqH[i]);
		TEST_VERIFY(dstCmpEqWRes[i]			== context.dstCmpEqW[i]);
		TEST_VERIFY(dstCmpGtBRes[i]			== context.dstCmpGtB[i]);
		TEST_VERIFY(dstCmpGtHRes[i]			== context.dstCmpGtH[i]);
		TEST_VERIFY(dstCmpGtWRes[i]			== context.dstCmpGtW[i]);
	}
}
