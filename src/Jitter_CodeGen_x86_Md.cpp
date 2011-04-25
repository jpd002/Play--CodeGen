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
	assert(((symbol->m_stackLocation + m_stackLevel) & 0xF) == 0);
	return CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, symbol->m_stackLocation + m_stackLevel + (elementIdx * 4));
}

CX86Assembler::CAddress CCodeGen_x86::MakeTemporary256SymbolElementAddress(CSymbol* symbol, unsigned int elementIdx)
{
	assert(symbol->m_type == SYM_TEMPORARY256);
	assert(((symbol->m_stackLocation + m_stackLevel) & 0x1F) == 0);
	return CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, symbol->m_stackLocation + m_stackLevel + elementIdx);
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
void CCodeGen_x86::Emit_Md_MemMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	((m_assembler).*(MDOP::OpVo()))(CX86Assembler::xMM0, MakeMemory128SymbolAddress(src1));
	m_assembler.MovapsVo(MakeMemory128SymbolAddress(dst), CX86Assembler::xMM0);
}

template <typename MDOP>
void CCodeGen_x86::Emit_Md_MemMemMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	m_assembler.MovapsVo(CX86Assembler::xMM0, MakeMemory128SymbolAddress(src1));
	((m_assembler).*(MDOP::OpVo()))(CX86Assembler::xMM0, MakeMemory128SymbolAddress(src2));
	m_assembler.MovapsVo(MakeMemory128SymbolAddress(dst), CX86Assembler::xMM0);
}

template <typename MDOP>
void CCodeGen_x86::Emit_Md_MemMemMemRev(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	m_assembler.MovapsVo(CX86Assembler::xMM0, MakeMemory128SymbolAddress(src2));
	((m_assembler).*(MDOP::OpVo()))(CX86Assembler::xMM0, MakeMemory128SymbolAddress(src1));
	m_assembler.MovapsVo(MakeMemory128SymbolAddress(dst), CX86Assembler::xMM0);
}

template <typename MDOPSHIFT> 
void CCodeGen_x86::Emit_Md_Shift_MemMemCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_CONSTANT);

	CX86Assembler::XMMREGISTER tmpReg = CX86Assembler::xMM0;

	m_assembler.MovapsVo(tmpReg, MakeMemory128SymbolAddress(src1));
	((m_assembler).*(MDOPSHIFT::OpVo()))(tmpReg, static_cast<uint8>(src2->m_valueLow));
	m_assembler.MovapsVo(MakeMemory128SymbolAddress(dst), tmpReg);
}

void CCodeGen_x86::Emit_Md_AddSSW_MemMemMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	CX86Assembler::REGISTER overflowReg = CX86Assembler::rAX;
	CX86Assembler::REGISTER underflowReg = CX86Assembler::rDX;
	CX86Assembler::REGISTER resultRegister = CX86Assembler::rCX;

	//Prepare registers
	m_assembler.MovId(overflowReg, 0x7FFFFFFF);
	m_assembler.MovId(underflowReg, 0x80000000);

	for(int i = 0; i < 4; i++)
	{
		CX86Assembler::LABEL doneLabel = m_assembler.CreateLabel();
		m_assembler.MovEd(resultRegister, MakeMemory128SymbolElementAddress(src1, i));
		m_assembler.AddEd(resultRegister, MakeMemory128SymbolElementAddress(src2, i));
		m_assembler.JnoJx(doneLabel);
		m_assembler.CmovsEd(resultRegister, CX86Assembler::MakeRegisterAddress(overflowReg));
		m_assembler.CmovnsEd(resultRegister, CX86Assembler::MakeRegisterAddress(underflowReg));
		m_assembler.MarkLabel(doneLabel);
		m_assembler.MovGd(MakeMemory128SymbolElementAddress(dst, i), resultRegister);
	}
}

void CCodeGen_x86::Emit_Md_AddUSW_MemMemMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	CX86Assembler::REGISTER overflowReg = CX86Assembler::rAX;
	CX86Assembler::REGISTER resultRegister = CX86Assembler::rCX;

	//Prepare registers
	m_assembler.MovId(overflowReg, 0xFFFFFFFF);

	for(int i = 0; i < 4; i++)
	{
		CX86Assembler::LABEL doneLabel = m_assembler.CreateLabel();
		m_assembler.MovEd(resultRegister, MakeMemory128SymbolElementAddress(src1, i));
		m_assembler.AddEd(resultRegister, MakeMemory128SymbolElementAddress(src2, i));
		m_assembler.JnbJx(doneLabel);

		//overflow:
		m_assembler.MovGd(CX86Assembler::MakeRegisterAddress(resultRegister), overflowReg);
		
		//done:
		m_assembler.MarkLabel(doneLabel);
		m_assembler.MovGd(MakeMemory128SymbolElementAddress(dst, i), resultRegister);
	}
}

void CCodeGen_x86::Emit_Md_PackHB_MemMemMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	CX86Assembler::XMMREGISTER resultRegister = CX86Assembler::xMM0;
	CX86Assembler::XMMREGISTER tempRegister = CX86Assembler::xMM1;
	CX86Assembler::XMMREGISTER maskRegister = CX86Assembler::xMM2;

	m_assembler.MovapsVo(resultRegister, MakeMemory128SymbolAddress(src2));
	m_assembler.MovapsVo(tempRegister, MakeMemory128SymbolAddress(src1));

	//Generate mask (0x00FF x8)
	m_assembler.PcmpeqdVo(maskRegister,	CX86Assembler::MakeXmmRegisterAddress(maskRegister));
	m_assembler.PsrlwVo(maskRegister, 0x08);
    
	//Mask both operands
	m_assembler.PandVo(resultRegister, CX86Assembler::MakeXmmRegisterAddress(maskRegister));
	m_assembler.PandVo(tempRegister, CX86Assembler::MakeXmmRegisterAddress(maskRegister));

	//Pack
	m_assembler.PackuswbVo(resultRegister, CX86Assembler::MakeXmmRegisterAddress(tempRegister));

	m_assembler.MovapsVo(MakeMemory128SymbolAddress(dst), resultRegister);
}

void CCodeGen_x86::Emit_Md_PackWH_MemMemMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	CX86Assembler::XMMREGISTER resultRegister = CX86Assembler::xMM0;
	CX86Assembler::XMMREGISTER tempRegister = CX86Assembler::xMM1;

	m_assembler.MovapsVo(resultRegister, MakeMemory128SymbolAddress(src2));
	m_assembler.MovapsVo(tempRegister, MakeMemory128SymbolAddress(src1));

	//Sign extend the lower half word of our registers
	m_assembler.PslldVo(resultRegister, 0x10);
	m_assembler.PsradVo(resultRegister, 0x10);

	m_assembler.PslldVo(tempRegister, 0x10);
	m_assembler.PsradVo(tempRegister, 0x10);

	//Pack
	m_assembler.PackssdwVo(resultRegister, CX86Assembler::MakeXmmRegisterAddress(tempRegister));

	m_assembler.MovapsVo(MakeMemory128SymbolAddress(dst), resultRegister);
}

void CCodeGen_x86::Emit_Md_Not_RelTmp(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type  == SYM_RELATIVE128);
	assert(src1->m_type == SYM_TEMPORARY128);

	m_assembler.MovapsVo(CX86Assembler::xMM0, MakeMemory128SymbolAddress(src1));
	m_assembler.PcmpeqdVo(CX86Assembler::xMM1, CX86Assembler::MakeXmmRegisterAddress(CX86Assembler::xMM1));
	m_assembler.PxorVo(CX86Assembler::xMM0, CX86Assembler::MakeXmmRegisterAddress(CX86Assembler::xMM1));

	m_assembler.MovapsVo(MakeMemory128SymbolAddress(dst), CX86Assembler::xMM0);
}

void CCodeGen_x86::Emit_Md_MovMasked_MemMemCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_CONSTANT);

	uint8 mask = static_cast<uint8>(src2->m_valueLow);
	assert(mask != 0);

	uint8 shuffle[4] = { 0x00, 0xE5, 0xEA, 0xFF };
	CX86Assembler::XMMREGISTER tmpReg = CX86Assembler::xMM0;

	m_assembler.MovapsVo(tmpReg, MakeMemory128SymbolAddress(src1));
	for(unsigned int i = 0; i < 4; i++)
	{
		if(mask & (1 << i))
		{
			if(i != 0)
			{
				m_assembler.ShufpsVo(tmpReg, CX86Assembler::MakeXmmRegisterAddress(tmpReg), shuffle[i]);
			}

			m_assembler.MovssEd(MakeMemory128SymbolElementAddress(dst, i), tmpReg);
		}
	}
}

void CCodeGen_x86::Emit_Md_Mov_MemMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	CX86Assembler::XMMREGISTER resultRegister = CX86Assembler::xMM0;

	m_assembler.MovapsVo(resultRegister, MakeMemory128SymbolAddress(src1));
	m_assembler.MovapsVo(MakeMemory128SymbolAddress(dst), resultRegister);
}

void CCodeGen_x86::Emit_Md_Abs_MemMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	CX86Assembler::XMMREGISTER maskRegister = CX86Assembler::xMM0;
	CX86Assembler::XMMREGISTER resultRegister = CX86Assembler::xMM1;
    
	m_assembler.MovapsVo(resultRegister, MakeMemory128SymbolAddress(src1));
	m_assembler.PcmpeqdVo(maskRegister, CX86Assembler::MakeXmmRegisterAddress(maskRegister));
	m_assembler.PsrldVo(maskRegister, 1);
	m_assembler.PandVo(resultRegister, CX86Assembler::MakeXmmRegisterAddress(maskRegister));
	m_assembler.MovapsVo(MakeMemory128SymbolAddress(dst), resultRegister);
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

void CCodeGen_x86::Emit_Md_IsNegative_RegMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	Emit_Md_IsNegative(m_registers[dst->m_valueLow], MakeMemory128SymbolAddress(src1));
}

void CCodeGen_x86::Emit_Md_IsNegative_MemMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	CX86Assembler::REGISTER tmpRegister = CX86Assembler::rAX;
	Emit_Md_IsNegative(tmpRegister, MakeMemory128SymbolAddress(src1));
	m_assembler.MovGd(MakeMemorySymbolAddress(dst), tmpRegister);
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

void CCodeGen_x86::Emit_Md_IsZero_RegMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	Emit_Md_IsZero(m_registers[dst->m_valueLow], MakeMemory128SymbolAddress(src1));
}

void CCodeGen_x86::Emit_Md_IsZero_MemMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	CX86Assembler::REGISTER tmpRegister = CX86Assembler::rAX;
	Emit_Md_IsZero(tmpRegister, MakeMemory128SymbolAddress(src1));
	m_assembler.MovGd(MakeMemorySymbolAddress(dst), tmpRegister);
}

void CCodeGen_x86::Emit_Md_Expand_MemReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	assert(src1->m_type == SYM_REGISTER);

	CX86Assembler::XMMREGISTER resultRegister = CX86Assembler::xMM0;

	m_assembler.MovdVo(resultRegister, CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
    m_assembler.ShufpsVo(resultRegister, CX86Assembler::MakeXmmRegisterAddress(resultRegister), 0x00);
	m_assembler.MovapsVo(MakeMemory128SymbolAddress(dst), resultRegister);
}

void CCodeGen_x86::Emit_Md_Expand_MemMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	CX86Assembler::XMMREGISTER resultRegister = CX86Assembler::xMM0;

	m_assembler.MovssEd(resultRegister, MakeMemorySymbolAddress(src1));
    m_assembler.ShufpsVo(resultRegister, CX86Assembler::MakeXmmRegisterAddress(resultRegister), 0x00);
	m_assembler.MovapsVo(MakeMemory128SymbolAddress(dst), resultRegister);
}

void CCodeGen_x86::Emit_Md_Expand_MemCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	assert(src1->m_type == SYM_CONSTANT);

	CX86Assembler::REGISTER cstRegister = CX86Assembler::rAX;
	CX86Assembler::XMMREGISTER resultRegister = CX86Assembler::xMM0;

	m_assembler.MovId(cstRegister, src1->m_valueLow);
	m_assembler.MovdVo(resultRegister, CX86Assembler::MakeRegisterAddress(cstRegister));
    m_assembler.ShufpsVo(resultRegister, CX86Assembler::MakeXmmRegisterAddress(resultRegister), 0x00);
	m_assembler.MovapsVo(MakeMemory128SymbolAddress(dst), resultRegister);
}

void CCodeGen_x86::Emit_Md_Srl256_MemMem(CSymbol* dst, CSymbol* src1, const CX86Assembler::CAddress& offsetAddress)
{
	CX86Assembler::REGISTER offsetRegister = CX86Assembler::rAX;
	CX86Assembler::XMMREGISTER resultRegister = CX86Assembler::xMM0;

	assert(src1->m_type == SYM_TEMPORARY256);

	m_assembler.MovEd(offsetRegister, offsetAddress);
	m_assembler.AndId(CX86Assembler::MakeRegisterAddress(offsetRegister), 0x7F);
	m_assembler.ShrEd(CX86Assembler::MakeRegisterAddress(offsetRegister), 3);
	m_assembler.AddId(CX86Assembler::MakeRegisterAddress(offsetRegister), src1->m_stackLocation + m_stackLevel);

	m_assembler.MovdquVo(resultRegister, CX86Assembler::MakeBaseIndexScaleAddress(CX86Assembler::rSP, offsetRegister, 1));
	m_assembler.MovapsVo(MakeMemory128SymbolAddress(dst), resultRegister);
}

void CCodeGen_x86::Emit_Md_Srl256_MemMemReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_REGISTER);

	Emit_Md_Srl256_MemMem(dst, src1, CX86Assembler::MakeRegisterAddress(m_registers[src2->m_valueLow]));
}

void CCodeGen_x86::Emit_Md_Srl256_MemMemMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	Emit_Md_Srl256_MemMem(dst, src1, MakeMemorySymbolAddress(src2));
}

void CCodeGen_x86::Emit_Md_Srl256_MemMemCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	CX86Assembler::XMMREGISTER resultRegister = CX86Assembler::xMM0;

	assert(src1->m_type == SYM_TEMPORARY256);
	assert(src2->m_type == SYM_CONSTANT);

	uint32 offset = (src2->m_valueLow & 0x7F) / 8;

	m_assembler.MovdquVo(resultRegister, MakeTemporary256SymbolElementAddress(src1, offset));
	m_assembler.MovapsVo(MakeMemory128SymbolAddress(dst), resultRegister);
}

void CCodeGen_x86::Emit_MergeTo256_MemMemMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type == SYM_TEMPORARY256);

	CX86Assembler::XMMREGISTER src1Register = CX86Assembler::xMM0;
	CX86Assembler::XMMREGISTER src2Register = CX86Assembler::xMM1;

	m_assembler.MovapsVo(src1Register, MakeMemory128SymbolAddress(src1));
	m_assembler.MovapsVo(src2Register, MakeMemory128SymbolAddress(src2));

	m_assembler.MovapsVo(MakeTemporary256SymbolElementAddress(dst, 0x00), src1Register);
	m_assembler.MovapsVo(MakeTemporary256SymbolElementAddress(dst, 0x10), src2Register);
}

CCodeGen_x86::CONSTMATCHER CCodeGen_x86::g_mdConstMatchers[] = 
{
	{ OP_MD_ADD_B,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_x86::Emit_Md_MemMemMem<MDOP_ADDB>					},
	{ OP_MD_ADD_H,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_x86::Emit_Md_MemMemMem<MDOP_ADDH>					},
	{ OP_MD_ADD_W,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_x86::Emit_Md_MemMemMem<MDOP_ADDW>					},
	{ OP_MD_ADDSS_W,			MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_x86::Emit_Md_AddSSW_MemMemMem						},
	{ OP_MD_ADDUS_W,			MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_x86::Emit_Md_AddUSW_MemMemMem						},

	{ OP_MD_SUB_B,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_x86::Emit_Md_MemMemMem<MDOP_SUBB>					},
	{ OP_MD_SUB_W,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_x86::Emit_Md_MemMemMem<MDOP_SUBW>					},

	{ OP_MD_CMPEQ_W,			MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_x86::Emit_Md_MemMemMem<MDOP_CMPEQW>				},
	{ OP_MD_CMPGT_H,			MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_x86::Emit_Md_MemMemMem<MDOP_CMPGTH>				},

	{ OP_MD_MIN_H,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_x86::Emit_Md_MemMemMem<MDOP_MINH>					},
	{ OP_MD_MAX_H,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_x86::Emit_Md_MemMemMem<MDOP_MAXH>					},

	{ OP_MD_AND,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_x86::Emit_Md_MemMemMem<MDOP_AND>					},
	{ OP_MD_OR,					MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_x86::Emit_Md_MemMemMem<MDOP_OR>					},
	{ OP_MD_XOR,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_x86::Emit_Md_MemMemMem<MDOP_XOR>					},

	{ OP_MD_SRLH,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_CONSTANT,			&CCodeGen_x86::Emit_Md_Shift_MemMemCst<MDOP_SRLH>			},
	{ OP_MD_SRAH,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_CONSTANT,			&CCodeGen_x86::Emit_Md_Shift_MemMemCst<MDOP_SRAH>			},
	{ OP_MD_SLLH,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_CONSTANT,			&CCodeGen_x86::Emit_Md_Shift_MemMemCst<MDOP_SLLH>			},

	{ OP_MD_SRLW,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_CONSTANT,			&CCodeGen_x86::Emit_Md_Shift_MemMemCst<MDOP_SRLW>			},
	{ OP_MD_SRAW,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_CONSTANT,			&CCodeGen_x86::Emit_Md_Shift_MemMemCst<MDOP_SRAW>			},
	{ OP_MD_SLLW,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_CONSTANT,			&CCodeGen_x86::Emit_Md_Shift_MemMemCst<MDOP_SLLW>			},

	{ OP_MD_SRL256,				MATCH_MEMORY128,			MATCH_MEMORY256,			MATCH_REGISTER,			&CCodeGen_x86::Emit_Md_Srl256_MemMemReg						},
	{ OP_MD_SRL256,				MATCH_MEMORY128,			MATCH_MEMORY256,			MATCH_MEMORY,			&CCodeGen_x86::Emit_Md_Srl256_MemMemMem						},
	{ OP_MD_SRL256,				MATCH_MEMORY128,			MATCH_MEMORY256,			MATCH_CONSTANT,			&CCodeGen_x86::Emit_Md_Srl256_MemMemCst						},

	{ OP_MD_EXPAND,				MATCH_MEMORY128,			MATCH_REGISTER,				MATCH_NIL,				&CCodeGen_x86::Emit_Md_Expand_MemReg						},
	{ OP_MD_EXPAND,				MATCH_MEMORY128,			MATCH_MEMORY,				MATCH_NIL,				&CCodeGen_x86::Emit_Md_Expand_MemMem						},
	{ OP_MD_EXPAND,				MATCH_MEMORY128,			MATCH_CONSTANT,				MATCH_NIL,				&CCodeGen_x86::Emit_Md_Expand_MemCst						},
	{ OP_MD_PACK_HB,			MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_x86::Emit_Md_PackHB_MemMemMem,					},
	{ OP_MD_PACK_WH,			MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_x86::Emit_Md_PackWH_MemMemMem,					},

	{ OP_MD_UNPACK_LOWER_BH,	MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_x86::Emit_Md_MemMemMemRev<MDOP_UNPACK_LOWER_BH>	},
	{ OP_MD_UNPACK_LOWER_HW,	MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_x86::Emit_Md_MemMemMemRev<MDOP_UNPACK_LOWER_HW>	},
	{ OP_MD_UNPACK_LOWER_WD,	MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_x86::Emit_Md_MemMemMemRev<MDOP_UNPACK_LOWER_WD>	},

	{ OP_MD_UNPACK_UPPER_BH,	MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_x86::Emit_Md_MemMemMemRev<MDOP_UNPACK_UPPER_BH>	},
	{ OP_MD_UNPACK_UPPER_WD,	MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_x86::Emit_Md_MemMemMemRev<MDOP_UNPACK_UPPER_WD>	},

	{ OP_MD_ADD_S,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_x86::Emit_Md_MemMemMem<MDOP_ADDS>					},
	{ OP_MD_SUB_S,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_x86::Emit_Md_MemMemMem<MDOP_SUBS>					},
	{ OP_MD_MUL_S,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_x86::Emit_Md_MemMemMem<MDOP_MULS>					},
	{ OP_MD_DIV_S,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_x86::Emit_Md_MemMemMem<MDOP_DIVS>					},

	{ OP_MD_ABS_S,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_NIL,				&CCodeGen_x86::Emit_Md_Abs_MemMem							},
	{ OP_MD_MIN_S,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_x86::Emit_Md_MemMemMem<MDOP_MINS>					},
	{ OP_MD_MAX_S,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_x86::Emit_Md_MemMemMem<MDOP_MAXS>					},

	{ OP_MD_NOT,				MATCH_RELATIVE128,			MATCH_TEMPORARY128,			MATCH_NIL,				&CCodeGen_x86::Emit_Md_Not_RelTmp							},

	{ OP_MD_ISNEGATIVE,			MATCH_REGISTER,				MATCH_MEMORY128,			MATCH_NIL,				&CCodeGen_x86::Emit_Md_IsNegative_RegMem					},
	{ OP_MD_ISNEGATIVE,			MATCH_MEMORY,				MATCH_MEMORY128,			MATCH_NIL,				&CCodeGen_x86::Emit_Md_IsNegative_MemMem					},

	{ OP_MD_ISZERO,				MATCH_REGISTER,				MATCH_MEMORY128,			MATCH_NIL,				&CCodeGen_x86::Emit_Md_IsZero_RegMem						},
	{ OP_MD_ISZERO,				MATCH_MEMORY,				MATCH_MEMORY128,			MATCH_NIL,				&CCodeGen_x86::Emit_Md_IsZero_MemMem						},

	{ OP_MD_TOWORD_TRUNCATE,	MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_NIL,				&CCodeGen_x86::Emit_Md_MemMem<MDOP_TOWORD_TRUNCATE>			},
	{ OP_MD_TOSINGLE,			MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_NIL,				&CCodeGen_x86::Emit_Md_MemMem<MDOP_TOSINGLE>				},

	{ OP_MOV,					MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_NIL,				&CCodeGen_x86::Emit_Md_Mov_MemMem							},
	{ OP_MD_MOV_MASKED,			MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_CONSTANT,			&CCodeGen_x86::Emit_Md_MovMasked_MemMemCst					},

	{ OP_MERGETO256,			MATCH_MEMORY256,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_x86::Emit_MergeTo256_MemMemMem					},

	{ OP_MOV,					MATCH_NIL,					MATCH_NIL,					MATCH_NIL,				NULL														},
};
