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

		jitter.MD_PushRel(offsetof(CONTEXT, src3));
		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_SubWSS();
		jitter.MD_PullRel(offsetof(CONTEXT, dstSubWSS));

		jitter.MD_PushRel(offsetof(CONTEXT, src3));
		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_SubWUS();
		jitter.MD_PullRel(offsetof(CONTEXT, dstSubWUS));

		jitter.MD_PushRel(offsetof(CONTEXT, srcSat0));
		jitter.MD_PushRel(offsetof(CONTEXT, srcSat1));
		jitter.MD_SubWSS();
		jitter.MD_PullRel(offsetof(CONTEXT, dstSubWSSSat));
	}
	jitter.End();

	m_function = CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}

void CMdSubTest::Run()
{
	CONTEXT ALIGN16 context;
	memset(&context, 0, sizeof(CONTEXT));
	
	static const uint8 srcSat0Value[16] =
	{
		0xFF, 0xFF, 0xFF, 0x7F,
		0x00, 0x00, 0x00, 0x80,
		0xFF, 0xFF, 0xFF, 0x7F,
		0x00, 0x00, 0x00, 0x80,
	};

	static const uint8 srcSat1Value[16] =
	{
		0x00, 0x00, 0x00, 0x80,
		0xFF, 0xFF, 0xFF, 0x7F,
		0x00, 0x00, 0x00, 0x80,
		0xFF, 0xFF, 0xFF, 0x7F,
	};

	for(unsigned int i = 0; i < 16; i++)
	{
		context.src0[i] = i;
		context.src1[i] = (i << 4);
		context.src3[i] = 0xC0;
		context.srcSat0[i] = srcSat0Value[i];
		context.srcSat1[i] = srcSat1Value[i];
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

	static const uint8 dstSubWSSRes[16] =
	{
		0xC0, 0xB0, 0xA0, 0x90,
		0x00, 0x00, 0x00, 0x80,
		0x40, 0x30, 0x20, 0x10,
		0x00, 0xF0, 0xDF, 0xCF
	};

	static const uint8 dstSubWUSRes[16] =
	{
		0xC0, 0xB0, 0xA0, 0x90,
		0x80, 0x70, 0x60, 0x50,
		0x40, 0x30, 0x20, 0x10,
		0x00, 0x00, 0x00, 0x00
	};

	for(unsigned int i = 0; i < 16; i++)
	{
		TEST_VERIFY(dstSubBRes[i]			== context.dstSubB[i]);
		TEST_VERIFY(dstSubBUSRes[i]			== context.dstSubBUS[i]);
		TEST_VERIFY(dstSubHRes[i]			== context.dstSubH[i]);
		TEST_VERIFY(dstSubHSSRes[i]			== context.dstSubHSS[i]);
		TEST_VERIFY(dstSubHUSRes[i]			== context.dstSubHUS[i]);
		TEST_VERIFY(dstSubWRes[i]			== context.dstSubW[i]);
		TEST_VERIFY(dstSubWSSRes[i]			== context.dstSubWSS[i]);
		TEST_VERIFY(dstSubWUSRes[i]			== context.dstSubWUS[i]);
		TEST_VERIFY(srcSat0Value[i]			== context.dstSubWSSSat[i]);
	}
}
