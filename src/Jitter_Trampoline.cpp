#include "Jitter_Trampoline.h"
#include "Jitter.h"
#include "Jitter_CodeGenFactory.h"

#include "MemStream.h"

Jitter::CJitter_Trampoline::CJitter_Trampoline()
{
	SetupTrumpoline();
}

void Jitter::CJitter_Trampoline::SetupTrumpoline()
{
	Jitter::CCodeGen* codeGen = Jitter::CreateCodeGen();
	codeGen->SetTrumpoline(true);
	Jitter::CJitter jitter(codeGen);

	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		jitter.PushRel64(offsetof(CONTEXT, context));
		jitter.CallRel64(offsetof(CONTEXT, code), 1);
	}
	jitter.End();
	codeGen->SetTrumpoline(false);

	m_function = CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}

void Jitter::CJitter_Trampoline::Trampoline(void* context, void* code)
{
	m_context.context = context;
	m_context.code = code;
	m_function(&m_context, true);
}
