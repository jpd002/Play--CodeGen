#include "Jitter_CodeGen_Wasm.h"
#include "WasmDefs.h"
#include "WasmModuleBuilder.h"

using namespace Jitter;

template <uint32 OP>
void CCodeGen_Wasm::Emit_Fpu_MemMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	PushRelativeAddress(dst);

	PushRelativeSingle(src1);
	PushRelativeSingle(src2);

	m_functionStream.Write8(OP);

	m_functionStream.Write8(Wasm::INST_F32_STORE);
	m_functionStream.Write8(0x02);
	m_functionStream.Write8(0x00);
}

void CCodeGen_Wasm::PushRelativeSingle(CSymbol* symbol)
{
	PushRelativeAddress(symbol);

	m_functionStream.Write8(Wasm::INST_F32_LOAD);
	m_functionStream.Write8(0x02);
	m_functionStream.Write8(0x00);
}

CCodeGen_Wasm::CONSTMATCHER CCodeGen_Wasm::g_fpuConstMatchers[] =
{
	{ OP_FP_ADD,         MATCH_MEMORY_FP_SINGLE,      MATCH_MEMORY_FP_SINGLE,  MATCH_MEMORY_FP_SINGLE,  MATCH_NIL,      &CCodeGen_Wasm::Emit_Fpu_MemMemMem<Wasm::INST_F32_ADD>       },

	{ OP_MOV,            MATCH_NIL,                   MATCH_NIL,               MATCH_NIL,               MATCH_NIL,      nullptr                                                      },
};
