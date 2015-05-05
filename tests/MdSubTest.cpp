#include "MdSubTest.h"
#include "MemStream.h"

void CMdSubTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		jitter.MD_PushRel(offsetof(CONTEXT, src3));
		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_SubB();
		jitter.MD_PullRel(offsetof(CONTEXT, dstSubB));

		jitter.MD_PushRel(offsetof(CONTEXT, src3));
		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_SubBUS();
		jitter.MD_PullRel(offsetof(CONTEXT, dstSubBUS));

		jitter.MD_PushRel(offsetof(CONTEXT, src3));
		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_SubH();
		jitter.MD_PullRel(offsetof(CONTEXT, dstSubH));

		jitter.MD_PushRel(offsetof(CONTEXT, src3));
		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_SubHSS();
		jitter.MD_PullRel(offsetof(CONTEXT, dstSubHSS));

		jitter.MD_PushRel(offsetof(CONTEXT, src3));
		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_SubHUS();
		jitter.MD_PullRel(offsetof(CONTEXT, dstSubHUS));

		jitter.MD_PushRel(offsetof(CONTEXT, src3));
		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_SubW();
		jitter.MD_PullRel(offsetof(CONTEXT, dstSubW));
	}
	jitter.End();

	m_function = CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}

void CMdSubTest::Run()
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
	}

	m_function(&context);

	static const uint8 dstSubBRes[16] =
	{
		0xC0, 0xB0, 0xA0, 0x90,
		0x80, 0x70, 0x60, 0x50,
		0x40, 0x30, 0x20, 0x10,
		0x00, 0xF0, 0xE0, 0xD0
	};

	static const uint8 dstSubBUSRes[16] =
	{
		0xC0, 0xB0, 0xA0, 0x90,
		0x80, 0x70, 0x60, 0x50,
		0x40, 0x30, 0x20, 0x10,
		0x00, 0x00, 0x00, 0x00
	};

	static const uint8 dstSubHRes[16] =
	{
		0xC0, 0xB0,
		0xA0, 0x90,

		0x80, 0x70,
		0x60, 0x50,

		0x40, 0x30,
		0x20, 0x10,

		0x00, 0xF0,
		0xE0, 0xCF
	};

	static const uint8 dstSubHSSRes[16] =
	{
		0xC0, 0xB0,
		0xA0, 0x90,

		0x00, 0x80,
		0x00, 0x80,

		0x40, 0x30,
		0x20, 0x10,

		0x00, 0xF0,
		0xE0, 0xCF
	};

	static const uint8 dstSubHUSRes[16] =
	{
		0xC0, 0xB0,
		0xA0, 0x90,

		0x80, 0x70,
		0x60, 0x50,

		0x40, 0x30,
		0x20, 0x10,

		0x00, 0x00,
		0x00, 0x00
	};

	static const uint8 dstSubWRes[16] =
	{
		0xC0, 0xB0, 0xA0, 0x90,
		0x80, 0x70, 0x60, 0x50,
		0x40, 0x30, 0x20, 0x10,
		0x00, 0xF0, 0xDF, 0xCF
	};

	for(unsigned int i = 0; i < 16; i++)
	{
		TEST_VERIFY(dstSubBRes[i]			== context.dstSubB[i]);
		TEST_VERIFY(dstSubBUSRes[i]			== context.dstSubBUS[i]);
		TEST_VERIFY(dstSubHRes[i]			== context.dstSubH[i]);
		TEST_VERIFY(dstSubHSSRes[i]			== context.dstSubHSS[i]);
		TEST_VERIFY(dstSubHUSRes[i]			== context.dstSubHUS[i]);
		TEST_VERIFY(dstSubWRes[i]			== context.dstSubW[i]);
	}
}
