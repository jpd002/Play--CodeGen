#pragma once

#include <boost/utility.hpp>
#include "Types.h"

class CMemoryFunction : public boost::noncopyable
{
public:
						CMemoryFunction();
						CMemoryFunction(const void*, size_t);
						CMemoryFunction(CMemoryFunction&&);

	virtual				~CMemoryFunction();

	bool				IsEmpty() const;

	CMemoryFunction&	operator =(CMemoryFunction&&);
	void				operator()(void*);

	void*				GetCode() const;
	size_t				GetSize() const;

private:
	void				Reset();

	void*				m_code;
	size_t				m_size;
};
