#pragma once

#include "Jitter_CodeGen.h"
//#include "AArch64Assembler.h"

namespace Jitter
{
	class CCodeGen_AArch64 : public CCodeGen
	{
	public:
		           CCodeGen_AArch64();
		virtual    ~CCodeGen_AArch64();

		void            GenerateCode(const StatementList&, unsigned int) override;
		void            SetStream(Framework::CStream*) override;
		void            RegisterExternalSymbols(CObjectFile*) const override;
		unsigned int    GetAvailableRegisterCount() const override;
		unsigned int    GetAvailableMdRegisterCount() const override;
		bool            CanHold128BitsReturnValueInRegisters() const override;

	private:
		Framework::CStream*    m_stream = nullptr;
//		CAArch32Assembler      m_assembler;
	};
};
