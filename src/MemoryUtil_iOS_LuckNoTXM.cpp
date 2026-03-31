#include "MemoryUtil.h"

#include <mach/mach.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstdint>

namespace CodeGen
{

	void* AllocateExecutableMemory_LuckNoTXM(size_t size)
	{
		// Allouer région RX
		uint8_t* rx_ptr = static_cast<uint8_t*>(
		    mmap(nullptr, size, PROT_READ | PROT_EXEC, MAP_ANON | MAP_PRIVATE, -1, 0));

		if(!rx_ptr || rx_ptr == MAP_FAILED) return nullptr;
		return rx_ptr;
	}

	void FreeExecutableMemory_LuckNoTXM(void* ptr, size_t size)
	{
		if(ptr)
		{
			munmap(ptr, size);
		}
	}

	ptrdiff_t AllocateWritableRegionAndGetDiff_LuckNoTXM(void* rx_ptr, size_t size)
	{
		// Créer miroir RW de rx_ptr
		vm_address_t rw_region = 0;
		vm_address_t target = reinterpret_cast<vm_address_t>(rx_ptr);
		vm_prot_t cur_protection = 0;
		vm_prot_t max_protection = 0;

		kern_return_t retval = vm_remap(
		    mach_task_self(), &rw_region, size, 0, true,
		    mach_task_self(), target, false,
		    &cur_protection, &max_protection, VM_INHERIT_DEFAULT);

		if(retval != KERN_SUCCESS) return 0;

		uint8_t* rw_ptr = reinterpret_cast<uint8_t*>(rw_region);

		if(mprotect(rw_ptr, size, PROT_READ | PROT_WRITE) != 0)
		{
			vm_deallocate(mach_task_self(), rw_region, size);
			return 0;
		}

		return rw_ptr - static_cast<uint8_t*>(rx_ptr);
	}

	void FreeWritableRegion_LuckNoTXM(void* rx_ptr, size_t size, ptrdiff_t diff)
	{
		uint8_t* rw_ptr = static_cast<uint8_t*>(rx_ptr) + diff;
		vm_deallocate(mach_task_self(), reinterpret_cast<vm_address_t>(rw_ptr), size);
	}

} // namespace CodeGen
