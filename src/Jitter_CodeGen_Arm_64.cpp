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

void CCodeGen_Arm::LoadMemory64InRegisters(CArmAssembler::REGISTER regLo, CArmAssembler::REGISTER regHi, CSymbol* symbol)
{
	//Could probably be replaced by LDM
	LoadMemory64LowInRegister(regLo, symbol);
	LoadMemory64HighInRegister(regHi, symbol);
}

void CCodeGen_Arm::StoreRegistersInMemory64(CSymbol* symbol, CArmAssembler::REGISTER regLo, CArmAssembler::REGISTER regHi)
{
	//Could probably be replaced by STM
	StoreRegisterInMemory64Low(symbol, regLo);
	StoreRegisterInMemory64High(symbol, regHi);
}

void CCodeGen_Arm::StoreRegisterInMemory64Low(CSymbol* symbol, CArmAssembler::REGISTER registerId)
{
	switch(symbol->m_type)
	{
	case SYM_RELATIVE64:
		m_assembler.Str(registerId, g_baseRegister, CArmAssembler::MakeImmediateLdrAddress(symbol->m_valueLow + 0));
		break;
	case SYM_TEMPORARY64:
		m_assembler.Str(registerId, CArmAssembler::rSP, CArmAssembler::MakeImmediateLdrAddress(symbol->m_stackLocation + m_stackLevel + 0));
		break;
	default:
		assert(false);
		break;
	}
}

void CCodeGen_Arm::StoreRegisterInMemory64High(CSymbol* symbol, CArmAssembler::REGISTER registerId)
{
	switch(symbol->m_type)
	{
	case SYM_RELATIVE64:
		m_assembler.Str(registerId, g_baseRegister, CArmAssembler::MakeImmediateLdrAddress(symbol->m_valueLow + 4));
		break;
	case SYM_TEMPORARY64:
		m_assembler.Str(registerId, CArmAssembler::rSP, CArmAssembler::MakeImmediateLdrAddress(symbol->m_stackLocation + m_stackLevel + 4));
		break;
	default:
		assert(false);
		break;
	}
}

void CCodeGen_Arm::Emit_Mov_Mem64Mem64(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto regLo = CArmAssembler::r0;
	auto regHi = CArmAssembler::r1;
	LoadMemory64LowInRegister(regLo, src1);
	LoadMemory64HighInRegister(regHi, src1);
	StoreRegisterInMemory64Low(dst, regLo);
	StoreRegisterInMemory64High(dst, regHi);
}

void CCodeGen_Arm::Emit_Mov_Mem64Cst64(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto regLo = CArmAssembler::r0;
	auto regHi = CArmAssembler::r1;
	LoadConstantInRegister(regLo, src1->m_valueLow);
	LoadConstantInRegister(regHi, src1->m_valueHigh);
	StoreRegisterInMemory64Low(dst, regLo);
	StoreRegisterInMemory64High(dst, regHi);
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

void CCodeGen_Arm::Emit_Add64_MemMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto regLo1 = CArmAssembler::r0;
	auto regHi1 = CArmAssembler::r1;
	auto regLo2 = CArmAssembler::r2;
	auto regHi2 = CArmAssembler::r3;

	LoadMemory64InRegisters(regLo1, regHi1, src1);
	LoadMemory64InRegisters(regLo2, regHi2, src2);

	m_assembler.Adds(regLo1, regLo1, regLo2);
	m_assembler.Adc(regHi1, regHi1, regHi2);

	StoreRegistersInMemory64(dst, regLo1, regHi1);
}

void CCodeGen_Arm::Emit_Add64_MemMemCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto regLo1 = CArmAssembler::r0;
	auto regHi1 = CArmAssembler::r1;
	auto regLo2 = CArmAssembler::r2;
	auto regHi2 = CArmAssembler::r3;

	LoadMemory64InRegisters(regLo1, regHi1, src1);
	LoadConstantInRegister(regLo2, src2->m_valueLow);
	LoadConstantInRegister(regHi2, src2->m_valueHigh);

	//TODO: Improve this by using immediate operands instead of loading constants in registers
	m_assembler.Adds(regLo1, regLo1, regLo2);
	m_assembler.Adc(regHi1, regHi1, regHi2);

	StoreRegistersInMemory64(dst, regLo1, regHi1);
}

void CCodeGen_Arm::Emit_Sub64_MemMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto regLo1 = CArmAssembler::r0;
	auto regHi1 = CArmAssembler::r1;
	auto regLo2 = CArmAssembler::r2;
	auto regHi2 = CArmAssembler::r3;

	LoadMemory64InRegisters(regLo1, regHi1, src1);
	LoadMemory64InRegisters(regLo2, regHi2, src2);

	m_assembler.Subs(regLo1, regLo1, regLo2);
	m_assembler.Sbc(regHi1, regHi1, regHi2);

	StoreRegistersInMemory64(dst, regLo1, regHi1);
}

void CCodeGen_Arm::Emit_And64_MemMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto regLo1 = CArmAssembler::r0;
	auto regHi1 = CArmAssembler::r1;
	auto regLo2 = CArmAssembler::r2;
	auto regHi2 = CArmAssembler::r3;

	LoadMemory64InRegisters(regLo1, regHi1, src1);
	LoadMemory64InRegisters(regLo2, regHi2, src2);

	m_assembler.And(regLo1, regLo1, regLo2);
	m_assembler.And(regHi1, regHi1, regHi2);

	StoreRegistersInMemory64(dst, regLo1, regHi1);
}

void CCodeGen_Arm::Emit_Sl64Var_MemMem(CSymbol* dst, CSymbol* src, CArmAssembler::REGISTER saReg)
{
	//saReg will be modified by this function, do not use PrepareRegister

	assert(saReg == CArmAssembler::r0);

	auto lessThan32Label = m_assembler.CreateLabel();
	auto doneLabel = m_assembler.CreateLabel();

	m_assembler.Cmp(saReg, CArmAssembler::MakeImmediateAluOperand(32, 0));
	m_assembler.BCc(CArmAssembler::CONDITION_LT, lessThan32Label);

	//greaterOrEqualThan32:
	{
		auto workReg = CArmAssembler::r1;
		auto dstLo = CArmAssembler::r2;
		auto dstHi = CArmAssembler::r3;

		LoadMemory64LowInRegister(workReg, src);

		auto fixedSaReg = CArmAssembler::r0;
		m_assembler.Sub(fixedSaReg, saReg, CArmAssembler::MakeImmediateAluOperand(32, 0));

		auto shiftHi = CArmAssembler::MakeVariableShift(CArmAssembler::SHIFT_LSL, fixedSaReg);
		m_assembler.Mov(dstHi, CArmAssembler::MakeRegisterAluOperand(workReg, shiftHi));

		m_assembler.Mov(dstLo, CArmAssembler::MakeImmediateAluOperand(0, 0));

		StoreRegistersInMemory64(dst, dstLo, dstHi);

		m_assembler.BCc(CArmAssembler::CONDITION_AL, doneLabel);
	}

	//lessThan32:
	m_assembler.MarkLabel(lessThan32Label);
	{
		auto dstReg = CArmAssembler::r1;
		auto loReg = CArmAssembler::r2;
		auto hiReg = CArmAssembler::r3;

		//Lo part -> (lo << sa)
		auto shiftLo = CArmAssembler::MakeVariableShift(CArmAssembler::SHIFT_LSL, saReg);
		LoadMemory64LowInRegister(loReg, src);
		m_assembler.Mov(dstReg, CArmAssembler::MakeRegisterAluOperand(loReg, shiftLo));
		StoreRegisterInMemory64Low(dst, dstReg);

		//Hi part -> (lo >> (32 - sa)) | (hi << sa)
		auto shiftHi1 = CArmAssembler::MakeVariableShift(CArmAssembler::SHIFT_LSL, saReg);
		LoadMemory64HighInRegister(hiReg, src);
		m_assembler.Mov(dstReg, CArmAssembler::MakeRegisterAluOperand(hiReg, shiftHi1));

		auto shiftHi2 = CArmAssembler::MakeVariableShift(CArmAssembler::SHIFT_LSR, saReg);
		m_assembler.Rsb(saReg, saReg, CArmAssembler::MakeImmediateAluOperand(32, 0));
		m_assembler.Mov(loReg, CArmAssembler::MakeRegisterAluOperand(loReg, shiftHi2));
		m_assembler.Or(dstReg, dstReg, loReg);

		StoreRegisterInMemory64High(dst, dstReg);
	}

	//done:
	m_assembler.MarkLabel(doneLabel);
}

void CCodeGen_Arm::Emit_Sll64_MemMemVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto saReg = CArmAssembler::r0;

	switch(src2->m_type)
	{
	case SYM_REGISTER:
		m_assembler.Mov(saReg, g_registers[src2->m_valueLow]);
		break;
	case SYM_RELATIVE:
	case SYM_TEMPORARY:
		LoadMemoryInRegister(saReg, src2);
		break;
	default:
		assert(0);
		break;
	}

	Emit_Sl64Var_MemMem(dst, src1, saReg);
}

void CCodeGen_Arm::Emit_Sll64_MemMemCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto shiftAmount = src2->m_valueLow;

	assert(shiftAmount < 0x40);
	assert(shiftAmount != 0);

	auto srcLo = CArmAssembler::r0;
	auto srcHi = CArmAssembler::r1;
	auto dstLo = CArmAssembler::r2;
	auto dstHi = CArmAssembler::r3;

	if(shiftAmount >= 32)
	{
		shiftAmount -= 32;

		LoadMemory64LowInRegister(srcLo, src1);

		auto shiftHi = CArmAssembler::MakeConstantShift(CArmAssembler::SHIFT_LSL, shiftAmount);
		m_assembler.Mov(dstHi, CArmAssembler::MakeRegisterAluOperand(srcLo, shiftHi));

		m_assembler.Mov(dstLo, CArmAssembler::MakeImmediateAluOperand(0, 0));

		StoreRegistersInMemory64(dst, dstLo, dstHi);
	}
	else //Amount < 32
	{
		LoadMemory64InRegisters(srcLo, srcHi, src1);

		//Lo part -> (lo << sa)
		auto shiftLo = CArmAssembler::MakeConstantShift(CArmAssembler::SHIFT_LSL, shiftAmount);

		m_assembler.Mov(dstLo, CArmAssembler::MakeRegisterAluOperand(srcLo, shiftLo));

		//Hi part -> (lo >> (32 - sa)) | (hi << sa)
		auto shiftHi1 = CArmAssembler::MakeConstantShift(CArmAssembler::SHIFT_LSL, shiftAmount);
		auto shiftHi2 = CArmAssembler::MakeConstantShift(CArmAssembler::SHIFT_LSR, 32 - shiftAmount);

		m_assembler.Mov(srcHi, CArmAssembler::MakeRegisterAluOperand(srcHi, shiftHi1));
		m_assembler.Mov(srcLo, CArmAssembler::MakeRegisterAluOperand(srcLo, shiftHi2));
		m_assembler.Or(dstHi, srcLo, srcHi);

		StoreRegistersInMemory64(dst, dstLo, dstHi);
	}
}

void CCodeGen_Arm::Emit_Sr64Var_MemMem(CSymbol* dst, CSymbol* src, CArmAssembler::REGISTER saReg, CArmAssembler::SHIFT shiftType)
{
	assert((shiftType == CArmAssembler::SHIFT_ASR) || (shiftType == CArmAssembler::SHIFT_LSR));

	//saReg will be modified by this function, do not use PrepareRegister

	assert(saReg == CArmAssembler::r0);

	auto lessThan32Label = m_assembler.CreateLabel();
	auto doneLabel = m_assembler.CreateLabel();

	m_assembler.Cmp(saReg, CArmAssembler::MakeImmediateAluOperand(32, 0));
	m_assembler.BCc(CArmAssembler::CONDITION_LT, lessThan32Label);

	//greaterOrEqualThan32:
	{
		auto workReg = CArmAssembler::r1;
		auto dstLo = CArmAssembler::r2;
		auto dstHi = CArmAssembler::r3;

		LoadMemory64HighInRegister(workReg, src);

		auto fixedSaReg = CArmAssembler::r0;
		m_assembler.Sub(fixedSaReg, saReg, CArmAssembler::MakeImmediateAluOperand(32, 0));

		auto shiftLo = CArmAssembler::MakeVariableShift(shiftType, fixedSaReg);
		m_assembler.Mov(dstLo, CArmAssembler::MakeRegisterAluOperand(workReg, shiftLo));

		if(shiftType == CArmAssembler::SHIFT_LSR)
		{
			m_assembler.Mov(dstHi, CArmAssembler::MakeImmediateAluOperand(0, 0));
		}
		else
		{
			auto shiftHi = CArmAssembler::MakeConstantShift(CArmAssembler::SHIFT_ASR, 31);
			m_assembler.Mov(dstHi, CArmAssembler::MakeRegisterAluOperand(workReg, shiftHi));
		}

		StoreRegistersInMemory64(dst, dstLo, dstHi);

		m_assembler.BCc(CArmAssembler::CONDITION_AL, doneLabel);
	}

	//lessThan32:
	m_assembler.MarkLabel(lessThan32Label);
	{
		auto dstReg = CArmAssembler::r1;
		auto loReg = CArmAssembler::r2;
		auto hiReg = CArmAssembler::r3;

		//Hi part -> (hi >> sa)
		auto shiftHi = CArmAssembler::MakeVariableShift(shiftType, saReg);
		LoadMemory64HighInRegister(hiReg, src);
		m_assembler.Mov(dstReg, CArmAssembler::MakeRegisterAluOperand(hiReg, shiftHi));
		StoreRegisterInMemory64High(dst, dstReg);

		//Lo part -> (hi << (32 - sa)) | (lo >> sa)
		auto shiftLo1 = CArmAssembler::MakeVariableShift(CArmAssembler::SHIFT_LSR, saReg);
		LoadMemory64LowInRegister(loReg, src);
		m_assembler.Mov(dstReg, CArmAssembler::MakeRegisterAluOperand(loReg, shiftLo1));

		auto shiftLo2 = CArmAssembler::MakeVariableShift(CArmAssembler::SHIFT_LSL, saReg);
		m_assembler.Rsb(saReg, saReg, CArmAssembler::MakeImmediateAluOperand(32, 0));
		m_assembler.Mov(hiReg, CArmAssembler::MakeRegisterAluOperand(hiReg, shiftLo2));
		m_assembler.Or(dstReg, dstReg, hiReg);

		StoreRegisterInMemory64Low(dst, dstReg);
	}

	//done:
	m_assembler.MarkLabel(doneLabel);
}

void CCodeGen_Arm::Emit_Sr64Cst_MemMem(CSymbol* dst, CSymbol* src, uint32 shiftAmount, CArmAssembler::SHIFT shiftType)
{
	assert(shiftAmount < 0x40);
	assert(shiftAmount != 0);

	assert((shiftType == CArmAssembler::SHIFT_ASR) || (shiftType == CArmAssembler::SHIFT_LSR));

	auto srcLo = CArmAssembler::r0;
	auto srcHi = CArmAssembler::r1;
	auto dstLo = CArmAssembler::r2;
	auto dstHi = CArmAssembler::r3;

	if(shiftAmount >= 32)
	{
		shiftAmount -= 32;

		LoadMemory64HighInRegister(srcHi, src);

		if(shiftAmount != 0)
		{
			auto shiftLo = CArmAssembler::MakeConstantShift(shiftType, shiftAmount);
			m_assembler.Mov(dstLo, CArmAssembler::MakeRegisterAluOperand(srcHi, shiftLo));
		}
		else
		{
			m_assembler.Mov(dstLo, srcHi);
		}

		if(shiftType == CArmAssembler::SHIFT_LSR)
		{
			m_assembler.Mov(dstHi, CArmAssembler::MakeImmediateAluOperand(0, 0));
		}
		else
		{
			auto shiftHi = CArmAssembler::MakeConstantShift(CArmAssembler::SHIFT_ASR, 31);
			m_assembler.Mov(dstHi, CArmAssembler::MakeRegisterAluOperand(srcHi, shiftHi));
		}

		StoreRegistersInMemory64(dst, dstLo, dstHi);
	}
	else //Amount < 32
	{
		LoadMemory64InRegisters(srcLo, srcHi, src);

		//Hi part -> (hi >> sa)
		auto shiftHi = CArmAssembler::MakeConstantShift(shiftType, shiftAmount);

		m_assembler.Mov(dstHi, CArmAssembler::MakeRegisterAluOperand(srcHi, shiftHi));

		//Lo part -> (hi << (32 - sa)) | (lo >> sa)
		auto shiftLo1 = CArmAssembler::MakeConstantShift(CArmAssembler::SHIFT_LSR, shiftAmount);
		auto shiftLo2 = CArmAssembler::MakeConstantShift(CArmAssembler::SHIFT_LSL, 32 - shiftAmount);

		m_assembler.Mov(srcLo, CArmAssembler::MakeRegisterAluOperand(srcLo, shiftLo1));
		m_assembler.Mov(srcHi, CArmAssembler::MakeRegisterAluOperand(srcHi, shiftLo2));
		m_assembler.Or(dstLo, srcLo, srcHi);

		StoreRegistersInMemory64(dst, dstLo, dstHi);
	}
}

void CCodeGen_Arm::Emit_Srl64_MemMemVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto saReg = CArmAssembler::r0;

	switch(src2->m_type)
	{
	case SYM_REGISTER:
		m_assembler.Mov(saReg, g_registers[src2->m_valueLow]);
		break;
	case SYM_RELATIVE:
	case SYM_TEMPORARY:
		LoadMemoryInRegister(saReg, src2);
		break;
	default:
		assert(0);
		break;
	}

	Emit_Sr64Var_MemMem(dst, src1, saReg, CArmAssembler::SHIFT_LSR);
}

void CCodeGen_Arm::Emit_Srl64_MemMemCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto shiftAmount = src2->m_valueLow;

	Emit_Sr64Cst_MemMem(dst, src1, shiftAmount, CArmAssembler::SHIFT_LSR);
}

void CCodeGen_Arm::Emit_Sra64_MemMemVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto saReg = CArmAssembler::r0;

	switch(src2->m_type)
	{
	case SYM_REGISTER:
		m_assembler.Mov(saReg, g_registers[src2->m_valueLow]);
		break;
	case SYM_RELATIVE:
	case SYM_TEMPORARY:
		LoadMemoryInRegister(saReg, src2);
		break;
	default:
		assert(0);
		break;
	}

	Emit_Sr64Var_MemMem(dst, src1, saReg, CArmAssembler::SHIFT_ASR);
}

void CCodeGen_Arm::Emit_Sra64_MemMemCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto shiftAmount = src2->m_valueLow;

	Emit_Sr64Cst_MemMem(dst, src1, shiftAmount, CArmAssembler::SHIFT_ASR);
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
	case Jitter::CONDITION_LE:
		unsignedCondition = Jitter::CONDITION_BE;
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

	{ OP_ADD64,			MATCH_MEMORY64,		MATCH_MEMORY64,		MATCH_MEMORY64,		&CCodeGen_Arm::Emit_Add64_MemMemMem				},
	{ OP_ADD64,			MATCH_MEMORY64,		MATCH_MEMORY64,		MATCH_CONSTANT64,	&CCodeGen_Arm::Emit_Add64_MemMemCst				},

	{ OP_SUB64,			MATCH_MEMORY64,		MATCH_MEMORY64,		MATCH_MEMORY64,		&CCodeGen_Arm::Emit_Sub64_MemMemMem				},

	{ OP_AND64,			MATCH_MEMORY64,		MATCH_MEMORY64,		MATCH_MEMORY64,		&CCodeGen_Arm::Emit_And64_MemMemMem,			},

	{ OP_SLL64,			MATCH_MEMORY64,		MATCH_MEMORY64,		MATCH_VARIABLE,		&CCodeGen_Arm::Emit_Sll64_MemMemVar				},
	{ OP_SLL64,			MATCH_MEMORY64,		MATCH_MEMORY64,		MATCH_CONSTANT,		&CCodeGen_Arm::Emit_Sll64_MemMemCst				},

	{ OP_SRL64,			MATCH_MEMORY64,		MATCH_MEMORY64,		MATCH_VARIABLE,		&CCodeGen_Arm::Emit_Srl64_MemMemVar				},
	{ OP_SRL64,			MATCH_MEMORY64,		MATCH_MEMORY64,		MATCH_CONSTANT,		&CCodeGen_Arm::Emit_Srl64_MemMemCst				},

	{ OP_SRA64,			MATCH_MEMORY64,		MATCH_MEMORY64,		MATCH_VARIABLE,		&CCodeGen_Arm::Emit_Sra64_MemMemVar				},
	{ OP_SRA64,			MATCH_MEMORY64,		MATCH_MEMORY64,		MATCH_CONSTANT,		&CCodeGen_Arm::Emit_Sra64_MemMemCst				},

	{ OP_CMP64,			MATCH_VARIABLE,		MATCH_MEMORY64,		MATCH_MEMORY64,		&CCodeGen_Arm::Emit_Cmp64_VarMemAny				},
	{ OP_CMP64,			MATCH_VARIABLE,		MATCH_MEMORY64,		MATCH_CONSTANT64,	&CCodeGen_Arm::Emit_Cmp64_VarMemAny				},

	{ OP_MOV,			MATCH_MEMORY64,		MATCH_MEMORY64,		MATCH_NIL,			&CCodeGen_Arm::Emit_Mov_Mem64Mem64				},
	{ OP_MOV,			MATCH_MEMORY64,		MATCH_CONSTANT64,	MATCH_NIL,			&CCodeGen_Arm::Emit_Mov_Mem64Cst64				},

	{ OP_MOV,			MATCH_NIL,			MATCH_NIL,			MATCH_NIL,			NULL											},
};
