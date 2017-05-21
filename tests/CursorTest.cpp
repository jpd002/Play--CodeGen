#include "CursorTest.h"
#include "MemStream.h"

#define VALUE_1 (10)
#define VALUE_2 (20)
#define VALUE_3 (30)
#define VALUE_4 (40)
#define VALUE_5 (50)
#define VALUE_6 (60)

void CCursorTest::Run()
{
	CONTEXT context;
	m_function(&context);

	TEST_VERIFY(context.result1 == (VALUE_2 - VALUE_4));
}

void CCursorTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		jitter.PushCst(VALUE_1); uint32 value1Cursor = jitter.GetTopCursor();
		jitter.PushCst(VALUE_2); uint32 value2Cursor = jitter.GetTopCursor();
		jitter.PushCst(VALUE_3); uint32 value3Cursor = jitter.GetTopCursor();
		jitter.PushCst(VALUE_4); uint32 value4Cursor = jitter.GetTopCursor();
		jitter.PushCst(VALUE_5); uint32 value5Cursor = jitter.GetTopCursor();
		jitter.PushCst(VALUE_6); uint32 value6Cursor = jitter.GetTopCursor();

		jitter.PushCursor(value2Cursor);
		jitter.PushCursor(value4Cursor);
		jitter.Sub();
		jitter.PullRel(offsetof(CONTEXT, result1));

		assert(jitter.GetTopCursor() == value6Cursor); jitter.PullTop();
		assert(jitter.GetTopCursor() == value5Cursor); jitter.PullTop();
		assert(jitter.GetTopCursor() == value4Cursor); jitter.PullTop();
		assert(jitter.GetTopCursor() == value3Cursor); jitter.PullTop();
		assert(jitter.GetTopCursor() == value2Cursor); jitter.PullTop();
		assert(jitter.GetTopCursor() == value1Cursor); jitter.PullTop();
	}
	jitter.End();

	m_function = CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}
