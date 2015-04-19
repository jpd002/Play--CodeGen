#include <assert.h>
#include <stdexcept>
#include "ArmAssembler.h"

#define OPCODE_BKPT (0xE1200070)

CArmAssembler::CArmAssembler()
: m_nextLabelId(1)
, m_stream(NULL)
{

}

CArmAssembler::~CArmAssembler()
{
	
}

void CArmAssembler::SetStream(Framework::CStream* stream)
{
	m_stream = stream;
}

CArmAssembler::LdrAddress CArmAssembler::MakeImmediateLdrAddress(int32 immediate)
{
	LdrAddress result;
	memset(&result, 0, sizeof(result));
	result.isImmediate = true;
	if(immediate < 0)
	{
		result.isNegative = (immediate < 0);
		immediate = -immediate;
	}
	assert((immediate & ~0xFFF) == 0);
	result.immediate = static_cast<uint16>(immediate);
	return result;
}

CArmAssembler::ImmediateAluOperand CArmAssembler::MakeImmediateAluOperand(uint8 immediate, uint8 rotateAmount)
{
	ImmediateAluOperand operand;
	operand.immediate = immediate;
	operand.rotate = rotateAmount;
	return operand;
}

CArmAssembler::RegisterAluOperand CArmAssembler::MakeRegisterAluOperand(CArmAssembler::REGISTER registerId, const AluLdrShift& shift)
{
	RegisterAluOperand result;
	result.rm = registerId;
	result.shift = *reinterpret_cast<const uint8*>(&shift);
	return result;
}

CArmAssembler::AluLdrShift CArmAssembler::MakeConstantShift(SHIFT shiftType, uint8 amount)
{
	assert(!(shiftType == SHIFT_ASR && amount == 0));
	assert(!(shiftType == SHIFT_LSR && amount == 0));

	AluLdrShift result;
	result.typeBit = 0;
	result.type = shiftType;
	result.amount = amount;
	return result;
}

CArmAssembler::AluLdrShift CArmAssembler::MakeVariableShift(SHIFT shiftType, CArmAssembler::REGISTER registerId)
{
	AluLdrShift result;
	result.typeBit = 1;
	result.type = shiftType;
	result.amount = registerId << 1;
	return result;
}

CArmAssembler::LABEL CArmAssembler::CreateLabel()
{
	return m_nextLabelId++;
}

void CArmAssembler::ClearLabels()
{
	m_labels.clear();
}

void CArmAssembler::MarkLabel(LABEL label)
{
	m_labels[label] = static_cast<size_t>(m_stream->Tell());
}

void CArmAssembler::ResolveLabelReferences()
{
	for(const auto& labelReferencePair : m_labelReferences)
	{
		auto label(m_labels.find(labelReferencePair.first));
		if(label == m_labels.end())
		{
			throw std::runtime_error("Invalid label.");
		}
		size_t referencePos = labelReferencePair.second;
		size_t labelPos = label->second;
		int offset = static_cast<int>(labelPos - referencePos) / 4;
		offset -= 2;

		m_stream->Seek(referencePos, Framework::STREAM_SEEK_SET);
		m_stream->Write8(static_cast<uint8>(offset >> 0));
		m_stream->Write8(static_cast<uint8>(offset >> 8));
		m_stream->Write8(static_cast<uint8>(offset >> 16));
		m_stream->Seek(0, Framework::STREAM_SEEK_END);
	}
	m_labelReferences.clear();
}

void CArmAssembler::GenericAlu(ALU_OPCODE op, bool setFlags, REGISTER rd, REGISTER rn, REGISTER rm)
{
	InstructionAlu instruction;
	instruction.operand		= rm;
	instruction.rn			= rn;
	instruction.rd			= rd;
	instruction.setFlags	= setFlags ? 1 : 0;
	instruction.opcode		= op;
	instruction.immediate	= 0;
	instruction.condition	= CONDITION_AL;
	uint32 opcode = *reinterpret_cast<uint32*>(&instruction);
	WriteWord(opcode);
}

void CArmAssembler::GenericAlu(ALU_OPCODE op, bool setFlags, REGISTER rd, REGISTER rn, const ImmediateAluOperand& operand)
{
	InstructionAlu instruction;
	instruction.operand		= *reinterpret_cast<const unsigned int*>(&operand);
	instruction.rd			= rd;
	instruction.rn			= rn;
	instruction.setFlags	= setFlags ? 1 : 0;
	instruction.opcode		= op;
	instruction.immediate	= 1;
	instruction.condition	= CONDITION_AL;
	uint32 opcode = *reinterpret_cast<uint32*>(&instruction);
	WriteWord(opcode);
}

void CArmAssembler::CreateLabelReference(LABEL label)
{
	LABELREF reference = static_cast<size_t>(m_stream->Tell());
	m_labelReferences.insert(LabelReferenceMapType::value_type(label, reference));
}

void CArmAssembler::Adc(REGISTER rd, REGISTER rn, REGISTER rm)
{
	GenericAlu(ALU_OPCODE_ADC, false, rd, rn, rm);
}

void CArmAssembler::Add(REGISTER rd, REGISTER rn, REGISTER rm)
{
	GenericAlu(ALU_OPCODE_ADD, false, rd, rn, rm);
}

void CArmAssembler::Add(REGISTER rd, REGISTER rn, const ImmediateAluOperand& operand)
{
	GenericAlu(ALU_OPCODE_ADD, false, rd, rn, operand);
}

void CArmAssembler::Adds(REGISTER rd, REGISTER rn, REGISTER rm)
{
	GenericAlu(ALU_OPCODE_ADD, true, rd, rn, rm);
}

void CArmAssembler::And(REGISTER rd, REGISTER rn, REGISTER rm)
{
	InstructionAlu instruction;
	instruction.operand = rm;
	instruction.rn = rn;
	instruction.rd = rd;
	instruction.setFlags = 0;
	instruction.opcode = ALU_OPCODE_AND;
	instruction.immediate = 0;
	instruction.condition = CONDITION_AL;
	uint32 opcode = *reinterpret_cast<uint32*>(&instruction);
	WriteWord(opcode);
}

void CArmAssembler::And(REGISTER rd, REGISTER rn, const ImmediateAluOperand& operand)
{
	InstructionAlu instruction;
	instruction.operand = *reinterpret_cast<const unsigned int*>(&operand);
	instruction.rd = rd;
	instruction.rn = rn;
	instruction.setFlags = 0;
	instruction.opcode = ALU_OPCODE_AND;
	instruction.immediate = 1;
	instruction.condition = CONDITION_AL;
	uint32 opcode = *reinterpret_cast<uint32*>(&instruction);
	WriteWord(opcode);
}

void CArmAssembler::BCc(CONDITION condition, LABEL label)
{
	CreateLabelReference(label);
	uint32 opcode = (condition << 28) | (0x0A000000);
	WriteWord(opcode);
}

void CArmAssembler::Bic(REGISTER rd, REGISTER rn, const ImmediateAluOperand& operand)
{
	InstructionAlu instruction;
	instruction.operand = *reinterpret_cast<const unsigned int*>(&operand);
	instruction.rd = rd;
	instruction.rn = rn;
	instruction.setFlags = 0;
	instruction.opcode = ALU_OPCODE_BIC;
	instruction.immediate = 1;
	instruction.condition = CONDITION_AL;
	uint32 opcode = *reinterpret_cast<uint32*>(&instruction);
	WriteWord(opcode);
}

void CArmAssembler::Bx(REGISTER rn)
{
	uint32 opcode = 0;
	opcode = (CONDITION_AL << 28) | (0x12FFF10) | (rn);
	WriteWord(opcode);
}

void CArmAssembler::Blx(REGISTER rn)
{
	uint32 opcode = 0;
	opcode = (CONDITION_AL << 28) | (0x12FFF30) | (rn);
	WriteWord(opcode);
}

void CArmAssembler::Clz(REGISTER rd, REGISTER rm)
{
	uint32 opcode = 0x016F0F10;
	opcode |= CONDITION_AL << 28;
	opcode |= rm;
	opcode |= (rd << 12);
	WriteWord(opcode);
}

void CArmAssembler::Cmn(REGISTER rn, const ImmediateAluOperand& operand)
{
	InstructionAlu instruction;
	instruction.operand = *reinterpret_cast<const unsigned int*>(&operand);
	instruction.rn = rn;
	instruction.rd = 0;
	instruction.setFlags = 1;
	instruction.opcode = ALU_OPCODE_CMN;
	instruction.immediate = 1;
	instruction.condition = CONDITION_AL;
	uint32 opcode = *reinterpret_cast<uint32*>(&instruction);
	WriteWord(opcode);
}

void CArmAssembler::Cmp(REGISTER rn, REGISTER rm)
{
	InstructionAlu instruction;
	instruction.operand = rm;
	instruction.rn = rn;
	instruction.rd = 0;
	instruction.setFlags = 1;
	instruction.opcode = ALU_OPCODE_CMP;
	instruction.immediate = 0;
	instruction.condition = CONDITION_AL;
	uint32 opcode = *reinterpret_cast<uint32*>(&instruction);
	WriteWord(opcode);
}

void CArmAssembler::Cmp(REGISTER rn, const ImmediateAluOperand& operand)
{
	InstructionAlu instruction;
	instruction.operand = *reinterpret_cast<const unsigned int*>(&operand);
	instruction.rn = rn;
	instruction.rd = 0;
	instruction.setFlags = 1;
	instruction.opcode = ALU_OPCODE_CMP;
	instruction.immediate = 1;
	instruction.condition = CONDITION_AL;
	uint32 opcode = *reinterpret_cast<uint32*>(&instruction);
	WriteWord(opcode);
}

void CArmAssembler::Eor(REGISTER rd, REGISTER rn, REGISTER rm)
{
	InstructionAlu instruction;
	instruction.operand = rm;
	instruction.rn = rn;
	instruction.rd = rd;
	instruction.setFlags = 0;
	instruction.opcode = ALU_OPCODE_EOR;
	instruction.immediate = 0;
	instruction.condition = CONDITION_AL;
	uint32 opcode = *reinterpret_cast<uint32*>(&instruction);
	WriteWord(opcode);
}

void CArmAssembler::Eor(REGISTER rd, REGISTER rn, const ImmediateAluOperand& operand)
{
	InstructionAlu instruction;
	instruction.operand = *reinterpret_cast<const unsigned int*>(&operand);
	instruction.rd = rd;
	instruction.rn = rn;
	instruction.setFlags = 0;
	instruction.opcode = ALU_OPCODE_EOR;
	instruction.immediate = 1;
	instruction.condition = CONDITION_AL;
	uint32 opcode = *reinterpret_cast<uint32*>(&instruction);
	WriteWord(opcode);
}

void CArmAssembler::Ldmia(REGISTER rbase, uint16 regList)
{
	//20 - Load
	//21 - Writeback
	//22 - User bit
	//23 - Down/Up (0/1)
	//24 - Post/Pre (0/1)
	uint32 opcode = (CONDITION_AL << 28) | (4 << 25) | (1 << 23) | (1 << 21) | (1 << 20) | (static_cast<uint32>(rbase) << 16) | regList;
	WriteWord(opcode);
}

void CArmAssembler::Ldr(REGISTER rd, REGISTER rbase, const LdrAddress& address)
{
	uint32 opcode = 0;
	assert(address.isImmediate);
	assert(!address.isNegative);
	opcode = (CONDITION_AL << 28) | (1 << 26) | (1 << 24) | (1 << 23) | (1 << 20) | (static_cast<uint32>(rbase) << 16) | (static_cast<uint32>(rd) << 12) | (static_cast<uint32>(address.immediate));
	WriteWord(opcode);
}

void CArmAssembler::Mov(REGISTER rd, REGISTER rm)
{
	InstructionAlu instruction;
	instruction.operand = rm;
	instruction.rd = rd;
	instruction.setFlags = 0;
	instruction.opcode = ALU_OPCODE_MOV;
	instruction.immediate = 0;
	instruction.condition = CONDITION_AL;
	uint32 opcode = *reinterpret_cast<uint32*>(&instruction);
	WriteWord(opcode);
}

void CArmAssembler::Mov(REGISTER rd, const RegisterAluOperand& operand)
{
	InstructionAlu instruction;
	instruction.operand = *reinterpret_cast<const unsigned int*>(&operand);
	instruction.rd = rd;
	instruction.setFlags = 0;
	instruction.opcode = ALU_OPCODE_MOV;
	instruction.immediate = 0;
	instruction.condition = CONDITION_AL;
	uint32 opcode = *reinterpret_cast<uint32*>(&instruction);
	WriteWord(opcode);
}

void CArmAssembler::Mov(REGISTER rd, const ImmediateAluOperand& operand)
{
	MovCc(CONDITION_AL, rd, operand);
}

void CArmAssembler::MovCc(CONDITION condition, REGISTER rd, const ImmediateAluOperand& operand)
{
	InstructionAlu instruction;
	instruction.operand = *reinterpret_cast<const unsigned int*>(&operand);
	instruction.rd = rd;
	instruction.rn = 0;
	instruction.setFlags = 0;
	instruction.opcode = ALU_OPCODE_MOV;
	instruction.immediate = 1;
	instruction.condition = condition;
	uint32 opcode = *reinterpret_cast<uint32*>(&instruction);
	WriteWord(opcode);
}

void CArmAssembler::Movw(REGISTER rd, uint16 value)
{
	uint32 opcode = 0x03000000;
	opcode |= (CONDITION_AL << 28);
	opcode |= (value >> 12) << 16;
	opcode |= (rd << 12);
	opcode |= (value & 0xFFF);
	WriteWord(opcode);
}

void CArmAssembler::Movt(REGISTER rd, uint16 value)
{
	uint32 opcode = 0x03400000;
	opcode |= (CONDITION_AL << 28);
	opcode |= (value >> 12) << 16;
	opcode |= (rd << 12);
	opcode |= (value & 0xFFF);
	WriteWord(opcode);
}

void CArmAssembler::Mvn(REGISTER rd, REGISTER rm)
{
	InstructionAlu instruction;
	instruction.operand = rm;
	instruction.rd = rd;
	instruction.setFlags = 0;
	instruction.opcode = ALU_OPCODE_MVN;
	instruction.immediate = 0;
	instruction.condition = CONDITION_AL;
	uint32 opcode = *reinterpret_cast<uint32*>(&instruction);
	WriteWord(opcode);
}

void CArmAssembler::Mvn(REGISTER rd, const ImmediateAluOperand& operand)
{
	InstructionAlu instruction;
	instruction.operand = *reinterpret_cast<const unsigned int*>(&operand);
	instruction.rd = rd;
	instruction.rn = 0;
	instruction.setFlags = 0;
	instruction.opcode = ALU_OPCODE_MVN;
	instruction.immediate = 1;
	instruction.condition = CONDITION_AL;
	uint32 opcode = *reinterpret_cast<uint32*>(&instruction);
	WriteWord(opcode);	
}

void CArmAssembler::Or(REGISTER rd, REGISTER rn, REGISTER rm)
{
	InstructionAlu instruction;
	instruction.operand = rm;
	instruction.rn = rn;
	instruction.rd = rd;
	instruction.setFlags = 0;
	instruction.opcode = ALU_OPCODE_ORR;
	instruction.immediate = 0;
	instruction.condition = CONDITION_AL;
	uint32 opcode = *reinterpret_cast<uint32*>(&instruction);
	WriteWord(opcode);
}

void CArmAssembler::Or(REGISTER rd, REGISTER rn, const ImmediateAluOperand& operand)
{
	InstructionAlu instruction;
	instruction.operand = *reinterpret_cast<const unsigned int*>(&operand);
	instruction.rd = rd;
	instruction.rn = rn;
	instruction.setFlags = 0;
	instruction.opcode = ALU_OPCODE_ORR;
	instruction.immediate = 1;
	instruction.condition = CONDITION_AL;
	uint32 opcode = *reinterpret_cast<uint32*>(&instruction);
	WriteWord(opcode);
}

void CArmAssembler::Rsb(REGISTER rd, REGISTER rn, const ImmediateAluOperand& operand)
{
	InstructionAlu instruction;
	instruction.operand = *reinterpret_cast<const unsigned int*>(&operand);
	instruction.rd = rd;
	instruction.rn = rn;
	instruction.setFlags = 0;
	instruction.opcode = ALU_OPCODE_RSB;
	instruction.immediate = 1;
	instruction.condition = CONDITION_AL;
	uint32 opcode = *reinterpret_cast<uint32*>(&instruction);
	WriteWord(opcode);
}

void CArmAssembler::Sbc(REGISTER rd, REGISTER rn, REGISTER rm)
{
	GenericAlu(ALU_OPCODE_SBC, false, rd, rn, rm);
}

void CArmAssembler::Smull(REGISTER rdLow, REGISTER rdHigh, REGISTER rn, REGISTER rm)
{
	uint32 opcode = 0;
	opcode = (CONDITION_AL << 28) | (0x06 << 21) | (rdHigh << 16) | (rdLow << 12) | (rm << 8) | (0x9 << 4) | (rn << 0);
	WriteWord(opcode);
}

void CArmAssembler::Stmdb(REGISTER rbase, uint16 regList)
{
	//20 - Load
	//21 - Writeback
	//22 - User bit
	//23 - Down/Up (0/1)
	//24 - Post/Pre (0/1)
	uint32 opcode = (CONDITION_AL << 28) | (4 << 25) | (1 << 24) | (1 << 21) | (static_cast<uint32>(rbase) << 16) | regList;
	WriteWord(opcode);
}

void CArmAssembler::Str(REGISTER rd, REGISTER rbase, const LdrAddress& address)
{
	assert(address.isImmediate);
	uint32 opcode = (CONDITION_AL << 28) | (1 << 26) | (1 << 24) | (0 << 20);
	opcode |= (address.isNegative) ? 0 : (1 << 23);
	opcode |= static_cast<uint32>(rbase) << 16;
	opcode |= static_cast<uint32>(rd) << 12;
	opcode |= static_cast<uint32>(address.immediate);
	WriteWord(opcode);
}

void CArmAssembler::Sub(REGISTER rd, REGISTER rn, REGISTER rm)
{
	GenericAlu(ALU_OPCODE_SUB, false, rd, rn, rm);
}

void CArmAssembler::Sub(REGISTER rd, REGISTER rn, const ImmediateAluOperand& operand)
{
	GenericAlu(ALU_OPCODE_SUB, false, rd, rn, operand);
}

void CArmAssembler::Subs(REGISTER rd, REGISTER rn, REGISTER rm)
{
	GenericAlu(ALU_OPCODE_SUB, true, rd, rn, rm);
}

void CArmAssembler::Teq(REGISTER rn, const ImmediateAluOperand& operand)
{
	InstructionAlu instruction;
	instruction.operand = *reinterpret_cast<const unsigned int*>(&operand);
	instruction.rd = 0;
	instruction.rn = rn;
	instruction.setFlags = 1;
	instruction.opcode = ALU_OPCODE_TEQ;
	instruction.immediate = 1;
	instruction.condition = CONDITION_AL;
	uint32 opcode = *reinterpret_cast<uint32*>(&instruction);
	WriteWord(opcode);
}

void CArmAssembler::Tst(REGISTER rn, REGISTER rm)
{
	GenericAlu(ALU_OPCODE_TST, true, CArmAssembler::r0, rn, rm);
}

void CArmAssembler::Umull(REGISTER rdLow, REGISTER rdHigh, REGISTER rn, REGISTER rm)
{
	uint32 opcode = 0;
	opcode = (CONDITION_AL << 28) | (0x04 << 21) | (rdHigh << 16) | (rdLow << 12) | (rm << 8) | (0x9 << 4) | (rn << 0);
	WriteWord(opcode);
}

//////////////////////////////////////////////////
// VFP/NEON Stuff
//////////////////////////////////////////////////

uint32 CArmAssembler::FPSIMD_EncodeSd(SINGLE_REGISTER sd)
{
	return ((sd >> 1) << 12) | ((sd & 1) << 22);
}

uint32 CArmAssembler::FPSIMD_EncodeSn(SINGLE_REGISTER sn)
{
	return ((sn >> 1) << 16) | ((sn & 1) <<  7);
}

uint32 CArmAssembler::FPSIMD_EncodeSm(SINGLE_REGISTER sm)
{
	return ((sm >> 1) <<  0) | ((sm & 1) <<  5);
}

uint32 CArmAssembler::FPSIMD_EncodeDn(DOUBLE_REGISTER dn)
{
	return ((dn & 0xF) << 16) | ((dn >> 4) <<  7);
}

uint32 CArmAssembler::FPSIMD_EncodeQd(QUAD_REGISTER qd)
{
	assert((qd & 1) == 0);
	return ((qd & 0xF) << 12) | ((qd >> 4) << 22);
}

uint32 CArmAssembler::FPSIMD_EncodeQn(QUAD_REGISTER qn)
{
	assert((qn & 1) == 0);
	return ((qn & 0xF) << 16) | ((qn >> 4) <<  7);
}

uint32 CArmAssembler::FPSIMD_EncodeQm(QUAD_REGISTER qm)
{
	assert((qm & 1) == 0);
	return ((qm & 0x0F) <<  0) | ((qm >> 4) <<  5);
}

void CArmAssembler::Vldr(SINGLE_REGISTER sd, REGISTER rbase, const LdrAddress& address)
{
	assert(address.isImmediate);
	assert(!address.isNegative);
	assert((address.immediate / 4) <= 0xFF);

	uint32 opcode = 0x0D900A00;
	opcode |= (CONDITION_AL << 28);
	opcode |= FPSIMD_EncodeSd(sd);
	opcode |= (static_cast<uint32>(rbase) << 16) | (static_cast<uint32>(address.immediate / 4));
	WriteWord(opcode);
}

void CArmAssembler::Vld1_32x4(QUAD_REGISTER qd, REGISTER rn)
{
	//TODO: Make this aligned

	uint32 opcode = 0xF4200A8F;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= static_cast<uint32>(rn) << 16;
	WriteWord(opcode);
}

void CArmAssembler::Vstr(SINGLE_REGISTER sd, REGISTER rbase, const LdrAddress& address)
{
	assert(address.isImmediate);
	assert(!address.isNegative);
	assert((address.immediate / 4) <= 0xFF);

	uint32 opcode = 0x0D800A00;
	opcode |= (CONDITION_AL << 28);
	opcode |= FPSIMD_EncodeSd(sd);
	opcode |= (static_cast<uint32>(rbase) << 16) | (static_cast<uint32>(address.immediate / 4));
	WriteWord(opcode);
}

void CArmAssembler::Vst1_32x4(QUAD_REGISTER qd, REGISTER rn)
{
	//TODO: Make this aligned

	uint32 opcode = 0xF4000A8F;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= static_cast<uint32>(rn) << 16;
	WriteWord(opcode);
}

void CArmAssembler::Vmov(DOUBLE_REGISTER dd, REGISTER rt, uint8 offset)
{
	uint32 opcode = 0x0E000B10;
	opcode |= (CONDITION_AL << 28);
	opcode |= (offset != 0) ? 0x00200000 : 0;
	opcode |= FPSIMD_EncodeDn(dd);
	opcode |= (rt << 12);
	WriteWord(opcode);
}

void CArmAssembler::Vmov(REGISTER rt, DOUBLE_REGISTER dn, uint8 offset)
{
	uint32 opcode = 0x0E100B10;
	opcode |= (CONDITION_AL << 28);
	opcode |= (offset != 0) ? 0x00200000 : 0;
	opcode |= FPSIMD_EncodeDn(dn);
	opcode |= (rt << 12);
	WriteWord(opcode);
}

void CArmAssembler::Vdup(QUAD_REGISTER qd, REGISTER rt)
{
	uint32 opcode = 0x0EA00B10;
	opcode |= (CONDITION_AL << 28);
	opcode |= FPSIMD_EncodeQn(qd);
	opcode |= (rt << 12);
	WriteWord(opcode);
}

void CArmAssembler::Vadd_F32(SINGLE_REGISTER sd, SINGLE_REGISTER sn, SINGLE_REGISTER sm)
{
	uint32 opcode = 0x0E300A00;
	opcode |= (CONDITION_AL << 28);
	opcode |= FPSIMD_EncodeSd(sd);
	opcode |= FPSIMD_EncodeSn(sn);
	opcode |= FPSIMD_EncodeSm(sm);
	WriteWord(opcode);
}

void CArmAssembler::Vadd_F32(QUAD_REGISTER qd, QUAD_REGISTER qn, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF2000D40;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQn(qn);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CArmAssembler::Vadd_I32(QUAD_REGISTER qd, QUAD_REGISTER qn, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF2200840;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQn(qn);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CArmAssembler::Vsub_F32(SINGLE_REGISTER sd, SINGLE_REGISTER sn, SINGLE_REGISTER sm)
{
	uint32 opcode = 0x0E300A40;
	opcode |= (CONDITION_AL << 28);
	opcode |= FPSIMD_EncodeSd(sd);
	opcode |= FPSIMD_EncodeSn(sn);
	opcode |= FPSIMD_EncodeSm(sm);
	WriteWord(opcode);
}

void CArmAssembler::Vsub_F32(QUAD_REGISTER qd, QUAD_REGISTER qn, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF2200D40;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQn(qn);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CArmAssembler::Vsub_I8(QUAD_REGISTER qd, QUAD_REGISTER qn, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF3000840;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQn(qn);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CArmAssembler::Vsub_I32(QUAD_REGISTER qd, QUAD_REGISTER qn, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF3200840;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQn(qn);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CArmAssembler::Vmul_F32(SINGLE_REGISTER sd, SINGLE_REGISTER sn, SINGLE_REGISTER sm)
{
	uint32 opcode = 0x0E200A00;
	opcode |= (CONDITION_AL << 28);
	opcode |= FPSIMD_EncodeSd(sd);
	opcode |= FPSIMD_EncodeSn(sn);
	opcode |= FPSIMD_EncodeSm(sm);
	WriteWord(opcode);
}

void CArmAssembler::Vmul_F32(QUAD_REGISTER qd, QUAD_REGISTER qn, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF3000D50;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQn(qn);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CArmAssembler::Vdiv_F32(SINGLE_REGISTER sd, SINGLE_REGISTER sn, SINGLE_REGISTER sm)
{
	uint32 opcode = 0x0E800A00;
	opcode |= (CONDITION_AL << 28);
	opcode |= FPSIMD_EncodeSd(sd);
	opcode |= FPSIMD_EncodeSn(sn);
	opcode |= FPSIMD_EncodeSm(sm);
	WriteWord(opcode);
}

void CArmAssembler::Vand(QUAD_REGISTER qd, QUAD_REGISTER qn, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF2000150;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQn(qn);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CArmAssembler::Vorn(QUAD_REGISTER qd, QUAD_REGISTER qn, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF2300150;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQn(qn);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CArmAssembler::Vorr(QUAD_REGISTER qd, QUAD_REGISTER qn, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF2200150;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQn(qn);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CArmAssembler::Veor(QUAD_REGISTER qd, QUAD_REGISTER qn, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF3000150;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQn(qn);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CArmAssembler::Vabs_F32(SINGLE_REGISTER sd, SINGLE_REGISTER sm)
{
	uint32 opcode = 0x0EB00AC0;
	opcode |= (CONDITION_AL << 28);
	opcode |= FPSIMD_EncodeSd(sd);
	opcode |= FPSIMD_EncodeSm(sm);
	WriteWord(opcode);
}

void CArmAssembler::Vabs_F32(QUAD_REGISTER qd, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF3B90740;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CArmAssembler::Vneg_F32(SINGLE_REGISTER sd, SINGLE_REGISTER sm)
{
	uint32 opcode = 0x0EB10A40;
	opcode |= (CONDITION_AL << 28);
	opcode |= FPSIMD_EncodeSd(sd);
	opcode |= FPSIMD_EncodeSm(sm);
	WriteWord(opcode);
}

void CArmAssembler::Vsqrt_F32(SINGLE_REGISTER sd, SINGLE_REGISTER sm)
{
	uint32 opcode = 0x0EB10AC0;
	opcode |= (CONDITION_AL << 28);
	opcode |= FPSIMD_EncodeSd(sd);
	opcode |= FPSIMD_EncodeSm(sm);
	WriteWord(opcode);
}

void CArmAssembler::Vceq_I32(QUAD_REGISTER qd, QUAD_REGISTER qn, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF3200850;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQn(qn);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CArmAssembler::Vcmp_F32(SINGLE_REGISTER sd, SINGLE_REGISTER sm)
{
	uint32 opcode = 0x0EB40A40;
	opcode |= (CONDITION_AL << 28);
	opcode |= FPSIMD_EncodeSd(sd);
	opcode |= FPSIMD_EncodeSm(sm);
	WriteWord(opcode);
}

void CArmAssembler::Vcvt_F32_S32(SINGLE_REGISTER sd, SINGLE_REGISTER sm)
{
	uint32 opcode = 0x0EB80AC0;
	opcode |= (CONDITION_AL << 28);
	opcode |= FPSIMD_EncodeSd(sd);
	opcode |= FPSIMD_EncodeSm(sm);
	WriteWord(opcode);
}

void CArmAssembler::Vcvt_S32_F32(SINGLE_REGISTER sd, SINGLE_REGISTER sm)
{
	uint32 opcode = 0x0EBD0AC0;
	opcode |= (CONDITION_AL << 28);
	opcode |= FPSIMD_EncodeSd(sd);
	opcode |= FPSIMD_EncodeSm(sm);
	WriteWord(opcode);
}

void CArmAssembler::Vmrs(REGISTER rt)
{
	uint32 opcode = 0x0EF10A10;
	opcode |= (CONDITION_AL << 28);
	opcode |= (rt << 12);
	WriteWord(opcode);
}

void CArmAssembler::Vrecpe_F32(QUAD_REGISTER qd, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF3BB0540;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CArmAssembler::Vrsqrte_F32(QUAD_REGISTER qd, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF3BB05C0;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CArmAssembler::Vmin_F32(QUAD_REGISTER qd, QUAD_REGISTER qn, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF2200F40;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQn(qn);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CArmAssembler::Vmax_F32(QUAD_REGISTER qd, QUAD_REGISTER qn, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF2000F40;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQn(qn);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CArmAssembler::WriteWord(uint32 value)
{
	m_stream->Write32(value);
}
