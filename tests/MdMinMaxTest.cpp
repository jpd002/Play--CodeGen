#include "MdMinMaxTest.h"
#include "MemStream.h"

void CMdMinMaxTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		jitter.MD_PushRel(offsetof(CONTEXT, src0));
		jitter.MD_PushRel(offsetof(CONTEXT, src2));
		jitter.MD_MinH();
		jitter.MD_PullRel(offsetof(CONTEXT, dstMinH));

		jitter.MD_PushRel(offsetof(CONTEXT, src0));
		jitter.MD_PushRel(offsetof(CONTEXT, src2));
		jitter.MD_MinW();
		jitter.MD_PullRel(offsetof(CONTEXT, dstMinW));

		jitter.MD_PushRel(offsetof(CONTEXT, src0));
		jitter.MD_PushRel(offsetof(CONTEXT, src2));
		jitter.MD_MaxH();
		jitter.MD_PullRel(offsetof(CONTEXT, dstMaxH));

		jitter.MD_PushRel(offsetof(CONTEXT, src0));
		jitter.MD_PushRel(offsetof(CONTEXT, src2));
		jitter.MD_MaxW();
		jitter.MD_PullRel(offsetof(CONTEXT, dstMaxW));
	}
	jitter.End();

	m_function = CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}

void CMdMinMaxTest::Run()
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

	static const uint8 dstMinHRes[16] =
	{
		0x00, 0x01,
		0x02, 0x03,
		0x04, 0x05,
		0x06, 0x07,
		0x08, 0x09,
		0x0A, 0x0B,
		0xC0, 0xD0,
		0xE0, 0xF0,
	};

	static const uint8 dstMinWRes[16] =
	{
		0x00, 0x01,
		0x02, 0x03,
		0x04, 0x05,
		0x06, 0x07,
		0x08, 0x09,
		0x0A, 0x0B,
		0xC0, 0xD0,
		0xE0, 0xF0,
	};

	static const uint8 dstMaxHRes[16] =
	{
		0x00, 0x01,
		0x02, 0x03,
		0x40, 0x50,
		0x60, 0x70,
		0x08, 0x09,
		0x0A, 0x0B,
		0x0C, 0x0D,
		0x0E, 0x0F,
	};

	static const uint8 dstMaxWRes[16] =
	{
		0x00, 0x01,
		0x02, 0x03,
		0x40, 0x50,
		0x60, 0x70,
		0x08, 0x09,
		0x0A, 0x0B,
		0x0C, 0x0D,
		0x0E, 0x0F,
	};

	for(unsigned int i = 0; i < 16; i++)
	{
		TEST_VERIFY(dstMinHRes[i]			== context.dstMinH[i]);
		TEST_VERIFY(dstMinWRes[i]			== context.dstMinW[i]);
		TEST_VERIFY(dstMaxHRes[i]			== context.dstMaxH[i]);
		TEST_VERIFY(dstMaxWRes[i]			== context.dstMaxW[i]);
	}
}
