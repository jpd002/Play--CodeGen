#ifndef _JITTER_CODEGEN_ARM_ALU_H_
#define _JITTER_CODEGEN_ARM_ALU_H_

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
void CCodeGen_Arm::Emit_Alu_RegRelCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();
	
	assert(dst->m_type  == SYM_REGISTER);
	assert(src1->m_type == SYM_RELATIVE);
	assert(src2->m_type == SYM_CONSTANT);
	
	CArmAssembler::REGISTER cstReg = CArmAssembler::r0;
	CArmAssembler::REGISTER tmpReg = CArmAssembler::r1;
	
	LoadRelativeInRegister(tmpReg, src1);
	
	uint8 immediate = 0;
	uint8 shiftAmount = 0;
	if(TryGetAluImmediateParams(src2->m_valueLow, immediate, shiftAmount))
	{
		((m_assembler).*(ALUOP::OpImm()))(g_registers[dst->m_valueLow], tmpReg, CArmAssembler::MakeImmediateAluOperand(immediate, shiftAmount));
	}
	else
	{
		LoadConstantInRegister(cstReg, src2->m_valueLow);
		((m_assembler).*(ALUOP::OpReg()))(g_registers[dst->m_valueLow], tmpReg, cstReg);
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

template <typename ALUOP>
void CCodeGen_Arm::Emit_Alu_RelRelCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();
	
	assert(dst->m_type  == SYM_RELATIVE);
	assert(src1->m_type == SYM_RELATIVE);
	assert(src2->m_type == SYM_CONSTANT);
	
	CArmAssembler::REGISTER cstReg = CArmAssembler::r0;
	CArmAssembler::REGISTER tmpReg = CArmAssembler::r1;
	
	LoadRelativeInRegister(tmpReg, src1);
	
	uint8 immediate = 0;
	uint8 shiftAmount = 0;
	if(TryGetAluImmediateParams(src2->m_valueLow, immediate, shiftAmount))
	{
		((m_assembler).*(ALUOP::OpImm()))(tmpReg, tmpReg, CArmAssembler::MakeImmediateAluOperand(immediate, shiftAmount));
	}
	else
	{
		LoadConstantInRegister(cstReg, src2->m_valueLow);
		((m_assembler).*(ALUOP::OpReg()))(tmpReg, tmpReg, cstReg);
	}
	
	StoreRegisterInRelative(dst, tmpReg);
}

#define ALU_CONST_MATCHERS(ALUOP_CST, ALUOP) \
	{ ALUOP_CST,	MATCH_REGISTER,		MATCH_REGISTER,		MATCH_REGISTER,		&CCodeGen_Arm::Emit_Alu_RegRegReg<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_REGISTER,		MATCH_REGISTER,		MATCH_CONSTANT,		&CCodeGen_Arm::Emit_Alu_RegRegCst<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_REGISTER,		MATCH_RELATIVE,		MATCH_CONSTANT,		&CCodeGen_Arm::Emit_Alu_RegRelCst<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_REGISTER,		MATCH_CONSTANT,		MATCH_REGISTER,		&CCodeGen_Arm::Emit_Alu_RegCstReg<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_RELATIVE,		MATCH_RELATIVE,		MATCH_CONSTANT,		&CCodeGen_Arm::Emit_Alu_RelRelCst<ALUOP>		},

#endif
