#include "Jitter_CodeGen_Wasm.h"
#include "WasmDefs.h"
#include "WasmModuleBuilder.h"

using namespace Jitter;

template <uint32 OP>
void CCodeGen_Wasm::Emit_Md_MemMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	PushRelativeAddress(dst);

	PushRelative128(src1);
	PushRelative128(src2);

	m_functionStream.Write8(Wasm::INST_PREFIX_SIMD);
	CWasmModuleBuilder::WriteULeb128(m_functionStream, OP);

	m_functionStream.Write8(Wasm::INST_PREFIX_SIMD);
	m_functionStream.Write8(Wasm::INST_V128_STORE);
	m_functionStream.Write8(0x04);
	m_functionStream.Write8(0x00);
}

void CCodeGen_Wasm::PushRelative128(CSymbol* symbol)
{
	PushRelativeAddress(symbol);

	m_functionStream.Write8(Wasm::INST_PREFIX_SIMD);
	m_functionStream.Write8(Wasm::INST_V128_LOAD);
	m_functionStream.Write8(0x04);
	m_functionStream.Write8(0x00);
}

void CCodeGen_Wasm::Emit_Md_Mov_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	PushRelativeAddress(dst);
	PushRelative128(src1);

	m_functionStream.Write8(Wasm::INST_PREFIX_SIMD);
	m_functionStream.Write8(Wasm::INST_V128_STORE);
	m_functionStream.Write8(0x04);
	m_functionStream.Write8(0x00);
}

void CCodeGen_Wasm::Emit_Md_LoadFromRef_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	PushRelativeAddress(dst);
	PrepareSymbolUse(src1);

	m_functionStream.Write8(Wasm::INST_PREFIX_SIMD);
	m_functionStream.Write8(Wasm::INST_V128_LOAD);
	m_functionStream.Write8(0x04);
	m_functionStream.Write8(0x00);

	m_functionStream.Write8(Wasm::INST_PREFIX_SIMD);
	m_functionStream.Write8(Wasm::INST_V128_STORE);
	m_functionStream.Write8(0x04);
	m_functionStream.Write8(0x00);
}

void CCodeGen_Wasm::Emit_Md_StoreAtRef_MemMem(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	PrepareSymbolUse(src1);
	PushRelative128(src2);

	m_functionStream.Write8(Wasm::INST_PREFIX_SIMD);
	m_functionStream.Write8(Wasm::INST_V128_STORE);
	m_functionStream.Write8(0x04);
	m_functionStream.Write8(0x00);
}

CCodeGen_Wasm::CONSTMATCHER CCodeGen_Wasm::g_mdConstMatchers[] =
{
	{ OP_MOV,            MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_NIL,           MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_Mov_MemMem                            },

	{ OP_MD_ADD_B,       MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_MemMemMem<Wasm::INST_I8x16_ADD>       },

	{ OP_MD_ADDSS_B,     MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_MemMemMem<Wasm::INST_I8x16_ADD_SAT_S> },

	{ OP_MD_ADDUS_B,     MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_MemMemMem<Wasm::INST_I8x16_ADD_SAT_U> },

	{ OP_LOADFROMREF,    MATCH_MEMORY128,      MATCH_MEM_REF,        MATCH_NIL,           MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_LoadFromRef_MemMem                    },
	{ OP_STOREATREF,     MATCH_NIL,            MATCH_MEM_REF,        MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_StoreAtRef_MemMem                     },

	{ OP_MOV,            MATCH_NIL,            MATCH_NIL,            MATCH_NIL,           MATCH_NIL,      nullptr                                                       },
};
