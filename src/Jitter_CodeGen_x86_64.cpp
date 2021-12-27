#include <algorithm>
#include "Jitter_CodeGen_x86_64.h"
#include <stdexcept>

using namespace Jitter;

CX86Assembler::REGISTER CCodeGen_x86_64::g_systemVRegisters[SYSTEMV_MAX_REGISTERS] =
{
	CX86Assembler::rBX,
	CX86Assembler::r12,
	CX86Assembler::r13,
	CX86Assembler::r14,
	CX86Assembler::r15,
};

CX86Assembler::REGISTER CCodeGen_x86_64::g_systemVParamRegs[SYSTEMV_MAX_PARAMS] =
{
	CX86Assembler::rDI,
	CX86Assembler::rSI,
	CX86Assembler::rDX,
	CX86Assembler::rCX,
	CX86Assembler::r8,
	CX86Assembler::r9,
};

CX86Assembler::REGISTER CCodeGen_x86_64::g_win32Registers[WIN32_MAX_REGISTERS] =
{
	CX86Assembler::rBX,
	CX86Assembler::rSI,
	CX86Assembler::rDI,
	CX86Assembler::r12,
	CX86Assembler::r13,
	CX86Assembler::r14,
	CX86Assembler::r15,
};

CX86Assembler::REGISTER CCodeGen_x86_64::g_win32ParamRegs[WIN32_MAX_PARAMS] =
{
	CX86Assembler::rCX,
	CX86Assembler::rDX,
	CX86Assembler::r8,
	CX86Assembler::r9,
};

//xMM0->xMM3 are used internally for temporary uses
CX86Assembler::XMMREGISTER CCodeGen_x86_64::g_mdRegisters[MAX_MDREGISTERS] =
{
	CX86Assembler::xMM4,
	CX86Assembler::xMM5,
	CX86Assembler::xMM6,
	CX86Assembler::xMM7,
	CX86Assembler::xMM8,
	CX86Assembler::xMM9,
	CX86Assembler::xMM10,
	CX86Assembler::xMM11,
	CX86Assembler::xMM12,
	CX86Assembler::xMM13,
	CX86Assembler::xMM14,
	CX86Assembler::xMM15
};

static uint64 CombineConstant64(uint32 cstLow, uint32 cstHigh)
{
	return (static_cast<uint64>(cstHigh) << 32) | static_cast<uint64>(cstLow);
}

//ALUOP
//-------------------------------------------------------------------

template <typename ALUOP>
void CCodeGen_x86_64::Emit_Alu64_MemMemMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	CX86Assembler::REGISTER tmpReg = CX86Assembler::rAX;

	m_assembler.MovEq(tmpReg, MakeMemory64SymbolAddress(src1));
	((m_assembler).*(ALUOP::OpEq()))(tmpReg, MakeMemory64SymbolAddress(src2));
	m_assembler.MovGq(MakeMemory64SymbolAddress(dst), tmpReg);
}

template <typename ALUOP>
void CCodeGen_x86_64::Emit_Alu64_MemMemCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_CONSTANT64);

	CX86Assembler::REGISTER tmpReg = CX86Assembler::rAX;
	uint64 constant = CombineConstant64(src2->m_valueLow, src2->m_valueHigh);

	m_assembler.MovEq(tmpReg, MakeMemory64SymbolAddress(src1));
	if(CX86Assembler::GetMinimumConstantSize64(constant) >= 4)
	{
		auto cstReg = CX86Assembler::rCX;
		m_assembler.MovIq(cstReg, constant);
		((m_assembler).*(ALUOP::OpEq()))(tmpReg, CX86Assembler::MakeRegisterAddress(cstReg));
	}
	else
	{
		((m_assembler).*(ALUOP::OpIq()))(CX86Assembler::MakeRegisterAddress(tmpReg), constant);
	}
	m_assembler.MovGq(MakeMemory64SymbolAddress(dst), tmpReg);
}

template <typename ALUOP>
void CCodeGen_x86_64::Emit_Alu64_MemCstMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_CONSTANT64);

	CX86Assembler::REGISTER tmpReg = CX86Assembler::rAX;
	uint64 constant = CombineConstant64(src1->m_valueLow, src1->m_valueHigh);

	m_assembler.MovIq(tmpReg, constant);
	((m_assembler).*(ALUOP::OpEq()))(tmpReg, MakeMemory64SymbolAddress(src2));
	m_assembler.MovGq(MakeMemory64SymbolAddress(dst), tmpReg);
}

#define ALU64_CONST_MATCHERS(ALUOP_CST, ALUOP) \
	{ ALUOP_CST, MATCH_MEMORY64, MATCH_MEMORY64,   MATCH_MEMORY64,   MATCH_NIL, &CCodeGen_x86_64::Emit_Alu64_MemMemMem<ALUOP> }, \
	{ ALUOP_CST, MATCH_MEMORY64, MATCH_MEMORY64,   MATCH_CONSTANT64, MATCH_NIL, &CCodeGen_x86_64::Emit_Alu64_MemMemCst<ALUOP> }, \
	{ ALUOP_CST, MATCH_MEMORY64, MATCH_CONSTANT64, MATCH_MEMORY64,   MATCH_NIL, &CCodeGen_x86_64::Emit_Alu64_MemCstMem<ALUOP> },

//SHIFTOP
//-------------------------------------------------------------------

template <typename SHIFTOP>
void CCodeGen_x86_64::Emit_Shift64_RelRelReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_RELATIVE64);
	assert(src1->m_type == SYM_RELATIVE64);
	assert(src2->m_type == SYM_REGISTER);

	CX86Assembler::REGISTER tmpReg = CX86Assembler::rAX;
	CX86Assembler::REGISTER shiftReg = CX86Assembler::rCX;

	m_assembler.MovEq(tmpReg, MakeRelative64SymbolAddress(src1));
	m_assembler.MovEd(shiftReg, CX86Assembler::MakeRegisterAddress(m_registers[src2->m_valueLow]));
	((m_assembler).*(SHIFTOP::OpVar()))(CX86Assembler::MakeRegisterAddress(tmpReg));
	m_assembler.MovGq(MakeRelative64SymbolAddress(dst), tmpReg);
}

template <typename SHIFTOP>
void CCodeGen_x86_64::Emit_Shift64_RelRelMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_RELATIVE64);
	assert(src1->m_type == SYM_RELATIVE64);

	CX86Assembler::REGISTER tmpReg = CX86Assembler::rAX;
	CX86Assembler::REGISTER shiftReg = CX86Assembler::rCX;

	m_assembler.MovEq(tmpReg, MakeRelative64SymbolAddress(src1));
	m_assembler.MovEd(shiftReg, MakeMemorySymbolAddress(src2));
	((m_assembler).*(SHIFTOP::OpVar()))(CX86Assembler::MakeRegisterAddress(tmpReg));
	m_assembler.MovGq(MakeRelative64SymbolAddress(dst), tmpReg);
}

template <typename SHIFTOP>
void CCodeGen_x86_64::Emit_Shift64_RelRelCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_RELATIVE64);
	assert(src1->m_type == SYM_RELATIVE64);
	assert(src2->m_type == SYM_CONSTANT);

	CX86Assembler::REGISTER tmpReg = CX86Assembler::rAX;

	m_assembler.MovEq(tmpReg, MakeRelative64SymbolAddress(src1));
	((m_assembler).*(SHIFTOP::OpCst()))(CX86Assembler::MakeRegisterAddress(tmpReg), static_cast<uint8>(src2->m_valueLow));
	m_assembler.MovGq(MakeRelative64SymbolAddress(dst), tmpReg);
}

#define SHIFT64_CONST_MATCHERS(SHIFTOP_CST, SHIFTOP) \
	{ SHIFTOP_CST, MATCH_RELATIVE64, MATCH_RELATIVE64, MATCH_REGISTER, MATCH_NIL, &CCodeGen_x86_64::Emit_Shift64_RelRelReg<SHIFTOP> }, \
	{ SHIFTOP_CST, MATCH_RELATIVE64, MATCH_RELATIVE64, MATCH_MEMORY,   MATCH_NIL, &CCodeGen_x86_64::Emit_Shift64_RelRelMem<SHIFTOP> }, \
	{ SHIFTOP_CST, MATCH_RELATIVE64, MATCH_RELATIVE64, MATCH_CONSTANT, MATCH_NIL, &CCodeGen_x86_64::Emit_Shift64_RelRelCst<SHIFTOP> },

CCodeGen_x86_64::CONSTMATCHER CCodeGen_x86_64::g_constMatchers[] = 
{
	{ OP_PARAM, MATCH_NIL, MATCH_CONTEXT,     MATCH_NIL, MATCH_NIL, &CCodeGen_x86_64::Emit_Param_Ctx    },
	{ OP_PARAM, MATCH_NIL, MATCH_REGISTER,    MATCH_NIL, MATCH_NIL, &CCodeGen_x86_64::Emit_Param_Reg    },
	{ OP_PARAM, MATCH_NIL, MATCH_MEMORY,      MATCH_NIL, MATCH_NIL, &CCodeGen_x86_64::Emit_Param_Mem    },
	{ OP_PARAM, MATCH_NIL, MATCH_CONSTANT,    MATCH_NIL, MATCH_NIL, &CCodeGen_x86_64::Emit_Param_Cst    },
	{ OP_PARAM, MATCH_NIL, MATCH_MEMORY64,    MATCH_NIL, MATCH_NIL, &CCodeGen_x86_64::Emit_Param_Mem64  },
	{ OP_PARAM, MATCH_NIL, MATCH_CONSTANT64,  MATCH_NIL, MATCH_NIL, &CCodeGen_x86_64::Emit_Param_Cst64  },
	{ OP_PARAM, MATCH_NIL, MATCH_REGISTER128, MATCH_NIL, MATCH_NIL, &CCodeGen_x86_64::Emit_Param_Reg128 },
	{ OP_PARAM, MATCH_NIL, MATCH_MEMORY128,   MATCH_NIL, MATCH_NIL, &CCodeGen_x86_64::Emit_Param_Mem128 },

	{ OP_PARAM_RET, MATCH_NIL, MATCH_MEMORY128, MATCH_NIL, MATCH_NIL, &CCodeGen_x86_64::Emit_Param_Mem128 },

	{ OP_CALL, MATCH_NIL, MATCH_CONSTANTPTR, MATCH_CONSTANT, MATCH_NIL, &CCodeGen_x86_64::Emit_Call },
	{ OP_CALL, MATCH_NIL, MATCH_RELATIVE64,  MATCH_CONSTANT, MATCH_NIL, &CCodeGen_x86_64::Emit_Call_Rel64 },

	{ OP_RETVAL, MATCH_REGISTER,    MATCH_NIL, MATCH_NIL, MATCH_NIL, &CCodeGen_x86_64::Emit_RetVal_Reg    },
	{ OP_RETVAL, MATCH_MEMORY,      MATCH_NIL, MATCH_NIL, MATCH_NIL, &CCodeGen_x86_64::Emit_RetVal_Mem    },
	{ OP_RETVAL, MATCH_MEMORY64,    MATCH_NIL, MATCH_NIL, MATCH_NIL, &CCodeGen_x86_64::Emit_RetVal_Mem64  },
	{ OP_RETVAL, MATCH_REGISTER128, MATCH_NIL, MATCH_NIL, MATCH_NIL, &CCodeGen_x86_64::Emit_RetVal_Reg128 },
	{ OP_RETVAL, MATCH_MEMORY128,   MATCH_NIL, MATCH_NIL, MATCH_NIL, &CCodeGen_x86_64::Emit_RetVal_Mem128 },

	{ OP_EXTERNJMP,     MATCH_NIL, MATCH_CONSTANTPTR, MATCH_NIL, MATCH_NIL, &CCodeGen_x86_64::Emit_ExternJmp },
	{ OP_EXTERNJMP_DYN, MATCH_NIL, MATCH_CONSTANTPTR, MATCH_NIL, MATCH_NIL, &CCodeGen_x86_64::Emit_ExternJmp },

	{ OP_MOV, MATCH_MEMORY64,   MATCH_MEMORY64,   MATCH_NIL, MATCH_NIL, &CCodeGen_x86_64::Emit_Mov_Mem64Mem64 },
	{ OP_MOV, MATCH_RELATIVE64, MATCH_CONSTANT64, MATCH_NIL, MATCH_NIL, &CCodeGen_x86_64::Emit_Mov_Rel64Cst64 },

	{ OP_MOV, MATCH_REG_REF, MATCH_MEM_REF, MATCH_NIL, MATCH_NIL, &CCodeGen_x86_64::Emit_Mov_RegRefMemRef },

	ALU64_CONST_MATCHERS(OP_ADD64, ALUOP64_ADD)
	ALU64_CONST_MATCHERS(OP_SUB64, ALUOP64_SUB)
	ALU64_CONST_MATCHERS(OP_AND64, ALUOP64_AND)

	SHIFT64_CONST_MATCHERS(OP_SLL64, SHIFTOP64_SLL)
	SHIFT64_CONST_MATCHERS(OP_SRL64, SHIFTOP64_SRL)
	SHIFT64_CONST_MATCHERS(OP_SRA64, SHIFTOP64_SRA)

	{ OP_CMP, MATCH_VARIABLE, MATCH_VARIABLE, MATCH_VARIABLE, MATCH_NIL, &CCodeGen_x86_64::Emit_Cmp_VarVarVar },
	{ OP_CMP, MATCH_VARIABLE, MATCH_VARIABLE, MATCH_CONSTANT, MATCH_NIL, &CCodeGen_x86_64::Emit_Cmp_VarVarCst },

	{ OP_CMP64, MATCH_REGISTER, MATCH_RELATIVE64, MATCH_RELATIVE64, MATCH_NIL, &CCodeGen_x86_64::Emit_Cmp64_RegRelRel },
	{ OP_CMP64, MATCH_REGISTER, MATCH_RELATIVE64, MATCH_CONSTANT64, MATCH_NIL, &CCodeGen_x86_64::Emit_Cmp64_RegRelCst },
	{ OP_CMP64, MATCH_MEMORY,   MATCH_RELATIVE64, MATCH_RELATIVE64, MATCH_NIL, &CCodeGen_x86_64::Emit_Cmp64_MemRelRel },
	{ OP_CMP64, MATCH_MEMORY,   MATCH_RELATIVE64, MATCH_CONSTANT64, MATCH_NIL, &CCodeGen_x86_64::Emit_Cmp64_MemRelCst },

	{ OP_RELTOREF, MATCH_VAR_REF, MATCH_CONSTANT, MATCH_NIL, MATCH_NIL, &CCodeGen_x86_64::Emit_RelToRef_VarCst },

	{ OP_ADDREF, MATCH_VAR_REF, MATCH_VAR_REF, MATCH_VARIABLE, MATCH_NIL, &CCodeGen_x86_64::Emit_AddRef_VarVarVar },
	{ OP_ADDREF, MATCH_VAR_REF, MATCH_VAR_REF, MATCH_CONSTANT, MATCH_NIL, &CCodeGen_x86_64::Emit_AddRef_VarVarCst },

	{ OP_ISREFNULL, MATCH_VARIABLE, MATCH_VAR_REF, MATCH_NIL, MATCH_NIL, &CCodeGen_x86_64::Emit_IsRefNull_VarVar },

	{ OP_LOADFROMREF, MATCH_MEMORY64, MATCH_VAR_REF, MATCH_NIL, MATCH_NIL, &CCodeGen_x86_64::Emit_LoadFromRef_64_MemVar },
	{ OP_LOADFROMREF, MATCH_VAR_REF, MATCH_VAR_REF, MATCH_NIL, MATCH_NIL, &CCodeGen_x86_64::Emit_LoadFromRef_Ref_VarVar },

	{ OP_STOREATREF, MATCH_NIL, MATCH_VAR_REF, MATCH_MEMORY64, MATCH_NIL, &CCodeGen_x86_64::Emit_StoreAtRef_64_VarMem },
	{ OP_STOREATREF, MATCH_NIL, MATCH_VAR_REF, MATCH_CONSTANT64, MATCH_NIL, &CCodeGen_x86_64::Emit_StoreAtRef_64_VarCst },

	{ OP_STORE8ATREF, MATCH_NIL, MATCH_VAR_REF, MATCH_VARIABLE, MATCH_NIL, &CCodeGen_x86_64::Emit_Store8AtRef_VarVar },

	{ OP_CONDJMP, MATCH_NIL, MATCH_VAR_REF, MATCH_CONSTANT, MATCH_NIL, &CCodeGen_x86_64::Emit_CondJmp_Ref_VarCst },

	{ OP_MOV, MATCH_NIL, MATCH_NIL, MATCH_NIL, MATCH_NIL, nullptr },
};

CCodeGen_x86_64::CCodeGen_x86_64()
{
	SetPlatformAbi(PLATFORM_ABI_SYSTEMV);
	CCodeGen_x86::m_mdRegisters = g_mdRegisters;

	for(CONSTMATCHER* constMatcher = g_constMatchers; constMatcher->emitter != NULL; constMatcher++)
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

void CCodeGen_x86_64::SetPlatformAbi(PLATFORM_ABI platformAbi)
{
	m_platformAbi = platformAbi;
	switch(m_platformAbi)
	{
	case PLATFORM_ABI_SYSTEMV:
		CCodeGen_x86::m_registers    = g_systemVRegisters;
		m_paramRegs                  = g_systemVParamRegs;
		m_maxRegisters               = SYSTEMV_MAX_REGISTERS;
		m_maxParams                  = SYSTEMV_MAX_PARAMS;
		m_hasMdRegRetValues          = true;
		break;
	case PLATFORM_ABI_WIN32:
		CCodeGen_x86::m_registers    = g_win32Registers;
		m_paramRegs                  = g_win32ParamRegs;
		m_maxRegisters               = WIN32_MAX_REGISTERS;
		m_maxParams                  = WIN32_MAX_PARAMS;
		m_hasMdRegRetValues          = false;
		break;
	default:
		throw std::runtime_error("Unsupported ABI.");
		break;
	}
}

unsigned int CCodeGen_x86_64::GetAvailableRegisterCount() const
{
	return m_maxRegisters;
}

unsigned int CCodeGen_x86_64::GetAvailableMdRegisterCount() const
{
	return MAX_MDREGISTERS;
}

bool CCodeGen_x86_64::CanHold128BitsReturnValueInRegisters() const
{
	return m_hasMdRegRetValues;
}

uint32 CCodeGen_x86_64::GetPointerSize() const
{
	return 8;
}

void CCodeGen_x86_64::Emit_Prolog(const StatementList& statements, unsigned int stackSize)
{
	m_params.clear();
	//Compute the size needed to store all function call parameters
	uint32 maxParamSpillSize = 0;
	{
		uint32 currParamSpillSize = 0;
		for(const auto& statement : statements)
		{
			switch(statement.op)
			{
			case OP_PARAM:
			case OP_PARAM_RET:
				{
					CSymbol* src1 = statement.src1->GetSymbol().get();
					switch(src1->m_type)
					{
					case SYM_REGISTER128:
						currParamSpillSize += 16;
						break;
					}
				}
				break;
			case OP_CALL:
				maxParamSpillSize = std::max<uint32>(currParamSpillSize, maxParamSpillSize);
				currParamSpillSize = 0;
				break;
			default:
				break;
			}
		}
	}
	
	assert((maxParamSpillSize & 0x0F) == 0);
	assert((stackSize & 0x0F) == 0);

	m_assembler.Push(CX86Assembler::rBP);
	m_assembler.MovEq(CX86Assembler::rBP, CX86Assembler::MakeRegisterAddress(m_paramRegs[0]));

	uint32 savedSize = 0;
	if(m_isTrampoline)
	{
		for(unsigned int i = 0; i < m_maxRegisters; i++)
		{
			m_assembler.Push(m_registers[i]);
			savedSize += 8;
		}
	} 

	uint32 savedRegAlignAdjust = (savedSize != 0) ? (0x10 - (savedSize & 0xF)) : 0;

	m_totalStackAlloc = savedRegAlignAdjust + maxParamSpillSize + stackSize;
	m_totalStackAlloc += 0x20;

	m_stackLevel = 0x20;
	m_paramSpillBase = 0x20 + stackSize;

	m_assembler.SubIq(CX86Assembler::MakeRegisterAddress(CX86Assembler::rSP), m_totalStackAlloc);

	//-------------------------------
	//Stack Frame
	//-------------------------------
	//(High address)
	//------------------
	//Saved registers + align adjustment
	//------------------
	//Params spill space
	//------------------			<----- rSP + m_paramSpillBase
	//Temporary symbols (stackSize) + align adjustment
	//------------------			<----- rSP + m_stackLevel
	//Param spill area for callee (0x20 bytes)
	//------------------			<----- rSP
	//(Low address)
}

void CCodeGen_x86_64::Emit_Epilog()
{
	m_assembler.AddIq(CX86Assembler::MakeRegisterAddress(CX86Assembler::rSP), m_totalStackAlloc);

	if(m_isTrampoline)
	{
		for(int i = m_maxRegisters - 1; i >= 0; i--)
		{
			m_assembler.Pop(m_registers[i]);
		}
	}

	m_assembler.Pop(CX86Assembler::rBP);
}

CX86Assembler::CAddress CCodeGen_x86_64::MakeConstant128Address(const LITERAL128& constant)
{
	auto literalId = m_assembler.CreateLiteral128(constant);
	return CX86Assembler::MakeLiteral128Address(literalId);
}

void CCodeGen_x86_64::Emit_Param_Ctx(const STATEMENT& statement)
{
	assert(m_params.size() < m_maxParams);

	m_params.push_back(
		[this] (CX86Assembler::REGISTER paramReg, uint32)
		{
			m_assembler.MovEq(paramReg, CX86Assembler::MakeRegisterAddress(CX86Assembler::rBP));
			return 0;
		}
	);
}

void CCodeGen_x86_64::Emit_Param_Reg(const STATEMENT& statement)
{
	assert(m_params.size() < m_maxParams);

	auto src1 = statement.src1->GetSymbol().get();

	m_params.push_back(
		[this, src1] (CX86Assembler::REGISTER paramReg, uint32)
		{
			m_assembler.MovEd(paramReg, CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
			return 0;
		}
	);
}

void CCodeGen_x86_64::Emit_Param_Mem(const STATEMENT& statement)
{
	assert(m_params.size() < m_maxParams);

	auto src1 = statement.src1->GetSymbol().get();

	m_params.push_back(
		[this, src1] (CX86Assembler::REGISTER paramReg, uint32)
		{
			m_assembler.MovEd(paramReg, MakeMemorySymbolAddress(src1));
			return 0;
		}
	);
}

void CCodeGen_x86_64::Emit_Param_Cst(const STATEMENT& statement)
{
	assert(m_params.size() < m_maxParams);

	auto src1 = statement.src1->GetSymbol().get();

	m_params.push_back(
		[this, src1] (CX86Assembler::REGISTER paramReg, uint32)
		{
			m_assembler.MovId(paramReg, src1->m_valueLow);
			return 0;
		}
	);
}

void CCodeGen_x86_64::Emit_Param_Mem64(const STATEMENT& statement)
{
	assert(m_params.size() < m_maxParams);

	auto src1 = statement.src1->GetSymbol().get();

	m_params.push_back(
		[this, src1] (CX86Assembler::REGISTER paramReg, uint32)
		{
			m_assembler.MovEq(paramReg, MakeMemory64SymbolAddress(src1));
			return 0;
		}
	);
}

void CCodeGen_x86_64::Emit_Param_Cst64(const STATEMENT& statement)
{
	assert(m_params.size() < m_maxParams);

	auto src1 = statement.src1->GetSymbol().get();

	m_params.push_back(
		[this, src1] (CX86Assembler::REGISTER paramReg, uint32)
		{
			m_assembler.MovIq(paramReg, CombineConstant64(src1->m_valueLow, src1->m_valueHigh));
			return 0;
		}
	);
}

void CCodeGen_x86_64::Emit_Param_Reg128(const STATEMENT& statement)
{
	assert(m_params.size() < m_maxParams);

	auto src1 = statement.src1->GetSymbol().get();

	m_params.push_back(
		[this, src1] (CX86Assembler::REGISTER paramReg, uint32 paramSpillOffset)
		{
			auto paramTempAddr = CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, m_paramSpillBase + paramSpillOffset);
			m_assembler.MovapsVo(paramTempAddr, m_mdRegisters[src1->m_valueLow]);
			m_assembler.LeaGq(paramReg, paramTempAddr);
			return 0x10;
		}
	);
}

void CCodeGen_x86_64::Emit_Param_Mem128(const STATEMENT& statement)
{
	assert(m_params.size() < m_maxParams);

	auto src1 = statement.src1->GetSymbol().get();

	m_params.push_back(
		[this, src1] (CX86Assembler::REGISTER paramReg, uint32)
		{
			m_assembler.LeaGq(paramReg, MakeMemory128SymbolAddress(src1));
			return 0;
		}
	);
}

void CCodeGen_x86_64::Emit_Call_Rel64(const STATEMENT& statement)
{
	const auto& src1 = statement.src1->GetSymbol().get();
	const auto& src2 = statement.src2->GetSymbol().get();

	unsigned int paramCount = src2->m_valueLow;
	uint32 paramSpillOffset = 0;

	for(unsigned int i = 0; i < paramCount; i++)
	{
		auto emitter(m_params.back());
		m_params.pop_back();
		paramSpillOffset += emitter(m_paramRegs[i], paramSpillOffset);
	}

	m_assembler.MovEq(CX86Assembler::rAX, MakeMemory64SymbolAddress(src1));
	m_assembler.CallEd(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX));
}

void CCodeGen_x86_64::Emit_Call(const STATEMENT& statement)
{
	const auto& src1 = statement.src1->GetSymbol().get();
	const auto& src2 = statement.src2->GetSymbol().get();

	unsigned int paramCount = src2->m_valueLow;
	uint32 paramSpillOffset = 0;

	for(unsigned int i = 0; i < paramCount; i++)
	{
		auto emitter(m_params.back());
		m_params.pop_back();
		paramSpillOffset += emitter(m_paramRegs[i], paramSpillOffset);
	}

	m_assembler.MovIq(CX86Assembler::rAX, CombineConstant64(src1->m_valueLow, src1->m_valueHigh));
	auto symbolRefLabel = m_assembler.CreateLabel();
	m_assembler.MarkLabel(symbolRefLabel, -8);
	m_symbolReferenceLabels.push_back(std::make_pair(src1->GetConstantPtr(), symbolRefLabel));
	m_assembler.CallEd(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX));
}

void CCodeGen_x86_64::Emit_RetVal_Reg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();

	assert(dst->m_type == SYM_REGISTER);

	m_assembler.MovGd(CX86Assembler::MakeRegisterAddress(m_registers[dst->m_valueLow]), CX86Assembler::rAX);
}

void CCodeGen_x86_64::Emit_RetVal_Mem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	m_assembler.MovGd(MakeMemorySymbolAddress(dst), CX86Assembler::rAX);
}

void CCodeGen_x86_64::Emit_RetVal_Mem64(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	m_assembler.MovGq(MakeMemory64SymbolAddress(dst), CX86Assembler::rAX);
}

void CCodeGen_x86_64::Emit_RetVal_Reg128(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	
	//TODO: Use only integer operations
	m_assembler.MovqVo(m_mdRegisters[dst->m_valueLow], CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX));
	m_assembler.MovqVo(CX86Assembler::xMM0, CX86Assembler::MakeRegisterAddress(CX86Assembler::rDX));
	m_assembler.ShufpsVo(m_mdRegisters[dst->m_valueLow], CX86Assembler::MakeXmmRegisterAddress(CX86Assembler::xMM0), 0x44);
}

void CCodeGen_x86_64::Emit_RetVal_Mem128(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	m_assembler.MovGq(MakeMemory128SymbolElementAddress(dst, 0), CX86Assembler::rAX);
	m_assembler.MovGq(MakeMemory128SymbolElementAddress(dst, 2), CX86Assembler::rDX);
}

void CCodeGen_x86_64::Emit_ExternJmp(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	
	m_assembler.MovEq(m_paramRegs[0], CX86Assembler::MakeRegisterAddress(g_baseRegister));
	Emit_Epilog();
	m_assembler.MovIq(CX86Assembler::rAX, CombineConstant64(src1->m_valueLow, src1->m_valueHigh));
	auto symbolRefLabel = m_assembler.CreateLabel();
	m_assembler.MarkLabel(symbolRefLabel, -8);
	m_symbolReferenceLabels.push_back(std::make_pair(src1->GetConstantPtr(), symbolRefLabel));
	m_assembler.JmpEd(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX));
}

void CCodeGen_x86_64::Emit_Mov_Mem64Mem64(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	m_assembler.MovEq(CX86Assembler::rAX, MakeMemory64SymbolAddress(src1));
	m_assembler.MovGq(MakeMemory64SymbolAddress(dst), CX86Assembler::rAX);
}

void CCodeGen_x86_64::Emit_Mov_Rel64Cst64(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type  == SYM_RELATIVE64);
	assert(src1->m_type == SYM_CONSTANT64);

	uint64 constant = CombineConstant64(src1->m_valueLow, src1->m_valueHigh);
	CX86Assembler::REGISTER tmpReg = CX86Assembler::rAX;

	if(constant == 0)
	{
		m_assembler.XorGq(CX86Assembler::MakeRegisterAddress(tmpReg), tmpReg);
	}
	else
	{
		m_assembler.MovIq(tmpReg, constant);
	}
	m_assembler.MovGq(MakeRelative64SymbolAddress(dst), tmpReg);
}

void CCodeGen_x86_64::Emit_Mov_RegRefMemRef(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type == SYM_REG_REFERENCE);

	m_assembler.MovEq(m_registers[dst->m_valueLow], MakeMemoryReferenceSymbolAddress(src1));
}

void CCodeGen_x86_64::Emit_Cmp_VarVarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDef(dst, CX86Assembler::rCX);
	auto src1Reg = PrepareSymbolRegisterUse(src1, CX86Assembler::rDX);

	m_assembler.CmpEd(src1Reg, MakeVariableSymbolAddress(src2));
	Cmp_GetFlag(CX86Assembler::MakeRegisterAddress(dstReg), statement.jmpCondition);
	m_assembler.MovzxEb(dstReg, CX86Assembler::MakeRegisterAddress(dstReg));

	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_x86_64::Emit_Cmp_VarVarCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDef(dst, CX86Assembler::rCX);

	m_assembler.CmpId(MakeVariableSymbolAddress(src1), src2->m_valueLow);
	Cmp_GetFlag(CX86Assembler::MakeRegisterAddress(dstReg), statement.jmpCondition);
	m_assembler.MovzxEb(dstReg, CX86Assembler::MakeRegisterAddress(dstReg));

	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_x86_64::Cmp64_RelRel(CX86Assembler::REGISTER dstReg, const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_RELATIVE64);
	assert(src2->m_type == SYM_RELATIVE64);

	m_assembler.XorEd(dstReg, CX86Assembler::MakeRegisterAddress(dstReg));

	auto tmpReg = CX86Assembler::rCX;
	m_assembler.MovEq(tmpReg, MakeRelative64SymbolAddress(src1));
	m_assembler.CmpEq(tmpReg, MakeRelative64SymbolAddress(src2));

	Cmp_GetFlag(CX86Assembler::MakeRegisterAddress(dstReg), statement.jmpCondition);
}

void CCodeGen_x86_64::Cmp64_RelCst(CX86Assembler::REGISTER dstReg, const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_RELATIVE64);
	assert(src2->m_type == SYM_CONSTANT64);

	uint64 constant = CombineConstant64(src2->m_valueLow, src2->m_valueHigh);

	m_assembler.XorEd(dstReg, CX86Assembler::MakeRegisterAddress(dstReg));

	auto tmpReg = CX86Assembler::rCX;
	m_assembler.MovEq(tmpReg, MakeRelative64SymbolAddress(src1));
	if(constant == 0)
	{
		auto cstReg = CX86Assembler::rDX;
		m_assembler.XorGq(CX86Assembler::MakeRegisterAddress(cstReg), cstReg);
		m_assembler.CmpEq(tmpReg, CX86Assembler::MakeRegisterAddress(cstReg));
	}
	else if(CX86Assembler::GetMinimumConstantSize64(constant) == 8)
	{
		auto cstReg = CX86Assembler::rDX;
		m_assembler.MovIq(cstReg, constant);
		m_assembler.CmpEq(tmpReg, CX86Assembler::MakeRegisterAddress(cstReg));
	}
	else
	{
		m_assembler.CmpIq(CX86Assembler::MakeRegisterAddress(tmpReg), constant);
	}

	Cmp_GetFlag(CX86Assembler::MakeRegisterAddress(dstReg), statement.jmpCondition);
}

void CCodeGen_x86_64::Emit_Cmp64_RegRelRel(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	assert(dst->m_type == SYM_REGISTER);
	Cmp64_RelRel(m_registers[dst->m_valueLow], statement);
}

void CCodeGen_x86_64::Emit_Cmp64_RegRelCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	assert(dst->m_type == SYM_REGISTER);
	Cmp64_RelCst(m_registers[dst->m_valueLow], statement);
}

void CCodeGen_x86_64::Emit_Cmp64_MemRelRel(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CX86Assembler::REGISTER tmpReg = CX86Assembler::rAX;
	Cmp64_RelRel(tmpReg, statement);
	m_assembler.MovGd(MakeMemorySymbolAddress(dst), tmpReg);
}

void CCodeGen_x86_64::Emit_Cmp64_MemRelCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CX86Assembler::REGISTER tmpReg = CX86Assembler::rAX;
	Cmp64_RelCst(tmpReg, statement);
	m_assembler.MovGd(MakeMemorySymbolAddress(dst), tmpReg);
}

void CCodeGen_x86_64::Emit_RelToRef_VarCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(src1->m_type == SYM_CONSTANT);

	auto dstReg = PrepareRefSymbolRegisterDef(dst, CX86Assembler::rAX);
	m_assembler.LeaGq(dstReg, CX86Assembler::MakeIndRegOffAddress(g_baseRegister, src1->m_valueLow));
	CommitRefSymbolRegister(dst, dstReg);
}

void CCodeGen_x86_64::Emit_AddRef_VarVarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	//TODO: Sign extend this, or we might get in trouble
	auto offsetReg = PrepareSymbolRegisterUse(src2, CX86Assembler::rCX);
	auto dstReg = PrepareRefSymbolRegisterDef(dst, CX86Assembler::rAX);

	if(!dst->Equals(src1))
	{
		m_assembler.MovEq(dstReg, MakeVariableReferenceSymbolAddress(src1));
	}

	m_assembler.AddEq(dstReg, CX86Assembler::MakeRegisterAddress(offsetReg));
	CommitRefSymbolRegister(dst, dstReg);
}

void CCodeGen_x86_64::Emit_AddRef_VarVarCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_CONSTANT);

	auto dstReg = PrepareRefSymbolRegisterDef(dst, CX86Assembler::rAX);

	if(!dst->Equals(src1))
	{
		m_assembler.MovEq(dstReg, MakeVariableReferenceSymbolAddress(src1));
	}

	m_assembler.AddIq(CX86Assembler::MakeRegisterAddress(dstReg), src2->m_valueLow);
	CommitRefSymbolRegister(dst, dstReg);
}

void CCodeGen_x86_64::Emit_IsRefNull_VarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto addressReg = PrepareRefSymbolRegisterUse(src1, CX86Assembler::rAX);
	auto dstReg = PrepareSymbolRegisterDef(dst, CX86Assembler::rDX);

	m_assembler.XorEd(dstReg, CX86Assembler::MakeRegisterAddress(dstReg));
	m_assembler.TestEq(addressReg, CX86Assembler::MakeRegisterAddress(addressReg));
	m_assembler.SeteEb(CX86Assembler::MakeRegisterAddress(dstReg));

	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_x86_64::Emit_LoadFromRef_64_MemVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto addressReg = PrepareRefSymbolRegisterUse(src1, CX86Assembler::rAX);
	auto dstReg = CX86Assembler::rCX;

	m_assembler.MovEq(dstReg, CX86Assembler::MakeIndRegAddress(addressReg));
	m_assembler.MovGq(MakeMemory64SymbolAddress(dst), dstReg);
}

void CCodeGen_x86_64::Emit_LoadFromRef_Ref_VarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto addressReg = PrepareRefSymbolRegisterUse(src1, CX86Assembler::rAX);
	auto dstReg = PrepareRefSymbolRegisterDef(dst, CX86Assembler::rDX);

	m_assembler.MovEq(dstReg, CX86Assembler::MakeIndRegAddress(addressReg));

	CommitRefSymbolRegister(dst, dstReg);
}

void CCodeGen_x86_64::Emit_StoreAtRef_64_VarMem(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto addressReg = PrepareRefSymbolRegisterUse(src1, CX86Assembler::rAX);
	auto valueReg = CX86Assembler::rDX;

	m_assembler.MovEq(valueReg, MakeMemory64SymbolAddress(src2));
	m_assembler.MovGq(CX86Assembler::MakeIndRegAddress(addressReg), valueReg);
}

void CCodeGen_x86_64::Emit_StoreAtRef_64_VarCst(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto addressReg = PrepareRefSymbolRegisterUse(src1, CX86Assembler::rAX);
	auto valueReg = CX86Assembler::rDX;

	m_assembler.MovIq(valueReg, src2->GetConstant64());
	m_assembler.MovGq(CX86Assembler::MakeIndRegAddress(addressReg), valueReg);
}

void CCodeGen_x86_64::Emit_Store8AtRef_VarVar(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto addressReg = PrepareRefSymbolRegisterUse(src1, CX86Assembler::rAX);
	auto valueReg = PrepareSymbolRegisterUse(src2, CX86Assembler::rDX);
	m_assembler.MovGb(CX86Assembler::MakeIndRegAddress(addressReg), valueReg);
}

void CCodeGen_x86_64::Emit_CondJmp_Ref_VarCst(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_CONSTANT);
	assert(src2->m_valueLow == 0);
	assert((statement.jmpCondition == CONDITION_NE) || (statement.jmpCondition == CONDITION_EQ));

	auto addressReg = PrepareRefSymbolRegisterUse(src1, CX86Assembler::rAX);

	m_assembler.TestEq(addressReg, CX86Assembler::MakeRegisterAddress(addressReg));

	CondJmp_JumpTo(GetLabel(statement.jmpBlock), statement.jmpCondition);
}

CX86Assembler::REGISTER CCodeGen_x86_64::PrepareRefSymbolRegisterDef(CSymbol* symbol, CX86Assembler::REGISTER preferedRegister)
{
	switch(symbol->m_type)
	{
	case SYM_REG_REFERENCE:
		return m_registers[symbol->m_valueLow];
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

CX86Assembler::REGISTER CCodeGen_x86_64::PrepareRefSymbolRegisterUse(CSymbol* symbol, CX86Assembler::REGISTER preferedRegister)
{
	switch(symbol->m_type)
	{
	case SYM_REG_REFERENCE:
		return m_registers[symbol->m_valueLow];
		break;
	case SYM_TMP_REFERENCE:
	case SYM_REL_REFERENCE:
		m_assembler.MovEq(preferedRegister, MakeMemoryReferenceSymbolAddress(symbol));
		return preferedRegister;
		break;
	default:
		throw std::runtime_error("Invalid symbol type.");
		break;
	}
}

void CCodeGen_x86_64::CommitRefSymbolRegister(CSymbol* symbol, CX86Assembler::REGISTER usedRegister)
{
	switch(symbol->m_type)
	{
	case SYM_REG_REFERENCE:
		assert(usedRegister == m_registers[symbol->m_valueLow]);
		break;
	case SYM_TMP_REFERENCE:
	case SYM_REL_REFERENCE:
		m_assembler.MovGq(MakeMemoryReferenceSymbolAddress(symbol), usedRegister);
		break;
	default:
		throw std::runtime_error("Invalid symbol type.");
		break;
	}
}
