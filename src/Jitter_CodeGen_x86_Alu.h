#ifndef _JITTER_CODEGEN_X86_ALU_H_
#define _JITTER_CODEGEN_X86_ALU_H_

template <typename ALUOP>
void CCodeGen_x86::Emit_Alu_RegRegReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	if(dst->Equals(src1))
	{
		((m_assembler).*(ALUOP::OpEd()))(m_registers[dst->m_valueLow], CX86Assembler::MakeRegisterAddress(m_registers[src2->m_valueLow]));
	}
	else
	{
		CX86Assembler::REGISTER src2register = m_registers[src2->m_valueLow];

		if(dst->Equals(src2))
		{
			m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeRegisterAddress(m_registers[src2->m_valueLow]));
			src2register = CX86Assembler::rAX;
		}

		m_assembler.MovEd(m_registers[dst->m_valueLow], CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
		((m_assembler).*(ALUOP::OpEd()))(m_registers[dst->m_valueLow], CX86Assembler::MakeRegisterAddress(src2register));
	}
}

template <typename ALUOP>
void CCodeGen_x86::Emit_Alu_RegRegRel(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_REGISTER);
	assert(src1->m_type == SYM_REGISTER);
	assert(src2->m_type == SYM_RELATIVE);

	if(!dst->Equals(src1))
	{
		m_assembler.MovEd(m_registers[dst->m_valueLow], CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	}

	((m_assembler).*(ALUOP::OpEd()))(m_registers[dst->m_valueLow], MakeRelativeSymbolAddress(src2));
}

template <typename ALUOP>
void CCodeGen_x86::Emit_Alu_RegRegCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	//We can optimize here if it's equal to zero
	assert(src2->m_valueLow != 0);

	if(!dst->Equals(src1))
	{
		m_assembler.MovEd(m_registers[dst->m_valueLow], CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	}

	((m_assembler).*(ALUOP::OpId()))(CX86Assembler::MakeRegisterAddress(m_registers[dst->m_valueLow]), src2->m_valueLow);
}

template <typename ALUOP>
void CCodeGen_x86::Emit_Alu_RegRegTmp(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_REGISTER);
	assert(src1->m_type == SYM_REGISTER);
	assert(src2->m_type == SYM_TEMPORARY);

	if(!dst->Equals(src1))
	{
		m_assembler.MovEd(m_registers[dst->m_valueLow], CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	}

	((m_assembler).*(ALUOP::OpEd()))(m_registers[dst->m_valueLow], MakeTemporarySymbolAddress(src2));
}

template <typename ALUOP>
void CCodeGen_x86::Emit_Alu_RegRelReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_REGISTER);
	assert(src1->m_type == SYM_RELATIVE);
	assert(src2->m_type == SYM_REGISTER);

	CX86Assembler::REGISTER src2register = m_registers[src2->m_valueLow];

	if(dst->Equals(src2))
	{
		m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeRegisterAddress(m_registers[src2->m_valueLow]));
		src2register = CX86Assembler::rAX;
	}

	m_assembler.MovEd(m_registers[dst->m_valueLow], MakeRelativeSymbolAddress(src1));
	((m_assembler).*(ALUOP::OpEd()))(m_registers[dst->m_valueLow], CX86Assembler::MakeRegisterAddress(src2register));
}

template <typename ALUOP>
void CCodeGen_x86::Emit_Alu_RegRelRel(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_REGISTER);
	assert(src1->m_type == SYM_RELATIVE);
	assert(src2->m_type == SYM_RELATIVE);

	m_assembler.MovEd(m_registers[dst->m_valueLow], MakeRelativeSymbolAddress(src1));
	((m_assembler).*(ALUOP::OpEd()))(m_registers[dst->m_valueLow], MakeRelativeSymbolAddress(src2));
}

template <typename ALUOP>
void CCodeGen_x86::Emit_Alu_RegRelCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_REGISTER);
	assert(src1->m_type == SYM_RELATIVE);
	assert(src2->m_type == SYM_CONSTANT);

	m_assembler.MovEd(m_registers[dst->m_valueLow], MakeRelativeSymbolAddress(src1));
	((m_assembler).*(ALUOP::OpId()))(CX86Assembler::MakeRegisterAddress(m_registers[dst->m_valueLow]), src2->m_valueLow);
}

template <typename ALUOP>
void CCodeGen_x86::Emit_Alu_RegCstReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(statement.op == OP_SUB);

	if(src1->m_valueLow)
	{
		m_assembler.MovId(CX86Assembler::rAX, src1->m_valueLow);
	}
	else
	{
		m_assembler.XorEd(CX86Assembler::rAX, CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX));
	}

	((m_assembler).*(ALUOP::OpEd()))(CX86Assembler::rAX, CX86Assembler::MakeRegisterAddress(m_registers[src2->m_valueLow]));
	m_assembler.MovEd(m_registers[dst->m_valueLow], CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX));
}

template <typename ALUOP>
void CCodeGen_x86::Emit_Alu_RegCstRel(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(statement.op == OP_SUB);
	assert(dst->m_type  == SYM_REGISTER);
	assert(src1->m_type == SYM_CONSTANT);
	assert(src2->m_type == SYM_RELATIVE);

	if(src1->m_valueLow)
	{
		m_assembler.MovId(m_registers[dst->m_valueLow], src1->m_valueLow);
	}
	else
	{
		m_assembler.XorEd(m_registers[dst->m_valueLow], CX86Assembler::MakeRegisterAddress(m_registers[dst->m_valueLow]));
	}

	((m_assembler).*(ALUOP::OpEd()))(m_registers[dst->m_valueLow], MakeRelativeSymbolAddress(src2));
}

template <typename ALUOP>
void CCodeGen_x86::Emit_Alu_RegTmpTmp(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	m_assembler.MovEd(m_registers[dst->m_valueLow], MakeTemporarySymbolAddress(src1));
	((m_assembler).*(ALUOP::OpEd()))(m_registers[dst->m_valueLow], MakeTemporarySymbolAddress(src2));
}

template <typename ALUOP>
void CCodeGen_x86::Emit_Alu_RegTmpCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	m_assembler.MovEd(m_registers[dst->m_valueLow], MakeTemporarySymbolAddress(src1));
	((m_assembler).*(ALUOP::OpId()))(CX86Assembler::MakeRegisterAddress(m_registers[dst->m_valueLow]), src2->m_valueLow);
}

template <typename ALUOP>
void CCodeGen_x86::Emit_Alu_RelRegReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_RELATIVE);
	assert(src1->m_type == SYM_REGISTER);
	assert(src2->m_type == SYM_REGISTER);

	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	((m_assembler).*(ALUOP::OpEd()))(CX86Assembler::rAX, CX86Assembler::MakeRegisterAddress(m_registers[src2->m_valueLow]));
	m_assembler.MovGd(MakeRelativeSymbolAddress(dst), CX86Assembler::rAX);
}

template <typename ALUOP>
void CCodeGen_x86::Emit_Alu_RelRegRel(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_RELATIVE);
	assert(src1->m_type == SYM_REGISTER);
	assert(src2->m_type == SYM_RELATIVE);

	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	((m_assembler).*(ALUOP::OpEd()))(CX86Assembler::rAX, MakeRelativeSymbolAddress(src2));
	m_assembler.MovGd(MakeRelativeSymbolAddress(dst), CX86Assembler::rAX);
}

template <typename ALUOP>
void CCodeGen_x86::Emit_Alu_RelRegCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_RELATIVE);
	assert(src1->m_type == SYM_REGISTER);
	assert(src2->m_type == SYM_CONSTANT);

	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	((m_assembler).*(ALUOP::OpId()))(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX), src2->m_valueLow);
	m_assembler.MovGd(MakeRelativeSymbolAddress(dst), CX86Assembler::rAX);
}

template <typename ALUOP>
void CCodeGen_x86::Emit_Alu_RelRegTmp(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_RELATIVE);
	assert(src1->m_type == SYM_REGISTER);
	assert(src2->m_type == SYM_TEMPORARY);

	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	((m_assembler).*(ALUOP::OpEd()))(CX86Assembler::rAX, MakeTemporarySymbolAddress(src2));
	m_assembler.MovGd(MakeRelativeSymbolAddress(dst), CX86Assembler::rAX);
}

template <typename ALUOP>
void CCodeGen_x86::Emit_Alu_RelRelReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_RELATIVE);
	assert(src1->m_type == SYM_RELATIVE);
	assert(src2->m_type == SYM_REGISTER);

	m_assembler.MovEd(CX86Assembler::rAX, MakeRelativeSymbolAddress(src1));
	((m_assembler).*(ALUOP::OpEd()))(CX86Assembler::rAX, CX86Assembler::MakeRegisterAddress(m_registers[src2->m_valueLow]));
	m_assembler.MovGd(MakeRelativeSymbolAddress(dst), CX86Assembler::rAX);
}

template <typename ALUOP>
void CCodeGen_x86::Emit_Alu_RelRelCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_RELATIVE);
	assert(src1->m_type == SYM_RELATIVE);
	assert(src2->m_type == SYM_CONSTANT);

	m_assembler.MovEd(CX86Assembler::rAX, MakeRelativeSymbolAddress(src1));
	((m_assembler).*(ALUOP::OpId()))(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX), src2->m_valueLow);
	m_assembler.MovGd(MakeRelativeSymbolAddress(dst), CX86Assembler::rAX);
}

template <typename ALUOP>
void CCodeGen_x86::Emit_Alu_RelRelRel(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	m_assembler.MovEd(CX86Assembler::rAX, MakeRelativeSymbolAddress(src1));
	((m_assembler).*(ALUOP::OpEd()))(CX86Assembler::rAX, MakeRelativeSymbolAddress(src2));
	m_assembler.MovGd(MakeRelativeSymbolAddress(dst), CX86Assembler::rAX);
}

template <typename ALUOP> 
void CCodeGen_x86::Emit_Alu_RelCstReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_RELATIVE);
	assert(src1->m_type == SYM_CONSTANT);
	assert(src2->m_type == SYM_REGISTER);

	m_assembler.MovId(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX), src1->m_valueLow);
	((m_assembler).*(ALUOP::OpEd()))(CX86Assembler::rAX, CX86Assembler::MakeRegisterAddress(m_registers[src2->m_valueLow]));
	m_assembler.MovGd(MakeRelativeSymbolAddress(dst), CX86Assembler::rAX);
}

template <typename ALUOP> 
void CCodeGen_x86::Emit_Alu_RelCstRel(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_RELATIVE);
	assert(src1->m_type == SYM_CONSTANT);
	assert(src2->m_type == SYM_RELATIVE);

	m_assembler.MovId(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX), src1->m_valueLow);
	((m_assembler).*(ALUOP::OpEd()))(CX86Assembler::rAX, MakeRelativeSymbolAddress(src2));
	m_assembler.MovGd(MakeRelativeSymbolAddress(dst), CX86Assembler::rAX);
}

template <typename ALUOP> 
void CCodeGen_x86::Emit_Alu_RelTmpCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_RELATIVE);
	assert(src1->m_type == SYM_TEMPORARY);
	assert(src2->m_type == SYM_CONSTANT);

	m_assembler.MovEd(CX86Assembler::rAX, MakeTemporarySymbolAddress(src1));
	((m_assembler).*(ALUOP::OpId()))(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX), src2->m_valueLow);
	m_assembler.MovGd(MakeRelativeSymbolAddress(dst), CX86Assembler::rAX);
}

template <typename ALUOP>
void CCodeGen_x86::Emit_Alu_TmpRegReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	((m_assembler).*(ALUOP::OpEd()))(CX86Assembler::rAX, CX86Assembler::MakeRegisterAddress(m_registers[src2->m_valueLow]));
	m_assembler.MovGd(MakeTemporarySymbolAddress(dst), CX86Assembler::rAX);
}

template <typename ALUOP>
void CCodeGen_x86::Emit_Alu_TmpRegRel(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_TEMPORARY);
	assert(src1->m_type == SYM_REGISTER);
	assert(src2->m_type == SYM_RELATIVE);

	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	((m_assembler).*(ALUOP::OpEd()))(CX86Assembler::rAX, MakeRelativeSymbolAddress(src2));
	m_assembler.MovGd(MakeTemporarySymbolAddress(dst), CX86Assembler::rAX);
}

template <typename ALUOP>
void CCodeGen_x86::Emit_Alu_TmpRegCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_TEMPORARY);
	assert(src1->m_type == SYM_REGISTER);
	assert(src2->m_type == SYM_CONSTANT);

	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	((m_assembler).*(ALUOP::OpId()))(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX), src2->m_valueLow);
	m_assembler.MovGd(MakeTemporarySymbolAddress(dst), CX86Assembler::rAX);
}

template <typename ALUOP>
void CCodeGen_x86::Emit_Alu_TmpRelRel(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_TEMPORARY);
	assert(src1->m_type == SYM_RELATIVE);
	assert(src2->m_type == SYM_RELATIVE);

	m_assembler.MovEd(CX86Assembler::rAX, MakeRelativeSymbolAddress(src1));
	((m_assembler).*(ALUOP::OpEd()))(CX86Assembler::rAX, MakeRelativeSymbolAddress(src2));
	m_assembler.MovGd(MakeTemporarySymbolAddress(dst), CX86Assembler::rAX);
}

template <typename ALUOP>
void CCodeGen_x86::Emit_Alu_TmpRelCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_TEMPORARY);
	assert(src1->m_type == SYM_RELATIVE);
	assert(src2->m_type == SYM_CONSTANT);

	m_assembler.MovEd(CX86Assembler::rAX, MakeRelativeSymbolAddress(src1));
	((m_assembler).*(ALUOP::OpId()))(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX), src2->m_valueLow);
	m_assembler.MovGd(MakeTemporarySymbolAddress(dst), CX86Assembler::rAX);
}

template <typename ALUOP>
void CCodeGen_x86::Emit_Alu_TmpTmpCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_TEMPORARY);
	assert(src1->m_type == SYM_TEMPORARY);
	assert(src2->m_type == SYM_CONSTANT);

	m_assembler.MovEd(CX86Assembler::rAX, MakeTemporarySymbolAddress(src1));
	((m_assembler).*(ALUOP::OpId()))(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX), src2->m_valueLow);
	m_assembler.MovGd(MakeTemporarySymbolAddress(dst), CX86Assembler::rAX);
}

#define ALU_CONST_MATCHERS(ALUOP_CST, ALUOP) \
	{ ALUOP_CST,	MATCH_REGISTER,		MATCH_REGISTER,		MATCH_REGISTER,		&CCodeGen_x86::Emit_Alu_RegRegReg<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_REGISTER,		MATCH_REGISTER,		MATCH_RELATIVE,		&CCodeGen_x86::Emit_Alu_RegRegRel<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_REGISTER,		MATCH_REGISTER,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_Alu_RegRegCst<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_REGISTER,		MATCH_REGISTER,		MATCH_TEMPORARY,	&CCodeGen_x86::Emit_Alu_RegRegTmp<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_REGISTER,		MATCH_RELATIVE,		MATCH_REGISTER,		&CCodeGen_x86::Emit_Alu_RegRelReg<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_REGISTER,		MATCH_RELATIVE,		MATCH_RELATIVE,		&CCodeGen_x86::Emit_Alu_RegRelRel<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_REGISTER,		MATCH_RELATIVE,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_Alu_RegRelCst<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_REGISTER,		MATCH_CONSTANT,		MATCH_REGISTER,		&CCodeGen_x86::Emit_Alu_RegCstReg<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_REGISTER,		MATCH_CONSTANT,		MATCH_RELATIVE,		&CCodeGen_x86::Emit_Alu_RegCstRel<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_REGISTER,		MATCH_TEMPORARY,	MATCH_CONSTANT,		&CCodeGen_x86::Emit_Alu_RegTmpCst<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_REGISTER,		MATCH_TEMPORARY,	MATCH_TEMPORARY,	&CCodeGen_x86::Emit_Alu_RegTmpTmp<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_RELATIVE,		MATCH_REGISTER,		MATCH_REGISTER,		&CCodeGen_x86::Emit_Alu_RelRegReg<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_RELATIVE,		MATCH_REGISTER,		MATCH_RELATIVE,		&CCodeGen_x86::Emit_Alu_RelRegRel<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_RELATIVE,		MATCH_REGISTER,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_Alu_RelRegCst<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_RELATIVE,		MATCH_REGISTER,		MATCH_TEMPORARY,	&CCodeGen_x86::Emit_Alu_RelRegTmp<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_RELATIVE,		MATCH_RELATIVE,		MATCH_REGISTER,		&CCodeGen_x86::Emit_Alu_RelRelReg<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_RELATIVE,		MATCH_RELATIVE,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_Alu_RelRelCst<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_RELATIVE,		MATCH_RELATIVE,		MATCH_RELATIVE,		&CCodeGen_x86::Emit_Alu_RelRelRel<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_RELATIVE,		MATCH_CONSTANT,		MATCH_REGISTER,		&CCodeGen_x86::Emit_Alu_RelCstReg<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_RELATIVE,		MATCH_CONSTANT,		MATCH_RELATIVE,		&CCodeGen_x86::Emit_Alu_RelCstRel<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_RELATIVE,		MATCH_TEMPORARY,	MATCH_CONSTANT,		&CCodeGen_x86::Emit_Alu_RelTmpCst<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_TEMPORARY,	MATCH_REGISTER,		MATCH_REGISTER,		&CCodeGen_x86::Emit_Alu_TmpRegReg<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_TEMPORARY,	MATCH_REGISTER,		MATCH_RELATIVE,		&CCodeGen_x86::Emit_Alu_TmpRegRel<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_TEMPORARY,	MATCH_REGISTER,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_Alu_TmpRegCst<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_TEMPORARY,	MATCH_RELATIVE,		MATCH_RELATIVE,		&CCodeGen_x86::Emit_Alu_TmpRelRel<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_TEMPORARY,	MATCH_RELATIVE,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_Alu_TmpRelCst<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_TEMPORARY,	MATCH_TEMPORARY,	MATCH_CONSTANT,		&CCodeGen_x86::Emit_Alu_TmpTmpCst<ALUOP>		}, 

#endif
