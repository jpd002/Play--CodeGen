#include <functional>
#include <assert.h>
#include <stdexcept>
#include "Jitter_CodeGen_x86.h"

using namespace Jitter;

#include "Jitter_CodeGen_x86_Alu.h"
#include "Jitter_CodeGen_x86_Shift.h"
#include "Jitter_CodeGen_x86_Mul.h"
#include "Jitter_CodeGen_x86_Div.h"

const LITERAL128 CCodeGen_x86::g_makeSzShufflePattern = {0x00020406080A0C0E, 0x8080808080808080};
const LITERAL128 CCodeGen_x86::g_fpCstOne = {0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000};
const LITERAL128 CCodeGen_x86::g_fpClampMask1 = {0x7F7FFFFF, 0x7F7FFFFF, 0x7F7FFFFF, 0x7F7FFFFF};
const LITERAL128 CCodeGen_x86::g_fpClampMask2 = {0xFF7FFFFF, 0xFF7FFFFF, 0xFF7FFFFF, 0xFF7FFFFF};

CX86Assembler::REGISTER CCodeGen_x86::g_baseRegister = CX86Assembler::rBP;

// clang-format off
CCodeGen_x86::CONSTMATCHER CCodeGen_x86::g_constMatchers[] = 
{ 
	{ OP_LABEL, MATCH_NIL, MATCH_NIL, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::MarkLabel  },

	{ OP_NOP,   MATCH_NIL, MATCH_NIL, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Nop   },
	{ OP_BREAK, MATCH_NIL, MATCH_NIL, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Break },

	ALU_CONST_MATCHERS(OP_ADD, ALUOP_ADD)
	ALU_CONST_MATCHERS(OP_SUB, ALUOP_SUB)
	ALU_CONST_MATCHERS(OP_AND, ALUOP_AND)
	ALU_CONST_MATCHERS(OP_OR,  ALUOP_OR)
	ALU_CONST_MATCHERS(OP_XOR, ALUOP_XOR)

	{ OP_NOT, MATCH_REGISTER, MATCH_REGISTER, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Not_RegReg },
	{ OP_NOT, MATCH_REGISTER, MATCH_MEMORY,   MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Not_RegMem },
	{ OP_NOT, MATCH_MEMORY,   MATCH_REGISTER, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Not_MemReg },
	{ OP_NOT, MATCH_MEMORY,   MATCH_MEMORY,   MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Not_MemMem },

	{ OP_LZC, MATCH_REGISTER, MATCH_VARIABLE, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Lzc_RegVar },
	{ OP_LZC, MATCH_MEMORY,   MATCH_VARIABLE, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Lzc_MemVar },

	SHIFT_CONST_MATCHERS(OP_SRL, SHIFTOP_SRL)
	SHIFT_CONST_MATCHERS(OP_SRA, SHIFTOP_SRA)
	SHIFT_CONST_MATCHERS(OP_SLL, SHIFTOP_SLL)

	{ OP_MOV, MATCH_REGISTER, MATCH_REGISTER, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Mov_RegReg },
	{ OP_MOV, MATCH_REGISTER, MATCH_MEMORY,   MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Mov_RegMem },
	{ OP_MOV, MATCH_REGISTER, MATCH_CONSTANT, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Mov_RegCst },
	{ OP_MOV, MATCH_MEMORY,   MATCH_REGISTER, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Mov_MemReg },
	{ OP_MOV, MATCH_MEMORY,   MATCH_MEMORY,   MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Mov_MemMem },
	{ OP_MOV, MATCH_MEMORY,   MATCH_CONSTANT, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Mov_MemCst },
	
	{ OP_JMP, MATCH_NIL, MATCH_NIL, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Jmp },

	{ OP_CONDJMP, MATCH_NIL, MATCH_REGISTER, MATCH_REGISTER, MATCH_NIL, &CCodeGen_x86::Emit_CondJmp_RegReg },
	{ OP_CONDJMP, MATCH_NIL, MATCH_REGISTER, MATCH_MEMORY,   MATCH_NIL, &CCodeGen_x86::Emit_CondJmp_RegMem },
	{ OP_CONDJMP, MATCH_NIL, MATCH_REGISTER, MATCH_CONSTANT, MATCH_NIL, &CCodeGen_x86::Emit_CondJmp_RegCst },
	{ OP_CONDJMP, MATCH_NIL, MATCH_MEMORY,   MATCH_MEMORY,   MATCH_NIL, &CCodeGen_x86::Emit_CondJmp_MemMem },
	{ OP_CONDJMP, MATCH_NIL, MATCH_MEMORY,   MATCH_CONSTANT, MATCH_NIL, &CCodeGen_x86::Emit_CondJmp_MemCst },

	{ OP_SELECT, MATCH_VARIABLE, MATCH_VARIABLE, MATCH_ANY, MATCH_ANY, &CCodeGen_x86::Emit_Select_VarVarAnyAny },

	{ OP_DIV, MATCH_MEMORY64, MATCH_VARIABLE, MATCH_VARIABLE, MATCH_NIL, &CCodeGen_x86::Emit_DivMem64VarVar<false> },
	{ OP_DIV, MATCH_MEMORY64, MATCH_VARIABLE, MATCH_CONSTANT, MATCH_NIL, &CCodeGen_x86::Emit_DivMem64VarCst<false> },
	{ OP_DIV, MATCH_MEMORY64, MATCH_CONSTANT, MATCH_VARIABLE, MATCH_NIL, &CCodeGen_x86::Emit_DivMem64CstVar<false> },

	{ OP_DIVS, MATCH_MEMORY64, MATCH_VARIABLE, MATCH_VARIABLE, MATCH_NIL, &CCodeGen_x86::Emit_DivMem64VarVar<true> },
	{ OP_DIVS, MATCH_MEMORY64, MATCH_VARIABLE, MATCH_CONSTANT, MATCH_NIL, &CCodeGen_x86::Emit_DivMem64VarCst<true> },
	{ OP_DIVS, MATCH_MEMORY64, MATCH_CONSTANT, MATCH_VARIABLE, MATCH_NIL, &CCodeGen_x86::Emit_DivMem64CstVar<true> },

	{ OP_MUL, MATCH_MEMORY64, MATCH_VARIABLE, MATCH_VARIABLE, MATCH_NIL, &CCodeGen_x86::Emit_MulMem64VarVar<false> },
	{ OP_MUL, MATCH_MEMORY64, MATCH_VARIABLE, MATCH_CONSTANT, MATCH_NIL, &CCodeGen_x86::Emit_MulMem64VarCst<false> },

	{ OP_MULS, MATCH_MEMORY64, MATCH_VARIABLE, MATCH_VARIABLE, MATCH_NIL, &CCodeGen_x86::Emit_MulMem64VarVar<true> },
	{ OP_MULS, MATCH_MEMORY64, MATCH_VARIABLE, MATCH_CONSTANT, MATCH_NIL, &CCodeGen_x86::Emit_MulMem64VarCst<true> },

	{ OP_MERGETO64, MATCH_MEMORY64, MATCH_REGISTER, MATCH_REGISTER, MATCH_NIL, &CCodeGen_x86::Emit_MergeTo64_Mem64RegReg },
	{ OP_MERGETO64, MATCH_MEMORY64, MATCH_REGISTER, MATCH_MEMORY,   MATCH_NIL, &CCodeGen_x86::Emit_MergeTo64_Mem64RegMem },
	{ OP_MERGETO64, MATCH_MEMORY64, MATCH_REGISTER, MATCH_CONSTANT, MATCH_NIL, &CCodeGen_x86::Emit_MergeTo64_Mem64RegCst },
	{ OP_MERGETO64, MATCH_MEMORY64, MATCH_MEMORY,   MATCH_REGISTER, MATCH_NIL, &CCodeGen_x86::Emit_MergeTo64_Mem64MemReg },
	{ OP_MERGETO64, MATCH_MEMORY64, MATCH_MEMORY,   MATCH_MEMORY,   MATCH_NIL, &CCodeGen_x86::Emit_MergeTo64_Mem64MemMem },
	{ OP_MERGETO64, MATCH_MEMORY64, MATCH_CONSTANT, MATCH_REGISTER, MATCH_NIL, &CCodeGen_x86::Emit_MergeTo64_Mem64CstReg },
	{ OP_MERGETO64, MATCH_MEMORY64, MATCH_CONSTANT, MATCH_MEMORY,   MATCH_NIL, &CCodeGen_x86::Emit_MergeTo64_Mem64CstMem },

	{ OP_EXTLOW64,  MATCH_VARIABLE, MATCH_MEMORY64, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_ExtLow64VarMem64 },
	{ OP_EXTHIGH64, MATCH_VARIABLE, MATCH_MEMORY64, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_ExtHigh64VarMem64 },

	{ OP_LOADFROMREF, MATCH_VARIABLE,    MATCH_VAR_REF, MATCH_NIL,   MATCH_NIL, &CCodeGen_x86::Emit_LoadFromRef_VarVar    },
	{ OP_LOADFROMREF, MATCH_VARIABLE,    MATCH_VAR_REF, MATCH_ANY32, MATCH_NIL, &CCodeGen_x86::Emit_LoadFromRef_VarVarAny },

	{ OP_LOAD8FROMREF, MATCH_VARIABLE, MATCH_VAR_REF, MATCH_NIL,   MATCH_NIL, &CCodeGen_x86::Emit_Load8FromRef_VarVar },
	{ OP_LOAD8FROMREF, MATCH_VARIABLE, MATCH_VAR_REF, MATCH_ANY32, MATCH_NIL, &CCodeGen_x86::Emit_Load8FromRef_VarVarAny },

	{ OP_LOAD16FROMREF, MATCH_VARIABLE, MATCH_VAR_REF, MATCH_NIL,   MATCH_NIL, &CCodeGen_x86::Emit_Load16FromRef_VarVar    },
	{ OP_LOAD16FROMREF, MATCH_VARIABLE, MATCH_VAR_REF, MATCH_ANY32, MATCH_NIL, &CCodeGen_x86::Emit_Load16FromRef_VarVarAny },

	{ OP_STOREATREF, MATCH_NIL, MATCH_VAR_REF, MATCH_VARIABLE, MATCH_NIL,      &CCodeGen_x86::Emit_StoreAtRef_VarVar    },
	{ OP_STOREATREF, MATCH_NIL, MATCH_VAR_REF, MATCH_CONSTANT, MATCH_NIL,      &CCodeGen_x86::Emit_StoreAtRef_VarCst    },
	{ OP_STOREATREF, MATCH_NIL, MATCH_VAR_REF, MATCH_ANY32,    MATCH_VARIABLE, &CCodeGen_x86::Emit_StoreAtRef_VarAnyVar },
	{ OP_STOREATREF, MATCH_NIL, MATCH_VAR_REF, MATCH_ANY32,    MATCH_CONSTANT, &CCodeGen_x86::Emit_StoreAtRef_VarAnyCst },

	{ OP_STORE8ATREF, MATCH_NIL, MATCH_VAR_REF, MATCH_CONSTANT, MATCH_NIL,      &CCodeGen_x86::Emit_Store8AtRef_VarCst    },
	{ OP_STORE8ATREF, MATCH_NIL, MATCH_VAR_REF, MATCH_ANY32,    MATCH_CONSTANT, &CCodeGen_x86::Emit_Store8AtRef_VarAnyCst },

	{ OP_STORE16ATREF, MATCH_NIL, MATCH_VAR_REF, MATCH_VARIABLE, MATCH_NIL,      &CCodeGen_x86::Emit_Store16AtRef_VarVar },
	{ OP_STORE16ATREF, MATCH_NIL, MATCH_VAR_REF, MATCH_CONSTANT, MATCH_NIL,      &CCodeGen_x86::Emit_Store16AtRef_VarCst },
	{ OP_STORE16ATREF, MATCH_NIL, MATCH_VAR_REF, MATCH_ANY32,    MATCH_VARIABLE, &CCodeGen_x86::Emit_Store16AtRef_VarAnyVar },
	{ OP_STORE16ATREF, MATCH_NIL, MATCH_VAR_REF, MATCH_ANY32,    MATCH_CONSTANT, &CCodeGen_x86::Emit_Store16AtRef_VarAnyCst },

	{ OP_MOV, MATCH_NIL, MATCH_NIL, MATCH_NIL, MATCH_NIL, nullptr },
};
// clang-format on

CCodeGen_x86::CCodeGen_x86(CX86CpuFeatures cpuFeatures)
    : m_cpuFeatures(cpuFeatures)
{
	InsertMatchers(g_constMatchers);
	InsertMatchers(g_fpuConstMatchers);

	if(cpuFeatures.hasAvx)
	{
		InsertMatchers(g_fpuAvxConstMatchers);
		InsertMatchers(g_mdAvxConstMatchers);

		if(cpuFeatures.hasAvx2)
		{
			InsertMatchers(g_mdAvx2ConstMatchers);
		}
		else
		{
			InsertMatchers(g_mdNoAvx2ConstMatchers);
		}
	}
	else
	{
		InsertMatchers(g_fpuSseConstMatchers);
		InsertMatchers(g_mdSseConstMatchers);

		if(cpuFeatures.hasSsse3)
		{
			InsertMatchers(g_mdSsse3ConstMatchers);
		}
		else
		{
			InsertMatchers(g_mdNoSsse3ConstMatchers);
		}

		if(cpuFeatures.hasSse41)
		{
			InsertMatchers(g_mdSse41ConstMatchers);
		}
		else
		{
			InsertMatchers(g_mdNoSse41ConstMatchers);
		}
	}
}

void CCodeGen_x86::GenerateCode(const StatementList& statements, unsigned int stackSize)
{
	assert(m_registers != nullptr);
	assert(m_mdRegisters != nullptr);
	assert(m_labels.empty());

	m_registerUsage = GetRegisterUsage(statements);

	//Align stacksize
	stackSize = (stackSize + 0xF) & ~0xF;
	m_stackLevel = 0;

	m_assembler.Begin();
	{
		CX86Assembler::LABEL rootLabel = m_assembler.CreateLabel();
		m_assembler.MarkLabel(rootLabel);

		Emit_Prolog(statements, stackSize);

		for(const auto& statement : statements)
		{
			bool found = false;
			auto begin = m_matchers.lower_bound(statement.op);
			auto end = m_matchers.upper_bound(statement.op);

			for(auto matchIterator(begin); matchIterator != end; matchIterator++)
			{
				const auto& matcher(matchIterator->second);
				if(!SymbolMatches(matcher.dstType, statement.dst)) continue;
				if(!SymbolMatches(matcher.src1Type, statement.src1)) continue;
				if(!SymbolMatches(matcher.src2Type, statement.src2)) continue;
				if(!SymbolMatches(matcher.src3Type, statement.src3)) continue;
				matcher.emitter(statement);
				found = true;
				break;
			}
			assert(found);
			if(!found)
			{
				throw std::exception();
			}
		}

		Emit_Epilog();
		m_assembler.Ret();
	}
	m_assembler.End();

	if(m_externalSymbolReferencedHandler)
	{
		for(const auto& symbolRefLabel : m_symbolReferenceLabels)
		{
			uint32 offset = m_assembler.GetLabelOffset(symbolRefLabel.second);
			m_externalSymbolReferencedHandler(symbolRefLabel.first, offset, CCodeGen::SYMBOL_REF_TYPE::NATIVE_POINTER);
		}
	}

	m_labels.clear();
	m_symbolReferenceLabels.clear();
}

void CCodeGen_x86::InsertMatchers(const CONSTMATCHER* constMatchers)
{
	for(auto* constMatcher = constMatchers; constMatcher->emitter != nullptr; constMatcher++)
	{
		MATCHER matcher;
		matcher.op = constMatcher->op;
		matcher.dstType = constMatcher->dstType;
		matcher.src1Type = constMatcher->src1Type;
		matcher.src2Type = constMatcher->src2Type;
		matcher.src3Type = constMatcher->src3Type;
		matcher.emitter = std::bind(constMatcher->emitter, this, std::placeholders::_1);
		m_matchers.insert(MatcherMapType::value_type(matcher.op, matcher));
	}
}

void CCodeGen_x86::SetStream(Framework::CStream* stream)
{
	m_assembler.SetStream(stream);
}

void CCodeGen_x86::RegisterExternalSymbols(CObjectFile*) const
{
	//Nothing to register
}

bool CCodeGen_x86::Has128BitsCallOperands() const
{
	return true;
}

bool CCodeGen_x86::SupportsExternalJumps() const
{
	return true;
}

bool CCodeGen_x86::SupportsCmpSelect() const
{
	return true;
}

CX86Assembler::LABEL CCodeGen_x86::GetLabel(uint32 blockId)
{
	CX86Assembler::LABEL result;
	LabelMapType::const_iterator labelIterator(m_labels.find(blockId));
	if(labelIterator == m_labels.end())
	{
		result = m_assembler.CreateLabel();
		m_labels[blockId] = result;
	}
	else
	{
		result = labelIterator->second;
	}
	return result;
}

CX86Assembler::CAddress CCodeGen_x86::MakeTemporarySymbolAddress(CSymbol* symbol)
{
	assert(symbol->m_type == SYM_TEMPORARY);
	assert(((symbol->m_stackLocation + m_stackLevel) & 3) == 0);
	return CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, symbol->m_stackLocation + m_stackLevel);
}

CX86Assembler::CAddress CCodeGen_x86::MakeRelativeSymbolAddress(CSymbol* symbol)
{
	assert(symbol->m_type == SYM_RELATIVE);
	assert((symbol->m_valueLow & 3) == 0);
	return CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, symbol->m_valueLow);
}

CX86Assembler::CAddress CCodeGen_x86::MakeMemorySymbolAddress(CSymbol* symbol)
{
	switch(symbol->m_type)
	{
	case SYM_RELATIVE:
		return MakeRelativeSymbolAddress(symbol);
		break;
	case SYM_TEMPORARY:
		return MakeTemporarySymbolAddress(symbol);
		break;
	default:
		throw std::exception();
		break;
	}
}

CX86Assembler::CAddress CCodeGen_x86::MakeVariableSymbolAddress(CSymbol* symbol)
{
	switch(symbol->m_type)
	{
	case SYM_REGISTER:
		return CX86Assembler::MakeRegisterAddress(m_registers[symbol->m_valueLow]);
		break;
	case SYM_RELATIVE:
		return MakeRelativeSymbolAddress(symbol);
		break;
	case SYM_TEMPORARY:
		return MakeTemporarySymbolAddress(symbol);
		break;
	default:
		throw std::exception();
		break;
	}
}

CX86Assembler::CAddress CCodeGen_x86::MakeRelativeReferenceSymbolAddress(CSymbol* symbol)
{
	FRAMEWORK_MAYBE_UNUSED static const size_t symbolMask = sizeof(void*) - 1;
	assert(symbol->m_type == SYM_REL_REFERENCE);
	assert((symbol->m_valueLow & symbolMask) == 0);
	return CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, symbol->m_valueLow);
}

CX86Assembler::CAddress CCodeGen_x86::MakeTemporaryReferenceSymbolAddress(CSymbol* symbol)
{
	FRAMEWORK_MAYBE_UNUSED static const size_t symbolMask = sizeof(void*) - 1;
	assert(symbol->m_type == SYM_TMP_REFERENCE);
	assert(((symbol->m_stackLocation + m_stackLevel) & symbolMask) == 0);
	return CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, symbol->m_stackLocation + m_stackLevel);
}

CX86Assembler::CAddress CCodeGen_x86::MakeMemoryReferenceSymbolAddress(CSymbol* symbol)
{
	switch(symbol->m_type)
	{
	case SYM_REL_REFERENCE:
		return MakeRelativeReferenceSymbolAddress(symbol);
		break;
	case SYM_TMP_REFERENCE:
		return MakeTemporaryReferenceSymbolAddress(symbol);
		break;
	default:
		throw std::exception();
		break;
	}
}

CX86Assembler::CAddress CCodeGen_x86::MakeVariableReferenceSymbolAddress(CSymbol* symbol)
{
	switch(symbol->m_type)
	{
	case SYM_REG_REFERENCE:
		return CX86Assembler::MakeRegisterAddress(m_registers[symbol->m_valueLow]);
		break;
	case SYM_REL_REFERENCE:
		return MakeRelativeReferenceSymbolAddress(symbol);
		break;
	case SYM_TMP_REFERENCE:
		return MakeTemporaryReferenceSymbolAddress(symbol);
		break;
	default:
		throw std::exception();
		break;
	}
}

CX86Assembler::CAddress CCodeGen_x86::MakeRefBaseScaleSymbolAddress(CSymbol* baseSymbol, CX86Assembler::REGISTER baseRegister,
                                                                    CSymbol* indexSymbol, CX86Assembler::REGISTER indexRegister, uint8 scale)
{
	baseRegister = PrepareRefSymbolRegisterUse(baseSymbol, baseRegister);
	if(indexSymbol->IsConstant())
	{
		uint32 scaledIndex = indexSymbol->m_valueLow * scale;
		return CX86Assembler::MakeIndRegOffAddress(baseRegister, scaledIndex);
	}
	else
	{
		indexRegister = PrepareSymbolRegisterUse(indexSymbol, indexRegister);
		return CX86Assembler::MakeBaseOffIndexScaleAddress(baseRegister, 0, indexRegister, scale);
	}
}

CX86Assembler::CAddress CCodeGen_x86::MakeRelative64SymbolAddress(CSymbol* symbol)
{
	assert(symbol->m_type == SYM_RELATIVE64);
	assert((symbol->m_valueLow & 7) == 0);
	return CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, symbol->m_valueLow);
}

CX86Assembler::CAddress CCodeGen_x86::MakeRelative64SymbolLoAddress(CSymbol* symbol)
{
	assert(symbol->m_type == SYM_RELATIVE64);
	assert((symbol->m_valueLow & 7) == 0);
	return CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, symbol->m_valueLow + 0);
}

CX86Assembler::CAddress CCodeGen_x86::MakeRelative64SymbolHiAddress(CSymbol* symbol)
{
	assert(symbol->m_type == SYM_RELATIVE64);
	assert((symbol->m_valueLow & 7) == 0);
	return CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, symbol->m_valueLow + 4);
}

CX86Assembler::CAddress CCodeGen_x86::MakeTemporary64SymbolAddress(CSymbol* symbol)
{
	assert(symbol->m_type == SYM_TEMPORARY64);
	assert(((symbol->m_stackLocation + m_stackLevel) & 7) == 0);
	return CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, symbol->m_stackLocation + m_stackLevel);
}

CX86Assembler::CAddress CCodeGen_x86::MakeTemporary64SymbolLoAddress(CSymbol* symbol)
{
	assert(symbol->m_type == SYM_TEMPORARY64);
	assert(((symbol->m_stackLocation + m_stackLevel) & 7) == 0);
	return CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, symbol->m_stackLocation + m_stackLevel + 0);
}

CX86Assembler::CAddress CCodeGen_x86::MakeTemporary64SymbolHiAddress(CSymbol* symbol)
{
	assert(symbol->m_type == SYM_TEMPORARY64);
	assert(((symbol->m_stackLocation + m_stackLevel) & 7) == 0);
	return CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, symbol->m_stackLocation + m_stackLevel + 4);
}

CX86Assembler::CAddress CCodeGen_x86::MakeMemory64SymbolAddress(CSymbol* symbol)
{
	switch(symbol->m_type)
	{
	case SYM_RELATIVE64:
		return MakeRelative64SymbolAddress(symbol);
		break;
	case SYM_TEMPORARY64:
		return MakeTemporary64SymbolAddress(symbol);
		break;
	default:
		throw std::exception();
		break;
	}
}

CX86Assembler::CAddress CCodeGen_x86::MakeMemory64SymbolLoAddress(CSymbol* symbol)
{
	switch(symbol->m_type)
	{
	case SYM_RELATIVE64:
		return MakeRelative64SymbolLoAddress(symbol);
		break;
	case SYM_TEMPORARY64:
		return MakeTemporary64SymbolLoAddress(symbol);
		break;
	default:
		throw std::exception();
		break;
	}
}

CX86Assembler::CAddress CCodeGen_x86::MakeMemory64SymbolHiAddress(CSymbol* symbol)
{
	switch(symbol->m_type)
	{
	case SYM_RELATIVE64:
		return MakeRelative64SymbolHiAddress(symbol);
		break;
	case SYM_TEMPORARY64:
		return MakeTemporary64SymbolHiAddress(symbol);
		break;
	default:
		throw std::exception();
		break;
	}
}

void CCodeGen_x86::MarkLabel(const STATEMENT& statement)
{
	CX86Assembler::LABEL label = GetLabel(statement.jmpBlock);
	m_assembler.MarkLabel(label);
}

void CCodeGen_x86::Emit_Nop(const STATEMENT& statement)
{
}

void CCodeGen_x86::Emit_Break(const STATEMENT& statement)
{
	m_assembler.Int3();
}

void CCodeGen_x86::Emit_Not_RegReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	if(!dst->Equals(src1))
	{
		m_assembler.MovEd(m_registers[dst->m_valueLow], CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	}

	m_assembler.NotEd(CX86Assembler::MakeRegisterAddress(m_registers[dst->m_valueLow]));
}

void CCodeGen_x86::Emit_Not_RegMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type == SYM_REGISTER);

	m_assembler.MovEd(m_registers[dst->m_valueLow], MakeMemorySymbolAddress(src1));
	m_assembler.NotEd(CX86Assembler::MakeRegisterAddress(m_registers[dst->m_valueLow]));
}

void CCodeGen_x86::Emit_Not_MemReg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(src1->m_type == SYM_REGISTER);

	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	m_assembler.NotEd(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX));
	m_assembler.MovGd(MakeMemorySymbolAddress(dst), CX86Assembler::rAX);
}

void CCodeGen_x86::Emit_Not_MemMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	m_assembler.MovEd(CX86Assembler::rAX, MakeMemorySymbolAddress(src1));
	m_assembler.NotEd(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX));
	m_assembler.MovGd(MakeMemorySymbolAddress(dst), CX86Assembler::rAX);
}

void CCodeGen_x86::Emit_Lzc(CX86Assembler::REGISTER dstRegister, const CX86Assembler::CAddress& srcAddress)
{
	auto set32Label = m_assembler.CreateLabel();
	auto startCountLabel = m_assembler.CreateLabel();
	auto doneLabel = m_assembler.CreateLabel();

	auto tmpRegister = CX86Assembler::rAX;

	m_assembler.MovEd(tmpRegister, srcAddress);
	m_assembler.TestEd(tmpRegister, CX86Assembler::MakeRegisterAddress(tmpRegister));
	m_assembler.JzJx(set32Label);
	m_assembler.JnsJx(startCountLabel);

	//reverse:
	m_assembler.NotEd(CX86Assembler::MakeRegisterAddress(tmpRegister));
	m_assembler.TestEd(tmpRegister, CX86Assembler::MakeRegisterAddress(tmpRegister));
	m_assembler.JzJx(set32Label);

	//startCount:
	m_assembler.MarkLabel(startCountLabel);
	m_assembler.BsrEd(dstRegister, CX86Assembler::MakeRegisterAddress(tmpRegister));
	m_assembler.NegEd(CX86Assembler::MakeRegisterAddress(dstRegister));
	m_assembler.AddId(CX86Assembler::MakeRegisterAddress(dstRegister), 0x1E);
	m_assembler.JmpJx(doneLabel);

	//set32
	m_assembler.MarkLabel(set32Label);
	m_assembler.MovId(dstRegister, 0x1F);

	//done
	m_assembler.MarkLabel(doneLabel);
}

void CCodeGen_x86::Emit_Lzc_RegVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	Emit_Lzc(m_registers[dst->m_valueLow], MakeVariableSymbolAddress(src1));
}

void CCodeGen_x86::Emit_Lzc_MemVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstRegister = CX86Assembler::rAX;

	Emit_Lzc(dstRegister, MakeVariableSymbolAddress(src1));
	m_assembler.MovGd(MakeMemorySymbolAddress(dst), dstRegister);
}

void CCodeGen_x86::Emit_Mov_RegReg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(!dst->Equals(src1));

	m_assembler.MovEd(m_registers[dst->m_valueLow], CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
}

void CCodeGen_x86::Emit_Mov_RegMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	m_assembler.MovEd(m_registers[dst->m_valueLow], MakeMemorySymbolAddress(src1));
}

void CCodeGen_x86::Emit_Mov_RegCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	if(src1->m_valueLow == 0)
	{
		m_assembler.XorEd(m_registers[dst->m_valueLow], CX86Assembler::MakeRegisterAddress(m_registers[dst->m_valueLow]));
	}
	else
	{
		m_assembler.MovId(m_registers[dst->m_valueLow], src1->m_valueLow);
	}
}

void CCodeGen_x86::Emit_Mov_MemReg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(src1->m_type == SYM_REGISTER);

	m_assembler.MovGd(MakeMemorySymbolAddress(dst), m_registers[src1->m_valueLow]);
}

void CCodeGen_x86::Emit_Mov_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	m_assembler.MovEd(CX86Assembler::rAX, MakeMemorySymbolAddress(src1));
	m_assembler.MovGd(MakeMemorySymbolAddress(dst), CX86Assembler::rAX);
}

void CCodeGen_x86::Emit_Mov_MemCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(src1->m_type == SYM_CONSTANT);

	m_assembler.MovId(MakeMemorySymbolAddress(dst), src1->m_valueLow);
}

void CCodeGen_x86::Emit_Jmp(const STATEMENT& statement)
{
	m_assembler.JmpJx(GetLabel(statement.jmpBlock));
}

void CCodeGen_x86::Emit_MergeTo64_Mem64RegReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_REGISTER);
	assert(src2->m_type == SYM_REGISTER);

	m_assembler.MovGd(MakeMemory64SymbolLoAddress(dst), m_registers[src1->m_valueLow]);
	m_assembler.MovGd(MakeMemory64SymbolHiAddress(dst), m_registers[src2->m_valueLow]);
}

void CCodeGen_x86::Emit_MergeTo64_Mem64RegMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_REGISTER);

	m_assembler.MovEd(CX86Assembler::rDX, MakeMemorySymbolAddress(src2));

	m_assembler.MovGd(MakeMemory64SymbolLoAddress(dst), m_registers[src1->m_valueLow]);
	m_assembler.MovGd(MakeMemory64SymbolHiAddress(dst), CX86Assembler::rDX);
}

void CCodeGen_x86::Emit_MergeTo64_Mem64RegCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_REGISTER);

	m_assembler.MovId(CX86Assembler::rDX, src2->m_valueLow);

	m_assembler.MovGd(MakeMemory64SymbolLoAddress(dst), m_registers[src1->m_valueLow]);
	m_assembler.MovGd(MakeMemory64SymbolHiAddress(dst), CX86Assembler::rDX);
}

void CCodeGen_x86::Emit_MergeTo64_Mem64MemReg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_REGISTER);

	m_assembler.MovEd(CX86Assembler::rAX, MakeMemorySymbolAddress(src1));

	m_assembler.MovGd(MakeMemory64SymbolLoAddress(dst), CX86Assembler::rAX);
	m_assembler.MovGd(MakeMemory64SymbolHiAddress(dst), m_registers[src2->m_valueLow]);
}

void CCodeGen_x86::Emit_MergeTo64_Mem64MemMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	m_assembler.MovEd(CX86Assembler::rAX, MakeMemorySymbolAddress(src1));
	m_assembler.MovEd(CX86Assembler::rDX, MakeMemorySymbolAddress(src2));

	m_assembler.MovGd(MakeMemory64SymbolLoAddress(dst), CX86Assembler::rAX);
	m_assembler.MovGd(MakeMemory64SymbolHiAddress(dst), CX86Assembler::rDX);
}

void CCodeGen_x86::Emit_MergeTo64_Mem64CstReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_CONSTANT);
	assert(src2->m_type == SYM_REGISTER);

	m_assembler.MovId(CX86Assembler::rAX, src1->m_valueLow);

	m_assembler.MovGd(MakeMemory64SymbolLoAddress(dst), CX86Assembler::rAX);
	m_assembler.MovGd(MakeMemory64SymbolHiAddress(dst), m_registers[src2->m_valueLow]);
}

void CCodeGen_x86::Emit_MergeTo64_Mem64CstMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_CONSTANT);

	m_assembler.MovId(CX86Assembler::rAX, src1->m_valueLow);
	m_assembler.MovEd(CX86Assembler::rDX, MakeMemorySymbolAddress(src2));

	m_assembler.MovGd(MakeMemory64SymbolLoAddress(dst), CX86Assembler::rAX);
	m_assembler.MovGd(MakeMemory64SymbolHiAddress(dst), CX86Assembler::rDX);
}

void CCodeGen_x86::Emit_ExtLow64VarMem64(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDef(dst, CX86Assembler::rAX);
	m_assembler.MovEd(dstReg, MakeMemory64SymbolLoAddress(src1));
	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_x86::Emit_ExtHigh64VarMem64(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDef(dst, CX86Assembler::rAX);
	m_assembler.MovEd(dstReg, MakeMemory64SymbolHiAddress(src1));
	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_x86::Emit_LoadFromRef_VarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto addressReg = PrepareRefSymbolRegisterUse(src1, CX86Assembler::rAX);
	auto dstReg = PrepareSymbolRegisterDef(dst, CX86Assembler::rDX);

	m_assembler.MovEd(dstReg, CX86Assembler::MakeIndRegAddress(addressReg));

	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_x86::Emit_LoadFromRef_VarVarAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	uint8 scale = static_cast<uint8>(statement.jmpCondition);

	auto dstReg = PrepareSymbolRegisterDef(dst, CX86Assembler::rDX);
	m_assembler.MovEd(dstReg, MakeRefBaseScaleSymbolAddress(src1, CX86Assembler::rAX, src2, CX86Assembler::rCX, scale));
	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_x86::Emit_Load8FromRef_VarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto addressReg = PrepareRefSymbolRegisterUse(src1, CX86Assembler::rAX);
	auto dstReg = PrepareSymbolRegisterDef(dst, CX86Assembler::rDX);

	m_assembler.MovzxEb(dstReg, CX86Assembler::MakeIndRegAddress(addressReg));

	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_x86::Emit_Load8FromRef_VarVarAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	uint8 scale = static_cast<uint8>(statement.jmpCondition);

	assert(scale == 1);

	auto dstReg = PrepareSymbolRegisterDef(dst, CX86Assembler::rDX);
	m_assembler.MovzxEb(dstReg, MakeRefBaseScaleSymbolAddress(src1, CX86Assembler::rAX, src2, CX86Assembler::rCX, scale));
	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_x86::Emit_Load16FromRef_VarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto addressReg = PrepareRefSymbolRegisterUse(src1, CX86Assembler::rAX);
	auto dstReg = PrepareSymbolRegisterDef(dst, CX86Assembler::rDX);

	m_assembler.MovzxEw(dstReg, CX86Assembler::MakeIndRegAddress(addressReg));

	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_x86::Emit_Load16FromRef_VarVarAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	uint8 scale = static_cast<uint8>(statement.jmpCondition);

	assert(scale == 1);

	auto dstReg = PrepareSymbolRegisterDef(dst, CX86Assembler::rDX);
	m_assembler.MovzxEw(dstReg, MakeRefBaseScaleSymbolAddress(src1, CX86Assembler::rAX, src2, CX86Assembler::rCX, scale));
	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_x86::Emit_StoreAtRef_VarVar(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto addressReg = PrepareRefSymbolRegisterUse(src1, CX86Assembler::rAX);
	auto valueReg = PrepareSymbolRegisterUse(src2, CX86Assembler::rDX);
	m_assembler.MovGd(CX86Assembler::MakeIndRegAddress(addressReg), valueReg);
}

void CCodeGen_x86::Emit_StoreAtRef_VarCst(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_CONSTANT);

	auto addressReg = PrepareRefSymbolRegisterUse(src1, CX86Assembler::rAX);
	m_assembler.MovId(CX86Assembler::MakeIndRegAddress(addressReg), src2->m_valueLow);
}

void CCodeGen_x86::Emit_StoreAtRef_VarAnyVar(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	auto src3 = statement.src3->GetSymbol().get();
	uint8 scale = static_cast<uint8>(statement.jmpCondition);

	auto valueReg = PrepareSymbolRegisterUse(src3, CX86Assembler::rDX);
	m_assembler.MovGd(MakeRefBaseScaleSymbolAddress(src1, CX86Assembler::rAX, src2, CX86Assembler::rCX, scale), valueReg);
}

void CCodeGen_x86::Emit_StoreAtRef_VarAnyCst(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	auto src3 = statement.src3->GetSymbol().get();
	uint8 scale = static_cast<uint8>(statement.jmpCondition);

	assert(src3->m_type == SYM_CONSTANT);

	m_assembler.MovId(MakeRefBaseScaleSymbolAddress(src1, CX86Assembler::rAX, src2, CX86Assembler::rCX, scale), src3->m_valueLow);
}

void CCodeGen_x86::Emit_Store8AtRef_VarCst(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_CONSTANT);

	auto addressReg = PrepareRefSymbolRegisterUse(src1, CX86Assembler::rAX);
	m_assembler.MovIb(CX86Assembler::MakeIndRegAddress(addressReg), static_cast<uint8>(src2->m_valueLow));
}

void CCodeGen_x86::Emit_Store8AtRef_VarAnyCst(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	auto src3 = statement.src3->GetSymbol().get();
	FRAMEWORK_MAYBE_UNUSED uint8 scale = static_cast<uint8>(statement.jmpCondition);

	assert(src3->m_type == SYM_CONSTANT);
	assert(scale == 1);

	m_assembler.MovIb(MakeRefBaseScaleSymbolAddress(src1, CX86Assembler::rAX, src2, CX86Assembler::rCX, scale), static_cast<uint8>(src3->m_valueLow));
}

void CCodeGen_x86::Emit_Store16AtRef_VarVar(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto addressReg = PrepareRefSymbolRegisterUse(src1, CX86Assembler::rAX);
	auto valueReg = PrepareSymbolRegisterUse(src2, CX86Assembler::rDX);
	m_assembler.MovGw(CX86Assembler::MakeIndRegAddress(addressReg), valueReg);
}

void CCodeGen_x86::Emit_Store16AtRef_VarCst(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_CONSTANT);

	auto addressReg = PrepareRefSymbolRegisterUse(src1, CX86Assembler::rAX);
	m_assembler.MovIw(CX86Assembler::MakeIndRegAddress(addressReg), static_cast<uint16>(src2->m_valueLow));
}

void CCodeGen_x86::Emit_Store16AtRef_VarAnyVar(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	auto src3 = statement.src3->GetSymbol().get();
	FRAMEWORK_MAYBE_UNUSED uint8 scale = static_cast<uint8>(statement.jmpCondition);

	assert(scale == 1);

	auto valueReg = PrepareSymbolRegisterUse(src3, CX86Assembler::rDX);
	m_assembler.MovGw(MakeRefBaseScaleSymbolAddress(src1, CX86Assembler::rAX, src2, CX86Assembler::rCX, scale), valueReg);
}

void CCodeGen_x86::Emit_Store16AtRef_VarAnyCst(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	auto src3 = statement.src3->GetSymbol().get();
	FRAMEWORK_MAYBE_UNUSED uint8 scale = static_cast<uint8>(statement.jmpCondition);

	assert(src3->m_type == SYM_CONSTANT);
	assert(scale == 1);

	m_assembler.MovIw(MakeRefBaseScaleSymbolAddress(src1, CX86Assembler::rAX, src2, CX86Assembler::rCX, scale), src3->m_valueLow);
}

void CCodeGen_x86::Cmp_GetFlag(const CX86Assembler::CAddress& dst, CONDITION flag)
{
	switch(flag)
	{
	case CONDITION_LT:
		m_assembler.SetlEb(dst);
		break;
	case CONDITION_LE:
		m_assembler.SetleEb(dst);
		break;
	case CONDITION_GT:
		m_assembler.SetgEb(dst);
		break;
	case CONDITION_GE:
		m_assembler.SetgeEb(dst);
		break;
	case CONDITION_EQ:
		m_assembler.SeteEb(dst);
		break;
	case CONDITION_NE:
		m_assembler.SetneEb(dst);
		break;
	case CONDITION_BL:
		m_assembler.SetbEb(dst);
		break;
	case CONDITION_BE:
		m_assembler.SetbeEb(dst);
		break;
	case CONDITION_AB:
		m_assembler.SetaEb(dst);
		break;
	case CONDITION_AE:
		m_assembler.SetaeEb(dst);
		break;
	default:
		assert(0);
		break;
	}
}

void CCodeGen_x86::CondJmp_JumpTo(CX86Assembler::LABEL label, Jitter::CONDITION condition)
{
	switch(condition)
	{
	case CONDITION_EQ:
		m_assembler.JzJx(label);
		break;
	case CONDITION_NE:
		m_assembler.JnzJx(label);
		break;
	case CONDITION_BL:
		m_assembler.JbJx(label);
		break;
	case CONDITION_BE:
		m_assembler.JbeJx(label);
		break;
	case CONDITION_AB:
		m_assembler.JnbeJx(label);
		break;
	case CONDITION_AE:
		m_assembler.JnbJx(label);
		break;
	case CONDITION_LT:
		m_assembler.JlJx(label);
		break;
	case CONDITION_LE:
		m_assembler.JleJx(label);
		break;
	case CONDITION_GT:
		m_assembler.JnleJx(label);
		break;
	case CONDITION_GE:
		m_assembler.JnlJx(label);
		break;
	default:
		assert(0);
		break;
	}
}

void CCodeGen_x86::Emit_CondJmp_RegReg(const STATEMENT& statement)
{
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_REGISTER);
	assert(src2->m_type == SYM_REGISTER);

	m_assembler.CmpEd(m_registers[src1->m_valueLow], CX86Assembler::MakeRegisterAddress(m_registers[src2->m_valueLow]));

	CondJmp_JumpTo(GetLabel(statement.jmpBlock), statement.jmpCondition);
}

void CCodeGen_x86::Emit_CondJmp_RegMem(const STATEMENT& statement)
{
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_REGISTER);

	m_assembler.CmpEd(m_registers[src1->m_valueLow], MakeMemorySymbolAddress(src2));

	CondJmp_JumpTo(GetLabel(statement.jmpBlock), statement.jmpCondition);
}

void CCodeGen_x86::Emit_CondJmp_RegCst(const STATEMENT& statement)
{
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_REGISTER);
	assert(src2->m_type == SYM_CONSTANT);

	if((src2->m_valueLow == 0) && (statement.jmpCondition == CONDITION_NE || statement.jmpCondition == CONDITION_EQ))
	{
		m_assembler.TestEd(m_registers[src1->m_valueLow], CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	}
	else
	{
		m_assembler.CmpId(CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]), src2->m_valueLow);
	}

	CondJmp_JumpTo(GetLabel(statement.jmpBlock), statement.jmpCondition);
}

void CCodeGen_x86::Emit_CondJmp_MemMem(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	m_assembler.MovEd(CX86Assembler::rAX, MakeMemorySymbolAddress(src1));
	m_assembler.CmpEd(CX86Assembler::rAX, MakeMemorySymbolAddress(src2));

	CondJmp_JumpTo(GetLabel(statement.jmpBlock), statement.jmpCondition);
}

void CCodeGen_x86::Emit_CondJmp_MemCst(const STATEMENT& statement)
{
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_CONSTANT);

	m_assembler.MovEd(CX86Assembler::rAX, MakeMemorySymbolAddress(src1));

	if((src2->m_valueLow == 0) && (statement.jmpCondition == CONDITION_NE || statement.jmpCondition == CONDITION_EQ))
	{
		m_assembler.TestEd(CX86Assembler::rAX, CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX));
	}
	else
	{
		m_assembler.CmpId(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX), src2->m_valueLow);
	}

	CondJmp_JumpTo(GetLabel(statement.jmpBlock), statement.jmpCondition);
}

void CCodeGen_x86::Emit_Select_VarVarAnyAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	auto src3 = statement.src3->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDef(dst, CX86Assembler::rAX);

	if(dst->IsRegister() && dst->Equals(src2))
	{
		m_assembler.CmpId(MakeVariableSymbolAddress(src1), 0);
		m_assembler.CmoveEd(dstReg, MakeVariableSymbolAddress(src3));
	}
	else if(dst->IsRegister() && dst->Equals(src3))
	{
		m_assembler.CmpId(MakeVariableSymbolAddress(src1), 0);
		m_assembler.CmovneEd(dstReg, MakeVariableSymbolAddress(src2));
	}
	else if(src2->IsConstant() && src3->IsConstant())
	{
		auto tmpReg = CX86Assembler::rCX;
		m_assembler.CmpId(MakeVariableSymbolAddress(src1), 0);
		m_assembler.MovId(dstReg, src2->m_valueLow);
		m_assembler.MovId(tmpReg, src3->m_valueLow);
		m_assembler.CmoveEd(dstReg, CX86Assembler::MakeRegisterAddress(tmpReg));
	}
	else if(src2->IsConstant())
	{
		m_assembler.CmpId(MakeVariableSymbolAddress(src1), 0);
		m_assembler.MovId(dstReg, src2->m_valueLow);
		m_assembler.CmoveEd(dstReg, MakeVariableSymbolAddress(src3));
	}
	else if(src3->IsConstant())
	{
		m_assembler.CmpId(MakeVariableSymbolAddress(src1), 0);
		m_assembler.MovId(dstReg, src3->m_valueLow);
		m_assembler.CmovneEd(dstReg, MakeVariableSymbolAddress(src2));
	}
	else
	{
		m_assembler.CmpId(MakeVariableSymbolAddress(src1), 0);
		m_assembler.MovEd(dstReg, MakeVariableSymbolAddress(src2));
		m_assembler.CmoveEd(dstReg, MakeVariableSymbolAddress(src3));
	}

	CommitSymbolRegister(dst, dstReg);
}

CX86Assembler::REGISTER CCodeGen_x86::PrepareSymbolRegisterDef(CSymbol* symbol, CX86Assembler::REGISTER preferedRegister)
{
	switch(symbol->m_type)
	{
	case SYM_REGISTER:
		return m_registers[symbol->m_valueLow];
		break;
	case SYM_TEMPORARY:
	case SYM_RELATIVE:
		return preferedRegister;
		break;
	default:
		throw std::runtime_error("Invalid symbol type.");
		break;
	}
}

CX86Assembler::REGISTER CCodeGen_x86::PrepareSymbolRegisterUse(CSymbol* symbol, CX86Assembler::REGISTER preferedRegister)
{
	switch(symbol->m_type)
	{
	case SYM_REGISTER:
		return m_registers[symbol->m_valueLow];
		break;
	case SYM_TEMPORARY:
	case SYM_RELATIVE:
		m_assembler.MovEd(preferedRegister, MakeMemorySymbolAddress(symbol));
		return preferedRegister;
		break;
	case SYM_CONSTANT:
		m_assembler.MovId(preferedRegister, symbol->m_valueLow);
		return preferedRegister;
		break;
	default:
		throw std::runtime_error("Invalid symbol type.");
		break;
	}
}

CX86Assembler::BYTEREGISTER CCodeGen_x86::PrepareSymbolByteRegisterUse(CSymbol* symbol, CX86Assembler::REGISTER preferedRegister)
{
	assert(CX86Assembler::HasByteRegister(preferedRegister));
	auto preferedByteRegister = CX86Assembler::GetByteRegister(preferedRegister);
	switch(symbol->m_type)
	{
	case SYM_REGISTER:
	{
		auto srcRegister = m_registers[symbol->m_valueLow];
		if(CX86Assembler::HasByteRegister(srcRegister))
		{
			return CX86Assembler::GetByteRegister(srcRegister);
		}
		else
		{
			m_assembler.MovEd(preferedRegister, CX86Assembler::MakeRegisterAddress(srcRegister));
			return preferedByteRegister;
		}
	}
	break;
	case SYM_TEMPORARY:
	case SYM_RELATIVE:
		m_assembler.MovEd(preferedRegister, MakeMemorySymbolAddress(symbol));
		return preferedByteRegister;
		break;
	case SYM_CONSTANT:
		m_assembler.MovId(preferedRegister, symbol->m_valueLow);
		return preferedByteRegister;
		break;
	default:
		throw std::runtime_error("Invalid symbol type.");
		break;
	}
}

void CCodeGen_x86::CommitSymbolRegister(CSymbol* symbol, CX86Assembler::REGISTER usedRegister)
{
	switch(symbol->m_type)
	{
	case SYM_REGISTER:
		assert(usedRegister == m_registers[symbol->m_valueLow]);
		break;
	case SYM_TEMPORARY:
	case SYM_RELATIVE:
		m_assembler.MovGd(MakeMemorySymbolAddress(symbol), usedRegister);
		break;
	default:
		throw std::runtime_error("Invalid symbol type.");
		break;
	}
}

CX86Assembler::XMMREGISTER CCodeGen_x86::PrepareSymbolRegisterDefFp32(CSymbol* symbol, CX86Assembler::XMMREGISTER preferedRegister)
{
	switch(symbol->m_type)
	{
	case SYM_FP_REGISTER32:
		return m_mdRegisters[symbol->m_valueLow];
		break;
	case SYM_FP_TEMPORARY32:
	case SYM_FP_RELATIVE32:
		return preferedRegister;
		break;
	default:
		throw std::runtime_error("Invalid symbol type.");
		break;
	}
}

CX86Assembler::XMMREGISTER CCodeGen_x86::PrepareSymbolRegisterDefMd(CSymbol* symbol, CX86Assembler::XMMREGISTER preferedRegister)
{
	switch(symbol->m_type)
	{
	case SYM_REGISTER128:
		return m_mdRegisters[symbol->m_valueLow];
		break;
	case SYM_TEMPORARY128:
	case SYM_RELATIVE128:
		return preferedRegister;
		break;
	default:
		throw std::runtime_error("Invalid symbol type.");
		break;
	}
}
