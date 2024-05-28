#include "Jitter_CodeGen_x86.h"

using namespace Jitter;

CX86Assembler::CAddress CCodeGen_x86::MakeRelativeFp32SymbolAddress(CSymbol* symbol)
{
	assert(symbol->m_type == SYM_FP_RELATIVE32);
	assert((symbol->m_valueLow & 0x3) == 0);
	return CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, symbol->m_valueLow);
}

CX86Assembler::CAddress CCodeGen_x86::MakeTemporaryFp32SymbolAddress(CSymbol* symbol)
{
	assert(symbol->m_type == SYM_FP_TEMPORARY32);
	assert(((symbol->m_stackLocation + m_stackLevel) & 0x3) == 0);
	return CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, symbol->m_stackLocation + m_stackLevel);
}

CX86Assembler::CAddress CCodeGen_x86::MakeMemoryFp32SymbolAddress(CSymbol* symbol)
{
	switch(symbol->m_type)
	{
	case SYM_FP_RELATIVE32:
		return MakeRelativeFp32SymbolAddress(symbol);
		break;
	case SYM_FP_TEMPORARY32:
		return MakeTemporaryFp32SymbolAddress(symbol);
		break;
	default:
		throw std::exception();
		break;
	}
}

CX86Assembler::SSE_CMP_TYPE CCodeGen_x86::GetSseConditionCode(Jitter::CONDITION condition)
{
	CX86Assembler::SSE_CMP_TYPE conditionCode = CX86Assembler::SSE_CMP_EQ;
	switch(condition)
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
	return conditionCode;
}

void CCodeGen_x86::Emit_Fp_AbsS_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	m_assembler.MovEd(CX86Assembler::rAX, MakeMemoryFp32SymbolAddress(src1));
	m_assembler.AndId(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX), 0x7FFFFFFF);
	m_assembler.MovGd(MakeMemoryFp32SymbolAddress(dst), CX86Assembler::rAX);
}

void CCodeGen_x86::Emit_Fp_NegS_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	m_assembler.MovEd(CX86Assembler::rAX, MakeMemoryFp32SymbolAddress(src1));
	m_assembler.XorId(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX), 0x80000000);
	m_assembler.MovGd(MakeMemoryFp32SymbolAddress(dst), CX86Assembler::rAX);
}

void CCodeGen_x86::Emit_Fp_LdCst_MemCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(src1->m_type == SYM_CONSTANT);

	CX86Assembler::REGISTER tmpRegister = CX86Assembler::rAX;

	m_assembler.MovId(tmpRegister, src1->m_valueLow);
	m_assembler.MovGd(MakeMemoryFp32SymbolAddress(dst), tmpRegister);
}

CCodeGen_x86::CONSTMATCHER CCodeGen_x86::g_fpuConstMatchers[] = 
{ 
	{ OP_FP_ABS_S, MATCH_FP_MEMORY32, MATCH_FP_MEMORY32, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_AbsS_MemMem },
	{ OP_FP_NEG_S, MATCH_FP_MEMORY32, MATCH_FP_MEMORY32, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_NegS_MemMem },

	{ OP_FP_LDCST, MATCH_FP_MEMORY32, MATCH_CONSTANT, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_LdCst_MemCst },

	{ OP_MOV, MATCH_NIL, MATCH_NIL, MATCH_NIL, MATCH_NIL, nullptr },
};
