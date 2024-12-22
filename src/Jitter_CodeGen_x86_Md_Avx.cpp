#include "Jitter_CodeGen_x86.h"
#include <stdexcept>

using namespace Jitter;

CX86Assembler::XMMREGISTER CCodeGen_x86::PrepareSymbolRegisterUseMdAvx(CSymbol* symbol, CX86Assembler::XMMREGISTER preferedRegister)
{
	switch(symbol->m_type)
	{
	case SYM_REGISTER128:
		return m_mdRegisters[symbol->m_valueLow];
		break;
	case SYM_TEMPORARY128:
	case SYM_RELATIVE128:
		m_assembler.VmovapsVo(preferedRegister, MakeMemory128SymbolAddress(symbol));
		return preferedRegister;
		break;
	default:
		throw std::runtime_error("Invalid symbol type.");
		break;
	}
}

void CCodeGen_x86::CommitSymbolRegisterMdAvx(CSymbol* symbol, CX86Assembler::XMMREGISTER usedRegister)
{
	switch(symbol->m_type)
	{
	case SYM_REGISTER128:
		assert(usedRegister == m_mdRegisters[symbol->m_valueLow]);
		break;
	case SYM_TEMPORARY128:
	case SYM_RELATIVE128:
		m_assembler.VmovapsVo(MakeMemory128SymbolAddress(symbol), usedRegister);
		break;
	default:
		throw std::runtime_error("Invalid symbol type.");
		break;
	}
}

template <typename MDOP>
void CCodeGen_x86::Emit_Md_Avx_VarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstRegister = PrepareSymbolRegisterDefMd(dst, CX86Assembler::xMM0);

	((m_assembler).*(MDOP::OpVoAvx()))(dstRegister, MakeVariable128SymbolAddress(src1));

	CommitSymbolRegisterMdAvx(dst, dstRegister);
}

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

template <typename MDOPSHIFT, uint8 SAMASK>
void CCodeGen_x86::Emit_Md_Avx_Shift_VarVarCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstRegister = PrepareSymbolRegisterDefMd(dst, CX86Assembler::xMM0);
	auto src1Register = PrepareSymbolRegisterUseMdAvx(src1, CX86Assembler::xMM1);

	((m_assembler).*(MDOPSHIFT::OpVoAvx()))(dstRegister, src1Register, static_cast<uint8>(src2->m_valueLow & SAMASK));

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

void CCodeGen_x86::Emit_Md_Avx_Mov_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto tmpRegister = CX86Assembler::xMM0;

	m_assembler.VmovapsVo(tmpRegister, MakeMemory128SymbolAddress(src1));
	m_assembler.VmovapsVo(MakeMemory128SymbolAddress(dst), tmpRegister);
}

void CCodeGen_x86::Emit_Md_Avx_MovMasked_VarVarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	uint8 mask = static_cast<uint8>(statement.jmpCondition);

	auto dstRegister = PrepareSymbolRegisterDefMd(dst, CX86Assembler::xMM0);
	auto src1Register = PrepareSymbolRegisterUseMdAvx(src1, CX86Assembler::xMM1);

	m_assembler.VblendpsVo(dstRegister, src1Register, MakeVariable128SymbolAddress(src2), mask);

	CommitSymbolRegisterMdAvx(dst, dstRegister);
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

void CCodeGen_x86::Emit_Md_Avx_ClampS_VarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstRegister = PrepareSymbolRegisterDefMd(dst, CX86Assembler::xMM0);
	auto src1Register = PrepareSymbolRegisterUseMdAvx(src1, CX86Assembler::xMM1);

	m_assembler.VpminsdVo(dstRegister, src1Register, MakeConstant128Address(g_fpClampMask1));
	m_assembler.VpminudVo(dstRegister, dstRegister, MakeConstant128Address(g_fpClampMask2));

	CommitSymbolRegisterMdAvx(dst, dstRegister);
}

void CCodeGen_x86::Emit_Md_Avx_PackHB_VarVarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstRegister = PrepareSymbolRegisterDefMd(dst, CX86Assembler::xMM0);
	auto tempRegister = CX86Assembler::xMM1;
	auto temp2Register = CX86Assembler::xMM2;
	auto maskRegister = CX86Assembler::xMM3;

	//Generate mask (0x00FF x8)
	m_assembler.VpcmpeqdVo(maskRegister, maskRegister, CX86Assembler::MakeXmmRegisterAddress(maskRegister));
	m_assembler.VpsrlwVo(maskRegister, maskRegister, 0x08);

	//Mask both operands
	m_assembler.VpandVo(temp2Register, maskRegister, MakeVariable128SymbolAddress(src2));
	m_assembler.VpandVo(tempRegister, maskRegister, MakeVariable128SymbolAddress(src1));

	//Pack
	m_assembler.VpackuswbVo(dstRegister, temp2Register, CX86Assembler::MakeXmmRegisterAddress(tempRegister));

	CommitSymbolRegisterMdAvx(dst, dstRegister);
}

void CCodeGen_x86::Emit_Md_Avx_PackWH_VarVarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstRegister = PrepareSymbolRegisterDefMd(dst, CX86Assembler::xMM0);
	auto resultRegister = CX86Assembler::xMM1;
	auto tempRegister = CX86Assembler::xMM2;

	m_assembler.VmovapsVo(resultRegister, MakeVariable128SymbolAddress(src2));
	m_assembler.VmovapsVo(tempRegister, MakeVariable128SymbolAddress(src1));

	//Sign extend the lower half word of our registers
	m_assembler.VpslldVo(resultRegister, resultRegister, 0x10);
	m_assembler.VpsradVo(resultRegister, resultRegister, 0x10);

	m_assembler.VpslldVo(tempRegister, tempRegister, 0x10);
	m_assembler.VpsradVo(tempRegister, tempRegister, 0x10);

	//Pack
	m_assembler.VpackssdwVo(dstRegister, resultRegister, CX86Assembler::MakeXmmRegisterAddress(tempRegister));

	CommitSymbolRegisterMdAvx(dst, dstRegister);
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

void CCodeGen_x86::Emit_Md_Avx_Abs_VarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstRegister = PrepareSymbolRegisterDefMd(dst, CX86Assembler::xMM0);
	auto maskRegister = CX86Assembler::xMM1;

	assert(dstRegister != maskRegister);

	m_assembler.VpcmpeqdVo(maskRegister, maskRegister, CX86Assembler::MakeXmmRegisterAddress(maskRegister));
	m_assembler.VpsrldVo(maskRegister, maskRegister, 1);
	m_assembler.VpandVo(dstRegister, maskRegister, MakeVariable128SymbolAddress(src1));

	CommitSymbolRegisterMdAvx(dst, dstRegister);
}

void CCodeGen_x86::Emit_Md_Avx_MakeSz_VarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstRegister = PrepareSymbolRegisterDef(dst, CX86Assembler::rDX);
	auto src1Register = PrepareSymbolRegisterUseMdAvx(src1, CX86Assembler::xMM0);
	auto szRegister = CX86Assembler::xMM1;
	auto zeroRegister = CX86Assembler::xMM2;

	//Compute sign
	m_assembler.VpsradVo(szRegister, src1Register, 31);

	//Compute zero
	m_assembler.VpxorVo(zeroRegister, zeroRegister, CX86Assembler::MakeXmmRegisterAddress(zeroRegister));
	m_assembler.VcmppsVo(zeroRegister, zeroRegister, CX86Assembler::MakeXmmRegisterAddress(src1Register), CX86Assembler::SSE_CMP_EQ);

	//Pack
	m_assembler.VpackssdwVo(szRegister, szRegister, CX86Assembler::MakeXmmRegisterAddress(zeroRegister));

	//Extract bits
	m_assembler.VpshufbVo(szRegister, szRegister, MakeConstant128Address(g_makeSzShufflePattern));
	m_assembler.VpmovmskbVo(dstRegister, szRegister);

	CommitSymbolRegister(dst, dstRegister);
}

void CCodeGen_x86::Emit_Md_Avx_Expand_VarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstRegister = PrepareSymbolRegisterDefMd(dst, CX86Assembler::xMM0);

	m_assembler.VmovdVo(dstRegister, MakeVariableSymbolAddress(src1));
	m_assembler.VshufpsVo(dstRegister, dstRegister, CX86Assembler::MakeXmmRegisterAddress(dstRegister), 0x00);

	CommitSymbolRegisterMdAvx(dst, dstRegister);
}

void CCodeGen_x86::Emit_Md_Avx_Expand_VarCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstRegister = PrepareSymbolRegisterDefMd(dst, CX86Assembler::xMM0);

	if(src1->m_valueLow == 0)
	{
		m_assembler.VpxorVo(dstRegister, dstRegister, CX86Assembler::MakeXmmRegisterAddress(dstRegister));
	}
	else if(src1->m_valueLow == 0x3F800000)
	{
		m_assembler.VmovapsVo(dstRegister, MakeConstant128Address(g_fpCstOne));
	}
	else
	{
		auto cstRegister = CX86Assembler::rAX;
		m_assembler.MovId(cstRegister, src1->m_valueLow);
		m_assembler.VmovdVo(dstRegister, CX86Assembler::MakeRegisterAddress(cstRegister));
		m_assembler.VshufpsVo(dstRegister, dstRegister, CX86Assembler::MakeXmmRegisterAddress(dstRegister), 0x00);
	}

	CommitSymbolRegisterMdAvx(dst, dstRegister);
}

void CCodeGen_x86::Emit_Md_Avx_Expand_VarVarCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstRegister = PrepareSymbolRegisterDefMd(dst, CX86Assembler::xMM0);
	auto srcRegister = PrepareSymbolRegisterUseMdAvx(src1, CX86Assembler::xMM1);
	uint8 shufImm = g_mdExpandShufPatterns[src2->m_valueLow];

	m_assembler.VshufpsVo(dstRegister, srcRegister, CX86Assembler::MakeXmmRegisterAddress(srcRegister), shufImm);

	CommitSymbolRegisterMdAvx(dst, dstRegister);
}

void CCodeGen_x86::Emit_Md_Avx2_Expand_VarReg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstRegister = PrepareSymbolRegisterDefMd(dst, CX86Assembler::xMM0);

	m_assembler.VmovdVo(dstRegister, MakeVariableSymbolAddress(src1));
	m_assembler.VpbroadcastdVo(dstRegister, CX86Assembler::MakeXmmRegisterAddress(dstRegister));

	CommitSymbolRegisterMdAvx(dst, dstRegister);
}

void CCodeGen_x86::Emit_Md_Avx2_Expand_VarMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstRegister = PrepareSymbolRegisterDefMd(dst, CX86Assembler::xMM0);

	m_assembler.VpbroadcastdVo(dstRegister, MakeMemorySymbolAddress(src1));

	CommitSymbolRegisterMdAvx(dst, dstRegister);
}

void CCodeGen_x86::Emit_Md_Avx2_Expand_VarCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstRegister = PrepareSymbolRegisterDefMd(dst, CX86Assembler::xMM0);

	if(src1->m_valueLow == 0)
	{
		m_assembler.VpxorVo(dstRegister, dstRegister, CX86Assembler::MakeXmmRegisterAddress(dstRegister));
	}
	else if(src1->m_valueLow == 0x3F800000)
	{
		m_assembler.VmovapsVo(dstRegister, MakeConstant128Address(g_fpCstOne));
	}
	else
	{
		auto cstRegister = CX86Assembler::rAX;
		m_assembler.MovId(cstRegister, src1->m_valueLow);
		m_assembler.VmovdVo(dstRegister, CX86Assembler::MakeRegisterAddress(cstRegister));
		m_assembler.VpbroadcastdVo(dstRegister, CX86Assembler::MakeXmmRegisterAddress(dstRegister));
	}

	CommitSymbolRegisterMdAvx(dst, dstRegister);
}

void CCodeGen_x86::Emit_Md_Avx2_Expand_VarRegCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstRegister = PrepareSymbolRegisterDefMd(dst, CX86Assembler::xMM0);
	auto srcRegister = m_mdRegisters[src1->m_valueLow];

	if(src2->m_valueLow == 0)
	{
		m_assembler.VpbroadcastdVo(dstRegister, CX86Assembler::MakeXmmRegisterAddress(srcRegister));
	}
	else
	{
		uint8 shufImm = g_mdExpandShufPatterns[src2->m_valueLow];
		m_assembler.VshufpsVo(dstRegister, srcRegister, CX86Assembler::MakeXmmRegisterAddress(srcRegister), shufImm);
	}

	CommitSymbolRegisterMdAvx(dst, dstRegister);
}

void CCodeGen_x86::Emit_Md_Avx2_Expand_VarMemCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstRegister = PrepareSymbolRegisterDefMd(dst, CX86Assembler::xMM0);
	m_assembler.VpbroadcastdVo(dstRegister, MakeMemory128SymbolElementAddress(src1, src2->m_valueLow));
	CommitSymbolRegisterMdAvx(dst, dstRegister);
}

void CCodeGen_x86::Emit_Avx_MergeTo256_MemVarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type == SYM_TEMPORARY256);

	auto src1Register = CX86Assembler::xMM0;
	auto src2Register = CX86Assembler::xMM1;

	//TODO: Improve this to write out registers directly to temporary's memory space
	//instead of passing by temporary registers

	m_assembler.VmovdqaVo(src1Register, MakeVariable128SymbolAddress(src1));
	m_assembler.VmovdqaVo(src2Register, MakeVariable128SymbolAddress(src2));

	m_assembler.VmovdqaVo(MakeTemporary256SymbolElementAddress(dst, 0x00), src1Register);
	m_assembler.VmovdqaVo(MakeTemporary256SymbolElementAddress(dst, 0x10), src2Register);
}

void CCodeGen_x86::Emit_Md_Avx_Srl256_VarMemVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto offsetRegister = CX86Assembler::rAX;
	auto resultRegister = CX86Assembler::xMM0;

	assert(src1->m_type == SYM_TEMPORARY256);

	m_assembler.MovEd(offsetRegister, MakeVariableSymbolAddress(src2));
	m_assembler.AndId(CX86Assembler::MakeRegisterAddress(offsetRegister), 0x7F);
	m_assembler.ShrEd(CX86Assembler::MakeRegisterAddress(offsetRegister), 3);
	m_assembler.AddId(CX86Assembler::MakeRegisterAddress(offsetRegister), src1->m_stackLocation + m_stackLevel);

	m_assembler.VmovdquVo(resultRegister, CX86Assembler::MakeBaseOffIndexScaleAddress(CX86Assembler::rSP, 0, offsetRegister, 1));
	m_assembler.VmovdqaVo(MakeVariable128SymbolAddress(dst), resultRegister);
}

void CCodeGen_x86::Emit_Md_Avx_Srl256_VarMemCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto resultRegister = CX86Assembler::xMM0;

	assert(src1->m_type == SYM_TEMPORARY256);
	assert(src2->m_type == SYM_CONSTANT);

	uint32 offset = (src2->m_valueLow & 0x7F) / 8;

	m_assembler.VmovdquVo(resultRegister, MakeTemporary256SymbolElementAddress(src1, offset));
	m_assembler.VmovdqaVo(MakeVariable128SymbolAddress(dst), resultRegister);
}

void CCodeGen_x86::Emit_Md_Avx_LoadFromRef_VarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto addressReg = PrepareRefSymbolRegisterUse(src1, CX86Assembler::rAX);
	auto dstReg = PrepareSymbolRegisterDefMd(dst, CX86Assembler::xMM0);

	m_assembler.VmovapsVo(dstReg, CX86Assembler::MakeIndRegAddress(addressReg));

	CommitSymbolRegisterMdAvx(dst, dstReg);
}

void CCodeGen_x86::Emit_Md_Avx_LoadFromRef_VarVarAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	uint8 scale = static_cast<uint8>(statement.jmpCondition);

	assert(scale == 1);

	auto dstReg = PrepareSymbolRegisterDefMd(dst, CX86Assembler::xMM0);
	m_assembler.VmovapsVo(dstReg, MakeRefBaseScaleSymbolAddress(src1, CX86Assembler::rAX, src2, CX86Assembler::rCX, scale));
	CommitSymbolRegisterMdAvx(dst, dstReg);
}

void CCodeGen_x86::Emit_Md_Avx_StoreAtRef_VarVar(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto addressReg = PrepareRefSymbolRegisterUse(src1, CX86Assembler::rAX);
	auto valueReg = PrepareSymbolRegisterUseMdAvx(src2, CX86Assembler::xMM0);

	m_assembler.VmovapsVo(CX86Assembler::MakeIndRegAddress(addressReg), valueReg);
}

void CCodeGen_x86::Emit_Md_Avx_StoreAtRef_VarAnyVar(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	uint8 scale = static_cast<uint8>(statement.jmpCondition);
	auto src3 = statement.src3->GetSymbol().get();

	assert(scale == 1);

	auto valueReg = PrepareSymbolRegisterUseMdAvx(src3, CX86Assembler::xMM0);
	m_assembler.VmovapsVo(MakeRefBaseScaleSymbolAddress(src1, CX86Assembler::rAX, src2, CX86Assembler::rCX, scale), valueReg);
}

void CCodeGen_x86::Emit_Md_Avx_LoadFromRefMasked_VarVarAnyVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	auto src3 = statement.src3->GetSymbol().get();
	uint8 mask = static_cast<uint8>(statement.jmpCondition);

	auto dstReg = PrepareSymbolRegisterDefMd(dst, CX86Assembler::xMM0);
	auto src3Reg = PrepareSymbolRegisterUseMdAvx(src3, CX86Assembler::xMM1);

	m_assembler.VblendpsVo(dstReg, src3Reg, MakeRefBaseScaleSymbolAddress(src1, CX86Assembler::rAX, src2, CX86Assembler::rCX, 1), mask);
	CommitSymbolRegisterMdAvx(dst, dstReg);
}

void CCodeGen_x86::Emit_Md_Avx_StoreAtRefMasked_VarAnyVar(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	auto src3 = statement.src3->GetSymbol().get();
	uint8 mask = static_cast<uint8>(statement.jmpCondition);

	auto valueReg = PrepareSymbolRegisterUseMdAvx(src3, CX86Assembler::xMM0);
	auto tmpReg = CX86Assembler::xMM1;
	auto dstAddress = MakeRefBaseScaleSymbolAddress(src1, CX86Assembler::rAX, src2, CX86Assembler::rCX, 1);

	m_assembler.VblendpsVo(tmpReg, valueReg, dstAddress, ~mask & 0x0F);
	m_assembler.VmovapsVo(dstAddress, tmpReg);
}

// clang-format off
CCodeGen_x86::CONSTMATCHER CCodeGen_x86::g_mdAvxConstMatchers[] = 
{
	{ OP_MD_ADD_B, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_ADDB> },
	{ OP_MD_ADD_H, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_ADDH> },
	{ OP_MD_ADD_W, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_ADDW> },

	{ OP_MD_ADDSS_B, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_ADDSSB> },
	{ OP_MD_ADDSS_H, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_ADDSSH> },
	{ OP_MD_ADDSS_W, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_AddSSW_VarVarVar       },

	{ OP_MD_ADDUS_B, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_ADDUSB> },
	{ OP_MD_ADDUS_H, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_ADDUSH> },
	{ OP_MD_ADDUS_W, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_AddUSW_VarVarVar       },

	{ OP_MD_SUB_B, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_SUBB> },
	{ OP_MD_SUB_H, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_SUBH> },
	{ OP_MD_SUB_W, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_SUBW> },

	{ OP_MD_SUBSS_H, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_SUBSSH> },
	{ OP_MD_SUBSS_W, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_SubSSW_VarVarVar       },

	{ OP_MD_SUBUS_B, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_SUBUSB> },
	{ OP_MD_SUBUS_H, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_SUBUSH> },
	{ OP_MD_SUBUS_W, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_SubUSW_VarVarVar       },

	{ OP_MD_CLAMP_S, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL,         MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_ClampS_VarVar          },

	{ OP_MD_CMPEQ_B, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_CMPEQB> },
	{ OP_MD_CMPEQ_H, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_CMPEQH> },
	{ OP_MD_CMPEQ_W, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_CMPEQW> },

	{ OP_MD_CMPGT_B, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_CMPGTB> },
	{ OP_MD_CMPGT_H, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_CMPGTH> },
	{ OP_MD_CMPGT_W, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_CMPGTW> },

	{ OP_MD_MIN_H, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_MINH> },
	{ OP_MD_MIN_W, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_MINW> },

	{ OP_MD_MAX_H, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_MAXH> },
	{ OP_MD_MAX_W, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_MAXW> },

	{ OP_MD_AND, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_AND> },
	{ OP_MD_OR,  MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_OR>  },
	{ OP_MD_XOR, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_XOR> },

	{ OP_MD_NOT, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_Not_VarVar },

	{ OP_MD_SRLH, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_CONSTANT, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_Shift_VarVarCst<MDOP_SRLH, 0x0F> },
	{ OP_MD_SRAH, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_CONSTANT, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_Shift_VarVarCst<MDOP_SRAH, 0x0F> },
	{ OP_MD_SLLH, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_CONSTANT, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_Shift_VarVarCst<MDOP_SLLH, 0x0F> },

	{ OP_MD_SRLW, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_CONSTANT, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_Shift_VarVarCst<MDOP_SRLW, 0x1F> },
	{ OP_MD_SRAW, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_CONSTANT, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_Shift_VarVarCst<MDOP_SRAW, 0x1F> },
	{ OP_MD_SLLW, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_CONSTANT, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_Shift_VarVarCst<MDOP_SLLW, 0x1F> },

	{ OP_MD_UNPACK_LOWER_BH, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_VarVarVarRev<MDOP_UNPACK_LOWER_BH> },
	{ OP_MD_UNPACK_LOWER_HW, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_VarVarVarRev<MDOP_UNPACK_LOWER_HW> },
	{ OP_MD_UNPACK_LOWER_WD, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_VarVarVarRev<MDOP_UNPACK_LOWER_WD> },

	{ OP_MD_UNPACK_UPPER_BH, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_VarVarVarRev<MDOP_UNPACK_UPPER_BH> },
	{ OP_MD_UNPACK_UPPER_HW, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_VarVarVarRev<MDOP_UNPACK_UPPER_HW> },
	{ OP_MD_UNPACK_UPPER_WD, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_VarVarVarRev<MDOP_UNPACK_UPPER_WD> },

	{ OP_MD_ADD_S, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_ADDS> },
	{ OP_MD_SUB_S, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_SUBS> },
	{ OP_MD_MUL_S, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_MULS> },
	{ OP_MD_DIV_S, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_DIVS> },

	{ OP_MD_ABS_S, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_Abs_VarVar },

	{ OP_MD_CMPLT_S, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_CMPLTS> },
	{ OP_MD_CMPGT_S, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_CMPGTS> },

	{ OP_MD_MIN_S, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_MINS> },
	{ OP_MD_MAX_S, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_MAXS> },

	{ OP_MD_TOWORD_TRUNCATE, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_VarVar<MDOP_TOWORD_TRUNCATE> },
	{ OP_MD_TOSINGLE,        MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_VarVar<MDOP_TOSINGLE>        },

	{ OP_MD_PACK_HB, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_PackHB_VarVarVar, },
	{ OP_MD_PACK_WH, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_PackWH_VarVarVar, },

	{ OP_MD_MAKESZ, MATCH_VARIABLE, MATCH_VARIABLE128, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_MakeSz_VarVar },

	{ OP_MOV, MATCH_REGISTER128, MATCH_VARIABLE128, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_Mov_RegVar, },
	{ OP_MOV, MATCH_MEMORY128,   MATCH_REGISTER128, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_Mov_MemReg, },
	{ OP_MOV, MATCH_MEMORY128,   MATCH_MEMORY128,   MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_Mov_MemMem, },

	{ OP_MD_MOV_MASKED, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_MovMasked_VarVarVar },

	{ OP_MERGETO256, MATCH_MEMORY256,   MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Avx_MergeTo256_MemVarVar },
	{ OP_MD_SRL256,  MATCH_VARIABLE128, MATCH_MEMORY256,   MATCH_VARIABLE,    MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_Srl256_VarMemVar  },
	{ OP_MD_SRL256,  MATCH_VARIABLE128, MATCH_MEMORY256,   MATCH_CONSTANT,    MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_Srl256_VarMemCst  },

	{ OP_LOADFROMREF, MATCH_VARIABLE128, MATCH_VAR_REF, MATCH_NIL,   MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_LoadFromRef_VarVar },
	{ OP_LOADFROMREF, MATCH_VARIABLE128, MATCH_VAR_REF, MATCH_ANY32, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_LoadFromRef_VarVarAny },

	{ OP_STOREATREF, MATCH_NIL, MATCH_VAR_REF, MATCH_VARIABLE128, MATCH_NIL,         &CCodeGen_x86::Emit_Md_Avx_StoreAtRef_VarVar },
	{ OP_STOREATREF, MATCH_NIL, MATCH_VAR_REF, MATCH_ANY32,       MATCH_VARIABLE128, &CCodeGen_x86::Emit_Md_Avx_StoreAtRef_VarAnyVar },

	{ OP_MD_LOADFROMREF_MASKED, MATCH_VARIABLE128, MATCH_VAR_REF, MATCH_ANY32, MATCH_VARIABLE128, &CCodeGen_x86::Emit_Md_Avx_LoadFromRefMasked_VarVarAnyVar },
	{ OP_MD_STOREATREF_MASKED,  MATCH_NIL,         MATCH_VAR_REF, MATCH_ANY32, MATCH_VARIABLE128, &CCodeGen_x86::Emit_Md_Avx_StoreAtRefMasked_VarAnyVar     },

	{ OP_MOV, MATCH_NIL,         MATCH_NIL,         MATCH_NIL, MATCH_NIL, nullptr },
};

CCodeGen_x86::CONSTMATCHER CCodeGen_x86::g_mdNoAvx2ConstMatchers[] =
{
	{ OP_MD_EXPAND, MATCH_VARIABLE128, MATCH_VARIABLE, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_Expand_VarVar },
	{ OP_MD_EXPAND, MATCH_VARIABLE128, MATCH_CONSTANT, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_Expand_VarCst },

	{ OP_MD_EXPAND, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_CONSTANT, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_Expand_VarVarCst },

	{ OP_MOV, MATCH_NIL,         MATCH_NIL,         MATCH_NIL, MATCH_NIL, nullptr },
};

CCodeGen_x86::CONSTMATCHER CCodeGen_x86::g_mdAvx2ConstMatchers[] =
{
	{ OP_MD_EXPAND, MATCH_VARIABLE128, MATCH_REGISTER, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx2_Expand_VarReg },
	{ OP_MD_EXPAND, MATCH_VARIABLE128, MATCH_MEMORY,   MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx2_Expand_VarMem },
	{ OP_MD_EXPAND, MATCH_VARIABLE128, MATCH_CONSTANT, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx2_Expand_VarCst },

	{ OP_MD_EXPAND, MATCH_VARIABLE128, MATCH_REGISTER128, MATCH_CONSTANT, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx2_Expand_VarRegCst },
	{ OP_MD_EXPAND, MATCH_VARIABLE128, MATCH_MEMORY128,   MATCH_CONSTANT, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx2_Expand_VarMemCst },

	{ OP_MOV, MATCH_NIL, MATCH_NIL, MATCH_NIL, MATCH_NIL, nullptr },
};
// clang-format on
