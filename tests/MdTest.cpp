#include "MdTest.h"
#include "MemStream.h"

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
		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_AddWSS();
		jitter.MD_PullRel(offsetof(CONTEXT, dstAddWSS));

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
	CONTEXT __declspec(align(16)) context;
	memset(&context, 0, sizeof(CONTEXT));
	
	for(unsigned int i = 0; i < 16; i++)
	{
		context.src0[i] = i;
		context.src1[i] = (i << 4);
	}

	(*m_function)(&context);

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
		TEST_VERIFY(dstUnpackLowerHWRes[i] == context.dstUnpackLowerHW[i]);
		TEST_VERIFY(dstUnpackLowerWDRes[i] == context.dstUnpackLowerWD[i]);
		TEST_VERIFY(dstUnpackUpperWDRes[i] == context.dstUnpackUpperWD[i]);
	}

	for(unsigned int i = 0; i < 4; i++)
	{
		uint32 value = *reinterpret_cast<uint32*>(&context.src1[i * 4]);
		uint32 result = *reinterpret_cast<uint32*>(&context.dstAddWSS[i * 4]);
		TEST_VERIFY(result == ComputeWordSignedSaturation(value, value));
	}
}
