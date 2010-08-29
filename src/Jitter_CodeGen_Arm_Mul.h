#ifndef _JITTER_CODEGEN_ARM_MUL_H_
#define _JITTER_CODEGEN_ARM_MUL_H_

template <bool isSigned>
void CCodeGen_Arm::Mul_GenericTmp64RegReg(CSymbol* dst, CArmAssembler::REGISTER src1, CArmAssembler::REGISTER src2)
{
	CArmAssembler::REGISTER resLo = CArmAssembler::r0;
	CArmAssembler::REGISTER resHi = CArmAssembler::r1;
	
	assert(dst->m_type == SYM_TEMPORARY64);
	assert(resLo != src1 && resLo != src2);
	assert(resHi != src1 && resHi != src2);
	
	if(isSigned)
	{
		m_assembler.Smull(resLo, resHi, src1, src2);
	}
	else
	{
		m_assembler.Umull(resLo, resHi, src1, src2);
	}
	
	m_assembler.Str(resLo, CArmAssembler::rSP, CArmAssembler::MakeImmediateLdrAddress(dst->m_stackLocation + m_stackLevel + 0));
	m_assembler.Str(resHi, CArmAssembler::rSP, CArmAssembler::MakeImmediateLdrAddress(dst->m_stackLocation + m_stackLevel + 4));
}

template <bool isSigned>
void CCodeGen_Arm::Emit_MulTmp64RegCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();
	
	assert(dst->m_type  == SYM_TEMPORARY64);
	assert(src1->m_type == SYM_REGISTER);
	assert(src2->m_type == SYM_CONSTANT);
	
	CArmAssembler::REGISTER cstReg = CArmAssembler::r3;
	LoadConstantInRegister(cstReg, src2->m_valueLow);
	Mul_GenericTmp64RegReg<isSigned>(dst, g_registers[src1->m_valueLow], cstReg);
}

template <bool isSigned>
void CCodeGen_Arm::Emit_MulTmp64RegReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();
	
	assert(dst->m_type  == SYM_TEMPORARY64);
	assert(src1->m_type == SYM_REGISTER);
	assert(src2->m_type == SYM_REGISTER);
	
	Mul_GenericTmp64RegReg<isSigned>(dst, g_registers[src1->m_valueLow], g_registers[src2->m_valueLow]);
}

template <bool isSigned>
void CCodeGen_Arm::Emit_MulTmp64RegRel(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();
	
	assert(dst->m_type  == SYM_TEMPORARY64);
	assert(src1->m_type == SYM_REGISTER);
	assert(src2->m_type == SYM_RELATIVE);
	
	CArmAssembler::REGISTER tmpReg = CArmAssembler::r3;
	LoadRelativeInRegister(tmpReg, src2);
	Mul_GenericTmp64RegReg<isSigned>(dst, g_registers[src1->m_valueLow], tmpReg);
}

#endif
