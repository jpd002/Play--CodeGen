#pragma once

#include <map>
#include "Literal128.h"
#include "Stream.h"

class CLiteralPool
{
public:
	CLiteralPool(Framework::CStream*);

	void AlignPool();
	uint64 GetLiteralPosition(const LITERAL128&);

private:
	Framework::CStream* m_stream;
	std::map<LITERAL128, uint64> m_literalPositions;
};
