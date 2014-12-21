#include "MdUnpackTest.h"
#include "MemStream.h"

void CMdUnpackTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		jitter.MD_PushRel(offsetof(CONTEXT, src0));
		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_UnpackLowerBH();
		jitter.MD_PullRel(offsetof(CONTEXT, dstUnpackLowerBH));

		jitter.MD_PushRel(offsetof(CONTEXT, src0));
		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_UnpackLowerHW();
		jitter.MD_PullRel(offsetof(CONTEXT, dstUnpackLowerHW));

		jitter.MD_PushRel(offsetof(CONTEXT, src0));
		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_UnpackLowerWD();
		jitter.MD_PullRel(offsetof(CONTEXT, dstUnpackLowerWD));

		jitter.MD_PushRel(offsetof(CONTEXT, src0));
		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_UnpackUpperBH();
		jitter.MD_PullRel(offsetof(CONTEXT, dstUnpackUpperBH));

		jitter.MD_PushRel(offsetof(CONTEXT, src0));
		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_UnpackUpperHW();
		jitter.MD_PullRel(offsetof(CONTEXT, dstUnpackUpperHW));

		jitter.MD_PushRel(offsetof(CONTEXT, src0));
		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_UnpackUpperWD();
		jitter.MD_PullRel(offsetof(CONTEXT, dstUnpackUpperWD));
	}
	jitter.End();

	m_function = CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}

void CMdUnpackTest::Run()
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

	static const uint8 dstUnpackLowerBHRes[16] = 
	{
		0x00, 0x00, 
		0x10, 0x01, 
		
		0x20, 0x02, 
		0x30, 0x03, 
		
		0x40, 0x04, 
		0x50, 0x05, 
		
		0x60, 0x06, 
		0x70, 0x07, 
	};

	static const uint8 dstUnpackLowerHWRes[16] = 
	{
		0x00, 0x10, 
		0x00, 0x01, 
		
		0x20, 0x30, 
		0x02, 0x03, 
		
		0x40, 0x50, 
		0x04, 0x05, 
		
		0x60, 0x70, 
		0x06, 0x07, 
	};

	static const uint8 dstUnpackLowerWDRes[16] =
	{
		0x00, 0x10, 0x20, 0x30,
		0x00, 0x01, 0x02, 0x03,

		0x40, 0x50, 0x60, 0x70,
		0x04, 0x05, 0x06, 0x07,
	};

	static const uint8 dstUnpackUpperBHRes[16] = 
	{
		0x80, 0x08, 
		0x90, 0x09, 
		
		0xA0, 0x0A, 
		0xB0, 0x0B, 
		
		0xC0, 0x0C, 
		0xD0, 0x0D, 
		
		0xE0, 0x0E, 
		0xF0, 0x0F, 
	};

	static const uint8 dstUnpackUpperHWRes[16] = 
	{
		0x80, 0x90, 
		0x08, 0x09, 
		
		0xA0, 0xB0, 
		0x0A, 0x0B, 
		
		0xC0, 0xD0, 
		0x0C, 0x0D, 
		
		0xE0, 0xF0, 
		0x0E, 0x0F, 
	};

	static const uint8 dstUnpackUpperWDRes[16] =
	{
		0x80, 0x90, 0xA0, 0xB0,
		0x08, 0x09, 0x0A, 0x0B,

		0xC0, 0xD0, 0xE0, 0xF0,
		0x0C, 0x0D, 0x0E, 0x0F,
	};

	for(unsigned int i = 0; i < 16; i++)
	{
		TEST_VERIFY(dstUnpackLowerBHRes[i]	== context.dstUnpackLowerBH[i]);
		TEST_VERIFY(dstUnpackLowerHWRes[i]	== context.dstUnpackLowerHW[i]);
		TEST_VERIFY(dstUnpackLowerWDRes[i]	== context.dstUnpackLowerWD[i]);
		TEST_VERIFY(dstUnpackUpperBHRes[i]	== context.dstUnpackUpperBH[i]);
		TEST_VERIFY(dstUnpackUpperHWRes[i]	== context.dstUnpackUpperHW[i]);
		TEST_VERIFY(dstUnpackUpperWDRes[i]	== context.dstUnpackUpperWD[i]);
	}
}
