#include "Jitter_CodeGen_x86_32.h"
#include <algorithm>

using namespace Jitter;

CX86Assembler::REGISTER CCodeGen_x86_32::g_registers[MAX_REGISTERS] =
{
	CX86Assembler::rBX,
	CX86Assembler::rSI,
	CX86Assembler::rDI,
};

CCodeGen_x86_32::CONSTMATCHER CCodeGen_x86_32::g_constMatchers[] = 
{ 
	{ OP_PARAM,			MATCH_NIL,			MATCH_CONTEXT,		MATCH_NIL,			&CCodeGen_x86_32::Emit_Param_Ctx			},
	{ OP_PARAM,			MATCH_NIL,			MATCH_MEMORY,		MATCH_NIL,			&CCodeGen_x86_32::Emit_Param_Mem			},
	{ OP_PARAM,			MATCH_NIL,			MATCH_CONSTANT,		MATCH_NIL,			&CCodeGen_x86_32::Emit_Param_Cst			},
	{ OP_PARAM,			MATCH_NIL,			MATCH_REGISTER,		MATCH_NIL,			&CCodeGen_x86_32::Emit_Param_Reg			},
	{ OP_PARAM,			MATCH_NIL,			MATCH_MEMORY64,		MATCH_NIL,			&CCodeGen_x86_32::Emit_Param_Mem64			},
	{ OP_PARAM,			MATCH_NIL,			MATCH_CONSTANT64,	MATCH_NIL,			&CCodeGen_x86_32::Emit_Param_Cst64			},
	{ OP_PARAM,			MATCH_NIL,			MATCH_MEMORY128,	MATCH_NIL,			&CCodeGen_x86_32::Emit_Param_Mem128			},

	{ OP_PARAM_RET,		MATCH_NIL,			MATCH_MEMORY128,	MATCH_NIL,			&CCodeGen_x86_32::Emit_ParamRet_Mem128		},
	
	{ OP_CALL,			MATCH_NIL,			MATCH_CONSTANT,		MATCH_CONSTANT,		&CCodeGen_x86_32::Emit_Call					},

	{ OP_RETVAL,		MATCH_TEMPORARY,	MATCH_NIL,			MATCH_NIL,			&CCodeGen_x86_32::Emit_RetVal_Tmp			},
	{ OP_RETVAL,		MATCH_REGISTER,		MATCH_NIL,			MATCH_NIL,			&CCodeGen_x86_32::Emit_RetVal_Reg			},
	{ OP_RETVAL,		MATCH_MEMORY64,		MATCH_NIL,			MATCH_NIL,			&CCodeGen_x86_32::Emit_RetVal_Mem64			},

	{ OP_MOV,			MATCH_MEMORY64,		MATCH_MEMORY64,		MATCH_NIL,			&CCodeGen_x86_32::Emit_Mov_Mem64Mem64		},
	{ OP_MOV,			MATCH_RELATIVE64,	MATCH_CONSTANT64,	MATCH_NIL,			&CCodeGen_x86_32::Emit_Mov_Rel64Cst64		},

	{ OP_ADD64,			MATCH_MEMORY64,		MATCH_MEMORY64,		MATCH_MEMORY64,		&CCodeGen_x86_32::Emit_Add64_MemMemMem		},
	{ OP_ADD64,			MATCH_RELATIVE64,	MATCH_RELATIVE64,	MATCH_CONSTANT64,	&CCodeGen_x86_32::Emit_Add64_RelRelCst		},

	{ OP_SUB64,			MATCH_RELATIVE64,	MATCH_RELATIVE64,	MATCH_RELATIVE64,	&CCodeGen_x86_32::Emit_Sub64_RelRelRel		},
	{ OP_SUB64,			MATCH_RELATIVE64,	MATCH_CONSTANT64,	MATCH_RELATIVE64,	&CCodeGen_x86_32::Emit_Sub64_RelCstRel		},

	{ OP_AND64,			MATCH_RELATIVE64,	MATCH_RELATIVE64,	MATCH_RELATIVE64,	&CCodeGen_x86_32::Emit_And64_RelRelRel		},

	{ OP_SRL64,			MATCH_MEMORY64,		MATCH_MEMORY64,		MATCH_REGISTER,		&CCodeGen_x86_32::Emit_Srl64_MemMemReg		},
	{ OP_SRL64,			MATCH_MEMORY64,		MATCH_MEMORY64,		MATCH_MEMORY,		&CCodeGen_x86_32::Emit_Srl64_MemMemMem		},
	{ OP_SRL64,			MATCH_RELATIVE64,	MATCH_RELATIVE64,	MATCH_CONSTANT,		&CCodeGen_x86_32::Emit_Srl64_RelRelCst		},

	{ OP_SRA64,			MATCH_MEMORY64,		MATCH_MEMORY64,		MATCH_REGISTER,		&CCodeGen_x86_32::Emit_Sra64_MemMemReg		},
	{ OP_SRA64,			MATCH_MEMORY64,		MATCH_MEMORY64,		MATCH_MEMORY,		&CCodeGen_x86_32::Emit_Sra64_MemMemMem		},
	{ OP_SRA64,			MATCH_RELATIVE64,	MATCH_RELATIVE64,	MATCH_CONSTANT,		&CCodeGen_x86_32::Emit_Sra64_RelRelCst		},

	{ OP_SLL64,			MATCH_MEMORY64,		MATCH_MEMORY64,		MATCH_REGISTER,		&CCodeGen_x86_32::Emit_Sll64_MemMemReg		},
	{ OP_SLL64,			MATCH_MEMORY64,		MATCH_MEMORY64,		MATCH_MEMORY,		&CCodeGen_x86_32::Emit_Sll64_MemMemMem		},
	{ OP_SLL64,			MATCH_RELATIVE64,	MATCH_RELATIVE64,	MATCH_CONSTANT,		&CCodeGen_x86_32::Emit_Sll64_RelRelCst		},

	{ OP_CMP64,			MATCH_REGISTER,		MATCH_RELATIVE64,	MATCH_RELATIVE64,	&CCodeGen_x86_32::Emit_Cmp64_RegRelRel		},
	{ OP_CMP64,			MATCH_RELATIVE,		MATCH_RELATIVE64,	MATCH_RELATIVE64,	&CCodeGen_x86_32::Emit_Cmp64_RelRelRel		},
	{ OP_CMP64,			MATCH_REGISTER,		MATCH_RELATIVE64,	MATCH_CONSTANT64,	&CCodeGen_x86_32::Emit_Cmp64_RegRelCst		},
	{ OP_CMP64,			MATCH_RELATIVE,		MATCH_RELATIVE64,	MATCH_CONSTANT64,	&CCodeGen_x86_32::Emit_Cmp64_RelRelCst		},
	{ OP_CMP64,			MATCH_TEMPORARY,	MATCH_RELATIVE64,	MATCH_RELATIVE64,	&CCodeGen_x86_32::Emit_Cmp64_TmpRelRoc		},
	{ OP_CMP64,			MATCH_TEMPORARY,	MATCH_RELATIVE64,	MATCH_CONSTANT64,	&CCodeGen_x86_32::Emit_Cmp64_TmpRelRoc		},

	{ OP_RELTOREF,		MATCH_TMP_REF,		MATCH_CONSTANT,		MATCH_NIL,			&CCodeGen_x86_32::Emit_RelToRef_TmpCst		},

	{ OP_ADDREF,		MATCH_MEM_REF,		MATCH_MEM_REF,		MATCH_REGISTER,		&CCodeGen_x86_32::Emit_AddRef_MemMemReg		},
	{ OP_ADDREF,		MATCH_MEM_REF,		MATCH_MEM_REF,		MATCH_CONSTANT,		&CCodeGen_x86_32::Emit_AddRef_MemMemCst		},

	{ OP_LOADFROMREF,	MATCH_REGISTER,		MATCH_TMP_REF,		MATCH_NIL,			&CCodeGen_x86_32::Emit_LoadFromRef_RegTmp	},
	{ OP_LOADFROMREF,	MATCH_MEMORY,		MATCH_TMP_REF,		MATCH_NIL,			&CCodeGen_x86_32::Emit_LoadFromRef_MemTmp	},

	{ OP_STOREATREF,	MATCH_NIL,			MATCH_TMP_REF,		MATCH_REGISTER,		&CCodeGen_x86_32::Emit_StoreAtRef_TmpReg	},
	{ OP_STOREATREF,	MATCH_NIL,			MATCH_TMP_REF,		MATCH_MEMORY,		&CCodeGen_x86_32::Emit_StoreAtRef_TmpMem	},
	{ OP_STOREATREF,	MATCH_NIL,			MATCH_TMP_REF,		MATCH_CONSTANT,		&CCodeGen_x86_32::Emit_StoreAtRef_TmpCst	},

	{ OP_MOV,			MATCH_NIL,			MATCH_NIL,			MATCH_NIL,			NULL										},
};

CCodeGen_x86_32::CCodeGen_x86_32()
: m_hasImplicitRetValueParam(false)
, m_paramAreaSize(0)
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

CCodeGen_x86_32::~CCodeGen_x86_32()
{

}

void CCodeGen_x86_32::Emit_Prolog(const StatementList& statements, unsigned int stackSize, uint32 registerUsage)
{	
	//Compute the size needed to store all function call parameters
	uint32 maxParamSize = 0;
	uint32 currParamSize = 0;
	
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
					case SYM_CONTEXT:
					case SYM_REGISTER:
					case SYM_RELATIVE:
					case SYM_CONSTANT:
					case SYM_TEMPORARY:
					case SYM_TEMPORARY128:
					case SYM_RELATIVE128:
						currParamSize += 4;
						break;
					case SYM_CONSTANT64:
					case SYM_TEMPORARY64:
					case SYM_RELATIVE64:
						currParamSize += 8;
						break;
					default:
						assert(0);
						break;
				}
			}
			break;
		case OP_CALL:
			//Maybe we need to reset currParamSize to zero here?
			maxParamSize = std::max<uint32>(currParamSize, maxParamSize);
			break;
		}
	}
	
	//Fetch parameter
	m_assembler.Push(CX86Assembler::rBP);
	m_assembler.MovEd(CX86Assembler::rBP, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, 8));

	//Save registers
	for(unsigned int i = 0; i < MAX_REGISTERS; i++)
	{
		if(registerUsage & (1 << i))
		{
			m_assembler.Push(m_registers[i]);
		}
	}

	//Align stack
	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeRegisterAddress(CX86Assembler::rSP));
	m_assembler.SubId(CX86Assembler::MakeRegisterAddress(CX86Assembler::rSP), 0x10);
	m_assembler.AndId(CX86Assembler::MakeRegisterAddress(CX86Assembler::rSP), ~0x0F);
	m_assembler.SubId(CX86Assembler::MakeRegisterAddress(CX86Assembler::rSP), 0x0C);
	m_assembler.Push(CX86Assembler::rAX);

	//Allocate stack space for temps
	uint32 totalStackSize = stackSize + maxParamSize;

	//Allocate stack space
	if(totalStackSize != 0)
	{
		//Check how much more stack space we need to allocate to stay 16-bytes aligned
		//We assume that the stack is aligned at 16-bytes at the start of the function
		uint32 paramStackAdjust = (0x10 - totalStackSize) & 0x0F;
		
		m_assembler.SubId(CX86Assembler::MakeRegisterAddress(CX86Assembler::rSP), totalStackSize + paramStackAdjust);
		
		m_stackLevel = maxParamSize + paramStackAdjust;
	}
	else
	{
		m_stackLevel = 0;
	}
	
	m_paramAreaSize = m_stackLevel;
}

void CCodeGen_x86_32::Emit_Epilog(unsigned int stackSize, uint32 registerUsage)
{
	uint32 totalStackSize = stackSize + m_paramAreaSize;
	if(totalStackSize != 0)
	{
		m_assembler.AddId(CX86Assembler::MakeRegisterAddress(CX86Assembler::rSP), totalStackSize);
	}

	m_assembler.Pop(CX86Assembler::rSP);

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

unsigned int CCodeGen_x86_32::GetAvailableRegisterCount() const
{
	return MAX_REGISTERS;
}

unsigned int CCodeGen_x86_32::GetAvailableMdRegisterCount() const
{
	return 0;
}

unsigned int CCodeGen_x86_32::GetAddressSize() const
{
	return 4;
}

bool CCodeGen_x86_32::CanHold128BitsReturnValueInRegisters() const
{
	return false;
}

uint32 CCodeGen_x86_32::WriteCtxParam(uint32 offset)
{
	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, offset), CX86Assembler::rBP);
	return 4;
}

uint32 CCodeGen_x86_32::WriteRegParam(uint32 offset, CX86Assembler::REGISTER regId)
{
	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, offset), regId);
	return 4;
}

uint32 CCodeGen_x86_32::WriteMemParam(uint32 offset, CSymbol* sym)
{
	m_assembler.MovEd(CX86Assembler::rAX, MakeMemorySymbolAddress(sym));
	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, offset), CX86Assembler::rAX);
	return 4;
}

uint32 CCodeGen_x86_32::WriteCstParam(uint32 offset, uint32 cst)
{
	m_assembler.MovId(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, offset), cst);
	return 4;
}

uint32 CCodeGen_x86_32::WriteMem64Param(uint32 offset, CSymbol* sym)
{
	m_assembler.MovEd(CX86Assembler::rAX, MakeMemory64SymbolLoAddress(sym));
	m_assembler.MovEd(CX86Assembler::rDX, MakeMemory64SymbolHiAddress(sym));
	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, offset + 0), CX86Assembler::rAX);
	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, offset + 4), CX86Assembler::rDX);
	return 8;
}

uint32 CCodeGen_x86_32::WriteCst64Param(uint32 offset, CSymbol* sym)
{
	m_assembler.MovId(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, offset + 0), sym->m_valueLow);
	m_assembler.MovId(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, offset + 4), sym->m_valueHigh);
	return 8;	
}

uint32 CCodeGen_x86_32::WriteMem128Param(uint32 offset, const CX86Assembler::CAddress& address)
{
	m_assembler.LeaGd(CX86Assembler::rAX, address);
	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, offset), CX86Assembler::rAX);
	return 4;
}

void CCodeGen_x86_32::Emit_Param_Mem(const STATEMENT& statement)
{
	CSymbol* src1 = statement.src1->GetSymbol().get();
	m_params.push_back(std::bind(&CCodeGen_x86_32::WriteMemParam, this, std::placeholders::_1, src1));
}

void CCodeGen_x86_32::Emit_Param_Ctx(const STATEMENT& statement)
{
	m_params.push_back(std::bind(&CCodeGen_x86_32::WriteCtxParam, this, std::placeholders::_1));
}

void CCodeGen_x86_32::Emit_Param_Cst(const STATEMENT& statement)
{
	CSymbol* src1 = statement.src1->GetSymbol().get();
	m_params.push_back(std::bind(&CCodeGen_x86_32::WriteCstParam, this, std::placeholders::_1, src1->m_valueLow));
}

void CCodeGen_x86_32::Emit_Param_Reg(const STATEMENT& statement)
{
	CSymbol* src1 = statement.src1->GetSymbol().get();
	m_params.push_back(std::bind(&CCodeGen_x86_32::WriteRegParam, this, std::placeholders::_1, m_registers[src1->m_valueLow]));
}

void CCodeGen_x86_32::Emit_Param_Mem64(const STATEMENT& statement)
{
	CSymbol* src1 = statement.src1->GetSymbol().get();
	m_params.push_back(std::bind(&CCodeGen_x86_32::WriteMem64Param, this, std::placeholders::_1, src1));
}

void CCodeGen_x86_32::Emit_Param_Cst64(const STATEMENT& statement)
{
	CSymbol* src1 = statement.src1->GetSymbol().get();
	m_params.push_back(std::bind(&CCodeGen_x86_32::WriteCst64Param, this, std::placeholders::_1, src1));
}

void CCodeGen_x86_32::Emit_Param_Mem128(const STATEMENT& statement)
{
	CSymbol* src1 = statement.src1->GetSymbol().get();
	m_params.push_back(std::bind(&CCodeGen_x86_32::WriteMem128Param, this, std::placeholders::_1, MakeMemory128SymbolAddress(src1)));
}

void CCodeGen_x86_32::Emit_ParamRet_Mem128(const STATEMENT& statement)
{
	//Basically the same as Param_Mem128, but special care must be taken
	//as System V ABI automatically cleans up that extra parameter that's
	//used as return value
	Emit_Param_Mem128(statement);
	assert(!m_hasImplicitRetValueParam);
	m_hasImplicitRetValueParam = true;
}

void CCodeGen_x86_32::Emit_Call(const STATEMENT& statement)
{
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();
	
	uint32 paramCount = src2->m_valueLow;
	uint32 currentOffset = 0;
	for(unsigned int i = 0; i < paramCount; i++)
	{
		auto emitter(m_params.back());
		m_params.pop_back();
		currentOffset += emitter(currentOffset);
	}
	assert(currentOffset <= m_paramAreaSize);
	
	m_assembler.MovId(CX86Assembler::rAX, src1->m_valueLow);
	auto symbolRefLabel = m_assembler.CreateLabel();
	m_assembler.MarkLabel(symbolRefLabel, -4);
	m_symbolReferenceLabels.push_back(std::make_pair(reinterpret_cast<void*>(src1->m_valueLow), symbolRefLabel));
	m_assembler.CallEd(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX));
	
#ifdef __APPLE__
	if(m_hasImplicitRetValueParam)
	{
		//Allocated stack space for the extra parameter is cleaned up by the callee. 
		//So adjust the amount of stack space we free up after the call
		m_assembler.SubId(CX86Assembler::MakeRegisterAddress(CX86Assembler::rSP), 4);
	}
#endif
	
	m_hasImplicitRetValueParam = false;
}

void CCodeGen_x86_32::Emit_RetVal_Tmp(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();

	m_assembler.MovGd(MakeTemporarySymbolAddress(dst), CX86Assembler::rAX);
}

void CCodeGen_x86_32::Emit_RetVal_Reg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();

	assert(dst->m_type == SYM_REGISTER);

	m_assembler.MovGd(CX86Assembler::MakeRegisterAddress(m_registers[dst->m_valueLow]), CX86Assembler::rAX);
}

void CCodeGen_x86_32::Emit_RetVal_Mem64(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();

	m_assembler.MovGd(MakeMemory64SymbolLoAddress(dst), CX86Assembler::rAX);
	m_assembler.MovGd(MakeMemory64SymbolHiAddress(dst), CX86Assembler::rDX);
}

void CCodeGen_x86_32::Emit_Mov_Mem64Mem64(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	m_assembler.MovEd(CX86Assembler::rAX, MakeMemory64SymbolLoAddress(src1));
	m_assembler.MovEd(CX86Assembler::rDX, MakeMemory64SymbolHiAddress(src1));

	m_assembler.MovGd(MakeMemory64SymbolLoAddress(dst), CX86Assembler::rAX);
	m_assembler.MovGd(MakeMemory64SymbolHiAddress(dst), CX86Assembler::rDX);
}

void CCodeGen_x86_32::Emit_Mov_Rel64Cst64(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type  == SYM_RELATIVE64);
	assert(src1->m_type == SYM_CONSTANT64);

	m_assembler.MovId(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, dst->m_valueLow + 0), src1->m_valueLow);
	m_assembler.MovId(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, dst->m_valueLow + 4), src1->m_valueHigh);
}

void CCodeGen_x86_32::Emit_Add64_MemMemMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	m_assembler.MovEd(CX86Assembler::rAX, MakeMemory64SymbolLoAddress(src1));
	m_assembler.MovEd(CX86Assembler::rDX, MakeMemory64SymbolHiAddress(src1));

	m_assembler.AddEd(CX86Assembler::rAX, MakeMemory64SymbolLoAddress(src2));
	m_assembler.AdcEd(CX86Assembler::rDX, MakeMemory64SymbolHiAddress(src2));

	m_assembler.MovGd(MakeMemory64SymbolLoAddress(dst), CX86Assembler::rAX);
	m_assembler.MovGd(MakeMemory64SymbolHiAddress(dst), CX86Assembler::rDX);
}

void CCodeGen_x86_32::Emit_Add64_RelRelCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_RELATIVE64);
	assert(src1->m_type == SYM_RELATIVE64);
	assert(src2->m_type == SYM_CONSTANT64);

	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow + 0));
	m_assembler.MovEd(CX86Assembler::rDX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow + 4));

	m_assembler.AddId(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX), src2->m_valueLow);
	m_assembler.AdcId(CX86Assembler::MakeRegisterAddress(CX86Assembler::rDX), src2->m_valueHigh);

	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, dst->m_valueLow + 0), CX86Assembler::rAX);
	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, dst->m_valueLow + 4), CX86Assembler::rDX);
}

void CCodeGen_x86_32::Emit_Sub64_RelRelRel(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_RELATIVE64);
	assert(src1->m_type == SYM_RELATIVE64);
	assert(src2->m_type == SYM_RELATIVE64);

	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow + 0));
	m_assembler.MovEd(CX86Assembler::rDX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow + 4));

	m_assembler.SubEd(CX86Assembler::rAX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src2->m_valueLow + 0));
	m_assembler.SbbEd(CX86Assembler::rDX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src2->m_valueLow + 4));

	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, dst->m_valueLow + 0), CX86Assembler::rAX);
	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, dst->m_valueLow + 4), CX86Assembler::rDX);
}

void CCodeGen_x86_32::Emit_Sub64_RelCstRel(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_RELATIVE64);
	assert(src1->m_type == SYM_CONSTANT64);
	assert(src2->m_type == SYM_RELATIVE64);

	m_assembler.MovId(CX86Assembler::rAX, src1->m_valueLow);
	m_assembler.MovId(CX86Assembler::rDX, src1->m_valueHigh);

	m_assembler.SubEd(CX86Assembler::rAX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src2->m_valueLow + 0));
	m_assembler.SbbEd(CX86Assembler::rDX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src2->m_valueLow + 4));

	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, dst->m_valueLow + 0), CX86Assembler::rAX);
	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, dst->m_valueLow + 4), CX86Assembler::rDX);
}

void CCodeGen_x86_32::Emit_And64_RelRelRel(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_RELATIVE64);
	assert(src1->m_type == SYM_RELATIVE64);
	assert(src2->m_type == SYM_RELATIVE64);

	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow + 0));
	m_assembler.MovEd(CX86Assembler::rDX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow + 4));

	m_assembler.AndEd(CX86Assembler::rAX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src2->m_valueLow + 0));
	m_assembler.AndEd(CX86Assembler::rDX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src2->m_valueLow + 4));

	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, dst->m_valueLow + 0), CX86Assembler::rAX);
	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, dst->m_valueLow + 4), CX86Assembler::rDX);
}

//---------------------------------------------------------------------------------
//SRL64
//---------------------------------------------------------------------------------

void CCodeGen_x86_32::Emit_Srl64_MemMemVar(const STATEMENT& statement, CX86Assembler::REGISTER shiftRegister)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type  == SYM_RELATIVE64);
	assert(src1->m_type == SYM_RELATIVE64);

	CX86Assembler::REGISTER regLo = CX86Assembler::rAX;
	CX86Assembler::REGISTER regHi = CX86Assembler::rDX;
	CX86Assembler::REGISTER regSa = CX86Assembler::rCX;

	CX86Assembler::LABEL lessThan32Label = m_assembler.CreateLabel();
	CX86Assembler::LABEL endLabel = m_assembler.CreateLabel();

	if(shiftRegister != regSa)
	{
		m_assembler.MovEd(regSa, CX86Assembler::MakeRegisterAddress(shiftRegister));
	}

	m_assembler.CmpId(CX86Assembler::MakeRegisterAddress(regSa), 32);
	m_assembler.JbJx(lessThan32Label);

	//greaterOrEqual:
	//---------------------------------------------
	m_assembler.MovEd(regLo, MakeMemory64SymbolHiAddress(src1));
	m_assembler.AndId(CX86Assembler::MakeRegisterAddress(regSa), 0x1F);
	m_assembler.ShrEd(CX86Assembler::MakeRegisterAddress(regLo));
	m_assembler.XorEd(regHi, CX86Assembler::MakeRegisterAddress(regHi));

	m_assembler.JmpJx(endLabel);

	//lessThan:
	//---------------------------------------------
	m_assembler.MarkLabel(lessThan32Label);

	m_assembler.MovEd(regLo, MakeMemory64SymbolLoAddress(src1));
	m_assembler.MovEd(regHi, MakeMemory64SymbolHiAddress(src1));

	m_assembler.ShrdEd(CX86Assembler::MakeRegisterAddress(regLo), regHi);
	m_assembler.ShrEd(CX86Assembler::MakeRegisterAddress(regHi));

	//end:
	//---------------------------------------------
	m_assembler.MarkLabel(endLabel);

	m_assembler.MovGd(MakeMemory64SymbolLoAddress(dst), regLo);
	m_assembler.MovGd(MakeMemory64SymbolHiAddress(dst), regHi);
}

void CCodeGen_x86_32::Emit_Srl64_MemMemReg(const STATEMENT& statement)
{
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_REGISTER);

	Emit_Srl64_MemMemVar(statement, g_registers[src2->m_valueLow]);
}

void CCodeGen_x86_32::Emit_Srl64_MemMemMem(const STATEMENT& statement)
{
	CSymbol* src2 = statement.src2->GetSymbol().get();

	CX86Assembler::REGISTER shiftAmount = CX86Assembler::rCX;

	m_assembler.MovEd(shiftAmount, MakeMemorySymbolAddress(src2));

	Emit_Srl64_MemMemVar(statement, shiftAmount);
}

void CCodeGen_x86_32::Emit_Srl64_RelRelCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_RELATIVE64);
	assert(src1->m_type == SYM_RELATIVE64);
	assert(src2->m_type == SYM_CONSTANT);

	uint8 shiftAmount = static_cast<uint8>(src2->m_valueLow);

	assert(shiftAmount < 0x40);
	assert(shiftAmount != 0);

	CX86Assembler::REGISTER regLo = CX86Assembler::rAX;
	CX86Assembler::REGISTER regHi = CX86Assembler::rDX;

	if(shiftAmount >= 32)
	{
		m_assembler.MovEd(regLo, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow + 4));

		if(shiftAmount != 32)
		{
			//shr reg, amount
			m_assembler.ShrEd(CX86Assembler::MakeRegisterAddress(regLo), shiftAmount & 0x1F);
		}

		m_assembler.XorEd(regHi, CX86Assembler::MakeRegisterAddress(regHi));
	}
	else //Amount < 32
	{
		m_assembler.MovEd(regLo, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow + 0));
		m_assembler.MovEd(regHi, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow + 4));

		if(shiftAmount != 0)
		{
			//shrd nReg1, nReg2, nAmount
			m_assembler.ShrdEd(CX86Assembler::MakeRegisterAddress(regLo), regHi, shiftAmount);

			//shr nReg2, nAmount
			m_assembler.ShrEd(CX86Assembler::MakeRegisterAddress(regHi), shiftAmount);
		}
	}

	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, dst->m_valueLow + 0), regLo);
	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, dst->m_valueLow + 4), regHi);
}

//---------------------------------------------------------------------------------
//SRA64
//---------------------------------------------------------------------------------

void CCodeGen_x86_32::Emit_Sra64_MemMemVar(const STATEMENT& statement, CX86Assembler::REGISTER shiftRegister)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type  == SYM_RELATIVE64);
	assert(src1->m_type == SYM_RELATIVE64);

	CX86Assembler::REGISTER regLo = CX86Assembler::rAX;
	CX86Assembler::REGISTER regHi = CX86Assembler::rDX;
	CX86Assembler::REGISTER regSa = CX86Assembler::rCX;

	CX86Assembler::LABEL lessThan32Label = m_assembler.CreateLabel();
	CX86Assembler::LABEL endLabel = m_assembler.CreateLabel();

	if(shiftRegister != regSa)
	{
		m_assembler.MovEd(regSa, CX86Assembler::MakeRegisterAddress(shiftRegister));
	}

	m_assembler.CmpId(CX86Assembler::MakeRegisterAddress(regSa), 32);
	m_assembler.JbJx(lessThan32Label);

	//greaterOrEqual:
	//---------------------------------------------
	m_assembler.MovEd(regLo, MakeMemory64SymbolHiAddress(src1));
	m_assembler.MovEd(regHi, MakeMemory64SymbolHiAddress(src1));

	m_assembler.AndId(CX86Assembler::MakeRegisterAddress(regSa), 0x1F);
	m_assembler.SarEd(CX86Assembler::MakeRegisterAddress(regLo));
	m_assembler.SarEd(CX86Assembler::MakeRegisterAddress(regHi), 31);

	m_assembler.JmpJx(endLabel);

	//lessThan:
	//---------------------------------------------
	m_assembler.MarkLabel(lessThan32Label);

	m_assembler.MovEd(regLo, MakeMemory64SymbolLoAddress(src1));
	m_assembler.MovEd(regHi, MakeMemory64SymbolHiAddress(src1));

	m_assembler.ShrdEd(CX86Assembler::MakeRegisterAddress(regLo), regHi);
	m_assembler.SarEd(CX86Assembler::MakeRegisterAddress(regHi));

	//end:
	//---------------------------------------------
	m_assembler.MarkLabel(endLabel);

	m_assembler.MovGd(MakeMemory64SymbolLoAddress(dst), regLo);
	m_assembler.MovGd(MakeMemory64SymbolHiAddress(dst), regHi);
}

void CCodeGen_x86_32::Emit_Sra64_MemMemReg(const STATEMENT& statement)
{
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_REGISTER);

	Emit_Sra64_MemMemVar(statement, g_registers[src2->m_valueLow]);
}

void CCodeGen_x86_32::Emit_Sra64_MemMemMem(const STATEMENT& statement)
{
	CSymbol* src2 = statement.src2->GetSymbol().get();

	CX86Assembler::REGISTER shiftAmount = CX86Assembler::rCX;

	m_assembler.MovEd(shiftAmount, MakeMemorySymbolAddress(src2));

	Emit_Sra64_MemMemVar(statement, shiftAmount);
}

void CCodeGen_x86_32::Emit_Sra64_RelRelCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_RELATIVE64);
	assert(src1->m_type == SYM_RELATIVE64);
	assert(src2->m_type == SYM_CONSTANT);

	uint8 shiftAmount = static_cast<uint8>(src2->m_valueLow);

	assert(shiftAmount < 0x40);
	assert(shiftAmount != 0);

	CX86Assembler::REGISTER regLo = CX86Assembler::rAX;
	CX86Assembler::REGISTER regHi = CX86Assembler::rDX;

	if(shiftAmount >= 32)
	{
		m_assembler.MovEd(regLo, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow + 4));

		if(shiftAmount != 32)
		{
			//sar reg, amount
			m_assembler.SarEd(CX86Assembler::MakeRegisterAddress(regLo), shiftAmount & 0x1F);
		}

		m_assembler.MovEd(regHi, CX86Assembler::MakeRegisterAddress(regLo));
		m_assembler.SarEd(CX86Assembler::MakeRegisterAddress(regHi), 31);
	}
	else //Amount < 32
	{
		m_assembler.MovEd(regLo, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow + 0));
		m_assembler.MovEd(regHi, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow + 4));

		if(shiftAmount != 0)
		{
			//shrd nReg1, nReg2, nAmount
			m_assembler.ShrdEd(CX86Assembler::MakeRegisterAddress(regLo), regHi, shiftAmount);

			//sar nReg2, nAmount
			m_assembler.SarEd(CX86Assembler::MakeRegisterAddress(regHi), shiftAmount);
		}
	}

	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, dst->m_valueLow + 0), regLo);
	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, dst->m_valueLow + 4), regHi);
}

//---------------------------------------------------------------------------------
//SLL64
//---------------------------------------------------------------------------------

void CCodeGen_x86_32::Emit_Sll64_MemMemVar(const STATEMENT& statement, CX86Assembler::REGISTER shiftRegister)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	CX86Assembler::LABEL doneLabel = m_assembler.CreateLabel();
	CX86Assembler::LABEL more32Label = m_assembler.CreateLabel();

	CX86Assembler::REGISTER amountReg = CX86Assembler::rCX;
	CX86Assembler::REGISTER resultLow = CX86Assembler::rAX;
	CX86Assembler::REGISTER resultHigh = CX86Assembler::rDX;

	if(shiftRegister != amountReg)
	{
		m_assembler.MovEd(amountReg, CX86Assembler::MakeRegisterAddress(shiftRegister));
	}

	m_assembler.MovEd(resultLow, MakeMemory64SymbolLoAddress(src1));
	m_assembler.MovEd(resultHigh, MakeMemory64SymbolHiAddress(src1));

	m_assembler.AndIb(CX86Assembler::MakeByteRegisterAddress(amountReg), 0x3F);
	m_assembler.TestEb(amountReg, CX86Assembler::MakeByteRegisterAddress(amountReg));
	m_assembler.JzJx(doneLabel);

	m_assembler.CmpIb(CX86Assembler::MakeByteRegisterAddress(amountReg), 0x20);
	m_assembler.JnbJx(more32Label);

	m_assembler.ShldEd(CX86Assembler::MakeRegisterAddress(resultHigh), resultLow);
	m_assembler.ShlEd(CX86Assembler::MakeRegisterAddress(resultLow));
	m_assembler.JmpJx(doneLabel);

//$more32
	m_assembler.MarkLabel(more32Label);

	m_assembler.MovEd(resultHigh, CX86Assembler::MakeRegisterAddress(resultLow));
	m_assembler.XorEd(resultLow, CX86Assembler::MakeRegisterAddress(resultLow));
	m_assembler.AndIb(CX86Assembler::MakeByteRegisterAddress(amountReg), 0x1F);
	m_assembler.ShlEd(CX86Assembler::MakeRegisterAddress(resultHigh));

//$done
	m_assembler.MarkLabel(doneLabel);

	m_assembler.MovGd(MakeMemory64SymbolLoAddress(dst), resultLow);
	m_assembler.MovGd(MakeMemory64SymbolHiAddress(dst), resultHigh);
}

void CCodeGen_x86_32::Emit_Sll64_MemMemReg(const STATEMENT& statement)
{
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_REGISTER);

	Emit_Sll64_MemMemVar(statement, g_registers[src2->m_valueLow]);
}

void CCodeGen_x86_32::Emit_Sll64_MemMemMem(const STATEMENT& statement)
{
	CSymbol* src2 = statement.src2->GetSymbol().get();

	CX86Assembler::REGISTER shiftAmount = CX86Assembler::rCX;

	m_assembler.MovEd(shiftAmount, MakeMemorySymbolAddress(src2));

	Emit_Sll64_MemMemVar(statement, shiftAmount);
}

void CCodeGen_x86_32::Emit_Sll64_RelRelCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_RELATIVE64);
	assert(src1->m_type == SYM_RELATIVE64);
	assert(src2->m_type == SYM_CONSTANT);

	uint8 shiftAmount = static_cast<uint8>(src2->m_valueLow);

	assert(shiftAmount < 0x40);
	assert(shiftAmount != 0);

	CX86Assembler::REGISTER regLo = CX86Assembler::rAX;
	CX86Assembler::REGISTER regHi = CX86Assembler::rDX;

	if(shiftAmount >= 32)
	{
		m_assembler.MovEd(regHi, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow + 0));

		if(shiftAmount != 0)
		{
			//shl reg, amount
			m_assembler.ShlEd(CX86Assembler::MakeRegisterAddress(regHi), shiftAmount & 0x1F);
		}

		m_assembler.XorEd(regLo, CX86Assembler::MakeRegisterAddress(regLo));
	}
	else //Amount < 32
	{
		m_assembler.MovEd(regLo, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow + 0));
		m_assembler.MovEd(regHi, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow + 4));

		//shld nReg2, nReg1, nAmount
		m_assembler.ShldEd(CX86Assembler::MakeRegisterAddress(regHi), regLo, shiftAmount);

		//shl nReg1, nAmount
		m_assembler.ShlEd(CX86Assembler::MakeRegisterAddress(regLo), shiftAmount);
	}

	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, dst->m_valueLow + 0), regLo);
	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, dst->m_valueLow + 4), regHi);
}

//---------------------------------------------------------------------------------
//CMP64
//---------------------------------------------------------------------------------

void CCodeGen_x86_32::Cmp64_Equal(const STATEMENT& statement)
{
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	struct CmpLo
	{
		void operator()(CX86Assembler& assembler, CX86Assembler::REGISTER registerId, CSymbol* symbol)
		{
			switch(symbol->m_type)
			{
			case SYM_RELATIVE64:
				assembler.CmpEd(registerId, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, symbol->m_valueLow + 0));
				break;
			case SYM_CONSTANT64:
				if(symbol->m_valueLow == 0)
				{
					assembler.TestEd(registerId, CX86Assembler::MakeRegisterAddress(registerId));
				}
				else
				{
					assembler.CmpId(CX86Assembler::MakeRegisterAddress(registerId), symbol->m_valueLow);
				}
				break;
			default:
				assert(0);
				break;
			}
		}
	};

	struct CmpHi
	{
		void operator()(CX86Assembler& assembler, CX86Assembler::REGISTER registerId, CSymbol* symbol)
		{
			switch(symbol->m_type)
			{
			case SYM_RELATIVE64:
				assembler.CmpEd(registerId, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, symbol->m_valueLow + 4));
				break;
			case SYM_CONSTANT64:
				if(symbol->m_valueHigh == 0)
				{
					assembler.TestEd(registerId, CX86Assembler::MakeRegisterAddress(registerId));
				}
				else
				{
					assembler.CmpId(CX86Assembler::MakeRegisterAddress(registerId), symbol->m_valueHigh);
				}
				break;
			default:
				assert(0);
				break;
			}
		}
	};

	assert(src1->m_type == SYM_RELATIVE64);

	bool isEqual	= (statement.jmpCondition == CONDITION_EQ);

	CX86Assembler::REGISTER valReg = CX86Assembler::rDX;
	CX86Assembler::REGISTER res1Reg = CX86Assembler::rAX;
	CX86Assembler::REGISTER res2Reg = CX86Assembler::rCX;

	m_assembler.MovEd(valReg, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow + 0));
	CmpLo()(m_assembler, valReg, src2);

	if(isEqual)
	{
		m_assembler.SeteEb(CX86Assembler::MakeRegisterAddress(res1Reg));
	}
	else
	{
		m_assembler.SetneEb(CX86Assembler::MakeRegisterAddress(res1Reg));
	}
	
	m_assembler.MovEd(valReg, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow + 4));
	CmpHi()(m_assembler, valReg, src2);

	if(isEqual)
	{
		m_assembler.SeteEb(CX86Assembler::MakeRegisterAddress(res2Reg));
	}
	else
	{
		m_assembler.SetneEb(CX86Assembler::MakeRegisterAddress(res2Reg));
	}

	if(isEqual)
	{
		m_assembler.AndEd(res1Reg, CX86Assembler::MakeRegisterAddress(res2Reg));
	}
	else
	{
		m_assembler.OrEd(res1Reg, CX86Assembler::MakeRegisterAddress(res2Reg));
	}

	m_assembler.MovzxEb(res1Reg, CX86Assembler::MakeRegisterAddress(res1Reg));
}

struct CompareOrder64Less
{
	typedef void (CX86Assembler::*OrderCheckFunction)(const CX86Assembler::CAddress&);

	static bool IsSigned(Jitter::CONDITION condition)
	{
		return (condition == CONDITION_LE) || (condition == CONDITION_LT); 
	}

	static bool OrEqual(Jitter::CONDITION condition)
	{
		return (condition == CONDITION_LE) || (condition == CONDITION_BE);
	}

	static OrderCheckFunction CheckOrderSigned()
	{
		return &CX86Assembler::SetlEb;		
	}

	static OrderCheckFunction CheckOrderUnsigned()
	{
		return &CX86Assembler::SetbEb;
	}

	static OrderCheckFunction CheckOrderOrEqualUnsigned()
	{
		return &CX86Assembler::SetbeEb;
	}
};

struct CompareOrder64Greater
{
	typedef void (CX86Assembler::*OrderCheckFunction)(const CX86Assembler::CAddress&);

	static bool IsSigned(Jitter::CONDITION condition)
	{
		return (condition == CONDITION_GE) || (condition == CONDITION_GT);
	}

	static bool OrEqual(Jitter::CONDITION condition)
	{
		return (condition == CONDITION_GE) || (condition == CONDITION_AE);
	}

	static OrderCheckFunction CheckOrderSigned()
	{
		return &CX86Assembler::SetgEb;		
	}

	static OrderCheckFunction CheckOrderUnsigned()
	{
		return &CX86Assembler::SetaEb;
	}

	static OrderCheckFunction CheckOrderOrEqualUnsigned()
	{
		return &CX86Assembler::SetaeEb;
	}
};

template <typename CompareTraits>
void CCodeGen_x86_32::Cmp64_Order(const STATEMENT& statement)
{
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	CompareTraits compareTraits;
	(void)compareTraits;

	struct CmpLo
	{
		void operator()(CX86Assembler& assembler, CX86Assembler::REGISTER registerId, CSymbol* symbol)
		{
			switch(symbol->m_type)
			{
			case SYM_RELATIVE64:
				assembler.CmpEd(registerId, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, symbol->m_valueLow + 0));
				break;
			case SYM_CONSTANT64:
				assembler.CmpId(CX86Assembler::MakeRegisterAddress(registerId), symbol->m_valueLow);
				break;
			default:
				assert(0);
				break;
			}
		}
	};

	struct CmpHi
	{
		void operator()(CX86Assembler& assembler, CX86Assembler::REGISTER registerId, CSymbol* symbol)
		{
			switch(symbol->m_type)
			{
			case SYM_RELATIVE64:
				assembler.CmpEd(registerId, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, symbol->m_valueLow + 4));
				break;
			case SYM_CONSTANT64:
				assembler.CmpId(CX86Assembler::MakeRegisterAddress(registerId), symbol->m_valueHigh);
				break;
			default:
				assert(0);
				break;
			}
		}
	};

	assert(src1->m_type == SYM_RELATIVE64);

	CX86Assembler::REGISTER regLo = CX86Assembler::rAX;
	CX86Assembler::REGISTER regHi = CX86Assembler::rDX;

	bool isSigned	= compareTraits.IsSigned(statement.jmpCondition);
	bool orEqual	= compareTraits.OrEqual(statement.jmpCondition);

	CX86Assembler::LABEL highOrderEqualLabel = m_assembler.CreateLabel();
	CX86Assembler::LABEL doneLabel = m_assembler.CreateLabel();

	/////////////////////////////////////////
	//Check high order word if equal

	m_assembler.MovEd(regHi, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow + 4));
	CmpHi()(m_assembler, regHi, src2);

	//je highOrderEqual
	m_assembler.JzJx(highOrderEqualLabel);

	///////////////////////////////////////////////////////////
	//If they aren't equal, this comparaison decides of result

	//setb/l reg[l]
	if(isSigned)
	{
		((m_assembler).*(compareTraits.CheckOrderSigned()))(CX86Assembler::MakeByteRegisterAddress(regLo));
	}
	else
	{
		((m_assembler).*(compareTraits.CheckOrderUnsigned()))(CX86Assembler::MakeByteRegisterAddress(regLo));
	}

	//movzx reg, reg[l]
	m_assembler.MovzxEb(regLo, CX86Assembler::MakeByteRegisterAddress(regLo));

	//jmp done
	m_assembler.JmpJx(doneLabel);

	//highOrderEqual: /////////////////////////////////////
	m_assembler.MarkLabel(highOrderEqualLabel);
	//If they are equal, next comparaison decides of result

	m_assembler.MovEd(regLo, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow + 0));
	CmpLo()(m_assembler, regLo, src2);

	//setb/be reg[l]
	if(orEqual)
	{
		((m_assembler).*(compareTraits.CheckOrderOrEqualUnsigned()))(CX86Assembler::MakeByteRegisterAddress(regLo));
	}
	else
	{
		((m_assembler).*(compareTraits.CheckOrderUnsigned()))(CX86Assembler::MakeByteRegisterAddress(regLo));
	}

	//movzx reg, reg[l]
	m_assembler.MovzxEb(regLo, CX86Assembler::MakeByteRegisterAddress(regLo));

	//done: ///////////////////////////////////////////////
	m_assembler.MarkLabel(doneLabel);
}

void CCodeGen_x86_32::Cmp64_GenericRel(const STATEMENT& statement)
{
	switch(statement.jmpCondition)
	{
	case CONDITION_BL:
	case CONDITION_LT:
	case CONDITION_LE:
		Cmp64_Order<CompareOrder64Less>(statement);
		break;
	case CONDITION_AB:
	case CONDITION_GT:
	case CONDITION_GE:
		Cmp64_Order<CompareOrder64Greater>(statement);
		break;
	case CONDITION_NE:
	case CONDITION_EQ:
		Cmp64_Equal(statement);
		break;
	default:
		assert(0);
		break;
	}
}

void CCodeGen_x86_32::Emit_Cmp64_RegRelRel(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();

	assert(dst->m_type == SYM_REGISTER);

	Cmp64_GenericRel(statement);
	m_assembler.MovGd(CX86Assembler::MakeRegisterAddress(m_registers[dst->m_valueLow]), CX86Assembler::rAX);
}

void CCodeGen_x86_32::Emit_Cmp64_RelRelRel(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();

	assert(dst->m_type == SYM_RELATIVE);

	Cmp64_GenericRel(statement);
	m_assembler.MovGd(MakeRelativeSymbolAddress(dst), CX86Assembler::rAX);
}

void CCodeGen_x86_32::Emit_Cmp64_RegRelCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();

	assert(dst->m_type == SYM_REGISTER);

	Cmp64_GenericRel(statement);
	m_assembler.MovGd(CX86Assembler::MakeRegisterAddress(m_registers[dst->m_valueLow]), CX86Assembler::rAX);
}

void CCodeGen_x86_32::Emit_Cmp64_RelRelCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();

	assert(dst->m_type == SYM_RELATIVE);

	Cmp64_GenericRel(statement);
	m_assembler.MovGd(MakeRelativeSymbolAddress(dst), CX86Assembler::rAX);
}

void CCodeGen_x86_32::Emit_Cmp64_TmpRelRoc(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();

	assert(dst->m_type == SYM_TEMPORARY);

	Cmp64_GenericRel(statement);
	m_assembler.MovGd(MakeTemporarySymbolAddress(dst), CX86Assembler::rAX);
}

void CCodeGen_x86_32::Emit_RelToRef_TmpCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type  == SYM_TMP_REFERENCE);
	assert(src1->m_type == SYM_CONSTANT);

	CX86Assembler::REGISTER tmpReg = CX86Assembler::rAX;
	m_assembler.LeaGd(tmpReg, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow));
	m_assembler.MovGd(MakeTemporaryReferenceSymbolAddress(dst), tmpReg);
}

void CCodeGen_x86_32::Emit_AddRef_MemMemReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_REGISTER);

	CX86Assembler::REGISTER tmpReg = CX86Assembler::rAX;
	m_assembler.MovEd(tmpReg, MakeMemoryReferenceSymbolAddress(src1));
	m_assembler.AddEd(tmpReg, CX86Assembler::MakeRegisterAddress(m_registers[src2->m_valueLow]));
	m_assembler.MovGd(MakeMemoryReferenceSymbolAddress(dst), tmpReg);
}

void CCodeGen_x86_32::Emit_AddRef_MemMemCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_CONSTANT);

	CX86Assembler::REGISTER tmpReg = CX86Assembler::rAX;
	m_assembler.MovEd(tmpReg, MakeMemoryReferenceSymbolAddress(src1));
	m_assembler.AddId(CX86Assembler::MakeRegisterAddress(tmpReg), src2->m_valueLow);
	m_assembler.MovGd(MakeMemoryReferenceSymbolAddress(dst), tmpReg);
}

void CCodeGen_x86_32::Emit_LoadFromRef_RegTmp(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type  == SYM_REGISTER);
	assert(src1->m_type == SYM_TMP_REFERENCE);

	CX86Assembler::REGISTER tmpReg = CX86Assembler::rAX;
	m_assembler.MovEd(tmpReg, MakeTemporaryReferenceSymbolAddress(src1));
	m_assembler.MovEd(m_registers[dst->m_valueLow], CX86Assembler::MakeIndRegAddress(tmpReg));
}

void CCodeGen_x86_32::Emit_LoadFromRef_MemTmp(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	assert(src1->m_type == SYM_TMP_REFERENCE);

	CX86Assembler::REGISTER addressReg = CX86Assembler::rAX;
	CX86Assembler::REGISTER valueReg = CX86Assembler::rDX;

	m_assembler.MovEd(addressReg, MakeTemporaryReferenceSymbolAddress(src1));
	m_assembler.MovEd(valueReg, CX86Assembler::MakeIndRegAddress(addressReg));
	m_assembler.MovGd(MakeMemorySymbolAddress(dst), valueReg);
}

void CCodeGen_x86_32::Emit_StoreAtRef_TmpReg(const STATEMENT& statement)
{
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_TMP_REFERENCE);
	assert(src2->m_type == SYM_REGISTER);

	CX86Assembler::REGISTER addressReg = CX86Assembler::rAX;

	m_assembler.MovEd(addressReg, MakeTemporaryReferenceSymbolAddress(src1));
	m_assembler.MovGd(CX86Assembler::MakeIndRegAddress(addressReg), m_registers[src2->m_valueLow]);
}

void CCodeGen_x86_32::Emit_StoreAtRef_TmpMem(const STATEMENT& statement)
{
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_TMP_REFERENCE);

	CX86Assembler::REGISTER addressReg = CX86Assembler::rAX;
	CX86Assembler::REGISTER valueReg = CX86Assembler::rDX;

	m_assembler.MovEd(addressReg, MakeTemporaryReferenceSymbolAddress(src1));
	m_assembler.MovEd(valueReg, MakeMemorySymbolAddress(src2));
	m_assembler.MovGd(CX86Assembler::MakeIndRegAddress(addressReg), valueReg);
}

void CCodeGen_x86_32::Emit_StoreAtRef_TmpCst(const STATEMENT& statement)
{
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_TMP_REFERENCE);
	assert(src2->m_type == SYM_CONSTANT);

	CX86Assembler::REGISTER tmpReg = CX86Assembler::rAX;
	m_assembler.MovEd(tmpReg, MakeTemporaryReferenceSymbolAddress(src1));
	m_assembler.MovId(CX86Assembler::MakeIndRegAddress(tmpReg), src2->m_valueLow);
}
