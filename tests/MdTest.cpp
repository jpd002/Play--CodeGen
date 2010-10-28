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
		jitter.MD_PushRel(offsetof(CONTEXT, src0));
		jitter.MD_PushRel(offsetof(CONTEXT, src1));
		jitter.MD_UnpackLowerHW();
		jitter.MD_PullRel(offsetof(CONTEXT, dst0));
	}
	jitter.End();

	m_function = new CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
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

	static const uint8 dst0res[16] = 
	{
		0x00, 0x10, 0x00, 0x01, 
		0x20, 0x30, 0x02, 0x03, 
		0x40, 0x50, 0x04, 0x05, 
		0x60, 0x70, 0x06, 0x07, 
	};

	for(unsigned int i = 0; i < 16; i++)
	{
		TEST_VERIFY(dst0res[i] == context.dst0[i]);
	}
}
