#include "Jitter_CodeGen_x86_32.h"
#include <algorithm>
#include <stdexcept>

using namespace Jitter;

CX86Assembler::REGISTER CCodeGen_x86_32::g_registers[MAX_REGISTERS] =
{
	CX86Assembler::rBX,
	CX86Assembler::rSI,
	CX86Assembler::rDI,
};

CX86Assembler::XMMREGISTER CCodeGen_x86_32::g_mdRegisters[MAX_MDREGISTERS] =
{
	CX86Assembler::xMM4,
	CX86Assembler::xMM5,
	CX86Assembler::xMM6,
	CX86Assembler::xMM7
};

CCodeGen_x86_32::CONSTMATCHER CCodeGen_x86_32::g_constMatchers[] = 
{ 
	{ OP_PARAM,			MATCH_NIL,			MATCH_CONTEXT,		MATCH_NIL,			&CCodeGen_x86_32::Emit_Param_Ctx				},
	{ OP_PARAM,			MATCH_NIL,			MATCH_MEMORY,		MATCH_NIL,			&CCodeGen_x86_32::Emit_Param_Mem				},
	{ OP_PARAM,			MATCH_NIL,			MATCH_CONSTANT,		MATCH_NIL,			&CCodeGen_x86_32::Emit_Param_Cst				},
	{ OP_PARAM,			MATCH_NIL,			MATCH_REGISTER,		MATCH_NIL,			&CCodeGen_x86_32::Emit_Param_Reg				},
	{ OP_PARAM,			MATCH_NIL,			MATCH_MEMORY64,		MATCH_NIL,			&CCodeGen_x86_32::Emit_Param_Mem64				},
	{ OP_PARAM,			MATCH_NIL,			MATCH_CONSTANT64,	MATCH_NIL,			&CCodeGen_x86_32::Emit_Param_Cst64				},
	{ OP_PARAM,			MATCH_NIL,			MATCH_REGISTER128,	MATCH_NIL,			&CCodeGen_x86_32::Emit_Param_Reg128				},
	{ OP_PARAM,			MATCH_NIL,			MATCH_MEMORY128,	MATCH_NIL,			&CCodeGen_x86_32::Emit_Param_Mem128				},

	{ OP_PARAM_RET,		MATCH_NIL,			MATCH_MEMORY128,	MATCH_NIL,			&CCodeGen_x86_32::Emit_ParamRet_Mem128			},
	
	{ OP_CALL,			MATCH_NIL,			MATCH_RELATIVE,		MATCH_CONSTANT,		&CCodeGen_x86_32::Emit_Call_Rel						},
	{ OP_CALL,			MATCH_NIL,			MATCH_CONSTANTPTR,	MATCH_CONSTANT,		&CCodeGen_x86_32::Emit_Call						},

	{ OP_RETVAL,		MATCH_TEMPORARY,	MATCH_NIL,			MATCH_NIL,			&CCodeGen_x86_32::Emit_RetVal_Tmp				},
	{ OP_RETVAL,		MATCH_REGISTER,		MATCH_NIL,			MATCH_NIL,			&CCodeGen_x86_32::Emit_RetVal_Reg				},
	{ OP_RETVAL,		MATCH_MEMORY64,		MATCH_NIL,			MATCH_NIL,			&CCodeGen_x86_32::Emit_RetVal_Mem64				},

	{ OP_EXTERNJMP,		MATCH_NIL,			MATCH_CONSTANTPTR,	MATCH_NIL,			&CCodeGen_x86_32::Emit_ExternJmp				},
	{ OP_EXTERNJMP_DYN,	MATCH_NIL,			MATCH_CONSTANTPTR,	MATCH_NIL,			&CCodeGen_x86_32::Emit_ExternJmp				},

	{ OP_MOV,			MATCH_MEMORY64,		MATCH_MEMORY64,		MATCH_NIL,			&CCodeGen_x86_32::Emit_Mov_Mem64Mem64			},
	{ OP_MOV,			MATCH_MEMORY64,		MATCH_CONSTANT64,	MATCH_NIL,			&CCodeGen_x86_32::Emit_Mov_Mem64Cst64			},

	{ OP_MOV,			MATCH_REG_REF,		MATCH_MEM_REF,		MATCH_NIL,			&CCodeGen_x86_32::Emit_Mov_RegRefMemRef			},

	{ OP_ADD64,			MATCH_MEMORY64,		MATCH_MEMORY64,		MATCH_MEMORY64,		&CCodeGen_x86_32::Emit_Add64_MemMemMem			},
	{ OP_ADD64,			MATCH_MEMORY64,		MATCH_MEMORY64,		MATCH_CONSTANT64,	&CCodeGen_x86_32::Emit_Add64_MemMemCst			},

	{ OP_SUB64,			MATCH_MEMORY64,		MATCH_MEMORY64,		MATCH_MEMORY64,		&CCodeGen_x86_32::Emit_Sub64_MemMemMem			},
	{ OP_SUB64,			MATCH_MEMORY64,		MATCH_MEMORY64,		MATCH_CONSTANT64,	&CCodeGen_x86_32::Emit_Sub64_MemMemCst			},
	{ OP_SUB64,			MATCH_MEMORY64,		MATCH_CONSTANT64,	MATCH_MEMORY64,		&CCodeGen_x86_32::Emit_Sub64_MemCstMem			},

	{ OP_AND64,			MATCH_MEMORY64,		MATCH_MEMORY64,		MATCH_MEMORY64,		&CCodeGen_x86_32::Emit_And64_MemMemMem			},

	{ OP_SRL64,			MATCH_MEMORY64,		MATCH_MEMORY64,		MATCH_REGISTER,		&CCodeGen_x86_32::Emit_Srl64_MemMemReg			},
	{ OP_SRL64,			MATCH_MEMORY64,		MATCH_MEMORY64,		MATCH_MEMORY,		&CCodeGen_x86_32::Emit_Srl64_MemMemMem			},
	{ OP_SRL64,			MATCH_MEMORY64,		MATCH_MEMORY64,		MATCH_CONSTANT,		&CCodeGen_x86_32::Emit_Srl64_MemMemCst			},

	{ OP_SRA64,			MATCH_MEMORY64,		MATCH_MEMORY64,		MATCH_REGISTER,		&CCodeGen_x86_32::Emit_Sra64_MemMemReg			},
	{ OP_SRA64,			MATCH_MEMORY64,		MATCH_MEMORY64,		MATCH_MEMORY,		&CCodeGen_x86_32::Emit_Sra64_MemMemMem			},
	{ OP_SRA64,			MATCH_MEMORY64,		MATCH_MEMORY64,		MATCH_CONSTANT,		&CCodeGen_x86_32::Emit_Sra64_MemMemCst			},

	{ OP_SLL64,			MATCH_MEMORY64,		MATCH_MEMORY64,		MATCH_REGISTER,		&CCodeGen_x86_32::Emit_Sll64_MemMemReg			},
	{ OP_SLL64,			MATCH_MEMORY64,		MATCH_MEMORY64,		MATCH_MEMORY,		&CCodeGen_x86_32::Emit_Sll64_MemMemMem			},
	{ OP_SLL64,			MATCH_MEMORY64,		MATCH_MEMORY64,		MATCH_CONSTANT,		&CCodeGen_x86_32::Emit_Sll64_MemMemCst			},

	{ OP_CMP,			MATCH_VARIABLE,		MATCH_VARIABLE,		MATCH_VARIABLE,		&CCodeGen_x86_32::Emit_Cmp_VarVarVar			},
	{ OP_CMP,			MATCH_VARIABLE,		MATCH_VARIABLE,		MATCH_CONSTANT,		&CCodeGen_x86_32::Emit_Cmp_VarVarCst			},

	{ OP_CMP64,			MATCH_REGISTER,		MATCH_RELATIVE64,	MATCH_RELATIVE64,	&CCodeGen_x86_32::Emit_Cmp64_RegRelRel			},
	{ OP_CMP64,			MATCH_RELATIVE,		MATCH_RELATIVE64,	MATCH_RELATIVE64,	&CCodeGen_x86_32::Emit_Cmp64_RelRelRel			},
	{ OP_CMP64,			MATCH_REGISTER,		MATCH_RELATIVE64,	MATCH_CONSTANT64,	&CCodeGen_x86_32::Emit_Cmp64_RegRelCst			},
	{ OP_CMP64,			MATCH_RELATIVE,		MATCH_RELATIVE64,	MATCH_CONSTANT64,	&CCodeGen_x86_32::Emit_Cmp64_RelRelCst			},
	{ OP_CMP64,			MATCH_TEMPORARY,	MATCH_RELATIVE64,	MATCH_RELATIVE64,	&CCodeGen_x86_32::Emit_Cmp64_TmpRelRoc			},
	{ OP_CMP64,			MATCH_TEMPORARY,	MATCH_RELATIVE64,	MATCH_CONSTANT64,	&CCodeGen_x86_32::Emit_Cmp64_TmpRelRoc			},

	{ OP_RELTOREF,		MATCH_VAR_REF,		MATCH_CONSTANT,		MATCH_NIL,			&CCodeGen_x86_32::Emit_RelToRef_VarCst			},

	{ OP_ADDREF,		MATCH_VAR_REF,		MATCH_VAR_REF,		MATCH_VARIABLE,		&CCodeGen_x86_32::Emit_AddRef_VarVarVar			},
	{ OP_ADDREF,		MATCH_VAR_REF,		MATCH_VAR_REF,		MATCH_CONSTANT,		&CCodeGen_x86_32::Emit_AddRef_VarVarCst			},

	{ OP_ISREFNULL,		MATCH_VARIABLE,		MATCH_VAR_REF,		MATCH_NIL,			&CCodeGen_x86_32::Emit_IsRefNull_VarVar			},

	{ OP_LOADFROMREF,	MATCH_MEMORY64,		MATCH_VAR_REF,		MATCH_NIL,			&CCodeGen_x86_32::Emit_LoadFromRef_64_MemVar	},
	{ OP_LOADFROMREF,	MATCH_VAR_REF,		MATCH_VAR_REF,		MATCH_NIL,			&CCodeGen_x86_32::Emit_LoadFromRef_Ref_VarVar	},

	{ OP_STOREATREF,	MATCH_NIL,			MATCH_VAR_REF,		MATCH_MEMORY64,		&CCodeGen_x86_32::Emit_StoreAtRef_64_VarMem		},
	{ OP_STOREATREF,	MATCH_NIL,			MATCH_VAR_REF,		MATCH_CONSTANT64,	&CCodeGen_x86_32::Emit_StoreAtRef_64_VarCst		},

	{ OP_STORE8ATREF,	MATCH_NIL,			MATCH_VAR_REF,		MATCH_VARIABLE,		&CCodeGen_x86_32::Emit_Store8AtRef_VarVar		},

	{ OP_CONDJMP,		MATCH_NIL,			MATCH_VAR_REF,		MATCH_CONSTANT,		&CCodeGen_x86_32::Emit_CondJmp_Ref_VarCst		},

	{ OP_MOV,			MATCH_NIL,			MATCH_NIL,			MATCH_NIL,			NULL											},
};

CCodeGen_x86_32::CCodeGen_x86_32()
{
	CCodeGen_x86::m_registers = g_registers;
	CCodeGen_x86::m_mdRegisters = g_mdRegisters;

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

void CCodeGen_x86_32::SetImplicitRetValueParamFixUpRequired(bool implicitRetValueParamFixUpRequired)
{
	m_implicitRetValueParamFixUpRequired = implicitRetValueParamFixUpRequired;
}

void CCodeGen_x86_32::Emit_Prolog(const StatementList& statements, unsigned int stackSize)
{	
	//Compute the size needed to store all function call parameters
	uint32 maxParamSize = 0;
	uint32 maxParamSpillSize = 0;
	{
		uint32 currParamSize = 0;
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
						case SYM_CONTEXT:
						case SYM_REGISTER:
						case SYM_RELATIVE:
						case SYM_CONSTANT:
						case SYM_TEMPORARY:
						case SYM_RELATIVE128:
						case SYM_TEMPORARY128:
							currParamSize += 4;
							break;
						case SYM_REGISTER128:
							currParamSize += 4;
							currParamSpillSize += 16;
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
				maxParamSize = std::max<uint32>(currParamSize, maxParamSize);
				maxParamSpillSize = std::max<uint32>(currParamSpillSize, maxParamSpillSize);
				currParamSize = 0;
				currParamSpillSize = 0;
				break;
			}
		}
	}

	bool requiresMakeMdSzConstant =
		[&] ()
		{
			if(!m_hasSsse3) return false;
			for(const auto& statement : statements)
			{
				if(statement.op == OP_MD_MAKESZ)
				{
					return true;
				}
			}
			return false;
		}();

	assert((stackSize & 0x0F) == 0);
	assert((maxParamSpillSize & 0x0F) == 0);
	maxParamSize = ((maxParamSize + 0xF) & ~0xF);

	//Fetch parameter
	m_assembler.Push(CX86Assembler::rBP);
	m_assembler.MovEd(CX86Assembler::rBP, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, 8));

	//Save registers
	if(m_isTrampoline)
	{
		for(unsigned int i = 0; i < MAX_REGISTERS; i++)
		{
			m_assembler.Push(m_registers[i]);
		}
	}

	//Align stack
	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeRegisterAddress(CX86Assembler::rSP));
	m_assembler.AndId(CX86Assembler::MakeRegisterAddress(CX86Assembler::rSP), ~0x0F);
	m_assembler.SubId(CX86Assembler::MakeRegisterAddress(CX86Assembler::rSP), 0x0C);
	m_assembler.Push(CX86Assembler::rAX);

	m_literalStackAlloc = requiresMakeMdSzConstant ? 0x10 : 0;
	if(requiresMakeMdSzConstant)
	{
		m_mdMakeSzConstantOffset = 0;
		m_assembler.PushId(g_makeSzShufflePattern.w3);
		m_assembler.PushId(g_makeSzShufflePattern.w2);
		m_assembler.PushId(g_makeSzShufflePattern.w1);
		m_assembler.PushId(g_makeSzShufflePattern.w0);
	}

	//Allocate stack space for temps
	m_totalStackAlloc = stackSize + maxParamSize + maxParamSpillSize;
	m_paramSpillBase = stackSize + maxParamSize;
	m_stackLevel = maxParamSize;

	//Allocate stack space
	if(m_totalStackAlloc != 0)
	{
		m_assembler.SubId(CX86Assembler::MakeRegisterAddress(CX86Assembler::rSP), m_totalStackAlloc);
	}

	m_literalBase = m_totalStackAlloc;
	
	//-------------------------------
	//Stack Frame
	//-------------------------------
	//(High address)
	//------------------
	//Saved registers + alignment adjustment
	//------------------
	//Saved rSP
	//------------------			<----- aligned on 0x10
	//Literals
	//------------------			<----- rSP + m_literalsBase
	//Params spill space
	//------------------			<----- rSP + m_paramSpillBase
	//Temporary symbols (stackSize) + alignment adjustment
	//------------------			<----- rSP + m_stackLevel
	//Param space for callee
	//------------------			<----- rSP
	//(Low address)
}

void CCodeGen_x86_32::Emit_Epilog()
{
	if((m_totalStackAlloc != 0) || (m_literalStackAlloc != 0))
	{
		m_assembler.AddId(CX86Assembler::MakeRegisterAddress(CX86Assembler::rSP), m_totalStackAlloc + m_literalStackAlloc);
	}

	m_assembler.Pop(CX86Assembler::rSP);

	if(m_isTrampoline)
	{
		for(int i = MAX_REGISTERS - 1; i >= 0; i--)
		{
			m_assembler.Pop(m_registers[i]);
		}
	}

	m_assembler.Pop(CX86Assembler::rBP);
}

CX86Assembler::CAddress CCodeGen_x86_32::MakeConstant128Address(const LITERAL128& constant)
{
	assert(constant == g_makeSzShufflePattern);
	assert(m_mdMakeSzConstantOffset != -1);
	return CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, m_literalBase + m_mdMakeSzConstantOffset);
}

unsigned int CCodeGen_x86_32::GetAvailableRegisterCount() const
{
	return MAX_REGISTERS;
}

unsigned int CCodeGen_x86_32::GetAvailableMdRegisterCount() const
{
	return MAX_MDREGISTERS;
}

bool CCodeGen_x86_32::CanHold128BitsReturnValueInRegisters() const
{
	return false;
}

uint32 CCodeGen_x86_32::GetPointerSize() const
{
	return 4;
}

void CCodeGen_x86_32::Emit_Param_Ctx(const STATEMENT& statement)
{
	m_params.push_back(
		[this] (CALL_STATE& state)
		{
			m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, state.paramOffset), CX86Assembler::rBP);
			state.paramOffset += 4;
		}
	);
}

void CCodeGen_x86_32::Emit_Param_Reg(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	m_params.push_back(
		[this, src1] (CALL_STATE& state)
		{
			m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, state.paramOffset), m_registers[src1->m_valueLow]);
			state.paramOffset += 4;
		}
	);
}

void CCodeGen_x86_32::Emit_Param_Mem(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	m_params.push_back(
		[this, src1] (CALL_STATE& state)
		{
			m_assembler.MovEd(CX86Assembler::rAX, MakeMemorySymbolAddress(src1));
			m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, state.paramOffset), CX86Assembler::rAX);
			state.paramOffset += 4;
		}
	);
}

void CCodeGen_x86_32::Emit_Param_Cst(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	m_params.push_back(
		[this, src1] (CALL_STATE& state)
		{
			m_assembler.MovId(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, state.paramOffset), src1->m_valueLow);
			state.paramOffset += 4;
		}
	);
}

void CCodeGen_x86_32::Emit_Param_Mem64(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	m_params.push_back(
		[this, src1] (CALL_STATE& state)
		{
			m_assembler.MovEd(CX86Assembler::rAX, MakeMemory64SymbolLoAddress(src1));
			m_assembler.MovEd(CX86Assembler::rDX, MakeMemory64SymbolHiAddress(src1));
			m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, state.paramOffset + 0), CX86Assembler::rAX);
			m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, state.paramOffset + 4), CX86Assembler::rDX);
			state.paramOffset += 8;
		}
	);
}

void CCodeGen_x86_32::Emit_Param_Cst64(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	m_params.push_back(
		[this, src1] (CALL_STATE& state)
		{
			m_assembler.MovId(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, state.paramOffset + 0), src1->m_valueLow);
			m_assembler.MovId(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, state.paramOffset + 4), src1->m_valueHigh);
			state.paramOffset += 8;
		}
	);
}

void CCodeGen_x86_32::Emit_Param_Reg128(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	m_params.push_back(
		[this, src1] (CALL_STATE& state)
		{
			auto paramTempAddr = CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, m_paramSpillBase + state.paramSpillOffset);

			m_assembler.MovapsVo(paramTempAddr, m_mdRegisters[src1->m_valueLow]);
			m_assembler.LeaGd(CX86Assembler::rAX, paramTempAddr);
			m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, state.paramOffset), CX86Assembler::rAX);

			state.paramOffset += 4;
			state.paramSpillOffset += 0x10;
		}
	);
}

void CCodeGen_x86_32::Emit_Param_Mem128(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	m_params.push_back(
		[this, src1] (CALL_STATE& state)
		{
			m_assembler.LeaGd(CX86Assembler::rAX, MakeMemory128SymbolAddress(src1));
			m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, state.paramOffset), CX86Assembler::rAX);
			state.paramOffset += 4;
		}
	);
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

void CCodeGen_x86_32::Emit_Call_Rel(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	uint32 paramCount = src2->m_valueLow;
	CALL_STATE callState;
	for(unsigned int i = 0; i < paramCount; i++)
	{
		auto emitter(m_params.back());
		m_params.pop_back();
		emitter(callState);
	}

	m_assembler.MovEd(CX86Assembler::rAX, MakeRelativeSymbolAddress(src1));
	m_assembler.CallEd(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX));

	if(m_hasImplicitRetValueParam && m_implicitRetValueParamFixUpRequired)
	{
		//Allocated stack space for the extra parameter is cleaned up by the callee. 
		//So adjust the amount of stack space we free up after the call
		m_assembler.SubId(CX86Assembler::MakeRegisterAddress(CX86Assembler::rSP), 4);
	}

	m_hasImplicitRetValueParam = false;
}

void CCodeGen_x86_32::Emit_Call(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	
	uint32 paramCount = src2->m_valueLow;
	CALL_STATE callState;
	for(unsigned int i = 0; i < paramCount; i++)
	{
		auto emitter(m_params.back());
		m_params.pop_back();
		emitter(callState);
	}
	
	m_assembler.MovId(CX86Assembler::rAX, src1->m_valueLow);
	auto symbolRefLabel = m_assembler.CreateLabel();
	m_assembler.MarkLabel(symbolRefLabel, -4);
	m_symbolReferenceLabels.push_back(std::make_pair(src1->GetConstantPtr(), symbolRefLabel));
	m_assembler.CallEd(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX));
	
	if(m_hasImplicitRetValueParam && m_implicitRetValueParamFixUpRequired)
	{
		//Allocated stack space for the extra parameter is cleaned up by the callee. 
		//So adjust the amount of stack space we free up after the call
		m_assembler.SubId(CX86Assembler::MakeRegisterAddress(CX86Assembler::rSP), 4);
	}
	
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

void CCodeGen_x86_32::Emit_ExternJmp(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	
	Emit_Epilog();
	m_assembler.MovId(CX86Assembler::rAX, src1->m_valueLow);
	auto symbolRefLabel = m_assembler.CreateLabel();
	m_assembler.MarkLabel(symbolRefLabel, -4);
	m_symbolReferenceLabels.push_back(std::make_pair(src1->GetConstantPtr(), symbolRefLabel));
	m_assembler.JmpEd(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX));
}

void CCodeGen_x86_32::Emit_Mov_Mem64Mem64(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	m_assembler.MovEd(CX86Assembler::rAX, MakeMemory64SymbolLoAddress(src1));
	m_assembler.MovEd(CX86Assembler::rDX, MakeMemory64SymbolHiAddress(src1));

	m_assembler.MovGd(MakeMemory64SymbolLoAddress(dst), CX86Assembler::rAX);
	m_assembler.MovGd(MakeMemory64SymbolHiAddress(dst), CX86Assembler::rDX);
}

void CCodeGen_x86_32::Emit_Mov_Mem64Cst64(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(src1->m_type == SYM_CONSTANT64);

	m_assembler.MovId(MakeMemory64SymbolLoAddress(dst), src1->m_valueLow);
	m_assembler.MovId(MakeMemory64SymbolHiAddress(dst), src1->m_valueHigh);
}

void CCodeGen_x86_32::Emit_Mov_RegRefMemRef(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type == SYM_REG_REFERENCE);

	m_assembler.MovEd(m_registers[dst->m_valueLow], MakeMemoryReferenceSymbolAddress(src1));
}

void CCodeGen_x86_32::Emit_Add64_MemMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	m_assembler.MovEd(CX86Assembler::rAX, MakeMemory64SymbolLoAddress(src1));
	m_assembler.MovEd(CX86Assembler::rDX, MakeMemory64SymbolHiAddress(src1));

	m_assembler.AddEd(CX86Assembler::rAX, MakeMemory64SymbolLoAddress(src2));
	m_assembler.AdcEd(CX86Assembler::rDX, MakeMemory64SymbolHiAddress(src2));

	m_assembler.MovGd(MakeMemory64SymbolLoAddress(dst), CX86Assembler::rAX);
	m_assembler.MovGd(MakeMemory64SymbolHiAddress(dst), CX86Assembler::rDX);
}

void CCodeGen_x86_32::Emit_Add64_MemMemCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_CONSTANT64);

	m_assembler.MovEd(CX86Assembler::rAX, MakeMemory64SymbolLoAddress(src1));
	m_assembler.MovEd(CX86Assembler::rDX, MakeMemory64SymbolHiAddress(src1));

	m_assembler.AddId(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX), src2->m_valueLow);
	m_assembler.AdcId(CX86Assembler::MakeRegisterAddress(CX86Assembler::rDX), src2->m_valueHigh);

	m_assembler.MovGd(MakeMemory64SymbolLoAddress(dst), CX86Assembler::rAX);
	m_assembler.MovGd(MakeMemory64SymbolHiAddress(dst), CX86Assembler::rDX);
}

void CCodeGen_x86_32::Emit_Sub64_MemMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	m_assembler.MovEd(CX86Assembler::rAX, MakeMemory64SymbolLoAddress(src1));
	m_assembler.MovEd(CX86Assembler::rDX, MakeMemory64SymbolHiAddress(src1));

	m_assembler.SubEd(CX86Assembler::rAX, MakeMemory64SymbolLoAddress(src2));
	m_assembler.SbbEd(CX86Assembler::rDX, MakeMemory64SymbolHiAddress(src2));

	m_assembler.MovGd(MakeMemory64SymbolLoAddress(dst), CX86Assembler::rAX);
	m_assembler.MovGd(MakeMemory64SymbolHiAddress(dst), CX86Assembler::rDX);
}

void CCodeGen_x86_32::Emit_Sub64_MemMemCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_CONSTANT64);

	m_assembler.MovEd(CX86Assembler::rAX, MakeMemory64SymbolLoAddress(src1));
	m_assembler.MovEd(CX86Assembler::rDX, MakeMemory64SymbolHiAddress(src1));

	m_assembler.SubId(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX), src2->m_valueLow);
	m_assembler.SbbId(CX86Assembler::MakeRegisterAddress(CX86Assembler::rDX), src2->m_valueHigh);

	m_assembler.MovGd(MakeMemory64SymbolLoAddress(dst), CX86Assembler::rAX);
	m_assembler.MovGd(MakeMemory64SymbolHiAddress(dst), CX86Assembler::rDX);
}

void CCodeGen_x86_32::Emit_Sub64_MemCstMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_CONSTANT64);

	m_assembler.MovId(CX86Assembler::rAX, src1->m_valueLow);
	m_assembler.MovId(CX86Assembler::rDX, src1->m_valueHigh);

	m_assembler.SubEd(CX86Assembler::rAX, MakeMemory64SymbolLoAddress(src2));
	m_assembler.SbbEd(CX86Assembler::rDX, MakeMemory64SymbolHiAddress(src2));

	m_assembler.MovGd(MakeMemory64SymbolLoAddress(dst), CX86Assembler::rAX);
	m_assembler.MovGd(MakeMemory64SymbolHiAddress(dst), CX86Assembler::rDX);
}

void CCodeGen_x86_32::Emit_And64_MemMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	m_assembler.MovEd(CX86Assembler::rAX, MakeMemory64SymbolLoAddress(src1));
	m_assembler.MovEd(CX86Assembler::rDX, MakeMemory64SymbolHiAddress(src1));

	m_assembler.AndEd(CX86Assembler::rAX, MakeMemory64SymbolLoAddress(src2));
	m_assembler.AndEd(CX86Assembler::rDX, MakeMemory64SymbolHiAddress(src2));

	m_assembler.MovGd(MakeMemory64SymbolLoAddress(dst), CX86Assembler::rAX);
	m_assembler.MovGd(MakeMemory64SymbolHiAddress(dst), CX86Assembler::rDX);
}

//---------------------------------------------------------------------------------
//SR64
//---------------------------------------------------------------------------------

void CCodeGen_x86_32::Emit_Sr64Var_MemMem(CSymbol* dst, CSymbol* src1, CX86Assembler::REGISTER shiftRegister, SHIFTRIGHT_TYPE shiftType)
{
	auto regLo = CX86Assembler::rAX;
	auto regHi = CX86Assembler::rDX;
	auto regSa = CX86Assembler::rCX;

	auto lessThan32Label = m_assembler.CreateLabel();
	auto endLabel = m_assembler.CreateLabel();

	if(shiftRegister != regSa)
	{
		m_assembler.MovEd(regSa, CX86Assembler::MakeRegisterAddress(shiftRegister));
	}

	m_assembler.AndId(CX86Assembler::MakeRegisterAddress(regSa), 0x3F);
	m_assembler.CmpId(CX86Assembler::MakeRegisterAddress(regSa), 32);
	m_assembler.JbJx(lessThan32Label);

	//greaterOrEqual:
	//---------------------------------------------
	m_assembler.MovEd(regLo, MakeMemory64SymbolHiAddress(src1));
	m_assembler.AndId(CX86Assembler::MakeRegisterAddress(regSa), 0x1F);

	if(shiftType == SHIFTRIGHT_LOGICAL)
	{
		m_assembler.ShrEd(CX86Assembler::MakeRegisterAddress(regLo));
		m_assembler.XorEd(regHi, CX86Assembler::MakeRegisterAddress(regHi));
	}
	else if(shiftType == SHIFTRIGHT_ARITHMETIC)
	{
		m_assembler.MovEd(regHi, CX86Assembler::MakeRegisterAddress(regLo));
		m_assembler.SarEd(CX86Assembler::MakeRegisterAddress(regHi), 31);
		m_assembler.SarEd(CX86Assembler::MakeRegisterAddress(regLo));
	}
	else
	{
		assert(false);
	}

	m_assembler.JmpJx(endLabel);

	//lessThan:
	//---------------------------------------------
	m_assembler.MarkLabel(lessThan32Label);

	m_assembler.MovEd(regLo, MakeMemory64SymbolLoAddress(src1));
	m_assembler.MovEd(regHi, MakeMemory64SymbolHiAddress(src1));

	m_assembler.ShrdEd(CX86Assembler::MakeRegisterAddress(regLo), regHi);
	if(shiftType == SHIFTRIGHT_LOGICAL)
	{
		m_assembler.ShrEd(CX86Assembler::MakeRegisterAddress(regHi));
	}
	else if(shiftType == SHIFTRIGHT_ARITHMETIC)
	{
		m_assembler.SarEd(CX86Assembler::MakeRegisterAddress(regHi));
	}
	else
	{
		assert(false);
	}

	//end:
	//---------------------------------------------
	m_assembler.MarkLabel(endLabel);

	m_assembler.MovGd(MakeMemory64SymbolLoAddress(dst), regLo);
	m_assembler.MovGd(MakeMemory64SymbolHiAddress(dst), regHi);
}

void CCodeGen_x86_32::Emit_Sr64Cst_MemMem(CSymbol* dst, CSymbol* src1, uint32 shiftAmount, SHIFTRIGHT_TYPE shiftType)
{
	assert(dst->m_type  == SYM_RELATIVE64);
	assert(src1->m_type == SYM_RELATIVE64);

	shiftAmount = shiftAmount & 0x3F;

	auto regLo = CX86Assembler::rAX;
	auto regHi = CX86Assembler::rDX;

	if(shiftAmount >= 32)
	{
		m_assembler.MovEd(regLo, MakeMemory64SymbolHiAddress(src1));

		if(shiftType == SHIFTRIGHT_LOGICAL)
		{
			if(shiftAmount != 32)
			{
				//shr reg, amount
				m_assembler.ShrEd(CX86Assembler::MakeRegisterAddress(regLo), shiftAmount & 0x1F);
			}

			m_assembler.XorEd(regHi, CX86Assembler::MakeRegisterAddress(regHi));
		}
		else if(shiftType == SHIFTRIGHT_ARITHMETIC)
		{
			if(shiftAmount != 32)
			{
				//sar reg, amount
				m_assembler.SarEd(CX86Assembler::MakeRegisterAddress(regLo), shiftAmount & 0x1F);
			}

			m_assembler.MovEd(regHi, CX86Assembler::MakeRegisterAddress(regLo));
			m_assembler.SarEd(CX86Assembler::MakeRegisterAddress(regHi), 31);
		}
		else
		{
			assert(false);
		}
	}
	else //Amount < 32
	{
		m_assembler.MovEd(regLo, MakeMemory64SymbolLoAddress(src1));
		m_assembler.MovEd(regHi, MakeMemory64SymbolHiAddress(src1));

		if(shiftAmount != 0)
		{
			//shrd nReg1, nReg2, nAmount
			m_assembler.ShrdEd(CX86Assembler::MakeRegisterAddress(regLo), regHi, shiftAmount);

			if(shiftType == SHIFTRIGHT_LOGICAL)
			{
				//shr nReg2, nAmount
				m_assembler.ShrEd(CX86Assembler::MakeRegisterAddress(regHi), shiftAmount);
			}
			else if(shiftType == SHIFTRIGHT_ARITHMETIC)
			{
				//sar nReg2, nAmount
				m_assembler.SarEd(CX86Assembler::MakeRegisterAddress(regHi), shiftAmount);
			}
			else
			{
				assert(false);
			}
		}
	}

	m_assembler.MovGd(MakeMemory64SymbolLoAddress(dst), regLo);
	m_assembler.MovGd(MakeMemory64SymbolHiAddress(dst), regHi);
}

//---------------------------------------------------------------------------------
//SRL64
//---------------------------------------------------------------------------------

void CCodeGen_x86_32::Emit_Srl64_MemMemReg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_REGISTER);

	Emit_Sr64Var_MemMem(dst, src1, g_registers[src2->m_valueLow], SHIFTRIGHT_LOGICAL);
}

void CCodeGen_x86_32::Emit_Srl64_MemMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto shiftAmount = CX86Assembler::rCX;
	m_assembler.MovEd(shiftAmount, MakeMemorySymbolAddress(src2));

	Emit_Sr64Var_MemMem(dst, src1, shiftAmount, SHIFTRIGHT_LOGICAL);
}

void CCodeGen_x86_32::Emit_Srl64_MemMemCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	Emit_Sr64Cst_MemMem(dst, src1, src2->m_valueLow, SHIFTRIGHT_LOGICAL);
}

//---------------------------------------------------------------------------------
//SRA64
//---------------------------------------------------------------------------------

void CCodeGen_x86_32::Emit_Sra64_MemMemReg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_REGISTER);

	Emit_Sr64Var_MemMem(dst, src1, g_registers[src2->m_valueLow], SHIFTRIGHT_ARITHMETIC);
}

void CCodeGen_x86_32::Emit_Sra64_MemMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto shiftAmount = CX86Assembler::rCX;
	m_assembler.MovEd(shiftAmount, MakeMemorySymbolAddress(src2));

	Emit_Sr64Var_MemMem(dst, src1, shiftAmount, SHIFTRIGHT_ARITHMETIC);
}

void CCodeGen_x86_32::Emit_Sra64_MemMemCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	Emit_Sr64Cst_MemMem(dst, src1, src2->m_valueLow, SHIFTRIGHT_ARITHMETIC);
}

//---------------------------------------------------------------------------------
//SLL64
//---------------------------------------------------------------------------------

void CCodeGen_x86_32::Emit_Sll64_MemMemVar(const STATEMENT& statement, CX86Assembler::REGISTER shiftRegister)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	CX86Assembler::LABEL doneLabel = m_assembler.CreateLabel();
	CX86Assembler::LABEL more32Label = m_assembler.CreateLabel();

	auto amountReg = CX86Assembler::rCX;
	auto amountRegByte = CX86Assembler::GetByteRegister(amountReg);
	auto resultLow = CX86Assembler::rAX;
	auto resultHigh = CX86Assembler::rDX;

	if(shiftRegister != amountReg)
	{
		m_assembler.MovEd(amountReg, CX86Assembler::MakeRegisterAddress(shiftRegister));
	}

	m_assembler.MovEd(resultLow, MakeMemory64SymbolLoAddress(src1));
	m_assembler.MovEd(resultHigh, MakeMemory64SymbolHiAddress(src1));

	m_assembler.AndIb(CX86Assembler::MakeByteRegisterAddress(amountRegByte), 0x3F);
	m_assembler.TestEb(amountRegByte, CX86Assembler::MakeByteRegisterAddress(amountRegByte));
	m_assembler.JzJx(doneLabel);

	m_assembler.CmpIb(CX86Assembler::MakeByteRegisterAddress(amountRegByte), 0x20);
	m_assembler.JnbJx(more32Label);

	m_assembler.ShldEd(CX86Assembler::MakeRegisterAddress(resultHigh), resultLow);
	m_assembler.ShlEd(CX86Assembler::MakeRegisterAddress(resultLow));
	m_assembler.JmpJx(doneLabel);

//$more32
	m_assembler.MarkLabel(more32Label);

	m_assembler.MovEd(resultHigh, CX86Assembler::MakeRegisterAddress(resultLow));
	m_assembler.XorEd(resultLow, CX86Assembler::MakeRegisterAddress(resultLow));
	m_assembler.AndIb(CX86Assembler::MakeByteRegisterAddress(amountRegByte), 0x1F);
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

void CCodeGen_x86_32::Emit_Sll64_MemMemCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_CONSTANT);

	uint8 shiftAmount = static_cast<uint8>(src2->m_valueLow & 0x3F);

	auto regLo = CX86Assembler::rAX;
	auto regHi = CX86Assembler::rDX;

	if(shiftAmount >= 32)
	{
		m_assembler.MovEd(regHi, MakeMemory64SymbolLoAddress(src1));

		if(shiftAmount != 0)
		{
			//shl reg, amount
			m_assembler.ShlEd(CX86Assembler::MakeRegisterAddress(regHi), shiftAmount & 0x1F);
		}

		m_assembler.XorEd(regLo, CX86Assembler::MakeRegisterAddress(regLo));
	}
	else //Amount < 32
	{
		m_assembler.MovEd(regLo, MakeMemory64SymbolLoAddress(src1));
		m_assembler.MovEd(regHi, MakeMemory64SymbolHiAddress(src1));

		//shld nReg2, nReg1, nAmount
		m_assembler.ShldEd(CX86Assembler::MakeRegisterAddress(regHi), regLo, shiftAmount);

		//shl nReg1, nAmount
		m_assembler.ShlEd(CX86Assembler::MakeRegisterAddress(regLo), shiftAmount);
	}

	m_assembler.MovGd(MakeMemory64SymbolLoAddress(dst), regLo);
	m_assembler.MovGd(MakeMemory64SymbolHiAddress(dst), regHi);
}

void CCodeGen_x86_32::Emit_Cmp_VarVarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDef(dst, CX86Assembler::rCX);
	auto src1Reg = PrepareSymbolRegisterUse(src1, CX86Assembler::rDX);
	auto cmpReg = CX86Assembler::bAL;

	m_assembler.CmpEd(src1Reg, MakeVariableSymbolAddress(src2));
	Cmp_GetFlag(CX86Assembler::MakeByteRegisterAddress(cmpReg), statement.jmpCondition);
	m_assembler.MovzxEb(dstReg, CX86Assembler::MakeByteRegisterAddress(cmpReg));

	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_x86_32::Emit_Cmp_VarVarCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDef(dst, CX86Assembler::rCX);
	auto cmpReg = CX86Assembler::bAL;

	m_assembler.CmpId(MakeVariableSymbolAddress(src1), src2->m_valueLow);
	Cmp_GetFlag(CX86Assembler::MakeByteRegisterAddress(cmpReg), statement.jmpCondition);
	m_assembler.MovzxEb(dstReg, CX86Assembler::MakeByteRegisterAddress(cmpReg));

	CommitSymbolRegister(dst, dstReg);
}

//---------------------------------------------------------------------------------
//CMP64
//---------------------------------------------------------------------------------

void CCodeGen_x86_32::Cmp64_Equal(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	const auto cmpLo = 
		[this](CX86Assembler::REGISTER registerId, CSymbol* symbol)
		{
			switch(symbol->m_type)
			{
			case SYM_RELATIVE64:
				m_assembler.CmpEd(registerId, MakeMemory64SymbolLoAddress(symbol));
				break;
			case SYM_CONSTANT64:
				if(symbol->m_valueLow == 0)
				{
					m_assembler.TestEd(registerId, CX86Assembler::MakeRegisterAddress(registerId));
				}
				else
				{
					m_assembler.CmpId(CX86Assembler::MakeRegisterAddress(registerId), symbol->m_valueLow);
				}
				break;
			default:
				assert(0);
				break;
			}
		};

	const auto cmpHi =
		[this](CX86Assembler::REGISTER registerId, CSymbol* symbol)
		{
			switch(symbol->m_type)
			{
			case SYM_RELATIVE64:
				m_assembler.CmpEd(registerId, MakeMemory64SymbolHiAddress(symbol));
				break;
			case SYM_CONSTANT64:
				if(symbol->m_valueHigh == 0)
				{
					m_assembler.TestEd(registerId, CX86Assembler::MakeRegisterAddress(registerId));
				}
				else
				{
					m_assembler.CmpId(CX86Assembler::MakeRegisterAddress(registerId), symbol->m_valueHigh);
				}
				break;
			default:
				assert(0);
				break;
			}
		};

	assert(src1->m_type == SYM_RELATIVE64);

	bool isEqual	= (statement.jmpCondition == CONDITION_EQ);

	auto valReg = CX86Assembler::rDX;
	auto res1Reg = CX86Assembler::rAX;
	auto res2Reg = CX86Assembler::rCX;
	auto res1RegByte = CX86Assembler::GetByteRegister(res1Reg);
	auto res2RegByte = CX86Assembler::GetByteRegister(res2Reg);

	m_assembler.MovEd(valReg, MakeMemory64SymbolLoAddress(src1));
	cmpLo(valReg, src2);

	if(isEqual)
	{
		m_assembler.SeteEb(CX86Assembler::MakeByteRegisterAddress(res1RegByte));
	}
	else
	{
		m_assembler.SetneEb(CX86Assembler::MakeByteRegisterAddress(res1RegByte));
	}
	
	m_assembler.MovEd(valReg, MakeMemory64SymbolHiAddress(src1));
	cmpHi(valReg, src2);

	if(isEqual)
	{
		m_assembler.SeteEb(CX86Assembler::MakeByteRegisterAddress(res2RegByte));
	}
	else
	{
		m_assembler.SetneEb(CX86Assembler::MakeByteRegisterAddress(res2RegByte));
	}

	if(isEqual)
	{
		m_assembler.AndEd(res1Reg, CX86Assembler::MakeRegisterAddress(res2Reg));
	}
	else
	{
		m_assembler.OrEd(res1Reg, CX86Assembler::MakeRegisterAddress(res2Reg));
	}

	m_assembler.MovzxEb(res1Reg, CX86Assembler::MakeByteRegisterAddress(res1RegByte));
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
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	CompareTraits compareTraits;
	(void)compareTraits;

	const auto cmpLo =
		[this](CX86Assembler::REGISTER registerId, CSymbol* symbol)
		{
			switch(symbol->m_type)
			{
			case SYM_RELATIVE64:
				m_assembler.CmpEd(registerId, MakeMemory64SymbolLoAddress(symbol));
				break;
			case SYM_CONSTANT64:
				m_assembler.CmpId(CX86Assembler::MakeRegisterAddress(registerId), symbol->m_valueLow);
				break;
			default:
				assert(0);
				break;
			}
		};

	const auto cmpHi =
		[this](CX86Assembler::REGISTER registerId, CSymbol* symbol)
		{
			switch(symbol->m_type)
			{
			case SYM_RELATIVE64:
				m_assembler.CmpEd(registerId, MakeMemory64SymbolHiAddress(symbol));
				break;
			case SYM_CONSTANT64:
				m_assembler.CmpId(CX86Assembler::MakeRegisterAddress(registerId), symbol->m_valueHigh);
				break;
			default:
				assert(0);
				break;
			}
		};

	assert(src1->m_type == SYM_RELATIVE64);

	auto regLo = CX86Assembler::rAX;
	auto regHi = CX86Assembler::rDX;
	auto regLoCmp = CX86Assembler::GetByteRegister(regLo);

	bool isSigned	= compareTraits.IsSigned(statement.jmpCondition);
	bool orEqual	= compareTraits.OrEqual(statement.jmpCondition);

	CX86Assembler::LABEL highOrderEqualLabel = m_assembler.CreateLabel();
	CX86Assembler::LABEL doneLabel = m_assembler.CreateLabel();

	/////////////////////////////////////////
	//Check high order word if equal

	m_assembler.MovEd(regHi, MakeMemory64SymbolHiAddress(src1));
	cmpHi(regHi, src2);

	//je highOrderEqual
	m_assembler.JzJx(highOrderEqualLabel);

	///////////////////////////////////////////////////////////
	//If they aren't equal, this comparaison decides of result

	//setb/l reg[l]
	if(isSigned)
	{
		((m_assembler).*(compareTraits.CheckOrderSigned()))(CX86Assembler::MakeByteRegisterAddress(regLoCmp));
	}
	else
	{
		((m_assembler).*(compareTraits.CheckOrderUnsigned()))(CX86Assembler::MakeByteRegisterAddress(regLoCmp));
	}

	//movzx reg, reg[l]
	m_assembler.MovzxEb(regLo, CX86Assembler::MakeByteRegisterAddress(regLoCmp));

	//jmp done
	m_assembler.JmpJx(doneLabel);

	//highOrderEqual: /////////////////////////////////////
	m_assembler.MarkLabel(highOrderEqualLabel);
	//If they are equal, next comparaison decides of result

	m_assembler.MovEd(regLo, MakeMemory64SymbolLoAddress(src1));
	cmpLo(regLo, src2);

	//setb/be reg[l]
	if(orEqual)
	{
		((m_assembler).*(compareTraits.CheckOrderOrEqualUnsigned()))(CX86Assembler::MakeByteRegisterAddress(regLoCmp));
	}
	else
	{
		((m_assembler).*(compareTraits.CheckOrderUnsigned()))(CX86Assembler::MakeByteRegisterAddress(regLoCmp));
	}

	//movzx reg, reg[l]
	m_assembler.MovzxEb(regLo, CX86Assembler::MakeByteRegisterAddress(regLoCmp));

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

void CCodeGen_x86_32::Emit_RelToRef_VarCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(src1->m_type == SYM_CONSTANT);

	auto dstReg = PrepareRefSymbolRegisterDef(dst, CX86Assembler::rAX);
	m_assembler.LeaGd(dstReg, CX86Assembler::MakeIndRegOffAddress(g_baseRegister, src1->m_valueLow));
	CommitRefSymbolRegister(dst, dstReg);
}

void CCodeGen_x86_32::Emit_AddRef_VarVarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto offsetReg = PrepareSymbolRegisterUse(src2, CX86Assembler::rCX);
	auto dstReg = PrepareRefSymbolRegisterDef(dst, CX86Assembler::rAX);

	if(!dst->Equals(src1))
	{
		m_assembler.MovEd(dstReg, MakeVariableReferenceSymbolAddress(src1));
	}

	m_assembler.AddEd(dstReg, CX86Assembler::MakeRegisterAddress(offsetReg));
	CommitRefSymbolRegister(dst, dstReg);
}

void CCodeGen_x86_32::Emit_AddRef_VarVarCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_CONSTANT);

	auto dstReg = PrepareRefSymbolRegisterDef(dst, CX86Assembler::rAX);

	if(!dst->Equals(src1))
	{
		m_assembler.MovEd(dstReg, MakeVariableReferenceSymbolAddress(src1));
	}

	m_assembler.AddId(CX86Assembler::MakeRegisterAddress(dstReg), src2->m_valueLow);
	CommitRefSymbolRegister(dst, dstReg);
}

void CCodeGen_x86_32::Emit_IsRefNull_VarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto addressReg = PrepareRefSymbolRegisterUse(src1, CX86Assembler::rAX);
	auto tstReg = CX86Assembler::bCL;
	auto dstReg = PrepareSymbolRegisterDef(dst, CX86Assembler::rDX);

	m_assembler.TestEd(addressReg, CX86Assembler::MakeRegisterAddress(addressReg));
	m_assembler.SeteEb(CX86Assembler::MakeByteRegisterAddress(tstReg));
	m_assembler.MovzxEb(dstReg, CX86Assembler::MakeByteRegisterAddress(tstReg));

	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_x86_32::Emit_LoadFromRef_64_MemVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto addressReg = PrepareRefSymbolRegisterUse(src1, CX86Assembler::rDX);
	auto dstLoReg = CX86Assembler::rAX;
	auto dstHiReg = CX86Assembler::rCX;

	m_assembler.MovEd(dstLoReg, CX86Assembler::MakeIndRegAddress(addressReg));
	m_assembler.MovEd(dstHiReg, CX86Assembler::MakeIndRegOffAddress(addressReg, 4));
	m_assembler.MovGd(MakeMemory64SymbolLoAddress(dst), dstLoReg);
	m_assembler.MovGd(MakeMemory64SymbolHiAddress(dst), dstHiReg);
}

void CCodeGen_x86_32::Emit_LoadFromRef_Ref_VarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto addressReg = PrepareRefSymbolRegisterUse(src1, CX86Assembler::rAX);
	auto dstReg = PrepareRefSymbolRegisterDef(dst, CX86Assembler::rDX);

	m_assembler.MovEd(dstReg, CX86Assembler::MakeIndRegAddress(addressReg));

	CommitRefSymbolRegister(dst, dstReg);
}

void CCodeGen_x86_32::Emit_StoreAtRef_64_VarMem(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto addressReg = PrepareRefSymbolRegisterUse(src1, CX86Assembler::rDX);
	auto valueLoReg = CX86Assembler::rAX;
	auto valueHiReg = CX86Assembler::rCX;

	m_assembler.MovEd(valueLoReg, MakeMemory64SymbolLoAddress(src2));
	m_assembler.MovEd(valueHiReg, MakeMemory64SymbolHiAddress(src2));
	m_assembler.MovGd(CX86Assembler::MakeIndRegAddress(addressReg), valueLoReg);
	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(addressReg, 4), valueHiReg);
}

void CCodeGen_x86_32::Emit_StoreAtRef_64_VarCst(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto addressReg = PrepareRefSymbolRegisterUse(src1, CX86Assembler::rDX);
	auto valueLoReg = CX86Assembler::rAX;
	auto valueHiReg = CX86Assembler::rCX;

	assert(src2->m_type == SYM_CONSTANT64);

	m_assembler.MovId(valueLoReg, src2->m_valueLow);
	m_assembler.MovId(valueHiReg, src2->m_valueHigh);
	m_assembler.MovGd(CX86Assembler::MakeIndRegAddress(addressReg), valueLoReg);
	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(addressReg, 4), valueHiReg);
}

void CCodeGen_x86_32::Emit_Store8AtRef_VarVar(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto addressReg = PrepareRefSymbolRegisterUse(src1, CX86Assembler::rAX);
	auto valueReg = PrepareSymbolByteRegisterUse(src2, CX86Assembler::rDX);
	m_assembler.MovGb(CX86Assembler::MakeIndRegAddress(addressReg), valueReg);
}

void CCodeGen_x86_32::Emit_CondJmp_Ref_VarCst(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_CONSTANT);
	assert(src2->m_valueLow == 0);
	assert((statement.jmpCondition == CONDITION_NE) || (statement.jmpCondition == CONDITION_EQ));

	auto addressReg = PrepareRefSymbolRegisterUse(src1, CX86Assembler::rAX);

	m_assembler.TestEd(addressReg, CX86Assembler::MakeRegisterAddress(addressReg));

	CondJmp_JumpTo(GetLabel(statement.jmpBlock), statement.jmpCondition);
}

CX86Assembler::REGISTER CCodeGen_x86_32::PrepareRefSymbolRegisterDef(CSymbol* symbol, CX86Assembler::REGISTER preferedRegister)
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

CX86Assembler::REGISTER CCodeGen_x86_32::PrepareRefSymbolRegisterUse(CSymbol* symbol, CX86Assembler::REGISTER preferedRegister)
{
	switch(symbol->m_type)
	{
	case SYM_REG_REFERENCE:
		return m_registers[symbol->m_valueLow];
		break;
	case SYM_TMP_REFERENCE:
	case SYM_REL_REFERENCE:
		m_assembler.MovEd(preferedRegister, MakeMemoryReferenceSymbolAddress(symbol));
		return preferedRegister;
		break;
	default:
		throw std::runtime_error("Invalid symbol type.");
		break;
	}
}

void CCodeGen_x86_32::CommitRefSymbolRegister(CSymbol* symbol, CX86Assembler::REGISTER usedRegister)
{
	switch(symbol->m_type)
	{
	case SYM_REG_REFERENCE:
		assert(usedRegister == m_registers[symbol->m_valueLow]);
		break;
	case SYM_TMP_REFERENCE:
	case SYM_REL_REFERENCE:
		m_assembler.MovGd(MakeMemoryReferenceSymbolAddress(symbol), usedRegister);
		break;
	default:
		throw std::runtime_error("Invalid symbol type.");
		break;
	}
}
