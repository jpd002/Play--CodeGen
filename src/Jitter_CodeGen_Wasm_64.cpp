#include "Jitter_CodeGen_Wasm.h"
#include "WasmDefs.h"
#include "WasmModuleBuilder.h"

using namespace Jitter;

void CCodeGen_Wasm::PushRelative64(CSymbol* symbol)
{
	PushRelativeAddress(symbol);

	m_functionStream.Write8(Wasm::INST_I64_LOAD);
	m_functionStream.Write8(0x03);
	m_functionStream.Write8(0x00);
}

void CCodeGen_Wasm::Emit_Mov64_MemAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	PrepareSymbolDef(dst);
	PrepareSymbolUse(src1);

	CommitSymbol(dst);
}

void CCodeGen_Wasm::Emit_Add64_MemAnyAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	PrepareSymbolDef(dst);
	PrepareSymbolUse(src1);
	PrepareSymbolUse(src2);

	m_functionStream.Write8(Wasm::INST_I64_ADD);

	CommitSymbol(dst);
}

void CCodeGen_Wasm::Emit_Sub64_MemAnyAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	PrepareSymbolDef(dst);
	PrepareSymbolUse(src1);
	PrepareSymbolUse(src2);

	m_functionStream.Write8(Wasm::INST_I64_SUB);

	CommitSymbol(dst);
}

void CCodeGen_Wasm::Emit_Cmp64_MemAnyAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	PrepareSymbolDef(dst);
	PrepareSymbolUse(src1);
	PrepareSymbolUse(src2);

	switch(statement.jmpCondition)
	{
	case CONDITION_EQ:
		m_functionStream.Write8(Wasm::INST_I64_EQ);
		break;
	case CONDITION_NE:
		m_functionStream.Write8(Wasm::INST_I64_NE);
		break;
	case CONDITION_BL:
		m_functionStream.Write8(Wasm::INST_I64_LT_U);
		break;
	case CONDITION_AB:
		m_functionStream.Write8(Wasm::INST_I64_GT_U);
		break;
	case CONDITION_LT:
		m_functionStream.Write8(Wasm::INST_I64_LT_S);
		break;
	case CONDITION_LE:
		m_functionStream.Write8(Wasm::INST_I64_LE_S);
		break;
	case CONDITION_GT:
		m_functionStream.Write8(Wasm::INST_I64_GT_S);
		break;
	default:
		assert(false);
		break;
	}

	CommitSymbol(dst);
}

void CCodeGen_Wasm::Emit_Load64FromRef_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	PrepareSymbolDef(dst);
	PrepareSymbolUse(src1);

	m_functionStream.Write8(Wasm::INST_I64_LOAD);
	m_functionStream.Write8(0x03);
	m_functionStream.Write8(0x00);

	CommitSymbol(dst);
}

void CCodeGen_Wasm::Emit_Store64AtRef_MemAny(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	PrepareSymbolUse(src1);
	PrepareSymbolUse(src2);

	m_functionStream.Write8(Wasm::INST_I64_STORE);
	m_functionStream.Write8(0x03);
	m_functionStream.Write8(0x00);
}

CCodeGen_Wasm::CONSTMATCHER CCodeGen_Wasm::g_64ConstMatchers[] =
{
	{ OP_MOV,            MATCH_MEMORY64,       MATCH_MEMORY64,       MATCH_NIL,           MATCH_NIL, &CCodeGen_Wasm::Emit_Mov64_MemAny                        },
	{ OP_MOV,            MATCH_MEMORY64,       MATCH_CONSTANT64,     MATCH_NIL,           MATCH_NIL, &CCodeGen_Wasm::Emit_Mov64_MemAny                        },

	{ OP_ADD64,          MATCH_MEMORY64,       MATCH_MEMORY64,       MATCH_MEMORY64,      MATCH_NIL, &CCodeGen_Wasm::Emit_Add64_MemAnyAny                     },
	{ OP_ADD64,          MATCH_MEMORY64,       MATCH_MEMORY64,       MATCH_CONSTANT64,    MATCH_NIL, &CCodeGen_Wasm::Emit_Add64_MemAnyAny                     },

	{ OP_SUB64,          MATCH_MEMORY64,       MATCH_MEMORY64,       MATCH_MEMORY64,      MATCH_NIL, &CCodeGen_Wasm::Emit_Sub64_MemAnyAny                     },
	{ OP_SUB64,          MATCH_MEMORY64,       MATCH_MEMORY64,       MATCH_CONSTANT64,    MATCH_NIL, &CCodeGen_Wasm::Emit_Sub64_MemAnyAny                     },
	{ OP_SUB64,          MATCH_MEMORY64,       MATCH_CONSTANT64,     MATCH_MEMORY64,      MATCH_NIL, &CCodeGen_Wasm::Emit_Sub64_MemAnyAny                     },

	{ OP_CMP64,          MATCH_MEMORY,         MATCH_MEMORY64,       MATCH_MEMORY64,      MATCH_NIL, &CCodeGen_Wasm::Emit_Cmp64_MemAnyAny                     },
	{ OP_CMP64,          MATCH_MEMORY,         MATCH_MEMORY64,       MATCH_CONSTANT64,    MATCH_NIL, &CCodeGen_Wasm::Emit_Cmp64_MemAnyAny                     },

	{ OP_LOADFROMREF,    MATCH_MEMORY64,       MATCH_MEM_REF,        MATCH_NIL,           MATCH_NIL, &CCodeGen_Wasm::Emit_Load64FromRef_MemMem                },
	{ OP_STOREATREF,     MATCH_NIL,            MATCH_MEM_REF,        MATCH_MEMORY64,      MATCH_NIL, &CCodeGen_Wasm::Emit_Store64AtRef_MemAny                 },
	{ OP_STOREATREF,     MATCH_NIL,            MATCH_MEM_REF,        MATCH_CONSTANT64,    MATCH_NIL, &CCodeGen_Wasm::Emit_Store64AtRef_MemAny                 },

	{ OP_MOV,            MATCH_NIL,            MATCH_NIL,            MATCH_NIL,           MATCH_NIL, nullptr                                                  },
};
