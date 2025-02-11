#include "Jitter_CodeGen_Wasm.h"
#include "WasmDefs.h"
#include "WasmModuleBuilder.h"

using namespace Jitter;

#include "Jitter_CodeGen_Wasm_LoadStore.h"

template <uint32 OP>
void CCodeGen_Wasm::Emit_Alu64_MemAnyAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	PrepareSymbolDef(dst);
	PrepareSymbolUse(src1);
	PrepareSymbolUse(src2);

	m_functionStream.Write8(OP);

	CommitSymbol(dst);
}

template <uint32 OP>
void CCodeGen_Wasm::Emit_Shift64_MemAnyAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	PrepareSymbolDef(dst);
	PrepareSymbolUse(src1);
	PrepareSymbolUse(src2);

	m_functionStream.Write8(Wasm::INST_I64_EXTEND_I32_U);
	m_functionStream.Write8(OP);

	CommitSymbol(dst);
}

void CCodeGen_Wasm::PushRelative64(CSymbol* symbol)
{
	PushRelativeAddress(symbol);

	m_functionStream.Write8(Wasm::INST_I64_LOAD);
	m_functionStream.Write8(0x03);
	m_functionStream.Write8(0x00);
}

void CCodeGen_Wasm::PushTemporary64(CSymbol* symbol)
{
	assert(symbol->m_type == SYM_TEMPORARY64);

	uint32 localIdx = GetTemporaryLocation(symbol);

	m_functionStream.Write8(Wasm::INST_LOCAL_GET);
	CWasmModuleBuilder::WriteULeb128(m_functionStream, localIdx);
}

void CCodeGen_Wasm::PullTemporary64(CSymbol* symbol)
{
	assert(symbol->m_type == SYM_TEMPORARY64);

	uint32 localIdx = GetTemporaryLocation(symbol);

	m_functionStream.Write8(Wasm::INST_LOCAL_SET);
	CWasmModuleBuilder::WriteULeb128(m_functionStream, localIdx);
}

void CCodeGen_Wasm::Emit_Mov64_MemAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	PrepareSymbolDef(dst);
	PrepareSymbolUse(src1);

	CommitSymbol(dst);
}

void CCodeGen_Wasm::Emit_MergeTo64_Mem64AnyAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	PrepareSymbolDef(dst);

	//Lower Part
	PrepareSymbolUse(src1);

	m_functionStream.Write8(Wasm::INST_I64_EXTEND_I32_U);

	//Upper Part
	PrepareSymbolUse(src2);

	m_functionStream.Write8(Wasm::INST_I64_EXTEND_I32_U);

	m_functionStream.Write8(Wasm::INST_I64_CONST);
	CWasmModuleBuilder::WriteSLeb128(m_functionStream, 32);

	m_functionStream.Write8(Wasm::INST_I64_SHL);

	//Combine
	m_functionStream.Write8(Wasm::INST_I64_OR);

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

void CCodeGen_Wasm::Emit_RetVal_Tmp64(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	PullTemporary64(dst);
}

// clang-format off
CCodeGen_Wasm::CONSTMATCHER CCodeGen_Wasm::g_64ConstMatchers[] =
{
	{ OP_MOV,            MATCH_MEMORY64,       MATCH_MEMORY64,       MATCH_NIL,           MATCH_NIL, &CCodeGen_Wasm::Emit_Mov64_MemAny                        },
	{ OP_MOV,            MATCH_MEMORY64,       MATCH_CONSTANT64,     MATCH_NIL,           MATCH_NIL, &CCodeGen_Wasm::Emit_Mov64_MemAny                        },

	{ OP_MERGETO64,      MATCH_MEMORY64,       MATCH_ANY,            MATCH_ANY,           MATCH_NIL, &CCodeGen_Wasm::Emit_MergeTo64_Mem64AnyAny               },

	{ OP_ADD64,          MATCH_MEMORY64,       MATCH_MEMORY64,       MATCH_MEMORY64,      MATCH_NIL, &CCodeGen_Wasm::Emit_Alu64_MemAnyAny<Wasm::INST_I64_ADD> },
	{ OP_ADD64,          MATCH_MEMORY64,       MATCH_MEMORY64,       MATCH_CONSTANT64,    MATCH_NIL, &CCodeGen_Wasm::Emit_Alu64_MemAnyAny<Wasm::INST_I64_ADD> },

	{ OP_SUB64,          MATCH_MEMORY64,       MATCH_MEMORY64,       MATCH_MEMORY64,      MATCH_NIL, &CCodeGen_Wasm::Emit_Alu64_MemAnyAny<Wasm::INST_I64_SUB> },
	{ OP_SUB64,          MATCH_MEMORY64,       MATCH_MEMORY64,       MATCH_CONSTANT64,    MATCH_NIL, &CCodeGen_Wasm::Emit_Alu64_MemAnyAny<Wasm::INST_I64_SUB> },
	{ OP_SUB64,          MATCH_MEMORY64,       MATCH_CONSTANT64,     MATCH_MEMORY64,      MATCH_NIL, &CCodeGen_Wasm::Emit_Alu64_MemAnyAny<Wasm::INST_I64_SUB> },

	{ OP_AND64,          MATCH_MEMORY64,       MATCH_MEMORY64,       MATCH_MEMORY64,      MATCH_NIL, &CCodeGen_Wasm::Emit_Alu64_MemAnyAny<Wasm::INST_I64_AND> },

	{ OP_SLL64,          MATCH_MEMORY64,       MATCH_MEMORY64,       MATCH_ANY,           MATCH_NIL, &CCodeGen_Wasm::Emit_Shift64_MemAnyAny<Wasm::INST_I64_SHL>     },

	{ OP_SRL64,          MATCH_MEMORY64,       MATCH_MEMORY64,       MATCH_ANY,           MATCH_NIL, &CCodeGen_Wasm::Emit_Shift64_MemAnyAny<Wasm::INST_I64_SHR_U>   },

	{ OP_SRA64,          MATCH_MEMORY64,       MATCH_MEMORY64,       MATCH_ANY,           MATCH_NIL, &CCodeGen_Wasm::Emit_Shift64_MemAnyAny<Wasm::INST_I64_SHR_S>   },

	{ OP_CMP64,          MATCH_MEMORY,         MATCH_MEMORY64,       MATCH_MEMORY64,      MATCH_NIL, &CCodeGen_Wasm::Emit_Cmp64_MemAnyAny                     },
	{ OP_CMP64,          MATCH_MEMORY,         MATCH_MEMORY64,       MATCH_CONSTANT64,    MATCH_NIL, &CCodeGen_Wasm::Emit_Cmp64_MemAnyAny                     },

	{ OP_LOADFROMREF,    MATCH_MEMORY64,       MATCH_MEM_REF,        MATCH_NIL,           MATCH_NIL, &CCodeGen_Wasm::Emit_Generic_LoadFromRef_MemVar<Wasm::INST_I64_LOAD, 3>    },
	{ OP_LOADFROMREF,    MATCH_MEMORY64,       MATCH_MEM_REF,        MATCH_ANY32,         MATCH_NIL, &CCodeGen_Wasm::Emit_Generic_LoadFromRef_MemVarAny<Wasm::INST_I64_LOAD, 3> },

	{ OP_STOREATREF,     MATCH_NIL,            MATCH_MEM_REF,        MATCH_MEMORY64,      MATCH_NIL,        &CCodeGen_Wasm::Emit_Generic_StoreAtRef_VarAny<Wasm::INST_I64_STORE, 3>    },
	{ OP_STOREATREF,     MATCH_NIL,            MATCH_MEM_REF,        MATCH_CONSTANT64,    MATCH_NIL,        &CCodeGen_Wasm::Emit_Generic_StoreAtRef_VarAny<Wasm::INST_I64_STORE, 3>    },
	{ OP_STOREATREF,     MATCH_NIL,            MATCH_MEM_REF,        MATCH_ANY32,         MATCH_MEMORY64,   &CCodeGen_Wasm::Emit_Generic_StoreAtRef_VarAnyAny<Wasm::INST_I64_STORE, 3> },
	{ OP_STOREATREF,     MATCH_NIL,            MATCH_MEM_REF,        MATCH_ANY32,         MATCH_CONSTANT64, &CCodeGen_Wasm::Emit_Generic_StoreAtRef_VarAnyAny<Wasm::INST_I64_STORE, 3> },

	{ OP_RETVAL,         MATCH_TEMPORARY64,    MATCH_NIL,            MATCH_NIL,           MATCH_NIL, &CCodeGen_Wasm::Emit_RetVal_Tmp64                        },

	{ OP_MOV,            MATCH_NIL,            MATCH_NIL,            MATCH_NIL,           MATCH_NIL, nullptr                                                  },
};
// clang-format on
