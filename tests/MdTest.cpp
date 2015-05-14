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
		jitter.MD_SrlH(9);
		jitter.MD_PullRel(offsetof(CONTEXT, dstSrlH));

		jitter.MD_PushRel(offsetof(CONTEXT, src0));
		jitter.MD_SllH(12);
		jitter.MD_PullRel(offsetof(CONTEXT, dstSllH));

		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_SraH(8);
		jitter.MD_PullRel(offsetof(CONTEXT, dstSraH));

		jitter.MD_PushRel(offsetof(CONTEXT, src0));
		jitter.MD_SrlW(15);
		jitter.MD_PullRel(offsetof(CONTEXT, dstSrlW));

		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_SraW(12);
		jitter.MD_PullRel(offsetof(CONTEXT, dstSraW));

		jitter.MD_PushRel(offsetof(CONTEXT, src0));
		jitter.MD_SllW(12);
		jitter.MD_PullRel(offsetof(CONTEXT, dstSllW));

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
	context.shiftAmount = 16;

	m_function(&context);

	static const uint8 dstMovRes[16] =
	{
		0x00, 0x10, 0x20, 0x30,
		0x40, 0x50, 0x60, 0x70,
		0x80, 0x90, 0xA0, 0xB0,
		0xC0, 0xD0, 0xE0, 0xF0
	};

	static const uint8 dstSrlH[16] =
	{
		0x00, 0x00,
		0x01, 0x00,
		0x02, 0x00,
		0x03, 0x00,
		0x04, 0x00,
		0x05, 0x00,
		0x06, 0x00,
		0x07, 0x00,
	};

	static const uint8 dstSraH[16] =
	{
		0x10, 0x00,		//0x1000 >> 8
		0x30, 0x00,		//0x3020
		0x50, 0x00,		//0x5040
		0x70, 0x00,		//0x7060
		0x90, 0xFF,		//0x9080
		0xB0, 0xFF,		//0xB0A0
		0xD0, 0xFF,		//0xD0C0
		0xF0, 0xFF,		//0xF0E0
	};

	static const uint8 dstSllH[16] =
	{
		0x00, 0x00,		//0x0100 << 12
		0x00, 0x20,		//0x0302
		0x00, 0x40,		//0x0504
		0x00, 0x60,		//0x0706
		0x00, 0x80,		//0x0908
		0x00, 0xA0,		//0x0B0A
		0x00, 0xC0,		//0x0D0C
		0x00, 0xE0,		//0x0F0E
	};

	static const uint8 dstSrlW[16] =
	{
		0x04, 0x06, 0x00, 0x00,
		0x0C, 0x0E, 0x00, 0x00,
		0x14, 0x16, 0x00, 0x00,
		0x1C, 0x1E, 0x00, 0x00,
	};

	static const uint8 dstSraW[16] =
	{
		0x01, 0x02, 0x03, 0x00,
		0x05, 0x06, 0x07, 0x00,
		0x09, 0x0A, 0xFB, 0xFF,
		0x0D, 0x0E, 0xFF, 0xFF,
	};

	static const uint8 dstSllW[16] =
	{
		0x00, 0x00, 0x10, 0x20,
		0x00, 0x40, 0x50, 0x60,
		0x00, 0x80, 0x90, 0xA0,
		0x00, 0xC0, 0xD0, 0xE0
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
		TEST_VERIFY(dstSrlH[i]				== context.dstSrlH[i]);
		TEST_VERIFY(dstSraH[i]				== context.dstSraH[i]);
		TEST_VERIFY(dstSllH[i]				== context.dstSllH[i]);
		TEST_VERIFY(dstSrlW[i]				== context.dstSrlW[i]);
		TEST_VERIFY(dstSraW[i]				== context.dstSraW[i]);
		TEST_VERIFY(dstSllW[i]				== context.dstSllW[i]);
		TEST_VERIFY(dstSrl256_1[i]			== context.dstSrl256_1[i]);
		TEST_VERIFY(dstSrl256_2[i]			== context.dstSrl256_2[i]);
		TEST_VERIFY(dstPackHBRes[i]			== context.dstPackHB[i]);
		TEST_VERIFY(dstPackWHRes[i]			== context.dstPackWH[i]);
	}
}
