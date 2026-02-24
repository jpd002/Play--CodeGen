# Guide d'implémentation JIT pour Play!-CodeGen

**Version:** 1.0
**Date:** 2026-01-17
**Cible:** Repository Play--CodeGen (module de génération de code)

---

## 📋 Table des matières

1. [Vue d'ensemble](#vue-densemble)
2. [Architecture des 3 modes JIT](#architecture-des-3-modes-jit)
3. [Dépendances](#dépendances)
4. [Structure des fichiers](#structure-des-fichiers)
5. [Implémentation C++](#implémentation-c)
6. [Configuration CMake](#configuration-cmake)
7. [API d'utilisation](#api-dutilisation)
8. [Tests](#tests)

---

## Vue d'ensemble

Ce document décrit l'implémentation **bas niveau** du système JIT multi-mode pour le module **Play--CodeGen**. Ce module fournit les primitives d'allocation de mémoire exécutable pour les trois modes :

- **Legacy** (iOS < 26) : Toggle W^X avec mprotect()
- **LuckNoTXM** (iOS 26+ sans TXM) : Miroirs RW/RX par allocation
- **LuckTXM** (iOS 26+ avec TXM) : Région de 512 MB pré-allouée

### Responsabilités de CodeGen

✅ Allocation de mémoire exécutable
✅ Gestion des miroirs RW/RX
✅ Toggle permissions Write/Execute (Legacy)
✅ Dispatcher entre les 3 modes

❌ Acquisition du JIT (géré par Play!)
❌ Détection TXM (géré par Play!)
❌ Interface utilisateur

---

## Architecture des 3 modes JIT

```
┌──────────────────────────────────────────────────┐
│   CodeGen::SetJitType(type)                      │
│   Appelé par Play! au démarrage                  │
└─────────────────┬────────────────────────────────┘
                  │
                  ▼
      ┌───────────┴───────────┐
      │  g_jit_type global    │
      └───────────┬───────────┘
                  │
     ┌────────────┴────────────┐
     │   Dispatcher routage    │
     └────────────┬────────────┘
                  │
    ┌─────────────┼─────────────┐
    │             │             │
    ▼             ▼             ▼
┌────────┐  ┌──────────┐  ┌──────────┐
│Legacy  │  │LuckNoTXM │  │ LuckTXM  │
│Impl.   │  │  Impl.   │  │  Impl.   │
└────────┘  └──────────┘  └──────────┘
```

---

## Dépendances

### lwmem - Lightweight Memory Manager

**Pourquoi :** Gère l'allocation dynamique dans la région de 512 MB (mode LuckTXM uniquement).

**Installation :**

```bash
cd Play--CodeGen
git submodule add https://github.com/MaJerle/lwmem.git deps/lwmem
git submodule update --init --recursive
```

**Vérification :**

```bash
ls deps/lwmem/lwmem/src/include/lwmem/lwmem.h
# Doit exister
```

### Headers système requis

```cpp
#include <mach/mach.h>          // vm_remap(), vm_deallocate()
#include <sys/mman.h>           // mmap(), mprotect(), munmap()
#include <sys/types.h>          // Types système
#include <unistd.h>             // sysconf(), getpid()
```

---

## Structure des fichiers

Créez cette arborescence dans **Play--CodeGen** :

```
Play--CodeGen/
├── deps/
│   └── lwmem/                           # Submodule Git
│
├── include/
│   ├── MemoryUtil.h                     # Interface principale (MODIFIER)
│   └── JITMemoryTracker.h               # Tracker pour mode Legacy
│
└── src/
    ├── MemoryUtil_iOS.cpp               # Dispatcher
    ├── MemoryUtil_iOS_Legacy.cpp        # Implémentation Legacy
    ├── MemoryUtil_iOS_LuckNoTXM.cpp     # Implémentation LuckNoTXM
    ├── MemoryUtil_iOS_LuckTXM.cpp       # Implémentation LuckTXM
    └── JITMemoryTracker.cpp             # Tracking W^X pour Legacy
```

**Total :** 7 fichiers (1 modifié, 6 créés)

---

## Implémentation C++

### 📄 `include/MemoryUtil.h`

**Ajoutez** ces déclarations à votre fichier MemoryUtil.h existant :

```cpp
#pragma once

#include <cstddef>

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

namespace CodeGen
{

// ============================================================================
// API PUBLIQUE
// ============================================================================

/// Alloue de la mémoire exécutable
/// @param size Taille en bytes (sera alignée sur page)
/// @return Pointeur vers mémoire exécutable, ou nullptr si échec
void* AllocateExecutableMemory(size_t size);

/// Libère de la mémoire exécutable
/// @param ptr Pointeur retourné par AllocateExecutableMemory()
/// @param size Taille originale
void FreeExecutableMemory(void* ptr, size_t size);

#if TARGET_OS_IOS

// ============================================================================
// TYPES ET CONFIGURATION (iOS uniquement)
// ============================================================================

/// Types de JIT supportés
enum class JitType {
  Legacy,      ///< iOS < 26 : Toggle W^X avec mprotect()
  LuckNoTXM,   ///< iOS 26+ sans TXM : Miroirs RW/RX par allocation
  LuckTXM      ///< iOS 26+ avec TXM : Région 512 MB pré-allouée
};

/// Configure le type de JIT à utiliser
/// @note DOIT être appelé avant toute allocation
/// @param type Type de JIT (détecté par Play!)
void SetJitType(JitType type);

// ============================================================================
// GESTION WRITE/EXECUTE (Legacy uniquement)
// ============================================================================

/// Active écriture, désactive exécution (Legacy)
void JITPageWriteEnableExecuteDisable(void* ptr);

/// Désactive écriture, active exécution (Legacy)
void JITPageWriteDisableExecuteEnable(void* ptr);

/// RAII wrapper pour accès en écriture sécurisé
struct ScopedJITPageWriteAndNoExecute {
  void* ptr;

  ScopedJITPageWriteAndNoExecute(void* region) : ptr(region) {
    JITPageWriteEnableExecuteDisable(ptr);
  }

  ~ScopedJITPageWriteAndNoExecute() {
    JITPageWriteDisableExecuteEnable(ptr);
  }
};

// ============================================================================
// GESTION RÉGIONS RW/RX (LuckNoTXM et LuckTXM)
// ============================================================================

/// Pré-alloue la région exécutable (LuckTXM uniquement)
/// @note Doit être appelé une fois au démarrage si mode LuckTXM
void AllocateExecutableMemoryRegion();

/// Obtient le décalage vers région writable
/// @param rx_ptr Pointeur exécutable
/// @param size Taille de la région
/// @return Décalage à ajouter pour obtenir pointeur writable
ptrdiff_t AllocateWritableRegionAndGetDiff(void* rx_ptr, size_t size);

/// Libère la région writable (LuckNoTXM uniquement)
/// @param rx_ptr Pointeur exécutable
/// @param size Taille
/// @param diff Décalage retourné par AllocateWritableRegionAndGetDiff()
void FreeWritableRegion(void* rx_ptr, size_t size, ptrdiff_t diff);

// ============================================================================
// DÉCLARATIONS INTERNES (ne pas utiliser directement)
// ============================================================================

// LuckTXM
void* AllocateExecutableMemory_LuckTXM(size_t size);
void FreeExecutableMemory_LuckTXM(void* ptr);
void AllocateExecutableMemoryRegion_LuckTXM();
ptrdiff_t AllocateWritableRegionAndGetDiff_LuckTXM();

// LuckNoTXM
void* AllocateExecutableMemory_LuckNoTXM(size_t size);
void FreeExecutableMemory_LuckNoTXM(void* ptr, size_t size);
ptrdiff_t AllocateWritableRegionAndGetDiff_LuckNoTXM(void* rx_ptr, size_t size);
void FreeWritableRegion_LuckNoTXM(void* rx_ptr, size_t size, ptrdiff_t diff);

// Legacy
void* AllocateExecutableMemory_Legacy(size_t size);
void FreeExecutableMemory_Legacy(void* ptr, size_t size);
void JITPageWriteEnableExecuteDisable_Legacy(void* ptr);
void JITPageWriteDisableExecuteEnable_Legacy(void* ptr);

#endif // TARGET_OS_IOS

} // namespace CodeGen
```

---

### 📄 `src/MemoryUtil_iOS.cpp`

**Dispatcher principal** qui route tous les appels :

```cpp
#include "MemoryUtil.h"

namespace CodeGen
{

// Variable globale stockant le type JIT actif
static JitType g_jit_type = JitType::LuckTXM;

void SetJitType(JitType type) {
  g_jit_type = type;
}

void* AllocateExecutableMemory(size_t size) {
  if (g_jit_type == JitType::LuckTXM)
    return AllocateExecutableMemory_LuckTXM(size);
  else if (g_jit_type == JitType::LuckNoTXM)
    return AllocateExecutableMemory_LuckNoTXM(size);
  else if (g_jit_type == JitType::Legacy)
    return AllocateExecutableMemory_Legacy(size);

  return nullptr;
}

void FreeExecutableMemory(void* ptr, size_t size) {
  if (g_jit_type == JitType::LuckTXM)
    FreeExecutableMemory_LuckTXM(ptr);
  else if (g_jit_type == JitType::LuckNoTXM)
    FreeExecutableMemory_LuckNoTXM(ptr, size);
  else if (g_jit_type == JitType::Legacy)
    FreeExecutableMemory_Legacy(ptr, size);
}

void AllocateExecutableMemoryRegion() {
  if (g_jit_type == JitType::LuckTXM) {
    AllocateExecutableMemoryRegion_LuckTXM();
  }
}

ptrdiff_t AllocateWritableRegionAndGetDiff(void* rx_ptr, size_t size) {
  if (g_jit_type == JitType::LuckTXM)
    return AllocateWritableRegionAndGetDiff_LuckTXM();
  else if (g_jit_type == JitType::LuckNoTXM)
    return AllocateWritableRegionAndGetDiff_LuckNoTXM(rx_ptr, size);

  return 0;
}

void FreeWritableRegion(void* rx_ptr, size_t size, ptrdiff_t diff) {
  if (g_jit_type == JitType::LuckNoTXM) {
    FreeWritableRegion_LuckNoTXM(rx_ptr, size, diff);
  }
}

void JITPageWriteEnableExecuteDisable(void* ptr) {
  if (g_jit_type == JitType::Legacy) {
    JITPageWriteEnableExecuteDisable_Legacy(ptr);
  }
}

void JITPageWriteDisableExecuteEnable(void* ptr) {
  if (g_jit_type == JitType::Legacy) {
    JITPageWriteDisableExecuteEnable_Legacy(ptr);
  }
}

} // namespace CodeGen
```

---

### 📄 `src/MemoryUtil_iOS_LuckTXM.cpp`

**Mode le plus performant** : 512 MB pré-alloués avec miroir RW permanent.

```cpp
#include "MemoryUtil.h"

#include <lwmem/lwmem.h>
#include <mach/mach.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstdint>

// 512 MiB de région exécutable pré-allouée
constexpr size_t EXECUTABLE_REGION_SIZE = 536870912;

static uint8_t* g_rx_region = nullptr;      // Pointeur région RX (exécutable)
static ptrdiff_t g_rw_region_diff = 0;      // Décalage RW - RX

namespace CodeGen
{

void AllocateExecutableMemoryRegion_LuckTXM() {
  if (g_rx_region) return; // Déjà alloué

  const size_t size = EXECUTABLE_REGION_SIZE;

  // 1. Allouer région RX (read-execute)
  uint8_t* rx_ptr = static_cast<uint8_t*>(
    mmap(nullptr, size, PROT_READ | PROT_EXEC, MAP_ANON | MAP_PRIVATE, -1, 0)
  );

  if (!rx_ptr || rx_ptr == MAP_FAILED) {
    // TODO: Log error
    return;
  }

  // 2. Signal TXM avec breakpoint spécial (ARM64)
  // Cette instruction informe le Trusted Execution Monitor de la région JIT
  asm ("mov x0, %0\n"
       "mov x1, %1\n"
       "brk #0x69" :: "r" (rx_ptr), "r" (size) : "x0", "x1");

  // 3. Créer miroir RW de la même mémoire physique
  vm_address_t rw_region = 0;
  vm_address_t target = reinterpret_cast<vm_address_t>(rx_ptr);
  vm_prot_t cur_protection = 0;
  vm_prot_t max_protection = 0;

  kern_return_t retval = vm_remap(
    mach_task_self(),    // Task cible
    &rw_region,          // Adresse de sortie (miroir)
    size,                // Taille
    0,                   // Mask
    true,                // Anywhere (laisse kernel choisir l'adresse)
    mach_task_self(),    // Task source
    target,              // Adresse source (rx_ptr)
    false,               // Copy (false = partage mémoire physique)
    &cur_protection,
    &max_protection,
    VM_INHERIT_DEFAULT
  );

  if (retval != KERN_SUCCESS) {
    munmap(rx_ptr, size);
    return;
  }

  uint8_t* rw_ptr = reinterpret_cast<uint8_t*>(rw_region);

  // 4. Forcer permissions RW sur le miroir
  if (mprotect(rw_ptr, size, PROT_READ | PROT_WRITE) != 0) {
    munmap(rx_ptr, size);
    vm_deallocate(mach_task_self(), rw_region, size);
    return;
  }

  // 5. Initialiser lwmem pour gérer allocation dynamique
  lwmem_region_t regions[] = {
    { (void*)rw_ptr, size },
    { NULL, 0 }
  };

  if (lwmem_assignmem(regions) == 0) {
    munmap(rx_ptr, size);
    vm_deallocate(mach_task_self(), rw_region, size);
    return;
  }

  g_rx_region = rx_ptr;
  g_rw_region_diff = rw_ptr - rx_ptr;
}

ptrdiff_t AllocateWritableRegionAndGetDiff_LuckTXM() {
  return g_rw_region_diff;
}

void* AllocateExecutableMemory_LuckTXM(size_t size) {
  if (g_rx_region == nullptr) return nullptr;

  const size_t pagesize = sysconf(_SC_PAGESIZE);

  // Allouer via lwmem avec espace pour alignement + métadonnées
  void* raw = lwmem_malloc(size + pagesize - 1 + sizeof(void*));

  if (!raw) return nullptr;

  // Aligner sur page boundary
  uintptr_t raw_addr = (uintptr_t)raw + sizeof(void*);
  uintptr_t aligned = (raw_addr + pagesize - 1) & ~(pagesize - 1);

  // Stocker pointeur raw pour lwmem_free()
  ((void**)aligned)[-1] = raw;

  // Retourner pointeur RX (exécutable) au lieu du RW
  return (uint8_t*)aligned - g_rw_region_diff;
}

void FreeExecutableMemory_LuckTXM(void* ptr) {
  if (!ptr) return;

  // Convertir ptr RX en RW, récupérer raw, libérer
  uint8_t* rw_ptr = static_cast<uint8_t*>(ptr) + g_rw_region_diff;
  void* raw = ((void**)rw_ptr)[-1];
  lwmem_free(raw);
}

} // namespace CodeGen
```

**Points clés :**
- Une seule allocation de 512 MB au démarrage
- Pas de mprotect() pendant l'exécution (ultra-rapide)
- lwmem gère l'allocation dynamique dans cette région
- Deux pointeurs : RX pour exécuter, RW pour écrire

---

### 📄 `src/MemoryUtil_iOS_LuckNoTXM.cpp`

**Mode flexible** : Crée un miroir RW pour chaque allocation.

```cpp
#include "MemoryUtil.h"

#include <mach/mach.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstdint>

namespace CodeGen
{

void* AllocateExecutableMemory_LuckNoTXM(size_t size) {
  // Allouer région RX
  uint8_t* rx_ptr = static_cast<uint8_t*>(
    mmap(nullptr, size, PROT_READ | PROT_EXEC, MAP_ANON | MAP_PRIVATE, -1, 0)
  );

  if (!rx_ptr || rx_ptr == MAP_FAILED) return nullptr;
  return rx_ptr;
}

void FreeExecutableMemory_LuckNoTXM(void* ptr, size_t size) {
  if (ptr) {
    munmap(ptr, size);
  }
}

ptrdiff_t AllocateWritableRegionAndGetDiff_LuckNoTXM(void* rx_ptr, size_t size) {
  // Créer miroir RW de rx_ptr
  vm_address_t rw_region = 0;
  vm_address_t target = reinterpret_cast<vm_address_t>(rx_ptr);
  vm_prot_t cur_protection = 0;
  vm_prot_t max_protection = 0;

  kern_return_t retval = vm_remap(
    mach_task_self(), &rw_region, size, 0, true,
    mach_task_self(), target, false,
    &cur_protection, &max_protection, VM_INHERIT_DEFAULT
  );

  if (retval != KERN_SUCCESS) return 0;

  uint8_t* rw_ptr = reinterpret_cast<uint8_t*>(rw_region);

  if (mprotect(rw_ptr, size, PROT_READ | PROT_WRITE) != 0) {
    vm_deallocate(mach_task_self(), rw_region, size);
    return 0;
  }

  return rw_ptr - static_cast<uint8_t*>(rx_ptr);
}

void FreeWritableRegion_LuckNoTXM(void* rx_ptr, size_t size, ptrdiff_t diff) {
  uint8_t* rw_ptr = static_cast<uint8_t*>(rx_ptr) + diff;
  vm_deallocate(mach_task_self(), reinterpret_cast<vm_address_t>(rw_ptr), size);
}

} // namespace CodeGen
```

**Points clés :**
- Miroir RW créé pour chaque allocation
- Plus flexible : pas de limite de 512 MB
- Légèrement moins performant que LuckTXM

---

### 📄 `src/MemoryUtil_iOS_Legacy.cpp`

**Mode compatible** : Toggle W^X avec mprotect().

```cpp
#include "MemoryUtil.h"
#include "JITMemoryTracker.h"

#include <sys/mman.h>
#include <unistd.h>

namespace CodeGen
{

static JITMemoryTracker g_jit_memory_tracker;

void* AllocateExecutableMemory_Legacy(size_t size) {
  void* ptr = mmap(nullptr, size, PROT_READ | PROT_EXEC,
                   MAP_ANON | MAP_PRIVATE, -1, 0);

  if (ptr == MAP_FAILED) ptr = nullptr;
  if (ptr == nullptr) return nullptr;

  g_jit_memory_tracker.RegisterJITRegion(ptr, size);
  return ptr;
}

void FreeExecutableMemory_Legacy(void* ptr, size_t size) {
  if (ptr) {
    munmap(ptr, size);
    g_jit_memory_tracker.UnregisterJITRegion(ptr);
  }
}

void JITPageWriteEnableExecuteDisable_Legacy(void* ptr) {
  g_jit_memory_tracker.JITRegionWriteEnableExecuteDisable(ptr);
}

void JITPageWriteDisableExecuteEnable_Legacy(void* ptr) {
  g_jit_memory_tracker.JITRegionWriteDisableExecuteEnable(ptr);
}

} // namespace CodeGen
```

---

### 📄 `include/JITMemoryTracker.h`

Gère le tracking des régions et le toggle W^X :

```cpp
#pragma once

#include <cstddef>
#include <map>
#include <mutex>

namespace CodeGen
{

class JITMemoryTracker {
public:
  void RegisterJITRegion(void* ptr, size_t size);
  void UnregisterJITRegion(void* ptr);
  void JITRegionWriteEnableExecuteDisable(void* ptr);
  void JITRegionWriteDisableExecuteEnable(void* ptr);

private:
  struct JITRegion {
    void* start;
    size_t size;
    int nesting_counter; // Support appels imbriqués
  };

  std::map<void*, JITRegion> m_regions;
  std::mutex m_mutex;

  JITRegion* FindRegion(void* ptr);
};

} // namespace CodeGen
```

---

### 📄 `src/JITMemoryTracker.cpp`

```cpp
#include "JITMemoryTracker.h"
#include <sys/mman.h>

namespace CodeGen
{

void JITMemoryTracker::RegisterJITRegion(void* ptr, size_t size) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_regions[ptr] = {ptr, size, 0};
}

void JITMemoryTracker::UnregisterJITRegion(void* ptr) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_regions.erase(ptr);
}

JITMemoryTracker::JITRegion* JITMemoryTracker::FindRegion(void* ptr) {
  for (auto& pair : m_regions) {
    JITRegion& region = pair.second;
    if (ptr >= region.start &&
        ptr < static_cast<char*>(region.start) + region.size) {
      return &region;
    }
  }
  return nullptr;
}

void JITMemoryTracker::JITRegionWriteEnableExecuteDisable(void* ptr) {
  std::lock_guard<std::mutex> lock(m_mutex);
  JITRegion* region = FindRegion(ptr);
  if (region) {
    if (region->nesting_counter == 0) {
      mprotect(region->start, region->size, PROT_READ | PROT_WRITE);
    }
    region->nesting_counter++;
  }
}

void JITMemoryTracker::JITRegionWriteDisableExecuteEnable(void* ptr) {
  std::lock_guard<std::mutex> lock(m_mutex);
  JITRegion* region = FindRegion(ptr);
  if (region) {
    region->nesting_counter--;
    if (region->nesting_counter == 0) {
      mprotect(region->start, region->size, PROT_READ | PROT_EXEC);
    }
  }
}

} // namespace CodeGen
```

---

## Configuration CMake

Modifiez `CMakeLists.txt` de Play--CodeGen :

```cmake
project(PlayCodeGen)

# Détection iOS
if(APPLE)
  if(IOS OR CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(TARGET_PLATFORM_IOS TRUE)
  endif()
endif()

# Configuration iOS
if(TARGET_PLATFORM_IOS)
  message(STATUS "Configuring Play--CodeGen for iOS with JIT support")

  # Ajouter lwmem
  add_subdirectory(deps/lwmem)

  # Sources iOS JIT
  set(IOS_JIT_SOURCES
    src/MemoryUtil_iOS.cpp
    src/MemoryUtil_iOS_Legacy.cpp
    src/MemoryUtil_iOS_LuckNoTXM.cpp
    src/MemoryUtil_iOS_LuckTXM.cpp
    src/JITMemoryTracker.cpp
  )

  target_sources(PlayCodeGen PRIVATE ${IOS_JIT_SOURCES})

  # Link lwmem
  target_link_libraries(PlayCodeGen PRIVATE lwmem)

  # Include directories
  target_include_directories(PlayCodeGen PRIVATE
    ${CMAKE_SOURCE_DIR}/deps/lwmem/lwmem/src/include
  )

  # Defines
  target_compile_definitions(PlayCodeGen PRIVATE
    IPHONEOS=1
    TARGET_OS_IOS=1
  )
endif()
```

---

## API d'utilisation

### Initialisation (au démarrage de l'app)

```cpp
#include "MemoryUtil.h"

void InitializeJIT(CodeGen::JitType type) {
  // 1. Configurer le type
  CodeGen::SetJitType(type);

  // 2. Pré-allouer région si LuckTXM
  if (type == CodeGen::JitType::LuckTXM) {
    CodeGen::AllocateExecutableMemoryRegion();
  }
}
```

### Utilisation mode LuckTXM / LuckNoTXM

```cpp
void CompileBlock() {
  const size_t codeSize = 4096;

  // 1. Allouer
  void* rx_ptr = CodeGen::AllocateExecutableMemory(codeSize);
  if (!rx_ptr) return;

  // 2. Obtenir pointeur writable
  ptrdiff_t diff = CodeGen::AllocateWritableRegionAndGetDiff(rx_ptr, codeSize);
  void* rw_ptr = static_cast<uint8_t*>(rx_ptr) + diff;

  // 3. Écrire code via rw_ptr
  GenerateCode(rw_ptr, codeSize);

  // 4. Exécuter via rx_ptr
  typedef void (*Func)();
  Func f = reinterpret_cast<Func>(rx_ptr);
  f();

  // 5. Libérer
  CodeGen::FreeWritableRegion(rx_ptr, codeSize, diff);
  CodeGen::FreeExecutableMemory(rx_ptr, codeSize);
}
```

### Utilisation mode Legacy

```cpp
void CompileBlock_Legacy() {
  const size_t codeSize = 4096;

  // 1. Allouer
  void* code = CodeGen::AllocateExecutableMemory(codeSize);
  if (!code) return;

  // 2. Activer écriture
  {
    CodeGen::ScopedJITPageWriteAndNoExecute guard(code);

    // Écrire code
    GenerateCode(code, codeSize);

    // Destructeur réactive exécution
  }

  // 3. Exécuter
  typedef void (*Func)();
  Func f = reinterpret_cast<Func>(code);
  f();

  // 4. Libérer
  CodeGen::FreeExecutableMemory(code, codeSize);
}
```

---

## Tests

### Test unitaire basique

```cpp
#include "MemoryUtil.h"
#include <cassert>
#include <cstdio>

void TestJITAllocation() {
  const size_t testSize = 4096;

  // Test allocation
  void* ptr = CodeGen::AllocateExecutableMemory(testSize);
  assert(ptr != nullptr);

  // Test writable region
  ptrdiff_t diff = CodeGen::AllocateWritableRegionAndGetDiff(ptr, testSize);
  void* rw_ptr = static_cast<uint8_t*>(ptr) + diff;

  // Test écriture : ARM64 "ret" instruction
  uint32_t* code = static_cast<uint32_t*>(rw_ptr);
  code[0] = 0xD65F03C0; // ret

  // Test exécution
  typedef void (*EmptyFunc)();
  EmptyFunc func = reinterpret_cast<EmptyFunc>(ptr);
  func(); // Doit juste retourner

  // Cleanup
  CodeGen::FreeWritableRegion(ptr, testSize, diff);
  CodeGen::FreeExecutableMemory(ptr, testSize);

  printf("✅ JIT allocation test passed\n");
}
```

### Intégration dans CMake

```cmake
if(BUILD_TESTING)
  add_executable(jit_test tests/jit_test.cpp)
  target_link_libraries(jit_test PRIVATE PlayCodeGen)
  add_test(NAME JITAllocation COMMAND jit_test)
endif()
```

---

## Checklist de validation

- [ ] lwmem ajouté comme submodule
- [ ] 7 fichiers créés/modifiés
- [ ] CMakeLists.txt configuré
- [ ] Compilation réussie sur iOS
- [ ] Test unitaire qui passe
- [ ] Testé sur simulateur (mode Unrestricted)
- [ ] Testé sur device réel

---

## Conclusion

Vous avez maintenant tous les fichiers nécessaires pour le module **Play--CodeGen**.

**Prochaine étape :** Consultez `JIT_INTEGRATION_PLAY.md` pour l'intégration dans l'app Play! principale (acquisition JIT, détection TXM, UI).
