#include "Jitter_CodeGen_AArch64.h"

using namespace Jitter;

CAArch64Assembler::REGISTER32    CCodeGen_AArch64::g_tempRegisters[] =
{
	CAArch64Assembler::w9,
	CAArch64Assembler::w10,
	CAArch64Assembler::w11,
	CAArch64Assembler::w12,
	CAArch64Assembler::w13,
	CAArch64Assembler::w14,
	CAArch64Assembler::w15
};

CAArch64Assembler::REGISTER64    CCodeGen_AArch64::g_tempRegisters64[] =
{
	CAArch64Assembler::x9,
	CAArch64Assembler::x10,
	CAArch64Assembler::x11,
	CAArch64Assembler::x12,
	CAArch64Assembler::x13,
	CAArch64Assembler::x14,
	CAArch64Assembler::x15
};

CAArch64Assembler::REGISTER64    CCodeGen_AArch64::g_baseRegister = CAArch64Assembler::x19;

template <typename ShiftOp>
void CCodeGen_AArch64::Emit_Shift_VarAnyVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	
	auto dstReg = PrepareSymbolRegisterDef(dst, GetNextTempRegister());
	auto src1Reg = PrepareSymbolRegisterUse(src1, GetNextTempRegister());
	auto src2Reg = PrepareSymbolRegisterUse(src2, GetNextTempRegister());
	((m_assembler).*(ShiftOp::OpReg()))(dstReg, src1Reg, src2Reg);
	CommitSymbolRegister(dst, dstReg);
}

template <typename ShiftOp>
void CCodeGen_AArch64::Emit_Shift_VarVarCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_CONSTANT);
	
	auto dstReg = PrepareSymbolRegisterDef(dst, GetNextTempRegister());
	auto src1Reg = PrepareSymbolRegisterUse(src1, GetNextTempRegister());
	((m_assembler).*(ShiftOp::OpImm()))(dstReg, src1Reg, src2->m_valueLow);
	CommitSymbolRegister(dst, dstReg);
}

template <typename Shift64Op>
void CCodeGen_AArch64::Emit_Shift64_MemMemCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_CONSTANT);

	auto src1Reg = GetNextTempRegister64();
	auto dstReg = GetNextTempRegister64();
	
	LoadMemory64InRegister(src1Reg, src1);
	((m_assembler).*(Shift64Op::OpImm()))(dstReg, src1Reg, src2->m_valueLow);
	StoreRegisterInMemory64(dst, dstReg);
}

CCodeGen_AArch64::CONSTMATCHER CCodeGen_AArch64::g_constMatchers[] =
{
	{ OP_MOV,      MATCH_VARIABLE,    MATCH_VARIABLE,    MATCH_NIL,         &CCodeGen_AArch64::Emit_Mov_VarVar                          },
	{ OP_MOV,      MATCH_MEMORY64,    MATCH_MEMORY64,    MATCH_NIL,         &CCodeGen_AArch64::Emit_Mov_Mem64Mem64                      },

	{ OP_SLL,      MATCH_VARIABLE,    MATCH_ANY,         MATCH_VARIABLE,    &CCodeGen_AArch64::Emit_Shift_VarAnyVar<SHIFTOP_LSL>        },
	{ OP_SRL,      MATCH_VARIABLE,    MATCH_ANY,         MATCH_VARIABLE,    &CCodeGen_AArch64::Emit_Shift_VarAnyVar<SHIFTOP_LSR>        },
	{ OP_SRA,      MATCH_VARIABLE,    MATCH_ANY,         MATCH_VARIABLE,    &CCodeGen_AArch64::Emit_Shift_VarAnyVar<SHIFTOP_ASR>        },

	{ OP_SLL,      MATCH_VARIABLE,    MATCH_VARIABLE,    MATCH_CONSTANT,    &CCodeGen_AArch64::Emit_Shift_VarVarCst<SHIFTOP_LSL>        },
	{ OP_SRL,      MATCH_VARIABLE,    MATCH_VARIABLE,    MATCH_CONSTANT,    &CCodeGen_AArch64::Emit_Shift_VarVarCst<SHIFTOP_LSR>        },
	{ OP_SRA,      MATCH_VARIABLE,    MATCH_VARIABLE,    MATCH_CONSTANT,    &CCodeGen_AArch64::Emit_Shift_VarVarCst<SHIFTOP_ASR>        },
	
	{ OP_SLL64,    MATCH_MEMORY64,    MATCH_MEMORY64,    MATCH_CONSTANT,    &CCodeGen_AArch64::Emit_Shift64_MemMemCst<SHIFT64OP_LSL>    },
	{ OP_SRL64,    MATCH_MEMORY64,    MATCH_MEMORY64,    MATCH_CONSTANT,    &CCodeGen_AArch64::Emit_Shift64_MemMemCst<SHIFT64OP_LSR>    },
	{ OP_SRA64,    MATCH_MEMORY64,    MATCH_MEMORY64,    MATCH_CONSTANT,    &CCodeGen_AArch64::Emit_Shift64_MemMemCst<SHIFT64OP_ASR>    },
	
	{ OP_LABEL,    MATCH_NIL,         MATCH_NIL,         MATCH_NIL,         &CCodeGen_AArch64::MarkLabel                                },
};

CCodeGen_AArch64::CCodeGen_AArch64()
{
	for(const auto& constMatcher : g_constMatchers)
	{
		MATCHER matcher;
		matcher.op       = constMatcher.op;
		matcher.dstType  = constMatcher.dstType;
		matcher.src1Type = constMatcher.src1Type;
		matcher.src2Type = constMatcher.src2Type;
		matcher.emitter  = std::bind(constMatcher.emitter, this, std::placeholders::_1);
		m_matchers.insert(MatcherMapType::value_type(matcher.op, matcher));
	}
}

CCodeGen_AArch64::~CCodeGen_AArch64()
{

}

unsigned int CCodeGen_AArch64::GetAvailableRegisterCount() const
{
	return 0;
}

unsigned int CCodeGen_AArch64::GetAvailableMdRegisterCount() const
{
	return 0;
}

bool CCodeGen_AArch64::CanHold128BitsReturnValueInRegisters() const
{
	return false;
}

void CCodeGen_AArch64::SetStream(Framework::CStream* stream)
{
	m_stream = stream;
	m_assembler.SetStream(stream);
}

void CCodeGen_AArch64::RegisterExternalSymbols(CObjectFile* objectFile) const
{

}

void CCodeGen_AArch64::GenerateCode(const StatementList& statements, unsigned int stackSize)
{
	m_nextTempRegister = 0;
	
	//Align stack size (must be aligned on 16 bytes boundary)
	stackSize = (stackSize + 0xF) & ~0xF;

	Emit_Prolog(stackSize);

	for(const auto& statement : statements)
	{
		bool found = false;
		auto begin = m_matchers.lower_bound(statement.op);
		auto end = m_matchers.upper_bound(statement.op);

		for(auto matchIterator(begin); matchIterator != end; matchIterator++)
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
		if(!found)
		{
			throw std::runtime_error("No suitable emitter found for statement.");
		}
	}
	
	Emit_Epilog(stackSize);

	m_assembler.ResolveLabelReferences();
	m_assembler.ClearLabels();
	m_labels.clear();
}

CAArch64Assembler::REGISTER32 CCodeGen_AArch64::GetNextTempRegister()
{
	auto result = g_tempRegisters[m_nextTempRegister];
	m_nextTempRegister++;
	m_nextTempRegister %= MAX_TEMP_REGS;
	return result;
}

CAArch64Assembler::REGISTER64 CCodeGen_AArch64::GetNextTempRegister64()
{
	auto result = g_tempRegisters64[m_nextTempRegister];
	m_nextTempRegister++;
	m_nextTempRegister %= MAX_TEMP_REGS;
	return result;
}

void CCodeGen_AArch64::LoadMemoryInRegister(CAArch64Assembler::REGISTER32 registerId, CSymbol* src)
{
	switch(src->m_type)
	{
	case SYM_RELATIVE:
		assert((src->m_valueLow & 0x03) == 0x00);
		m_assembler.Ldr(registerId, g_baseRegister, src->m_valueLow);
		break;
	case SYM_TEMPORARY:
		m_assembler.Ldr(registerId, CAArch64Assembler::xSP, src->m_stackLocation + m_stackLevel);
		break;
	default:
		assert(0);
		break;
	}
}

void CCodeGen_AArch64::StoreRegisterInMemory(CSymbol* dst, CAArch64Assembler::REGISTER32 registerId)
{
	switch(dst->m_type)
	{
	case SYM_RELATIVE:
		assert((dst->m_valueLow & 0x03) == 0x00);
		m_assembler.Str(registerId, g_baseRegister, dst->m_valueLow);
		break;
	case SYM_TEMPORARY:
		m_assembler.Str(registerId, CAArch64Assembler::xSP, dst->m_stackLocation + m_stackLevel);
		break;
	default:
		assert(0);
		break;
	}
}

void CCodeGen_AArch64::LoadMemory64InRegister(CAArch64Assembler::REGISTER64 registerId, CSymbol* src)
{
	switch(src->m_type)
	{
	case SYM_RELATIVE64:
		assert((src->m_valueLow & 0x07) == 0x00);
		m_assembler.Ldr(registerId, g_baseRegister, src->m_valueLow);
		break;
	default:
		assert(0);
		break;
	}
}

void CCodeGen_AArch64::StoreRegisterInMemory64(CSymbol* dst, CAArch64Assembler::REGISTER64 registerId)
{
	switch(dst->m_type)
	{
	case SYM_RELATIVE64:
		assert((dst->m_valueLow & 0x07) == 0x00);
		m_assembler.Str(registerId, g_baseRegister, dst->m_valueLow);
		break;
	default:
		assert(0);
		break;
	}
}

CAArch64Assembler::REGISTER32 CCodeGen_AArch64::PrepareSymbolRegisterDef(CSymbol* symbol, CAArch64Assembler::REGISTER32 preferedRegister)
{
	switch(symbol->m_type)
	{
//	case SYM_REGISTER:
//		return g_registers[symbol->m_valueLow];
//		break;
	case SYM_TEMPORARY:
	case SYM_RELATIVE:
		return preferedRegister;
		break;
	default:
		throw std::runtime_error("Invalid symbol type.");
		break;
	}
}

CAArch64Assembler::REGISTER32 CCodeGen_AArch64::PrepareSymbolRegisterUse(CSymbol* symbol, CAArch64Assembler::REGISTER32 preferedRegister)
{
	switch(symbol->m_type)
	{
//	case SYM_REGISTER:
//		return g_registers[symbol->m_valueLow];
//		break;
	case SYM_TEMPORARY:
	case SYM_RELATIVE:
		LoadMemoryInRegister(preferedRegister, symbol);
		return preferedRegister;
		break;
//	case SYM_CONSTANT:
//		LoadConstantInRegister(preferedRegister, symbol->m_valueLow);
//		return preferedRegister;
//		break;
	default:
		throw std::runtime_error("Invalid symbol type.");
		break;
	}
}

void CCodeGen_AArch64::CommitSymbolRegister(CSymbol* symbol, CAArch64Assembler::REGISTER32 usedRegister)
{
	switch(symbol->m_type)
	{
//	case SYM_REGISTER:
//		assert(usedRegister == g_registers[symbol->m_valueLow]);
//		break;
	case SYM_TEMPORARY:
	case SYM_RELATIVE:
		StoreRegisterInMemory(symbol, usedRegister);
		break;
	default:
		throw std::runtime_error("Invalid symbol type.");
		break;
	}
}

void CCodeGen_AArch64::Emit_Prolog(uint32 stackSize)
{
	m_assembler.Mov(g_baseRegister, CAArch64Assembler::x0);
}

void CCodeGen_AArch64::Emit_Epilog(uint32 stackSize)
{
	m_assembler.Ret();
}

CAArch64Assembler::LABEL CCodeGen_AArch64::GetLabel(uint32 blockId)
{
	CAArch64Assembler::LABEL result;
	auto labelIterator(m_labels.find(blockId));
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

void CCodeGen_AArch64::MarkLabel(const STATEMENT& statement)
{
	auto label = GetLabel(statement.jmpBlock);
	m_assembler.MarkLabel(label);
}

void CCodeGen_AArch64::Emit_Mov_VarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDef(dst, GetNextTempRegister());
	auto src1Reg = PrepareSymbolRegisterUse(src1, GetNextTempRegister());
	m_assembler.Mov(dstReg, src1Reg);
	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_AArch64::Emit_Mov_Mem64Mem64(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	
	auto tmpReg = GetNextTempRegister64();
	LoadMemory64InRegister(tmpReg, src1);
	StoreRegisterInMemory64(dst, tmpReg);
}
