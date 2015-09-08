#pragma once

#include "Jitter_CodeGen.h"
#include "AArch64Assembler.h"

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
		typedef std::map<uint32, CAArch64Assembler::LABEL> LabelMapType;
		typedef void (CCodeGen_AArch64::*ConstCodeEmitterType)(const STATEMENT&);

		struct CONSTMATCHER
		{
			OPERATION               op;
			MATCHTYPE               dstType;
			MATCHTYPE               src1Type;
			MATCHTYPE               src2Type;
			ConstCodeEmitterType    emitter;
		};

		void    LoadRelativeInRegister(CAArch64Assembler::REGISTER32, CSymbol*);
		void    StoreRegisterInRelative(CSymbol*, CAArch64Assembler::REGISTER32);
		void    LoadTemporaryInRegister(CAArch64Assembler::REGISTER32, CSymbol*);
		void    StoreRegisterInTemporary(CSymbol*, CAArch64Assembler::REGISTER32);
		
		void    LoadMemoryInRegister(CAArch64Assembler::REGISTER32, CSymbol*);
		void    StoreRegisterInMemory(CSymbol*, CAArch64Assembler::REGISTER32);
		
		CAArch64Assembler::REGISTER32    PrepareSymbolRegisterDef(CSymbol*, CAArch64Assembler::REGISTER32);
		CAArch64Assembler::REGISTER32    PrepareSymbolRegisterUse(CSymbol*, CAArch64Assembler::REGISTER32);
		void                             CommitSymbolRegister(CSymbol*, CAArch64Assembler::REGISTER32);
		
		void    Emit_Prolog(uint32);
		void    Emit_Epilog(uint32);
		
		CAArch64Assembler::LABEL GetLabel(uint32);
		void                     MarkLabel(const STATEMENT&);
		
		void    Emit_Mov_VarVar(const STATEMENT&);
		
		//SHIFT
		void    Emit_Shift_Generic(const STATEMENT&);

		static CONSTMATCHER    g_constMatchers[];
		static CAArch64Assembler::REGISTER64    g_baseRegister;

		Framework::CStream*    m_stream = nullptr;
		CAArch64Assembler      m_assembler;
		LabelMapType           m_labels;
		uint32                 m_stackLevel = 0;
	};
};
