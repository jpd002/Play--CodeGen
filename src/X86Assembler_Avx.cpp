#include <cassert>
#include "X86Assembler.h"

void CX86Assembler::WriteVex(VEX_PREFIX prefix, XMMREGISTER& dst, XMMREGISTER src1, const CAddress& src2)
{
	assert(src2.nIsExtendedModRM == 0);

	bool isExtendedR = (dst > xMM7);
	if(isExtendedR)
	{
		dst = static_cast<XMMREGISTER>(dst & 7);
	}

	WriteByte(0xC5);

	uint8 v = 0;
	v |= prefix;
	v |= (~static_cast<uint8>(src1) & 0xF) << 3;
	v |= (isExtendedR ? 0 : 1) << 7;

	WriteByte(v);
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

void CX86Assembler::VpadddVo(XMMREGISTER dst, XMMREGISTER src1, const CAddress& src2)
{
	WriteVex(VEX_PREFIX_66, dst, src1, src2);
	WriteByte(0xFE);
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

void CX86Assembler::VpcmpeqdVo(XMMREGISTER dst, XMMREGISTER src1, const CAddress& src2)
{
	WriteVex(VEX_PREFIX_66, dst, src1, src2);
	WriteByte(0x76);
	CAddress newAddress(src2);
	newAddress.ModRm.nFnReg = dst;
	newAddress.Write(&m_tmpStream);
}
