#pragma once

template <bool isSigned>
void CCodeGen_x86::Emit_MulMem64VarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	m_assembler.MovEd(CX86Assembler::rAX, MakeVariableSymbolAddress(src2));
	if(isSigned)
	{
		m_assembler.ImulEd(MakeVariableSymbolAddress(src1));
	}
	else
	{
		m_assembler.MulEd(MakeVariableSymbolAddress(src1));
	}
	m_assembler.MovGd(MakeMemory64SymbolLoAddress(dst), CX86Assembler::rAX);
	m_assembler.MovGd(MakeMemory64SymbolHiAddress(dst), CX86Assembler::rDX);
}

template <bool isSigned>
void CCodeGen_x86::Emit_MulMem64VarCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_CONSTANT);

	m_assembler.MovId(CX86Assembler::rAX, src2->m_valueLow);
	if(isSigned)
	{
		m_assembler.ImulEd(MakeVariableSymbolAddress(src1));
	}
	else
	{
		m_assembler.MulEd(MakeVariableSymbolAddress(src1));
	}
	m_assembler.MovGd(MakeMemory64SymbolLoAddress(dst), CX86Assembler::rAX);
	m_assembler.MovGd(MakeMemory64SymbolHiAddress(dst), CX86Assembler::rDX);
}
