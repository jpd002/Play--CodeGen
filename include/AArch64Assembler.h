#pragma once

#include <map>
#include "Stream.h"

class CAArch64Assembler
{
public:
	enum REGISTER32
	{
		w0,  w1,  w2,  w3,
		w4,  w5,  w6,  w7,
		w8,  w9,  w10, w11,
		w12, w13, w14, w15,
		w16, w17, w18, w19,
		w20, w21, w22, w23,
		w24, w25, w26, w27,
		w28, w29, w30, wZR
	};

	enum REGISTER64
	{
		x0,  x1,  x2,  x3,
		x4,  x5,  x6,  x7,
		x8,  x9,  x10, x11,
		x12, x13, x14, x15,
		x16, x17, x18, x19,
		x20, x21, x22, x23,
		x24, x25, x26, x27,
		x28, x29, x30, xZR, xSP = 31
	};

	typedef unsigned int LABEL;
	
	           CAArch64Assembler();
	virtual    ~CAArch64Assembler();

	void    SetStream(Framework::CStream*);

	LABEL    CreateLabel();
	void     ClearLabels();
	void     MarkLabel(LABEL);
	void     ResolveLabelReferences();

	void    Asr(REGISTER32, REGISTER32, uint8);
	void    Ldr(REGISTER32, REGISTER64, uint32);
	void    Mov(REGISTER32, REGISTER32);
	void    Mov(REGISTER64, REGISTER64);
	void    Ret(REGISTER64 = x30);
	void    Str(REGISTER32, REGISTER64, uint32);
	
private:
	typedef size_t LABELREF;
	
	typedef std::map<LABEL, size_t> LabelMapType;
	typedef std::multimap<LABEL, LABELREF> LabelReferenceMapType;
	
	void    WriteWord(uint32);
	
	unsigned int             m_nextLabelId = 1;
	LabelMapType             m_labels;
	LabelReferenceMapType    m_labelReferences;
	
	Framework::CStream*    m_stream = nullptr;
};
