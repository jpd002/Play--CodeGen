#include "Jitter_CodeGen_Arm.h"

using namespace Jitter;

void CCodeGen_Arm::LoadMemory64LowInRegister(CArmAssembler::REGISTER registerId, CSymbol* symbol)
{
	switch(symbol->m_type)
	{
	case SYM_RELATIVE64:
		m_assembler.Ldr(registerId, g_baseRegister, CArmAssembler::MakeImmediateLdrAddress(symbol->m_valueLow + 0));
		break;
	case SYM_TEMPORARY64:
		m_assembler.Ldr(registerId, CArmAssembler::rSP, CArmAssembler::MakeImmediateLdrAddress(symbol->m_stackLocation + m_stackLevel + 0));
		break;
	default:
		assert(false);
		break;
	}
}

void CCodeGen_Arm::LoadMemory64HighInRegister(CArmAssembler::REGISTER registerId, CSymbol* symbol)
{
	switch(symbol->m_type)
	{
	case SYM_RELATIVE64:
		m_assembler.Ldr(registerId, g_baseRegister, CArmAssembler::MakeImmediateLdrAddress(symbol->m_valueLow + 4));
		break;
	case SYM_TEMPORARY64:
		m_assembler.Ldr(registerId, CArmAssembler::rSP, CArmAssembler::MakeImmediateLdrAddress(symbol->m_stackLocation + m_stackLevel + 4));
		break;
	default:
		assert(false);
		break;
	}
}

void CCodeGen_Arm::Emit_ExtLow64VarMem64(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDef(dst, CArmAssembler::r0);
	LoadMemory64LowInRegister(dstReg, src1);
	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_Arm::Emit_ExtHigh64VarMem64(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDef(dst, CArmAssembler::r0);
	LoadMemory64HighInRegister(dstReg, src1);
	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_Arm::Cmp64_RegSymLo(CArmAssembler::REGISTER src1Reg, CSymbol* src2, CArmAssembler::REGISTER src2Reg)
{
	switch(src2->m_type)
	{
	case SYM_RELATIVE64:
	case SYM_TEMPORARY64:
		LoadMemory64LowInRegister(src2Reg, src2);
		m_assembler.Cmp(src1Reg, src2Reg);
		break;
	case SYM_CONSTANT64:
		Cmp_GenericRegCst(src1Reg, src2->m_valueLow, src2Reg);
		break;
	default:
		assert(false);
		break;
	}
}

void CCodeGen_Arm::Cmp64_RegSymHi(CArmAssembler::REGISTER src1Reg, CSymbol* src2, CArmAssembler::REGISTER src2Reg)
{
	switch(src2->m_type)
	{
	case SYM_RELATIVE64:
	case SYM_TEMPORARY64:
		LoadMemory64HighInRegister(src2Reg, src2);
		m_assembler.Cmp(src1Reg, src2Reg);
		break;
	case SYM_CONSTANT64:
		Cmp_GenericRegCst(src1Reg, src2->m_valueHigh, src2Reg);
		break;
	default:
		assert(false);
		break;
	}
}

void CCodeGen_Arm::Cmp64_Equal(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDef(dst, CArmAssembler::r0);
	auto src1Reg = CArmAssembler::r1;
	auto src2Reg = CArmAssembler::r2;
	auto tempValReg = CArmAssembler::r3;

	/////////////////////////////////////////
	//Check high order word
	LoadMemory64HighInRegister(src1Reg, src1);
	Cmp64_RegSymHi(src1Reg, src2, src2Reg);
	Cmp_GetFlag(tempValReg, statement.jmpCondition);

	/////////////////////////////////////////
	//Check low order word
	LoadMemory64LowInRegister(src1Reg, src1);
	Cmp64_RegSymLo(src1Reg, src2, src2Reg);
	Cmp_GetFlag(dstReg, statement.jmpCondition);

	if(statement.jmpCondition == Jitter::CONDITION_EQ)
	{
		m_assembler.And(dstReg, dstReg, tempValReg);
	}
	else if(statement.jmpCondition == Jitter::CONDITION_NE)
	{
		m_assembler.Or(dstReg, dstReg, tempValReg);
	}
	else
	{
		//Shouldn't get here
		assert(false);
	}

	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_Arm::Cmp64_Order(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto doneLabel = m_assembler.CreateLabel();
	auto highOrderEqualLabel = m_assembler.CreateLabel();

	auto dstReg = PrepareSymbolRegisterDef(dst, CArmAssembler::r0);
	auto src1Reg = CArmAssembler::r1;
	auto src2Reg = CArmAssembler::r2;

	/////////////////////////////////////////
	//Check high order word if equal

	//Compare with constant?
	LoadMemory64HighInRegister(src1Reg, src1);
	Cmp64_RegSymHi(src1Reg, src2, src2Reg);

	m_assembler.BCc(CArmAssembler::CONDITION_EQ, highOrderEqualLabel);

	///////////////////////////////////////////////////////////
	//If they aren't equal, this comparaison decides of result

	Cmp_GetFlag(dstReg, statement.jmpCondition);
	m_assembler.BCc(CArmAssembler::CONDITION_AL, doneLabel);

	///////////////////////////////////////////////////////////
	//If they are equal, next comparaison decides of result

	//highOrderEqual: /////////////////////////////////////
	m_assembler.MarkLabel(highOrderEqualLabel);

	LoadMemory64LowInRegister(src1Reg, src1);
	Cmp64_RegSymLo(src1Reg, src2, src2Reg);

	auto unsignedCondition = Jitter::CONDITION_NEVER;
	switch(statement.jmpCondition)
	{
	case Jitter::CONDITION_LT:
		unsignedCondition = Jitter::CONDITION_BL;
		break;
	case Jitter::CONDITION_GT:
		unsignedCondition = Jitter::CONDITION_AB;
		break;
	case Jitter::CONDITION_BL:
		unsignedCondition = statement.jmpCondition;
		break;
	default:
		assert(0);
		break;
	}

	Cmp_GetFlag(dstReg, unsignedCondition);

	//done: ///////////////////////////////////////////////
	m_assembler.MarkLabel(doneLabel);

	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_Arm::Emit_Cmp64_VarMemAny(const STATEMENT& statement)
{
	switch(statement.jmpCondition)
	{
	case CONDITION_BL:
	case CONDITION_LT:
	case CONDITION_LE:
	case CONDITION_AB:
	case CONDITION_GT:
	case CONDITION_GE:
		Cmp64_Order(statement);
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

CCodeGen_Arm::CONSTMATCHER CCodeGen_Arm::g_64ConstMatchers[] = 
{
	{ OP_EXTLOW64,		MATCH_VARIABLE,		MATCH_MEMORY64,		MATCH_NIL,			&CCodeGen_Arm::Emit_ExtLow64VarMem64			},
	{ OP_EXTHIGH64,		MATCH_VARIABLE,		MATCH_MEMORY64,		MATCH_NIL,			&CCodeGen_Arm::Emit_ExtHigh64VarMem64			},

	{ OP_CMP64,			MATCH_VARIABLE,		MATCH_MEMORY64,		MATCH_MEMORY64,		&CCodeGen_Arm::Emit_Cmp64_VarMemAny				},
	{ OP_CMP64,			MATCH_VARIABLE,		MATCH_MEMORY64,		MATCH_CONSTANT64,	&CCodeGen_Arm::Emit_Cmp64_VarMemAny				},

	{ OP_MOV,			MATCH_NIL,			MATCH_NIL,			MATCH_NIL,			NULL											},
};
