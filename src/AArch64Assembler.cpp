#include <assert.h>
#include <stdexcept>
#include "AArch64Assembler.h"

CAArch64Assembler::CAArch64Assembler()
{

}

CAArch64Assembler::~CAArch64Assembler()
{
	
}

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

void CAArch64Assembler::ResolveLabelReferences()
{
#if 0
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
#endif
	m_labelReferences.clear();
}

void CAArch64Assembler::Asr(REGISTER32 rd, REGISTER32 rn, uint8 sa)
{
	uint32 opcode = 0x13000000;
	uint32 imms = 0x1F;
	uint32 immr = sa & 0x1F;
	opcode |= (rd << 0);
	opcode |= (rn << 5);
	opcode |= (imms << 10);
	opcode |= (immr << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Ldr(REGISTER32 rt, REGISTER64 rn, uint32 offset)
{
	assert((offset & 0x03) == 0);
	uint32 scaledOffset = offset / 4;
	assert(scaledOffset < 0x1000);
	uint32 opcode = 0xB9400000;
	opcode |= (rt << 0);
	opcode |= (rn << 5);
	opcode |= (scaledOffset << 10);
	WriteWord(opcode);
}

void CAArch64Assembler::Mov(REGISTER32 rd, REGISTER32 rm)
{
	uint32 opcode = 0x2A0003E0;
	opcode |= (rd <<  0);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Mov(REGISTER64 rd, REGISTER64 rm)
{
	uint32 opcode = 0xAA0003E0;
	opcode |= (rd <<  0);
	opcode |= (rm << 16);
	WriteWord(opcode);
}

void CAArch64Assembler::Ret(REGISTER64 rn)
{
	uint32 opcode = 0xD65F0000;
	opcode |= (rn << 5);
	WriteWord(opcode);
}

void CAArch64Assembler::Str(REGISTER32 rt, REGISTER64 rn, uint32 offset)
{
	assert((offset & 0x03) == 0);
	uint32 scaledOffset = offset / 4;
	assert(scaledOffset < 0x1000);
	uint32 opcode = 0xB9000000;
	opcode |= (rt << 0);
	opcode |= (rn << 5);
	opcode |= (scaledOffset << 10);
	WriteWord(opcode);
}

void CAArch64Assembler::WriteWord(uint32 value)
{
	m_stream->Write32(value);
}
