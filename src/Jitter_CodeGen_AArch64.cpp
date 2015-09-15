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

CAArch64Assembler::REGISTER32    CCodeGen_AArch64::g_paramRegisters[MAX_PARAM_REGS] =
{
	CAArch64Assembler::w0,
	CAArch64Assembler::w1,
	CAArch64Assembler::w2,
	CAArch64Assembler::w3,
	CAArch64Assembler::w4,
	CAArch64Assembler::w5,
	CAArch64Assembler::w6,
	CAArch64Assembler::w7,
};

CAArch64Assembler::REGISTER64    CCodeGen_AArch64::g_paramRegisters64[MAX_PARAM_REGS] =
{
	CAArch64Assembler::x0,
	CAArch64Assembler::x1,
	CAArch64Assembler::x2,
	CAArch64Assembler::x3,
	CAArch64Assembler::x4,
	CAArch64Assembler::x5,
	CAArch64Assembler::x6,
	CAArch64Assembler::x7,
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
void CCodeGen_AArch64::Emit_Shift64_MemMemVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstReg = GetNextTempRegister64();
	auto src1Reg = GetNextTempRegister64();
	auto src2Reg = PrepareSymbolRegisterUse(src2, GetNextTempRegister());
	
	LoadMemory64InRegister(src1Reg, src1);
	((m_assembler).*(Shift64Op::OpReg()))(dstReg, src1Reg, static_cast<CAArch64Assembler::REGISTER64>(src2Reg));
	StoreRegisterInMemory64(dst, dstReg);
}

template <typename Shift64Op>
void CCodeGen_AArch64::Emit_Shift64_MemMemCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_CONSTANT);

	auto dstReg = GetNextTempRegister64();
	auto src1Reg = GetNextTempRegister64();
	
	LoadMemory64InRegister(src1Reg, src1);
	((m_assembler).*(Shift64Op::OpImm()))(dstReg, src1Reg, src2->m_valueLow);
	StoreRegisterInMemory64(dst, dstReg);
}

CCodeGen_AArch64::CONSTMATCHER CCodeGen_AArch64::g_constMatchers[] =
{
	{ OP_MOV,        MATCH_MEMORY,       MATCH_ANY,            MATCH_NIL,         &CCodeGen_AArch64::Emit_Mov_MemAny                          },
	{ OP_MOV,        MATCH_VARIABLE,     MATCH_ANY,            MATCH_NIL,         &CCodeGen_AArch64::Emit_Mov_VarAny                          },
	{ OP_MOV,        MATCH_MEMORY64,     MATCH_MEMORY64,       MATCH_NIL,         &CCodeGen_AArch64::Emit_Mov_Mem64Mem64                      },

	{ OP_PARAM,      MATCH_NIL,          MATCH_CONTEXT,        MATCH_NIL,         &CCodeGen_AArch64::Emit_Param_Ctx                           },
	
	{ OP_CALL,       MATCH_NIL,          MATCH_CONSTANTPTR,    MATCH_CONSTANT,    &CCodeGen_AArch64::Emit_Call                                },
	
	{ OP_RETVAL,     MATCH_TEMPORARY,    MATCH_NIL,            MATCH_NIL,         &CCodeGen_AArch64::Emit_RetVal_Tmp                          },
	
	{ OP_JMP,        MATCH_NIL,          MATCH_NIL,            MATCH_NIL,         &CCodeGen_AArch64::Emit_Jmp                                 },
	
	{ OP_CONDJMP,    MATCH_NIL,          MATCH_VARIABLE,       MATCH_CONSTANT,    &CCodeGen_AArch64::Emit_CondJmp_VarCst                      },
	
	{ OP_SLL,        MATCH_VARIABLE,     MATCH_ANY,            MATCH_VARIABLE,    &CCodeGen_AArch64::Emit_Shift_VarAnyVar<SHIFTOP_LSL>        },
	{ OP_SRL,        MATCH_VARIABLE,     MATCH_ANY,            MATCH_VARIABLE,    &CCodeGen_AArch64::Emit_Shift_VarAnyVar<SHIFTOP_LSR>        },
	{ OP_SRA,        MATCH_VARIABLE,     MATCH_ANY,            MATCH_VARIABLE,    &CCodeGen_AArch64::Emit_Shift_VarAnyVar<SHIFTOP_ASR>        },

	{ OP_SLL,        MATCH_VARIABLE,     MATCH_VARIABLE,       MATCH_CONSTANT,    &CCodeGen_AArch64::Emit_Shift_VarVarCst<SHIFTOP_LSL>        },
	{ OP_SRL,        MATCH_VARIABLE,     MATCH_VARIABLE,       MATCH_CONSTANT,    &CCodeGen_AArch64::Emit_Shift_VarVarCst<SHIFTOP_LSR>        },
	{ OP_SRA,        MATCH_VARIABLE,     MATCH_VARIABLE,       MATCH_CONSTANT,    &CCodeGen_AArch64::Emit_Shift_VarVarCst<SHIFTOP_ASR>        },
	
	{ OP_SLL64,      MATCH_MEMORY64,     MATCH_MEMORY64,       MATCH_VARIABLE,    &CCodeGen_AArch64::Emit_Shift64_MemMemVar<SHIFT64OP_LSL>    },
	{ OP_SRL64,      MATCH_MEMORY64,     MATCH_MEMORY64,       MATCH_VARIABLE,    &CCodeGen_AArch64::Emit_Shift64_MemMemVar<SHIFT64OP_LSR>    },
	{ OP_SRA64,      MATCH_MEMORY64,     MATCH_MEMORY64,       MATCH_VARIABLE,    &CCodeGen_AArch64::Emit_Shift64_MemMemVar<SHIFT64OP_ASR>    },

	{ OP_SLL64,      MATCH_MEMORY64,     MATCH_MEMORY64,       MATCH_CONSTANT,    &CCodeGen_AArch64::Emit_Shift64_MemMemCst<SHIFT64OP_LSL>    },
	{ OP_SRL64,      MATCH_MEMORY64,     MATCH_MEMORY64,       MATCH_CONSTANT,    &CCodeGen_AArch64::Emit_Shift64_MemMemCst<SHIFT64OP_LSR>    },
	{ OP_SRA64,      MATCH_MEMORY64,     MATCH_MEMORY64,       MATCH_CONSTANT,    &CCodeGen_AArch64::Emit_Shift64_MemMemCst<SHIFT64OP_ASR>    },
	
	{ OP_LABEL,      MATCH_NIL,          MATCH_NIL,            MATCH_NIL,         &CCodeGen_AArch64::MarkLabel                                },
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
		m_assembler.Ldr(registerId, CAArch64Assembler::xSP, src->m_stackLocation);
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
		m_assembler.Str(registerId, CAArch64Assembler::xSP, dst->m_stackLocation);
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

void CCodeGen_AArch64::LoadConstantInRegister(CAArch64Assembler::REGISTER32 registerId, uint32 constant)
{
	if((constant & 0x0000FFFF) == constant)
	{
		m_assembler.Movz(registerId, static_cast<uint16>(constant & 0xFFFF), 0);
	}
	else if((constant & 0xFFFF0000) == constant)
	{
		m_assembler.Movz(registerId, static_cast<uint16>(constant >> 16), 1);
	}
	else if(~(constant & 0x0000FFFF) == constant)
	{
		assert(false);
	}
	else if(~(constant & 0xFFFF0000) == constant)
	{
		assert(false);
	}
	else
	{
		m_assembler.Movz(registerId, static_cast<uint16>(constant & 0xFFFF), 0);
		m_assembler.Movk(registerId, static_cast<uint16>(constant >> 16), 1);
	}
}

void CCodeGen_AArch64::LoadConstant64InRegister(CAArch64Assembler::REGISTER64 registerId, uint64 constant)
{
	bool loaded = false;
	static const uint64 masks[4] =
	{
		0x000000000000FFFFULL,
		0x00000000FFFF0000ULL,
		0x0000FFFF00000000ULL,
		0xFFFF000000000000ULL
	};
	for(unsigned int i = 0; i < 4; i++)
	{
		if((constant & masks[i]) != 0)
		{
			if(loaded)
			{
				m_assembler.Movk(registerId, constant >> (i * 16), i);
			}
			else
			{
				m_assembler.Movz(registerId, constant >> (i * 16), i);
			}
			loaded = true;
		}
	}
	assert(loaded);
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
	case SYM_CONSTANT:
		LoadConstantInRegister(preferedRegister, symbol->m_valueLow);
		return preferedRegister;
		break;
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

CAArch64Assembler::REGISTER64 CCodeGen_AArch64::PrepareParam64(PARAM_STATE& paramState)
{
	assert(!paramState.prepared);
	paramState.prepared = true;
	if(paramState.index < MAX_PARAM_REGS)
	{
		return g_paramRegisters64[paramState.index];
	}
	else
	{
		assert(false);
	}
}

void CCodeGen_AArch64::CommitParam64(PARAM_STATE& paramState)
{
	assert(paramState.prepared);
	paramState.prepared = false;
	if(paramState.index < MAX_PARAM_REGS)
	{
		//Nothing to do
	}
	else
	{
		assert(false);
	}
	paramState.index++;
}

void CCodeGen_AArch64::Emit_Prolog(uint32 stackSize)
{
	m_assembler.Stp_PreIdx(CAArch64Assembler::x29, CAArch64Assembler::x30, CAArch64Assembler::xSP, -16);
	m_assembler.Mov_Sp(CAArch64Assembler::x29, CAArch64Assembler::xSP);
	if(stackSize != 0)
	{
		m_assembler.Sub(CAArch64Assembler::xSP, CAArch64Assembler::xSP, stackSize, CAArch64Assembler::ADDSUB_IMM_SHIFT_LSL0);
	}
	m_assembler.Mov(g_baseRegister, CAArch64Assembler::x0);
}

void CCodeGen_AArch64::Emit_Epilog(uint32 stackSize)
{
	m_assembler.Mov_Sp(CAArch64Assembler::xSP, CAArch64Assembler::x29);
	m_assembler.Ldp_PostIdx(CAArch64Assembler::x29, CAArch64Assembler::x30, CAArch64Assembler::xSP, 16);
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

void CCodeGen_AArch64::Emit_Mov_MemAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto src1Reg = PrepareSymbolRegisterUse(src1, GetNextTempRegister());
	StoreRegisterInMemory(dst, src1Reg);
}

void CCodeGen_AArch64::Emit_Mov_VarAny(const STATEMENT& statement)
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

void CCodeGen_AArch64::Emit_Param_Ctx(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	
	assert(src1->m_type == SYM_CONTEXT);
	
	m_params.push_back(
		[this] (PARAM_STATE& paramState)
		{
			auto paramReg = PrepareParam64(paramState);
			m_assembler.Mov(paramReg, g_baseRegister);
			CommitParam64(paramState);
		}
	);
}

void CCodeGen_AArch64::Emit_Call(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	
	assert(src1->m_type == SYM_CONSTANTPTR);
	assert(src2->m_type == SYM_CONSTANT);

	unsigned int paramCount = src2->m_valueLow;
	PARAM_STATE paramState;

	for(unsigned int i = 0; i < paramCount; i++)
	{
		auto emitter(m_params.back());
		m_params.pop_back();
		emitter(paramState);
	}
	
	auto fctAddressReg = GetNextTempRegister64();
	LoadConstant64InRegister(fctAddressReg, src1->GetConstantPtr());
	m_assembler.Blr(fctAddressReg);
}

void CCodeGen_AArch64::Emit_RetVal_Tmp(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	assert(dst->m_type == SYM_TEMPORARY);
	StoreRegisterInMemory(dst, CAArch64Assembler::w0);
}

void CCodeGen_AArch64::Emit_Jmp(const STATEMENT& statement)
{
	m_assembler.B(GetLabel(statement.jmpBlock));
}

void CCodeGen_AArch64::Emit_CondJmp(const STATEMENT& statement)
{
	auto label(GetLabel(statement.jmpBlock));
	
	switch(statement.jmpCondition)
	{
	case CONDITION_EQ:
		m_assembler.BCc(CAArch64Assembler::CONDITION_EQ, label);
		break;
	case CONDITION_NE:
		m_assembler.BCc(CAArch64Assembler::CONDITION_NE, label);
		break;
	case CONDITION_LT:
		m_assembler.BCc(CAArch64Assembler::CONDITION_LT, label);
		break;
	case CONDITION_LE:
		m_assembler.BCc(CAArch64Assembler::CONDITION_LE, label);
		break;
	case CONDITION_GT:
		m_assembler.BCc(CAArch64Assembler::CONDITION_GT, label);
		break;
	case CONDITION_GE:
		m_assembler.BCc(CAArch64Assembler::CONDITION_GE, label);
		break;
	default:
		assert(0);
		break;
	}
}

void CCodeGen_AArch64::Emit_CondJmp_VarCst(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	
	assert(src2->m_type == SYM_CONSTANT);
	assert(src2->m_valueLow < 4096);
	
	auto src1Reg = PrepareSymbolRegisterUse(src1, GetNextTempRegister());
	m_assembler.Cmp(src1Reg, src2->m_valueLow, CAArch64Assembler::ADDSUB_IMM_SHIFT_LSL0);
	Emit_CondJmp(statement);
}
