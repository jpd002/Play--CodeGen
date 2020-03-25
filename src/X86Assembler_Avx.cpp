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

void CX86Assembler::VmovdqaVo(XMMREGISTER dst, const CAddress& src)
{
	WriteVex(VEX_PREFIX_66, dst, CX86Assembler::xMM0, src);
	WriteByte(0x6F);
	CAddress newAddress(src);
	newAddress.ModRm.nFnReg = dst;
	newAddress.Write(&m_tmpStream);
}

void CX86Assembler::VmovdqaVo(const CAddress& dst, XMMREGISTER src)
{
	WriteVex(VEX_PREFIX_66, src, CX86Assembler::xMM0, dst);
	WriteByte(0x7F);
	CAddress newAddress(dst);
	newAddress.ModRm.nFnReg = src;
	newAddress.Write(&m_tmpStream);
}

void CX86Assembler::VmovapsVo(XMMREGISTER dst, const CAddress& src)
{
	WriteVex(VEX_PREFIX_NONE, dst, CX86Assembler::xMM0, src);
	WriteByte(0x28);
	CAddress newAddress(src);
	newAddress.ModRm.nFnReg = dst;
	newAddress.Write(&m_tmpStream);
}

void CX86Assembler::VmovapsVo(const CAddress& dst, XMMREGISTER src)
{
	WriteVex(VEX_PREFIX_NONE, src, CX86Assembler::xMM0, dst);
	WriteByte(0x29);
	CAddress newAddress(dst);
	newAddress.ModRm.nFnReg = src;
	newAddress.Write(&m_tmpStream);
}

void CX86Assembler::VpaddbVo(XMMREGISTER dst, XMMREGISTER src1, const CAddress& src2)
{
	WriteVex(VEX_PREFIX_66, dst, src1, src2);
	WriteByte(0xFC);
	CAddress newAddress(src2);
	newAddress.ModRm.nFnReg = dst;
	newAddress.Write(&m_tmpStream);
}

void CX86Assembler::VpaddwVo(XMMREGISTER dst, XMMREGISTER src1, const CAddress& src2)
{
	WriteVex(VEX_PREFIX_66, dst, src1, src2);
	WriteByte(0xFD);
	CAddress newAddress(src2);
	newAddress.ModRm.nFnReg = dst;
	newAddress.Write(&m_tmpStream);
}

void CX86Assembler::VpadddVo(XMMREGISTER dst, XMMREGISTER src1, const CAddress& src2)
{
	WriteVex(VEX_PREFIX_66, dst, src1, src2);
	WriteByte(0xFE);
	CAddress newAddress(src2);
	newAddress.ModRm.nFnReg = dst;
	newAddress.Write(&m_tmpStream);
}

void CX86Assembler::VpaddsbVo(XMMREGISTER dst, XMMREGISTER src1, const CAddress& src2)
{
	WriteVex(VEX_PREFIX_66, dst, src1, src2);
	WriteByte(0xEC);
	CAddress newAddress(src2);
	newAddress.ModRm.nFnReg = dst;
	newAddress.Write(&m_tmpStream);
}

void CX86Assembler::VpaddswVo(XMMREGISTER dst, XMMREGISTER src1, const CAddress& src2)
{
	WriteVex(VEX_PREFIX_66, dst, src1, src2);
	WriteByte(0xED);
	CAddress newAddress(src2);
	newAddress.ModRm.nFnReg = dst;
	newAddress.Write(&m_tmpStream);
}

void CX86Assembler::VpaddusbVo(XMMREGISTER dst, XMMREGISTER src1, const CAddress& src2)
{
	WriteVex(VEX_PREFIX_66, dst, src1, src2);
	WriteByte(0xDC);
	CAddress newAddress(src2);
	newAddress.ModRm.nFnReg = dst;
	newAddress.Write(&m_tmpStream);
}

void CX86Assembler::VpadduswVo(XMMREGISTER dst, XMMREGISTER src1, const CAddress& src2)
{
	WriteVex(VEX_PREFIX_66, dst, src1, src2);
	WriteByte(0xDD);
	CAddress newAddress(src2);
	newAddress.ModRm.nFnReg = dst;
	newAddress.Write(&m_tmpStream);
}

void CX86Assembler::VpandVo(XMMREGISTER dst, XMMREGISTER src1, const CAddress& src2)
{
	WriteVex(VEX_PREFIX_66, dst, src1, src2);
	WriteByte(0xDB);
	CAddress newAddress(src2);
	newAddress.ModRm.nFnReg = dst;
	newAddress.Write(&m_tmpStream);
}

void CX86Assembler::VporVo(XMMREGISTER dst, XMMREGISTER src1, const CAddress& src2)
{
	WriteVex(VEX_PREFIX_66, dst, src1, src2);
	WriteByte(0xEB);
	CAddress newAddress(src2);
	newAddress.ModRm.nFnReg = dst;
	newAddress.Write(&m_tmpStream);
}

void CX86Assembler::VpxorVo(XMMREGISTER dst, XMMREGISTER src1, const CAddress& src2)
{
	WriteVex(VEX_PREFIX_66, dst, src1, src2);
	WriteByte(0xEF);
	CAddress newAddress(src2);
	newAddress.ModRm.nFnReg = dst;
	newAddress.Write(&m_tmpStream);
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
	WriteVex(VEX_PREFIX_66, dst, src1, src2);
	WriteByte(0x76);
	CAddress newAddress(src2);
	newAddress.ModRm.nFnReg = dst;
	newAddress.Write(&m_tmpStream);
}

void CX86Assembler::VpcmpgtdVo(XMMREGISTER dst, XMMREGISTER src1, const CAddress& src2)
{
	WriteVex(VEX_PREFIX_66, dst, src1, src2);
	WriteByte(0x66);
	CAddress newAddress(src2);
	newAddress.ModRm.nFnReg = dst;
	newAddress.Write(&m_tmpStream);
}
