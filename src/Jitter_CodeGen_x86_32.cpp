#include "Jitter_CodeGen_x86_32.h"

using namespace Jitter;

CX86Assembler::REGISTER CCodeGen_x86_32::g_registers[3] =
{
	CX86Assembler::rBX,
	CX86Assembler::rSI,
	CX86Assembler::rDI,
};

CCodeGen_x86_32::CONSTMATCHER CCodeGen_x86_32::g_constMatchers[] = 
{ 
	{ OP_PARAM,		MATCH_NIL,			MATCH_CONTEXT,		MATCH_NIL,			&CCodeGen_x86_32::Emit_Param_Ctx		},
	{ OP_PARAM,		MATCH_NIL,			MATCH_RELATIVE,		MATCH_NIL,			&CCodeGen_x86_32::Emit_Param_Rel		},
	{ OP_PARAM,		MATCH_NIL,			MATCH_CONSTANT,		MATCH_NIL,			&CCodeGen_x86_32::Emit_Param_Cst		},
	{ OP_PARAM,		MATCH_NIL,			MATCH_REGISTER,		MATCH_NIL,			&CCodeGen_x86_32::Emit_Param_Reg		},
	{ OP_PARAM,		MATCH_NIL,			MATCH_TEMPORARY,	MATCH_NIL,			&CCodeGen_x86_32::Emit_Param_Tmp		},

	{ OP_CALL,		MATCH_NIL,			MATCH_CONSTANT,		MATCH_CONSTANT,		&CCodeGen_x86_32::Emit_Call				},

	{ OP_RETVAL,	MATCH_TEMPORARY,	MATCH_NIL,			MATCH_NIL,			&CCodeGen_x86_32::Emit_RetVal_Tmp		},
	{ OP_RETVAL,	MATCH_REGISTER,		MATCH_NIL,			MATCH_NIL,			&CCodeGen_x86_32::Emit_RetVal_Reg		},

	{ OP_ADD64,		MATCH_RELATIVE64,	MATCH_RELATIVE64,	MATCH_RELATIVE64,	&CCodeGen_x86_32::Emit_Add64_RelRelRel	},

	{ OP_AND64,		MATCH_RELATIVE64,	MATCH_RELATIVE64,	MATCH_RELATIVE64,	&CCodeGen_x86_32::Emit_And64_RelRelRel	},

	{ OP_SRL64,		MATCH_RELATIVE64,	MATCH_RELATIVE64,	MATCH_CONSTANT,		&CCodeGen_x86_32::Emit_Srl64_RelRelCst	},

	{ OP_SLL64,		MATCH_RELATIVE64,	MATCH_RELATIVE64,	MATCH_CONSTANT,		&CCodeGen_x86_32::Emit_Sll64_RelRelCst	},

	{ OP_CMP64,		MATCH_REGISTER,		MATCH_RELATIVE64,	MATCH_RELATIVE64,	&CCodeGen_x86_32::Emit_Cmp64_RegRelRel	},
	{ OP_CMP64,		MATCH_RELATIVE,		MATCH_RELATIVE64,	MATCH_RELATIVE64,	&CCodeGen_x86_32::Emit_Cmp64_RelRelRel	},
	{ OP_CMP64,		MATCH_TEMPORARY,	MATCH_RELATIVE64,	MATCH_RELATIVE64,	&CCodeGen_x86_32::Emit_Cmp64_TmpRelRel	},

	{ OP_MOV,		MATCH_NIL,			MATCH_NIL,			MATCH_NIL,			NULL									},
};

CCodeGen_x86_32::CCodeGen_x86_32()
{
	CCodeGen_x86::m_registers = g_registers;

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
}

CCodeGen_x86_32::~CCodeGen_x86_32()
{

}

void CCodeGen_x86_32::Emit_Prolog(unsigned int stackSize, uint32 registerUsage)
{
	//Allocate stack space
	if(stackSize != 0)
	{
		m_assembler.SubId(CX86Assembler::MakeRegisterAddress(CX86Assembler::rSP), stackSize);
	}
}

void CCodeGen_x86_32::Emit_Epilog(unsigned int stackSize, uint32 registerUsage)
{
	if(stackSize != 0)
	{
		m_assembler.AddId(CX86Assembler::MakeRegisterAddress(CX86Assembler::rSP), stackSize);
	}
	m_assembler.Ret();
}

unsigned int CCodeGen_x86_32::GetAvailableRegisterCount() const
{
	//We have ebx, esi and edi
	return 3;
}

void CCodeGen_x86_32::Emit_Param_Rel(const STATEMENT& statement)
{
	CSymbol* src1 = statement.src1->GetSymbol().get();
	m_assembler.PushEd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow));
	m_stackLevel += 4;
}

void CCodeGen_x86_32::Emit_Param_Ctx(const STATEMENT& statement)
{
	m_assembler.Push(CX86Assembler::rBP);
	m_stackLevel += 4;
}

void CCodeGen_x86_32::Emit_Param_Cst(const STATEMENT& statement)
{
	CSymbol* src1 = statement.src1->GetSymbol().get();
	m_assembler.PushId(src1->m_valueLow);
	m_stackLevel += 4;
}

void CCodeGen_x86_32::Emit_Param_Reg(const STATEMENT& statement)
{
	CSymbol* src1 = statement.src1->GetSymbol().get();
	m_assembler.Push(m_registers[src1->m_valueLow]);
	m_stackLevel += 4;
}

void CCodeGen_x86_32::Emit_Param_Tmp(const STATEMENT& statement)
{
	CSymbol* src1 = statement.src1->GetSymbol().get();
	m_assembler.PushEd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, src1->m_stackLocation + m_stackLevel));
	m_stackLevel += 4;
}

void CCodeGen_x86_32::Emit_Call(const STATEMENT& statement)
{
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	m_assembler.MovId(CX86Assembler::rAX, src1->m_valueLow);
	m_assembler.CallEd(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX));
	m_assembler.AddId(CX86Assembler::MakeRegisterAddress(CX86Assembler::rSP), src2->m_valueLow * 4);

	m_stackLevel = 0;
}

void CCodeGen_x86_32::Emit_RetVal_Tmp(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();

	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, dst->m_stackLocation), CX86Assembler::rAX);
}

void CCodeGen_x86_32::Emit_RetVal_Reg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();

	m_assembler.MovGd(CX86Assembler::MakeRegisterAddress(m_registers[dst->m_valueLow]), CX86Assembler::rAX);
}

void CCodeGen_x86_32::Emit_Add64_RelRelRel(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_RELATIVE64);
	assert(src1->m_type == SYM_RELATIVE64);
	assert(src2->m_type == SYM_RELATIVE64);

	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow + 0));
	m_assembler.MovEd(CX86Assembler::rDX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow + 4));

	m_assembler.AddEd(CX86Assembler::rAX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src2->m_valueLow + 0));
	m_assembler.AdcEd(CX86Assembler::rDX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src2->m_valueLow + 4));

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

void CCodeGen_x86_32::Cmp64_Equal(const STATEMENT& statement)
{
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_RELATIVE64);
	assert(src2->m_type == SYM_RELATIVE64);

	bool isEqual	= (statement.jmpCondition == CONDITION_EQ);

	CX86Assembler::REGISTER valReg = CX86Assembler::rDX;
	CX86Assembler::REGISTER res1Reg = CX86Assembler::rAX;
	CX86Assembler::REGISTER res2Reg = CX86Assembler::rCX;

	m_assembler.MovEd(valReg, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow + 0));
	m_assembler.CmpEd(valReg, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src2->m_valueLow + 0));

	if(isEqual)
	{
		m_assembler.SeteEb(CX86Assembler::MakeRegisterAddress(res1Reg));
	}
	else
	{
		m_assembler.SetneEb(CX86Assembler::MakeRegisterAddress(res1Reg));
	}
	
	m_assembler.MovEd(valReg, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow + 4));
	m_assembler.CmpEd(valReg, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src2->m_valueLow + 4));

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

void CCodeGen_x86_32::Cmp64_LessThan(const STATEMENT& statement)
{
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_RELATIVE64);
	assert(src2->m_type == SYM_RELATIVE64);

	CX86Assembler::REGISTER regLo = CX86Assembler::rAX;
	CX86Assembler::REGISTER regHi = CX86Assembler::rDX;

	bool isSigned	= (statement.jmpCondition == CONDITION_LE) || (statement.jmpCondition == CONDITION_LT); 
	bool orEqual	= (statement.jmpCondition == CONDITION_LE) || (statement.jmpCondition == CONDITION_BE);

    CX86Assembler::LABEL highOrderEqualLabel = m_assembler.CreateLabel();
    CX86Assembler::LABEL doneLabel = m_assembler.CreateLabel();

    /////////////////////////////////////////
	//Check high order word if equal

	m_assembler.MovEd(regHi, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow + 4));
	m_assembler.CmpEd(regHi, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src2->m_valueLow + 4));

    //je highOrderEqual
    m_assembler.JeJb(highOrderEqualLabel);

	///////////////////////////////////////////////////////////
	//If they aren't equal, this comparaison decides of result

	//setb/l reg[l]
    if(isSigned)
    {
        m_assembler.SetlEb(CX86Assembler::MakeByteRegisterAddress(regLo));
    }
    else
    {
        m_assembler.SetbEb(CX86Assembler::MakeByteRegisterAddress(regLo));
    }

	//movzx reg, reg[l]
    m_assembler.MovzxEb(regLo, CX86Assembler::MakeByteRegisterAddress(regLo));

	//jmp done
    m_assembler.JmpJb(doneLabel);

	//highOrderEqual: /////////////////////////////////////
    m_assembler.MarkLabel(highOrderEqualLabel);
    //If they are equal, next comparaison decides of result

	m_assembler.MovEd(regLo, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow + 0));
	m_assembler.CmpEd(regLo, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src2->m_valueLow + 0));

    //setb/be reg[l]
    if(orEqual)
    {
        m_assembler.SetbeEb(CX86Assembler::MakeByteRegisterAddress(regLo));
    }
    else
    {
        m_assembler.SetbEb(CX86Assembler::MakeByteRegisterAddress(regLo));
    }

	//movzx reg, reg[l]
    m_assembler.MovzxEb(regLo, CX86Assembler::MakeByteRegisterAddress(regLo));

    //done: ///////////////////////////////////////////////
    m_assembler.MarkLabel(doneLabel);
}

void CCodeGen_x86_32::Cmp64_GenericRelRel(const STATEMENT& statement)
{
	switch(statement.jmpCondition)
	{
	case CONDITION_BL:
	case CONDITION_LT:
	case CONDITION_LE:
		Cmp64_LessThan(statement);
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

	Cmp64_GenericRelRel(statement);
	m_assembler.MovGd(CX86Assembler::MakeRegisterAddress(m_registers[dst->m_valueLow]), CX86Assembler::rAX);
}

void CCodeGen_x86_32::Emit_Cmp64_RelRelRel(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();

	assert(dst->m_type == SYM_RELATIVE);

	Cmp64_GenericRelRel(statement);
	m_assembler.MovGd(MakeRelativeSymbolAddress(dst), CX86Assembler::rAX);
}

void CCodeGen_x86_32::Emit_Cmp64_TmpRelRel(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();

	assert(dst->m_type == SYM_TEMPORARY);

	Cmp64_GenericRelRel(statement);
	m_assembler.MovGd(MakeTemporarySymbolAddress(dst), CX86Assembler::rAX);
}
