#include "MdTest.h"
#include "MemStream.h"

#ifdef _MSC_VER
#define ALIGN16 __declspec(align(16))
#else
#define ALIGN16 __attribute__((aligned(16)))
#endif

CMdTest::CMdTest()
{

}

CMdTest::~CMdTest()
{
	delete m_function;
}

void CMdTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_PullRel(offsetof(CONTEXT, dstMov));

		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_AddB();
		jitter.MD_PullRel(offsetof(CONTEXT, dstAddB));

		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_AddH();
		jitter.MD_PullRel(offsetof(CONTEXT, dstAddH));

		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_AddW();
		jitter.MD_PullRel(offsetof(CONTEXT, dstAddW));

		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_AddWUS();
		jitter.MD_PullRel(offsetof(CONTEXT, dstAddWUS));

		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_AddWSS();
		jitter.MD_PullRel(offsetof(CONTEXT, dstAddWSS));

		jitter.MD_PushRel(offsetof(CONTEXT, src0));
		jitter.MD_PushRel(offsetof(CONTEXT, src2));
		jitter.MD_CmpEqW();
		jitter.MD_PullRel(offsetof(CONTEXT, dstCmpEqW));

		//Shifts
		jitter.MD_PushRel(offsetof(CONTEXT, src0));
		jitter.MD_SrlH(9);
		jitter.MD_PullRel(offsetof(CONTEXT, dstSrlH));

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
		jitter.MD_PullRel(offsetof(CONTEXT, dstSrl256));

		//Packs
		jitter.MD_PushRel(offsetof(CONTEXT, src0));
		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_PackHB();
		jitter.MD_PullRel(offsetof(CONTEXT, dstPackHB));

		jitter.MD_PushRel(offsetof(CONTEXT, src0));
		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_PackWH();
		jitter.MD_PullRel(offsetof(CONTEXT, dstPackWH));

		//Unpacks
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
		jitter.MD_UnpackUpperWD();
		jitter.MD_PullRel(offsetof(CONTEXT, dstUnpackUpperWD));
	}
	jitter.End();

	m_function = new CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}

uint32 CMdTest::ComputeWordUnsignedSaturation(uint32 value0, uint32 value1)
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

uint32 CMdTest::ComputeWordSignedSaturation(uint32 value0, uint32 value1)
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
	}

	(*m_function)(&context);

	static const uint8 dstMovRes[16] =
	{
		0x00, 0x10, 0x20, 0x30,
		0x40, 0x50, 0x60, 0x70,
		0x80, 0x90, 0xA0, 0xB0,
		0xC0, 0xD0, 0xE0, 0xF0
	};

	static const uint8 dstAddBRes[16] =
	{
		0x00, 0x20, 0x40, 0x60,
		0x80, 0xA0, 0xC0, 0xE0,
		0x00, 0x20, 0x40, 0x60,
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

	static const uint8 dstAddWRes[16] =
	{
		0x00, 0x20, 0x40, 0x60,
		0x80, 0xA0, 0xC0, 0xE0,
		0x00, 0x21, 0x41, 0x61,
		0x80, 0xA1, 0xC1, 0xE1
	};

	static const uint8 dstCmpEqWRes[16] =
	{
		0xFF, 0xFF, 0xFF, 0xFF,
		0x00, 0x00, 0x00, 0x00,
		0xFF, 0xFF, 0xFF, 0xFF,
		0x00, 0x00, 0x00, 0x00,
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

	static const uint8 dstSrl256[16] =
	{
		0x60, 0x70, 0x80, 0x90,
		0xA0, 0xB0, 0xC0, 0xD0,
		0xE0, 0xF0, 0x00, 0x01,
		0x02, 0x03, 0x04, 0x05
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

	static const uint8 dstUnpackUpperWDRes[16] =
	{
		0x80, 0x90, 0xA0, 0xB0,
		0x08, 0x09, 0x0A, 0x0B,

		0xC0, 0xD0, 0xE0, 0xF0,
		0x0C, 0x0D, 0x0E, 0x0F,
	};

	for(unsigned int i = 0; i < 16; i++)
	{
		TEST_VERIFY(dstMovRes[i]			== context.dstMov[i]);
		TEST_VERIFY(dstAddBRes[i]			== context.dstAddB[i]);
		TEST_VERIFY(dstAddHRes[i]			== context.dstAddH[i]);
		TEST_VERIFY(dstAddWRes[i]			== context.dstAddW[i]);
		TEST_VERIFY(dstCmpEqWRes[i]			== context.dstCmpEqW[i]);
		TEST_VERIFY(dstSrlH[i]				== context.dstSrlH[i]);
		TEST_VERIFY(dstSrlW[i]				== context.dstSrlW[i]);
		TEST_VERIFY(dstSraW[i]				== context.dstSraW[i]);
		TEST_VERIFY(dstSllW[i]				== context.dstSllW[i]);
		TEST_VERIFY(dstSrl256[i]			== context.dstSrl256[i]);
		TEST_VERIFY(dstPackHBRes[i]			== context.dstPackHB[i]);
		TEST_VERIFY(dstPackWHRes[i]			== context.dstPackWH[i]);
		TEST_VERIFY(dstUnpackLowerBHRes[i]	== context.dstUnpackLowerBH[i]);
		TEST_VERIFY(dstUnpackLowerHWRes[i]	== context.dstUnpackLowerHW[i]);
		TEST_VERIFY(dstUnpackLowerWDRes[i]	== context.dstUnpackLowerWD[i]);
		TEST_VERIFY(dstUnpackUpperWDRes[i]	== context.dstUnpackUpperWD[i]);
	}

	for(unsigned int i = 0; i < 4; i++)
	{
		uint32 value = *reinterpret_cast<uint32*>(&context.src1[i * 4]);
		uint32 result = *reinterpret_cast<uint32*>(&context.dstAddWUS[i * 4]);
		TEST_VERIFY(result == ComputeWordUnsignedSaturation(value, value));
	}

	for(unsigned int i = 0; i < 4; i++)
	{
		uint32 value = *reinterpret_cast<uint32*>(&context.src1[i * 4]);
		uint32 result = *reinterpret_cast<uint32*>(&context.dstAddWSS[i * 4]);
		TEST_VERIFY(result == ComputeWordSignedSaturation(value, value));
	}
}
