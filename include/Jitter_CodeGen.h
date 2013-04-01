#ifndef _JITTER_CODEGEN_H_
#define _JITTER_CODEGEN_H_

#include "Stream.h"
#include "Jitter_Statement.h"
#include <map>
#include <functional>

namespace Jitter
{
	class CCodeGen
	{
	public:
		typedef std::function<void (void*, uint32)> ExternalSymbolReferencedHandler;

		virtual					~CCodeGen() {};

		virtual void			SetStream(Framework::CStream*) = 0;
		void					SetExternalSymbolReferencedHandler(const ExternalSymbolReferencedHandler&);

		virtual void			GenerateCode(const StatementList&, unsigned int) = 0;
		virtual unsigned int	GetAvailableRegisterCount() const = 0;
		virtual bool			CanHold128BitsReturnValueInRegisters() const = 0;

	protected:
		enum MATCHTYPE
		{
			MATCH_ANY,
			MATCH_NIL,

			MATCH_CONTEXT,
			MATCH_CONSTANT,
			MATCH_REGISTER,
			MATCH_RELATIVE,
			MATCH_TEMPORARY,
			MATCH_MEMORY,

			MATCH_REL_REF,
			MATCH_TMP_REF,
			MATCH_MEM_REF,

			MATCH_RELATIVE64,
			MATCH_TEMPORARY64,
			MATCH_CONSTANT64,
			MATCH_MEMORY64,

			MATCH_RELATIVE128,
			MATCH_TEMPORARY128,
			MATCH_MEMORY128,

			MATCH_MEMORY256,

			MATCH_RELATIVE_FP_SINGLE,
			MATCH_MEMORY_FP_SINGLE,

			MATCH_RELATIVE_FP_INT32,
		};

		typedef std::function<void (const STATEMENT&)> CodeEmitterType;

		struct MATCHER
		{
			OPERATION			op;
			MATCHTYPE			dstType;
			MATCHTYPE			src1Type;
			MATCHTYPE			src2Type;
			CodeEmitterType		emitter;
		};

		typedef std::multimap<OPERATION, MATCHER> MatcherMapType;

		bool								SymbolMatches(MATCHTYPE, const SymbolRefPtr&);

		MatcherMapType						m_matchers;
		ExternalSymbolReferencedHandler		m_externalSymbolReferencedHandler;
	};
}

#endif
