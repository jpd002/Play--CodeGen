#include "Jitter_CodeGen_Arm.h"

using namespace Jitter;

template <typename FPUOP>
void CCodeGen_Arm::Emit_Fpu_RelRelRel(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_FP_REL_SINGLE);
	assert(src1->m_type == SYM_FP_REL_SINGLE);
	assert(src2->m_type == SYM_FP_REL_SINGLE);

	m_assembler.Flds(CArmAssembler::s0, g_baseRegister, CArmAssembler::MakeImmediateLdrAddress(src1->m_valueLow));
	m_assembler.Flds(CArmAssembler::s1, g_baseRegister, CArmAssembler::MakeImmediateLdrAddress(src2->m_valueLow));
	((m_assembler).*(FPUOP::OpReg()))(CArmAssembler::s2, CArmAssembler::s0, CArmAssembler::s1);
	m_assembler.Fsts(CArmAssembler::s2, g_baseRegister, CArmAssembler::MakeImmediateLdrAddress(dst->m_valueLow));
}

CCodeGen_Arm::CONSTMATCHER CCodeGen_Arm::g_fpuConstMatchers[] = 
{ 
	{ OP_FP_ADD,	MATCH_RELATIVE_FP_SINGLE,	MATCH_RELATIVE_FP_SINGLE,		MATCH_RELATIVE_FP_SINGLE,	&CCodeGen_Arm::Emit_Fpu_RelRelRel<FPUOP_ADD>		},

//	{ OP_FP_MUL,	MATCH_RELATIVE_FP_SINGLE,	MATCH_RELATIVE_FP_SINGLE,		MATCH_RELATIVE_FP_SINGLE,	&CCodeGen_Arm::Emit_Fpu_RelRelRel<FPUOP_MUL>		},

//	{ OP_FP_DIV,	MATCH_RELATIVE_FP_SINGLE,	MATCH_RELATIVE_FP_SINGLE,		MATCH_RELATIVE_FP_SINGLE,	&CCodeGen_Arm::Emit_Fpu_RelRelRel<FPUOP_DIV>		},

//	{ OP_FP_RCPL,	MATCH_RELATIVE_FP_SINGLE,	MATCH_RELATIVE_FP_SINGLE,		MATCH_NIL,					&CCodeGen_Arm::Emit_Fpu_RelRel<FPUOP_RCPL>			},

//	{ OP_FP_SQRT,	MATCH_RELATIVE_FP_SINGLE,	MATCH_RELATIVE_FP_SINGLE,		MATCH_NIL,					&CCodeGen_Arm::Emit_Fpu_RelRel<FPUOP_SQRT>			},

//	{ OP_FP_NEG,	MATCH_RELATIVE_FP_SINGLE,	MATCH_RELATIVE_FP_SINGLE,		MATCH_NIL,					&CCodeGen_Arm::Emit_Fp_Neg_RelRel					},

	{ OP_MOV,		MATCH_NIL,					MATCH_NIL,						MATCH_NIL,					NULL												},
};
