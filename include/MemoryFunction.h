#ifndef _MEMORYFUNCTION_H_
#define _MEMORYFUNCTION_H_

#include <boost/utility.hpp>
#include "Types.h"

class CMemoryFunction : public boost::noncopyable
{
public:
					CMemoryFunction(const void*, size_t);
	virtual			~CMemoryFunction();

	void			operator()(void*);

	void*			GetCode() const;
	size_t			GetSize() const;

private:
	void*			m_code;
	size_t			m_size;
};

#endif
