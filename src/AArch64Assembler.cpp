#include <assert.h>
#include <stdexcept>
#include "AArch64Assembler.h"
#include "LiteralPool.h"

void CAArch64Assembler::SetStream(Framework::CStream* stream)
{
	m_stream = stream;
}

CAArch64Assembler::LABEL CAArch64Assembler::CreateLabel()
{
	return m_nextLabelId++;
}

void CAArch64Assembler::ClearLabels()
{
	m_labels.clear();
}

void CAArch64Assembler::MarkLabel(LABEL label)
{
	m_labels[label] = static_cast<size_t>(m_stream->Tell());
}

void CAArch64Assembler::CreateBranchLabelReference(LABEL label, CONDITION condition)
{
	LABELREF reference;
	reference.offset = static_cast<size_t>(m_stream->Tell());
	reference.condition = condition;
	m_labelReferences.insert(std::make_pair(label, reference));
}

void CAArch64Assembler::CreateCompareBranchLabelReference(LABEL label, CONDITION condition, REGISTER32 cbRegister)
{
	LABELREF reference;
	reference.offset = static_cast<size_t>(m_stream->Tell());
	reference.condition = condition;
	reference.cbz = true;
	reference.cbRegister = cbRegister;
	m_labelReferences.insert(std::make_pair(label, reference));
}

void CAArch64Assembler::CreateCompareBranchLabelReference(LABEL label, CONDITION condition, REGISTER64 cbRegister)
{
	LABELREF reference;
	reference.offset = static_cast<size_t>(m_stream->Tell());
	reference.condition = condition;
	reference.cbz64 = true;
	reference.cbRegister = static_cast<REGISTER32>(cbRegister);
	m_labelReferences.insert(std::make_pair(label, reference));
}

void CAArch64Assembler::ResolveLabelReferences()
{
	for(const auto& labelReferencePair : m_labelReferences)
	{
		auto label(m_labels.find(labelReferencePair.first));
		if(label == m_labels.end())
		{
			throw std::runtime_error("Invalid label.");
		}
		const auto& labelReference = labelReferencePair.second;
		size_t labelPos = label->second;
		int offset = static_cast<int>(labelPos - labelReference.offset) / 4;

		m_stream->Seek(labelReference.offset, Framework::STREAM_SEEK_SET);
		if(labelReference.condition == CONDITION_AL)
		{
			uint32 opcode = 0x14000000;
			opcode |= (offset & 0x3FFFFFF);
			WriteWord(opcode);
		}
		else
		{
			if(labelReference.cbz || labelReference.cbz64)
			{
				assert((labelReference.condition == CONDITION_EQ) || (labelReference.condition == CONDITION_NE));
				uint32 opcode = [&labelReference]() {
					uint32 sf = labelReference.cbz64 ? 0x80000000 : 0;
					switch(labelReference.condition)
					{
					case CONDITION_EQ:
						return 0x34000000 | sf;
						break;
					case CONDITION_NE:
						return 0x35000000 | sf;
						break;
					default:
						assert(false);
						return 0U;
					}
				}();
				opcode |= (offset & 0x7FFFF) << 5;
				opcode |= labelReference.cbRegister;
				WriteWord(opcode);
			}
			else
			{
				uint32 opcode = 0x54000000;
				opcode |= (offset & 0x7FFFF) << 5;
				opcode |= labelReference.condition;
				WriteWord(opcode);
			}
		}
	}
	m_stream->Seek(0, Framework::STREAM_SEEK_END);
	m_labelReferences.clear();
}

void CAArch64Assembler::ResolveLiteralReferences()
{
	if(m_literal128Refs.empty()) return;

	CLiteralPool literalPool(m_stream);
	literalPool.AlignPool();

	for(const auto& literalRef : m_literal128Refs)
	{
		auto literalPos = static_cast<uint32>(literalPool.GetLiteralPosition(literalRef.value));
		m_stream->Seek(literalRef.offset, Framework::STREAM_SEEK_SET);
		auto offset = literalPos - literalRef.offset;
		assert((offset & 0x03) == 0);
		assert(offset < 0x100000);
		offset /= 4;
		m_stream->Write32(0x9C000000 | static_cast<uint32>(offset << 5) | literalRef.rt);
	}
	m_literal128Refs.clear();
	m_stream->Seek(0, Framework::STREAM_SEEK_END);
}

void CAArch64Assembler::Add(REGISTER32 rd, REGISTER32 rn, REGISTER32 rm)
{
	uint32 opcode = 0x0B000000;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Add(REGISTER64 rd, REGISTER64 rn, REGISTER64 rm)
{
	uint32 opcode = 0x8B000000;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Add(REGISTER32 rd, REGISTER32 rn, uint16 imm, ADDSUB_IMM_SHIFT_TYPE shift)
{
	WriteAddSubOpImm(0x11000000, shift, imm, rn, rd);
}

void CAArch64Assembler::Add(REGISTER64 rd, REGISTER64 rn, uint16 imm, ADDSUB_IMM_SHIFT_TYPE shift)
{
	WriteAddSubOpImm(0x91000000, shift, imm, rn, rd);
}

void CAArch64Assembler::Add_4s(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
	uint32 opcode = 0x4EA08400;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Add_8h(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
	uint32 opcode = 0x4E608400;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Add_16b(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
	uint32 opcode = 0x4E208400;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::And(REGISTER32 rd, REGISTER32 rn, REGISTER32 rm)
{
	uint32 opcode = 0x0A000000;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::And(REGISTER64 rd, REGISTER64 rn, REGISTER64 rm)
{
	uint32 opcode = 0x8A000000;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::And(REGISTER32 rd, REGISTER32 rn, uint8 n, uint8 immr, uint8 imms)
{
	WriteLogicalOpImm(0x12000000, n, immr, imms, rn, rd);
}

void CAArch64Assembler::And_16b(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
	uint32 opcode = 0x4E201C00;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Asr(REGISTER32 rd, REGISTER32 rn, uint8 sa)
{
	uint32 imms = 0x1F;
	uint32 immr = sa & 0x1F;
	WriteLogicalOpImm(0x13000000, 0, immr, imms, rn, rd);
}

void CAArch64Assembler::Asr(REGISTER64 rd, REGISTER64 rn, uint8 sa)
{
	uint32 imms = 0x3F;
	uint32 immr = sa & 0x3F;
	WriteLogicalOpImm(0x93400000, 0, immr, imms, rn, rd);
}

void CAArch64Assembler::Asrv(REGISTER32 rd, REGISTER32 rn, REGISTER32 rm)
{
	WriteDataProcOpReg2(0x1AC02800, rm, rn, rd);
}

void CAArch64Assembler::Asrv(REGISTER64 rd, REGISTER64 rn, REGISTER64 rm)
{
	WriteDataProcOpReg2(0x9AC02800, rm, rn, rd);
}

void CAArch64Assembler::B(LABEL label)
{
	CreateBranchLabelReference(label, CONDITION_AL);
	WriteWord(0);
}

void CAArch64Assembler::B_offset(uint32 offset)
{
	assert((offset & 0x3) == 0);
	offset /= 4;
	assert(offset < 0x40000000);
	uint32 opcode = 0x14000000;
	opcode |= offset;
	WriteWord(opcode);
}

void CAArch64Assembler::Bl(uint32 offset)
{
	assert((offset & 0x3) == 0);
	offset /= 4;
	assert(offset < 0x40000000);
	uint32 opcode = 0x94000000;
	opcode |= offset;
	WriteWord(opcode);
}

void CAArch64Assembler::Br(REGISTER64 rn)
{
	uint32 opcode = 0xD61F0000;
	opcode |= (rn << 5);
	WriteWord(opcode);
}

void CAArch64Assembler::BCc(CONDITION condition, LABEL label)
{
	CreateBranchLabelReference(label, condition);
	WriteWord(0);
}

void CAArch64Assembler::Blr(REGISTER64 rn)
{
	uint32 opcode = 0xD63F0000;
	opcode |= (rn << 5);
	WriteWord(opcode);
}

void CAArch64Assembler::Cbnz(REGISTER32 rt, LABEL label)
{
	CreateCompareBranchLabelReference(label, CONDITION_NE, rt);
	WriteWord(0);
}

void CAArch64Assembler::Cbnz(REGISTER64 rt, LABEL label)
{
	CreateCompareBranchLabelReference(label, CONDITION_NE, rt);
	WriteWord(0);
}

void CAArch64Assembler::Cbz(REGISTER32 rt, LABEL label)
{
	CreateCompareBranchLabelReference(label, CONDITION_EQ, rt);
	WriteWord(0);
}

void CAArch64Assembler::Cbz(REGISTER64 rt, LABEL label)
{
	CreateCompareBranchLabelReference(label, CONDITION_EQ, rt);
	WriteWord(0);
}

void CAArch64Assembler::Clz(REGISTER32 rd, REGISTER32 rn)
{
	uint32 opcode = 0x5AC01000;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	WriteWord(opcode);
}

void CAArch64Assembler::Cmeq_16b(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
	uint32 opcode = 0x6E208C00;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Cmeq_8h(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
	uint32 opcode = 0x6E608C00;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Cmeq_4s(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
	uint32 opcode = 0x6EA08C00;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Cmgt_16b(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
	uint32 opcode = 0x4E203400;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Cmgt_8h(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
	uint32 opcode = 0x4E603400;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Cmgt_4s(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
	uint32 opcode = 0x4EA03400;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Cmltz_4s(REGISTERMD rd, REGISTERMD rn)
{
	uint32 opcode = 0x4EA0A800;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	WriteWord(opcode);
}

void CAArch64Assembler::Cmn(REGISTER32 rn, uint16 imm, ADDSUB_IMM_SHIFT_TYPE shift)
{
	WriteAddSubOpImm(0x31000000, shift, imm, rn, wZR);
}

void CAArch64Assembler::Cmn(REGISTER64 rn, uint16 imm, ADDSUB_IMM_SHIFT_TYPE shift)
{
	WriteAddSubOpImm(0xB1000000, shift, imm, rn, wZR);
}

void CAArch64Assembler::Cmp(REGISTER32 rn, REGISTER32 rm)
{
	uint32 opcode = 0x6B000000;
	opcode |= (wZR << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Cmp(REGISTER64 rn, REGISTER64 rm)
{
	uint32 opcode = 0xEB000000;
	opcode |= (wZR << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Cmp(REGISTER32 rn, uint16 imm, ADDSUB_IMM_SHIFT_TYPE shift)
{
	WriteAddSubOpImm(0x71000000, shift, imm, rn, wZR);
}

void CAArch64Assembler::Cmp(REGISTER64 rn, uint16 imm, ADDSUB_IMM_SHIFT_TYPE shift)
{
	WriteAddSubOpImm(0xF1000000, shift, imm, rn, wZR);
}

void CAArch64Assembler::Csel(REGISTER32 rd, REGISTER32 rn, REGISTER32 rm, CONDITION condition)
{
	uint32 opcode = 0x1A800000;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (condition << 12);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Cset(REGISTER32 rd, CONDITION condition)
{
	uint32 opcode = 0x1A800400;
	opcode |= (rd << 0);
	opcode |= (wZR << 5);
	opcode |= ((condition ^ 1) << 12); //Inverting lsb inverts condition
	opcode |= (wZR << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Dup_4s(REGISTERMD rd, REGISTER32 rn)
{
	uint32 opcode = 0x4E040C00;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	WriteWord(opcode);
}

void CAArch64Assembler::Dup_4s(REGISTERMD rd, REGISTERMD rn, uint8 index)
{
	assert(index < 4);
	uint32 opcode = 0x4E040400;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (index << 19);
	WriteWord(opcode);
}

void CAArch64Assembler::Eor(REGISTER32 rd, REGISTER32 rn, REGISTER32 rm)
{
	uint32 opcode = 0x4A000000;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Eor(REGISTER32 rd, REGISTER32 rn, uint8 n, uint8 immr, uint8 imms)
{
	WriteLogicalOpImm(0x52000000, n, immr, imms, rn, rd);
}

void CAArch64Assembler::Eor_16b(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
	uint32 opcode = 0x6E201C00;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Fabs_1s(REGISTERMD rd, REGISTERMD rn)
{
	uint32 opcode = 0x1E20C000;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	WriteWord(opcode);
}

void CAArch64Assembler::Fabs_4s(REGISTERMD rd, REGISTERMD rn)
{
	uint32 opcode = 0x4EA0F800;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	WriteWord(opcode);
}

void CAArch64Assembler::Fadd_1s(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
	uint32 opcode = 0x1E202800;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Fadd_4s(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
	uint32 opcode = 0x4E20D400;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Fcmeqz_4s(REGISTERMD rd, REGISTERMD rn)
{
	uint32 opcode = 0x4EA0D800;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	WriteWord(opcode);
}

void CAArch64Assembler::Fcmge_4s(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
	uint32 opcode = 0x6E20E400;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Fcmgt_4s(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
	uint32 opcode = 0x6EA0E400;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Fcmltz_4s(REGISTERMD rd, REGISTERMD rn)
{
	uint32 opcode = 0x4EA0E800;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	WriteWord(opcode);
}

void CAArch64Assembler::Fcmp_1s(REGISTERMD rn, REGISTERMD rm)
{
	uint32 opcode = 0x1E202000;
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Fcvtzs_1s(REGISTERMD rd, REGISTERMD rn)
{
	uint32 opcode = 0x5EA1B800;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	WriteWord(opcode);
}

void CAArch64Assembler::Fcvtzs_4s(REGISTERMD rd, REGISTERMD rn)
{
	uint32 opcode = 0x4EA1B800;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	WriteWord(opcode);
}

void CAArch64Assembler::Fdiv_1s(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
	uint32 opcode = 0x1E201800;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Fdiv_4s(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
	uint32 opcode = 0x6E20FC00;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Fmov_1s(REGISTERMD rd, REGISTER32 rn)
{
	uint32 opcode = 0x1E270000;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	WriteWord(opcode);
}

void CAArch64Assembler::Fmov_1s(REGISTERMD rd, uint8 imm)
{
	uint32 opcode = 0x1E201000;
	opcode |= (rd << 0);
	opcode |= (imm << 13);
	WriteWord(opcode);
}

void CAArch64Assembler::Fmov_4s(REGISTERMD rd, uint8 imm)
{
	uint32 cmode = 0xF;
	uint32 immHi = (imm >> 5);
	uint32 immLo = (imm & 0x1F);
	uint32 opcode = 0x4F000400;
	opcode |= (rd << 0);
	opcode |= (cmode << 12);
	opcode |= (immHi << 16);
	opcode |= (immLo << 5);
	WriteWord(opcode);
}

void CAArch64Assembler::Fmul_1s(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
	uint32 opcode = 0x1E200800;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Fmul_4s(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
	uint32 opcode = 0x6E20DC00;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Fmax_1s(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
	uint32 opcode = 0x1E204800;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Fmax_4s(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
	uint32 opcode = 0x4E20F400;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Fneg_1s(REGISTERMD rd, REGISTERMD rn)
{
	uint32 opcode = 0x1E214000;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	WriteWord(opcode);
}

void CAArch64Assembler::Fneg_4s(REGISTERMD rd, REGISTERMD rn)
{
	uint32 opcode = 0x6EA0F800;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	WriteWord(opcode);
}

void CAArch64Assembler::Fmin_1s(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
	uint32 opcode = 0x1E205800;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Fmin_4s(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
	uint32 opcode = 0x4EA0F400;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Fsqrt_1s(REGISTERMD rd, REGISTERMD rn)
{
	uint32 opcode = 0x1E21C000;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	WriteWord(opcode);
}

void CAArch64Assembler::Fsub_1s(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
	uint32 opcode = 0x1E203800;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Fsub_4s(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
	uint32 opcode = 0x4EA0D400;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Ins_1s(REGISTERMD rd, uint8 index1, REGISTERMD rn, uint8 index2)
{
	assert(index1 < 4);
	assert(index2 < 4);
	index1 &= 0x3;
	index2 &= 0x3;
	uint32 opcode = 0x6E040400;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (index2 << 13);
	opcode |= (index1 << 19);
	WriteWord(opcode);
}

void CAArch64Assembler::Ins_1d(REGISTERMD rd, uint8 index, REGISTER64 rn)
{
	assert(index < 2);
	index &= 0x1;
	uint8 imm5 = (index << 4) | 0x8;
	uint32 opcode = 0x4E001C00;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (imm5 << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Ins_1d(REGISTERMD rd, uint8 index1, REGISTERMD rn, uint8 index2)
{
	assert(index1 < 2);
	assert(index2 < 2);
	index1 &= 0x1;
	index2 &= 0x1;
	uint32 opcode = 0x6E080400;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (index2 << 14);
	opcode |= (index1 << 20);
	WriteWord(opcode);
}

void CAArch64Assembler::Ld1_4s(REGISTERMD rt, REGISTER64 rn)
{
	uint32 opcode = 0x4C407800;
	opcode |= (rt << 0);
	opcode |= (rn << 5);
	WriteWord(opcode);
}

void CAArch64Assembler::Ldp_PostIdx(REGISTER64 rt, REGISTER64 rt2, REGISTER64 rn, int32 offset)
{
	assert((offset & 0x07) == 0);
	int32 scaledOffset = offset / 8;
	assert(scaledOffset >= -64 && scaledOffset <= 63);
	uint32 opcode = 0xA8C00000;
	opcode |= (rt << 0);
	opcode |= (rn << 5);
	opcode |= (rt2 << 10);
	opcode |= ((scaledOffset & 0x7F) << 15);
	WriteWord(opcode);
}

void CAArch64Assembler::Ldr(REGISTER32 rt, REGISTER64 rn, uint32 offset)
{
	assert((offset & 0x03) == 0);
	uint32 scaledOffset = offset / 4;
	assert(scaledOffset < 0x1000);
	WriteLoadStoreOpImm(0xB9400000, scaledOffset, rn, rt);
}

void CAArch64Assembler::Ldr(REGISTER32 rt, REGISTER64 rn, REGISTER64 rm, bool scaled)
{
	uint32 opcode = 0xB8606800;
	opcode |= (rt << 0);
	opcode |= (rn << 5);
	opcode |= scaled ? (1 << 12) : 0;
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Ldr(REGISTER64 rt, REGISTER64 rn, uint32 offset)
{
	assert((offset & 0x07) == 0);
	uint32 scaledOffset = offset / 8;
	assert(scaledOffset < 0x1000);
	WriteLoadStoreOpImm(0xF9400000, scaledOffset, rn, rt);
}

void CAArch64Assembler::Ldr(REGISTER64 rt, REGISTER64 rn, REGISTER64 rm, bool scaled)
{
	uint32 opcode = 0xF8606800;
	opcode |= (rt << 0);
	opcode |= (rn << 5);
	opcode |= scaled ? (1 << 12) : 0;
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Ldrb(REGISTER32 rt, REGISTER64 rn, uint32 offset)
{
	uint32 scaledOffset = offset;
	assert(scaledOffset < 0x1000);
	WriteLoadStoreOpImm(0x39400000, scaledOffset, rn, rt);
}

void CAArch64Assembler::Ldrb(REGISTER32 rt, REGISTER64 rn, REGISTER64 rm, bool scaled)
{
	uint32 opcode = 0x38606800;
	opcode |= (rt << 0);
	opcode |= (rn << 5);
	opcode |= scaled ? (1 << 12) : 0;
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Ldrh(REGISTER32 rt, REGISTER64 rn, uint32 offset)
{
	assert((offset & 0x01) == 0);
	uint32 scaledOffset = offset / 2;
	assert(scaledOffset < 0x1000);
	WriteLoadStoreOpImm(0x79400000, scaledOffset, rn, rt);
}

void CAArch64Assembler::Ldrh(REGISTER32 rt, REGISTER64 rn, REGISTER64 rm, bool scaled)
{
	uint32 opcode = 0x78606800;
	opcode |= (rt << 0);
	opcode |= (rn << 5);
	opcode |= scaled ? (1 << 12) : 0;
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Ldr_Pc(REGISTER64 rt, uint32 offset)
{
	assert((offset & 0x03) == 0);
	uint32 scaledOffset = offset / 4;
	assert(scaledOffset < 0x40000);
	uint32 opcode = 0x58000000;
	opcode |= (rt << 0);
	opcode |= scaledOffset << 5;
	WriteWord(opcode);
}

void CAArch64Assembler::Ldr_Pc(REGISTERMD rt, const LITERAL128& literal)
{
	LITERAL128REF literalRef;
	literalRef.offset = static_cast<size_t>(m_stream->Tell());
	literalRef.value = literal;
	literalRef.rt = rt;
	m_literal128Refs.push_back(literalRef);
	WriteWord(0);
}

void CAArch64Assembler::Ldr_1s(REGISTERMD rt, REGISTER64 rn, uint32 offset)
{
	assert((offset & 0x03) == 0);
	uint32 scaledOffset = offset / 4;
	assert(scaledOffset < 0x1000);
	WriteLoadStoreOpImm(0xBD400000, scaledOffset, rn, rt);
}

void CAArch64Assembler::Ldr_1q(REGISTERMD rt, REGISTER64 rn, uint32 offset)
{
	assert((offset & 0x0F) == 0);
	uint32 scaledOffset = offset / 0x10;
	assert(scaledOffset < 0x1000);
	WriteLoadStoreOpImm(0x3DC00000, scaledOffset, rn, rt);
}

void CAArch64Assembler::Ldr_1q(REGISTERMD rt, REGISTER64 rn, REGISTER64 rm, bool scaled)
{
	uint32 opcode = 0x3CE04800;
	opcode |= (rt << 0);
	opcode |= (rn << 5);
	opcode |= scaled ? (1 << 12) : 0;
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Lsl(REGISTER32 rd, REGISTER32 rn, uint8 sa)
{
	uint32 imms = 0x1F - (sa & 0x1F);
	uint32 immr = -sa & 0x1F;
	WriteLogicalOpImm(0x53000000, 0, immr, imms, rn, rd);
}

void CAArch64Assembler::Lsl(REGISTER64 rd, REGISTER64 rn, uint8 sa)
{
	uint32 imms = 0x3F - (sa & 0x3F);
	uint32 immr = -sa & 0x3F;
	WriteLogicalOpImm(0xD3400000, 0, immr, imms, rn, rd);
}

void CAArch64Assembler::Lslv(REGISTER32 rd, REGISTER32 rn, REGISTER32 rm)
{
	WriteDataProcOpReg2(0x1AC02000, rm, rn, rd);
}

void CAArch64Assembler::Lslv(REGISTER64 rd, REGISTER64 rn, REGISTER64 rm)
{
	WriteDataProcOpReg2(0x9AC02000, rm, rn, rd);
}

void CAArch64Assembler::Lsr(REGISTER32 rd, REGISTER32 rn, uint8 sa)
{
	uint32 imms = 0x1F;
	uint32 immr = sa & 0x1F;
	WriteLogicalOpImm(0x53000000, 0, immr, imms, rn, rd);
}

void CAArch64Assembler::Lsr(REGISTER64 rd, REGISTER64 rn, uint8 sa)
{
	uint32 imms = 0x3F;
	uint32 immr = sa & 0x3F;
	WriteLogicalOpImm(0xD3400000, 0, immr, imms, rn, rd);
}

void CAArch64Assembler::Lsrv(REGISTER32 rd, REGISTER32 rn, REGISTER32 rm)
{
	WriteDataProcOpReg2(0x1AC02400, rm, rn, rd);
}

void CAArch64Assembler::Lsrv(REGISTER64 rd, REGISTER64 rn, REGISTER64 rm)
{
	WriteDataProcOpReg2(0x9AC02400, rm, rn, rd);
}

void CAArch64Assembler::Mov(REGISTER32 rd, REGISTER32 rm)
{
	uint32 opcode = 0x2A0003E0;
	opcode |= (rd << 0);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Mov(REGISTER64 rd, REGISTER64 rm)
{
	uint32 opcode = 0xAA0003E0;
	opcode |= (rd << 0);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Mov(REGISTERMD rd, REGISTERMD rn)
{
	Orr_16b(rd, rn, rn);
}

void CAArch64Assembler::Mov_Sp(REGISTER64 rd, REGISTER64 rn)
{
	uint32 opcode = 0x91000000;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	WriteWord(opcode);
}

void CAArch64Assembler::Movi_4s(REGISTERMD rd, uint8 imm, MOVI_4S_IMM_SHIFT_TYPE shiftType)
{
	uint32 cmode = 0;
	switch(shiftType)
	{
	case MOVI_4S_IMM_SHIFT_LSL0:
		cmode = 0;
		break;
	case MOVI_4S_IMM_SHIFT_LSL24:
		cmode = 6;
		break;
	default:
		assert(false);
		break;
	}
	uint32 immHi = (imm >> 5);
	uint32 immLo = (imm & 0x1F);
	uint32 opcode = 0x4F000400;
	opcode |= (rd << 0);
	opcode |= (cmode << 12);
	opcode |= (immHi << 16);
	opcode |= (immLo << 5);
	WriteWord(opcode);
}

void CAArch64Assembler::Movn(REGISTER32 rd, uint16 imm, uint8 pos)
{
	assert(pos < 2);
	WriteMoveWideOpImm(0x12800000, pos, imm, rd);
}

void CAArch64Assembler::Movk(REGISTER32 rd, uint16 imm, uint8 pos)
{
	assert(pos < 2);
	WriteMoveWideOpImm(0x72800000, pos, imm, rd);
}

void CAArch64Assembler::Movk(REGISTER64 rd, uint16 imm, uint8 pos)
{
	assert(pos < 4);
	WriteMoveWideOpImm(0xF2800000, pos, imm, rd);
}

void CAArch64Assembler::Movz(REGISTER32 rd, uint16 imm, uint8 pos)
{
	assert(pos < 2);
	WriteMoveWideOpImm(0x52800000, pos, imm, rd);
}

void CAArch64Assembler::Movz(REGISTER64 rd, uint16 imm, uint8 pos)
{
	assert(pos < 4);
	WriteMoveWideOpImm(0xD2800000, pos, imm, rd);
}

void CAArch64Assembler::Mrs_Fpcr(REGISTER64 rt)
{
	uint32 opcode = 0xD5300000;
	uint32 fpcrOp = 0xDA20;
	opcode |= (rt << 0);
	opcode |= (fpcrOp << 5);
	WriteWord(opcode);
}

void CAArch64Assembler::Msr_Fpcr(REGISTER64 rt)
{
	uint32 opcode = 0xD5100000;
	uint32 fpcrOp = 0xDA20;
	opcode |= (rt << 0);
	opcode |= (fpcrOp << 5);
	WriteWord(opcode);
}

void CAArch64Assembler::Msub(REGISTER32 rd, REGISTER32 rn, REGISTER32 rm, REGISTER32 ra)
{
	uint32 opcode = 0x1B008000;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (ra << 10);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Mvn(REGISTER32 rd, REGISTER32 rm)
{
	uint32 opcode = 0x2A200000;
	opcode |= (rd << 0);
	opcode |= (wZR << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Mvn_16b(REGISTERMD rd, REGISTERMD rn)
{
	uint32 opcode = 0x6E205800;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	WriteWord(opcode);
}

void CAArch64Assembler::Mvni_4s(REGISTERMD rd, uint8 imm, MOVI_4S_IMM_SHIFT_TYPE shiftType)
{
	uint32 cmode = 0;
	switch(shiftType)
	{
	case MOVI_4S_IMM_SHIFT_LSL0:
		cmode = 0;
		break;
	case MOVI_4S_IMM_SHIFT_LSL16:
		cmode = 4;
		break;
	case MOVI_4S_IMM_SHIFT_LSL24:
		cmode = 6;
		break;
	default:
		assert(false);
		break;
	}
	uint32 immHi = (imm >> 5);
	uint32 immLo = (imm & 0x1F);
	uint32 opcode = 0x6F000400;
	opcode |= (rd << 0);
	opcode |= (cmode << 12);
	opcode |= (immHi << 16);
	opcode |= (immLo << 5);
	WriteWord(opcode);
}

void CAArch64Assembler::Orn_16b(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
	uint32 opcode = 0x4EE01C00;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Orr(REGISTER32 rd, REGISTER32 rn, REGISTER32 rm)
{
	uint32 opcode = 0x2A000000;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Orr(REGISTER32 rd, REGISTER32 rn, uint8 n, uint8 immr, uint8 imms)
{
	WriteLogicalOpImm(0x32000000, n, immr, imms, rn, rd);
}

void CAArch64Assembler::Orr_16b(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
	uint32 opcode = 0x4EA01C00;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Ret(REGISTER64 rn)
{
	uint32 opcode = 0xD65F0000;
	opcode |= (rn << 5);
	WriteWord(opcode);
}

void CAArch64Assembler::Scvtf_1s(REGISTERMD rd, REGISTERMD rn)
{
	uint32 opcode = 0x5E21D800;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	WriteWord(opcode);
}

void CAArch64Assembler::Scvtf_4s(REGISTERMD rd, REGISTERMD rn)
{
	uint32 opcode = 0x4E21D800;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	WriteWord(opcode);
}

void CAArch64Assembler::Sdiv(REGISTER32 rd, REGISTER32 rn, REGISTER32 rm)
{
	uint32 opcode = 0x1AC00C00;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Shl_4s(REGISTERMD rd, REGISTERMD rn, uint8 sa)
{
	uint8 immhb = (sa & 0x1F) + 32;
	uint32 opcode = 0x4F005400;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (immhb << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Shl_8h(REGISTERMD rd, REGISTERMD rn, uint8 sa)
{
	uint8 immhb = (sa & 0xF) + 16;
	uint32 opcode = 0x4F005400;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (immhb << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Smax_4s(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
	uint32 opcode = 0x4EA06400;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Smax_8h(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
	uint32 opcode = 0x4E606400;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Smin_4s(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
	uint32 opcode = 0x4EA06C00;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Smin_8h(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
	uint32 opcode = 0x4E606C00;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Smull(REGISTER64 rd, REGISTER32 rn, REGISTER32 rm)
{
	uint32 opcode = 0x9B200000;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (wZR << 10);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Sshr_4s(REGISTERMD rd, REGISTERMD rn, uint8 sa)
{
	uint8 immhb = (32 * 2) - (sa & 0x1F);
	uint32 opcode = 0x4F000400;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (immhb << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Sshr_8h(REGISTERMD rd, REGISTERMD rn, uint8 sa)
{
	uint8 immhb = (16 * 2) - (sa & 0xF);
	uint32 opcode = 0x4F000400;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (immhb << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Sqadd_4s(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
	uint32 opcode = 0x4EA00C00;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Sqadd_8h(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
	uint32 opcode = 0x4E600C00;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Sqadd_16b(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
	uint32 opcode = 0x4E200C00;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Sqsub_4s(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
	uint32 opcode = 0x4EA02C00;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Sqsub_8h(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
	uint32 opcode = 0x4E602C00;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::St1_4s(REGISTERMD rt, REGISTER64 rn)
{
	uint32 opcode = 0x4C007800;
	opcode |= (rt << 0);
	opcode |= (rn << 5);
	WriteWord(opcode);
}

void CAArch64Assembler::Stp(REGISTER32 rt, REGISTER32 rt2, REGISTER64 rn, int32 offset)
{
	assert((offset & 0x03) == 0);
	int32 scaledOffset = offset / 4;
	assert(scaledOffset >= -64 && scaledOffset <= 63);
	uint32 opcode = 0x29000000;
	opcode |= (rt << 0);
	opcode |= (rn << 5);
	opcode |= (rt2 << 10);
	opcode |= ((scaledOffset & 0x7F) << 15);
	WriteWord(opcode);
}

void CAArch64Assembler::Stp_PreIdx(REGISTER64 rt, REGISTER64 rt2, REGISTER64 rn, int32 offset)
{
	assert((offset & 0x07) == 0);
	int32 scaledOffset = offset / 8;
	assert(scaledOffset >= -64 && scaledOffset <= 63);
	uint32 opcode = 0xA9800000;
	opcode |= (rt << 0);
	opcode |= (rn << 5);
	opcode |= (rt2 << 10);
	opcode |= ((scaledOffset & 0x7F) << 15);
	WriteWord(opcode);
}

void CAArch64Assembler::Str(REGISTER32 rt, REGISTER64 rn, uint32 offset)
{
	assert((offset & 0x03) == 0);
	uint32 scaledOffset = offset / 4;
	assert(scaledOffset < 0x1000);
	WriteLoadStoreOpImm(0xB9000000, scaledOffset, rn, rt);
}

void CAArch64Assembler::Str(REGISTER32 rt, REGISTER64 rn, REGISTER64 rm, bool scaled)
{
	uint32 opcode = 0xB8206800;
	opcode |= (rt << 0);
	opcode |= (rn << 5);
	opcode |= scaled ? (1 << 12) : 0;
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Str(REGISTER64 rt, REGISTER64 rn, uint32 offset)
{
	assert((offset & 0x07) == 0);
	uint32 scaledOffset = offset / 8;
	assert(scaledOffset < 0x1000);
	WriteLoadStoreOpImm(0xF9000000, scaledOffset, rn, rt);
}

void CAArch64Assembler::Str(REGISTER64 rt, REGISTER64 rn, REGISTER64 rm, bool scaled)
{
	uint32 opcode = 0xF8206800;
	opcode |= (rt << 0);
	opcode |= (rn << 5);
	opcode |= scaled ? (1 << 12) : 0;
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Strb(REGISTER32 rt, REGISTER64 rn, uint32 offset)
{
	uint32 scaledOffset = offset;
	assert(scaledOffset < 0x1000);
	WriteLoadStoreOpImm(0x39000000, scaledOffset, rn, rt);
}

void CAArch64Assembler::Strb(REGISTER32 rt, REGISTER64 rn, REGISTER64 rm, bool scaled)
{
	uint32 opcode = 0x38206800;
	opcode |= (rt << 0);
	opcode |= (rn << 5);
	opcode |= scaled ? (1 << 12) : 0;
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Strh(REGISTER32 rt, REGISTER64 rn, uint32 offset)
{
	assert((offset & 0x01) == 0);
	uint32 scaledOffset = offset / 2;
	assert(scaledOffset < 0x1000);
	WriteLoadStoreOpImm(0x79000000, scaledOffset, rn, rt);
}

void CAArch64Assembler::Strh(REGISTER32 rt, REGISTER64 rn, REGISTER64 rm, bool scaled)
{
	uint32 opcode = 0x78206800;
	opcode |= (rt << 0);
	opcode |= (rn << 5);
	opcode |= scaled ? (1 << 12) : 0;
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Str_1s(REGISTERMD rt, REGISTER64 rn, uint32 offset)
{
	assert((offset & 0x03) == 0);
	uint32 scaledOffset = offset / 4;
	assert(scaledOffset < 0x1000);
	WriteLoadStoreOpImm(0xBD000000, scaledOffset, rn, rt);
}

void CAArch64Assembler::Str_1q(REGISTERMD rt, REGISTER64 rn, uint32 offset)
{
	assert((offset & 0x0F) == 0);
	uint32 scaledOffset = offset / 0x10;
	assert(scaledOffset < 0x1000);
	WriteLoadStoreOpImm(0x3D800000, scaledOffset, rn, rt);
}

void CAArch64Assembler::Str_1q(REGISTERMD rt, REGISTER64 rn, REGISTER64 rm, bool scaled)
{
	uint32 opcode = 0x3CA04800;
	opcode |= (rt << 0);
	opcode |= (rn << 5);
	opcode |= scaled ? (1 << 12) : 0;
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Sub(REGISTER32 rd, REGISTER32 rn, REGISTER32 rm)
{
	uint32 opcode = 0x4B000000;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Sub(REGISTER64 rd, REGISTER64 rn, REGISTER64 rm)
{
	uint32 opcode = 0xCB000000;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Sub(REGISTER32 rd, REGISTER32 rn, uint16 imm, ADDSUB_IMM_SHIFT_TYPE shift)
{
	WriteAddSubOpImm(0x51000000, shift, imm, rn, rd);
}

void CAArch64Assembler::Sub(REGISTER64 rd, REGISTER64 rn, uint16 imm, ADDSUB_IMM_SHIFT_TYPE shift)
{
	WriteAddSubOpImm(0xD1000000, shift, imm, rn, rd);
}

void CAArch64Assembler::Sub_4s(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
	uint32 opcode = 0x6EA08400;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Sub_8h(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
	uint32 opcode = 0x6E608400;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Sub_16b(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
	uint32 opcode = 0x6E208400;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Tbl(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
	uint32 opcode = 0x4E002000;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Tst(REGISTER32 rn, REGISTER32 rm)
{
	uint32 opcode = 0x6A000000;
	opcode |= (wZR << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Tst(REGISTER64 rn, REGISTER64 rm)
{
	uint32 opcode = 0xEA000000;
	opcode |= (wZR << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Uaddlv_8h(REGISTERMD rd, REGISTERMD rn)
{
	uint32 opcode = 0x6E703800;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	WriteWord(opcode);
}

void CAArch64Assembler::Uaddlv_16b(REGISTERMD rd, REGISTERMD rn)
{
	uint32 opcode = 0x6E303800;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	WriteWord(opcode);
}

void CAArch64Assembler::Udiv(REGISTER32 rd, REGISTER32 rn, REGISTER32 rm)
{
	uint32 opcode = 0x1AC00800;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Umin_4s(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
	uint32 opcode = 0x6EA06C00;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Umov_1s(REGISTER32 rd, REGISTERMD rn, uint8 index)
{
	assert(index < 4);
	uint8 imm5 = 0x4 | ((index & 3) << 3);
	uint32 opcode = 0x0E003C00;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (imm5 << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Umull(REGISTER64 rd, REGISTER32 rn, REGISTER32 rm)
{
	uint32 opcode = 0x9BA00000;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (wZR << 10);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Uqadd_4s(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
	uint32 opcode = 0x6EA00C00;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Uqadd_8h(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
	uint32 opcode = 0x6E600C00;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Uqadd_16b(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
	uint32 opcode = 0x6E200C00;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Uqsub_4s(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
	uint32 opcode = 0x6EA02C00;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Uqsub_8h(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
	uint32 opcode = 0x6E602C00;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Uqsub_16b(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
	uint32 opcode = 0x6E202C00;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Ushr_4s(REGISTERMD rd, REGISTERMD rn, uint8 sa)
{
	uint8 immhb = (32 * 2) - (sa & 0x1F);
	uint32 opcode = 0x6F000400;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (immhb << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Ushr_8h(REGISTERMD rd, REGISTERMD rn, uint8 sa)
{
	uint8 immhb = (16 * 2) - (sa & 0xF);
	uint32 opcode = 0x6F000400;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (immhb << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Xtn1_4h(REGISTERMD rd, REGISTERMD rn)
{
	uint32 opcode = 0x0E612800;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	WriteWord(opcode);
}

void CAArch64Assembler::Xtn1_8b(REGISTERMD rd, REGISTERMD rn)
{
	uint32 opcode = 0x0E212800;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	WriteWord(opcode);
}

void CAArch64Assembler::Xtn2_8h(REGISTERMD rd, REGISTERMD rn)
{
	uint32 opcode = 0x4E612800;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	WriteWord(opcode);
}

void CAArch64Assembler::Xtn2_16b(REGISTERMD rd, REGISTERMD rn)
{
	uint32 opcode = 0x4E212800;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	WriteWord(opcode);
}

void CAArch64Assembler::Zip1_4s(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
	uint32 opcode = 0x4E803800;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Zip1_8h(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
	uint32 opcode = 0x4E403800;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Zip1_16b(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
	uint32 opcode = 0x4E003800;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Zip2_4s(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
	uint32 opcode = 0x4E807800;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Zip2_8h(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
	uint32 opcode = 0x4E407800;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Zip2_16b(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
	uint32 opcode = 0x4E007800;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::WriteAddSubOpImm(uint32 opcode, uint32 shift, uint32 imm, uint32 rn, uint32 rd)
{
	assert(imm < 0x1000);
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= ((imm & 0xFFF) << 10);
	opcode |= (shift << 22);
	WriteWord(opcode);
}

void CAArch64Assembler::WriteDataProcOpReg2(uint32 opcode, uint32 rm, uint32 rn, uint32 rd)
{
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::WriteLogicalOpImm(uint32 opcode, uint32 n, uint32 immr, uint32 imms, uint32 rn, uint32 rd)
{
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (imms << 10);
	opcode |= (immr << 16);
	opcode |= (n << 22);
	WriteWord(opcode);
}

void CAArch64Assembler::WriteLoadStoreOpImm(uint32 opcode, uint32 imm, uint32 rn, uint32 rt)
{
	opcode |= (rt << 0);
	opcode |= (rn << 5);
	opcode |= (imm << 10);
	WriteWord(opcode);
}

void CAArch64Assembler::WriteMoveWideOpImm(uint32 opcode, uint32 hw, uint32 imm, uint32 rd)
{
	opcode |= (rd << 0);
	opcode |= (imm << 5);
	opcode |= (hw << 21);
	WriteWord(opcode);
}

void CAArch64Assembler::WriteWord(uint32 value)
{
	m_stream->Write32(value);
}
