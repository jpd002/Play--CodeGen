#include "MemoryUtil.h"

#include <lwmem/lwmem.h>
#include <mach/mach.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstdint>

// 512 MiB de région exécutable pré-allouée
constexpr size_t EXECUTABLE_REGION_SIZE = 536870912;

static uint8_t* g_rx_region = nullptr; // Pointeur région RX (exécutable)
static ptrdiff_t g_rw_region_diff = 0; // Décalage RW - RX

namespace CodeGen
{

	void AllocateExecutableMemoryRegion_LuckTXM()
	{
		if(g_rx_region) return; // Déjà alloué

		const size_t size = EXECUTABLE_REGION_SIZE;

		// 1. Allouer région RX (read-execute)
		uint8_t* rx_ptr = static_cast<uint8_t*>(
		    mmap(nullptr, size, PROT_READ | PROT_EXEC, MAP_ANON | MAP_PRIVATE, -1, 0));

		if(!rx_ptr || rx_ptr == MAP_FAILED)
		{
			// TODO: Log error
			return;
		}

		// 2. Signal TXM avec breakpoint spécial (ARM64)
		// Cette instruction informe le Trusted Execution Monitor de la région JIT
		asm("mov x0, %0\n"
		    "mov x1, %1\n"
		    "brk #0x69" ::"r"(rx_ptr),
		    "r"(size)
		    : "x0", "x1");

		// 3. Créer miroir RW de la même mémoire physique
		vm_address_t rw_region = 0;
		vm_address_t target = reinterpret_cast<vm_address_t>(rx_ptr);
		vm_prot_t cur_protection = 0;
		vm_prot_t max_protection = 0;

		kern_return_t retval = vm_remap(
		    mach_task_self(), // Task cible
		    &rw_region,       // Adresse de sortie (miroir)
		    size,             // Taille
		    0,                // Mask
		    true,             // Anywhere (laisse kernel choisir l'adresse)
		    mach_task_self(), // Task source
		    target,           // Adresse source (rx_ptr)
		    false,            // Copy (false = partage mémoire physique)
		    &cur_protection,
		    &max_protection,
		    VM_INHERIT_DEFAULT);

		if(retval != KERN_SUCCESS)
		{
			munmap(rx_ptr, size);
			return;
		}

		uint8_t* rw_ptr = reinterpret_cast<uint8_t*>(rw_region);

		// 4. Forcer permissions RW sur le miroir
		if(mprotect(rw_ptr, size, PROT_READ | PROT_WRITE) != 0)
		{
			munmap(rx_ptr, size);
			vm_deallocate(mach_task_self(), rw_region, size);
			return;
		}

		// 5. Initialiser lwmem pour gérer allocation dynamique
		lwmem_region_t regions[] = {
		    {(void*)rw_ptr, size},
		    {NULL, 0}};

		if(lwmem_assignmem(regions) == 0)
		{
			munmap(rx_ptr, size);
			vm_deallocate(mach_task_self(), rw_region, size);
			return;
		}

		g_rx_region = rx_ptr;
		g_rw_region_diff = rw_ptr - rx_ptr;
	}

	ptrdiff_t AllocateWritableRegionAndGetDiff_LuckTXM()
	{
		return g_rw_region_diff;
	}

	void* AllocateExecutableMemory_LuckTXM(size_t size)
	{
		if(g_rx_region == nullptr) return nullptr;

		const size_t pagesize = sysconf(_SC_PAGESIZE);

		// Allouer via lwmem avec espace pour alignement + métadonnées
		void* raw = lwmem_malloc(size + pagesize - 1 + sizeof(void*));

		if(!raw) return nullptr;

		// Aligner sur page boundary
		uintptr_t raw_addr = (uintptr_t)raw + sizeof(void*);
		uintptr_t aligned = (raw_addr + pagesize - 1) & ~(pagesize - 1);

		// Stocker pointeur raw pour lwmem_free()
		((void**)aligned)[-1] = raw;

		// Retourner pointeur RX (exécutable) au lieu du RW
		return (uint8_t*)aligned - g_rw_region_diff;
	}

	void FreeExecutableMemory_LuckTXM(void* ptr)
	{
		if(!ptr) return;

		// Convertir ptr RX en RW, récupérer raw, libérer
		uint8_t* rw_ptr = static_cast<uint8_t*>(ptr) + g_rw_region_diff;
		void* raw = ((void**)rw_ptr)[-1];
		lwmem_free(raw);
	}

} // namespace CodeGen
