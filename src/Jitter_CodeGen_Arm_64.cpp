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

void CCodeGen_Arm::Emit_Cmp64_VarMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(statement.jmpCondition == CONDITION_BL);

	auto doneLabel = m_assembler.CreateLabel();
	auto highOrderEqualLabel = m_assembler.CreateLabel();

	auto dstReg = PrepareSymbolRegisterDef(dst, CArmAssembler::r0);
	auto src1Reg = CArmAssembler::r1;
	auto src2Reg = CArmAssembler::r2;

	/////////////////////////////////////////
	//Check high order word if equal

	//Compare with constant?
	LoadMemory64HighInRegister(src1Reg, src1);
	LoadMemory64HighInRegister(src2Reg, src2);
	m_assembler.Cmp(src1Reg, src2Reg);

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
	LoadMemory64LowInRegister(src2Reg, src2);
	m_assembler.Cmp(src1Reg, src2Reg);

	auto unsignedCondition = Jitter::CONDITION_NEVER;
	switch(statement.jmpCondition)
	{
	case Jitter::CONDITION_LT:
		unsignedCondition = Jitter::CONDITION_BL;
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

CCodeGen_Arm::CONSTMATCHER CCodeGen_Arm::g_64ConstMatchers[] = 
{
	{ OP_EXTLOW64,		MATCH_VARIABLE,		MATCH_MEMORY64,		MATCH_NIL,			&CCodeGen_Arm::Emit_ExtLow64VarMem64			},
	{ OP_EXTHIGH64,		MATCH_VARIABLE,		MATCH_MEMORY64,		MATCH_NIL,			&CCodeGen_Arm::Emit_ExtHigh64VarMem64			},

	{ OP_CMP64,			MATCH_VARIABLE,		MATCH_MEMORY64,		MATCH_MEMORY64,		&CCodeGen_Arm::Emit_Cmp64_VarMemMem				},

	{ OP_MOV,			MATCH_NIL,			MATCH_NIL,			MATCH_NIL,			NULL											},
};
