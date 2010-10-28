#include "Jitter_CodeGen_x86.h"

using namespace Jitter;

CX86Assembler::CAddress CCodeGen_x86::MakeRelative128SymbolAddress(CSymbol* symbol)
{
	assert(symbol->m_type == SYM_RELATIVE128);
	assert((symbol->m_valueLow & 0xF) == 0);
	return CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, symbol->m_valueLow);
}

CX86Assembler::CAddress CCodeGen_x86::MakeTemporary128SymbolAddress(CSymbol* symbol)
{
	assert(symbol->m_type == SYM_TEMPORARY128);
	assert(((symbol->m_stackLocation + m_stackLevel) & 0xF) == 0);
	return CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, symbol->m_stackLocation + m_stackLevel);
}

template <typename MDOP>
void CCodeGen_x86::Emit_Md_SymSymSym(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_RELATIVE128);
	assert(src2->m_type == SYM_RELATIVE128);

	m_assembler.MovapsVo(CX86Assembler::xMM0, MakeRelative128SymbolAddress(src1));
	((m_assembler).*(MDOP::OpVo()))(CX86Assembler::xMM0, MakeRelative128SymbolAddress(src2));

	switch(dst->m_type)
	{
	case SYM_RELATIVE128:
		m_assembler.MovapsVo(MakeRelative128SymbolAddress(dst), CX86Assembler::xMM0);
		break;
	case SYM_TEMPORARY128:
		m_assembler.MovapsVo(MakeTemporary128SymbolAddress(dst), CX86Assembler::xMM0);
		break;
	default:
		assert(0);
		break;
	}
}

template <typename MDOP>
void CCodeGen_x86::Emit_Md_SymSymSymRev(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_RELATIVE128);
	assert(src2->m_type == SYM_RELATIVE128);

	m_assembler.MovapsVo(CX86Assembler::xMM0, MakeRelative128SymbolAddress(src2));
	((m_assembler).*(MDOP::OpVo()))(CX86Assembler::xMM0, MakeRelative128SymbolAddress(src1));

	switch(dst->m_type)
	{
	case SYM_RELATIVE128:
		m_assembler.MovapsVo(MakeRelative128SymbolAddress(dst), CX86Assembler::xMM0);
		break;
	case SYM_TEMPORARY128:
		m_assembler.MovapsVo(MakeTemporary128SymbolAddress(dst), CX86Assembler::xMM0);
		break;
	default:
		assert(0);
		break;
	}
}

void CCodeGen_x86::Emit_Md_Not_RelTmp(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type  == SYM_RELATIVE128);
	assert(src1->m_type == SYM_TEMPORARY128);

	m_assembler.MovapsVo(CX86Assembler::xMM0, MakeTemporary128SymbolAddress(src1));
	m_assembler.PcmpeqdVo(CX86Assembler::xMM1, CX86Assembler::MakeXmmRegisterAddress(CX86Assembler::xMM1));
	m_assembler.PxorVo(CX86Assembler::xMM0, CX86Assembler::MakeXmmRegisterAddress(CX86Assembler::xMM1));

	m_assembler.MovapsVo(MakeRelative128SymbolAddress(dst), CX86Assembler::xMM0);
}

CCodeGen_x86::CONSTMATCHER CCodeGen_x86::g_mdConstMatchers[] = 
{
	{ OP_MD_SUB_B,				MATCH_RELATIVE128,			MATCH_RELATIVE128,			MATCH_RELATIVE128,		&CCodeGen_x86::Emit_Md_SymSymSym<MDOP_SUBB>					},

	{ OP_MD_SUB_W,				MATCH_RELATIVE128,			MATCH_RELATIVE128,			MATCH_RELATIVE128,		&CCodeGen_x86::Emit_Md_SymSymSym<MDOP_SUBW>					},

	{ OP_MD_AND,				MATCH_RELATIVE128,			MATCH_RELATIVE128,			MATCH_RELATIVE128,		&CCodeGen_x86::Emit_Md_SymSymSym<MDOP_AND>					},

	{ OP_MD_OR,					MATCH_RELATIVE128,			MATCH_RELATIVE128,			MATCH_RELATIVE128,		&CCodeGen_x86::Emit_Md_SymSymSym<MDOP_OR>					},
	{ OP_MD_OR,					MATCH_TEMPORARY128,			MATCH_RELATIVE128,			MATCH_RELATIVE128,		&CCodeGen_x86::Emit_Md_SymSymSym<MDOP_OR>					},

	{ OP_MD_XOR,				MATCH_RELATIVE128,			MATCH_RELATIVE128,			MATCH_RELATIVE128,		&CCodeGen_x86::Emit_Md_SymSymSym<MDOP_XOR>					},

	{ OP_MD_UNPACK_LOWER_HW,	MATCH_RELATIVE128,			MATCH_RELATIVE128,			MATCH_RELATIVE128,		&CCodeGen_x86::Emit_Md_SymSymSymRev<MDOP_UNPACK_LOWER_HW>	},

	{ OP_MD_NOT,				MATCH_RELATIVE128,			MATCH_TEMPORARY128,			MATCH_NIL,				&CCodeGen_x86::Emit_Md_Not_RelTmp							},

	{ OP_MOV,					MATCH_NIL,					MATCH_NIL,					MATCH_NIL,				NULL														},
};
