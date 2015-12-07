#include "Jitter_CodeGen_AArch64.h"

using namespace Jitter;

void CCodeGen_AArch64::LoadMemoryFpSingleInRegister(CAArch64Assembler::REGISTERMD reg, CSymbol* symbol)
{
	switch(symbol->m_type)
	{
	case SYM_FP_REL_SINGLE:
		m_assembler.Ldr_1s(reg, g_baseRegister, symbol->m_valueLow);
		break;
	case SYM_FP_TMP_SINGLE:
		m_assembler.Ldr_1s(reg, CAArch64Assembler::xSP, symbol->m_stackLocation);
		break;
	default:
		assert(false);
		break;
	}
}

void CCodeGen_AArch64::StoreRegisterInMemoryFpSingle(CSymbol* symbol, CAArch64Assembler::REGISTERMD reg)
{
	switch(symbol->m_type)
	{
	case SYM_FP_REL_SINGLE:
		m_assembler.Str_1s(reg, g_baseRegister, symbol->m_valueLow);
		break;
	case SYM_FP_TMP_SINGLE:
		m_assembler.Str_1s(reg, CAArch64Assembler::xSP, symbol->m_stackLocation);
		break;
	default:
		assert(false);
		break;
	}
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
	
	LoadMemoryFpSingleInRegister(src1Reg, src1);
	LoadMemoryFpSingleInRegister(src2Reg, src2);
	((m_assembler).*(FPUOP::OpReg()))(dstReg, src1Reg, src2Reg);
	StoreRegisterInMemoryFpSingle(dst, dstReg);
}

CCodeGen_AArch64::CONSTMATCHER CCodeGen_AArch64::g_fpuConstMatchers[] =
{
	{ OP_FP_ADD,    MATCH_MEMORY_FP_SINGLE,    MATCH_MEMORY_FP_SINGLE,    MATCH_MEMORY_FP_SINGLE,    &CCodeGen_AArch64::Emit_Fpu_MemMemMem<FPUOP_ADD>    },
	{ OP_FP_SUB,    MATCH_MEMORY_FP_SINGLE,    MATCH_MEMORY_FP_SINGLE,    MATCH_MEMORY_FP_SINGLE,    &CCodeGen_AArch64::Emit_Fpu_MemMemMem<FPUOP_SUB>    },
	{ OP_FP_MUL,    MATCH_MEMORY_FP_SINGLE,    MATCH_MEMORY_FP_SINGLE,    MATCH_MEMORY_FP_SINGLE,    &CCodeGen_AArch64::Emit_Fpu_MemMemMem<FPUOP_MUL>    },
	{ OP_FP_DIV,    MATCH_MEMORY_FP_SINGLE,    MATCH_MEMORY_FP_SINGLE,    MATCH_MEMORY_FP_SINGLE,    &CCodeGen_AArch64::Emit_Fpu_MemMemMem<FPUOP_DIV>    },

	{ OP_MOV,       MATCH_NIL,                 MATCH_NIL,                 MATCH_NIL,                 nullptr                                             },
};
