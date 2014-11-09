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

void CCodeGen_Arm::Emit_Fp_Rcpl_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type  == SYM_FP_REL_SINGLE);
	assert(src1->m_type == SYM_FP_REL_SINGLE);

	//No scalar version of VREPCE on ARMv7
	LoadMemoryFpSingleInRegister(CArmAssembler::s0, src1);
	m_assembler.Vrecpe_F32(CArmAssembler::q1, CArmAssembler::q0);
	StoreRegisterInMemoryFpSingle(dst, CArmAssembler::s4);
}

void CCodeGen_Arm::Emit_Fp_Rsqrt_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type  == SYM_FP_REL_SINGLE);
	assert(src1->m_type == SYM_FP_REL_SINGLE);

	//No scalar version of VRSQRTE on ARMv7
	LoadMemoryFpSingleInRegister(CArmAssembler::s0, src1);
	m_assembler.Vrsqrte_F32(CArmAssembler::q1, CArmAssembler::q0);
	StoreRegisterInMemoryFpSingle(dst, CArmAssembler::s4);
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
	{ OP_FP_ADD,	MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,	&CCodeGen_Arm::Emit_Fpu_MemMemMem<FPUOP_ADD>	},
	{ OP_FP_MUL,	MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,	&CCodeGen_Arm::Emit_Fpu_MemMemMem<FPUOP_MUL>	},
	{ OP_FP_DIV,	MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,	&CCodeGen_Arm::Emit_Fpu_MemMemMem<FPUOP_DIV>	},

	{ OP_FP_RCPL,	MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,		MATCH_NIL,				&CCodeGen_Arm::Emit_Fp_Rcpl_MemMem				},
	{ OP_FP_SQRT,	MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,		MATCH_NIL,				&CCodeGen_Arm::Emit_Fpu_MemMem<FPUOP_SQRT>		},
	{ OP_FP_RSQRT,	MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,		MATCH_NIL,				&CCodeGen_Arm::Emit_Fp_Rsqrt_MemMem				},

	{ OP_FP_ABS,	MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,		MATCH_NIL,				&CCodeGen_Arm::Emit_Fpu_MemMem<FPUOP_ABS>		},
	{ OP_FP_NEG,	MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,		MATCH_NIL,				&CCodeGen_Arm::Emit_Fpu_MemMem<FPUOP_NEG>		},

	{ OP_FP_LDCST,	MATCH_TEMPORARY_FP_SINGLE,	MATCH_CONSTANT,				MATCH_NIL,				&CCodeGen_Arm::Emit_Fp_LdCst_TmpCst				},

	{ OP_MOV,		MATCH_NIL,					MATCH_NIL,					MATCH_NIL,				NULL											},
};
