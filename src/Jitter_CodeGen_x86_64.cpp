#include "Jitter_CodeGen_x86_64.h"

using namespace Jitter;

CX86Assembler::REGISTER CCodeGen_x86_64::g_registers[MAX_REGISTERS] =
{
	CX86Assembler::rBX,
	CX86Assembler::rSI,
	CX86Assembler::rDI,
	CX86Assembler::r12,
	CX86Assembler::r13,
	CX86Assembler::r14,
	CX86Assembler::r15,
};

CX86Assembler::REGISTER CCodeGen_x86_64::g_paramRegs[MAX_PARAMS] =
{
	CX86Assembler::rCX,
	CX86Assembler::rDX,
	CX86Assembler::r8,
	CX86Assembler::r9,
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
	((m_assembler).*(ALUOP::OpIq()))(CX86Assembler::MakeRegisterAddress(tmpReg), constant);
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
	{ ALUOP_CST,	MATCH_MEMORY64,		MATCH_MEMORY64,		MATCH_MEMORY64,		&CCodeGen_x86_64::Emit_Alu64_MemMemMem<ALUOP>	}, \
	{ ALUOP_CST,	MATCH_MEMORY64,		MATCH_MEMORY64,		MATCH_CONSTANT64,	&CCodeGen_x86_64::Emit_Alu64_MemMemCst<ALUOP>	}, \
	{ ALUOP_CST,	MATCH_MEMORY64,		MATCH_CONSTANT64,	MATCH_MEMORY64,		&CCodeGen_x86_64::Emit_Alu64_MemCstMem<ALUOP>	},

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
	{ SHIFTOP_CST,	MATCH_RELATIVE64,	MATCH_RELATIVE64,	MATCH_REGISTER,		&CCodeGen_x86_64::Emit_Shift64_RelRelReg<SHIFTOP>		}, \
	{ SHIFTOP_CST,	MATCH_RELATIVE64,	MATCH_RELATIVE64,	MATCH_MEMORY,		&CCodeGen_x86_64::Emit_Shift64_RelRelMem<SHIFTOP>		}, \
	{ SHIFTOP_CST,	MATCH_RELATIVE64,	MATCH_RELATIVE64,	MATCH_CONSTANT,		&CCodeGen_x86_64::Emit_Shift64_RelRelCst<SHIFTOP>		},

CCodeGen_x86_64::CONSTMATCHER CCodeGen_x86_64::g_constMatchers[] = 
{
	{ OP_PARAM,			MATCH_NIL,			MATCH_CONTEXT,		MATCH_NIL,			&CCodeGen_x86_64::Emit_Param_Ctx							},
	{ OP_PARAM,			MATCH_NIL,			MATCH_REGISTER,		MATCH_NIL,			&CCodeGen_x86_64::Emit_Param_Reg							},
	{ OP_PARAM,			MATCH_NIL,			MATCH_MEMORY,		MATCH_NIL,			&CCodeGen_x86_64::Emit_Param_Mem							},
	{ OP_PARAM,			MATCH_NIL,			MATCH_CONSTANT,		MATCH_NIL,			&CCodeGen_x86_64::Emit_Param_Cst							},
	{ OP_PARAM,			MATCH_NIL,			MATCH_MEMORY64,		MATCH_NIL,			&CCodeGen_x86_64::Emit_Param_Mem64							},
	{ OP_PARAM,			MATCH_NIL,			MATCH_CONSTANT64,	MATCH_NIL,			&CCodeGen_x86_64::Emit_Param_Cst64							},
	{ OP_PARAM,			MATCH_NIL,			MATCH_MEMORY128,	MATCH_NIL,			&CCodeGen_x86_64::Emit_Param_Mem128							},

	{ OP_PARAM_RET,		MATCH_NIL,			MATCH_MEMORY128,	MATCH_NIL,			&CCodeGen_x86_64::Emit_Param_Mem128							},

	{ OP_CALL,			MATCH_NIL,			MATCH_CONSTANT64,	MATCH_CONSTANT,		&CCodeGen_x86_64::Emit_Call									},

	{ OP_RETVAL,		MATCH_REGISTER,		MATCH_NIL,			MATCH_NIL,			&CCodeGen_x86_64::Emit_RetVal_Reg							},
	{ OP_RETVAL,		MATCH_MEMORY,		MATCH_NIL,			MATCH_NIL,			&CCodeGen_x86_64::Emit_RetVal_Mem							},
	{ OP_RETVAL,		MATCH_MEMORY64,		MATCH_NIL,			MATCH_NIL,			&CCodeGen_x86_64::Emit_RetVal_Mem64							},

	{ OP_MOV,			MATCH_MEMORY64,		MATCH_MEMORY64,		MATCH_NIL,			&CCodeGen_x86_64::Emit_Mov_Mem64Mem64						},
	{ OP_MOV,			MATCH_RELATIVE64,	MATCH_CONSTANT64,	MATCH_NIL,			&CCodeGen_x86_64::Emit_Mov_Rel64Cst64						},

	ALU64_CONST_MATCHERS(OP_ADD64, ALUOP64_ADD)
	ALU64_CONST_MATCHERS(OP_SUB64, ALUOP64_SUB)
	ALU64_CONST_MATCHERS(OP_AND64, ALUOP64_AND)

	SHIFT64_CONST_MATCHERS(OP_SLL64, SHIFTOP64_SLL)
	SHIFT64_CONST_MATCHERS(OP_SRL64, SHIFTOP64_SRL)
	SHIFT64_CONST_MATCHERS(OP_SRA64, SHIFTOP64_SRA)

	{ OP_CMP64,			MATCH_REGISTER,		MATCH_RELATIVE64,	MATCH_RELATIVE64,	&CCodeGen_x86_64::Emit_Cmp64_RegRelRel						},
	{ OP_CMP64,			MATCH_REGISTER,		MATCH_RELATIVE64,	MATCH_CONSTANT64,	&CCodeGen_x86_64::Emit_Cmp64_RegRelCst						},
	{ OP_CMP64,			MATCH_MEMORY,		MATCH_RELATIVE64,	MATCH_RELATIVE64,	&CCodeGen_x86_64::Emit_Cmp64_MemRelRel						},
	{ OP_CMP64,			MATCH_MEMORY,		MATCH_RELATIVE64,	MATCH_CONSTANT64,	&CCodeGen_x86_64::Emit_Cmp64_MemRelCst						},

	{ OP_RELTOREF,		MATCH_TMP_REF,		MATCH_CONSTANT,		MATCH_NIL,			&CCodeGen_x86_64::Emit_RelToRef_TmpCst						},

	{ OP_ADDREF,		MATCH_MEM_REF,		MATCH_MEM_REF,		MATCH_REGISTER,		&CCodeGen_x86_64::Emit_AddRef_MemMemReg						},
	{ OP_ADDREF,		MATCH_MEM_REF,		MATCH_MEM_REF,		MATCH_CONSTANT,		&CCodeGen_x86_64::Emit_AddRef_MemMemCst						},

	{ OP_LOADFROMREF,	MATCH_REGISTER,		MATCH_TMP_REF,		MATCH_NIL,			&CCodeGen_x86_64::Emit_LoadFromRef_RegTmp					},

	{ OP_STOREATREF,	MATCH_NIL,			MATCH_MEM_REF,		MATCH_REGISTER,		&CCodeGen_x86_64::Emit_StoreAtRef_MemReg					},
	{ OP_STOREATREF,	MATCH_NIL,			MATCH_MEM_REF,		MATCH_CONSTANT,		&CCodeGen_x86_64::Emit_StoreAtRef_MemCst					},

	{ OP_MOV,			MATCH_NIL,			MATCH_NIL,			MATCH_NIL,			NULL														},
};

CCodeGen_x86_64::CCodeGen_x86_64()
{
	CCodeGen_x86::m_registers = g_registers;

	for(CONSTMATCHER* constMatcher = g_constMatchers; constMatcher->emitter != NULL; constMatcher++)
	{
		MATCHER matcher;
		matcher.op			= constMatcher->op;
		matcher.dstType		= constMatcher->dstType;
		matcher.src1Type	= constMatcher->src1Type;
		matcher.src2Type	= constMatcher->src2Type;
		matcher.emitter		= std::bind(constMatcher->emitter, this, std::placeholders::_1);
		m_matchers.insert(MatcherMapType::value_type(matcher.op, matcher));
	}
}

CCodeGen_x86_64::~CCodeGen_x86_64()
{

}

unsigned int CCodeGen_x86_64::GetAvailableRegisterCount() const
{
	return MAX_REGISTERS;
}

void CCodeGen_x86_64::Emit_Prolog(const StatementList&, unsigned int stackSize, uint32 registerUsage)
{
	m_params.clear();

	m_assembler.Push(CX86Assembler::rBP);
	m_assembler.MovEq(CX86Assembler::rBP, CX86Assembler::MakeRegisterAddress(CX86Assembler::rDX));

	uint32 savedCount = 0;
	for(unsigned int i = 0; i < MAX_REGISTERS; i++)
	{
		if(registerUsage & (1 << i))
		{
			m_assembler.Push(m_registers[i]);
			savedCount += 8;
		}
	}

	savedCount = 0x10 - (savedCount & 0xF);

	m_totalStackAlloc = savedCount + ((stackSize + 0xF) & ~0xF);
	m_totalStackAlloc += 0x20;

	m_stackLevel = 0x20;

	m_assembler.SubIq(CX86Assembler::MakeRegisterAddress(CX86Assembler::rSP), m_totalStackAlloc);
}

void CCodeGen_x86_64::Emit_Epilog(unsigned int stackSize, uint32 registerUsage)
{
	m_assembler.AddIq(CX86Assembler::MakeRegisterAddress(CX86Assembler::rSP), m_totalStackAlloc);

	for(int i = MAX_REGISTERS - 1; i >= 0; i--)
	{
		if(registerUsage & (1 << i))
		{
			m_assembler.Pop(m_registers[i]);
		}
	}

	m_assembler.Pop(CX86Assembler::rBP);
	m_assembler.Ret();
}

void CCodeGen_x86_64::Emit_Param_Ctx(const STATEMENT& statement)
{
	assert(m_params.size() < MAX_PARAMS);

	m_params.push_back(std::bind(
		&CX86Assembler::MovEq, &m_assembler, std::placeholders::_1, CX86Assembler::MakeRegisterAddress(CX86Assembler::rBP)));
}

void CCodeGen_x86_64::Emit_Param_Reg(const STATEMENT& statement)
{
	assert(m_params.size() < MAX_PARAMS);

	CSymbol* src1 = statement.src1->GetSymbol().get();

	m_params.push_back(std::bind(
		&CX86Assembler::MovEd, &m_assembler, std::placeholders::_1, CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow])));
}

void CCodeGen_x86_64::Emit_Param_Mem(const STATEMENT& statement)
{
	assert(m_params.size() < MAX_PARAMS);

	CSymbol* src1 = statement.src1->GetSymbol().get();

	m_params.push_back(std::bind(&CX86Assembler::MovEd, &m_assembler, std::placeholders::_1, MakeMemorySymbolAddress(src1)));
}

void CCodeGen_x86_64::Emit_Param_Cst(const STATEMENT& statement)
{
	assert(m_params.size() < MAX_PARAMS);

	CSymbol* src1 = statement.src1->GetSymbol().get();

	void (CX86Assembler::*MovFunction)(CX86Assembler::REGISTER, uint32) = &CX86Assembler::MovId;
	m_params.push_back(std::bind(MovFunction, &m_assembler, std::placeholders::_1, src1->m_valueLow));
}

void CCodeGen_x86_64::Emit_Param_Mem64(const STATEMENT& statement)
{
	assert(m_params.size() < MAX_PARAMS);

	CSymbol* src1 = statement.src1->GetSymbol().get();

	m_params.push_back(std::bind(
		&CX86Assembler::MovEq, &m_assembler, std::placeholders::_1, MakeMemory64SymbolAddress(src1)));
}

void CCodeGen_x86_64::Emit_Param_Cst64(const STATEMENT& statement)
{
	assert(m_params.size() < MAX_PARAMS);

	CSymbol* src1 = statement.src1->GetSymbol().get();

	m_params.push_back(std::bind(
		&CX86Assembler::MovIq, &m_assembler, std::placeholders::_1, CombineConstant64(src1->m_valueLow, src1->m_valueHigh)));
}

void CCodeGen_x86_64::Emit_Param_Mem128(const STATEMENT& statement)
{
	assert(m_params.size() < MAX_PARAMS);

	CSymbol* src1 = statement.src1->GetSymbol().get();

	m_params.push_back(std::bind(
		&CX86Assembler::LeaGq, &m_assembler, std::placeholders::_1, MakeMemory128SymbolAddress(src1)));
}

void CCodeGen_x86_64::Emit_Call(const STATEMENT& statement)
{
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	unsigned int paramCount = src2->m_valueLow;

	for(unsigned int i = 0; i < paramCount; i++)
	{
		ParamEmitterFunction emitter(m_params.back());
		m_params.pop_back();
		emitter(g_paramRegs[i]);
	}

	m_assembler.MovIq(CX86Assembler::rAX, CombineConstant64(src1->m_valueLow, src1->m_valueHigh));
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

void CCodeGen_x86_64::Cmp64_RelRel(CX86Assembler::REGISTER dstReg, const STATEMENT& statement)
{
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_RELATIVE64);
	assert(src2->m_type == SYM_RELATIVE64);

	CX86Assembler::REGISTER tmpReg = CX86Assembler::rAX;
	m_assembler.MovEq(tmpReg, MakeRelative64SymbolAddress(src1));
	m_assembler.CmpEq(tmpReg, MakeRelative64SymbolAddress(src2));

	Cmp_GetFlag(CX86Assembler::MakeRegisterAddress(tmpReg), statement.jmpCondition);
	m_assembler.MovzxEb(dstReg, CX86Assembler::MakeRegisterAddress(tmpReg));
}

void CCodeGen_x86_64::Cmp64_RelCst(CX86Assembler::REGISTER dstReg, const STATEMENT& statement)
{
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_RELATIVE64);
	assert(src2->m_type == SYM_CONSTANT64);

	uint64 constant = CombineConstant64(src2->m_valueLow, src2->m_valueHigh);

	CX86Assembler::REGISTER tmpReg = CX86Assembler::rAX;
	m_assembler.MovEq(tmpReg, MakeRelative64SymbolAddress(src1));
	if(constant == 0)
	{
		CX86Assembler::REGISTER cstReg = CX86Assembler::rDX;
		m_assembler.XorGq(CX86Assembler::MakeRegisterAddress(cstReg), cstReg);
		m_assembler.CmpEq(tmpReg, CX86Assembler::MakeRegisterAddress(cstReg));
	}
	else
	{
		m_assembler.CmpIq(CX86Assembler::MakeRegisterAddress(tmpReg), constant);
	}

	Cmp_GetFlag(CX86Assembler::MakeRegisterAddress(tmpReg), statement.jmpCondition);
	m_assembler.MovzxEb(dstReg, CX86Assembler::MakeRegisterAddress(tmpReg));
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

void CCodeGen_x86_64::Emit_RelToRef_TmpCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type  == SYM_TMP_REFERENCE);
	assert(src1->m_type == SYM_CONSTANT);

	CX86Assembler::REGISTER tmpReg = CX86Assembler::rAX;
	m_assembler.LeaGq(tmpReg, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow));
	m_assembler.MovGq(MakeTemporaryReferenceSymbolAddress(dst), tmpReg);
}

void CCodeGen_x86_64::Emit_AddRef_MemMemReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_REGISTER);

	CX86Assembler::REGISTER tmpReg = CX86Assembler::rAX;
	m_assembler.MovEq(tmpReg, MakeMemoryReferenceSymbolAddress(src1));
	m_assembler.AddEq(tmpReg, CX86Assembler::MakeRegisterAddress(m_registers[src2->m_valueLow]));
	m_assembler.MovGq(MakeMemoryReferenceSymbolAddress(dst), tmpReg);
}

void CCodeGen_x86_64::Emit_AddRef_MemMemCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_CONSTANT);

	CX86Assembler::REGISTER tmpReg = CX86Assembler::rAX;
	m_assembler.MovEq(tmpReg, MakeMemoryReferenceSymbolAddress(src1));
	m_assembler.AddIq(CX86Assembler::MakeRegisterAddress(tmpReg), src2->m_valueLow);
	m_assembler.MovGq(MakeMemoryReferenceSymbolAddress(dst), tmpReg);
}

void CCodeGen_x86_64::Emit_LoadFromRef_RegTmp(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type  == SYM_REGISTER);
	assert(src1->m_type == SYM_TMP_REFERENCE);

	CX86Assembler::REGISTER tmpReg = CX86Assembler::rAX;
	m_assembler.MovEq(tmpReg, MakeTemporaryReferenceSymbolAddress(src1));
	m_assembler.MovEd(m_registers[dst->m_valueLow], CX86Assembler::MakeIndRegAddress(tmpReg));
}

void CCodeGen_x86_64::Emit_StoreAtRef_MemReg(const STATEMENT& statement)
{
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_REGISTER);

	CX86Assembler::REGISTER tmpReg = CX86Assembler::rAX;
	m_assembler.MovEq(tmpReg, MakeMemoryReferenceSymbolAddress(src1));
	m_assembler.MovGd(CX86Assembler::MakeIndRegAddress(tmpReg), m_registers[src2->m_valueLow]);
}

void CCodeGen_x86_64::Emit_StoreAtRef_MemCst(const STATEMENT& statement)
{
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_CONSTANT);

	CX86Assembler::REGISTER tmpReg = CX86Assembler::rAX;
	m_assembler.MovEq(tmpReg, MakeMemoryReferenceSymbolAddress(src1));
	m_assembler.MovId(CX86Assembler::MakeIndRegAddress(tmpReg), src2->m_valueLow);
}
