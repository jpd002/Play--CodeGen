#pragma once

#include "Types.h"

class CMemoryFunction
{
public:
						CMemoryFunction();
						CMemoryFunction(const void*, size_t);
						CMemoryFunction(const CMemoryFunction&) = delete;
						CMemoryFunction(CMemoryFunction&&);

	virtual				~CMemoryFunction();

	bool				IsEmpty() const;

	CMemoryFunction&	operator =(const CMemoryFunction&) = delete;

	CMemoryFunction&	operator =(CMemoryFunction&&);
	void				operator()(void*, bool = false);

	void*				GetCode() const;
	size_t				GetSize() const;

	void				BeginModify();
	void				EndModify();
	
private:
	void				ClearCache();
	void				Reset();

	void*				m_code;
	size_t				m_size;
};
