#include <cassert>
#include "X86Assembler.h"

void CX86Assembler::WriteVex(VEX_PREFIX prefix, XMMREGISTER& dst, XMMREGISTER src1, const CAddress& src2)
{
	assert(src2.nIsExtendedSib == 0);

	bool isExtendedR = (dst > xMM7);
	if(isExtendedR)
	{
		dst = static_cast<XMMREGISTER>(dst & 7);
	}

	if(src2.nIsExtendedModRM)
	{
		//Three byte VEX
		uint8 b1 = 0;
		b1 |= 1; //Opcode Map 1
		b1 |= (src2.nIsExtendedModRM ? 0 : 1) << 5;
		b1 |= (src2.nIsExtendedSib ? 0 : 1) << 6;
		b1 |= (isExtendedR ? 0 : 1) << 7;
		
		uint8 b2 = 0;
		b2 |= prefix;
		b2 |= (~static_cast<uint8>(src1) & 0xF) << 3;

		WriteByte(0xC4);
		WriteByte(b1);
		WriteByte(b2);
	}
	else
	{
		//Two byte VEX
		uint8 b1 = 0;
		b1 |= prefix;
		b1 |= (~static_cast<uint8>(src1) & 0xF) << 3;
		b1 |= (isExtendedR ? 0 : 1) << 7;

		WriteByte(0xC5);
		WriteByte(b1);
	}
}

void CX86Assembler::WriteVexVoOp(VEX_PREFIX prefix, uint8 op, XMMREGISTER dst, XMMREGISTER src1, const CAddress& src2)
{
	WriteVex(prefix, dst, src1, src2);
	WriteByte(op);
	CAddress newAddress(src2);
	newAddress.ModRm.nFnReg = dst;
	newAddress.Write(&m_tmpStream);
}

void CX86Assembler::VmovdqaVo(XMMREGISTER dst, const CAddress& src)
{
	WriteVexVoOp(VEX_PREFIX_66, 0x6F, dst, CX86Assembler::xMM0, src);
}

void CX86Assembler::VmovdqaVo(const CAddress& dst, XMMREGISTER src)
{
	WriteVexVoOp(VEX_PREFIX_66, 0x7F, src, CX86Assembler::xMM0, dst);
}

void CX86Assembler::VmovapsVo(XMMREGISTER dst, const CAddress& src)
{
	WriteVexVoOp(VEX_PREFIX_NONE, 0x28, dst, CX86Assembler::xMM0, src);
}

void CX86Assembler::VmovapsVo(const CAddress& dst, XMMREGISTER src)
{
	WriteVexVoOp(VEX_PREFIX_NONE, 0x29, src, CX86Assembler::xMM0, dst);
}

void CX86Assembler::VpaddbVo(XMMREGISTER dst, XMMREGISTER src1, const CAddress& src2)
{
	WriteVexVoOp(VEX_PREFIX_66, 0xFC, dst, src1, src2);
}

void CX86Assembler::VpaddwVo(XMMREGISTER dst, XMMREGISTER src1, const CAddress& src2)
{
	WriteVexVoOp(VEX_PREFIX_66, 0xFD, dst, src1, src2);
}

void CX86Assembler::VpadddVo(XMMREGISTER dst, XMMREGISTER src1, const CAddress& src2)
{
	WriteVexVoOp(VEX_PREFIX_66, 0xFE, dst, src1, src2);
}

void CX86Assembler::VpaddsbVo(XMMREGISTER dst, XMMREGISTER src1, const CAddress& src2)
{
	WriteVexVoOp(VEX_PREFIX_66, 0xEC, dst, src1, src2);
}

void CX86Assembler::VpaddswVo(XMMREGISTER dst, XMMREGISTER src1, const CAddress& src2)
{
	WriteVexVoOp(VEX_PREFIX_66, 0xED, dst, src1, src2);
}

void CX86Assembler::VpaddusbVo(XMMREGISTER dst, XMMREGISTER src1, const CAddress& src2)
{
	WriteVexVoOp(VEX_PREFIX_66, 0xDC, dst, src1, src2);
}

void CX86Assembler::VpadduswVo(XMMREGISTER dst, XMMREGISTER src1, const CAddress& src2)
{
	WriteVexVoOp(VEX_PREFIX_66, 0xDD, dst, src1, src2);
}

void CX86Assembler::VpsubbVo(XMMREGISTER dst, XMMREGISTER src1, const CAddress& src2)
{
	WriteVexVoOp(VEX_PREFIX_66, 0xF8, dst, src1, src2);
}

void CX86Assembler::VpsubwVo(XMMREGISTER dst, XMMREGISTER src1, const CAddress& src2)
{
	WriteVexVoOp(VEX_PREFIX_66, 0xF9, dst, src1, src2);
}

void CX86Assembler::VpsubdVo(XMMREGISTER dst, XMMREGISTER src1, const CAddress& src2)
{
	WriteVexVoOp(VEX_PREFIX_66, 0xFA, dst, src1, src2);
}

void CX86Assembler::VpsubswVo(XMMREGISTER dst, XMMREGISTER src1, const CAddress& src2)
{
	WriteVexVoOp(VEX_PREFIX_66, 0xE9, dst, src1, src2);
}

void CX86Assembler::VpsubusbVo(XMMREGISTER dst, XMMREGISTER src1, const CAddress& src2)
{
	WriteVexVoOp(VEX_PREFIX_66, 0xD8, dst, src1, src2);
}

void CX86Assembler::VpsubuswVo(XMMREGISTER dst, XMMREGISTER src1, const CAddress& src2)
{
	WriteVexVoOp(VEX_PREFIX_66, 0xD9, dst, src1, src2);
}

void CX86Assembler::VpandVo(XMMREGISTER dst, XMMREGISTER src1, const CAddress& src2)
{
	WriteVexVoOp(VEX_PREFIX_66, 0xDB, dst, src1, src2);
}

void CX86Assembler::VporVo(XMMREGISTER dst, XMMREGISTER src1, const CAddress& src2)
{
	WriteVexVoOp(VEX_PREFIX_66, 0xEB, dst, src1, src2);
}

void CX86Assembler::VpxorVo(XMMREGISTER dst, XMMREGISTER src1, const CAddress& src2)
{
	WriteVexVoOp(VEX_PREFIX_66, 0xEF, dst, src1, src2);
}

void CX86Assembler::VpslldVo(XMMREGISTER dst, XMMREGISTER src, uint8 amount)
{
	auto address = MakeXmmRegisterAddress(dst);
	address.ModRm.nFnReg = 0x06;
	WriteVex(VEX_PREFIX_66, dst, src, address);
	WriteByte(0x72);
	address.Write(&m_tmpStream);
	WriteByte(amount);
}

void CX86Assembler::VpsrldVo(XMMREGISTER dst, XMMREGISTER src, uint8 amount)
{
	auto address = MakeXmmRegisterAddress(dst);
	address.ModRm.nFnReg = 0x02;
	WriteVex(VEX_PREFIX_66, dst, src, address);
	WriteByte(0x72);
	address.Write(&m_tmpStream);
	WriteByte(amount);
}

void CX86Assembler::VpsradVo(XMMREGISTER dst, XMMREGISTER src, uint8 amount)
{
	auto address = MakeXmmRegisterAddress(dst);
	address.ModRm.nFnReg = 0x04;
	WriteVex(VEX_PREFIX_66, dst, src, address);
	WriteByte(0x72);
	address.Write(&m_tmpStream);
	WriteByte(amount);
}

void CX86Assembler::VpcmpeqdVo(XMMREGISTER dst, XMMREGISTER src1, const CAddress& src2)
{
	WriteVexVoOp(VEX_PREFIX_66, 0x76, dst, src1, src2);
}

void CX86Assembler::VpcmpgtdVo(XMMREGISTER dst, XMMREGISTER src1, const CAddress& src2)
{
	WriteVexVoOp(VEX_PREFIX_66, 0x66, dst, src1, src2);
}
