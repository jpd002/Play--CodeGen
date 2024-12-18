#include "Jitter_CodeGen_Wasm.h"
#include "WasmDefs.h"
#include "WasmModuleBuilder.h"

using namespace Jitter;

template <uint32 OP>
void CCodeGen_Wasm::Emit_Fpu_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	PrepareSymbolDef(dst);
	PrepareSymbolUse(src1);

	m_functionStream.Write8(OP);

	CommitSymbol(dst);
}

template <uint32 OP>
void CCodeGen_Wasm::Emit_Fpu_MemMemMem(const STATEMENT& statement)
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

void CCodeGen_Wasm::PushRelativeFp32(CSymbol* symbol)
{
	PushRelativeAddress(symbol);

	m_functionStream.Write8(Wasm::INST_F32_LOAD);
	m_functionStream.Write8(0x02);
	m_functionStream.Write8(0x00);
}

void CCodeGen_Wasm::PushTemporaryFp32(CSymbol* symbol)
{
	assert(symbol->m_type == SYM_FP_TEMPORARY32);

	uint32 localIdx = GetTemporaryLocation(symbol);

	m_functionStream.Write8(Wasm::INST_LOCAL_GET);
	CWasmModuleBuilder::WriteULeb128(m_functionStream, localIdx);
}

void CCodeGen_Wasm::PullTemporaryFp32(CSymbol* symbol)
{
	assert(symbol->m_type == SYM_FP_TEMPORARY32);

	uint32 localIdx = GetTemporaryLocation(symbol);

	m_functionStream.Write8(Wasm::INST_LOCAL_SET);
	CWasmModuleBuilder::WriteULeb128(m_functionStream, localIdx);
}

void CCodeGen_Wasm::Emit_Fp_Cmp_AnyMemMem(const STATEMENT& statement)
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
		m_functionStream.Write8(Wasm::INST_F32_EQ);
		break;
	case CONDITION_BL:
		m_functionStream.Write8(Wasm::INST_F32_LT);
		break;
	case CONDITION_BE:
		m_functionStream.Write8(Wasm::INST_F32_LE);
		break;
	case CONDITION_AB:
		m_functionStream.Write8(Wasm::INST_F32_GT);
		break;
	default:
		assert(false);
		break;
	}

	CommitSymbol(dst);
}

void CCodeGen_Wasm::Emit_Fp_Rcpl_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	PrepareSymbolDef(dst);

	m_functionStream.Write8(Wasm::INST_F32_CONST);
	m_functionStream.Write32(0x3F800000);

	PrepareSymbolUse(src1);

	m_functionStream.Write8(Wasm::INST_F32_DIV);

	CommitSymbol(dst);
}

void CCodeGen_Wasm::Emit_Fp_Rsqrt_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	PrepareSymbolDef(dst);

	m_functionStream.Write8(Wasm::INST_F32_CONST);
	m_functionStream.Write32(0x3F800000);

	PrepareSymbolUse(src1);

	m_functionStream.Write8(Wasm::INST_F32_SQRT);

	m_functionStream.Write8(Wasm::INST_F32_DIV);

	CommitSymbol(dst);
}

void CCodeGen_Wasm::Emit_Fp_Clamp_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	PrepareSymbolDef(dst);

	{
		PrepareSymbolUse(src1);

		m_functionStream.Write8(Wasm::INST_PREFIX_SIMD);
		CWasmModuleBuilder::WriteULeb128(m_functionStream, Wasm::INST_F32x4_SPLAT);
	}

	//Load first constant
	{
		m_functionStream.Write8(Wasm::INST_I32_CONST);
		CWasmModuleBuilder::WriteSLeb128(m_functionStream, static_cast<int32>(0x7F7FFFFF));

		m_functionStream.Write8(Wasm::INST_PREFIX_SIMD);
		CWasmModuleBuilder::WriteULeb128(m_functionStream, Wasm::INST_I32x4_SPLAT);
	}

	m_functionStream.Write8(Wasm::INST_PREFIX_SIMD);
	CWasmModuleBuilder::WriteULeb128(m_functionStream, Wasm::INST_I32x4_MIN_S);

	//Load second constant
	{
		m_functionStream.Write8(Wasm::INST_I32_CONST);
		CWasmModuleBuilder::WriteSLeb128(m_functionStream, static_cast<int32>(0xFF7FFFFF));

		m_functionStream.Write8(Wasm::INST_PREFIX_SIMD);
		CWasmModuleBuilder::WriteULeb128(m_functionStream, Wasm::INST_I32x4_SPLAT);
	}

	m_functionStream.Write8(Wasm::INST_PREFIX_SIMD);
	CWasmModuleBuilder::WriteULeb128(m_functionStream, Wasm::INST_I32x4_MIN_U);

	m_functionStream.Write8(Wasm::INST_PREFIX_SIMD);
	CWasmModuleBuilder::WriteULeb128(m_functionStream, Wasm::INST_F32x4_EXTRACT_LANE);
	m_functionStream.Write8(0);

	CommitSymbol(dst);
}

void CCodeGen_Wasm::Emit_Fp_ToSingleI32_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(src1->m_type == SYM_FP_RELATIVE32);

	PrepareSymbolDef(dst);

	//Can't use PrepareSymbolUse here because src1 is a f32 and we need an i32
	PushRelativeAddress(src1);
	m_functionStream.Write8(Wasm::INST_I32_LOAD);
	m_functionStream.Write8(0x02);
	m_functionStream.Write8(0x00);

	m_functionStream.Write8(Wasm::INST_F32_CONVERT_I32_S);

	CommitSymbol(dst);
}

void CCodeGen_Wasm::Emit_Fp_ToInt32TruncS_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type == SYM_FP_RELATIVE32);

	PrepareSymbolDef(dst);
	PrepareSymbolUse(src1);

	m_functionStream.Write8(Wasm::INST_PREFIX_FC);
	m_functionStream.Write8(Wasm::INST_I32_TRUNC_SAT_F32_S);

	//Can't use CommitSymbol here since we got a i32 on the stack and our dst symbol is a f32.
	m_functionStream.Write8(Wasm::INST_I32_STORE);
	m_functionStream.Write8(0x02);
	m_functionStream.Write8(0x00);
}

void CCodeGen_Wasm::Emit_Fp_LdCst_TmpCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type == SYM_FP_TEMPORARY32);
	assert(src1->m_type == SYM_CONSTANT);

	PrepareSymbolDef(dst);

	m_functionStream.Write8(Wasm::INST_F32_CONST);
	m_functionStream.Write32(src1->m_valueLow);

	CommitSymbol(dst);
}

// clang-format off
CCodeGen_Wasm::CONSTMATCHER CCodeGen_Wasm::g_fpuConstMatchers[] =
{
	{ OP_FP_ADD_S,           MATCH_FP_MEMORY32,      MATCH_FP_MEMORY32,   MATCH_FP_MEMORY32,  MATCH_NIL,      &CCodeGen_Wasm::Emit_Fpu_MemMemMem<Wasm::INST_F32_ADD>       },
	{ OP_FP_SUB_S,           MATCH_FP_MEMORY32,      MATCH_FP_MEMORY32,   MATCH_FP_MEMORY32,  MATCH_NIL,      &CCodeGen_Wasm::Emit_Fpu_MemMemMem<Wasm::INST_F32_SUB>       },
	{ OP_FP_MUL_S,           MATCH_FP_MEMORY32,      MATCH_FP_MEMORY32,   MATCH_FP_MEMORY32,  MATCH_NIL,      &CCodeGen_Wasm::Emit_Fpu_MemMemMem<Wasm::INST_F32_MUL>       },
	{ OP_FP_DIV_S,           MATCH_FP_MEMORY32,      MATCH_FP_MEMORY32,   MATCH_FP_MEMORY32,  MATCH_NIL,      &CCodeGen_Wasm::Emit_Fpu_MemMemMem<Wasm::INST_F32_DIV>       },

	{ OP_FP_CMP_S,           MATCH_ANY,              MATCH_FP_MEMORY32,   MATCH_FP_MEMORY32,  MATCH_NIL,      &CCodeGen_Wasm::Emit_Fp_Cmp_AnyMemMem                        },

	{ OP_FP_MIN_S,           MATCH_FP_MEMORY32,      MATCH_FP_MEMORY32,   MATCH_FP_MEMORY32,  MATCH_NIL,      &CCodeGen_Wasm::Emit_Fpu_MemMemMem<Wasm::INST_F32_MIN>       },
	{ OP_FP_MAX_S,           MATCH_FP_MEMORY32,      MATCH_FP_MEMORY32,   MATCH_FP_MEMORY32,  MATCH_NIL,      &CCodeGen_Wasm::Emit_Fpu_MemMemMem<Wasm::INST_F32_MAX>       },

	{ OP_FP_RCPL_S,          MATCH_FP_MEMORY32,      MATCH_FP_MEMORY32,   MATCH_NIL,          MATCH_NIL,      &CCodeGen_Wasm::Emit_Fp_Rcpl_MemMem                          },
	{ OP_FP_SQRT_S,          MATCH_FP_MEMORY32,      MATCH_FP_MEMORY32,   MATCH_NIL,          MATCH_NIL,      &CCodeGen_Wasm::Emit_Fpu_MemMem<Wasm::INST_F32_SQRT>         },
	{ OP_FP_RSQRT_S,         MATCH_FP_MEMORY32,      MATCH_FP_MEMORY32,   MATCH_NIL,          MATCH_NIL,      &CCodeGen_Wasm::Emit_Fp_Rsqrt_MemMem                         },

	{ OP_FP_CLAMP_S,         MATCH_FP_MEMORY32,      MATCH_FP_MEMORY32,   MATCH_NIL,          MATCH_NIL,      &CCodeGen_Wasm::Emit_Fp_Clamp_MemMem                         },

	{ OP_FP_ABS_S,           MATCH_FP_MEMORY32,      MATCH_FP_MEMORY32,   MATCH_NIL,          MATCH_NIL,      &CCodeGen_Wasm::Emit_Fpu_MemMem<Wasm::INST_F32_ABS>          },
	{ OP_FP_NEG_S,           MATCH_FP_MEMORY32,      MATCH_FP_MEMORY32,   MATCH_NIL,          MATCH_NIL,      &CCodeGen_Wasm::Emit_Fpu_MemMem<Wasm::INST_F32_NEG>          },

	{ OP_FP_TOSINGLE_I32,    MATCH_FP_MEMORY32,      MATCH_FP_RELATIVE32, MATCH_NIL,          MATCH_NIL,      &CCodeGen_Wasm::Emit_Fp_ToSingleI32_MemMem                   },
	{ OP_FP_TOINT32_TRUNC_S, MATCH_FP_RELATIVE32,    MATCH_FP_MEMORY32,   MATCH_NIL,          MATCH_NIL,      &CCodeGen_Wasm::Emit_Fp_ToInt32TruncS_MemMem                 },

	{ OP_FP_LDCST,           MATCH_FP_TEMPORARY32,   MATCH_CONSTANT,      MATCH_NIL,          MATCH_NIL,      &CCodeGen_Wasm::Emit_Fp_LdCst_TmpCst                         },

	{ OP_MOV,                MATCH_NIL,              MATCH_NIL,           MATCH_NIL,          MATCH_NIL,      nullptr                                                      },
};
// clang-format on
