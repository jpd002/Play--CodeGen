#include "Jitter_CodeGenFactory.h"

#ifdef WIN32

	#ifdef _M_X64
		#include "Jitter_CodeGen_x86_64.h"
	#else
		#include "Jitter_CodeGen_x86_32.h"
	#endif

#elif defined(__APPLE__)

	#include "TargetConditionals.h"
	#if TARGET_CPU_ARM
		#include "Jitter_CodeGen_Arm.h"
	#else
		#include "Jitter_CodeGen_x86_64.h"
	#endif

#endif

Jitter::CCodeGen* Jitter::CreateCodeGen()
{
#ifdef WIN32
	
	#ifdef _M_X64
		return new Jitter::CCodeGen_x86_64();
	#else
		return new Jitter::CCodeGen_x86_32();
	#endif
	
#elif defined(__APPLE__)
	
	#if TARGET_CPU_ARM
		return new Jitter::CCodeGen_Arm();
	#else
		return new Jitter::CCodeGen_x86_64();
	#endif

#endif
}
