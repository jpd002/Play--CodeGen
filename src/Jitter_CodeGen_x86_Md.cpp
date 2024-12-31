#include "Jitter_CodeGen_x86.h"
#include <stdexcept>

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
	//assert(((symbol->m_stackLocation + m_stackLevel) & 0xF) == 0);
	return CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, symbol->m_stackLocation + m_stackLevel + (elementIdx * 4));
}

CX86Assembler::CAddress CCodeGen_x86::MakeTemporary256SymbolElementAddress(CSymbol* symbol, unsigned int elementIdx)
{
	assert(symbol->m_type == SYM_TEMPORARY256);
	assert(((symbol->m_stackLocation + m_stackLevel) & 0x1F) == 0);
	return CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, symbol->m_stackLocation + m_stackLevel + elementIdx);
}

CX86Assembler::CAddress CCodeGen_x86::MakeVariable128SymbolAddress(CSymbol* symbol)
{
	switch(symbol->m_type)
	{
	case SYM_REGISTER128:
		return CX86Assembler::MakeXmmRegisterAddress(m_mdRegisters[symbol->m_valueLow]);
		break;
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
