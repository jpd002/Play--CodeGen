#include "Singleton.h"
#include "MemoryFunction.h"

namespace Jitter
{
	class CJitter_Trampoline : public CSingleton<CJitter_Trampoline>
	{
		public:
			CJitter_Trampoline();
			~CJitter_Trampoline() = default;

			void Trampoline(void*, void*);

		private:
			struct CONTEXT
			{
				void* context;
				void* code;
			};
			void SetupTrumpoline();

			CONTEXT m_context;
			CMemoryFunction m_function;

	};
};
