#include "MdAddTest.h"
#include "MemStream.h"

uint32 CMdAddTest::ComputeWordUnsignedSaturation(uint32 value0, uint32 value1)
{
	uint64 value0ext = static_cast<uint32>(value0);
	uint64 value1ext = static_cast<uint32>(value1);
	uint64 result = value0ext + value1ext;
	if(result > 0xFFFFFFFF)
	{
		result = 0xFFFFFFFF;
	}
	return static_cast<uint32>(result);
}

uint32 CMdAddTest::ComputeWordSignedSaturation(uint32 value0, uint32 value1)
{
	int64 value0ext = static_cast<int32>(value0);
	int64 value1ext = static_cast<int32>(value1);
	int64 result = value0ext + value1ext;
	if(result > 0x7FFFFFFF)
	{
		result = 0x7FFFFFFF;
	}
	else if(result < static_cast<int32>(0x80000000))
	{
		result = static_cast<int32>(0x80000000);
	}
	return static_cast<uint32>(result);
}

void CMdAddTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_AddB();
		jitter.MD_PullRel(offsetof(CONTEXT, dstAddB));

		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_AddBUS();
		jitter.MD_PullRel(offsetof(CONTEXT, dstAddBUS));

		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_AddBSS();
		jitter.MD_PullRel(offsetof(CONTEXT, dstAddBSS));

		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_AddH();
		jitter.MD_PullRel(offsetof(CONTEXT, dstAddH));

		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_AddHUS();
		jitter.MD_PullRel(offsetof(CONTEXT, dstAddHUS));

		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_AddHSS();
		jitter.MD_PullRel(offsetof(CONTEXT, dstAddHSS));

		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_AddW();
		jitter.MD_PullRel(offsetof(CONTEXT, dstAddW));

		jitter.MD_PushRel(offsetof(CONTEXT, src2));
		jitter.MD_PushRel(offsetof(CONTEXT, src3));
		jitter.MD_AddWUS();
		jitter.MD_PullRel(offsetof(CONTEXT, dstAddWUS));

		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_AddWSS();
		jitter.MD_PullRel(offsetof(CONTEXT, dstAddWSS));
	}
	jitter.End();

	m_function = CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}

void CMdAddTest::Run()
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

	*reinterpret_cast<uint32*>(&context.src2[0]) = ~0;
	*reinterpret_cast<uint32*>(&context.src3[0]) = 1;

	m_function(&context);

	static const uint8 dstAddBRes[16] =
	{
		0x00, 0x20, 0x40, 0x60,
		0x80, 0xA0, 0xC0, 0xE0,
		0x00, 0x20, 0x40, 0x60,
		0x80, 0xA0, 0xC0, 0xE0,
	};

	static const uint8 dstAddBUSRes[16] =
	{
		0x00, 0x20, 0x40, 0x60,
		0x80, 0xA0, 0xC0, 0xE0,
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,
	};

	static const uint8 dstAddBSSRes[16] =
	{
		0x00, 0x20, 0x40, 0x60,
		0x7F, 0x7F, 0x7F, 0x7F,
		0x80, 0x80, 0x80, 0x80,
		0x80, 0xA0, 0xC0, 0xE0,
	};

	static const uint8 dstAddHRes[16] =
	{
		0x00, 0x20, 
		0x40, 0x60,
		
		0x80, 0xA0, 
		0xC0, 0xE0,
		
		0x00, 0x21, 
		0x40, 0x61,
		
		0x80, 0xA1, 
		0xC0, 0xE1
	};

	static const uint8 dstAddHUSRes[16] =
	{
		0x00, 0x20,
		0x40, 0x60,

		0x80, 0xA0,
		0xC0, 0xE0,

		0xFF, 0xFF,
		0xFF, 0xFF,

		0xFF, 0xFF,
		0xFF, 0xFF,
	};

	static const uint8 dstAddHSSRes[16] =
	{
		0x00, 0x20,
		0x40, 0x60,

		0xFF, 0x7F,
		0xFF, 0x7F,

		0x00, 0x80,
		0x00, 0x80,

		0x80, 0xA1,
		0xC0, 0xE1
	};

	static const uint8 dstAddWRes[16] =
	{
		0x00, 0x20, 0x40, 0x60,
		0x80, 0xA0, 0xC0, 0xE0,
		0x00, 0x21, 0x41, 0x61,
		0x80, 0xA1, 0xC1, 0xE1
	};

	for(unsigned int i = 0; i < 16; i++)
	{
		TEST_VERIFY(dstAddBRes[i]			== context.dstAddB[i]);
		TEST_VERIFY(dstAddBUSRes[i]			== context.dstAddBUS[i]);
		TEST_VERIFY(dstAddBSSRes[i]			== context.dstAddBSS[i]);
		TEST_VERIFY(dstAddHRes[i]			== context.dstAddH[i]);
		TEST_VERIFY(dstAddHUSRes[i]			== context.dstAddHUS[i]);
		TEST_VERIFY(dstAddHSSRes[i]			== context.dstAddHSS[i]);
		TEST_VERIFY(dstAddWRes[i]			== context.dstAddW[i]);
	}

	for(unsigned int i = 0; i < 4; i++)
	{
		uint32 value2 = *reinterpret_cast<uint32*>(&context.src2[i * 4]);
		uint32 value3 = *reinterpret_cast<uint32*>(&context.src3[i * 4]);
		uint32 result = *reinterpret_cast<uint32*>(&context.dstAddWUS[i * 4]);
		TEST_VERIFY(result == ComputeWordUnsignedSaturation(value2, value3));
	}

	for(unsigned int i = 0; i < 4; i++)
	{
		uint32 value = *reinterpret_cast<uint32*>(&context.src1[i * 4]);
		uint32 result = *reinterpret_cast<uint32*>(&context.dstAddWSS[i * 4]);
		TEST_VERIFY(result == ComputeWordSignedSaturation(value, value));
	}
}
