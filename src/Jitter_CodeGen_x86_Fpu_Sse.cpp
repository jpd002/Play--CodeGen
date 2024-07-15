#include "Jitter_CodeGen_x86.h"
#include <stdexcept>

using namespace Jitter;

void CCodeGen_x86::CommitSymbolRegisterFp32Sse(CSymbol* symbol, CX86Assembler::XMMREGISTER usedRegister)
{
	switch(symbol->m_type)
	{
	case SYM_FP_REGISTER32:
		assert(usedRegister == m_mdRegisters[symbol->m_valueLow]);
		break;
	case SYM_FP_TEMPORARY32:
	case SYM_FP_RELATIVE32:
		m_assembler.MovssEd(MakeMemoryFp32SymbolAddress(symbol), usedRegister);
		break;
	default:
		throw std::runtime_error("Invalid symbol type.");
		break;
	}
}

template <typename FPUOP>
void CCodeGen_x86::Emit_Fp32_RegVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	((m_assembler).*(FPUOP::OpEd()))(m_mdRegisters[dst->m_valueLow], MakeVariableFp32SymbolAddress(src1));
}

template <typename FPOP>
void CCodeGen_x86::Emit_Fp32_MemVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstRegister = CX86Assembler::xMM0;

	((m_assembler).*(FPOP::OpEd()))(dstRegister, MakeVariableFp32SymbolAddress(src1));
	m_assembler.MovssEd(MakeMemoryFp32SymbolAddress(dst), dstRegister);
}

template <typename FPOP>
void CCodeGen_x86::Emit_Fp32_RegRegReg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	if(dst->Equals(src1))
	{
		((m_assembler).*(FPOP::OpEd()))(m_mdRegisters[dst->m_valueLow],
		                                CX86Assembler::MakeXmmRegisterAddress(m_mdRegisters[src2->m_valueLow]));
	}
	else
	{
		auto src2Register = m_mdRegisters[src2->m_valueLow];

		if(dst->Equals(src2))
		{
			m_assembler.MovssEd(CX86Assembler::xMM0, CX86Assembler::MakeXmmRegisterAddress(m_mdRegisters[src2->m_valueLow]));
			src2Register = CX86Assembler::xMM0;
		}

		m_assembler.MovssEd(m_mdRegisters[dst->m_valueLow], CX86Assembler::MakeXmmRegisterAddress(m_mdRegisters[src1->m_valueLow]));
		((m_assembler).*(FPOP::OpEd()))(m_mdRegisters[dst->m_valueLow], CX86Assembler::MakeXmmRegisterAddress(src2Register));
	}
}

template <typename FPOP>
void CCodeGen_x86::Emit_Fp32_RegMemReg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstRegister = m_mdRegisters[dst->m_valueLow];
	auto src2Register = m_mdRegisters[src2->m_valueLow];

	if(dst->Equals(src2))
	{
		m_assembler.MovssEd(CX86Assembler::xMM0, CX86Assembler::MakeXmmRegisterAddress(src2Register));
		src2Register = CX86Assembler::xMM0;
	}

	m_assembler.MovssEd(dstRegister, MakeMemoryFp32SymbolAddress(src1));
	((m_assembler).*(FPOP::OpEd()))(dstRegister, CX86Assembler::MakeXmmRegisterAddress(src2Register));
}

template <typename FPOP>
void CCodeGen_x86::Emit_Fp32_RegVarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	//If we get in here, it must absolutely mean that the second source isn't a register
	//Otherwise, some of the assumuptions done below will be wrong (dst mustn't be equal to src2)
	assert(src2->m_type != SYM_FP_REGISTER32);

	auto dstRegister = m_mdRegisters[dst->m_valueLow];

	if(!dst->Equals(src1))
	{
		m_assembler.MovssEd(dstRegister, MakeVariableFp32SymbolAddress(src1));
	}

	((m_assembler).*(FPOP::OpEd()))(dstRegister, MakeVariableFp32SymbolAddress(src2));
}

template <typename FPUOP>
void CCodeGen_x86::Emit_Fp32_MemVarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstRegister = CX86Assembler::xMM0;

	m_assembler.MovssEd(dstRegister, MakeVariableFp32SymbolAddress(src1));
	((m_assembler).*(FPUOP::OpEd()))(dstRegister, MakeVariableFp32SymbolAddress(src2));
	m_assembler.MovssEd(MakeMemoryFp32SymbolAddress(dst), dstRegister);
}

template <typename MDOPSINGLEOP>
void CCodeGen_x86::Emit_Fp32_SingleOp_RegVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto resultRegister = m_mdRegisters[dst->m_valueLow];

	if(!dst->Equals(src1))
	{
		m_assembler.MovssEd(resultRegister, MakeVariableFp32SymbolAddress(src1));
	}

	((*this).*(MDOPSINGLEOP::OpVr()))(resultRegister);
}

template <typename MDOPSINGLEOP>
void CCodeGen_x86::Emit_Fp32_SingleOp_MemVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto resultRegister = CX86Assembler::xMM0;

	m_assembler.MovssEd(resultRegister, MakeVariableFp32SymbolAddress(src1));
	((*this).*(MDOPSINGLEOP::OpVr()))(resultRegister);
	m_assembler.MovssEd(MakeMemoryFp32SymbolAddress(dst), resultRegister);
}

void CCodeGen_x86::Emit_Fp32_Mov_RegMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type == SYM_FP_REGISTER32);

	m_assembler.MovssEd(m_mdRegisters[dst->m_valueLow], MakeMemoryFp32SymbolAddress(src1));
}

void CCodeGen_x86::Emit_Fp32_Mov_MemReg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(src1->m_type == SYM_FP_REGISTER32);

	m_assembler.MovssEd(MakeMemoryFp32SymbolAddress(dst), m_mdRegisters[src1->m_valueLow]);
}

void CCodeGen_x86::Emit_Fp32_LdCst_RegCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type == SYM_FP_REGISTER32);
	assert(src1->m_type == SYM_CONSTANT);

	auto tmpRegister = CX86Assembler::rAX;

	m_assembler.MovId(tmpRegister, src1->m_valueLow);
	m_assembler.MovdVo(m_mdRegisters[dst->m_valueLow], CX86Assembler::MakeRegisterAddress(tmpRegister));
}

void CCodeGen_x86::Emit_Fp_CmpS_VarVarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDef(dst, CX86Assembler::rAX);

	auto conditionCode = GetSseConditionCode(statement.jmpCondition);
	m_assembler.MovssEd(CX86Assembler::xMM0, MakeVariableFp32SymbolAddress(src1));
	m_assembler.CmpssEd(CX86Assembler::xMM0, MakeVariableFp32SymbolAddress(src2), conditionCode);
	m_assembler.MovdVo(CX86Assembler::MakeRegisterAddress(dstReg), CX86Assembler::xMM0);

	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_x86::Emit_Fp_CmpS_VarMemCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_CONSTANT);

	auto conditionCode = GetSseConditionCode(statement.jmpCondition);
	auto dstReg = PrepareSymbolRegisterDef(dst, CX86Assembler::rAX);
	auto src1Reg = CX86Assembler::xMM0;
	auto src2Reg = CX86Assembler::xMM1;

	if(src2->m_valueLow == 0)
	{
		m_assembler.PxorVo(src2Reg, CX86Assembler::MakeXmmRegisterAddress(src2Reg));
	}
	else
	{
		auto cstReg = CX86Assembler::rDX;
		assert(dstReg != cstReg);
		m_assembler.MovId(cstReg, src2->m_valueLow);
		m_assembler.MovdVo(src2Reg, CX86Assembler::MakeRegisterAddress(cstReg));
	}

	m_assembler.MovssEd(src1Reg, MakeMemoryFp32SymbolAddress(src1));
	m_assembler.CmpssEd(src1Reg, CX86Assembler::MakeXmmRegisterAddress(src2Reg), conditionCode);
	m_assembler.MovdVo(CX86Assembler::MakeRegisterAddress(dstReg), src1Reg);

	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_x86::Emit_Fp_RsqrtS_VarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto tmpIntRegister = CX86Assembler::rAX;
	auto dstRegister = PrepareSymbolRegisterDefFp32(dst, CX86Assembler::xMM0);
	auto sqrtRegister = CX86Assembler::xMM1;

	m_assembler.SqrtssEd(sqrtRegister, MakeVariableFp32SymbolAddress(src1));
	m_assembler.MovId(tmpIntRegister, 0x3F800000);
	m_assembler.MovdVo(dstRegister, CX86Assembler::MakeRegisterAddress(tmpIntRegister));
	m_assembler.DivssEd(dstRegister, CX86Assembler::MakeXmmRegisterAddress(sqrtRegister));

	CommitSymbolRegisterFp32Sse(dst, dstRegister);
}

void CCodeGen_x86::Emit_Fp_RcplS_RegVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type == SYM_FP_REGISTER32);

	auto tmpIntRegister = CX86Assembler::rAX;
	auto dstRegister = m_mdRegisters[dst->m_valueLow];

	m_assembler.MovId(tmpIntRegister, 0x3F800000);
	if(dst->Equals(src1))
	{
		auto src1Register = CX86Assembler::xMM1;
		m_assembler.MovssEd(src1Register, CX86Assembler::MakeXmmRegisterAddress(m_mdRegisters[src1->m_valueLow]));
		m_assembler.MovdVo(dstRegister, CX86Assembler::MakeRegisterAddress(tmpIntRegister));
		m_assembler.DivssEd(dstRegister, CX86Assembler::MakeXmmRegisterAddress(src1Register));
	}
	else
	{
		m_assembler.MovdVo(dstRegister, CX86Assembler::MakeRegisterAddress(tmpIntRegister));
		m_assembler.DivssEd(dstRegister, MakeVariableFp32SymbolAddress(src1));
	}
}

void CCodeGen_x86::Emit_Fp_RcplS_MemVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto tmpIntRegister = CX86Assembler::rAX;
	auto dstRegister = CX86Assembler::xMM0;

	m_assembler.MovId(tmpIntRegister, 0x3F800000);
	m_assembler.MovdVo(dstRegister, CX86Assembler::MakeRegisterAddress(tmpIntRegister));
	m_assembler.DivssEd(dstRegister, MakeMemoryFp32SymbolAddress(src1));
	m_assembler.MovssEd(MakeMemoryFp32SymbolAddress(dst), dstRegister);
}

void CCodeGen_x86::Emit_Fp_ClampS_RegVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstRegister = m_mdRegisters[dst->m_valueLow];

	if(!dst->Equals(src1))
	{
		m_assembler.MovssEd(dstRegister, MakeVariableFp32SymbolAddress(src1));
	}

	m_assembler.PminsdVo(dstRegister, MakeConstant128Address(g_fpClampMask1));
	m_assembler.PminudVo(dstRegister, MakeConstant128Address(g_fpClampMask2));
}

void CCodeGen_x86::Emit_Fp_ClampS_MemVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstRegister = CX86Assembler::xMM0;

	m_assembler.MovssEd(dstRegister, MakeVariableFp32SymbolAddress(src1));
	m_assembler.PminsdVo(dstRegister, MakeConstant128Address(g_fpClampMask1));
	m_assembler.PminudVo(dstRegister, MakeConstant128Address(g_fpClampMask2));
	m_assembler.MovssEd(MakeMemoryFp32SymbolAddress(dst), dstRegister);
}

void CCodeGen_x86::Emit_Fp_ToSingleI32_VarReg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(src1->m_type == SYM_FP_REGISTER32);

	auto dstRegister = PrepareSymbolRegisterDefFp32(dst, CX86Assembler::xMM0);
	auto tmpIntRegister = CX86Assembler::rAX;

	m_assembler.MovdVo(CX86Assembler::MakeRegisterAddress(tmpIntRegister), m_mdRegisters[src1->m_valueLow]);
	m_assembler.Cvtsi2ssEd(dstRegister, CX86Assembler::MakeRegisterAddress(tmpIntRegister));

	CommitSymbolRegisterFp32Sse(dst, dstRegister);
}

void CCodeGen_x86::Emit_Fp_ToSingleI32_VarMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstRegister = PrepareSymbolRegisterDefFp32(dst, CX86Assembler::xMM0);

	m_assembler.Cvtsi2ssEd(dstRegister, MakeVariableFp32SymbolAddress(src1));

	CommitSymbolRegisterFp32Sse(dst, dstRegister);
}

void CCodeGen_x86::Emit_Fp_ToInt32TruncS_RegVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type == SYM_FP_REGISTER32);

	auto tmpIntRegister = CX86Assembler::rAX;

	m_assembler.Cvttss2siEd(tmpIntRegister, MakeVariableFp32SymbolAddress(src1));
	m_assembler.MovdVo(m_mdRegisters[dst->m_valueLow], CX86Assembler::MakeRegisterAddress(tmpIntRegister));
}

void CCodeGen_x86::Emit_Fp_ToInt32TruncS_MemVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto tmpIntRegister = CX86Assembler::rAX;

	m_assembler.Cvttss2siEd(tmpIntRegister, MakeVariableFp32SymbolAddress(src1));
	m_assembler.MovGd(MakeMemoryFp32SymbolAddress(dst), tmpIntRegister);
}

#define FP_CONST_MATCHERS_3OPS(FPOP_CST, FPOP) \
	{ FPOP_CST, MATCH_FP_REGISTER32, MATCH_FP_REGISTER32, MATCH_FP_REGISTER32, MATCH_NIL, &CCodeGen_x86::Emit_Fp32_RegRegReg<FPOP> }, \
	{ FPOP_CST, MATCH_FP_REGISTER32, MATCH_FP_MEMORY32,   MATCH_FP_REGISTER32, MATCH_NIL, &CCodeGen_x86::Emit_Fp32_RegMemReg<FPOP> }, \
	{ FPOP_CST, MATCH_FP_REGISTER32, MATCH_FP_VARIABLE32, MATCH_FP_VARIABLE32, MATCH_NIL, &CCodeGen_x86::Emit_Fp32_RegVarVar<FPOP> }, \
	{ FPOP_CST, MATCH_FP_MEMORY32,   MATCH_FP_VARIABLE32, MATCH_FP_VARIABLE32, MATCH_NIL, &CCodeGen_x86::Emit_Fp32_MemVarVar<FPOP> },

#define FP_CONST_MATCHERS_SINGLEOP(FPOP_CST, FPOP) \
	{ FPOP_CST, MATCH_FP_REGISTER32, MATCH_FP_VARIABLE32, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp32_SingleOp_RegVar<FPOP> }, \
	{ FPOP_CST, MATCH_FP_MEMORY32,   MATCH_FP_VARIABLE32, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp32_SingleOp_MemVar<FPOP> },

CCodeGen_x86::CONSTMATCHER CCodeGen_x86::g_fpuSseConstMatchers[] = 
{
	FP_CONST_MATCHERS_3OPS(OP_FP_ADD_S, FP32OP_ADD)
	FP_CONST_MATCHERS_3OPS(OP_FP_SUB_S, FP32OP_SUB)
	FP_CONST_MATCHERS_3OPS(OP_FP_MUL_S, FP32OP_MUL)
	FP_CONST_MATCHERS_3OPS(OP_FP_DIV_S, FP32OP_DIV)
	FP_CONST_MATCHERS_3OPS(OP_FP_MAX_S, FP32OP_MAX)
	FP_CONST_MATCHERS_3OPS(OP_FP_MIN_S, FP32OP_MIN)

	{ OP_FP_CMP_S, MATCH_VARIABLE, MATCH_FP_VARIABLE32, MATCH_FP_VARIABLE32, MATCH_NIL, &CCodeGen_x86::Emit_Fp_CmpS_VarVarVar },
	{ OP_FP_CMP_S, MATCH_VARIABLE, MATCH_FP_MEMORY32,   MATCH_CONSTANT,      MATCH_NIL, &CCodeGen_x86::Emit_Fp_CmpS_VarMemCst },

	{ OP_FP_SQRT_S,  MATCH_FP_REGISTER32, MATCH_FP_VARIABLE32, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp32_RegVar<FP32OP_SQRT> },
	{ OP_FP_SQRT_S,  MATCH_FP_MEMORY32,   MATCH_FP_VARIABLE32, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp32_MemVar<FP32OP_SQRT> },
	{ OP_FP_RSQRT_S, MATCH_FP_VARIABLE32, MATCH_FP_VARIABLE32, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_RsqrtS_VarVar         },
	{ OP_FP_RCPL_S,  MATCH_FP_REGISTER32, MATCH_FP_VARIABLE32, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_RcplS_RegVar          },
	{ OP_FP_RCPL_S,  MATCH_FP_MEMORY32,   MATCH_FP_VARIABLE32, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_RcplS_MemVar          },

	FP_CONST_MATCHERS_SINGLEOP(OP_FP_ABS_S, MDOP_ABS)
	FP_CONST_MATCHERS_SINGLEOP(OP_FP_NEG_S, MDOP_NEG)

	{ OP_FP_CLAMP_S, MATCH_FP_REGISTER32, MATCH_FP_VARIABLE32, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_ClampS_RegVar },
	{ OP_FP_CLAMP_S, MATCH_FP_MEMORY32,   MATCH_FP_VARIABLE32, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_ClampS_MemVar },

	{ OP_FP_TOSINGLE_I32,    MATCH_FP_VARIABLE32, MATCH_FP_REGISTER32, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_ToSingleI32_VarReg   },
	{ OP_FP_TOSINGLE_I32,    MATCH_FP_VARIABLE32, MATCH_FP_MEMORY32,   MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_ToSingleI32_VarMem   },
	{ OP_FP_TOINT32_TRUNC_S, MATCH_FP_REGISTER32, MATCH_FP_VARIABLE32, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_ToInt32TruncS_RegVar },
	{ OP_FP_TOINT32_TRUNC_S, MATCH_FP_MEMORY32,   MATCH_FP_VARIABLE32, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_ToInt32TruncS_MemVar },

	{ OP_MOV,      MATCH_FP_REGISTER32, MATCH_FP_MEMORY32,   MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp32_Mov_RegMem   },
	{ OP_MOV,      MATCH_FP_MEMORY32,   MATCH_FP_REGISTER32, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp32_Mov_MemReg   },
	{ OP_FP_LDCST, MATCH_FP_REGISTER32, MATCH_CONSTANT,      MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp32_LdCst_RegCst },

	{ OP_MOV, MATCH_NIL, MATCH_NIL, MATCH_NIL, MATCH_NIL, nullptr },
};
