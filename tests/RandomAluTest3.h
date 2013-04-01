#ifndef _RANDOMALUTEST3_H_
#define _RANDOMALUTEST3_H_

#include "Test.h"
#include "MemoryFunction.h"

class CRandomAluTest3 : public CTest
{
public:
						CRandomAluTest3(bool);
	virtual				~CRandomAluTest3();

	void				Compile(Jitter::CJitter&);
	void				Run();

private:
	struct CONTEXT
	{
		uint32 number1;
		uint32 number2;
	};

	bool				m_useConstant;
	CONTEXT				m_context;
	CMemoryFunction		m_function;
};

#endif
