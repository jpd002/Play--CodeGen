#include "Jitter_CodeGen_AArch64.h"

using namespace Jitter;

void CCodeGen_AArch64::LoadMemoryFp32InRegister(CAArch64Assembler::REGISTERMD reg, CSymbol* symbol)
{
	switch(symbol->m_type)
	{
	case SYM_FP_RELATIVE32:
		m_assembler.Ldr_1s(reg, g_baseRegister, symbol->m_valueLow);
		break;
	case SYM_FP_TEMPORARY32:
		m_assembler.Ldr_1s(reg, CAArch64Assembler::xSP, symbol->m_stackLocation);
		break;
	default:
		assert(false);
		break;
	}
}

void CCodeGen_AArch64::StoreRegisterInMemoryFp32(CSymbol* symbol, CAArch64Assembler::REGISTERMD reg)
{
	switch(symbol->m_type)
	{
	case SYM_FP_RELATIVE32:
		m_assembler.Str_1s(reg, g_baseRegister, symbol->m_valueLow);
		break;
	case SYM_FP_TEMPORARY32:
		m_assembler.Str_1s(reg, CAArch64Assembler::xSP, symbol->m_stackLocation);
		break;
	default:
		assert(false);
		break;
	}
}

template <typename FPUOP>
void CCodeGen_AArch64::Emit_Fpu_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstReg = GetNextTempRegisterMd();
	auto src1Reg = GetNextTempRegisterMd();
	
	LoadMemoryFp32InRegister(src1Reg, src1);
	((m_assembler).*(FPUOP::OpReg()))(dstReg, src1Reg);
	StoreRegisterInMemoryFp32(dst, dstReg);
}

template <typename FPUOP>
void CCodeGen_AArch64::Emit_Fpu_MemMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstReg = GetNextTempRegisterMd();
	auto src1Reg = GetNextTempRegisterMd();
	auto src2Reg = GetNextTempRegisterMd();
	
	LoadMemoryFp32InRegister(src1Reg, src1);
	LoadMemoryFp32InRegister(src2Reg, src2);
	((m_assembler).*(FPUOP::OpReg()))(dstReg, src1Reg, src2Reg);
	StoreRegisterInMemoryFp32(dst, dstReg);
}

void CCodeGen_AArch64::Emit_Fp_Cmp_AnyMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDef(dst, GetNextTempRegister());
	auto src1Reg = GetNextTempRegisterMd();
	auto src2Reg = GetNextTempRegisterMd();

	LoadMemoryFp32InRegister(src1Reg, src1);
	LoadMemoryFp32InRegister(src2Reg, src2);
	m_assembler.Fcmp_1s(src1Reg, src2Reg);
	Cmp_GetFlag(dstReg, statement.jmpCondition);
	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_AArch64::Emit_Fp_Rcpl_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	
	auto dstReg = GetNextTempRegisterMd();
	auto src1Reg = GetNextTempRegisterMd();
	auto oneReg = GetNextTempRegisterMd();
	
	m_assembler.Fmov_1s(oneReg, 0x70);	//Loads 1.0f
	LoadMemoryFp32InRegister(src1Reg, src1);
	m_assembler.Fdiv_1s(dstReg, oneReg, src1Reg);
	StoreRegisterInMemoryFp32(dst, dstReg);
}

void CCodeGen_AArch64::Emit_Fp_Rsqrt_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	
	auto dstReg = GetNextTempRegisterMd();
	auto src1Reg = GetNextTempRegisterMd();
	auto oneReg = GetNextTempRegisterMd();
	
	m_assembler.Fmov_1s(oneReg, 0x70);	//Loads 1.0f
	LoadMemoryFp32InRegister(src1Reg, src1);
	m_assembler.Fsqrt_1s(src1Reg, src1Reg);
	m_assembler.Fdiv_1s(dstReg, oneReg, src1Reg);
	StoreRegisterInMemoryFp32(dst, dstReg);
}

void CCodeGen_AArch64::Emit_Fp_Clamp_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto resultReg = GetNextTempRegisterMd();
	auto cst1Reg = GetNextTempRegisterMd();
	auto cst2Reg = GetNextTempRegisterMd();

	m_assembler.Ldr_Pc(cst1Reg, g_fpClampMask1);
	m_assembler.Ldr_Pc(cst2Reg, g_fpClampMask2);

	LoadMemoryFp32InRegister(resultReg, src1);
	m_assembler.Smin_4s(resultReg, resultReg, cst1Reg);
	m_assembler.Umin_4s(resultReg, resultReg, cst2Reg);
	StoreRegisterInMemoryFp32(dst, resultReg);
}

void CCodeGen_AArch64::Emit_Fp_ToSingleI32_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstReg = GetNextTempRegisterMd();
	auto src1Reg = GetNextTempRegisterMd();

	LoadMemoryFp32InRegister(src1Reg, src1);
	m_assembler.Scvtf_1s(dstReg, src1Reg);
	StoreRegisterInMemoryFp32(dst, dstReg);
}

void CCodeGen_AArch64::Emit_Fp_ToInt32TruncS_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstReg = GetNextTempRegisterMd();
	auto src1Reg = GetNextTempRegisterMd();

	LoadMemoryFp32InRegister(src1Reg, src1);
	m_assembler.Fcvtzs_1s(dstReg, src1Reg);
	StoreRegisterInMemoryFp32(dst, dstReg);
}

void CCodeGen_AArch64::Emit_Fp_LdCst_TmpCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type  == SYM_FP_TEMPORARY32);
	assert(src1->m_type == SYM_CONSTANT);

	auto tmpReg = GetNextTempRegister();
	
	LoadConstantInRegister(tmpReg, src1->m_valueLow);
	m_assembler.Str(tmpReg, CAArch64Assembler::xSP, dst->m_stackLocation);
}

CCodeGen_AArch64::CONSTMATCHER CCodeGen_AArch64::g_fpuConstMatchers[] =
{
	{ OP_FP_ADD_S,           MATCH_FP_MEMORY32,       MATCH_FP_MEMORY32,     MATCH_FP_MEMORY32,    MATCH_NIL, &CCodeGen_AArch64::Emit_Fpu_MemMemMem<FPUOP_ADD>    },
	{ OP_FP_SUB_S,           MATCH_FP_MEMORY32,       MATCH_FP_MEMORY32,     MATCH_FP_MEMORY32,    MATCH_NIL, &CCodeGen_AArch64::Emit_Fpu_MemMemMem<FPUOP_SUB>    },
	{ OP_FP_MUL_S,           MATCH_FP_MEMORY32,       MATCH_FP_MEMORY32,     MATCH_FP_MEMORY32,    MATCH_NIL, &CCodeGen_AArch64::Emit_Fpu_MemMemMem<FPUOP_MUL>    },
	{ OP_FP_DIV_S,           MATCH_FP_MEMORY32,       MATCH_FP_MEMORY32,     MATCH_FP_MEMORY32,    MATCH_NIL, &CCodeGen_AArch64::Emit_Fpu_MemMemMem<FPUOP_DIV>    },

	{ OP_FP_CMP_S,           MATCH_ANY,               MATCH_FP_MEMORY32,     MATCH_FP_MEMORY32,    MATCH_NIL, &CCodeGen_AArch64::Emit_Fp_Cmp_AnyMemMem            },

	{ OP_FP_MIN_S,           MATCH_FP_MEMORY32,       MATCH_FP_MEMORY32,     MATCH_FP_MEMORY32,    MATCH_NIL, &CCodeGen_AArch64::Emit_Fpu_MemMemMem<FPUOP_MIN>    },
	{ OP_FP_MAX_S,           MATCH_FP_MEMORY32,       MATCH_FP_MEMORY32,     MATCH_FP_MEMORY32,    MATCH_NIL, &CCodeGen_AArch64::Emit_Fpu_MemMemMem<FPUOP_MAX>    },

	{ OP_FP_RCPL_S,          MATCH_FP_MEMORY32,       MATCH_FP_MEMORY32,     MATCH_NIL,            MATCH_NIL, &CCodeGen_AArch64::Emit_Fp_Rcpl_MemMem              },
	{ OP_FP_SQRT_S,          MATCH_FP_MEMORY32,       MATCH_FP_MEMORY32,     MATCH_NIL,            MATCH_NIL, &CCodeGen_AArch64::Emit_Fpu_MemMem<FPUOP_SQRT>      },
	{ OP_FP_RSQRT_S,         MATCH_FP_MEMORY32,       MATCH_FP_MEMORY32,     MATCH_NIL,            MATCH_NIL, &CCodeGen_AArch64::Emit_Fp_Rsqrt_MemMem             },

	{ OP_FP_CLAMP_S,         MATCH_FP_MEMORY32,       MATCH_FP_MEMORY32,     MATCH_NIL,            MATCH_NIL, &CCodeGen_AArch64::Emit_Fp_Clamp_MemMem             },

	{ OP_FP_ABS_S,           MATCH_FP_MEMORY32,       MATCH_FP_MEMORY32,     MATCH_NIL,            MATCH_NIL, &CCodeGen_AArch64::Emit_Fpu_MemMem<FPUOP_ABS>       },
	{ OP_FP_NEG_S,           MATCH_FP_MEMORY32,       MATCH_FP_MEMORY32,     MATCH_NIL,            MATCH_NIL, &CCodeGen_AArch64::Emit_Fpu_MemMem<FPUOP_NEG>       },

	{ OP_FP_TOSINGLE_I32,    MATCH_FP_MEMORY32,       MATCH_FP_MEMORY32,     MATCH_NIL,            MATCH_NIL, &CCodeGen_AArch64::Emit_Fp_ToSingleI32_MemMem       },
	{ OP_FP_TOINT32_TRUNC_S, MATCH_FP_MEMORY32,       MATCH_FP_MEMORY32,     MATCH_NIL,            MATCH_NIL, &CCodeGen_AArch64::Emit_Fp_ToInt32TruncS_MemMem     },

	{ OP_FP_LDCST,           MATCH_FP_TEMPORARY32,    MATCH_CONSTANT,        MATCH_NIL,            MATCH_NIL, &CCodeGen_AArch64::Emit_Fp_LdCst_TmpCst             },

	{ OP_MOV,                MATCH_NIL,               MATCH_NIL,             MATCH_NIL,            MATCH_NIL, nullptr                                             },
};
