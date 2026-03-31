# Guide de migration JIT : Dolphin iOS → Play! PS2 Emulator

**Version:** 1.0
**Date:** 2026-01-17
**Auteur:** Analyse basée sur Dolphin iOS JIT Implementation

---

## 📋 Table des matières

1. [Vue d'ensemble](#vue-densemble)
2. [Architecture du système JIT](#architecture-du-système-jit)
3. [Dépendances et prérequis](#dépendances-et-prérequis)
4. [Structure des fichiers](#structure-des-fichiers)
5. [Implémentation C++](#implémentation-c)
6. [Implémentation Objective-C/Swift](#implémentation-objective-cswift)
7. [Intégration dans Play!](#intégration-dans-play)
8. [Configuration CMake](#configuration-cmake)
9. [Tests et validation](#tests-et-validation)
10. [Troubleshooting](#troubleshooting)

---

## Vue d'ensemble

Ce guide vous permet d'implémenter dans **Play! PS2 Emulator** le système JIT avancé de Dolphin iOS qui supporte:

- ✅ **iOS < 26** : Mode Legacy avec toggle de permissions (W^X)
- ✅ **iOS 26+ sans TXM** : Miroirs RW/RX créés dynamiquement par allocation
- ✅ **iOS 26+ avec TXM** : Région pré-allouée de 512 MB avec miroir RW/RX permanent

### Avantages de cette implémentation

- **Performance optimale** sur iOS 26+ avec TXM (allocation unique, pas de toggle)
- **Compatibilité rétroactive** avec toutes les versions iOS
- **Détection automatique** du matériel (TXM) et de la version iOS
- **Acquisition JIT automatique** via PTrace, AltServer, JitStreamer, ou Debugger

---

## Architecture du système JIT

### Schéma de décision

```
┌─────────────────────────────────────┐
│   Démarrage de l'application        │
└──────────────┬──────────────────────┘
               │
               ▼
┌─────────────────────────────────────┐
│   JitManager.shared() détecte:      │
│   - Version iOS (< 26 ou >= 26)     │
│   - Présence TXM (fichier firmware) │
│   - JIT acquis (debugger actif?)    │
└──────────────┬──────────────────────┘
               │
               ▼
        ┌──────┴──────┐
        │ iOS >= 26 ? │
        └──────┬──────┘
               │
       ┌───────┴────────┐
       NO              YES
       │                │
       ▼                ▼
  ┌─────────┐    ┌──────────┐
  │ Legacy  │    │ TXM ?    │
  │  Mode   │    └────┬─────┘
  └─────────┘         │
                  ┌───┴────┐
                  NO      YES
                  │        │
                  ▼        ▼
            ┌──────────┐ ┌──────────┐
            │LuckNoTXM │ │ LuckTXM  │
            └──────────┘ └──────────┘
```

### Détail des 3 modes

| Mode | iOS Version | TXM | Allocation | Miroir RW | Performance |
|------|-------------|-----|------------|-----------|-------------|
| **Legacy** | < 26 | N/A | mmap() par allocation | Non (toggle mprotect) | ⭐⭐⭐ |
| **LuckNoTXM** | >= 26 | ❌ | mmap() par allocation | vm_remap() par alloc | ⭐⭐⭐⭐ |
| **LuckTXM** | >= 26 | ✅ | Région de 512MB unique | vm_remap() global | ⭐⭐⭐⭐⭐ |

---

## Dépendances et prérequis

### 1. lwmem - Lightweight Memory Manager

**Pourquoi:** Gère l'allocation dynamique dans la région de 512 MB en mode LuckTXM.

**Installation:**

```bash
cd Play--CodeGen
git submodule add https://github.com/MaJerle/lwmem.git deps/lwmem
git submodule update --init --recursive
```

**Vérification:**

```bash
ls deps/lwmem/lwmem/src/include/lwmem/lwmem.h
# Doit exister
```

### 2. Headers système requis

Votre code aura besoin de:

```cpp
#include <mach/mach.h>          // vm_remap(), vm_deallocate()
#include <sys/mman.h>           // mmap(), mprotect(), munmap()
#include <spawn.h>              // posix_spawnp()
#include <sys/types.h>          // pid_t
#include <unistd.h>             // sysconf(), getpid()
```

**⚠️ Fonction non documentée nécessaire:**

```cpp
extern int csops(pid_t pid, unsigned int ops, void* useraddr, size_t usersize);
```

---

## Structure des fichiers

Créez l'arborescence suivante dans **Play--CodeGen**:

```
Play--CodeGen/
├── deps/
│   └── lwmem/                           # Submodule Git
│
├── include/
│   ├── MemoryUtil.h                     # Interface principale (MODIFIER)
│   ├── JITMemoryTracker.h               # Tracker pour mode Legacy
│   └── ios/
│       ├── JitManager.h
│       ├── JitManager+Debugger.h
│       ├── JitManager+PTrace.h
│       ├── JitManager+AltServer.h       # Optionnel
│       └── JitAcquisitionService.swift
│
└── src/
    ├── MemoryUtil_iOS.cpp               # Dispatcher (routage)
    ├── MemoryUtil_iOS_Legacy.cpp        # Implémentation iOS < 26
    ├── MemoryUtil_iOS_LuckNoTXM.cpp     # Implémentation iOS 26+ sans TXM
    ├── MemoryUtil_iOS_LuckTXM.cpp       # Implémentation iOS 26+ avec TXM
    ├── JITMemoryTracker.cpp             # Tracking pour Legacy
    └── ios/
        ├── JitManager.m
        ├── JitManager+Debugger.m
        ├── JitManager+PTrace.m
        └── JitManager+AltServer.m       # Optionnel
```

**Total:** 17 fichiers à créer/modifier

---

## Implémentation C++

### 📄 `include/MemoryUtil.h`

Ajoutez ces déclarations à votre fichier existant:

```cpp
#pragma once

#include <cstddef>

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

namespace CodeGen
{

// Fonctions principales
void* AllocateExecutableMemory(size_t size);
void FreeExecutableMemory(void* ptr, size_t size);

#if TARGET_OS_IOS

// Types de JIT supportés
enum class JitType {
  Legacy,      // iOS < 26
  LuckNoTXM,   // iOS 26+ sans TXM
  LuckTXM      // iOS 26+ avec TXM
};

void SetJitType(JitType type);

// Gestion write/execute pour iOS
void JITPageWriteEnableExecuteDisable(void* ptr);
void JITPageWriteDisableExecuteEnable(void* ptr);

// RAII wrapper pour accès en écriture sécurisé
struct ScopedJITPageWriteAndNoExecute {
  void* ptr;

  ScopedJITPageWriteAndNoExecute(void* region) : ptr(region) {
    JITPageWriteEnableExecuteDisable(ptr);
  }

  ~ScopedJITPageWriteAndNoExecute() {
    JITPageWriteDisableExecuteEnable(ptr);
  }
};

// Gestion des régions mémoire
void AllocateExecutableMemoryRegion();
ptrdiff_t AllocateWritableRegionAndGetDiff(void* rx_ptr, size_t size);
void FreeWritableRegion(void* rx_ptr, size_t size, ptrdiff_t diff);

// Déclarations des fonctions spécifiques par type
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

### 📄 `src/MemoryUtil_iOS.cpp` (Dispatcher)

Ce fichier route tous les appels vers l'implémentation appropriée:

```cpp
#include "MemoryUtil.h"

namespace CodeGen
{

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

**Mode le plus performant** : Région de 512 MB pré-allouée avec miroir RW permanent.

```cpp
#include "MemoryUtil.h"

#include <lwmem/lwmem.h>
#include <mach/mach.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstdint>

// 512 MiB - région pré-allouée
constexpr size_t EXECUTABLE_REGION_SIZE = 536870912;

static uint8_t* g_rx_region = nullptr;
static ptrdiff_t g_rw_region_diff = 0;

namespace CodeGen
{

void AllocateExecutableMemoryRegion_LuckTXM() {
  if (g_rx_region) return; // Déjà alloué

  const size_t size = EXECUTABLE_REGION_SIZE;

  // Allouer région RX (read-execute)
  uint8_t* rx_ptr = static_cast<uint8_t*>(
    mmap(nullptr, size, PROT_READ | PROT_EXEC, MAP_ANON | MAP_PRIVATE, -1, 0)
  );

  if (!rx_ptr) {
    // Gérer l'erreur (log, exception, etc.)
    return;
  }

  // Signal TXM avec breakpoint spécial (instruction ARM64)
  // Cette instruction informe le Trusted Execution Monitor de la région JIT
  asm ("mov x0, %0\n"
       "mov x1, %1\n"
       "brk #0x69" :: "r" (rx_ptr), "r" (size) : "x0", "x1");

  // Créer miroir RW (read-write) de la même mémoire physique
  vm_address_t rw_region = 0;
  vm_address_t target = reinterpret_cast<vm_address_t>(rx_ptr);
  vm_prot_t cur_protection = 0;
  vm_prot_t max_protection = 0;

  kern_return_t retval = vm_remap(
    mach_task_self(),    // Task cible
    &rw_region,          // Adresse de sortie (miroir)
    size,                // Taille
    0,                   // Mask
    true,                // Anywhere (laisse le kernel choisir l'adresse)
    mach_task_self(),    // Task source
    target,              // Adresse source (rx_ptr)
    false,               // Copy (false = partage la même mémoire physique)
    &cur_protection,
    &max_protection,
    VM_INHERIT_DEFAULT
  );

  if (retval != KERN_SUCCESS) return;

  uint8_t* rw_ptr = reinterpret_cast<uint8_t*>(rw_region);

  // Forcer permissions RW sur le miroir
  if (mprotect(rw_ptr, size, PROT_READ | PROT_WRITE) != 0) return;

  // Initialiser lwmem pour gérer l'allocation dynamique dans cette région
  lwmem_region_t regions[] = {
    { (void*)rw_ptr, size },
    { NULL, 0 }
  };

  if (lwmem_assignmem(regions) == 0) return;

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

  // Stocker le pointeur raw pour lwmem_free()
  ((void**)aligned)[-1] = raw;

  // Retourner le pointeur RX (executable) au lieu du RW
  return (uint8_t*)aligned - g_rw_region_diff;
}

void FreeExecutableMemory_LuckTXM(void* ptr) {
  // Récupérer le pointeur raw et libérer
  lwmem_free(((void**)ptr)[-1]);
}

} // namespace CodeGen
```

**Points clés:**
- 🔒 **Une seule allocation** de 512 MB au démarrage
- ⚡ **Pas de mprotect()** pendant l'exécution (ultra-rapide)
- 🧠 **lwmem** gère l'allocation dynamique dans cette région
- 📍 **Deux pointeurs** : RX pour exécuter, RW pour écrire

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
  uint8_t* rx_ptr = static_cast<uint8_t*>(
    mmap(nullptr, size, PROT_READ | PROT_EXEC, MAP_ANON | MAP_PRIVATE, -1, 0)
  );

  if (!rx_ptr) return nullptr;
  return rx_ptr;
}

void FreeExecutableMemory_LuckNoTXM(void* ptr, size_t size) {
  if (ptr) {
    munmap(ptr, size);
  }
}

ptrdiff_t AllocateWritableRegionAndGetDiff_LuckNoTXM(void* rx_ptr, size_t size) {
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

  if (mprotect(rw_ptr, size, PROT_READ | PROT_WRITE) != 0) return 0;

  return rw_ptr - static_cast<uint8_t*>(rx_ptr);
}

void FreeWritableRegion_LuckNoTXM(void* rx_ptr, size_t size, ptrdiff_t diff) {
  uint8_t* rw_ptr = static_cast<uint8_t*>(rx_ptr) + diff;
  vm_deallocate(mach_task_self(), reinterpret_cast<vm_address_t>(rw_ptr), size);
}

} // namespace CodeGen
```

**Points clés:**
- 🔁 **Miroir par allocation** : chaque mmap() a son propre vm_remap()
- 📦 **Plus flexible** : pas de limite de 512 MB
- 🐌 **Légèrement moins performant** que LuckTXM (overhead par allocation)

---

### 📄 `src/MemoryUtil_iOS_Legacy.cpp`

**Mode compatible** : Toggle entre WRITE et EXECUTE via mprotect().

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

Gère le tracking des régions JIT et le toggle W^X:

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
    int nesting_counter; // Support pour appels imbriqués
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
      // Seulement toggle si pas déjà en mode write
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
      // Seulement toggle si plus d'appels imbriqués
      mprotect(region->start, region->size, PROT_READ | PROT_EXEC);
    }
  }
}

} // namespace CodeGen
```

**Points clés:**
- 🔢 **Nesting counter** : supporte les appels imbriqués
- 🔒 **Thread-safe** : utilise std::mutex
- 📍 **Lookup dynamique** : trouve la région contenant un pointeur donné

---

## Implémentation Objective-C/Swift

### 📄 `include/ios/JitManager.h`

Interface principale du gestionnaire JIT:

```objc
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface JitManager : NSObject

@property (readonly, assign) BOOL acquiredJit;
@property (nonatomic, nullable) NSString* acquisitionError;
@property (readonly, assign) BOOL deviceHasTxm;

+ (JitManager*)shared;
- (void)recheckIfJitIsAcquired;

@end

NS_ASSUME_NONNULL_END
```

---

### 📄 `src/ios/JitManager.m`

```objc
#import "JitManager.h"
#import "JitManager+Debugger.h"

typedef NS_ENUM(NSInteger, PLAYJitType) {
  PLAYJitTypeDebugger,
  PLAYJitTypeUnrestricted
};

@interface JitManager ()
@property (readwrite, assign) BOOL acquiredJit;
@property (readwrite, assign) BOOL deviceHasTxm;
@end

@implementation JitManager {
  PLAYJitType _jitType;
}

+ (JitManager*)shared {
  static JitManager* sharedInstance = nil;
  static dispatch_once_t onceToken;

  dispatch_once(&onceToken, ^{
    sharedInstance = [[self alloc] init];
  });

  return sharedInstance;
}

- (id)init {
  if (self = [super init]) {
#if TARGET_OS_SIMULATOR
    _jitType = PLAYJitTypeUnrestricted;
#else
    _jitType = PLAYJitTypeDebugger;
#endif

    self.acquiredJit = NO;

    if (@available(iOS 26, *)) {
      self.deviceHasTxm = [self checkIfDeviceUsesTXM];
    } else {
      self.deviceHasTxm = NO;
    }
  }

  return self;
}

- (void)recheckIfJitIsAcquired {
  if (_jitType == PLAYJitTypeDebugger) {
    if (self.deviceHasTxm) {
      NSDictionary* environment = [[NSProcessInfo processInfo] environment];

      // Bloquer Xcode sur iOS 26 + TXM (crash garanti)
      if ([environment objectForKey:@"XCODE"] != nil) {
        static dispatch_once_t onceToken;
        dispatch_once(&onceToken, ^{
          self.acquisitionError = @"JIT cannot be enabled while running within Xcode on iOS 26.";
        });
        return;
      }
    }

    self.acquiredJit = [self checkIfProcessIsDebugged];

    if (self.deviceHasTxm && self.acquiredJit) {
      self.acquisitionError = @"A debugger is attached. However, if the debugger is not StikDebug, Play! will crash when emulation starts.";
    }
  } else if (_jitType == PLAYJitTypeUnrestricted) {
    self.acquiredJit = YES;
  }
}

@end
```

---

### 📄 `include/ios/JitManager+Debugger.h`

```objc
#import "JitManager.h"

@interface JitManager (Debugger)
- (BOOL)checkIfProcessIsDebugged;
- (BOOL)checkIfDeviceUsesTXM;
@end
```

---

### 📄 `src/ios/JitManager+Debugger.m`

```objc
#import "JitManager+Debugger.h"

#define CS_OPS_STATUS 0
#define CS_DEBUGGED 0x10000000

extern int csops(pid_t pid, unsigned int ops, void* useraddr, size_t usersize);

@implementation JitManager (Debugger)

- (BOOL)checkIfProcessIsDebugged {
  int flags;
  if (csops(getpid(), CS_OPS_STATUS, &flags, sizeof(flags)) != 0) {
    return NO;
  }
  return flags & CS_DEBUGGED;
}

- (nullable NSString*)filePathAtPath:(NSString*)path withLength:(NSUInteger)length {
  NSError *error = nil;
  NSArray<NSString *> *items = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:path error:&error];
  if (!items) return nil;

  for (NSString *entry in items) {
    if (entry.length == length) {
      return [path stringByAppendingPathComponent:entry];
    }
  }
  return nil;
}

- (BOOL)checkIfDeviceUsesTXM {
  // Primary: /System/Volumes/Preboot/<36>/boot/<96>/usr/.../Ap,TrustedExecutionMonitor.img4
  NSString* bootUUID = [self filePathAtPath:@"/System/Volumes/Preboot" withLength:36];
  if (bootUUID) {
    NSString* bootDir = [bootUUID stringByAppendingPathComponent:@"boot"];
    NSString* ninetySixCharPath = [self filePathAtPath:bootDir withLength:96];
    if (ninetySixCharPath) {
      NSString* img = [ninetySixCharPath stringByAppendingPathComponent:
                       @"usr/standalone/firmware/FUD/Ap,TrustedExecutionMonitor.img4"];
      return access(img.fileSystemRepresentation, F_OK) == 0;
    }
  }

  // Fallback: /private/preboot/<96>/usr/.../Ap,TrustedExecutionMonitor.img4
  NSString* fallback = [self filePathAtPath:@"/private/preboot" withLength:96];
  if (fallback) {
    NSString* img = [fallback stringByAppendingPathComponent:
                     @"usr/standalone/firmware/FUD/Ap,TrustedExecutionMonitor.img4"];
    return access(img.fileSystemRepresentation, F_OK) == 0;
  }

  return NO;
}

@end
```

**Comment fonctionne la détection TXM:**

1. Cherche un dossier de 36 caractères dans `/System/Volumes/Preboot/` (UUID)
2. Cherche un dossier de 96 caractères dans `<UUID>/boot/`
3. Vérifie l'existence de `usr/standalone/firmware/FUD/Ap,TrustedExecutionMonitor.img4`
4. Si trouvé → TXM présent, sinon → pas de TXM

---

### 📄 `include/ios/JitManager+PTrace.h`

```objc
#import "JitManager.h"

extern const char* _Nonnull PLAYJitPTraceChildProcessArgument;

@interface JitManager (PTrace)
- (BOOL)checkCanAcquireJitByPTrace;
- (void)runPTraceStartupTasks;
- (void)acquireJitByPTrace;
@end
```

---

### 📄 `src/ios/JitManager+PTrace.m`

**Technique PTrace** : Lance un processus enfant qui appelle `PT_TRACE_ME`, ce qui marque le parent comme "debugged".

```objc
#import "JitManager+PTrace.h"
#import "JitManager+Debugger.h"

#import <spawn.h>

void* SecTaskCreateFromSelf(CFAllocatorRef allocator);
CFTypeRef SecTaskCopyValueForEntitlement(void* task, CFStringRef entitlement, CFErrorRef* _Nullable error);

#define PT_TRACE_ME 0
#define PT_DETACH 11
int ptrace(int request, pid_t pid, caddr_t caddr, int data);

extern char** environ;

const char* _Nonnull PLAYJitPTraceChildProcessArgument = "ptraceChild";

@implementation JitManager (PTrace)

- (BOOL)checkCanAcquireJitByPTrace {
  // Vérifie l'entitlement "platform-application" (jailbreak/sideload)
  void* task = SecTaskCreateFromSelf(NULL);
  CFTypeRef entitlementValue = SecTaskCopyValueForEntitlement(task, CFSTR("platform-application"), NULL);

  if (entitlementValue == NULL) return NO;

  BOOL result = entitlementValue == kCFBooleanTrue;

  CFRelease(entitlementValue);
  CFRelease(task);

  return result;
}

- (void)runPTraceStartupTasks {
  // Appelé par le processus enfant au démarrage
  ptrace(PT_TRACE_ME, 0, NULL, 0);
}

- (void)acquireJitByPTrace {
  if (![self checkCanAcquireJitByPTrace]) return;

  const char* executablePath = [[[NSBundle mainBundle] executablePath] UTF8String];
  const char* arguments[] = { executablePath, PLAYJitPTraceChildProcessArgument, NULL };

  pid_t childPid;
  if (posix_spawnp(&childPid, executablePath, NULL, NULL, (char* const*)arguments, environ) == 0) {
    waitpid(childPid, NULL, WUNTRACED);
    ptrace(PT_DETACH, childPid, NULL, 0);
    kill(childPid, SIGTERM);
    wait(NULL);

    [self recheckIfJitIsAcquired];
  } else {
    self.acquisitionError = [NSString stringWithFormat:@"Failed to spawn child process for PTrace style JIT, errno %d", errno];
  }
}

@end
```

**Flux PTrace:**
1. Vérifie entitlement `platform-application`
2. Spawne processus enfant avec argument `"ptraceChild"`
3. Enfant appelle `PT_TRACE_ME`
4. Parent devient "debugged"
5. Parent detach et kill l'enfant
6. Recheck si JIT acquis

---

### 📄 `include/ios/JitAcquisitionService.swift`

Service d'acquisition automatique au démarrage de l'app:

```swift
import UIKit

class JitAcquisitionService: UIResponder, UIApplicationDelegate {

  func application(
    _ application: UIApplication,
    didFinishLaunchingWithOptions launchOptions: [UIApplication.LaunchOptionsKey : Any]? = nil
  ) -> Bool {

    let manager = JitManager.shared()

    // Vérifier si déjà acquis (ex: debugger attaché)
    manager.recheckIfJitIsAcquired()

    if !manager.acquiredJit {
      // Tenter acquisition via PTrace
      manager.acquireJitByPTrace()

      #if NONJAILBROKEN
      // Si vous implémentez AltServer (optionnel)
      // manager.acquireJitByAltServer()
      #endif
    }

    return true
  }
}
```

**Intégration dans `main()` de votre app:**

```objc
int main(int argc, char* argv[]) {
  @autoreleasepool {
    // Si lancé en mode ptrace child, exécuter puis exit
    if (argc > 1 && strcmp(argv[1], PLAYJitPTraceChildProcessArgument) == 0) {
      [[JitManager shared] runPTraceStartupTasks];
      return 0;
    }

    // Lancement normal de l'app
    return UIApplicationMain(argc, argv, nil,
                            NSStringFromClass([JitAcquisitionService class]));
  }
}
```

---

## Intégration dans Play!

### Au démarrage de l'émulateur

Dans votre code d'initialisation de l'émulateur (probablement dans `AppDelegate` ou équivalent):

```objc
#import "JitManager.h"
#import "MemoryUtil.h"

- (void)initializeEmulator {
  JitManager* jitManager = [JitManager shared];
  [jitManager recheckIfJitIsAcquired];

  if (jitManager.acquiredJit) {
    if (@available(iOS 26, *)) {
      if (jitManager.deviceHasTxm) {
        // iOS 26+ avec TXM : mode le plus performant
        CodeGen::SetJitType(CodeGen::JitType::LuckTXM);

        // IMPORTANT: Pré-allouer la région de 512 MB
        CodeGen::AllocateExecutableMemoryRegion();
      } else {
        // iOS 26+ sans TXM
        CodeGen::SetJitType(CodeGen::JitType::LuckNoTXM);
      }
    } else {
      // iOS < 26
      CodeGen::SetJitType(CodeGen::JitType::Legacy);
    }

    NSLog(@"JIT configured successfully");
  } else {
    NSLog(@"JIT not acquired: %@", jitManager.acquisitionError);
    // Afficher une alerte à l'utilisateur
  }
}
```

---

### Dans votre code de génération JIT

#### **Cas 1 : Mode LuckTXM ou LuckNoTXM (iOS 26+)**

```cpp
#include "MemoryUtil.h"

void CompilePS2Code() {
  const size_t codeSize = 4096; // Exemple

  // 1. Allouer mémoire exécutable
  void* rx_ptr = CodeGen::AllocateExecutableMemory(codeSize);
  if (!rx_ptr) {
    // Gérer l'erreur
    return;
  }

  // 2. Obtenir pointeur writable
  ptrdiff_t diff = CodeGen::AllocateWritableRegionAndGetDiff(rx_ptr, codeSize);
  void* rw_ptr = static_cast<uint8_t*>(rx_ptr) + diff;

  // 3. Écrire le code compilé via rw_ptr
  GenerateARMInstructions(rw_ptr, codeSize);

  // 4. Exécuter via rx_ptr
  typedef void (*JitFunction)();
  JitFunction func = reinterpret_cast<JitFunction>(rx_ptr);
  func();

  // 5. Libérer quand terminé
  CodeGen::FreeWritableRegion(rx_ptr, codeSize, diff);
  CodeGen::FreeExecutableMemory(rx_ptr, codeSize);
}
```

**Points clés:**
- ✅ **rx_ptr** : Utilisez pour exécuter
- ✅ **rw_ptr** : Utilisez pour écrire
- ❌ **Ne jamais écrire via rx_ptr** (crash)
- ❌ **Ne jamais exécuter via rw_ptr** (non exécutable)

---

#### **Cas 2 : Mode Legacy (iOS < 26)**

```cpp
#include "MemoryUtil.h"

void CompilePS2Code_Legacy() {
  const size_t codeSize = 4096;

  // 1. Allouer mémoire
  void* code = CodeGen::AllocateExecutableMemory(codeSize);
  if (!code) return;

  // 2. Activer écriture avec RAII wrapper
  {
    CodeGen::ScopedJITPageWriteAndNoExecute guard(code);

    // Le code est maintenant WRITABLE mais pas EXECUTABLE
    GenerateARMInstructions(code, codeSize);

    // Le destructeur de 'guard' réactive automatiquement EXECUTE
  }

  // 3. Exécuter (maintenant EXECUTABLE mais pas WRITABLE)
  typedef void (*JitFunction)();
  JitFunction func = reinterpret_cast<JitFunction>(code);
  func();

  // 4. Libérer
  CodeGen::FreeExecutableMemory(code, codeSize);
}
```

**Avantages du RAII wrapper:**
- ✅ Pas d'oubli de restore execute
- ✅ Exception-safe
- ✅ Supporte l'imbrication

---

### Exemple complet d'intégration

```cpp
#include "MemoryUtil.h"

class PS2Recompiler {
public:
  void* CompileBlock(uint32_t ps2Address, size_t estimatedSize) {
    void* rx_ptr = CodeGen::AllocateExecutableMemory(estimatedSize);
    if (!rx_ptr) return nullptr;

    #if TARGET_OS_IOS
      ptrdiff_t diff = CodeGen::AllocateWritableRegionAndGetDiff(rx_ptr, estimatedSize);
      void* rw_ptr = static_cast<uint8_t*>(rx_ptr) + diff;

      // Compiler vers rw_ptr
      size_t actualSize = EmitARMCode(rw_ptr, ps2Address);

      // Libérer miroir RW (LuckNoTXM uniquement, ignoré sur LuckTXM)
      CodeGen::FreeWritableRegion(rx_ptr, estimatedSize, diff);
    #else
      // Desktop/autres plateformes
      EmitARMCode(rx_ptr, ps2Address);
    #endif

    // Retourner pointeur exécutable
    m_compiledBlocks[ps2Address] = rx_ptr;
    return rx_ptr;
  }

  void FreeBlock(uint32_t ps2Address) {
    auto it = m_compiledBlocks.find(ps2Address);
    if (it != m_compiledBlocks.end()) {
      CodeGen::FreeExecutableMemory(it->second, /* size */);
      m_compiledBlocks.erase(it);
    }
  }

private:
  std::map<uint32_t, void*> m_compiledBlocks;

  size_t EmitARMCode(void* dest, uint32_t ps2Address);
};
```

---

## Configuration CMake

### Modification de `CMakeLists.txt`

Ajoutez ceci à votre CMakeLists principal:

```cmake
project(PlayCodeGen)

# Détection de la plateforme
if(APPLE)
  if(IOS OR CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(TARGET_PLATFORM_IOS TRUE)
  endif()
endif()

# Ajout de lwmem pour iOS
if(TARGET_PLATFORM_IOS)
  message(STATUS "Configuring for iOS with JIT support")

  # Ajouter lwmem
  add_subdirectory(deps/lwmem)

  # Fichiers source iOS
  set(IOS_JIT_SOURCES
    src/MemoryUtil_iOS.cpp
    src/MemoryUtil_iOS_Legacy.cpp
    src/MemoryUtil_iOS_LuckNoTXM.cpp
    src/MemoryUtil_iOS_LuckTXM.cpp
    src/JITMemoryTracker.cpp
    src/ios/JitManager.m
    src/ios/JitManager+Debugger.m
    src/ios/JitManager+PTrace.m
  )

  target_sources(PlayCodeGen PRIVATE ${IOS_JIT_SOURCES})

  # Link libraries
  target_link_libraries(PlayCodeGen PRIVATE lwmem)

  # Include directories
  target_include_directories(PlayCodeGen PRIVATE
    ${CMAKE_SOURCE_DIR}/deps/lwmem/lwmem/src/include
    ${CMAKE_SOURCE_DIR}/include/ios
  )

  # Compiler definitions
  target_compile_definitions(PlayCodeGen PRIVATE
    IPHONEOS=1
    TARGET_OS_IOS=1
  )

  # Enable Objective-C++
  set_source_files_properties(
    ${IOS_JIT_SOURCES}
    PROPERTIES
    COMPILE_FLAGS "-x objective-c++"
  )
endif()
```

---

### Configuration Xcode (si applicable)

Si vous utilisez Xcode directement:

1. **Build Settings → Other Linker Flags:**
   ```
   -framework Foundation
   -framework UIKit
   ```

2. **Build Settings → Header Search Paths:**
   ```
   $(PROJECT_DIR)/deps/lwmem/lwmem/src/include
   $(PROJECT_DIR)/include/ios
   ```

3. **Build Settings → Enable Modules:**
   ```
   YES
   ```

4. **Build Phases → Link Binary With Libraries:**
   - Ajouter `liblwmem.a`

---

## Tests et validation

### Checklist de tests

- [ ] **Compilation réussie** sur toutes les plateformes
- [ ] **Tests unitaires** pour chaque mode JIT
- [ ] **Test simulateur iOS** (doit utiliser mode Unrestricted)
- [ ] **Test iPhone iOS < 26** (doit utiliser mode Legacy)
- [ ] **Test iPhone iOS 26+ sans TXM** (doit utiliser mode LuckNoTXM)
- [ ] **Test iPhone iOS 26+ avec TXM** (doit utiliser mode LuckTXM)

---

### Code de test simple

Ajoutez ce test dans votre suite de tests:

```cpp
#include "MemoryUtil.h"
#include <cassert>

void TestJITAllocation() {
  const size_t testSize = 4096;

  // Test allocation
  void* ptr = CodeGen::AllocateExecutableMemory(testSize);
  assert(ptr != nullptr);

  // Test writable region
  ptrdiff_t diff = CodeGen::AllocateWritableRegionAndGetDiff(ptr, testSize);
  void* rw_ptr = static_cast<uint8_t*>(ptr) + diff;

  // Test écriture
  uint32_t* code = static_cast<uint32_t*>(rw_ptr);
  code[0] = 0xD65F03C0; // ARM64: ret

  // Test exécution
  typedef void (*EmptyFunc)();
  EmptyFunc func = reinterpret_cast<EmptyFunc>(ptr);
  func(); // Doit simplement retourner

  // Cleanup
  CodeGen::FreeWritableRegion(ptr, testSize, diff);
  CodeGen::FreeExecutableMemory(ptr, testSize);

  printf("✅ JIT allocation test passed\n");
}
```

---

### Logs de debug

Ajoutez des logs pour diagnostiquer:

```objc
JitManager* mgr = [JitManager shared];
NSLog(@"=== JIT Status ===");
NSLog(@"iOS Version: %@", [[UIDevice currentDevice] systemVersion]);
NSLog(@"JIT Acquired: %d", mgr.acquiredJit);
NSLog(@"Device has TXM: %d", mgr.deviceHasTxm);
NSLog(@"Acquisition Error: %@", mgr.acquisitionError ?: @"None");

if (@available(iOS 26, *)) {
  NSLog(@"JIT Mode: %@", mgr.deviceHasTxm ? @"LuckTXM" : @"LuckNoTXM");
} else {
  NSLog(@"JIT Mode: Legacy");
}
```

**Exemple de sortie attendue:**

```
=== JIT Status ===
iOS Version: 26.0
JIT Acquired: 1
Device has TXM: 1
Acquisition Error: None
JIT Mode: LuckTXM
```

---

## Troubleshooting

### Erreur: "lwmem_malloc returned nullptr"

**Cause:** Région de 512 MB épuisée (mode LuckTXM uniquement).

**Solutions:**
1. Réduire la taille des allocations JIT
2. Réutiliser les blocs compilés
3. Implémenter un cache avec eviction
4. Augmenter `EXECUTABLE_REGION_SIZE` (avec précaution)

---

### Erreur: "vm_remap failed" (retval != KERN_SUCCESS)

**Cause:** Permissions insuffisantes ou iOS trop ancien.

**Solutions:**
1. Vérifier iOS >= 14.0
2. Vérifier que JIT est acquis (`acquiredJit == YES`)
3. Sur simulateur, vérifier qu'il est en mode Debug
4. Vérifier les entitlements (`get-task-allow`, `dynamic-codesigning`)

---

### Crash lors de l'exécution du code JIT

**Cause:** Mauvais pointeur (rx vs rw).

**Diagnostic:**
```cpp
void* rx_ptr = CodeGen::AllocateExecutableMemory(size);
ptrdiff_t diff = CodeGen::AllocateWritableRegionAndGetDiff(rx_ptr, size);
void* rw_ptr = static_cast<uint8_t*>(rx_ptr) + diff;

NSLog(@"RX ptr: %p", rx_ptr);
NSLog(@"RW ptr: %p", rw_ptr);
NSLog(@"Diff: %td", diff);

// Vérifier que vous écrivez via rw_ptr
// Vérifier que vous exécutez via rx_ptr
```

**Solutions:**
- ✅ Toujours **écrire via rw_ptr**
- ✅ Toujours **exécuter via rx_ptr**
- ✅ Garder track du `diff` pour chaque allocation

---

### Crash avec "killed by TXM" sur iOS 26

**Cause:** Xcode debugger attaché ou code non signé correctement.

**Solutions:**
1. **Ne PAS lancer depuis Xcode** sur iOS 26+ avec TXM
2. Utiliser **StikDebug** ou sideload via **AltStore**
3. Vérifier que `brk #0x69` est bien exécuté

---

### JIT non acquis (acquiredJit == NO)

**Diagnostic:**
```objc
NSLog(@"Acquisition error: %@", [JitManager shared].acquisitionError);
```

**Causes possibles:**

| Erreur | Cause | Solution |
|--------|-------|----------|
| `nil` (pas d'erreur) | Debugger non attaché | Lancer via Xcode en debug ou PTrace |
| "Failed to spawn child process" | Entitlement manquant | Vérifier `platform-application` |
| "JIT cannot be enabled while running within Xcode on iOS 26" | Xcode + iOS 26 + TXM | Utiliser StikDebug ou AltStore |

---

### Problème de performance (lent)

**Diagnostic:**
1. Vérifier le mode JIT actif:
   ```objc
   if (@available(iOS 26, *)) {
     if ([JitManager shared].deviceHasTxm) {
       NSLog(@"Mode: LuckTXM (le plus rapide)");
     } else {
       NSLog(@"Mode: LuckNoTXM (moyen)");
     }
   } else {
     NSLog(@"Mode: Legacy (lent)");
   }
   ```

2. Vérifier le nombre de `mprotect()` calls (Legacy uniquement):
   ```cpp
   // Ajouter un compteur dans JITMemoryTracker
   static int g_mprotect_count = 0;
   g_mprotect_count++;
   NSLog(@"mprotect calls: %d", g_mprotect_count);
   ```

**Solutions:**
- Mode Legacy: Minimiser les appels à `JITPageWrite*`
- Mode LuckNoTXM: Réutiliser les miroirs RW si possible
- Mode LuckTXM: Déjà optimal

---

## Ressources et références

### Documentation Apple

- [Mach VM Interface](https://developer.apple.com/documentation/kernel/vm_remap)
- [Memory Management Programming Guide](https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/MemoryMgmt/Articles/MemoryMgmt.html)

### Projets similaires

- **Dolphin iOS** : Source de cette implémentation
  - [GitHub](https://github.com/oatmealdome/dolphin)

- **PojavLauncher** : Technique PTrace
  - [GitHub](https://github.com/PojavLauncherTeam/PojavLauncher_iOS)

- **StikDebug** : Debugger iOS pour JIT
  - [GitHub](https://github.com/StephenDev0/StikDebug)

### lwmem

- **Documentation** : [GitHub lwmem](https://github.com/MaJerle/lwmem)
- **API Reference** : Voir `deps/lwmem/docs/`

---

## Annexe : Comparaison des modes

### Performance

| Opération | Legacy | LuckNoTXM | LuckTXM |
|-----------|--------|-----------|---------|
| Allocation (première fois) | ⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| Écriture code | ⭐⭐ (mprotect) | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| Exécution code | ⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| Libération | ⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ |

### Complexité d'utilisation

| Aspect | Legacy | LuckNoTXM | LuckTXM |
|--------|--------|-----------|---------|
| Setup initial | Simple | Moyen | Complexe |
| Utilisation courante | RAII wrapper | RX/RW pointers | RX/RW pointers |
| Gestion mémoire | Automatique | Manuelle (diff) | Manuelle (diff) |

### Limites

| Mode | Limite principale |
|------|-------------------|
| **Legacy** | Performance (mprotect syscalls) |
| **LuckNoTXM** | Overhead par allocation (vm_remap) |
| **LuckTXM** | Taille fixe 512 MB |

---

## Conclusion

Vous disposez maintenant d'une méthodologie complète pour implémenter le système JIT avancé de Dolphin iOS dans Play! PS2 Emulator.

### Étapes suivantes recommandées

1. ✅ Installer lwmem
2. ✅ Créer tous les fichiers listés
3. ✅ Modifier CMakeLists.txt
4. ✅ Compiler et tester sur simulateur
5. ✅ Tester sur appareil réel iOS < 26
6. ✅ Tester sur appareil réel iOS 26+
7. ✅ Mesurer les performances avant/après

### Support

Si vous rencontrez des problèmes:
1. Consultez la section [Troubleshooting](#troubleshooting)
2. Vérifiez les logs avec la section Debug
3. Comparez avec l'implémentation Dolphin iOS de référence

---

**Bonne chance avec votre implémentation!** 🚀
