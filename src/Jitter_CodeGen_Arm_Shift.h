#ifndef _JITTER_CODEGEN_ARM_SHIFT_H_
#define _JITTER_CODEGEN_ARM_SHIFT_H_

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

template <CArmAssembler::SHIFT shiftType> 
void CCodeGen_Arm::Emit_Shift_RegRelReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();
	
	assert(dst->m_type  == SYM_REGISTER);
	assert(src1->m_type == SYM_RELATIVE);
	assert(src2->m_type == SYM_REGISTER);
	
	CArmAssembler::REGISTER src1Reg = CArmAssembler::r1;
	
	LoadRelativeInRegister(src1Reg, src1);
	
	m_assembler.Mov(g_registers[dst->m_valueLow], 
					CArmAssembler::MakeRegisterAluOperand(src1Reg, 
														  CArmAssembler::MakeVariableShift(shiftType, g_registers[src2->m_valueLow])));
}

template <CArmAssembler::SHIFT shiftType> 
void CCodeGen_Arm::Emit_Shift_RegRelCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();
	
	assert(dst->m_type  == SYM_REGISTER);
	assert(src1->m_type == SYM_RELATIVE);
	assert(src2->m_type == SYM_CONSTANT);
	
	CArmAssembler::REGISTER src1Reg = CArmAssembler::r1;
	
	LoadRelativeInRegister(src1Reg, src1);
	
	m_assembler.Mov(g_registers[dst->m_valueLow], 
					CArmAssembler::MakeRegisterAluOperand(src1Reg, 
														  CArmAssembler::MakeConstantShift(shiftType, static_cast<uint8>(src2->m_valueLow))));
}

template <CArmAssembler::SHIFT shiftType> 
void CCodeGen_Arm::Emit_Shift_RegCstReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();
	
	assert(dst->m_type  == SYM_REGISTER);
	assert(src1->m_type == SYM_CONSTANT);
	assert(src2->m_type == SYM_REGISTER);
	
	LoadConstantInRegister(CArmAssembler::r0, src1->m_valueLow);
	
	m_assembler.Mov(g_registers[dst->m_valueLow], 
					CArmAssembler::MakeRegisterAluOperand(CArmAssembler::r0, 
														  CArmAssembler::MakeVariableShift(shiftType, g_registers[src2->m_valueLow])));
}

template <CArmAssembler::SHIFT shiftType> 
void CCodeGen_Arm::Emit_Shift_RegCstRel(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();
	
	assert(dst->m_type  == SYM_REGISTER);
	assert(src1->m_type == SYM_CONSTANT);
	assert(src2->m_type == SYM_RELATIVE);
	
	LoadConstantInRegister(CArmAssembler::r0, src1->m_valueLow);
	LoadRelativeInRegister(CArmAssembler::r1, src2);
	
	m_assembler.Mov(g_registers[dst->m_valueLow], 
					CArmAssembler::MakeRegisterAluOperand(CArmAssembler::r0, 
														  CArmAssembler::MakeVariableShift(shiftType, CArmAssembler::r1)));
}

template <CArmAssembler::SHIFT shiftType> 
void CCodeGen_Arm::Emit_Shift_RelRegCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();
	
	assert(dst->m_type  == SYM_RELATIVE);
	assert(src1->m_type == SYM_REGISTER);
	assert(src2->m_type == SYM_CONSTANT);
	
	m_assembler.Mov(CArmAssembler::r0, 
					CArmAssembler::MakeRegisterAluOperand(g_registers[src1->m_valueLow], 
														  CArmAssembler::MakeConstantShift(shiftType, static_cast<uint8>(src2->m_valueLow))));
	StoreRegisterInRelative(dst, CArmAssembler::r0);
}

template <CArmAssembler::SHIFT shiftType> 
void CCodeGen_Arm::Emit_Shift_RelRelCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();
	
	assert(dst->m_type  == SYM_RELATIVE);
	assert(src1->m_type == SYM_RELATIVE);
	assert(src2->m_type == SYM_CONSTANT);
	
	CArmAssembler::REGISTER dstReg = CArmAssembler::r0;
	CArmAssembler::REGISTER src1Reg = CArmAssembler::r1;
	
	LoadRelativeInRegister(src1Reg, src1);
	
	m_assembler.Mov(dstReg, 
					CArmAssembler::MakeRegisterAluOperand(src1Reg, 
														  CArmAssembler::MakeConstantShift(shiftType, static_cast<uint8>(src2->m_valueLow))));
	StoreRegisterInRelative(dst, dstReg);
}

template <CArmAssembler::SHIFT shiftType> 
void CCodeGen_Arm::Emit_Shift_RelTmpCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();
	
	assert(dst->m_type  == SYM_RELATIVE);
	assert(src1->m_type == SYM_TEMPORARY);
	assert(src2->m_type == SYM_CONSTANT);
	
	CArmAssembler::REGISTER dstReg = CArmAssembler::r0;
	CArmAssembler::REGISTER src1Reg = CArmAssembler::r1;
	
	LoadTemporaryInRegister(src1Reg, src1);
	
	m_assembler.Mov(dstReg, 
					CArmAssembler::MakeRegisterAluOperand(src1Reg, 
														  CArmAssembler::MakeConstantShift(shiftType, static_cast<uint8>(src2->m_valueLow))));
	StoreRegisterInRelative(dst, dstReg);
}

template <CArmAssembler::SHIFT shiftType> 
void CCodeGen_Arm::Emit_Shift_TmpTmpCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();
	
	assert(dst->m_type  == SYM_TEMPORARY);
	assert(src1->m_type == SYM_TEMPORARY);
	assert(src2->m_type == SYM_CONSTANT);
	
	CArmAssembler::REGISTER dstReg = CArmAssembler::r0;
	CArmAssembler::REGISTER src1Reg = CArmAssembler::r1;
	
	LoadTemporaryInRegister(src1Reg, src1);
	
	m_assembler.Mov(dstReg, 
					CArmAssembler::MakeRegisterAluOperand(src1Reg, 
														  CArmAssembler::MakeConstantShift(shiftType, static_cast<uint8>(src2->m_valueLow))));
	StoreRegisterInTemporary(dst, dstReg);
}

#define SHIFT_CONST_MATCHERS(SHIFTOP_CST, SHIFTOP) \
	{ SHIFTOP_CST,	MATCH_REGISTER,		MATCH_REGISTER,		MATCH_REGISTER,		&CCodeGen_Arm::Emit_Shift_RegRegReg<SHIFTOP>	}, \
	{ SHIFTOP_CST,	MATCH_REGISTER,		MATCH_REGISTER,		MATCH_CONSTANT,		&CCodeGen_Arm::Emit_Shift_RegRegCst<SHIFTOP>	}, \
	{ SHIFTOP_CST,	MATCH_REGISTER,		MATCH_RELATIVE,		MATCH_REGISTER,		&CCodeGen_Arm::Emit_Shift_RegRelReg<SHIFTOP>	}, \
	{ SHIFTOP_CST,	MATCH_REGISTER,		MATCH_RELATIVE,		MATCH_CONSTANT,		&CCodeGen_Arm::Emit_Shift_RegRelCst<SHIFTOP>	}, \
	{ SHIFTOP_CST,	MATCH_REGISTER,		MATCH_CONSTANT,		MATCH_REGISTER,		&CCodeGen_Arm::Emit_Shift_RegCstReg<SHIFTOP>	}, \
	{ SHIFTOP_CST,	MATCH_REGISTER,		MATCH_CONSTANT,		MATCH_RELATIVE,		&CCodeGen_Arm::Emit_Shift_RegCstRel<SHIFTOP>	}, \
	{ SHIFTOP_CST,	MATCH_RELATIVE,		MATCH_REGISTER,		MATCH_CONSTANT,		&CCodeGen_Arm::Emit_Shift_RelRegCst<SHIFTOP>	}, \
	{ SHIFTOP_CST,	MATCH_RELATIVE,		MATCH_RELATIVE,		MATCH_CONSTANT,		&CCodeGen_Arm::Emit_Shift_RelRelCst<SHIFTOP>	}, \
	{ SHIFTOP_CST,	MATCH_RELATIVE,		MATCH_TEMPORARY,	MATCH_CONSTANT,		&CCodeGen_Arm::Emit_Shift_RelTmpCst<SHIFTOP>	}, \
	{ SHIFTOP_CST,	MATCH_TEMPORARY,	MATCH_TEMPORARY,	MATCH_CONSTANT,		&CCodeGen_Arm::Emit_Shift_TmpTmpCst<SHIFTOP>	},

#endif
