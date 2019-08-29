#include "LiteralPool.h"

CLiteralPool::CLiteralPool(Framework::CStream* stream)
: m_stream(stream)
{

}

void CLiteralPool::AlignPool()
{
	unsigned int alignSize = m_stream->Tell() & 0x0F;
	if(alignSize != 0)
	{
		LITERAL128 tempLit(0, 0);
		m_stream->Write(&tempLit, 0x10 - alignSize);
	}
}

uint64 CLiteralPool::GetLiteralPosition(const LITERAL128& literal)
{
	auto literalPosIterator = m_literalPositions.find(literal);
	if(literalPosIterator == std::end(m_literalPositions))
	{
		m_stream->Seek(0, Framework::STREAM_SEEK_END);
		uint32 literalPos = m_stream->Tell();
		m_stream->Write64(literal.lo);
		m_stream->Write64(literal.hi);
		m_literalPositions.insert(std::make_pair(literal, literalPos));
		return literalPos;
	}
	else
	{
		return literalPosIterator->second;
	}
}
