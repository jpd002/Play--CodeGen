#include <functional>
#include <assert.h>
#include "Jitter_CodeGen_x86.h"

using namespace Jitter;

#include "Jitter_CodeGen_x86_Alu.h"
#include "Jitter_CodeGen_x86_Shift.h"
#include "Jitter_CodeGen_x86_Mul.h"
#include "Jitter_CodeGen_x86_Div.h"

CCodeGen_x86::CONSTMATCHER CCodeGen_x86::g_constMatchers[] = 
{ 
	{ OP_LABEL,		MATCH_NIL,			MATCH_NIL,			MATCH_NIL,			&CCodeGen_x86::MarkLabel							},

	{ OP_NOP,		MATCH_NIL,			MATCH_NIL,			MATCH_NIL,			&CCodeGen_x86::Emit_Nop								},

	ALU_CONST_MATCHERS(OP_ADD, ALUOP_ADD)
	ALU_CONST_MATCHERS(OP_SUB, ALUOP_SUB)
	ALU_CONST_MATCHERS(OP_AND, ALUOP_AND)
	ALU_CONST_MATCHERS(OP_OR,  ALUOP_OR)
	ALU_CONST_MATCHERS(OP_XOR, ALUOP_XOR)

	{ OP_CMP,		MATCH_REGISTER,		MATCH_REGISTER,		MATCH_REGISTER,		&CCodeGen_x86::Emit_Cmp_RegRegReg					},
	{ OP_CMP,		MATCH_REGISTER,		MATCH_REGISTER,		MATCH_RELATIVE,		&CCodeGen_x86::Emit_Cmp_RegRegRel					},
	{ OP_CMP,		MATCH_REGISTER,		MATCH_REGISTER,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_Cmp_RegRegCst					},
	{ OP_CMP,		MATCH_REGISTER,		MATCH_RELATIVE,		MATCH_RELATIVE,		&CCodeGen_x86::Emit_Cmp_RegRelRel					},
	{ OP_CMP,		MATCH_REGISTER,		MATCH_RELATIVE,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_Cmp_RegRelCst					},
	{ OP_CMP,		MATCH_RELATIVE,		MATCH_REGISTER,		MATCH_REGISTER,		&CCodeGen_x86::Emit_Cmp_RelRegReg					},
	{ OP_CMP,		MATCH_RELATIVE,		MATCH_REGISTER,		MATCH_RELATIVE,		&CCodeGen_x86::Emit_Cmp_RelRegRel					},
	{ OP_CMP,		MATCH_RELATIVE,		MATCH_REGISTER,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_Cmp_RelRegCst					},
	{ OP_CMP,		MATCH_RELATIVE,		MATCH_RELATIVE,		MATCH_RELATIVE,		&CCodeGen_x86::Emit_Cmp_RelRelRel					},
	{ OP_CMP,		MATCH_RELATIVE,		MATCH_RELATIVE,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_Cmp_RelRelCst					},
	
	{ OP_NOT,		MATCH_REGISTER,		MATCH_REGISTER,		MATCH_NIL,			&CCodeGen_x86::Emit_Not_RegReg						},
	{ OP_NOT,		MATCH_REGISTER,		MATCH_RELATIVE,		MATCH_NIL,			&CCodeGen_x86::Emit_Not_RegRel						},
	{ OP_NOT,		MATCH_REGISTER,		MATCH_TEMPORARY,	MATCH_NIL,			&CCodeGen_x86::Emit_Not_RegTmp						},
	{ OP_NOT,		MATCH_RELATIVE,		MATCH_REGISTER,		MATCH_NIL,			&CCodeGen_x86::Emit_Not_RelReg						},
	{ OP_NOT,		MATCH_RELATIVE,		MATCH_TEMPORARY,	MATCH_NIL,			&CCodeGen_x86::Emit_Not_RelTmp						},

	{ OP_LZC,		MATCH_REGISTER,		MATCH_MEMORY,		MATCH_NIL,			&CCodeGen_x86::Emit_Lzc_RegMem						},
	{ OP_LZC,		MATCH_MEMORY,		MATCH_MEMORY,		MATCH_NIL,			&CCodeGen_x86::Emit_Lzc_MemMem						},

	SHIFT_CONST_MATCHERS(OP_SRL, SHIFTOP_SRL)
	SHIFT_CONST_MATCHERS(OP_SRA, SHIFTOP_SRA)
	SHIFT_CONST_MATCHERS(OP_SLL, SHIFTOP_SLL)

	{ OP_MOV,		MATCH_REGISTER,		MATCH_RELATIVE,		MATCH_NIL,			&CCodeGen_x86::Emit_Mov_RegRel						},
	{ OP_MOV,		MATCH_REGISTER,		MATCH_REGISTER,		MATCH_NIL,			&CCodeGen_x86::Emit_Mov_RegReg						},
	{ OP_MOV,		MATCH_REGISTER,		MATCH_CONSTANT,		MATCH_NIL,			&CCodeGen_x86::Emit_Mov_RegCst						},
	{ OP_MOV,		MATCH_REGISTER,		MATCH_TEMPORARY,	MATCH_NIL,			&CCodeGen_x86::Emit_Mov_RegTmp						},
	{ OP_MOV,		MATCH_RELATIVE,		MATCH_REGISTER,		MATCH_NIL,			&CCodeGen_x86::Emit_Mov_RelReg						},
	{ OP_MOV,		MATCH_RELATIVE,		MATCH_RELATIVE,		MATCH_NIL,			&CCodeGen_x86::Emit_Mov_RelRel						},
	{ OP_MOV,		MATCH_RELATIVE,		MATCH_CONSTANT,		MATCH_NIL,			&CCodeGen_x86::Emit_Mov_RelCst						},
	{ OP_MOV,		MATCH_RELATIVE,		MATCH_TEMPORARY,	MATCH_NIL,			&CCodeGen_x86::Emit_Mov_RelTmp						},
	{ OP_MOV,		MATCH_TEMPORARY,	MATCH_REGISTER,		MATCH_NIL,			&CCodeGen_x86::Emit_Mov_TmpReg						},
	{ OP_MOV,		MATCH_TEMPORARY,	MATCH_RELATIVE,		MATCH_NIL,			&CCodeGen_x86::Emit_Mov_TmpRel						},
	
	{ OP_JMP,		MATCH_NIL,			MATCH_NIL,			MATCH_NIL,			&CCodeGen_x86::Emit_Jmp								},

	{ OP_CONDJMP,	MATCH_NIL,			MATCH_REGISTER,		MATCH_REGISTER,		&CCodeGen_x86::Emit_CondJmp_RegReg					},
	{ OP_CONDJMP,	MATCH_NIL,			MATCH_REGISTER,		MATCH_RELATIVE,		&CCodeGen_x86::Emit_CondJmp_RegRel					},
	{ OP_CONDJMP,	MATCH_NIL,			MATCH_REGISTER,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_CondJmp_RegCst					},
	{ OP_CONDJMP,	MATCH_NIL,			MATCH_RELATIVE,		MATCH_RELATIVE,		&CCodeGen_x86::Emit_CondJmp_RelRel					},
	{ OP_CONDJMP,	MATCH_NIL,			MATCH_RELATIVE,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_CondJmp_RelCst					},
	{ OP_CONDJMP,	MATCH_NIL,			MATCH_TEMPORARY,	MATCH_CONSTANT,		&CCodeGen_x86::Emit_CondJmp_TmpCst					},

	{ OP_DIV,		MATCH_TEMPORARY64,	MATCH_REGISTER,		MATCH_REGISTER,		&CCodeGen_x86::Emit_DivTmp64RegReg<false>			},
	{ OP_DIV,		MATCH_TEMPORARY64,	MATCH_REGISTER,		MATCH_MEMORY,		&CCodeGen_x86::Emit_DivTmp64RegMem<false>			},
	{ OP_DIV,		MATCH_TEMPORARY64,	MATCH_REGISTER,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_DivTmp64RegCst<false>			},
	{ OP_DIV,		MATCH_TEMPORARY64,	MATCH_RELATIVE,		MATCH_REGISTER,		&CCodeGen_x86::Emit_DivTmp64RelReg<false>			},
	{ OP_DIV,		MATCH_TEMPORARY64,	MATCH_MEMORY,		MATCH_MEMORY,		&CCodeGen_x86::Emit_DivTmp64MemMem<false>			},
	{ OP_DIV,		MATCH_TEMPORARY64,	MATCH_MEMORY,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_DivTmp64MemCst<false>			},
	{ OP_DIV,		MATCH_TEMPORARY64,	MATCH_CONSTANT,		MATCH_REGISTER,		&CCodeGen_x86::Emit_DivTmp64CstReg<false>			},
	{ OP_DIV,		MATCH_TEMPORARY64,	MATCH_CONSTANT,		MATCH_MEMORY,		&CCodeGen_x86::Emit_DivTmp64CstMem<false>			},

	{ OP_DIVS,		MATCH_TEMPORARY64,	MATCH_REGISTER,		MATCH_REGISTER,		&CCodeGen_x86::Emit_DivTmp64RegReg<true>			},
	{ OP_DIVS,		MATCH_TEMPORARY64,	MATCH_REGISTER,		MATCH_MEMORY,		&CCodeGen_x86::Emit_DivTmp64RegMem<true>			},
	{ OP_DIVS,		MATCH_TEMPORARY64,	MATCH_REGISTER,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_DivTmp64RegCst<true>			},
	{ OP_DIVS,		MATCH_TEMPORARY64,	MATCH_RELATIVE,		MATCH_REGISTER,		&CCodeGen_x86::Emit_DivTmp64RelReg<true>			},
	{ OP_DIVS,		MATCH_TEMPORARY64,	MATCH_MEMORY,		MATCH_MEMORY,		&CCodeGen_x86::Emit_DivTmp64MemMem<true>			},
	{ OP_DIVS,		MATCH_TEMPORARY64,	MATCH_MEMORY,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_DivTmp64MemCst<true>			},
	{ OP_DIVS,		MATCH_TEMPORARY64,	MATCH_CONSTANT,		MATCH_REGISTER,		&CCodeGen_x86::Emit_DivTmp64CstReg<true>			},
	{ OP_DIVS,		MATCH_TEMPORARY64,	MATCH_CONSTANT,		MATCH_MEMORY,		&CCodeGen_x86::Emit_DivTmp64CstMem<true>			},

	{ OP_MUL,		MATCH_TEMPORARY64,	MATCH_REGISTER,		MATCH_REGISTER,		&CCodeGen_x86::Emit_MulTmp64RegReg<false>			},
	{ OP_MUL,		MATCH_TEMPORARY64,	MATCH_REGISTER,		MATCH_RELATIVE,		&CCodeGen_x86::Emit_MulTmp64RegRel<false>			},
	{ OP_MUL,		MATCH_TEMPORARY64,	MATCH_REGISTER,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_MulTmp64RegCst<false>			},
	{ OP_MUL,		MATCH_TEMPORARY64,	MATCH_RELATIVE,		MATCH_RELATIVE,		&CCodeGen_x86::Emit_MulTmp64RelRel<false>			},
	{ OP_MUL,		MATCH_TEMPORARY64,	MATCH_RELATIVE,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_MulTmp64RelCst<false>			},

	{ OP_MULS,		MATCH_TEMPORARY64,	MATCH_REGISTER,		MATCH_REGISTER,		&CCodeGen_x86::Emit_MulTmp64RegReg<true>			},
	{ OP_MULS,		MATCH_TEMPORARY64,	MATCH_REGISTER,		MATCH_RELATIVE,		&CCodeGen_x86::Emit_MulTmp64RegRel<true>			},
	{ OP_MULS,		MATCH_TEMPORARY64,	MATCH_REGISTER,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_MulTmp64RegCst<true>			},
	{ OP_MULS,		MATCH_TEMPORARY64,	MATCH_RELATIVE,		MATCH_RELATIVE,		&CCodeGen_x86::Emit_MulTmp64RelRel<true>			},
	{ OP_MULS,		MATCH_TEMPORARY64,	MATCH_RELATIVE,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_MulTmp64RelCst<true>			},

	{ OP_MULSHL,	MATCH_REGISTER,		MATCH_REGISTER,		MATCH_REGISTER,		&CCodeGen_x86::Emit_MulSHL_RegRegReg				},
	{ OP_MULSHL,	MATCH_MEMORY,		MATCH_MEMORY,		MATCH_REGISTER,		&CCodeGen_x86::Emit_MulSHL_MemMemReg				},
	{ OP_MULSHL,	MATCH_MEMORY,		MATCH_MEMORY,		MATCH_MEMORY,		&CCodeGen_x86::Emit_MulSHL_MemMemMem				},

	{ OP_MULSHH,	MATCH_REGISTER,		MATCH_REGISTER,		MATCH_MEMORY,		&CCodeGen_x86::Emit_MulSHH_RegRegMem				},
	{ OP_MULSHH,	MATCH_MEMORY,		MATCH_REGISTER,		MATCH_REGISTER,		&CCodeGen_x86::Emit_MulSHH_MemRegReg				},
	{ OP_MULSHH,	MATCH_MEMORY,		MATCH_MEMORY,		MATCH_REGISTER,		&CCodeGen_x86::Emit_MulSHH_MemMemReg				},
	{ OP_MULSHH,	MATCH_MEMORY,		MATCH_MEMORY,		MATCH_MEMORY,		&CCodeGen_x86::Emit_MulSHH_MemMemMem				},

	{ OP_MERGETO64,	MATCH_TEMPORARY64,	MATCH_REGISTER,		MATCH_REGISTER,		&CCodeGen_x86::Emit_MergeTo64_Tmp64RegReg			},
	{ OP_MERGETO64,	MATCH_TEMPORARY64,	MATCH_REGISTER,		MATCH_MEMORY,		&CCodeGen_x86::Emit_MergeTo64_Tmp64RegMem			},
	{ OP_MERGETO64,	MATCH_TEMPORARY64,	MATCH_MEMORY,		MATCH_MEMORY,		&CCodeGen_x86::Emit_MergeTo64_Tmp64MemMem			},
	{ OP_MERGETO64,	MATCH_TEMPORARY64,	MATCH_CONSTANT,		MATCH_REGISTER,		&CCodeGen_x86::Emit_MergeTo64_Tmp64CstReg			},
	{ OP_MERGETO64,	MATCH_TEMPORARY64,	MATCH_CONSTANT,		MATCH_MEMORY,		&CCodeGen_x86::Emit_MergeTo64_Tmp64CstMem			},

	{ OP_EXTLOW64,	MATCH_REGISTER,		MATCH_TEMPORARY64,	MATCH_NIL,			&CCodeGen_x86::Emit_ExtLow64RegTmp64				},
	{ OP_EXTLOW64,	MATCH_RELATIVE,		MATCH_TEMPORARY64,	MATCH_NIL,			&CCodeGen_x86::Emit_ExtLow64RelTmp64				},
	{ OP_EXTLOW64,	MATCH_TEMPORARY,	MATCH_TEMPORARY64,	MATCH_NIL,			&CCodeGen_x86::Emit_ExtLow64TmpTmp64				},
	
	{ OP_EXTHIGH64,	MATCH_REGISTER,		MATCH_TEMPORARY64,	MATCH_NIL,			&CCodeGen_x86::Emit_ExtHigh64RegTmp64				},
	{ OP_EXTHIGH64,	MATCH_RELATIVE,		MATCH_TEMPORARY64,	MATCH_NIL,			&CCodeGen_x86::Emit_ExtHigh64RelTmp64				},
	{ OP_EXTHIGH64,	MATCH_TEMPORARY,	MATCH_TEMPORARY64,	MATCH_NIL,			&CCodeGen_x86::Emit_ExtHigh64TmpTmp64				},

	{ OP_MOV,		MATCH_NIL,			MATCH_NIL,			MATCH_NIL,			NULL												},
};

CCodeGen_x86::CCodeGen_x86()
: m_registers(NULL)
{
	for(CONSTMATCHER* constMatcher = g_constMatchers; constMatcher->emitter != NULL; constMatcher++)
	{
		MATCHER matcher;
		matcher.op			= constMatcher->op;
		matcher.dstType		= constMatcher->dstType;
		matcher.src1Type	= constMatcher->src1Type;
		matcher.src2Type	= constMatcher->src2Type;
		matcher.emitter		= std::tr1::bind(constMatcher->emitter, this, std::tr1::placeholders::_1);
		m_matchers.insert(MatcherMapType::value_type(matcher.op, matcher));
	}

	for(CONSTMATCHER* constMatcher = g_fpuConstMatchers; constMatcher->emitter != NULL; constMatcher++)
	{
		MATCHER matcher;
		matcher.op			= constMatcher->op;
		matcher.dstType		= constMatcher->dstType;
		matcher.src1Type	= constMatcher->src1Type;
		matcher.src2Type	= constMatcher->src2Type;
		matcher.emitter		= std::tr1::bind(constMatcher->emitter, this, std::tr1::placeholders::_1);
		m_matchers.insert(MatcherMapType::value_type(matcher.op, matcher));
	}

	for(CONSTMATCHER* constMatcher = g_mdConstMatchers; constMatcher->emitter != NULL; constMatcher++)
	{
		MATCHER matcher;
		matcher.op			= constMatcher->op;
		matcher.dstType		= constMatcher->dstType;
		matcher.src1Type	= constMatcher->src1Type;
		matcher.src2Type	= constMatcher->src2Type;
		matcher.emitter		= std::tr1::bind(constMatcher->emitter, this, std::tr1::placeholders::_1);
		m_matchers.insert(MatcherMapType::value_type(matcher.op, matcher));
	}
}

CCodeGen_x86::~CCodeGen_x86()
{

}

void CCodeGen_x86::GenerateCode(const StatementList& statements, unsigned int stackSize)
{
	assert(m_registers != NULL);

	uint32 registerUsage = 0;
	for(StatementList::const_iterator statementIterator(statements.begin());
		statementIterator != statements.end(); statementIterator++)
	{
		const STATEMENT& statement(*statementIterator);
		if(CSymbol* dst = dynamic_symbolref_cast(SYM_REGISTER, statement.dst))
		{
			registerUsage |= (1 << dst->m_valueLow);
		}
	}

	//Align stacksize
	stackSize = (stackSize + 0xF) & ~0xF;

	m_stackLevel = 0;
	Emit_Prolog(stackSize, registerUsage);

	for(StatementList::const_iterator statementIterator(statements.begin());
		statementIterator != statements.end(); statementIterator++)
	{
		const STATEMENT& statement(*statementIterator);

		bool found = false;
		MatcherMapType::const_iterator begin = m_matchers.lower_bound(statement.op);
		MatcherMapType::const_iterator end = m_matchers.upper_bound(statement.op);

		for(MatcherMapType::const_iterator matchIterator(begin); matchIterator != end; matchIterator++)
		{
			const MATCHER& matcher(matchIterator->second);
			if(!SymbolMatches(matcher.dstType, statement.dst)) continue;
			if(!SymbolMatches(matcher.src1Type, statement.src1)) continue;
			if(!SymbolMatches(matcher.src2Type, statement.src2)) continue;
			matcher.emitter(statement);
			found = true;
			break;
		}
		assert(found);
	}

	m_assembler.ResolveLabelReferences();
	m_assembler.ClearLabels();
	m_labels.clear();

	Emit_Epilog(stackSize, registerUsage);
}

void CCodeGen_x86::SetStream(Framework::CStream* stream)
{
	m_assembler.SetStream(stream);
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

CX86Assembler::CAddress CCodeGen_x86::MakeRelative64SymbolAddress(CSymbol* symbol)
{
	assert(symbol->m_type == SYM_RELATIVE64);
	assert((symbol->m_valueLow & 7) == 0);
	return CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, symbol->m_valueLow);
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

void CCodeGen_x86::MarkLabel(const STATEMENT& statement)
{
	CX86Assembler::LABEL label = GetLabel(statement.jmpBlock);
	m_assembler.MarkLabel(label);
}

void CCodeGen_x86::Emit_Nop(const STATEMENT& statement)
{
	
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

void CCodeGen_x86::Emit_Not_RegRel(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type  == SYM_REGISTER);
	assert(src1->m_type == SYM_RELATIVE);

	m_assembler.MovEd(m_registers[dst->m_valueLow], CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow));
	m_assembler.NotEd(CX86Assembler::MakeRegisterAddress(m_registers[dst->m_valueLow]));
}

void CCodeGen_x86::Emit_Not_RegTmp(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type  == SYM_REGISTER);
	assert(src1->m_type == SYM_TEMPORARY);

	m_assembler.MovEd(m_registers[dst->m_valueLow], CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, src1->m_stackLocation + m_stackLevel));
	m_assembler.NotEd(CX86Assembler::MakeRegisterAddress(m_registers[dst->m_valueLow]));
}

void CCodeGen_x86::Emit_Not_RelReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type  == SYM_RELATIVE);
	assert(src1->m_type == SYM_REGISTER);

	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	m_assembler.NotEd(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX));
	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, dst->m_valueLow), CX86Assembler::rAX);
}

void CCodeGen_x86::Emit_Not_RelTmp(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type  == SYM_RELATIVE);
	assert(src1->m_type == SYM_TEMPORARY);

	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, src1->m_stackLocation + m_stackLevel));
	m_assembler.NotEd(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX));
	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, dst->m_valueLow), CX86Assembler::rAX);
}

void CCodeGen_x86::Lzc_RegMem(CX86Assembler::REGISTER dstRegister, const CX86Assembler::CAddress& srcAddress)
{
    CX86Assembler::LABEL set32Label = m_assembler.CreateLabel();
    CX86Assembler::LABEL startCountLabel = m_assembler.CreateLabel();
    CX86Assembler::LABEL doneLabel = m_assembler.CreateLabel();

	CX86Assembler::REGISTER tmpRegister = CX86Assembler::rAX;

	m_assembler.MovEd(tmpRegister, srcAddress);
	m_assembler.TestEd(tmpRegister, CX86Assembler::MakeRegisterAddress(tmpRegister));
	m_assembler.JeJb(set32Label);
	m_assembler.JnsJb(startCountLabel);

	//reverse:
	m_assembler.NotEd(CX86Assembler::MakeRegisterAddress(tmpRegister));
	m_assembler.TestEd(tmpRegister, CX86Assembler::MakeRegisterAddress(tmpRegister));
	m_assembler.JeJb(set32Label);

	//startCount:
	m_assembler.MarkLabel(startCountLabel);
    m_assembler.BsrEd(dstRegister, CX86Assembler::MakeRegisterAddress(tmpRegister));
    m_assembler.NegEd(CX86Assembler::MakeRegisterAddress(dstRegister));
    m_assembler.AddId(CX86Assembler::MakeRegisterAddress(dstRegister), 0x1E);
    m_assembler.JmpJb(doneLabel);

    //set32
    m_assembler.MarkLabel(set32Label);
    m_assembler.MovId(dstRegister, 0x1F);

    //done
    m_assembler.MarkLabel(doneLabel);
}

void CCodeGen_x86::Emit_Lzc_RegMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type == SYM_REGISTER);

	Lzc_RegMem(m_registers[dst->m_valueLow], MakeMemorySymbolAddress(src1));
}

void CCodeGen_x86::Emit_Lzc_MemMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	Lzc_RegMem(CX86Assembler::rAX, MakeMemorySymbolAddress(src1));
	m_assembler.MovGd(MakeMemorySymbolAddress(dst), CX86Assembler::rAX);
}

void CCodeGen_x86::Emit_Mov_RegReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	assert(!dst->Equals(src1));

	m_assembler.MovEd(m_registers[dst->m_valueLow], CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
}

void CCodeGen_x86::Emit_Mov_RegRel(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	m_assembler.MovEd(m_registers[dst->m_valueLow], CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow));
}

void CCodeGen_x86::Emit_Mov_RegCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	if(src1->m_valueLow == 0)
	{
		m_assembler.XorEd(m_registers[dst->m_valueLow], CX86Assembler::MakeRegisterAddress(m_registers[dst->m_valueLow]));
	}
	else
	{
		m_assembler.MovId(m_registers[dst->m_valueLow], src1->m_valueLow);
	}
}

void CCodeGen_x86::Emit_Mov_RegTmp(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	m_assembler.MovEd(m_registers[dst->m_valueLow], CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, src1->m_stackLocation + m_stackLevel));
}

void CCodeGen_x86::Emit_Mov_RelReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type  == SYM_RELATIVE);
	assert(src1->m_type == SYM_REGISTER);

	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, dst->m_valueLow), m_registers[src1->m_valueLow]);
}

void CCodeGen_x86::Emit_Mov_RelRel(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type  == SYM_RELATIVE);
	assert(src1->m_type == SYM_RELATIVE);

	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow));
	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, dst->m_valueLow), CX86Assembler::rAX);
}

void CCodeGen_x86::Emit_Mov_RelCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	m_assembler.MovId(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, dst->m_valueLow), src1->m_valueLow);
}

void CCodeGen_x86::Emit_Mov_RelTmp(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type  == SYM_RELATIVE);
	assert(src1->m_type == SYM_TEMPORARY);

	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, src1->m_stackLocation + m_stackLevel));
	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, dst->m_valueLow), CX86Assembler::rAX);
}

void CCodeGen_x86::Emit_Mov_TmpReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type  == SYM_TEMPORARY);
	assert(src1->m_type == SYM_REGISTER);

	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, dst->m_stackLocation + m_stackLevel), m_registers[src1->m_valueLow]);
}

void CCodeGen_x86::Emit_Mov_TmpRel(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type  == SYM_TEMPORARY);
	assert(src1->m_type == SYM_RELATIVE);

	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow));
	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, dst->m_stackLocation + m_stackLevel), CX86Assembler::rAX);
}

void CCodeGen_x86::Emit_Jmp(const STATEMENT& statement)
{
	m_assembler.JmpJb(GetLabel(statement.jmpBlock));
}

void CCodeGen_x86::Emit_MergeTo64_Tmp64RegReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_TEMPORARY64);
	assert(src1->m_type == SYM_REGISTER);
	assert(src2->m_type == SYM_REGISTER);

	m_assembler.MovGd(MakeTemporary64SymbolLoAddress(dst), m_registers[src1->m_valueLow]);
	m_assembler.MovGd(MakeTemporary64SymbolHiAddress(dst), m_registers[src2->m_valueLow]);
}

void CCodeGen_x86::Emit_MergeTo64_Tmp64RegMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_TEMPORARY64);
	assert(src1->m_type == SYM_REGISTER);

	m_assembler.MovEd(CX86Assembler::rDX, MakeMemorySymbolAddress(src2));

	m_assembler.MovGd(MakeTemporary64SymbolLoAddress(dst), m_registers[src1->m_valueLow]);
	m_assembler.MovGd(MakeTemporary64SymbolHiAddress(dst), CX86Assembler::rDX);
}

void CCodeGen_x86::Emit_MergeTo64_Tmp64MemMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_TEMPORARY64);

	m_assembler.MovEd(CX86Assembler::rAX, MakeMemorySymbolAddress(src1));
	m_assembler.MovEd(CX86Assembler::rDX, MakeMemorySymbolAddress(src2));

	m_assembler.MovGd(MakeTemporary64SymbolLoAddress(dst), CX86Assembler::rAX);
	m_assembler.MovGd(MakeTemporary64SymbolHiAddress(dst), CX86Assembler::rDX);
}

void CCodeGen_x86::Emit_MergeTo64_Tmp64CstReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_TEMPORARY64);
	assert(src1->m_type == SYM_CONSTANT);
	assert(src2->m_type == SYM_REGISTER);

	m_assembler.MovId(CX86Assembler::rAX, src1->m_valueLow);

	m_assembler.MovGd(MakeTemporary64SymbolLoAddress(dst), CX86Assembler::rAX);
	m_assembler.MovGd(MakeTemporary64SymbolHiAddress(dst), m_registers[src2->m_valueLow]);
}

void CCodeGen_x86::Emit_MergeTo64_Tmp64CstMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_TEMPORARY64);
	assert(src1->m_type == SYM_CONSTANT);

	m_assembler.MovId(CX86Assembler::rAX, src1->m_valueLow);
	m_assembler.MovEd(CX86Assembler::rDX, MakeMemorySymbolAddress(src2));

	m_assembler.MovGd(MakeTemporary64SymbolLoAddress(dst), CX86Assembler::rAX);
	m_assembler.MovGd(MakeTemporary64SymbolHiAddress(dst), CX86Assembler::rDX);
}

void CCodeGen_x86::Emit_ExtLow64RegTmp64(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type  == SYM_REGISTER);
	assert(src1->m_type == SYM_TEMPORARY64);

	m_assembler.MovEd(m_registers[dst->m_valueLow], CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, src1->m_stackLocation + m_stackLevel + 0));
}

void CCodeGen_x86::Emit_ExtLow64RelTmp64(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type  == SYM_RELATIVE);
	assert(src1->m_type == SYM_TEMPORARY64);

	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, src1->m_stackLocation + m_stackLevel + 0));
	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, dst->m_valueLow), CX86Assembler::rAX);
}

void CCodeGen_x86::Emit_ExtLow64TmpTmp64(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type  == SYM_TEMPORARY);
	assert(src1->m_type == SYM_TEMPORARY64);

	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, src1->m_stackLocation + m_stackLevel + 0));
	m_assembler.MovGd(MakeTemporarySymbolAddress(dst), CX86Assembler::rAX);
}

void CCodeGen_x86::Emit_ExtHigh64RegTmp64(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type  == SYM_REGISTER);
	assert(src1->m_type == SYM_TEMPORARY64);

	m_assembler.MovEd(m_registers[dst->m_valueLow], CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, src1->m_stackLocation + m_stackLevel + 4));
}

void CCodeGen_x86::Emit_ExtHigh64RelTmp64(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type  == SYM_RELATIVE);
	assert(src1->m_type == SYM_TEMPORARY64);

	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, src1->m_stackLocation + m_stackLevel + 4));
	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, dst->m_valueLow), CX86Assembler::rAX);
}

void CCodeGen_x86::Emit_ExtHigh64TmpTmp64(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type  == SYM_TEMPORARY);
	assert(src1->m_type == SYM_TEMPORARY64);

	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, src1->m_stackLocation + m_stackLevel + 4));
	m_assembler.MovGd(MakeTemporarySymbolAddress(dst), CX86Assembler::rAX);
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

void CCodeGen_x86::Emit_Cmp_RegRegReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	m_assembler.CmpEd(m_registers[src1->m_valueLow], CX86Assembler::MakeRegisterAddress(m_registers[src2->m_valueLow]));
	Cmp_GetFlag(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX), statement.jmpCondition);
	m_assembler.MovzxEb(m_registers[dst->m_valueLow], CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX));
}

void CCodeGen_x86::Emit_Cmp_RegRegRel(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	m_assembler.CmpEd(m_registers[src1->m_valueLow], CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src2->m_valueLow));
	Cmp_GetFlag(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX), statement.jmpCondition);
	m_assembler.MovzxEb(m_registers[dst->m_valueLow], CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX));
}

void CCodeGen_x86::Emit_Cmp_RegRegCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	m_assembler.CmpId(CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]), src2->m_valueLow);
	Cmp_GetFlag(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX), statement.jmpCondition);
	m_assembler.MovzxEb(m_registers[dst->m_valueLow], CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX));
}

void CCodeGen_x86::Emit_Cmp_RegRelRel(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_REGISTER);
	assert(src1->m_type == SYM_RELATIVE);
	assert(src2->m_type == SYM_RELATIVE);

	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow));
	m_assembler.CmpEd(CX86Assembler::rAX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src2->m_valueLow));
	Cmp_GetFlag(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX), statement.jmpCondition);
	m_assembler.MovzxEb(m_registers[dst->m_valueLow], CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX));
}

void CCodeGen_x86::Emit_Cmp_RegRelCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_REGISTER);
	assert(src1->m_type == SYM_RELATIVE);
	assert(src2->m_type == SYM_CONSTANT);

	m_assembler.CmpId(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow), src2->m_valueLow);
	Cmp_GetFlag(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX), statement.jmpCondition);
	m_assembler.MovzxEb(m_registers[dst->m_valueLow], CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX));
}

void CCodeGen_x86::Emit_Cmp_RelRegReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_RELATIVE);
	assert(src1->m_type == SYM_REGISTER);
	assert(src2->m_type == SYM_REGISTER);

	m_assembler.CmpEd(m_registers[src1->m_valueLow], CX86Assembler::MakeRegisterAddress(m_registers[src2->m_valueLow]));
	Cmp_GetFlag(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX), statement.jmpCondition);
	m_assembler.MovzxEb(CX86Assembler::rAX, CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX));
	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, dst->m_valueLow), CX86Assembler::rAX);
}

void CCodeGen_x86::Emit_Cmp_RelRegRel(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_RELATIVE);
	assert(src1->m_type == SYM_REGISTER);
	assert(src2->m_type == SYM_RELATIVE);

	m_assembler.CmpEd(m_registers[src1->m_valueLow], CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src2->m_valueLow));
	Cmp_GetFlag(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX), statement.jmpCondition);
	m_assembler.MovzxEb(CX86Assembler::rAX, CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX));
	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, dst->m_valueLow), CX86Assembler::rAX);
}

void CCodeGen_x86::Emit_Cmp_RelRegCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_RELATIVE);
	assert(src1->m_type == SYM_REGISTER);
	assert(src2->m_type == SYM_CONSTANT);

	m_assembler.CmpId(CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]), src2->m_valueLow);
	Cmp_GetFlag(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX), statement.jmpCondition);
	m_assembler.MovzxEb(CX86Assembler::rAX, CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX));
	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, dst->m_valueLow), CX86Assembler::rAX);
}

void CCodeGen_x86::Emit_Cmp_RelRelRel(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_RELATIVE);
	assert(src1->m_type == SYM_RELATIVE);
	assert(src2->m_type == SYM_RELATIVE);

	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow));
	m_assembler.CmpEd(CX86Assembler::rAX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src2->m_valueLow));
	Cmp_GetFlag(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX), statement.jmpCondition);
	m_assembler.MovzxEb(CX86Assembler::rAX, CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX));
	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, dst->m_valueLow), CX86Assembler::rAX);
}

void CCodeGen_x86::Emit_Cmp_RelRelCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_RELATIVE);
	assert(src1->m_type == SYM_RELATIVE);
	assert(src2->m_type == SYM_CONSTANT);

	m_assembler.CmpId(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow), src2->m_valueLow);
	Cmp_GetFlag(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX), statement.jmpCondition);
	m_assembler.MovzxEb(CX86Assembler::rAX, CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX));
	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, dst->m_valueLow), CX86Assembler::rAX);
}

void CCodeGen_x86::CondJmp_JumpTo(CX86Assembler::LABEL label, Jitter::CONDITION condition)
{
	switch(condition)
	{
	case CONDITION_EQ:
		m_assembler.JeJb(label);
		break;
	case CONDITION_NE:
		m_assembler.JneJb(label);
		break;
	case CONDITION_GT:
		m_assembler.JgJb(label);
		break;
	case CONDITION_LE:
		m_assembler.JleJb(label);
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

void CCodeGen_x86::Emit_CondJmp_RegRel(const STATEMENT& statement)
{
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_REGISTER);
	assert(src2->m_type == SYM_RELATIVE);

	m_assembler.CmpEd(m_registers[src1->m_valueLow], CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src2->m_valueLow));

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

void CCodeGen_x86::Emit_CondJmp_RelRel(const STATEMENT& statement)
{
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_RELATIVE);
	assert(src2->m_type == SYM_RELATIVE);

	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow));
	m_assembler.CmpEd(CX86Assembler::rAX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src2->m_valueLow));

	CondJmp_JumpTo(GetLabel(statement.jmpBlock), statement.jmpCondition);
}

void CCodeGen_x86::Emit_CondJmp_RelCst(const STATEMENT& statement)
{
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_RELATIVE);
	assert(src2->m_type == SYM_CONSTANT);

	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow));
	
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

void CCodeGen_x86::Emit_CondJmp_TmpCst(const STATEMENT& statement)
{
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_TEMPORARY);
	assert(src2->m_type == SYM_CONSTANT);

	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, src1->m_stackLocation + m_stackLevel));
	
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

void CCodeGen_x86::Emit_MulSHL(const CX86Assembler::CAddress& dst, const CX86Assembler::CAddress& src1, const CX86Assembler::CAddress& src2)
{
	CX86Assembler::REGISTER lowRegister = CX86Assembler::rAX;
	CX86Assembler::REGISTER highRegister = CX86Assembler::rDX;

	m_assembler.MovEw(lowRegister, src1);
	m_assembler.ImulEw(src2);

	m_assembler.AndId(CX86Assembler::MakeRegisterAddress(lowRegister), 0xFFFF);
	m_assembler.ShlEd(CX86Assembler::MakeRegisterAddress(highRegister), 16);
	m_assembler.OrEd(lowRegister, CX86Assembler::MakeRegisterAddress(highRegister));

	m_assembler.MovGd(dst, lowRegister);
}

void CCodeGen_x86::Emit_MulSHL_RegRegReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_REGISTER);
	assert(src1->m_type == SYM_REGISTER);
	assert(src2->m_type == SYM_REGISTER);

	Emit_MulSHL(
		CX86Assembler::MakeRegisterAddress(m_registers[dst->m_valueLow]), 
		CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]),
		CX86Assembler::MakeRegisterAddress(m_registers[src2->m_valueLow])
		);
}

void CCodeGen_x86::Emit_MulSHL_MemMemReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_REGISTER);

	Emit_MulSHL(
		MakeMemorySymbolAddress(dst), 
		MakeMemorySymbolAddress(src1),
		CX86Assembler::MakeRegisterAddress(m_registers[src2->m_valueLow])
		);
}

void CCodeGen_x86::Emit_MulSHL_MemMemMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	Emit_MulSHL(
		MakeMemorySymbolAddress(dst), 
		MakeMemorySymbolAddress(src1),
		MakeMemorySymbolAddress(src2)
		);
}

void CCodeGen_x86::Emit_MulSHH(const CX86Assembler::CAddress& dst, const CX86Assembler::CAddress& src1, const CX86Assembler::CAddress& src2)
{
	CX86Assembler::REGISTER lowRegister = CX86Assembler::rAX;
	CX86Assembler::REGISTER highRegister = CX86Assembler::rDX;

	m_assembler.MovEd(lowRegister, src1);
	m_assembler.ShrEd(CX86Assembler::MakeRegisterAddress(lowRegister), 16);

	m_assembler.MovEd(highRegister, src2);
	m_assembler.ShrEd(CX86Assembler::MakeRegisterAddress(highRegister), 16);

	m_assembler.ImulEw(CX86Assembler::MakeRegisterAddress(highRegister));

	m_assembler.AndId(CX86Assembler::MakeRegisterAddress(lowRegister), 0xFFFF);
	m_assembler.ShlEd(CX86Assembler::MakeRegisterAddress(highRegister), 16);
	m_assembler.OrEd(lowRegister, CX86Assembler::MakeRegisterAddress(highRegister));

	m_assembler.MovGd(dst, lowRegister);
}

void CCodeGen_x86::Emit_MulSHH_RegRegMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_REGISTER);
	assert(src1->m_type == SYM_REGISTER);

	Emit_MulSHH(
		CX86Assembler::MakeRegisterAddress(m_registers[dst->m_valueLow]),
		CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]),
		MakeMemorySymbolAddress(src2)
		);
}

void CCodeGen_x86::Emit_MulSHH_MemRegReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_REGISTER);
	assert(src2->m_type == SYM_REGISTER);

	Emit_MulSHH(
		MakeMemorySymbolAddress(dst), 
		CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]), 
		CX86Assembler::MakeRegisterAddress(m_registers[src2->m_valueLow])
		);
}

void CCodeGen_x86::Emit_MulSHH_MemMemReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_REGISTER);

	Emit_MulSHH(
		MakeMemorySymbolAddress(dst), 
		MakeMemorySymbolAddress(src1),
		CX86Assembler::MakeRegisterAddress(m_registers[src2->m_valueLow])
		);
}

void CCodeGen_x86::Emit_MulSHH_MemMemMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	Emit_MulSHH(
		MakeMemorySymbolAddress(dst), 
		MakeMemorySymbolAddress(src1),
		MakeMemorySymbolAddress(src2)
		);
}
