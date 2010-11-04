#include "Jitter_CodeGen_x86.h"

using namespace Jitter;

CX86Assembler::CAddress CCodeGen_x86::MakeRelative128SymbolElementAddress(CSymbol* symbol, unsigned int elementIdx)
{
	assert(symbol->m_type == SYM_RELATIVE128);
	assert((symbol->m_valueLow & 0xF) == 0);
	return CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, symbol->m_valueLow + (elementIdx * 4));
}

CX86Assembler::CAddress CCodeGen_x86::MakeTemporary128SymbolElementAddress(CSymbol* symbol, unsigned int elementIdx)
{
	assert(symbol->m_type == SYM_TEMPORARY128);
	assert(((symbol->m_stackLocation + m_stackLevel) & 0xF) == 0);
	return CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, symbol->m_stackLocation + m_stackLevel + (elementIdx * 4));
}

CX86Assembler::CAddress CCodeGen_x86::MakeMemory128SymbolAddress(CSymbol* symbol)
{
	switch(symbol->m_type)
	{
	case SYM_RELATIVE128:
		return MakeRelative128SymbolElementAddress(symbol, 0);
		break;
	case SYM_TEMPORARY128:
		return MakeTemporary128SymbolElementAddress(symbol, 0);
		break;
	default:
		throw std::exception();
		break;
	}
}

CX86Assembler::CAddress CCodeGen_x86::MakeMemory128SymbolElementAddress(CSymbol* symbol, unsigned int elementIdx)
{
	switch(symbol->m_type)
	{
	case SYM_RELATIVE128:
		return MakeRelative128SymbolElementAddress(symbol, elementIdx);
		break;
	case SYM_TEMPORARY128:
		return MakeTemporary128SymbolElementAddress(symbol, elementIdx);
		break;
	default:
		throw std::exception();
		break;
	}
}

template <typename MDOP>
void CCodeGen_x86::Emit_Md_MemMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	((m_assembler).*(MDOP::OpVo()))(CX86Assembler::xMM0, MakeMemory128SymbolAddress(src1));
	m_assembler.MovapsVo(MakeMemory128SymbolAddress(dst), CX86Assembler::xMM0);
}

template <typename MDOP>
void CCodeGen_x86::Emit_Md_MemMemMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	m_assembler.MovapsVo(CX86Assembler::xMM0, MakeMemory128SymbolAddress(src1));
	((m_assembler).*(MDOP::OpVo()))(CX86Assembler::xMM0, MakeMemory128SymbolAddress(src2));
	m_assembler.MovapsVo(MakeMemory128SymbolAddress(dst), CX86Assembler::xMM0);
}

template <typename MDOP>
void CCodeGen_x86::Emit_Md_MemMemMemRev(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	m_assembler.MovapsVo(CX86Assembler::xMM0, MakeMemory128SymbolAddress(src2));
	((m_assembler).*(MDOP::OpVo()))(CX86Assembler::xMM0, MakeMemory128SymbolAddress(src1));
	m_assembler.MovapsVo(MakeMemory128SymbolAddress(dst), CX86Assembler::xMM0);
}

void CCodeGen_x86::Emit_Md_AddSSW_MemMemMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	CX86Assembler::REGISTER overflowReg = CX86Assembler::rAX;
	CX86Assembler::REGISTER underflowReg = CX86Assembler::rDX;
	CX86Assembler::REGISTER resultRegister = CX86Assembler::rCX;

	//Prepare registers
	m_assembler.MovId(overflowReg, 0x7FFFFFFF);
	m_assembler.MovId(underflowReg, 0x80000000);

	for(int i = 0; i < 4; i++)
	{
		CX86Assembler::LABEL doneLabel = m_assembler.CreateLabel();
		m_assembler.MovEd(resultRegister, MakeMemory128SymbolElementAddress(src1, i));
		m_assembler.AddEd(resultRegister, MakeMemory128SymbolElementAddress(src2, i));
		m_assembler.JnoJb(doneLabel);
		m_assembler.CmovsEd(resultRegister, CX86Assembler::MakeRegisterAddress(overflowReg));
		m_assembler.CmovnsEd(resultRegister, CX86Assembler::MakeRegisterAddress(underflowReg));
		m_assembler.MarkLabel(doneLabel);
		m_assembler.MovGd(MakeMemory128SymbolElementAddress(dst, i), resultRegister);
	}
}

void CCodeGen_x86::Emit_Md_Not_RelTmp(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type  == SYM_RELATIVE128);
	assert(src1->m_type == SYM_TEMPORARY128);

	m_assembler.MovapsVo(CX86Assembler::xMM0, MakeMemory128SymbolAddress(src1));
	m_assembler.PcmpeqdVo(CX86Assembler::xMM1, CX86Assembler::MakeXmmRegisterAddress(CX86Assembler::xMM1));
	m_assembler.PxorVo(CX86Assembler::xMM0, CX86Assembler::MakeXmmRegisterAddress(CX86Assembler::xMM1));

	m_assembler.MovapsVo(MakeMemory128SymbolAddress(dst), CX86Assembler::xMM0);
}

void CCodeGen_x86::Emit_Md_MovMasked_MemMemCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_CONSTANT);

	uint8 mask = static_cast<uint8>(src2->m_valueLow);
	assert(mask != 0);

	uint8 shuffle[4] = { 0x00, 0xE5, 0xEA, 0xFF };
	CX86Assembler::XMMREGISTER tmpReg = CX86Assembler::xMM0;

	m_assembler.MovapsVo(tmpReg, MakeMemory128SymbolAddress(src1));
	for(unsigned int i = 0; i < 4; i++)
	{
		if(mask & (1 << i))
		{
			if(i != 0)
			{
				m_assembler.ShufpsVo(tmpReg, CX86Assembler::MakeXmmRegisterAddress(tmpReg), shuffle[i]);
			}

			m_assembler.MovssEd(MakeMemory128SymbolElementAddress(dst, i), tmpReg);
		}
	}
}

void CCodeGen_x86::Emit_Md_IsNegative_MemMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	CX86Assembler::XMMREGISTER resultRegister = CX86Assembler::xMM0;

	m_assembler.MovapsVo(resultRegister, MakeMemory128SymbolAddress(src1));
	m_assembler.PsradVo(resultRegister, 31);
	m_assembler.MovapsVo(MakeMemory128SymbolAddress(dst), resultRegister);
}

void CCodeGen_x86::Emit_Md_IsZero_MemMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	CX86Assembler::XMMREGISTER resultRegister = CX86Assembler::xMM0;
	CX86Assembler::XMMREGISTER zeroRegister = CX86Assembler::xMM1;

	m_assembler.MovapsVo(resultRegister, MakeMemory128SymbolAddress(src1));
	m_assembler.PandnVo(zeroRegister, CX86Assembler::MakeXmmRegisterAddress(zeroRegister));
	m_assembler.PcmpeqdVo(resultRegister, CX86Assembler::MakeXmmRegisterAddress(zeroRegister));
	m_assembler.MovapsVo(MakeMemory128SymbolAddress(dst), resultRegister);
}

void CCodeGen_x86::Emit_Md_Expand_MemReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	assert(src1->m_type == SYM_REGISTER);

	CX86Assembler::XMMREGISTER resultRegister = CX86Assembler::xMM0;

	m_assembler.MovdVo(resultRegister, CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
    m_assembler.ShufpsVo(resultRegister, CX86Assembler::MakeXmmRegisterAddress(resultRegister), 0x00);
	m_assembler.MovapsVo(MakeMemory128SymbolAddress(dst), resultRegister);
}

void CCodeGen_x86::Emit_Md_Expand_MemMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	CX86Assembler::XMMREGISTER resultRegister = CX86Assembler::xMM0;

	m_assembler.MovssEd(resultRegister, MakeMemorySymbolAddress(src1));
    m_assembler.ShufpsVo(resultRegister, CX86Assembler::MakeXmmRegisterAddress(resultRegister), 0x00);
	m_assembler.MovapsVo(MakeMemory128SymbolAddress(dst), resultRegister);
}

void CCodeGen_x86::Emit_Md_Expand_MemCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	assert(src1->m_type == SYM_CONSTANT);

	CX86Assembler::REGISTER cstRegister = CX86Assembler::rAX;
	CX86Assembler::XMMREGISTER resultRegister = CX86Assembler::xMM0;

	m_assembler.MovId(cstRegister, src1->m_valueLow);
	m_assembler.MovdVo(resultRegister, CX86Assembler::MakeRegisterAddress(cstRegister));
    m_assembler.ShufpsVo(resultRegister, CX86Assembler::MakeXmmRegisterAddress(resultRegister), 0x00);
	m_assembler.MovapsVo(MakeMemory128SymbolAddress(dst), resultRegister);
}

CCodeGen_x86::CONSTMATCHER CCodeGen_x86::g_mdConstMatchers[] = 
{
	{ OP_MD_ADDSS_W,			MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_x86::Emit_Md_AddSSW_MemMemMem						},

	{ OP_MD_SUB_B,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_x86::Emit_Md_MemMemMem<MDOP_SUBB>					},
	{ OP_MD_SUB_W,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_x86::Emit_Md_MemMemMem<MDOP_SUBW>					},

	{ OP_MD_AND,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_x86::Emit_Md_MemMemMem<MDOP_AND>					},
	{ OP_MD_OR,					MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_x86::Emit_Md_MemMemMem<MDOP_OR>					},
	{ OP_MD_XOR,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_x86::Emit_Md_MemMemMem<MDOP_XOR>					},

	{ OP_MD_EXPAND,				MATCH_MEMORY128,			MATCH_REGISTER,				MATCH_NIL,				&CCodeGen_x86::Emit_Md_Expand_MemReg						},
	{ OP_MD_EXPAND,				MATCH_MEMORY128,			MATCH_MEMORY,				MATCH_NIL,				&CCodeGen_x86::Emit_Md_Expand_MemMem						},
	{ OP_MD_EXPAND,				MATCH_MEMORY128,			MATCH_CONSTANT,				MATCH_NIL,				&CCodeGen_x86::Emit_Md_Expand_MemCst						},
	{ OP_MD_UNPACK_LOWER_HW,	MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_x86::Emit_Md_MemMemMemRev<MDOP_UNPACK_LOWER_HW>	},
	{ OP_MD_UNPACK_LOWER_WD,	MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_x86::Emit_Md_MemMemMemRev<MDOP_UNPACK_LOWER_WD>	},
	{ OP_MD_UNPACK_UPPER_WD,	MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_x86::Emit_Md_MemMemMemRev<MDOP_UNPACK_UPPER_WD>	},

	{ OP_MD_ADD_S,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_x86::Emit_Md_MemMemMem<MDOP_ADDS>					},
	{ OP_MD_SUB_S,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_x86::Emit_Md_MemMemMem<MDOP_SUBS>					},
	{ OP_MD_MUL_S,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_x86::Emit_Md_MemMemMem<MDOP_MULS>					},

	{ OP_MD_NOT,				MATCH_RELATIVE128,			MATCH_TEMPORARY128,			MATCH_NIL,				&CCodeGen_x86::Emit_Md_Not_RelTmp							},

	{ OP_MD_ISNEGATIVE,			MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_NIL,				&CCodeGen_x86::Emit_Md_IsNegative_MemMem					},
	{ OP_MD_ISZERO,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_NIL,				&CCodeGen_x86::Emit_Md_IsZero_MemMem						},
	{ OP_MD_TOWORD_TRUNCATE,	MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_NIL,				&CCodeGen_x86::Emit_Md_MemMem<MDOP_TOWORD_TRUNCATE>			},

	{ OP_MD_MOV_MASKED,			MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_CONSTANT,			&CCodeGen_x86::Emit_Md_MovMasked_MemMemCst					},

	{ OP_MOV,					MATCH_NIL,					MATCH_NIL,					MATCH_NIL,				NULL														},
};
