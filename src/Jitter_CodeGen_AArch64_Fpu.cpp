#include "Jitter_CodeGen_AArch64.h"
#include "Jitter.h"
#include <stdexcept>

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

CAArch64Assembler::REGISTERMD CCodeGen_AArch64::PrepareSymbolRegisterDefFp(CSymbol* symbol)
{
	switch(symbol->m_type)
	{
	case SYM_FP_REGISTER32:
		assert(symbol->m_valueLow < MAX_MDREGISTERS);
		return g_registersMd[symbol->m_valueLow];
		break;
	case SYM_FP_TEMPORARY32:
	case SYM_FP_RELATIVE32:
		return GetNextTempRegisterMd();
		break;
	default:
		throw std::exception();
		break;
	}
}

CAArch64Assembler::REGISTERMD CCodeGen_AArch64::PrepareSymbolRegisterUseFp(CSymbol* symbol)
{
	switch(symbol->m_type)
	{
	case SYM_FP_REGISTER32:
		assert(symbol->m_valueLow < MAX_MDREGISTERS);
		return g_registersMd[symbol->m_valueLow];
		break;
	case SYM_FP_TEMPORARY32:
	case SYM_FP_RELATIVE32:
	{
		auto tempRegister = GetNextTempRegisterMd();
		LoadMemoryFp32InRegister(tempRegister, symbol);
		return tempRegister;
	}
	break;
	default:
		throw std::exception();
		break;
	}
}

void CCodeGen_AArch64::CommitSymbolRegisterFp(CSymbol* symbol, CAArch64Assembler::REGISTERMD usedRegister)
{
	switch(symbol->m_type)
	{
	case SYM_FP_REGISTER32:
		assert(usedRegister == g_registersMd[symbol->m_valueLow]);
		break;
	case SYM_FP_TEMPORARY32:
	case SYM_FP_RELATIVE32:
		StoreRegisterInMemoryFp32(symbol, usedRegister);
		break;
	default:
		assert(false);
		break;
	}
}

template <typename FPUOP>
void CCodeGen_AArch64::Emit_Fpu_VarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDefFp(dst);
	auto src1Reg = PrepareSymbolRegisterUseFp(src1);

	((m_assembler).*(FPUOP::OpReg()))(dstReg, src1Reg);

	CommitSymbolRegisterFp(dst, dstReg);
}

template <typename FPUOP>
void CCodeGen_AArch64::Emit_Fpu_VarVarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDefFp(dst);
	auto src1Reg = PrepareSymbolRegisterUseFp(src1);
	auto src2Reg = PrepareSymbolRegisterUseFp(src2);

	((m_assembler).*(FPUOP::OpReg()))(dstReg, src1Reg, src2Reg);

	CommitSymbolRegisterFp(dst, dstReg);
}

void CCodeGen_AArch64::Emit_Fp32_Mov_RegMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type == SYM_FP_REGISTER32);

	LoadMemoryFp32InRegister(g_registersMd[dst->m_valueLow], src1);
}

void CCodeGen_AArch64::Emit_Fp32_Mov_MemReg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(src1->m_type == SYM_FP_REGISTER32);

	StoreRegisterInMemoryFp32(dst, g_registersMd[src1->m_valueLow]);
}

void CCodeGen_AArch64::Emit_Fp32_LdCst_RegCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type == SYM_FP_REGISTER32);
	assert(src1->m_type == SYM_CONSTANT);

	//TODO: Load from ZR register if we have zero here
	//TODO: If we have a small constant, we can use the immediate version
	assert(src1->m_valueLow != 0);

	auto tmpReg = GetNextTempRegister();
	LoadConstantInRegister(tmpReg, src1->m_valueLow);
	m_assembler.Fmov_1s(g_registersMd[dst->m_valueLow], tmpReg);
}

void CCodeGen_AArch64::Emit_Fp32_LdCst_TmpCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type == SYM_FP_TEMPORARY32);
	assert(src1->m_type == SYM_CONSTANT);

	auto tmpReg = GetNextTempRegister();

	LoadConstantInRegister(tmpReg, src1->m_valueLow);
	m_assembler.Str(tmpReg, CAArch64Assembler::xSP, dst->m_stackLocation);
}

void CCodeGen_AArch64::Emit_Fp_Cmp_AnyVarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDef(dst, GetNextTempRegister());
	auto src1Reg = PrepareSymbolRegisterUseFp(src1);
	auto src2Reg = PrepareSymbolRegisterUseFp(src2);

	m_assembler.Fcmp_1s(src1Reg, src2Reg);
	Cmp_GetFlag(dstReg, statement.jmpCondition);

	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_AArch64::Emit_Fp_Rcpl_VarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDefFp(dst);
	auto src1Reg = PrepareSymbolRegisterUseFp(src1);
	auto oneReg = GetNextTempRegisterMd();

	m_assembler.Fmov_1s(oneReg, 0x70); //Loads 1.0f
	m_assembler.Fdiv_1s(dstReg, oneReg, src1Reg);

	CommitSymbolRegisterFp(dst, dstReg);
}

void CCodeGen_AArch64::Emit_Fp_Rsqrt_VarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDefFp(dst);
	auto src1Reg = PrepareSymbolRegisterUseFp(src1);
	auto oneReg = GetNextTempRegisterMd();
	auto tmpReg = GetNextTempRegisterMd();

	m_assembler.Fmov_1s(oneReg, 0x70); //Loads 1.0f
	m_assembler.Fsqrt_1s(tmpReg, src1Reg);
	m_assembler.Fdiv_1s(dstReg, oneReg, tmpReg);

	CommitSymbolRegisterFp(dst, dstReg);
}

void CCodeGen_AArch64::Emit_Fp_Clamp_VarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDefFp(dst);
	auto src1Reg = PrepareSymbolRegisterUseFp(src1);
	auto cst1Reg = PrepareLiteralRegisterMd(&g_fpClampMask1);
	auto cst2Reg = PrepareLiteralRegisterMd(&g_fpClampMask2);

	m_assembler.Smin_4s(dstReg, src1Reg, cst1Reg);
	m_assembler.Umin_4s(dstReg, dstReg, cst2Reg);

	CommitSymbolRegisterFp(dst, dstReg);
}

void CCodeGen_AArch64::Emit_Fp_ToSingleI32_VarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDefFp(dst);
	auto src1Reg = PrepareSymbolRegisterUseFp(src1);

	m_assembler.Scvtf_1s(dstReg, src1Reg);

	CommitSymbolRegisterFp(dst, dstReg);
}

void CCodeGen_AArch64::Emit_Fp_ToInt32TruncS_VarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDefFp(dst);
	auto src1Reg = PrepareSymbolRegisterUseFp(src1);

	m_assembler.Fcvtzs_1s(dstReg, src1Reg);

	CommitSymbolRegisterFp(dst, dstReg);
}

void CCodeGen_AArch64::Emit_Fp_SetRoundingMode_Cst(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();

	auto tmpReg = GetNextTempRegister();

	static constexpr uint32 FPCR_RMODE_MASK = (3 << 22);
	uint32 fpcrRmode = 0;
	switch(src1->m_valueLow)
	{
	case Jitter::CJitter::ROUND_NEAREST:
		fpcrRmode = (0 << 22);
		break;
	case Jitter::CJitter::ROUND_MINUSINFINITY:
		fpcrRmode = (2 << 22);
		break;
	case Jitter::CJitter::ROUND_PLUSINFINITY:
		fpcrRmode = (1 << 22);
		break;
	case Jitter::CJitter::ROUND_TRUNCATE:
		fpcrRmode = (3 << 22);
		break;
	default:
		assert(false);
		break;
	}

	m_assembler.Mrs_Fpcr(static_cast<CAArch64Assembler::REGISTER64>(tmpReg));

	{
		LOGICAL_IMM_PARAMS maskImm;
		bool maskImmSuccess = TryGetLogicalImmParams(~FPCR_RMODE_MASK, maskImm);
		assert(maskImmSuccess);
		m_assembler.And(tmpReg, tmpReg, maskImm.n, maskImm.immr, maskImm.imms);
	}

	if(fpcrRmode != 0)
	{
		LOGICAL_IMM_PARAMS rmodeImm;
		bool rmodeImmSuccess = TryGetLogicalImmParams(fpcrRmode, rmodeImm);
		assert(rmodeImmSuccess);
		m_assembler.Orr(tmpReg, tmpReg, rmodeImm.n, rmodeImm.immr, rmodeImm.imms);
	}

	m_assembler.Msr_Fpcr(static_cast<CAArch64Assembler::REGISTER64>(tmpReg));
}

// clang-format off
CCodeGen_AArch64::CONSTMATCHER CCodeGen_AArch64::g_fpuConstMatchers[] =
{
	{ OP_FP_ADD_S,           MATCH_FP_VARIABLE32,     MATCH_FP_VARIABLE32,   MATCH_FP_VARIABLE32,  MATCH_NIL, &CCodeGen_AArch64::Emit_Fpu_VarVarVar<FPUOP_ADD>    },
	{ OP_FP_SUB_S,           MATCH_FP_VARIABLE32,     MATCH_FP_VARIABLE32,   MATCH_FP_VARIABLE32,  MATCH_NIL, &CCodeGen_AArch64::Emit_Fpu_VarVarVar<FPUOP_SUB>    },
	{ OP_FP_MUL_S,           MATCH_FP_VARIABLE32,     MATCH_FP_VARIABLE32,   MATCH_FP_VARIABLE32,  MATCH_NIL, &CCodeGen_AArch64::Emit_Fpu_VarVarVar<FPUOP_MUL>    },
	{ OP_FP_DIV_S,           MATCH_FP_VARIABLE32,     MATCH_FP_VARIABLE32,   MATCH_FP_VARIABLE32,  MATCH_NIL, &CCodeGen_AArch64::Emit_Fpu_VarVarVar<FPUOP_DIV>    },

	{ OP_FP_CMP_S,           MATCH_ANY,               MATCH_FP_VARIABLE32,   MATCH_FP_VARIABLE32,  MATCH_NIL, &CCodeGen_AArch64::Emit_Fp_Cmp_AnyVarVar            },

	{ OP_FP_MIN_S,           MATCH_FP_VARIABLE32,     MATCH_FP_VARIABLE32,   MATCH_FP_VARIABLE32,  MATCH_NIL, &CCodeGen_AArch64::Emit_Fpu_VarVarVar<FPUOP_MIN>    },
	{ OP_FP_MAX_S,           MATCH_FP_VARIABLE32,     MATCH_FP_VARIABLE32,   MATCH_FP_VARIABLE32,  MATCH_NIL, &CCodeGen_AArch64::Emit_Fpu_VarVarVar<FPUOP_MAX>    },

	{ OP_FP_RCPL_S,          MATCH_FP_VARIABLE32,     MATCH_FP_VARIABLE32,   MATCH_NIL,            MATCH_NIL, &CCodeGen_AArch64::Emit_Fp_Rcpl_VarVar              },
	{ OP_FP_SQRT_S,          MATCH_FP_VARIABLE32,     MATCH_FP_VARIABLE32,   MATCH_NIL,            MATCH_NIL, &CCodeGen_AArch64::Emit_Fpu_VarVar<FPUOP_SQRT>      },
	{ OP_FP_RSQRT_S,         MATCH_FP_VARIABLE32,     MATCH_FP_VARIABLE32,   MATCH_NIL,            MATCH_NIL, &CCodeGen_AArch64::Emit_Fp_Rsqrt_VarVar             },

	{ OP_FP_CLAMP_S,         MATCH_FP_VARIABLE32,     MATCH_FP_VARIABLE32,   MATCH_NIL,            MATCH_NIL, &CCodeGen_AArch64::Emit_Fp_Clamp_VarVar             },

	{ OP_FP_ABS_S,           MATCH_FP_VARIABLE32,     MATCH_FP_VARIABLE32,   MATCH_NIL,            MATCH_NIL, &CCodeGen_AArch64::Emit_Fpu_VarVar<FPUOP_ABS>       },
	{ OP_FP_NEG_S,           MATCH_FP_VARIABLE32,     MATCH_FP_VARIABLE32,   MATCH_NIL,            MATCH_NIL, &CCodeGen_AArch64::Emit_Fpu_VarVar<FPUOP_NEG>       },

	{ OP_FP_TOSINGLE_I32,    MATCH_FP_VARIABLE32,     MATCH_FP_VARIABLE32,   MATCH_NIL,            MATCH_NIL, &CCodeGen_AArch64::Emit_Fp_ToSingleI32_VarVar       },
	{ OP_FP_TOINT32_TRUNC_S, MATCH_FP_VARIABLE32,     MATCH_FP_VARIABLE32,   MATCH_NIL,            MATCH_NIL, &CCodeGen_AArch64::Emit_Fp_ToInt32TruncS_VarVar     },

	{ OP_MOV,                MATCH_FP_REGISTER32,     MATCH_FP_MEMORY32,     MATCH_NIL,            MATCH_NIL, &CCodeGen_AArch64::Emit_Fp32_Mov_RegMem             },
	{ OP_MOV,                MATCH_FP_MEMORY32,       MATCH_FP_REGISTER32,   MATCH_NIL,            MATCH_NIL, &CCodeGen_AArch64::Emit_Fp32_Mov_MemReg             },
	{ OP_FP_LDCST,           MATCH_FP_REGISTER32,     MATCH_CONSTANT,        MATCH_NIL,            MATCH_NIL, &CCodeGen_AArch64::Emit_Fp32_LdCst_RegCst           },
	{ OP_FP_LDCST,           MATCH_FP_TEMPORARY32,    MATCH_CONSTANT,        MATCH_NIL,            MATCH_NIL, &CCodeGen_AArch64::Emit_Fp32_LdCst_TmpCst           },

	{ OP_FP_SETROUNDINGMODE, MATCH_NIL,               MATCH_CONSTANT,        MATCH_NIL,            MATCH_NIL, &CCodeGen_AArch64::Emit_Fp_SetRoundingMode_Cst      },

	{ OP_MOV,                MATCH_NIL,               MATCH_NIL,             MATCH_NIL,            MATCH_NIL, nullptr                                             },
};
// clang-format on
