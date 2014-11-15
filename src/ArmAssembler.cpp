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

CArmAssembler::LdrAddress CArmAssembler::MakeImmediateLdrAddress(uint32 immediate)
{
	LdrAddress result;
	memset(&result, 0, sizeof(result));
	assert((immediate & ~0xFFF) == 0);
	result.isImmediate = true;
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
    for(LabelReferenceMapType::iterator labelRef(m_labelReferences.begin());
        m_labelReferences.end() != labelRef; labelRef++)
    {
        LabelMapType::iterator label(m_labels.find(labelRef->first));
        if(label == m_labels.end())
        {
            throw std::runtime_error("Invalid label.");
        }
        size_t referencePos = labelRef->second;
        size_t labelPos = label->second;
		int offset = static_cast<int>(labelPos - referencePos) / 4;
		assert(offset >= 2);
		offset -= 2;

		m_stream->Seek(referencePos, Framework::STREAM_SEEK_SET);
        m_stream->Write8(static_cast<uint8>(offset >> 0));
        m_stream->Write8(static_cast<uint8>(offset >> 8));
        m_stream->Write8(static_cast<uint8>(offset >> 16));
		m_stream->Seek(0, Framework::STREAM_SEEK_END);
    }
    m_labelReferences.clear();
}

void CArmAssembler::CreateLabelReference(LABEL label)
{
    LABELREF reference = static_cast<size_t>(m_stream->Tell());
    m_labelReferences.insert(LabelReferenceMapType::value_type(label, reference));
}

void CArmAssembler::Add(REGISTER rd, REGISTER rn, REGISTER rm)
{
	InstructionAlu instruction;
	instruction.operand = rm;
	instruction.rn = rn;
	instruction.rd = rd;
	instruction.setFlags = 0;
	instruction.opcode = ALU_OPCODE_ADD;
	instruction.immediate = 0;
	instruction.condition = CONDITION_AL;
	uint32 opcode = *reinterpret_cast<uint32*>(&instruction);
	WriteWord(opcode);
}

void CArmAssembler::Add(REGISTER rd, REGISTER rn, const ImmediateAluOperand& operand)
{
	InstructionAlu instruction;
	instruction.operand = *reinterpret_cast<const unsigned int*>(&operand);
	instruction.rd = rd;
	instruction.rn = rn;
	instruction.setFlags = 0;
	instruction.opcode = ALU_OPCODE_ADD;
	instruction.immediate = 1;
	instruction.condition = CONDITION_AL;
	uint32 opcode = *reinterpret_cast<uint32*>(&instruction);
	WriteWord(opcode);
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
	uint32 opcode = 0;
	assert(address.isImmediate);
	opcode = (CONDITION_AL << 28) | (1 << 26) | (1 << 24) | (1 << 23) | (0 << 20) | (static_cast<uint32>(rbase) << 16) | (static_cast<uint32>(rd) << 12) | (static_cast<uint32>(address.immediate));
	WriteWord(opcode);
}

void CArmAssembler::Sub(REGISTER rd, REGISTER rn, REGISTER rm)
{
	InstructionAlu instruction;
	instruction.operand = rm;
	instruction.rn = rn;
	instruction.rd = rd;
	instruction.setFlags = 0;
	instruction.opcode = ALU_OPCODE_SUB;
	instruction.immediate = 0;
	instruction.condition = CONDITION_AL;
	uint32 opcode = *reinterpret_cast<uint32*>(&instruction);
	WriteWord(opcode);
}

void CArmAssembler::Sub(REGISTER rd, REGISTER rn, const ImmediateAluOperand& operand)
{
	InstructionAlu instruction;
	instruction.operand = *reinterpret_cast<const unsigned int*>(&operand);
	instruction.rd = rd;
	instruction.rn = rn;
	instruction.setFlags = 0;
	instruction.opcode = ALU_OPCODE_SUB;
	instruction.immediate = 1;
	instruction.condition = CONDITION_AL;
	uint32 opcode = *reinterpret_cast<uint32*>(&instruction);
	WriteWord(opcode);
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
	assert((address.immediate / 4) <= 0xFF);

	uint32 opcode = 0x0D900A00;
	opcode |= (CONDITION_AL << 28);
	opcode |= FPSIMD_EncodeSd(sd);
	opcode |= (static_cast<uint32>(rbase) << 16) | (static_cast<uint32>(address.immediate / 4));
	WriteWord(opcode);
}

void CArmAssembler::Vstr(SINGLE_REGISTER sd, REGISTER rbase, const LdrAddress& address)
{
	assert((address.immediate / 4) <= 0xFF);

	uint32 opcode = 0x0D800A00;
	opcode |= (CONDITION_AL << 28);
	opcode |= FPSIMD_EncodeSd(sd);
	opcode |= (static_cast<uint32>(rbase) << 16) | (static_cast<uint32>(address.immediate / 4));
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

void CArmAssembler::Vmul_F32(SINGLE_REGISTER sd, SINGLE_REGISTER sn, SINGLE_REGISTER sm)
{
	uint32 opcode = 0x0E200A00;
	opcode |= (CONDITION_AL << 28);
	opcode |= FPSIMD_EncodeSd(sd);
	opcode |= FPSIMD_EncodeSn(sn);
	opcode |= FPSIMD_EncodeSm(sm);
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

void CArmAssembler::Vabs_F32(SINGLE_REGISTER sd, SINGLE_REGISTER sm)
{
	uint32 opcode = 0x0EB00AC0;
	opcode |= (CONDITION_AL << 28);
	opcode |= FPSIMD_EncodeSd(sd);
	opcode |= FPSIMD_EncodeSm(sm);
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
