#ifndef _JITTER_CODEGEN_X86_32_H_
#define _JITTER_CODEGEN_X86_32_H_

#include "Jitter_CodeGen_x86.h"

namespace Jitter
{
	class CCodeGen_x86_32 : public CCodeGen_x86
	{
	public:
											CCodeGen_x86_32();
		virtual								~CCodeGen_x86_32();

		unsigned int						GetAvailableRegisterCount() const;

	protected:
		virtual void						Emit_Prolog(unsigned int, uint32);
		virtual void						Emit_Epilog(unsigned int, uint32);

		//PARAM
		void								Emit_Param_Ctx(const STATEMENT&);
		void								Emit_Param_Rel(const STATEMENT&);
		void								Emit_Param_Cst(const STATEMENT&);
		void								Emit_Param_Reg(const STATEMENT&);
		void								Emit_Param_Tmp(const STATEMENT&);

		//CALL
		void								Emit_Call(const STATEMENT&);

		//RETURNVALUE
		void								Emit_RetVal_Tmp(const STATEMENT&);
		void								Emit_RetVal_Reg(const STATEMENT&);

		//ADD64
		void								Emit_Add64_RelRelRel(const STATEMENT&);

		//AND64
		void								Emit_And64_RelRelRel(const STATEMENT&);

		//SRL64
		void								Emit_Srl64_RelRelCst(const STATEMENT&);

		//SLL64
		void								Emit_Sll64_RelRelCst(const STATEMENT&);

		//CMP64
		void								Cmp64_Equal(const STATEMENT&);
		void								Cmp64_LessThan(const STATEMENT&);
		void								Cmp64_GenericRelRel(const STATEMENT&);
		void								Emit_Cmp64_RegRelRel(const STATEMENT&);
		void								Emit_Cmp64_RelRelRel(const STATEMENT&);
		void								Emit_Cmp64_TmpRelRel(const STATEMENT&);

	private:
		typedef void (CCodeGen_x86_32::*ConstCodeEmitterType)(const STATEMENT&);

		struct CONSTMATCHER
		{
			OPERATION				op;
			MATCHTYPE				dstType;
			MATCHTYPE				src1Type;
			MATCHTYPE				src2Type;
			ConstCodeEmitterType	emitter;
		};

		static CONSTMATCHER					g_constMatchers[];
		static CX86Assembler::REGISTER		g_registers[];
	};
}

#endif
