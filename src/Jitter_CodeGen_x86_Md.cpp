#include "Jitter_CodeGen_x86.h"

using namespace Jitter;

CX86Assembler::CAddress CCodeGen_x86::MakeRelative128SymbolElementAddress(CSymbol* symbol, unsigned int elementIdx)
{
	assert(symbol->m_type == SYM_RELATIVE128);
	assert((symbol->m_valueLow & 0xF) == 0);
	return CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, symbol->m_valueLow + (elementIdx * 4));
}

CX86Assembler::CAddress CCodeGen_x86::MakeTemporary128SymbolElementAddress(CSymbol* symbol, unsigned int elementIdx)
{
	assert(symbol->m_type == SYM_TEMPORARY128);
//	assert(((symbol->m_stackLocation + m_stackLevel) & 0xF) == 0);
	return CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, symbol->m_stackLocation + m_stackLevel + (elementIdx * 4));
}

CX86Assembler::CAddress CCodeGen_x86::MakeTemporary256SymbolElementAddress(CSymbol* symbol, unsigned int elementIdx)
{
	assert(symbol->m_type == SYM_TEMPORARY256);
	assert(((symbol->m_stackLocation + m_stackLevel) & 0x1F) == 0);
	return CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, symbol->m_stackLocation + m_stackLevel + elementIdx);
}

CX86Assembler::CAddress CCodeGen_x86::MakeVariable128SymbolAddress(CSymbol* symbol)
{
	switch(symbol->m_type)
	{
	case SYM_REGISTER128:
		return CX86Assembler::MakeXmmRegisterAddress(m_mdRegisters[symbol->m_valueLow]);
		break;
	case SYM_RELATIVE128:
		return MakeRelative128SymbolElementAddress(symbol, 0);
		break;
	case SYM_TEMPORARY128:
		return MakeTemporary128SymbolElementAddress(symbol, 0);
		break;
	default:
		throw std::exception();
		break;
	}
}

CX86Assembler::CAddress CCodeGen_x86::MakeMemory128SymbolAddress(CSymbol* symbol)
{
	switch(symbol->m_type)
	{
	case SYM_RELATIVE128:
		return MakeRelative128SymbolElementAddress(symbol, 0);
		break;
	case SYM_TEMPORARY128:
		return MakeTemporary128SymbolElementAddress(symbol, 0);
		break;
	default:
		throw std::exception();
		break;
	}
}

CX86Assembler::CAddress CCodeGen_x86::MakeMemory128SymbolElementAddress(CSymbol* symbol, unsigned int elementIdx)
{
	switch(symbol->m_type)
	{
	case SYM_RELATIVE128:
		return MakeRelative128SymbolElementAddress(symbol, elementIdx);
		break;
	case SYM_TEMPORARY128:
		return MakeTemporary128SymbolElementAddress(symbol, elementIdx);
		break;
	default:
		throw std::exception();
		break;
	}
}

template <typename MDOP>
void CCodeGen_x86::Emit_Md_RegVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	((m_assembler).*(MDOP::OpVo()))(m_mdRegisters[dst->m_valueLow], MakeVariable128SymbolAddress(src1));
}

template <typename MDOP>
void CCodeGen_x86::Emit_Md_MemVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstRegister = CX86Assembler::xMM0;

	((m_assembler).*(MDOP::OpVo()))(dstRegister, MakeVariable128SymbolAddress(src1));
	m_assembler.MovapsVo(MakeMemory128SymbolAddress(dst), dstRegister);
}

template <typename MDOP>
void CCodeGen_x86::Emit_Md_RegRegReg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	if(dst->Equals(src1))
	{
		((m_assembler).*(MDOP::OpVo()))(m_mdRegisters[dst->m_valueLow], 
			CX86Assembler::MakeXmmRegisterAddress(m_mdRegisters[src2->m_valueLow]));
	}
	else
	{
		auto src2Register = m_mdRegisters[src2->m_valueLow];

		if(dst->Equals(src2))
		{
			m_assembler.MovapsVo(CX86Assembler::xMM0, CX86Assembler::MakeXmmRegisterAddress(m_mdRegisters[src2->m_valueLow]));
			src2Register = CX86Assembler::xMM0;
		}

		m_assembler.MovapsVo(m_mdRegisters[dst->m_valueLow], CX86Assembler::MakeXmmRegisterAddress(m_mdRegisters[src1->m_valueLow]));
		((m_assembler).*(MDOP::OpVo()))(m_mdRegisters[dst->m_valueLow], CX86Assembler::MakeXmmRegisterAddress(src2Register));
	}
}

template <typename MDOP>
void CCodeGen_x86::Emit_Md_RegVarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	//If we get in here, it must absolutely mean that the second source isn't a register
	//Otherwise, some of the assumuptions done below will be wrong (dst mustn't be equal to src2)
	assert(src2->m_type != SYM_REGISTER);

	auto dstRegister = m_mdRegisters[dst->m_valueLow];

	if(!dst->Equals(src1))
	{
		m_assembler.MovapsVo(dstRegister, MakeVariable128SymbolAddress(src1));
	}

	((m_assembler).*(MDOP::OpVo()))(dstRegister, MakeVariable128SymbolAddress(src2));
}

template <typename MDOP>
void CCodeGen_x86::Emit_Md_MemVarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstRegister = CX86Assembler::xMM0;

	m_assembler.MovapsVo(dstRegister, MakeVariable128SymbolAddress(src1));
	((m_assembler).*(MDOP::OpVo()))(dstRegister, MakeVariable128SymbolAddress(src2));
	m_assembler.MovapsVo(MakeMemory128SymbolAddress(dst), dstRegister);
}

template <typename MDOP>
void CCodeGen_x86::Emit_Md_VarVarVarRev(const STATEMENT& statement)
{
	//TODO: This could be improved further, but we might want
	//to reverse the operands somewhere else as to not
	//copy paste the code from the "non-reversed" path

	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstRegister = CX86Assembler::xMM0;

	m_assembler.MovapsVo(dstRegister, MakeVariable128SymbolAddress(src2));
	((m_assembler).*(MDOP::OpVo()))(dstRegister, MakeVariable128SymbolAddress(src1));
	m_assembler.MovapsVo(MakeVariable128SymbolAddress(dst), dstRegister);
}

template <typename MDOPSHIFT> 
void CCodeGen_x86::Emit_Md_Shift_RegVarCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstRegister = m_mdRegisters[dst->m_valueLow];

	if(!dst->Equals(src1))
	{
		m_assembler.MovapsVo(dstRegister, MakeVariable128SymbolAddress(src1));
	}

	((m_assembler).*(MDOPSHIFT::OpVo()))(dstRegister, static_cast<uint8>(src2->m_valueLow));
}

template <typename MDOPSHIFT> 
void CCodeGen_x86::Emit_Md_Shift_MemVarCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto tmpRegister = CX86Assembler::xMM0;

	m_assembler.MovapsVo(tmpRegister, MakeVariable128SymbolAddress(src1));
	((m_assembler).*(MDOPSHIFT::OpVo()))(tmpRegister, static_cast<uint8>(src2->m_valueLow));
	m_assembler.MovapsVo(MakeMemory128SymbolAddress(dst), tmpRegister);
}

template <typename MDOPFLAG>
void CCodeGen_x86::Emit_Md_GetFlag_RegVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	((*this).*(MDOPFLAG::OpEd()))(m_registers[dst->m_valueLow], MakeVariable128SymbolAddress(src1));
}

template <typename MDOPFLAG>
void CCodeGen_x86::Emit_Md_GetFlag_MemVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto tmpRegister = CX86Assembler::rAX;
	((*this).*(MDOPFLAG::OpEd()))(tmpRegister, MakeVariable128SymbolAddress(src1));
	m_assembler.MovGd(MakeMemorySymbolAddress(dst), tmpRegister);
}

void CCodeGen_x86::Emit_Md_AddSSW_VarVarVar(const STATEMENT& statement)
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
//		sign >>= 31;
//		res = (res & sign) | (ux & ~sign);
//		
//		return res;
//	}

	//ux = src1
	//uy = src2
	m_assembler.MovapsVo(uxRegister, MakeVariable128SymbolAddress(src1));
	m_assembler.MovapsVo(uyRegister, MakeVariable128SymbolAddress(src2));
	
	//res = ux + uy
	m_assembler.MovapsVo(resRegister, CX86Assembler::MakeXmmRegisterAddress(uxRegister));
	m_assembler.PadddVo(resRegister, CX86Assembler::MakeXmmRegisterAddress(uyRegister));

	//cst = 0x7FFFFFFF
	m_assembler.PcmpeqdVo(cstRegister, CX86Assembler::MakeXmmRegisterAddress(cstRegister));
	m_assembler.PsrldVo(cstRegister, 1);

	//ux = (ux >> 31)
	m_assembler.PsrldVo(uxRegister, 31);

	//ux += 0x7FFFFFFF
	m_assembler.PadddVo(uxRegister, CX86Assembler::MakeXmmRegisterAddress(cstRegister));

	//uy = ~(uy ^ res)
	//------
	//uy ^ res
	m_assembler.PxorVo(uyRegister, CX86Assembler::MakeXmmRegisterAddress(resRegister));

	//~(uy ^ res)
	m_assembler.PcmpeqdVo(cstRegister, CX86Assembler::MakeXmmRegisterAddress(cstRegister));
	m_assembler.PxorVo(uyRegister, CX86Assembler::MakeXmmRegisterAddress(cstRegister));

	//cst = ux ^ uy (reloading uy from src2 because we don't have any registers available)
	m_assembler.MovapsVo(cstRegister ,CX86Assembler::MakeXmmRegisterAddress(uxRegister));
	m_assembler.PxorVo(cstRegister, MakeVariable128SymbolAddress(src2));

	//uy = ((ux ^ uy) | ~(uy ^ res)) >> 31; (signed operation)
	m_assembler.PorVo(uyRegister, CX86Assembler::MakeXmmRegisterAddress(cstRegister));
	m_assembler.PsradVo(uyRegister, 31);

	//res = (res & uy)	(uy is the sign value)
	m_assembler.PandVo(resRegister, CX86Assembler::MakeXmmRegisterAddress(uyRegister));

	//ux = (ux & ~uy)
	//------
	//~uy
	m_assembler.PcmpeqdVo(cstRegister, CX86Assembler::MakeXmmRegisterAddress(cstRegister));
	m_assembler.PxorVo(uyRegister, CX86Assembler::MakeXmmRegisterAddress(cstRegister));

	//ux & ~uy
	m_assembler.PandVo(uxRegister, CX86Assembler::MakeXmmRegisterAddress(uyRegister));

	//res = (res & uy) | (ux & ~uy)
	m_assembler.PorVo(resRegister, CX86Assembler::MakeXmmRegisterAddress(uxRegister));

	//Copy final result
	m_assembler.MovapsVo(MakeVariable128SymbolAddress(dst), resRegister);
}

void CCodeGen_x86::Emit_Md_AddUSW_VarVarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto xRegister = CX86Assembler::xMM0;
	auto tmpRegister = CX86Assembler::xMM1;
	auto resRegister = CX86Assembler::xMM2;

//	This is based on code from http://locklessinc.com/articles/sat_arithmetic/
//	u32b sat_addu32b(u32b x, u32b y)
//	{
//		u32b res = x + y;
//		res |= -(res < x);
//	
//		return res;
//	}

	m_assembler.MovapsVo(xRegister, MakeVariable128SymbolAddress(src1));
	m_assembler.MovapsVo(resRegister, CX86Assembler::MakeXmmRegisterAddress(xRegister));
	m_assembler.PadddVo(resRegister, MakeVariable128SymbolAddress(src2));
	
	//-(res < x)
	m_assembler.MovapsVo(tmpRegister, CX86Assembler::MakeXmmRegisterAddress(resRegister));
	m_assembler.PsubdVo(tmpRegister, CX86Assembler::MakeXmmRegisterAddress(xRegister));
	m_assembler.PsradVo(tmpRegister, 31);

	//res |= -(res < x)
	m_assembler.PorVo(resRegister, CX86Assembler::MakeXmmRegisterAddress(tmpRegister));

	//Store result
	m_assembler.MovapsVo(MakeVariable128SymbolAddress(dst), resRegister);
}

void CCodeGen_x86::Emit_Md_PackHB_VarVarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto resultRegister = CX86Assembler::xMM0;
	auto tempRegister = CX86Assembler::xMM1;
	auto maskRegister = CX86Assembler::xMM2;

	m_assembler.MovapsVo(resultRegister, MakeVariable128SymbolAddress(src2));
	m_assembler.MovapsVo(tempRegister, MakeVariable128SymbolAddress(src1));

	//Generate mask (0x00FF x8)
	m_assembler.PcmpeqdVo(maskRegister, CX86Assembler::MakeXmmRegisterAddress(maskRegister));
	m_assembler.PsrlwVo(maskRegister, 0x08);

	//Mask both operands
	m_assembler.PandVo(resultRegister, CX86Assembler::MakeXmmRegisterAddress(maskRegister));
	m_assembler.PandVo(tempRegister, CX86Assembler::MakeXmmRegisterAddress(maskRegister));

	//Pack
	m_assembler.PackuswbVo(resultRegister, CX86Assembler::MakeXmmRegisterAddress(tempRegister));

	m_assembler.MovapsVo(MakeVariable128SymbolAddress(dst), resultRegister);
}

void CCodeGen_x86::Emit_Md_PackWH_VarVarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto resultRegister = CX86Assembler::xMM0;
	auto tempRegister = CX86Assembler::xMM1;

	m_assembler.MovapsVo(resultRegister, MakeVariable128SymbolAddress(src2));
	m_assembler.MovapsVo(tempRegister, MakeVariable128SymbolAddress(src1));

	//Sign extend the lower half word of our registers
	m_assembler.PslldVo(resultRegister, 0x10);
	m_assembler.PsradVo(resultRegister, 0x10);

	m_assembler.PslldVo(tempRegister, 0x10);
	m_assembler.PsradVo(tempRegister, 0x10);

	//Pack
	m_assembler.PackssdwVo(resultRegister, CX86Assembler::MakeXmmRegisterAddress(tempRegister));

	m_assembler.MovapsVo(MakeVariable128SymbolAddress(dst), resultRegister);
}

void CCodeGen_x86::Emit_Md_Not_VarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto resultRegister = CX86Assembler::xMM0;
	auto cstRegister = CX86Assembler::xMM1;

	m_assembler.MovapsVo(resultRegister, MakeVariable128SymbolAddress(src1));
	m_assembler.PcmpeqdVo(cstRegister, CX86Assembler::MakeXmmRegisterAddress(cstRegister));
	m_assembler.PxorVo(resultRegister, CX86Assembler::MakeXmmRegisterAddress(cstRegister));

	m_assembler.MovapsVo(MakeVariable128SymbolAddress(dst), resultRegister);
}

void CCodeGen_x86::Emit_Md_MovMasked_VarVarCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_CONSTANT);

	auto mask0Register = CX86Assembler::xMM0;
	auto mask1Register = CX86Assembler::xMM1;

	//mask0 = 0x00000000
	//mask1 = 0xFFFFFFFF
	m_assembler.PxorVo(mask0Register, CX86Assembler::MakeXmmRegisterAddress(mask0Register));
	m_assembler.PcmpeqdVo(mask1Register, CX86Assembler::MakeXmmRegisterAddress(mask1Register));

	//mask0 -> 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000 (use movhlps?)
	m_assembler.ShufpsVo(mask0Register, CX86Assembler::MakeXmmRegisterAddress(mask1Register), 0x00);

	//Generate shuffle selector
	//0x00 -> gives us 0x00000000
	//0x02 -> gives us 0xFFFFFFFF
	uint8 shuffleSelector = 0;
	for(unsigned int i = 0; i < 4; i++)
	{
		if(src2->m_valueLow & (1 << i))
		{
			shuffleSelector |= (0x02) << (i * 2);
		}
	}

	//mask0 -> proper mask
	m_assembler.ShufpsVo(mask0Register, CX86Assembler::MakeXmmRegisterAddress(mask0Register), shuffleSelector);

	//mask1 -> mask inverse
	m_assembler.PxorVo(mask1Register, CX86Assembler::MakeXmmRegisterAddress(mask0Register));

	//Generate result
	m_assembler.PandVo(mask0Register, MakeVariable128SymbolAddress(src1));
	m_assembler.PandVo(mask1Register, MakeVariable128SymbolAddress(dst));
	m_assembler.PorVo(mask0Register, CX86Assembler::MakeXmmRegisterAddress(mask1Register));

	m_assembler.MovapsVo(MakeVariable128SymbolAddress(dst), mask0Register);
}

void CCodeGen_x86::Emit_Md_Mov_RegVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	m_assembler.MovapsVo(m_mdRegisters[dst->m_valueLow], MakeVariable128SymbolAddress(src1));
}

void CCodeGen_x86::Emit_Md_Mov_MemReg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	m_assembler.MovapsVo(MakeMemory128SymbolAddress(dst), m_mdRegisters[src1->m_valueLow]);
}

void CCodeGen_x86::Emit_Md_Mov_MemMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	CX86Assembler::XMMREGISTER resultRegister = CX86Assembler::xMM0;

	m_assembler.MovapsVo(resultRegister, MakeMemory128SymbolAddress(src1));
	m_assembler.MovapsVo(MakeMemory128SymbolAddress(dst), resultRegister);
}

void CCodeGen_x86::Emit_Md_Abs_RegReg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	if(!dst->Equals(src1))
	{
		m_assembler.MovapsVo(m_mdRegisters[dst->m_valueLow], CX86Assembler::MakeXmmRegisterAddress(m_mdRegisters[src1->m_valueLow]));
	}

	Emit_Md_Abs(m_mdRegisters[dst->m_valueLow]);
}

void CCodeGen_x86::Emit_Md_Abs_MemVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto resultRegister = CX86Assembler::xMM0;

	m_assembler.MovapsVo(resultRegister, MakeVariable128SymbolAddress(src1));
	Emit_Md_Abs(resultRegister);
	m_assembler.MovapsVo(MakeMemory128SymbolAddress(dst), resultRegister);
}

void CCodeGen_x86::Emit_Md_Abs(CX86Assembler::XMMREGISTER dstRegister)
{
	auto maskRegister = CX86Assembler::xMM1;

	assert(dstRegister != maskRegister);

	m_assembler.PcmpeqdVo(maskRegister, CX86Assembler::MakeXmmRegisterAddress(maskRegister));
	m_assembler.PsrldVo(maskRegister, 1);
	m_assembler.PandVo(dstRegister, CX86Assembler::MakeXmmRegisterAddress(maskRegister));
}

void CCodeGen_x86::Emit_Md_IsNegative(CX86Assembler::REGISTER dstRegister, const CX86Assembler::CAddress& srcAddress)
{
	CX86Assembler::XMMREGISTER valueRegister = CX86Assembler::xMM0;
	CX86Assembler::XMMREGISTER zeroRegister = CX86Assembler::xMM1;
	CX86Assembler::XMMREGISTER tmpRegister = CX86Assembler::xMM2;
	CX86Assembler::REGISTER tmpFlagRegister(CX86Assembler::rDX);
	assert(dstRegister != tmpFlagRegister);

	//valueRegister = [srcAddress]
	m_assembler.MovapsVo(valueRegister, srcAddress);

	//----- Generate isZero

	//tmpRegister = 0
	m_assembler.PandnVo(tmpRegister, CX86Assembler::MakeXmmRegisterAddress(tmpRegister));

	//zeroRegister = 0xFFFFFFFF
	m_assembler.PcmpeqdVo(zeroRegister, CX86Assembler::MakeXmmRegisterAddress(zeroRegister));

	//zeroRegister = 0x7FFFFFFF
	m_assembler.PsrldVo(zeroRegister, 1);

	//zeroRegister &= valueRegister
	m_assembler.PandVo(zeroRegister, CX86Assembler::MakeXmmRegisterAddress(valueRegister));

	//zeroRegister = (zeroRegister == tmpRegister)
	m_assembler.PcmpeqdVo(zeroRegister, CX86Assembler::MakeXmmRegisterAddress(tmpRegister));

	//----- Generate isNegative
	//valueRegister >>= 31 (s-extended)
	m_assembler.PsradVo(valueRegister, 31);

	//----- Generate result
	//zeroRegister = (not zeroRegister) & valueRegister
	m_assembler.PandnVo(zeroRegister, CX86Assembler::MakeXmmRegisterAddress(valueRegister));

	//Extract bits
	m_assembler.PmovmskbVo(tmpFlagRegister, zeroRegister);
	
	//Generate bit field
	m_assembler.XorEd(dstRegister, CX86Assembler::MakeRegisterAddress(dstRegister));
	for(unsigned int i = 0; i < 4; i++)
	{
		m_assembler.ShrEd(CX86Assembler::MakeRegisterAddress(tmpFlagRegister), 4);
		m_assembler.RclEd(CX86Assembler::MakeRegisterAddress(dstRegister), 1);
	}
}

void CCodeGen_x86::Emit_Md_IsZero(CX86Assembler::REGISTER dstRegister, const CX86Assembler::CAddress& srcAddress)
{
	CX86Assembler::XMMREGISTER valueRegister = CX86Assembler::xMM0;
	CX86Assembler::XMMREGISTER zeroRegister = CX86Assembler::xMM1;
	CX86Assembler::REGISTER tmpFlagRegister(CX86Assembler::rDX);
	assert(dstRegister != tmpFlagRegister);

	//Get value - And with 0x7FFFFFFF to remove sign bit
	m_assembler.PcmpeqdVo(valueRegister, CX86Assembler::MakeXmmRegisterAddress(valueRegister));
	m_assembler.PsrldVo(valueRegister, 1);
	m_assembler.PandVo(valueRegister, srcAddress);

	//Generate zero and compare
	m_assembler.PandnVo(zeroRegister, CX86Assembler::MakeXmmRegisterAddress(zeroRegister));
	m_assembler.PcmpeqdVo(valueRegister, CX86Assembler::MakeXmmRegisterAddress(zeroRegister));

	//Extract bits
	m_assembler.PmovmskbVo(tmpFlagRegister, valueRegister);
	
	//Generate bit field
	m_assembler.XorEd(dstRegister, CX86Assembler::MakeRegisterAddress(dstRegister));
	for(unsigned int i = 0; i < 4; i++)
	{
		m_assembler.ShrEd(CX86Assembler::MakeRegisterAddress(tmpFlagRegister), 4);
		m_assembler.RclEd(CX86Assembler::MakeRegisterAddress(dstRegister), 1);
	}
}

void CCodeGen_x86::Emit_Md_Expand_RegReg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto resultRegister = m_mdRegisters[dst->m_valueLow];

	m_assembler.MovdVo(resultRegister, CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	m_assembler.ShufpsVo(resultRegister, CX86Assembler::MakeXmmRegisterAddress(resultRegister), 0x00);
}

void CCodeGen_x86::Emit_Md_Expand_RegMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto resultRegister = m_mdRegisters[dst->m_valueLow];

	m_assembler.MovssEd(resultRegister, MakeMemorySymbolAddress(src1));
	m_assembler.ShufpsVo(resultRegister, CX86Assembler::MakeXmmRegisterAddress(resultRegister), 0x00);
}

void CCodeGen_x86::Emit_Md_Expand_RegCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto cstRegister = CX86Assembler::rAX;
	auto resultRegister = m_mdRegisters[dst->m_valueLow];

	m_assembler.MovId(cstRegister, src1->m_valueLow);
	m_assembler.MovdVo(resultRegister, CX86Assembler::MakeRegisterAddress(cstRegister));
	m_assembler.ShufpsVo(resultRegister, CX86Assembler::MakeXmmRegisterAddress(resultRegister), 0x00);
}

void CCodeGen_x86::Emit_Md_Expand_MemReg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto resultRegister = CX86Assembler::xMM0;

	m_assembler.MovdVo(resultRegister, CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	m_assembler.ShufpsVo(resultRegister, CX86Assembler::MakeXmmRegisterAddress(resultRegister), 0x00);
	m_assembler.MovapsVo(MakeMemory128SymbolAddress(dst), resultRegister);
}

void CCodeGen_x86::Emit_Md_Expand_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto resultRegister = CX86Assembler::xMM0;

	m_assembler.MovssEd(resultRegister, MakeMemorySymbolAddress(src1));
	m_assembler.ShufpsVo(resultRegister, CX86Assembler::MakeXmmRegisterAddress(resultRegister), 0x00);
	m_assembler.MovapsVo(MakeMemory128SymbolAddress(dst), resultRegister);
}

void CCodeGen_x86::Emit_Md_Expand_MemCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto cstRegister = CX86Assembler::rAX;
	auto resultRegister = CX86Assembler::xMM0;

	m_assembler.MovId(cstRegister, src1->m_valueLow);
	m_assembler.MovdVo(resultRegister, CX86Assembler::MakeRegisterAddress(cstRegister));
	m_assembler.ShufpsVo(resultRegister, CX86Assembler::MakeXmmRegisterAddress(resultRegister), 0x00);
	m_assembler.MovapsVo(MakeMemory128SymbolAddress(dst), resultRegister);
}

void CCodeGen_x86::Emit_Md_Srl256_VarMem(CSymbol* dst, CSymbol* src1, const CX86Assembler::CAddress& offsetAddress)
{
	auto offsetRegister = CX86Assembler::rAX;
	auto resultRegister = CX86Assembler::xMM0;

	assert(src1->m_type == SYM_TEMPORARY256);

	m_assembler.MovEd(offsetRegister, offsetAddress);
	m_assembler.AndId(CX86Assembler::MakeRegisterAddress(offsetRegister), 0x7F);
	m_assembler.ShrEd(CX86Assembler::MakeRegisterAddress(offsetRegister), 3);
	m_assembler.AddId(CX86Assembler::MakeRegisterAddress(offsetRegister), src1->m_stackLocation + m_stackLevel);

	m_assembler.MovdquVo(resultRegister, CX86Assembler::MakeBaseIndexScaleAddress(CX86Assembler::rSP, offsetRegister, 1));
	m_assembler.MovapsVo(MakeVariable128SymbolAddress(dst), resultRegister);
}

void CCodeGen_x86::Emit_Md_Srl256_VarMemVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	Emit_Md_Srl256_VarMem(dst, src1, MakeVariableSymbolAddress(src2));
}

void CCodeGen_x86::Emit_Md_Srl256_VarMemCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto resultRegister = CX86Assembler::xMM0;

	assert(src1->m_type == SYM_TEMPORARY256);
	assert(src2->m_type == SYM_CONSTANT);

	uint32 offset = (src2->m_valueLow & 0x7F) / 8;

	m_assembler.MovdquVo(resultRegister, MakeTemporary256SymbolElementAddress(src1, offset));
	m_assembler.MovapsVo(MakeVariable128SymbolAddress(dst), resultRegister);
}

void CCodeGen_x86::Emit_MergeTo256_MemVarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type == SYM_TEMPORARY256);

	auto src1Register = CX86Assembler::xMM0;
	auto src2Register = CX86Assembler::xMM1;

	//TODO: Improve this to write out registers directly to temporary's memory space
	//instead of passing by temporary registers

	m_assembler.MovapsVo(src1Register, MakeVariable128SymbolAddress(src1));
	m_assembler.MovapsVo(src2Register, MakeVariable128SymbolAddress(src2));

	m_assembler.MovapsVo(MakeTemporary256SymbolElementAddress(dst, 0x00), src1Register);
	m_assembler.MovapsVo(MakeTemporary256SymbolElementAddress(dst, 0x10), src2Register);
}

#define MD_CONST_MATCHERS_SHIFT(MDOP_CST, MDOP) \
	{ MDOP_CST,				MATCH_REGISTER128,			MATCH_VARIABLE128,			MATCH_CONSTANT,			&CCodeGen_x86::Emit_Md_Shift_RegVarCst<MDOP>			}, \
	{ MDOP_CST,				MATCH_MEMORY128,			MATCH_VARIABLE128,			MATCH_CONSTANT,			&CCodeGen_x86::Emit_Md_Shift_MemVarCst<MDOP>			},

#define MD_CONST_MATCHERS_2OPS(MDOP_CST, MDOP) \
	{ MDOP_CST,				MATCH_REGISTER128,			MATCH_VARIABLE128,			MATCH_NIL,				&CCodeGen_x86::Emit_Md_RegVar<MDOP>						}, \
	{ MDOP_CST,				MATCH_MEMORY128,			MATCH_VARIABLE128,			MATCH_NIL,				&CCodeGen_x86::Emit_Md_MemVar<MDOP>						},

#define MD_CONST_MATCHERS_3OPS(MDOP_CST, MDOP) \
	{ MDOP_CST,				MATCH_REGISTER128,			MATCH_REGISTER128,			MATCH_REGISTER128,		&CCodeGen_x86::Emit_Md_RegRegReg<MDOP>					}, \
	{ MDOP_CST,				MATCH_REGISTER128,			MATCH_VARIABLE128,			MATCH_VARIABLE128,		&CCodeGen_x86::Emit_Md_RegVarVar<MDOP>					}, \
	{ MDOP_CST,				MATCH_MEMORY128,			MATCH_VARIABLE128,			MATCH_VARIABLE128,		&CCodeGen_x86::Emit_Md_MemVarVar<MDOP>					},

#define MD_CONST_MATCHERS_3OPS_REV(MDOP_CST, MDOP) \
	{ MDOP_CST,				MATCH_VARIABLE128,			MATCH_VARIABLE128,			MATCH_VARIABLE128,		&CCodeGen_x86::Emit_Md_VarVarVarRev<MDOP>				},

CCodeGen_x86::CONSTMATCHER CCodeGen_x86::g_mdConstMatchers[] = 
{
	MD_CONST_MATCHERS_3OPS(OP_MD_ADD_B,		MDOP_ADDB)
	MD_CONST_MATCHERS_3OPS(OP_MD_ADDUS_B,	MDOP_ADDUSB)

	MD_CONST_MATCHERS_3OPS(OP_MD_ADD_H,		MDOP_ADDH)
	MD_CONST_MATCHERS_3OPS(OP_MD_ADDSS_H,	MDOP_ADDSSH)

	MD_CONST_MATCHERS_3OPS(OP_MD_ADD_W,		MDOP_ADDW)

	{ OP_MD_ADDSS_W,			MATCH_VARIABLE128,			MATCH_VARIABLE128,			MATCH_VARIABLE128,		&CCodeGen_x86::Emit_Md_AddSSW_VarVarVar						},
	{ OP_MD_ADDUS_W,			MATCH_VARIABLE128,			MATCH_VARIABLE128,			MATCH_VARIABLE128,		&CCodeGen_x86::Emit_Md_AddUSW_VarVarVar						},

	MD_CONST_MATCHERS_3OPS(OP_MD_SUB_B,		MDOP_SUBB)
	MD_CONST_MATCHERS_3OPS(OP_MD_SUBSS_H,	MDOP_SUBSSH)

	MD_CONST_MATCHERS_3OPS(OP_MD_SUB_W,		MDOP_SUBW)

	MD_CONST_MATCHERS_3OPS(OP_MD_CMPEQ_B,	MDOP_CMPEQB)
	MD_CONST_MATCHERS_3OPS(OP_MD_CMPEQ_H,	MDOP_CMPEQH)
	MD_CONST_MATCHERS_3OPS(OP_MD_CMPEQ_W,	MDOP_CMPEQW)
	MD_CONST_MATCHERS_3OPS(OP_MD_CMPGT_H,	MDOP_CMPGTH)

	MD_CONST_MATCHERS_3OPS(OP_MD_MIN_H,		MDOP_MINH)
	MD_CONST_MATCHERS_3OPS(OP_MD_MIN_W,		MDOP_MINW)

	MD_CONST_MATCHERS_3OPS(OP_MD_MAX_H,		MDOP_MAXH)
	MD_CONST_MATCHERS_3OPS(OP_MD_MAX_W,		MDOP_MAXW)

	MD_CONST_MATCHERS_3OPS(OP_MD_AND,		MDOP_AND)
	MD_CONST_MATCHERS_3OPS(OP_MD_OR,		MDOP_OR)
	MD_CONST_MATCHERS_3OPS(OP_MD_XOR,		MDOP_XOR)

	MD_CONST_MATCHERS_SHIFT(OP_MD_SRLH,		MDOP_SRLH)
	MD_CONST_MATCHERS_SHIFT(OP_MD_SRAH,		MDOP_SRAH)
	MD_CONST_MATCHERS_SHIFT(OP_MD_SLLH,		MDOP_SLLH)

	MD_CONST_MATCHERS_SHIFT(OP_MD_SRLW,		MDOP_SRLW)
	MD_CONST_MATCHERS_SHIFT(OP_MD_SRAW,		MDOP_SRAW)
	MD_CONST_MATCHERS_SHIFT(OP_MD_SLLW,		MDOP_SLLW)

	{ OP_MD_SRL256,				MATCH_VARIABLE128,			MATCH_MEMORY256,			MATCH_VARIABLE,			&CCodeGen_x86::Emit_Md_Srl256_VarMemVar						},
	{ OP_MD_SRL256,				MATCH_VARIABLE128,			MATCH_MEMORY256,			MATCH_CONSTANT,			&CCodeGen_x86::Emit_Md_Srl256_VarMemCst						},

	{ OP_MD_EXPAND,				MATCH_REGISTER128,			MATCH_REGISTER,				MATCH_NIL,				&CCodeGen_x86::Emit_Md_Expand_RegReg						},
	{ OP_MD_EXPAND,				MATCH_REGISTER128,			MATCH_MEMORY,				MATCH_NIL,				&CCodeGen_x86::Emit_Md_Expand_RegMem						},
	{ OP_MD_EXPAND,				MATCH_REGISTER128,			MATCH_CONSTANT,				MATCH_NIL,				&CCodeGen_x86::Emit_Md_Expand_RegCst						},
	{ OP_MD_EXPAND,				MATCH_MEMORY128,			MATCH_REGISTER,				MATCH_NIL,				&CCodeGen_x86::Emit_Md_Expand_MemReg						},
	{ OP_MD_EXPAND,				MATCH_MEMORY128,			MATCH_MEMORY,				MATCH_NIL,				&CCodeGen_x86::Emit_Md_Expand_MemMem						},
	{ OP_MD_EXPAND,				MATCH_MEMORY128,			MATCH_CONSTANT,				MATCH_NIL,				&CCodeGen_x86::Emit_Md_Expand_MemCst						},

	{ OP_MD_PACK_HB,			MATCH_VARIABLE128,			MATCH_VARIABLE128,			MATCH_VARIABLE128,		&CCodeGen_x86::Emit_Md_PackHB_VarVarVar,					},
	{ OP_MD_PACK_WH,			MATCH_VARIABLE128,			MATCH_VARIABLE128,			MATCH_VARIABLE128,		&CCodeGen_x86::Emit_Md_PackWH_VarVarVar,					},

	MD_CONST_MATCHERS_3OPS_REV(OP_MD_UNPACK_LOWER_BH, MDOP_UNPACK_LOWER_BH)
	MD_CONST_MATCHERS_3OPS_REV(OP_MD_UNPACK_LOWER_HW, MDOP_UNPACK_LOWER_HW)
	MD_CONST_MATCHERS_3OPS_REV(OP_MD_UNPACK_LOWER_WD, MDOP_UNPACK_LOWER_WD)

	MD_CONST_MATCHERS_3OPS_REV(OP_MD_UNPACK_UPPER_BH, MDOP_UNPACK_UPPER_BH)
	MD_CONST_MATCHERS_3OPS_REV(OP_MD_UNPACK_UPPER_WD, MDOP_UNPACK_UPPER_WD)

	MD_CONST_MATCHERS_3OPS(OP_MD_ADD_S, MDOP_ADDS)
	MD_CONST_MATCHERS_3OPS(OP_MD_SUB_S, MDOP_SUBS)
	MD_CONST_MATCHERS_3OPS(OP_MD_MUL_S, MDOP_MULS)
	MD_CONST_MATCHERS_3OPS(OP_MD_DIV_S, MDOP_DIVS)

	{ OP_MD_ABS_S,				MATCH_REGISTER128,			MATCH_REGISTER128,			MATCH_NIL,				&CCodeGen_x86::Emit_Md_Abs_RegReg							},
	{ OP_MD_ABS_S,				MATCH_MEMORY128,			MATCH_VARIABLE128,			MATCH_NIL,				&CCodeGen_x86::Emit_Md_Abs_MemVar							},

	MD_CONST_MATCHERS_3OPS(OP_MD_MIN_S, MDOP_MINS)
	MD_CONST_MATCHERS_3OPS(OP_MD_MAX_S, MDOP_MAXS)

	{ OP_MD_NOT,				MATCH_VARIABLE128,			MATCH_VARIABLE128,			MATCH_NIL,				&CCodeGen_x86::Emit_Md_Not_VarVar							},

	{ OP_MD_ISNEGATIVE,			MATCH_REGISTER,				MATCH_VARIABLE128,			MATCH_NIL,				&CCodeGen_x86::Emit_Md_GetFlag_RegVar<MDOP_ISNEGATIVE>		},
	{ OP_MD_ISNEGATIVE,			MATCH_MEMORY,				MATCH_VARIABLE128,			MATCH_NIL,				&CCodeGen_x86::Emit_Md_GetFlag_MemVar<MDOP_ISNEGATIVE>		},

	{ OP_MD_ISZERO,				MATCH_REGISTER,				MATCH_VARIABLE128,			MATCH_NIL,				&CCodeGen_x86::Emit_Md_GetFlag_RegVar<MDOP_ISZERO>			},
	{ OP_MD_ISZERO,				MATCH_MEMORY,				MATCH_VARIABLE128,			MATCH_NIL,				&CCodeGen_x86::Emit_Md_GetFlag_MemVar<MDOP_ISZERO>			},

	MD_CONST_MATCHERS_2OPS(OP_MD_TOWORD_TRUNCATE,	MDOP_TOWORD_TRUNCATE)
	MD_CONST_MATCHERS_2OPS(OP_MD_TOSINGLE,			MDOP_TOSINGLE)

	{ OP_MOV,					MATCH_REGISTER128,			MATCH_VARIABLE128,			MATCH_NIL,				&CCodeGen_x86::Emit_Md_Mov_RegVar,							},
	{ OP_MOV,					MATCH_MEMORY128,			MATCH_REGISTER128,			MATCH_NIL,				&CCodeGen_x86::Emit_Md_Mov_MemReg							},
	{ OP_MOV,					MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_NIL,				&CCodeGen_x86::Emit_Md_Mov_MemMem							},
	{ OP_MD_MOV_MASKED,			MATCH_VARIABLE128,			MATCH_VARIABLE128,			MATCH_CONSTANT,			&CCodeGen_x86::Emit_Md_MovMasked_VarVarCst					},

	{ OP_MERGETO256,			MATCH_MEMORY256,			MATCH_VARIABLE128,			MATCH_VARIABLE128,		&CCodeGen_x86::Emit_MergeTo256_MemVarVar					},

	{ OP_MOV,					MATCH_NIL,					MATCH_NIL,					MATCH_NIL,				NULL														},
};
