#pragma once

template <bool isSigned>
void CCodeGen_x86::Emit_DivMem64VarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	m_assembler.MovEd(CX86Assembler::rAX, MakeVariableSymbolAddress(src1));
	if(isSigned)
	{
		m_assembler.Cdq();
		m_assembler.IdivEd(MakeVariableSymbolAddress(src2));
	}
	else
	{
		m_assembler.XorEd(CX86Assembler::rDX, CX86Assembler::MakeRegisterAddress(CX86Assembler::rDX));
		m_assembler.DivEd(MakeVariableSymbolAddress(src2));
	}
	m_assembler.MovGd(MakeMemory64SymbolLoAddress(dst), CX86Assembler::rAX);
	m_assembler.MovGd(MakeMemory64SymbolHiAddress(dst), CX86Assembler::rDX);
}

template <bool isSigned>
void CCodeGen_x86::Emit_DivMem64VarCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_CONSTANT);

	m_assembler.MovEd(CX86Assembler::rAX, MakeVariableSymbolAddress(src1));
	m_assembler.MovId(CX86Assembler::rCX, src2->m_valueLow);
	if(isSigned)
	{
		m_assembler.Cdq();
		m_assembler.IdivEd(CX86Assembler::MakeRegisterAddress(CX86Assembler::rCX));
	}
	else
	{
		m_assembler.XorEd(CX86Assembler::rDX, CX86Assembler::MakeRegisterAddress(CX86Assembler::rDX));
		m_assembler.DivEd(CX86Assembler::MakeRegisterAddress(CX86Assembler::rCX));
	}
	m_assembler.MovGd(MakeMemory64SymbolLoAddress(dst), CX86Assembler::rAX);
	m_assembler.MovGd(MakeMemory64SymbolHiAddress(dst), CX86Assembler::rDX);
}

template <bool isSigned>
void CCodeGen_x86::Emit_DivMem64CstVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_CONSTANT);

	m_assembler.MovId(CX86Assembler::rAX, src1->m_valueLow);
	if(isSigned)
	{
		m_assembler.Cdq();
		m_assembler.IdivEd(MakeVariableSymbolAddress(src2));
	}
	else
	{
		m_assembler.XorEd(CX86Assembler::rDX, CX86Assembler::MakeRegisterAddress(CX86Assembler::rDX));
		m_assembler.DivEd(MakeVariableSymbolAddress(src2));
	}
	m_assembler.MovGd(MakeMemory64SymbolLoAddress(dst), CX86Assembler::rAX);
	m_assembler.MovGd(MakeMemory64SymbolHiAddress(dst), CX86Assembler::rDX);
}
