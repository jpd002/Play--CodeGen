#include <cassert>
#include "X86Assembler.h"

void CX86Assembler::VmovapsVo(XMMREGISTER dst, const CAddress& src)
{
	uint8 v = 0;
	v |= 0x00;  //no prefix
	v |= (~0 & 0xF) << 3;
	v |= (1) << 7;

	WriteByte(0xC5);
	WriteByte(v);
	WriteByte(0x28);
	CAddress newAddress(src);
	newAddress.ModRm.nFnReg = dst;
	newAddress.Write(&m_tmpStream);
}

void CX86Assembler::VmovapsVo(const CAddress& dst, XMMREGISTER src)
{
	uint8 v = 0;
	v |= 0x00;  //no prefix
	v |= (~0 & 0xF) << 3;
	v |= (1) << 7;

	WriteByte(0xC5);
	WriteByte(v);
	WriteByte(0x29);
	CAddress newAddress(dst);
	newAddress.ModRm.nFnReg = src;
	newAddress.Write(&m_tmpStream);
}

void CX86Assembler::VpadddVo(XMMREGISTER dst, XMMREGISTER src1, const CAddress& src2)
{
	uint8 v = 0;
	v |= 0x01;  //0x66 prefix
	v |= (~src1 & 0xF) << 3;
	v |= (1) << 7;

	WriteByte(0xC5);
	WriteByte(v);
	WriteByte(0xFE);
	CAddress newAddress(src2);
	newAddress.ModRm.nFnReg = dst;
	newAddress.Write(&m_tmpStream);
}
