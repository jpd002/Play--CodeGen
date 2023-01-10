#include "RegAllocTempTest.h"
#include "MemStream.h"
#include "Jitter_CodeGen_Wasm.h"

#define TEST_NUMBER1 (0xDEADDEAD)
#define TEST_NUMBER2 (0xCAFECAFE)
#define TEST_NUMBER3 (0xDADADADA)
#define TEST_NUMBER4 (0x5A5A5A5A)
#define TEST_NUMBER5 (0xA5A5A5A5)

extern "C" void RegAllocTempTest_DummyFunction(uint32 value1, uint32 value2, uint32 value3)
{

}

void CRegAllocTempTest::PrepareExternalFunctions()
{
	Jitter::CWasmFunctionRegistry::RegisterFunction(reinterpret_cast<uintptr_t>(&RegAllocTempTest_DummyFunction), "_RegAllocTempTest_DummyFunction", "viii");
}

void CRegAllocTempTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		jitter.PushRel(offsetof(CONTEXT, inValue));
		jitter.PushCst(TEST_NUMBER2);
		jitter.Add();
		uint32 tempCursor = jitter.GetTopCursor();

		jitter.PushCursor(tempCursor);
		jitter.PushCst(TEST_NUMBER3);
		jitter.Add();

		jitter.PushCursor(tempCursor);
		jitter.PushCst(TEST_NUMBER4);
		jitter.Add();

		jitter.PushCursor(tempCursor);
		jitter.PushCst(TEST_NUMBER5);
		jitter.Add();

		jitter.Call(reinterpret_cast<void*>(&RegAllocTempTest_DummyFunction), 3, Jitter::CJitter::RETURN_VALUE_NONE);

		//Temp is still in stack, pulling it into outValue
		jitter.PullRel(offsetof(CONTEXT, outValue));
	}
	jitter.End();

	m_function = FunctionType(codeStream.GetBuffer(), codeStream.GetSize());
}

void CRegAllocTempTest::Run()
{
	m_context.inValue = TEST_NUMBER1;
	m_function(&m_context);
	TEST_VERIFY(m_context.outValue == (TEST_NUMBER1 + TEST_NUMBER2));
}
