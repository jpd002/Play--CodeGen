#include <stdexcept>
#include "Jitter_CodeGen_AArch32.h"
#include "ObjectFile.h"
#include "BitManip.h"
#include "maybe_unused.h"
#ifdef __ANDROID__
#include <cpu-features.h>
#endif

using namespace Jitter;

CAArch32Assembler::REGISTER CCodeGen_AArch32::g_baseRegister = CAArch32Assembler::r11;
CAArch32Assembler::REGISTER CCodeGen_AArch32::g_callAddressRegister = CAArch32Assembler::r4;
CAArch32Assembler::REGISTER CCodeGen_AArch32::g_tempParamRegister0 = CAArch32Assembler::r4;
CAArch32Assembler::REGISTER CCodeGen_AArch32::g_tempParamRegister1 = CAArch32Assembler::r5;

const LITERAL128 CCodeGen_AArch32::g_fpClampMask1(0x7F7FFFFF, 0x7F7FFFFF, 0x7F7FFFFF, 0x7F7FFFFF);
const LITERAL128 CCodeGen_AArch32::g_fpClampMask2(0xFF7FFFFF, 0xFF7FFFFF, 0xFF7FFFFF, 0xFF7FFFFF);

// clang-format off
CAArch32Assembler::REGISTER CCodeGen_AArch32::g_registers[MAX_REGISTERS] =
{
	CAArch32Assembler::r4,
	CAArch32Assembler::r5,
	CAArch32Assembler::r6,
	CAArch32Assembler::r7,
	CAArch32Assembler::r8,
	CAArch32Assembler::r10,
};

CAArch32Assembler::REGISTER CCodeGen_AArch32::g_paramRegs[MAX_PARAM_REGS] =
{
	CAArch32Assembler::r0,
	CAArch32Assembler::r1,
	CAArch32Assembler::r2,
	CAArch32Assembler::r3,
};
// clang-format on

static CAArch32Assembler::CONDITION GetConditionCode(Jitter::CONDITION cond)
{
	switch(cond)
	{
	case CONDITION_EQ:
		return CAArch32Assembler::CONDITION_EQ;
	case CONDITION_NE:
		return CAArch32Assembler::CONDITION_NE;
	case CONDITION_BL:
		return CAArch32Assembler::CONDITION_CC;
	case CONDITION_BE:
		return CAArch32Assembler::CONDITION_LS;
	case CONDITION_AB:
		return CAArch32Assembler::CONDITION_HI;
	case CONDITION_AE:
		return CAArch32Assembler::CONDITION_CS;
	case CONDITION_LT:
		return CAArch32Assembler::CONDITION_LT;
	case CONDITION_LE:
		return CAArch32Assembler::CONDITION_LE;
	case CONDITION_GT:
		return CAArch32Assembler::CONDITION_GT;
	case CONDITION_GE:
		return CAArch32Assembler::CONDITION_GE;
	default:
		assert(false);
		return CAArch32Assembler::CONDITION_AL;
	}
}

template <typename ALUOP>
void CCodeGen_AArch32::Emit_Alu_GenericAnyAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDef(dst, CAArch32Assembler::r0);
	auto src1Reg = PrepareSymbolRegisterUse(src1, CAArch32Assembler::r1);
	auto src2Reg = PrepareSymbolRegisterUse(src2, CAArch32Assembler::r2);
	((m_assembler).*(ALUOP::OpReg()))(dstReg, src1Reg, src2Reg);
	CommitSymbolRegister(dst, dstReg);
}

template <typename ALUOP>
void CCodeGen_AArch32::Emit_Alu_GenericAnyCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_CONSTANT);

	auto dstReg = PrepareSymbolRegisterDef(dst, CAArch32Assembler::r0);
	auto src1Reg = PrepareSymbolRegisterUse(src1, CAArch32Assembler::r1);
	uint32 cst = src2->m_valueLow;

	bool supportsNegative = ALUOP::OpImmNeg() != NULL;
	bool supportsComplement = ALUOP::OpImmNot() != NULL;

	uint8 immediate = 0;
	uint8 shiftAmount = 0;
	if(TryGetAluImmediateParams(cst, immediate, shiftAmount))
	{
		((m_assembler).*(ALUOP::OpImm()))(dstReg, src1Reg, CAArch32Assembler::MakeImmediateAluOperand(immediate, shiftAmount));
	}
	else if(supportsNegative && TryGetAluImmediateParams(-static_cast<int32>(cst), immediate, shiftAmount))
	{
		((m_assembler).*(ALUOP::OpImmNeg()))(dstReg, src1Reg, CAArch32Assembler::MakeImmediateAluOperand(immediate, shiftAmount));
	}
	else if(supportsComplement && TryGetAluImmediateParams(~cst, immediate, shiftAmount))
	{
		((m_assembler).*(ALUOP::OpImmNot()))(dstReg, src1Reg, CAArch32Assembler::MakeImmediateAluOperand(immediate, shiftAmount));
	}
	else
	{
		auto cstReg = PrepareSymbolRegisterUse(src2, CAArch32Assembler::r2);
		assert(cstReg != dstReg && cstReg != src1Reg);
		((m_assembler).*(ALUOP::OpReg()))(dstReg, src1Reg, cstReg);
	}

	CommitSymbolRegister(dst, dstReg);
}

// clang-format off
#define ALU_CONST_MATCHERS(ALUOP_CST, ALUOP) \
	{ ALUOP_CST, MATCH_ANY, MATCH_ANY, MATCH_CONSTANT, MATCH_NIL, &CCodeGen_AArch32::Emit_Alu_GenericAnyCst<ALUOP> }, \
	{ ALUOP_CST, MATCH_ANY, MATCH_ANY, MATCH_ANY,      MATCH_NIL, &CCodeGen_AArch32::Emit_Alu_GenericAnyAny<ALUOP> },
// clang-format on

#include "Jitter_CodeGen_AArch32_Div.h"

template <bool isSigned>
void CCodeGen_AArch32::Emit_MulTmp64AnyAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto resLoReg = CAArch32Assembler::r0;
	auto resHiReg = CAArch32Assembler::r1;
	auto src1Reg = PrepareSymbolRegisterUse(src1, CAArch32Assembler::r2);
	auto src2Reg = PrepareSymbolRegisterUse(src2, CAArch32Assembler::r3);

	assert(dst->m_type == SYM_TEMPORARY64);
	assert(resLoReg != src1Reg && resLoReg != src2Reg);
	assert(resHiReg != src1Reg && resHiReg != src2Reg);

	if(isSigned)
	{
		m_assembler.Smull(resLoReg, resHiReg, src1Reg, src2Reg);
	}
	else
	{
		m_assembler.Umull(resLoReg, resHiReg, src1Reg, src2Reg);
	}

	m_assembler.Str(resLoReg, CAArch32Assembler::rSP, CAArch32Assembler::MakeImmediateLdrAddress(dst->m_stackLocation + m_stackLevel + 0));
	m_assembler.Str(resHiReg, CAArch32Assembler::rSP, CAArch32Assembler::MakeImmediateLdrAddress(dst->m_stackLocation + m_stackLevel + 4));
}

template <CAArch32Assembler::SHIFT shiftType>
void CCodeGen_AArch32::Emit_Shift_Generic(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDef(dst, CAArch32Assembler::r0);
	auto src1Reg = PrepareSymbolRegisterUse(src1, CAArch32Assembler::r1);
	auto shift = GetAluShiftFromSymbol(shiftType, src2, CAArch32Assembler::r2);
	m_assembler.Mov(dstReg, CAArch32Assembler::MakeRegisterAluOperand(src1Reg, shift));
	CommitSymbolRegister(dst, dstReg);
}

// clang-format off
CCodeGen_AArch32::CONSTMATCHER CCodeGen_AArch32::g_constMatchers[] = 
{ 
	{ OP_LABEL, MATCH_NIL,    MATCH_NIL, MATCH_NIL, MATCH_NIL, &CCodeGen_AArch32::MarkLabel },

	{ OP_NOP, MATCH_NIL,      MATCH_NIL, MATCH_NIL, MATCH_NIL, &CCodeGen_AArch32::Emit_Nop },
	
	{ OP_MOV, MATCH_REGISTER, MATCH_REGISTER, MATCH_NIL, MATCH_NIL, &CCodeGen_AArch32::Emit_Mov_RegReg },
	{ OP_MOV, MATCH_REGISTER, MATCH_MEMORY,   MATCH_NIL, MATCH_NIL, &CCodeGen_AArch32::Emit_Mov_RegMem },
	{ OP_MOV, MATCH_REGISTER, MATCH_CONSTANT, MATCH_NIL, MATCH_NIL, &CCodeGen_AArch32::Emit_Mov_RegCst },
	{ OP_MOV, MATCH_MEMORY,   MATCH_REGISTER, MATCH_NIL, MATCH_NIL, &CCodeGen_AArch32::Emit_Mov_MemReg },
	{ OP_MOV, MATCH_MEMORY,   MATCH_MEMORY,   MATCH_NIL, MATCH_NIL, &CCodeGen_AArch32::Emit_Mov_MemMem },
	{ OP_MOV, MATCH_MEMORY,   MATCH_CONSTANT, MATCH_NIL, MATCH_NIL, &CCodeGen_AArch32::Emit_Mov_MemCst },

	{ OP_MOV, MATCH_REG_REF, MATCH_MEM_REF, MATCH_NIL, MATCH_NIL, &CCodeGen_AArch32::Emit_Mov_RegRefMemRef },
	{ OP_MOV, MATCH_MEM_REF, MATCH_REG_REF, MATCH_NIL, MATCH_NIL, &CCodeGen_AArch32::Emit_Mov_MemRefRegRef },

	ALU_CONST_MATCHERS(OP_ADD, ALUOP_ADD)
	ALU_CONST_MATCHERS(OP_SUB, ALUOP_SUB)
	ALU_CONST_MATCHERS(OP_AND, ALUOP_AND)
	ALU_CONST_MATCHERS(OP_OR,  ALUOP_OR)
	ALU_CONST_MATCHERS(OP_XOR, ALUOP_XOR)
	
	{ OP_LZC, MATCH_VARIABLE, MATCH_VARIABLE, MATCH_NIL, MATCH_NIL, &CCodeGen_AArch32::Emit_Lzc_VarVar },

	{ OP_SRL, MATCH_ANY, MATCH_ANY, MATCH_ANY, MATCH_NIL, &CCodeGen_AArch32::Emit_Shift_Generic<CAArch32Assembler::SHIFT_LSR> },
	{ OP_SRA, MATCH_ANY, MATCH_ANY, MATCH_ANY, MATCH_NIL, &CCodeGen_AArch32::Emit_Shift_Generic<CAArch32Assembler::SHIFT_ASR> },
	{ OP_SLL, MATCH_ANY, MATCH_ANY, MATCH_ANY, MATCH_NIL, &CCodeGen_AArch32::Emit_Shift_Generic<CAArch32Assembler::SHIFT_LSL> },

	{ OP_PARAM, MATCH_NIL, MATCH_CONTEXT,    MATCH_NIL, MATCH_NIL, &CCodeGen_AArch32::Emit_Param_Ctx    },
	{ OP_PARAM, MATCH_NIL, MATCH_REGISTER,   MATCH_NIL, MATCH_NIL, &CCodeGen_AArch32::Emit_Param_Reg    },
	{ OP_PARAM, MATCH_NIL, MATCH_MEMORY,     MATCH_NIL, MATCH_NIL, &CCodeGen_AArch32::Emit_Param_Mem    },
	{ OP_PARAM, MATCH_NIL, MATCH_CONSTANT,   MATCH_NIL, MATCH_NIL, &CCodeGen_AArch32::Emit_Param_Cst    },
	{ OP_PARAM, MATCH_NIL, MATCH_MEMORY64,   MATCH_NIL, MATCH_NIL, &CCodeGen_AArch32::Emit_Param_Mem64  },
	{ OP_PARAM, MATCH_NIL, MATCH_CONSTANT64, MATCH_NIL, MATCH_NIL, &CCodeGen_AArch32::Emit_Param_Cst64  },
	{ OP_PARAM, MATCH_NIL, MATCH_MEMORY128,  MATCH_NIL, MATCH_NIL, &CCodeGen_AArch32::Emit_Param_Mem128 },

	{ OP_PARAM_RET, MATCH_NIL, MATCH_TEMPORARY128, MATCH_NIL, MATCH_NIL, &CCodeGen_AArch32::Emit_ParamRet_Tmp128 },

	{ OP_CALL, MATCH_NIL, MATCH_CONSTANTPTR, MATCH_CONSTANT, MATCH_NIL, &CCodeGen_AArch32::Emit_Call },
	
	{ OP_RETVAL, MATCH_REGISTER,  MATCH_NIL, MATCH_NIL, MATCH_NIL, &CCodeGen_AArch32::Emit_RetVal_Reg   },
	{ OP_RETVAL, MATCH_TEMPORARY, MATCH_NIL, MATCH_NIL, MATCH_NIL, &CCodeGen_AArch32::Emit_RetVal_Tmp   },
	{ OP_RETVAL, MATCH_MEMORY64,  MATCH_NIL, MATCH_NIL, MATCH_NIL, &CCodeGen_AArch32::Emit_RetVal_Mem64 },

	{ OP_EXTERNJMP,     MATCH_NIL, MATCH_CONSTANTPTR, MATCH_NIL, MATCH_NIL, &CCodeGen_AArch32::Emit_ExternJmp        },
	{ OP_EXTERNJMP_DYN, MATCH_NIL, MATCH_CONSTANTPTR, MATCH_NIL, MATCH_NIL, &CCodeGen_AArch32::Emit_ExternJmpDynamic },

	{ OP_JMP, MATCH_NIL, MATCH_NIL, MATCH_NIL, MATCH_NIL, &CCodeGen_AArch32::Emit_Jmp },

	{ OP_CONDJMP, MATCH_NIL, MATCH_VARIABLE, MATCH_CONSTANT, MATCH_NIL, &CCodeGen_AArch32::Emit_CondJmp_VarCst     },
	{ OP_CONDJMP, MATCH_NIL, MATCH_VARIABLE, MATCH_VARIABLE, MATCH_NIL, &CCodeGen_AArch32::Emit_CondJmp_VarVar     },
	{ OP_CONDJMP, MATCH_NIL, MATCH_VAR_REF,  MATCH_CONSTANT, MATCH_NIL, &CCodeGen_AArch32::Emit_CondJmp_Ref_VarCst },
	
	{ OP_CMP, MATCH_ANY, MATCH_ANY, MATCH_CONSTANT, MATCH_NIL, &CCodeGen_AArch32::Emit_Cmp_AnyAnyCst },
	{ OP_CMP, MATCH_ANY, MATCH_ANY, MATCH_ANY,      MATCH_NIL, &CCodeGen_AArch32::Emit_Cmp_AnyAnyAny },

	{ OP_SELECT, MATCH_VARIABLE, MATCH_VARIABLE, MATCH_ANY, MATCH_ANY, &CCodeGen_AArch32::Emit_Select_VarVarAnyAny },
	
	{ OP_CMPSELECT_P1, MATCH_NIL,      MATCH_ANY, MATCH_VARIABLE, MATCH_NIL, &CCodeGen_AArch32::Emit_CmpSelectP1_AnyVar    },
	{ OP_CMPSELECT_P2, MATCH_VARIABLE, MATCH_ANY, MATCH_ANY,      MATCH_NIL, &CCodeGen_AArch32::Emit_CmpSelectP2_VarAnyAny },

	{ OP_NOT, MATCH_REGISTER, MATCH_REGISTER, MATCH_NIL, MATCH_NIL, &CCodeGen_AArch32::Emit_Not_RegReg },
	{ OP_NOT, MATCH_MEMORY,   MATCH_REGISTER, MATCH_NIL, MATCH_NIL, &CCodeGen_AArch32::Emit_Not_MemReg },
	{ OP_NOT, MATCH_MEMORY,   MATCH_MEMORY,   MATCH_NIL, MATCH_NIL, &CCodeGen_AArch32::Emit_Not_MemMem },
	
	{ OP_DIV,  MATCH_TEMPORARY64, MATCH_ANY, MATCH_ANY, MATCH_NIL, &CCodeGen_AArch32::Emit_DivTmp64AnyAny<false> },
	{ OP_DIVS, MATCH_TEMPORARY64, MATCH_ANY, MATCH_ANY, MATCH_NIL, &CCodeGen_AArch32::Emit_DivTmp64AnyAny<true>  },
	
	{ OP_MUL,  MATCH_TEMPORARY64, MATCH_ANY, MATCH_ANY, MATCH_NIL, &CCodeGen_AArch32::Emit_MulTmp64AnyAny<false> },
	{ OP_MULS, MATCH_TEMPORARY64, MATCH_ANY, MATCH_ANY, MATCH_NIL, &CCodeGen_AArch32::Emit_MulTmp64AnyAny<true>  },

	{ OP_RELTOREF, MATCH_VAR_REF, MATCH_CONSTANT, MATCH_ANY, MATCH_NIL, &CCodeGen_AArch32::Emit_RelToRef_VarCst },

	{ OP_ADDREF, MATCH_VAR_REF, MATCH_VAR_REF, MATCH_ANY, MATCH_NIL, &CCodeGen_AArch32::Emit_AddRef_VarVarAny },
	
	{ OP_ISREFNULL, MATCH_VARIABLE, MATCH_VAR_REF, MATCH_ANY, MATCH_NIL, &CCodeGen_AArch32::Emit_IsRefNull_VarVar },

	{ OP_LOADFROMREF, MATCH_VARIABLE, MATCH_VAR_REF, MATCH_NIL,   MATCH_NIL, &CCodeGen_AArch32::Emit_LoadFromRef_VarVar    },
	{ OP_LOADFROMREF, MATCH_VARIABLE, MATCH_VAR_REF, MATCH_ANY32, MATCH_NIL, &CCodeGen_AArch32::Emit_LoadFromRef_VarVarAny },

	{ OP_LOADFROMREF, MATCH_VAR_REF, MATCH_VAR_REF, MATCH_NIL, MATCH_NIL, &CCodeGen_AArch32::Emit_LoadFromRef_Ref_VarVar },
	{ OP_LOADFROMREF, MATCH_VAR_REF, MATCH_VAR_REF, MATCH_ANY32, MATCH_NIL, &CCodeGen_AArch32::Emit_LoadFromRef_Ref_VarVarAny },

	{ OP_LOAD8FROMREF, MATCH_VARIABLE, MATCH_VAR_REF, MATCH_NIL, MATCH_NIL, &CCodeGen_AArch32::Emit_Load8FromRef_MemVar },
	{ OP_LOAD8FROMREF, MATCH_VARIABLE, MATCH_VAR_REF, MATCH_ANY32, MATCH_NIL, &CCodeGen_AArch32::Emit_Load8FromRef_MemVarAny },

	{ OP_LOAD16FROMREF, MATCH_VARIABLE, MATCH_VAR_REF, MATCH_NIL, MATCH_NIL, &CCodeGen_AArch32::Emit_Load16FromRef_MemVar },
	{ OP_LOAD16FROMREF, MATCH_VARIABLE, MATCH_VAR_REF, MATCH_ANY32, MATCH_NIL, &CCodeGen_AArch32::Emit_Load16FromRef_MemVarAny },

	{ OP_STOREATREF, MATCH_NIL, MATCH_VAR_REF, MATCH_ANY32, MATCH_NIL, &CCodeGen_AArch32::Emit_StoreAtRef_VarAny },
	{ OP_STOREATREF, MATCH_NIL, MATCH_VAR_REF, MATCH_ANY32, MATCH_ANY32, &CCodeGen_AArch32::Emit_StoreAtRef_VarAnyAny },

	{ OP_STORE8ATREF, MATCH_NIL, MATCH_VAR_REF, MATCH_ANY, MATCH_NIL, &CCodeGen_AArch32::Emit_Store8AtRef_VarAny },
	{ OP_STORE8ATREF, MATCH_NIL, MATCH_VAR_REF, MATCH_ANY, MATCH_ANY32, &CCodeGen_AArch32::Emit_Store8AtRef_VarAnyAny },

	{ OP_STORE16ATREF, MATCH_NIL, MATCH_VAR_REF, MATCH_ANY, MATCH_NIL, &CCodeGen_AArch32::Emit_Store16AtRef_VarAny },
	{ OP_STORE16ATREF, MATCH_NIL, MATCH_VAR_REF, MATCH_ANY, MATCH_ANY32, &CCodeGen_AArch32::Emit_Store16AtRef_VarAnyAny },

	{ OP_MOV, MATCH_NIL, MATCH_NIL, MATCH_NIL, MATCH_NIL, nullptr },
};
// clang-format on

CCodeGen_AArch32::CCodeGen_AArch32()
{
#ifdef __ANDROID__
	auto cpuFeatures = android_getCpuFeatures();
	if(cpuFeatures & ANDROID_CPU_ARM_FEATURE_IDIV_ARM)
	{
		m_hasIntegerDiv = true;
	}
#endif

	InsertMatchers(g_constMatchers);
	InsertMatchers(g_64ConstMatchers);
	InsertMatchers(g_fpuConstMatchers);
	InsertMatchers(g_mdConstMatchers);
}

void CCodeGen_AArch32::SetPlatformAbi(PLATFORM_ABI platformAbi)
{
	m_platformAbi = platformAbi;
}

unsigned int CCodeGen_AArch32::GetAvailableRegisterCount() const
{
	return MAX_REGISTERS;
}

unsigned int CCodeGen_AArch32::GetAvailableMdRegisterCount() const
{
	return 0;
}

bool CCodeGen_AArch32::Has128BitsCallOperands() const
{
	return true;
}

bool CCodeGen_AArch32::CanHold128BitsReturnValueInRegisters() const
{
	return false;
}

bool CCodeGen_AArch32::SupportsExternalJumps() const
{
	return true;
}

bool CCodeGen_AArch32::SupportsCmpSelect() const
{
	return true;
}

uint32 CCodeGen_AArch32::GetPointerSize() const
{
	return 4;
}

void CCodeGen_AArch32::SetStream(Framework::CStream* stream)
{
	m_stream = stream;
	m_assembler.SetStream(stream);
}

void CCodeGen_AArch32::RegisterExternalSymbols(CObjectFile* objectFile) const
{
	objectFile->AddExternalSymbol("_CodeGen_AArch32_div_unsigned", reinterpret_cast<uintptr_t>(&CodeGen_AArch32_div_unsigned));
	objectFile->AddExternalSymbol("_CodeGen_AArch32_div_signed", reinterpret_cast<uintptr_t>(&CodeGen_AArch32_div_signed));
	objectFile->AddExternalSymbol("_CodeGen_AArch32_mod_unsigned", reinterpret_cast<uintptr_t>(&CodeGen_AArch32_mod_unsigned));
	objectFile->AddExternalSymbol("_CodeGen_AArch32_mod_signed", reinterpret_cast<uintptr_t>(&CodeGen_AArch32_mod_signed));
}

void CCodeGen_AArch32::GenerateCode(const StatementList& statements, unsigned int stackSize)
{
	//Align stack size (must be aligned on 16 bytes boundary)
	m_stackSize = (stackSize + 0xF) & ~0xF;

	m_registerSave = GetSavedRegisterList(GetRegisterUsage(statements));

	Emit_Prolog();

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
			if(!SymbolMatches(matcher.src3Type, statement.src3)) continue;
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

	Emit_Epilog();
	m_assembler.Bx(CAArch32Assembler::rLR);

	m_assembler.ResolveLabelReferences();
	m_assembler.ClearLabels();
	m_assembler.ResolveLiteralReferences();
	m_labels.clear();
}

void CCodeGen_AArch32::InsertMatchers(const CONSTMATCHER* constMatchers)
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

uint16 CCodeGen_AArch32::GetSavedRegisterList(uint32 registerUsage)
{
	uint16 registerSave = 0;
	for(unsigned int i = 0; i < MAX_REGISTERS; i++)
	{
		if((1 << i) & registerUsage)
		{
			registerSave |= (1 << g_registers[i]);
		}
	}
	registerSave |= (1 << g_callAddressRegister);
	registerSave |= (1 << g_tempParamRegister0);
	registerSave |= (1 << g_tempParamRegister1);
	registerSave |= (1 << g_baseRegister);
	registerSave |= (1 << CAArch32Assembler::rLR);
	return registerSave;
}

void CCodeGen_AArch32::Emit_Prolog()
{
	m_assembler.Stmdb(CAArch32Assembler::rSP, m_registerSave);
	m_assembler.Mov(CAArch32Assembler::r11, CAArch32Assembler::r0);

	//Align stack to 16 bytes boundary
	m_assembler.Mov(CAArch32Assembler::r0, CAArch32Assembler::rSP);
	m_assembler.Bic(CAArch32Assembler::rSP, CAArch32Assembler::rSP, CAArch32Assembler::MakeImmediateAluOperand(0xF, 0));
	m_assembler.Sub(CAArch32Assembler::rSP, CAArch32Assembler::rSP, CAArch32Assembler::MakeImmediateAluOperand(0xC, 0));
	m_assembler.Stmdb(CAArch32Assembler::rSP, (1 << CAArch32Assembler::r0));

	if(m_stackSize != 0)
	{
		uint8 allocImm = 0, allocSa = 0;
		bool succeeded = TryGetAluImmediateParams(m_stackSize, allocImm, allocSa);
		if(succeeded)
		{
			m_assembler.Sub(CAArch32Assembler::rSP, CAArch32Assembler::rSP, CAArch32Assembler::MakeImmediateAluOperand(allocImm, allocSa));
		}
		else
		{
			auto stackResReg = CAArch32Assembler::r0;
			LoadConstantInRegister(stackResReg, m_stackSize);
			m_assembler.Sub(CAArch32Assembler::rSP, CAArch32Assembler::rSP, stackResReg);
		}
	}
	m_stackLevel = 0;
}

void CCodeGen_AArch32::Emit_Epilog()
{
	//Since Emit_Epilog can be called by Emit_ExternJmp, we need to be
	//extra careful not to write to r0, since it's used to save and
	//transmit the function's parameter (r11) to the target
	static const auto stackTempRegister = CAArch32Assembler::r3;

	if(m_stackSize != 0)
	{
		uint8 allocImm = 0, allocSa = 0;
		bool succeeded = TryGetAluImmediateParams(m_stackSize, allocImm, allocSa);
		if(succeeded)
		{
			m_assembler.Add(CAArch32Assembler::rSP, CAArch32Assembler::rSP, CAArch32Assembler::MakeImmediateAluOperand(allocImm, allocSa));
		}
		else
		{
			LoadConstantInRegister(stackTempRegister, m_stackSize);
			m_assembler.Add(CAArch32Assembler::rSP, CAArch32Assembler::rSP, stackTempRegister);
		}
	}

	//Restore previous unaligned SP
	m_assembler.Ldmia(CAArch32Assembler::rSP, (1 << stackTempRegister));
	m_assembler.Mov(CAArch32Assembler::rSP, stackTempRegister);

	m_assembler.Ldmia(CAArch32Assembler::rSP, m_registerSave);
}

bool CCodeGen_AArch32::TryGetAluImmediateParams(uint32 constant, uint8& immediate, uint8& shiftAmount)
{
	uint32 shadowConstant = constant;
	shiftAmount = 0xFF;

	for(unsigned int i = 0; i < 16; i++)
	{
		if((shadowConstant & 0xFF) == shadowConstant)
		{
			shiftAmount = i;
			break;
		}
		shadowConstant = __builtin_rotateleft32(shadowConstant, 2);
	}

	if(shiftAmount != 0xFF)
	{
		immediate = static_cast<uint8>(shadowConstant);
		return true;
	}
	else
	{
		return false;
	}
}

void CCodeGen_AArch32::LoadConstantInRegister(CAArch32Assembler::REGISTER registerId, uint32 constant)
{
	//Try normal move
	{
		uint8 immediate = 0;
		uint8 shiftAmount = 0;
		if(TryGetAluImmediateParams(constant, immediate, shiftAmount))
		{
			m_assembler.Mov(registerId, CAArch32Assembler::MakeImmediateAluOperand(immediate, shiftAmount));
			return;
		}
	}

	//Try not move
	{
		uint8 immediate = 0;
		uint8 shiftAmount = 0;
		if(TryGetAluImmediateParams(~constant, immediate, shiftAmount))
		{
			m_assembler.Mvn(registerId, CAArch32Assembler::MakeImmediateAluOperand(immediate, shiftAmount));
			return;
		}
	}

	//Otherwise, use paired move
	m_assembler.Movw(registerId, static_cast<uint16>(constant & 0xFFFF));
	if((constant & 0xFFFF0000) != 0)
	{
		m_assembler.Movt(registerId, static_cast<uint16>(constant >> 16));
	}
}

void CCodeGen_AArch32::LoadConstantPtrInRegister(CAArch32Assembler::REGISTER registerId, uintptr_t constant)
{
	m_assembler.Movw(registerId, static_cast<uint16>(constant & 0xFFFF));
	m_assembler.Movt(registerId, static_cast<uint16>(constant >> 16));

	if(m_externalSymbolReferencedHandler)
	{
		auto position = m_stream->GetLength();
		m_externalSymbolReferencedHandler(constant, position - 8, CCodeGen::SYMBOL_REF_TYPE::ARMV7_LOAD_HALF);
	}
}

void CCodeGen_AArch32::LoadRefIndexAddress(CAArch32Assembler::REGISTER dstRegister, CSymbol* refSymbol, CAArch32Assembler::REGISTER refRegister, CSymbol* indexSymbol, CAArch32Assembler::REGISTER indexRegister, uint8 scale)
{
	assert(scale == 1);

	refRegister = PrepareSymbolRegisterUseRef(refSymbol, refRegister);

	if(uint8 immediate = 0, shiftAmount = 0;
	   indexSymbol->IsConstant() && TryGetAluImmediateParams(indexSymbol->m_valueLow, immediate, shiftAmount))
	{
		m_assembler.Add(dstRegister, refRegister, CAArch32Assembler::MakeImmediateAluOperand(immediate, shiftAmount));
	}
	else
	{
		auto indexReg = PrepareSymbolRegisterUse(indexSymbol, indexRegister);
		m_assembler.Add(dstRegister, refRegister, indexReg);
	}
}

void CCodeGen_AArch32::LoadMemoryInRegister(CAArch32Assembler::REGISTER registerId, CSymbol* src)
{
	switch(src->m_type)
	{
	case SYM_RELATIVE:
		assert((src->m_valueLow & 0x03) == 0x00);
		m_assembler.Ldr(registerId, g_baseRegister, CAArch32Assembler::MakeImmediateLdrAddress(src->m_valueLow));
		break;
	case SYM_TEMPORARY:
		m_assembler.Ldr(registerId, CAArch32Assembler::rSP, CAArch32Assembler::MakeImmediateLdrAddress(src->m_stackLocation + m_stackLevel));
		break;
	default:
		assert(0);
		break;
	}
}

void CCodeGen_AArch32::StoreRegisterInMemory(CSymbol* dst, CAArch32Assembler::REGISTER registerId)
{
	switch(dst->m_type)
	{
	case SYM_RELATIVE:
		assert((dst->m_valueLow & 0x03) == 0x00);
		m_assembler.Str(registerId, g_baseRegister, CAArch32Assembler::MakeImmediateLdrAddress(dst->m_valueLow));
		break;
	case SYM_TEMPORARY:
		m_assembler.Str(registerId, CAArch32Assembler::rSP, CAArch32Assembler::MakeImmediateLdrAddress(dst->m_stackLocation + m_stackLevel));
		break;
	default:
		assert(0);
		break;
	}
}

void CCodeGen_AArch32::LoadMemoryReferenceInRegister(CAArch32Assembler::REGISTER registerId, CSymbol* src)
{
	switch(src->m_type)
	{
	case SYM_REL_REFERENCE:
		LoadRelativeReferenceInRegister(registerId, src);
		break;
	case SYM_TMP_REFERENCE:
		LoadTemporaryReferenceInRegister(registerId, src);
		break;
	default:
		assert(false);
		break;
	}
}

void CCodeGen_AArch32::LoadRelativeReferenceInRegister(CAArch32Assembler::REGISTER registerId, CSymbol* src)
{
	assert(src->m_type == SYM_REL_REFERENCE);
	assert((src->m_valueLow & 0x03) == 0x00);
	m_assembler.Ldr(registerId, g_baseRegister, CAArch32Assembler::MakeImmediateLdrAddress(src->m_valueLow));
}

void CCodeGen_AArch32::LoadTemporaryReferenceInRegister(CAArch32Assembler::REGISTER registerId, CSymbol* src)
{
	assert(src->m_type == SYM_TMP_REFERENCE);
	m_assembler.Ldr(registerId, CAArch32Assembler::rSP, CAArch32Assembler::MakeImmediateLdrAddress(src->m_stackLocation + m_stackLevel));
}

void CCodeGen_AArch32::StoreRegisterInTemporaryReference(CSymbol* dst, CAArch32Assembler::REGISTER registerId)
{
	assert(dst->m_type == SYM_TMP_REFERENCE);
	m_assembler.Str(registerId, CAArch32Assembler::rSP, CAArch32Assembler::MakeImmediateLdrAddress(dst->m_stackLocation + m_stackLevel));
}

CAArch32Assembler::AluLdrShift CCodeGen_AArch32::GetAluShiftFromSymbol(CAArch32Assembler::SHIFT shiftType, CSymbol* symbol, CAArch32Assembler::REGISTER preferedRegister)
{
	switch(symbol->m_type)
	{
	case SYM_REGISTER:
		m_assembler.And(preferedRegister, g_registers[symbol->m_valueLow], CAArch32Assembler::MakeImmediateAluOperand(0x1F, 0));
		return CAArch32Assembler::MakeVariableShift(shiftType, preferedRegister);
		break;
	case SYM_TEMPORARY:
	case SYM_RELATIVE:
		LoadMemoryInRegister(preferedRegister, symbol);
		m_assembler.And(preferedRegister, preferedRegister, CAArch32Assembler::MakeImmediateAluOperand(0x1F, 0));
		return CAArch32Assembler::MakeVariableShift(shiftType, preferedRegister);
		break;
	case SYM_CONSTANT:
		return CAArch32Assembler::MakeConstantShift(shiftType, static_cast<uint8>(symbol->m_valueLow & 0x1F));
		break;
	default:
		throw std::runtime_error("Invalid symbol type.");
		break;
	}
}

CAArch32Assembler::LdrAddress CCodeGen_AArch32::MakeScaledLdrAddress(CAArch32Assembler::REGISTER indexReg, uint8 scale)
{
	switch(scale)
	{
	default:
		assert(false);
		[[fallthrough]];
	case 1:
		return CAArch32Assembler::MakeRegisterLdrAddress(indexReg);
	case 4:
		return CAArch32Assembler::MakeRegisterLdrAddress(indexReg, CAArch32Assembler::SHIFT_LSL, 2);
	}
}

CAArch32Assembler::REGISTER CCodeGen_AArch32::PrepareSymbolRegisterDef(CSymbol* symbol, CAArch32Assembler::REGISTER preferedRegister)
{
	switch(symbol->m_type)
	{
	case SYM_REGISTER:
		return g_registers[symbol->m_valueLow];
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

CAArch32Assembler::REGISTER CCodeGen_AArch32::PrepareSymbolRegisterUse(CSymbol* symbol, CAArch32Assembler::REGISTER preferedRegister)
{
	switch(symbol->m_type)
	{
	case SYM_REGISTER:
		return g_registers[symbol->m_valueLow];
		break;
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

void CCodeGen_AArch32::CommitSymbolRegister(CSymbol* symbol, CAArch32Assembler::REGISTER usedRegister)
{
	switch(symbol->m_type)
	{
	case SYM_REGISTER:
		assert(usedRegister == g_registers[symbol->m_valueLow]);
		break;
	case SYM_TEMPORARY:
	case SYM_RELATIVE:
		StoreRegisterInMemory(symbol, usedRegister);
		break;
	default:
		throw std::runtime_error("Invalid symbol type.");
		break;
	}
}

CAArch32Assembler::REGISTER CCodeGen_AArch32::PrepareSymbolRegisterDefRef(CSymbol* symbol, CAArch32Assembler::REGISTER preferedRegister)
{
	switch(symbol->m_type)
	{
	case SYM_REG_REFERENCE:
		assert(symbol->m_valueLow < MAX_REGISTERS);
		return g_registers[symbol->m_valueLow];
		break;
	case SYM_TMP_REFERENCE:
	case SYM_REL_REFERENCE:
		return preferedRegister;
		break;
	default:
		throw std::runtime_error("Invalid symbol type.");
		break;
	}
}

CAArch32Assembler::REGISTER CCodeGen_AArch32::PrepareSymbolRegisterUseRef(CSymbol* symbol, CAArch32Assembler::REGISTER preferedRegister)
{
	switch(symbol->m_type)
	{
	case SYM_REG_REFERENCE:
		assert(symbol->m_valueLow < MAX_REGISTERS);
		return g_registers[symbol->m_valueLow];
		break;
	case SYM_TMP_REFERENCE:
	case SYM_REL_REFERENCE:
		LoadMemoryReferenceInRegister(preferedRegister, symbol);
		return preferedRegister;
		break;
	default:
		throw std::runtime_error("Invalid symbol type.");
		break;
	}
}

void CCodeGen_AArch32::CommitSymbolRegisterRef(CSymbol* symbol, CAArch32Assembler::REGISTER usedRegister)
{
	switch(symbol->m_type)
	{
	case SYM_REG_REFERENCE:
		assert(usedRegister == g_registers[symbol->m_valueLow]);
		break;
	case SYM_TMP_REFERENCE:
		StoreRegisterInTemporaryReference(symbol, usedRegister);
		break;
	default:
		throw std::runtime_error("Invalid symbol type.");
		break;
	}
}

CAArch32Assembler::REGISTER CCodeGen_AArch32::PrepareParam(PARAM_STATE& paramState)
{
	assert(!paramState.prepared);
	paramState.prepared = true;
	if(paramState.index < MAX_PARAM_REGS)
	{
		return g_paramRegs[paramState.index];
	}
	else
	{
		return g_tempParamRegister0;
	}
}

CCodeGen_AArch32::ParamRegisterPair CCodeGen_AArch32::PrepareParam64(PARAM_STATE& paramState)
{
	assert(!paramState.prepared);
	paramState.prepared = true;
	//On ARM EABI, 64 bits params have a 8-byte alignment requirement
	//If the param is passed through registers, it has to start on an even register
	//On iOS ABI, 64 bit params have a 4-byte alignment, alignment is not required
	if(m_platformAbi == PLATFORM_ABI_ARMEABI)
	{
		if(paramState.index & 1)
		{
			paramState.index++;
		}
	}
	ParamRegisterPair result;
	for(unsigned int i = 0; i < 2; i++)
	{
		if((paramState.index + i) < MAX_PARAM_REGS)
		{
			result[i] = g_paramRegs[paramState.index + i];
		}
		else
		{
			result[i] = (i == 0) ? g_tempParamRegister0 : g_tempParamRegister1;
		}
	}
	return result;
}

void CCodeGen_AArch32::CommitParam(PARAM_STATE& paramState)
{
	assert(paramState.prepared);
	paramState.prepared = false;
	if(paramState.index < MAX_PARAM_REGS)
	{
		//Nothing to do
	}
	else
	{
		uint32 stackSlot = ((paramState.index - MAX_PARAM_REGS) + 1) * 4;
		m_assembler.Str(g_tempParamRegister0, CAArch32Assembler::rSP,
		                CAArch32Assembler::MakeImmediateLdrAddress(m_stackLevel - stackSlot));
	}
	paramState.index++;
}

void CCodeGen_AArch32::CommitParam64(PARAM_STATE& paramState)
{
	assert(paramState.prepared);
	paramState.prepared = false;
	uint32 stackPosition = std::max<int>((paramState.index - MAX_PARAM_REGS) + 2, 0);
	for(unsigned int i = 0; i < 2; i++)
	{
		if(paramState.index < MAX_PARAM_REGS)
		{
			//Nothing to do
		}
		else
		{
			assert(stackPosition > 0);
			auto tempParamReg = (i == 0) ? g_tempParamRegister0 : g_tempParamRegister1;
			uint32 stackSlot = i;
			if(stackPosition == 1)
			{
				//Special case for when parameters are spread between
				//registers and stack
				assert(i == 1);
				stackSlot = 0;
			}
			m_assembler.Str(tempParamReg, CAArch32Assembler::rSP,
			                CAArch32Assembler::MakeImmediateLdrAddress(m_stackLevel - (stackPosition * 4) + (stackSlot * 4)));
		}
	}
	paramState.index += 2;
}

CAArch32Assembler::LABEL CCodeGen_AArch32::GetLabel(uint32 blockId)
{
	CAArch32Assembler::LABEL result;
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

void CCodeGen_AArch32::MarkLabel(const STATEMENT& statement)
{
	auto label = GetLabel(statement.jmpBlock);
	m_assembler.MarkLabel(label);
}

void CCodeGen_AArch32::Emit_Nop(const STATEMENT& statement)
{
}

void CCodeGen_AArch32::Emit_Param_Ctx(const STATEMENT& statement)
{
	FRAMEWORK_MAYBE_UNUSED auto src1 = statement.src1->GetSymbol().get();

	assert(src1->m_type == SYM_CONTEXT);

	m_params.push_back(
	    [this](PARAM_STATE& paramState) {
		    auto paramReg = PrepareParam(paramState);
		    m_assembler.Mov(paramReg, g_baseRegister);
		    CommitParam(paramState);
	    });
}

void CCodeGen_AArch32::Emit_Param_Reg(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();

	assert(src1->m_type == SYM_REGISTER);

	m_params.push_back(
	    [this, src1](PARAM_STATE& paramState) {
		    auto paramReg = PrepareParam(paramState);
		    m_assembler.Mov(paramReg, g_registers[src1->m_valueLow]);
		    CommitParam(paramState);
	    });
}

void CCodeGen_AArch32::Emit_Param_Mem(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();

	m_params.push_back(
	    [this, src1](PARAM_STATE& paramState) {
		    auto paramReg = PrepareParam(paramState);
		    LoadMemoryInRegister(paramReg, src1);
		    CommitParam(paramState);
	    });
}

void CCodeGen_AArch32::Emit_Param_Cst(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();

	assert(src1->m_type == SYM_CONSTANT);

	m_params.push_back(
	    [this, src1](PARAM_STATE& paramState) {
		    auto paramReg = PrepareParam(paramState);
		    LoadConstantInRegister(paramReg, src1->m_valueLow);
		    CommitParam(paramState);
	    });
}

void CCodeGen_AArch32::Emit_Param_Mem64(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();

	m_params.push_back(
	    [this, src1](PARAM_STATE& paramState) {
		    auto paramRegs = PrepareParam64(paramState);
		    LoadMemory64InRegisters(paramRegs[0], paramRegs[1], src1);
		    CommitParam64(paramState);
	    });
}

void CCodeGen_AArch32::Emit_Param_Cst64(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();

	m_params.push_back(
	    [this, src1](PARAM_STATE& paramState) {
		    auto paramRegs = PrepareParam64(paramState);
		    LoadConstantInRegister(paramRegs[0], src1->m_valueLow);
		    LoadConstantInRegister(paramRegs[1], src1->m_valueHigh);
		    CommitParam64(paramState);
	    });
}

void CCodeGen_AArch32::Emit_Param_Mem128(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();

	m_params.push_back(
	    [this, src1](PARAM_STATE& paramState) {
		    auto paramReg = PrepareParam(paramState);
		    LoadMemory128AddressInRegister(paramReg, src1);
		    CommitParam(paramState);
	    });
}

void CCodeGen_AArch32::Emit_ParamRet_Tmp128(const STATEMENT& statement)
{
	Emit_Param_Mem128(statement);
}

void CCodeGen_AArch32::Emit_Call(const STATEMENT& statement)
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

	uint32 stackAlloc = (paramState.index > MAX_PARAM_REGS) ? ((paramState.index - MAX_PARAM_REGS) * 4) : 0;

	if(stackAlloc != 0)
	{
		m_assembler.Sub(CAArch32Assembler::rSP, CAArch32Assembler::rSP,
		                CAArch32Assembler::MakeImmediateAluOperand(stackAlloc, 0));
	}

	//No value should be saved in r4 at this point (register is spilled before)
	LoadConstantPtrInRegister(g_callAddressRegister, src1->GetConstantPtr());
	m_assembler.Mov(CAArch32Assembler::rLR, CAArch32Assembler::rPC);
	m_assembler.Mov(CAArch32Assembler::rPC, g_callAddressRegister);

	if(stackAlloc != 0)
	{
		m_assembler.Add(CAArch32Assembler::rSP, CAArch32Assembler::rSP,
		                CAArch32Assembler::MakeImmediateAluOperand(stackAlloc, 0));
	}
}

void CCodeGen_AArch32::Emit_RetVal_Reg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();

	assert(dst->m_type == SYM_REGISTER);

	m_assembler.Mov(g_registers[dst->m_valueLow], CAArch32Assembler::r0);
}

void CCodeGen_AArch32::Emit_RetVal_Tmp(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();

	assert(dst->m_type == SYM_TEMPORARY);

	StoreRegisterInMemory(dst, CAArch32Assembler::r0);
}

void CCodeGen_AArch32::Emit_RetVal_Mem64(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();

	StoreRegistersInMemory64(dst, CAArch32Assembler::r0, CAArch32Assembler::r1);
}

void CCodeGen_AArch32::Emit_ExternJmp(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();

	assert(src1->m_type == SYM_CONSTANTPTR);

	m_assembler.Mov(CAArch32Assembler::r0, g_baseRegister);
	Emit_Epilog();

	auto targetAddressRegister = CAArch32Assembler::r1;
	LoadConstantPtrInRegister(targetAddressRegister, src1->GetConstantPtr());
	m_assembler.Mov(CAArch32Assembler::rPC, targetAddressRegister);
}

void CCodeGen_AArch32::Emit_ExternJmpDynamic(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();

	assert(src1->m_type == SYM_CONSTANTPTR);

	m_assembler.Mov(CAArch32Assembler::r0, g_baseRegister);
	Emit_Epilog();

	m_assembler.Ldr_Pc(CAArch32Assembler::rPC, -4);

	//Write target function address
	if(m_externalSymbolReferencedHandler)
	{
		auto position = m_stream->GetLength();
		m_externalSymbolReferencedHandler(src1->GetConstantPtr(), position, CCodeGen::SYMBOL_REF_TYPE::NATIVE_POINTER);
	}
	m_stream->Write32(src1->GetConstantPtr());
}

void CCodeGen_AArch32::Emit_Mov_RegReg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	m_assembler.Mov(g_registers[dst->m_valueLow], g_registers[src1->m_valueLow]);
}

void CCodeGen_AArch32::Emit_Mov_RegMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	LoadMemoryInRegister(g_registers[dst->m_valueLow], src1);
}

void CCodeGen_AArch32::Emit_Mov_RegCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type == SYM_REGISTER);
	assert(src1->m_type == SYM_CONSTANT);

	LoadConstantInRegister(g_registers[dst->m_valueLow], src1->m_valueLow);
}

void CCodeGen_AArch32::Emit_Mov_MemReg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(src1->m_type == SYM_REGISTER);

	StoreRegisterInMemory(dst, g_registers[src1->m_valueLow]);
}

void CCodeGen_AArch32::Emit_Mov_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto tmpReg = CAArch32Assembler::r0;
	LoadMemoryInRegister(tmpReg, src1);
	StoreRegisterInMemory(dst, tmpReg);
}

void CCodeGen_AArch32::Emit_Mov_MemCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(src1->m_type == SYM_CONSTANT);

	auto tmpReg = CAArch32Assembler::r0;
	LoadConstantInRegister(tmpReg, src1->m_valueLow);
	StoreRegisterInMemory(dst, tmpReg);
}

void CCodeGen_AArch32::Emit_Mov_RegRefMemRef(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type == SYM_REG_REFERENCE);

	LoadMemoryReferenceInRegister(g_registers[dst->m_valueLow], src1);
}

void CCodeGen_AArch32::Emit_Mov_MemRefRegRef(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(src1->m_type == SYM_REG_REFERENCE);

	StoreRegisterInTemporaryReference(dst, g_registers[src1->m_valueLow]);
}

void CCodeGen_AArch32::Emit_Lzc_VarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstRegister = PrepareSymbolRegisterDef(dst, CAArch32Assembler::r0);
	auto src1Register = PrepareSymbolRegisterUse(src1, CAArch32Assembler::r1);

	auto set32Label = m_assembler.CreateLabel();
	auto startCountLabel = m_assembler.CreateLabel();
	auto doneLabel = m_assembler.CreateLabel();

	m_assembler.Mov(dstRegister, src1Register);
	m_assembler.Tst(dstRegister, dstRegister);
	m_assembler.BCc(CAArch32Assembler::CONDITION_EQ, set32Label);
	m_assembler.BCc(CAArch32Assembler::CONDITION_PL, startCountLabel);

	//reverse:
	m_assembler.Mvn(dstRegister, dstRegister);
	m_assembler.Tst(dstRegister, dstRegister);
	m_assembler.BCc(CAArch32Assembler::CONDITION_EQ, set32Label);

	//startCount:
	m_assembler.MarkLabel(startCountLabel);
	m_assembler.Clz(dstRegister, dstRegister);
	m_assembler.Sub(dstRegister, dstRegister, CAArch32Assembler::MakeImmediateAluOperand(1, 0));
	m_assembler.BCc(CAArch32Assembler::CONDITION_AL, doneLabel);

	//set32:
	m_assembler.MarkLabel(set32Label);
	LoadConstantInRegister(dstRegister, 0x1F);

	//done
	m_assembler.MarkLabel(doneLabel);

	CommitSymbolRegister(dst, dstRegister);
}

void CCodeGen_AArch32::Emit_Jmp(const STATEMENT& statement)
{
	m_assembler.BCc(CAArch32Assembler::CONDITION_AL, GetLabel(statement.jmpBlock));
}

void CCodeGen_AArch32::Emit_CondJmp(const STATEMENT& statement)
{
	auto label = GetLabel(statement.jmpBlock);
	auto cond = GetConditionCode(statement.jmpCondition);
	m_assembler.BCc(cond, label);
}

void CCodeGen_AArch32::Emit_CondJmp_VarVar(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type != SYM_CONSTANT); //We can do better if we have a constant

	auto src1Reg = PrepareSymbolRegisterUse(src1, CAArch32Assembler::r1);
	auto src2Reg = PrepareSymbolRegisterUse(src2, CAArch32Assembler::r2);
	m_assembler.Cmp(src1Reg, src2Reg);
	Emit_CondJmp(statement);
}

void CCodeGen_AArch32::Emit_CondJmp_VarCst(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_CONSTANT);

	auto src1Reg = PrepareSymbolRegisterUse(src1, CAArch32Assembler::r1);
	Cmp_GenericRegCst(src1Reg, src2->m_valueLow, CAArch32Assembler::r2);
	Emit_CondJmp(statement);
}

void CCodeGen_AArch32::Emit_CondJmp_Ref_VarCst(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	FRAMEWORK_MAYBE_UNUSED auto src2 = statement.src2->GetSymbol().get();

	auto src1Reg = PrepareSymbolRegisterUseRef(src1, CAArch32Assembler::r0);

	assert(src2->m_type == SYM_CONSTANT);
	assert(src2->m_valueLow == 0);
	assert((statement.jmpCondition == CONDITION_NE) || (statement.jmpCondition == CONDITION_EQ));

	auto label = GetLabel(statement.jmpBlock);

	m_assembler.Tst(src1Reg, src1Reg);
	switch(statement.jmpCondition)
	{
	case CONDITION_EQ:
		m_assembler.BCc(CAArch32Assembler::CONDITION_EQ, label);
		break;
	case CONDITION_NE:
		m_assembler.BCc(CAArch32Assembler::CONDITION_NE, label);
		break;
	default:
		assert(false);
		break;
	}
}

void CCodeGen_AArch32::Cmp_GetFlag(CAArch32Assembler::REGISTER registerId, Jitter::CONDITION condition)
{
	CAArch32Assembler::ImmediateAluOperand trueOperand(CAArch32Assembler::MakeImmediateAluOperand(1, 0));
	CAArch32Assembler::ImmediateAluOperand falseOperand(CAArch32Assembler::MakeImmediateAluOperand(0, 0));

	auto trueCondition = GetConditionCode(condition);
	auto falseCondition = GetConditionCode(NegateCondition(condition));

	m_assembler.MovCc(trueCondition, registerId, trueOperand);
	m_assembler.MovCc(falseCondition, registerId, falseOperand);
}

void CCodeGen_AArch32::Cmp_GenericRegCst(CAArch32Assembler::REGISTER src1Reg, uint32 src2, CAArch32Assembler::REGISTER src2Reg)
{
	uint8 immediate = 0;
	uint8 shiftAmount = 0;
	if(TryGetAluImmediateParams(src2, immediate, shiftAmount))
	{
		m_assembler.Cmp(src1Reg, CAArch32Assembler::MakeImmediateAluOperand(immediate, shiftAmount));
	}
	else if(TryGetAluImmediateParams(-static_cast<int32>(src2), immediate, shiftAmount))
	{
		m_assembler.Cmn(src1Reg, CAArch32Assembler::MakeImmediateAluOperand(immediate, shiftAmount));
	}
	else
	{
		assert(src1Reg != src2Reg);
		LoadConstantInRegister(src2Reg, src2);
		m_assembler.Cmp(src1Reg, src2Reg);
	}
}

void CCodeGen_AArch32::Emit_Cmp_AnyAnyAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDef(dst, CAArch32Assembler::r0);
	auto src1Reg = PrepareSymbolRegisterUse(src1, CAArch32Assembler::r1);
	auto src2Reg = PrepareSymbolRegisterUse(src2, CAArch32Assembler::r2);

	m_assembler.Cmp(src1Reg, src2Reg);
	Cmp_GetFlag(dstReg, statement.jmpCondition);
	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_AArch32::Emit_Cmp_AnyAnyCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_CONSTANT);

	auto dstReg = PrepareSymbolRegisterDef(dst, CAArch32Assembler::r0);
	auto src1Reg = PrepareSymbolRegisterUse(src1, CAArch32Assembler::r1);
	auto cst = src2->m_valueLow;

	Cmp_GenericRegCst(src1Reg, cst, CAArch32Assembler::r2);
	Cmp_GetFlag(dstReg, statement.jmpCondition);
	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_AArch32::Emit_Select_VarVarAnyAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	auto src3 = statement.src3->GetSymbol().get();

	//TODO: This could be slightly improved if we have immediate operands

	auto dstReg = PrepareSymbolRegisterDef(dst, CAArch32Assembler::r0);
	auto src1Reg = PrepareSymbolRegisterUse(src1, CAArch32Assembler::r1);
	auto src2Reg = PrepareSymbolRegisterUse(src2, CAArch32Assembler::r2);
	auto src3Reg = PrepareSymbolRegisterUse(src3, CAArch32Assembler::r3);

	m_assembler.Tst(src1Reg, src1Reg);
	m_assembler.MovCc(CAArch32Assembler::CONDITION_NE, dstReg, src2Reg);
	m_assembler.MovCc(CAArch32Assembler::CONDITION_EQ, dstReg, src3Reg);

	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_AArch32::Emit_CmpSelectP1_AnyVar(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto src1Reg = PrepareSymbolRegisterUse(src1, CAArch32Assembler::r1);
	auto src2Reg = PrepareSymbolRegisterUse(src2, CAArch32Assembler::r2);

	m_assembler.Cmp(src1Reg, src2Reg);
}

void CCodeGen_AArch32::Emit_CmpSelectP2_VarAnyAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDef(dst, CAArch32Assembler::r0);
	auto src1Reg = PrepareSymbolRegisterUse(src1, CAArch32Assembler::r1);
	auto src2Reg = PrepareSymbolRegisterUse(src2, CAArch32Assembler::r2);

	auto trueCondition = GetConditionCode(statement.jmpCondition);
	auto falseCondition = GetConditionCode(NegateCondition(statement.jmpCondition));

	m_assembler.MovCc(trueCondition, dstReg, src1Reg);
	m_assembler.MovCc(falseCondition, dstReg, src2Reg);

	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_AArch32::Emit_Not_RegReg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type == SYM_REGISTER);
	assert(src1->m_type == SYM_REGISTER);

	m_assembler.Mvn(g_registers[dst->m_valueLow], g_registers[src1->m_valueLow]);
}

void CCodeGen_AArch32::Emit_Not_MemReg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(src1->m_type == SYM_REGISTER);

	auto dstReg = CAArch32Assembler::r1;
	m_assembler.Mvn(dstReg, g_registers[src1->m_valueLow]);
	StoreRegisterInMemory(dst, dstReg);
}

void CCodeGen_AArch32::Emit_Not_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto srcReg = CAArch32Assembler::r0;
	auto dstReg = CAArch32Assembler::r1;
	LoadMemoryInRegister(srcReg, src1);
	m_assembler.Mvn(dstReg, srcReg);
	StoreRegisterInMemory(dst, dstReg);
}

void CCodeGen_AArch32::Emit_RelToRef_VarCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(src1->m_type == SYM_CONSTANT);

	auto dstReg = PrepareSymbolRegisterDefRef(dst, CAArch32Assembler::r0);

	uint8 immediate = 0;
	uint8 shiftAmount = 0;

	if(TryGetAluImmediateParams(src1->m_valueLow, immediate, shiftAmount))
	{
		m_assembler.Add(dstReg, g_baseRegister, CAArch32Assembler::MakeImmediateAluOperand(immediate, shiftAmount));
	}
	else
	{
		LoadConstantInRegister(dstReg, src1->m_valueLow);
		m_assembler.Add(dstReg, dstReg, g_baseRegister);
	}

	CommitSymbolRegisterRef(dst, dstReg);
}

void CCodeGen_AArch32::Emit_AddRef_VarVarAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDefRef(dst, CAArch32Assembler::r0);

	LoadRefIndexAddress(dstReg, src1, CAArch32Assembler::r1, src2, CAArch32Assembler::r2, 1);

	CommitSymbolRegisterRef(dst, dstReg);
}

void CCodeGen_AArch32::Emit_IsRefNull_VarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto addressReg = PrepareSymbolRegisterUseRef(src1, CAArch32Assembler::r0);
	auto dstReg = PrepareSymbolRegisterDef(dst, CAArch32Assembler::r1);

	m_assembler.Tst(addressReg, addressReg);
	Cmp_GetFlag(dstReg, Jitter::CONDITION_EQ);

	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_AArch32::Emit_LoadFromRef_VarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto addressReg = PrepareSymbolRegisterUseRef(src1, CAArch32Assembler::r0);
	auto dstReg = PrepareSymbolRegisterDef(dst, CAArch32Assembler::r1);

	m_assembler.Ldr(dstReg, addressReg, CAArch32Assembler::MakeImmediateLdrAddress(0));

	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_AArch32::Emit_LoadFromRef_VarVarAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	uint8 scale = static_cast<uint8>(statement.jmpCondition);

	assert((scale == 1) || (scale == 4));

	auto dstReg = PrepareSymbolRegisterDef(dst, CAArch32Assembler::r0);
	auto addressReg = PrepareSymbolRegisterUseRef(src1, CAArch32Assembler::r1);

	if(uint32 scaledIndex = (src2->m_valueLow * scale); src2->IsConstant() && (scaledIndex < 0x1000))
	{
		m_assembler.Ldr(dstReg, addressReg, CAArch32Assembler::MakeImmediateLdrAddress(scaledIndex));
	}
	else
	{
		auto indexReg = PrepareSymbolRegisterUse(src2, CAArch32Assembler::r2);
		m_assembler.Ldr(dstReg, addressReg, MakeScaledLdrAddress(indexReg, scale));
	}

	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_AArch32::Emit_LoadFromRef_Ref_VarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDefRef(dst, CAArch32Assembler::r0);
	auto src1Reg = PrepareSymbolRegisterUseRef(src1, CAArch32Assembler::r1);

	m_assembler.Ldr(dstReg, src1Reg, CAArch32Assembler::MakeImmediateLdrAddress(0));

	CommitSymbolRegisterRef(dst, dstReg);
}

void CCodeGen_AArch32::Emit_LoadFromRef_Ref_VarVarAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	uint8 scale = static_cast<uint8>(statement.jmpCondition);

	assert(scale == 4);

	auto dstReg = PrepareSymbolRegisterDefRef(dst, CAArch32Assembler::r0);
	auto src1Reg = PrepareSymbolRegisterUseRef(src1, CAArch32Assembler::r1);

	if(uint32 scaledIndex = (src2->m_valueLow * scale); src2->IsConstant() && (scaledIndex < 0x1000))
	{
		m_assembler.Ldr(dstReg, src1Reg, CAArch32Assembler::MakeImmediateLdrAddress(scaledIndex));
	}
	else
	{
		auto indexReg = PrepareSymbolRegisterUse(src2, CAArch32Assembler::r2);
		m_assembler.Ldr(dstReg, src1Reg, MakeScaledLdrAddress(indexReg, scale));
	}

	CommitSymbolRegisterRef(dst, dstReg);
}

void CCodeGen_AArch32::Emit_Load8FromRef_MemVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto addressReg = PrepareSymbolRegisterUseRef(src1, CAArch32Assembler::r0);
	auto dstReg = PrepareSymbolRegisterDef(dst, CAArch32Assembler::r1);

	m_assembler.Ldrb(dstReg, addressReg, CAArch32Assembler::MakeImmediateLdrAddress(0));

	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_AArch32::Emit_Load8FromRef_MemVarAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	uint8 scale = static_cast<uint8>(statement.jmpCondition);

	assert(scale == 1);

	auto dstReg = PrepareSymbolRegisterDef(dst, CAArch32Assembler::r0);
	auto addressReg = PrepareSymbolRegisterUseRef(src1, CAArch32Assembler::r1);

	if(uint32 scaledIndex = (src2->m_valueLow * scale); src2->IsConstant() && (scaledIndex < 0x1000))
	{
		m_assembler.Ldrb(dstReg, addressReg, CAArch32Assembler::MakeImmediateLdrAddress(scaledIndex));
	}
	else
	{
		auto indexReg = PrepareSymbolRegisterUse(src2, CAArch32Assembler::r2);
		m_assembler.Ldrb(dstReg, addressReg, MakeScaledLdrAddress(indexReg, scale));
	}

	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_AArch32::Emit_Load16FromRef_MemVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto addressReg = PrepareSymbolRegisterUseRef(src1, CAArch32Assembler::r0);
	auto dstReg = PrepareSymbolRegisterDef(dst, CAArch32Assembler::r1);

	m_assembler.Ldrh(dstReg, addressReg, CAArch32Assembler::MakeImmediateLdrAddress(0));

	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_AArch32::Emit_Load16FromRef_MemVarAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	uint8 scale = static_cast<uint8>(statement.jmpCondition);

	assert(scale == 1);

	auto dstReg = PrepareSymbolRegisterDef(dst, CAArch32Assembler::r0);
	auto addressReg = PrepareSymbolRegisterUseRef(src1, CAArch32Assembler::r1);

	if(uint32 scaledIndex = (src2->m_valueLow * scale); src2->IsConstant() && (scaledIndex < 0x1000))
	{
		m_assembler.Ldrh(dstReg, addressReg, CAArch32Assembler::MakeImmediateLdrAddress(scaledIndex));
	}
	else
	{
		auto indexReg = PrepareSymbolRegisterUse(src2, CAArch32Assembler::r2);
		m_assembler.Ldrh(dstReg, addressReg, MakeScaledLdrAddress(indexReg, scale));
	}

	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_AArch32::Emit_StoreAtRef_VarAny(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto addressReg = PrepareSymbolRegisterUseRef(src1, CAArch32Assembler::r0);
	auto valueReg = PrepareSymbolRegisterUse(src2, CAArch32Assembler::r1);

	m_assembler.Str(valueReg, addressReg, CAArch32Assembler::MakeImmediateLdrAddress(0));
}

void CCodeGen_AArch32::Emit_StoreAtRef_VarAnyAny(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	auto src3 = statement.src3->GetSymbol().get();
	uint8 scale = static_cast<uint8>(statement.jmpCondition);

	assert((scale == 1) || (scale == 4));

	auto addressReg = PrepareSymbolRegisterUseRef(src1, CAArch32Assembler::r0);
	auto valueReg = PrepareSymbolRegisterUse(src3, CAArch32Assembler::r2);

	if(uint32 scaledIndex = (src2->m_valueLow * scale); src2->IsConstant() && (scaledIndex < 0x1000))
	{
		m_assembler.Str(valueReg, addressReg, CAArch32Assembler::MakeImmediateLdrAddress(scaledIndex));
	}
	else
	{
		auto indexReg = PrepareSymbolRegisterUse(src2, CAArch32Assembler::r1);
		m_assembler.Str(valueReg, addressReg, MakeScaledLdrAddress(indexReg, scale));
	}
}

void CCodeGen_AArch32::Emit_Store8AtRef_VarAny(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto addressReg = PrepareSymbolRegisterUseRef(src1, CAArch32Assembler::r0);
	auto valueReg = PrepareSymbolRegisterUse(src2, CAArch32Assembler::r1);

	m_assembler.Strb(valueReg, addressReg, CAArch32Assembler::MakeImmediateLdrAddress(0));
}

void CCodeGen_AArch32::Emit_Store8AtRef_VarAnyAny(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	auto src3 = statement.src3->GetSymbol().get();
	uint8 scale = static_cast<uint8>(statement.jmpCondition);

	assert(scale == 1);

	auto addressReg = PrepareSymbolRegisterUseRef(src1, CAArch32Assembler::r0);
	auto valueReg = PrepareSymbolRegisterUse(src3, CAArch32Assembler::r2);

	if(uint32 scaledIndex = (src2->m_valueLow * scale); src2->IsConstant() && (scaledIndex < 0x1000))
	{
		m_assembler.Strb(valueReg, addressReg, CAArch32Assembler::MakeImmediateLdrAddress(scaledIndex));
	}
	else
	{
		auto indexReg = PrepareSymbolRegisterUse(src2, CAArch32Assembler::r1);
		m_assembler.Strb(valueReg, addressReg, MakeScaledLdrAddress(indexReg, scale));
	}
}

void CCodeGen_AArch32::Emit_Store16AtRef_VarAny(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto addressReg = PrepareSymbolRegisterUseRef(src1, CAArch32Assembler::r0);
	auto valueReg = PrepareSymbolRegisterUse(src2, CAArch32Assembler::r1);

	m_assembler.Strh(valueReg, addressReg, CAArch32Assembler::MakeImmediateLdrAddress(0));
}

void CCodeGen_AArch32::Emit_Store16AtRef_VarAnyAny(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	auto src3 = statement.src3->GetSymbol().get();
	uint8 scale = static_cast<uint8>(statement.jmpCondition);

	assert(scale == 1);

	auto addressReg = PrepareSymbolRegisterUseRef(src1, CAArch32Assembler::r0);
	auto valueReg = PrepareSymbolRegisterUse(src3, CAArch32Assembler::r2);

	if(uint32 scaledIndex = (src2->m_valueLow * scale); src2->IsConstant() && (scaledIndex < 0x1000))
	{
		m_assembler.Strh(valueReg, addressReg, CAArch32Assembler::MakeImmediateLdrAddress(scaledIndex));
	}
	else
	{
		auto indexReg = PrepareSymbolRegisterUse(src2, CAArch32Assembler::r1);
		m_assembler.Strh(valueReg, addressReg, MakeScaledLdrAddress(indexReg, scale));
	}
}
