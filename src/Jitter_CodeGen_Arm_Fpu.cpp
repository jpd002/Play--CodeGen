#include "Jitter_CodeGen_Arm.h"

using namespace Jitter;

void CCodeGen_Arm::LoadMemoryFpSingleInRegister(CArmAssembler::SINGLE_REGISTER reg, CSymbol* symbol)
{
	switch(symbol->m_type)
	{
	case SYM_FP_REL_SINGLE:
		LoadRelativeFpSingleInRegister(reg, symbol);
		break;
	case SYM_FP_TMP_SINGLE:
		LoadTemporaryFpSingleInRegister(reg, symbol);
		break;
	default:
		assert(false);
		break;
	}
}

void CCodeGen_Arm::StoreRegisterInMemoryFpSingle(CSymbol* symbol, CArmAssembler::SINGLE_REGISTER reg)
{
	switch(symbol->m_type)
	{
	case SYM_FP_REL_SINGLE:
		StoreRelativeFpSingleInRegister(symbol, reg);
		break;
	case SYM_FP_TMP_SINGLE:
		StoreTemporaryFpSingleInRegister(symbol, reg);
		break;
	default:
		assert(false);
		break;
	}
}

void CCodeGen_Arm::LoadRelativeFpSingleInRegister(CArmAssembler::SINGLE_REGISTER reg, CSymbol* symbol)
{
	assert(symbol->m_type == SYM_FP_REL_SINGLE);
	m_assembler.Vldr(reg, g_baseRegister, CArmAssembler::MakeImmediateLdrAddress(symbol->m_valueLow));
}

void CCodeGen_Arm::StoreRelativeFpSingleInRegister(CSymbol* symbol, CArmAssembler::SINGLE_REGISTER reg)
{
	assert(symbol->m_type == SYM_FP_REL_SINGLE);
	m_assembler.Vstr(reg, g_baseRegister, CArmAssembler::MakeImmediateLdrAddress(symbol->m_valueLow));
}

void CCodeGen_Arm::LoadTemporaryFpSingleInRegister(CArmAssembler::SINGLE_REGISTER reg, CSymbol* symbol)
{
	assert(symbol->m_type == SYM_FP_TMP_SINGLE);
	m_assembler.Vldr(reg, CArmAssembler::rSP, CArmAssembler::MakeImmediateLdrAddress(symbol->m_stackLocation + m_stackLevel));
}

void CCodeGen_Arm::StoreTemporaryFpSingleInRegister(CSymbol* symbol, CArmAssembler::SINGLE_REGISTER reg)
{
	assert(symbol->m_type == SYM_FP_TMP_SINGLE);
	m_assembler.Vstr(reg, CArmAssembler::rSP, CArmAssembler::MakeImmediateLdrAddress(symbol->m_stackLocation + m_stackLevel));
}

template <typename FPUOP>
void CCodeGen_Arm::Emit_Fpu_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	LoadMemoryFpSingleInRegister(CArmAssembler::s0, src1);
	((m_assembler).*(FPUOP::OpReg()))(CArmAssembler::s1, CArmAssembler::s0);
	StoreRegisterInMemoryFpSingle(dst, CArmAssembler::s1);
}

template <typename FPUOP>
void CCodeGen_Arm::Emit_Fpu_MemMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	LoadMemoryFpSingleInRegister(CArmAssembler::s0, src1);
	LoadMemoryFpSingleInRegister(CArmAssembler::s1, src2);
	((m_assembler).*(FPUOP::OpReg()))(CArmAssembler::s2, CArmAssembler::s0, CArmAssembler::s1);
	StoreRegisterInMemoryFpSingle(dst, CArmAssembler::s2);
}

template <typename FPUMDOP>
void CCodeGen_Arm::Emit_FpuMd_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	LoadMemoryFpSingleInRegister(CArmAssembler::s0, src1);
	((m_assembler).*(FPUMDOP::OpReg()))(CArmAssembler::q1, CArmAssembler::q0);
	StoreRegisterInMemoryFpSingle(dst, CArmAssembler::s4);
}

template <typename FPUMDOP>
void CCodeGen_Arm::Emit_FpuMd_MemMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	LoadMemoryFpSingleInRegister(CArmAssembler::s0, src1);
	LoadMemoryFpSingleInRegister(CArmAssembler::s4, src2);
	((m_assembler).*(FPUMDOP::OpReg()))(CArmAssembler::q2, CArmAssembler::q0, CArmAssembler::q1);
	StoreRegisterInMemoryFpSingle(dst, CArmAssembler::s8);
}

void CCodeGen_Arm::Emit_Fp_Cmp_AnyMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDef(dst, CArmAssembler::r0);

	LoadMemoryFpSingleInRegister(CArmAssembler::s0, src1);
	LoadMemoryFpSingleInRegister(CArmAssembler::s1, src2);
	m_assembler.Vcmp_F32(CArmAssembler::s0, CArmAssembler::s1);
	m_assembler.Vmrs(CArmAssembler::rPC);	//Move to general purpose status register
	switch(statement.jmpCondition)
	{
	case Jitter::CONDITION_BE:
		m_assembler.MovCc(CArmAssembler::CONDITION_LE, dstReg, CArmAssembler::MakeImmediateAluOperand(1, 0));
		m_assembler.MovCc(CArmAssembler::CONDITION_GT, dstReg, CArmAssembler::MakeImmediateAluOperand(0, 0));
		break;
	case Jitter::CONDITION_BL:
		m_assembler.MovCc(CArmAssembler::CONDITION_LT, dstReg, CArmAssembler::MakeImmediateAluOperand(1, 0));
		m_assembler.MovCc(CArmAssembler::CONDITION_GE, dstReg, CArmAssembler::MakeImmediateAluOperand(0, 0));
		break;
	case Jitter::CONDITION_EQ:
		m_assembler.MovCc(CArmAssembler::CONDITION_EQ, dstReg, CArmAssembler::MakeImmediateAluOperand(1, 0));
		m_assembler.MovCc(CArmAssembler::CONDITION_NE, dstReg, CArmAssembler::MakeImmediateAluOperand(0, 0));
		break;
	default:
		assert(0);
		break;
	}

	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_Arm::Emit_Fp_Mov_MemSRelI32(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(src1->m_type == SYM_FP_REL_INT32);

	auto dstReg = CArmAssembler::s0;
	auto src1Reg = CArmAssembler::s1;

	m_assembler.Vldr(src1Reg, g_baseRegister, CArmAssembler::MakeImmediateLdrAddress(src1->m_valueLow));
	m_assembler.Vcvt_F32_S32(dstReg, src1Reg);
	StoreRegisterInMemoryFpSingle(dst, dstReg);
}

void CCodeGen_Arm::Emit_Fp_ToIntTrunc_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstReg = CArmAssembler::s0;
	auto src1Reg = CArmAssembler::s1;

	LoadMemoryFpSingleInRegister(src1Reg, src1);
	m_assembler.Vcvt_S32_F32(dstReg, src1Reg);
	StoreRegisterInMemoryFpSingle(dst, dstReg);
}

void CCodeGen_Arm::Emit_Fp_LdCst_TmpCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type  == SYM_FP_TMP_SINGLE);
	assert(src1->m_type == SYM_CONSTANT);

	LoadConstantInRegister(CArmAssembler::r0, src1->m_valueLow);
	m_assembler.Str(CArmAssembler::r0, CArmAssembler::rSP, CArmAssembler::MakeImmediateLdrAddress(dst->m_stackLocation + m_stackLevel));
}

CCodeGen_Arm::CONSTMATCHER CCodeGen_Arm::g_fpuConstMatchers[] = 
{
	{ OP_FP_ADD,			MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,	&CCodeGen_Arm::Emit_Fpu_MemMemMem<FPUOP_ADD>		},
	{ OP_FP_SUB,			MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,	&CCodeGen_Arm::Emit_Fpu_MemMemMem<FPUOP_SUB>		},
	{ OP_FP_MUL,			MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,	&CCodeGen_Arm::Emit_Fpu_MemMemMem<FPUOP_MUL>		},
	{ OP_FP_DIV,			MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,	&CCodeGen_Arm::Emit_Fpu_MemMemMem<FPUOP_DIV>		},

	{ OP_FP_CMP,			MATCH_ANY,					MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,	&CCodeGen_Arm::Emit_Fp_Cmp_AnyMemMem				},

	{ OP_FP_MIN,			MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,	&CCodeGen_Arm::Emit_FpuMd_MemMemMem<FPUMDOP_MIN>	},
	{ OP_FP_MAX,			MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,	&CCodeGen_Arm::Emit_FpuMd_MemMemMem<FPUMDOP_MAX>	},

	{ OP_FP_RCPL,			MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,		MATCH_NIL,				&CCodeGen_Arm::Emit_FpuMd_MemMem<FPUMDOP_RCPL>		},
	{ OP_FP_SQRT,			MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,		MATCH_NIL,				&CCodeGen_Arm::Emit_Fpu_MemMem<FPUOP_SQRT>			},
	{ OP_FP_RSQRT,			MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,		MATCH_NIL,				&CCodeGen_Arm::Emit_FpuMd_MemMem<FPUMDOP_RSQRT>		},

	{ OP_FP_ABS,			MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,		MATCH_NIL,				&CCodeGen_Arm::Emit_Fpu_MemMem<FPUOP_ABS>			},
	{ OP_FP_NEG,			MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,		MATCH_NIL,				&CCodeGen_Arm::Emit_Fpu_MemMem<FPUOP_NEG>			},

	{ OP_MOV,				MATCH_MEMORY_FP_SINGLE,		MATCH_RELATIVE_FP_INT32,	MATCH_NIL,				&CCodeGen_Arm::Emit_Fp_Mov_MemSRelI32				},
	{ OP_FP_TOINT_TRUNC,	MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,		MATCH_NIL,				&CCodeGen_Arm::Emit_Fp_ToIntTrunc_MemMem			},

	{ OP_FP_LDCST,			MATCH_TEMPORARY_FP_SINGLE,	MATCH_CONSTANT,				MATCH_NIL,				&CCodeGen_Arm::Emit_Fp_LdCst_TmpCst					},

	{ OP_MOV,				MATCH_NIL,					MATCH_NIL,					MATCH_NIL,				NULL												},
};
