#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <algorithm>
#include <cstdint>
#include "AlignedAlloc.h"
#include "MemoryFunction.h"

// clang-format off

#define BLOCK_ALIGN 0x10

#ifdef _WIN32
	#define MEMFUNC_USE_WIN32
#elif defined(__APPLE__)
#include "TargetConditionals.h"
#include <libkern/OSCacheControl.h>
#include <mach/mach_init.h>
#include <mach/vm_map.h>

// BreakpointJIT pour iOS 26 TXM
#if TARGET_OS_IPHONE
extern "C" {
    void* BreakGetJITMapping(void* addr, size_t len) __attribute__((weak_import));
}
#endif

#if TARGET_OS_OSX
#define MEMFUNC_USE_MMAP
#define MEMFUNC_MMAP_ADDITIONAL_FLAGS (MAP_JIT)
#if TARGET_CPU_ARM64
#define MEMFUNC_MMAP_REQUIRES_JIT_WRITE_PROTECT
#endif
#else
#define MEMFUNC_USE_MACHVM
#if TARGET_OS_IPHONE
#define MEMFUNC_MACHVM_STRICT_PROTECTION
#endif
#endif
#elif defined(__EMSCRIPTEN__)
	#include <emscripten.h>
	#define MEMFUNC_USE_WASM
#else
	#define MEMFUNC_USE_MMAP
#endif

#if defined(MEMFUNC_USE_WIN32)
#include <windows.h>
#elif defined(MEMFUNC_USE_MACHVM)
#include <mach/mach_init.h>
#include <mach/vm_map.h>
#elif defined(MEMFUNC_USE_MMAP)
#include <sys/mman.h>
#include <pthread.h>
#elif defined(MEMFUNC_USE_WASM)
EM_JS_DEPS(WasmMemoryFunction, "$addFunction,$removeFunction");
EM_JS(int, WasmCreateFunction, (emscripten::EM_VAL moduleHandle),
{
	let module = Emval.toValue(moduleHandle);
	let moduleInstance = new WebAssembly.Instance(module, {
		env: {
			memory: wasmMemory,
			fctTable : Module.codeGenImportTable
		}
	});
	let fct = moduleInstance.exports.codeGenFunc;
	let fctId = addFunction(fct, 'vi');
	return fctId;
});
EM_JS(void, WasmDeleteFunction, (int fctId),
{
	removeFunction(fctId);
});
EM_JS(emscripten::EM_VAL, WasmCreateModule, (uintptr_t code, uintptr_t size),
{
	//var fs = require('fs');
	let moduleBytes = HEAP8.subarray(code, code + size);
	//fs.writeFileSync('module.wasm', moduleBytes);
	//{
	//	let bytesCopy = new Uint8Array(moduleBytes);
	//	let blob = new Blob([bytesCopy], { type: "binary/octet-stream" });
	//	let url = URL.createObjectURL(blob);
	//	console.log(url);
	//}
	let module = new WebAssembly.Module(moduleBytes);
	return Emval.toHandle(module);
});
#else
#error "No API to use for CMemoryFunction"
#endif

CMemoryFunction::CMemoryFunction()
: m_code(nullptr)
, m_size(0)
{

}

CMemoryFunction::CMemoryFunction(const void* code, size_t size)
: m_code(nullptr)
, m_size(0)
{
#ifdef __APPLE__
    // Détecter mode TXM iOS 26
    const char* hasTxm = getenv("PLAY_HAS_TXM");
    m_ios26TxmMode = (hasTxm != nullptr) && (hasTxm[0] == '1');
    
#if TARGET_OS_IPHONE
    if(m_ios26TxmMode && BreakGetJITMapping != nullptr)
    {
        // Calculer taille alignée sur page
        vm_size_t page_size = 0;
        host_page_size(mach_task_self(), &page_size);
        if(page_size == 0) page_size = 16384; // Fallback ARM64
        size_t allocSize = ((size + page_size - 1) / page_size) * page_size;
        
        // Obtenir mémoire RX via BreakpointJIT
        void* rx = BreakGetJITMapping(nullptr, allocSize);
        
        if(rx != nullptr)
        {
            // Créer alias RW temporaire via vm_remap
            vm_address_t rwAddress = 0;
            vm_prot_t curProt = VM_PROT_NONE;
            vm_prot_t maxProt = VM_PROT_NONE;
            
            kern_return_t kr = vm_remap(
                mach_task_self(),
                &rwAddress,
                allocSize,
                0,
                VM_FLAGS_ANYWHERE,
                mach_task_self(),
                (vm_address_t)rx,
                FALSE,
                &curProt,
                &maxProt,
                VM_INHERIT_NONE
            );
            
            if(kr == KERN_SUCCESS)
            {
                // Rendre l'alias writable
                kern_return_t protResult = vm_protect(
                    mach_task_self(), 
                    rwAddress, 
                    allocSize, 
                    FALSE, 
                    VM_PROT_READ | VM_PROT_WRITE
                );
                
                if(protResult == KERN_SUCCESS)
                {
                    // Copier le code JIT dans l'alias RW
                    memcpy((void*)rwAddress, code, size);
                    
                    // Libérer l'alias RW (on garde seulement RX)
                    vm_deallocate(mach_task_self(), rwAddress, allocSize);
                    
                    // Stocker le pointeur RX pour exécution
                    m_code = rx;
                    m_rxMemory = rx;
                    m_size = allocSize;
                    ClearCache();
                    return; // Succès - ne pas exécuter le code legacy
                }
                
                // Échec vm_protect - cleanup
                vm_deallocate(mach_task_self(), rwAddress, allocSize);
            }
            
            // Échec vm_remap - la mémoire RX reste allouée par BreakpointJIT
            // On ne peut pas la libérer, mais on peut réessayer en mode legacy
        }
        
        // Échec BreakpointJIT - désactiver TXM mode
        m_ios26TxmMode = false;
    }
#endif // TARGET_OS_IPHONE
#endif // __APPLE__

    // ========== CODE LEGACY  ==========
  
#if defined(MEMFUNC_USE_WIN32)
	m_size = size;
	m_code = framework_aligned_alloc(size, BLOCK_ALIGN);
	memcpy(m_code, code, size);
	
	DWORD oldProtect = 0;
	BOOL result = VirtualProtect(m_code, size, PAGE_EXECUTE_READWRITE, &oldProtect);
	assert(result == TRUE);
#elif defined(MEMFUNC_USE_MACHVM)
	vm_size_t page_size = 0;
	host_page_size(mach_task_self(), &page_size);
	unsigned int allocSize = ((size + page_size - 1) / page_size) * page_size;
	vm_allocate(mach_task_self(), reinterpret_cast<vm_address_t*>(&m_code), allocSize, TRUE); 
	memcpy(m_code, code, size);
	vm_prot_t protection =
	#ifdef MEMFUNC_MACHVM_STRICT_PROTECTION
		VM_PROT_READ | VM_PROT_EXECUTE;
	#else
		VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE;
	#endif
	kern_return_t result = vm_protect(mach_task_self(), reinterpret_cast<vm_address_t>(m_code), size, 0, protection);
	assert(result == 0);
	m_size = allocSize;
#elif defined(MEMFUNC_USE_MMAP)
	uint32 additionalMapFlags = 0;
	#ifdef MEMFUNC_MMAP_ADDITIONAL_FLAGS
		additionalMapFlags = MEMFUNC_MMAP_ADDITIONAL_FLAGS;
	#endif
	m_size = size;
	m_code = mmap(nullptr, size, PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS | additionalMapFlags, -1, 0);
	assert(m_code != MAP_FAILED);
#ifdef MEMFUNC_MMAP_REQUIRES_JIT_WRITE_PROTECT
	pthread_jit_write_protect_np(false);
#endif
	memcpy(m_code, code, size);
#ifdef MEMFUNC_MMAP_REQUIRES_JIT_WRITE_PROTECT
	pthread_jit_write_protect_np(true);
#endif
#elif defined(MEMFUNC_USE_WASM)
	m_wasmModule = emscripten::val::take_ownership(WasmCreateModule(reinterpret_cast<uintptr_t>(code), size));
	m_size = size;
	m_code = reinterpret_cast<void*>(WasmCreateFunction(m_wasmModule.as_handle()));
#endif
	ClearCache();
#if !defined(MEMFUNC_USE_WASM)
	assert((reinterpret_cast<uintptr_t>(m_code) & (BLOCK_ALIGN - 1)) == 0);
#endif
}

CMemoryFunction::~CMemoryFunction()
{
	Reset();
}

void CMemoryFunction::ClearCache()
{
#ifdef __APPLE__
	sys_icache_invalidate(m_code, m_size);
#elif defined(MEMFUNC_USE_MMAP)
	#if defined(__arm__) || defined(__aarch64__)
		__clear_cache(m_code, reinterpret_cast<uint8*>(m_code) + m_size);
	#endif
#endif
}

void CMemoryFunction::Reset()
{
    if(m_code != nullptr)
    {
#ifdef __APPLE__
#if TARGET_OS_IPHONE
        if(m_ios26TxmMode)
        {
            m_rxMemory = nullptr;
            m_rwAliasMemory = nullptr;
            m_code = nullptr;
            m_size = 0;
            return;
        }
#endif
#endif

#if defined(MEMFUNC_USE_WIN32)
        framework_aligned_free(m_code);
#elif defined(MEMFUNC_USE_MACHVM)
        vm_deallocate(mach_task_self(), reinterpret_cast<vm_address_t>(m_code), m_size);
#elif defined(MEMFUNC_USE_MMAP)
        munmap(m_code, m_size);
#elif defined(MEMFUNC_USE_WASM)
        WasmDeleteFunction(reinterpret_cast<int>(m_code));
#endif
    }
    
    m_code = nullptr;
    m_size = 0;
    
#if defined(MEMFUNC_USE_WASM)
    m_wasmModule = emscripten::val();
#endif
}

bool CMemoryFunction::IsEmpty() const
{
	return m_code == nullptr;
}

CMemoryFunction& CMemoryFunction::operator =(CMemoryFunction&& rhs)
{
	Reset();
	std::swap(m_code, rhs.m_code);
	std::swap(m_size, rhs.m_size);
#if defined(MEMFUNC_USE_WASM)
	std::swap(m_wasmModule, rhs.m_wasmModule);
#endif
	return (*this);
}

void CMemoryFunction::operator()(void* context)
{
	typedef void (*FctType)(void*);
	auto fct = reinterpret_cast<FctType>(m_code);
	fct(context);
}

void* CMemoryFunction::GetCode() const
{
	return m_code;
}

size_t CMemoryFunction::GetSize() const
{
	return m_size;
}

void CMemoryFunction::BeginModify()
{
#if defined(MEMFUNC_USE_MACHVM) && defined(MEMFUNC_MACHVM_STRICT_PROTECTION)
#ifdef __APPLE__
   
    if(m_ios26TxmMode)
    {
        return;
    }
#endif
    kern_return_t result = vm_protect(
        mach_task_self(),
        reinterpret_cast<vm_address_t>(m_code),
        m_size,
        0,
        VM_PROT_READ | VM_PROT_WRITE
    );
    assert(result == 0);
#elif defined(MEMFUNC_USE_MMAP) && defined(MEMFUNC_MMAP_REQUIRES_JIT_WRITE_PROTECT)
    pthread_jit_write_protect_np(false);
#endif
}


void CMemoryFunction::EndModify()
{
#if defined(MEMFUNC_USE_MACHVM) && defined(MEMFUNC_MACHVM_STRICT_PROTECTION)
#ifdef __APPLE__
    // En mode TXM, la mémoire est déjà RX
    if(m_ios26TxmMode)
    {
        ClearCache();
        return;
    }
#endif
    kern_return_t result = vm_protect(
        mach_task_self(),
        reinterpret_cast<vm_address_t>(m_code),
        m_size,
        0,
        VM_PROT_READ | VM_PROT_EXECUTE
    );
    assert(result == 0);
#elif defined(MEMFUNC_USE_MMAP) && defined(MEMFUNC_MMAP_REQUIRES_JIT_WRITE_PROTECT)
    pthread_jit_write_protect_np(true);
#endif
    ClearCache();
}


CMemoryFunction CMemoryFunction::CreateInstance()
{
#if defined(MEMFUNC_USE_WASM)
	CMemoryFunction result;
	result.m_wasmModule = m_wasmModule;
	result.m_size = m_size;
	result.m_code = reinterpret_cast<void*>(WasmCreateFunction(m_wasmModule.as_handle()));
	return result;
#else
	return CMemoryFunction(GetCode(), GetSize());
#endif
}
