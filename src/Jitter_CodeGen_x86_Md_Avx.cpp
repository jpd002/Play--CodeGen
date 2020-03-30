#include "Jitter_CodeGen_x86.h"

using namespace Jitter;

template <typename MDOP>
void CCodeGen_x86::Emit_Md_Avx_VarVarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstRegister = PrepareSymbolRegisterDefMd(dst, CX86Assembler::xMM0);
	auto src1Register = PrepareSymbolRegisterUseMdAvx(src1, CX86Assembler::xMM1);

	((m_assembler).*(MDOP::OpVoAvx()))(dstRegister, src1Register, MakeVariable128SymbolAddress(src2));

	CommitSymbolRegisterMdAvx(dst, dstRegister);
}

template <typename MDOP>
void CCodeGen_x86::Emit_Md_Avx_VarVarVarRev(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstRegister = PrepareSymbolRegisterDefMd(dst, CX86Assembler::xMM0);
	auto src2Register = PrepareSymbolRegisterUseMdAvx(src2, CX86Assembler::xMM1);

	((m_assembler).*(MDOP::OpVoAvx()))(dstRegister, src2Register, MakeVariable128SymbolAddress(src1));

	CommitSymbolRegisterMdAvx(dst, dstRegister);
}

void CCodeGen_x86::Emit_Md_Avx_Mov_RegVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	m_assembler.VmovapsVo(m_mdRegisters[dst->m_valueLow], MakeVariable128SymbolAddress(src1));
}

void CCodeGen_x86::Emit_Md_Avx_Mov_MemReg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	m_assembler.VmovapsVo(MakeMemory128SymbolAddress(dst), m_mdRegisters[src1->m_valueLow]);
}

void CCodeGen_x86::Emit_Md_Avx_AddSSW_VarVarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto uxRegister = CX86Assembler::xMM0;
	auto uyRegister = CX86Assembler::xMM1;
	auto resRegister = CX86Assembler::xMM2;
	auto cstRegister = CX86Assembler::xMM3;

//	This is based on code from http://locklessinc.com/articles/sat_arithmetic/ modified to work without cmovns
//	s32b sat_adds32b(s32b x, s32b y)
//	{
//		u32b ux = x;
//		u32b uy = y;
//		u32b res = ux + uy;
//	
//		/* Calculate overflowed result. (Don't change the sign bit of ux) */
//		ux = (ux >> 31) + INT_MAX;
//	
//		s32b sign = (s32b) ((ux ^ uy) | ~(uy ^ res))
//		sign >>= 31;		/* Arithmetic shift, either 0 or ~0*/
//		res = (res & sign) | (ux & ~sign);
//		
//		return res;
//	}

	//ux = src1
	//uy = src2
	m_assembler.VmovdqaVo(uxRegister, MakeVariable128SymbolAddress(src1));
	m_assembler.VmovdqaVo(uyRegister, MakeVariable128SymbolAddress(src2));
	
	//res = ux + uy
	m_assembler.VpadddVo(resRegister, uxRegister, CX86Assembler::MakeXmmRegisterAddress(uyRegister));

	//cst = 0x7FFFFFFF
	m_assembler.VpcmpeqdVo(cstRegister, cstRegister, CX86Assembler::MakeXmmRegisterAddress(cstRegister));
	m_assembler.VpsrldVo(cstRegister, cstRegister, 1);

	//ux = (ux >> 31)
	m_assembler.VpsrldVo(uxRegister, uxRegister, 31);

	//ux += 0x7FFFFFFF
	m_assembler.VpadddVo(uxRegister, uxRegister, CX86Assembler::MakeXmmRegisterAddress(cstRegister));

	//uy = ~(uy ^ res)
	//------
	//uy ^ res
	m_assembler.VpxorVo(uyRegister, uyRegister, CX86Assembler::MakeXmmRegisterAddress(resRegister));

	//~(uy ^ res)
	m_assembler.VpcmpeqdVo(cstRegister, cstRegister, CX86Assembler::MakeXmmRegisterAddress(cstRegister));
	m_assembler.VpxorVo(uyRegister, uyRegister, CX86Assembler::MakeXmmRegisterAddress(cstRegister));

	//cst = ux ^ uy (reloading uy from src2 because we don't have any registers available)
	m_assembler.VpxorVo(cstRegister, uxRegister, MakeVariable128SymbolAddress(src2));

	//uy = ((ux ^ uy) | ~(uy ^ res)) >> 31; (signed operation)
	m_assembler.VporVo(uyRegister, uyRegister, CX86Assembler::MakeXmmRegisterAddress(cstRegister));
	m_assembler.VpsradVo(uyRegister, uyRegister, 31);

	//res = (res & uy)	(uy is the sign value)
	m_assembler.VpandVo(resRegister, resRegister, CX86Assembler::MakeXmmRegisterAddress(uyRegister));

	//ux = (ux & ~uy)
	//------
	//~uy
	m_assembler.VpcmpeqdVo(cstRegister, cstRegister, CX86Assembler::MakeXmmRegisterAddress(cstRegister));
	m_assembler.VpxorVo(uyRegister, uyRegister, CX86Assembler::MakeXmmRegisterAddress(cstRegister));

	//ux & ~uy
	m_assembler.VpandVo(uxRegister, uxRegister, CX86Assembler::MakeXmmRegisterAddress(uyRegister));

	//res = (res & uy) | (ux & ~uy)
	m_assembler.VporVo(resRegister, resRegister, CX86Assembler::MakeXmmRegisterAddress(uxRegister));

	//Copy final result
	m_assembler.VmovdqaVo(MakeVariable128SymbolAddress(dst), resRegister);
}

void CCodeGen_x86::Emit_Md_Avx_SubSSW_VarVarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto uxRegister = CX86Assembler::xMM0;
	auto uyRegister = CX86Assembler::xMM1;
	auto resRegister = CX86Assembler::xMM2;
	auto cstRegister = CX86Assembler::xMM3;

//	This is based on code from http://locklessinc.com/articles/sat_arithmetic/ modified to work without cmovns
//	s32b sat_subs32b(s32b x, s32b y)
//	{
//		u32b ux = x;
//		u32b uy = y;
//		u32b res = ux - uy;
//		
//		ux = (ux >> 31) + INT_MAX;
//		
//		s32b sign = (s32b) ((ux ^ uy) & (ux ^ res))
//		sign >>= 31;		/* Arithmetic shift, either 0 or ~0*/
//		res = (res & ~sign) | (ux & sign);
//		
//		return res;
//	}

	//ux = src1
	//uy = src2
	m_assembler.VmovdqaVo(uxRegister, MakeVariable128SymbolAddress(src1));
	
	//res = ux - uy
	m_assembler.VpsubdVo(resRegister, uxRegister, MakeVariable128SymbolAddress(src2));

	//cst = 0x7FFFFFFF
	m_assembler.VpcmpeqdVo(cstRegister, cstRegister, CX86Assembler::MakeXmmRegisterAddress(cstRegister));
	m_assembler.VpsrldVo(cstRegister, cstRegister, 1);

	//ux = (ux >> 31)
	m_assembler.VpsrldVo(uxRegister, uxRegister, 31);

	//ux += 0x7FFFFFFF
	m_assembler.VpadddVo(uxRegister, uxRegister, CX86Assembler::MakeXmmRegisterAddress(cstRegister));

	//uy = (ux ^ res)
	//------
	//ux ^ res
	m_assembler.VpxorVo(uyRegister, uxRegister, CX86Assembler::MakeXmmRegisterAddress(resRegister));

	//cst = ux ^ uy (reloading uy from src2 because we don't have any registers available)
	m_assembler.VpxorVo(cstRegister, uxRegister, MakeVariable128SymbolAddress(src2));

	//uy = ((ux ^ uy) & (ux ^ res)) >> 31; (signed operation)
	m_assembler.VpandVo(uyRegister, uyRegister, CX86Assembler::MakeXmmRegisterAddress(cstRegister));
	m_assembler.VpsradVo(uyRegister, uyRegister, 31);

	//ux = (ux & uy)	(uy is the sign value)
	m_assembler.VpandVo(uxRegister, uxRegister, CX86Assembler::MakeXmmRegisterAddress(uyRegister));

	//res = (res & ~uy)
	//------
	//~uy
	m_assembler.VpcmpeqdVo(cstRegister, cstRegister, CX86Assembler::MakeXmmRegisterAddress(cstRegister));
	m_assembler.VpxorVo(uyRegister, uyRegister, CX86Assembler::MakeXmmRegisterAddress(cstRegister));

	//res & ~uy
	m_assembler.VpandVo(resRegister, resRegister, CX86Assembler::MakeXmmRegisterAddress(uyRegister));

	//res = (res & ~uy) | (ux & uy)
	m_assembler.VporVo(resRegister, resRegister, CX86Assembler::MakeXmmRegisterAddress(uxRegister));

	//Copy final result
	m_assembler.VmovdqaVo(MakeVariable128SymbolAddress(dst), resRegister);
}

void CCodeGen_x86::Emit_Md_Avx_AddUSW_VarVarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto xRegister = CX86Assembler::xMM0;
	auto resRegister = CX86Assembler::xMM1;
	auto tmpRegister = CX86Assembler::xMM2;
	auto tmp2Register = CX86Assembler::xMM3;

//	This is based on code from http://locklessinc.com/articles/sat_arithmetic/
//	u32b sat_addu32b(u32b x, u32b y)
//	{
//		u32b res = x + y;
//		res |= -(res < x);
//	
//		return res;
//	}

	m_assembler.VmovdqaVo(xRegister, MakeVariable128SymbolAddress(src1));
	m_assembler.VpadddVo(resRegister, xRegister, MakeVariable128SymbolAddress(src2));
	
	//-(res < x)
	//PCMPGT will compare two signed integers, but we want unsigned comparison
	//Thus, we add 0x80000000 to both values to "convert" them to signed
	m_assembler.VpcmpeqdVo(tmpRegister, tmpRegister, CX86Assembler::MakeXmmRegisterAddress(tmpRegister));
	m_assembler.VpslldVo(tmpRegister, tmpRegister, 31);
	m_assembler.VpadddVo(tmpRegister, tmpRegister, CX86Assembler::MakeXmmRegisterAddress(resRegister));

	m_assembler.VpcmpeqdVo(tmp2Register, tmp2Register, CX86Assembler::MakeXmmRegisterAddress(tmp2Register));
	m_assembler.VpslldVo(tmp2Register, tmp2Register, 31);
	m_assembler.VpadddVo(tmp2Register, tmp2Register, CX86Assembler::MakeXmmRegisterAddress(xRegister));

	m_assembler.VpcmpgtdVo(tmp2Register, tmp2Register, CX86Assembler::MakeXmmRegisterAddress(tmpRegister));

	//res |= -(res < x)
	m_assembler.VporVo(resRegister, resRegister, CX86Assembler::MakeXmmRegisterAddress(tmp2Register));

	//Store result
	m_assembler.VmovdqaVo(MakeVariable128SymbolAddress(dst), resRegister);
}

void CCodeGen_x86::Emit_Md_Avx_SubUSW_VarVarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto xRegister = CX86Assembler::xMM0;
	auto resRegister = CX86Assembler::xMM1;
	auto tmpRegister = CX86Assembler::xMM2;
	auto tmp2Register = CX86Assembler::xMM3;

//	This is based on code from http://locklessinc.com/articles/sat_arithmetic/
//	u32b sat_subu32b(u32b x, u32b y)
//	{
//		u32b res = x - y;
//		res &= -(res <= x);
//	
//		return res;
//	}

	m_assembler.VmovdqaVo(xRegister, MakeVariable128SymbolAddress(src1));
	m_assembler.VpsubdVo(resRegister, xRegister, MakeVariable128SymbolAddress(src2));

	//-(res <= x)
	//PCMPGT will compare two signed integers, but we want unsigned comparison
	//Thus, we add 0x80000000 to both values to "convert" them to signed
	m_assembler.VpcmpeqdVo(tmpRegister, tmpRegister, CX86Assembler::MakeXmmRegisterAddress(tmpRegister));
	m_assembler.VpslldVo(tmpRegister, tmpRegister, 31);
	m_assembler.VpadddVo(tmpRegister, tmpRegister, CX86Assembler::MakeXmmRegisterAddress(resRegister));

	m_assembler.VpcmpeqdVo(tmp2Register, tmp2Register, CX86Assembler::MakeXmmRegisterAddress(tmp2Register));
	m_assembler.VpslldVo(tmp2Register, tmp2Register, 31);
	m_assembler.VpadddVo(tmp2Register, tmp2Register, CX86Assembler::MakeXmmRegisterAddress(xRegister));

	m_assembler.VpcmpeqdVo(xRegister, tmp2Register, CX86Assembler::MakeXmmRegisterAddress(tmpRegister));
	m_assembler.VpcmpgtdVo(tmp2Register, tmp2Register, CX86Assembler::MakeXmmRegisterAddress(tmpRegister));
	m_assembler.VporVo(tmp2Register, tmp2Register, CX86Assembler::MakeXmmRegisterAddress(xRegister));

	//res &= -(res <= x);
	m_assembler.VpandVo(resRegister, resRegister, CX86Assembler::MakeXmmRegisterAddress(tmp2Register));

	//Store result
	m_assembler.VmovdqaVo(MakeVariable128SymbolAddress(dst), resRegister);
}

void CCodeGen_x86::Emit_Md_Avx_Not_VarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstRegister = PrepareSymbolRegisterDefMd(dst, CX86Assembler::xMM0);
	auto cstRegister = CX86Assembler::xMM1;

	assert(dstRegister != cstRegister);

	m_assembler.VpcmpeqdVo(cstRegister, cstRegister, CX86Assembler::MakeXmmRegisterAddress(cstRegister));
	m_assembler.VpxorVo(dstRegister, cstRegister, MakeVariable128SymbolAddress(src1));

	CommitSymbolRegisterMdAvx(dst, dstRegister);
}

CCodeGen_x86::CONSTMATCHER CCodeGen_x86::g_mdAvxConstMatchers[] = 
{
	{ OP_MD_ADD_B, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_ADDB> },
	{ OP_MD_ADD_H, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_ADDH> },
	{ OP_MD_ADD_W, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_ADDW> },

	{ OP_MD_ADDSS_B, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_ADDSSB> },
	{ OP_MD_ADDSS_H, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_ADDSSH> },
	{ OP_MD_ADDSS_W, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, &CCodeGen_x86::Emit_Md_Avx_AddSSW_VarVarVar       },

	{ OP_MD_ADDUS_B, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_ADDUSB> },
	{ OP_MD_ADDUS_H, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_ADDUSH> },
	{ OP_MD_ADDUS_W, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, &CCodeGen_x86::Emit_Md_Avx_AddUSW_VarVarVar       },

	{ OP_MD_SUB_B, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_SUBB> },
	{ OP_MD_SUB_H, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_SUBH> },
	{ OP_MD_SUB_W, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_SUBW> },

	{ OP_MD_SUBSS_H, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_SUBSSH> },
	{ OP_MD_SUBSS_W, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, &CCodeGen_x86::Emit_Md_Avx_SubSSW_VarVarVar       },

	{ OP_MD_SUBUS_B, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_SUBUSB> },
	{ OP_MD_SUBUS_H, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_SUBUSH> },
	{ OP_MD_SUBUS_W, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, &CCodeGen_x86::Emit_Md_Avx_SubUSW_VarVarVar       },

	{ OP_MD_CMPEQ_B, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_CMPEQB> },
	{ OP_MD_CMPEQ_H, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_CMPEQH> },
	{ OP_MD_CMPEQ_W, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_CMPEQW> },

	{ OP_MD_CMPGT_B, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_CMPGTB> },
	{ OP_MD_CMPGT_H, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_CMPGTH> },
	{ OP_MD_CMPGT_W, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_CMPGTW> },

	{ OP_MD_AND, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_AND> },
	{ OP_MD_OR,  MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_OR>  },
	{ OP_MD_XOR, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_XOR> },

	{ OP_MD_NOT, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_Not_VarVar },

	{ OP_MD_UNPACK_LOWER_BH, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, &CCodeGen_x86::Emit_Md_Avx_VarVarVarRev<MDOP_UNPACK_LOWER_BH> },
	{ OP_MD_UNPACK_LOWER_HW, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, &CCodeGen_x86::Emit_Md_Avx_VarVarVarRev<MDOP_UNPACK_LOWER_HW> },
	{ OP_MD_UNPACK_LOWER_WD, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, &CCodeGen_x86::Emit_Md_Avx_VarVarVarRev<MDOP_UNPACK_LOWER_WD> },

	{ OP_MD_UNPACK_UPPER_BH, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, &CCodeGen_x86::Emit_Md_Avx_VarVarVarRev<MDOP_UNPACK_UPPER_BH> },
	{ OP_MD_UNPACK_UPPER_HW, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, &CCodeGen_x86::Emit_Md_Avx_VarVarVarRev<MDOP_UNPACK_UPPER_HW> },
	{ OP_MD_UNPACK_UPPER_WD, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, &CCodeGen_x86::Emit_Md_Avx_VarVarVarRev<MDOP_UNPACK_UPPER_WD> },

	{ OP_MOV, MATCH_REGISTER128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_Mov_RegVar, },
	{ OP_MOV, MATCH_MEMORY128,   MATCH_REGISTER128, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_Mov_MemReg, },

	{ OP_MOV, MATCH_NIL,         MATCH_NIL,         MATCH_NIL, nullptr },
};
