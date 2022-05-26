#pragma once

#include "Types.h"
#include "Stream.h"
#include "MemStream.h"
#include "Literal128.h"
#include <map>
#include <vector>

class CX86Assembler
{
public:
	enum REGISTER
	{
		rAX = 0,
		rCX,
		rDX,
		rBX,
		rSP,
		rBP,
		rSI,
		rDI,
		r8,
		r9,
		r10,
		r11,
		r12,
		r13,
		r14,
		r15,
	};

	enum BYTEREGISTER
	{
		bAL = 0,
		bCL,
		bDL,
		bBL,
		bAH,
		bCH,
		bDH,
		bBH
	};

	enum XMMREGISTER
	{
		xMM0 = 0,
		xMM1,
		xMM2,
		xMM3,
		xMM4,
		xMM5,
		xMM6,
		xMM7,
		xMM8,
		xMM9,
		xMM10,
		xMM11,
		xMM12,
		xMM13,
		xMM14,
		xMM15,
	};

	typedef unsigned int LABEL;
	typedef unsigned int LITERAL128ID;

	class CAddress
	{
	public:
						CAddress();

		bool			nIsExtendedModRM;
		bool			nIsExtendedSib;
		bool			usesLegacyByteRegister;

		union MODRMBYTE
		{
			struct
			{
				unsigned int nRM : 3;
				unsigned int nFnReg : 3;
				unsigned int nMod : 2;
			};
			uint8 nByte;
		};

		union SIB
		{
			struct
			{
				unsigned int base : 3;
				unsigned int index : 3;
				unsigned int scale : 2;
			};
			uint8 byteValue;
		};

		MODRMBYTE		ModRm;
		SIB				sib;
		uint32			nOffset;
		LITERAL128ID	literal128Id = 0;

		bool			HasSib() const;
		bool			NeedsExtendedByteAddress() const;
		void			Write(Framework::CStream*);
	};

	virtual									~CX86Assembler() = default;

	void									Begin();
	void									End();

	void									SetStream(Framework::CStream*);

	static CAddress							MakeRegisterAddress(REGISTER);
	static CAddress							MakeXmmRegisterAddress(XMMREGISTER);
	static CAddress							MakeByteRegisterAddress(BYTEREGISTER);
	static CAddress							MakeIndRegAddress(REGISTER);
	static CAddress							MakeIndRegOffAddress(REGISTER, uint32);
	static CAddress							MakeBaseIndexScaleAddress(REGISTER, REGISTER, uint8);
	static CAddress							MakeLiteral128Address(LITERAL128ID);
	static CAddress							MakeBaseOffIndexScaleAddress(REGISTER, uint32, REGISTER, uint8);

	static bool								HasByteRegister(REGISTER);
	static BYTEREGISTER						GetByteRegister(REGISTER);

	static unsigned int						GetMinimumConstantSize(uint32);
	static unsigned int						GetMinimumConstantSize64(uint64);

	LABEL									CreateLabel();
	void									MarkLabel(LABEL, int32 = 0);
	uint32									GetLabelOffset(LABEL) const;

	LITERAL128ID							CreateLiteral128(const LITERAL128&);
	void									ResolveLiteralReferences();

	void									AdcEd(REGISTER, const CAddress&);
	void									AdcId(const CAddress&, uint32);
	void									AddEd(REGISTER, const CAddress&);
	void									AddEq(REGISTER, const CAddress&);
	void									AddId(const CAddress&, uint32);
	void									AddIq(const CAddress&, uint64);
	void									AndEd(REGISTER, const CAddress&);
	void									AndEq(REGISTER, const CAddress&);
	void									AndIb(const CAddress&, uint8);
	void									AndId(const CAddress&, uint32);
	void									AndIq(const CAddress&, uint64);
	void									BsrEd(REGISTER, const CAddress&);
	void									CallEd(const CAddress&);
	void									CmovsEd(REGISTER, const CAddress&);
	void									CmovnsEd(REGISTER, const CAddress&);
	void									CmpEd(REGISTER, const CAddress&);
	void									CmpEq(REGISTER, const CAddress&);
	void									CmpIb(const CAddress&, uint8);
	void									CmpId(const CAddress&, uint32);
	void									CmpIq(const CAddress&, uint64);
	void									Cdq();
	void									DivEd(const CAddress&);
	void									IdivEd(const CAddress&);
	void									ImulEw(const CAddress&);
	void									ImulEd(const CAddress&);
	void									Int3();
	void									JbJx(LABEL);
	void									JbeJx(LABEL);
	void									JnbJx(LABEL);
	void									JzJx(LABEL);
	void									JnlJx(LABEL);
	void									JnleJx(LABEL);
	void									JlJx(LABEL);
	void									JleJx(LABEL);
	void									JmpEd(const CAddress&);
	void									JmpJx(LABEL);
	void									JnzJx(LABEL);
	void									JnbeJx(LABEL);
	void									JnoJx(LABEL);
	void									JnsJx(LABEL);
	void									LeaGd(REGISTER, const CAddress&);
	void									LeaGq(REGISTER, const CAddress&);
	void									MovEw(REGISTER, const CAddress&);
	void									MovEd(REGISTER, const CAddress&);
	void									MovEq(REGISTER, const CAddress&);
	void									MovGb(const CAddress&, BYTEREGISTER);
	void									MovGb(const CAddress&, REGISTER);
	void									MovGw(const CAddress&, REGISTER);
	void									MovGd(const CAddress&, REGISTER);
	void									MovGq(const CAddress&, REGISTER);
	void									MovId(REGISTER, uint32);
	void									MovIq(REGISTER, uint64);
	void									MovIb(const CAddress&, uint8);
	void									MovIw(const CAddress&, uint16);
	void									MovId(const CAddress&, uint32);
	void									MovsxEb(REGISTER, const CAddress&);
	void									MovsxEw(REGISTER, const CAddress&);
	void									MovzxEb(REGISTER, const CAddress&);
	void									MovzxEw(REGISTER, const CAddress&);
	void									MulEd(const CAddress&);
	void									NegEd(const CAddress&);
	void									Nop();
	void									NotEd(const CAddress&);
	void									OrEd(REGISTER, const CAddress&);
	void									OrId(const CAddress&, uint32);
	void									Pop(REGISTER);
	void									Push(REGISTER);
	void									PushEd(const CAddress&);
	void									PushId(uint32);
	void									RclEd(const CAddress&, uint8);
	void									RepMovsb();
	void									Ret();
	void									SarEd(const CAddress&);
	void									SarEd(const CAddress&, uint8);
	void									SarEq(const CAddress&);
	void									SarEq(const CAddress&, uint8);
	void									SbbEd(REGISTER, const CAddress&);
	void									SbbId(const CAddress&, uint32);
	void									SetaEb(const CAddress&);
	void									SetaeEb(const CAddress&);
	void									SetbEb(const CAddress&);
	void									SetbeEb(const CAddress&);
	void									SeteEb(const CAddress&);
	void									SetneEb(const CAddress&);
	void									SetlEb(const CAddress&);
	void									SetleEb(const CAddress&);
	void									SetgEb(const CAddress&);
	void									ShrEd(const CAddress&);
	void									ShrEd(const CAddress&, uint8);
	void									ShrEq(const CAddress&);
	void									ShrEq(const CAddress&, uint8);
	void									ShrdEd(const CAddress&, REGISTER);
	void									ShrdEd(const CAddress&, REGISTER, uint8);
	void									ShlEd(const CAddress&);
	void									ShlEd(const CAddress&, uint8);
	void									ShlEq(const CAddress&);
	void									ShlEq(const CAddress&, uint8);
	void									ShldEd(const CAddress&, REGISTER);
	void									ShldEd(const CAddress&, REGISTER, uint8);
	void									SubEd(REGISTER, const CAddress&);
	void									SubEq(REGISTER, const CAddress&);
	void									SubId(const CAddress&, uint32);
	void									SubIq(const CAddress&, uint64);
	void									TestEb(BYTEREGISTER, const CAddress&);
	void									TestEd(REGISTER, const CAddress&);
	void									TestEq(REGISTER, const CAddress&);
	void									XorEd(REGISTER, const CAddress&);
	void									XorId(const CAddress&, uint32);
	void									XorGd(const CAddress&, REGISTER);
	void									XorGq(const CAddress&, REGISTER);

	//FPU
	void									FldEd(const CAddress&);
	void									FildEd(const CAddress&);
	void									FstpEd(const CAddress&);
	void									FistpEd(const CAddress&);
	void									FisttpEd(const CAddress&);
	void									FaddpSt(uint8);
	void									FsubpSt(uint8);
	void									FmulpSt(uint8);
	void									FdivpSt(uint8);
	void									Fwait();
	void									Fsin();
	void									FnstcwEw(const CAddress&);
	void									FldcwEw(const CAddress&);

	//SSE

	enum SSE_CMP_TYPE
	{
		SSE_CMP_EQ = 0,
		SSE_CMP_LT = 1,
		SSE_CMP_LE = 2,
		SSE_CMP_UNORD = 3,
		SSE_CMP_NEQ = 4,
		SSE_CMP_NLT = 5,
		SSE_CMP_NLE = 6,
		SSE_CMP_ORD = 7,
	};

	void									MovdVo(XMMREGISTER, const CAddress&);
	void									MovdVo(const CAddress&, XMMREGISTER);
	void									MovqVo(XMMREGISTER, const CAddress&);
	void									MovdqaVo(XMMREGISTER, const CAddress&);
	void									MovdqaVo(const CAddress&, XMMREGISTER);
	void									MovdquVo(XMMREGISTER, const CAddress&);
	void									MovdquVo(const CAddress&, XMMREGISTER);
	void									MovapsVo(const CAddress&, XMMREGISTER);
	void									MovapsVo(XMMREGISTER, const CAddress&);
	void									PackssdwVo(XMMREGISTER, const CAddress&);
	void									PackuswbVo(XMMREGISTER, const CAddress&);

	void									PaddbVo(XMMREGISTER, const CAddress&);
	void									PaddsbVo(XMMREGISTER, const CAddress&);
	void									PaddusbVo(XMMREGISTER, const CAddress&);
	void									PaddwVo(XMMREGISTER, const CAddress&);
	void									PaddswVo(XMMREGISTER, const CAddress&);
	void									PadduswVo(XMMREGISTER, const CAddress&);
	void									PadddVo(XMMREGISTER, const CAddress&);

	void									PandVo(XMMREGISTER, const CAddress&);
	void									PandnVo(XMMREGISTER, const CAddress&);
	void									PcmpeqbVo(XMMREGISTER, const CAddress&);
	void									PcmpeqwVo(XMMREGISTER, const CAddress&);
	void									PcmpeqdVo(XMMREGISTER, const CAddress&);
	void									PcmpgtbVo(XMMREGISTER, const CAddress&);
	void									PcmpgtwVo(XMMREGISTER, const CAddress&);
	void									PcmpgtdVo(XMMREGISTER, const CAddress&);
	void									PmaxswVo(XMMREGISTER, const CAddress&);
	void									PmaxsdVo(XMMREGISTER, const CAddress&);
	void									PminswVo(XMMREGISTER, const CAddress&);
	void									PminsdVo(XMMREGISTER, const CAddress&);
	void									PmovmskbVo(REGISTER, XMMREGISTER);
	void									PorVo(XMMREGISTER, const CAddress&);
	void									PshufbVo(XMMREGISTER, const CAddress&);
	void									PshufdVo(XMMREGISTER, const CAddress&, uint8);
	void									PsllwVo(XMMREGISTER, uint8);
	void									PslldVo(XMMREGISTER, uint8);
	void									PsrawVo(XMMREGISTER, uint8);
	void									PsradVo(XMMREGISTER, uint8);
	void									PsrlwVo(XMMREGISTER, uint8);
	void									PsrldVo(XMMREGISTER, uint8);

	void									PsubbVo(XMMREGISTER, const CAddress&);
	void									PsubusbVo(XMMREGISTER, const CAddress&);
	void									PsubwVo(XMMREGISTER, const CAddress&);
	void									PsubswVo(XMMREGISTER, const CAddress&);
	void									PsubuswVo(XMMREGISTER, const CAddress&);
	void									PsubdVo(XMMREGISTER, const CAddress&);

	void									PunpcklbwVo(XMMREGISTER, const CAddress&);
	void									PunpcklwdVo(XMMREGISTER, const CAddress&);
	void									PunpckldqVo(XMMREGISTER, const CAddress&);
	void									PunpckhbwVo(XMMREGISTER, const CAddress&);
	void									PunpckhwdVo(XMMREGISTER, const CAddress&);
	void									PunpckhdqVo(XMMREGISTER, const CAddress&);
	void									PxorVo(XMMREGISTER, const CAddress&);

	void									MovssEd(const CAddress&, XMMREGISTER);
	void									MovssEd(XMMREGISTER, const CAddress&);
	void									AddssEd(XMMREGISTER, const CAddress&);
	void									SubssEd(XMMREGISTER, const CAddress&);
	void									MaxssEd(XMMREGISTER, const CAddress&);
	void									MinssEd(XMMREGISTER, const CAddress&);
	void									MulssEd(XMMREGISTER, const CAddress&);
	void									DivssEd(XMMREGISTER, const CAddress&);
	void									RcpssEd(XMMREGISTER, const CAddress&);
	void									RsqrtssEd(XMMREGISTER, const CAddress&);
	void									SqrtssEd(XMMREGISTER, const CAddress&);
	void									CmpssEd(XMMREGISTER, const CAddress&, SSE_CMP_TYPE);
	void									CmppsVo(XMMREGISTER, const CAddress&, SSE_CMP_TYPE);
	void									CmpltpsVo(XMMREGISTER, const CAddress&);
	void									CmpgtpsVo(XMMREGISTER, const CAddress&);
	void									Cvtsi2ssEd(XMMREGISTER, const CAddress&);
	void									Cvttss2siEd(REGISTER, const CAddress&);
	void									Cvtdq2psVo(XMMREGISTER, const CAddress&);
	void									Cvttps2dqVo(XMMREGISTER, const CAddress&);

	void									AddpsVo(XMMREGISTER, const CAddress&);
	void									BlendpsVo(XMMREGISTER, const CAddress&, uint8);
	void									DivpsVo(XMMREGISTER, const CAddress&);
	void									MaxpsVo(XMMREGISTER, const CAddress&);
	void									MinpsVo(XMMREGISTER, const CAddress&);
	void									MulpsVo(XMMREGISTER, const CAddress&);
	void									SubpsVo(XMMREGISTER, const CAddress&);
	void									ShufpsVo(XMMREGISTER, const CAddress&, uint8);

	//AVX
	void									VmovdVo(XMMREGISTER, const CAddress&);
	void									VmovdVo(const CAddress&, XMMREGISTER);

	void									VmovssEd(XMMREGISTER, const CAddress&);
	void									VmovssEd(const CAddress&, XMMREGISTER);

	void									VaddssEd(XMMREGISTER, XMMREGISTER, const CAddress&);
	void									VsubssEd(XMMREGISTER, XMMREGISTER, const CAddress&);
	void									VmulssEd(XMMREGISTER, XMMREGISTER, const CAddress&);
	void									VdivssEd(XMMREGISTER, XMMREGISTER, const CAddress&);
	void									VmaxssEd(XMMREGISTER, XMMREGISTER, const CAddress&);
	void									VminssEd(XMMREGISTER, XMMREGISTER, const CAddress&);
	
	void									VcmpssEd(XMMREGISTER, XMMREGISTER, const CAddress&, SSE_CMP_TYPE);

	void									VsqrtssEd(XMMREGISTER, XMMREGISTER, const CAddress&);

	void									Vcvtsi2ssEd(XMMREGISTER, const CAddress&);
	void									Vcvttss2siEd(REGISTER, const CAddress&);

	void									VmovdqaVo(XMMREGISTER, const CAddress&);
	void									VmovdqaVo(const CAddress&, XMMREGISTER);
	void									VmovdquVo(XMMREGISTER, const CAddress&);
	void									VmovapsVo(XMMREGISTER, const CAddress&);
	void									VmovapsVo(const CAddress&, XMMREGISTER);

	void									VpaddbVo(XMMREGISTER, XMMREGISTER, const CAddress&);
	void									VpaddwVo(XMMREGISTER, XMMREGISTER, const CAddress&);
	void									VpadddVo(XMMREGISTER, XMMREGISTER, const CAddress&);

	void									VpaddsbVo(XMMREGISTER, XMMREGISTER, const CAddress&);
	void									VpaddswVo(XMMREGISTER, XMMREGISTER, const CAddress&);

	void									VpaddusbVo(XMMREGISTER, XMMREGISTER, const CAddress&);
	void									VpadduswVo(XMMREGISTER, XMMREGISTER, const CAddress&);

	void									VpsubbVo(XMMREGISTER, XMMREGISTER, const CAddress&);
	void									VpsubwVo(XMMREGISTER, XMMREGISTER, const CAddress&);
	void									VpsubdVo(XMMREGISTER, XMMREGISTER, const CAddress&);

	void									VpsubswVo(XMMREGISTER, XMMREGISTER, const CAddress&);
	
	void									VpsubusbVo(XMMREGISTER, XMMREGISTER, const CAddress&);
	void									VpsubuswVo(XMMREGISTER, XMMREGISTER, const CAddress&);

	void									VpandVo(XMMREGISTER, XMMREGISTER, const CAddress&);
	void									VporVo(XMMREGISTER, XMMREGISTER, const CAddress&);
	void									VpxorVo(XMMREGISTER, XMMREGISTER, const CAddress&);

	void									VpsllwVo(XMMREGISTER, XMMREGISTER, uint8);
	void									VpsrlwVo(XMMREGISTER, XMMREGISTER, uint8);
	void									VpsrawVo(XMMREGISTER, XMMREGISTER, uint8);

	void									VpslldVo(XMMREGISTER, XMMREGISTER, uint8);
	void									VpsrldVo(XMMREGISTER, XMMREGISTER, uint8);
	void									VpsradVo(XMMREGISTER, XMMREGISTER, uint8);

	void									VpcmpeqbVo(XMMREGISTER, XMMREGISTER, const CAddress&);
	void									VpcmpeqwVo(XMMREGISTER, XMMREGISTER, const CAddress&);
	void									VpcmpeqdVo(XMMREGISTER, XMMREGISTER, const CAddress&);

	void									VpcmpgtbVo(XMMREGISTER, XMMREGISTER, const CAddress&);
	void									VpcmpgtwVo(XMMREGISTER, XMMREGISTER, const CAddress&);
	void									VpcmpgtdVo(XMMREGISTER, XMMREGISTER, const CAddress&);

	void									VpmaxswVo(XMMREGISTER, XMMREGISTER, const CAddress&);
	void									VpmaxsdVo(XMMREGISTER, XMMREGISTER, const CAddress&);
	void									VpminswVo(XMMREGISTER, XMMREGISTER, const CAddress&);
	void									VpminsdVo(XMMREGISTER, XMMREGISTER, const CAddress&);

	void									VpackssdwVo(XMMREGISTER, XMMREGISTER, const CAddress&);
	void									VpackuswbVo(XMMREGISTER, XMMREGISTER, const CAddress&);

	void									VpunpcklbwVo(XMMREGISTER, XMMREGISTER, const CAddress&);
	void									VpunpcklwdVo(XMMREGISTER, XMMREGISTER, const CAddress&);
	void									VpunpckldqVo(XMMREGISTER, XMMREGISTER, const CAddress&);
	void									VpunpckhbwVo(XMMREGISTER, XMMREGISTER, const CAddress&);
	void									VpunpckhwdVo(XMMREGISTER, XMMREGISTER, const CAddress&);
	void									VpunpckhdqVo(XMMREGISTER, XMMREGISTER, const CAddress&);

	void									VpshufbVo(XMMREGISTER, XMMREGISTER, const CAddress&);
	void									VpmovmskbVo(REGISTER, XMMREGISTER);

	void									VandpsVo(XMMREGISTER, XMMREGISTER, const CAddress&);
	void									VaddpsVo(XMMREGISTER, XMMREGISTER, const CAddress&);
	void									VsubpsVo(XMMREGISTER, XMMREGISTER, const CAddress&);
	void									VmulpsVo(XMMREGISTER, XMMREGISTER, const CAddress&);
	void									VdivpsVo(XMMREGISTER, XMMREGISTER, const CAddress&);

	void									VcmpltpsVo(XMMREGISTER, XMMREGISTER, const CAddress&);
	void									VcmpgtpsVo(XMMREGISTER, XMMREGISTER, const CAddress&);

	void									VxorpsVo(XMMREGISTER, XMMREGISTER, const CAddress&);

	void									VminpsVo(XMMREGISTER, XMMREGISTER, const CAddress&);
	void									VmaxpsVo(XMMREGISTER, XMMREGISTER, const CAddress&);

	void									Vcvtdq2psVo(XMMREGISTER, const CAddress&);
	void									Vcvttps2dqVo(XMMREGISTER, const CAddress&);

	void									VcmppsVo(XMMREGISTER, XMMREGISTER, const CAddress&, SSE_CMP_TYPE);

	void									VblendpsVo(XMMREGISTER, XMMREGISTER, const CAddress&, uint8);
	void									VshufpsVo(XMMREGISTER, XMMREGISTER, const CAddress&, uint8);

private:
	enum JMP_TYPE
	{
		JMP_O	= 0,
		JMP_NO	= 1,
		JMP_B	= 2,
		JMP_NB	= 3,
		JMP_Z	= 4,
		JMP_NZ	= 5,
		JMP_BE	= 6,
		JMP_NBE	= 7,

		JMP_S	= 8,
		JMP_NS	= 9,
		JMP_P	= 10,
		JMP_NP	= 11,
		JMP_L	= 12,
		JMP_NL	= 13,
		JMP_LE	= 14,
		JMP_NLE	= 15,

		JMP_ALWAYS,
	};

	enum JMP_LENGTH
	{
		JMP_NOTSET,
		JMP_NEAR,
		JMP_FAR
	};

	enum VEX_OPCODE_MAP : uint8
	{
		VEX_OPCODE_MAP_NONE = 0x01,
		VEX_OPCODE_MAP_66 = 0x11,
		VEX_OPCODE_MAP_66_38 = 0x12,
		VEX_OPCODE_MAP_66_3A = 0x13,
		VEX_OPCODE_MAP_F3 = 0x21,
		VEX_OPCODE_MAP_F2 = 0x31
	};

	struct LABELREF
	{
		LABELREF()
			: label(0)
			, offset(0)
			, type(JMP_ALWAYS)
			, length(JMP_NOTSET)
		{

		}

		LABEL		label;
		uint32		offset;
		JMP_TYPE	type;
		JMP_LENGTH	length;
	};

	typedef std::vector<LABELREF> LabelRefArray;

	struct LITERAL128REF
	{
		uint32 offset = 0;
		LITERAL128 value = LITERAL128(0, 0);
	};
	typedef std::map<LITERAL128ID, LITERAL128REF> Literal128Refs;

	struct LABELINFO
	{
		LABELINFO()
			: start(0)
			, size(0)
			, projectedStart(0)
		{

		}

		uint32			start;
		uint32			size;
		uint32			projectedStart;
		LabelRefArray	labelRefs;
		Literal128Refs	literal128Refs;
	};

	typedef std::map<LABEL, LABELINFO> LabelMap;
	typedef std::vector<LABEL> LabelArray;
	typedef std::vector<uint8> ByteArray;

	void									WriteRexByte(bool, const CAddress&);
	void									WriteRexByte(bool, const CAddress&, REGISTER&, bool = false);
	void									WriteVex(VEX_OPCODE_MAP, XMMREGISTER&, XMMREGISTER, const CAddress&);
	void									WriteEbOp_0F(uint8, uint8, const CAddress&);
	void									WriteEbGbOp(uint8, bool, const CAddress&, REGISTER);
	void									WriteEbGbOp(uint8, bool, const CAddress&, BYTEREGISTER);
	void									WriteEbGvOp0F(uint8, bool, const CAddress&, REGISTER);
	void									WriteEvOp(uint8, uint8, bool, const CAddress&);
	void									WriteEvGvOp(uint8, bool, const CAddress&, REGISTER);
	void									WriteEvGvOp0F(uint8, bool, const CAddress&, REGISTER);
	void									WriteEvIb(uint8, const CAddress&, uint8);
	void									WriteEvId(uint8, const CAddress&, uint32);
	void									WriteEvIq(uint8, const CAddress&, uint64);
	void									WriteEdVdOp(uint8, const CAddress&, XMMREGISTER);
	void									WriteEdVdOp_0F(uint8, const CAddress&, XMMREGISTER);
	void									WriteEdVdOp_66_0F(uint8, const CAddress&, XMMREGISTER);
	void									WriteEdVdOp_66_0F_64b(uint8, const CAddress&, XMMREGISTER);
	void									WriteEdVdOp_66_0F_38(uint8, const CAddress&, XMMREGISTER);
	void									WriteEdVdOp_66_0F_3A(uint8, const CAddress&, XMMREGISTER);
	void									WriteEdVdOp_F3_0F(uint8, const CAddress&, XMMREGISTER);
	void									WriteVrOp_66_0F(uint8, uint8, XMMREGISTER);
	void									WriteVexVoOp(VEX_OPCODE_MAP, uint8, XMMREGISTER, XMMREGISTER, const CAddress&);
	void									WriteVexShiftVoOp(uint8, uint8, XMMREGISTER, XMMREGISTER, uint8);
	void									WriteStOp(uint8, uint8, uint8);

	void									CreateLabelReference(LABEL, JMP_TYPE);

	void									IncrementJumpOffsetsLocal(LABELINFO&, LabelRefArray::iterator, unsigned int);
	void									IncrementJumpOffsets(LabelArray::const_iterator, unsigned int);

	static unsigned int						GetJumpSize(JMP_TYPE, JMP_LENGTH);
	static void								WriteJump(Framework::CStream*, JMP_TYPE, JMP_LENGTH, uint32);

	void									WriteByte(uint8);
	void									WriteWord(uint16);
	void									WriteDWord(uint32);

	LabelMap								m_labels;
	LabelArray								m_labelOrder;
	LABEL									m_nextLabelId = 1;
	LITERAL128ID							m_nextLiteral128Id = 1;
	LABELINFO*								m_currentLabel = nullptr;
	Framework::CStream*						m_outputStream = nullptr;
	Framework::CMemStream					m_tmpStream;
	ByteArray								m_copyBuffer;
};
