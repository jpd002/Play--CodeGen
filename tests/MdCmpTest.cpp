#include "MdCmpTest.h"
#include "MemStream.h"

void CMdCmpTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		jitter.MD_PushRel(offsetof(CONTEXT, src0));
		jitter.MD_PushRel(offsetof(CONTEXT, src2));
		jitter.MD_CmpEqW();
		jitter.MD_PullRel(offsetof(CONTEXT, dstCmpEqW));

		jitter.MD_PushRel(offsetof(CONTEXT, src0));
		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_CmpGtH();
		jitter.MD_PullRel(offsetof(CONTEXT, dstCmpGtH));
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
	}

	m_function(&context);

	static const uint8 dstCmpEqWRes[16] =
	{
		0xFF, 0xFF, 0xFF, 0xFF,
		0x00, 0x00, 0x00, 0x00,
		0xFF, 0xFF, 0xFF, 0xFF,
		0x00, 0x00, 0x00, 0x00,
	};

	static const uint8 dstCmpGtHRes[16] =
	{
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,
	};

	for(unsigned int i = 0; i < 16; i++)
	{
		TEST_VERIFY(dstCmpEqWRes[i]			== context.dstCmpEqW[i]);
		TEST_VERIFY(dstCmpGtHRes[i]			== context.dstCmpGtH[i]);
	}
}
