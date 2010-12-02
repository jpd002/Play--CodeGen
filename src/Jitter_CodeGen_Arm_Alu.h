#ifndef _JITTER_CODEGEN_ARM_ALU_H_
#define _JITTER_CODEGEN_ARM_ALU_H_

template <typename ALUOP>
void CCodeGen_Arm::Alu_GenericRegRegCst(CArmAssembler::REGISTER dst, CArmAssembler::REGISTER src1, uint32 src2)
{
	bool supportsNegative	= ALUOP::OpImmNeg() != NULL;
	bool supportsComplement = ALUOP::OpImmNot() != NULL;
	
	uint8 immediate = 0;
	uint8 shiftAmount = 0;
	if(TryGetAluImmediateParams(src2, immediate, shiftAmount))
	{
		((m_assembler).*(ALUOP::OpImm()))(dst, src1, CArmAssembler::MakeImmediateAluOperand(immediate, shiftAmount));
	}
	else if(supportsNegative && TryGetAluImmediateParams(-src2, immediate, shiftAmount))
	{
		((m_assembler).*(ALUOP::OpImmNeg()))(dst, src1, CArmAssembler::MakeImmediateAluOperand(immediate, shiftAmount));
	}
	else if(supportsComplement && TryGetAluImmediateParams(~src2, immediate, shiftAmount))
	{
		((m_assembler).*(ALUOP::OpImmNot()))(dst, src1, CArmAssembler::MakeImmediateAluOperand(immediate, shiftAmount));
	}
	else
	{
		CArmAssembler::REGISTER cstReg = CArmAssembler::r0;
		assert(cstReg != dst && cstReg != src1);
		LoadConstantInRegister(cstReg, src2);
		((m_assembler).*(ALUOP::OpReg()))(dst, src1, cstReg);
	}	
}

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
void CCodeGen_Arm::Emit_Alu_RegRegRel(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();
	
	assert(dst->m_type  == SYM_REGISTER);
	assert(src1->m_type == SYM_REGISTER);
	assert(src2->m_type == SYM_RELATIVE);
	
	CArmAssembler::REGISTER tmpReg = CArmAssembler::r1;
	LoadRelativeInRegister(tmpReg, src2);
	((m_assembler).*(ALUOP::OpReg()))(g_registers[dst->m_valueLow], g_registers[src1->m_valueLow], tmpReg);
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
	
	Alu_GenericRegRegCst<ALUOP>(g_registers[dst->m_valueLow], g_registers[src1->m_valueLow], src2->m_valueLow);
}

template <typename ALUOP>
void CCodeGen_Arm::Emit_Alu_RegRelRel(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();
	
	assert(dst->m_type  == SYM_REGISTER);
	assert(src1->m_type == SYM_RELATIVE);
	assert(src2->m_type == SYM_RELATIVE);
	
	CArmAssembler::REGISTER src1Reg = CArmAssembler::r0;
	CArmAssembler::REGISTER src2Reg = CArmAssembler::r1;
	
	LoadRelativeInRegister(src1Reg, src1);
	LoadRelativeInRegister(src2Reg, src2);
	((m_assembler).*(ALUOP::OpReg()))(g_registers[dst->m_valueLow], src1Reg, src2Reg);
}

template <typename ALUOP>
void CCodeGen_Arm::Emit_Alu_RegRelCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();
	
	assert(dst->m_type  == SYM_REGISTER);
	assert(src1->m_type == SYM_RELATIVE);
	assert(src2->m_type == SYM_CONSTANT);
	
	CArmAssembler::REGISTER tmpReg = CArmAssembler::r1;
	LoadRelativeInRegister(tmpReg, src1);
	Alu_GenericRegRegCst<ALUOP>(g_registers[dst->m_valueLow], tmpReg, src2->m_valueLow);
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

template <typename ALUOP>
void CCodeGen_Arm::Emit_Alu_RelRegReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();
	
	assert(dst->m_type  == SYM_RELATIVE);
	assert(src1->m_type == SYM_REGISTER);
	assert(src2->m_type == SYM_REGISTER);
	
	CArmAssembler::REGISTER tmpReg = CArmAssembler::r1;
	((m_assembler).*(ALUOP::OpReg()))(tmpReg, g_registers[src1->m_valueLow], g_registers[src2->m_valueLow]);
	StoreRegisterInRelative(dst, tmpReg);
}

template <typename ALUOP>
void CCodeGen_Arm::Emit_Alu_RelRegRel(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();
	
	assert(dst->m_type  == SYM_RELATIVE);
	assert(src1->m_type == SYM_REGISTER);
	assert(src2->m_type == SYM_RELATIVE);
	
	CArmAssembler::REGISTER tmpReg = CArmAssembler::r1;
	LoadRelativeInRegister(tmpReg, src2);
	((m_assembler).*(ALUOP::OpReg()))(tmpReg, g_registers[src1->m_valueLow], tmpReg);
	StoreRegisterInRelative(dst, tmpReg);
}

template <typename ALUOP>
void CCodeGen_Arm::Emit_Alu_RelRegCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();
	
	assert(dst->m_type  == SYM_RELATIVE);
	assert(src1->m_type == SYM_REGISTER);
	assert(src2->m_type == SYM_CONSTANT);
	
	CArmAssembler::REGISTER tmpReg = CArmAssembler::r1;
	Alu_GenericRegRegCst<ALUOP>(tmpReg, g_registers[src1->m_valueLow], src2->m_valueLow);
	StoreRegisterInRelative(dst, tmpReg);
}

template <typename ALUOP>
void CCodeGen_Arm::Emit_Alu_RelRelReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();
	
	assert(dst->m_type  == SYM_RELATIVE);
	assert(src1->m_type == SYM_RELATIVE);
	assert(src2->m_type == SYM_REGISTER);
	
	CArmAssembler::REGISTER src1Reg = CArmAssembler::r0;
	
	LoadRelativeInRegister(src1Reg, src1);
	((m_assembler).*(ALUOP::OpReg()))(src1Reg, src1Reg, g_registers[src2->m_valueLow]);
	StoreRegisterInRelative(dst, src1Reg);
}

template <typename ALUOP>
void CCodeGen_Arm::Emit_Alu_RelRelRel(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();
	
	assert(dst->m_type  == SYM_RELATIVE);
	assert(src1->m_type == SYM_RELATIVE);
	assert(src2->m_type == SYM_RELATIVE);
	
	CArmAssembler::REGISTER src1Reg = CArmAssembler::r0;
	CArmAssembler::REGISTER src2Reg = CArmAssembler::r1;
	
	LoadRelativeInRegister(src1Reg, src1);
	LoadRelativeInRegister(src2Reg, src2);
	((m_assembler).*(ALUOP::OpReg()))(src1Reg, src1Reg, src2Reg);
	StoreRegisterInRelative(dst, src1Reg);
}

template <typename ALUOP>
void CCodeGen_Arm::Emit_Alu_RelRelCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();
	
	assert(dst->m_type  == SYM_RELATIVE);
	assert(src1->m_type == SYM_RELATIVE);
	assert(src2->m_type == SYM_CONSTANT);
	
	CArmAssembler::REGISTER tmpReg = CArmAssembler::r1;
	LoadRelativeInRegister(tmpReg, src1);
	Alu_GenericRegRegCst<ALUOP>(tmpReg, tmpReg, src2->m_valueLow);
	StoreRegisterInRelative(dst, tmpReg);
}

template <typename ALUOP>
void CCodeGen_Arm::Emit_Alu_TmpRegCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();
	
	assert(dst->m_type  == SYM_TEMPORARY);
	assert(src1->m_type == SYM_REGISTER);
	assert(src2->m_type == SYM_CONSTANT);
	
	CArmAssembler::REGISTER tmpReg = CArmAssembler::r1;
	Alu_GenericRegRegCst<ALUOP>(tmpReg, g_registers[src1->m_valueLow], src2->m_valueLow);
	StoreRegisterInTemporary(dst, tmpReg);
}

template <typename ALUOP>
void CCodeGen_Arm::Emit_Alu_TmpRelCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();
	
	assert(dst->m_type  == SYM_TEMPORARY);
	assert(src1->m_type == SYM_RELATIVE);
	assert(src2->m_type == SYM_CONSTANT);
	
	CArmAssembler::REGISTER tmpReg = CArmAssembler::r1;
	LoadRelativeInRegister(tmpReg, src1);
	Alu_GenericRegRegCst<ALUOP>(tmpReg, tmpReg, src2->m_valueLow);
	StoreRegisterInTemporary(dst, tmpReg);
}

template <typename ALUOP>
void CCodeGen_Arm::Emit_Alu_TmpTmpCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();
	
	assert(dst->m_type  == SYM_TEMPORARY);
	assert(src1->m_type == SYM_TEMPORARY);
	assert(src2->m_type == SYM_CONSTANT);
	
	CArmAssembler::REGISTER tmpReg = CArmAssembler::r1;
	LoadTemporaryInRegister(tmpReg, src1);
	Alu_GenericRegRegCst<ALUOP>(tmpReg, tmpReg, src2->m_valueLow);
	StoreRegisterInTemporary(dst, tmpReg);
}

#define ALU_CONST_MATCHERS(ALUOP_CST, ALUOP) \
	{ ALUOP_CST,	MATCH_REGISTER,		MATCH_REGISTER,		MATCH_REGISTER,		&CCodeGen_Arm::Emit_Alu_RegRegReg<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_REGISTER,		MATCH_REGISTER,		MATCH_RELATIVE,		&CCodeGen_Arm::Emit_Alu_RegRegRel<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_REGISTER,		MATCH_REGISTER,		MATCH_CONSTANT,		&CCodeGen_Arm::Emit_Alu_RegRegCst<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_REGISTER,		MATCH_RELATIVE,		MATCH_RELATIVE,		&CCodeGen_Arm::Emit_Alu_RegRelRel<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_REGISTER,		MATCH_RELATIVE,		MATCH_CONSTANT,		&CCodeGen_Arm::Emit_Alu_RegRelCst<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_REGISTER,		MATCH_CONSTANT,		MATCH_REGISTER,		&CCodeGen_Arm::Emit_Alu_RegCstReg<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_RELATIVE,		MATCH_REGISTER,		MATCH_REGISTER,		&CCodeGen_Arm::Emit_Alu_RelRegReg<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_RELATIVE,		MATCH_REGISTER,		MATCH_RELATIVE,		&CCodeGen_Arm::Emit_Alu_RelRegRel<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_RELATIVE,		MATCH_REGISTER,		MATCH_CONSTANT,		&CCodeGen_Arm::Emit_Alu_RelRegCst<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_RELATIVE,		MATCH_RELATIVE,		MATCH_REGISTER,		&CCodeGen_Arm::Emit_Alu_RelRelReg<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_RELATIVE,		MATCH_RELATIVE,		MATCH_RELATIVE,		&CCodeGen_Arm::Emit_Alu_RelRelRel<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_RELATIVE,		MATCH_RELATIVE,		MATCH_CONSTANT,		&CCodeGen_Arm::Emit_Alu_RelRelCst<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_TEMPORARY,	MATCH_REGISTER,		MATCH_CONSTANT,		&CCodeGen_Arm::Emit_Alu_TmpRegCst<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_TEMPORARY,	MATCH_RELATIVE,		MATCH_CONSTANT,		&CCodeGen_Arm::Emit_Alu_TmpRelCst<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_TEMPORARY,	MATCH_TEMPORARY,	MATCH_CONSTANT,		&CCodeGen_Arm::Emit_Alu_TmpTmpCst<ALUOP>		},

#endif
