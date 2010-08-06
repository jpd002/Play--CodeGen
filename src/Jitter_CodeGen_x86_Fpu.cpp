#include "Jitter_CodeGen_x86.h"

using namespace Jitter;

template <typename FPUOP>
void CCodeGen_x86::Emit_Fpu_RelRel(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	((m_assembler).*(FPUOP::OpEd()))(CX86Assembler::xMM0, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow));
	m_assembler.MovssEd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, dst->m_valueLow), CX86Assembler::xMM0);
}

template <typename FPUOP>
void CCodeGen_x86::Emit_Fpu_RelRelRel(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	m_assembler.MovssEd(CX86Assembler::xMM0, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow));
	((m_assembler).*(FPUOP::OpEd()))(CX86Assembler::xMM0, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src2->m_valueLow));
	m_assembler.MovssEd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, dst->m_valueLow), CX86Assembler::xMM0);
}

void CCodeGen_x86::Emit_Fp_Neg_RelRel(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow));
	m_assembler.XorId(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX), 0x80000000);
	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, dst->m_valueLow), CX86Assembler::rAX);
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
	{ OP_FP_ADD,			MATCH_RELATIVE_FP_SINGLE,	MATCH_RELATIVE_FP_SINGLE,		MATCH_RELATIVE_FP_SINGLE,	&CCodeGen_x86::Emit_Fpu_RelRelRel<FPUOP_ADD>		},

	{ OP_FP_MUL,			MATCH_RELATIVE_FP_SINGLE,	MATCH_RELATIVE_FP_SINGLE,		MATCH_RELATIVE_FP_SINGLE,	&CCodeGen_x86::Emit_Fpu_RelRelRel<FPUOP_MUL>		},

	{ OP_FP_DIV,			MATCH_RELATIVE_FP_SINGLE,	MATCH_RELATIVE_FP_SINGLE,		MATCH_RELATIVE_FP_SINGLE,	&CCodeGen_x86::Emit_Fpu_RelRelRel<FPUOP_DIV>		},

	{ OP_FP_RCPL,			MATCH_RELATIVE_FP_SINGLE,	MATCH_RELATIVE_FP_SINGLE,		MATCH_NIL,					&CCodeGen_x86::Emit_Fpu_RelRel<FPUOP_RCPL>			},

	{ OP_FP_SQRT,			MATCH_RELATIVE_FP_SINGLE,	MATCH_RELATIVE_FP_SINGLE,		MATCH_NIL,					&CCodeGen_x86::Emit_Fpu_RelRel<FPUOP_SQRT>			},

	{ OP_FP_NEG,			MATCH_RELATIVE_FP_SINGLE,	MATCH_RELATIVE_FP_SINGLE,		MATCH_NIL,					&CCodeGen_x86::Emit_Fp_Neg_RelRel					},

	{ OP_FP_TOINT_TRUNC,	MATCH_RELATIVE_FP_SINGLE,	MATCH_RELATIVE_FP_SINGLE,		MATCH_NIL,					&CCodeGen_x86::Emit_Fp_ToIntTrunc_RelRel			},

	{ OP_MOV,				MATCH_NIL,					MATCH_NIL,						MATCH_NIL,					NULL												},
};
