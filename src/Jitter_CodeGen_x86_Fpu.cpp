#include "Jitter_CodeGen_x86.h"

using namespace Jitter;

CX86Assembler::CAddress CCodeGen_x86::MakeRelativeFpSingleSymbolAddress(CSymbol* symbol)
{
	assert(symbol->m_type == SYM_FP_REL_SINGLE);
	assert((symbol->m_valueLow & 0x3) == 0);
	return CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, symbol->m_valueLow);
}

CX86Assembler::CAddress CCodeGen_x86::MakeTemporaryFpSingleSymbolAddress(CSymbol* symbol)
{
	assert(symbol->m_type == SYM_FP_TMP_SINGLE);
	assert(((symbol->m_stackLocation + m_stackLevel) & 0x3) == 0);
	return CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, symbol->m_stackLocation + m_stackLevel);
}

CX86Assembler::CAddress CCodeGen_x86::MakeMemoryFpSingleSymbolAddress(CSymbol* symbol)
{
	switch(symbol->m_type)
	{
	case SYM_FP_REL_SINGLE:
		return MakeRelativeFpSingleSymbolAddress(symbol);
		break;
	case SYM_FP_TMP_SINGLE:
		return MakeTemporaryFpSingleSymbolAddress(symbol);
		break;
	default:
		throw std::exception();
		break;
	}
}

template <typename FPUOP>
void CCodeGen_x86::Emit_Fpu_MemMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	((m_assembler).*(FPUOP::OpEd()))(CX86Assembler::xMM0, MakeMemoryFpSingleSymbolAddress(src1));
	m_assembler.MovssEd(MakeMemoryFpSingleSymbolAddress(dst), CX86Assembler::xMM0);
}

template <typename FPUOP>
void CCodeGen_x86::Emit_Fpu_MemMemMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	m_assembler.MovssEd(CX86Assembler::xMM0, MakeMemoryFpSingleSymbolAddress(src1));
	((m_assembler).*(FPUOP::OpEd()))(CX86Assembler::xMM0, MakeMemoryFpSingleSymbolAddress(src2));
	m_assembler.MovssEd(MakeMemoryFpSingleSymbolAddress(dst), CX86Assembler::xMM0);
}

void CCodeGen_x86::Emit_Fp_Cmp_RelRel(CX86Assembler::REGISTER dstReg, const STATEMENT& statement)
{
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_FP_REL_SINGLE);
	assert(src2->m_type == SYM_FP_REL_SINGLE);

    CX86Assembler::SSE_CMP_TYPE conditionCode;
    switch(statement.jmpCondition)
    {
    case CONDITION_EQ:
        conditionCode = CX86Assembler::SSE_CMP_EQ;
        break;
    case CONDITION_BL:
        conditionCode = CX86Assembler::SSE_CMP_LT;
        break;
    case CONDITION_BE:
        conditionCode = CX86Assembler::SSE_CMP_LE;
        break;
    case CONDITION_AB:
        conditionCode = CX86Assembler::SSE_CMP_NLE;
        break;
    default:
        assert(0);
        break;
    }

    //Can't move directly to register using MOVSS, so we use CVTTSS2SI
    //0x00000000 -- CVT -> zero
    //0xFFFFFFFF -- CVT -> not zero
	m_assembler.MovssEd(CX86Assembler::xMM0, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow));
	m_assembler.CmpssEd(CX86Assembler::xMM0, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src2->m_valueLow), conditionCode);
    m_assembler.Cvttss2siEd(dstReg, CX86Assembler::MakeXmmRegisterAddress(CX86Assembler::xMM0));
}

void CCodeGen_x86::Emit_Fp_Cmp_SymRelRel(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();

	switch(dst->m_type)
	{
	case SYM_REGISTER:
		Emit_Fp_Cmp_RelRel(m_registers[dst->m_valueLow], statement);
		break;
	case SYM_RELATIVE:
	case SYM_TEMPORARY:
		Emit_Fp_Cmp_RelRel(CX86Assembler::rAX, statement);
		m_assembler.MovGd(MakeMemorySymbolAddress(dst), CX86Assembler::rAX);
		break;
	default:
		assert(0);
		break;
	}
}

void CCodeGen_x86::Emit_Fp_Abs_MemMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	m_assembler.MovEd(CX86Assembler::rAX, MakeMemoryFpSingleSymbolAddress(src1));
	m_assembler.AndId(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX), 0x7FFFFFFF);
	m_assembler.MovGd(MakeMemoryFpSingleSymbolAddress(dst), CX86Assembler::rAX);
}

void CCodeGen_x86::Emit_Fp_Neg_MemMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	m_assembler.MovEd(CX86Assembler::rAX, MakeMemoryFpSingleSymbolAddress(src1));
	m_assembler.XorId(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX), 0x80000000);
	m_assembler.MovGd(MakeMemoryFpSingleSymbolAddress(dst), CX86Assembler::rAX);
}

void CCodeGen_x86::Emit_Fp_Mov_RelSRelI32(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type  == SYM_FP_REL_SINGLE);
	assert(src1->m_type == SYM_FP_REL_INT32);

	m_assembler.Cvtsi2ssEd(CX86Assembler::xMM0, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow));
	m_assembler.MovssEd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, dst->m_valueLow), CX86Assembler::xMM0);
}

void CCodeGen_x86::Emit_Fp_ToIntTrunc_RelRel(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type  == SYM_FP_REL_SINGLE);
	assert(src1->m_type == SYM_FP_REL_SINGLE);

	m_assembler.Cvttss2siEd(CX86Assembler::rAX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow));
	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, dst->m_valueLow), CX86Assembler::rAX);
}

CCodeGen_x86::CONSTMATCHER CCodeGen_x86::g_fpuConstMatchers[] = 
{ 
	{ OP_FP_ADD,			MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,			MATCH_MEMORY_FP_SINGLE,		&CCodeGen_x86::Emit_Fpu_MemMemMem<FPUOP_ADD>		},

	{ OP_FP_SUB,			MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,			MATCH_MEMORY_FP_SINGLE,		&CCodeGen_x86::Emit_Fpu_MemMemMem<FPUOP_SUB>		},

	{ OP_FP_MUL,			MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,			MATCH_MEMORY_FP_SINGLE,		&CCodeGen_x86::Emit_Fpu_MemMemMem<FPUOP_MUL>		},

	{ OP_FP_DIV,			MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,			MATCH_MEMORY_FP_SINGLE,		&CCodeGen_x86::Emit_Fpu_MemMemMem<FPUOP_DIV>		},

	{ OP_FP_CMP,			MATCH_REGISTER,				MATCH_RELATIVE_FP_SINGLE,		MATCH_RELATIVE_FP_SINGLE,	&CCodeGen_x86::Emit_Fp_Cmp_SymRelRel				},

	{ OP_FP_CMP,			MATCH_RELATIVE,				MATCH_RELATIVE_FP_SINGLE,		MATCH_RELATIVE_FP_SINGLE,	&CCodeGen_x86::Emit_Fp_Cmp_SymRelRel				},

	{ OP_FP_CMP,			MATCH_TEMPORARY,			MATCH_RELATIVE_FP_SINGLE,		MATCH_RELATIVE_FP_SINGLE,	&CCodeGen_x86::Emit_Fp_Cmp_SymRelRel				},

	{ OP_FP_SQRT,			MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,			MATCH_NIL,					&CCodeGen_x86::Emit_Fpu_MemMem<FPUOP_SQRT>			},

	{ OP_FP_RSQRT,			MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,			MATCH_NIL,					&CCodeGen_x86::Emit_Fpu_MemMem<FPUOP_RSQRT>			},

	{ OP_FP_RCPL,			MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,			MATCH_NIL,					&CCodeGen_x86::Emit_Fpu_MemMem<FPUOP_RCPL>			},

	{ OP_FP_ABS,			MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,			MATCH_NIL,					&CCodeGen_x86::Emit_Fp_Abs_MemMem					},

	{ OP_FP_NEG,			MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,			MATCH_NIL,					&CCodeGen_x86::Emit_Fp_Neg_MemMem					},

	{ OP_MOV,				MATCH_RELATIVE_FP_SINGLE,	MATCH_RELATIVE_FP_INT32,		MATCH_NIL,					&CCodeGen_x86::Emit_Fp_Mov_RelSRelI32				},

	{ OP_FP_TOINT_TRUNC,	MATCH_RELATIVE_FP_SINGLE,	MATCH_RELATIVE_FP_SINGLE,		MATCH_NIL,					&CCodeGen_x86::Emit_Fp_ToIntTrunc_RelRel			},

	{ OP_MOV,				MATCH_NIL,					MATCH_NIL,						MATCH_NIL,					NULL												},
};
