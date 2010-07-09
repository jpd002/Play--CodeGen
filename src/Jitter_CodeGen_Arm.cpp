#include "Jitter_CodeGen_Arm.h"

using namespace Jitter;

CArmAssembler::REGISTER CCodeGen_Arm::g_baseRegister = CArmAssembler::r11;

CArmAssembler::REGISTER CCodeGen_Arm::g_registers[MAX_REGISTERS] =
{
	CArmAssembler::r4,
	CArmAssembler::r5,
	CArmAssembler::r6,
	CArmAssembler::r7,
	CArmAssembler::r8,
	CArmAssembler::r10,
};

CArmAssembler::REGISTER CCodeGen_Arm::g_paramRegs[MAX_PARAMS] =
{
	CArmAssembler::r0,
	CArmAssembler::r1,
	CArmAssembler::r2,
	CArmAssembler::r3,
};

template <typename ALUOP>
void CCodeGen_Arm::Emit_Alu_RegRegReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();
	
	assert(dst->m_type  == SYM_REGISTER);
	assert(src1->m_type == SYM_REGISTER);
	assert(src2->m_type == SYM_REGISTER);
	
	((m_assembler).*(ALUOP::OpReg()))(g_registers[dst->m_valueLow], g_registers[src1->m_valueLow], g_registers[src2->m_valueLow]);
}

template <typename ALUOP>
void CCodeGen_Arm::Emit_Alu_RegRegCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();
	
	assert(dst->m_type  == SYM_REGISTER);
	assert(src1->m_type == SYM_REGISTER);
	assert(src2->m_type == SYM_CONSTANT);
	
	uint8 immediate = 0;
	uint8 shiftAmount = 0;
	if(TryGetAluImmediateParams(src2->m_valueLow, immediate, shiftAmount))
	{
		((m_assembler).*(ALUOP::OpImm()))(g_registers[dst->m_valueLow], g_registers[src1->m_valueLow], CArmAssembler::MakeImmediateAluOperand(immediate, shiftAmount));
	}
	else
	{
		LoadConstantInRegister(CArmAssembler::r0, src2->m_valueLow);
		((m_assembler).*(ALUOP::OpReg()))(g_registers[dst->m_valueLow], g_registers[src1->m_valueLow], CArmAssembler::r0);
	}
}

template <typename ALUOP>
void CCodeGen_Arm::Emit_Alu_RegCstReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();
	
	assert(dst->m_type  == SYM_REGISTER);
	assert(src1->m_type == SYM_CONSTANT);
	assert(src2->m_type == SYM_REGISTER);
	
	LoadConstantInRegister(CArmAssembler::r0, src1->m_valueLow);
	((m_assembler).*(ALUOP::OpReg()))(g_registers[dst->m_valueLow], CArmAssembler::r0, g_registers[src2->m_valueLow]);
}

template <CArmAssembler::SHIFT shiftType> 
void CCodeGen_Arm::Emit_Shift_RegRegReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_REGISTER);
	assert(src1->m_type == SYM_REGISTER);
	assert(src2->m_type == SYM_REGISTER);
	
	m_assembler.Mov(g_registers[dst->m_valueLow], 
		CArmAssembler::MakeRegisterAluOperand(g_registers[src1->m_valueLow], 
		CArmAssembler::MakeVariableShift(shiftType, g_registers[src2->m_valueLow])));
}

template <CArmAssembler::SHIFT shiftType> 
void CCodeGen_Arm::Emit_Shift_RegRegCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();
	
	assert(dst->m_type  == SYM_REGISTER);
	assert(src1->m_type == SYM_REGISTER);
	assert(src2->m_type == SYM_CONSTANT);
	
	m_assembler.Mov(g_registers[dst->m_valueLow], 
					CArmAssembler::MakeRegisterAluOperand(g_registers[src1->m_valueLow], 
														  CArmAssembler::MakeConstantShift(shiftType, static_cast<uint8>(src2->m_valueLow))));
}

#define ALU_CONST_MATCHERS(ALUOP_CST, ALUOP) \
	{ ALUOP_CST,	MATCH_REGISTER,		MATCH_REGISTER,		MATCH_REGISTER,		&CCodeGen_Arm::Emit_Alu_RegRegReg<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_REGISTER,		MATCH_REGISTER,		MATCH_CONSTANT,		&CCodeGen_Arm::Emit_Alu_RegRegCst<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_REGISTER,		MATCH_CONSTANT,		MATCH_REGISTER,		&CCodeGen_Arm::Emit_Alu_RegCstReg<ALUOP>		},

#define SHIFT_CONST_MATCHERS(SHIFTOP_CST, SHIFTOP) \
	{ SHIFTOP_CST,	MATCH_REGISTER,		MATCH_REGISTER,		MATCH_REGISTER,		&CCodeGen_Arm::Emit_Shift_RegRegReg<SHIFTOP>	}, \
	{ SHIFTOP_CST,	MATCH_REGISTER,		MATCH_REGISTER,		MATCH_CONSTANT,		&CCodeGen_Arm::Emit_Shift_RegRegCst<SHIFTOP>	},

CCodeGen_Arm::CONSTMATCHER CCodeGen_Arm::g_constMatchers[] = 
{ 
	{ OP_LABEL,		MATCH_NIL,			MATCH_NIL,			MATCH_NIL,			&CCodeGen_Arm::MarkLabel							},

	{ OP_MOV,		MATCH_REGISTER,		MATCH_REGISTER,		MATCH_NIL,			&CCodeGen_Arm::Emit_Mov_RegReg						},
	{ OP_MOV,		MATCH_REGISTER,		MATCH_RELATIVE,		MATCH_NIL,			&CCodeGen_Arm::Emit_Mov_RegRel						},
	{ OP_MOV,		MATCH_REGISTER,		MATCH_CONSTANT,		MATCH_NIL,			&CCodeGen_Arm::Emit_Mov_RegCst						},
	{ OP_MOV,		MATCH_RELATIVE,		MATCH_REGISTER,		MATCH_NIL,			&CCodeGen_Arm::Emit_Mov_RelReg						},

	ALU_CONST_MATCHERS(OP_ADD, ALUOP_ADD)
	ALU_CONST_MATCHERS(OP_SUB, ALUOP_SUB)
	ALU_CONST_MATCHERS(OP_AND, ALUOP_AND)
		
	SHIFT_CONST_MATCHERS(OP_SRL, CArmAssembler::SHIFT_LSR)
	SHIFT_CONST_MATCHERS(OP_SRA, CArmAssembler::SHIFT_ASR)
	SHIFT_CONST_MATCHERS(OP_SLL, CArmAssembler::SHIFT_LSL)
	
	{ OP_PARAM,		MATCH_NIL,			MATCH_CONTEXT,		MATCH_NIL,			&CCodeGen_Arm::Emit_Param_Ctx						},

	{ OP_CALL,		MATCH_NIL,			MATCH_CONSTANT,		MATCH_CONSTANT,		&CCodeGen_Arm::Emit_Call							},
	
	{ OP_RETVAL,	MATCH_REGISTER,		MATCH_NIL,			MATCH_NIL,			&CCodeGen_Arm::Emit_RetVal_Reg						},

	{ OP_JMP,		MATCH_NIL,			MATCH_NIL,			MATCH_NIL,			&CCodeGen_Arm::Emit_Jmp								},

	{ OP_CONDJMP,	MATCH_NIL,			MATCH_REGISTER,		MATCH_CONSTANT,		&CCodeGen_Arm::Emit_CondJmp_RegCst					},
	
	{ OP_CMP,		MATCH_REGISTER,		MATCH_REGISTER,		MATCH_REGISTER,		&CCodeGen_Arm::Emit_Cmp_RegRegReg					},
	{ OP_CMP,		MATCH_REGISTER,		MATCH_REGISTER,		MATCH_CONSTANT,		&CCodeGen_Arm::Emit_Cmp_RegRegCst					},

	{ OP_NOT,		MATCH_REGISTER,		MATCH_REGISTER,		MATCH_NIL,			&CCodeGen_Arm::Emit_Not_RegReg						},
	
	{ OP_MOV,		MATCH_NIL,			MATCH_NIL,			MATCH_NIL,			NULL												},
};

CCodeGen_Arm::CCodeGen_Arm()
: m_stream(NULL)
, m_literalPool(NULL)
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

	m_literalPool = new uint32[LITERAL_POOL_SIZE];
}

CCodeGen_Arm::~CCodeGen_Arm()
{
	if(m_literalPool)
	{
		delete [] m_literalPool;
		m_literalPool = NULL;
	}
}

unsigned int CCodeGen_Arm::GetAvailableRegisterCount() const
{
	return MAX_REGISTERS;
}

void CCodeGen_Arm::SetStream(Framework::CStream* stream)
{
	m_stream = stream;
	m_assembler.SetStream(stream);
}

void CCodeGen_Arm::GenerateCode(const StatementList& statements, unsigned int stackSize)
{
	m_lastLiteralPtr = 0;

	//Save return address
	m_assembler.Sub(CArmAssembler::rSP, CArmAssembler::rSP, CArmAssembler::MakeImmediateAluOperand(0x04, 0));
	m_assembler.Str(CArmAssembler::rLR, CArmAssembler::rSP, CArmAssembler::MakeImmediateLdrAddress(0x00));

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

	m_assembler.Ldr(CArmAssembler::rLR, CArmAssembler::rSP, CArmAssembler::MakeImmediateLdrAddress(0x00));
	m_assembler.Add(CArmAssembler::rSP, CArmAssembler::rSP, CArmAssembler::MakeImmediateAluOperand(0x04, 0));
	m_assembler.Bx(CArmAssembler::rLR);

	m_assembler.ResolveLabelReferences();
	m_assembler.ClearLabels();
	m_labels.clear();
	
	DumpLiteralPool();
}

uint32 CCodeGen_Arm::RotateRight(uint32 value)
{
	uint32 carry = value & 1;
	value >>= 1;
	value |= carry << 31;
	return value;
}

uint32 CCodeGen_Arm::RotateLeft(uint32 value)
{
	uint32 carry = value >> 31;
	value <<= 1;
	value |= carry;
	return value;
}

bool CCodeGen_Arm::TryGetAluImmediateParams(uint32 constant, uint8& immediate, uint8& shiftAmount)
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
		shadowConstant = RotateLeft(shadowConstant);
		shadowConstant = RotateLeft(shadowConstant);
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

void CCodeGen_Arm::LoadConstantInRegister(CArmAssembler::REGISTER registerId, uint32 constant)
{	
	//Try normal move
	{
		uint8 immediate = 0;
		uint8 shiftAmount = 0;
		if(TryGetAluImmediateParams(constant, immediate, shiftAmount))
		{
			m_assembler.Mov(registerId, CArmAssembler::MakeImmediateAluOperand(immediate, shiftAmount));				
			return;
		}
	}
	
	//Try not move
	{
		uint8 immediate = 0;
		uint8 shiftAmount = 0;
		if(TryGetAluImmediateParams(~constant, immediate, shiftAmount))
		{
			m_assembler.Mvn(registerId, CArmAssembler::MakeImmediateAluOperand(immediate, shiftAmount));
			return;
		}
	}
		
	//Store as constant in literal table
	{
		//Search for an existing literal
		unsigned int literalPtr = -1;
		for(unsigned int i = 0; i < m_lastLiteralPtr; i++)
		{
			if(m_literalPool[i] == constant) 
			{
				literalPtr = i;
				break;
			}
		}
		if(literalPtr == -1)
		{
			assert(m_lastLiteralPtr != LITERAL_POOL_SIZE);
			literalPtr = m_lastLiteralPtr++;
			m_literalPool[literalPtr] = constant;
		}
		
		LITERAL_POOL_REF reference;
		reference.poolPtr = literalPtr;
		reference.dstRegister = registerId;
		reference.offset = static_cast<unsigned int>(m_stream->Tell());
		m_literalPoolRefs.push_back(reference);
		
		//Write a blank instruction
		m_stream->Write32(0);
	}
}

void CCodeGen_Arm::DumpLiteralPool()
{
	if(m_lastLiteralPtr)
	{
		uint32 literalPoolPos = static_cast<uint32>(m_stream->Tell());
		m_stream->Write(m_literalPool, sizeof(uint32) * m_lastLiteralPtr);
		
		for(LiteralPoolRefList::const_iterator referenceIterator(m_literalPoolRefs.begin());
			referenceIterator != m_literalPoolRefs.end(); referenceIterator++)
		{
			const LITERAL_POOL_REF& reference(*referenceIterator);
			m_stream->Seek(reference.offset, Framework::STREAM_SEEK_SET);
			uint32 literalOffset = (reference.poolPtr * 4 + literalPoolPos) - reference.offset - 8;
			m_assembler.Ldr(reference.dstRegister, CArmAssembler::rPC, CArmAssembler::MakeImmediateLdrAddress(literalOffset));
		}
	}
	
	m_literalPoolRefs.clear();
	m_lastLiteralPtr = 0;
}

CArmAssembler::LABEL CCodeGen_Arm::GetLabel(uint32 blockId)
{
	CArmAssembler::LABEL result;
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

void CCodeGen_Arm::MarkLabel(const STATEMENT& statement)
{
	CArmAssembler::LABEL label = GetLabel(statement.jmpBlock);
	m_assembler.MarkLabel(label);
}

void CCodeGen_Arm::Emit_Param_Ctx(const STATEMENT& statement)
{
	CSymbol* src1 = statement.src1->GetSymbol().get();

	assert(src1->m_type == SYM_CONTEXT);
	assert(m_params.size() < MAX_PARAMS);
	
	void (CArmAssembler::*MovFunction)(CArmAssembler::REGISTER, CArmAssembler::REGISTER) = &CArmAssembler::Mov;
	m_params.push_back(std::tr1::bind(MovFunction, &m_assembler, std::tr1::placeholders::_1, g_baseRegister));	
}

void CCodeGen_Arm::Emit_Call(const STATEMENT& statement)
{
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();
	
	assert(src1->m_type == SYM_CONSTANT);
	assert(src2->m_type == SYM_CONSTANT);

	unsigned int paramCount = src2->m_valueLow;
	
	for(unsigned int i = 0; i < paramCount; i++)
	{
		ParamEmitterFunction emitter(m_params.back());
		m_params.pop_back();
		emitter(g_paramRegs[i]);
	}

	m_assembler.Mov(CArmAssembler::rLR, CArmAssembler::rPC);
	LoadConstantInRegister(CArmAssembler::rPC, src1->m_valueLow);
}

void CCodeGen_Arm::Emit_RetVal_Reg(const STATEMENT& statement)
{	
	CSymbol* dst = statement.dst->GetSymbol().get();
	
	assert(dst->m_type == SYM_REGISTER);
	
	m_assembler.Mov(g_registers[dst->m_valueLow], CArmAssembler::r0);
}

void CCodeGen_Arm::Emit_Mov_RegReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	m_assembler.Mov(g_registers[dst->m_valueLow], g_registers[src1->m_valueLow]);
}

void CCodeGen_Arm::Emit_Mov_RegRel(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	m_assembler.Ldr(g_registers[dst->m_valueLow], g_baseRegister, CArmAssembler::MakeImmediateLdrAddress(src1->m_valueLow));
}

void CCodeGen_Arm::Emit_Mov_RegCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	LoadConstantInRegister(g_registers[dst->m_valueLow], src1->m_valueLow);
}

void CCodeGen_Arm::Emit_Mov_RelReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	m_assembler.Str(g_registers[src1->m_valueLow], g_baseRegister, CArmAssembler::MakeImmediateLdrAddress(dst->m_valueLow));
}

void CCodeGen_Arm::Emit_Jmp(const STATEMENT& statement)
{
	m_assembler.BCc(CArmAssembler::CONDITION_AL, GetLabel(statement.jmpBlock));
}

void CCodeGen_Arm::Emit_CondJmp_RegCst(const STATEMENT& statement)
{
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();	
	
	CArmAssembler::LABEL label(GetLabel(statement.jmpBlock));

	assert(src1->m_type == SYM_REGISTER);
	assert(src2->m_type == SYM_CONSTANT);
	
	uint8 immediate = 0;
	uint8 shiftAmount = 0;
	if(TryGetAluImmediateParams(src2->m_valueLow, immediate, shiftAmount))
	{
		m_assembler.Cmp(g_registers[src1->m_valueLow], CArmAssembler::MakeImmediateAluOperand(immediate, shiftAmount));
	}
	else
	{
		LoadConstantInRegister(CArmAssembler::r0, src2->m_valueLow);
		m_assembler.Cmp(g_registers[src1->m_valueLow], CArmAssembler::r0);
	}
	
	switch(statement.jmpCondition)
	{
	case CONDITION_NE:
		m_assembler.BCc(CArmAssembler::CONDITION_NE, label);
		break;
	default:
		assert(0);
		break;
	}
}

void CCodeGen_Arm::Cmp_GetFlag(CArmAssembler::REGISTER registerId, Jitter::CONDITION condition)
{
	switch(condition)
	{
	case CONDITION_LT:
			m_assembler.MovCc(CArmAssembler::CONDITION_GE, registerId, CArmAssembler::MakeImmediateAluOperand(0, 0));
			m_assembler.MovCc(CArmAssembler::CONDITION_LT, registerId, CArmAssembler::MakeImmediateAluOperand(1, 0));
			break;
	case CONDITION_NE:
			m_assembler.MovCc(CArmAssembler::CONDITION_EQ, registerId, CArmAssembler::MakeImmediateAluOperand(0, 0));
			m_assembler.MovCc(CArmAssembler::CONDITION_NE, registerId, CArmAssembler::MakeImmediateAluOperand(1, 0));
			break;
	default:
			assert(0);
			break;
	}
}

void CCodeGen_Arm::Emit_Cmp_RegRegReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	m_assembler.Cmp(g_registers[src1->m_valueLow], g_registers[src2->m_valueLow]);
	Cmp_GetFlag(g_registers[dst->m_valueLow], statement.jmpCondition);
}

void CCodeGen_Arm::Emit_Cmp_RegRegCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	LoadConstantInRegister(CArmAssembler::r0, src2->m_valueLow);
	m_assembler.Cmp(g_registers[src1->m_valueLow], CArmAssembler::r0);
	Cmp_GetFlag(g_registers[dst->m_valueLow], statement.jmpCondition);
}

void CCodeGen_Arm::Emit_Not_RegReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type  == SYM_REGISTER);
	assert(src1->m_type == SYM_REGISTER);

	m_assembler.Mvn(g_registers[dst->m_valueLow], g_registers[src1->m_valueLow]);
}
