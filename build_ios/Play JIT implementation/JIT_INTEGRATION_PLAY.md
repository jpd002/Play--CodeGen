# Guide d'intégration JIT pour Play! (App iOS)

**Version:** 1.0
**Date:** 2026-01-17
**Cible:** Repository Play (application iOS principale)

---

## 📋 Table des matières

1. [Vue d'ensemble](#vue-densemble)
2. [Prérequis](#prérequis)
3. [Structure des fichiers](#structure-des-fichiers)
4. [Implémentation Objective-C/Swift](#implémentation-objective-cswift)
5. [Intégration dans l'app](#intégration-dans-lapp)
6. [Configuration Xcode](#configuration-xcode)
7. [Tests et validation](#tests-et-validation)
8. [Troubleshooting](#troubleshooting)

---

## Vue d'ensemble

Ce document décrit l'implémentation de la **couche haute** du système JIT pour l'application iOS **Play!**. Cette couche est responsable de :

✅ Acquisition du JIT (PTrace, AltServer, Debugger)
✅ Détection de TXM (Trusted Execution Monitor)
✅ Détection de la version iOS
✅ Configuration du module Play--CodeGen
✅ Interface utilisateur (messages d'erreur)

### Responsabilités de Play! (App)

✅ Détecter la version iOS (< 26 ou >= 26)
✅ Détecter la présence de TXM sur l'appareil
✅ Acquérir le JIT au démarrage
✅ Configurer Play--CodeGen avec le bon type JIT
✅ Afficher les erreurs à l'utilisateur

❌ Allocation mémoire bas niveau (géré par CodeGen)
❌ Gestion miroirs RW/RX (géré par CodeGen)

---

## Prérequis

### 1. Play--CodeGen configuré

Vous devez d'abord avoir complété l'implémentation de **Play--CodeGen** selon le document `JIT_IMPLEMENTATION_CODEGEN.md`.

**Vérification :**

```bash
# Dans Play--CodeGen
ls deps/lwmem/lwmem/src/include/lwmem/lwmem.h
ls include/MemoryUtil.h
ls src/MemoryUtil_iOS.cpp
# Tous doivent exister
```

### 2. Linking avec CodeGen

Votre app iOS doit linker avec la bibliothèque Play--CodeGen :

```ruby
# Podfile (si vous utilisez CocoaPods)
target 'Play' do
  use_frameworks!

  # Link vers Play--CodeGen
  pod 'PlayCodeGen', :path => '../Play--CodeGen'
end
```

**OU** dans Xcode :

```
Target Play → Build Phases → Link Binary With Libraries
→ Ajouter PlayCodeGen.framework
```

---

## Structure des fichiers

Créez cette arborescence dans votre projet **Play!** (app iOS) :

```
Play/
└── Source/
    └── iOS/
        └── JIT/
            ├── JitManager.h
            ├── JitManager.m
            ├── JitManager+Debugger.h
            ├── JitManager+Debugger.m
            ├── JitManager+PTrace.h
            ├── JitManager+PTrace.m
            ├── JitManager+AltServer.h          # Optionnel
            ├── JitManager+AltServer.m          # Optionnel
            └── JitAcquisitionService.swift
```

**Total :** 7-9 fichiers à créer

---

## Implémentation Objective-C/Swift

### 📄 `Source/iOS/JIT/JitManager.h`

Interface principale du gestionnaire JIT :

```objc
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface JitManager : NSObject

/// Indique si le JIT a été acquis avec succès
@property (readonly, assign) BOOL acquiredJit;

/// Message d'erreur si acquisition a échoué (nil si succès)
@property (nonatomic, nullable) NSString* acquisitionError;

/// Indique si l'appareil possède TXM (iOS 26+ uniquement)
@property (readonly, assign) BOOL deviceHasTxm;

/// Singleton
+ (JitManager*)shared;

/// Revérifie si le JIT est acquis (via debugger)
- (void)recheckIfJitIsAcquired;

@end

NS_ASSUME_NONNULL_END
```

---

### 📄 `Source/iOS/JIT/JitManager.m`

Implémentation principale :

```objc
#import "JitManager.h"
#import "JitManager+Debugger.h"

typedef NS_ENUM(NSInteger, PLAYJitType) {
  PLAYJitTypeDebugger,      // Détection via csops()
  PLAYJitTypeUnrestricted   // Simulateur (pas de restrictions)
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

    // Détecter TXM uniquement sur iOS 26+
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
    // Sur iOS 26 + TXM, bloquer Xcode (crash garanti)
    if (self.deviceHasTxm) {
      NSDictionary* environment = [[NSProcessInfo processInfo] environment];

      if ([environment objectForKey:@"XCODE"] != nil) {
        static dispatch_once_t onceToken;
        dispatch_once(&onceToken, ^{
          self.acquisitionError = @"JIT cannot be enabled while running within Xcode on iOS 26 with TXM. Use StikDebug or sideload via AltStore.";
        });
        return;
      }
    }

    // Vérifier si processus est débogué
    self.acquiredJit = [self checkIfProcessIsDebugged];

    // Warning si debugger attaché sur TXM (sauf StikDebug)
    if (self.deviceHasTxm && self.acquiredJit) {
      self.acquisitionError = @"⚠️ A debugger is attached. If not StikDebug, Play! will crash when emulation starts.";
    }
  } else if (_jitType == PLAYJitTypeUnrestricted) {
    // Simulateur : JIT toujours disponible
    self.acquiredJit = YES;
  }
}

@end
```

---

### 📄 `Source/iOS/JIT/JitManager+Debugger.h`

Extension pour détection debugger et TXM :

```objc
#import "JitManager.h"

@interface JitManager (Debugger)

/// Vérifie si le processus est marqué comme "debugged"
- (BOOL)checkIfProcessIsDebugged;

/// Vérifie si l'appareil possède TXM (iOS 26+)
- (BOOL)checkIfDeviceUsesTXM;

@end
```

---

### 📄 `Source/iOS/JIT/JitManager+Debugger.m`

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

// Helper : cherche fichier de longueur spécifique dans un dossier
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
  // Méthode basée sur StikDebug
  // Cherche : /System/Volumes/Preboot/<36 chars>/boot/<96 chars>/usr/standalone/firmware/FUD/Ap,TrustedExecutionMonitor.img4

  // Primary path
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

  // Fallback path
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

**Comment fonctionne la détection TXM :**

1. Cherche un dossier UUID de 36 caractères dans `/System/Volumes/Preboot/`
2. Cherche un dossier de 96 caractères dans `<UUID>/boot/`
3. Vérifie l'existence du fichier firmware TXM
4. Si trouvé → TXM présent

---

### 📄 `Source/iOS/JIT/JitManager+PTrace.h`

Extension pour acquisition via PTrace :

```objc
#import "JitManager.h"

/// Argument CLI utilisé pour lancer le processus enfant
extern const char* _Nonnull PLAYJitPTraceChildProcessArgument;

@interface JitManager (PTrace)

/// Vérifie si PTrace peut être utilisé (nécessite entitlement)
- (BOOL)checkCanAcquireJitByPTrace;

/// Exécute les tâches de démarrage PTrace (appelé par processus enfant)
- (void)runPTraceStartupTasks;

/// Acquiert le JIT via technique PTrace
- (void)acquireJitByPTrace;

@end
```

---

### 📄 `Source/iOS/JIT/JitManager+PTrace.m`

**Technique PTrace** : Spawne un processus enfant qui appelle `PT_TRACE_ME`, marquant le parent comme "debugged".

```objc
#import "JitManager+PTrace.h"
#import "JitManager+Debugger.h"

#import <spawn.h>

// API privées (nécessaires)
void* SecTaskCreateFromSelf(CFAllocatorRef allocator);
CFTypeRef SecTaskCopyValueForEntitlement(void* task, CFStringRef entitlement, CFErrorRef* _Nullable error);

#define PT_TRACE_ME 0
#define PT_DETACH 11
int ptrace(int request, pid_t pid, caddr_t caddr, int data);

extern char** environ;

const char* _Nonnull PLAYJitPTraceChildProcessArgument = "ptraceChild";

@implementation JitManager (PTrace)

- (BOOL)checkCanAcquireJitByPTrace {
  // PTrace nécessite l'entitlement "platform-application"
  // (disponible sur jailbreak ou sideload avec entitlements spéciaux)

  void* task = SecTaskCreateFromSelf(NULL);
  CFTypeRef entitlementValue = SecTaskCopyValueForEntitlement(task, CFSTR("platform-application"), NULL);

  if (entitlementValue == NULL) {
    CFRelease(task);
    return NO;
  }

  BOOL result = (entitlementValue == kCFBooleanTrue);

  CFRelease(entitlementValue);
  CFRelease(task);

  return result;
}

- (void)runPTraceStartupTasks {
  // Appelé par le processus enfant
  // PT_TRACE_ME marque le parent comme "debugged"
  ptrace(PT_TRACE_ME, 0, NULL, 0);
}

- (void)acquireJitByPTrace {
  if (![self checkCanAcquireJitByPTrace]) {
    self.acquisitionError = @"PTrace not available (missing platform-application entitlement)";
    return;
  }

  const char* executablePath = [[[NSBundle mainBundle] executablePath] UTF8String];
  const char* arguments[] = { executablePath, PLAYJitPTraceChildProcessArgument, NULL };

  pid_t childPid;
  int result = posix_spawnp(&childPid, executablePath, NULL, NULL, (char* const*)arguments, environ);

  if (result == 0) {
    // Attendre que l'enfant soit arrêté
    waitpid(childPid, NULL, WUNTRACED);

    // Détacher et tuer l'enfant
    ptrace(PT_DETACH, childPid, NULL, 0);
    kill(childPid, SIGTERM);
    wait(NULL);

    // Revérifier si JIT acquis
    [self recheckIfJitIsAcquired];

    if (self.acquiredJit) {
      NSLog(@"✅ JIT acquired via PTrace");
    }
  } else {
    self.acquisitionError = [NSString stringWithFormat:@"Failed to spawn PTrace child process (errno %d)", errno];
  }
}

@end
```

**Flux PTrace :**
1. Vérifie entitlement `platform-application`
2. Spawne processus enfant avec argument `"ptraceChild"`
3. Enfant appelle `PT_TRACE_ME`
4. Parent devient "debugged" → JIT acquis
5. Parent detach et kill l'enfant

---

### 📄 `Source/iOS/JIT/JitAcquisitionService.swift`

Service d'acquisition automatique au lancement :

```swift
import UIKit

/// Service responsable de l'acquisition automatique du JIT au démarrage
class JitAcquisitionService: UIResponder, UIApplicationDelegate {

  func application(
    _ application: UIApplication,
    didFinishLaunchingWithOptions launchOptions: [UIApplication.LaunchOptionsKey : Any]? = nil
  ) -> Bool {

    let manager = JitManager.shared()

    // 1. Vérifier si JIT déjà acquis (ex: debugger attaché)
    manager.recheckIfJitIsAcquired()

    // 2. Si pas encore acquis, tenter PTrace
    if !manager.acquiredJit {
      manager.acquireJitByPTrace()

      #if NONJAILBROKEN
      // 3. Si toujours pas acquis, tenter AltServer (optionnel)
      if !manager.acquiredJit {
        // manager.acquireJitByAltServer()
      }
      #endif
    }

    // 4. Logger le résultat
    if manager.acquiredJit {
      NSLog("✅ JIT acquired successfully")
    } else {
      NSLog("❌ JIT not acquired: \(manager.acquisitionError ?? "Unknown error")")
    }

    return true
  }
}
```

---

## Intégration dans l'app

### 1. Modification du `main.m`

Modifiez le point d'entrée de votre app :

```objc
#import <UIKit/UIKit.h>
#import "JitAcquisitionService.h"
#import "JitManager+PTrace.h"

int main(int argc, char* argv[]) {
  @autoreleasepool {
    // Si lancé en mode PTrace child, exécuter puis exit
    if (argc > 1 && strcmp(argv[1], PLAYJitPTraceChildProcessArgument) == 0) {
      [[JitManager shared] runPTraceStartupTasks];
      return 0;
    }

    // Lancement normal de l'app
    return UIApplicationMain(
      argc,
      argv,
      nil,
      NSStringFromClass([JitAcquisitionService class])
    );
  }
}
```

**Important :**
- Le `if (argc > 1...)` détecte si c'est le processus enfant PTrace
- Si oui, exécute `PT_TRACE_ME` puis exit
- Sinon, lance l'app normalement

---

### 2. Configuration de Play--CodeGen

Dans votre code d'initialisation de l'émulateur (ex: `AppDelegate` ou `EmulatorViewController`) :

```objc
#import "JitManager.h"
#import <PlayCodeGen/MemoryUtil.h>  // Depuis Play--CodeGen

- (void)configureJIT {
  JitManager* jitManager = [JitManager shared];
  [jitManager recheckIfJitIsAcquired];

  if (!jitManager.acquiredJit) {
    // Afficher alerte à l'utilisateur
    [self showJITErrorAlert:jitManager.acquisitionError];
    return;
  }

  // Configurer Play--CodeGen selon iOS version et TXM
  if (@available(iOS 26, *)) {
    if (jitManager.deviceHasTxm) {
      // iOS 26+ avec TXM : mode le plus performant
      NSLog(@"Configuring JIT: LuckTXM mode");
      CodeGen::SetJitType(CodeGen::JitType::LuckTXM);

      // IMPORTANT: Pré-allouer la région de 512 MB
      CodeGen::AllocateExecutableMemoryRegion();
    } else {
      // iOS 26+ sans TXM
      NSLog(@"Configuring JIT: LuckNoTXM mode");
      CodeGen::SetJitType(CodeGen::JitType::LuckNoTXM);
    }
  } else {
    // iOS < 26
    NSLog(@"Configuring JIT: Legacy mode");
    CodeGen::SetJitType(CodeGen::JitType::Legacy);
  }

  NSLog(@"✅ JIT configured successfully");
}

- (void)showJITErrorAlert:(NSString*)errorMessage {
  UIAlertController* alert = [UIAlertController
    alertControllerWithTitle:@"JIT Not Available"
    message:errorMessage ?: @"Play! requires JIT to run. Please sideload via AltStore or use a debugger."
    preferredStyle:UIAlertControllerStyleAlert
  ];

  [alert addAction:[UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:nil]];

  [self.window.rootViewController presentViewController:alert animated:YES completion:nil];
}
```

---

### 3. Exemple d'utilisation dans le recompiler

Une fois JIT configuré, votre recompiler peut utiliser l'API CodeGen :

```cpp
#include <PlayCodeGen/MemoryUtil.h>

class PS2Recompiler {
public:
  void* CompileBlock(uint32_t ps2Address, const uint8_t* ps2Code, size_t codeSize) {
    // 1. Allouer mémoire exécutable
    void* rx_ptr = CodeGen::AllocateExecutableMemory(codeSize);
    if (!rx_ptr) return nullptr;

    // 2. Obtenir pointeur writable
    ptrdiff_t diff = CodeGen::AllocateWritableRegionAndGetDiff(rx_ptr, codeSize);
    void* rw_ptr = static_cast<uint8_t*>(rx_ptr) + diff;

    // 3. Compiler code PS2 → ARM64 via pointeur RW
    size_t compiledSize = EmitARMCode(rw_ptr, ps2Code, codeSize);

    // 4. Libérer miroir RW (ignoré sur LuckTXM, nécessaire sur LuckNoTXM)
    CodeGen::FreeWritableRegion(rx_ptr, codeSize, diff);

    // 5. Retourner pointeur exécutable
    m_compiledBlocks[ps2Address] = rx_ptr;
    return rx_ptr;
  }

  void ExecuteBlock(uint32_t ps2Address) {
    void* code = m_compiledBlocks[ps2Address];
    if (!code) return;

    // Exécuter via pointeur RX
    typedef void (*JitFunc)();
    JitFunc func = reinterpret_cast<JitFunc>(code);
    func();
  }

private:
  std::unordered_map<uint32_t, void*> m_compiledBlocks;
};
```

---

## Configuration Xcode

### 1. Build Settings

Ajoutez dans **Target Play → Build Settings** :

| Setting | Value |
|---------|-------|
| **Other Linker Flags** | `-framework Foundation -framework UIKit` |
| **Header Search Paths** | `$(PROJECT_DIR)/../Play--CodeGen/include` |
| **Library Search Paths** | `$(PROJECT_DIR)/../Play--CodeGen/build` |

### 2. Entitlements (Info.plist ou .entitlements)

Pour PTrace et JIT :

```xml
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <!-- JIT code signing -->
  <key>com.apple.security.cs.allow-jit</key>
  <true/>

  <!-- Debugging (pour PTrace) -->
  <key>com.apple.security.get-task-allow</key>
  <true/>

  <!-- Dynamic code signing -->
  <key>com.apple.security.cs.allow-unsigned-executable-memory</key>
  <true/>
</dict>
</plist>
```

**Note :** Ces entitlements nécessitent sideload via AltStore, pas disponibles sur App Store.

---

### 3. Bridging Header (si Swift ↔ Objective-C)

Créez `Play-Bridging-Header.h` :

```objc
#import "JitManager.h"
#import "JitManager+PTrace.h"
#import "JitManager+Debugger.h"
```

Dans Build Settings :
```
Objective-C Bridging Header: $(PROJECT_DIR)/Play-Bridging-Header.h
```

---

## Tests et validation

### Test 1 : Logs de démarrage

Au lancement de l'app, vous devriez voir :

```
=== JIT Configuration ===
iOS Version: 26.0
Device has TXM: YES
JIT Acquired: YES
JIT Mode: LuckTXM
✅ JIT configured successfully
```

### Test 2 : Vérification programmatique

Ajoutez un écran de debug dans votre app :

```swift
import UIKit

class DebugViewController: UIViewController {
  override func viewDidLoad() {
    super.viewDidLoad()

    let manager = JitManager.shared()
    let info = """
    === JIT Status ===
    iOS Version: \(UIDevice.current.systemVersion)
    JIT Acquired: \(manager.acquiredJit ? "YES" : "NO")
    Device has TXM: \(manager.deviceHasTxm ? "YES" : "NO")
    Error: \(manager.acquisitionError ?? "None")

    JIT Mode: \(getJITModeName())
    """

    print(info)
  }

  func getJITModeName() -> String {
    if #available(iOS 26, *) {
      return JitManager.shared().deviceHasTxm ? "LuckTXM" : "LuckNoTXM"
    } else {
      return "Legacy"
    }
  }
}
```

### Test 3 : Compilation et exécution

Test minimal de JIT :

```objc
- (void)testJIT {
  const size_t size = 4096;

  void* rx = CodeGen::AllocateExecutableMemory(size);
  assert(rx != nullptr);

  ptrdiff_t diff = CodeGen::AllocateWritableRegionAndGetDiff(rx, size);
  void* rw = (uint8_t*)rx + diff;

  // ARM64: ret instruction
  ((uint32_t*)rw)[0] = 0xD65F03C0;

  typedef void (*Func)();
  Func f = (Func)rx;
  f(); // Doit retourner sans crash

  CodeGen::FreeWritableRegion(rx, size, diff);
  CodeGen::FreeExecutableMemory(rx, size);

  NSLog(@"✅ JIT test passed");
}
```

---

## Troubleshooting

### Problème 1 : JIT non acquis

**Symptôme :**
```
❌ JIT not acquired: nil
```

**Diagnostic :**
```objc
NSLog(@"Acquisition error: %@", [JitManager shared].acquisitionError);
```

**Solutions :**

| Erreur | Cause | Solution |
|--------|-------|----------|
| `nil` | Debugger non attaché | Lancer via Xcode debug ou PTrace |
| "PTrace not available" | Entitlement manquant | Vérifier `.entitlements` |
| "JIT cannot be enabled while running within Xcode on iOS 26" | Xcode + iOS 26 + TXM | Utiliser StikDebug ou AltStore |

---

### Problème 2 : Crash au lancement émulation

**Symptôme :**
```
Thread 1: EXC_BAD_ACCESS (code=2, address=0x...)
```

**Causes possibles :**

1. **TXM + debugger non-StikDebug :**
   ```objc
   if (manager.deviceHasTxm && manager.acquiredJit) {
     NSLog(@"⚠️ Warning: %@", manager.acquisitionError);
     // Afficher warning utilisateur
   }
   ```

2. **Région 512 MB non pré-allouée (LuckTXM) :**
   ```objc
   if (type == CodeGen::JitType::LuckTXM) {
     CodeGen::AllocateExecutableMemoryRegion(); // IMPORTANT
   }
   ```

3. **Mauvais pointeur (RX vs RW) :**
   ```cpp
   // ❌ FAUX : écrire via RX
   memcpy(rx_ptr, code, size);

   // ✅ BON : écrire via RW
   void* rw_ptr = (uint8_t*)rx_ptr + diff;
   memcpy(rw_ptr, code, size);
   ```

---

### Problème 3 : "lwmem_malloc returned nullptr"

**Cause :** Région de 512 MB épuisée (mode LuckTXM).

**Solutions :**

1. **Réduire allocations :**
   ```cpp
   // Éviter d'allouer plus que nécessaire
   void* code = CodeGen::AllocateExecutableMemory(actualSize);
   ```

2. **Implémenter cache avec eviction :**
   ```cpp
   if (m_compiledBlocks.size() > MAX_BLOCKS) {
     // Libérer le bloc le plus ancien
     auto oldest = m_compiledBlocks.begin();
     CodeGen::FreeExecutableMemory(oldest->second, ...);
     m_compiledBlocks.erase(oldest);
   }
   ```

3. **Augmenter taille région (avec précaution) :**
   ```cpp
   // Dans MemoryUtil_iOS_LuckTXM.cpp
   constexpr size_t EXECUTABLE_REGION_SIZE = 1073741824; // 1 GB
   ```

---

### Problème 4 : Performance dégradée

**Diagnostic :**

1. Vérifier le mode JIT actif :
   ```objc
   if (@available(iOS 26, *)) {
     if ([JitManager shared].deviceHasTxm) {
       NSLog(@"Mode: LuckTXM (optimal)");
     } else {
       NSLog(@"Mode: LuckNoTXM (bon)");
     }
   } else {
     NSLog(@"Mode: Legacy (moins performant)");
   }
   ```

2. Mode Legacy : minimiser les toggles W^X :
   ```cpp
   // ❌ INEFFICACE
   for (int i = 0; i < 100; i++) {
     CodeGen::JITPageWriteEnableExecuteDisable(code);
     WriteOneInstruction(code);
     CodeGen::JITPageWriteDisableExecuteEnable(code);
   }

   // ✅ EFFICACE
   {
     CodeGen::ScopedJITPageWriteAndNoExecute guard(code);
     for (int i = 0; i < 100; i++) {
       WriteOneInstruction(code);
     }
   }
   ```

---

## Checklist de validation

- [ ] Tous les fichiers créés (7-9 fichiers)
- [ ] `main.m` modifié pour PTrace child
- [ ] Entitlements configurés
- [ ] Build Settings corrects
- [ ] Test sur simulateur (mode Unrestricted)
- [ ] Test sur device iOS < 26 (mode Legacy)
- [ ] Test sur device iOS 26+ sans TXM (mode LuckNoTXM)
- [ ] Test sur device iOS 26+ avec TXM (mode LuckTXM)
- [ ] Logs de debug affichent le bon mode
- [ ] Émulation fonctionne sans crash

---

## Ressources

### Projets de référence

- **Dolphin iOS** : Implémentation source
  - [GitHub](https://github.com/oatmealdome/dolphin)

- **StikDebug** : Debugger iOS pour JIT
  - [GitHub](https://github.com/StephenDev0/StikDebug)

- **PojavLauncher** : Technique PTrace
  - [GitHub](https://github.com/PojavLauncherTeam/PojavLauncher_iOS)

### Documentation Apple

- [Code Signing Entitlements](https://developer.apple.com/documentation/bundleresources/entitlements)
- [Process and Thread Programming Guide](https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/Multithreading/Introduction/Introduction.html)

---

## Conclusion

Vous disposez maintenant de tous les éléments pour intégrer le système JIT dans l'application **Play!** iOS.

### Workflow complet

1. ✅ Implémenter Play--CodeGen (voir `JIT_IMPLEMENTATION_CODEGEN.md`)
2. ✅ Créer les fichiers JitManager dans Play! (ce document)
3. ✅ Configurer Xcode et entitlements
4. ✅ Modifier `main.m` pour PTrace
5. ✅ Initialiser JIT au démarrage
6. ✅ Configurer Play--CodeGen selon mode détecté
7. ✅ Tester sur device réel

**Bonne chance avec votre intégration!** 🎮🚀
