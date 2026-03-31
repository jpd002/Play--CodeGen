#include "MemoryUtil.h"
#include "JITMemoryTracker.h"

#include <sys/mman.h>
#include <unistd.h>

namespace CodeGen
{

	static JITMemoryTracker g_jit_memory_tracker;

	void* AllocateExecutableMemory_Legacy(size_t size)
	{
		void* ptr = mmap(nullptr, size, PROT_READ | PROT_EXEC,
		                 MAP_ANON | MAP_PRIVATE, -1, 0);

		if(ptr == MAP_FAILED) ptr = nullptr;
		if(ptr == nullptr) return nullptr;

		g_jit_memory_tracker.RegisterJITRegion(ptr, size);
		return ptr;
	}

	void FreeExecutableMemory_Legacy(void* ptr, size_t size)
	{
		if(ptr)
		{
			munmap(ptr, size);
			g_jit_memory_tracker.UnregisterJITRegion(ptr);
		}
	}

	void JITPageWriteEnableExecuteDisable_Legacy(void* ptr)
	{
		g_jit_memory_tracker.JITRegionWriteEnableExecuteDisable(ptr);
	}

	void JITPageWriteDisableExecuteEnable_Legacy(void* ptr)
	{
		g_jit_memory_tracker.JITRegionWriteDisableExecuteEnable(ptr);
	}

} // namespace CodeGen
