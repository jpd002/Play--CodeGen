#ifndef _JITTER_CODEGEN_X86_SHIFT_H_
#define _JITTER_CODEGEN_X86_SHIFT_H_

template <typename SHIFTOP>
void CCodeGen_x86::Emit_Shift_RegRegReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	m_assembler.MovEd(CX86Assembler::rCX, CX86Assembler::MakeRegisterAddress(m_registers[src2->m_valueLow]));

	if(!dst->Equals(src1))
	{
		m_assembler.MovEd(m_registers[dst->m_valueLow], CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	}

	((m_assembler).*(SHIFTOP::OpVar()))(CX86Assembler::MakeRegisterAddress(m_registers[dst->m_valueLow]));
}

template <typename SHIFTOP>
void CCodeGen_x86::Emit_Shift_RegRegRel(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	m_assembler.MovEd(CX86Assembler::rCX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src2->m_valueLow));

	if(!dst->Equals(src1))
	{
		m_assembler.MovEd(m_registers[dst->m_valueLow], CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	}

	((m_assembler).*(SHIFTOP::OpVar()))(CX86Assembler::MakeRegisterAddress(m_registers[dst->m_valueLow]));
}

template <typename SHIFTOP>
void CCodeGen_x86::Emit_Shift_RegRegCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	if(!dst->Equals(src1))
	{
		m_assembler.MovEd(m_registers[dst->m_valueLow], CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	}

	((m_assembler).*(SHIFTOP::OpCst()))(CX86Assembler::MakeRegisterAddress(m_registers[dst->m_valueLow]), static_cast<uint8>(src2->m_valueLow));
}

template <typename SHIFTOP>
void CCodeGen_x86::Emit_Shift_RegRelReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_REGISTER);
	assert(src1->m_type == SYM_RELATIVE);
	assert(src2->m_type == SYM_REGISTER);

	m_assembler.MovEd(CX86Assembler::rCX, CX86Assembler::MakeRegisterAddress(m_registers[src2->m_valueLow]));
	m_assembler.MovEd(m_registers[dst->m_valueLow], CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow));
	((m_assembler).*(SHIFTOP::OpVar()))(CX86Assembler::MakeRegisterAddress(m_registers[dst->m_valueLow]));
}

template <typename SHIFTOP>
void CCodeGen_x86::Emit_Shift_RegRelRel(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_REGISTER);
	assert(src1->m_type == SYM_RELATIVE);
	assert(src2->m_type == SYM_RELATIVE);

	m_assembler.MovEd(CX86Assembler::rCX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src2->m_valueLow));
	m_assembler.MovEd(m_registers[dst->m_valueLow], CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow));
	((m_assembler).*(SHIFTOP::OpVar()))(CX86Assembler::MakeRegisterAddress(m_registers[dst->m_valueLow]));
}

template <typename SHIFTOP>
void CCodeGen_x86::Emit_Shift_RegRelCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	m_assembler.MovEd(m_registers[dst->m_valueLow], CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow));
	((m_assembler).*(SHIFTOP::OpCst()))(CX86Assembler::MakeRegisterAddress(m_registers[dst->m_valueLow]), static_cast<uint8>(src2->m_valueLow));
}

template <typename SHIFTOP>
void CCodeGen_x86::Emit_Shift_RegCstReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_REGISTER);
	assert(src1->m_type == SYM_CONSTANT);
	assert(src2->m_type == SYM_REGISTER);

	m_assembler.MovEd(CX86Assembler::rCX, CX86Assembler::MakeRegisterAddress(m_registers[src2->m_valueLow]));
	m_assembler.MovId(m_registers[dst->m_valueLow], src1->m_valueLow);
	((m_assembler).*(SHIFTOP::OpVar()))(CX86Assembler::MakeRegisterAddress(m_registers[dst->m_valueLow]));
}

template <typename SHIFTOP>
void CCodeGen_x86::Emit_Shift_RegCstRel(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_REGISTER);
	assert(src1->m_type == SYM_CONSTANT);
	assert(src2->m_type == SYM_RELATIVE);

	m_assembler.MovEd(CX86Assembler::rCX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src2->m_valueLow));
	m_assembler.MovId(m_registers[dst->m_valueLow], src1->m_valueLow);
	((m_assembler).*(SHIFTOP::OpVar()))(CX86Assembler::MakeRegisterAddress(m_registers[dst->m_valueLow]));
}

template <typename SHIFTOP>
void CCodeGen_x86::Emit_Shift_RegTmpCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_REGISTER);
	assert(src1->m_type == SYM_TEMPORARY);
	assert(src2->m_type == SYM_CONSTANT);

	m_assembler.MovEd(m_registers[dst->m_valueLow], CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, src1->m_stackLocation + m_stackLevel));
	((m_assembler).*(SHIFTOP::OpCst()))(CX86Assembler::MakeRegisterAddress(m_registers[dst->m_valueLow]), static_cast<uint8>(src2->m_valueLow));
}

template <typename SHIFTOP>
void CCodeGen_x86::Emit_Shift_RelRegReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	m_assembler.MovEd(CX86Assembler::rCX, CX86Assembler::MakeRegisterAddress(m_registers[src2->m_valueLow]));
	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	((m_assembler).*(SHIFTOP::OpVar()))(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX));
	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, dst->m_valueLow), CX86Assembler::rAX);
}

template <typename SHIFTOP>
void CCodeGen_x86::Emit_Shift_RelRegRel(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_RELATIVE);
	assert(src1->m_type == SYM_REGISTER);
	assert(src2->m_type == SYM_RELATIVE);

	m_assembler.MovEd(CX86Assembler::rCX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src2->m_valueLow));
	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	((m_assembler).*(SHIFTOP::OpVar()))(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX));
	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, dst->m_valueLow), CX86Assembler::rAX);
}

template <typename SHIFTOP>
void CCodeGen_x86::Emit_Shift_RelRegCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_RELATIVE);
	assert(src1->m_type == SYM_REGISTER);
	assert(src2->m_type == SYM_CONSTANT);

	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	((m_assembler).*(SHIFTOP::OpCst()))(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX), static_cast<uint8>(src2->m_valueLow));
	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, dst->m_valueLow), CX86Assembler::rAX);
}

template <typename SHIFTOP>
void CCodeGen_x86::Emit_Shift_RelRelReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_RELATIVE);
	assert(src1->m_type == SYM_RELATIVE);
	assert(src2->m_type == SYM_REGISTER);

	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow));
	m_assembler.MovEd(CX86Assembler::rCX, CX86Assembler::MakeRegisterAddress(m_registers[src2->m_valueLow]));
	((m_assembler).*(SHIFTOP::OpVar()))(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX));
	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, dst->m_valueLow), CX86Assembler::rAX);
}

template <typename SHIFTOP>
void CCodeGen_x86::Emit_Shift_RelRelRel(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_RELATIVE);
	assert(src1->m_type == SYM_RELATIVE);
	assert(src2->m_type == SYM_RELATIVE);

	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow));
	m_assembler.MovEd(CX86Assembler::rCX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src2->m_valueLow));
	((m_assembler).*(SHIFTOP::OpVar()))(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX));
	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, dst->m_valueLow), CX86Assembler::rAX);
}

template <typename SHIFTOP>
void CCodeGen_x86::Emit_Shift_RelRelCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_RELATIVE);
	assert(src1->m_type == SYM_RELATIVE);
	assert(src2->m_type == SYM_CONSTANT);

	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow));
	((m_assembler).*(SHIFTOP::OpCst()))(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX), static_cast<uint8>(src2->m_valueLow));
	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, dst->m_valueLow), CX86Assembler::rAX);
}

template <typename SHIFTOP>
void CCodeGen_x86::Emit_Shift_RelCstReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_RELATIVE);
	assert(src1->m_type == SYM_CONSTANT);
	assert(src2->m_type == SYM_REGISTER);

	m_assembler.MovId(CX86Assembler::rAX, src1->m_valueLow);
	m_assembler.MovEd(CX86Assembler::rCX, CX86Assembler::MakeRegisterAddress(m_registers[src2->m_valueLow]));
	((m_assembler).*(SHIFTOP::OpVar()))(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX));
	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, dst->m_valueLow), CX86Assembler::rAX);
}

template <typename SHIFTOP>
void CCodeGen_x86::Emit_Shift_RelCstRel(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_RELATIVE);
	assert(src1->m_type == SYM_CONSTANT);
	assert(src2->m_type == SYM_RELATIVE);

	m_assembler.MovId(CX86Assembler::rAX, src1->m_valueLow);
	m_assembler.MovEd(CX86Assembler::rCX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src2->m_valueLow));
	((m_assembler).*(SHIFTOP::OpVar()))(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX));
	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, dst->m_valueLow), CX86Assembler::rAX);
}

template <typename SHIFTOP>
void CCodeGen_x86::Emit_Shift_RelTmpCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_RELATIVE);
	assert(src1->m_type == SYM_TEMPORARY);
	assert(src2->m_type == SYM_CONSTANT);

	m_assembler.MovEd(CX86Assembler::rAX, MakeTemporarySymbolAddress(src1));
	((m_assembler).*(SHIFTOP::OpCst()))(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX), static_cast<uint8>(src2->m_valueLow));
	m_assembler.MovGd(MakeRelativeSymbolAddress(dst), CX86Assembler::rAX);
}

template <typename SHIFTOP>
void CCodeGen_x86::Emit_Shift_TmpRegCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_TEMPORARY);
	assert(src1->m_type == SYM_REGISTER);
	assert(src2->m_type == SYM_CONSTANT);

	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	((m_assembler).*(SHIFTOP::OpCst()))(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX), static_cast<uint8>(src2->m_valueLow));
	m_assembler.MovGd(MakeTemporarySymbolAddress(dst), CX86Assembler::rAX);
}

template <typename SHIFTOP>
void CCodeGen_x86::Emit_Shift_TmpRelCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_TEMPORARY);
	assert(src1->m_type == SYM_RELATIVE);
	assert(src2->m_type == SYM_CONSTANT);

	m_assembler.MovEd(CX86Assembler::rAX, MakeRelativeSymbolAddress(src1));
	((m_assembler).*(SHIFTOP::OpCst()))(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX), static_cast<uint8>(src2->m_valueLow));
	m_assembler.MovGd(MakeTemporarySymbolAddress(dst), CX86Assembler::rAX);
}

template <typename SHIFTOP>
void CCodeGen_x86::Emit_Shift_TmpCstRel(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_TEMPORARY);
	assert(src1->m_type == SYM_CONSTANT);
	assert(src2->m_type == SYM_RELATIVE);

	m_assembler.MovId(CX86Assembler::rAX, src1->m_valueLow);
	m_assembler.MovEd(CX86Assembler::rCX, MakeRelativeSymbolAddress(src2));
	((m_assembler).*(SHIFTOP::OpVar()))(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX));
	m_assembler.MovGd(MakeTemporarySymbolAddress(dst), CX86Assembler::rAX);
}

//template <typename SHIFTOP>
//void CCodeGen_x86::Emit_Shift_TmpCstReg(const STATEMENT& statement)
//{
//	CSymbol* dst = statement.dst->GetSymbol().get();
//	CSymbol* src1 = statement.src1->GetSymbol().get();
//	CSymbol* src2 = statement.src2->GetSymbol().get();
//
//	assert(dst->m_type  == SYM_TEMPORARY);
//	assert(src1->m_type == SYM_CONSTANT);
//	assert(src2->m_type == SYM_REGISTER);
//
//	m_assembler.MovId(CX86Assembler::rAX, src1->m_valueLow);
//	m_assembler.MovEd(CX86Assembler::rCX, CX86Assembler::MakeRegisterAddress(m_registers[src2->m_valueLow]));
//	((m_assembler).*(SHIFTOP::OpVar()))(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX));
//	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, dst->m_stackLocation + m_stackLevel), CX86Assembler::rAX);
//}

template <typename SHIFTOP>
void CCodeGen_x86::Emit_Shift_TmpTmpCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_TEMPORARY);
	assert(src1->m_type == SYM_TEMPORARY);
	assert(src2->m_type == SYM_CONSTANT);

	if(dst->Equals(src1))
	{
		((m_assembler).*(SHIFTOP::OpCst()))(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, dst->m_stackLocation + m_stackLevel), 
			static_cast<uint8>(src2->m_valueLow));
	}
	else
	{
		m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, src1->m_stackLocation + m_stackLevel));
		((m_assembler).*(SHIFTOP::OpCst()))(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX), static_cast<uint8>(src2->m_valueLow));
		m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, dst->m_stackLocation + m_stackLevel), CX86Assembler::rAX);
	}
}

#define SHIFT_CONST_MATCHERS(SHIFTOP_CST, SHIFTOP) \
	{ SHIFTOP_CST,	MATCH_REGISTER,		MATCH_REGISTER,		MATCH_REGISTER,		&CCodeGen_x86::Emit_Shift_RegRegReg<SHIFTOP>	}, \
	{ SHIFTOP_CST,	MATCH_REGISTER,		MATCH_REGISTER,		MATCH_RELATIVE,		&CCodeGen_x86::Emit_Shift_RegRegRel<SHIFTOP>	}, \
	{ SHIFTOP_CST,	MATCH_REGISTER,		MATCH_REGISTER,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_Shift_RegRegCst<SHIFTOP>	}, \
	{ SHIFTOP_CST,	MATCH_REGISTER,		MATCH_RELATIVE,		MATCH_REGISTER,		&CCodeGen_x86::Emit_Shift_RegRelReg<SHIFTOP>	}, \
	{ SHIFTOP_CST,	MATCH_REGISTER,		MATCH_RELATIVE,		MATCH_RELATIVE,		&CCodeGen_x86::Emit_Shift_RegRelRel<SHIFTOP>	}, \
	{ SHIFTOP_CST,	MATCH_REGISTER,		MATCH_RELATIVE,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_Shift_RegRelCst<SHIFTOP>	}, \
	{ SHIFTOP_CST,	MATCH_REGISTER,		MATCH_CONSTANT,		MATCH_REGISTER,		&CCodeGen_x86::Emit_Shift_RegCstReg<SHIFTOP>	}, \
	{ SHIFTOP_CST,	MATCH_REGISTER,		MATCH_CONSTANT,		MATCH_RELATIVE,		&CCodeGen_x86::Emit_Shift_RegCstRel<SHIFTOP>	}, \
	{ SHIFTOP_CST,	MATCH_REGISTER,		MATCH_TEMPORARY,	MATCH_CONSTANT,		&CCodeGen_x86::Emit_Shift_RegTmpCst<SHIFTOP>	}, \
	{ SHIFTOP_CST,	MATCH_RELATIVE,		MATCH_REGISTER,		MATCH_REGISTER,		&CCodeGen_x86::Emit_Shift_RelRegReg<SHIFTOP>	}, \
	{ SHIFTOP_CST,	MATCH_RELATIVE,		MATCH_REGISTER,		MATCH_RELATIVE,		&CCodeGen_x86::Emit_Shift_RelRegRel<SHIFTOP>	}, \
	{ SHIFTOP_CST,	MATCH_RELATIVE,		MATCH_REGISTER,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_Shift_RelRegCst<SHIFTOP>	}, \
	{ SHIFTOP_CST,	MATCH_RELATIVE,		MATCH_RELATIVE,		MATCH_REGISTER,		&CCodeGen_x86::Emit_Shift_RelRelReg<SHIFTOP>	}, \
	{ SHIFTOP_CST,	MATCH_RELATIVE,		MATCH_RELATIVE,		MATCH_RELATIVE,		&CCodeGen_x86::Emit_Shift_RelRelRel<SHIFTOP>	}, \
	{ SHIFTOP_CST,	MATCH_RELATIVE,		MATCH_CONSTANT,		MATCH_REGISTER,		&CCodeGen_x86::Emit_Shift_RelCstReg<SHIFTOP>	}, \
	{ SHIFTOP_CST,	MATCH_RELATIVE,		MATCH_CONSTANT,		MATCH_RELATIVE,		&CCodeGen_x86::Emit_Shift_RelCstRel<SHIFTOP>	}, \
	{ SHIFTOP_CST,	MATCH_RELATIVE,		MATCH_RELATIVE,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_Shift_RelRelCst<SHIFTOP>	}, \
	{ SHIFTOP_CST,	MATCH_RELATIVE,		MATCH_TEMPORARY,	MATCH_CONSTANT,		&CCodeGen_x86::Emit_Shift_RelTmpCst<SHIFTOP>	}, \
	{ SHIFTOP_CST,	MATCH_TEMPORARY,	MATCH_REGISTER,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_Shift_TmpRegCst<SHIFTOP>	}, \
	{ SHIFTOP_CST,	MATCH_TEMPORARY,	MATCH_RELATIVE,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_Shift_TmpRelCst<SHIFTOP>	}, \
	{ SHIFTOP_CST,	MATCH_TEMPORARY,	MATCH_CONSTANT,		MATCH_RELATIVE,		&CCodeGen_x86::Emit_Shift_TmpCstRel<SHIFTOP>	}, \
	{ SHIFTOP_CST,	MATCH_TEMPORARY,	MATCH_TEMPORARY,	MATCH_CONSTANT,		&CCodeGen_x86::Emit_Shift_TmpTmpCst<SHIFTOP>	},

#endif
