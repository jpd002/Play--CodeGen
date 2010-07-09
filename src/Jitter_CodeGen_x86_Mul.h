#ifndef _JITTER_CODEGEN_X86_MUL_H_
#define _JITTER_CODEGEN_X86_MUL_H_

template <bool isSigned>
void CCodeGen_x86::Emit_MulTmp64RegCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	m_assembler.MovId(CX86Assembler::rAX, src2->m_valueLow);
	if(isSigned)
	{
		m_assembler.ImulEd(CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	}
	else
	{
		m_assembler.MulEd(CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	}
	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, dst->m_stackLocation + m_stackLevel + 0), CX86Assembler::rAX);
	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, dst->m_stackLocation + m_stackLevel + 4), CX86Assembler::rDX);
}

template <bool isSigned>
void CCodeGen_x86::Emit_MulTmp64RegRel(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src2->m_valueLow));
	if(isSigned)
	{
		m_assembler.ImulEd(CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	}
	else
	{
		m_assembler.MulEd(CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	}
	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, dst->m_stackLocation + m_stackLevel + 0), CX86Assembler::rAX);
	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, dst->m_stackLocation + m_stackLevel + 4), CX86Assembler::rDX);
}

template <bool isSigned>
void CCodeGen_x86::Emit_MulTmp64RegReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeRegisterAddress(m_registers[src2->m_valueLow]));
	if(isSigned)
	{
		m_assembler.ImulEd(CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	}
	else
	{
		m_assembler.MulEd(CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	}
	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, dst->m_stackLocation + m_stackLevel + 0), CX86Assembler::rAX);
	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, dst->m_stackLocation + m_stackLevel + 4), CX86Assembler::rDX);
}

#endif
