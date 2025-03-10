#pragma once

#include "Stream.h"
#include "Jitter_Statement.h"
#include <map>
#include <functional>

namespace Jitter
{
	class CObjectFile;

	class CCodeGen
	{
	public:
		enum class SYMBOL_REF_TYPE
		{
			NATIVE_POINTER,
			ARMV7_LOAD_HALF,
			ARMV8_PCRELATIVE,
		};

		typedef std::function<void(uintptr_t, uint32, SYMBOL_REF_TYPE)> ExternalSymbolReferencedHandler;

		virtual ~CCodeGen(){};

		virtual void SetStream(Framework::CStream*) = 0;
		void SetExternalSymbolReferencedHandler(const ExternalSymbolReferencedHandler&);

		virtual void GenerateCode(const StatementList&, unsigned int) = 0;
		virtual unsigned int GetAvailableRegisterCount() const = 0;
		virtual unsigned int GetAvailableMdRegisterCount() const = 0;
		virtual bool Has128BitsCallOperands() const = 0;
		virtual bool CanHold128BitsReturnValueInRegisters() const = 0;
		virtual bool SupportsExternalJumps() const = 0;
		virtual bool SupportsCmpSelect() const = 0;
		virtual void RegisterExternalSymbols(CObjectFile*) const = 0;
		virtual uint32 GetPointerSize() const = 0;

	protected:
		enum MATCHTYPE
		{
			MATCH_ANY,
			MATCH_NIL,

			MATCH_CONTEXT,
			MATCH_CONSTANTPTR,

			MATCH_CONSTANT,
			MATCH_REGISTER,
			MATCH_RELATIVE,
			MATCH_TEMPORARY,
			MATCH_MEMORY,   //Either relative or temporary
			MATCH_VARIABLE, //Either relative or temporary or register
			MATCH_ANY32,    //Any of the 32-bit sized operands (variable or constant)

			MATCH_REG_REF,
			MATCH_REL_REF,
			MATCH_TMP_REF,
			MATCH_MEM_REF,
			MATCH_VAR_REF,

			MATCH_RELATIVE64,
			MATCH_TEMPORARY64,
			MATCH_CONSTANT64,
			MATCH_MEMORY64,

			MATCH_REGISTER128,
			MATCH_RELATIVE128,
			MATCH_TEMPORARY128,
			MATCH_MEMORY128,
			MATCH_VARIABLE128,

			MATCH_MEMORY256,

			MATCH_FP_REGISTER32,
			MATCH_FP_RELATIVE32,
			MATCH_FP_TEMPORARY32,
			MATCH_FP_MEMORY32,
			MATCH_FP_VARIABLE32,
		};

		typedef std::function<void(const STATEMENT&)> CodeEmitterType;

		struct MATCHER
		{
			OPERATION op;
			MATCHTYPE dstType;
			MATCHTYPE src1Type;
			MATCHTYPE src2Type;
			MATCHTYPE src3Type = MATCH_NIL;
			CodeEmitterType emitter;
		};

		typedef std::multimap<OPERATION, MATCHER> MatcherMapType;

		bool SymbolMatches(MATCHTYPE, const SymbolRefPtr&);
		static uint32 GetRegisterUsage(const StatementList&);

		MatcherMapType m_matchers;
		ExternalSymbolReferencedHandler m_externalSymbolReferencedHandler;
	};
}
