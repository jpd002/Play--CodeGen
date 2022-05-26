#include <functional>
#include <array>
#include <assert.h>
#include <stdexcept>
#include "Jitter_CodeGen_x86.h"

//Check if CPUID is available
#ifdef _WIN32
#define HAS_CPUID
#define HAS_CPUID_MSVC
#include <intrin.h>
#endif

#if defined(__linux__)
#if defined(__i386__) || defined(__x86_64__)
#define HAS_CPUID
#define HAS_CPUID_GCC
#include <cpuid.h>
#endif
#endif

#if defined(__APPLE__)
#include <TargetConditionals.h>
#if TARGET_CPU_X86 || TARGET_CPU_X86_64
#define HAS_CPUID
#define HAS_CPUID_GCC
#include <cpuid.h>
#endif
#endif

using namespace Jitter;

#include "Jitter_CodeGen_x86_Alu.h"
#include "Jitter_CodeGen_x86_Shift.h"
#include "Jitter_CodeGen_x86_Mul.h"
#include "Jitter_CodeGen_x86_Div.h"

CX86Assembler::REGISTER CCodeGen_x86::g_baseRegister = CX86Assembler::rBP;

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

	{ OP_DIV, MATCH_TEMPORARY64, MATCH_REGISTER, MATCH_REGISTER, MATCH_NIL, &CCodeGen_x86::Emit_DivTmp64RegReg<false> },
	{ OP_DIV, MATCH_TEMPORARY64, MATCH_REGISTER, MATCH_MEMORY,   MATCH_NIL, &CCodeGen_x86::Emit_DivTmp64RegMem<false> },
	{ OP_DIV, MATCH_TEMPORARY64, MATCH_REGISTER, MATCH_CONSTANT, MATCH_NIL, &CCodeGen_x86::Emit_DivTmp64RegCst<false> },
	{ OP_DIV, MATCH_TEMPORARY64, MATCH_MEMORY,   MATCH_REGISTER, MATCH_NIL, &CCodeGen_x86::Emit_DivTmp64MemReg<false> },
	{ OP_DIV, MATCH_TEMPORARY64, MATCH_MEMORY,   MATCH_MEMORY,   MATCH_NIL, &CCodeGen_x86::Emit_DivTmp64MemMem<false> },
	{ OP_DIV, MATCH_TEMPORARY64, MATCH_MEMORY,   MATCH_CONSTANT, MATCH_NIL, &CCodeGen_x86::Emit_DivTmp64MemCst<false> },
	{ OP_DIV, MATCH_TEMPORARY64, MATCH_CONSTANT, MATCH_REGISTER, MATCH_NIL, &CCodeGen_x86::Emit_DivTmp64CstReg<false> },
	{ OP_DIV, MATCH_TEMPORARY64, MATCH_CONSTANT, MATCH_MEMORY,   MATCH_NIL, &CCodeGen_x86::Emit_DivTmp64CstMem<false> },

	{ OP_DIVS, MATCH_TEMPORARY64, MATCH_REGISTER, MATCH_REGISTER, MATCH_NIL, &CCodeGen_x86::Emit_DivTmp64RegReg<true> },
	{ OP_DIVS, MATCH_TEMPORARY64, MATCH_REGISTER, MATCH_MEMORY,   MATCH_NIL, &CCodeGen_x86::Emit_DivTmp64RegMem<true> },
	{ OP_DIVS, MATCH_TEMPORARY64, MATCH_REGISTER, MATCH_CONSTANT, MATCH_NIL, &CCodeGen_x86::Emit_DivTmp64RegCst<true> },
	{ OP_DIVS, MATCH_TEMPORARY64, MATCH_MEMORY,   MATCH_REGISTER, MATCH_NIL, &CCodeGen_x86::Emit_DivTmp64MemReg<true> },
	{ OP_DIVS, MATCH_TEMPORARY64, MATCH_MEMORY,   MATCH_MEMORY,   MATCH_NIL, &CCodeGen_x86::Emit_DivTmp64MemMem<true> },
	{ OP_DIVS, MATCH_TEMPORARY64, MATCH_MEMORY,   MATCH_CONSTANT, MATCH_NIL, &CCodeGen_x86::Emit_DivTmp64MemCst<true> },
	{ OP_DIVS, MATCH_TEMPORARY64, MATCH_CONSTANT, MATCH_REGISTER, MATCH_NIL, &CCodeGen_x86::Emit_DivTmp64CstReg<true> },
	{ OP_DIVS, MATCH_TEMPORARY64, MATCH_CONSTANT, MATCH_MEMORY,   MATCH_NIL, &CCodeGen_x86::Emit_DivTmp64CstMem<true> },

	{ OP_MUL, MATCH_TEMPORARY64, MATCH_REGISTER, MATCH_REGISTER, MATCH_NIL, &CCodeGen_x86::Emit_MulTmp64RegReg<false> },
	{ OP_MUL, MATCH_TEMPORARY64, MATCH_REGISTER, MATCH_MEMORY,   MATCH_NIL, &CCodeGen_x86::Emit_MulTmp64RegMem<false> },
	{ OP_MUL, MATCH_TEMPORARY64, MATCH_REGISTER, MATCH_CONSTANT, MATCH_NIL, &CCodeGen_x86::Emit_MulTmp64RegCst<false> },
	{ OP_MUL, MATCH_TEMPORARY64, MATCH_MEMORY,   MATCH_MEMORY,   MATCH_NIL, &CCodeGen_x86::Emit_MulTmp64MemMem<false> },
	{ OP_MUL, MATCH_TEMPORARY64, MATCH_MEMORY,   MATCH_CONSTANT, MATCH_NIL, &CCodeGen_x86::Emit_MulTmp64MemCst<false> },

	{ OP_MULS, MATCH_TEMPORARY64, MATCH_REGISTER, MATCH_REGISTER, MATCH_NIL, &CCodeGen_x86::Emit_MulTmp64RegReg<true> },
	{ OP_MULS, MATCH_TEMPORARY64, MATCH_REGISTER, MATCH_MEMORY,   MATCH_NIL, &CCodeGen_x86::Emit_MulTmp64RegMem<true> },
	{ OP_MULS, MATCH_TEMPORARY64, MATCH_REGISTER, MATCH_CONSTANT, MATCH_NIL, &CCodeGen_x86::Emit_MulTmp64RegCst<true> },
	{ OP_MULS, MATCH_TEMPORARY64, MATCH_MEMORY,   MATCH_MEMORY,   MATCH_NIL, &CCodeGen_x86::Emit_MulTmp64MemMem<true> },
	{ OP_MULS, MATCH_TEMPORARY64, MATCH_MEMORY,   MATCH_CONSTANT, MATCH_NIL, &CCodeGen_x86::Emit_MulTmp64MemCst<true> },

	{ OP_MERGETO64, MATCH_MEMORY64, MATCH_REGISTER, MATCH_REGISTER, MATCH_NIL, &CCodeGen_x86::Emit_MergeTo64_Mem64RegReg },
	{ OP_MERGETO64, MATCH_MEMORY64, MATCH_REGISTER, MATCH_MEMORY,   MATCH_NIL, &CCodeGen_x86::Emit_MergeTo64_Mem64RegMem },
	{ OP_MERGETO64, MATCH_MEMORY64, MATCH_MEMORY,   MATCH_REGISTER, MATCH_NIL, &CCodeGen_x86::Emit_MergeTo64_Mem64MemReg },
	{ OP_MERGETO64, MATCH_MEMORY64, MATCH_MEMORY,   MATCH_MEMORY,   MATCH_NIL, &CCodeGen_x86::Emit_MergeTo64_Mem64MemMem },
	{ OP_MERGETO64, MATCH_MEMORY64, MATCH_CONSTANT, MATCH_REGISTER, MATCH_NIL, &CCodeGen_x86::Emit_MergeTo64_Mem64CstReg },
	{ OP_MERGETO64, MATCH_MEMORY64, MATCH_CONSTANT, MATCH_MEMORY,   MATCH_NIL, &CCodeGen_x86::Emit_MergeTo64_Mem64CstMem },

	{ OP_EXTLOW64, MATCH_REGISTER, MATCH_TEMPORARY64, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_ExtLow64RegTmp64 },
	{ OP_EXTLOW64, MATCH_MEMORY,   MATCH_TEMPORARY64, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_ExtLow64MemTmp64 },
	
	{ OP_EXTHIGH64, MATCH_REGISTER, MATCH_TEMPORARY64, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_ExtHigh64RegTmp64 },
	{ OP_EXTHIGH64, MATCH_MEMORY,   MATCH_TEMPORARY64, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_ExtHigh64MemTmp64 },

	{ OP_LOADFROMREF, MATCH_VARIABLE,    MATCH_VAR_REF, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_LoadFromRef_VarVar    },
	{ OP_LOADFROMREF, MATCH_REGISTER128, MATCH_VAR_REF, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_LoadFromRef_Md_RegVar },
	{ OP_LOADFROMREF, MATCH_MEMORY128,   MATCH_VAR_REF, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_LoadFromRef_Md_MemVar },

	{ OP_LOADFROMREFIDX, MATCH_VARIABLE, MATCH_VAR_REF, MATCH_VARIABLE, MATCH_NIL, &CCodeGen_x86::Emit_LoadFromRefIdx_VarVarVar },
	{ OP_LOADFROMREFIDX, MATCH_VARIABLE, MATCH_VAR_REF, MATCH_CONSTANT, MATCH_NIL, &CCodeGen_x86::Emit_LoadFromRefIdx_VarVarCst },

	{ OP_LOAD8FROMREF, MATCH_VARIABLE, MATCH_VAR_REF, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Load8FromRef_VarVar },

	{ OP_LOAD16FROMREF, MATCH_VARIABLE, MATCH_VAR_REF, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Load16FromRef_VarVar },

	{ OP_STOREATREF, MATCH_NIL, MATCH_VAR_REF, MATCH_VARIABLE,    MATCH_NIL, &CCodeGen_x86::Emit_StoreAtRef_VarVar    },
	{ OP_STOREATREF, MATCH_NIL, MATCH_VAR_REF, MATCH_CONSTANT,    MATCH_NIL, &CCodeGen_x86::Emit_StoreAtRef_VarCst    },
	{ OP_STOREATREF, MATCH_NIL, MATCH_VAR_REF, MATCH_REGISTER128, MATCH_NIL, &CCodeGen_x86::Emit_StoreAtRef_Md_VarReg },
	{ OP_STOREATREF, MATCH_NIL, MATCH_VAR_REF, MATCH_MEMORY128,   MATCH_NIL, &CCodeGen_x86::Emit_StoreAtRef_Md_VarMem },

	{ OP_STOREATREFIDX, MATCH_NIL, MATCH_VAR_REF, MATCH_VARIABLE, MATCH_VARIABLE, &CCodeGen_x86::Emit_StoreAtRefIdx_VarVarVar },
	{ OP_STOREATREFIDX, MATCH_NIL, MATCH_VAR_REF, MATCH_VARIABLE, MATCH_CONSTANT, &CCodeGen_x86::Emit_StoreAtRefIdx_VarVarCst },
	{ OP_STOREATREFIDX, MATCH_NIL, MATCH_VAR_REF, MATCH_CONSTANT, MATCH_VARIABLE, &CCodeGen_x86::Emit_StoreAtRefIdx_VarCstVar },
	{ OP_STOREATREFIDX, MATCH_NIL, MATCH_VAR_REF, MATCH_CONSTANT, MATCH_CONSTANT, &CCodeGen_x86::Emit_StoreAtRefIdx_VarCstCst },

	{ OP_STORE8ATREF, MATCH_NIL, MATCH_VAR_REF, MATCH_CONSTANT, MATCH_NIL, &CCodeGen_x86::Emit_Store8AtRef_VarCst },

	{ OP_STORE16ATREF, MATCH_NIL, MATCH_VAR_REF, MATCH_VARIABLE, MATCH_NIL, &CCodeGen_x86::Emit_Store16AtRef_VarVar },
	{ OP_STORE16ATREF, MATCH_NIL, MATCH_VAR_REF, MATCH_CONSTANT, MATCH_NIL, &CCodeGen_x86::Emit_Store16AtRef_VarCst },

	{ OP_MOV, MATCH_NIL, MATCH_NIL, MATCH_NIL, MATCH_NIL, nullptr },
};

CCodeGen_x86::CCodeGen_x86()
{
	SetGenerationFlags();

	InsertMatchers(g_constMatchers);
	InsertMatchers(g_fpuConstMatchers);

	if(m_hasAvx)
	{
		InsertMatchers(g_fpuAvxConstMatchers);
		InsertMatchers(g_mdAvxConstMatchers);
	}
	else
	{
		InsertMatchers(g_fpuSseConstMatchers);
		InsertMatchers(g_mdConstMatchers);

		if(m_hasSsse3)
		{
			InsertMatchers(g_mdFpFlagSsse3ConstMatchers);
		}
		else
		{
			InsertMatchers(g_mdFpFlagConstMatchers);
		}

		if(m_hasSse41)
		{
			InsertMatchers(g_mdMinMaxWSse41ConstMatchers);
			InsertMatchers(g_mdMovMaskedSse41ConstMatchers);
		}
		else
		{
			InsertMatchers(g_mdMinMaxWConstMatchers);
			InsertMatchers(g_mdMovMaskedConstMatchers);
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
		matcher.op       = constMatcher->op;
		matcher.dstType  = constMatcher->dstType;
		matcher.src1Type = constMatcher->src1Type;
		matcher.src2Type = constMatcher->src2Type;
		matcher.src3Type = constMatcher->src3Type;
		matcher.emitter  = std::bind(constMatcher->emitter, this, std::placeholders::_1);
		m_matchers.insert(MatcherMapType::value_type(matcher.op, matcher));
	}
}

void CCodeGen_x86::SetGenerationFlags()
{
	static const uint32 CPUID_FLAG_SSSE3 = 0x000200;
	static const uint32 CPUID_FLAG_SSE41 = 0x080000;
	static const uint32 CPUID_FLAG_AVX = 0x10000000;

#ifdef HAS_CPUID

#ifdef HAS_CPUID_MSVC
	std::array<int, 4> cpuInfo;
	__cpuid(cpuInfo.data(), 1);
#endif //HAS_CPUID_MSVC

#ifdef HAS_CPUID_GCC
	std::array<unsigned int, 4> cpuInfo;
	__get_cpuid(1, &cpuInfo[0], &cpuInfo[1], &cpuInfo[2], &cpuInfo[3]);
#endif //HAS_CPUID_GCC

	m_hasSsse3 = (cpuInfo[2] & CPUID_FLAG_SSSE3) != 0;
	m_hasSse41 = (cpuInfo[2] & CPUID_FLAG_SSE41) != 0;
	m_hasAvx = (cpuInfo[2] & CPUID_FLAG_AVX) != 0;

#endif //HAS_CPUID

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
	size_t symbolMask = sizeof(void*) - 1;
	assert(symbol->m_type == SYM_REL_REFERENCE);
	assert((symbol->m_valueLow & symbolMask) == 0);
	return CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, symbol->m_valueLow);
}

CX86Assembler::CAddress CCodeGen_x86::MakeTemporaryReferenceSymbolAddress(CSymbol* symbol)
{
	size_t symbolMask = sizeof(void*) - 1;
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

void CCodeGen_x86::Emit_ExtLow64RegTmp64(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type  == SYM_REGISTER);
	assert(src1->m_type == SYM_TEMPORARY64);

	m_assembler.MovEd(m_registers[dst->m_valueLow], CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, src1->m_stackLocation + m_stackLevel + 0));
}

void CCodeGen_x86::Emit_ExtLow64MemTmp64(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(src1->m_type == SYM_TEMPORARY64);

	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, src1->m_stackLocation + m_stackLevel + 0));
	m_assembler.MovGd(MakeMemorySymbolAddress(dst), CX86Assembler::rAX);
}

void CCodeGen_x86::Emit_ExtHigh64RegTmp64(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type  == SYM_REGISTER);
	assert(src1->m_type == SYM_TEMPORARY64);

	m_assembler.MovEd(m_registers[dst->m_valueLow], CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, src1->m_stackLocation + m_stackLevel + 4));
}

void CCodeGen_x86::Emit_ExtHigh64MemTmp64(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(src1->m_type == SYM_TEMPORARY64);

	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, src1->m_stackLocation + m_stackLevel + 4));
	m_assembler.MovGd(MakeMemorySymbolAddress(dst), CX86Assembler::rAX);
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

void CCodeGen_x86::Emit_LoadFromRef_Md_RegVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type == SYM_REGISTER128);

	auto addressReg = PrepareRefSymbolRegisterUse(src1, CX86Assembler::rAX);

	m_assembler.MovapsVo(m_mdRegisters[dst->m_valueLow], CX86Assembler::MakeIndRegAddress(addressReg));
}

void CCodeGen_x86::Emit_LoadFromRef_Md_MemVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto addressReg = PrepareRefSymbolRegisterUse(src1, CX86Assembler::rAX);
	auto valueReg = CX86Assembler::xMM0;

	m_assembler.MovapsVo(valueReg, CX86Assembler::MakeIndRegAddress(addressReg));
	m_assembler.MovapsVo(MakeMemory128SymbolAddress(dst), valueReg);
}

void CCodeGen_x86::Emit_LoadFromRefIdx_VarVarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto addressReg = PrepareRefSymbolRegisterUse(src1, CX86Assembler::rAX);
	auto indexReg = PrepareSymbolRegisterUse(src2, CX86Assembler::rCX);
	auto dstReg = PrepareSymbolRegisterDef(dst, CX86Assembler::rDX);

	uint8 scale = static_cast<uint8>(statement.jmpCondition);
	assert(scale == 1);

	m_assembler.MovEd(dstReg, CX86Assembler::MakeBaseIndexScaleAddress(addressReg, indexReg, scale));

	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_x86::Emit_LoadFromRefIdx_VarVarCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto addressReg = PrepareRefSymbolRegisterUse(src1, CX86Assembler::rAX);
	auto dstReg = PrepareSymbolRegisterDef(dst, CX86Assembler::rDX);

	uint8 scale = static_cast<uint8>(statement.jmpCondition);
	assert(scale == 1);

	m_assembler.MovEd(dstReg, CX86Assembler::MakeIndRegOffAddress(addressReg, src2->m_valueLow));

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

void CCodeGen_x86::Emit_Load16FromRef_VarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto addressReg = PrepareRefSymbolRegisterUse(src1, CX86Assembler::rAX);
	auto dstReg = PrepareSymbolRegisterDef(dst, CX86Assembler::rDX);

	m_assembler.MovzxEw(dstReg, CX86Assembler::MakeIndRegAddress(addressReg));

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

void CCodeGen_x86::Emit_StoreAtRef_Md_VarReg(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_REGISTER128);

	auto addressReg = PrepareRefSymbolRegisterUse(src1, CX86Assembler::rAX);
	m_assembler.MovapsVo(CX86Assembler::MakeIndRegAddress(addressReg), m_mdRegisters[src2->m_valueLow]);
}

void CCodeGen_x86::Emit_StoreAtRef_Md_VarMem(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto addressReg = PrepareRefSymbolRegisterUse(src1, CX86Assembler::rAX);
	auto valueReg = CX86Assembler::xMM0;

	m_assembler.MovapsVo(valueReg, MakeMemory128SymbolAddress(src2));
	m_assembler.MovapsVo(CX86Assembler::MakeIndRegAddress(addressReg), valueReg);
}

void CCodeGen_x86::Emit_StoreAtRefIdx_VarVarVar(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	auto src3 = statement.src3->GetSymbol().get();
	
	uint8 scale = static_cast<uint8>(statement.jmpCondition);
	
	auto addressReg = PrepareRefSymbolRegisterUse(src1, CX86Assembler::rAX);
	auto indexReg = PrepareSymbolRegisterUse(src2, CX86Assembler::rDX);
	auto valueReg = PrepareSymbolRegisterUse(src3, CX86Assembler::rCX);

	m_assembler.MovGd(CX86Assembler::MakeBaseIndexScaleAddress(addressReg, indexReg, scale), valueReg);
}

void CCodeGen_x86::Emit_StoreAtRefIdx_VarVarCst(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	auto src3 = statement.src3->GetSymbol().get();
	
	assert(src3->m_type == SYM_CONSTANT);

	uint8 scale = static_cast<uint8>(statement.jmpCondition);
	
	auto addressReg = PrepareRefSymbolRegisterUse(src1, CX86Assembler::rAX);
	auto indexReg = PrepareSymbolRegisterUse(src2, CX86Assembler::rDX);

	m_assembler.MovId(CX86Assembler::MakeBaseIndexScaleAddress(addressReg, indexReg, scale), src3->m_valueLow);
}

void CCodeGen_x86::Emit_StoreAtRefIdx_VarCstVar(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	auto src3 = statement.src3->GetSymbol().get();
	
	uint8 scale = static_cast<uint8>(statement.jmpCondition);
	assert(scale == 1);
	
	auto addressReg = PrepareRefSymbolRegisterUse(src1, CX86Assembler::rAX);
	auto valueReg = PrepareSymbolRegisterUse(src3, CX86Assembler::rCX);

	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(addressReg, src2->m_valueLow), valueReg);
}

void CCodeGen_x86::Emit_StoreAtRefIdx_VarCstCst(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	auto src3 = statement.src3->GetSymbol().get();
	
	assert(src2->m_type == SYM_CONSTANT);
	assert(src3->m_type == SYM_CONSTANT);

	uint8 scale = static_cast<uint8>(statement.jmpCondition);
	assert(scale == 1);

	auto addressReg = PrepareRefSymbolRegisterUse(src1, CX86Assembler::rAX);

	m_assembler.MovId(CX86Assembler::MakeIndRegOffAddress(addressReg, src2->m_valueLow), src3->m_valueLow);
}

void CCodeGen_x86::Emit_Store8AtRef_VarCst(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_CONSTANT);

	auto addressReg = PrepareRefSymbolRegisterUse(src1, CX86Assembler::rAX);
	m_assembler.MovIb(CX86Assembler::MakeIndRegAddress(addressReg), static_cast<uint8>(src2->m_valueLow));
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
	case CONDITION_EQ:
		m_assembler.SeteEb(dst);
		break;
	case CONDITION_NE:
		m_assembler.SetneEb(dst);
		break;
	case CONDITION_BL:
		m_assembler.SetbEb(dst);
		break;
	case CONDITION_AB:
		m_assembler.SetaEb(dst);
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

CX86Assembler::XMMREGISTER CCodeGen_x86::PrepareSymbolRegisterUseMdAvx(CSymbol* symbol, CX86Assembler::XMMREGISTER preferedRegister)
{
	switch(symbol->m_type)
	{
	case SYM_REGISTER128:
		return m_mdRegisters[symbol->m_valueLow];
		break;
	case SYM_TEMPORARY128:
	case SYM_RELATIVE128:
		m_assembler.VmovapsVo(preferedRegister, MakeMemory128SymbolAddress(symbol));
		return preferedRegister;
		break;
	default:
		throw std::runtime_error("Invalid symbol type.");
		break;
	}
}

void CCodeGen_x86::CommitSymbolRegisterMdAvx(CSymbol* symbol, CX86Assembler::XMMREGISTER usedRegister)
{
	switch(symbol->m_type)
	{
	case SYM_REGISTER128:
		assert(usedRegister == m_mdRegisters[symbol->m_valueLow]);
		break;
	case SYM_TEMPORARY128:
	case SYM_RELATIVE128:
		m_assembler.VmovapsVo(MakeMemory128SymbolAddress(symbol), usedRegister);
		break;
	default:
		throw std::runtime_error("Invalid symbol type.");
		break;
	}
}
CX86Assembler::XMMREGISTER CCodeGen_x86::PrepareSymbolRegisterDefFpu(CSymbol* symbol, CX86Assembler::XMMREGISTER preferedRegister)
{
	switch(symbol->m_type)
	{
	case SYM_REGISTER128:
		return m_mdRegisters[symbol->m_valueLow];
		break;
	case SYM_FP_REL_SINGLE:
	case SYM_FP_TMP_SINGLE:
	case SYM_TEMPORARY128:
	case SYM_RELATIVE128:
		return preferedRegister;
		break;
	default:
		throw std::runtime_error("Invalid symbol type.");
		break;
	}
}

CX86Assembler::XMMREGISTER CCodeGen_x86::PrepareSymbolRegisterUseFpuAvx(CSymbol* symbol, CX86Assembler::XMMREGISTER preferedRegister)
{
	switch(symbol->m_type)
	{
	case SYM_REGISTER128:
		return m_mdRegisters[symbol->m_valueLow];
		break;
	case SYM_TEMPORARY128:
	case SYM_RELATIVE128:
			m_assembler.VmovssEd(preferedRegister, MakeMemory128SymbolAddress(symbol));
			return preferedRegister;
	break;
	case SYM_FP_REL_INT32:
	case SYM_FP_REL_SINGLE:
	case SYM_FP_TMP_SINGLE:
			m_assembler.VmovssEd(preferedRegister, MakeMemoryFpSingleSymbolAddress(symbol));
		return preferedRegister;
		break;
	default:
		throw std::runtime_error("Invalid symbol type.");
		break;
	}
}

void CCodeGen_x86::CommitSymbolRegisterFpuAvx(CSymbol* symbol, CX86Assembler::XMMREGISTER usedRegister)
{
	switch(symbol->m_type)
	{
	case SYM_REGISTER128:
		assert(usedRegister == m_mdRegisters[symbol->m_valueLow]);
		break;
	case SYM_TEMPORARY128:
	case SYM_RELATIVE128:
			m_assembler.VmovssEd(MakeMemory128SymbolAddress(symbol), usedRegister);
		break;
	case SYM_FP_REL_SINGLE:
	case SYM_FP_TMP_SINGLE:
			m_assembler.VmovssEd(MakeMemoryFpSingleSymbolAddress(symbol), usedRegister);
		break;
	default:
		throw std::runtime_error("Invalid symbol type.");
		break;
	}
}
