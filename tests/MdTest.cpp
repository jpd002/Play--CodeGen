#include "MdTest.h"
#include "MemStream.h"

void CMdTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_PullRel(offsetof(CONTEXT, dstMov));

		//Shifts
		jitter.MD_PushRel(offsetof(CONTEXT, src0));
		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.PushCst(48);
		jitter.MD_Srl256();
		jitter.MD_PullRel(offsetof(CONTEXT, dstSrl256_1));

		jitter.MD_PushRel(offsetof(CONTEXT, src0));
		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.PushRel(offsetof(CONTEXT, shiftAmount));
		jitter.MD_Srl256();
		jitter.MD_PullRel(offsetof(CONTEXT, dstSrl256_2));

		//Packs
		jitter.MD_PushRel(offsetof(CONTEXT, src0));
		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_PackHB();
		jitter.MD_PullRel(offsetof(CONTEXT, dstPackHB));

		jitter.MD_PushRel(offsetof(CONTEXT, src0));
		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_PackWH();
		jitter.MD_PullRel(offsetof(CONTEXT, dstPackWH));

		//Aliased packs
		jitter.MD_PushRel(offsetof(CONTEXT, dstPackHBAlias));
		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_PackHB();
		jitter.MD_PullRel(offsetof(CONTEXT, dstPackHBAlias));

		jitter.MD_PushRel(offsetof(CONTEXT, dstPackWHAlias));
		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_PackWH();
		jitter.MD_PullRel(offsetof(CONTEXT, dstPackWHAlias));
	}
	jitter.End();

	m_function = CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}

void CMdTest::Run()
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
	for(unsigned int i = 0; i < 16; i++)
	{
		context.dstPackHBAlias[i] = context.src0[i];
		context.dstPackWHAlias[i] = context.src0[i];
	}
	context.shiftAmount = 16;

	m_function(&context);

	static const uint8 dstMovRes[16] =
	{
		0x00, 0x10, 0x20, 0x30,
		0x40, 0x50, 0x60, 0x70,
		0x80, 0x90, 0xA0, 0xB0,
		0xC0, 0xD0, 0xE0, 0xF0
	};

	static const uint8 dstSrl256_1[16] =
	{
		0x60, 0x70, 0x80, 0x90,
		0xA0, 0xB0, 0xC0, 0xD0,
		0xE0, 0xF0, 0x00, 0x01,
		0x02, 0x03, 0x04, 0x05
	};

	static const uint8 dstSrl256_2[16] =
	{
		0x20, 0x30, 0x40, 0x50,
		0x60, 0x70, 0x80, 0x90,
		0xA0, 0xB0, 0xC0, 0xD0,
		0xE0, 0xF0, 0x00, 0x01
	};

	static const uint8 dstPackHBRes[16] =
	{
		0x00, 0x20, 
		0x40, 0x60,
		0x80, 0xA0,
		0xC0, 0xE0,

		0x00, 0x02,
		0x04, 0x06,
		0x08, 0x0A,
		0x0C, 0x0E,
	};

	static const uint8 dstPackWHRes[16] =
	{
		0x00, 0x10, 
		0x40, 0x50,
		0x80, 0x90,
		0xC0, 0xD0,

		0x00, 0x01,
		0x04, 0x05,
		0x08, 0x09,
		0x0C, 0x0D,
	};

	for(unsigned int i = 0; i < 16; i++)
	{
		TEST_VERIFY(dstMovRes[i]			== context.dstMov[i]);
		TEST_VERIFY(dstSrl256_1[i]			== context.dstSrl256_1[i]);
		TEST_VERIFY(dstSrl256_2[i]			== context.dstSrl256_2[i]);
		TEST_VERIFY(dstPackHBRes[i]			== context.dstPackHB[i]);
		TEST_VERIFY(dstPackWHRes[i]			== context.dstPackWH[i]);
		TEST_VERIFY(dstPackHBRes[i]			== context.dstPackHBAlias[i]);
		TEST_VERIFY(dstPackWHRes[i]			== context.dstPackWHAlias[i]);
	}
}
