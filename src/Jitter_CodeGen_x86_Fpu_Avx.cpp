#include "Jitter_CodeGen_x86.h"

using namespace Jitter;

template <typename FPUOP>
void CCodeGen_x86::Emit_Fpu_Avx_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstRegister = PrepareSymbolRegisterDefFpu(dst, CX86Assembler::xMM0);
	auto src1Register = PrepareSymbolRegisterUseFpuAvx(src1, CX86Assembler::xMM1);

	((m_assembler).*(FPUOP::OpEdAvx()))(dstRegister, dstRegister, CX86Assembler::MakeXmmRegisterAddress(src1Register));

	CommitSymbolRegisterFpuAvx(dst, dstRegister);

}

void CCodeGen_x86::Emit_Fp_Avx_Neg_VarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto tmpIntRegister = CX86Assembler::rAX;
	auto tmpXMMRegister = CX86Assembler::xMM2;
	auto tmpXMM2Register = CX86Assembler::xMM3;
	auto dstRegister = PrepareSymbolRegisterDefFpu(dst, CX86Assembler::xMM0);
	auto src1Register = PrepareSymbolRegisterUseFpuAvx(src1, CX86Assembler::xMM1);

	m_assembler.MovId(tmpIntRegister, 0x80000000);
	m_assembler.VmovdVo(tmpXMMRegister, CX86Assembler::MakeRegisterAddress(tmpIntRegister));
	m_assembler.VxorpsVo(dstRegister, src1Register, CX86Assembler::MakeXmmRegisterAddress(tmpXMMRegister));
	CommitSymbolRegisterFpuAvx(dst, dstRegister);
}

void CCodeGen_x86::Emit_Fp_Avx_Abs_VarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstRegister = PrepareSymbolRegisterDefFpu(dst, CX86Assembler::xMM0);
	auto src1Register = PrepareSymbolRegisterUseFpuAvx(src1, CX86Assembler::xMM1);
	auto tmpXMMRegister = CX86Assembler::xMM2;
	auto tmpXMM2Register = CX86Assembler::xMM3;

	m_assembler.VxorpsVo(tmpXMMRegister, tmpXMMRegister, CX86Assembler::MakeXmmRegisterAddress(tmpXMMRegister));
	m_assembler.VsubpsVo(tmpXMM2Register, tmpXMMRegister, CX86Assembler::MakeXmmRegisterAddress(src1Register));
	m_assembler.VmaxpsVo(dstRegister, tmpXMM2Register, CX86Assembler::MakeXmmRegisterAddress(src1Register));
	CommitSymbolRegisterFpuAvx(dst, dstRegister);
}

template <typename FPUOP>
void CCodeGen_x86::Emit_Fpu_Avx_VarVarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstRegister = PrepareSymbolRegisterDefFpu(dst, CX86Assembler::xMM0);
	auto src1Register = PrepareSymbolRegisterUseFpuAvx(src1, CX86Assembler::xMM1);
	auto src2Register = PrepareSymbolRegisterUseFpuAvx(src2, CX86Assembler::xMM2);

	((m_assembler).*(FPUOP::OpEdAvx()))(dstRegister, src1Register, CX86Assembler::MakeXmmRegisterAddress(src2Register));
	CommitSymbolRegisterFpuAvx(dst, dstRegister);
}

void CCodeGen_x86::Emit_Fp_Avx_Cmp_VarMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDef(dst, CX86Assembler::rAX);
	auto src1Register = PrepareSymbolRegisterUseFpuAvx(src1, CX86Assembler::xMM0);
	auto src2Register = PrepareSymbolRegisterUseFpuAvx(src2, CX86Assembler::xMM1);
	auto cmpReg = CX86Assembler::xMM2;
	auto resReg = CX86Assembler::xMM3;

	auto conditionCode = GetSseConditionCode(statement.jmpCondition);
	m_assembler.VmovssEd(cmpReg, CX86Assembler::MakeXmmRegisterAddress(src1Register));
	m_assembler.VcmpssEd(resReg, cmpReg, CX86Assembler::MakeXmmRegisterAddress(src2Register), conditionCode);
	m_assembler.VmovdVo(CX86Assembler::MakeRegisterAddress(dstReg), resReg);

	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_x86::Emit_Fp_Avx_Rsqrt_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	{StatementList list = {statement};
	DumpStatementList(list);}

	auto tmpIntRegister = CX86Assembler::rAX;
	auto resultRegister = PrepareSymbolRegisterDefFpu(dst, CX86Assembler::xMM0);
	auto sqrtRegister = PrepareSymbolRegisterUseFpuAvx(src1, CX86Assembler::xMM1);

	m_assembler.VsqrtssEd(sqrtRegister, CX86Assembler::xMM0, CX86Assembler::MakeXmmRegisterAddress(sqrtRegister));
	m_assembler.MovId(tmpIntRegister, 0x3F800000);
	m_assembler.VmovdVo(resultRegister, CX86Assembler::MakeRegisterAddress(tmpIntRegister));
	m_assembler.VdivssEd(resultRegister, resultRegister, CX86Assembler::MakeXmmRegisterAddress(sqrtRegister));
	CommitSymbolRegisterFpuAvx(dst, resultRegister);
}

void CCodeGen_x86::Emit_Fp_Avx_Rcpl_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto tmpIntRegister = CX86Assembler::rAX;
	auto resultRegister = PrepareSymbolRegisterDefFpu(dst, CX86Assembler::xMM0);
	auto src1Register = PrepareSymbolRegisterUseFpuAvx(src1, CX86Assembler::xMM1);

	m_assembler.MovId(tmpIntRegister, 0x3F800000);
	m_assembler.VmovdVo(resultRegister, CX86Assembler::MakeRegisterAddress(tmpIntRegister));
	m_assembler.VdivssEd(resultRegister, resultRegister, CX86Assembler::MakeXmmRegisterAddress(src1Register));

	CommitSymbolRegisterFpuAvx(dst, resultRegister);

}

void CCodeGen_x86::Emit_Fp_Avx_Mov_RegSRelI32(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type  == SYM_REGISTER128);
	assert(src1->m_type == SYM_FP_REL_INT32);
	auto dstRegister = PrepareSymbolRegisterDefMd(dst, CX86Assembler::xMM0);
	m_assembler.Vcvtsi2ssEd(dstRegister, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow));
}

void CCodeGen_x86::Emit_Fp_Avx_Mov_Reg32RelI32(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type  == SYM_REGISTER);
	assert(src1->m_type == SYM_FP_REL_INT32);
	auto dstRegister = CX86Assembler::xMM3;
	auto dstReg = PrepareSymbolRegisterDef(dst, CX86Assembler::rAX);

	m_assembler.Vcvtsi2ssEd(dstRegister, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow));
	m_assembler.VmovdVo(CX86Assembler::MakeRegisterAddress(dstReg), dstRegister);
}

void CCodeGen_x86::Emit_Fp_Avx_Mov_RelSReg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type  == SYM_FP_REL_SINGLE);
	assert(src1->m_type == SYM_REGISTER128);
	auto src1Register = PrepareSymbolRegisterDefMd(src1, CX86Assembler::xMM1);
	m_assembler.VmovssEd(MakeMemoryFpSingleSymbolAddress(dst), src1Register);
}


void CCodeGen_x86::Emit_Fp_Avx_Mov_RegSRelS(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type  == SYM_REGISTER128);
	assert(src1->m_type == SYM_FP_REL_SINGLE);
	auto dstRegister = PrepareSymbolRegisterDefMd(dst, CX86Assembler::xMM1);
	m_assembler.VmovssEd(dstRegister, MakeMemoryFpSingleSymbolAddress(src1));
}

void CCodeGen_x86::Emit_Fp_Avx_Mov_Reg128Rel(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type  == SYM_REGISTER128);
	assert(src1->m_type == SYM_RELATIVE);
	auto dstRegister = PrepareSymbolRegisterDefFpu(dst, CX86Assembler::xMM1);
	auto src1Reg = PrepareSymbolRegisterUse(src1, CX86Assembler::rAX);

	m_assembler.VmovdVo(dstRegister, CX86Assembler::MakeRegisterAddress(src1Reg));
	m_assembler.VshufpsVo(dstRegister, dstRegister, CX86Assembler::MakeXmmRegisterAddress(dstRegister), 0x00);
}


void CCodeGen_x86::Emit_Fp_Avx_Mov_RelSRelI32(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type  == SYM_FP_REL_SINGLE);
	assert(src1->m_type == SYM_FP_REL_INT32);

	m_assembler.Vcvtsi2ssEd(CX86Assembler::xMM0, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow));
	m_assembler.VmovssEd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, dst->m_valueLow), CX86Assembler::xMM0);
}

void CCodeGen_x86::Emit_Fp_Avx_ToIntTrunc_RelRel(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto src1Reg = PrepareSymbolRegisterUseFpuAvx(src1, CX86Assembler::xMM1);

	m_assembler.Vcvttss2siEd(CX86Assembler::rAX, CX86Assembler::MakeXmmRegisterAddress(src1Reg));
	if(
		dst->m_type == SYM_RELATIVE128 || dst->m_type == SYM_TEMPORARY128
		|| dst->m_type == SYM_FP_REL_SINGLE || dst->m_type == SYM_FP_TMP_SINGLE
		|| dst->m_type == SYM_REGISTER128
	)
	{
		auto dstReg = PrepareSymbolRegisterDefFpu(dst, CX86Assembler::xMM0);
		m_assembler.VmovdVo(dstReg, CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX));
		CommitSymbolRegisterFpuAvx(dst, dstReg);
	}
	else if(dst->m_type == SYM_RELATIVE || dst->m_type == SYM_TEMPORARY)
	{
		m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, dst->m_valueLow), CX86Assembler::rAX);
	}
	else
	{
		throw std::exception();
	}
}

CCodeGen_x86::CONSTMATCHER CCodeGen_x86::g_fpuAvxConstMatchers[] = 
{
	{ OP_FP_ADD, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_Avx_VarVarVar<FPUOP_ADD> },
	{ OP_FP_SUB, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_Avx_VarVarVar<FPUOP_SUB> },
	{ OP_FP_MUL, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_Avx_VarVarVar<FPUOP_MUL> },
	{ OP_FP_DIV, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_Avx_VarVarVar<FPUOP_DIV> },
	{ OP_FP_MAX, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_Avx_VarVarVar<FPUOP_MAX> },
	{ OP_FP_MIN, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_Avx_VarVarVar<FPUOP_MIN> },

	{ OP_FP_ADD, MATCH_MEMORY_FP_SINGLE, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_Avx_VarVarVar<FPUOP_ADD> },
	{ OP_FP_SUB, MATCH_MEMORY_FP_SINGLE, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_Avx_VarVarVar<FPUOP_SUB> },
	{ OP_FP_MUL, MATCH_MEMORY_FP_SINGLE, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_Avx_VarVarVar<FPUOP_MUL> },
	{ OP_FP_DIV, MATCH_MEMORY_FP_SINGLE, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_Avx_VarVarVar<FPUOP_DIV> },
	{ OP_FP_MAX, MATCH_MEMORY_FP_SINGLE, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_Avx_VarVarVar<FPUOP_MAX> },
	{ OP_FP_MIN, MATCH_MEMORY_FP_SINGLE, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_Avx_VarVarVar<FPUOP_MIN> },


	{ OP_FP_ADD, MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_Avx_VarVarVar<FPUOP_ADD> },
	{ OP_FP_SUB, MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_Avx_VarVarVar<FPUOP_SUB> },
	{ OP_FP_MUL, MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_Avx_VarVarVar<FPUOP_MUL> },
	{ OP_FP_DIV, MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_Avx_VarVarVar<FPUOP_DIV> },
	{ OP_FP_MAX, MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_Avx_VarVarVar<FPUOP_MAX> },
	{ OP_FP_MIN, MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_Avx_VarVarVar<FPUOP_MIN> },

	{ OP_FP_ADD, MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_Avx_VarVarVar<FPUOP_ADD> },
	{ OP_FP_SUB, MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_Avx_VarVarVar<FPUOP_SUB> },
	{ OP_FP_MUL, MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_Avx_VarVarVar<FPUOP_MUL> },
	{ OP_FP_DIV, MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_Avx_VarVarVar<FPUOP_DIV> },
	{ OP_FP_MAX, MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_Avx_VarVarVar<FPUOP_MAX> },
	{ OP_FP_MIN, MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_Avx_VarVarVar<FPUOP_MIN> },


	{ OP_FP_ADD, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_MEMORY_FP_SINGLE, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_Avx_VarVarVar<FPUOP_ADD> },
	{ OP_FP_SUB, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_MEMORY_FP_SINGLE, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_Avx_VarVarVar<FPUOP_SUB> },
	{ OP_FP_MUL, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_MEMORY_FP_SINGLE, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_Avx_VarVarVar<FPUOP_MUL> },
	{ OP_FP_DIV, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_MEMORY_FP_SINGLE, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_Avx_VarVarVar<FPUOP_DIV> },
	{ OP_FP_MAX, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_MEMORY_FP_SINGLE, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_Avx_VarVarVar<FPUOP_MAX> },
	{ OP_FP_MIN, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_MEMORY_FP_SINGLE, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_Avx_VarVarVar<FPUOP_MIN> },


	{ OP_FP_ADD, MATCH_VARIABLE128, MATCH_MEMORY_FP_SINGLE, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_Avx_VarVarVar<FPUOP_ADD> },
	{ OP_FP_SUB, MATCH_VARIABLE128, MATCH_MEMORY_FP_SINGLE, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_Avx_VarVarVar<FPUOP_SUB> },
	{ OP_FP_MUL, MATCH_VARIABLE128, MATCH_MEMORY_FP_SINGLE, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_Avx_VarVarVar<FPUOP_MUL> },
	{ OP_FP_DIV, MATCH_VARIABLE128, MATCH_MEMORY_FP_SINGLE, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_Avx_VarVarVar<FPUOP_DIV> },
	{ OP_FP_MAX, MATCH_VARIABLE128, MATCH_MEMORY_FP_SINGLE, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_Avx_VarVarVar<FPUOP_MAX> },
	{ OP_FP_MIN, MATCH_VARIABLE128, MATCH_MEMORY_FP_SINGLE, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_Avx_VarVarVar<FPUOP_MIN> },

	{ OP_FP_ADD, MATCH_MEMORY_FP_SINGLE, MATCH_VARIABLE128, MATCH_MEMORY_FP_SINGLE, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_Avx_VarVarVar<FPUOP_ADD> },
	{ OP_FP_SUB, MATCH_MEMORY_FP_SINGLE, MATCH_VARIABLE128, MATCH_MEMORY_FP_SINGLE, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_Avx_VarVarVar<FPUOP_SUB> },
	{ OP_FP_MUL, MATCH_MEMORY_FP_SINGLE, MATCH_VARIABLE128, MATCH_MEMORY_FP_SINGLE, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_Avx_VarVarVar<FPUOP_MUL> },
	{ OP_FP_DIV, MATCH_MEMORY_FP_SINGLE, MATCH_VARIABLE128, MATCH_MEMORY_FP_SINGLE, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_Avx_VarVarVar<FPUOP_DIV> },
	{ OP_FP_MAX, MATCH_MEMORY_FP_SINGLE, MATCH_VARIABLE128, MATCH_MEMORY_FP_SINGLE, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_Avx_VarVarVar<FPUOP_MAX> },
	{ OP_FP_MIN, MATCH_MEMORY_FP_SINGLE, MATCH_VARIABLE128, MATCH_MEMORY_FP_SINGLE, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_Avx_VarVarVar<FPUOP_MIN> },

	{ OP_FP_ADD, MATCH_VARIABLE128, MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_Avx_VarVarVar<FPUOP_ADD> },
	{ OP_FP_SUB, MATCH_VARIABLE128, MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_Avx_VarVarVar<FPUOP_SUB> },
	{ OP_FP_MUL, MATCH_VARIABLE128, MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_Avx_VarVarVar<FPUOP_MUL> },
	{ OP_FP_DIV, MATCH_VARIABLE128, MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_Avx_VarVarVar<FPUOP_DIV> },
	{ OP_FP_MAX, MATCH_VARIABLE128, MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_Avx_VarVarVar<FPUOP_MAX> },
	{ OP_FP_MIN, MATCH_VARIABLE128, MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_Avx_VarVarVar<FPUOP_MIN> },

	{ OP_FP_ABS, MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_Avx_Abs_VarVar },
	{ OP_FP_ABS, MATCH_VARIABLE128, MATCH_MEMORY_FP_SINGLE, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_Avx_Abs_VarVar },
	{ OP_FP_ABS, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_Avx_Abs_VarVar },
	{ OP_FP_ABS, MATCH_MEMORY_FP_SINGLE, MATCH_VARIABLE128, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_Avx_Abs_VarVar },

	{ OP_FP_NEG, MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_Avx_Neg_VarVar },
	{ OP_FP_NEG, MATCH_MEMORY_FP_SINGLE, MATCH_VARIABLE128, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_Avx_Neg_VarVar },
	{ OP_FP_NEG, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_Avx_Neg_VarVar },
	{ OP_FP_NEG, MATCH_VARIABLE128, MATCH_MEMORY_FP_SINGLE, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_Avx_Neg_VarVar },

	{ OP_FP_CMP, MATCH_VARIABLE, MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, MATCH_NIL, &CCodeGen_x86::Emit_Fp_Avx_Cmp_VarMemMem },
	{ OP_FP_CMP, MATCH_VARIABLE, MATCH_MEMORY_FP_SINGLE, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Fp_Avx_Cmp_VarMemMem },
	{ OP_FP_CMP, MATCH_VARIABLE, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Fp_Avx_Cmp_VarMemMem },
	{ OP_FP_CMP, MATCH_VARIABLE, MATCH_VARIABLE128, MATCH_MEMORY_FP_SINGLE, MATCH_NIL, &CCodeGen_x86::Emit_Fp_Avx_Cmp_VarMemMem },

	{ OP_FP_RSQRT, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_Avx_Rsqrt_MemMem        },
	{ OP_FP_RSQRT, MATCH_VARIABLE128, MATCH_MEMORY_FP_SINGLE, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_Avx_Rsqrt_MemMem        },
	{ OP_FP_RSQRT, MATCH_MEMORY_FP_SINGLE, MATCH_VARIABLE128, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_Avx_Rsqrt_MemMem        },
	// { OP_FP_SQRT,  MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_Avx_MemMem<FPUOP_SQRT> },
	{ OP_FP_SQRT,  MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_Avx_MemMem<FPUOP_SQRT> },
	{ OP_FP_SQRT,  MATCH_MEMORY_FP_SINGLE, MATCH_VARIABLE128, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_Avx_MemMem<FPUOP_SQRT> },
	{ OP_FP_RCPL,  MATCH_VARIABLE128, MATCH_MEMORY_FP_SINGLE, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_Avx_Rcpl_MemMem         },
	{ OP_FP_RCPL,  MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_Avx_Rcpl_MemMem         },

	{ OP_FP_SQRT,  MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_Avx_MemMem<FPUOP_SQRT> },
	{ OP_FP_RSQRT, MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_Avx_Rsqrt_MemMem        },
	{ OP_FP_RCPL,  MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_Avx_Rcpl_MemMem         },

	{ OP_MOV,            MATCH_VARIABLE128, MATCH_VARIABLE,  MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_Avx_Mov_Reg128Rel    },
	{ OP_MOV,            MATCH_VARIABLE, MATCH_RELATIVE_FP_INT32,  MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_Avx_Mov_Reg32RelI32    },
	{ OP_MOV,            MATCH_REGISTER128, MATCH_RELATIVE_FP_INT32,  MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_Avx_Mov_RegSRelI32    },
	{ OP_MOV,            MATCH_RELATIVE_FP_SINGLE, MATCH_REGISTER128, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_Avx_Mov_RelSReg    },
	{ OP_MOV,            MATCH_REGISTER128, MATCH_RELATIVE_FP_SINGLE, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_Avx_Mov_RegSRelS    },

	{ OP_MOV,            MATCH_RELATIVE_FP_SINGLE, MATCH_RELATIVE_FP_INT32,  MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_Avx_Mov_RelSRelI32    },
	{ OP_FP_TOINT_TRUNC, MATCH_RELATIVE_FP_SINGLE, MATCH_RELATIVE_FP_SINGLE, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_Avx_ToIntTrunc_RelRel },
	{ OP_FP_TOINT_TRUNC, MATCH_RELATIVE_FP_SINGLE, MATCH_VARIABLE128, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_Avx_ToIntTrunc_RelRel },
	{ OP_FP_TOINT_TRUNC, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_Avx_ToIntTrunc_RelRel },
	{ OP_FP_TOINT_TRUNC, MATCH_VARIABLE128, MATCH_RELATIVE_FP_SINGLE, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_Avx_ToIntTrunc_RelRel },

	{ OP_MOV, MATCH_NIL, MATCH_NIL, MATCH_NIL, MATCH_NIL, nullptr },
};
